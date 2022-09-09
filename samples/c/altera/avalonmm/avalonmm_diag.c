/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/****************************************************************************
*  File: AVALONMM_diag.c
*
*  Sample user-mode diagnostics application for accessing Altera PCI Express
*  cards with Avalon-MM support, using the WinDriver WDC API.
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*****************************************************************************/

#include "wds_lib.h"

#include "utils.h"
#include "status_strings.h"
#include "diag_lib.h"
#include "wdc_diag_lib.h"
#include "pci_menus_common.h"

#include "wds_diag_lib.h"

#include "avalonmm_lib.h"

/*************************************************************
  General definitions
 *************************************************************/
 /* Error messages display */
#define AVALONMM_ERR printf

/* Trace messages display */
#define AVALONMM_TRACE printf

/* --------------------------------------------------
    PCI configuration registers information
   -------------------------------------------------- */
   /* Configuration registers information array */
static const WDC_REG gAVALONMM_CfgRegs[] = {
    { WDC_AD_CFG_SPACE, PCI_VID, WDC_SIZE_16, WDC_READ_WRITE, "VID",
        "Vendor ID" },
    { WDC_AD_CFG_SPACE, PCI_DID, WDC_SIZE_16, WDC_READ_WRITE, "DID",
        "Device ID" },
    { WDC_AD_CFG_SPACE, PCI_CR, WDC_SIZE_16, WDC_READ_WRITE, "CMD", "Command" },
    { WDC_AD_CFG_SPACE, PCI_SR, WDC_SIZE_16, WDC_READ_WRITE, "STS", "Status" },
    { WDC_AD_CFG_SPACE, PCI_REV, WDC_SIZE_32, WDC_READ_WRITE, "RID_CLCD",
        "Revision ID &\nClass Code" },
    { WDC_AD_CFG_SPACE, PCI_CCSC, WDC_SIZE_8, WDC_READ_WRITE, "SCC",
        "Sub Class Code" },
    { WDC_AD_CFG_SPACE, PCI_CCBC, WDC_SIZE_8, WDC_READ_WRITE, "BCC",
        "Base Class Code" },
    { WDC_AD_CFG_SPACE, PCI_CLSR, WDC_SIZE_8, WDC_READ_WRITE, "CALN",
        "Cache Line Size" },
    { WDC_AD_CFG_SPACE, PCI_LTR, WDC_SIZE_8, WDC_READ_WRITE, "LAT",
        "Latency Timer" },
    { WDC_AD_CFG_SPACE, PCI_HDR, WDC_SIZE_8, WDC_READ_WRITE, "HDR",
        "Header Type" },
    { WDC_AD_CFG_SPACE, PCI_BISTR, WDC_SIZE_8, WDC_READ_WRITE, "BIST",
        "Built-in Self Test" },
    { WDC_AD_CFG_SPACE, PCI_BAR0, WDC_SIZE_32, WDC_READ_WRITE, "BADDR0",
        "Base Address 0" },
    { WDC_AD_CFG_SPACE, PCI_BAR1, WDC_SIZE_32, WDC_READ_WRITE, "BADDR1",
        "Base Address 1" },
    { WDC_AD_CFG_SPACE, PCI_BAR2, WDC_SIZE_32, WDC_READ_WRITE, "BADDR2",
        "Base Address 2" },
    { WDC_AD_CFG_SPACE, PCI_BAR3, WDC_SIZE_32, WDC_READ_WRITE, "BADDR3",
        "Base Address 3" },
    { WDC_AD_CFG_SPACE, PCI_BAR4, WDC_SIZE_32, WDC_READ_WRITE, "BADDR4",
        "Base Address 4" },
    { WDC_AD_CFG_SPACE, PCI_BAR5, WDC_SIZE_32, WDC_READ_WRITE, "BADDR5",
        "Base Address 5" },
    { WDC_AD_CFG_SPACE, PCI_CIS, WDC_SIZE_32, WDC_READ_WRITE, "CIS",
        "CardBus CIS\npointer" },
    { WDC_AD_CFG_SPACE, PCI_SVID, WDC_SIZE_16, WDC_READ_WRITE, "SVID",
        "Sub-system\nVendor ID" },
    { WDC_AD_CFG_SPACE, PCI_SDID, WDC_SIZE_16, WDC_READ_WRITE, "SDID",
        "Sub-system\nDevice ID" },
    { WDC_AD_CFG_SPACE, PCI_EROM, WDC_SIZE_32, WDC_READ_WRITE, "EROM",
        "Expansion ROM\nBase Address" },
    { WDC_AD_CFG_SPACE, PCI_CAP, WDC_SIZE_8, WDC_READ_WRITE, "NEW_CAP",
        "New Capabilities\nPointer" },
    { WDC_AD_CFG_SPACE, PCI_ILR, WDC_SIZE_32, WDC_READ_WRITE, "INTLN",
        "Interrupt Line" },
    { WDC_AD_CFG_SPACE, PCI_IPR, WDC_SIZE_32, WDC_READ_WRITE, "INTPIN",
        "Interrupt Pin" },
    { WDC_AD_CFG_SPACE, PCI_MGR, WDC_SIZE_32, WDC_READ_WRITE, "MINGNT",
        "Minimum Required\nBurst Period" },
    { WDC_AD_CFG_SPACE, PCI_MLR, WDC_SIZE_32, WDC_READ_WRITE, "MAXLAT",
        "Maximum Latency" },
};

static const WDC_REG gAVALONMM_ext_CfgRegs[] = {
    { WDC_AD_CFG_SPACE, PCIE_CAP_ID, WDC_SIZE_8, WDC_READ_WRITE, "PCIE_CAP_ID",
        "PCI Express\nCapability ID" },
    { WDC_AD_CFG_SPACE, NEXT_CAP_PTR, WDC_SIZE_8, WDC_READ_WRITE,
        "NEXT_CAP_PTR", "Next Capabiliy Pointer" },
    { WDC_AD_CFG_SPACE, CAP_REG, WDC_SIZE_16, WDC_READ_WRITE, "CAP_REG",
        "Capabilities Register" },
    { WDC_AD_CFG_SPACE, DEV_CAPS, WDC_SIZE_32, WDC_READ_WRITE, "DEV_CAPS",
        "Device Capabilities" },
    { WDC_AD_CFG_SPACE, DEV_CTL, WDC_SIZE_16, WDC_READ_WRITE, "DEV_CTL",
        "Device Control" },
    { WDC_AD_CFG_SPACE, DEV_STS, WDC_SIZE_16, WDC_READ_WRITE, "DEV_STS",
        "Device Status" },
    { WDC_AD_CFG_SPACE, LNK_CAPS, WDC_SIZE_32, WDC_READ_WRITE, "LNK_CAPS",
        "Link Capabilities" },
    { WDC_AD_CFG_SPACE, LNK_CTL, WDC_SIZE_16, WDC_READ_WRITE, "LNK_CTL",
        "Link Control" },
    { WDC_AD_CFG_SPACE, LNK_STS, WDC_SIZE_16, WDC_READ_WRITE, "LNK_STS",
        "Link Status" },
    { WDC_AD_CFG_SPACE, SLOT_CAPS, WDC_SIZE_32, WDC_READ_WRITE, "SLOT_CAPS",
        "Slot Capabilities" },
    { WDC_AD_CFG_SPACE, SLOT_CTL, WDC_SIZE_16, WDC_READ_WRITE, "SLOT_CTL",
        "Slot Control" },
    { WDC_AD_CFG_SPACE, SLOT_STS, WDC_SIZE_16, WDC_READ_WRITE, "SLOT_STS",
        "Slot Status" },
    { WDC_AD_CFG_SPACE, ROOT_CAPS, WDC_SIZE_16, WDC_READ_WRITE, "ROOT_CAPS",
        "Root Capabilities" },
    { WDC_AD_CFG_SPACE, ROOT_CTL, WDC_SIZE_16, WDC_READ_WRITE, "ROOT_CTL",
        "Root Control" },
    { WDC_AD_CFG_SPACE, ROOT_STS, WDC_SIZE_32, WDC_READ_WRITE, "ROOT_STS",
        "Root Status" },
    { WDC_AD_CFG_SPACE, DEV_CAPS2, WDC_SIZE_32, WDC_READ_WRITE, "DEV_CAPS2",
        "Device Capabilities 2" },
    { WDC_AD_CFG_SPACE, DEV_CTL2, WDC_SIZE_16, WDC_READ_WRITE, "DEV_CTL2",
        "Device Control 2" },
    { WDC_AD_CFG_SPACE, DEV_STS2, WDC_SIZE_16, WDC_READ_WRITE, "DEV_STS2",
        "Device Status 2" },
    { WDC_AD_CFG_SPACE, LNK_CAPS2, WDC_SIZE_32, WDC_READ_WRITE, "LNK_CAPS2",
        "Link Capabilities 2" },
    { WDC_AD_CFG_SPACE, LNK_CTL2, WDC_SIZE_16, WDC_READ_WRITE, "LNK_CTL2",
        "Link Control 2" },
    { WDC_AD_CFG_SPACE, LNK_STS2, WDC_SIZE_16, WDC_READ_WRITE, "LNK_STS2",
        "Link Status 2" },
    { WDC_AD_CFG_SPACE, SLOT_CAPS2, WDC_SIZE_32, WDC_READ_WRITE, "SLOT_CAPS2",
        "Slot Capabilities 2" },
    { WDC_AD_CFG_SPACE, SLOT_CTL2, WDC_SIZE_16, WDC_READ_WRITE, "SLOT_CTL2",
        "Slot Control 2" },
    { WDC_AD_CFG_SPACE, SLOT_STS2, WDC_SIZE_16, WDC_READ_WRITE, "SLOT_STS2",
        "Slot Status 2" },

    { WDC_AD_CFG_SPACE, AVALONMM_VSEC_CAPHDR_REG, WDC_SIZE_16, WDC_READ,
        "VSEC_CAPHDR", "Altera-Defined VSEC Capability header" },
    { WDC_AD_CFG_SPACE, AVALONMM_VERNEXTCAP_REG, WDC_SIZE_16, WDC_READ,
        "VER_NEXTCAP", "Altera VSEC Version & Next capability offset" },
    { WDC_AD_CFG_SPACE, AVALONMM_VSECID_REG, WDC_SIZE_16, WDC_READ, "VSEC_ID",
        "VSEC ID Intel-Defined, Vendor-Specific Header" },
    { WDC_AD_CFG_SPACE, AVALONMM_VSECREVLEN_REG, WDC_SIZE_16, WDC_READ,
        "VSECREV_LEN", "VSEC Revision & Length" },
    { WDC_AD_CFG_SPACE, AVALONMM_MARKER_REG, WDC_SIZE_32, WDC_READ, "MARKER",
        "Intel Marker" },
    { WDC_AD_CFG_SPACE, AVALONMM_JTAGDW0_REG, WDC_SIZE_32, WDC_READ, "JTAGDW0",
        "JTAG Silicon ID DW0 JTAG Silicon ID" },
    { WDC_AD_CFG_SPACE, AVALONMM_JTAGDW1_REG, WDC_SIZE_32, WDC_READ, "JTAGDW1",
        "JTAG Silicon ID DW1 JTAG Silicon ID" },
    { WDC_AD_CFG_SPACE, AVALONMM_JTAGDW2_REG, WDC_SIZE_32, WDC_READ, "JTAGDW2",
        "JTAG Silicon ID DW2 JTAG Silicon ID" },
    { WDC_AD_CFG_SPACE, AVALONMM_JTAGDW3_REG, WDC_SIZE_32, WDC_READ, "JTAGDW3",
        "JTAG Silicon ID DW3 JTAG Silicon ID" },
    { WDC_AD_CFG_SPACE, AVALONMM_BOARDTYPEID_REG, WDC_SIZE_16, WDC_READ,
        "USER_DEVICE_BOARD_TYPE_ID", "User Device or Board Type ID" },
    { WDC_AD_CFG_SPACE, AVALONMM_CVPSTAT_REG, WDC_SIZE_16, WDC_READ,
        "CVP_STATUS", "CvP Status" },
    { WDC_AD_CFG_SPACE, AVALONMM_CVPMODECTRL_REG, WDC_SIZE_32, WDC_READ_WRITE,
        "CVP_MODE_CTRL", "CvP Mode Control" },
    { WDC_AD_CFG_SPACE, AVALONMM_CVPDATA2_REG, WDC_SIZE_32, WDC_READ_WRITE,
        "CVP_DATA2", "CvP Data2 Register" },
    { WDC_AD_CFG_SPACE, AVALONMM_CVPDATA_REG, WDC_SIZE_32, WDC_READ_WRITE,
        "CVP_DATA", "CvP Data Register" },
    { WDC_AD_CFG_SPACE, AVALONMM_CVPPROG_REG, WDC_SIZE_32, WDC_READ_WRITE,
        "CVP_PROG", "CvP Programming Control Register" },
    { WDC_AD_CFG_SPACE, AVALONMM_UCSTATMASK_REG, WDC_SIZE_32, WDC_READ_WRITE,
        "UC_ERROR_STATUS", "Uncorrectable Internal Error Status Register" },
    { WDC_AD_CFG_SPACE, AVALONMM_UCMASK_REG, WDC_SIZE_32, WDC_READ_WRITE,
        "UC_ERROR_MASK", "Uncorrectable Internal Error Mask Register" },
    { WDC_AD_CFG_SPACE, AVALONMM_CSTATMASK_REG, WDC_SIZE_32, WDC_READ_WRITE,
        "C_ERROR_STATUS", "Correctable Internal Error Status Register" },
    { WDC_AD_CFG_SPACE, AVALONMM_CMASK_REG, WDC_SIZE_32, WDC_READ_WRITE,
        "C_ERROR_MASK", "Correctable Internal Error Mask Register" },
};

#define AVALONMM_CFG_EXT_REGS_NUM \
    sizeof(gAVALONMM_ext_CfgRegs) / sizeof(WDC_REG)
#define AVALONMM_CFG_REGS_NUM sizeof(gAVALONMM_CfgRegs) / sizeof(WDC_REG)

const WDC_REG *gpAVALONMM_CfgRegs = gAVALONMM_CfgRegs;


// -----------------------------------------------
//    AVALONMM run-time registers information
// -----------------------------------------------
// Run-time registers information array
const WDC_REG gpAVALONMM_Regs[] = {
    { AVALONMM_RXM_BAR0, AVALONMM_RC_READ_STATUS_DESCRIPTOR_LOW, WDC_SIZE_32,
        WDC_READ_WRITE, "RC_RD_STATUS_AND_DESC_BASE_LOW",
        "RC Read Status and Descriptor Base(Low)" },
    { AVALONMM_RXM_BAR0, AVALONMM_RC_READ_STATUS_DESCRIPTOR_HIGH, WDC_SIZE_32,
        WDC_READ_WRITE, "RC_RD_STATUS_AND_DESC_BASE_HIGH",
        "RC Read Status and Descriptor Base(High)" },
    { AVALONMM_RXM_BAR0, AVALONMM_EP_READ_DESCRIPTOR_FIFO_LOW, WDC_SIZE_32,
        WDC_READ_WRITE, "EP_RD_DESC_FIFO_BASE_LOW",
        "RC Read Status and Descriptor Base(Low)" },
    { AVALONMM_RXM_BAR0, AVALONMM_EP_READ_DESCRIPTOR_FIFO_HIGH, WDC_SIZE_32,
        WDC_READ_WRITE, "EP_RD_DESC_FIFO_BASE_HIGH",
        "RC Read Status and Descriptor Base(High)" },
    { AVALONMM_RXM_BAR0, AVALONMM_RD_DMA_LAST_PTR, WDC_SIZE_32,
        WDC_READ_WRITE, "RD_DMA_LAST_PTR", "Read DMA last ptr" },
    { AVALONMM_RXM_BAR0, AVALONMM_RD_TABLE_SIZE, WDC_SIZE_32,
        WDC_READ_WRITE, "RD_TABLE_SIZE", "Read table size" },
    { AVALONMM_RXM_BAR0, AVALONMM_RD_CONTROL, WDC_SIZE_32,
        WDC_READ_WRITE, "RD_CONTROL", "Read control" },

    { AVALONMM_RXM_BAR0, AVALONMM_RC_WRITE_STATUS_DESCRIPTOR_LOW, WDC_SIZE_32,
        WDC_READ_WRITE, "RC_WR_STATUS_AND_DESC_BASE_LOW",
        "RC Write Status and Descriptor Base(Low)" },
    { AVALONMM_RXM_BAR0, AVALONMM_RC_WRITE_STATUS_DESCRIPTOR_HIGH,
        WDC_SIZE_32, WDC_READ_WRITE, "RC_WR_STATUS_AND_DESC_BASE_HIGH",
        "RC Write Status and Descriptor Base(High)" },
    { AVALONMM_RXM_BAR0, AVALONMM_EP_WRITE_DESCRIPTOR_FIFO_LOW, WDC_SIZE_32,
        WDC_READ_WRITE, "EP_WR_DESC_FIFO_BASE_LOW",
        "RC Write Status and Descriptor Base(Low)" },
    { AVALONMM_RXM_BAR0, AVALONMM_EP_WRITE_DESCRIPTOR_FIFO_HIGH, WDC_SIZE_32,
        WDC_READ_WRITE, "EP_WR_DESC_FIFO_BASE_HIGH",
        "RC Write Status and Descriptor Base(High)" },
    { AVALONMM_RXM_BAR0, AVALONMM_WR_DMA_LAST_PTR, WDC_SIZE_32,
        WDC_READ_WRITE, "WR_DMA_LAST_PTR", "Write DMA last ptr" },
    { AVALONMM_RXM_BAR0, AVALONMM_WR_TABLE_SIZE, WDC_SIZE_32,
        WDC_READ_WRITE, "WR_TABLE_SIZE", "Write table size" },
    { AVALONMM_RXM_BAR0, AVALONMM_WR_CONTROL, WDC_SIZE_32,
        WDC_READ_WRITE, "WR_CONTROL", "Write control" },
};

#define AVALONMM_REGS_NUM sizeof(gpAVALONMM_Regs) / sizeof(WDC_REG)

/*************************************************************
  Static functions prototypes
 *************************************************************/
static DIAG_MENU_OPTION *MenuMainInit(WDC_DEVICE_HANDLE *phDev);

/* -----------------------------------------------
   Device Open
   ----------------------------------------------- */
static void MenuDeviceOpenInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

/* -----------------------------------------------
    Read/write the configuration space
   ----------------------------------------------- */
static void MenuCfgInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

/* ----------------------------------------------------
    Plug-and-play and power management events handling
   ---------------------------------------------------- */
static void MenuEventsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

/* -----------------------------------------------
    Read/write memory and I/O addresses
   ----------------------------------------------- */
static void MenuReadWriteAddrInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

#ifdef HAS_INTS
/* -----------------------------------------------
    Interrupt handling
   ----------------------------------------------- */
static void MenuInterruptsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);
#endif /* ifdef HAS_INTS */

/* -----------------------------------------------
    DMA memory handling
   ----------------------------------------------- */
static void MenuDmaInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

/* -----------------------------------------------
    Read/write the run-time registers
   ----------------------------------------------- */
static void MenuRwRegsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);


/*************************************************************
  Functions implementation
 *************************************************************/

static DWORD AVALONMM_Init(WDC_DEVICE_HANDLE *phDev)
{
    /* Initialize the AVALONMM library */
    DWORD dwStatus = AVALONMM_LibInit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        AVALONMM_ERR("%s: Failed to initialize the AVALONMM"
            "library: %s", __FUNCTION__, AVALONMM_GetLastErr());
        return dwStatus;
    }

    PrintDbgMessage(D_ERROR, S_PCI, "WinDriver user mode version %s\n",
        WD_VERSION_STR);

    /* Find and open a PCI device (by default ID) */
    if (AVALONMM_DEFAULT_VENDOR_ID)
    {
        *phDev = AVALONMM_DeviceOpen(AVALONMM_DEFAULT_VENDOR_ID,
            AVALONMM_DEFAULT_DEVICE_ID);
        if (!*phDev)
        {
            AVALONMM_ERR("%s: Failed opening PCI device", __FUNCTION__);
            return WD_OPERATION_FAILED;
        }
    }

    return WD_STATUS_SUCCESS;
}

int main(void)
{
    WDC_DEVICE_HANDLE hDev = NULL;
    DIAG_MENU_OPTION *pMenuRoot;
    DWORD dwStatus = AVALONMM_Init(&hDev);

    if (dwStatus)
        return dwStatus;

    AVALONMM_TRACE("\n");
    AVALONMM_TRACE("AVALONMM diagnostic utility.\n");
    AVALONMM_TRACE("Application accesses hardware using " WD_PROD_NAME "\n");

    pMenuRoot = MenuMainInit(&hDev);

    /* Busy loop that runs the menu tree created above and communicates
        with the user */
    return DIAG_MenuRun(pMenuRoot);
}

/* -----------------------------------------------
    Main diagnostics menu
   ----------------------------------------------- */
static DWORD MenuMainExitCb(PVOID pCbCtx)
{
    WDC_DEVICE_HANDLE hDev = *(WDC_DEVICE_HANDLE *)pCbCtx;
    DWORD dwStatus = 0;

    /* Perform necessary cleanup before exiting the program: */
    /* Close the device handle */
    if (hDev)
    {
        if (!AVALONMM_DeviceClose(hDev))
            AVALONMM_ERR("AVALONMM_diag: Failed closing PCI device: %s",
                AVALONMM_GetLastErr());
    }

    /* Uninitialize libraries */
    dwStatus = AVALONMM_LibUninit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        AVALONMM_ERR("AVALONMM_diag: Failed to uninitialize the AVALONMM"
            "library: %s", AVALONMM_GetLastErr());
    }

    return dwStatus;
}

static DIAG_MENU_OPTION *MenuMainInit(WDC_DEVICE_HANDLE *phDev)
{
    static DIAG_MENU_OPTION menuRoot = { 0 };
    strcpy(menuRoot.cTitleName, "AVALONMM main menu");
    menuRoot.cbExit = MenuMainExitCb;

    menuRoot.pCbCtx = phDev;

    MenuCommonScanBusInit(&menuRoot);
    MenuDeviceOpenInit(&menuRoot, phDev);
    MenuCfgInit(&menuRoot, phDev);
    MenuEventsInit(&menuRoot, phDev);

    MenuReadWriteAddrInit(&menuRoot, phDev);
#ifdef HAS_INTS
    MenuInterruptsInit(&menuRoot, phDev);
#endif /* ifdef HAS_INTS */

    MenuDmaInit(&menuRoot, phDev);

    MenuRwRegsInit(&menuRoot, phDev);

    return &menuRoot;
}

/* -----------------------------------------------
   Device Open
   ----------------------------------------------- */
static DWORD MenuDeviceOpenCb(PVOID pCbCtx)
{
    WDC_DEVICE_HANDLE *phDev = (WDC_DEVICE_HANDLE *)pCbCtx;

    if (*phDev)
    {
        if (!AVALONMM_DeviceClose(*phDev))
            AVALONMM_ERR("AVALONMM_diag: Failed closing PCI device: %s\n",
                AVALONMM_GetLastErr());
    }

    *phDev = AVALONMM_DeviceOpen(0, 0);
    if (!*phDev)
    {
        AVALONMM_ERR("%s: Failed opening PCI device", __FUNCTION__);
        return WD_OPERATION_FAILED;
    }

    return WD_STATUS_SUCCESS;
}

static void MenuDeviceOpenInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static DIAG_MENU_OPTION menuDeviceOpen = { 0 };

    strcpy(menuDeviceOpen.cOptionName, "Find and open a PCI device");
    menuDeviceOpen.cbEntry = MenuDeviceOpenCb;

    DIAG_MenuSetCtxAndParentForMenus(&menuDeviceOpen, 1, phDev,
        pParentMenu);
}

/* -----------------------------------------------
    Read/write the configuration space
   ----------------------------------------------- */
static void MenuCfgInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static MENU_CTX_CFG cfgCtx;

    BZERO(cfgCtx);

    cfgCtx.phDev = phDev;
    cfgCtx.pCfgRegs = gAVALONMM_CfgRegs;
    cfgCtx.dwCfgRegsNum = AVALONMM_CFG_REGS_NUM;
    cfgCtx.pCfgExpRegs = gAVALONMM_ext_CfgRegs;
    cfgCtx.dwCfgExpRegsNum = AVALONMM_CFG_EXT_REGS_NUM;

    MenuCommonCfgInit(pParentMenu, &cfgCtx);
}

/* ----------------------------------------------------
    Plug-and-play and power management events handling
   ---------------------------------------------------- */

   /* Diagnostics plug-and-play and power management events handler routine */
static void DiagEventHandler(WDC_DEVICE_HANDLE hDev, DWORD dwAction)
{
    /* TODO: You can modify this function in order to implement your own
     *       diagnostics events handler routine. */

    AVALONMM_TRACE("\nReceived event notification (device handle 0x%p): ",
        hDev);

    switch (dwAction)
    {
    case WD_INSERT:
        AVALONMM_TRACE("WD_INSERT\n");
        break;
    case WD_REMOVE:
        AVALONMM_TRACE("WD_REMOVE\n");
        break;
    case WD_POWER_CHANGED_D0:
        AVALONMM_TRACE("WD_POWER_CHANGED_D0\n");
        break;
    case WD_POWER_CHANGED_D1:
        AVALONMM_TRACE("WD_POWER_CHANGED_D1\n");
        break;
    case WD_POWER_CHANGED_D2:
        AVALONMM_TRACE("WD_POWER_CHANGED_D2\n");
        break;
    case WD_POWER_CHANGED_D3:
        AVALONMM_TRACE("WD_POWER_CHANGED_D3\n");
        break;
    case WD_POWER_SYSTEM_WORKING:
        AVALONMM_TRACE("WD_POWER_SYSTEM_WORKING\n");
        break;
    case WD_POWER_SYSTEM_SLEEPING1:
        AVALONMM_TRACE("WD_POWER_SYSTEM_SLEEPING1\n");
        break;
    case WD_POWER_SYSTEM_SLEEPING2:
        AVALONMM_TRACE("WD_POWER_SYSTEM_SLEEPING2\n");
        break;
    case WD_POWER_SYSTEM_SLEEPING3:
        AVALONMM_TRACE("WD_POWER_SYSTEM_SLEEPING3\n");
        break;
    case WD_POWER_SYSTEM_HIBERNATE:
        AVALONMM_TRACE("WD_POWER_SYSTEM_HIBERNATE\n");
        break;
    case WD_POWER_SYSTEM_SHUTDOWN:
        AVALONMM_TRACE("WD_POWER_SYSTEM_SHUTDOWN\n");
        break;
    default:
        AVALONMM_TRACE("0x%x\n", dwAction);
        break;
    }
}

static DWORD MenuEventsRegisterOptionCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    DWORD dwStatus = AVALONMM_EventRegister(*(pEventsMenusCtx->phDev),
        (AVALONMM_EVENT_HANDLER)pEventsMenusCtx->DiagEventHandler);

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        AVALONMM_TRACE("Events registered\n");
        pEventsMenusCtx->fRegistered = TRUE;
    }
    else
    {
        AVALONMM_ERR("Failed to register events. Last error [%s]\n",
            AVALONMM_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuEventsUnregisterOptionCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    DWORD dwStatus = AVALONMM_EventUnregister(*(pEventsMenusCtx->phDev));

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        AVALONMM_TRACE("Events unregistered\n");
        pEventsMenusCtx->fRegistered = FALSE;
    }
    else
    {
        AVALONMM_ERR("Failed to unregister events. Last error [%s]\n",
            AVALONMM_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuEventsCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    pEventsMenusCtx->fRegistered = AVALONMM_EventIsRegistered(
        *pEventsMenusCtx->phDev);

#ifdef WIN32
    if (!pEventsMenusCtx->fRegistered)
    {
        printf("NOTICE: An INF must be installed for your device in order to \n"
            "        call your user-mode event handler.\n"
            "        You can generate an INF file using the DriverWizard.");
    }
#endif

    return WD_STATUS_SUCCESS;
}

static void MenuEventsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static MENU_EVENTS_CALLBACKS eventsMenuCbs = { 0 };
    static MENU_CTX_EVENTS eventsMenusCtx = { 0 };

    eventsMenuCbs.eventsMenuEntryCb = MenuEventsCb;
    eventsMenuCbs.eventsEnableCb = MenuEventsRegisterOptionCb;
    eventsMenuCbs.eventsDisableCb = MenuEventsUnregisterOptionCb;

    eventsMenusCtx.phDev = phDev;
    eventsMenusCtx.DiagEventHandler = (DIAG_EVENT_HANDLER)DiagEventHandler;

    MenuCommonEventsInit(pParentMenu, &eventsMenusCtx, &eventsMenuCbs);
}

/* -----------------------------------------------
    Read/write memory and I/O addresses
   ----------------------------------------------- */

static void MenuReadWriteAddrInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static MENU_CTX_READ_WRITE_ADDR rwAddrMenusCtx = { 0 };

    rwAddrMenusCtx.phDev = phDev;
    rwAddrMenusCtx.fBlock = FALSE;
    rwAddrMenusCtx.mode = WDC_MODE_32;
    rwAddrMenusCtx.dwAddrSpace = ACTIVE_ADDR_SPACE_NEEDS_INIT;

    MenuCommonRwAddrInit(pParentMenu, &rwAddrMenusCtx);
}
#ifdef HAS_INTS
/* -----------------------------------------------
    Interrupt handling
   ----------------------------------------------- */

   /* Diagnostics interrupt handler routine */
static void DiagIntHandler(WDC_DEVICE_HANDLE hDev,
    AVALONMM_INT_RESULT *pIntResult)
{
    /* TODO: You can modify this function in order to implement your own
             diagnostics interrupt handler routine */

    UNUSED_VAR(hDev);

    AVALONMM_TRACE("Got interrupt number %d\n", pIntResult->dwCounter);
    AVALONMM_TRACE("Interrupt Type: %s\n",
        WDC_DIAG_IntTypeDescriptionGet(pIntResult->dwEnabledIntType));

    if (WDC_INT_IS_MSI(pIntResult->dwEnabledIntType))
        AVALONMM_TRACE("Message Data: 0x%x\n", pIntResult->dwLastMessage);
}

static DWORD MenuInterruptsEnableOptionCb(PVOID pCbCtx)
{
    MENU_CTX_INTERRUPTS *pInterruptsMenusCtx = (MENU_CTX_INTERRUPTS *)pCbCtx;
    DWORD dwStatus = AVALONMM_IntEnable(*(pInterruptsMenusCtx->phDev),
        (AVALONMM_INT_HANDLER)pInterruptsMenusCtx->funcIntHandler);

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        AVALONMM_TRACE("Interrupts enabled\n");
        AVALONMM_TRACE("\nNOTICE: MSI Interrupts are hardware enabled by "
            "default.\n"
            "        When turning on the DMA mechanism, even if using the "
            "polling method an interrupt might occur.\n"
            "        Check the Avalon MM user specification for more info.\n"
            "\n");
        pInterruptsMenusCtx->fIntsEnabled = TRUE;
    }
    else
    {
        AVALONMM_ERR("Failed enabling interrupts. Error [0x%x - %s]\n",
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

static DWORD MenuInterruptsDisableOptionCb(PVOID pCbCtx)
{
    MENU_CTX_INTERRUPTS *pInterruptsMenusCtx = (MENU_CTX_INTERRUPTS *)pCbCtx;
    DWORD dwStatus = AVALONMM_IntDisable(*(pInterruptsMenusCtx->phDev));

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        AVALONMM_TRACE("Interrupts disabled\n");
        pInterruptsMenusCtx->fIntsEnabled = FALSE;
    }
    else
    {
        AVALONMM_ERR("Failed disabling interrupts: %s\n",
            AVALONMM_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuInterruptsCb(PVOID pCbCtx)
{
    MENU_CTX_INTERRUPTS *pInterruptsMenusCtx = (MENU_CTX_INTERRUPTS *)pCbCtx;

    pInterruptsMenusCtx->fIntsEnabled = AVALONMM_IntIsEnabled(
        *pInterruptsMenusCtx->phDev);

    return WD_STATUS_SUCCESS;
}

static void MenuInterruptsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static MENU_INTERRUPTS_CALLBACKS interruptsMenuCbs = { 0 };
    static MENU_CTX_INTERRUPTS interruptsMenusCtx = { 0 };

    interruptsMenuCbs.interruptsMenuEntryCb = MenuInterruptsCb;
    interruptsMenuCbs.interruptsEnableCb = MenuInterruptsEnableOptionCb;
    interruptsMenuCbs.interruptsDisableCb = MenuInterruptsDisableOptionCb;

    interruptsMenusCtx.phDev = phDev;
    interruptsMenusCtx.funcIntHandler = (DIAG_INT_HANDLER)DiagIntHandler;

    MenuCommonInterruptsInit(pParentMenu, &interruptsMenusCtx,
        &interruptsMenuCbs);
}
#endif /* ifdef HAS_INTS */
/* -----------------------------------------------
    DMA memory handling
   ----------------------------------------------- */

static BOOL MenuDmaIsDeviceNull(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_DMA *pCtx = (MENU_CTX_DMA *)pMenu->pCbCtx;

    return *pCtx->phDev == NULL;
}

static BOOL DmaGetAllocInput(MENU_CTX_DMA *pDmaMenusCtx)
{
    DWORD option;

    AVALONMM_TRACE("\nSelect DMA direction:\n");
    AVALONMM_TRACE("1. From device\n");
    AVALONMM_TRACE("2. To device\n");
    AVALONMM_TRACE("%d. Cancel\n", DIAG_EXIT_MENU);

    if ((DIAG_INPUT_SUCCESS != DIAG_GetMenuOption(&option, 2)) ||
        (DIAG_EXIT_MENU == option))
    {
        return FALSE;
    }

    pDmaMenusCtx->fToDevice = (1 == option) ? FALSE : TRUE;

    if (pDmaMenusCtx->fToDevice)
    {
        /* Get DMA buffer pattern for host to device transfer */
        if (DIAG_INPUT_SUCCESS !=
            DIAG_InputUINT32((PVOID)&pDmaMenusCtx->u32Pattern,
                "\nEnter DMA data pattern as 32 bit packet", TRUE, 0, 0))
        {
            return FALSE;
        }
    }

    /* Get number of packets */
    if (DIAG_INPUT_SUCCESS !=
        DIAG_InputDWORD((PVOID)&pDmaMenusCtx->dwNumPackets,
            "\nEnter number of packets to transfer (32 bit packets)", FALSE, 1,
            AVALONMM_TRANSACTION_MAX_TRANSFER_SIZE / sizeof(UINT32)))
    {
        return FALSE;
    }

    pDmaMenusCtx->dwBytes = pDmaMenusCtx->dwNumPackets * sizeof(UINT32);

    /* Get FPGA offset */
    if (DIAG_INPUT_SUCCESS !=
        DIAG_InputUINT64((PVOID)&pDmaMenusCtx->u64FPGAOffset,
            "\nEnter FPGA offset for transfer", TRUE, 0, 0))
    {
        return FALSE;
    }

    return TRUE;
}

static DWORD MenuDmaAllocSgOptionCb(PVOID pCbCtx)
{
    UINT32 *pu32Buf = NULL;
    MENU_CTX_DMA *pDmaMenusCtx = (MENU_CTX_DMA *)pCbCtx;
    DWORD dwStatus, i;

    if (!DmaGetAllocInput(pDmaMenusCtx))
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    pDmaMenusCtx->pBuf = calloc(pDmaMenusCtx->dwBytes, sizeof(BYTE));
    if (!pDmaMenusCtx->pBuf)
    {
        AVALONMM_ERR("Failed allocating user memory for SG. size [%d]\n",
            pDmaMenusCtx->dwBytes);
        dwStatus = WD_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    if (pDmaMenusCtx->fToDevice)
    {
        pu32Buf = (UINT32 *)pDmaMenusCtx->pBuf;
        for (i = 0; i < pDmaMenusCtx->dwBytes / sizeof(UINT32); i++)
            pu32Buf[i] = pDmaMenusCtx->u32Pattern;
    }

    pDmaMenusCtx->dwOptions = DMA_ALLOW_64BIT_ADDRESS |
        (pDmaMenusCtx->fToDevice ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

    pDmaMenusCtx->fIsDmaExecuted = FALSE;

    dwStatus = AVALONMM_DmaInit(pDmaMenusCtx);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        AVALONMM_ERR("%s: Failed to initilaze DMA transaction. Error 0x%x - "
            "%s\n", __FUNCTION__, dwStatus, Stat2Str(dwStatus));
    }

Exit:
    return dwStatus;
}

static DWORD MenuDmaUnInitBuffOptionCb(PVOID pCbCtx)
{
    MENU_CTX_DMA *pDmaMenusCtx = (MENU_CTX_DMA *)pCbCtx;

    AVALONMM_FreeDmaMem(pDmaMenusCtx);

    if (pDmaMenusCtx->pBuf)
    {
        free(pDmaMenusCtx->pBuf);
        pDmaMenusCtx->pBuf = NULL;
    }

    return WD_STATUS_SUCCESS;
}

static BOOL MenuDmaIsDmaHandleNotNull(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_DMA *pCtx = (MENU_CTX_DMA *)pMenu->pCbCtx;

    return pCtx->pDmaBuffer != NULL;
}

static BOOL MenuDmaIsDmaHandleNull(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_DMA *pCtx = (MENU_CTX_DMA *)pMenu->pCbCtx;

    return pCtx->pDmaBuffer == NULL;
}

static BOOL MenuDmaIsDmaTransactionExecuted(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_DMA *pCtx = (MENU_CTX_DMA *)pMenu->pCbCtx;

    return pCtx->pDmaBuffer == NULL || pCtx->fIsDmaExecuted == TRUE;
}

static BOOL MenuDmaIsDmaTransactionNotExecuted(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_DMA *pCtx = (MENU_CTX_DMA *)pMenu->pCbCtx;

    return pCtx->pDmaBuffer == NULL || pCtx->fIsDmaExecuted == FALSE;
}

static BOOL MenuDmaIsVerificationCheckEnabled(
    DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_DMA *pCtx = (MENU_CTX_DMA *)pMenu->pCbCtx;

    return pCtx->pDmaBuffer == NULL || pCtx->fRunVerificationCheck == TRUE;
}

static BOOL MenuDmaIsVerificationCheckDisabled(
    DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_DMA *pCtx = (MENU_CTX_DMA *)pMenu->pCbCtx;

    return pCtx->pDmaBuffer == NULL || pCtx->fRunVerificationCheck == FALSE;
}

static DWORD MenuDmaTransactionEnableVerificationCheckOptionCb(PVOID pCbCtx)
{
    MENU_CTX_DMA *pMenuDmaTransferCtx = (MENU_CTX_DMA *)pCbCtx;

    pMenuDmaTransferCtx->fRunVerificationCheck = TRUE;

    return WD_STATUS_SUCCESS;
}

static DWORD MenuDmaTransactionDisableVerificationCheckOptionCb(PVOID pCbCtx)
{
    MENU_CTX_DMA *pMenuDmaTransferCtx = (MENU_CTX_DMA *)pCbCtx;

    pMenuDmaTransferCtx->fRunVerificationCheck = FALSE;

    return WD_STATUS_SUCCESS;
}

static DWORD MenuDmaTransactionExecuteOptionCb(PVOID pCbCtx)
{
    MENU_CTX_DMA *pMenuDmaTransferCtx = (MENU_CTX_DMA *)pCbCtx;
    DWORD dwStatus;

    dwStatus = AVALONMM_DmaTransactionExecute(pMenuDmaTransferCtx);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        AVALONMM_ERR("%s: Failed to execute DMA transaction. Error 0x%x - "
            "%s\n", __FUNCTION__, dwStatus, Stat2Str(dwStatus));
    }
    else
    {
        AVALONMM_TRACE("DMA transaction has been finished\n");
        pMenuDmaTransferCtx->fIsDmaExecuted = TRUE;
    }

    return WD_STATUS_SUCCESS;
}

static DWORD MenuDmaTransactionReleaseOptionCb(PVOID pCbCtx)
{
    MENU_CTX_DMA *pMenuDmaTransferCtx = (MENU_CTX_DMA *)pCbCtx;
    DWORD dwStatus;

    dwStatus = AVALONMM_DmaTransactionRelease(pMenuDmaTransferCtx);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        AVALONMM_ERR("%s: Failed to release DMA transaction. Error 0x%x - "
            "%s\n", __FUNCTION__, dwStatus, Stat2Str(dwStatus));
    }
    else
    {
        pMenuDmaTransferCtx->fIsDmaExecuted = FALSE;
    }

    return WD_STATUS_SUCCESS;
}

static DWORD MenuDmaTransactionShowBufferOptionCb(PVOID pCbCtx)
{
    MENU_CTX_DMA *pMenuDmaTransferCtx = (MENU_CTX_DMA *)pCbCtx;

    DIAG_PrintHexBuffer(pMenuDmaTransferCtx->pBuf, pMenuDmaTransferCtx->dwBytes,
        FALSE);

    return WD_STATUS_SUCCESS;
}


static void MenuDmaTransactionSubMenusInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_DMA *pDmaSubMenusCtx)
{
    static DIAG_MENU_OPTION initDmaTransactionMenu = { 0 };
    static DIAG_MENU_OPTION executeTransactionMenu = { 0 };
    static DIAG_MENU_OPTION releaseTransactionMenu = { 0 };
    static DIAG_MENU_OPTION displayBufferMenu = { 0 };
    static DIAG_MENU_OPTION enableVerificationMenu = { 0 };
    static DIAG_MENU_OPTION disableVerificationMenu = { 0 };
    static DIAG_MENU_OPTION uninitDmaTransactionMenu = { 0 };
    static DIAG_MENU_OPTION options[7] = { 0 };

    strcpy(initDmaTransactionMenu.cOptionName, "Initialize DMA transaction");
    initDmaTransactionMenu.cbEntry = MenuDmaAllocSgOptionCb;
    initDmaTransactionMenu.cbIsHidden = MenuDmaIsDmaHandleNotNull;

    strcpy(executeTransactionMenu.cOptionName, "Execute transaction");
    executeTransactionMenu.cbEntry = MenuDmaTransactionExecuteOptionCb;
    executeTransactionMenu.cbIsHidden = MenuDmaIsDmaTransactionExecuted;

    strcpy(releaseTransactionMenu.cOptionName, "Release transaction");
    releaseTransactionMenu.cbEntry = MenuDmaTransactionReleaseOptionCb;
    releaseTransactionMenu.cbIsHidden = MenuDmaIsDmaTransactionNotExecuted;

    strcpy(displayBufferMenu.cOptionName, "Display transferred buffer "
        "content");
    displayBufferMenu.cbEntry = MenuDmaTransactionShowBufferOptionCb;
    displayBufferMenu.cbIsHidden = MenuDmaIsDmaHandleNull;

    strcpy(enableVerificationMenu.cOptionName, "Enable verification check at the "
        "end of transfer");
    enableVerificationMenu.cbEntry =
        MenuDmaTransactionEnableVerificationCheckOptionCb;
    enableVerificationMenu.cbIsHidden = MenuDmaIsVerificationCheckEnabled;

    strcpy(disableVerificationMenu.cOptionName, "Disable verification check at the "
        "end of transfer");
    disableVerificationMenu.cbEntry =
        MenuDmaTransactionDisableVerificationCheckOptionCb;
    disableVerificationMenu.cbIsHidden = MenuDmaIsVerificationCheckDisabled;

    strcpy(uninitDmaTransactionMenu.cOptionName, "Uninitialize DMA transaction");
    uninitDmaTransactionMenu.cbEntry = MenuDmaUnInitBuffOptionCb;
    uninitDmaTransactionMenu.cbIsHidden = MenuDmaIsDmaHandleNull;

    options[0] = initDmaTransactionMenu;
    options[1] = executeTransactionMenu;
    options[2] = releaseTransactionMenu;
    options[3] = displayBufferMenu;
    options[4] = enableVerificationMenu;
    options[5] = disableVerificationMenu;
    options[6] = uninitDmaTransactionMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        pDmaSubMenusCtx, pParentMenu);
}

static void MenuDmaSubMenusInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_DMA *pDmaSubMenusCtx)
{
    static DIAG_MENU_OPTION performDMATransferOption = { 0 };

    strcpy(performDMATransferOption.cTitleName, "DMA transaction (polling)");
    strcpy(performDMATransferOption.cOptionName, "Perform DMA transfer (polling)");

    DIAG_MenuSetCtxAndParentForMenus(&performDMATransferOption, 1,
        pDmaSubMenusCtx, pParentMenu);
    MenuDmaTransactionSubMenusInit(&performDMATransferOption, pDmaSubMenusCtx);
}

static void MenuDmaInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static DIAG_MENU_OPTION dmaMenuOption = { 0 };
    static MENU_CTX_DMA dmaMenusCtx = { 0 };

    strcpy(dmaMenuOption.cOptionName, "Direct Memory Access (DMA)");
    strcpy(dmaMenuOption.cTitleName, "Direct Memory Access (DMA)");
    dmaMenuOption.cbIsHidden = MenuDmaIsDeviceNull;
    dmaMenuOption.cbExit = MenuDmaUnInitBuffOptionCb;

    dmaMenusCtx.fRunVerificationCheck = TRUE;
    dmaMenusCtx.phDev = phDev;

    DIAG_MenuSetCtxAndParentForMenus(&dmaMenuOption, 1, &dmaMenusCtx,
        pParentMenu);
    MenuDmaSubMenusInit(&dmaMenuOption, &dmaMenusCtx);
}

/* -----------------------------------------------
    Read/write the run-time registers
   ----------------------------------------------- */
static void MenuRwRegsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static MENU_CTX_RW_REGS regsMenusCtx;

    BZERO(regsMenusCtx);

    regsMenusCtx.phDev = phDev;
    regsMenusCtx.pRegsArr = gpAVALONMM_Regs;
    regsMenusCtx.dwRegsNum = AVALONMM_REGS_NUM;
    strcpy(regsMenusCtx.sModuleName, "AVALONMM");

    MenuCommonRwRegsInit(pParentMenu, &regsMenusCtx);
}





