/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

using System;
using System.Text;
using System.Runtime.InteropServices;

namespace Jungo
{
    using DWORD = System.UInt32;
    using CBOOL = System.UInt32;
    using BOOL = System.Boolean;
    using WDC_DRV_OPEN_OPTIONS = System.UInt32;
    using UINT32 = System.UInt32;
    using UINT64 = System.UInt64;
    using KPTR = System.UInt64;
    using UPTR = System.UInt64;
    using PHYS_ADDR = System.UInt64;
    using WORD = System.UInt16;
    using BYTE = System.Byte;
    using HANDLE = System.IntPtr;
    using WDC_DEVICE_HANDLE = System.IntPtr;

#if WIN32
    using WDC_DBG_OPTIONS = System.UInt32;
#else
    using WDC_DBG_OPTIONS = System.UInt64;
#endif

    namespace wdapi_dotnet
    {
        public static class wdc_lib_consts
        {
            public const int MAX_NAME = 128;
            public const int MAX_DESC = 128;
            public const int MAX_NAME_DISPLAY = 22;

            public const int WDC_AD_CFG_SPACE = 0xFF;

            public const WDC_DRV_OPEN_OPTIONS WDC_DRV_OPEN_BASIC = 0x0;
            public const WDC_DRV_OPEN_OPTIONS WDC_DRV_OPEN_CHECK_VER = 0x1;
            public const WDC_DRV_OPEN_OPTIONS WDC_DRV_OPEN_REG_LIC = 0x2;
            public const WDC_DRV_OPEN_OPTIONS WDC_DRV_OPEN_ALL =
                (WDC_DRV_OPEN_CHECK_VER | WDC_DRV_OPEN_REG_LIC);
            public const DWORD WDC_DRV_OPEN_KP = WDC_DRV_OPEN_BASIC;
            public const WDC_DRV_OPEN_OPTIONS WDC_DRV_OPEN_DEFAULT =
                WDC_DRV_OPEN_ALL;

            /* Send WDC debug messages to the Debug Monitor */
            public const WDC_DBG_OPTIONS WDC_DBG_OUT_DBM = 0x1;

            /* Send WDC debug messages to a debug file (default: stderr)
             * [User-mode only] */
            public const WDC_DBG_OPTIONS WDC_DBG_OUT_FILE = 0x2;

            /* Display only error WDC debug messages */
            public const WDC_DBG_OPTIONS WDC_DBG_LEVEL_ERR = 0x10;

            /* Display error and trace WDC debug messages */
            public const WDC_DBG_OPTIONS WDC_DBG_LEVEL_TRACE = 0x20;

            /* Do not print debug messages */
            public const WDC_DBG_OPTIONS WDC_DBG_NONE = 0x100;

            /* Convenient debug options combinations/defintions */
            public const WDC_DBG_OPTIONS WDC_DBG_DEFAULT =
                (WDC_DBG_OUT_DBM | WDC_DBG_LEVEL_TRACE);

            public const WDC_DBG_OPTIONS WDC_DBG_DBM_ERR =
                (WDC_DBG_OUT_DBM | WDC_DBG_LEVEL_ERR);
            public const WDC_DBG_OPTIONS WDC_DBG_DBM_TRACE =
                (WDC_DBG_OUT_DBM | WDC_DBG_LEVEL_TRACE);

            public const WDC_DBG_OPTIONS WDC_DBG_FILE_ERR =
                (WDC_DBG_OUT_FILE | WDC_DBG_LEVEL_ERR);
            public const WDC_DBG_OPTIONS WDC_DBG_FILE_TRACE =
                (WDC_DBG_OUT_FILE | WDC_DBG_LEVEL_TRACE);

            public const WDC_DBG_OPTIONS WDC_DBG_DBM_FILE_ERR =
                (WDC_DBG_OUT_DBM | (WDC_DBG_OUT_FILE | WDC_DBG_LEVEL_ERR));
            public const WDC_DBG_OPTIONS WDC_DBG_DBM_FILE_TRACE =
                (WDC_DBG_OUT_DBM | (WDC_DBG_OUT_FILE | WDC_DBG_LEVEL_TRACE));

            public const WDC_DBG_OPTIONS WDC_DBG_FULL =
                (WDC_DBG_OUT_DBM | WDC_DBG_OUT_FILE | WDC_DBG_LEVEL_TRACE);

            public const uint WDC_SIZE_8 = sizeof(BYTE);
            public const uint WDC_SIZE_16 = sizeof(WORD);
            public const uint WDC_SIZE_32 = sizeof(UINT32);
            public const uint WDC_SIZE_64 = sizeof(UINT64);
        };

        public enum WDC_PCI_HEADER_TYPE
        {
            HEADER_TYPE_NORMAL = 0x01,
            HEADER_TYPE_BRIDGE = 0x02,
            HEADER_TYPE_CARDBUS = 0x04,
            HEADER_TYPE_NRML_BRIDGE = HEADER_TYPE_NORMAL | HEADER_TYPE_BRIDGE,
            HEADER_TYPE_NRML_CARDBUS = HEADER_TYPE_NORMAL | HEADER_TYPE_CARDBUS,
            HEADER_TYPE_BRIDGE_CARDBUS = HEADER_TYPE_BRIDGE |
                HEADER_TYPE_CARDBUS,
            HEADER_TYPE_ALL = HEADER_TYPE_NORMAL | HEADER_TYPE_BRIDGE |
                HEADER_TYPE_CARDBUS
        };

        public enum WDC_ADDR_MODE
        {
            WDC_MODE_8 = (int)wdc_lib_consts.WDC_SIZE_8,
            WDC_MODE_16 = (int)wdc_lib_consts.WDC_SIZE_16,
            WDC_MODE_32 = (int)wdc_lib_consts.WDC_SIZE_32,
            WDC_MODE_64 = (int)wdc_lib_consts.WDC_SIZE_64
        }

        /** Read/write address options */
        public enum WDC_ADDR_RW_OPTIONS
        {
            WDC_ADDR_RW_DEFAULT = 0x0, /* Default: memory resource - direct access;
                                        autoincrement on block transfers */
            WDC_ADDR_RW_NO_AUTOINC = 0x4 /* Hold device address constant while
                                             reading/writing a block */
        }

        public enum WDC_DIRECTION
        {
            WDC_WRITE,
            WDC_READ,
            WDC_READ_WRITE,
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct WDC_PCI_SCAN_RESULT
        {
            public DWORD dwNumDevices;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 256)]
            public WD_PCI_ID[] deviceId;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 256)]
            public WD_PCI_SLOT[] deviceSlot;
        }

        public enum WDC_SLEEP_OPTIONS
        {
            WDC_SLEEP_BUSY = 0,
            WDC_SLEEP_NON_BUSY = windrvr_consts.SLEEP_NON_BUSY
        };

        public struct WDC_PCI_SCAN_CAPS_RESULT
        {
            public DWORD dwNumCaps;      // Number of capabilities found
            [MarshalAs(UnmanagedType.ByValArray, SizeConst =
                windrvr_consts.WD_PCI_MAX_CAPS)]
            public WD_PCI_CAP[] pciCaps;	// ID and offset of found capability

        };

        [StructLayout(LayoutKind.Sequential)]
        public struct WDC_ADDR_DESC
        {
            public DWORD dwAddrSpace;         /* Address space number */
            public CBOOL fIsMemory;           /* TRUE: memory address space;
                                        * FALSE: I/O */
            public DWORD dwItemIndex;         /* Index of address space in
                                        * pDev.cardReg.Card.Item array */
            public DWORD reserved;
            public UINT64 qwBytes;            /* Size of address space */
            public KPTR pAddr;               /* I/O / memory kernel mapped address -
                                        * for WD_Transfer(), WD_MultiTransfer()
                                        * or direct kernel access */
            public UPTR pUserDirectMemAddr;   /* Memory address for direct user-mode
                                        * access */
        };

        public class wdc_lib_decl
        {
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_DriverOpen(DWORD openOptions,
                String sLicense);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_DriverClose();

            /* Get a handle to WinDriver (required for WD_XXX functions) */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern HANDLE WDC_GetWDHandle();

            /* Get a device's user context */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern IntPtr WDC_GetDevContext(
                WDC_DEVICE_HANDLE hDev);

            /* Get a device's bus type */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern WD_BUS_TYPE WDC_GetBusType(
                WDC_DEVICE_HANDLE hDev);

            /* Sleep (default option - WDC_SLEEP_BUSY) */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_Sleep(DWORD dwMicroSecs,
                WDC_SLEEP_OPTIONS options);

            /* Set debug options for the WDC library */
            [DllImport(windrvr_decl.DLL_NAME,
                CallingConvention = CallingConvention.Cdecl)]
            public static extern DWORD WDC_SetDebugOptions(
                WDC_DBG_OPTIONS dbgOptions, String sDbgFile);

            /* Debug messages display */
            [DllImport(windrvr_decl.DLL_NAME,
                CallingConvention = CallingConvention.Cdecl)]
            public static extern void WDC_Err(string format);

            [DllImport(windrvr_decl.DLL_NAME,
                CallingConvention = CallingConvention.Cdecl)]
            public static extern void WDC_Trace(string format);


            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciScanDevices(DWORD dwVendorId,
                DWORD dwDeviceId,
                ref WDC_PCI_SCAN_RESULT pPciScanResult);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciScanCaps(WDC_DEVICE_HANDLE hDev,
                DWORD dwCapId,
                ref WDC_PCI_SCAN_CAPS_RESULT pPciScanCapsResult);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciScanExtCaps(
                WDC_DEVICE_HANDLE hDev, DWORD dwCapId,
                ref WDC_PCI_SCAN_CAPS_RESULT pPciScanCapsResult);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciGetDeviceInfo(
                ref WD_PCI_CARD_INFO pDeviceInfo);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciDeviceOpen(ref IntPtr dev,
                ref WD_PCI_CARD_INFO deviceInfo, IntPtr pDevCtx);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciDeviceClose(
                IntPtr hDev);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_IsaDeviceOpen(
                ref WDC_DEVICE_HANDLE hDev, ref IntPtr pDeviceInfo,
                IntPtr pDevCtx);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_IsaDeviceClose(
                WDC_DEVICE_HANDLE hDev);

            /* -----------------------------------------------
               Set card cleanup commands
               ----------------------------------------------- */
            [DllImport(windrvr_decl.DLL_NAME)]
            private static extern DWORD WDC_CardCleanupSetup(
                WDC_DEVICE_HANDLE hDev, WD_TRANSFER[] Cmd, DWORD dwCmds,
                CBOOL fForceCleanup);
            public static DWORD WDC_CardCleanupSetup(
                WDC_DEVICE_HANDLE hDev, WD_TRANSFER[] Cmd, DWORD dwCmds,
                BOOL fForceCleanup)
            {
                return WDC_CardCleanupSetup(hDev, Cmd, dwCmds,
                    wdc_lib_macros.DotnetBoolToCBool(fForceCleanup));
            }

            /* -----------------------------------------------
               Kernel PlugIn
               ----------------------------------------------- */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_KernelPlugInOpen(
                WDC_DEVICE_HANDLE hDev, String pcKPDriverName,
                IntPtr pKPOpenData);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_CallKerPlug(WDC_DEVICE_HANDLE hDev,
                DWORD dwMsg, IntPtr pData, ref DWORD pdwResult);

            /* Read/write a device's address space (8/16/32/64 bits) */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_ReadAddr8(WDC_DEVICE_HANDLE hDev,
                DWORD dwAddrSpace, KPTR dwOffset, ref BYTE val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_ReadAddr16(WDC_DEVICE_HANDLE hDev,
                DWORD dwAddrSpace, KPTR dwOffset, ref WORD val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_ReadAddr32(WDC_DEVICE_HANDLE hDev,
                DWORD dwAddrSpace, KPTR dwOffset, ref UINT32 val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_ReadAddr64(WDC_DEVICE_HANDLE hDev,
                DWORD dwAddrSpace, KPTR dwOffset, ref UINT64 val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_WriteAddr8(WDC_DEVICE_HANDLE hDev,
                DWORD dwAddrSpace, KPTR dwOffset, BYTE val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_WriteAddr16(WDC_DEVICE_HANDLE hDev,
                DWORD dwAddrSpace, KPTR dwOffset, WORD val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_WriteAddr32(WDC_DEVICE_HANDLE hDev,
                DWORD dwAddrSpace, KPTR dwOffset, UInt32 val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_WriteAddr64(WDC_DEVICE_HANDLE hDev,
                DWORD dwAddrSpace, KPTR dwOffset, UInt64 val);

            /* Read/write a block of addresses */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_ReadAddrBlock(WDC_DEVICE_HANDLE hDev,
                DWORD dwAddrSpace, KPTR dwOffset, DWORD dwBytes,
                ref IntPtr pData, WDC_ADDR_MODE mode,
                WDC_ADDR_RW_OPTIONS options);

#if WIN32 // DWORD=UINT64 when not on WINDOWS, so #if WIN32 in order to avoid duplicate definition
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_ReadAddrBlock(WDC_DEVICE_HANDLE hDev,
                DWORD dwAddrSpace, KPTR dwOffset, DWORD dwBytes,
                UINT64[] pData, WDC_ADDR_MODE mode,
                WDC_ADDR_RW_OPTIONS options);
#endif

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_ReadAddrBlock(WDC_DEVICE_HANDLE hDev,
                DWORD dwAddrSpace, KPTR dwOffset, DWORD dwBytes,
                DWORD[] pData, WDC_ADDR_MODE mode,
                WDC_ADDR_RW_OPTIONS options);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_ReadAddrBlock(WDC_DEVICE_HANDLE hDev,
                DWORD dwAddrSpace, KPTR dwOffset, DWORD dwBytes,
                WORD[] pData, WDC_ADDR_MODE mode,
                WDC_ADDR_RW_OPTIONS options);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_ReadAddrBlock(WDC_DEVICE_HANDLE hDev,
                DWORD dwAddrSpace, KPTR dwOffset, DWORD dwBytes,
                BYTE[] pData, WDC_ADDR_MODE mode,
                WDC_ADDR_RW_OPTIONS options);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_WriteAddrBlock
                (WDC_DEVICE_HANDLE hDev, DWORD dwAddrSpace, KPTR dwOffset,
                DWORD dwBytes, ref IntPtr pData, WDC_ADDR_MODE mode,
                WDC_ADDR_RW_OPTIONS options);

#if WIN32 // DWORD=UINT64 when not on WINDOWS, so #if WIN32 in order to avoid duplicate definition
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_WriteAddrBlock(
                WDC_DEVICE_HANDLE hDev,
                DWORD dwAddrSpace, KPTR dwOffset, DWORD dwBytes,
                UINT64[] pData, WDC_ADDR_MODE mode,
                WDC_ADDR_RW_OPTIONS options);
#endif

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_WriteAddrBlock
                (WDC_DEVICE_HANDLE hDev, DWORD dwAddrSpace, KPTR dwOffset,
                DWORD dwBytes, DWORD[] pData, WDC_ADDR_MODE mode,
                WDC_ADDR_RW_OPTIONS options);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_WriteAddrBlock(
                WDC_DEVICE_HANDLE hDev, DWORD dwAddrSpace, KPTR dwOffset,
                DWORD dwBytes, WORD[] pData, WDC_ADDR_MODE mode,
                WDC_ADDR_RW_OPTIONS options);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_WriteAddrBlock(
                WDC_DEVICE_HANDLE hDev, DWORD dwAddrSpace, KPTR dwOffset,
                DWORD dwBytes, BYTE[] pData, WDC_ADDR_MODE mode,
                WDC_ADDR_RW_OPTIONS options);

            /* Read/Write multiple addresses */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_MultiTransfer(
                IntPtr /* WD_TRANSFER* */ pTrans, DWORD dwNumTrans);

            /* Is address space active */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern BOOL WDC_AddrSpaceIsActive(
                WDC_DEVICE_HANDLE hDev, DWORD dwAddrSpace);

            /* -----------------------------------------------
              Access PCI configuration space
              ----------------------------------------------- */
            /* Read/write a block of any length from the PCI configuration
             * space. Identify device by its location. */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciReadCfgBySlot(ref WD_PCI_SLOT pPciSlot,
                DWORD dwOffset, IntPtr pData, DWORD dwBytes);

            /* Identify device by handle */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciReadCfg(WDC_DEVICE_HANDLE hDev,
                DWORD dwOffset, IntPtr pData, DWORD dwBytes);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciWriteCfg(WDC_DEVICE_HANDLE hDev,
                DWORD dwOffset, IntPtr pData, DWORD dwBytes);

            /* Read/write 8/16/32/64 bits from the PCI configuration space.
               Identify device by its location. */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciReadCfgBySlot8(ref WD_PCI_SLOT pPciSlot,
                DWORD dwOffset, ref BYTE val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciReadCfgBySlot16(ref WD_PCI_SLOT pPciSlot,
                DWORD dwOffset, ref WORD val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciReadCfgBySlot32(ref WD_PCI_SLOT pPciSlot,
                DWORD dwOffset, ref UInt32 val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciReadCfgBySlot64(ref WD_PCI_SLOT pPciSlot,
                DWORD dwOffset, ref UINT64 val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciWriteCfgBySlot8(ref WD_PCI_SLOT pPciSlot,
                DWORD dwOffset, BYTE val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciWriteCfgBySlot16(ref WD_PCI_SLOT pPciSlot,
                DWORD dwOffset, WORD val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciWriteCfgBySlot32(ref WD_PCI_SLOT pPciSlot,
                DWORD dwOffset, UInt32 val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciWriteCfgBySlot64(ref WD_PCI_SLOT pPciSlot,
                DWORD dwOffset, UINT64 val);

            /* Read/write 8/16/32/64 bits from the PCI configuration space.
               Identify device by handle */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciReadCfg8(WDC_DEVICE_HANDLE hDev,
                DWORD dwOffset, ref BYTE val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciReadCfg16(WDC_DEVICE_HANDLE hDev,
                DWORD dwOffset, ref WORD val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciReadCfg32(WDC_DEVICE_HANDLE hDev,
                DWORD dwOffset, ref UInt32 val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciReadCfg64(WDC_DEVICE_HANDLE hDev,
                DWORD dwOffset, ref UINT64 val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciWriteCfg8(WDC_DEVICE_HANDLE hDev,
                DWORD dwOffset, BYTE val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciWriteCfg16(WDC_DEVICE_HANDLE hDev,
                DWORD dwOffset, WORD val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciWriteCfg32(WDC_DEVICE_HANDLE hDev,
                DWORD dwOffset, UInt32 val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciWriteCfg64(WDC_DEVICE_HANDLE hDev,
                DWORD dwOffset, UINT64 val);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciGetHeaderType(
                WDC_DEVICE_HANDLE hDev, ref WDC_PCI_HEADER_TYPE header_type);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciGetExpressOffset(
                WDC_DEVICE_HANDLE hDev, ref DWORD pdwOffset);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciGetExpressGenBySlot(
                ref WD_PCI_SLOT pPciSlot);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_PciGetExpressGen(
                WDC_DEVICE_HANDLE hDev);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD PciConfRegData2Str(
                WDC_DEVICE_HANDLE hDev, DWORD dwOffset, StringBuilder pBuf,
                DWORD dwInLen, ref DWORD pdwOutLen);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD PciExpressConfRegData2Str(
                WDC_DEVICE_HANDLE hDev, DWORD dwOffset, StringBuilder pBuf,
                DWORD dwInLen, ref DWORD pdwOutLen);

            /* -----------------------------------------------
              Interrupts
              ----------------------------------------------- */
            [DllImport(windrvr_decl.DLL_NAME)]
            private static extern DWORD WDC_IntEnable(WDC_DEVICE_HANDLE hDev,
                IntPtr pTransCmds, DWORD dwNumCmds, DWORD dwOptions,
                [MarshalAs(UnmanagedType.FunctionPtr)]
                INT_HANDLER funcIntHandler,
                IntPtr pData, CBOOL fUseKP);

            public static DWORD WDC_IntEnable(WDC_DEVICE dev,
                WD_TRANSFER[] TransCmds, DWORD dwNumCmds, DWORD dwOptions,
                [MarshalAs(UnmanagedType.FunctionPtr)]
                INT_HANDLER funcIntHandler,
                IntPtr pData, BOOL fUseKP)
            {
                GCHandle gchCmds = GCHandle.Alloc(TransCmds,
                    GCHandleType.Pinned);
                DWORD dwStatus = wdc_lib_decl.WDC_IntEnable(dev.hDev,
                    gchCmds.AddrOfPinnedObject(), dwNumCmds, dwOptions,
                    funcIntHandler, pData,
                    wdc_lib_macros.DotnetBoolToCBool(fUseKP));

                WDC_DEVICE_N wdcDeviceNative =
                    Marshal.PtrToStructure<WDC_DEVICE_N>(dev.hDev);

                if ((DWORD)WD_ERROR_CODES.WD_STATUS_SUCCESS == dwStatus)
                {
                    dev.Int = wdcDeviceNative.Int;
                    dev.hIntThread = wdcDeviceNative.hIntThread;
                    dev.hIntCmdsPin = gchCmds;
                }
                else
                {
                    gchCmds.Free();
                }

                return dwStatus;
            }

            [DllImport(windrvr_decl.DLL_NAME)]
            private static extern DWORD WDC_IntDisable(WDC_DEVICE_HANDLE hDev);

            public static DWORD WDC_IntDisable(WDC_DEVICE dev)
            {
                DWORD dwStatus = WDC_IntDisable(dev.hDev);
                WDC_DEVICE_N wdcDeviceNative =
                    Marshal.PtrToStructure<WDC_DEVICE_N>(dev.hDev);

                if ((DWORD)WD_ERROR_CODES.WD_STATUS_SUCCESS == dwStatus)
                {
                    dev.hIntThread = wdcDeviceNative.hIntThread;
                    dev.hIntCmdsPin.Free();
                }

                return dwStatus;
            }

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern BOOL WDC_IntIsEnabled(WDC_DEVICE_HANDLE hDev);

            /* -----------------------------------------------
              Events
              ----------------------------------------------- */
            [DllImport(windrvr_decl.DLL_NAME)]
            private static extern DWORD WDC_EventRegister(
                WDC_DEVICE_HANDLE hDev, DWORD dwActions,
                [MarshalAs(UnmanagedType.FunctionPtr)]
                EVENT_HANDLER_DOTNET funcEventHandler,
                IntPtr pData, CBOOL fUseKP);

            public static DWORD WDC_EventRegister(
                WDC_DEVICE dev, DWORD dwActions,
                [MarshalAs(UnmanagedType.FunctionPtr)]
                EVENT_HANDLER_DOTNET funcEventHandler,
                IntPtr pData, BOOL fUseKP)
            {
                DWORD dwStatus = WDC_EventRegister(dev.hDev, dwActions,
                    funcEventHandler, pData,
                    wdc_lib_macros.DotnetBoolToCBool(fUseKP));
                if ((DWORD)WD_ERROR_CODES.WD_STATUS_SUCCESS == dwStatus)
                {
                    dev.hEvent =
                        Marshal.PtrToStructure<WDC_DEVICE_N>(dev.hDev).hEvent;
                }

                return dwStatus;
            }


            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern BOOL WDC_EventIsRegistered(
                WDC_DEVICE_HANDLE hDev);

            [DllImport(windrvr_decl.DLL_NAME)]
            private static extern DWORD WDC_EventUnregister(
                WDC_DEVICE_HANDLE hDev);

            public static DWORD WDC_EventUnregister(WDC_DEVICE dev)
            {
                DWORD dwStatus = WDC_EventUnregister(dev.hDev);
                if ((DWORD)WD_ERROR_CODES.WD_STATUS_SUCCESS == dwStatus)
                {
                    dev.hEvent =
                        Marshal.PtrToStructure<WDC_DEVICE_N>(dev.hDev).hEvent;
                }

                return dwStatus;
            }

            /* -----------------------------------------------
              DMA
              ----------------------------------------------- */
            /* Lock a contiguous DMA buffer */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_DMAContigBufLock
                (WDC_DEVICE_HANDLE hDev, ref IntPtr ppBuf, DWORD dwOptions,
                DWORD dwDMABufSize, ref IntPtr ppDma);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_DMASGBufLock
                (WDC_DEVICE_HANDLE hDev, IntPtr pBuf, DWORD dwOptions,
                DWORD dwDMABufSize, ref IntPtr ppDma);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_DMABufGet(DWORD hDma,
                ref IntPtr ppDma);

            /* These functions are related to DMA transactions */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD
                WDC_DMATransactionContigInit(WDC_DEVICE_HANDLE hDev,
                ref IntPtr ppBuf, DWORD dwOptions, DWORD dwDMABufSize,
                ref IntPtr ppDma, IntPtr interruptParams, DWORD dwAlignment);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_DMATransactionSGInit(
                WDC_DEVICE_HANDLE hDev, IntPtr pBuf, DWORD dwOptions,
                DWORD dwDMABufSize, ref IntPtr ppDma, IntPtr interruptParams,
                DWORD dwMaxTransferSize, DWORD dwTransferElementSize);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_DMATransactionExecute(IntPtr pDma,
                [MarshalAs(UnmanagedType.FunctionPtr)]
                DMA_TRANSACTION_CALLBACK DMATransactionCallback,
                IntPtr DMATransactionCallbackCtx);

            [DllImport(windrvr_decl.DLL_NAME)]
            private static extern DWORD WDC_DMATransferCompletedAndCheck(
                IntPtr pDma, CBOOL fRunCallback);
            public static DWORD WDC_DMATransferCompletedAndCheck(
                IntPtr pDma, BOOL fRunCallback)
            {
                return WDC_DMATransferCompletedAndCheck(pDma,
                    wdc_lib_macros.DotnetBoolToCBool(fRunCallback));
            }

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_DMATransactionRelease(IntPtr pDma);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_DMATransactionUninit(IntPtr pDma);

            /* Lock a DMA buffer for reserved memory */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_DMAReservedBufLock(
                WDC_DEVICE_HANDLE hDev, PHYS_ADDR qwAddr, ref IntPtr ppBuf,
                DWORD dwOptions, DWORD dwDMABufSize, ref IntPtr ppDma);

            /* Unlock a DMA buffer */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_DMABufUnlock(IntPtr pDma);

            /* Synchronize cache of all CPUs with the DMA buffer,
             * should be called before DMA transfer */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_DMASyncCpu(IntPtr pDma);

            /* Flush the data from I/O cache and update the CPU caches.
             * Should be called after DMA transfer. */
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDC_DMASyncIo(IntPtr pDma);
        }
        public class wdc_defs_macros
        {
            /* -----------------------------------------------
            Memory / I/O / Registers
            ----------------------------------------------- */
            /* Get direct memory address pointer */
            public static UPTR WDC_MEM_DIRECT_ADDR(WDC_ADDR_DESC addrDesc)
            {
                return addrDesc.pUserDirectMemAddr;
            }

            public static BOOL WDC_ADDR_IS_MEM(WDC_ADDR_DESC addrDesc)
            {
                return addrDesc.fIsMemory == 0 ? false : true;
            }

            public static DWORD WDC_GET_KP_HANDLE(WDC_DEVICE wdcDevice)
            {
                return wdcDevice.kerPlug.hKernelPlugIn;
            }

            /* Does the device use a Kernel PlugIn driver */
            public static BOOL WDC_IS_KP(WDC_DEVICE wdcDevice)
            {
                return (WDC_GET_KP_HANDLE(wdcDevice) != 0);
            }

            /* Get the device's resources struct (WD_CARD) */
            public static WD_CARD WDC_GET_PCARD(WDC_DEVICE wdcDevice)
            {
                return wdcDevice.cardReg.Card;
            }

            /* Get card handle */
            public static DWORD WDC_GET_CARD_HANDLE(WDC_DEVICE wdcDevice)
            {
                return wdcDevice.cardReg.hCard;
            }

            /* Get the WD PCI slot */
            public static WD_PCI_SLOT WDC_GET_PPCI_SLOT(WDC_DEVICE wdcDevice)
            {
                return wdcDevice.slot;
            }

            public static WDC_ADDR_DESC WDC_GET_ADDR_DESC(WDC_DEVICE wdcDevice,
                DWORD dwAddrSpace)
            {
                return wdcDevice.pAddrDesc[dwAddrSpace];
            }

            public static DWORD WDC_GET_ENABLED_INT_TYPE(WDC_DEVICE wdcDevice)
            {
                return wdcDevice.Int.dwEnabledIntType;
            }

            public static DWORD WDC_GET_ENABLED_INT_LAST_MSG(
                WDC_DEVICE wdcDevice)
            {
                return wdcDevice.Int.dwLastMessage;
            }

            public static bool WDC_INT_IS_MSI(DWORD dwIntType)
            {
                return (dwIntType &
                    ((DWORD)(WD_INTERRUPT_TYPE.INTERRUPT_MESSAGE) |
                    (DWORD)(WD_INTERRUPT_TYPE.INTERRUPT_MESSAGE_X))) != 0;
            }
        }

        public class wdc_lib_macros
        {

            public static DWORD WDC_ADDR_MODE_TO_SIZE(WDC_ADDR_MODE mode)
            {
                return (DWORD)mode;
            }

            public static WDC_ADDR_MODE WDC_ADDR_SIZE_TO_MODE(DWORD size)
            {
                return (((size) > wdc_lib_consts.WDC_SIZE_32) ?
                    WDC_ADDR_MODE.WDC_MODE_64 :
                    ((size) > wdc_lib_consts.WDC_SIZE_16) ?
                    WDC_ADDR_MODE.WDC_MODE_32 :
                    ((size) > wdc_lib_consts.WDC_SIZE_8) ?
                    WDC_ADDR_MODE.WDC_MODE_16 : WDC_ADDR_MODE.WDC_MODE_8);
            }

            public static BYTE WDC_ReadMem8(UPTR addr, DWORD off)
            {
                UPTR ptrAddr = addr + off;
                return Marshal.ReadByte((IntPtr)ptrAddr);
            }

            public static WORD WDC_ReadMem16(UPTR addr, DWORD off)
            {
                UPTR ptrAddr = addr + off;
                return (WORD)Marshal.ReadInt16((IntPtr)ptrAddr);
            }

            public static DWORD WDC_ReadMem32(UPTR addr, DWORD off)
            {
                UPTR ptrAddr = addr + off;
                return (DWORD)Marshal.ReadInt32((IntPtr)ptrAddr);
            }

            public static UINT64 WDC_ReadMem64(UPTR addr, DWORD off)
            {
                UPTR ptrAddr = addr + off;
                return (UINT64)Marshal.ReadInt64((IntPtr)ptrAddr);
            }

            public static void WDC_WriteMem8(UPTR addr, DWORD off, BYTE val)
            {
                UPTR ptrAddr = addr + off;
                Marshal.WriteByte((IntPtr)ptrAddr, val);
            }

            public static void WDC_WriteMem16(UPTR addr, DWORD off, WORD val)
            {
                UPTR ptrAddr = addr + off;
                Marshal.WriteInt16((IntPtr)ptrAddr, (short)val);
            }

            public static void WDC_WriteMem32(UPTR addr, DWORD off, UINT32 val)
            {
                UPTR ptrAddr = addr + off;
                Marshal.WriteInt32((IntPtr)ptrAddr, (int)val);
            }

            public static void WDC_WriteMem64(UPTR addr, DWORD off, UINT64 val)
            {
                UPTR ptrAddr = addr + off;
                Marshal.WriteInt64((IntPtr)ptrAddr, (Int64)val);
            }

            public static CBOOL DotnetBoolToCBool(BOOL fVal)
            {
                return Convert.ToUInt32(fVal);
            }

            public static String GetCapabilityStr(DWORD cap_id)
            {
                String str;

                switch (cap_id)
                {
                    case PciRegs.PCI_CAP_LIST_ID:
                        str = "Null Capability";
                        break;
                    case PciRegs.PCI_CAP_ID_PM:
                        str = "Power Management";
                        break;
                    case PciRegs.PCI_CAP_ID_AGP:
                        str = "Accelerated Graphics Port";
                        break;
                    case PciRegs.PCI_CAP_ID_VPD:
                        str = "Vital Product Data";
                        break;
                    case PciRegs.PCI_CAP_ID_SLOTID:
                        str = "Slot Identification";
                        break;
                    case PciRegs.PCI_CAP_ID_MSI:
                        str = "Message Signalled Interrupts (MSI)";
                        break;
                    case PciRegs.PCI_CAP_ID_CHSWP:
                        str = "CompactPCI HotSwap";
                        break;
                    case PciRegs.PCI_CAP_ID_PCIX:
                        str = "PCI-X";
                        break;
                    case PciRegs.PCI_CAP_ID_HT:
                        str = "HyperTransport";
                        break;
                    case PciRegs.PCI_CAP_ID_VNDR:
                        str = "Vendor-Specific";
                        break;
                    case PciRegs.PCI_CAP_ID_DBG:
                        str = "Debug port";
                        break;
                    case PciRegs.PCI_CAP_ID_CCRC:
                        str = "CompactPCI Central Resource Control";
                        break;
                    case PciRegs.PCI_CAP_ID_SHPC:
                        str = "PCI Standard Hot-Plug Controller";
                        break;
                    case PciRegs.PCI_CAP_ID_SSVID:
                        str = "Bridge subsystem vendor/device ID";
                        break;
                    case PciRegs.PCI_CAP_ID_AGP3:
                        str = "AGP Target PCI-PCI bridge";
                        break;
                    case PciRegs.PCI_CAP_ID_SECDEV:
                        str = "Secure Device";
                        break;
                    case PciRegs.PCI_CAP_ID_EXP:
                        str = "PCI Express";
                        break;
                    case PciRegs.PCI_CAP_ID_MSIX:
                        str = "Extended Message Signalled Interrupts (MSI-X)";
                        break;
                    case PciRegs.PCI_CAP_ID_SATA:
                        str = "SATA Data/Index Conf.";
                        break;
                    case PciRegs.PCI_CAP_ID_AF:
                        str = "PCI Advanced Features";
                        break;
                    default:
                        str = "Unknown";
                        break;
                }

                return str;
            }

            public static String GetExtendedCapabilityStr(DWORD cap_id)
            {
                String str;

                switch (cap_id)
                {
                    case PciRegs.PCI_CAP_LIST_ID:
                        str = "Null Capability";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_ERR:
                        str = "Advanced Error Reporting (AER)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_VC:
                        str = "Virtual Channel (VC)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_DSN:
                        str = "Device Serial Number";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_PWR:
                        str = "Power Budgeting";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_RCLD:
                        str = "Root Complex Link Declaration";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_RCILC:
                        str = "Root Complex Internal Link Control";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_RCEC:
                        str = "Root Complex Event Collector Endpoint Association";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_MFVC:
                        str = "Multi-Function Virtual Channel (MFVC)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_VC9:
                        str = "Virtual Channel (VC)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_RCRB:
                        str = "Root Complex Register Block (RCRB) Header";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_VNDR:
                        str = "Vendor-Specific Extended Capability (VSEC)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_CAC:
                        str = "Configuration Access Correlation (CAC)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_ACS:
                        str = "Access Control Services (ACS)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_ARI:
                        str = "Alternative Routing-ID Interpretation (ARI)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_ATS:
                        str = "Address Translation Services (ATS)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_SRIOV:
                        str = "Single Root I/O Virtualization (SR-IOV)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_MRIOV:
                        str = "Multi-Root I/O Virtualization (MR-IOV)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_MCAST:
                        str = "Multicast";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_PRI:
                        str = "Page Request";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_AMD_XXX:
                        str = "Reserved for AMD";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_REBAR:
                        str = "Resizable BAR";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_DPA:
                        str = "Dynamic Power Allocation (DPA)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_TPH:
                        str = "TLP Processing Hints (TPH)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_LTR:
                        str = "Latency Tolerance Reporting (LTR)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_SECPCI:
                        str = "Secondary PCI Express";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_PMUX:
                        str = "Protocol Multiplexing (PMUX)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_PASID:
                        str = "Process Address Space ID (PASID)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_LNR:
                        str = "LN Requester (LNR)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_DPC:
                        str = "Downstream Port Containment (DPC)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_L1PMS:
                        str = "L1 PM Substates";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_PTM:
                        str = "Precision Time Measurement (PTM)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_MPHY:
                        str = "PCI Express over M-PHY (M-PCIe)";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_FRSQ:
                        str = "FRS Queueing";
                        break;
                    case PciRegs.PCI_EXT_CAP_ID_RTR:
                        str = "Readiness Time Reporting";
                        break;
                    default:
                        str = "Unknown";
                        break;
                }

                return str;
            }
        }
    }
}
