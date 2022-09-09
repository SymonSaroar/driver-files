/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/***************************************************************************
*  File: p6466_diag.c
*
*  Sample user-mode diagnostics application for accessing PLX 6466
*  devices using WinDriver's API and the plx_lib_6466, plx_diag_lib_6466,
*  plx_lib and plx_diag_lib libraries.
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
****************************************************************************/

#include "status_strings.h"
#include "diag_lib.h"
#include "wdc_diag_lib.h"
#include "../diag_lib/plx_diag_lib.h"
#include "lib_6466/plx_lib_6466.h"

/*************************************************************
  General definitions
 *************************************************************/
/* Default vendor and device IDs */
#define P6466_DEFAULT_VENDOR_ID 0x10b5 /* Vendor ID */
#define P6466_DEFAULT_DEVICE_ID 0x6540 /* Device ID */

/*************************************************************
  Global variables
 *************************************************************/
/* User's input command */
static CHAR gsInput[256];

/* Configuration registers information array */
static const WDC_REG gPLX_CfgRegs[] = {
    { WDC_AD_CFG_SPACE, PLX6466_VID, WDC_SIZE_16, WDC_READ_WRITE, "VID",
        "Vendor ID" },
    { WDC_AD_CFG_SPACE, PLX6466_DID, WDC_SIZE_16, WDC_READ_WRITE, "DID",
        "Device ID" },
    { WDC_AD_CFG_SPACE, PLX6466_CR, WDC_SIZE_16, WDC_READ_WRITE, "CMD",
        "Command" },
    { WDC_AD_CFG_SPACE, PLX6466_PSR, WDC_SIZE_16, WDC_READ_WRITE, "PSTS",
        "Primary Status" },
    { WDC_AD_CFG_SPACE, PLX6466_REV, WDC_SIZE_32, WDC_READ_WRITE, "RID_CLCD",
        "Revision ID & Class Code" },
    { WDC_AD_CFG_SPACE, PLX6466_CCSC, WDC_SIZE_8, WDC_READ_WRITE, "SCC",
        "Sub Class Code" },
    { WDC_AD_CFG_SPACE, PLX6466_CCBC, WDC_SIZE_8, WDC_READ_WRITE, "BCC",
        "Base Class Code" },
    { WDC_AD_CFG_SPACE, PLX6466_CLSR, WDC_SIZE_8, WDC_READ_WRITE, "CALN",
        "Cache Line Size" },
    { WDC_AD_CFG_SPACE, PLX6466_PLTR, WDC_SIZE_8, WDC_READ_WRITE, "PLAT",
        "Primary Latency Timer" },
    { WDC_AD_CFG_SPACE, PLX6466_HDR, WDC_SIZE_8, WDC_READ_WRITE, "HDR",
        "Header Type" },
    { WDC_AD_CFG_SPACE, PLX6466_BISTR, WDC_SIZE_8, WDC_READ_WRITE, "BIST",
        "Built-in Self Test" },
    { WDC_AD_CFG_SPACE, PLX6466_BAR0, WDC_SIZE_32, WDC_READ_WRITE, "BADDR0",
        "Base Address 0" },
    { WDC_AD_CFG_SPACE, PLX6466_BAR1, WDC_SIZE_32, WDC_READ_WRITE, "BADDR1",
        "Base Address 1" },
    { WDC_AD_CFG_SPACE, PLX6466_PBN, WDC_SIZE_8, WDC_READ_WRITE, "PBN",
        "Primary bus number" },
    { WDC_AD_CFG_SPACE, PLX6466_SBN, WDC_SIZE_8, WDC_READ_WRITE, "SBN",
        "Secondary bus number" },
    { WDC_AD_CFG_SPACE, PLX6466_SUBN, WDC_SIZE_8, WDC_READ_WRITE, "SUBN",
        "Subordinate bus number" },
    { WDC_AD_CFG_SPACE, PLX6466_SLTR, WDC_SIZE_8, WDC_READ_WRITE, "SLTR",
        "Secondary latency timer" },
    { WDC_AD_CFG_SPACE, PLX6466_IOB, WDC_SIZE_8, WDC_READ_WRITE, "IOB",
        "I/O Base" },
    { WDC_AD_CFG_SPACE, PLX6466_IOL, WDC_SIZE_8, WDC_READ_WRITE, "IOL",
        "I/O Limit" },
    { WDC_AD_CFG_SPACE, PLX6466_SSR, WDC_SIZE_8, WDC_READ_WRITE, "SSR",
        "Secondary Status register" },
    { WDC_AD_CFG_SPACE, PLX6466_MEMB, WDC_SIZE_16, WDC_READ_WRITE, "MEMB",
        "Memory Base" },
    { WDC_AD_CFG_SPACE, PLX6466_MEML, WDC_SIZE_16, WDC_READ_WRITE, "MEML",
        "Memory Limit" },
    { WDC_AD_CFG_SPACE, PLX6466_PMB, WDC_SIZE_16, WDC_READ_WRITE, "PMB",
        "Prefetchable Memory Base" },
    { WDC_AD_CFG_SPACE, PLX6466_PML, WDC_SIZE_16, WDC_READ_WRITE, "PML",
        "Prefetchable Memory Limit" },
    { WDC_AD_CFG_SPACE, PLX6466_PMBU, WDC_SIZE_32, WDC_READ_WRITE, "PMBU",
        "Prefetchable Memory Base Upper 32 bits" },
    { WDC_AD_CFG_SPACE, PLX6466_PMLU, WDC_SIZE_32, WDC_READ_WRITE, "PMLU",
        "Prefetchable Memory Limit Upper 32 bits" },
    { WDC_AD_CFG_SPACE, PLX6466_CAP, WDC_SIZE_8, WDC_READ_WRITE, "NEW_CAP",
        "New Capabilities Pointer" },
    { WDC_AD_CFG_SPACE, PLX6466_IPR, WDC_SIZE_8, WDC_READ_WRITE, "INTPIN",
        "Interrupt Pin" },
        /* PLX-specific configuration registers */
        { WDC_AD_CFG_SPACE, PLX6466_PMCI, WDC_SIZE_8, WDC_READ_WRITE, "PMCI",
            "Power Management Capability ID" },
        { WDC_AD_CFG_SPACE, PLX6466_PMNCP, WDC_SIZE_8, WDC_READ_WRITE, "PMNCP",
            "Power Management Next Capability Pointer" },
        { WDC_AD_CFG_SPACE, PLX6466_PMC, WDC_SIZE_16, WDC_READ_WRITE, "PMCAP",
            "Power Management Capabilities" },
        { WDC_AD_CFG_SPACE, PLX6466_PMCS, WDC_SIZE_16, WDC_READ_WRITE, "PMCSR",
            "Power Management Control/Status" },
        { WDC_AD_CFG_SPACE, PLX6466_HSCL, WDC_SIZE_8, WDC_READ_WRITE, "HS_CAPID",
            "Hot Swap Capability ID" },
        { WDC_AD_CFG_SPACE, PLX6466_HSNCP, WDC_SIZE_8, WDC_READ_WRITE, "HS_NEXT",
            "Hot Swap Next Capability Pointer" },
        { WDC_AD_CFG_SPACE, PLX6466_HSCS, WDC_SIZE_8, WDC_READ_WRITE, "HS_CSR",
            "Hot Swap Control/Status" },
        { WDC_AD_CFG_SPACE, PLX6466_VCI, WDC_SIZE_8, WDC_READ_WRITE, "VPD_CAPID",
            "PCI Vital Product Data Capability ID" },
        { WDC_AD_CFG_SPACE, PLX6466_VNCP, WDC_SIZE_8, WDC_READ_WRITE, "VPD_NEXT",
            "PCI Vital Product Next Capability Pointer" },
        { WDC_AD_CFG_SPACE, PLX6466_VPDA, WDC_SIZE_16, WDC_READ_WRITE, "VPD_ADDR",
            "PCI Vital Product Data Address" },
        { WDC_AD_CFG_SPACE, PLX6466_VPDD, WDC_SIZE_32, WDC_READ_WRITE, "VPD_DATA",
            "PCI VPD Data" },

            /* NOTE - add here more registers according to the configuration space */
};

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
void PLX6466_DIAG_MenuDeviceOpenInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

/* -----------------------------------------------
    Read/write the configuration space
   ----------------------------------------------- */
static void PLX6466_DIAG_MenuReadWriteCfgSpaceInit(
    DIAG_MENU_OPTION *pParentMenu, WDC_DEVICE_HANDLE *phDev);

/* ----------------------------------------------------
    Plug-and-play and power management events handling
   ---------------------------------------------------- */
static void PLX6466_DIAG_MenuEventsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev, PLX_EVENT_HANDLER DiagEventHandler);
static void DiagEventHandler(WDC_DEVICE_HANDLE hDev, DWORD dwAction);

/* -----------------------------------------------
    Access the serial EEPROM
   ----------------------------------------------- */
static void PLX6466_DIAG_MenuEEPROMInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

/*************************************************************
  Functions implementation
 *************************************************************/
static DWORD PLX_Init(WDC_DEVICE_HANDLE *phDev)
{
    /* Initialize the PLX library */
    DWORD dwStatus = PLX_LibInit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        PLX_DIAG_ERR("p6466_diag: Failed to initialize the PLX library: %s",
            PLX_GetLastErr());
        return dwStatus;
    }

    /* Find and open a PLX 6466 device (by default ID) */
    if (P6466_DEFAULT_VENDOR_ID)
    {
        *phDev = PLX6466_DeviceOpen(P6466_DEFAULT_VENDOR_ID,
            P6466_DEFAULT_DEVICE_ID);
        if (!*phDev)
        {
            PLX_DIAG_ERR("p6466_diag: Failed locating and opening a handle to "
                "device (VID 0x%x DID 0x%x)\n", P6466_DEFAULT_VENDOR_ID,
                P6466_DEFAULT_DEVICE_ID);
        }
    }

    return WD_STATUS_SUCCESS;
}

int main(void)
{
    DWORD dwStatus;
    WDC_DEVICE_HANDLE hDev = NULL;
    DIAG_MENU_OPTION *pMenuRoot;

    printf("\n");
    printf("PLX 6466 diagnostic utility.\n");
    printf("Application accesses hardware using " WD_PROD_NAME ".\n");

    /* Initialize the PLX library */
    dwStatus = PLX_Init(&hDev);
    if (WD_STATUS_SUCCESS != dwStatus)
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
    DWORD dwStatus = WD_STATUS_SUCCESS;
    WDC_DEVICE_HANDLE *phDev = (WDC_DEVICE_HANDLE *)pCbCtx;

    /* Perform necessary cleanup before exiting the program */
    if (*phDev && !PLX_DeviceClose(*phDev))
        PLX_DIAG_ERR("p6466_diag: Failed closing PLX device: %s",
            PLX_GetLastErr());

    dwStatus = PLX_LibUninit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        PLX_DIAG_ERR("p6466_diag: Failed to uninit the PLX library: %s",
            PLX_GetLastErr());
    }

    return WD_STATUS_SUCCESS;
}

static DIAG_MENU_OPTION *MenuMainInit(WDC_DEVICE_HANDLE *phDev)
{
    static DIAG_MENU_OPTION menuRoot = { 0 };
    strcpy(menuRoot.cTitleName, "PLX 6466 main menu");
    menuRoot.cbExit = MenuMainExitCb;
    menuRoot.pCbCtx = phDev;

    MenuCommonScanBusInit(&menuRoot);
    PLX6466_DIAG_MenuDeviceOpenInit(&menuRoot, phDev);
    PLX6466_DIAG_MenuReadWriteCfgSpaceInit(&menuRoot, phDev);
    PLX6466_DIAG_MenuEventsInit(&menuRoot, phDev, DiagEventHandler);
    PLX6466_DIAG_MenuEEPROMInit(&menuRoot, phDev);

    return &menuRoot;
}

/* -----------------------------------------------
     Device Open
    ----------------------------------------------- */
static DWORD MenuDeviceOpenCb(PVOID pCbCtx)
{
    WDC_DEVICE_HANDLE *phDev = (WDC_DEVICE_HANDLE *)pCbCtx;

    /* Close open device handle (if exists) */
    if (*phDev && !PLX_DeviceClose(*phDev))
        PLX_DIAG_ERR("p6466_diag: Failed closing PLX device: %s",
            PLX_GetLastErr());

    /* Open a new device handle */
    *phDev = PLX6466_DeviceOpen(0, 0);
    if (!*phDev)
    {
        /* TODO: Remove VID DID from print? */
        PLX_DIAG_ERR("plx6466_diag: Failed locating and opening a handle "
            "to device (VID 0x%x DID 0x%x)\n", 0,
            0);
    }

    return WD_STATUS_SUCCESS;
}

void PLX6466_DIAG_MenuDeviceOpenInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static DIAG_MENU_OPTION deviceOpenMenu = { 0 };
    strcpy(deviceOpenMenu.cOptionName, "Find and open a PLX device");
    deviceOpenMenu.cbEntry = MenuDeviceOpenCb;

    DIAG_MenuSetCtxAndParentForMenus(&deviceOpenMenu, 1, phDev,
        pParentMenu);
}

/* -----------------------------------------------
    Read/write the configuration space
   ----------------------------------------------- */

static void PLX6466_DIAG_MenuReadWriteCfgSpaceInit(
    DIAG_MENU_OPTION *pParentMenu, WDC_DEVICE_HANDLE *phDev)
{
    static MENU_CTX_CFG cfgCtx;

    BZERO(cfgCtx);
    cfgCtx.phDev = phDev;
    cfgCtx.pCfgRegs = gPLX_CfgRegs;
    cfgCtx.dwCfgRegsNum = sizeof(gPLX_CfgRegs) / sizeof(*gPLX_CfgRegs);

    MenuCommonCfgInit(pParentMenu, &cfgCtx);
}

/* ----------------------------------------------------
    Plug-and-play and power management events handling
   ---------------------------------------------------- */
static DWORD MenuEventsRegisterOptionCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    DWORD dwStatus = PLX6466_EventRegister(*(pEventsMenusCtx->phDev),
        (PLX_EVENT_HANDLER)pEventsMenusCtx->DiagEventHandler);

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Events registered\n");
        pEventsMenusCtx->fRegistered = TRUE;
    }
    else
    {
        printf("Failed to register events. Last error [%s]\n",
            PLX_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuEventsUnregisterOptionCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    DWORD dwStatus = PLX6466_EventUnregister(*(pEventsMenusCtx->phDev));

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Events unregistered\n");
        pEventsMenusCtx->fRegistered = FALSE;
    }
    else
    {
        printf("Failed to unregister events. Last error [%s]\n",
            PLX_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuEventsCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    pEventsMenusCtx->fRegistered = PLX6466_EventIsRegistered(
        *pEventsMenusCtx->phDev);

    return WD_STATUS_SUCCESS;
}

static void PLX6466_DIAG_MenuEventsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev, PLX_EVENT_HANDLER DiagEventHandler)
{
    MENU_EVENTS_CALLBACKS eventsMenuCbs = { 0 };
    eventsMenuCbs.eventsMenuEntryCb = MenuEventsCb;
    eventsMenuCbs.eventsEnableCb = MenuEventsRegisterOptionCb;
    eventsMenuCbs.eventsDisableCb = MenuEventsUnregisterOptionCb;

    static MENU_CTX_EVENTS eventsMenusCtx = { 0 };
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
    Access the serial EEPROM
   ----------------------------------------------- */

/* Access the serial EEPROM on the board */
#define EEPROM_MAX_OFFSET 0xFF
static BOOL MenuIsEepromNotExists(DIAG_MENU_OPTION *pMenu)
{
    return !PLX6466_EEPROM_VPD_Validate(*((WDC_DEVICE_HANDLE *)pMenu->pCbCtx));
}

static DWORD MenuEepromDisplayOptionCb(PVOID pCbCtx)
{
    WDC_DEVICE_HANDLE hDev = *(WDC_DEVICE_HANDLE *)pCbCtx;
    DWORD dwStatus, dwOffset;
    UINT32 u32Data;

    for (dwOffset = 0; dwOffset < EEPROM_MAX_OFFSET; dwOffset += 4)
    {
        if (!(dwOffset % 0x10))
            printf("\n %02X: ", dwOffset);
        dwStatus = PLX6466_EEPROM_VPD_Read32(hDev, dwOffset, &u32Data);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            printf("\nError occurred while reading offset 0x%x of the"
                " serial EEPROM.\n Error 0x%x - %s\n", dwOffset,
                dwStatus, Stat2Str(dwStatus));
            break;
        }
        printf("%08X  ", u32Data);
    }
    printf("\n");

    return dwStatus;
}

static DWORD MenuEepromReadOptionCb(PVOID pCbCtx)
{
    WDC_DEVICE_HANDLE hDev = *(WDC_DEVICE_HANDLE *)pCbCtx;
    DWORD dwStatus, dwOffset;
    UINT32 u32Data;

    sprintf(gsInput, "Enter offset to read from (must be a multiple of"
        "%d)", WDC_SIZE_32);
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(&dwOffset, gsInput, TRUE,
        0, EEPROM_MAX_OFFSET))
    {
        return WD_INVALID_PARAMETER;
    }

    dwStatus = PLX6466_EEPROM_VPD_Read32(hDev, dwOffset, &u32Data);

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Read 0x%X from offset 0x%x of the serial EEPROM\n",
            u32Data, dwOffset);
    }
    else
        printf("Failed reading from offset 0x%x of the serial EEPROM."
            "\n", dwOffset);

    return dwStatus;
}

static DWORD MenuEepromWriteOptionCb(PVOID pCbCtx)
{
    WDC_DEVICE_HANDLE hDev = *(WDC_DEVICE_HANDLE *)pCbCtx;
    DWORD dwStatus, dwOffset;
    UINT32 u32Data;

    sprintf(gsInput, "Enter offset to write to (must be a multiple of"
        "%d)", WDC_SIZE_32);
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(&dwOffset, gsInput, TRUE,
        0, EEPROM_MAX_OFFSET))
    {
        return WD_INVALID_PARAMETER;
    }

    if (DIAG_INPUT_SUCCESS != DIAG_InputNum((PVOID)&u32Data,
        "Enter data to write", TRUE, sizeof(u32Data), 0, 0))
    {
        return WD_INVALID_PARAMETER;
    }

    dwStatus = PLX6466_EEPROM_VPD_Write32(hDev, dwOffset, u32Data);

    printf("%s 0x%X to offset 0x%x of the serial EEPROM\n",
        (WD_STATUS_SUCCESS == dwStatus) ? "Wrote" : "Failed to write",
        u32Data, dwOffset);

    return dwStatus;
}

static void PLX_DIAG_MenuEEPROMSubMenusInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static DIAG_MENU_OPTION displayEepromMenu = { 0 };
    static DIAG_MENU_OPTION readSerialEepromMenu = { 0 };
    static DIAG_MENU_OPTION writeSerialEepromMenu = { 0 };
    static DIAG_MENU_OPTION options[3] = { 0 };

    strcpy(displayEepromMenu.cOptionName, "Display EEPROM content");
    displayEepromMenu.cbEntry = MenuEepromDisplayOptionCb;

    strcpy(readSerialEepromMenu.cOptionName, "Read from the serial EEPROM");
    readSerialEepromMenu.cbEntry = MenuEepromReadOptionCb;

    strcpy(writeSerialEepromMenu.cOptionName, "Write to the serial EEPROM");
    writeSerialEepromMenu.cbEntry = MenuEepromWriteOptionCb;

    options[0] = displayEepromMenu;
    options[1] = readSerialEepromMenu;
    options[2] = writeSerialEepromMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        phDev, pParentMenu);
}


static DWORD MenuEEPROMCb(PVOID pCbCtx)
{
    UNUSED_VAR(pCbCtx);

    printf("NOTE: EEPROM data is accessed via Vital Product Data (VPD) as"
        "DWORDs\n");

    return WD_STATUS_SUCCESS;
}

static void PLX6466_DIAG_MenuEEPROMInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static DIAG_MENU_OPTION eepromMenuRoot = { 0 };

    strcpy(eepromMenuRoot.cOptionName, "Access the serial EEPROM on the "
        "board");
    strcpy(eepromMenuRoot.cTitleName, "Access the board's serial EEPROM");
    eepromMenuRoot.cbEntry = MenuEEPROMCb;
    eepromMenuRoot.cbIsHidden = MenuIsEepromNotExists;

    PLX_DIAG_MenuEEPROMSubMenusInit(&eepromMenuRoot, phDev);

    DIAG_MenuSetCtxAndParentForMenus(&eepromMenuRoot, 1, phDev,
        pParentMenu);
}


