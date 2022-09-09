/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/****************************************************************************
*  File: bmd_diag.c
*
*  Sample user-mode diagnostics application for accessing Xilinx PCI Express
*  cards with BMD design, using the WinDriver WDC API.
*  The sample was tested with Xilinx's Virtex and Spartan development kits.
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*****************************************************************************/

#include "diag_lib.h"
#include "wdc_diag_lib.h"
#include "pci_menus_common.h"
#include "bmd_lib.h"

/*************************************************************
  General definitions
 *************************************************************/
/* Error messages display */
#define BMD_ERR printf

/* --------------------------------------------------
    BMD configuration registers information
   -------------------------------------------------- */
/* Configuration registers information array */
static const WDC_REG gBMD_CfgRegs[] = {
    { WDC_AD_CFG_SPACE, PCI_VID, WDC_SIZE_16, WDC_READ_WRITE, "VID",
        "Vendor ID" },
    { WDC_AD_CFG_SPACE, PCI_DID, WDC_SIZE_16, WDC_READ_WRITE, "DID",
        "Device ID" },
    { WDC_AD_CFG_SPACE, PCI_CR, WDC_SIZE_16, WDC_READ_WRITE, "CMD",
        "Command" },
    { WDC_AD_CFG_SPACE, PCI_SR, WDC_SIZE_16, WDC_READ_WRITE, "STS", "Status" },
    { WDC_AD_CFG_SPACE, PCI_REV, WDC_SIZE_32, WDC_READ_WRITE, "RID_CLCD",
        "Revision ID & Class Code" },
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
        "CardBus CIS Pointer" },
    { WDC_AD_CFG_SPACE, PCI_SVID, WDC_SIZE_16, WDC_READ_WRITE, "SVID",
        "Sub-system Vendor ID" },
    { WDC_AD_CFG_SPACE, PCI_SDID, WDC_SIZE_16, WDC_READ_WRITE, "SDID",
        "Sub-system Device ID" },
    { WDC_AD_CFG_SPACE, PCI_EROM, WDC_SIZE_32, WDC_READ_WRITE, "EROM",
        "Expansion ROM Base Address" },
    { WDC_AD_CFG_SPACE, PCI_CAP, WDC_SIZE_8, WDC_READ_WRITE, "NEW_CAP",
        "New Capabilities Pointer" },
    { WDC_AD_CFG_SPACE, PCI_ILR, WDC_SIZE_32, WDC_READ_WRITE, "INTLN",
        "Interrupt Line" },
    { WDC_AD_CFG_SPACE, PCI_IPR, WDC_SIZE_32, WDC_READ_WRITE, "INTPIN",
        "Interrupt Pin" },
    { WDC_AD_CFG_SPACE, PCI_MGR, WDC_SIZE_32, WDC_READ_WRITE, "MINGNT",
        "Minimum Required Burst Period" },
    { WDC_AD_CFG_SPACE, PCI_MLR, WDC_SIZE_32, WDC_READ_WRITE, "MAXLAT",
        "Maximum Latency" },
};

#define BMD_CFG_REGS_NUM (sizeof(gBMD_CfgRegs)/sizeof(WDC_REG))

/* TODO: For read-only or write-only registers, change the direction field of
 *       the relevant registers in gBMD_CfgRegs to WDC_READ or WDC_WRITE. */
/* NOTE: You can define additional configuration registers in gBMD_CfgRegs. */
const WDC_REG *gpBMD_CfgRegs = gBMD_CfgRegs;

/* -----------------------------------------------
    BMD run-time registers information
   ----------------------------------------------- */
/* Run-time registers information array */
static const WDC_REG gBMD_Regs[] = {
    { BMD_SPACE, BMD_DSCR_OFFSET, WDC_SIZE_32, WDC_READ_WRITE, "DSCR",
        "Device Control Status Register" },
    { BMD_SPACE, BMD_DDMACR_OFFSET, WDC_SIZE_32, WDC_READ_WRITE,
        "DDMACR", "Device DMA Control Status Register" },
    { BMD_SPACE, BMD_WDMATLPA_OFFSET, WDC_SIZE_32, WDC_READ_WRITE,
        "WDMATLPA", "Write DMA TLP Address" },
    { BMD_SPACE, BMD_WDMATLPS_OFFSET, WDC_SIZE_32, WDC_READ_WRITE,
        "WDMATLPS", "Write DMA TLP Size" },
    { BMD_SPACE, BMD_WDMATLPC_OFFSET, WDC_SIZE_32, WDC_READ_WRITE,
        "WDMATLPC", "Write DMA TLP Count" },
    { BMD_SPACE, BMD_WDMATLPP_OFFSET, WDC_SIZE_32, WDC_READ_WRITE,
        "WDMATLPP", "Write DMA Data Pattern" },
    { BMD_SPACE, BMD_RDMATLPP_OFFSET, WDC_SIZE_32, WDC_READ_WRITE,
        "RDMATLPP", "Read DMA Expected Data Pattern" },
    { BMD_SPACE, BMD_RDMATLPA_OFFSET, WDC_SIZE_32, WDC_READ_WRITE,
        "RDMATLPA", "Read DMA TLP Address" },
    { BMD_SPACE, BMD_RDMATLPS_OFFSET, WDC_SIZE_32, WDC_READ_WRITE,
        "RDMATLPS", "Read DMA TLP Size" },
    { BMD_SPACE, BMD_RDMATLPC_OFFSET, WDC_SIZE_32, WDC_READ_WRITE,
        "RDMATLPC", "Read DMA TLP Count" },
    { BMD_SPACE, BMD_WDMAPERF_OFFSET, WDC_SIZE_32, WDC_READ, "WDMAPERF",
        "Write DMA Performance" },
    { BMD_SPACE, BMD_RDMAPERF_OFFSET, WDC_SIZE_32, WDC_READ, "RDMAPERF",
        "Read DMA Performance" },
    { BMD_SPACE, BMD_RDMASTAT_OFFSET, WDC_SIZE_32, WDC_READ, "RDMASTAT",
        "Read DMA Status" },
    { BMD_SPACE, BMD_NRDCOMP_OFFSET, WDC_SIZE_32, WDC_READ, "NRDCOMP",
        "Number of Read Completion w/ Data" },
    { BMD_SPACE, BMD_RCOMPDSIZE_OFFSET, WDC_SIZE_32, WDC_READ,
        "RCOMPDSIZE", "Read Completion Data Size" },
    { BMD_SPACE, BMD_DLWSTAT_OFFSET, WDC_SIZE_32, WDC_READ, "DLWSTAT",
        "Device Link Width Status" },
    { BMD_SPACE, BMD_DLTRSSTAT_OFFSET, WDC_SIZE_32, WDC_READ,
        "DLTRSSTAT", "Device Link Transaction Size Status" },
    { BMD_SPACE, BMD_DMISCCONT_OFFSET, WDC_SIZE_32, WDC_READ_WRITE,
        "DMISCCONT", "Device Miscellaneous Control" },
};

const WDC_REG *gpBMD_Regs = gBMD_Regs;

#define BMD_REGS_NUM (sizeof(gBMD_Regs)/sizeof(gBMD_Regs[0]))

typedef struct {
    BMD_DMA_HANDLE hDma;
    PVOID pBuf;
} DMA_STRUCT, *PDMA_STRUCT;

typedef struct {
    WDC_DEVICE_HANDLE *phDev;
    PDMA_STRUCT pDma;
} MENU_CTX_BMD;

/*************************************************************
  Static functions prototypes
 *************************************************************/
 /* -----------------------------------------------
     Main diagnostics menu
    ----------------------------------------------- */
static DIAG_MENU_OPTION *MenuMainInit(WDC_DEVICE_HANDLE *phDev);

/* -----------------------------------------------
   Device Open
   ----------------------------------------------- */
static void MenuDeviceOpenInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_BMD *pBmdMenuCtx);

/* -----------------------------------------------
    Device close
   ----------------------------------------------- */
static void DeviceClose(WDC_DEVICE_HANDLE hDev, PDMA_STRUCT pDma);

/* -----------------------------------------------
    Read/write memory and I/O addresses
   ----------------------------------------------- */
static void MenuReadWriteAddrInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

/* -----------------------------------------------
    Read/write the configuration space
   ----------------------------------------------- */
static void MenuCfgInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

/* -----------------------------------------------
    Read/write the run-time regs
   ----------------------------------------------- */
static void MenuRwRegsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */
static void MenuEventsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);
static void DiagEventHandler(WDC_DEVICE_HANDLE hDev, DWORD dwAction);


/* -----------------------------------------------
    Direct Memory Access (DMA)
   ---------------------------------------------- */
static void MenuDmaInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev, PDMA_STRUCT pDma);
static DWORD DmaOpen(PVOID pCbCtx);
static DWORD DmaClose(WDC_DEVICE_HANDLE hDev, PDMA_STRUCT pDma);
static void DiagDmaIntHandler(WDC_DEVICE_HANDLE hDev,
    BMD_INT_RESULT *pIntResult);
static void DmaTransferVerify(WDC_DEVICE_HANDLE hdev, PVOID pBuf,
    DWORD dwNumItems, UINT32 u32Pattern, BOOL fIsRead);

/*************************************************************
  Functions implementation
 *************************************************************/
static DWORD BMD_Init(WDC_DEVICE_HANDLE *phDev)
{
    /* Initialize the BMD library */
    DWORD dwStatus = BMD_LibInit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        BMD_ERR("bmd_diag: Failed to initialize the BMD library: %s",
            BMD_GetLastErr());
        return dwStatus;
    }

    /* Find and open a BMD device (by default ID) */
    *phDev = BMD_DeviceOpen(BMD_DEFAULT_VENDOR_ID, BMD_DEFAULT_DEVICE_ID);

    return WD_STATUS_SUCCESS;
}

int main(void)
{
    WDC_DEVICE_HANDLE hDev = NULL;
    DIAG_MENU_OPTION *pMenuRoot;
    DWORD dwStatus;

    printf("\n");
    printf("BMD diagnostic utility.\n");
    printf("Application accesses hardware using " WD_PROD_NAME ".\n");

    dwStatus = BMD_Init(&hDev);
    if (dwStatus)
        return dwStatus;

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
    MENU_CTX_BMD *pBmdMenuCtx = (MENU_CTX_BMD *)pCbCtx;
    DWORD dwStatus;

    /* Perform necessary cleanup before exiting the program: */
    /* Close the device handle */
    if (*pBmdMenuCtx->phDev)
        BMD_DeviceClose(*pBmdMenuCtx->phDev);

    /* Uninitialize libraries */
    dwStatus = BMD_LibUninit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        BMD_ERR("bmd_diag: Failed to uninitialize the BMD library: %s",
            BMD_GetLastErr());
    }

    return dwStatus;
}

static DIAG_MENU_OPTION *MenuMainInit(WDC_DEVICE_HANDLE *phDev)
{
    static DMA_STRUCT dma = { 0 };

    static MENU_CTX_BMD bmdMenuCtx = { 0 };
    static DIAG_MENU_OPTION menuRoot = { 0 };

    bmdMenuCtx.phDev = phDev;
    bmdMenuCtx.pDma = &dma;

    strcpy(menuRoot.cTitleName, "BMD main menu");
    menuRoot.cbExit = MenuMainExitCb;
    menuRoot.pCbCtx = &bmdMenuCtx;

    MenuCommonScanBusInit(&menuRoot);
    MenuDeviceOpenInit(&menuRoot, &bmdMenuCtx);
    MenuReadWriteAddrInit(&menuRoot, phDev);
    MenuCfgInit(&menuRoot, phDev);
    MenuRwRegsInit(&menuRoot, phDev);
    MenuDmaInit(&menuRoot, phDev, &dma);
    MenuEventsInit(&menuRoot, phDev);

  
    return &menuRoot;
}

/* -----------------------------------------------
   Device Open
   ----------------------------------------------- */
static DWORD MenuDeviceOpenCb(PVOID pCbCtx)
{
    MENU_CTX_BMD *pBmdMenuCtx = (MENU_CTX_BMD *)pCbCtx;

    if (*pBmdMenuCtx->phDev)
        DeviceClose(*pBmdMenuCtx->phDev, pBmdMenuCtx->pDma);

    *(pBmdMenuCtx->phDev) = BMD_DeviceOpen(0, 0);

    return WD_STATUS_SUCCESS;
}

static void MenuDeviceOpenInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_BMD *pBmdMenuCtx)
{
    static DIAG_MENU_OPTION menuDeviceOpen = { 0 };

    strcpy(menuDeviceOpen.cOptionName, "Find and open a BMD device");
    menuDeviceOpen.cbEntry = MenuDeviceOpenCb;

    DIAG_MenuSetCtxAndParentForMenus(&menuDeviceOpen, 1, pBmdMenuCtx,
        pParentMenu);
}

/* Close a handle to a BMD device */
static void DeviceClose(WDC_DEVICE_HANDLE hDev, PDMA_STRUCT pDma)
{
    /* Validate the WDC device handle */
    if (!hDev)
        return;

    /* Close the DMA handle (if open) */
    if (pDma)
        DmaClose(hDev, pDma);

    /* Close the WDC device handle */
    if (!BMD_DeviceClose(hDev))
    {
        BMD_ERR("DeviceClose: Failed closing BMD device: %s",
            BMD_GetLastErr());
    }
}

/* -----------------------------------------------
    Read/write memory and I/O addresses
   ----------------------------------------------- */
static void MenuReadWriteAddrInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static MENU_CTX_READ_WRITE_ADDR rwAddrMenusCtx;

    rwAddrMenusCtx.phDev = phDev;
    rwAddrMenusCtx.fBlock = FALSE;
    rwAddrMenusCtx.mode = WDC_MODE_32;
    rwAddrMenusCtx.dwAddrSpace = ACTIVE_ADDR_SPACE_NEEDS_INIT;

    MenuCommonRwAddrInit(pParentMenu, &rwAddrMenusCtx);
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
    cfgCtx.pCfgRegs = gBMD_CfgRegs;
    cfgCtx.dwCfgRegsNum = BMD_CFG_REGS_NUM;

    MenuCommonCfgInit(pParentMenu, &cfgCtx);
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
    regsMenusCtx.pRegsArr = gBMD_Regs;
    regsMenusCtx.dwRegsNum = BMD_REGS_NUM;
    strcpy(regsMenusCtx.sModuleName, "BMD");

    MenuCommonRwRegsInit(pParentMenu, &regsMenusCtx);
}

/* -----------------------------------------------
    Direct Memory Access (DMA)
   ----------------------------------------------- */
typedef struct {
    WDC_DEVICE_HANDLE *phDev;
    PDMA_STRUCT pDma;
    BOOL fPolling;
} MENU_CTX_DMA;

static BOOL MenuDmaIsDeviceNull(DIAG_MENU_OPTION *pMenu)
{
    return *((MENU_CTX_DMA *)pMenu->pCbCtx)->phDev == NULL;
}

static BOOL MenuDmaIsDmaHandleNotNull(DIAG_MENU_OPTION *pMenu)
{
    return ((MENU_CTX_DMA *)pMenu->pCbCtx)->pDma->hDma != NULL;
}

static BOOL MenuDmaIsDmaHandleNull(DIAG_MENU_OPTION *pMenu)
{
    return ((MENU_CTX_DMA *)pMenu->pCbCtx)->pDma->hDma == NULL;
}

static DWORD MenuDmaCloseOptionCb(PVOID pCbCtx)
{
    MENU_CTX_DMA *pMenuDmaCtx = (MENU_CTX_DMA *)pCbCtx;

    return DmaClose(*(pMenuDmaCtx->phDev), pMenuDmaCtx->pDma);
}

static void MenuDmaPollingSubMenusInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_DMA *pMenuDmaCtx)
{
    static DIAG_MENU_OPTION openDmaMenu = { 0 };
    static DIAG_MENU_OPTION closeDmaMenu = { 0 };
    static DIAG_MENU_OPTION options[2] = { 0 };

    strcpy(openDmaMenu.cOptionName, "Open DMA");
    openDmaMenu.cbEntry = DmaOpen;
    openDmaMenu.cbIsHidden = MenuDmaIsDmaHandleNotNull;

    strcpy(closeDmaMenu.cOptionName, "Close DMA");
    closeDmaMenu.cbEntry = MenuDmaCloseOptionCb;
    closeDmaMenu.cbIsHidden = MenuDmaIsDmaHandleNull;

    options[0] = openDmaMenu;
    options[1] = closeDmaMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        pMenuDmaCtx, pParentMenu);
}
#ifdef HAS_INTS
static void MenuDmaInterruptsSubMenusInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_DMA *pMenuDmaCtx)
{
    static DIAG_MENU_OPTION openDmaMenu = { 0 };
    static DIAG_MENU_OPTION closeDmaMenu = { 0 };
    static DIAG_MENU_OPTION options[2] = { 0 };

    strcpy(openDmaMenu.cOptionName, "Open DMA");
    openDmaMenu.cbEntry = DmaOpen;
    openDmaMenu.cbIsHidden = MenuDmaIsDmaHandleNotNull;

    strcpy(closeDmaMenu.cOptionName, "Close DMA");
    closeDmaMenu.cbEntry = MenuDmaCloseOptionCb;
    closeDmaMenu.cbIsHidden = MenuDmaIsDmaHandleNull;

    options[0] = openDmaMenu;
    options[1] = closeDmaMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        pMenuDmaCtx, pParentMenu);
}

static DWORD MenuDmaInterruptsCb(PVOID pCbCtx)
{
    MENU_CTX_DMA *pDmaMenuCtx = (MENU_CTX_DMA *)pCbCtx;

    pDmaMenuCtx->fPolling = FALSE;
    printf("Open/close Direct Memory Access (DMA)\n"
        "using interrupts completion method\n");
    printf("----------------------------------\n");

    return WD_STATUS_SUCCESS;
}
#endif /* ifdef HAS_INTS */

static DWORD MenuDmaPollingCb(PVOID pCbCtx)
{
    MENU_CTX_DMA* pDmaMenuCtx = (MENU_CTX_DMA*)pCbCtx;

    pDmaMenuCtx->fPolling = TRUE;
    printf("Open/close Direct Memory Access (DMA)\n"
        "using polling completion method\n");
    printf("----------------------------------\n");

    return WD_STATUS_SUCCESS;
}
static void MenuDmaInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev, PDMA_STRUCT pDma)
{
    static DIAG_MENU_OPTION dmaPollingMenuRoot = { 0 };
    static DIAG_MENU_OPTION dmaInterruptsMenuRoot = { 0 };
    static MENU_CTX_DMA dmaMenuCtx = { 0 };

    strcpy(dmaPollingMenuRoot.cOptionName, "Open/close Direct Memory Access "
        "(DMA) using polling completion method");
    dmaPollingMenuRoot.cbEntry = MenuDmaPollingCb;
    dmaPollingMenuRoot.cbIsHidden = MenuDmaIsDeviceNull;

    dmaMenuCtx.phDev = phDev;
    dmaMenuCtx.pDma = pDma;

    /* Polling branch */
    MenuDmaPollingSubMenusInit(&dmaPollingMenuRoot, &dmaMenuCtx);
    DIAG_MenuSetCtxAndParentForMenus(&dmaPollingMenuRoot, 1, &dmaMenuCtx,
        pParentMenu);

#ifdef HAS_INTS
    strcpy(dmaInterruptsMenuRoot.cOptionName, "Open/close Direct Memory "
        "Access (DMA) using interrupts completion method");
    dmaInterruptsMenuRoot.cbEntry = MenuDmaInterruptsCb;
    dmaInterruptsMenuRoot.cbIsHidden = MenuDmaIsDeviceNull;

    /* Interrupts branch */

    MenuDmaInterruptsSubMenusInit(&dmaInterruptsMenuRoot, &dmaMenuCtx);
    DIAG_MenuSetCtxAndParentForMenus(&dmaInterruptsMenuRoot, 1, &dmaMenuCtx,
        pParentMenu);
#endif /* ifdef HAS_INTS */
}

#ifdef HAS_INTS
/* Diagnostics DMA interrupt handler routine */
static void DiagDmaIntHandler(WDC_DEVICE_HANDLE hDev,
    BMD_INT_RESULT *pIntResult)
{
    printf("\n###\nDMA %s based interrupt, received #%d\n",
        pIntResult->fIsMessageBased ? "message" : "line",
        pIntResult->dwCounter);
    if (pIntResult->fIsMessageBased)
        printf("Message data 0x%x\n", pIntResult->dwLastMessage);

    printf("###\n\n");
    DmaTransferVerify(hDev, pIntResult->pBuf, pIntResult->dwBufNumItems,
        pIntResult->u32Pattern, pIntResult->fIsRead);
}
#endif /* ifdef HAS_INTS */

/* DMA user input menu */
static BOOL MenuDmaOpenGetInput(PWORD pwNumTLPs, UINT32 *pu32Pattern,
    PDWORD pdwOptions)
{
    DWORD option;

    /* Get DMA direction and set the DMA options accordingly */
    printf("\nSelect DMA direction:\n");
    printf("1. From device\n");
    printf("2. To device\n");
    printf("%d. Cancel\n", DIAG_EXIT_MENU);
    if ((DIAG_INPUT_SUCCESS != DIAG_GetMenuOption(&option, 2)) ||
        (DIAG_EXIT_MENU == option))
    {
        return FALSE;
    }

    *pdwOptions = (1 == option) ? DMA_FROM_DEVICE : DMA_TO_DEVICE;

    /* Get number of Transaction Layer Packets (TLPs) for the DMA transfer */
    if (DIAG_INPUT_SUCCESS != DIAG_InputWORD(pwNumTLPs,
        "\nEnter DMA TLP count", FALSE, 1, 65535))
    {
        return FALSE;
    }

    /* Get data pattern */
    if (DIAG_INPUT_SUCCESS != DIAG_InputUINT32(pu32Pattern,
        "\nEnter DMA data pattern", TRUE, 0, 0))
    {
        return FALSE;
    }

    printf("\n");

    return TRUE;
}

/* Verify a DMA transfer */
static void DmaTransferVerify(WDC_DEVICE_HANDLE hDev, PVOID pBuf,
    DWORD dwNumItems, UINT32 u32Pattern, BOOL fIsRead)
{
    DWORD i;

    if (fIsRead) /* Verify a host-to-device (read) DMA transfer */
    {
        /* Verify the transfer by reading the device's HW error bit */
        if (!BMD_DmaIsReadSucceed(hDev))
            goto Error;
    }
    else /* Verify a device-to-host (write) DMA transfer */
    {
        /* Verify the transfer by comparing the data in each element of the
         * host's DMA buffer (pBuf) to the original data pattern (u32Pattern) */
        for (i = 0; i < dwNumItems; i++)
        {
            if (((UINT32 *)(pBuf))[i] != u32Pattern)
            {
                BMD_ERR("Device-to-host (write) DMA data mismatch: host data "
                    "(pBuf[%d]) = %08X, original data pattern (u32Pattern) = "
                    "%08X\n", i, ((UINT32 *)(pBuf))[i], u32Pattern);
                goto Error;
            }
        }

    }

    printf("DMA transfer completed and verified.\n");
    return;

Error:
    printf("DMA transfer verification failed.\n");
}

/* Open DMA */
static DWORD DmaOpen(PVOID pCbCtx)
{
    MENU_CTX_DMA *pDmaMenuCtx = (MENU_CTX_DMA *)pCbCtx;

    WDC_DEVICE_HANDLE hDev = *(pDmaMenuCtx->phDev);
    PDMA_STRUCT pDma = pDmaMenuCtx->pDma;
    BOOL fPolling = pDmaMenuCtx->fPolling;
    DWORD dwStatus, dwOptions, dwNumItems, i;
    WORD wTLPNumItems, wNumTLPs;
    UINT32 u32Pattern;
    BOOL fIsRead, fEnable64bit;
    BYTE bTrafficClass;

    /* Get user input */
    if (!MenuDmaOpenGetInput(&wNumTLPs, &u32Pattern, &dwOptions))
        return WD_INVALID_PARAMETER;

    /* Determine the DMA direction: host-to-device=read; device-to-host=write */
    fIsRead = dwOptions & DMA_FROM_DEVICE ? FALSE : TRUE;

    /* The BMD reference design does not support Scatter/Gather DMA, so we use
     * contiguous buffer DMA */
    dwOptions |= DMA_KERNEL_BUFFER_ALLOC;

    /* Calculate the DMA TLP (payload packet) size, in units of UINT32 */
    wTLPNumItems = BMD_DmaGetMaxPacketSize(hDev, fIsRead) / sizeof(UINT32);

    /* Calculate the total DMA transfer size, in units of UINT32 */
    dwNumItems = (DWORD)wNumTLPs * (DWORD)wTLPNumItems;

    /* Open a DMA handle */
    dwStatus = BMD_DmaOpen(hDev, &pDma->pBuf, dwOptions,
        dwNumItems * sizeof(UINT32), &pDma->hDma);

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        printf("\nFailed to open DMA handle. Error 0x%x - %s\n", dwStatus,
            Stat2Str(dwStatus));
        return dwStatus;
    }

    printf("\nDMA handle was opened successfully (handle 0x%p)\n",
        (void *)pDma->hDma);
    printf("DMA TLP size, in units of UINT32: 0x%hx\n", wTLPNumItems);

    /* Initialize the DMA buffer using the user-defined pattern */
    for (i = 0; i < dwNumItems; i++)
    {
        if (fIsRead) /* Host-to-device (read) DMA buffer */
        {
            /* Initialize the host's DMA buffer using the given data pattern */
            ((UINT32 *)(pDma->pBuf))[i] = u32Pattern;
        }
        else /* Device-to-host (write) DMA buffer */
        {
            /* Initialize the host's DMA buffer with dummy data, to be
             * overwritten later by a device-to-host (write) DMA transfer */
            ((UINT32 *)(pDma->pBuf))[i] = 0xdeadbeaf;
        }
    }

    /* Prepare the device registers for DMA transfer */
    fEnable64bit = FALSE;
    bTrafficClass = 0;
    BMD_DmaDevicePrepare(pDma->hDma, fIsRead, wTLPNumItems, wNumTLPs,
        u32Pattern, fEnable64bit, bTrafficClass);

#ifdef HAS_INTS
    if (!fPolling) /* Interrupt-based DMA */
    {
        /* Enable DMA interrupts */
        BMD_DmaIntEnable(hDev, fIsRead);

        if (!BMD_IntIsEnabled(hDev))
        {
            dwStatus = BMD_IntEnable(hDev, DiagDmaIntHandler);

            if (WD_STATUS_SUCCESS != dwStatus)
            {
                printf("\nFailed enabling DMA interrupts. Error 0x%x - %s\n",
                    dwStatus, Stat2Str(dwStatus));
                goto Error;
            }

            printf("\nDMA interrupts enabled\n");
        }
    }
    else /* Polling-based DMA */
    {
        /* Disable DMA interrupts */
        BMD_DmaIntDisable(hDev, fIsRead);
    }
#endif /* ifdef HAS_INTS */

    /* Start DMA transfer */
    printf("Start DMA transfer\n");
    BMD_DmaStart(pDma->hDma, fIsRead);

    /* Poll for DMA completion (when using polling-based DMA) */
    if (fPolling)
    {
        if (BMD_DmaPollCompletion(pDma->hDma, fIsRead))
        {
            /* Verify the DMA transfer */
            DmaTransferVerify(hDev, pDma->pBuf, dwNumItems, u32Pattern,
                fIsRead);
        }
        else
            printf("DMA transfer completion polling timeout\n");
    }

    goto Exit;
#ifdef HAS_INTS
Error:
    DmaClose(hDev, pDma);
#endif /* ifdef HAS_INTS */
Exit:
    return dwStatus;
}

/* Close DMA */
static DWORD DmaClose(WDC_DEVICE_HANDLE hDev, PDMA_STRUCT pDma)
{

    if (!pDma)
        return WD_STATUS_SUCCESS;

#ifdef HAS_INTS
    /* Disable DMA interrupts */
    if (BMD_IntIsEnabled(hDev))
    {
        DWORD dwStatus = BMD_IntDisable(hDev);
        printf("DMA interrupts disable%s\n",
            (WD_STATUS_SUCCESS == dwStatus) ? "d" : " failed");
    }
#endif /* ifdef HAS_INTS */

    if (pDma->hDma)
    {
        /* Close the device's DMA handle */
        BMD_DmaClose((void *)pDma->hDma);
        printf("DMA closed (handle 0x%p)\n", (void *)pDma->hDma);
    }

    BZERO(*pDma);

    return WD_STATUS_SUCCESS;
}

/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */
static DWORD MenuEventsRegisterOptionCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    DWORD dwStatus = BMD_EventRegister(*(pEventsMenusCtx->phDev),
        (BMD_EVENT_HANDLER)pEventsMenusCtx->DiagEventHandler);

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Events registered\n");
        pEventsMenusCtx->fRegistered = TRUE;
    }
    else
    {
        BMD_ERR("Failed to register events. Last error [%s]\n",
            BMD_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuEventsUnregisterOptionCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    DWORD dwStatus = BMD_EventUnregister(*(pEventsMenusCtx->phDev));

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Events unregistered\n");
        pEventsMenusCtx->fRegistered = FALSE;
    }
    else
    {
        BMD_ERR("Failed to unregister events. Last error [%s]\n",
            BMD_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuEventsCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    pEventsMenusCtx->fRegistered = BMD_EventIsRegistered(
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

/* Diagnostics plug-and-play and power management events handler routine */
static void DiagEventHandler(WDC_DEVICE_HANDLE hDev, DWORD dwAction)
{
    /* TODO: You can modify this function in order to implement your own
             diagnostics events handler routine. */

    printf("\nReceived event notification (device handle 0x%p): ", hDev);
    switch (dwAction)
    {
    case WD_INSERT:
        printf("WD_INSERT\n");
        break;
    case WD_REMOVE:
        printf("WD_REMOVE\n");
        break;
    case WD_POWER_CHANGED_D0:
        printf("WD_POWER_CHANGED_D0\n");
        break;
    case WD_POWER_CHANGED_D1:
        printf("WD_POWER_CHANGED_D1\n");
        break;
    case WD_POWER_CHANGED_D2:
        printf("WD_POWER_CHANGED_D2\n");
        break;
    case WD_POWER_CHANGED_D3:
        printf("WD_POWER_CHANGED_D3\n");
        break;
    case WD_POWER_SYSTEM_WORKING:
        printf("WD_POWER_SYSTEM_WORKING\n");
        break;
    case WD_POWER_SYSTEM_SLEEPING1:
        printf("WD_POWER_SYSTEM_SLEEPING1\n");
        break;
    case WD_POWER_SYSTEM_SLEEPING2:
        printf("WD_POWER_SYSTEM_SLEEPING2\n");
        break;
    case WD_POWER_SYSTEM_SLEEPING3:
        printf("WD_POWER_SYSTEM_SLEEPING3\n");
        break;
    case WD_POWER_SYSTEM_HIBERNATE:
        printf("WD_POWER_SYSTEM_HIBERNATE\n");
        break;
    case WD_POWER_SYSTEM_SHUTDOWN:
        printf("WD_POWER_SYSTEM_SHUTDOWN\n");
        break;
    default:
        printf("0x%x\n", dwAction);
        break;
    }
}


