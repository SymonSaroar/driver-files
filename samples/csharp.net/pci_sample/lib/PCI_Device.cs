/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

// Note: This code sample is provided AS-IS and as a guiding sample only.

using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Data;

using Jungo.wdapi_dotnet;
using wdc_err = Jungo.wdapi_dotnet.WD_ERROR_CODES;
using item_types = Jungo.wdapi_dotnet.ITEM_TYPE;
using UINT64 = System.UInt64;
using UINT32 = System.UInt32;
using DWORD = System.UInt32;
using WORD = System.UInt16;
using BYTE = System.Byte;
using BOOL = System.Boolean;
using WDC_DEVICE_HANDLE = System.IntPtr;
using HANDLE = System.IntPtr;
using System.Linq.Expressions;
using System.Collections.Generic;

namespace Jungo.pci_lib
{
    /* Kernel PlugIn Device Address Description struct */
    public struct PCI_DEV_ADDR_DESC
    {
        public DWORD dwNumAddrSpaces; /* Total number of device address
                                         spaces */
        public IntPtr AddrDesc;       /* Array of device address spaces
                                         information */
    };

    /* Kernel PlugIn version information struct */
    public struct KP_PCI_VERSION
    {
        public DWORD dwVer;
        public string cVer; //max size = 100
    };

    /* Kernel PlugIn messages status */
    public enum KP_PCI_STATUS
    {
        KP_PCI_STATUS_OK          = 0x1,
        KP_PCI_STATUS_MSG_NO_IMPL = 0x1000,
        KP_PCI_STATUS_FAIL        = 0x1001,
    };

    public enum KP_PCI_MSG
    {
        KP_PCI_MSG_VERSION = 1,
    };

    /* PCI diagnostics plug-and-play and power management events handler
     * function type */
    public delegate void USER_EVENT_CALLBACK(ref WD_EVENT pEvent,
            PCI_Device dev);
    /* PCI diagnostics interrupt handler function type */
    public delegate void USER_INTERRUPT_CALLBACK(PCI_Device device);

    public class PCI_Device
    {
        private WDC_DEVICE m_wdcDevice = new WDC_DEVICE();
#if !NET5_0
        protected MarshalWdcDevice m_wdcDeviceMarshaler;
#endif
        private USER_EVENT_CALLBACK m_userEventHandler;
        private USER_INTERRUPT_CALLBACK m_userIntHandler;
        private EVENT_HANDLER_DOTNET m_eventHandler;
        private INT_HANDLER m_intHandler;
        protected string m_sDeviceLongDesc;
        protected string m_sDeviceShortDesc;
        private PCI_Regs m_regs;
        /* Kernel PlugIn driver name (should be no more than 8 characters) */
        private string m_sKP_PCI_DRIVER_NAME = "KP_PCI";

#region " constructors "
        /* constructors & destructors */
        internal protected PCI_Device(WD_PCI_SLOT slot): this(0, 0, slot){}

        internal protected PCI_Device(DWORD dwVendorId, DWORD dwDeviceId,
            WD_PCI_SLOT slot)
        {
            m_wdcDevice = new WDC_DEVICE();
            m_wdcDevice.id.dwVendorId = dwVendorId;
            m_wdcDevice.id.dwDeviceId = dwDeviceId;
            m_wdcDevice.slot = slot;
#if !NET5_0
            m_wdcDeviceMarshaler = new MarshalWdcDevice();
#endif
            m_eventHandler = new EVENT_HANDLER_DOTNET(EventHandler);
            m_intHandler = new INT_HANDLER(IntHandler);
            m_regs = new PCI_Regs();
            SetDescription();
        }

        public PCI_Device(ref PCI_Device dev)
        {
            m_wdcDevice = dev.m_wdcDevice;
            m_wdcDevice.id.dwVendorId = dev.wdcDevice.id.dwVendorId;
            m_wdcDevice.id.dwDeviceId = dev.wdcDevice.id.dwDeviceId;
            m_wdcDevice.slot = dev.slot;
#if !NET5_0
            m_wdcDeviceMarshaler = new MarshalWdcDevice();
#endif
            /* The following members are always created
                upon construction and cannot be copied */
            m_eventHandler = new EVENT_HANDLER_DOTNET(EventHandler);
            m_intHandler = new INT_HANDLER(IntHandler);
            m_regs = new PCI_Regs();
            SetDescription();
        }

        public void Dispose()
        {
            Close();
        }
#endregion

#region " properties "
        /*******************
        *  properties       *
        *******************/

        public IntPtr Handle
        {
            get
            {
                if (m_wdcDevice == null)
                    return IntPtr.Zero;
                return m_wdcDevice.hDev;
            }
            set
            {
                m_wdcDevice.hDev = value;
            }
        }

        protected WDC_DEVICE wdcDevice
        {
            get
            {
                return m_wdcDevice;
            }
            set
            {
                m_wdcDevice = value;
            }
        }

        public WD_INTERRUPT wdcInterrupt
        {
            get
            {
                return wdcDevice.Int;
            }
            set
            {
                wdcDevice.Int = value;
            }
        }

        public WD_PCI_ID id
        {
            get
            {
                return m_wdcDevice.id;
            }
            set
            {
                m_wdcDevice.id = value;
            }
        }

        public WD_PCI_SLOT slot
        {
            get
            {
                return m_wdcDevice.slot;
            }
            set
            {
                m_wdcDevice.slot = value;
            }
        }

        public DWORD NumAddrSpaces
        {
            get
            {
                return m_wdcDevice.dwNumAddrSpaces;
            }
            set
            {
                m_wdcDevice.dwNumAddrSpaces = value;
            }
        }

        public WDC_ADDR_DESC[] AddrDesc
        {
            get
            {
                return m_wdcDevice.pAddrDesc;
            }
            set
            {
                m_wdcDevice.pAddrDesc = value;
            }
        }

        public PCI_Regs Regs
        {
            get
            {
                return m_regs;
            }
        }
#endregion

public virtual DWORD CallWinDriverAPIFunc(Action func)
        {
            func();
            return (DWORD)wdc_err.WD_STATUS_SUCCESS;
        }

        public virtual T CallWinDriverAPIFunc<T>(Expression<Func<T>> func)
        {
            object res = null;

            MethodCallExpression methodCall = func.Body as MethodCallExpression;
            if (methodCall != null)
                res = func.Compile().Invoke();

            return (T)res;
        }

        #region " utilities "
        /*******************
        *  utilities       *
        *******************/

        /* public methods */
        public string[] AddrDescToString(bool bMemOnly)
        {
            string[] sAddr = new string[AddrDesc.Length];
            for (int i = 0; i < sAddr.Length; ++i)
            {
                sAddr[i] = "BAR " + AddrDesc[i].dwAddrSpace.ToString() +
                    (wdc_defs_macros.WDC_ADDR_IS_MEM(AddrDesc[i]) ? " Memory ":
                    " I/O ");

                if (CallWinDriverAPIFunc(() =>
                        wdc_lib_decl.WDC_AddrSpaceIsActive(
                        Handle, AddrDesc[i].dwAddrSpace)) == true)
                {
                    WD_ITEMS item =
                        m_wdcDevice.cardReg.Card.Item[AddrDesc[i].dwItemIndex];
                    UINT64 pAddr =
                        wdc_defs_macros.WDC_ADDR_IS_MEM(AddrDesc[i]) ?
                            item.I.Mem.pPhysicalAddr : item.I.IO.pAddr;

                    sAddr[i] += pAddr.ToString("X") + " - " +
                        (pAddr + AddrDesc[i].qwBytes - 1).ToString("X") +
                        " (" + AddrDesc[i].qwBytes.ToString("X") + " bytes)";
                }
                else
                {
                    sAddr[i] += "Inactive address space";
                }
            }
            return sAddr;
        }

        public string ToString(BOOL bLong)
        {
            return (bLong) ? m_sDeviceLongDesc: m_sDeviceShortDesc;
        }

        public bool IsMySlot(ref WD_PCI_SLOT slot)
        {
            if (m_wdcDevice.slot.dwDomain == slot.dwDomain &&
                m_wdcDevice.slot.dwBus == slot.dwBus &&
                m_wdcDevice.slot.dwSlot == slot.dwSlot &&
                m_wdcDevice.slot.dwFunction == slot.dwFunction)
                return true;

            return false;
        }

        /** protected methods **/

        protected void SetDescription()
        {
            DWORD dwExpressGen = CallWinDriverAPIFunc(() =>
                wdc_lib_decl.WDC_PciGetExpressGenBySlot(ref m_wdcDevice.slot));
            m_sDeviceLongDesc = string.Format("PCI{0} Device: Vendor ID 0x{1:X}"
                + ", Device ID 0x{2:X}, Physical Location {3:X}:{4:X}:{5:X}",
                (dwExpressGen > 0) ? " Express Gen" + dwExpressGen.ToString()
                : "", id.dwVendorId, id.dwDeviceId, slot.dwBus, slot.dwSlot,
                slot.dwFunction);

            m_sDeviceShortDesc = string.Format("Device " +
                "{0:X},{1:X} {2:X}:{3:X}:{4:X}", id.dwVendorId,
                id.dwDeviceId, slot.dwBus, slot.dwSlot, slot.dwFunction);
        }

        /** private methods **/

        private bool DeviceValidate()
        {
            DWORD i, dwNumAddrSpaces = m_wdcDevice.dwNumAddrSpaces;

            /* NOTE: You can modify the implementation of this function in
             * order to verify that the device has the resources you expect to
             * find */

            /* Verify that the device has at least one active address space */
            for (i = 0; i < dwNumAddrSpaces; i++)
            {
                if (CallWinDriverAPIFunc(() =>
                        wdc_lib_decl.WDC_AddrSpaceIsActive(Handle, i)) == true)
                {
                    return true;
                }
            }

            /* In this sample we accept the device even if it doesn't have any
             * address spaces */
            Log.TraceLog("PCI_Device.DeviceValidate: Device does not have "
                + "any active memory or I/O address spaces " + "(" +
                this.ToString(false) + ")");
            return true;
        }

#endregion

#region " Device Open/Close "
        /****************************
         *  Device Open & Close     *
         * **************************/

        /* public methods */
        public virtual DWORD Open()
        {
            DWORD dwStatus;
            WD_PCI_CARD_INFO deviceInfo = new WD_PCI_CARD_INFO();
            PCI_DEV_ADDR_DESC devAddrDesc;
            GCHandle gc_handle;
            IntPtr pDevAddrDesc;

            /* Retrieve the device's resources information */
            deviceInfo.pciSlot = slot;
#if NET5_0
            dwStatus = wdc_lib_decl.WDC_PciGetDeviceInfo(ref deviceInfo);
#else
            dwStatus = CallWinDriverAPIFunc(() =>
                wdc_lib_decl.WDC_PciGetDeviceInfo(deviceInfo));
#endif
            if ((DWORD)wdc_err.WD_STATUS_SUCCESS != dwStatus)
            {
                Log.ErrLog("PCI_Device.Open: Failed retrieving the "
                    + "device's resources information. Error 0x" +
                    dwStatus.ToString("X") + ": " + utils.Stat2Str(dwStatus) +
                    "(" + this.ToString(false) +")");
                return dwStatus;
            }

            /* NOTE: You can modify the device's resources information here,
             * if necessary (mainly the deviceInfo.Card.Items array or the
             * items number - deviceInfo.Card.dwItems) in order to register
             * only some of the resources or register only a portion of a
             * specific address space, for example. */
#if NET5_0
            dwStatus = wdc_lib_decl.WDC_PciDeviceOpen(ref m_wdcDevice.hDev,
                ref deviceInfo, IntPtr.Zero);
#else
            dwStatus = CallWinDriverAPIFunc(() =>
                wdc_lib_decl.WDC_PciDeviceOpen(ref m_wdcDevice, deviceInfo,
                    IntPtr.Zero));
#endif
            if ((DWORD)wdc_err.WD_STATUS_SUCCESS != dwStatus)
            {
                Log.ErrLog("PCI_Device.Open: Failed opening a " +
                    "WDC device handle. Error 0x" + dwStatus.ToString("X") +
                    ": " + utils.Stat2Str(dwStatus) + "(" +
                    this.ToString(false) + ")");

                goto Error;
            }

#if NET5_0
            /* Marshalling the native WDC_DEVICE struct from native to managed
             * Using custom casting operator(defined in windrvr_netcore.cs) */
            m_wdcDevice = m_wdcDevice.hDev;
#endif
            Log.TraceLog("PCI_Device.Open: Opened a PCI device " +
                this.ToString(false));

            /* We're calling a c-language method, passing a pointer.
             * To create this pointer in c#, we use the 'pinned' method and
             * Marshal allocation, which creates unmanaged memory (which we must
             * free - no garbage collection) */
            devAddrDesc.dwNumAddrSpaces = NumAddrSpaces;
            gc_handle = GCHandle.Alloc(AddrDesc);
            devAddrDesc.AddrDesc =
                    Marshal.UnsafeAddrOfPinnedArrayElement(AddrDesc, 0);

            pDevAddrDesc = Marshal.AllocHGlobal(
                    Marshal.SizeOf(typeof(PCI_DEV_ADDR_DESC)));
            if (pDevAddrDesc == IntPtr.Zero)
            {
                Log.ErrLog("PCI_Device.Open: Failed to allocate pDevAddrDesc " +
                        "memory");
                dwStatus = (DWORD)wdc_err.WD_INSUFFICIENT_RESOURCES;
                gc_handle.Free();
                goto Error;
            }

            Marshal.StructureToPtr(devAddrDesc, pDevAddrDesc, false);

            /* Open a Kernel PlugIn handle to the device */
#if NET5_0
            dwStatus = wdc_lib_decl.WDC_KernelPlugInOpen(m_wdcDevice.hDev,
                m_sKP_PCI_DRIVER_NAME, pDevAddrDesc);
#else
            dwStatus = CallWinDriverAPIFunc(() =>
                wdc_lib_decl.WDC_KernelPlugInOpen(m_wdcDevice,
                    m_sKP_PCI_DRIVER_NAME, pDevAddrDesc));
#endif
            if ((DWORD)wdc_err.WD_STATUS_SUCCESS != dwStatus)
            {
                Log.ErrLog("PCI_Device.Open: Failed opening a " +
                    "Kernel PlugIn handle to the KP driver. Error 0x" +
                    dwStatus.ToString("X") + ": " + utils.Stat2Str(dwStatus) +
                    "(" + this.ToString(false) + ")");
            }
            else
            {
                Log.TraceLog("PCI_Device.Open: Opened a handle to " +
                    "Kernel PlugIn driver " + m_sKP_PCI_DRIVER_NAME +
                    this.ToString(false));
            }
            Marshal.FreeHGlobal(pDevAddrDesc);
            gc_handle.Free();

            /* Validate device information */
            if (DeviceValidate() != true)
            {
                dwStatus = (DWORD)wdc_err.WD_NO_RESOURCES_ON_DEVICE;
                goto Error;
            }

            /* This a short example of communicating with the Kernel PlugIn
               KP_PCI driver using WDC_CallKerPlug() in the user-mode, and
               KP_PCI_Call() in the kernel mode */
            if (wdc_defs_macros.WDC_IS_KP(wdcDevice))
            {
                DWORD dwKPResult = 0;
                KP_PCI_VERSION kpVer;
                IntPtr pData = Marshal.AllocHGlobal(
                        Marshal.SizeOf(typeof(DWORD)) + 100);
                if (pData == IntPtr.Zero)
                {
                    Log.ErrLog("PCI_Device.Open: Failed to allocate memory");
                    dwStatus = (DWORD)wdc_err.WD_INSUFFICIENT_RESOURCES;
                    goto Error;
                }

                /* Get Kernel PlugIn driver version */
                dwStatus = CallWinDriverAPIFunc(() =>
                    wdc_lib_decl.WDC_CallKerPlug(Handle,
                        (DWORD)KP_PCI_MSG.KP_PCI_MSG_VERSION, pData,
                        ref dwKPResult));
                if ((DWORD)wdc_err.WD_STATUS_SUCCESS != dwStatus)
                {
                    Log.ErrLog("Failed sending a kerplug message to the Kernel "
                        + "PlugIn driver status: " + utils.Stat2Str(dwStatus));
                }
                else if ((DWORD)KP_PCI_STATUS.KP_PCI_STATUS_OK != dwKPResult)
                {
                    Log.ErrLog("Kernel PlugIn Get Version message " +
                        KP_PCI_MSG.KP_PCI_MSG_VERSION.ToString("X") + "failed. "
                        + "Kernel PlugIn status: " + dwKPResult.ToString("X"));
                }
                else
                {
                    kpVer.dwVer = (DWORD)Marshal.ReadInt32(pData);
                    IntPtr x = new IntPtr(pData.ToInt64() +
                            Marshal.SizeOf(typeof(DWORD)));
                    kpVer.cVer = Marshal.PtrToStringAnsi(x, 100);
                    Log.TraceLog("Using " + m_sKP_PCI_DRIVER_NAME +
                        "Kernel-PlugIn driver version " +
                        (kpVer.dwVer / 100).ToString("X") + "." +
                        (kpVer.dwVer % 100).ToString("X") + " - " + kpVer.cVer);
                }

                Marshal.FreeHGlobal(pData);
            }

            return (DWORD)wdc_err.WD_STATUS_SUCCESS;

Error:
            if (Handle != IntPtr.Zero)
                Close();

            return dwStatus;
        }

        public virtual bool Close()
        {
            DWORD dwStatus;

            if (Handle == IntPtr.Zero)
            {
                Log.ErrLog("PCI_Device.Close: Error - NULL device handle");
                return false;
            }

            /* unregister events */
            dwStatus = EventUnregister();

            /* Disable interrupts */
            dwStatus = DisableInterrupts();

            /* Close the device */
            dwStatus = CallWinDriverAPIFunc(() =>
                wdc_lib_decl.WDC_PciDeviceClose(Handle));
            if ((DWORD)wdc_err.WD_STATUS_SUCCESS != dwStatus)
            {
                Log.ErrLog("PCI_Device.Close: Failed closing a "
                    + "WDC device handle (0x" + Handle.ToInt64().ToString("X")
                    + ". Error 0x" + dwStatus.ToString("X") + ": " +
                    utils.Stat2Str(dwStatus) + this.ToString(false));
            }
            else
            {
                Log.TraceLog("PCI_Device.Close: " +
                    this.ToString(false) + " was closed successfully");
            }

            return ((DWORD)wdc_err.WD_STATUS_SUCCESS == dwStatus);
        }
#endregion

#region " Interrupts "
        /****************************
         *       Interrupts         *
         * **************************/

        /* public methods */
        public bool IsEnabledInt()
        {
            return CallWinDriverAPIFunc(() =>
                wdc_lib_decl.WDC_IntIsEnabled(this.Handle));
        }

        protected virtual DWORD CreateIntTransCmds(out WD_TRANSFER[]
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

                return (DWORD)wdc_err.WD_STATUS_SUCCESS;
        }

        protected virtual DWORD DisableCardInts()
        {
            /* TODO: You can add code here to write to the device in order
               to physically disable the hardware interrupts */
            return (DWORD)wdc_err.WD_STATUS_SUCCESS;
        }

        protected BOOL IsItemExists(WDC_DEVICE Dev, DWORD item)
        {
            DWORD i;
            DWORD dwNumItems = Dev.cardReg.Card.dwItems;

            for (i = 0; i < dwNumItems; i++)
            {
                if (Dev.cardReg.Card.Item[i].item == item)
                  return true;
            }

            return false;
        }

        public DWORD EnableInterrupts(USER_INTERRUPT_CALLBACK userIntCb,
                IntPtr pData)
        {
            WD_TRANSFER[] pIntTransCmds = null;
            DWORD dwNumCmds;

            if (userIntCb == null)
            {
                Log.TraceLog("PCI_Device.EnableInterrupts: "
                    + "user callback is invalid");
                return (DWORD)wdc_err.WD_INVALID_PARAMETER;
            }

            if (!IsItemExists(m_wdcDevice, (DWORD)item_types.ITEM_INTERRUPT))
            {
                Log.TraceLog("PCI_Device.EnableInterrupts: "
                    + "Device doesn't have any interrupts");
                return (DWORD)wdc_err.WD_OPERATION_FAILED;
            }

            m_userIntHandler = userIntCb;


            if (m_intHandler == null)
            {
                Log.ErrLog("PCI_Device.EnableInterrupts: interrupt handler is "
                    + "null (" + this.ToString(false) + ")");
                return (DWORD)wdc_err.WD_INVALID_PARAMETER;
            }

            if (CallWinDriverAPIFunc(() =>
                wdc_lib_decl.WDC_IntIsEnabled(Handle)) == true)
            {
                Log.ErrLog("PCI_Device.EnableInterrupts: " +
                    "interrupts are already enabled (" +
                    this.ToString(false) + ")");
                return (DWORD)wdc_err.WD_OPERATION_ALREADY_DONE;
            }

            DWORD dwStatus = CreateIntTransCmds(out pIntTransCmds,
                    out dwNumCmds);
            if (dwStatus != (DWORD)wdc_err.WD_STATUS_SUCCESS)
                return dwStatus;

            dwStatus = CallWinDriverAPIFunc(() =>
                wdc_lib_decl.WDC_IntEnable(wdcDevice, pIntTransCmds,
                    dwNumCmds, (DWORD)WD_INTERRUPT_OPTIONS.INTERRUPT_CMD_COPY,
                    m_intHandler, pData, wdc_defs_macros.WDC_IS_KP(wdcDevice)));
            if ((DWORD)wdc_err.WD_STATUS_SUCCESS != dwStatus)
            {
                Log.ErrLog("PCI_Device.EnableInterrupts: Failed "
                    + "enabling interrupts. Error " + dwStatus.ToString("X") +
                    ": " + utils.Stat2Str(dwStatus) + "(" +
                    this.ToString(false) + ")");
                m_intHandler = null;
                return dwStatus;
            }

             /* TODO: You can add code here to write to the device in order
                      to physically enable the hardware interrupts */

            Log.TraceLog("PCI_Device: enabled interrupts (" +
                    this.ToString(false) + ")");

            return dwStatus;
        }

        public DWORD DisableInterrupts()
        {
            DWORD dwStatus;

            if (CallWinDriverAPIFunc(() =>
                wdc_lib_decl.WDC_IntIsEnabled(this.Handle)) == false)
            {
                Log.ErrLog("PCI_Device.DisableInterrupts: interrupts are "
                    + "already disabled... (" + this.ToString(false) + ")");
                return (DWORD)wdc_err.WD_OPERATION_ALREADY_DONE;
            }

            /* Physically disabling the hardware interrupts */
            dwStatus = DisableCardInts();

            dwStatus = CallWinDriverAPIFunc(() =>
                wdc_lib_decl.WDC_IntDisable(m_wdcDevice));
            if (dwStatus != (DWORD)wdc_err.WD_STATUS_SUCCESS)
            {
                Log.ErrLog("PCI_Device.DisableInterrupts: Failed to" +
                    "disable interrupts. Error " + dwStatus.ToString("X")
                    + ": " + utils.Stat2Str(dwStatus) + " (" +
                    this.ToString(false) + ")");
            }
            else
            {
                Log.TraceLog("PCI_Device.DisableInterrupts: Interrupts are " +
                    "disabled (" + this.ToString(false) + ")");
            }

            return dwStatus;
        }

        public bool IsMsiInt()
        {
            return wdc_defs_macros.WDC_INT_IS_MSI(
                wdc_defs_macros.WDC_GET_ENABLED_INT_TYPE(wdcDevice));
        }

        public DWORD GetEnableIntLastMsg()
        {
            return wdc_defs_macros.WDC_GET_ENABLED_INT_LAST_MSG(wdcDevice);
        }

        public string WDC_DIAG_IntTypeDescriptionGet()
        {
            DWORD dwIntType =
                wdc_defs_macros.WDC_GET_ENABLED_INT_TYPE(wdcDevice);

            if ((dwIntType & (DWORD)(WD_INTERRUPT_TYPE.INTERRUPT_MESSAGE_X))
                != 0)
            {
                return "Extended Message-Signaled Interrupt (MSI-X)";
            }
            else if ((dwIntType & (DWORD)(WD_INTERRUPT_TYPE.INTERRUPT_MESSAGE))
                != 0)
            {
                return "Message-Signaled Interrupt (MSI)";
            }
            else if ((dwIntType &
                (DWORD)(WD_INTERRUPT_TYPE.INTERRUPT_LEVEL_SENSITIVE)) != 0)
            {
               return "Level-Sensitive Interrupt";
            }

            return "Edge-Triggered Interrupt";
        }

        /* private methods */
        private void IntHandler(IntPtr pDev)
        {
#if !NET5_0
            wdcDevice.Int =
                (WD_INTERRUPT)m_wdcDeviceMarshaler.MarshalDevWdInterrupt(pDev);

            /* to obtain the data that was read at interrupt use:
             * WD_TRANSFER[] transCommands;
             * transCommands = (WD_TRANSFER[])
             *     m_wdcDeviceMarshaler.MarshalDevpWdTrans(wdcDevice.Int.Cmd,
             *     wdcDevice.Int.dwCmds);
             */
#endif

            if (m_userIntHandler != null)
                m_userIntHandler(this);
        }

#endregion

#region " Events"
        /****************************
         *          Events          *
         * **************************/

        /* public methods */
        public bool IsEventRegistered()
        {
            if (Handle == IntPtr.Zero)
                return false;

            return CallWinDriverAPIFunc(() =>
                wdc_lib_decl.WDC_EventIsRegistered(Handle));
        }

        public DWORD EventRegister(USER_EVENT_CALLBACK userEventHandler)
        {
            DWORD dwStatus;
            DWORD dwActions = (DWORD)windrvr_consts.WD_ACTIONS_ALL;
            /* TODO: Modify the above to set up the plug-and-play/power
             * management events for which you wish to receive notifications.
             * dwActions can be set to any combination of the WD_EVENT_ACTION
             * flags defined in windrvr.h */

            if (userEventHandler == null)
            {
                Log.ErrLog("PCI_Device.EventRegister: user callback is "
                    + "null");
                return (DWORD)wdc_err.WD_INVALID_PARAMETER;
            }

            /* Check if event is already registered */
            if (CallWinDriverAPIFunc(() =>
                wdc_lib_decl.WDC_EventIsRegistered(Handle)) == true)
            {
                Log.ErrLog("PCI_Device.EventRegister: Events are already "
                    + "registered ...");
                return (DWORD)wdc_err.WD_OPERATION_ALREADY_DONE;
            }

            m_userEventHandler = userEventHandler;

            /* Register event */
            dwStatus = CallWinDriverAPIFunc(() =>
                wdc_lib_decl.WDC_EventRegister(m_wdcDevice, dwActions,
                    m_eventHandler, Handle,
                    wdc_defs_macros.WDC_IS_KP(wdcDevice)));

            if ((DWORD)wdc_err.WD_STATUS_SUCCESS != dwStatus)
            {
                Log.ErrLog("PCI_Device.EventRegister: Failed to register "
                    + "events. Error 0x" + dwStatus.ToString("X") +
                    utils.Stat2Str(dwStatus));
                m_userEventHandler = null;
            }
            else
            {
                Log.TraceLog("PCI_Device.EventRegister: events are " +
                    " registered (" + this.ToString(false) +")");
            }

            return dwStatus;
        }

        public DWORD EventUnregister()
        {
            DWORD dwStatus;

            if (CallWinDriverAPIFunc(() =>
                wdc_lib_decl.WDC_EventIsRegistered(Handle)) == false)
            {
                Log.ErrLog("PCI_Device.EventUnregister: No events " +
                    "currently registered ...(" + this.ToString(false) + ")");
                return (DWORD)wdc_err.WD_OPERATION_ALREADY_DONE;
            }

            dwStatus = CallWinDriverAPIFunc(() =>
                wdc_lib_decl.WDC_EventUnregister(m_wdcDevice));

            if ((DWORD)wdc_err.WD_STATUS_SUCCESS != dwStatus)
            {
                Log.ErrLog("PCI_Device.EventUnregister: Failed to " +
                    " unregister events. Error 0x" + dwStatus.ToString("X") +
                    ": " + utils.Stat2Str(dwStatus) + "(" +
                    this.ToString(false) + ")");
            }
            else
            {
                Log.TraceLog("PCI_Device.EventUnregister: Unregistered " +
                    " events (" + this.ToString(false) + ")");
            }

            return dwStatus;
        }

        /** private methods **/

        /* event callback method */
        private void EventHandler(IntPtr pWdEvent, IntPtr pDev)
        {
#if NET5_0
            WD_EVENT wdEvent = Marshal.PtrToStructure<WD_EVENT>(pWdEvent);
#else
            MarshalWdEvent wdEventMarshaler = new MarshalWdEvent();
            WD_EVENT wdEvent =
                    (WD_EVENT)wdEventMarshaler.MarshalNativeToManaged(pWdEvent);
            m_wdcDevice.Event =
                (WD_EVENT)m_wdcDeviceMarshaler.MarshalDevWdEvent(pDev);
#endif
            if (m_userEventHandler != null)
                m_userEventHandler(ref wdEvent, this);
        }
#endregion

    }
}

