/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/*****************************************************************************
*  File: qsys_diag.c
*
*  Sample user-mode diagnostics application for accessing Altera PCI Express
*  cards with Qsys design, using the WinDriver WDC API.
*  The sample was tested with Altera's Stratix IV GX development kit.
*  For more information on the Qsys design, refer to Altera's
*  "PCI Express in Qsys Example Designs" wiki page:
*  http://alterawiki.com/wiki/PCI_Express_in_Qsys_Example_Designs
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
******************************************************************************/

#include <stdio.h>
#include "wdc_defs.h"
#include "wdc_lib.h"
#include "utils.h"
#include "status_strings.h"
#include "diag_lib.h"
#include "wdc_diag_lib.h"
#include "pci_menus_common.h"
#include "pci_regs.h"
#include "qsys_lib.h"

#if defined(UNIX)
    #include <sys/time.h>
#endif

/*************************************************************
  General definitions
 *************************************************************/
/* Error messages display */
#define QSYS_ERR printf

/*************************************************************
  Global variables
 *************************************************************/
/* User's input command */
static CHAR gsInput[256];

/* --------------------------------------------------
    Qsys configuration registers information
   -------------------------------------------------- */
/* Configuration registers information array */
const WDC_REG gQSYS_CfgRegs[] = {
    { WDC_AD_CFG_SPACE, PCI_VID, WDC_SIZE_16, WDC_READ_WRITE, "VID",
        "Vendor ID" },
    { WDC_AD_CFG_SPACE, PCI_DID, WDC_SIZE_16, WDC_READ_WRITE, "DID",
        "Device ID" },
    { WDC_AD_CFG_SPACE, PCI_CR, WDC_SIZE_16, WDC_READ_WRITE, "CMD", "Command" },
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
#define QSYS_CFG_REGS_NUM sizeof(gQSYS_CfgRegs) / sizeof(WDC_REG)
/* TODO: For read-only or write-only registers, change the direction field of
         the relevant registers in gQSYS_CfgRegs to WDC_READ or WDC_WRITE. */
/* NOTE: You can define additional configuration registers in gQSYS_CfgRegs.
 */
const WDC_REG *gpQSYS_CfgRegs = gQSYS_CfgRegs;

/* -----------------------------------------------
    Qsys run-time registers information
   ----------------------------------------------- */
/* Run-time registers information array */
/* const WDC_REG gQSYS_Regs[]; */
const WDC_REG *gpQSYS_Regs = NULL;
/* TODO: You can remove the comment from the gQSYS_Regs array declaration and
   fill the array with run-time registers information for your device. If you
   select to do so, be sure to set gpQSYS_Regs to point to gQSYS_Regs:
const WDC_REG *gpQSYS_Regs = gQSYS_Regs;
*/
#define QSYS_REGS_NUM 0

/*************************************************************
  Static functions prototypes
 *************************************************************/
/* -----------------------------------------------
    Main diagnostics menu
   ----------------------------------------------- */
typedef struct {
    WDC_DEVICE_HANDLE *phDev;
    QSYS_PDMA_STRUCT pDma;
} MENU_CTX_QSYS;

/* Main diagnostics menu */
static DIAG_MENU_OPTION *MenuMainInit(WDC_DEVICE_HANDLE *phDev);

/* -----------------------------------------------
    Device open
   ----------------------------------------------- */
static void MenuDeviceOpenInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_QSYS *pQsysMenuCtx);

/* -----------------------------------------------
    Device close
   ----------------------------------------------- */
static void DeviceClose(WDC_DEVICE_HANDLE hDev, QSYS_PDMA_STRUCT pDma);

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
#ifdef HAS_INTS
/* -----------------------------------------------
    Interrupts
   ----------------------------------------------- */
static void MenuInterruptsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);
static void DiagIntHandler(WDC_DEVICE_HANDLE hDev,
    QSYS_INT_RESULT *pIntResult);
#endif /* ifdef HAS_INTS */

/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */
static void MenuEventsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);
static void DiagEventHandler(WDC_DEVICE_HANDLE hDev, DWORD dwAction);

/* -----------------------------------------------
    Direct Memory Access (DMA)
   ---------------------------------------------- */
static void MenuDmaPollingInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_QSYS *pMenuCtx);
static DWORD DmaOpen(PVOID pCbCtx);
static DWORD DmaClose(WDC_DEVICE_HANDLE hDev, QSYS_PDMA_STRUCT pDma);
static void DmaTransferVerify(QSYS_PDMA_STRUCT pDma, PVOID pBuf, PVOID pBufOrig,
    BOOL fIsToDevice);

/* -----------------------------------------------
    Performance Tests
   ----------------------------------------------- */
static void MenuPerformanceInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_QSYS *pQsysMenuCtx);
static DWORD PerformanceTestDma(PVOID pCbCtx);


/*************************************************************
  Functions implementation
 *************************************************************/
static DWORD QSYS_Init(WDC_DEVICE_HANDLE *phDev)
{
    /* Initialize the Qsys library */
    DWORD dwStatus = QSYS_LibInit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        QSYS_ERR("altera_diag: Failed to initialize the Qsys library: %s",
            QSYS_GetLastErr());
        return dwStatus;
    }

    /* Find and open a Qsys device (by default ID) */
    *phDev = QSYS_DeviceOpen(QSYS_DEFAULT_VENDOR_ID,
        QSYS_DEFAULT_DEVICE_ID);

    if (*phDev && !QSYS_IsQsysRevision(*phDev))
    {
        QSYS_ERR("altera_diag: This is not SOPC/Qsys design\n");
        return 0;
    }

    return WD_STATUS_SUCCESS;
}

int main(void)
{
    WDC_DEVICE_HANDLE hDev = NULL;
    DIAG_MENU_OPTION *pMenuRoot;
    DWORD dwStatus;

    printf("\n");
    printf("Qsys diagnostic utility.\n");
    printf("Application accesses hardware using " WD_PROD_NAME ".\n");

    /* Initialize the Qsys library */
    dwStatus = QSYS_Init(&hDev);
    if (dwStatus)
        return dwStatus;

    pMenuRoot = MenuMainInit(&hDev);

    /* Busy loop that runs the menu tree created above and communicating
        with the user */
    return DIAG_MenuRun(pMenuRoot);
}

/* -----------------------------------------------
    Main diagnostics menu
   ----------------------------------------------- */
static BOOL MenuIsDeviceNull(DIAG_MENU_OPTION *pMenu)
{
    return *((MENU_CTX_QSYS *)pMenu->pCbCtx)->phDev == NULL;
}

static DWORD MenuMainExitCb(PVOID pCbCtx)
{
    MENU_CTX_QSYS *pQsysMenuCtx = (MENU_CTX_QSYS *)pCbCtx;
    DWORD dwStatus;

    /* Perform necessary cleanup before exiting the program: */
    /* Close the device handle */
    if (*(pQsysMenuCtx->phDev))
        DeviceClose(*(pQsysMenuCtx->phDev), NULL);

    /* Uninitialize libraries */
    dwStatus = QSYS_LibUninit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        QSYS_ERR("altera_diag: Failed to uninitialize the Qsys library: %s",
            QSYS_GetLastErr());
    }

    return dwStatus;
}

static DIAG_MENU_OPTION *MenuMainInit(WDC_DEVICE_HANDLE *phDev)
{
    static QSYS_DMA_STRUCT dma = { 0 };

    static MENU_CTX_QSYS qsysMenuCtx = { 0 };
    static DIAG_MENU_OPTION menuRoot = { 0 };

    qsysMenuCtx.phDev = phDev;
    qsysMenuCtx.pDma = &dma;

    strcpy(menuRoot.cTitleName, "QSYS main menu");
    menuRoot.cbExit = MenuMainExitCb;
    menuRoot.pCbCtx = &qsysMenuCtx;

    MenuCommonScanBusInit(&menuRoot);
    MenuDeviceOpenInit(&menuRoot, &qsysMenuCtx);
    MenuReadWriteAddrInit(&menuRoot, phDev);
    MenuCfgInit(&menuRoot, phDev);
    MenuRwRegsInit(&menuRoot, phDev);
    MenuDmaPollingInit(&menuRoot, &qsysMenuCtx);
#ifdef HAS_INTS
    MenuInterruptsInit(&menuRoot, phDev);
#endif /* ifdef HAS_INTS */

    MenuEventsInit(&menuRoot, phDev);
    MenuPerformanceInit(&menuRoot, &qsysMenuCtx);

    return &menuRoot;
}

/* -----------------------------------------------
    Device find, open and close
   ----------------------------------------------- */
static DWORD MenuDeviceOpenCb(PVOID pCbCtx)
{
    MENU_CTX_QSYS *pQsysMenuCtx = (MENU_CTX_QSYS *)pCbCtx;

    if (*pQsysMenuCtx->phDev)
        DeviceClose(*pQsysMenuCtx->phDev, pQsysMenuCtx->pDma);

    *(pQsysMenuCtx->phDev) = QSYS_DeviceOpen(0, 0);

    return WD_STATUS_SUCCESS;
}

static void MenuDeviceOpenInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_QSYS *pQsysMenuCtx)
{
    static DIAG_MENU_OPTION menuDeviceOpen = { 0 };
    strcpy(menuDeviceOpen.cOptionName, "Find and open a QSYS device");
    menuDeviceOpen.cbEntry = MenuDeviceOpenCb;

    DIAG_MenuSetCtxAndParentForMenus(&menuDeviceOpen, 1, pQsysMenuCtx,
        pParentMenu);
}

/* Close a handle to a Qsys device */
static void DeviceClose(WDC_DEVICE_HANDLE hDev, QSYS_PDMA_STRUCT pDma)
{
    /* Validate the WDC device handle */
    if (!hDev)
        return;

    /* Close the DMA handle (if open) */
    if (pDma)
        DmaClose(hDev, pDma);

    /* Close the WDC device handle */
    if (!QSYS_DeviceClose(hDev))
    {
        QSYS_ERR("DeviceClose: Failed closing Qsys device: %s",
            QSYS_GetLastErr());
    }
}

/* -----------------------------------------------
    Read / write memory and I / O addresses
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
    cfgCtx.pCfgRegs = gQSYS_CfgRegs;
    cfgCtx.dwCfgRegsNum = QSYS_CFG_REGS_NUM;

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
    regsMenusCtx.pRegsArr = gpQSYS_Regs;
    regsMenusCtx.dwRegsNum = QSYS_REGS_NUM;
    strcpy(regsMenusCtx.sModuleName, "QSYS");

    MenuCommonRwRegsInit(pParentMenu, &regsMenusCtx);
}
#ifdef HAS_INTS
/* -----------------------------------------------
    Interrupt handling
   ----------------------------------------------- */
static DWORD MenuInterruptsEnableOptionCb(PVOID pCbCtx)
{
    MENU_CTX_INTERRUPTS *pInterruptsMenusCtx = (MENU_CTX_INTERRUPTS *)pCbCtx;
    DWORD dwStatus = QSYS_IntEnable(*(pInterruptsMenusCtx->phDev),
        (QSYS_INT_HANDLER)pInterruptsMenusCtx->funcIntHandler);

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Interrupts enabled\n");
        pInterruptsMenusCtx->fIntsEnabled = TRUE;
    }
    else
    {
        QSYS_ERR("Failed enabling interrupts. Error [0x%x - %s]\n",
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

static DWORD MenuInterruptsDisableOptionCb(PVOID pCbCtx)
{
    MENU_CTX_INTERRUPTS *pInterruptsMenusCtx = (MENU_CTX_INTERRUPTS *)pCbCtx;
    DWORD dwStatus = QSYS_IntDisable(*(pInterruptsMenusCtx->phDev));

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Interrupts disabled\n");
        pInterruptsMenusCtx->fIntsEnabled = FALSE;
    }
    else
    {
        QSYS_ERR("Failed disabling interrupts: %s\n", QSYS_GetLastErr());
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

    pInterruptsMenusCtx->fIntsEnabled = QSYS_IntIsEnabled(
        *pInterruptsMenusCtx->phDev);

    if (dwIntOptions & INTERRUPT_LEVEL_SENSITIVE)
    {
        /* TODO: You can remove this message after you have modified the
           implementation of QSYS_IntEnable() in qsys_lib.c to correctly
           acknowledge level-sensitive interrupts (see guidelines in
           QSYS_IntEnable()). */
        printf("\n");
        printf("WARNING!!!\n");
        printf("----------\n");
        printf("Your hardware has level sensitive interrupts.\n");
        printf("Before enabling the interrupts, %s first modify the source "
            "code\n of QSYS_IntEnable(), in the file qsys_lib.c, to correctly "
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
    MENU_INTERRUPTS_CALLBACKS interruptsMenuCbs = { 0 };
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
static void DiagIntHandler(WDC_DEVICE_HANDLE hDev, QSYS_INT_RESULT *pIntResult)
{
    UNUSED_VAR(hDev);

    /* TODO: You can modify this function in order to implement your own
       diagnostics interrupt handler routine.
       For example, if you select to modify the code to use interrupts to
       detect DMA transfer completion, you can call DmaTransferVerify() from
       the interrupt handler routine. */

    printf("Got interrupt number %d\n", pIntResult->dwCounter);
    printf("Interrupt Type: %s\n",
        WDC_DIAG_IntTypeDescriptionGet(pIntResult->dwEnabledIntType));
    if (WDC_INT_IS_MSI(pIntResult->dwEnabledIntType))
        printf("Message Data: 0x%x\n", pIntResult->dwLastMessage);
}
#endif /* ifdef HAS_INTS */

/* -----------------------------------------------
    Direct Memory Access (DMA)
   ----------------------------------------------- */
static BOOL MenuDmaIsDmaHandleNotNull(DIAG_MENU_OPTION *pMenu)
{
    return ((MENU_CTX_QSYS *)pMenu->pCbCtx)->pDma->pDma != NULL;
}

static BOOL MenuDmaIsDmaHandleNull(DIAG_MENU_OPTION *pMenu)
{
    return ((MENU_CTX_QSYS *)pMenu->pCbCtx)->pDma->pDma == NULL;
}

static DWORD MenuDmaCloseOptionCb(PVOID pCbCtx)
{
    MENU_CTX_QSYS *pMenuCtx = (MENU_CTX_QSYS *)pCbCtx;

    return DmaClose(*(pMenuCtx->phDev), pMenuCtx->pDma);
}

static void MenuDmaSubMenusInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_QSYS *pMenuCtx)
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
        pMenuCtx, pParentMenu);
}

static void MenuDmaPollingInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_QSYS *pMenuCtx)
{
    static DIAG_MENU_OPTION dmaPollingMenuRoot = { 0 };
    strcpy(dmaPollingMenuRoot.cOptionName, "Open/close Direct Memory Access "
        "(DMA)");
    strcpy(dmaPollingMenuRoot.cTitleName, "Open/close Direct Memory Access "
        "(DMA)\nusing polling completion method");
    dmaPollingMenuRoot.cbIsHidden = MenuIsDeviceNull;

    MenuDmaSubMenusInit(&dmaPollingMenuRoot, pMenuCtx);
    DIAG_MenuSetCtxAndParentForMenus(&dmaPollingMenuRoot, 1, pMenuCtx,
        pParentMenu);
}

/* Default number of DMA transfer data packets for a DMA performance test */
#define PERFORMANCE_DMA_NUM_PKTS_DEFAULT 8192 /* 8,192 packets = 65,536 bytes */

/* DMA user input menu */
static BOOL MenuDmaOpenGetInput(PDWORD pdwNumPkts, UINT32 *pu32Pattern,
    PDWORD pdwOptions, BOOL *pfUserPattern)
{
    DWORD option, dwNumPktsDefault = 0;

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

    /* Get the number of packets for the DMA transfer */
    sprintf(gsInput, "\nEnter number of %d-byte DMA packets",
        QSYS_DMA_PKT_BYTES);

    if (*pdwNumPkts) /* Default number of packets provided */
    {
        dwNumPktsDefault = *pdwNumPkts;
        sprintf(gsInput + strlen(gsInput), ", or 0 to use the default (%d packets)",
            dwNumPktsDefault);
    }

    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(pdwNumPkts, gsInput, FALSE,
        dwNumPktsDefault ? 0 : 1, 0xffffffff))
    {
        return FALSE;
    }

    if (!*pdwNumPkts)
        *pdwNumPkts = dwNumPktsDefault;

    /* Determine the data pattern type -- user-defined or automatic (serial) */
    *pfUserPattern = FALSE;
    printf("\nSelect DMA data pattern:\n");
    printf("1. Automatic serial data (0, 1, 2, ...)\n");
    printf("2. Your pattern\n");
    printf("%d. Cancel\n", DIAG_EXIT_MENU);

    if ((DIAG_INPUT_SUCCESS !=
        DIAG_GetMenuOption(&option, 2)) || (DIAG_EXIT_MENU == option))
    {
        return FALSE;
    }

    if (option == 2)
    {
        /* Get user data pattern */
        sprintf(gsInput, "\nEnter a %zu-byte DMA data pattern",
            QSYS_DMA_ITEM_BYTES);

        if (DIAG_INPUT_SUCCESS !=
            DIAG_InputUINT32(pu32Pattern, gsInput, TRUE, 0, 0))
        {
            return FALSE;
        }
        *pfUserPattern = TRUE;
    }

    printf("\n");

    return TRUE;
}

/* Verify a DMA transfer */
static void DmaTransferVerify(QSYS_PDMA_STRUCT pDma, PVOID pBuf, PVOID pBufOrig,
    BOOL fIsToDevice)
{
    DWORD i, dwNumItems;

    /* Calculate the number of UINT32 DMA transfer items */
    dwNumItems = (pDma->dwBufSize - QSYS_DMA_DONE_BYTES) / QSYS_DMA_ITEM_BYTES;

    /* Verify DMA transfer completion in the device */
    if (!QSYS_DmaTransferVerify(pDma->hDev))
    {
        QSYS_ERR("QSYS_DmaTransferVerify failed\n");
        goto Error;
    }

    if (fIsToDevice) /* Verify data for a host-to-device (read) DMA transfer */
    {
        UINT32 u32ReadData, u32ExpectedData;

        /* Compare the data in each 32-bit device DMA register to the data in
         * the parallel host DMA buffer item; (this is slow, but it works) */
        for (i = 0; i < dwNumItems; i++)
        {
            u32ReadData = QSYS_ReadReg32(pDma->hDev, AD_PCI_BAR0,
                pDma->dwTargetAddr + i * QSYS_DMA_ITEM_BYTES);
            u32ExpectedData = ((UINT32 *)(pBuf))[i];
            if (u32ExpectedData != u32ReadData)
            {
                QSYS_ERR("Host-to-device (read) DMA data mismatch: device "
                    "data = %08X, expected data (pBuf[%d]) = %08X\n",
                    u32ReadData, i, u32ExpectedData);
                goto Error;
            }
        }

    }
    else /* Verify data for a device-to-host (write) DMA transfer */
    {
        /* Compare the provided original data (pBufOrig) to the provided read
         * data (pBuf) */
        for (i = 0; i < dwNumItems; i++)
        {
            if (((UINT32 *)(pBuf))[i] != ((UINT32 *)(pBufOrig))[i])
            {
                QSYS_ERR("Device-to-host (write) DMA data mismatch: "
                    "host data (pBuf[%d]) = %08X, expected data "
                    "(pBufOrig[%d]) = %08X\n", i, ((UINT32 *)(pBuf))[i], i,
                    ((UINT32 *)(pBufOrig))[i]);
                goto Error;
            }
        }
    }

    printf("DMA transfer completed and verified.\n");
    return;

Error:
    printf("DMA transfer verification failed.\n");
}

/* Open DMA and prepare the device */
static DWORD DmaOpenAndPrepare(WDC_DEVICE_HANDLE hDev, QSYS_PDMA_STRUCT pDma,
    BOOL *pfIsToDevice, PDWORD pdwNumItems, UINT32 *pu32Pattern,
    UINT32 *pu32ExpectedData, UINT32 **ppBufOrig, BOOL fRunPerfTest)
{
    DWORD dwStatus, dwOptions, dwNumPkts, dwBufItems;
    BOOL fUserData;
    UINT32 i;

    /* For a performance test, set default transfer packets number */
    if (fRunPerfTest)
        dwNumPkts = PERFORMANCE_DMA_NUM_PKTS_DEFAULT;

    /* Get user input */
    if (!MenuDmaOpenGetInput(&dwNumPkts, pu32Pattern, &dwOptions, &fUserData))
        return WD_WINDRIVER_STATUS_ERROR;

    /* Calculate the number of UINT32 DMA transfer items */
    *pdwNumItems = dwNumPkts * QSYS_DMA_PKT_NUM_ITEMS;

    /* Determine the DMA direction: host-to-device=read; device-to-host=write */
    *pfIsToDevice = dwOptions & DMA_FROM_DEVICE ? FALSE : TRUE;

    /* Qsys sample design does not support physical addresses higher than 16M */
    dwOptions |= DMA_KBUF_BELOW_16M;

    /* Open a DMA handle */
    dwStatus = QSYS_DmaOpen(hDev, pDma, dwOptions, *pdwNumItems);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        printf("\nFailed to open DMA handle. Error 0x%x - %s\n", dwStatus,
            Stat2Str(dwStatus));
        return dwStatus;
    }

    printf("\nDMA handle was opened successfully\n");

    /* For device-to-host (write) DMA, allocate a data-verification buffer */
    if (!*pfIsToDevice)
        *ppBufOrig = (UINT32 *)malloc(*pdwNumItems * QSYS_DMA_ITEM_BYTES);

    /* Initialize DMA data */
    for (i = 0; i < *pdwNumItems; i++)
    {
        /* Use user data, if provided; otherwise, use serial data (0, 1, ...) */
        UINT32 data = fUserData ? *pu32Pattern : i;

        if (*pfIsToDevice) /* Host-to-device (read) DMA buffer */
        {
            /* Initialize the host's DMA buffer with the data */
            ((UINT32 *)(pDma->pBuf))[i] = data;
        }
        else /* Device-to-host (write) DMA buffer */
        {
            /* Initialize the device's DMA buffer with the data */
            WDC_WriteAddr32(hDev, AD_PCI_BAR0,
                pDma->dwTargetAddr + i * QSYS_DMA_ITEM_BYTES, data);
            /* Store the data in the data-verification buffer */
            (*ppBufOrig)[i] = data;
            /* Initialize the host's DMA buffer with dummy data, to be
             * overwritten later by a device-to-host (write) DMA transfer */
            ((UINT32 *)(pDma->pBuf))[i] = 0xdeadbeaf;
        }
    }

    /* Clear the host's DMA transfer completion detection data */
    dwBufItems = pDma->dwBufSize / QSYS_DMA_ITEM_BYTES;
    for (i = *pdwNumItems; i < dwBufItems; i++)
        ((UINT32 *)(pDma->pBuf))[i] = 0;

    /* Store the expected data that will be used to verify the DMA transfer
     * completion -- the data in the last 4 bytes (QSYS_DMA_DONE_DETECT_BYTES)
     * of the first 32 bytes (QSYS_DMA_DONE_BYTES) of the transfer data */
    if (*pfIsToDevice)
    {
        *pu32ExpectedData = ((UINT32 *)(pDma->pBuf))[QSYS_DMA_DONE_NUM_ITEMS -
            QSYS_DMA_DONE_DETECT_NUM_ITEMS];
    }
    else
    {
        *pu32ExpectedData = (*ppBufOrig)[QSYS_DMA_DONE_NUM_ITEMS -
            QSYS_DMA_DONE_DETECT_NUM_ITEMS];
    }

    /* Prepare the device for DMA transfer */
    QSYS_DmaDevicePrepare(pDma, *pfIsToDevice);

    /* If you select to modify the code to use interrupts to detect DMA
     * transfer completion, call QSYS_IntEnable() here to enable interrupts. */

    return dwStatus;
}

/* Open DMA */
static DWORD DmaOpen(PVOID pCbCtx)
{
    MENU_CTX_QSYS *pQsysMenuCtx = (MENU_CTX_QSYS *)pCbCtx;

    WDC_DEVICE_HANDLE hDev = *(pQsysMenuCtx->phDev);
    QSYS_PDMA_STRUCT pDma = pQsysMenuCtx->pDma;
    DWORD dwStatus, dwNumItems = 0;
    UINT32 u32Pattern, u32ExpectedData, *pBufOrig = NULL;
    BOOL fIsToDevice;

    /* Open DMA and prepare the device */
    dwStatus = DmaOpenAndPrepare(hDev, pDma, &fIsToDevice, &dwNumItems,
        &u32Pattern, &u32ExpectedData, &pBufOrig, FALSE);

    if (WD_STATUS_SUCCESS != dwStatus)
        goto Exit;

    /* Start DMA */
    printf("Start DMA transfer\n");
    QSYS_DmaStart(pDma, fIsToDevice);

    /* Poll for DMA transfer completion */
    if (QSYS_DmaPollCompletion(pDma, u32ExpectedData))
    {
        /* Verify the DMA transfer */
        DmaTransferVerify(pDma, pDma->pBuf, pBufOrig, fIsToDevice);
    }
    else
    {
        printf("DMA transfer completion polling timeout\n");
    }

Exit:
    /* Free the device-to-host data-verification buffer (if allocated) */
    if (pBufOrig)
        free(pBufOrig);

    return dwStatus;
}

/* Close DMA */
static DWORD DmaClose(WDC_DEVICE_HANDLE hDev, QSYS_PDMA_STRUCT pDma)
{
    UNUSED_VAR(hDev);

    if (!pDma)
        return WD_STATUS_SUCCESS;

    /* If you select to modify the code to use interrupts to detect DMA
     * transfer completion, call QSYS_IntIsEnabled() here to check if DMA
     * interrupts are currently enabled, and if so, call QSYS_IntDisable() to
     * disable the interrupts. */

    if (pDma->pDma)
    {
        /* Close the device's DMA handle */
        QSYS_DmaClose(pDma);
        printf("DMA closed\n");
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
    DWORD dwStatus = QSYS_EventRegister(*(pEventsMenusCtx->phDev),
        (QSYS_EVENT_HANDLER)pEventsMenusCtx->DiagEventHandler);

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Events registered\n");
        pEventsMenusCtx->fRegistered = TRUE;
    }
    else
    {
        QSYS_ERR("Failed to register events. Last error [%s]\n",
            QSYS_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuEventsUnregisterOptionCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    DWORD dwStatus = QSYS_EventUnregister(*(pEventsMenusCtx->phDev));

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Events unregistered\n");
        pEventsMenusCtx->fRegistered = FALSE;
    }
    else
    {
        QSYS_ERR("Failed to unregister events. Last error [%s]\n",
            QSYS_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuEventsCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    pEventsMenusCtx->fRegistered = QSYS_EventIsRegistered(
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

/* -----------------------------------------------
    Performance Tests
   ----------------------------------------------- */
/* Performance tests menu options */
enum {
    MENU_PERFORMANCE_DMA = 1,
    MENU_PERFORMANCE_EXIT = DIAG_EXIT_MENU,
};

/* Performance tests menu */
static void MenuPerformanceInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_QSYS *pQsysMenuCtx)
{
    static DIAG_MENU_OPTION dmaPerformanceRoot = { 0 };

    strcpy(dmaPerformanceRoot.cOptionName, "Run performance tests");
    dmaPerformanceRoot.cbEntry = PerformanceTestDma;
    dmaPerformanceRoot.cbIsHidden = MenuIsDeviceNull;

    DIAG_MenuSetCtxAndParentForMenus(&dmaPerformanceRoot, 1, pQsysMenuCtx,
        pParentMenu);
}

/* Time value type */
#if defined(UNIX)   /* Unix */
    typedef struct timeval TIME_TYPE;
#else               /* Windows */
    typedef LARGE_INTEGER TIME_TYPE;
#endif

/* Get the current time */
static void GetCurTime(TIME_TYPE *time)
{
#if defined(UNIX)   /* Unix */
    gettimeofday(time, NULL);
#else               /* Windows */
    QueryPerformanceCounter(time);
#endif
}

/* Calculate time difference (including conversion for MByte/sec values) */
static double TimeDiff(TIME_TYPE *end, TIME_TYPE *start)
{
#if defined(UNIX)   /* Unix */
    return (end->tv_usec - start->tv_usec) +
        (end->tv_sec - start->tv_sec) * 1000000;
#else               /* Windows */
    TIME_TYPE ctr_freq;

    if (!QueryPerformanceFrequency(&ctr_freq))
        return (double)-1;

    return (double)((end->QuadPart - start->QuadPart) * 1000000 /
        ctr_freq.QuadPart);
#endif
}

/* ===============================================
    DMA Performance Test
   =============================================== */

/* DMA performance test */
static DWORD PerformanceTestDma(PVOID pCbCtx)
{
    MENU_CTX_QSYS *pQsysMenuCtx = (MENU_CTX_QSYS *)pCbCtx;

    WDC_DEVICE_HANDLE hDev = *(pQsysMenuCtx->phDev);
    QSYS_PDMA_STRUCT pDma = pQsysMenuCtx->pDma;
    DWORD dwStatus, dwNumItems = 0;
    UINT32 u32Pattern, u32ExpectedData, *pBufOrig = NULL;
    BOOL fIsToDevice;
    double dblAcummTime;
    TIME_TYPE IStart, IEnd;

    /* Open DMA and prepare the device */
    dwStatus = DmaOpenAndPrepare(hDev, pDma, &fIsToDevice, &dwNumItems,
        &u32Pattern, &u32ExpectedData, &pBufOrig, TRUE);

    if (WD_STATUS_SUCCESS != dwStatus)
        goto Exit;

    /* Start DMA */
    printf("Start DMA transfer\n");
    /* Log the start-of-transfer time */
    GetCurTime(&IStart);
    /* Start the DMA transfer */
    QSYS_DmaStart(pDma, fIsToDevice);

    /* Poll for DMA transfer completion */
    if (QSYS_DmaPollCompletion(pDma, u32ExpectedData))
    {
        double dblMBps = 0;

        /* Log the end-of-transfer time */
        GetCurTime(&IEnd);
        /* Get the DMA transfer time */
        dblAcummTime = TimeDiff(&IEnd, &IStart);
        /* Calculate the DMA performance */
        dblMBps = (dwNumItems * QSYS_DMA_ITEM_BYTES) / dblAcummTime;
        printf("DMA %s transfer time: %f MBytes per second\n",
            fIsToDevice ? "to device" : "from device", dblMBps);

        /* Verify the DMA transfer */
        DmaTransferVerify(pDma, pDma->pBuf, pBufOrig, fIsToDevice);
    }
    else
    {
        printf("DMA transfer completion polling timeout\n");
    }

Exit:
    if (pDma->pDma && WD_OPERATION_ALREADY_DONE != dwStatus)
    {
        /* Close the device's DMA handle */
        QSYS_DmaClose(pDma);
        printf("DMA closed\n");
    }

    /* Free the device-to-host data-verification buffer (if allocated) */
    if (pBufOrig)
        free(pBufOrig);

    return dwStatus;
}



