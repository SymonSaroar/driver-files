''' Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com '''

import sys, os
sys.path.append(os.path.dirname(os.path.abspath(__file__)) + "/..")
from pci_lib import *
from wdlib.diag_lib import *
from wdlib.wdc_diag_lib import *
from wdlib.pci_menus_common import *
#ifndef ISA
from wdlib.wds_diag_lib import *
#endif # ifndef ISA

gpDevCtx = PCI_DEV_CTX()

#ifndef ISA
# --------------------------------------------------
#    PCI configuration registers information
# --------------------------------------------------
# Configuration registers information array

gPCI_CfgRegs = [ \
    WDC_REG(WDC_AD_CFG_SPACE, PCI_VID, WDC_SIZE_16, WDC_READ_WRITE, "VID",
        "Vendor ID"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_DID, WDC_SIZE_16, WDC_READ_WRITE, "DID",
        "Device ID"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_CR, WDC_SIZE_16, WDC_READ_WRITE, "CMD",
        "Command"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_SR, WDC_SIZE_16, WDC_READ_WRITE, "STS",
        "Status"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_REV, WDC_SIZE_32, WDC_READ_WRITE, "RID_CLCD",
        "Revision ID &\nClass Code"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_CCSC, WDC_SIZE_8, WDC_READ_WRITE, "SCC",
        "Sub Class Code"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_CCBC, WDC_SIZE_8, WDC_READ_WRITE, "BCC",
        "Base Class Code"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_CLSR, WDC_SIZE_8, WDC_READ_WRITE, "CALN",
        "Cache Line Size"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_LTR, WDC_SIZE_8, WDC_READ_WRITE, "LAT",
        "Latency Timer"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_HDR, WDC_SIZE_8, WDC_READ_WRITE, "HDR",
        "Header Type"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_BISTR, WDC_SIZE_8, WDC_READ_WRITE, "BIST",
        "Built-in Self Test"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_BAR0, WDC_SIZE_32, WDC_READ_WRITE, "BADDR0",
        "Base Address 0"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_BAR1, WDC_SIZE_32, WDC_READ_WRITE, "BADDR1",
        "Base Address 1"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_BAR2, WDC_SIZE_32, WDC_READ_WRITE, "BADDR2",
        "Base Address 2"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_BAR3, WDC_SIZE_32, WDC_READ_WRITE, "BADDR3",
        "Base Address 3"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_BAR4, WDC_SIZE_32, WDC_READ_WRITE, "BADDR4",
        "Base Address 4"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_BAR5, WDC_SIZE_32, WDC_READ_WRITE, "BADDR5",
        "Base Address 5"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_CIS, WDC_SIZE_32, WDC_READ_WRITE, "CIS",
        "CardBus CIS\npointer"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_SVID, WDC_SIZE_16, WDC_READ_WRITE, "SVID",
        "Sub-system\nVendor ID"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_SDID, WDC_SIZE_16, WDC_READ_WRITE, "SDID",
        "Sub-system\nDevice ID"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_EROM, WDC_SIZE_32, WDC_READ_WRITE, "EROM",
        "Expansion ROM\nBase Address"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_CAP, WDC_SIZE_8, WDC_READ_WRITE, "NEW_CAP",
        "New Capabilities\nPointer"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_ILR, WDC_SIZE_32, WDC_READ_WRITE, "INTLN",
        "Interrupt Line"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_IPR, WDC_SIZE_32, WDC_READ_WRITE, "INTPIN",
        "Interrupt Pin"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_MGR, WDC_SIZE_32, WDC_READ_WRITE, "MINGNT",
        "Minimum Required\nBurst Period"),
    WDC_REG(WDC_AD_CFG_SPACE, PCI_MLR, WDC_SIZE_32, WDC_READ_WRITE, "MAXLAT",
        "Maximum Latency")]

gPCI_ext_CfgRegs =\
    [WDC_REG(WDC_AD_CFG_SPACE, PCIE_CAP_ID, WDC_SIZE_8, WDC_READ_WRITE,
        "PCIE_CAP_ID", "PCI Express\nCapability ID"),
    WDC_REG(WDC_AD_CFG_SPACE, NEXT_CAP_PTR, WDC_SIZE_8, WDC_READ_WRITE,
        "NEXT_CAP_PTR", "Next Capabiliy Pointer"),
    WDC_REG(WDC_AD_CFG_SPACE, CAP_REG, WDC_SIZE_16, WDC_READ_WRITE, "CAP_REG",
        "Capabilities Register"),
    WDC_REG(WDC_AD_CFG_SPACE, DEV_CAPS, WDC_SIZE_32, WDC_READ_WRITE, "DEV_CAPS",
        "Device Capabilities"),
    WDC_REG(WDC_AD_CFG_SPACE, DEV_CTL, WDC_SIZE_16, WDC_READ_WRITE, "DEV_CTL",
        "Device Control"),
    WDC_REG(WDC_AD_CFG_SPACE, DEV_STS, WDC_SIZE_16, WDC_READ_WRITE, "DEV_STS",
        "Device Status"),
    WDC_REG(WDC_AD_CFG_SPACE, LNK_CAPS, WDC_SIZE_32, WDC_READ_WRITE, "LNK_CAPS",
        "Link Capabilities"),
    WDC_REG(WDC_AD_CFG_SPACE, LNK_CTL, WDC_SIZE_16, WDC_READ_WRITE, "LNK_CTL",
        "Link Control"),
    WDC_REG(WDC_AD_CFG_SPACE, LNK_STS, WDC_SIZE_16, WDC_READ_WRITE, "LNK_STS",
        "Link Status"),
    WDC_REG(WDC_AD_CFG_SPACE, SLOT_CAPS, WDC_SIZE_32, WDC_READ_WRITE,
        "SLOT_CAPS", "Slot Capabilities"),
    WDC_REG(WDC_AD_CFG_SPACE, SLOT_CTL, WDC_SIZE_16, WDC_READ_WRITE, "SLOT_CTL",
        "Slot Control"),
    WDC_REG(WDC_AD_CFG_SPACE, SLOT_STS, WDC_SIZE_16, WDC_READ_WRITE, "SLOT_STS",
        "Slot Status"),
    WDC_REG(WDC_AD_CFG_SPACE, ROOT_CAPS, WDC_SIZE_16, WDC_READ_WRITE,
        "ROOT_CAPS", "Root Capabilities"),
    WDC_REG(WDC_AD_CFG_SPACE, ROOT_CTL, WDC_SIZE_16, WDC_READ_WRITE, "ROOT_CTL",
        "Root Control"),
    WDC_REG(WDC_AD_CFG_SPACE, ROOT_STS, WDC_SIZE_32, WDC_READ_WRITE, "ROOT_STS",
        "Root Status"),
    WDC_REG(WDC_AD_CFG_SPACE, DEV_CAPS2, WDC_SIZE_32, WDC_READ_WRITE,
        "DEV_CAPS2", "Device Capabilities 2"),
    WDC_REG(WDC_AD_CFG_SPACE, DEV_CTL2, WDC_SIZE_16, WDC_READ_WRITE, "DEV_CTL2",
        "Device Control 2"),
    WDC_REG(WDC_AD_CFG_SPACE, DEV_STS2, WDC_SIZE_16, WDC_READ_WRITE, "DEV_STS2",
        "Device Status 2"),
    WDC_REG(WDC_AD_CFG_SPACE, LNK_CAPS2, WDC_SIZE_32, WDC_READ_WRITE,
        "LNK_CAPS2", "Link Capabilities 2"),
    WDC_REG(WDC_AD_CFG_SPACE, LNK_CTL2, WDC_SIZE_16, WDC_READ_WRITE, "LNK_CTL2",
        "Link Control 2"),
    WDC_REG(WDC_AD_CFG_SPACE, LNK_STS2, WDC_SIZE_16, WDC_READ_WRITE, "LNK_STS2",
        "Link Status 2"),
    WDC_REG(WDC_AD_CFG_SPACE, SLOT_CAPS2, WDC_SIZE_32, WDC_READ_WRITE,
        "SLOT_CAPS2", "Slot Capabilities 2"),
    WDC_REG(WDC_AD_CFG_SPACE, SLOT_CTL2, WDC_SIZE_16, WDC_READ_WRITE,
        "SLOT_CTL2", "Slot Control 2"),
    WDC_REG(WDC_AD_CFG_SPACE, SLOT_STS2, WDC_SIZE_16, WDC_READ_WRITE,
        "SLOT_STS2", "Slot Status 2")]

PCI_CFG_EXT_REGS_NUM = len(gPCI_ext_CfgRegs)
PCI_CFG_REGS_NUM = len(gPCI_CfgRegs)

# TODO: For read-only or write-only registers, change the direction field of
#        the relevant registers in gPCI_CfgRegs to WDC_READ or WDC_WRITE.
# NOTE: You can define additional configuration registers in gPCI_CfgRegs.

#----------------------------------------------------
# SRIOV
#----------------------------------------------------

fIsEnabledSRIOV = False

#endif # ifndef ISA
# 

#************************************************************
#  Static functions prototypes
# ************************************************************
g_hDevs = [None]

def PCI_Init():
    global g_hDevs
    # Initialize the PCI library
    dwStatus = PCI_LibInit()
    if WD_STATUS_SUCCESS != dwStatus:
        WDC_DIAG_ERR("pci_diag: Failed to initialize the PCI library: 0x%lX - "
            "%s" % (dwStatus, Stat2Str(dwStatus)))
        return dwStatus

#ifndef ISA
    wdapi_va.PrintDbgMessage(D_ERROR, S_PCI, "WinDriver user mode version %s" %
        WD_VERSION_STR)

    # Find and open a PCI device (by default ID)
    if PCI_DEFAULT_VENDOR_ID:
        g_hDevs[0] = PCI_DeviceOpen(DWORD(PCI_DEFAULT_VENDOR_ID),
            DWORD(PCI_DEFAULT_DEVICE_ID))

#else # ifdef ISA
#    # Open a handle to the device
#    g_hDevs[0] = PCI_DeviceOpen(())
#    if not g_hDevs[0]:
#        PCI_ERR("pci_diag: Failed opening a handle to the device")
#        return None;
#endif # ifdef ISA

    # 
    # Get Kernel PlugIn driver version
    if g_hDevs[0] and WDC_IS_KP(g_hDevs[0]):
        CheckKPDriverVer(g_hDevs[0])
    # 

    return dwStatus

def main():
    global g_hDevs
    global fIsEnabledSRIOV

    print("PCI diagnostic utility.")
    print("Application accesses hardware using " + WD_PROD_NAME)
    # 
    print("and a Kernel PlugIn driver (%s)." % KP_PCI_DRIVER_NAME)
    # 
    if sys.platform == "win32":
        print("NOTICE: Some functionalities may not work without having\n"
            "        installed an INF file for the requested device. You can\n"
            "        generate an INF file using the DriverWizard.")


    dwStatus = PCI_Init()
    if (dwStatus):
        return dwStatus

    # Display main diagnostics menu for communicating with the device
    pMenuRoot = MenuMainInit()

    # Busy loop that runs the menu tree created above and communicates
    #   with the user
    return DIAG_MenuRun(pMenuRoot)

# -----------------------------------------------
#    Main diagnostics menu
#   -----------------------------------------------
def MenuIsDeviceNull(pMenu):
    global g_hDevs
    return g_hDevs[0] == None

def MenuMainExitCb(pCbCtx):
    global g_hDevs

#ifndef ISA
    for hDev in g_hDevs:
        if hDev:
            if sys.platform.startswith('linux') and fIsEnabledSRIOV:
                dwStatus = PCI_SriovDisable(hDev)
                if dwStatus:
                    print("pci_diag: Failed Disabling SR-IOV: %s\n" %
                        Stat2Str(dwStatus))

            PCI_DeviceClose(hDev)

    if wdapi.WDS_IsIpcRegistered():
        wdapi.WDS_IpcUnRegister()
#else # ifdef ISA
#    for hDev in g_hDevs:
#       if hDev and not PCI_DeviceClose(hDev):
#           PCI_ERR("pci_diag: Failed closing ISA device");
#endif # ifndef ISA

    # Uninitialize libraries
    dwStatus = PCI_LibUninit()
    if WD_STATUS_SUCCESS != dwStatus:
        WDC_DIAG_ERR("pci_diag: Failed to uninitialize the PCI library")

    return dwStatus


def MenuMainInit():

    menuRoot = DIAG_MENU_OPTION(
        cbExit = MenuMainExitCb,
        cTitleName = "PCI main menu"
    )

#ifndef ISA
    MenuCommonScanBusInit(menuRoot)
    MenuDeviceOpenInit(menuRoot)
    MenuSharedBufferInit(menuRoot)
    MenuIpcInit(menuRoot)
    MenuCfgInit(menuRoot)
    MenuEventsInit(menuRoot)
#endif #ifndef ISA

    MenuReadWriteAddrInit(menuRoot)

#ifdef HAS_INTS
    MenuInterruptsInit(menuRoot)
#endif # ifdef HAS_INTS

    MenuDmaInit(menuRoot)

#if defined(LINUX) && !defined(ISA)
    if sys.platform.startswith('linux'):
        MenuSriovInit(menuRoot)
#endif

    # 

    return menuRoot

#ifndef ISA

# -----------------------------------------------
#   Device Open
#   -----------------------------------------------
def MenuDeviceOpenCb(pCbCtx):
    global g_hDevs
    global fIsEnabledSRIOV

    if g_hDevs[0]:
#ifdef LINUX
        if sys.platform.startswith('linux') and fIsEnabledSRIOV:
            dwStatus = PCI_SriovDisable(g_hDevs[0])
            if dwStatus:
                PCI_ERR("pci_diag: Failed Disabling SR-IOV: %s\n",
                    Stat2Str(dwStatus))
#endif # ifdef LINUX

        PCI_DeviceClose(g_hDevs[0])

    g_hDevs[0] = PCI_DeviceOpen(0, 0)

    # 
    # Get Kernel PlugIn driver version
    if g_hDevs[0] and WDC_IS_KP(g_hDevs[0]):
        CheckKPDriverVer(g_hDevs[0])
    # 

    return WD_STATUS_SUCCESS

def MenuDeviceOpenInit(pParentMenu):
    menuDeviceOpenRoot = DIAG_MENU_OPTION(
        cbEntry = MenuDeviceOpenCb,
        cOptionName = "Find and open a PCI device"
    )

    DIAG_MenuSetCtxAndParentForMenus([menuDeviceOpenRoot], None, pParentMenu)

# -----------------------------------------------
#    Read/write the configuration space
# -----------------------------------------------

def MenuCfgInit(pParentMenu):
    global g_hDevs

    cfgMenuCtx = MENU_CTX_CFG(g_hDevs, gPCI_CfgRegs, PCI_CFG_REGS_NUM,
        gPCI_ext_CfgRegs, PCI_CFG_EXT_REGS_NUM)

    MenuCommonCfgInit(pParentMenu, cfgMenuCtx)


# ----------------------------------------------------
#    Plug-and-play and power management events handling
# ----------------------------------------------------
# Diagnostics plug-and-play and power management events handler routine
def DiagEventHandler(hDev, dwAction):
    # TODO: You can modify this function in order to implement your own
    #        diagnostics events handler routine.

    print("\nReceived event notification (device handle 0x%lX): " % hDev)

    if dwAction == WD_INSERT:
        print("WD_INSERT")
    elif dwAction == WD_REMOVE:
        print("WD_REMOVE")
    elif dwAction == WD_POWER_CHANGED_D0:
        print("WD_POWER_CHANGED_D0")
    elif dwAction == WD_POWER_CHANGED_D1:
        print("WD_POWER_CHANGED_D1")
    elif dwAction == WD_POWER_CHANGED_D2:
        print("WD_POWER_CHANGED_D2")
    elif dwAction == WD_POWER_CHANGED_D3:
        print("WD_POWER_CHANGED_D3")
    elif dwAction == WD_POWER_SYSTEM_WORKING:
        print("WD_POWER_SYSTEM_WORKING")
    elif dwAction == WD_POWER_SYSTEM_SLEEPING1:
        print("WD_POWER_SYSTEM_SLEEPING1")
    elif dwAction == WD_POWER_SYSTEM_SLEEPING2:
        print("WD_POWER_SYSTEM_SLEEPING2")
    elif dwAction == WD_POWER_SYSTEM_SLEEPING3:
        print("WD_POWER_SYSTEM_SLEEPING3")
    elif dwAction == WD_POWER_SYSTEM_HIBERNATE:
        print("WD_POWER_SYSTEM_HIBERNATE")
    elif dwAction == WD_POWER_SYSTEM_SHUTDOWN:
        print("WD_POWER_SYSTEM_SHUTDOWN")
    else:
        print("0x%lx\n" % dwAction)

# Reference to callback to keep it alive
# WARNING: Without this the program will crash upon handler call!
gf_DiagEventHandler = PCI_EVENT_HANDLER(DiagEventHandler)

def MenuEventsRegisterCb(pCbCtx):
    global g_hDevs
    if WD_STATUS_SUCCESS == PCI_EventRegister(g_hDevs[0],
            pCbCtx.DiagEventHandler):
        print("Events registered")
        pCbCtx.fIsRegistered = True
    else:
        WDC_DIAG_ERR("Failed to register events")

def MenuEventsUnregisterCb(pCbCtx):
    global g_hDevs
    if WD_STATUS_SUCCESS == PCI_EventUnregister(g_hDevs[0]):
        print("Events unregistered")
        pCbCtx.fIsRegistered = False
    else:
        WDC_DIAG_ERR("Failed to unregister events")

def MenuEventsCb(pCbCtx):
    global g_hDevs
    pCbCtx.fRegistered = PCI_EventIsRegistered(g_hDevs[0])

    if sys.platform == "win32" and not pCbCtx.fRegistered:
        print("NOTICE: An INF must be installed for your device in order to \n"
            "        call your user-mode event handler.\n"
            "        You can generate an INF file using the DriverWizard.")

    return WD_STATUS_SUCCESS

def MenuEventsInit(pParentMenu):
    global g_hDevs

    eventsMenuCtx = MENU_CTX_EVENTS(g_hDevs, gf_DiagEventHandler)
    eventsMenuCallbacks = MENU_EVENTS_CALLBACKS()

    eventsMenuCallbacks.eventsMenuEntryCb = MenuEventsCb
    eventsMenuCallbacks.eventsEnableCb = MenuEventsRegisterCb
    eventsMenuCallbacks.eventsDisableCb = MenuEventsUnregisterCb

    MenuCommonEventsInit(pParentMenu, eventsMenuCtx, eventsMenuCallbacks)

#endif #ifndef ISA

# -----------------------------------------------
#    Read/write memory and I/O addresses
#   -----------------------------------------------

def MenuReadWriteAddrInit(pParentMenu):
    global g_hDevs

    rwAddrMenusCtx = MENU_CTX_RW_ADDR(g_hDevs, False, WDC_MODE_32,
        ACTIVE_ADDR_SPACE_NEEDS_INIT)
    MenuCommonRwAddrInit(pParentMenu, rwAddrMenusCtx)

#ifdef HAS_INTS
# -----------------------------------------------
#   Interrupt handling
# -----------------------------------------------

# Diagnostics interrupt handler routine
def DiagIntHandler(hDev, pIntResult):
    # TODO: You can modify this function in order to implement your own
    #         diagnostics interrupt handler routine

    print("Got interrupt number %ld" % pIntResult.dwCounter)
    print("Interrupt Type: %s" %
        WDC_DIAG_IntTypeDescriptionGet(pIntResult.dwEnabledIntType))
    if WDC_INT_IS_MSI(pIntResult.dwEnabledIntType):
        print("Message Data: 0x%lx" % pIntResult.dwLastMessage)
    print

# Reference to callback to keep it alive (Without this the program will crash
# upon handler call)
gf_DiagIntHandler = PCI_INT_HANDLER(DiagIntHandler)

def MenuInterruptsEnableOptionCb(pCbCtx):
    global g_hDevs
    dwStatus = PCI_IntEnable(g_hDevs[0], pCbCtx.funcIntHandler)
    if WD_STATUS_SUCCESS == dwStatus:
        print("Interrupts enabled")
        pCbCtx.fIntsEnabled = True
    else:
        WDC_DIAG_ERR("Failed enabling interrupts. Error [0x%lx - "
            "%s]\n" % (dwStatus, Stat2Str(dwStatus)))

    return dwStatus

def MenuInterruptsDisableOptionCb(pCbCtx):
    global g_hDevs
    dwStatus = PCI_IntDisable(g_hDevs[0])
    if WD_STATUS_SUCCESS == dwStatus:
        print("Interrupts disabled")
        pCbCtx.fIntsEnabled = False
    else:
        WDC_DIAG_ERR("Failed disabling interrupts")

    return dwStatus

def MenuInterruptsCb(pCbCtx):
    global g_hDevs
    dwIntOptions = WDC_GET_INT_OPTIONS(g_hDevs[0])
    fIsMsi = WDC_INT_IS_MSI(dwIntOptions)
    pCbCtx.fIntsEnabled = PCI_IntIsEnabled(g_hDevs[0])

    # 
    if (dwIntOptions & INTERRUPT_LEVEL_SENSITIVE and not pCbCtx.fIntsEnabled):
        # TODO: You can remove this message after you have modified the
        #  implementation of PCI_IntEnable() in pci_lib.c to correctly
        #   acknowledge level-sensitive interrupts (see guidelines in
        #   PCI_IntEnable()).
        print("\n")
        print("WARNING!!!")
        print("----------")
        print("Your hardware has level sensitive interrupts.")
        print("Before enabling the interrupts, %s first modify the source "
            "code\n of PCI_IntEnable(), in the file pci_lib.c, to correctly "
            "acknowledge\n%s interrupts when they occur, as dictated by "
            "the hardware's specification." %
            ("it is recommended that" if fIsMsi else "you must",
            "level sensitive" if fIsMsi else ""))
    # 

    return WD_STATUS_SUCCESS

def MenuInterruptsInit(pParentMenu):
    global g_hDevs

    interruptsMenusCtx = MENU_CTX_INTERRUPTS(g_hDevs, gf_DiagIntHandler)
    interruptsMenusCallbacks = MENU_INTERRUPTS_CALLBACKS()

    interruptsMenusCallbacks.interruptsEnableCb = MenuInterruptsEnableOptionCb
    interruptsMenusCallbacks.interruptsDisableCb = MenuInterruptsDisableOptionCb
    interruptsMenusCallbacks.interruptsMenuEntryCb = MenuInterruptsCb

    MenuCommonInterruptsInit(pParentMenu, interruptsMenusCtx,
        interruptsMenusCallbacks)

#endif # ifdef HAS_INTS
# -----------------------------------------------
#    DMA memory handling
# -----------------------------------------------
# DMA menu options
(   MENU_DMA_ALLOCATE_CONTIG,
    MENU_DMA_ALLOCATE_SG,
    MENU_DMA_RESERVED_MEM) = range(1,4)

class MENU_CTX_DMA(object):
    def __init__(self, pBuf, pDma):
        self.pBuf = pBuf
        self.pDma = pDma
        self.size = 0
        self.dwOptions = 0
        self.qwAddr = 0
        self.dwDmaAddressWidth = 0

def MenuDmaIsKernelBufferNotExists(pMenu):
    return not (pMenu.pCbCtx.pDma and
        (pMenu.pCbCtx.pDma.contents.dwOptions & DMA_KERNEL_BUFFER_ALLOC))

def FreeDmaMem(pDma):
    if pDma:
        dwStatus = PCI_DmaBufUnlock(pDma)
        if WD_STATUS_SUCCESS == dwStatus:
            print("DMA memory freed")
            pDma = POINTER(WD_DMA)()
        else:
            WDC_DIAG_ERR("Failed trying to free DMA memory. Error [0x%lx - "
                "%s]\n" % (dwStatus, Stat2Str(dwStatus)))

    return pDma


def DmaGetAllocInput(menuDmaCtx, dwOption):
    dwOptions, size, dmaAddressWidth = 0, 0, 0

    if dwOption == MENU_DMA_RESERVED_MEM:
        (qwAddr, dwStatus) = DIAG_InputNum("Enter reserved memory address"
        " (64 bit hex uint) ", True, sizeof(UINT64), 1, 0xFFFFFFFFFFFFFFFF)
        if DIAG_INPUT_SUCCESS != dwStatus:
                return False

    if dwOption == MENU_DMA_ALLOCATE_CONTIG or \
        dwOption == MENU_DMA_ALLOCATE_SG or \
        dwOption == MENU_DMA_RESERVED_MEM:
        (size, dwStatus) = DIAG_InputNum("Enter memory allocation size in "
        "bytes (32 bit uint) ", True, sizeof(DWORD), 1, 0xFFFFFFFF)
        if DIAG_INPUT_SUCCESS != dwStatus:
            return False

    if dwOption == MENU_DMA_ALLOCATE_CONTIG:
        (dmaAddressWidth, dwStatus) = DIAG_InputNum("Enter maximum amount"
            " of bits of an address that your device supports, use 0 for "
            "default value (32 bit uint) ", False, sizeof(DWORD), 0, 64)
        if DIAG_INPUT_SUCCESS != dwStatus:
            return False

        dwOptions = DMA_KBUF_ALLOC_SPECIFY_ADDRESS_WIDTH | \
            (dmaAddressWidth.value << DMA_OPTIONS_MAX_SHIFT)

    menuDmaCtx.dwOptions = dwOptions
    menuDmaCtx.size = size
    menuDmaCtx.dmaAddressWidth = dmaAddressWidth

    # Free DMA memory before trying the new allocation
    menuDmaCtx.pDma = FreeDmaMem(menuDmaCtx.pDma)

    return True

def MenuDmaAllocContigOptionCb(pCbCtx):
    global g_hDevs
    if (not DmaGetAllocInput(pCbCtx, MENU_DMA_ALLOCATE_CONTIG)):
        return WD_INVALID_PARAMETER

    dwStatus = PCI_DmaAllocContig(g_hDevs[0], pCbCtx)
    if WD_STATUS_SUCCESS == dwStatus:
        print("Contiguous memory allocated. user addr [0x%lx], "
            "physical addr [0x%lx], size [%ld(0x%lx)]\n" %
            (pCbCtx.pDma.contents.pUserAddr,
            pCbCtx.pDma.contents.Page[0].pPhysicalAddr,
            pCbCtx.pDma.contents.Page[0].dwBytes,
            pCbCtx.pDma.contents.Page[0].dwBytes))
    else:
        WDC_DIAG_ERR("Failed allocating contiguous memory. size "
            "[0x%lx], Error [0x%lx - %s]\n" % (pCbCtx.size.value, dwStatus,
            Stat2Str(dwStatus)))

    return dwStatus

def MenuDmaAllocSgOptionCb(pCbCtx):
    global g_hDevs

    if (not DmaGetAllocInput(pCbCtx, MENU_DMA_ALLOCATE_SG)):
        return WD_INVALID_PARAMETER

    pCbCtx.pBuf = (c_byte * pCbCtx.size.value)() # Allocate a buffer of size "size"
    if not pCbCtx.pBuf:
        WDC_DIAG_ERR("Failed allocating user memory for SG. size [%lx]"
            % pCbCtx.size)
        return WD_INSUFFICIENT_RESOURCES

    dwStatus = PCI_DmaAllocSg(g_hDevs[0], pCbCtx)
    if WD_STATUS_SUCCESS == dwStatus:
        print("SG memory allocated. user addr [0x%lx], size [0x%lx]" %
            (addressof(pCbCtx.pBuf), pCbCtx.size.value))
        print("Pages physical addresses:")
        for i in range(min(pCbCtx.pDma.contents.dwPages, WD_DMA_PAGES)):
            print("%lu) physical addr [0x%lx], size [%ld(0x%lx)]" %
                (i + 1, pCbCtx.pDma.contents.Page[i].pPhysicalAddr,
                pCbCtx.pDma.contents.Page[i].dwBytes,
                pCbCtx.pDma.contents.Page[i].dwBytes))
            if i == WD_DMA_PAGES - 1:
                print("\nWarning: Maximum WD_DMA_PAGES default value"
                    " reached, results truncated.\n"
                    "         You can manually change this value in"
                    " windrvr.py if needed.\n")
    else:
        WDC_DIAG_ERR("Failed allocating SG memory. size [%ld], Error "
            "[0x%lx - %s]\n" % (pCbCtx.size, dwStatus,
            Stat2Str(dwStatus)))

    return dwStatus

def MenuDmaReservedOptionCb(pCbCtx):
    global g_hDevs

    if (not DmaGetAllocInput(pCbCtx, MENU_DMA_RESERVED_MEM)):
        return WD_INVALID_PARAMETER

    dwStatus = PCI_DmaAllocReserved(g_hDevs[0], pCbCtx)
    if WD_STATUS_SUCCESS == dwStatus:
        print(("Reserved memory claimed. user addr [%lx], bus addr "
            "[0x%lx], size [%ld(0x%lx)]\n") % (addressof(pCbCtx.pBuf.contents),
            pCbCtx.pDma.contents.Page[0].pPhysicalAddr,
            pCbCtx.pDma.contents.Page[0].dwBytes,
            pCbCtx.pDma.contents.Page[0].dwBytes))
    else:
        WDC_DIAG_ERR("Failed claiming reserved memory. size [%ld], "
            "Error [0x%lx - %s]\n" % (pCbCtx.size.value, dwStatus,
            Stat2Str(dwStatus)))

    return dwStatus

def MenuDmaSendSharedBuffOptionCb(pCbCtx):
    WDS_DIAG_IpcSendDmaContigToGroup(pCbCtx.pDma)
    return WD_STATUS_SUCCESS

def MenuDmaExitCb(pCbCtx):
    pCbCtx.pDma = FreeDmaMem(pCbCtx.pDma)
    pCbCtx.dwOptions = 0
    pCbCtx.dwDmaAddressWidth = 0
    pCbCtx.qwAddr = 0
    pCbCtx.pBuf = PVOID()

    return WD_STATUS_SUCCESS

def MenuDmaSubMenusInit(pParentMenu, menuDmaCtx):
     options = [
        DIAG_MENU_OPTION (
            cOptionName = "Allocate contiguous memory",
            cbEntry = MenuDmaAllocContigOptionCb ),

        DIAG_MENU_OPTION (
            cOptionName = "Allocate scatter-gather memory",
            cbEntry = MenuDmaAllocSgOptionCb ),

        DIAG_MENU_OPTION (
            cOptionName = "Use reserved memory",
            cbEntry = MenuDmaReservedOptionCb ),

        DIAG_MENU_OPTION (
            cOptionName = "Send buffer through IPC to all group processes",
            cbEntry = MenuDmaSendSharedBuffOptionCb,
            cbIsHidden = MenuDmaIsKernelBufferNotExists ),

        DIAG_MENU_OPTION (
            cOptionName = "Free DMA memory",
            cbEntry = MenuDmaExitCb )
     ]

     DIAG_MenuSetCtxAndParentForMenus(options, menuDmaCtx, pParentMenu)

def MenuDmaInit(pParentMenu):
    menuDmaCtx = MENU_CTX_DMA(PVOID(), POINTER(WD_DMA)())
    dmaMenuRoot = DIAG_MENU_OPTION(
        cOptionName = "Allocate/free memory for DMA",
        cTitleName = "DMA Memory",
        cbIsHidden = MenuIsDeviceNull,
        cbExit = MenuDmaExitCb
    )

    MenuDmaSubMenusInit(dmaMenuRoot, menuDmaCtx)
    DIAG_MenuSetCtxAndParentForMenus([dmaMenuRoot], menuDmaCtx, pParentMenu)

# -----------------------------------------------
#   SRIOV handling
# -----------------------------------------------
def MenuSriovEnableOptionCb(pCbCtx):
    global g_hDevs
    global fIsEnabledSRIOV

    (dwDesiredNumVFs, dwStatus) = DIAG_InputNum("How many Virtual "
            "Functions would you like to enable:" , True, sizeof(DWORD),
            0, 0xFFFFFFFF)
    if DIAG_INPUT_SUCCESS != dwStatus:
        print("Wrong Input")
        return dwStatus

    dwStatus = PCI_SriovEnable(g_hDevs[0], dwDesiredNumVFs)
    if WD_STATUS_SUCCESS == dwStatus:
        print("SR-IOV enabled successfully")
        fIsEnabledSRIOV = True
    else:
        WDC_DIAG_ERR("Failed enabling SR-IOV. Error [0x%lx - "
                "%s]\n" % (dwStatus, Stat2Str(dwStatus)))

    return dwStatus

def MenuSriovDisableOptionCb(pCbCtx):
    global g_hDevs
    global fIsEnabledSRIOV

    dwStatus = PCI_SriovDisable(g_hDevs[0])
    if WD_STATUS_SUCCESS == dwStatus:
        print("SR-IOV disabled successfully")
        fIsEnabledSRIOV = False
    else:
        WDC_DIAG_ERR("Failed disabling SR-IOV. Error [0x%lx - "
                "%s]\n" % (dwStatus, Stat2Str(dwStatus)))
    return dwStatus

def MenuSriovCb(pCbCtx):
    print("\n")

    dwStatus = PCI_SriovGetNumVFs(pCbCtx.g_hDevs[0], pCbCtx.dwNumVFs)
    if WD_STATUS_SUCCESS != dwStatus:
        print("Could not obtain dwNumVFs.")
    else:
        print("SR-IOV is %s. dwNumVFs: [%ld]" %
            ("Enabled" if pCbCtx.dwNumVFs.value > 0 else  "Disabled",
            pCbCtx.dwNumVFs.value))

    print("-----------\n")
    return WD_STATUS_SUCCESS

def MenuSriovExitCb(pCbCtx):
    pCbCtx.dwNumVFs = DWORD(0)
    return WD_STATUS_SUCCESS

def MenuSriovInit(pParentMenu):
    global g_hDevs

    sriovMenusCtx = MENU_CTX_SRIOV(g_hDevs)
    sriovMenusCallbacks = MENU_SRIOV_CALLBACKS()

    sriovMenusCallbacks.sriovEnableCb = MenuSriovEnableOptionCb
    sriovMenusCallbacks.sriovDisableCb = MenuSriovDisableOptionCb
    sriovMenusCallbacks.sriovMenuEntryCb = MenuSriovCb
    sriovMenusCallbacks.sriovMenuExitCb = MenuSriovExitCb

    MenuCommonSriovInit(pParentMenu, sriovMenusCtx, sriovMenusCallbacks)

# 
def CheckKPDriverVer(hDev):
    kpVer = KP_PCI_VERSION()
    dwKPResult = DWORD()

    # Get Kernel PlugIn Driver version
    dwStatus = wdapi.WDC_CallKerPlug(hDev, KP_PCI_MSG_VERSION, byref(kpVer),
        byref(dwKPResult))
    if WD_STATUS_SUCCESS != dwStatus:
        WDC_DIAG_ERR("Failed sending a \'Get Version\' message [0x%x] to the "
            "Kernel-PlugIn driver [%s]. Error [0x%lx - %s]\n" %
            (KP_PCI_MSG_VERSION, KP_PCI_DRIVER_NAME, dwStatus,
            Stat2Str(dwStatus)))

    elif KP_PCI_STATUS_OK != dwKPResult.value:
        WDC_DIAG_ERR("Kernel-PlugIn \'Get Version\' message [0x%x] failed. "
            "Kernel PlugIn status [0x%lx]\n" % (KP_PCI_MSG_VERSION,
                dwKPResult.value))
        dwStatus = WD_INCORRECT_VERSION
    else:
        print("Using [%s] Kernel-Plugin driver version [%ld.%02ld - %s]\n" %
            (KP_PCI_DRIVER_NAME, kpVer.dwVer / 100, kpVer.dwVer % 100,
            kpVer.cVer))

    return dwStatus

# 

# -----------------------------------------------
#    Read/write the run-time registers
#   -----------------------------------------------

# 


if __name__ == "__main__":
    main()


