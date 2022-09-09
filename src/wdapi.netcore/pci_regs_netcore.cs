/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

namespace Jungo
{
    namespace wdapi_dotnet
    {
        public class PciRegs
        {
            /* PCI basic and extended capability lists last updated from-
             *     PCI Code and ID Assignment Specification Revision 1.5 */
            /* Some of the following strings and macros are taken from
             * include/uapi/linux/pci_regs.h in Linux Kernel source */
            /* Capability lists */
            public const int PCI_CAP_LIST_ID = 0;     /**<  Capability ID */
            public const int PCI_CAP_ID_PM = 0x01;  /**<  Power Management */
            public const int PCI_CAP_ID_AGP = 0x02;  /**<  Accelerated Graphics Port */
            public const int PCI_CAP_ID_VPD = 0x03;  /**<  Vital Product Data */
            public const int PCI_CAP_ID_SLOTID = 0x04; /**<  Slot Identification */
            public const int PCI_CAP_ID_MSI = 0x05;  /**<  Message Signalled Interrupts */
            public const int PCI_CAP_ID_CHSWP = 0x06;  /**<  CompactPCI HotSwap */
            public const int PCI_CAP_ID_PCIX = 0x07;  /**<  PCI-X */
            public const int PCI_CAP_ID_HT = 0x08;  /**<  HyperTransport */
            public const int PCI_CAP_ID_VNDR = 0x09;  /**<  Vendor-Specific */
            public const int PCI_CAP_ID_DBG = 0x0A;  /**<  Debug port */
            public const int PCI_CAP_ID_CCRC = 0x0B;  /**<  CompactPCI Central Resource Control */
            public const int PCI_CAP_ID_SHPC = 0x0C;  /**<  PCI Standard Hot-Plug Controller */
            public const int PCI_CAP_ID_SSVID = 0x0D;  /**<  Bridge subsystem vendor/device ID */
            public const int PCI_CAP_ID_AGP3 = 0x0E;  /**<  AGP Target PCI-PCI bridge */
            public const int PCI_CAP_ID_SECDEV = 0x0F;  /**<  Secure Device */
            public const int PCI_CAP_ID_EXP = 0x10;  /**<  PCI Express */
            public const int PCI_CAP_ID_MSIX = 0x11;  /**<  MSI-X */
            public const int PCI_CAP_ID_SATA = 0x12;  /**<  SATA Data/Index Conf. */
            public const int PCI_CAP_ID_AF = 0x13;  /**<  PCI Advanced Features */
            public const int PCI_CAP_LIST_NEXT = 1;     /**<  Next capability in the list */


            public const int PCI_EXT_CAP_ID_ERR = 0x0001; /**<  Advanced Error Reporting */
            public const int PCI_EXT_CAP_ID_VC = 0x0002;  /**<  Virtual Channel Capability */
            public const int PCI_EXT_CAP_ID_DSN = 0x0003;  /**<  Device Serial Number */
            public const int PCI_EXT_CAP_ID_PWR = 0x0004;  /**<  Power Budgeting */
            public const int PCI_EXT_CAP_ID_RCLD = 0x0005;  /**<  Root Complex Link Declaration */
            public const int PCI_EXT_CAP_ID_RCILC = 0x0006;  /**<  Root Complex Internal Link Control */
            public const int PCI_EXT_CAP_ID_RCEC = 0x0007;  /**<  Root Complex Event Collector */
            public const int PCI_EXT_CAP_ID_MFVC = 0x0008;  /**<  Multi-Function VC Capability */
            public const int PCI_EXT_CAP_ID_VC9 = 0x0009;  /**<  same as _VC */
            public const int PCI_EXT_CAP_ID_RCRB = 0x000A;  /**<  Root Complex RB? */
            public const int PCI_EXT_CAP_ID_VNDR = 0x000B;  /**<  Vendor-Specific */
            public const int PCI_EXT_CAP_ID_CAC = 0x000C;  /**<  Config Access - obsolete */
            public const int PCI_EXT_CAP_ID_ACS = 0x000D;  /**<  Access Control Services */
            public const int PCI_EXT_CAP_ID_ARI = 0x000E;  /**<  Alternate Routing ID */
            public const int PCI_EXT_CAP_ID_ATS = 0x000F;  /**<  Address Translation Services */
            public const int PCI_EXT_CAP_ID_SRIOV = 0x0010;  /**<  Single Root I/O Virtualization */
            public const int PCI_EXT_CAP_ID_MRIOV = 0x0011;  /**<  Multi Root I/O Virtualization */
            public const int PCI_EXT_CAP_ID_MCAST = 0x0012;  /**<  Multicast */
            public const int PCI_EXT_CAP_ID_PRI = 0x0013;  /**<  Page Request Interface */
            public const int PCI_EXT_CAP_ID_AMD_XXX = 0x0014;  /**<  Reserved for AMD */
            public const int PCI_EXT_CAP_ID_REBAR = 0x0015;  /**<  Resizable BAR */
            public const int PCI_EXT_CAP_ID_DPA = 0x0016;  /**<  Dynamic Power Allocation */
            public const int PCI_EXT_CAP_ID_TPH = 0x0017;  /**<  TPH Requester */
            public const int PCI_EXT_CAP_ID_LTR = 0x0018;  /**<  Latency Tolerance Reporting */
            public const int PCI_EXT_CAP_ID_SECPCI = 0x0019;  /**<  Secondary PCIe Capability */
            public const int PCI_EXT_CAP_ID_PMUX = 0x001A;  /**<  Protocol Multiplexing */
            public const int PCI_EXT_CAP_ID_PASID = 0x001B;  /**<  Process Address Space ID */
            public const int PCI_EXT_CAP_ID_LNR = 0x001C;  /**<  LN Requester (LNR) */
            public const int PCI_EXT_CAP_ID_DPC = 0x001D;  /**<  Downstream Port Containment (DPC) */
            public const int PCI_EXT_CAP_ID_L1PMS = 0x001E;  /**<  L1 PM Substates */
            public const int PCI_EXT_CAP_ID_PTM = 0x001F;  /**<  Precision Time Measurement (PTM) */
            public const int PCI_EXT_CAP_ID_MPHY = 0x0020;  /**<  PCI Express over M-PHY (M-PCIe) */
            public const int PCI_EXT_CAP_ID_FRSQ = 0x0021;  /**<  FRS Queueing */
            public const int PCI_EXT_CAP_ID_RTR = 0x0022;  /**<  Readiness Time Reporting */
        }

        /* -----------------------------------------------
            PCI configuration registers offsets
           ----------------------------------------------- */
        public enum PCI_CONFIG_REGS_OFFSET {
            PCI_VID = 0x00, /**<  Vendor ID */
            PCI_DID = 0x02, /**<  Device ID */
            PCI_CR = 0x04, /**<  Command register */
            PCI_SR = 0x06, /**<  Status register */
            PCI_REV = 0x08, /**<  Revision ID */
            PCI_CCR = 0x09, /**<  Class code */
            PCI_CCSC = 0x0a, /**<  Sub class code */
            PCI_CCBC = 0x0b, /**<  Base class code */
            PCI_CLSR = 0x0c, /**<  Cache line size */
            PCI_LTR = 0x0d, /**<  Latency timer */
            PCI_HDR = 0x0e, /**<  Header type */
            PCI_BISTR = 0x0f, /**<  Built-in self test */
            PCI_BAR0 = 0x10, /**<  Base address register */
            PCI_BAR1 = 0x14, /**<  Base address register */
            PCI_BAR2 = 0x18, /**<  Base address register */
            PCI_BAR3 = 0x1c, /**<  Base address register */
            PCI_BAR4 = 0x20, /**<  Base address register */
            PCI_BAR5 = 0x24, /**<  Base address register */
            PCI_CIS = 0x28, /**<  CardBus CIS pointer */
            PCI_SVID = 0x2c, /**<  Sub-system vendor ID */
            PCI_SDID = 0x2e, /**<  Sub-system device ID */
            PCI_EROM = 0x30, /**<  Expansion ROM base address */
            PCI_CAP = 0x34, /**<  New capability pointer */
            PCI_ILR = 0x3c, /**<  Interrupt line */
            PCI_IPR = 0x3d, /**<  Interrupt pin */
            PCI_MGR = 0x3e, /**<  Minimum required burst period */
            PCI_MLR = 0x3f  /**<  Maximum latency - How often device must gain PCI bus
                       * access */
        }

        public enum PCIE_CONFIG_REGS_OFFSET {
            PCIE_CAP_ID = 0x0,
            NEXT_CAP_PTR = 0x1,
            CAP_REG = 0x2,
            DEV_CAPS = 0x4,
            DEV_CTL = 0x8,
            DEV_STS = 0xa,
            LNK_CAPS = 0xc,
            LNK_CTL = 0x10,
            LNK_STS = 0x12,
            SLOT_CAPS = 0x14,
            SLOT_CTL = 0x18,
            SLOT_STS = 0x1a,
            ROOT_CAPS = 0x1c,
            ROOT_CTL = 0x1e,
            ROOT_STS = 0x20,
            DEV_CAPS2 = 0x24,
            DEV_CTL2 = 0x28,
            DEV_STS2 = 0x2a,
            LNK_CAPS2 = 0x2c,
            LNK_CTL2 = 0x30,
            LNK_STS2 = 0x32,
            SLOT_CAPS2 = 0x34,
            SLOT_CTL2 = 0x38,
            SLOT_STS2 = 0x3a
        }

        /** PCI base address spaces (BARs) */
        public enum AD_PCI_BAR {
            AD_PCI_BAR0 = 0,
            AD_PCI_BAR1 = 1,
            AD_PCI_BAR2 = 2,
            AD_PCI_BAR3 = 3,
            AD_PCI_BAR4 = 4,
            AD_PCI_BAR5 = 5,
            AD_PCI_BARS = 6
        }

        public enum PCI_CFG_REG
        {
            PCI_VID = PCI_CONFIG_REGS_OFFSET.PCI_VID, /* Vendor ID */
            PCI_DID = PCI_CONFIG_REGS_OFFSET.PCI_DID, /* Device ID */
            PCI_CR = PCI_CONFIG_REGS_OFFSET.PCI_CR, /* Command register */
            PCI_SR = PCI_CONFIG_REGS_OFFSET.PCI_SR, /* Status register */
            PCI_REV = PCI_CONFIG_REGS_OFFSET.PCI_REV, /* Revision ID */
            PCI_CCR = PCI_CONFIG_REGS_OFFSET.PCI_CCR, /* Class code */
            PCI_CCSC = PCI_CONFIG_REGS_OFFSET.PCI_CCSC, /* Sub class code */
            PCI_CCBC = PCI_CONFIG_REGS_OFFSET.PCI_CCBC, /* Base class code */
            PCI_CLSR = PCI_CONFIG_REGS_OFFSET.PCI_CLSR, /* Cache line size */
            PCI_LTR = PCI_CONFIG_REGS_OFFSET.PCI_LTR, /* Latency timer */
            PCI_HDR = PCI_CONFIG_REGS_OFFSET.PCI_HDR, /* Header type */
            PCI_BISTR = PCI_CONFIG_REGS_OFFSET.PCI_BISTR, /* Built-in self test */
            PCI_BAR0 = PCI_CONFIG_REGS_OFFSET.PCI_BAR0, /* Base address register */
            PCI_BAR1 = PCI_CONFIG_REGS_OFFSET.PCI_BAR1, /* Base address register */
            PCI_BAR2 = PCI_CONFIG_REGS_OFFSET.PCI_BAR2, /* Base address register */
            PCI_BAR3 = PCI_CONFIG_REGS_OFFSET.PCI_BAR3, /* Base address register */
            PCI_BAR4 = PCI_CONFIG_REGS_OFFSET.PCI_BAR4, /* Base address register */
            PCI_BAR5 = PCI_CONFIG_REGS_OFFSET.PCI_BAR5, /* Base address register */
            PCI_CIS = PCI_CONFIG_REGS_OFFSET.PCI_CIS, /* CardBus CIS pointer */
            PCI_SVID = PCI_CONFIG_REGS_OFFSET.PCI_SVID, /* Sub-system vendor ID */
            PCI_SDID = PCI_CONFIG_REGS_OFFSET.PCI_SDID, /* Sub-system device ID */
            PCI_EROM = PCI_CONFIG_REGS_OFFSET.PCI_EROM, /* Expansion ROM base address */
            PCI_CAP = 0x34, /* New capability pointer */
            PCI_ILR = PCI_CONFIG_REGS_OFFSET.PCI_ILR, /* Interrupt line */
            PCI_IPR = PCI_CONFIG_REGS_OFFSET.PCI_IPR, /* Interrupt pin */
            PCI_MGR = PCI_CONFIG_REGS_OFFSET.PCI_MGR, /* Minimum required burst period */
            PCI_MLR = PCI_CONFIG_REGS_OFFSET.PCI_MLR, /* Maximum latency - How often device must gain
                               * PCI bus access */
        };
        public enum PCI_CFG_REG_EXPRESS
        {
            PCIE_CAP_ID = PCIE_CONFIG_REGS_OFFSET.PCIE_CAP_ID,
            NEXT_CAP_PTR = PCIE_CONFIG_REGS_OFFSET.NEXT_CAP_PTR,
            CAP_REG = PCIE_CONFIG_REGS_OFFSET.CAP_REG,
            DEV_CAPS = PCIE_CONFIG_REGS_OFFSET.DEV_CAPS,
            DEV_CTL = PCIE_CONFIG_REGS_OFFSET.DEV_CTL,
            DEV_STS = PCIE_CONFIG_REGS_OFFSET.DEV_STS,
            LNK_CAPS = PCIE_CONFIG_REGS_OFFSET.LNK_CAPS,
            LNK_CTL = PCIE_CONFIG_REGS_OFFSET.LNK_CTL,
            LNK_STS = PCIE_CONFIG_REGS_OFFSET.LNK_STS,
            SLOT_CAPS = PCIE_CONFIG_REGS_OFFSET.SLOT_CAPS,
            SLOT_CTL = PCIE_CONFIG_REGS_OFFSET.SLOT_CTL,
            SLOT_STS = PCIE_CONFIG_REGS_OFFSET.SLOT_STS,
            ROOT_CAPS = PCIE_CONFIG_REGS_OFFSET.ROOT_CAPS,
            ROOT_CTL = PCIE_CONFIG_REGS_OFFSET.ROOT_CTL,
            ROOT_STS = PCIE_CONFIG_REGS_OFFSET.ROOT_STS,
            DEV_CAPS2 = PCIE_CONFIG_REGS_OFFSET.DEV_CAPS2,
            DEV_CTL2 = PCIE_CONFIG_REGS_OFFSET.DEV_CTL2,
            DEV_STS2 = PCIE_CONFIG_REGS_OFFSET.DEV_STS2,
            LNK_CAPS2 = PCIE_CONFIG_REGS_OFFSET.LNK_CAPS2,
            LNK_CTL2 = PCIE_CONFIG_REGS_OFFSET.LNK_CTL2,
            LNK_STS2 = PCIE_CONFIG_REGS_OFFSET.LNK_STS2,
            SLOT_CAPS2 = PCIE_CONFIG_REGS_OFFSET.SLOT_CAPS2,
            SLOT_CTL2 = PCIE_CONFIG_REGS_OFFSET.SLOT_CTL2,
            SLOT_STS2 = PCIE_CONFIG_REGS_OFFSET.SLOT_STS2
        };

        /* PCI base address spaces (BARs) */
        public enum PCI_BARS
        {
            AD_PCI_BAR0 = AD_PCI_BAR.AD_PCI_BAR0,
            AD_PCI_BAR1 = AD_PCI_BAR.AD_PCI_BAR1,
            AD_PCI_BAR2 = AD_PCI_BAR.AD_PCI_BAR2,
            AD_PCI_BAR3 = AD_PCI_BAR.AD_PCI_BAR3,
            AD_PCI_BAR4 = AD_PCI_BAR.AD_PCI_BAR4,
            AD_PCI_BAR5 = AD_PCI_BAR.AD_PCI_BAR5,
            AD_PCI_BARS = AD_PCI_BAR.AD_PCI_BARS,
        };

        public enum PCI_CAP
        {
            LIST_ID   = PciRegs.PCI_CAP_LIST_ID,     /* Capability ID */
            ID_PM     = PciRegs.PCI_CAP_ID_PM,  /* Power Management */
            ID_AGP    = PciRegs.PCI_CAP_ID_AGP,  /* Accelerated Graphics Port */
            ID_VPD    = PciRegs.PCI_CAP_ID_VPD,  /* Vital Product Data */
            ID_SLOTID = PciRegs.PCI_CAP_ID_SLOTID,  /* Slot Identification */
            ID_MSI    = PciRegs.PCI_CAP_ID_MSI,  /* Message Signalled Interrupts */
            ID_CHSWP  = PciRegs.PCI_CAP_ID_CHSWP,  /* CompactPCI HotSwap */
            ID_PCIX   = PciRegs.PCI_CAP_ID_PCIX,  /* PCI-X */
            ID_HT     = PciRegs.PCI_CAP_ID_HT,  /* HyperTransport */
            ID_VNDR   = PciRegs.PCI_CAP_ID_VNDR,  /* Vendor-Specific */
            ID_DBG    = PciRegs.PCI_CAP_ID_DBG,  /* Debug port */
            ID_CCRC   = PciRegs.PCI_CAP_ID_CCRC,  /* CompactPCI Central Resource Control */
            ID_SHPC   = PciRegs.PCI_CAP_ID_SHPC,  /* PCI Standard Hot-Plug Controller */
            ID_SSVID  = PciRegs.PCI_CAP_ID_SSVID,  /* Bridge subsystem vendor/device ID */
            ID_AGP3   = PciRegs.PCI_CAP_ID_AGP3,  /* AGP Target PCI-PCI bridge */
            ID_SECDEV = PciRegs.PCI_CAP_ID_SECDEV,  /* Secure Device */
            ID_EXP    = PciRegs.PCI_CAP_ID_EXP,  /* PCI Express */
            ID_MSIX   = PciRegs.PCI_CAP_ID_MSIX,  /* MSI-X */
            ID_SATA   = PciRegs.PCI_CAP_ID_SATA,  /* SATA Data/Index Conf. */
            ID_AF     = PciRegs.PCI_CAP_ID_AF,  /* PCI Advanced Features */
            LIST_NEXT = PciRegs.PCI_CAP_LIST_NEXT,     /* Next capability in the list */
        };

        public enum PCI_EXT_CAP
        {
            ID_ERR      = PciRegs.PCI_EXT_CAP_ID_ERR,  /* Advanced Error Reporting */
            ID_VC       = PciRegs.PCI_EXT_CAP_ID_VC,  /* Virtual Channel Capability */
            ID_DSN      = PciRegs.PCI_EXT_CAP_ID_DSN,  /* Device Serial Number */
            ID_PWR      = PciRegs.PCI_EXT_CAP_ID_PWR,  /* Power Budgeting */
            ID_RCLD     = PciRegs.PCI_EXT_CAP_ID_RCLD,  /* Root Complex Link Declaration */
            ID_RCILC    = PciRegs.PCI_EXT_CAP_ID_RCILC,  /* Root Complex Internal Link Control */
            ID_RCEC     = PciRegs.PCI_EXT_CAP_ID_RCEC,  /* Root Complex Event Collector */
            ID_MFVC     = PciRegs.PCI_EXT_CAP_ID_MFVC,  /* Multi-Function VC Capability */
            ID_VC9      = PciRegs.PCI_EXT_CAP_ID_VC9,  /* same as _VC */
            ID_RCRB     = PciRegs.PCI_EXT_CAP_ID_RCRB,  /* Root Complex RB? */
            ID_VNDR     = PciRegs.PCI_EXT_CAP_ID_VNDR,  /* Vendor-Specific */
            ID_CAC      = PciRegs.PCI_EXT_CAP_ID_CAC,  /* Config Access - obsolete */
            ID_ACS      = PciRegs.PCI_EXT_CAP_ID_ACS,  /* Access Control Services */
            ID_ARI      = PciRegs.PCI_EXT_CAP_ID_ARI,  /* Alternate Routing ID */
            ID_ATS      = PciRegs.PCI_EXT_CAP_ID_ATS,  /* Address Translation Services */
            ID_SRIOV    = PciRegs.PCI_EXT_CAP_ID_SRIOV,  /* Single Root I/O Virtualization */
            ID_MRIOV    = PciRegs.PCI_EXT_CAP_ID_MRIOV,  /* Multi Root I/O Virtualization */
            ID_MCAST    = PciRegs.PCI_EXT_CAP_ID_MCAST,  /* Multicast */
            ID_PRI      = PciRegs.PCI_EXT_CAP_ID_PRI,  /* Page Request Interface */
            ID_AMD_XXX  = PciRegs.PCI_EXT_CAP_ID_AMD_XXX,  /* Reserved for AMD */
            ID_REBAR    = PciRegs.PCI_EXT_CAP_ID_REBAR,  /* Resizable BAR */
            ID_DPA      = PciRegs.PCI_EXT_CAP_ID_DPA,  /* Dynamic Power Allocation */
            ID_TPH      = PciRegs.PCI_EXT_CAP_ID_TPH,  /* TPH Requester */
            ID_LTR      = PciRegs.PCI_EXT_CAP_ID_LTR,  /* Latency Tolerance Reporting */
            ID_SECPCI   = PciRegs.PCI_EXT_CAP_ID_SECPCI,  /* Secondary PCIe Capability */
            ID_PMUX     = PciRegs.PCI_EXT_CAP_ID_PMUX,  /* Protocol Multiplexing */
            ID_PASID    = PciRegs.PCI_EXT_CAP_ID_PASID,  /* Process Address Space ID */
            ID_LNR      = PciRegs.PCI_EXT_CAP_ID_LNR,  /* LN Requester (LNR) */
            ID_DPC      = PciRegs.PCI_EXT_CAP_ID_DPC,  /* Downstream Port Containment (DPC) */
            ID_L1PMS    = PciRegs.PCI_EXT_CAP_ID_L1PMS,  /* L1 PM Substates */
            ID_PTM      = PciRegs.PCI_EXT_CAP_ID_PTM,  /* Precision Time Measurement (PTM) */
            ID_MPHY     = PciRegs.PCI_EXT_CAP_ID_MPHY,  /* PCI Express over M-PHY (M-PCIe) */
            ID_FRSQ     = PciRegs.PCI_EXT_CAP_ID_FRSQ,  /* FRS Queueing */
            ID_RTR      = PciRegs.PCI_EXT_CAP_ID_RTR,  /* Readiness Time Reporting */
        };
    }
}
