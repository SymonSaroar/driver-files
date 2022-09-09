#define WD_DRIVER_NAME_CHANGE

using System;
using Jungo.wdapi_dotnet;
using static Jungo.wdapi_dotnet.wdc_lib_decl;
using static Jungo.wdapi_dotnet.windrvr_decl;
using static Jungo.wdapi_dotnet.wdc_lib_consts;
using static Jungo.wdapi_dotnet.windrvr_consts;
using static Jungo.wdapi_dotnet.WD_ERROR_CODES;
using static Jungo.wdapi_dotnet.wdc_defs_macros;
using System.Runtime.InteropServices;

using DWORD = System.UInt32;

namespace Jungo.diag_lib
{
    public class pci_lib
    {
        private const string PCI_DEFAULT_LICENSE_STRING = "12345abcde1234.license";
        private const string WD_PROD_NAME = "windrvr" + windrvr_decl.WD_VERSION;
        private const string WD_DEFAULT_DRIVER_NAME_BASE = WD_PROD_NAME;
        private const string PCI_DEFAULT_DRIVER_NAME = WD_DEFAULT_DRIVER_NAME_BASE;

        /* User interrupt handler, located in xxx_diag.cs */
        public delegate void DiagIntHandler(object ctx);
        private static DiagIntHandler m_userIntHandler;

        public delegate void DiagEventHandler(object ctx);
        private static DiagEventHandler m_userEventHandler;

        public static void PCI_LibInit()
        {
            DWORD dwStatus;

#if WD_DRIVER_NAME_CHANGE
            /* Set the driver name */
            if (String.IsNullOrEmpty(WD_DriverName(PCI_DEFAULT_DRIVER_NAME)))
            {
                throw new WinDriverException(
                    WD_SYSTEM_INTERNAL_ERROR,
                    "Failed to set the driver name for WDC library");
            }
#endif

            dwStatus = WDC_SetDebugOptions(WDC_DBG_DEFAULT, null);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException((WD_ERROR_CODES)dwStatus,
                    "Failed to initialize debug options for WDC library");
            }

            /* Open a handle to the driver and initialize the WDC library */
            dwStatus = WDC_DriverOpen(
                WDC_DRV_OPEN_DEFAULT, PCI_DEFAULT_LICENSE_STRING);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException((WD_ERROR_CODES)dwStatus,
                    "Failed to initialize the WDC library");
            }
        }

        /* Check whether a given device contains an item of the specified type */
        static bool IsItemExists(WDC_DEVICE wdcDevice, ITEM_TYPE itemType)
        {
            bool isExists = false;

            foreach (WD_ITEMS item in wdcDevice.cardReg.Card.Item)
            {
                if ((ITEM_TYPE)item.item == itemType)
                {
                    isExists = true;
                    break;
                }
            }

            return isExists;
        }

        public static WDC_PCI_SCAN_RESULT DeviceFind(DWORD dwVendorId, DWORD dwDeviceId)
        {
            DWORD dwStatus;
            WDC_PCI_SCAN_RESULT scanResult = new();

            dwStatus = WDC_PciScanDevices(dwVendorId, dwDeviceId,
                ref scanResult);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException((WD_ERROR_CODES)dwStatus,
                    "DeviceFind: Failed scanning the PCI bus");
            }

            DWORD dwNumDevices = scanResult.dwNumDevices;
            if (dwNumDevices == 0)
            {
                throw new WinDriverException(
                    WD_DEVICE_NOT_FOUND, String.Format(
                    "No matching PCI device was found for search criteria " +
                    "(Vendor ID 0x{0:X}, Device ID 0x{1:X})", dwVendorId,
                    dwDeviceId));
            }

            return scanResult;
        }

        public static WDC_DEVICE PCI_DeviceOpen(WD_PCI_SLOT slot, string kpName = "")
        {
            DWORD dwStatus;
            IntPtr hDev = IntPtr.Zero;
            IntPtr pDevCtx = IntPtr.Zero;
            WDC_DEVICE wdcDevice;
            WD_PCI_CARD_INFO deviceInfo = new()
            {
                pciSlot = slot
            };

            dwStatus = WDC_PciGetDeviceInfo(ref deviceInfo);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException((WD_ERROR_CODES)dwStatus,
                    "DeviceOpen: Failed retrieving the device's resources");
            }

            dwStatus = WDC_PciDeviceOpen(ref hDev,
                ref deviceInfo, pDevCtx);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "Failed opening a WDC device handle.");
            }

            if (!string.IsNullOrEmpty(kpName))
            {
                /* Open a handle to a Kernel PlugIn driver */
                WDC_KernelPlugInOpen(hDev, kpName, IntPtr.Zero);
            }

            /* Return handle to the new device */
            WDC_Trace(string.Format("PCI_DeviceOpen: Opened a PCI device (handle 0x{0:x}){1}" +
                "Device is {2} using a Kernel PlugIn driver {3}", hDev,
                Environment.NewLine, (WDC_IS_KP(hDev)) ? "" : "not", kpName));

            wdcDevice = WDC_DEVICE.hDevToWdcDevice(hDev);

            return wdcDevice;
        }

        public static void DeviceClose(WDC_DEVICE wdcDevice)
        {
            DWORD dwStatus;

            WDC_Trace($"PCI_DeviceClose: Entered. Device handle 0x{wdcDevice.hDev:x}");

            if (wdcDevice.hDev == IntPtr.Zero)
            {
                throw new WinDriverException(WD_INVALID_PARAMETER,
                    "PCI_DeviceClose: Error - NULL device object");
            }

            PCI_IntDisable(wdcDevice);

            // Close the device handle
            dwStatus = WDC_PciDeviceClose(wdcDevice.hDev);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException((WD_ERROR_CODES)dwStatus,
                    "Failed closing a WDC device handle (0x{0:x})");
            }
        }

        private static void ValidateDevice(IntPtr hDev)
        {
            if (hDev == IntPtr.Zero)
            {
                throw new WinDriverException(WD_INVALID_PARAMETER,
                    "Null Device Handler");
            }
        }

        private static void PCI_IntHandler(IntPtr pCtx)
        {
            m_userIntHandler?.Invoke(pCtx);
        }

        private static void CreateIntTransCmds(out WD_TRANSFER[]
            pIntTransCmds, out DWORD dwNumCmds)
        {
            /* Define the number of interrupt transfer commands to use */
            DWORD NUM_TRANS_CMDS = 0;
            pIntTransCmds = new WD_TRANSFER[NUM_TRANS_CMDS];

            /*
               TODO: Your hardware has level sensitive interrupts, which must be
                     acknowledged in the kernel immediately when they are received.
                     Since the information for acknowledging the interrupts is
                     hardware-specific, YOU MUST ADD CODE to read/write the relevant
                     register(s) in order to correctly acknowledge the interrupts
                     on your device, as dictated by your hardware's specifications.
                     When adding transfer commands, be sure to also modify the
                     definition of NUM_TRANS_CMDS (above) accordingly.

               *************************************************************************
               * NOTE: If you attempt to use this code without first modifying it in   *
               *       order to correctly acknowledge your device's interrupts, as     *
               *       explained above, the OS will HANG when an interrupt occurs!     *
               *************************************************************************
            */
            dwNumCmds = NUM_TRANS_CMDS;
        }

        public static bool PCI_EventIsRegistered(WDC_DEVICE wdcDevice)
        {
            ValidateDevice(wdcDevice.hDev);

            return WDC_EventIsRegistered(wdcDevice.hDev);
        }

        static void PCI_EventHandler(IntPtr pEvent, IntPtr pData)
        {
            WDC_Trace($"PCI_EventHandler entered, pData [0x{pData:x}]");

            m_userEventHandler?.Invoke(pData);
        }

        public static void PCI_EventRegister(WDC_DEVICE wdcDevice,
            DiagEventHandler diagEventHandler)
        {
            DWORD dwStatus;
            DWORD dwActions = WD_ACTIONS_ALL;

            WDC_Trace($"PCI_EventRegister entered. Device handle 0x{wdcDevice.hDev:x}");

            ValidateDevice(wdcDevice.hDev);

            m_userEventHandler = diagEventHandler;

            /* Check whether the event is already registered */
            if (WDC_EventIsRegistered(wdcDevice.hDev))
            {
                throw new WinDriverException(WD_OPERATION_ALREADY_DONE,
                    "Events are already registered");
            }

            /* Register the event */
            dwStatus = WDC_EventRegister(wdcDevice.hDev, dwActions, PCI_EventHandler,
                wdcDevice.hDev, WDC_IS_KP(wdcDevice.hDev));
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "Failed to register events");
            }

            WDC_Trace("Events registered");
        }

        /* Unregister a plug-and-play or power management event */
        public static void PCI_EventUnregister(WDC_DEVICE wdcDevice)
        {
            DWORD dwStatus;

            WDC_Trace($"PCI_EventUnregister entered. Device handle 0x{wdcDevice.hDev:x}");

            ValidateDevice(wdcDevice.hDev);

            /* Check whether the event is currently registered */
            if (!WDC_EventIsRegistered(wdcDevice.hDev))
            {
                throw new WinDriverException(WD_OPERATION_ALREADY_DONE,
                    "Cannot unregister events - no events currently registered");
            }

            /* Unregister the event */
            dwStatus = WDC_EventUnregister(wdcDevice.hDev);

            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "Failed to unregister events");
            }
        }

        public static void PCI_IntEnable(WDC_DEVICE wdcDevice,
            DiagIntHandler diagIntHandler, IntPtr pData)
        {
            DWORD dwStatus, dwNumCmds = 0;
            WD_TRANSFER[] pIntTransCmds = null;

            WDC_Trace($"PCI_IntEnable entered. Device handle 0x{wdcDevice.hDev:x}");

            ValidateDevice(wdcDevice.hDev);

            m_userIntHandler = diagIntHandler;

            /* Verify that the device has an interrupt item */
            if (!IsItemExists(wdcDevice, ITEM_TYPE.ITEM_INTERRUPT))
            {
                throw new WinDriverException(
                    WD_OPERATION_FAILED, "Device doesn't have any interrupts");
            }

            if (WDC_IntIsEnabled(wdcDevice.hDev))
            {
                throw new WinDriverException(
                    WD_OPERATION_ALREADY_DONE, "Interrupts are already enabled ...");
            }

            if (!WDC_IS_KP(wdcDevice))
                CreateIntTransCmds(out pIntTransCmds, out dwNumCmds);

            dwStatus = WDC_IntEnable(wdcDevice, pIntTransCmds,
                    dwNumCmds, (DWORD)WD_INTERRUPT_OPTIONS.INTERRUPT_CMD_COPY,
                    PCI_IntHandler, pData, WDC_IS_KP(wdcDevice));
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "Failed enabling interrupts");
            }

            /* TODO: You can add code here to write to the device in order
                     to physically enable the hardware interrupts */
        }

        public static void PCI_IntDisable(WDC_DEVICE wdcDevice)
        {
            DWORD dwStatus;

            WDC_Trace($"PCI_IntDisable entered. Device handle 0x{wdcDevice.hDev:x}");

            ValidateDevice(wdcDevice.hDev);

            if (!WDC_IntIsEnabled(wdcDevice.hDev))
            {
                throw new WinDriverException(
                    WD_OPERATION_ALREADY_DONE,
                    "Interrupts are already disabled ...");
            }

            /* TODO: You can add code here to write to the device in order to
                     physically disable the hardware interrupts. */

            /* Disable interrupts */
            dwStatus = WDC_IntDisable(wdcDevice);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "Failed disabling interrupts.");
            }
        }

        public static void PCI_DmaAllocContig(IntPtr hDev, out IntPtr pBuf,
            DWORD dwOptions, DWORD dwSize, out IntPtr pDma)
        {
            DWORD dwStatus;

            pDma = new();
            pBuf = new();

            dwStatus = WDC_DMAContigBufLock(hDev, ref pBuf,
                dwOptions, dwSize, ref pDma);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    $"Failed allocating contiguous memory. size [{dwSize}]");
            }
        }

        public static void PCI_DmaAllocSg(IntPtr hDev, out IntPtr pBuf,
            DWORD dwSize, out IntPtr pDma)
        {
            DWORD dwStatus, dwOptions = 0;

            pDma = new();
            pBuf = Marshal.AllocHGlobal((int)dwSize);

            dwStatus = WDC_DMASGBufLock(hDev, pBuf, dwOptions,
                dwSize, ref pDma);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                Marshal.FreeHGlobal(pBuf);
                throw new WinDriverException(dwStatus,
                     $"Failed allocating sg. size [{dwSize}]");
            }
        }

        public static void PCI_DmaAllocReserved(IntPtr hDev, UInt64 qwAddress,
            out IntPtr pBuf, DWORD dwSize, out IntPtr pDma)
        {
            DWORD dwStatus, dwOptions = 0;

            pDma = new();
            pBuf = new();

            dwStatus = WDC_DMAReservedBufLock(hDev, qwAddress, ref pBuf,
                dwOptions, dwSize, ref pDma);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                $"Failed claiming reserved memory. size [{dwSize}]");
            }
        }

        public static void PCI_DmaFree(IntPtr pDma, IntPtr pBuf)
        {
            DWORD dwStatus;
            WD_DMA dma;
            bool isSg;

            if (pDma == IntPtr.Zero)
                return;

            dma = Marshal.PtrToStructure<WD_DMA>(pDma);
            isSg =
               (dma.dwOptions & (DWORD)WD_DMA_OPTIONS.DMA_KERNEL_BUFFER_ALLOC) == 0;

            dwStatus = WDC_DMABufUnlock(pDma);
            if (dwStatus != (DWORD)WD_STATUS_SUCCESS)
            {
                throw new WinDriverException(dwStatus,
                    "Failed trying to free DMA memory");
            }

            if (isSg)
                Marshal.FreeHGlobal(pBuf);
        }
    }
}


