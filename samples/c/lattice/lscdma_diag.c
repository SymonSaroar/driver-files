/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/****************************************************************************
*  File: lscdma_diag.c
*
*  Sample user-mode diagnostics application for accessing Lattice PCI Express
*  cards with SGDMA support, using the WinDriver WDC API.
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*****************************************************************************/

#include "diag_lib.h"
#include "wdc_diag_lib.h"
#include "pci_menus_common.h"
#include "lscdma_lib.h"

/*************************************************************
  General definitions
 *************************************************************/
/* Error messages display */
#define LSCDMA_ERR printf

/* --------------------------------------------------
    LSCDMA configuration registers information
   -------------------------------------------------- */
/* Configuration registers information array */
const WDC_REG gLSCDMA_CfgRegs[] = {
    { WDC_AD_CFG_SPACE, PCI_VID, WDC_SIZE_16, WDC_READ_WRITE, "VID", "Vendor ID" },
    { WDC_AD_CFG_SPACE, PCI_DID, WDC_SIZE_16, WDC_READ_WRITE, "DID", "Device ID" },
    { WDC_AD_CFG_SPACE, PCI_CR, WDC_SIZE_16, WDC_READ_WRITE, "CMD", "Command" },
    { WDC_AD_CFG_SPACE, PCI_SR, WDC_SIZE_16, WDC_READ_WRITE, "STS", "Status" },
    { WDC_AD_CFG_SPACE, PCI_REV, WDC_SIZE_32, WDC_READ_WRITE, "RID_CLCD", "Revision ID & Class Code" },
    { WDC_AD_CFG_SPACE, PCI_CCSC, WDC_SIZE_8, WDC_READ_WRITE, "SCC", "Sub Class Code" },
    { WDC_AD_CFG_SPACE, PCI_CCBC, WDC_SIZE_8, WDC_READ_WRITE, "BCC", "Base Class Code" },
    { WDC_AD_CFG_SPACE, PCI_CLSR, WDC_SIZE_8, WDC_READ_WRITE, "CALN", "Cache Line Size" },
    { WDC_AD_CFG_SPACE, PCI_LTR, WDC_SIZE_8, WDC_READ_WRITE, "LAT", "Latency Timer" },
    { WDC_AD_CFG_SPACE, PCI_HDR, WDC_SIZE_8, WDC_READ_WRITE, "HDR", "Header Type" },
    { WDC_AD_CFG_SPACE, PCI_BISTR, WDC_SIZE_8, WDC_READ_WRITE, "BIST", "Built-in Self Test" },
    { WDC_AD_CFG_SPACE, PCI_BAR0, WDC_SIZE_32, WDC_READ_WRITE, "BADDR0", "Base Address 0" },
    { WDC_AD_CFG_SPACE, PCI_BAR1, WDC_SIZE_32, WDC_READ_WRITE, "BADDR1", "Base Address 1" },
    { WDC_AD_CFG_SPACE, PCI_BAR2, WDC_SIZE_32, WDC_READ_WRITE, "BADDR2", "Base Address 2" },
    { WDC_AD_CFG_SPACE, PCI_BAR3, WDC_SIZE_32, WDC_READ_WRITE, "BADDR3", "Base Address 3" },
    { WDC_AD_CFG_SPACE, PCI_BAR4, WDC_SIZE_32, WDC_READ_WRITE, "BADDR4", "Base Address 4" },
    { WDC_AD_CFG_SPACE, PCI_BAR5, WDC_SIZE_32, WDC_READ_WRITE, "BADDR5", "Base Address 5" },
    { WDC_AD_CFG_SPACE, PCI_CIS, WDC_SIZE_32, WDC_READ_WRITE, "CIS", "CardBus CIS Pointer" },
    { WDC_AD_CFG_SPACE, PCI_SVID, WDC_SIZE_16, WDC_READ_WRITE, "SVID", "Sub-system Vendor ID" },
    { WDC_AD_CFG_SPACE, PCI_SDID, WDC_SIZE_16, WDC_READ_WRITE, "SDID", "Sub-system Device ID" },
    { WDC_AD_CFG_SPACE, PCI_EROM, WDC_SIZE_32, WDC_READ_WRITE, "EROM", "Expansion ROM Base Address" },
    { WDC_AD_CFG_SPACE, PCI_CAP, WDC_SIZE_8, WDC_READ_WRITE, "NEW_CAP", "New Capabilities Pointer" },
    { WDC_AD_CFG_SPACE, PCI_ILR, WDC_SIZE_32, WDC_READ_WRITE, "INTLN", "Interrupt Line" },
    { WDC_AD_CFG_SPACE, PCI_IPR, WDC_SIZE_32, WDC_READ_WRITE, "INTPIN", "Interrupt Pin" },
    { WDC_AD_CFG_SPACE, PCI_MGR, WDC_SIZE_32, WDC_READ_WRITE, "MINGNT", "Minimum Required Burst Period" },
    { WDC_AD_CFG_SPACE, PCI_MLR, WDC_SIZE_32, WDC_READ_WRITE, "MAXLAT", "Maximum Latency" },
};
#define LSCDMA_CFG_REGS_NUM (sizeof(gLSCDMA_CfgRegs) / sizeof(WDC_REG))
/* NOTE: You can define additional configuration registers in gLSCDMA_CfgRegs. */

/* -----------------------------------------------
    LSCDMA run-time registers information
   ----------------------------------------------- */
/* Run-time registers information array */
const WDC_REG gLSCDMA_Regs[] =
{
    /* GPIO registers */
    { AD_PCI_BAR0, GPIO_ID_REG_OFFSET, WDC_SIZE_32, WDC_READ, "GPIO_ID_REG", "GPIO ID register" },
    { AD_PCI_BAR0, GPIO_SCRACH_OFFSET, WDC_SIZE_32, WDC_READ_WRITE, "SCRACH", "Scratch pad" },
    { AD_PCI_BAR0, GPIO_LED14SEG_OFFSET, WDC_SIZE_16, WDC_READ_WRITE, "LED14SEG", "14 segment LED" },
    { AD_PCI_BAR0, GPIO_DIPSW_OFFSET, WDC_SIZE_16, WDC_READ_WRITE, "DIPSW", "DIP switch value" },
    { AD_PCI_BAR0, GPIO_CNTRCTRL_OFFSET, WDC_SIZE_32, WDC_READ_WRITE, "CNTRCTRL", "Generic down counter control" },
    { AD_PCI_BAR0, GPIO_CNTRVAL_OFFSET, WDC_SIZE_32, WDC_READ_WRITE, "CNTRVAL", "Counter value" },
    { AD_PCI_BAR0, GPIO_CNTRRELOAD_OFFSET, WDC_SIZE_32, WDC_READ_WRITE, "CNTRRELOAD", "Counter reload value" },
    { AD_PCI_BAR0, GPIO_DMAREQ_OFFSET, WDC_SIZE_32, WDC_READ_WRITE, "DMAREQ", "DMA Request & DMA Ack (per channel)" },
    { AD_PCI_BAR0, GPIO_WR_CNTR_OFFSET, WDC_SIZE_32, WDC_READ_WRITE, "WR_CNTR", "DMA Write Counter" },
    { AD_PCI_BAR0, GPIO_RD_CNTR_OFFSET, WDC_SIZE_32, WDC_READ_WRITE, "RD_CNTR", "DMA Read Counter" },

    /* Interrupt controller registers */
    { AD_PCI_BAR0, INTCTL_ID_OFFSET, WDC_SIZE_32, WDC_READ_WRITE, "INTCTL_ID", "Interrupt controller ID" },
    { AD_PCI_BAR0, INTCTL_CTRL_OFFSET, WDC_SIZE_32, WDC_READ_WRITE, "INTCTL_CTRL", "Interrupt control register" },
    { AD_PCI_BAR0, INTCTL_STATUS_OFFSET, WDC_SIZE_32, WDC_READ_WRITE, "INTCTL_STATUS", "Interrupt status register" },
    { AD_PCI_BAR0, INTCTL_ENABLE_OFFSET, WDC_SIZE_32, WDC_READ_WRITE, "INTCTL_ENABLE", "Interrupt mask for all sources" },

    /* SGDMA registers */
    { AD_PCI_BAR0, SGDMA_IPID_OFFSET, WDC_SIZE_32, WDC_READ, "SGDMA_IPID", "SGDMA IP identification register" },
    { AD_PCI_BAR0, SGDMA_IPVER_OFFSET, WDC_SIZE_32, WDC_READ, "SGDMA_IPVER", "SGDMA IP version register" },
    { AD_PCI_BAR0, SGDMA_GCONTROL_OFFSET, WDC_SIZE_32, WDC_READ_WRITE, "SGDMA_GCONTROL", "SGDMA global control register" },
    { AD_PCI_BAR0, SGDMA_GSTATUS_OFFSET, WDC_SIZE_32, WDC_READ_WRITE, "SGDMA_GSTATUS", "SGDMA global status register" },
    { AD_PCI_BAR0, SGDMA_GEVENT_OFFSET, WDC_SIZE_32, WDC_READ_WRITE, "SGDMA_GEVENT", "SGDMA global channel event register and mask" },
    { AD_PCI_BAR0, SGDMA_GERROR_OFFSET, WDC_SIZE_32, WDC_READ_WRITE, "SGDMA_GERROR", "SGDMA global channel error register and mask" },
    { AD_PCI_BAR0, SGDMA_GARBITER_OFFSET, WDC_SIZE_32, WDC_READ_WRITE, "SGDMA_GARBITER", "SGDMA global arbiter control register" },
    { AD_PCI_BAR0, SGDMA_GAUX_OFFSET, WDC_SIZE_32, WDC_READ_WRITE, "SGDMA_GAUX", "SGDMA auxiliary inputs and outputs" },
};
#define LSCDMA_REGS_NUM (sizeof(gLSCDMA_Regs) / sizeof(WDC_REG))
/* NOTE: You can define additional run-time registers in gLSCDMA_CfgRegs. */

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
    WDC_DEVICE_HANDLE *phDev);

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
    Interrupt handling
   ----------------------------------------------- */
static void MenuInterruptsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);
static void DiagIntHandler(WDC_DEVICE_HANDLE hDev, LSCDMA_INT_RESULT *pIntResult);

/* -----------------------------------------------
    Direct Memory Access (DMA)
   ---------------------------------------------- */
static void MenuDmaInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

/* ----------------------------------------------------
    Plug-and-play and power management events handling
   ---------------------------------------------------- */
static void MenuEventsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);
static void DiagEventHandler(WDC_DEVICE_HANDLE hDev, DWORD dwAction);

/*************************************************************
  Functions implementation
 *************************************************************/
static DWORD LSCDMA_Init(WDC_DEVICE_HANDLE *phDev)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;

    /* Initialize the LSCDMA library */
    dwStatus = LSCDMA_LibInit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        LSCDMA_ERR("xdma_diag: Failed to initialize the XDMA library: %s",
            LSCDMA_GetLastErr());
        return dwStatus;
    }

    /* Find and open a LSCDMA device (by default ID) */
    *phDev = LSCDMA_DeviceOpen(LSCDMA_DEFAULT_VENDOR_ID,
        LSCDMA_DEFAULT_DEVICE_ID);

    return dwStatus;
}

int main(void)
{
    WDC_DEVICE_HANDLE hDev = NULL;
    DIAG_MENU_OPTION *pMenuRoot;
    DWORD dwStatus;

    printf("\n");
    printf("LSCDMA diagnostic utility.\n");
    printf("Application accesses hardware using " WD_PROD_NAME "\n");

    dwStatus = LSCDMA_Init(&hDev);
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
    WDC_DEVICE_HANDLE hDev = *(WDC_DEVICE_HANDLE *)pCbCtx;
    DWORD dwStatus;

    /* Perform necessary cleanup before exiting the program */
    if (hDev && !LSCDMA_DeviceClose(hDev))
        LSCDMA_ERR("lscdma_diag: Failed closing LSCDMA device: %s",
            LSCDMA_GetLastErr());

    dwStatus = LSCDMA_LibUninit();
    if (WD_STATUS_SUCCESS != dwStatus)
        LSCDMA_ERR("lscdma_diag: Failed to uninit the LSCDMA library: %s",
            LSCDMA_GetLastErr());

    return dwStatus;
}

static DIAG_MENU_OPTION *MenuMainInit(WDC_DEVICE_HANDLE *phDev)
{
    static DIAG_MENU_OPTION menuRoot = { 0 };

    strcpy(menuRoot.cTitleName, "LSCDMA main menu");
    menuRoot.cbExit = MenuMainExitCb;
    menuRoot.pCbCtx = phDev;

    MenuCommonScanBusInit(&menuRoot);
    MenuDeviceOpenInit(&menuRoot, phDev);
    MenuReadWriteAddrInit(&menuRoot, phDev);
    MenuCfgInit(&menuRoot, phDev);
    MenuRwRegsInit(&menuRoot, phDev);
    MenuDmaInit(&menuRoot, phDev);
    MenuEventsInit(&menuRoot, phDev);
    MenuInterruptsInit(&menuRoot, phDev);

    return &menuRoot;
}

/* -----------------------------------------------
   Device Open
   ----------------------------------------------- */
static DWORD MenuDeviceOpenCb(PVOID pCbCtx)
{
    WDC_DEVICE_HANDLE *phDev = (WDC_DEVICE_HANDLE *)pCbCtx;

    if (*phDev && !LSCDMA_DeviceClose(*phDev))
        LSCDMA_ERR("lscdma_diag: Failed closing LSCDMA device: %s",
            LSCDMA_GetLastErr());

    *phDev = LSCDMA_DeviceOpen(0, 0);

    return WD_STATUS_SUCCESS;
}

static void MenuDeviceOpenInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static DIAG_MENU_OPTION menuDeviceOpen = { 0 };

    strcpy(menuDeviceOpen.cOptionName, "Find and open a LSCDMA device");
    menuDeviceOpen.cbEntry = MenuDeviceOpenCb;

    DIAG_MenuSetCtxAndParentForMenus(&menuDeviceOpen, 1, phDev,
        pParentMenu);
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

/* -----------------------------------------------
    Read/write the configuration space
   ----------------------------------------------- */
static void MenuCfgInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static MENU_CTX_CFG cfgCtx;

    BZERO(cfgCtx);
    cfgCtx.phDev = phDev;
    cfgCtx.pCfgRegs = gLSCDMA_CfgRegs;
    cfgCtx.dwCfgRegsNum = LSCDMA_CFG_REGS_NUM;

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
    regsMenusCtx.pRegsArr = gLSCDMA_Regs;
    regsMenusCtx.dwRegsNum = LSCDMA_REGS_NUM;
    strcpy(regsMenusCtx.sModuleName, "LSCDMA");

    MenuCommonRwRegsInit(pParentMenu, &regsMenusCtx);
}

/* -----------------------------------------------
    Interrupt handling
   ----------------------------------------------- */
static DWORD MenuInterruptsEnableOptionCb(PVOID pCbCtx)
{
    MENU_CTX_INTERRUPTS *pInterruptsMenusCtx = (MENU_CTX_INTERRUPTS *)pCbCtx;
    DWORD dwStatus = LSCDMA_IntEnable(*(pInterruptsMenusCtx->phDev),
        (LSCDMA_INT_HANDLER)pInterruptsMenusCtx->funcIntHandler);

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Interrupts enabled\n");
        pInterruptsMenusCtx->fIntsEnabled = TRUE;
    }
    else
    {
        LSCDMA_ERR("Failed enabling interrupts. Error [0x%x - %s]\n",
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

static DWORD MenuInterruptsDisableOptionCb(PVOID pCbCtx)
{
    MENU_CTX_INTERRUPTS *pInterruptsMenusCtx = (MENU_CTX_INTERRUPTS *)pCbCtx;
    DWORD dwStatus = LSCDMA_IntDisable(*(pInterruptsMenusCtx->phDev));

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Interrupts disabled\n");
        pInterruptsMenusCtx->fIntsEnabled = FALSE;
    }
    else
    {
        LSCDMA_ERR("Failed disabling interrupts: %s\n", LSCDMA_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuInterruptsCb(PVOID pCbCtx)
{
    DWORD dwIntOptions;
    BOOL fIsMsi;
    MENU_CTX_INTERRUPTS *pInterruptsMenusCtx = (MENU_CTX_INTERRUPTS *)pCbCtx;

    dwIntOptions = WDC_GET_INT_OPTIONS(*pInterruptsMenusCtx->phDev);
    fIsMsi = WDC_INT_IS_MSI(dwIntOptions);

    pInterruptsMenusCtx->fIntsEnabled = LSCDMA_IntIsEnabled(
        *pInterruptsMenusCtx->phDev);

    if (dwIntOptions & INTERRUPT_LEVEL_SENSITIVE)
    {
        /* TODO: You can remove this message after you have modified the
           implementation of LSCDMA_IntEnable() in lscdma_lib.c to correctly
           acknowledge level-sensitive interrupts (see guidelines in
           LSCDMA_IntEnable()). */
        printf("\n");
        printf("WARNING!!!\n");
        printf("----------\n");
        printf("Your hardware has level sensitive interrupts.\n");
        printf("Before enabling the interrupts, %s first modify the source "
            "code\n of LSCDMA_IntEnable(), in the file lscdma_lib.c, to correctly "
            "acknowledge\n%s interrupts when they occur, as dictated by "
            "the hardware's specification.\n\n",
            fIsMsi ? "it is recommended that" : "you must",
            fIsMsi ? "level sensitive" : "");
    }

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

/* Diagnostics interrupt handler routine */
static void DiagIntHandler(WDC_DEVICE_HANDLE hDev, LSCDMA_INT_RESULT *pIntResult)
{
    /* TODO: You can modify this function in order to implement your own
             diagnostics interrupt handler routine */

    printf("Got interrupt number %d\n", pIntResult->dwCounter);
    printf("Interrupt Type: %s\n",
        WDC_DIAG_IntTypeDescriptionGet(pIntResult->dwEnabledIntType));
    if (WDC_INT_IS_MSI(pIntResult->dwEnabledIntType))
        printf("Message Data: 0x%x\n", pIntResult->dwLastMessage);

    if (pIntResult->hDma)
    {
        printf("DMA transfer completed succesfully\n");

        if (!LSCDMA_DmaIstoDevice(pIntResult->hDma))
        {
            PVOID pBuf;
            DWORD dwBytes;

            pBuf = LSCDMA_DmaBufferGet(pIntResult->hDma, &dwBytes);
            DIAG_PrintHexBuffer(pBuf, dwBytes, FALSE);
        }
    }

    UNUSED_VAR(hDev);
}

/* -----------------------------------------------
    Direct Memory Access (DMA)
   ---------------------------------------------- */

typedef struct {
    WDC_DEVICE_HANDLE *phDev;
    LSCDMA_DMA_HANDLE hDma;
} MENU_CTX_DMA;

static BOOL MenuDmaIsDeviceNull(DIAG_MENU_OPTION *pMenu)
{
    return *((MENU_CTX_DMA *)pMenu->pCbCtx)->phDev == NULL;
}

static BOOL MenuDmaIsDmaHandleNotNull(DIAG_MENU_OPTION *pMenu)
{
    return ((MENU_CTX_DMA *)pMenu->pCbCtx)->hDma != NULL;
}

static BOOL MenuDmaIsDmaHandleNull(DIAG_MENU_OPTION *pMenu)
{
    return ((MENU_CTX_DMA *)pMenu->pCbCtx)->hDma == NULL;
}

/* DMA user input menu */
static BOOL MenuDmaOpenGetInput(BOOL *pfToDevice, DWORD *pdwPages,
    UINT32 *pu32Pattern)
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
    *pfToDevice = (1 == option) ? FALSE : TRUE;

    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(pdwPages,
        "\nEnter number of pages to transfer (4096 bytes per page)", FALSE, 1,
        *pfToDevice ? MAX_DMA_READ_DESCS : MAX_DMA_WRITE_DESCS))
    {
        return FALSE;
    }

    if (*pfToDevice)
    {
        /* Get DMA buffer pattern for host to device transfer */
        if (DIAG_INPUT_SUCCESS != DIAG_InputUINT32(pu32Pattern,
            "\nEnter DMA data pattern as 32 bit packet", TRUE, 0, 0))
        {
            return FALSE;
        }
    }

    printf("\n");

    return TRUE;
}

static LSCDMA_DMA_HANDLE LSCDMA_DIAG_DmaOpen(WDC_DEVICE_HANDLE hDev)
{
    BOOL fToDevice;
    DWORD i, dwBytes, dwPages, dwStatus;
    UINT32 u32Pattern;
    UINT32 *pBuf;
    LSCDMA_DMA_HANDLE hDma = NULL;

    /* Get user input */
    if (!MenuDmaOpenGetInput(&fToDevice, &dwPages, &u32Pattern))
        return NULL;

    hDma = LSCDMA_DmaOpen(hDev, dwPages * 4096, fToDevice);
    if (!hDma)
    {
        LSCDMA_ERR("Failed opening DMA handle\n");
        return NULL;
    }

    dwStatus = LSCDMA_IntEnable(hDev, DiagIntHandler);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        LSCDMA_ERR("Failed enabling interrupts\n");
        goto Error;
    }
    LSCDMA_DmaInterruptsEnable(hDma);

    pBuf = (UINT32 *)LSCDMA_DmaBufferGet(hDma, &dwBytes);
    for (i = 0; i < (dwBytes / sizeof(UINT32)); i++)
        pBuf[i] = u32Pattern;

    LSCDMA_DmaStart(hDma);

    return hDma;

Error:
    LSCDMA_DmaClose(hDma);
    return NULL;
}

static DWORD LSCDMA_DIAG_DmaClose(PVOID pCbCtx)
{
    DWORD dwStatus;
    MENU_CTX_DMA *pMenuDmaCtx = (MENU_CTX_DMA *)pCbCtx;
    WDC_DEVICE_HANDLE hDev = *(pMenuDmaCtx->phDev);

    if (pMenuDmaCtx->hDma)
    {

        LSCDMA_DMAStop(pMenuDmaCtx->hDma);
        if (WDC_IntIsEnabled(hDev))
        {
            LSCDMA_DmaInterruptsDisable(pMenuDmaCtx->hDma);
            dwStatus = LSCDMA_IntDisable(hDev);
            printf("DMA interrupts disable%s\n",
                (WD_STATUS_SUCCESS == dwStatus) ? "d" : " failed");
        }
        LSCDMA_DmaClose(pMenuDmaCtx->hDma);

        pMenuDmaCtx->hDma = NULL;
        printf("Closed DMA handle");
    }

    return WD_STATUS_SUCCESS;
}

static DWORD MenuDmaOpenOptionCb(PVOID pCbCtx)
{
    MENU_CTX_DMA *pMenuDmaCtx = (MENU_CTX_DMA *)pCbCtx;

    pMenuDmaCtx->hDma = LSCDMA_DIAG_DmaOpen(*(pMenuDmaCtx->phDev));
    if (!pMenuDmaCtx->hDma)
        LSCDMA_ERR("DMA open failed\n");

    return WD_STATUS_SUCCESS;
}

static void MenuDmaSubMenusInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_DMA *pMenuDmaCtx)
{
    static DIAG_MENU_OPTION openDmaMenu = { 0 };
    static DIAG_MENU_OPTION closeDmaMenu = { 0 };
    static DIAG_MENU_OPTION options[2] = { 0 };

    strcpy(openDmaMenu.cOptionName, "Open DMA");
    openDmaMenu.cbEntry = MenuDmaOpenOptionCb;
    openDmaMenu.cbIsHidden = MenuDmaIsDmaHandleNotNull;

    strcpy(closeDmaMenu.cOptionName, "Close DMA");
    closeDmaMenu.cbEntry = LSCDMA_DIAG_DmaClose;
    closeDmaMenu.cbIsHidden = MenuDmaIsDmaHandleNull;

    options[0] = openDmaMenu;
    options[1] = closeDmaMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        pMenuDmaCtx, pParentMenu);
}

static void MenuDmaInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static DIAG_MENU_OPTION menuDmaRoot = { 0 };
    static MENU_CTX_DMA menuDmaCtx = { 0 };

    strcpy(menuDmaRoot.cOptionName, "Direct Memory Access (DMA)");
    strcpy(menuDmaRoot.cTitleName, "Open/Close Direct Memory Access (DMA)");
    menuDmaRoot.cbExit = LSCDMA_DIAG_DmaClose;
    menuDmaRoot.cbIsHidden = MenuDmaIsDeviceNull;

    menuDmaCtx.phDev = phDev;

    MenuDmaSubMenusInit(&menuDmaRoot, &menuDmaCtx);
    DIAG_MenuSetCtxAndParentForMenus(&menuDmaRoot, 1, &menuDmaCtx,
        pParentMenu);
}

/* ----------------------------------------------------
    Plug-and-play and power management events handling
   ---------------------------------------------------- */
static DWORD MenuEventsRegisterOptionCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    DWORD dwStatus = LSCDMA_EventRegister(*(pEventsMenusCtx->phDev),
        (LSCDMA_EVENT_HANDLER)pEventsMenusCtx->DiagEventHandler);

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Events registered\n");
        pEventsMenusCtx->fRegistered = TRUE;
    }
    else
    {
        LSCDMA_ERR("Failed to register events. Last error [%s]\n",
            LSCDMA_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuEventsUnregisterOptionCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    DWORD dwStatus = LSCDMA_EventUnregister(*(pEventsMenusCtx->phDev));

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Events unregistered\n");
        pEventsMenusCtx->fRegistered = FALSE;
    }
    else
    {
        LSCDMA_ERR("Failed to unregister events. Last error [%s]\n",
            LSCDMA_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuEventsCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    pEventsMenusCtx->fRegistered = LSCDMA_EventIsRegistered(
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

/* Plug-and-play and power management events handler routine */
static void DiagEventHandler(WDC_DEVICE_HANDLE hDev, DWORD dwAction)
{
    /* TODO: You can modify this function in order to implement your own
             diagnostics events handler routine */

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


