using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

using Jungo.wdapi_dotnet;
using static Jungo.wdapi_dotnet.wdc_lib_decl;
using static Jungo.wdapi_dotnet.wdc_lib_consts;
using static Jungo.wdapi_dotnet.wdc_defs_macros;
using static Jungo.wdapi_dotnet.windrvr_consts;
using static Jungo.diag_lib.wdc_diag_lib;
using static Jungo.diag_lib.wds_diag_lib;
using static Jungo.diag_lib.pci_lib;
using static Jungo.wdapi_dotnet.WD_DMA_OPTIONS;
using static Jungo.wdapi_dotnet.WD_ERROR_CODES;

using DWORD = System.UInt32;

namespace Jungo.diag_lib
{
    static class pci_diag
    {
        public const DWORD PCI_DEFAULT_VENDOR_ID = 0x0;
        public const DWORD PCI_DEFAULT_DEVICE_ID = 0x0;
        private static WDC_DEVICE m_WdcDevice = new();
        private static readonly MENU_CTX_CFG m_menuCtxCfg = new();
        private static readonly MENU_CTX_READ_WRITE_ADDR m_menuReadWriteAddr =
            new()
            {
                AddrSpace = ACTIVE_ADDR_SPACE_NEEDS_INIT,
                Mode = WDC_ADDR_MODE.WDC_MODE_32
            };
        private static readonly MENU_CTX_EVENTS m_menuCtxEvents = new();
        private static readonly MENU_CTX_INTERRUPTS m_menuCtxInterrupts = new();
        private static readonly MENU_CTX_DMA m_menuCtxDMA = new();

        /* @regs_lib_enums@ */

        /* @regs_info@ */

        public class MENU_CTX_CFG
        {
            public readonly IList<WDC_REG> CfgRegs = new List<WDC_REG>();
            public readonly IList<WDC_REG> CfgExpRegs = new List<WDC_REG>();

            public MENU_CTX_CFG()
            {
                InitCfgRegs();
                InitCfgExpRegs();
            }
            private void InitCfgRegs()
            {
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_VID,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "VID",
                    Description = "Vendor ID"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_DID,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "DID",
                    Description = "Device ID"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_CR,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "CMD",
                    Description = "Command"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_SR,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "STS",
                    Description = "Status"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_REV,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "RID_CLCD",
                    Description = "Revision ID & Class Code"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_CCSC,
                    Size = WDC_SIZE_8,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "SCC",
                    Description = "Sub Class Code"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_CCBC,
                    Size = WDC_SIZE_8,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "BCC",
                    Description = "Base Class Code"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_CLSR,
                    Size = WDC_SIZE_8,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "CALN",
                    Description = "Cache Line Size"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_LTR,
                    Size = WDC_SIZE_8,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "LAT",
                    Description = "Latency Timer"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_HDR,
                    Size = WDC_SIZE_8,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "HDR",
                    Description = "Header Type"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_BISTR,
                    Size = WDC_SIZE_8,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "BIST",
                    Description = "Built-in Self Test"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_BAR0,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "BADDR0",
                    Description = "Base Address 0"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_BAR1,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "BADDR1",
                    Description = "Base Address 1"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_BAR2,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "BADDR2",
                    Description = "Base Address 2"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_BAR3,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "BADDR3",
                    Description = "Base Address 3"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_BAR4,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "BADDR4",
                    Description = "Base Address 4"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_BAR5,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "BADDR5",
                    Description = "Base Address 5"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_CIS,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "CIS",
                    Description = "CardBus CISpointer"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_SVID,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "SVID",
                    Description = "Sub-system Vendor ID"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_SDID,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "SDID",
                    Description = "Sub-system Device ID"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_EROM,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "EROM",
                    Description = "Expansion ROM Base Address"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_CAP,
                    Size = WDC_SIZE_8,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "NEW_CAP",
                    Description = "New Capabilities Pointer"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_ILR,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "INTLN",
                    Description = "Interrupt Line"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_IPR,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "INTPIN",
                    Description = "Interrupt Pin"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_MGR,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "MINGNT",
                    Description = "Minimum Required Burst Period"
                });
                CfgRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG.PCI_MLR,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "MAXLAT",
                    Description = "Maximum Latency"
                });
            }
            private void InitCfgExpRegs()
            {
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.PCIE_CAP_ID,
                    Size = WDC_SIZE_8,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "PCIE_CAP_ID",
                    Description = "PCI Express Capability ID"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.NEXT_CAP_PTR,
                    Size = WDC_SIZE_8,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "NEXT_CAP_PTR",
                    Description = "Next Capabiliy Pointer"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.CAP_REG,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "CAP_REG",
                    Description = "Capabilities Register"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.DEV_CAPS,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "DEV_CAPS",
                    Description = "Device Capabilities"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.DEV_CTL,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "DEV_CTL",
                    Description = "Device Control"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.DEV_STS,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "DEV_STS",
                    Description = "Device Status"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.LNK_CAPS,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "LNK_CAPS",
                    Description = "Link Capabilities"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.LNK_CTL,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "LNK_CTL",
                    Description = "Link Control"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.LNK_STS,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "LNK_STS",
                    Description = "Link Status"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.SLOT_CAPS,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "SLOT_CAPS",
                    Description = "Slot Capabilities"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.SLOT_CTL,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "SLOT_CTL",
                    Description = "Slot Control"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.SLOT_STS,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "SLOT_STS",
                    Description = "Slot Status"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.ROOT_CAPS,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "ROOT_CAPS",
                    Description = "Root Capabilities"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.ROOT_CTL,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "ROOT_CTL",
                    Description = "Root Control"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.ROOT_STS,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "ROOT_STS",
                    Description = "Root Status"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.DEV_CAPS2,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "DEV_CAPS2",
                    Description = "Device Capabilities 2"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.DEV_CTL2,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "DEV_CTL2",
                    Description = "Device Control 2"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.DEV_STS2,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "DEV_STS2",
                    Description = "Device Status 2"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.LNK_CAPS2,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "LNK_CAPS2",
                    Description = "Link Capabilities 2"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.LNK_CTL2,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "LNK_CTL2",
                    Description = "Link Control 2"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.LNK_STS2,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "LNK_STS2",
                    Description = "Link Status 2"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.SLOT_CAPS2,
                    Size = WDC_SIZE_32,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "SLOT_CAPS2",
                    Description = "Slot Capabilities 2"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.SLOT_CTL2,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "SLOT_CTL2",
                    Description = "Slot Control 2"
                });
                CfgExpRegs.Add(new()
                {
                    AddrSpace = WDC_AD_CFG_SPACE,
                    Offset = (UInt32)PCI_CFG_REG_EXPRESS.SLOT_STS2,
                    Size = WDC_SIZE_16,
                    Direction = WDC_DIRECTION.WDC_READ_WRITE,
                    Name = "SLOT_STS2",
                    Description = "Slot Status 2"
                });
            }
        }

        public class MENU_CTX_EVENTS
        {
            public bool IsEventEnabled { get; set; }
        }

        public class MENU_CTX_READ_WRITE_ADDR
        {
            public DWORD AddrSpace { get; set; }
            public WDC_ADDR_MODE Mode { get; set; }
            public bool IsBlock { get; set; }
        }

        public class MENU_CTX_INTERRUPTS
        {
            public bool IsIntEnabled { get; set; }
        }

        public class MENU_CTX_DMA
        {
            public DWORD Size { get; set; }
            public DWORD Options { get; set; }
            public UInt64 Address { get; set; }
            public IntPtr pBuf { get; set; } = IntPtr.Zero;
            public IntPtr pDma { get; set; } = IntPtr.Zero;
            public DWORD DmaAddressWidth { get; set; }
        }

        public enum DMA_MENU_OPTION
        {
            MENU_DMA_ALLOCATE_CONTIG,
            MENU_DMA_ALLOCATE_SG,
            MENU_DMA_RESERVED_MEM
        };

        public static WD_ERROR_CODES MainMenuCb(object ctx)
        {
            if (m_WdcDevice.hDev != IntPtr.Zero)
            {
                Console.WriteLine();
                Console.WriteLine("Open device:");
                Console.WriteLine(GetPciDeviceInfo(m_WdcDevice.id,
                    m_WdcDevice.slot));
            }

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES FindAndOpenCb(object ctx)
        {
            DWORD dwVendorId, dwDeviceId;
            WDC_DEVICE wdcDevice = new();

            dwVendorId = Diag.InputDword(true, "Enter vendor ID");

            dwDeviceId = Diag.InputDword(true, "Enter device ID");

            FindAndOpen(ref wdcDevice, dwVendorId, dwDeviceId);
            if (wdcDevice.hDev != IntPtr.Zero)
                m_WdcDevice = wdcDevice;

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES ScanBusCb(object ctx)
        {
            PciDevicesInfoPrintAll();

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuCfgCb(object ctx)
        {

            Console.WriteLine("Configuration registers:");
            Console.WriteLine("------------------------");

            RegsInfoPrint(m_WdcDevice, m_menuCtxCfg.CfgRegs,
                false);

            if (WDC_PciGetExpressGen(m_WdcDevice.hDev) != 0)
            {
                RegsInfoPrint(m_WdcDevice, m_menuCtxCfg.CfgExpRegs,
                    true);
            }

            Console.WriteLine("");

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuCfgReadOffsetOptionCb(object ctx)
        {
            ReadWriteBlock(m_WdcDevice, WDC_DIRECTION.WDC_READ,
                WDC_AD_CFG_SPACE);

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuCfgWriteOffsetOptionCb(object ctx)
        {
            ReadWriteBlock(m_WdcDevice, WDC_DIRECTION.WDC_WRITE,
                WDC_AD_CFG_SPACE);

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuCfgReadAllOptionCb(object ctx)
        {

            ReadRegsAll(m_WdcDevice, m_menuCtxCfg.CfgRegs, true, false);

            if (WDC_PciGetExpressGen(m_WdcDevice.hDev) != 0)
            {
                ReadRegsAll(m_WdcDevice, m_menuCtxCfg.CfgExpRegs,
                    true, true);
            }

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuCfgReadNamedRegOptionCb(object ctx)
        {
            ReadWriteRegFromList(m_WdcDevice, m_menuCtxCfg.CfgRegs,
                WDC_DIRECTION.WDC_READ, true, false);

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuCfgWriteNamedRegOptionCb(object ctx)
        {
            ReadWriteRegFromList(m_WdcDevice, m_menuCtxCfg.CfgRegs,
                WDC_DIRECTION.WDC_WRITE, true, false);

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuCfgReadNamedExpRegOptionCb(object ctx)
        {

            ReadWriteRegFromList(m_WdcDevice, m_menuCtxCfg.CfgRegs,
                WDC_DIRECTION.WDC_READ, true, true);

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuCfgWriteNamedExpRegOptionCb(object ctx)
        {
            ReadWriteRegFromList(m_WdcDevice, m_menuCtxCfg.CfgRegs,
                WDC_DIRECTION.WDC_WRITE, true, true);

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuCfgScanPciCapsOptionCb(object ctx)
        {
            PciScanCaps(m_WdcDevice);

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuCfgScanPciSpecificCapsOptionCb(object ctx)
        {
            DWORD dwCapId = Diag.InputDword(true, "Enter requested capability ID");

            PciScanCaps(m_WdcDevice, dwCapId);

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuCfgScanPciExpressCapsOptionCb(object ctx)
        {
            PciExpressScanCaps(m_WdcDevice);

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuCfgScanPciExpressSpecificCapsOptionCb(object ctx)
        {
            DWORD dwCapId = Diag.InputDword(true, "Enter requested capability ID");

            PciExpressScanCaps(m_WdcDevice, dwCapId);

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuRwAddrCb(object ctx)
        {
            DWORD dwAddrSpace;

            /* Initialize active address space */
            if (WDC_AD_CFG_SPACE == m_menuReadWriteAddr.AddrSpace)
            {
                DWORD dwNumAddrSpaces = m_WdcDevice.dwNumAddrSpaces;

                /* Find the first active address space */
                for (dwAddrSpace = 0; dwAddrSpace < dwNumAddrSpaces;
                    dwAddrSpace++)
                {
                    if (WDC_AddrSpaceIsActive(m_WdcDevice.hDev, dwAddrSpace))
                        break;
                }

                /* Sanity check */
                if (dwAddrSpace == dwNumAddrSpaces)
                {
                    throw new WinDriverException(WD_NO_RESOURCES_ON_DEVICE,
                        "active address spaces");
                }

                m_menuReadWriteAddr.AddrSpace = dwAddrSpace;
            }

            Console.WriteLine();
            Console.WriteLine("Current Read/Write configurations:");
            Console.WriteLine("----------------------------------");
            Console.WriteLine("Currently active address space: "
                + $"BAR {m_menuReadWriteAddr.AddrSpace}");

            string bitsStr = m_menuReadWriteAddr.Mode switch
            {
                WDC_ADDR_MODE.WDC_MODE_8 => "8 bit",
                WDC_ADDR_MODE.WDC_MODE_16 => "16 bit",
                WDC_ADDR_MODE.WDC_MODE_32 => "32 bit",
                _ => "64 bit",
            };

            Console.WriteLine($"Currently active read/write mode: {bitsStr}");
            Console.WriteLine("Currently active transfer type: {0}",
                m_menuReadWriteAddr.IsBlock ?
                "block transfers" : "non-block transfers");
            Console.WriteLine("");

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuRwAddrSetAddrSpace(object ctx)
        {
            DWORD dwAddrSpace;
            DWORD dwNumAddrSpaces = m_WdcDevice.dwNumAddrSpaces;
            DiagMenuOption setActiveAddressSpaceMenu = new();

            Console.WriteLine("");
            Console.WriteLine("Select an active address space:");
            Console.WriteLine("-------------------------------");

            for (dwAddrSpace = 0; dwAddrSpace < dwNumAddrSpaces; dwAddrSpace++)
            {
                ADDR_SPACE_INFO addrSpaceInfo = new()
                {
                    AddrSpace = dwAddrSpace
                };

                GetAddrSpaceInfo(m_WdcDevice, addrSpaceInfo);
                string optionName = $"{addrSpaceInfo.Name}\t\t" +
                    $"{addrSpaceInfo.Type}\t{addrSpaceInfo.Desc}";
                DiagMenuOption addrSpaceMenu = new()
                {
                    OptionName = optionName
                };
                setActiveAddressSpaceMenu.AddChild(addrSpaceMenu);
            }

            dwAddrSpace = (DWORD)DiagMenu.RunOnce(in setActiveAddressSpaceMenu);

            if (!WDC_AddrSpaceIsActive(m_WdcDevice.hDev, dwAddrSpace))
            {
                throw new WinDriverException(WD_INVALID_PARAMETER,
                    "You have selected an inactive address space");
            }

            m_menuReadWriteAddr.AddrSpace = dwAddrSpace;

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuRwAddrChangeModeOptionCb(object ctx)
        {
            SetMode(out WDC_ADDR_MODE mode);

            m_menuReadWriteAddr.Mode = mode;

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuRwAddrChangeTransTypeOptionCb(object ctx)
        {
            m_menuReadWriteAddr.IsBlock = !m_menuReadWriteAddr.IsBlock;

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuRwAddrReadOptionOptionCb(object ctx)
        {
            if (m_menuReadWriteAddr.IsBlock)
            {
                ReadWriteBlock(m_WdcDevice, WDC_DIRECTION.WDC_READ,
                    m_menuReadWriteAddr.AddrSpace);
            }
            else
            {
                ReadWriteFromBar(m_WdcDevice, m_menuReadWriteAddr.AddrSpace,
                    m_menuReadWriteAddr.Mode, WDC_DIRECTION.WDC_READ);
            }

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuRwAddrWriteOptionOptionCb(object ctx)
        {
            if (m_menuReadWriteAddr.IsBlock)
            {
                ReadWriteBlock(m_WdcDevice, WDC_DIRECTION.WDC_WRITE,
                    m_menuReadWriteAddr.AddrSpace);
            }
            else
            {
                ReadWriteFromBar(m_WdcDevice, m_menuReadWriteAddr.AddrSpace,
                    m_menuReadWriteAddr.Mode, WDC_DIRECTION.WDC_WRITE);
            }

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES MenuRwAddrExitCb(object ctx)
        {
            m_menuReadWriteAddr.AddrSpace = ACTIVE_ADDR_SPACE_NEEDS_INIT;

            return WD_STATUS_SUCCESS;
        }

        public static bool MenuIsDeviceNull(DiagMenuOption menu)
        {
            return m_WdcDevice.hDev == IntPtr.Zero;
        }

        public static bool MenuCfgIsDeviceNotExpress(DiagMenuOption menu)
        {
            return WDC_PciGetExpressGen(m_WdcDevice.hDev) == 0;
        }

        static void FindAndOpen(ref WDC_DEVICE wdcDevice,
            DWORD dwVendorId = PCI_DEFAULT_VENDOR_ID,
            DWORD dwDeviceId = PCI_DEFAULT_DEVICE_ID)
        {
            int input = 0;
            WDC_PCI_SCAN_RESULT scanResult;
            scanResult = pci_lib.DeviceFind(dwVendorId, dwDeviceId);

            if (scanResult.dwNumDevices > 1)
            {
                DiagMenuOption findRootMenu = new()
                {
                    TitleName = String.Format("Found {0} matching " +
                    "device{1} [ Vendor ID 0x{2}{3}, Device ID 0x{4}{5} ]:",
                    scanResult.dwNumDevices,
                    scanResult.dwNumDevices > 1 ? "s" : "",
                    dwVendorId, (dwVendorId != 0) ? "" : " (ALL)",
                    dwDeviceId, (dwDeviceId != 0) ? "" : " (ALL)")
                };

                for (DWORD i = 0; i < scanResult.dwNumDevices; i++)
                {
                    string optionName = GetPciDeviceInfo(scanResult.deviceId[i],
                        scanResult.deviceSlot[i]);

                    DiagMenuOption deviceMenu = new()
                    {
                        OptionName = optionName
                    };
                    findRootMenu.AddChild(deviceMenu);
                }
                input = DiagMenu.RunOnce(in findRootMenu);
            }
            if (input != DiagMenu.EXIT_MENU)
            {
                if (wdcDevice.hDev != IntPtr.Zero)
                    pci_lib.DeviceClose(wdcDevice);

                wdcDevice = pci_lib.PCI_DeviceOpen(scanResult.deviceSlot[input],
                    Kernel_plugin.KP_PCI_DRIVER_NAME);

                if (wdcDevice.hDev != IntPtr.Zero && WDC_IS_KP(wdcDevice))
                    Kernel_plugin.CheckKPDriverVer(wdcDevice);
            }
        }

        static void PCI_Init(out WDC_DEVICE wdcDevice)
        {
            wdcDevice = new();

            pci_lib.PCI_LibInit();

            WDC_Trace($"WinDriver user mode version {windrvr_decl.WD_VERSION}");

#pragma warning disable CS0162 // Unreachable code
            if (PCI_DEFAULT_VENDOR_ID != 0)
            {
                FindAndOpen(ref wdcDevice);
                m_WdcDevice = wdcDevice;
            }
#pragma warning restore CS0162 // Unreachable code
        }

        public static void MenuCommonScanBusInit(DiagMenuOption menuRoot)
        {
            DiagMenuOption MenuScanBusOption = new()
            {
                OptionName = "Scan PCI bus",
                CbEntry = ScanBusCb
            };

            menuRoot.AddChild(MenuScanBusOption);
        }

        public static void MenuDeviceOpenInit(DiagMenuOption menuRoot)
        {
            DiagMenuOption MenuScanBusOption = new()
            {
                OptionName = "Find and open a PCI device",
                CbEntry = FindAndOpenCb
            };

            menuRoot.AddChild(MenuScanBusOption);
        }

        public static void InitScanCapabilitiesChildrenMenu(
            DiagMenuOption scanCapabilitiesMenu)
        {
            DiagMenuOption scanPciCapabilitiesMenu = new()
            {
                OptionName = "Scan PCI capabilities",
                CbEntry = MenuCfgScanPciCapsOptionCb
            };
            DiagMenuOption scanSpecificPciCapabilityMenu = new()
            {
                OptionName = "Scan specific PCI capability",
                CbEntry = MenuCfgScanPciSpecificCapsOptionCb
            };
            DiagMenuOption scanPciExpressExtendedCapabilities = new()
            {
                OptionName = "Scan PCI Express extended capabilities",
                CbEntry = MenuCfgScanPciExpressCapsOptionCb
            };
            DiagMenuOption scanSpecificPciExpressExtendedCapabilityMenu = new()
            {
                OptionName = "Scan specific PCI Express extended capability",
                CbEntry = MenuCfgScanPciExpressSpecificCapsOptionCb
            };

            IList<DiagMenuOption> mscanCapabilitiesChildren =
                new List<DiagMenuOption>()
            {
                scanPciCapabilitiesMenu,
                scanSpecificPciCapabilityMenu,
                scanPciExpressExtendedCapabilities,
                scanSpecificPciExpressExtendedCapabilityMenu,
            };
            scanCapabilitiesMenu.AddChildren(mscanCapabilitiesChildren);
        }

        public static void MenuCfgInit(DiagMenuOption menuRoot)
        {
            DiagMenuOption menuCfgMenuOption = new()
            {
                OptionName = "Read/write the PCI configuration space",
                TitleName = "Read/write the device's configuration space",
                CbEntry = MenuCfgCb,
                CbIsHidden = MenuIsDeviceNull
            };

            DiagMenuOption readOffsetMenu = new()
            {
                OptionName = "Read from an offset",
                CbEntry = MenuCfgReadOffsetOptionCb
            };

            DiagMenuOption writeOffsetMenu = new()
            {
                OptionName = "Write to an offset",
                CbEntry = MenuCfgWriteOffsetOptionCb
            };

            DiagMenuOption readAllConfigurationRegistersMenu = new()
            {
                OptionName = "Read all configuration registers " +
                    "defined for the device (see list above)",
                CbEntry = MenuCfgReadAllOptionCb
            };

            DiagMenuOption readNamedRegisterMenu = new()
            {
                OptionName = "Read from a named register",
                CbEntry = MenuCfgReadNamedRegOptionCb
            };

            DiagMenuOption writeNamedRegisterMenu = new()
            {
                OptionName = "Write to a named register",
                CbEntry = MenuCfgWriteNamedRegOptionCb
            };

            DiagMenuOption readPcieNamedRegisterMenu = new()
            {
                OptionName = "Read from a named PCI Express register",
                CbEntry = MenuCfgReadNamedExpRegOptionCb,
                CbIsHidden = MenuCfgIsDeviceNotExpress
            };

            DiagMenuOption writePcieNamedRegisterMenu = new()
            {
                OptionName = "Write to a named PCI Express register",
                CbEntry = MenuCfgWriteNamedExpRegOptionCb,
                CbIsHidden = MenuCfgIsDeviceNotExpress
            };

            DiagMenuOption scanCapabilitiesMenu = new()
            {
                OptionName = "Scan PCI/PCIe capabilities",
            };
            InitScanCapabilitiesChildrenMenu(scanCapabilitiesMenu);

            IList<DiagMenuOption> menuCfgChildren = new List<DiagMenuOption>()
            {
                readOffsetMenu,
                writeOffsetMenu,
                readAllConfigurationRegistersMenu,
                readNamedRegisterMenu,
                writeNamedRegisterMenu,
                readPcieNamedRegisterMenu,
                writePcieNamedRegisterMenu,
                scanCapabilitiesMenu
            };

            menuCfgMenuOption.AddChildren(menuCfgChildren);
            menuRoot.AddChild(menuCfgMenuOption);
        }

        public static bool MenuEventsAreEnabled(DiagMenuOption menu)
        {
            return m_menuCtxEvents.IsEventEnabled;
        }

        public static bool MenuEventsAreDisabled(DiagMenuOption menu)
        {
            return !m_menuCtxEvents.IsEventEnabled;
        }

        private static WD_ERROR_CODES EventsEnableCb(object ctx)
        {
            IntPtr pData = IntPtr.Zero;

            PCI_EventRegister(m_WdcDevice, DiagIntHandler);
            m_menuCtxEvents.IsEventEnabled = true;

            Console.WriteLine("Interrupts enabled");

            return WD_STATUS_SUCCESS;
        }

        private static WD_ERROR_CODES EventsDisableCb(object ctx)
        {
            PCI_EventUnregister(m_WdcDevice);
            m_menuCtxEvents.IsEventEnabled = false;

            Console.WriteLine("Interrupts disabled");

            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES EventsMenuEntryCb(object ctx)
        {
            m_menuCtxEvents.IsEventEnabled = PCI_EventIsRegistered(m_WdcDevice);

#if WIN32
            if (!m_menuCtxEvents.IsEventEnabled)
            {
                Console.WriteLine("NOTICE: An INF must be installed for your device in order to");
                Console.WriteLine("\tcall your user-mode event handler.");
                Console.WriteLine("\tYou can generate an INF file using the DriverWizard.");
            }
#endif

            return WD_STATUS_SUCCESS;
        }

        public static void MenuEventsInit(DiagMenuOption menuRoot)
        {
            DiagMenuOption menuEventsMenuOption = new()
            {
                OptionName = "Register/unregister plug-and-play " +
                 "and power management events",
                TitleName = "Plug-and-play and power management events",
                CbIsHidden = MenuIsDeviceNull,
                CbEntry = EventsMenuEntryCb,
            };
            DiagMenuOption MenuEventsEnableOption = new()
            {
                OptionName = "Register Events",
                CbEntry = EventsEnableCb,
                CbIsHidden = MenuEventsAreEnabled
            };
            DiagMenuOption MenuEventsDisableOption = new()
            {
                OptionName = "Unregister Events",
                CbEntry = EventsDisableCb,
                CbIsHidden = MenuEventsAreDisabled
            };

            IList<DiagMenuOption> menuEventsChildren = new List<DiagMenuOption>()
            {
                MenuEventsEnableOption,
                MenuEventsDisableOption
            };

            menuEventsMenuOption.AddChildren(menuEventsChildren);
            menuRoot.AddChild(menuEventsMenuOption);
        }

        public static void MenuReadWriteAddrInit(DiagMenuOption menuRoot)
        {
            DiagMenuOption menuReadWriteAddrMenuOption = new()
            {
                OptionName = "Read/write memory and I/O addresses on the device",
                TitleName = "Read/write the device's memory and I/O ranges",
                CbIsHidden = MenuIsDeviceNull,
                CbEntry = MenuRwAddrCb,
                CbExit = MenuRwAddrExitCb,
            };

            DiagMenuOption changeAddressSpaceMenu = new()
            {
                OptionName = "Change active address space for read/write",
                CbEntry = MenuRwAddrSetAddrSpace,
            };
            DiagMenuOption changeIoModeMenu = new()
            {
                OptionName = "Change active read/write mode",
                CbEntry = MenuRwAddrChangeModeOptionCb,
            };
            DiagMenuOption toggleTransferTypeMenu = new()
            {
                OptionName = "Toggle active transfer type",
                CbEntry = MenuRwAddrChangeTransTypeOptionCb,
            };
            DiagMenuOption readAddressSpaceMenu = new()
            {
                OptionName = "Read from active address space",
                CbEntry = MenuRwAddrReadOptionOptionCb,
            };
            DiagMenuOption writeAddressSpaceMenu = new()
            {
                OptionName = "Write to active address space",
                CbEntry = MenuRwAddrWriteOptionOptionCb,
            };

            IList<DiagMenuOption> menuReadWriteAddrChildren =
                new List<DiagMenuOption>()
                {
                    changeAddressSpaceMenu,
                    changeIoModeMenu,
                    toggleTransferTypeMenu,
                    readAddressSpaceMenu,
                    writeAddressSpaceMenu,
                };

            menuReadWriteAddrMenuOption.AddChildren(menuReadWriteAddrChildren);
            menuRoot.AddChild(menuReadWriteAddrMenuOption);
        }

        private static void DiagIntHandler(object ctx)
        {
            /* TODO: You can modify this function in order to implement your own
             diagnostics interrupt handler routine */

            Console.WriteLine($"Got interrupt number {m_WdcDevice.Int.dwCounter}");
            Console.WriteLine($"Interrupt Type:" +
                $"{IntTypeDescriptionGet(m_WdcDevice.Int.dwEnabledIntType)}");

            if (WDC_INT_IS_MSI(m_WdcDevice.Int.dwEnabledIntType))
                Console.WriteLine("Message Data: 0x{0:x}", m_WdcDevice.Int.dwLastMessage);
        }

        public static bool MenuInterruptsAreEnabled(DiagMenuOption menu)
        {
            return m_menuCtxInterrupts.IsIntEnabled;
        }

        public static bool MenuInterruptsAreDisabled(DiagMenuOption menu)
        {
            return !m_menuCtxInterrupts.IsIntEnabled;
        }

        private static WD_ERROR_CODES InterruptsMenuEntryCb(object ctx)
        {
            DWORD dwIntOptions = m_WdcDevice.Int.dwOptions;
            bool fIsMsi = WDC_INT_IS_MSI(dwIntOptions);

            if ((dwIntOptions & (DWORD)WD_INTERRUPT_TYPE.INTERRUPT_LEVEL_SENSITIVE) == 1 &&
                !m_menuCtxInterrupts.IsIntEnabled)
            {
                /* TODO: You can remove this message after you have modified the
                   implementation of PCI_IntEnable() in pci_lib.cs to correctly
                   acknowledge level-sensitive interrupts (see guidelines in
                   PCI_IntEnable()). */
                Console.WriteLine("");
                Console.WriteLine("WARNING!!!");
                Console.WriteLine("----------");
                Console.WriteLine("Your hardware has level sensitive interrupts.");
                Console.WriteLine("Before enabling the interrupts, {0} first modify the source code",
                    fIsMsi ? "it is recommended that" : "you must");
                Console.WriteLine("of PCI_IntEnable(), in the file pci_lib.cs, to correctly acknowledge");
                Console.WriteLine("{0} interrupts when they occur, as dictated by" +
                    "the hardware's specification", fIsMsi ? "level sensitive" : "");
                Console.WriteLine("");
            }

            return WD_STATUS_SUCCESS;
        }

        private static WD_ERROR_CODES InterruptsEnableCb(object ctx)
        {
            IntPtr pData = IntPtr.Zero;

            PCI_IntEnable(m_WdcDevice, DiagIntHandler, pData);
            m_menuCtxInterrupts.IsIntEnabled = true;

            Console.WriteLine("Interrupts enabled");

            return WD_STATUS_SUCCESS;
        }

        private static WD_ERROR_CODES InterruptsDisableCb(object ctx)
        {
            PCI_IntDisable(m_WdcDevice);
            m_menuCtxInterrupts.IsIntEnabled = false;

            Console.WriteLine("Interrupts disabled");

            return WD_STATUS_SUCCESS;
        }

        public static void MenuInterruptsInit(DiagMenuOption menuRoot)
        {
            DiagMenuOption menuInterruptsMenuOption = new()
            {
                OptionName = "Enable/disable the device's interrupts",
                TitleName = "Interrupts",
                CbIsHidden = MenuIsDeviceNull,
                CbEntry = InterruptsMenuEntryCb
            };
            DiagMenuOption MenuInterruptsEnableOption = new()
            {
                OptionName = "Enable interrupts",
                CbEntry = InterruptsEnableCb,
                CbIsHidden = MenuInterruptsAreEnabled
            };
            DiagMenuOption MenuInterruptsDisableOption = new()
            {
                OptionName = "Disable interrupts",
                CbEntry = InterruptsDisableCb,
                CbIsHidden = MenuInterruptsAreDisabled
            };

            IList<DiagMenuOption> menuInterruptsChildren = new List<DiagMenuOption>()
            {
                MenuInterruptsEnableOption,
                MenuInterruptsDisableOption
            };

            menuInterruptsMenuOption.AddChildren(menuInterruptsChildren);
            menuRoot.AddChild(menuInterruptsMenuOption);
        }

        public static void DmaGetInput(DMA_MENU_OPTION option)
        {
            if (option == DMA_MENU_OPTION.MENU_DMA_RESERVED_MEM)
            {
#if WIN32
                Console.WriteLine("Warning: The address for the reserved memory should be " +
                    "calculated according to the values listed in registry key");
                Console.WriteLine("HKLM/HARDWARE/RESOURCEMAP/System Resources/Physical Memory.");
                Console.WriteLine("Any other address may result in a BSOD. For more details " +
                    "please refer to Tech Doc #129");
                Console.WriteLine();
#endif

                Console.Write("Enter reserved memory address (64 bit hex uint): 0x");
                m_menuCtxDMA.Address = Diag.InputQword(true);
            }

            m_menuCtxDMA.Size = Diag.InputDword(false, "Enter memory allocation size in bytes");

            if (option == DMA_MENU_OPTION.MENU_DMA_ALLOCATE_CONTIG)
            {
                Console.WriteLine("Enter DMA address width of an address " +
                    "that your device supports, use 0 for default value " +
                    "(32 bit uint)");
                m_menuCtxDMA.DmaAddressWidth = Diag.InputDword(false);

                m_menuCtxDMA.Options = (DWORD)DMA_KBUF_ALLOC_SPECIFY_ADDRESS_WIDTH |
                    (m_menuCtxDMA.DmaAddressWidth <<
                        DMA_OPTIONS_ADDRESS_WIDTH_SHIFT);
            }

            /* Free DMA memory before trying the new allocation */
            DmaFreeCb();
        }

        public static bool MenuDmaIsContigBufferNotExists(DiagMenuOption menu)
        {
            WD_DMA dma;
            if (m_menuCtxDMA.pDma == IntPtr.Zero)
                return true;

            dma = Marshal.PtrToStructure<WD_DMA>(m_menuCtxDMA.pDma);
            return (dma.dwOptions & (DWORD)DMA_KERNEL_BUFFER_ALLOC) == 0;
        }

        public static WD_ERROR_CODES DmaAllocContigCb(object ctx)
        {
            WD_DMA dma;

            DmaGetInput(DMA_MENU_OPTION.MENU_DMA_ALLOCATE_CONTIG);
            PCI_DmaAllocContig(m_WdcDevice.hDev, out IntPtr pBuf,
                m_menuCtxDMA.Options, m_menuCtxDMA.Size, out IntPtr pDma);

            m_menuCtxDMA.pBuf = pBuf;
            m_menuCtxDMA.pDma = pDma;
            dma = Marshal.PtrToStructure<WD_DMA>(m_menuCtxDMA.pDma);

            Console.WriteLine("Contiguous memory allocated. user addr [{0:x}], " +
                "physical addr [0x{1:x}], size [{2}(0x{3:x})]",
                m_menuCtxDMA.pBuf,
                dma.Page[0].pPhysicalAddr,
                dma.Page[0].dwBytes,
                dma.Page[0].dwBytes);

            return WD_STATUS_SUCCESS;

        }

        public static WD_ERROR_CODES DmaAllocSgCb(object ctx)
        {
            WD_DMA dma;

            DmaGetInput(DMA_MENU_OPTION.MENU_DMA_ALLOCATE_SG);
            pci_lib.PCI_DmaAllocSg(m_WdcDevice.hDev, out IntPtr pBuf, m_menuCtxDMA.Size,
               out IntPtr pDma);

            m_menuCtxDMA.pBuf = pBuf;
            m_menuCtxDMA.pDma = pDma;
            dma = Marshal.PtrToStructure<WD_DMA>(m_menuCtxDMA.pDma);
            Console.WriteLine("SG memory allocated. user addr [{0:x}], size [{1}]",
                m_menuCtxDMA.pBuf, m_menuCtxDMA.Size);

            Console.WriteLine("Pages physical addresses:");
            for (DWORD i = 0; i < dma.dwPages; i++)
            {
                Console.WriteLine("({0}) physical addr [0x{1:x}], " +
                    "size [{2}(0x{3:x})]", i + 1,
                    dma.Page[i].pPhysicalAddr,
                    dma.Page[i].dwBytes,
                    dma.Page[i].dwBytes);
            }

            return WD_STATUS_SUCCESS;

        }

        public static WD_ERROR_CODES DmaUseReservedCb(object ctx)
        {
            WD_DMA dma;

            DmaGetInput(DMA_MENU_OPTION.MENU_DMA_RESERVED_MEM);
            PCI_DmaAllocReserved(m_WdcDevice.hDev, m_menuCtxDMA.Address,
                out IntPtr pBuf, m_menuCtxDMA.Size, out IntPtr pDma);

            m_menuCtxDMA.pBuf = pBuf;
            m_menuCtxDMA.pDma = pDma;
            dma = Marshal.PtrToStructure<WD_DMA>(m_menuCtxDMA.pDma);

            Console.WriteLine("Reserved memory claimed. user addr [{0}], " +
                "physical addr [0x{1:x}], size [{2}(0x{3:x})]",
                m_menuCtxDMA.pBuf,
                dma.Page[0].pPhysicalAddr,
                dma.Page[0].dwBytes,
                dma.Page[0].dwBytes);

            return WD_STATUS_SUCCESS;

        }

        public static WD_ERROR_CODES MenuDmaSendSharedBufOptionCb(object ctx)
        {
            WDS_DIAG_IpcSendDmaContigToGroup(m_menuCtxDMA.pDma);
            return WD_STATUS_SUCCESS;
        }

        public static WD_ERROR_CODES DmaFreeCb(object ctx = null)
        {
            PCI_DmaFree(m_menuCtxDMA.pDma, m_menuCtxDMA.pBuf);
            m_menuCtxDMA.pDma = IntPtr.Zero;
            m_menuCtxDMA.pBuf = IntPtr.Zero;

            Console.WriteLine("DMA memory freed");

            return WD_STATUS_SUCCESS;
        }

        public static void MenuDmaInit(DiagMenuOption menuRoot)
        {
            DiagMenuOption menuDmaMenuOption = new()
            {
                OptionName = "Allocate/free memory for DMA",
                TitleName = "DMA memory",
                CbIsHidden = MenuIsDeviceNull,
                CbExit = DmaFreeCb
            };
            DiagMenuOption MenuAllocateContigOption = new()
            {
                OptionName = "Allocate contiguous memory",
                CbEntry = DmaAllocContigCb
            };
            DiagMenuOption MenuAllocateSgOption = new()
            {
                OptionName = "Allocate scatter-gather memory",
                CbEntry = DmaAllocSgCb
            };
            DiagMenuOption MenuUseReservedMemoryOption = new()
            {
                OptionName = "Use reserved memory",
                CbEntry = DmaUseReservedCb
            };
            DiagMenuOption MenuDmaSendSharedBufOption = new()
            {
                OptionName = "Send buffer through IPC to all group processes",
                CbEntry = MenuDmaSendSharedBufOptionCb,
                CbIsHidden = MenuDmaIsContigBufferNotExists
            };
            DiagMenuOption MenuFreeDmaMemoryOption = new()
            {
                OptionName = "Free DMA memory",
                CbEntry = DmaFreeCb
            };

            IList<DiagMenuOption> menuDmaChildren = new List<DiagMenuOption>()
            {
                MenuAllocateContigOption,
                MenuAllocateSgOption,
                MenuUseReservedMemoryOption,
                MenuDmaSendSharedBufOption,
                MenuFreeDmaMemoryOption
            };

            menuDmaMenuOption.AddChildren(menuDmaChildren);
            menuRoot.AddChild(menuDmaMenuOption);
        }

        public static class Kernel_plugin
        {
            public const string KP_PCI_DRIVER_NAME = "KP_PCI";

            public enum KP_PCI_MSG
            {
                GET_VERSION = 1, /* Query the version of the Kernel PlugIn */
            };

            public enum KP_PCI_STATUS
            {
                OK = 0x1,
                MSG_NO_IMPL = 0x1000,
                FAIL = 0x1001,
            };

            [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
            public struct KP_PCI_VERSION
            {
                public DWORD dwVer;

                [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 100)]
                public string cVer;
            }

            public static DWORD CheckKPDriverVer(WDC_DEVICE wdcDevice)
            {
                DWORD dwStatus;
                DWORD dwKPResult = 0;
                KP_PCI_VERSION kpVer = new();

                IntPtr pData = Marshal.AllocHGlobal(sizeof(DWORD) + 100);
                Marshal.StructureToPtr(kpVer, pData, false);

                /* Get Kernel PlugIn Driver version */
                dwStatus = WDC_CallKerPlug(wdcDevice.hDev,
                    (uint)KP_PCI_MSG.GET_VERSION, pData, ref dwKPResult);
                if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
                {
                    throw new WinDriverException(dwStatus,
                            $"Failed sending a {KP_PCI_MSG.GET_VERSION} " +
                            $"message [0x{KP_PCI_MSG.GET_VERSION:x}] to the " +
                            $"Kernel-PlugIn driver {KP_PCI_DRIVER_NAME}");
                }

                if ((DWORD)KP_PCI_STATUS.OK != dwKPResult)
                {
                    throw new WinDriverException(WD_INCORRECT_VERSION,
                        $"Kernel-PlugIn {KP_PCI_MSG.GET_VERSION} " +
                        $"message [0x{KP_PCI_MSG.GET_VERSION:x}] failed. " +
                        $"Kernel-PlugIn status [0x%{dwKPResult}:x]");
                }

                Console.WriteLine($"Using {KP_PCI_DRIVER_NAME} Kernel-Plugin " +
                    $"driver version [{kpVer.dwVer:0.00} - {kpVer.cVer}]");

                return dwStatus;
            }
        }

        /* @regs_menu_rw_func@ */

        static void Main(string[] args)
        {
            Console.WriteLine("PCI diagnostic utility.");
            Console.WriteLine("Application accesses hardware using WinDriver.");
            Console.WriteLine($"and a Kernel PlugIn driver ({Kernel_plugin.KP_PCI_DRIVER_NAME}).");

            try
            {
                PCI_Init(out WDC_DEVICE wdcDevice);

                DiagMenuOption menuRoot = new()
                {
                    TitleName = "PCI main menu",
                    CbEntry = MainMenuCb
                };

                MenuCommonScanBusInit(menuRoot);
                MenuDeviceOpenInit(menuRoot);
                MenuSharedBufferInit(menuRoot);
                MenuIpcInit(menuRoot);
                MenuCfgInit(menuRoot);
                MenuEventsInit(menuRoot);
                MenuReadWriteAddrInit(menuRoot);
                MenuInterruptsInit(menuRoot);
                MenuDmaInit(menuRoot);

                /* @regs_menu_init@ */

                DiagMenu.Run(menuRoot);
            }
            catch (Exception e)
            {
                Console.WriteLine("{0} Exception caught.", e);
            }
        }
    }
}


