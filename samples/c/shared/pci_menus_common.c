#if !defined(__KERNEL__)

#include "pci_menus_common.h"

BOOL MenuCommonIsDeviceNull(DIAG_MENU_OPTION *pMenu)
{
    return *((WDC_DEVICE_HANDLE *)pMenu->pCbCtx) == NULL;
}

#ifndef ISA
/* -----------------------------------------------
   Scan Bus
   ----------------------------------------------- */
static DWORD MenuCommonScanBusCb(PVOID pCbCtx)
{
    UNUSED_VAR(pCbCtx);
    WDC_DIAG_PciDevicesInfoPrintAll(FALSE);

    return WD_STATUS_SUCCESS;
}

DIAG_MENU_OPTION *MenuCommonScanBusInit(DIAG_MENU_OPTION *pParentMenu)
{
    static DIAG_MENU_OPTION menuScanBusOption = { 0 };

    strcpy(menuScanBusOption.cOptionName, "Scan PCI bus");
    menuScanBusOption.cbEntry = MenuCommonScanBusCb;

    DIAG_MenuSetCtxAndParentForMenus(&menuScanBusOption, 1, NULL,
        pParentMenu);

    return &menuScanBusOption;
}

/* -----------------------------------------------
    Read/write the configuration space
   ----------------------------------------------- */
static BOOL MenuCfgIsDeviceNull(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_CFG *pCfgCtx = (MENU_CTX_CFG *)pMenu->pCbCtx;

    return *(pCfgCtx->phDev) == NULL;
}

static BOOL MenuCfgIsDeviceNotExpress(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_CFG *pCfgCtx = (MENU_CTX_CFG *)pMenu->pCbCtx;

    return WDC_PciGetExpressGen(*(pCfgCtx->phDev)) == 0;
}

static DWORD MenuCfgReadOffsetOptionCb(PVOID pCbCtx)
{
    MENU_CTX_CFG *pCfgCtx = (MENU_CTX_CFG *)pCbCtx;

    WDC_DIAG_ReadWriteBlock(*(pCfgCtx->phDev), WDC_READ, WDC_AD_CFG_SPACE);

    return WD_STATUS_SUCCESS;
}

static DWORD MenuCfgWriteOffsetOptionCb(PVOID pCbCtx)
{
    MENU_CTX_CFG *pCfgCtx = (MENU_CTX_CFG *)pCbCtx;

    WDC_DIAG_ReadWriteBlock(*(pCfgCtx->phDev), WDC_WRITE, WDC_AD_CFG_SPACE);

    return WD_STATUS_SUCCESS;
}

static DWORD MenuCfgReadAllOptionCb(PVOID pCbCtx)
{
    MENU_CTX_CFG *pCfgCtx = (MENU_CTX_CFG *)pCbCtx;

    WDC_DIAG_ReadRegsAll(*(pCfgCtx->phDev), pCfgCtx->pCfgRegs,
        pCfgCtx->dwCfgRegsNum, TRUE, FALSE);

    if (WDC_PciGetExpressGen(*(pCfgCtx->phDev)) != 0)
    {
        WDC_DIAG_ReadRegsAll(*(pCfgCtx->phDev), pCfgCtx->pCfgExpRegs,
            pCfgCtx->dwCfgExpRegsNum, TRUE, TRUE);
    }
    return WD_STATUS_SUCCESS;
}

static DWORD MenuCfgReadNamedRegOptionCb(PVOID pCbCtx)
{
    MENU_CTX_CFG *pCfgCtx = (MENU_CTX_CFG *)pCbCtx;

    WDC_DIAG_ReadWriteReg(*(pCfgCtx->phDev), pCfgCtx->pCfgRegs,
        pCfgCtx->dwCfgRegsNum, WDC_READ, TRUE, FALSE);

    return WD_STATUS_SUCCESS;
}

static DWORD MenuCfgWriteNamedRegOptionCb(PVOID pCbCtx)
{
    MENU_CTX_CFG *pCfgCtx = (MENU_CTX_CFG *)pCbCtx;

    WDC_DIAG_ReadWriteReg(*(pCfgCtx->phDev), pCfgCtx->pCfgRegs,
        pCfgCtx->dwCfgRegsNum, WDC_WRITE, TRUE, FALSE);

    return WD_STATUS_SUCCESS;
}

static DWORD MenuCfgReadNamedExpRegOptionCb(PVOID pCbCtx)
{
    MENU_CTX_CFG *pCfgCtx = (MENU_CTX_CFG *)pCbCtx;

    WDC_DIAG_ReadWriteReg(*(pCfgCtx->phDev), pCfgCtx->pCfgExpRegs,
        pCfgCtx->dwCfgExpRegsNum, WDC_READ, TRUE, TRUE);

    return WD_STATUS_SUCCESS;
}

static DWORD MenuCfgWriteNamedExpRegOptionCb(PVOID pCbCtx)
{
    MENU_CTX_CFG *pCfgCtx = (MENU_CTX_CFG *)pCbCtx;

    WDC_DIAG_ReadWriteReg(*(pCfgCtx->phDev), pCfgCtx->pCfgExpRegs,
        pCfgCtx->dwCfgExpRegsNum, WDC_WRITE, TRUE, TRUE);

    return WD_STATUS_SUCCESS;
}

static DWORD MenuCfgScanCapsOptionCb(PVOID pCbCtx)
{
    MENU_CTX_CFG *pCfgCtx = (MENU_CTX_CFG *)pCbCtx;

    WDC_DIAG_ScanPCICapabilities(*((pCfgCtx->phDev)));

    return WD_STATUS_SUCCESS;
}

static void MenuCommonCfgSubMenusInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_CFG *pCfgCtx)
{
    static DIAG_MENU_OPTION readOffsetMenu = { 0 };
    static DIAG_MENU_OPTION writeOffsetMenu = { 0 };
    static DIAG_MENU_OPTION readAllConfigurationRegistersMenu = { 0 };
    static DIAG_MENU_OPTION readNamedRegisterMenu = { 0 };
    static DIAG_MENU_OPTION writeNamedRegisterMenu = { 0 };
    static DIAG_MENU_OPTION readPcieNamedRegisterMenu = { 0 };
    static DIAG_MENU_OPTION writePcieNamedRegisterMenu = { 0 };
    static DIAG_MENU_OPTION scanCapabilitiesMenu = { 0 };
    static DIAG_MENU_OPTION options[8] = { 0 };

    strcpy(readOffsetMenu.cOptionName, "Read from an offset");
    readOffsetMenu.cbEntry = MenuCfgReadOffsetOptionCb;

    strcpy(writeOffsetMenu.cOptionName, "Write to an offset");
    writeOffsetMenu.cbEntry = MenuCfgWriteOffsetOptionCb;

    strcpy(readAllConfigurationRegistersMenu.cOptionName, "Read all configuration registers "
        "defined for the device (see list above)");
    readAllConfigurationRegistersMenu.cbEntry = MenuCfgReadAllOptionCb;

    strcpy(readNamedRegisterMenu.cOptionName, "Read from a named register");
    readNamedRegisterMenu.cbEntry = MenuCfgReadNamedRegOptionCb;

    strcpy(writeNamedRegisterMenu.cOptionName, "Write to a named register");
    writeNamedRegisterMenu.cbEntry = MenuCfgWriteNamedRegOptionCb;

    strcpy(readPcieNamedRegisterMenu.cOptionName, "Read from a named PCI Express register");
    readPcieNamedRegisterMenu.cbEntry = MenuCfgReadNamedExpRegOptionCb;
    readPcieNamedRegisterMenu.cbIsHidden = MenuCfgIsDeviceNotExpress;

    strcpy(writePcieNamedRegisterMenu.cOptionName, "Write to a named PCI Express register");
    writePcieNamedRegisterMenu.cbEntry = MenuCfgWriteNamedExpRegOptionCb;
    writePcieNamedRegisterMenu.cbIsHidden = MenuCfgIsDeviceNotExpress;

    strcpy(scanCapabilitiesMenu.cOptionName, "Scan PCI/PCIe capabilities");
    scanCapabilitiesMenu.cbEntry = MenuCfgScanCapsOptionCb;

    options[0] = readOffsetMenu;
    options[1] = writeOffsetMenu;
    options[2] = readAllConfigurationRegistersMenu;
    options[3] = readNamedRegisterMenu;
    options[4] = writeNamedRegisterMenu;
    options[5] = readPcieNamedRegisterMenu;
    options[6] = writePcieNamedRegisterMenu;
    options[7] = scanCapabilitiesMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options), pCfgCtx,
        pParentMenu);
}

static DWORD MenuCfgCb(PVOID pCbCtx)
{
    MENU_CTX_CFG *pCfgCtx = (MENU_CTX_CFG *)pCbCtx;

    printf("\nConfiguration registers:\n");
    printf("------------------------");

    WDC_DIAG_RegsInfoPrint(*(pCfgCtx->phDev), pCfgCtx->pCfgRegs,
        pCfgCtx->dwCfgRegsNum,
        WDC_DIAG_REG_PRINT_ALL & ~WDC_DIAG_REG_PRINT_ADDR_SPACE, FALSE);

    if (WDC_PciGetExpressGen(*(pCfgCtx->phDev)) != 0)
    {
        WDC_DIAG_RegsInfoPrint(*(pCfgCtx->phDev), pCfgCtx->pCfgExpRegs,
            pCfgCtx->dwCfgExpRegsNum,
            WDC_DIAG_REG_PRINT_ALL & ~WDC_DIAG_REG_PRINT_ADDR_SPACE, TRUE);
    }

    printf("\n");

    return WD_STATUS_SUCCESS;
}

DIAG_MENU_OPTION *MenuCommonCfgInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_CFG *pCfgCtx)
{
    static DIAG_MENU_OPTION cfgMenuOption = { 0 };

    strcpy(cfgMenuOption.cOptionName, "Read/write the PCI configuration space");
    strcpy(cfgMenuOption.cTitleName, "Read/write the device's configuration space");
    cfgMenuOption.cbEntry = MenuCfgCb;
    cfgMenuOption.cbIsHidden = MenuCfgIsDeviceNull;

    DIAG_MenuSetCtxAndParentForMenus(&cfgMenuOption, 1, pCfgCtx, pParentMenu);
    MenuCommonCfgSubMenusInit(&cfgMenuOption, pCfgCtx);

    return &cfgMenuOption;
}

/* ----------------------------------------------------
    Plug-and-play and power management events handling
   ---------------------------------------------------- */

static BOOL MenuEventsIsDeviceNull(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_EVENTS *pCtx = (MENU_CTX_EVENTS *)pMenu->pCbCtx;

    return *pCtx->phDev == NULL;
}

static BOOL MenuEventsIsRegistered(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_EVENTS *pCtx = (MENU_CTX_EVENTS *)pMenu->pCbCtx;

    return pCtx->fRegistered;
}

static BOOL MenuEventsIsUnregistered(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_EVENTS *pCtx = (MENU_CTX_EVENTS *)pMenu->pCbCtx;

    return !pCtx->fRegistered;
}

static void MenuCommonEventsSubMenusInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_EVENTS *pEventsMenusCtx, MENU_EVENTS_CALLBACKS *pEventsCallbacks)
{
    static DIAG_MENU_OPTION registerEventsMenu = { 0 };
    static DIAG_MENU_OPTION unRegisterEventsMenu = { 0 };
    static DIAG_MENU_OPTION options[2] = { 0 };

    strcpy(registerEventsMenu.cOptionName, "Register Events");
    registerEventsMenu.cbEntry = pEventsCallbacks->eventsEnableCb;
    registerEventsMenu.cbIsHidden = MenuEventsIsRegistered;

    strcpy(unRegisterEventsMenu.cOptionName, "Unregister Events");
    unRegisterEventsMenu.cbEntry = pEventsCallbacks->eventsDisableCb;
    unRegisterEventsMenu.cbIsHidden = MenuEventsIsUnregistered;

    options[0] = registerEventsMenu;
    options[1] = unRegisterEventsMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        pEventsMenusCtx, pParentMenu);
}

DIAG_MENU_OPTION *MenuCommonEventsInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_EVENTS *pEventsMenusCtx, MENU_EVENTS_CALLBACKS *pEventsCallbacks)
{
    static DIAG_MENU_OPTION eventsMenuOption = { 0 };

    strcpy(eventsMenuOption.cOptionName, "Register/unregister plug-and-play "
        "and power management events");
    strcpy(eventsMenuOption.cTitleName, "Plug-and-play and power management "
        "events");
    eventsMenuOption.cbIsHidden = MenuEventsIsDeviceNull;
    eventsMenuOption.cbEntry = pEventsCallbacks->eventsMenuEntryCb;
    eventsMenuOption.cbExit = pEventsCallbacks->eventsMenuExitCb;

    DIAG_MenuSetCtxAndParentForMenus(&eventsMenuOption, 1, pEventsMenusCtx,
        pParentMenu);
    MenuCommonEventsSubMenusInit(&eventsMenuOption, pEventsMenusCtx,
        pEventsCallbacks);

    return &eventsMenuOption;
}

#ifdef LINUX
/* -----------------------------------------------
   SRIOV handling
   ----------------------------------------------- */
BOOL MenuSriovIsDeviceNull(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_SRIOV *pCtx = (MENU_CTX_SRIOV *)pMenu->pCbCtx;

    return *pCtx->phDev == NULL;
}

BOOL MenuSriovAreNoVFs(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_SRIOV *pCtx = (MENU_CTX_SRIOV *)pMenu->pCbCtx;

    return pCtx->dwNumVFs == 0;
}

BOOL MenuSriovAreVFsExists(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_SRIOV *pCtx = (MENU_CTX_SRIOV *)pMenu->pCbCtx;

    return pCtx->dwNumVFs > 0;
}

static void MenuCommonSriovSubMenusInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_SRIOV *pSriovMenusCtx, MENU_SRIOV_CALLBACKS *pSriovCallbacks)
{
    static DIAG_MENU_OPTION enableSriovMenu = { 0 };
    static DIAG_MENU_OPTION disableSriovMenu = { 0 };
    static DIAG_MENU_OPTION options[2] = { 0 };

    strcpy(enableSriovMenu.cOptionName, "Enable SR-IOV");
    enableSriovMenu.cbIsHidden = MenuSriovAreVFsExists;
    enableSriovMenu.cbEntry = pSriovCallbacks->sriovEnableCb;

    strcpy(disableSriovMenu.cOptionName, "Disable SR-IOV");
    disableSriovMenu.cbIsHidden = MenuSriovAreNoVFs;
    disableSriovMenu.cbEntry = pSriovCallbacks->sriovDisableCb;

    options[0] = enableSriovMenu;
    options[1] = disableSriovMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        pSriovMenusCtx, pParentMenu);
}

DIAG_MENU_OPTION *MenuCommonSriovInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_SRIOV *pSriovCtx, MENU_SRIOV_CALLBACKS *pSriovCallbacks)
{
    static DIAG_MENU_OPTION sriovMenuOption = { 0 };

    strcpy(sriovMenuOption.cOptionName, "Enable/disable SR-IOV capability");
    sriovMenuOption.cbIsHidden = MenuSriovIsDeviceNull;
    sriovMenuOption.cbEntry = pSriovCallbacks->sriovMenuEntryCb;
    sriovMenuOption.cbExit = pSriovCallbacks->sriovMenuExitCb;

    DIAG_MenuSetCtxAndParentForMenus(&sriovMenuOption, 1, pSriovCtx,
        pParentMenu);
    MenuCommonSriovSubMenusInit(&sriovMenuOption, pSriovCtx,
        pSriovCallbacks);

    return &sriovMenuOption;
}
#endif /* ifdef LINUX*/
#endif /* ifndef ISA*/

/* -----------------------------------------------
    Read/write memory and I/O addresses
   ----------------------------------------------- */
#ifdef ISA
#define PCI_ADDR_SPACES_NUM 0
#endif /* ifdef ISA */

static BOOL MenuRwAddrIsDeviceNull(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_READ_WRITE_ADDR *pCtx = (MENU_CTX_READ_WRITE_ADDR *)pMenu->pCbCtx;

    return *(pCtx->phDev) == NULL;
}

static DWORD MenuRwAddrSetAddrSpace(PVOID pData)
{
    WDC_DEVICE_HANDLE hDev = *((MENU_CTX_READ_WRITE_ADDR *)pData)->phDev;
    PDWORD pdwAddrSpace = &((MENU_CTX_READ_WRITE_ADDR *)pData)->dwAddrSpace;
    DWORD dwAddrSpace;
#ifndef ISA
    DWORD dwNumAddrSpaces = WDC_DIAG_GetNumAddrSpaces(hDev);
    /* TODO ISA*/
#else /* ifdef ISA */
    DWORD dwNumAddrSpaces = PCI_ADDR_SPACES_NUM;
#endif /* ifdef ISA */
    ADDR_SPACE_INFO addrSpaceInfo;

    printf("\n");
    printf("Select an active address space:\n");
    printf("-------------------------------\n");

    for (dwAddrSpace = 0; dwAddrSpace < dwNumAddrSpaces; dwAddrSpace++)
    {
        BZERO(addrSpaceInfo);
        addrSpaceInfo.dwAddrSpace = dwAddrSpace;
        if (!WDC_DIAG_GetAddrSpaceInfo(hDev, &addrSpaceInfo))
        {
            printf("SetAddrSpace: Error - Failed to get address space "
                "information");
            return WD_WINDRIVER_STATUS_ERROR;
        }

        printf("%d. %-*s %-*s %s\n",
            dwAddrSpace + 1,
            MAX_NAME_DISPLAY, addrSpaceInfo.sName,
            MAX_TYPE - 1, addrSpaceInfo.sType,
            addrSpaceInfo.sDesc);
    }
    printf("\n");

    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&dwAddrSpace,
        "Enter option", FALSE, 1, dwNumAddrSpaces))
    {
        return WD_INVALID_PARAMETER;
    }

    dwAddrSpace--;
    if (!WDC_AddrSpaceIsActive(hDev, dwAddrSpace))
    {
        printf("You have selected an inactive address space\n");
        return WD_INVALID_PARAMETER;
    }

    *pdwAddrSpace = dwAddrSpace;

    return WD_STATUS_SUCCESS;
}

static DWORD MenuRwAddrChangeModeOptionCb(PVOID pCbCtx)
{
    MENU_CTX_READ_WRITE_ADDR *pRwAddrMenusCtx =
        (MENU_CTX_READ_WRITE_ADDR *)pCbCtx;

    WDC_DIAG_SetMode(&(pRwAddrMenusCtx->mode));

    return WD_STATUS_SUCCESS;
}

static DWORD MenuRwAddrChangeTransTypeOptionCb(PVOID pCbCtx)
{
    MENU_CTX_READ_WRITE_ADDR *pRwAddrMenusCtx =
        (MENU_CTX_READ_WRITE_ADDR *)pCbCtx;

    pRwAddrMenusCtx->fBlock = !pRwAddrMenusCtx->fBlock;

    return WD_STATUS_SUCCESS;
}

static DWORD ReadWriteAddr(MENU_CTX_READ_WRITE_ADDR
    *pRwAddrMenusCtx, WDC_DIRECTION direction)
{
    if (pRwAddrMenusCtx->fBlock)
    {
        WDC_DIAG_ReadWriteBlock(*(pRwAddrMenusCtx->phDev), direction,
            pRwAddrMenusCtx->dwAddrSpace);
    }
    else
    {
        WDC_DIAG_ReadWriteAddr(*(pRwAddrMenusCtx->phDev), direction,
            pRwAddrMenusCtx->dwAddrSpace, pRwAddrMenusCtx->mode);
    }

    return WD_STATUS_SUCCESS;
}

static DWORD MenuRwAddrReadOptionOptionCb(PVOID pCbCtx)
{
    MENU_CTX_READ_WRITE_ADDR *pRwAddrMenusCtx =
        (MENU_CTX_READ_WRITE_ADDR *)pCbCtx;

    return ReadWriteAddr(pRwAddrMenusCtx, WDC_READ);
}

static DWORD MenuRwAddrWriteOptionOptionCb(PVOID pCbCtx)
{
    MENU_CTX_READ_WRITE_ADDR *pRwAddrMenusCtx =
        (MENU_CTX_READ_WRITE_ADDR *)pCbCtx;

    return ReadWriteAddr(pRwAddrMenusCtx, WDC_WRITE);
}

static void MenuCommonRwAddrSubMenusInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_READ_WRITE_ADDR *pRwAddrMenusCtx)
{
    static DIAG_MENU_OPTION changeAddressSpaceMenu = { 0 };
    static DIAG_MENU_OPTION changeIoModeMenu = { 0 };
    static DIAG_MENU_OPTION toggleTransferTypeMenu = { 0 };
    static DIAG_MENU_OPTION readAddressSpaceMenu = { 0 };
    static DIAG_MENU_OPTION writeAddressSpaceMenu = { 0 };
    static DIAG_MENU_OPTION options[5] = { 0 };

    strcpy(changeAddressSpaceMenu.cOptionName, "Change active address space "
        "for read/write");
    changeAddressSpaceMenu.cbEntry = MenuRwAddrSetAddrSpace;

    strcpy(changeIoModeMenu.cOptionName, "Change active read/write mode");
    changeIoModeMenu.cbEntry = MenuRwAddrChangeModeOptionCb;

    strcpy(toggleTransferTypeMenu.cOptionName, "Toggle active transfer type");
    toggleTransferTypeMenu.cbEntry = MenuRwAddrChangeTransTypeOptionCb;

    strcpy(readAddressSpaceMenu.cOptionName, "Read from active address space");
    readAddressSpaceMenu.cbEntry = MenuRwAddrReadOptionOptionCb;

    strcpy(writeAddressSpaceMenu.cOptionName, "Write to active address space");
    writeAddressSpaceMenu.cbEntry = MenuRwAddrWriteOptionOptionCb;

    options[0] = changeAddressSpaceMenu;
    options[1] = changeIoModeMenu;
    options[2] = toggleTransferTypeMenu;
    options[3] = readAddressSpaceMenu;
    options[4] = writeAddressSpaceMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        pRwAddrMenusCtx, pParentMenu);
}

static DWORD MenuRwAddrExitCb(PVOID pCbCtx)
{
    ((MENU_CTX_READ_WRITE_ADDR *)pCbCtx)->dwAddrSpace =
        ACTIVE_ADDR_SPACE_NEEDS_INIT;

    return WD_STATUS_SUCCESS;
}


static DWORD MenuRwAddrCb(PVOID pCbCtx)
{
    MENU_CTX_READ_WRITE_ADDR *pRwAddrMenusCtx =
        (MENU_CTX_READ_WRITE_ADDR *)pCbCtx;
    PDWORD pdwAddrSpace = &pRwAddrMenusCtx->dwAddrSpace;
    WDC_ADDR_MODE mode = pRwAddrMenusCtx->mode;

    /* Initialize active address space */
    if (ACTIVE_ADDR_SPACE_NEEDS_INIT == *pdwAddrSpace)
    {
#ifndef ISA
        DWORD dwNumAddrSpaces = WDC_DIAG_GetNumAddrSpaces(*(
            pRwAddrMenusCtx->phDev));
#else /* ifdef ISA */
        DWORD dwNumAddrSpaces = PCI_ADDR_SPACES_NUM;
#endif /* ifdef ISA */
        /* Find the first active address space */
        for (*pdwAddrSpace = 0; *pdwAddrSpace < dwNumAddrSpaces;
            (*pdwAddrSpace)++)
        {
            if (WDC_AddrSpaceIsActive(*(pRwAddrMenusCtx->phDev),
                *pdwAddrSpace))
                break;
        }

        /* Sanity check */
        if (*pdwAddrSpace == dwNumAddrSpaces)
        {
            printf("MenuReadWriteAddr: Error - No active address spaces "
                "found\n");
            *pdwAddrSpace = ACTIVE_ADDR_SPACE_NEEDS_INIT;
            return WD_NO_RESOURCES_ON_DEVICE;
        }
    }
    printf("\nCurrent Read/Write configurations:\n");
    printf("----------------------------------\n");
    printf("Currently active address space : ");
#ifndef ISA
    printf("BAR %d\n", *pdwAddrSpace);
#else /* ifdef ISA */
    printf("AddrSpace %d\n", *pdwAddrSpace);
#endif /* ifdef ISA */
    printf("Currently active read/write mode: %s\n",
        (WDC_MODE_8 == mode) ? "8 bit" : (WDC_MODE_16 == mode) ? "16 bit" :
        (WDC_MODE_32 == mode) ? "32 bit" : "64 bit");
    printf("Currently active transfer type: %s\n",
        pRwAddrMenusCtx->fBlock ? "block transfers" :
        "non-block transfers");
    printf("\n");

    return WD_STATUS_SUCCESS;
}

DIAG_MENU_OPTION *MenuCommonRwAddrInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_READ_WRITE_ADDR *pRwAddrMenusCtx)
{
    static DIAG_MENU_OPTION rwAddrMenu = { 0 };

    strcpy(rwAddrMenu.cOptionName, "Read/write memory and I/O addresses on the"
        " device");
    strcpy(rwAddrMenu.cTitleName, "Read/write the device's memory and I/O "
        "ranges");
    rwAddrMenu.cbIsHidden = MenuRwAddrIsDeviceNull;
    rwAddrMenu.cbEntry = MenuRwAddrCb;
    rwAddrMenu.cbExit = MenuRwAddrExitCb;

    DIAG_MenuSetCtxAndParentForMenus(&rwAddrMenu, 1, pRwAddrMenusCtx,
        pParentMenu);
    MenuCommonRwAddrSubMenusInit(&rwAddrMenu, pRwAddrMenusCtx);

    return &rwAddrMenu;
}

/* -----------------------------------------------
    Interrupt handling
   ----------------------------------------------- */
static BOOL MenuInterruptsIsDeviceNull(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_INTERRUPTS *pCtx = (MENU_CTX_INTERRUPTS *)pMenu->pCbCtx;

    return *(pCtx->phDev) == NULL;
}

static BOOL MenuInterruptsAreEnabled(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_INTERRUPTS *pCtx = (MENU_CTX_INTERRUPTS *)pMenu->pCbCtx;

    return pCtx->fIntsEnabled;
}

static BOOL MenuInterruptsAreDisabled(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_INTERRUPTS *pCtx = (MENU_CTX_INTERRUPTS *)pMenu->pCbCtx;

    return !pCtx->fIntsEnabled;
}

static void MenuCommonInterruptsSubMenusInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_INTERRUPTS *pInterruptsMenusCtx,
    MENU_INTERRUPTS_CALLBACKS *pInterruptsCallbacks)
{
    static DIAG_MENU_OPTION enableInterruptsMenu = { 0 };
    static DIAG_MENU_OPTION disableInterruptsMenu = { 0 };
    static DIAG_MENU_OPTION options[2] = { 0 };

    strcpy(enableInterruptsMenu.cOptionName, "Enable interrupts");
    enableInterruptsMenu.cbIsHidden = MenuInterruptsAreEnabled;
    enableInterruptsMenu.cbEntry = pInterruptsCallbacks->interruptsEnableCb;


    strcpy(disableInterruptsMenu.cOptionName, "Disable interrupts");
    disableInterruptsMenu.cbIsHidden = MenuInterruptsAreDisabled;
    disableInterruptsMenu.cbEntry = pInterruptsCallbacks->interruptsDisableCb;

    options[0] = enableInterruptsMenu;
    options[1] = disableInterruptsMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        pInterruptsMenusCtx, pParentMenu);
}

DIAG_MENU_OPTION *MenuCommonInterruptsInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_INTERRUPTS *pInterruptsMenusCtx,
    MENU_INTERRUPTS_CALLBACKS *pInterruptsCallbacks)
{
    static DIAG_MENU_OPTION intsMenuOption = { 0 };

    strcpy(intsMenuOption.cOptionName, "Enable/disable the device's interrupts");
    strcpy(intsMenuOption.cTitleName, "Interrupts");
    intsMenuOption.cbIsHidden = MenuInterruptsIsDeviceNull;
    intsMenuOption.cbEntry = pInterruptsCallbacks->interruptsMenuEntryCb;

    DIAG_MenuSetCtxAndParentForMenus(&intsMenuOption, 1, pInterruptsMenusCtx,
        pParentMenu);
    MenuCommonInterruptsSubMenusInit(&intsMenuOption, pInterruptsMenusCtx,
        pInterruptsCallbacks);

    return &intsMenuOption;
}

/* -------------------------------------------------------
    Read/write the run-time/configuration block registers
   -------------------------------------------------------*/
static BOOL MenuRwRegsIsDeviceNullOrRegsEmpty(DIAG_MENU_OPTION *pMenu)
{
    MENU_CTX_RW_REGS *pRegsMenusCtx = (MENU_CTX_RW_REGS *)(pMenu->pCbCtx);

    return (*(pRegsMenusCtx->phDev) == NULL) ||
        (pRegsMenusCtx->dwRegsNum == 0);
}

static DWORD MenuRwRegsReadAllOptionCb(PVOID pCbCtx)
{
    MENU_CTX_RW_REGS *pRegsMenusCtx = (MENU_CTX_RW_REGS *)pCbCtx;

    WDC_DIAG_ReadRegsAll(*(pRegsMenusCtx->phDev), pRegsMenusCtx->pRegsArr,
        pRegsMenusCtx->dwRegsNum, FALSE, FALSE);

    return WD_STATUS_SUCCESS;
}

static DWORD MenuRwRegsReadOptionCb(PVOID pCbCtx)
{
    MENU_CTX_RW_REGS *pRegsMenusCtx = (MENU_CTX_RW_REGS *)pCbCtx;

    WDC_DIAG_ReadWriteReg(*(pRegsMenusCtx->phDev), pRegsMenusCtx->pRegsArr,
        pRegsMenusCtx->dwRegsNum, WDC_READ, FALSE, FALSE);

    return WD_STATUS_SUCCESS;
}

static DWORD MenuRwRegsWriteOptionCb(PVOID pCbCtx)
{
    MENU_CTX_RW_REGS *pRegsMenusCtx = (MENU_CTX_RW_REGS *)pCbCtx;

    WDC_DIAG_ReadWriteReg(*(pRegsMenusCtx->phDev), pRegsMenusCtx->pRegsArr,
        pRegsMenusCtx->dwRegsNum, WDC_WRITE, FALSE, FALSE);

    return WD_STATUS_SUCCESS;
}

static void MenuCommonRwRegsSubMenusInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_RW_REGS *pRegsMenusCtx)
{
    static DIAG_MENU_OPTION readAllRegistersMenu = { 0 };
    static DIAG_MENU_OPTION readRegisterMenu = { 0 };
    static DIAG_MENU_OPTION writeRegisterMenu = { 0 };
    static DIAG_MENU_OPTION options[3] = { 0 };

    strcpy(readAllRegistersMenu.cOptionName, "Read all registers(see list "
        "above)");
    readAllRegistersMenu.cbEntry = MenuRwRegsReadAllOptionCb;

    strcpy(readRegisterMenu.cOptionName, "Read from a register");
    readRegisterMenu.cbEntry = MenuRwRegsReadOptionCb;

    strcpy(writeRegisterMenu.cOptionName, "Write to a register");
    writeRegisterMenu.cbEntry = MenuRwRegsWriteOptionCb;

    options[0] = readAllRegistersMenu;
    options[1] = readRegisterMenu;
    options[2] = writeRegisterMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        pRegsMenusCtx, pParentMenu);
}

static DWORD MenuRwRegsCb(PVOID pCbCtx)
{
    MENU_CTX_RW_REGS *pRegsMenusCtx = (MENU_CTX_RW_REGS *)pCbCtx;

    WDC_DIAG_RegsInfoPrint(*(pRegsMenusCtx->phDev),
        pRegsMenusCtx->pRegsArr, pRegsMenusCtx->dwRegsNum,
        WDC_DIAG_REG_PRINT_ALL, FALSE);

    return WD_STATUS_SUCCESS;
}

DIAG_MENU_OPTION *MenuCommonRwRegsInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_RW_REGS *pRegsMenusCtx)
{
    static DIAG_MENU_OPTION rwRegsMenuOption = { 0 };

    rwRegsMenuOption.cbEntry = MenuRwRegsCb;
    rwRegsMenuOption.cbIsHidden = MenuRwRegsIsDeviceNullOrRegsEmpty;

    sprintf(rwRegsMenuOption.cOptionName,
        "Read/write the %s registers",
        pRegsMenusCtx->fIsConfig ? "configuration block" : "run-time");

    sprintf(rwRegsMenuOption.cTitleName,
        "%s %s registers",
        pRegsMenusCtx->sModuleName,
        pRegsMenusCtx->fIsConfig ? "configuration block" : "run-time");

    DIAG_MenuSetCtxAndParentForMenus(&rwRegsMenuOption, 1,
        pRegsMenusCtx, pParentMenu);
    MenuCommonRwRegsSubMenusInit(&rwRegsMenuOption, pRegsMenusCtx);

    return &rwRegsMenuOption;
}

#endif
