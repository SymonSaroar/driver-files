/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/************************************************************************
*  File: pci_diag.c
*
*  Sample user-mode diagnostics application for accessing PCI
*  devices, possibly via a Kernel PlugIn driver using WinDriver's API.

*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*************************************************************************/

#include "wds_lib.h"

#include "utils.h"
#include "status_strings.h"
#include "diag_lib.h"
#include "wdc_diag_lib.h"
#include "pci_menus_common.h"

#include "wds_diag_lib.h"

#include "pci_lib.h"

/*************************************************************
  General definitions
 *************************************************************/
/* Error messages display */
#define PCI_ERR printf

/* Trace messages display */
#define PCI_TRACE printf

/*************************************************************
  Global variables
 *************************************************************/
/* User's input command */
static CHAR gsInput[256];

/* --------------------------------------------------
    PCI configuration registers information
   -------------------------------------------------- */
/* Configuration registers information array */
static const WDC_REG gPCI_CfgRegs[] = {
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

static const WDC_REG gPCI_ext_CfgRegs[] = {
    { WDC_AD_CFG_SPACE, PCIE_CAP_ID, WDC_SIZE_8, WDC_READ_WRITE, "PCIE_CAP_ID",
        "PCI Express\nCapability ID" },
    { WDC_AD_CFG_SPACE, NEXT_CAP_PTR, WDC_SIZE_8, WDC_READ_WRITE, "NEXT_CAP_PTR",
        "Next Capabiliy Pointer" },
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
};

#define PCI_CFG_EXT_REGS_NUM sizeof(gPCI_ext_CfgRegs) / sizeof(WDC_REG)
#define PCI_CFG_REGS_NUM sizeof(gPCI_CfgRegs) / sizeof(WDC_REG)

/* TODO: For read-only or write-only registers, change the direction field of
         the relevant registers in gPCI_CfgRegs to WDC_READ or WDC_WRITE. */
/* NOTE: You can define additional configuration registers in gPCI_CfgRegs. */
const WDC_REG *gpPCI_CfgRegs = gPCI_CfgRegs;


#ifdef LINUX
    static BOOL fIsEnabledSRIOV = FALSE;
#endif


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

/* -----------------------------------------------
    Interrupt handling
   ----------------------------------------------- */
static void MenuInterruptsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

/* -----------------------------------------------
    DMA memory handling
   ----------------------------------------------- */
static void MenuDmaInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

/* -----------------------------------------------
   SRIOV handling
   ----------------------------------------------- */
#if defined(LINUX) && !defined(ISA)
static void MenuSriovInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);
#endif


/* ----------------------------------------------------
    Kernel Plugin Version Check
   ---------------------------------------------------- */
static DWORD CheckKPDriverVer(WDC_DEVICE_HANDLE hDev);

/*************************************************************
  Functions implementation
 *************************************************************/

static DWORD PCI_Init(WDC_DEVICE_HANDLE *phDev)
{
    /* Initialize the PCI library */
    DWORD dwStatus = PCI_LibInit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        PCI_ERR("pci_diag: Failed to initialize the PCI library: %s",
            PCI_GetLastErr());
        return dwStatus;
    }

    PrintDbgMessage(D_ERROR, S_PCI, "WinDriver user mode version %s\n",
        WD_VERSION_STR);

    /* Find and open a PCI device (by default ID) */
    if (PCI_DEFAULT_VENDOR_ID)
        *phDev = PCI_DeviceOpen(PCI_DEFAULT_VENDOR_ID, PCI_DEFAULT_DEVICE_ID);

    /* Get Kernel PlugIn driver version */
    if (*phDev && WDC_IS_KP(*phDev))
        CheckKPDriverVer(*phDev);

    return WD_STATUS_SUCCESS;
}

int main(void)
{
    WDC_DEVICE_HANDLE hDev = NULL;
    DIAG_MENU_OPTION *pMenuRoot;
    DWORD dwStatus = PCI_Init(&hDev);

    if (dwStatus)
        return dwStatus;

    PCI_TRACE("\n");
    PCI_TRACE("PCI diagnostic utility.\n");
    PCI_TRACE("Application accesses hardware using " WD_PROD_NAME "\n");
    PCI_TRACE("and a Kernel PlugIn driver (%s).\n", KP_PCI_DRIVER_NAME);

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
#ifdef LINUX
        if (fIsEnabledSRIOV)
        {
            dwStatus = PCI_SriovDisable(hDev);
            if (dwStatus)
            {
                PCI_ERR("pci_diag: Failed Disabling SR-IOV: %s\n",
                    Stat2Str(dwStatus));
            }
        }
#endif
        if (!PCI_DeviceClose(hDev))
            PCI_ERR("pci_diag: Failed closing PCI device: %s",
                PCI_GetLastErr());
    }

    if (WDS_IsIpcRegistered())
        WDS_IpcUnRegister();

    /* Uninitialize libraries */
    dwStatus = PCI_LibUninit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        PCI_ERR("pci_diag: Failed to uninitialize the PCI library: %s",
            PCI_GetLastErr());
    }

    return dwStatus;

}

static DIAG_MENU_OPTION *MenuMainInit(WDC_DEVICE_HANDLE *phDev)
{
    static DIAG_MENU_OPTION menuRoot = { 0 };

    menuRoot.cbExit = MenuMainExitCb;
    strcpy(menuRoot.cTitleName, "PCI main menu");

    menuRoot.pCbCtx = phDev;

    MenuCommonScanBusInit(&menuRoot);
    MenuDeviceOpenInit(&menuRoot, phDev);
    MenuSharedBufferInit(&menuRoot);
    MenuIpcInit(&menuRoot);
    MenuCfgInit(&menuRoot, phDev);
    MenuEventsInit(&menuRoot, phDev);

    MenuReadWriteAddrInit(&menuRoot, phDev);

    MenuInterruptsInit(&menuRoot, phDev);

    MenuDmaInit(&menuRoot, phDev);

#if defined(LINUX) && !defined(ISA)
    MenuSriovInit(&menuRoot, phDev);
#endif


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
#ifdef LINUX
        if (fIsEnabledSRIOV)
        {
            DWORD dwStatus = PCI_SriovDisable(*phDev);
            if (dwStatus)
            {
                PCI_ERR("pci_diag: Failed Disabling SR-IOV: %s\n",
                    Stat2Str(dwStatus));
            }
        }
#endif
        if (!PCI_DeviceClose(*phDev))
            PCI_ERR("pci_diag: Failed closing PCI device: %s",
                PCI_GetLastErr());
    }
    *phDev = PCI_DeviceOpen(0, 0);

    /* Get Kernel PlugIn driver version */
    if (*phDev && WDC_IS_KP(*phDev))
        CheckKPDriverVer(*phDev);

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
    static MENU_CTX_CFG cfgCtx = { 0 };

    cfgCtx.phDev = phDev;
    cfgCtx.pCfgRegs = gPCI_CfgRegs;
    cfgCtx.dwCfgRegsNum = PCI_CFG_REGS_NUM;
    cfgCtx.pCfgExpRegs = gPCI_ext_CfgRegs;
    cfgCtx.dwCfgExpRegsNum = PCI_CFG_EXT_REGS_NUM;

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

    PCI_TRACE("\nReceived event notification (device handle 0x%p): ", hDev);

    switch (dwAction)
    {
    case WD_INSERT:
        PCI_TRACE("WD_INSERT\n");
        break;
    case WD_REMOVE:
        PCI_TRACE("WD_REMOVE\n");
        break;
    case WD_POWER_CHANGED_D0:
        PCI_TRACE("WD_POWER_CHANGED_D0\n");
        break;
    case WD_POWER_CHANGED_D1:
        PCI_TRACE("WD_POWER_CHANGED_D1\n");
        break;
    case WD_POWER_CHANGED_D2:
        PCI_TRACE("WD_POWER_CHANGED_D2\n");
        break;
    case WD_POWER_CHANGED_D3:
        PCI_TRACE("WD_POWER_CHANGED_D3\n");
        break;
    case WD_POWER_SYSTEM_WORKING:
        PCI_TRACE("WD_POWER_SYSTEM_WORKING\n");
        break;
    case WD_POWER_SYSTEM_SLEEPING1:
        PCI_TRACE("WD_POWER_SYSTEM_SLEEPING1\n");
        break;
    case WD_POWER_SYSTEM_SLEEPING2:
        PCI_TRACE("WD_POWER_SYSTEM_SLEEPING2\n");
        break;
    case WD_POWER_SYSTEM_SLEEPING3:
        PCI_TRACE("WD_POWER_SYSTEM_SLEEPING3\n");
        break;
    case WD_POWER_SYSTEM_HIBERNATE:
        PCI_TRACE("WD_POWER_SYSTEM_HIBERNATE\n");
        break;
    case WD_POWER_SYSTEM_SHUTDOWN:
        PCI_TRACE("WD_POWER_SYSTEM_SHUTDOWN\n");
        break;
    default:
        PCI_TRACE("0x%x\n", dwAction);
        break;
    }
}

static DWORD MenuEventsRegisterOptionCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    DWORD dwStatus = PCI_EventRegister(*(pEventsMenusCtx->phDev),
        (PCI_EVENT_HANDLER)pEventsMenusCtx->DiagEventHandler);

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        PCI_TRACE("Events registered\n");
        pEventsMenusCtx->fRegistered = TRUE;
    }
    else
    {
        PCI_ERR("Failed to register events. Last error [%s]\n",
            PCI_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuEventsUnregisterOptionCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    DWORD dwStatus = PCI_EventUnregister(*(pEventsMenusCtx->phDev));

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        PCI_TRACE("Events unregistered\n");
        pEventsMenusCtx->fRegistered = FALSE;
    }
    else
    {
        PCI_ERR("Failed to unregister events. Last error [%s]\n",
            PCI_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuEventsCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    pEventsMenusCtx->fRegistered = PCI_EventIsRegistered(
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

/* -----------------------------------------------
    Interrupt handling
   ----------------------------------------------- */

/* Diagnostics interrupt handler routine */
static void DiagIntHandler(WDC_DEVICE_HANDLE hDev,
    PCI_INT_RESULT *pIntResult)
{
    /* TODO: You can modify this function in order to implement your own
             diagnostics interrupt handler routine */

    UNUSED_VAR(hDev);

    PCI_TRACE("Got interrupt number %d\n", pIntResult->dwCounter);
    PCI_TRACE("Interrupt Type: %s\n",
        WDC_DIAG_IntTypeDescriptionGet(pIntResult->dwEnabledIntType));

    if (WDC_INT_IS_MSI(pIntResult->dwEnabledIntType))
        PCI_TRACE("Message Data: 0x%x\n", pIntResult->dwLastMessage);
}

static DWORD MenuInterruptsEnableOptionCb(PVOID pCbCtx)
{
    MENU_CTX_INTERRUPTS *pInterruptsMenusCtx = (MENU_CTX_INTERRUPTS *)pCbCtx;
    DWORD dwStatus = PCI_IntEnable(*(pInterruptsMenusCtx->phDev),
        (PCI_INT_HANDLER)pInterruptsMenusCtx->funcIntHandler);

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        PCI_TRACE("Interrupts enabled\n");
        pInterruptsMenusCtx->fIntsEnabled = TRUE;
    }
    else
    {
        PCI_ERR("Failed enabling interrupts. Error [0x%x - %s]\n",
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

static DWORD MenuInterruptsDisableOptionCb(PVOID pCbCtx)
{
    MENU_CTX_INTERRUPTS *pInterruptsMenusCtx = (MENU_CTX_INTERRUPTS *)pCbCtx;
    DWORD dwStatus = PCI_IntDisable(*(pInterruptsMenusCtx->phDev));

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        PCI_TRACE("Interrupts disabled\n");
        pInterruptsMenusCtx->fIntsEnabled = FALSE;
    }
    else
    {
        PCI_ERR("Failed disabling interrupts: %s\n", PCI_GetLastErr());
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

    pInterruptsMenusCtx->fIntsEnabled = PCI_IntIsEnabled(
        *pInterruptsMenusCtx->phDev);

    if ((dwIntOptions & INTERRUPT_LEVEL_SENSITIVE) &&
        !pInterruptsMenusCtx->fIntsEnabled)
    {
        /* TODO: You can remove this message after you have modified the
           implementation of PCI_IntEnable() in pci_lib.c to correctly
           acknowledge level-sensitive interrupts (see guidelines in
           PCI_IntEnable()). */
        PCI_TRACE("\n");
        PCI_TRACE("WARNING!!!\n");
        PCI_TRACE("----------\n");
        PCI_TRACE("Your hardware has level sensitive interrupts.\n");
        PCI_TRACE("Before enabling the interrupts, %s first modify the source "
            "code\n of PCI_IntEnable(), in the file pci_lib.c, to correctly "
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

/* -----------------------------------------------
    DMA memory handling
   ----------------------------------------------- */
enum {
    MENU_DMA_ALLOCATE_CONTIG,
    MENU_DMA_ALLOCATE_SG,
    MENU_DMA_RESERVED_MEM
};

static BOOL MenuDmaIsContigBufferNotExists(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_DMA *pCtx = (MENU_CTX_DMA *)pMenu->pCbCtx;

    return !(pCtx->pDma && (pCtx->pDma->dwOptions & DMA_KERNEL_BUFFER_ALLOC));
}

static BOOL MenuDmaIsDeviceNull(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_DMA *pCtx = (MENU_CTX_DMA *)pMenu->pCbCtx;

    return *pCtx->phDev == NULL;
}

static void FreeDmaMem(PVOID *ppBuf, WD_DMA **ppDma)
{
    DWORD dwStatus;
    BOOL fIsSG; /* Is Scatter Gather DMA */

    if (!(*ppDma))
        return;

    fIsSG = !((*ppDma)->dwOptions & DMA_KERNEL_BUFFER_ALLOC);

    dwStatus = PCI_DmaBufUnlock(*ppDma);
    if (WD_STATUS_SUCCESS == dwStatus)
    {
        PCI_TRACE("DMA memory freed\n");
    }
    else
    {
        PCI_ERR("Failed trying to free DMA memory. Error [0x%x - %s]\n",
            dwStatus, Stat2Str(dwStatus));
    }

    if (fIsSG)
        free(*ppBuf);

    *ppBuf = NULL;
    *ppDma = NULL;
}

static BOOL DmaGetAllocInput(MENU_CTX_DMA *pDmaMenusCtx, DWORD dwOption)
{
    if (dwOption == MENU_DMA_RESERVED_MEM)
    {
#ifdef WIN32
        PCI_TRACE("Warning: The address for the reserved memory should be "
            "calculated according to the values listed in registry key\n"
            "HKLM/HARDWARE/RESOURCEMAP/System Resources/Physical Memory.\n"
            "Any other address may result in a BSOD. For more details "
            "please refer to Tech Doc #129\n\n");
#endif
        sprintf(gsInput, "Enter reserved memory address "
            "(64 bit hex uint) ");
        pDmaMenusCtx->qwAddr = 0;
        if (DIAG_INPUT_SUCCESS != DIAG_InputUINT64(&pDmaMenusCtx->qwAddr,
            gsInput, TRUE, 1, 0xFFFFFFFFFFFFFFFF))
        {
            return FALSE;
        }
    }

    if (dwOption == MENU_DMA_ALLOCATE_CONTIG ||
        dwOption == MENU_DMA_ALLOCATE_SG ||
        dwOption == MENU_DMA_RESERVED_MEM)
    {
        sprintf(gsInput, "Enter memory allocation size in bytes "
            "(32 bit uint) ");
        pDmaMenusCtx->size = 0;
        if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&pDmaMenusCtx->size,
            gsInput, FALSE, 1, 0xFFFFFFFF))
        {
            return FALSE;
        }
        if (dwOption == MENU_DMA_ALLOCATE_CONTIG)
        {
            sprintf(gsInput, "Enter DMA address width of an address "
                "that your device supports, use 0 for default value "
                "(32 bit uint)");
            if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(
                (PVOID)&pDmaMenusCtx->dwDmaAddressWidth, gsInput, FALSE,
                0, 64))
            {
                return FALSE;
            }

            pDmaMenusCtx->dwOptions = DMA_KBUF_ALLOC_SPECIFY_ADDRESS_WIDTH |
                (pDmaMenusCtx->dwDmaAddressWidth <<
                    DMA_OPTIONS_ADDRESS_WIDTH_SHIFT);
        }

        /* Free DMA memory before trying the new allocation */
        FreeDmaMem(&(pDmaMenusCtx->pBuf), &(pDmaMenusCtx->pDma));
    }

    return TRUE;
}

static DWORD MenuDmaAllocContigOptionCb(PVOID pCbCtx)
{
    MENU_CTX_DMA *pDmaMenusCtx = (MENU_CTX_DMA *)pCbCtx;
    DWORD dwStatus;

    if (!DmaGetAllocInput(pDmaMenusCtx, MENU_DMA_ALLOCATE_CONTIG))
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    dwStatus = PCI_DmaAllocContig(pDmaMenusCtx);
    if (WD_STATUS_SUCCESS == dwStatus)
    {
        PCI_TRACE("Contiguous memory allocated. user addr [%p], "
            "physical addr [0x%" PRI64 "x], size [%u(0x%x)]\n",
            pDmaMenusCtx->pBuf,
            pDmaMenusCtx->pDma->Page[0].pPhysicalAddr,
            pDmaMenusCtx->pDma->Page[0].dwBytes,
            pDmaMenusCtx->pDma->Page[0].dwBytes);
    }
    else
    {
        PCI_ERR("Failed allocating contiguous memory. size [%u], "
            "Error [0x%x - %s]\n", pDmaMenusCtx->size, dwStatus,
            Stat2Str(dwStatus));
    }

Exit:
    return dwStatus;
}

static DWORD MenuDmaAllocSgOptionCb(PVOID pCbCtx)
{
    MENU_CTX_DMA *pDmaMenusCtx = (MENU_CTX_DMA *)pCbCtx;
    DWORD dwStatus;

    if (!DmaGetAllocInput(pDmaMenusCtx, MENU_DMA_ALLOCATE_SG))
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    pDmaMenusCtx->pBuf = malloc(pDmaMenusCtx->size);
    if (!pDmaMenusCtx->pBuf)
    {
        PCI_ERR("Failed allocating user memory for SG. size [%d]\n",
            pDmaMenusCtx->size);
        dwStatus = WD_OPERATION_FAILED;
        goto Exit;
    }

    dwStatus = PCI_DmaAllocSg(pDmaMenusCtx);
    if (WD_STATUS_SUCCESS == dwStatus)
    {
        DWORD i;

        PCI_TRACE("SG memory allocated. user addr [%p], size [%d]\n",
            pDmaMenusCtx->pBuf, pDmaMenusCtx->size);

        PCI_TRACE("Pages physical addresses:\n");
        for (i = 0; i < pDmaMenusCtx->pDma->dwPages; i++)
        {
            PCI_TRACE("%u) physical addr [0x%" PRI64 "x], "
                "size [%d(0x%x)]\n", i + 1,
                pDmaMenusCtx->pDma->Page[i].pPhysicalAddr,
                pDmaMenusCtx->pDma->Page[i].dwBytes,
                pDmaMenusCtx->pDma->Page[i].dwBytes);
        }
    }
    else
    {
        PCI_ERR("Failed allocating SG memory. size [%d], "
            "Error [0x%x - %s]\n", pDmaMenusCtx->size, dwStatus,
            Stat2Str(dwStatus));
        free(pDmaMenusCtx->pBuf);
    }

Exit:
    return dwStatus;
}

static DWORD MenuDmaReservedOptionCb(PVOID pCbCtx)
{
    MENU_CTX_DMA *pDmaMenusCtx = (MENU_CTX_DMA *)pCbCtx;
    DWORD dwStatus;

    if (!DmaGetAllocInput(pDmaMenusCtx, MENU_DMA_RESERVED_MEM))
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    dwStatus = PCI_DmaAllocReserved(pDmaMenusCtx);
    if (WD_STATUS_SUCCESS == dwStatus)
    {
        PCI_TRACE("Reserved memory claimed. user addr [%p], "
            "bus addr [0x%" PRI64 "x], size [%d(0x%x)]\n", pDmaMenusCtx->pBuf,
            pDmaMenusCtx->pDma->Page[0].pPhysicalAddr,
            pDmaMenusCtx->pDma->Page[0].dwBytes,
            pDmaMenusCtx->pDma->Page[0].dwBytes);
    }
    else
    {
        PCI_ERR("Failed claiming reserved memory. size [%d], "
            "Error [0x%x - %s]\n", pDmaMenusCtx->size,
            dwStatus, Stat2Str(dwStatus));
    }

Exit:
    return dwStatus;
}

static DWORD MenuDmaSendSharedBufOptionCb(PVOID pCbCtx)
{
    MENU_CTX_DMA *pDmaMenusCtx = (MENU_CTX_DMA *)pCbCtx;

    WDS_DIAG_IpcSendDmaContigToGroup(pDmaMenusCtx->pDma);

    return WD_STATUS_SUCCESS;
}

static DWORD MenuDmaFreeBufOptionCb(PVOID pCbCtx)
{
    MENU_CTX_DMA *pDmaMenusCtx = (MENU_CTX_DMA *)pCbCtx;

    FreeDmaMem(&pDmaMenusCtx->pBuf, &pDmaMenusCtx->pDma);

    return WD_STATUS_SUCCESS;
}

static DWORD MenuDmaExitCb(PVOID pCbCtx)
{
    MENU_CTX_DMA *pDmaMenusCtx = (MENU_CTX_DMA *)pCbCtx;

    FreeDmaMem(&pDmaMenusCtx->pBuf, &pDmaMenusCtx->pDma);
    pDmaMenusCtx->dwOptions = 0;
    pDmaMenusCtx->dwDmaAddressWidth = 0;
    pDmaMenusCtx->qwAddr = 0;

    return WD_STATUS_SUCCESS;
}

    #define MENU_DMA_PCI_INCREASE 1

static void MenuDmaSubMenusInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_DMA *pDmaSubMenusCtx)
{
    static DIAG_MENU_OPTION allocateContigMenu = { 0 };
    static DIAG_MENU_OPTION allocateSgMenu = { 0 };
    static DIAG_MENU_OPTION useReservedMemoryMenu = { 0 };
    static DIAG_MENU_OPTION sendBufferThroughIpcMenu = { 0 };
    static DIAG_MENU_OPTION freeDmaMemoryMenu = { 0 };
    static DIAG_MENU_OPTION options[4 + MENU_DMA_PCI_INCREASE] = { 0 };

    strcpy(allocateContigMenu.cOptionName, "Allocate contiguous memory");
    allocateContigMenu.cbEntry = MenuDmaAllocContigOptionCb;

    strcpy(allocateSgMenu.cOptionName, "Allocate scatter-gather memory");
    allocateSgMenu.cbEntry = MenuDmaAllocSgOptionCb;

    strcpy(useReservedMemoryMenu.cOptionName, "Use reserved memory");
    useReservedMemoryMenu.cbEntry = MenuDmaReservedOptionCb;

    strcpy(sendBufferThroughIpcMenu.cOptionName, "Send buffer through IPC to "
        "all group processes");
    sendBufferThroughIpcMenu.cbEntry = MenuDmaSendSharedBufOptionCb;
    sendBufferThroughIpcMenu.cbIsHidden = MenuDmaIsContigBufferNotExists;

    strcpy(freeDmaMemoryMenu.cOptionName, "Free DMA memory");
    freeDmaMemoryMenu.cbEntry = MenuDmaFreeBufOptionCb;

    options[0] = allocateContigMenu;
    options[1] = allocateSgMenu;
    options[2] = useReservedMemoryMenu;
    options[3] = sendBufferThroughIpcMenu;
    options[3 + MENU_DMA_PCI_INCREASE] = freeDmaMemoryMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        pDmaSubMenusCtx, pParentMenu);
}

static void MenuDmaInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static DIAG_MENU_OPTION dmaMenuOption = { 0 };
    static MENU_CTX_DMA dmaMenusCtx = { 0 };

    strcpy(dmaMenuOption.cOptionName, "Allocate/free memory for DMA");
    strcpy(dmaMenuOption.cTitleName, "DMA memory");
    dmaMenuOption.cbIsHidden = MenuDmaIsDeviceNull;
    dmaMenuOption.cbExit = MenuDmaExitCb;

    dmaMenusCtx.phDev = phDev;
    dmaMenusCtx.pDma = NULL;
    dmaMenusCtx.pBuf = NULL;
    dmaMenusCtx.dwDmaAddressWidth = 0;
    dmaMenusCtx.dwOptions = 0;

    DIAG_MenuSetCtxAndParentForMenus(&dmaMenuOption, 1, &dmaMenusCtx,
        pParentMenu);
    MenuDmaSubMenusInit(&dmaMenuOption, &dmaMenusCtx);
}

/* -----------------------------------------------
   SRIOV handling
   ----------------------------------------------- */
#if defined(LINUX) && !defined(ISA)

static DWORD MenuSriovEnableOptionCb(PVOID pCbCtx)
{
    MENU_CTX_SRIOV *pSriovMenusCtx = (MENU_CTX_SRIOV *)pCbCtx;
    DWORD dwDesiredNumVFs, dwStatus = 0;

    if (pSriovMenusCtx->dwNumVFs > 0)
        goto Exit;

    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&dwDesiredNumVFs,
        "How many Virtual Functions would you like to enable:", FALSE,
        0, 0))
    {
        printf("Wrong Input\n");
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    dwStatus = PCI_SriovEnable(*(pSriovMenusCtx->phDev), dwDesiredNumVFs);
    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("SR-IOV enabled successfully\n");
        *(pSriovMenusCtx->pfIsEnabledSRIOV) = TRUE;
    }
    else
    {
        printf("Failed enabling SR-IOV. Error [0x%x - %s]\n",
            dwStatus, Stat2Str(dwStatus));
    }

Exit:
    return dwStatus;
}

static DWORD MenuSriovDisableOptionCb(PVOID pCbCtx)
{
    MENU_CTX_SRIOV *pSriovMenusCtx = (MENU_CTX_SRIOV *)pCbCtx;
    DWORD dwStatus = 0;
    if (!pSriovMenusCtx->dwNumVFs)
        goto Exit;

    dwStatus = PCI_SriovDisable(*(pSriovMenusCtx->phDev));
    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("SR-IOV disabled successfully\n");
        *(pSriovMenusCtx->pfIsEnabledSRIOV) = FALSE;
    }
    else
    {
        printf("Failed disabling SR-IOV. Error [0x%x - %s]\n",
            dwStatus, Stat2Str(dwStatus));
    }

Exit:
    return dwStatus;
}

static DWORD MenuSriovCb(PVOID pCbCtx)
{
    MENU_CTX_SRIOV *pSriovMenusCtx = (MENU_CTX_SRIOV *)pCbCtx;
    PDWORD pdwNumVFs = &pSriovMenusCtx->dwNumVFs;

    printf("\n");

    if (WD_STATUS_SUCCESS != PCI_SriovGetNumVFs(*(pSriovMenusCtx->phDev),
        pdwNumVFs))
    {
        printf("Could not obtain dwNumVFs.\n");
    }
    else
    {
        printf("SR-IOV is %s. dwNumVFs: [%d]\n",
            (*pdwNumVFs > 0) ? "Enabled" : "Disabled", *pdwNumVFs);
    }

    printf("-----------\n");

    return WD_STATUS_SUCCESS;
}

static void MenuSriovInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static MENU_CTX_SRIOV sriovMenusCtx = { 0 };
    static MENU_SRIOV_CALLBACKS sriovCallbacks = { 0 };

    sriovMenusCtx.phDev = phDev;
    sriovMenusCtx.dwNumVFs = 0;
    sriovMenusCtx.pfIsEnabledSRIOV = &fIsEnabledSRIOV;

    sriovCallbacks.sriovEnableCb = MenuSriovEnableOptionCb;
    sriovCallbacks.sriovDisableCb = MenuSriovDisableOptionCb;
    sriovCallbacks.sriovMenuEntryCb = MenuSriovCb;

    MenuCommonSriovInit(pParentMenu, &sriovMenusCtx, &sriovCallbacks);
}
#endif


static DWORD CheckKPDriverVer(WDC_DEVICE_HANDLE hDev)
{
    KP_PCI_VERSION kpVer = { 0 };
    DWORD dwStatus;
    DWORD dwKPResult = 0;

    /* Get Kernel PlugIn Driver version */
    dwStatus = WDC_CallKerPlug(hDev, KP_PCI_MSG_VERSION, &kpVer, &dwKPResult);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        PCI_ERR("Failed sending a \'Get Version\' message [0x%x] to the "
            "Kernel-PlugIn driver [%s]. Error [0x%x - %s]\n",
            KP_PCI_MSG_VERSION, KP_PCI_DRIVER_NAME, dwStatus,
            Stat2Str(dwStatus));
    }
    else if (KP_PCI_STATUS_OK != dwKPResult)
    {
        PCI_ERR("Kernel-PlugIn \'Get Version\' message [0x%x] failed. "
            "Kernel PlugIn status [0x%x]\n", KP_PCI_MSG_VERSION, dwKPResult);
        dwStatus = WD_INCORRECT_VERSION;
    }
    else
    {
        PCI_TRACE("Using [%s] Kernel-Plugin driver version [%d.%02d - %s]\n",
            KP_PCI_DRIVER_NAME, kpVer.dwVer / 100, kpVer.dwVer % 100,
            kpVer.cVer);
    }

    return dwStatus;
}


