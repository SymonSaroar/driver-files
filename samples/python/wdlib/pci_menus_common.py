''' Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com '''

import sys, os
sys.path.append(os.path.dirname(os.path.abspath(__file__)) + "/..")
from wdlib.diag_lib import *
from wdlib.wdc_diag_lib import *

def MenuCommonIsDeviceNull(pMenu):
    return (pMenu.pCbCtx.hDevs[0] == None)

# -----------------------------------------------
#   Scan Bus
#   -----------------------------------------------
def MenuScanBusCb(pCbCtx):
    WDC_DIAG_PciDevicesInfoPrintAll(False)
    return WD_STATUS_SUCCESS

def MenuCommonScanBusInit(pParentMenu):
    menuScanBusRoot = DIAG_MENU_OPTION(
        cbEntry = MenuScanBusCb,
        cOptionName = "Scan PCI bus"
    )

    DIAG_MenuSetCtxAndParentForMenus([menuScanBusRoot], None, pParentMenu)

# -----------------------------------------------
#    Read/write the configuration space
#   -----------------------------------------------
class MENU_CTX_CFG(object):
    def __init__(self, hDevs, pCfgRegs = [], dwCfgRegsNum = 0, pCfgExpRegs = [],
            dwCfgExpRegsNum = 0):
        self.hDevs = hDevs
        self.pCfgRegs = pCfgRegs
        self.dwCfgRegsNum = dwCfgRegsNum
        self.pCfgExpRegs = pCfgExpRegs
        self.dwCfgExpRegsNum = dwCfgExpRegsNum

def MenuCfgIsDeviceNotExpress(pMenu):
    return wdapi.WDC_PciGetExpressGen(pMenu.pCbCtx.hDevs[0]) == 0

def MenuCfgReadOffsetOptionCb(pCbCtx):
    WDC_DIAG_ReadWriteBlock(pCbCtx.hDevs[0], WDC_READ, WDC_AD_CFG_SPACE)
    return WD_STATUS_SUCCESS

def MenuCfgWriteOffsetOptionCb(pCbCtx):
    WDC_DIAG_ReadWriteBlock(pCbCtx.hDevs[0], WDC_WRITE, WDC_AD_CFG_SPACE)
    return WD_STATUS_SUCCESS

def MenuCfgReadAllOptionCb(pCbCtx):
    WDC_DIAG_ReadRegsAll(pCbCtx.hDevs[0], pCbCtx.pCfgRegs,
        pCbCtx.dwCfgRegsNum, True, False)

    if wdapi.WDC_PciGetExpressGen(pCbCtx.hDevs[0]) != 0:
        WDC_DIAG_ReadRegsAll(pCbCtx.hDevs[0], pCbCtx.pCfgExpRegs,
            pCbCtx.dwCfgExpRegsNum, True, True)

def MenuCfgReadNamedRegOptionCb(pCbCtx):
    WDC_DIAG_ReadWriteReg(pCbCtx.hDevs[0], pCbCtx.pCfgRegs,
        pCbCtx.dwCfgRegsNum, WDC_READ, True, False)
    return WD_STATUS_SUCCESS

def MenuCfgWriteNamedRegOptionCb(pCbCtx):
    WDC_DIAG_ReadWriteReg(pCbCtx.hDevs[0], pCbCtx.pCfgRegs,
        pCbCtx.dwCfgRegsNum, WDC_WRITE, True, False)
    return WD_STATUS_SUCCESS

def MenuCfgReadNamedExpRegOptionCb(pCbCtx):
    WDC_DIAG_ReadWriteReg(pCbCtx.hDevs[0], pCbCtx.pCfgExpRegs,
        pCbCtx.dwCfgExpRegsNum, WDC_READ, True, True)
    return WD_STATUS_SUCCESS

def MenuCfgWriteNamedExpRegOptionCb(pCbCtx):
    WDC_DIAG_ReadWriteReg(pCbCtx.hDevs[0], pCbCtx.pCfgExpRegs,
        pCbCtx.dwCfgExpRegsNum, WDC_WRITE, True, True)
    return WD_STATUS_SUCCESS

def MenuCfgScanCapsOptionCb(pCbCtx):
    WDC_DIAG_ScanPCICapabilities(pCbCtx.hDevs[0],
        wdapi.WDC_PciGetExpressGen(pCbCtx.hDevs[0]) != 0)
    return WD_STATUS_SUCCESS

def MenuCfgInitSubMenus(pParentMenu, cfgMenuCtx):
    options = [
        DIAG_MENU_OPTION (
            cOptionName = "Read from an offset",
            cbEntry = MenuCfgReadOffsetOptionCb ),

        DIAG_MENU_OPTION (
            cOptionName = "Write to an offset",
            cbEntry = MenuCfgWriteOffsetOptionCb ),

        DIAG_MENU_OPTION (
            cOptionName = "Read all configuration registers defined for"
                "the device (see list above)",
            cbEntry = MenuCfgReadAllOptionCb ),

        DIAG_MENU_OPTION (
            cOptionName = "Read from a named register",
            cbEntry = MenuCfgReadNamedRegOptionCb ),

        DIAG_MENU_OPTION (
            cOptionName = "Write to a named  register",
            cbEntry = MenuCfgWriteNamedRegOptionCb ),

        DIAG_MENU_OPTION (
            cOptionName = "Read from a named PCI Express register",
            cbEntry = MenuCfgReadNamedExpRegOptionCb,
            cbIsHidden = MenuCfgIsDeviceNotExpress ),

        DIAG_MENU_OPTION (
            cOptionName = "Write to a named PCI Express register",
            cbEntry = MenuCfgWriteNamedExpRegOptionCb,
            cbIsHidden = MenuCfgIsDeviceNotExpress ),

        DIAG_MENU_OPTION (
            cOptionName = "Scan PCI/PCIe capabilities",
            cbEntry = MenuCfgScanCapsOptionCb )
    ]

    DIAG_MenuSetCtxAndParentForMenus(options, cfgMenuCtx, pParentMenu)


def MenuCfgCb(pCbCtx):
    # Display predefined registers information
    print("\nConfiguration registers:")
    print("------------------------")
    WDC_DIAG_RegsInfoPrint(pCbCtx.hDevs[0], pCbCtx.pCfgRegs,
        pCbCtx.dwCfgRegsNum,
        WDC_DIAG_REG_PRINT_ALL & ~WDC_DIAG_REG_PRINT_ADDR_SPACE, False)

    if (wdapi.WDC_PciGetExpressGen(pCbCtx.hDevs[0]) != 0):
        WDC_DIAG_RegsInfoPrint(pCbCtx.hDevs[0], pCbCtx.pCfgExpRegs,
            pCbCtx.dwCfgExpRegsNum,
            WDC_DIAG_REG_PRINT_ALL & ~WDC_DIAG_REG_PRINT_ADDR_SPACE, True)

    return WD_STATUS_SUCCESS

def MenuCommonCfgInit(pParentMenu, cfgMenuCtx):
    cfgMenuRoot = DIAG_MENU_OPTION(
        cOptionName = "Read/write the PCI configuration space",
        cTitleName = "Read/write the device's configuration space",
        cbEntry = MenuCfgCb,
        cbIsHidden = MenuCommonIsDeviceNull
    )

    MenuCfgInitSubMenus(cfgMenuRoot, cfgMenuCtx)
    DIAG_MenuSetCtxAndParentForMenus([cfgMenuRoot], cfgMenuCtx, pParentMenu)

    return cfgMenuRoot

# ----------------------------------------------------
#    Plug-and-play and power management events handling
#   ----------------------------------------------------
class MENU_EVENTS_CALLBACKS(object):
    eventsEnableCb = None
    eventsDisableCb = None
    eventsMenuEntryCb = None
    eventsMenuExitCb = None

class MENU_CTX_EVENTS(object):
    def __init__(self, hDevs, DiagEventHandler, fIsRegistered = False):
        self.hDevs = hDevs
        self.fIsRegistered = fIsRegistered
        self.DiagEventHandler = DiagEventHandler
        self.fIsRegistered = fIsRegistered

def MenuEventsIsRegistered(pMenu):
    return pMenu.pCbCtx.fIsRegistered

def MenuEventsIsUnregistered(pMenu):
    return not pMenu.pCbCtx.fIsRegistered

def MenuEventsSubMenusInit(pParentMenu, eventsMenuCtx,
        eventsMenuCallbacks):
    options = [
        DIAG_MENU_OPTION (
            cOptionName = "Register Events",
            cbEntry = eventsMenuCallbacks.eventsEnableCb,
            cbIsHidden =  MenuEventsIsRegistered ),

        DIAG_MENU_OPTION (
            cOptionName = "Unregister Events",
            cbEntry = eventsMenuCallbacks.eventsDisableCb,
            cbIsHidden =  MenuEventsIsUnregistered )
    ]

    DIAG_MenuSetCtxAndParentForMenus(options, eventsMenuCtx,
        pParentMenu)


def MenuCommonEventsInit(pParentMenu, eventsMenuCtx, eventsMenuCallbacks):
    eventsMenuRoot = DIAG_MENU_OPTION(
        cOptionName = "Register/unregister plug-and-play and power "
              "management events",
        cTitleName = "Plug-and-play and power management events",
        cbIsHidden = MenuCommonIsDeviceNull,
        cbEntry = eventsMenuCallbacks.eventsMenuEntryCb,
        cbExit = eventsMenuCallbacks.eventsMenuExitCb
    )

    MenuEventsSubMenusInit(eventsMenuRoot, eventsMenuCtx,
        eventsMenuCallbacks)
    DIAG_MenuSetCtxAndParentForMenus([eventsMenuRoot], eventsMenuCtx,
        pParentMenu)

    return eventsMenuRoot

# -----------------------------------------------
#    Read/write memory and I/O addresses
# -----------------------------------------------

#ifdef ISA
#PCI_ADDR_SPACES_NUM = 0
#endif #ifdef ISA

ACTIVE_ADDR_SPACE_NEEDS_INIT = 0xFF

class MENU_CTX_RW_ADDR(object):
    def __init__(self, hDevs, fBlock, mode, dwAddrSpace):
        self.hDevs = hDevs
        self.fBlock = fBlock
        self.mode = mode
        self.dwAddrSpace = dwAddrSpace

def MenuRwAddrSetAddrSpace(pCbCtx):
#ifndef ISA
    dwNumAddrSpaces = WDC_DIAG_GetNumAddrSpaces(pCbCtx.hDevs[0])
#else # ifdef ISA
#   dwNumAddrSpaces = PCI_ADDR_SPACES_NUM
#endif # ifdef ISA
    addrSpaceInfo = WDC_DIAG_ADDR_SPACE_INFO()

    print("\n")
    print("Select an active address space:")
    print("-------------------------------")

    for dwAddrSpace in range(dwNumAddrSpaces):
        addrSpaceInfo.dwAddrSpace = dwAddrSpace
        if not WDC_DIAG_GetAddrSpaceInfo(pCbCtx.hDevs[0], addrSpaceInfo):
            WDC_DIAG_ERR("SetAddrSpace: Error - Failed to get address space "
                "information.")
            return WD_WINDRIVER_STATUS_ERROR

        print("%ld. %-*s %-*s %s" % (dwAddrSpace + 1, MAX_NAME_DISPLAY,
            addrSpaceInfo.sName, MAX_TYPE - 1, addrSpaceInfo.sType,
            addrSpaceInfo.sDesc))

    (dwAddrSpace, dwStatus) = DIAG_InputNum("Enter option", False,
        sizeof(DWORD), 1, dwNumAddrSpaces)
    if DIAG_INPUT_SUCCESS != dwStatus:
        return WD_INVALID_PARAMETER

    dwAddrSpace.value -= 1
    if not wdapi.WDC_AddrSpaceIsActive(pCbCtx.hDevs[0], dwAddrSpace):
        print("You have selected an inactive address space")
        return WD_INVALID_PARAMETER

    pCbCtx.dwAddrSpace = dwAddrSpace.value
    return WD_STATUS_SUCCESS

def MenuRwAddrChangeModeOptionCb(pCbCtx):
    (mode, dwStatus) = WDC_DIAG_SetMode()
    if dwStatus == DIAG_INPUT_SUCCESS:
        pCbCtx.mode = mode

    return dwStatus

def MenuRwAddrChangeTransTypeOptionCb(pCbCtx):
    pCbCtx.fBlock = not pCbCtx.fBlock
    return WD_STATUS_SUCCESS

def ReadWriteAddr(rwAddrMenusCtx, direction):
    if rwAddrMenusCtx.fBlock:
        WDC_DIAG_ReadWriteBlock(rwAddrMenusCtx.hDevs[0], direction,
            rwAddrMenusCtx.dwAddrSpace)
    else:
        WDC_DIAG_ReadWriteAddr(rwAddrMenusCtx.hDevs[0], direction,
            rwAddrMenusCtx.dwAddrSpace, rwAddrMenusCtx.mode)

    return WD_STATUS_SUCCESS

def MenuRwAddrReadOptionOptionCb(pCbCtx):
    return ReadWriteAddr(pCbCtx, WDC_READ)

def MenuRwAddrWriteOptionOptionCb(pCbCtx):
    return ReadWriteAddr(pCbCtx, WDC_WRITE)

def MenuRwAddrSubMenusInit(pParentMenu, rwAddrMenusCtx):
    options = [
        DIAG_MENU_OPTION (
            cOptionName = "Change active address space for read/write",
            cbEntry = MenuRwAddrSetAddrSpace ),

        DIAG_MENU_OPTION (
            cOptionName = "Change active read/write mode",
            cbEntry = MenuRwAddrChangeModeOptionCb ),

        DIAG_MENU_OPTION (
            cOptionName = "Toggle active transfer type",
            cbEntry = MenuRwAddrChangeTransTypeOptionCb ),

        DIAG_MENU_OPTION (
            cOptionName = "Read from active address space",
            cbEntry = MenuRwAddrReadOptionOptionCb ),

        DIAG_MENU_OPTION (
            cOptionName = "Write to active address space",
            cbEntry = MenuRwAddrWriteOptionOptionCb )
    ]

    DIAG_MenuSetCtxAndParentForMenus(options, rwAddrMenusCtx,
        pParentMenu)

def MenuRwAddrCb(pCbCtx):
    if ACTIVE_ADDR_SPACE_NEEDS_INIT == pCbCtx.dwAddrSpace:
#ifndef ISA
        dwNumAddrSpaces = WDC_DIAG_GetNumAddrSpaces(pCbCtx.hDevs[0])
#else # ifdef ISA
#       dwNumAddrSpaces = PCI_ADDR_SPACES_NUM
#endif # ifdef ISA
# Find the first active address space
        for pCbCtx.dwAddrSpace in range(dwNumAddrSpaces):
            if wdapi.WDC_AddrSpaceIsActive(pCbCtx.hDevs[0],
                    pCbCtx.dwAddrSpace):
                break

        # Sanity check
        if pCbCtx.dwAddrSpace == dwNumAddrSpaces:
            # TODO check print
            WDC_DIAG_ERR("MenuReadWriteAddr: Error - No active address spaces "
                "found\n")
            pCbCtx.dwAddrSpace = ACTIVE_ADDR_SPACE_NEEDS_INIT
            return WD_NO_RESOURCES_ON_DEVICE

    print("\nCurrent Read/Write configurations:")
    print("----------------------------------")
    print("Currently active address space : ")
#ifndef ISA
    print("BAR %ld" % pCbCtx.dwAddrSpace)
#else /* ifdef ISA */
#   print("AddrSpace %ld" % pCbCtx.dwAddrSpace)
#endif /* ifdef ISA */
    print("Currently active read/write mode: %s" % \
            ("8 bit" if WDC_MODE_8 == pCbCtx.mode \
            else "16 bit" if WDC_MODE_16 == pCbCtx.mode \
            else "32 bit" if WDC_MODE_32 == pCbCtx.mode \
            else "64 bit"))
    print("Currently active transfer type: %s" % \
        ("block transfers" if pCbCtx.fBlock else "non-block transfers"))

    return WD_STATUS_SUCCESS

def MenuRwAddrExitCb(pCbCtx):
    pCbCtx.dwAddrSpace = ACTIVE_ADDR_SPACE_NEEDS_INIT

    return WD_STATUS_SUCCESS

def MenuCommonRwAddrInit(pParentMenu, rwAddrMenusCtx):
    rwAddrMenuRoot = DIAG_MENU_OPTION(
        cOptionName = "Read/write memory and I/O addresses on the device",
        cTitleName = "Read/write the device's memory and I/O ranges",
        cbIsHidden = MenuCommonIsDeviceNull,
        cbEntry = MenuRwAddrCb,
        cbExit = MenuRwAddrExitCb
    )

    MenuRwAddrSubMenusInit(rwAddrMenuRoot, rwAddrMenusCtx)
    DIAG_MenuSetCtxAndParentForMenus([rwAddrMenuRoot], rwAddrMenusCtx,
        pParentMenu)

    return rwAddrMenuRoot

# -----------------------------------------------
#   Interrupt handling
# -----------------------------------------------
class MENU_INTERRUPTS_CALLBACKS(object):
    interruptsEnableCb = None
    interruptsDisableCb = None
    interruptsMenuEntryCb = None
    interruptsMenuExitCb = None

class MENU_CTX_INTERRUPTS(object):
    def __init__(self, hDevs, funcIntHandler, fIntsEnabled = False,
        pData = None):
        self.hDevs = hDevs
        self.funcIntHandler = funcIntHandler
        self.fIntsEnabled = fIntsEnabled
        self.pData = pData  # Optional additional data. If needed, pass to
                            # xxx_IntEnable in the xxx_lib.h file

def MenuInterruptsAreEnabled(pMenu):
    return pMenu.pCbCtx.fIntsEnabled

def MenuInterruptsAreDisabled(pMenu):
    return not pMenu.pCbCtx.fIntsEnabled

def MenuInterruptsSubMenusInit(pParentMenu, interruptsMenusCtx,
        interrupsMenuCallbacks):
    options = [
        DIAG_MENU_OPTION (
            cOptionName = "Enable Interrupts",
            cbEntry = interrupsMenuCallbacks.interruptsEnableCb,
            cbIsHidden =  MenuInterruptsAreEnabled ),

        DIAG_MENU_OPTION (
            cOptionName = "Disable Interrupts",
            cbEntry = interrupsMenuCallbacks.interruptsDisableCb,
            cbIsHidden =  MenuInterruptsAreDisabled )
    ]

    DIAG_MenuSetCtxAndParentForMenus(options, interruptsMenusCtx,
        pParentMenu)


def MenuCommonInterruptsInit(pParentMenu, interruptsMenusCtx,
    interrupsMenuCallbacks):
    interruptsMenuRoot = DIAG_MENU_OPTION(
        cOptionName = "Enable/disable the device's interrupts",
        cTitleName = "Interrupts",
        cbIsHidden = MenuCommonIsDeviceNull,
        cbEntry = interrupsMenuCallbacks.interruptsMenuEntryCb,
        cbExit = interrupsMenuCallbacks.interruptsMenuExitCb
    )

    MenuInterruptsSubMenusInit(interruptsMenuRoot, interruptsMenusCtx,
        interrupsMenuCallbacks)
    DIAG_MenuSetCtxAndParentForMenus([interruptsMenuRoot], interruptsMenusCtx,
        pParentMenu)

# -----------------------------------------------
#   SRIOV handling
# -----------------------------------------------
#ifdef LINUX

class MENU_SRIOV_CALLBACKS(object):
    sriovEnableCb = None
    sriovDisableCb = None
    sriovMenuEntryCb = None
    sriovMenuExitCb = None

class MENU_CTX_SRIOV(object):
    def __init__(self, hDevs):
        self.hDevs = hDevs
        self.dwNumVFs = DWORD(0)

def MenuSriovIsEnabled(pMenu):
    return pMenu.pCbCtx.dwNumVFs.value > 0

def MenuSriovIsDisabled(pMenu):
    return pMenu.pCbCtx.dwNumVFs.value == 0

def MenuSriovSubMenusInit(pParentMenu, sriovMenusCtx, sriovMenuCallbacks):
    options = [
        DIAG_MENU_OPTION (
            cOptionName = "Enable SRIOV",
            cbEntry = sriovMenuCallbacks.sriovEnableCb,
            cbIsHidden =  MenuSriovIsEnabled ),

        DIAG_MENU_OPTION (
            cOptionName = "Disable SRIOV",
            cbEntry = sriovMenuCallbacks.sriovDisableCb,
            cbIsHidden =  MenuSriovIsDisabled )
    ]

    DIAG_MenuSetCtxAndParentForMenus(options, sriovMenusCtx, pParentMenu)

def MenuCommonSriovInit(pParentMenu, sriovMenusCtx, sriovMenuCallbacks):
    sriovMenuRoot = DIAG_MENU_OPTION (
        cOptionName = "Enable/disable SR-IOV capability",
        cbEntry = sriovMenuCallbacks.sriovMenuEntryCb,
        cbExit = sriovMenuCallbacks.sriovMenuExitCb,
        cbIsHidden =  MenuCommonIsDeviceNull )

    MenuSriovSubMenusInit(sriovMenuRoot, sriovMenusCtx, sriovMenuCallbacks)
    DIAG_MenuSetCtxAndParentForMenus([sriovMenuRoot], sriovMenusCtx,
        pParentMenu)

#endif #ifdef LINUX

# -----------------------------------------------
#    Read/write the run-time registers
#   -----------------------------------------------
class MENU_CTX_RW_REGS(object):
    def __init__(self, hDevs, pRegsArr, dwRegsNum, sModuleName,
            fIsConfig = False):
        self.hDevs = hDevs
        self.pRegsArr = pRegsArr
        self.dwRegsNum = dwRegsNum
        self.sModuleName = sModuleName
        self.fIsConfig = fIsConfig

def MenuRwRegsIsDeviceNullOrRegsEmpty(pMenu):
    return (pMenu.pCbCtx.hDevs[0].value == 0 or
        pMenu.pCbCtx.dwRegsNum == 0)

def MenuRwRegsReadAllOptionCb(pCbCtx):
    WDC_DIAG_ReadRegsAll(pCbCtx.hDevs[0], pCbCtx.pRegsArr,
        pCbCtx.dwRegsNum, False, False)

    return WD_STATUS_SUCCESS

def MenuRwRegsReadOptionCb(pCbCtx):
    WDC_DIAG_ReadWriteReg(pCbCtx.hDevs[0], pCbCtx.pRegsArr, pCbCtx.dwRegsNum,
        WDC_READ, False, False)

    return WD_STATUS_SUCCESS

def MenuRwRegsWriteOptionCb(pCbCtx):
    WDC_DIAG_ReadWriteReg(pCbCtx.hDevs[0], pCbCtx.pRegsArr, pCbCtx.dwRegsNum,
        WDC_WRITE, False, False)

    return WD_STATUS_SUCCESS

def MenuRwRegsSubMenusInit(pParentMenu, regsMenusCtx):
    options = [
        DIAG_MENU_OPTION (
            cOptionName = "Read all registers(see list above)",
            cbEntry = MenuRwRegsReadAllOptionCb),

        DIAG_MENU_OPTION (
            cOptionName = "Read from a register",
            cbEntry = MenuRwRegsReadOptionCb ),

        DIAG_MENU_OPTION (
            cOptionName = "Write to a register",
            cbEntry = MenuRwRegsWriteOptionCb )
    ]

    DIAG_MenuSetCtxAndParentForMenus(options, regsMenusCtx, pParentMenu)

def MenuRwRegsCb(pCbCtx):
    WDC_DIAG_RegsInfoPrint(pCbCtx.hDevs[0], pCbCtx.pRegsArr, pCbCtx.dwRegsNum,
        WDC_DIAG_REG_PRINT_ALL, False)

    return WD_STATUS_SUCCESS

def MenuCommonRwRegsInit(pParentMenu, regsMenusCtx):
    rwRegsMenuRoot = DIAG_MENU_OPTION (
        cbEntry = MenuRwRegsCb,
        cbIsHidden =  MenuRwRegsIsDeviceNullOrRegsEmpty )

    rwRegsMenuRoot.cOptionName = ("Read/write the %s registers" % (
        "configuration block" if regsMenusCtx.fIsConfig else "run-time"))

    rwRegsMenuRoot.cTitleName = ("%s %s registers" % (
        regsMenusCtx.sModuleName,
        "configuration block" if regsMenusCtx.fIsConfig else "run-time"))

    MenuRwRegsSubMenusInit(rwRegsMenuRoot, regsMenusCtx)
    DIAG_MenuSetCtxAndParentForMenus([rwRegsMenuRoot], regsMenusCtx,
        pParentMenu)

    return rwRegsMenuRoot

