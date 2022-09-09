''' Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com '''

from ctypes import *
from wdlib.wdc_lib import *
from wdlib.pci_regs import *
from wdlib.wdc_diag_lib import *
import sys

def TraceLog(s):
    wdapi_va.WDC_Trace(b"PCI lib TRACE: %s\n", s.encode('utf-8'))
    #print ("PCI lib TRACE: " + s)

def ErrLog(s):
    wdapi_va.WDC_Err(b"PCI lib ERROR: %s\n", s.encode('utf-8'))
    #print ("PCI lib ERROR: " + s)

KP_PCI_DRIVER_NAME = "KP_PCI"

# Kernel PlugIn messages - used in WDC_CallKerPlug() calls (user mode)
# KP_PCI_Call() (kernel mode)
KP_PCI_MSG_VERSION = 1

# Kernel PlugIn messages status
KP_PCI_STATUS_OK          = 0x1
KP_PCI_STATUS_MSG_NO_IMPL = 0x1000
KP_PCI_STATUS_FAIL        = 0x1001

#ifndef ISA
# Default vendor and device IDs (0 == all)
# TODO: Replace the ID values with your device's vendor and device IDs
PCI_DEFAULT_VENDOR_ID = 0x0 # Vendor ID
PCI_DEFAULT_DEVICE_ID = 0x0 # Device ID
#else # ifdef ISA
## ISA address spaces information
#PCI_ADDR_SPACES_NUM = 0 # Number of address spaces

## Base address spaces numbers
# 

## Physical base addresses
# 

## Size (in bytes) of address spaces
# 

##ifdef HAS_INTS
## Interrupts information
#PCI_INTS_NUM = 1 # Number of interrupts

## Interrupt Request (IRQ)
# 

## Interrupt registration options
# 
##endif # ifdef HAS_INTS

## TODO: Add address space info
## Total number of items - address spaces, interrupts and bus items
#PCI_ITEMS_NUM = PCI_ADDR_SPACES_NUM + 1
##endif # ifdef ISA

# 
## Interrupt acknowledgment information
## TODO: Use correct values according to the specification of your device.
INTCSR            = 0x00          # Interrupt register
INTCSR_ADDR_SPACE = 0             # Interrupt register's address space
ALL_INT_MASK      = 0xFFFFFFFF    # Interrupt acknowledgment command
# 

# WinDriver license registration string
# TODO: When using a registered WinDriver version, replace the license string
#         below with the development license in order to use on the development
#         machine.
#         Once you require to distribute the driver's package to other machines,
#         please replace the string with a distribution license
PCI_DEFAULT_LICENSE_STRING = b"12345abcde1234.license"

PCI_DEFAULT_DRIVER_NAME = WD_DEFAULT_DRIVER_NAME_BASE

# Kernel PlugIn version information struct
class KP_PCI_VERSION(Structure): _fields_ = \
    [("dwVer", DWORD),
    ("cVer", CHAR * 100)]

# Device address description struct
PWDC_ADDR_DESC = POINTER(WDC_ADDR_DESC)
class PCI_DEV_ADDR_DESC(Structure): _fields_ = \
    [("dwNumAddrSpaces", DWORD),   # Total number of device address spaces
    ("pAddrDesc", PWDC_ADDR_DESC)] # Array of device address spaces information

# Address space information struct
MAX_TYPE = 8
class PCI_ADDR_SPACE_INFO():
    dwAddrSpace = 0
    sType = ""
    sName = ""
    sDesc = ""

# Interrupt result information struct
class PCI_INT_RESULT(Structure): _fields_ = \
    [("dwCounter", DWORD), # Number of interrupts received
    ("dwLost", DWORD),     # Number of interrupts not yet handled
    ("waitResult", WD_INTERRUPT_WAIT_RESULT), # See WD_INTERRUPT_WAIT_RESULT values
                                              #    in windrvr.h
    ("dwEnabledIntType", DWORD), # Interrupt type that was actually enabled
                                 #   (MSI/MSI-X / Level Sensitive / Edge-Triggered)
    ("dwLastMessage", DWORD)]    # Message data of the last received MSI/MSI-X
                                 # (Windows Vista and higher) N/A to line-based
                                 #  interrupts.

# TODO: You can add fields to PCI_INT_RESULT to store any additional
#        information that you wish to pass to your diagnostics interrupt
#         handler routine (DiagIntHandler() in pci_diag.py).

# PCI diagnostics interrupt handler function type
#typedef void (*PCI_INT_HANDLER)(WDC_DEVICE_HANDLE hDev,
#    PCI_INT_RESULT *pIntResult)
PCI_INT_HANDLER = DLLCALLCONV(None, WDC_DEVICE_HANDLE, PCI_INT_RESULT)

# PCI diagnostics plug-and-play and power management events handler function
# type
#typedef void (*PCI_EVENT_HANDLER)(WDC_DEVICE_HANDLE hDev, DWORD dwAction)
PCI_EVENT_HANDLER = DLLCALLCONV(None, WDC_DEVICE_HANDLE, DWORD)

# 
# PCI device information struct
class PCI_DEV_CTX(Structure): _fields_ = \
    [
    #ifdef HAS_INTS
    # 
    # 
    ("pIntTransCmds", PVOID),
    # 
    ("funcDiagIntHandler", PCI_INT_HANDLER),    # Interrupt handler routine
    #endif # ifdef HAS_INTS
    #ifndef ISA
    ("funcDiagEventHandler", PCI_EVENT_HANDLER), # Event handler routine
    #else # ifdef ISA
    #ifndef HAS_INTS
    #("pData", PVOID)
    # TODO: Set pData to point to any device-specific data that you wish to
    #        store or replace pData with your own device context information
    #endif # ifndef HAS_INTS
    #endif # ifdef ISA
    ]
PPCI_DEV_CTX = POINTER(PCI_DEV_CTX)

class KP_PCI_VERSION(Structure): _fields_ = \
    [("dwVer", DWORD),
     ("cVer", CHAR * 100)]

# *************************************************************
#  Global variables definitions
# *************************************************************
# Library initialization reference count
LibInit_count = 0

# Validate a device handle
def IsValidDevice(hDev, sFunc):
    if not hDev or not wdapi.WDC_GetDevContext(hDev):
        ErrLog ("%s: NULL device %s\n" % (sFunc, "handle" if not pDev else
            "context"))
        return False

    return True

def PCI_LibInit():
    try:
        # Increase the library's reference count initialize the library only once
        LibInit_count += 1
    except:
        LibInit_count = 0
    if LibInit_count > 1:
        return WD_STATUS_SUCCESS

    #ifdef WD_DRIVER_NAME_CHANGE
    # Set the driver name
    if not wdapi.WD_DriverName(PCI_DEFAULT_DRIVER_NAME):
        ErrLog("Failed to set the driver name for WDC library.\n")
        return WD_SYSTEM_INTERNAL_ERROR
    #endif

    # Set WDC library's debug options
    # (default: level=TRACE redirect output to the Debug Monitor)
    dwStatus = wdapi.WDC_SetDebugOptions(WDC_DBG_DEFAULT, 0)
    if WD_STATUS_SUCCESS != dwStatus:
        ErrLog("Failed to initialize debug options for WDC library.\n"
            "Error 0x%lx - %s\n" % (dwStatus, Stat2Str(dwStatus)))
        return dwStatus

    # Open a handle to the driver and initialize the WDC library
    dwStatus = wdapi.WDC_DriverOpen(WDC_DRV_OPEN_DEFAULT,
        PCI_DEFAULT_LICENSE_STRING)
    if WD_STATUS_SUCCESS != dwStatus:
        ErrLog("Failed to initialize the WDC library. Error 0x%lx - %s\n" %
            (dwStatus, Stat2Str(dwStatus)))
        return dwStatus

    return WD_STATUS_SUCCESS

def PCI_LibUninit():
    global LibInit_count
    # Decrease the library's reference count uninitialize the library only
    # when there are no more open handles to the library
    LibInit_count -= 1
    if LibInit_count > 0:
        return WD_STATUS_SUCCESS

    # Uninitialize the WDC library and close the handle to WinDriver
    dwStatus = wdapi.WDC_DriverClose()
    if WD_STATUS_SUCCESS != dwStatus:
        ErrLog("Failed to uninit the WDC library. Error 0x%lx - %s\n" %
            (dwStatus, Stat2Str(dwStatus)))

    return dwStatus

# -----------------------------------------------
#    Device open/close
# -----------------------------------------------
# Open a device handle
#ifndef ISA
def PCI_DeviceOpen(dwVendorId, dwDeviceId):
    global gpDevCtx

    gpDevCtx = PCI_DEV_CTX()
    hDev = WDC_DIAG_DeviceFindAndOpen(dwVendorId, dwDeviceId, gpDevCtx,
        KP_PCI_DRIVER_NAME, PCI_DEV_ADDR_DESC)

    # Validate device information
    if not hDev or not DeviceValidate(cast(hDev, PWDC_DEVICE)):
        return PCI_DeviceOpen_err(hDev)

    return hDev
#else # ifdef ISA
#def PCI_DeviceOpen():
#    global gpDevCtx
#    hDev = WDC_DEVICE_HANDLE()
#    gpDevCtx = PCI_DEV_CTX()
#    # 
#    devAddrDesc = PCI_DEV_ADDR_DESC()
#    # 
#
#    # Set the device's resources information
#    SetDeviceResources(deviceInfo)
#
#    # Open a device handle
#    dwStatus = wdapi.WDC_IsaDeviceOpen(byref(hDev), byref(deviceInfo),
#        byref(gpDevCtx))
#
#    if WD_STATUS_SUCCESS != dwStatus:
#        ErrLog("Failed opening a WDC device handle. Error 0x%lx - %s\n" %
#            (dwStatus, Stat2Str(dwStatus)))
#        return PCI_DeviceOpen_err(hDev)
#    pDev = cast(hDev, PWDC_DEVICE)
#
#    # 
#    devAddrDesc.dwNumAddrSpaces = pDev.contents.dwNumAddrSpaces
#    devAddrDesc.pAddrDesc = pDev.contents.pAddrDesc
#
#    # Open a handle to a Kernel PlugIn driver
#    wdapi.WDC_KernelPlugInOpen(hDev, KP_PCI_DRIVER_NAME.encode('utf-8'),
#        byref(devAddrDesc))
#    # 
#
#    # Return handle to the new device
#    TraceLog("PCI_DeviceOpen: Opened a PCI device (handle 0x%lx)\n"
#        "Device is %susing a Kernel PlugIn driver (%s)\n" % (hDev.value,
#        "" if WDC_IS_KP(pDev) else "not " , KP_PCI_DRIVER_NAME))
#    return hDev
#endif # ifdef ISA
def PCI_DeviceOpen_err(hDev):
    if hDev:
        PCI_DeviceClose(hDev)

    return None

# Close device handle
def PCI_DeviceClose(hDev):
    global gpDevCtx
    TraceLog("PCI_DeviceClose: Entered. Device handle 0x%lx\n" % hDev.value)

    # Validate the device handle
    if not hDev:
        ErrLog("PCI_DeviceClose: Error - NULL device handle\n")
        return False

#ifdef HAS_INTS
    # Disable interrupts (if enabled)
    if wdapi.WDC_IntIsEnabled(hDev):
        dwStatus = PCI_IntDisable(hDev)
        if WD_STATUS_SUCCESS != dwStatus:
            ErrLog("Failed disabling interrupts. Error 0x%lx - %s\n" %
                (dwStatus, Stat2Str(dwStatus)))
#endif # ifdef HAS_INTS


    if gpDevCtx:
        gpDevCtx = PCI_DEV_CTX()

    # Close the device handle
    return WDC_DIAG_DeviceClose(hDev)

#ifndef ISA
# Validate device information
def DeviceValidate(pDev):
    # NOTE: You can modify the implementation of this function in order to
    #         verify that the device has the resources you expect to find.

    # Verify that the device has at least one active address space
    for i in range(pDev.contents.dwNumAddrSpaces):
        if wdapi.WDC_AddrSpaceIsActive(pDev, i):
            TraceLog("Address space number %d is active" % i)
            return True

    # In this sample we accept the device even if it doesn't have any
    # address spaces
    TraceLog("Device does not have any active memory or I/O address spaces\n")
    return True
#else # ifdef ISA
#def SetDeviceResources(DeviceInfo):
#    DeviceInfo.dwItems = PCI_ITEMS_NUM
#    pItem = DeviceInfo.Item[0]
#    # Bus
#    pItem.item = ITEM_BUS
#    pItem.I.Bus.dwBusType = WD_BUS_ISA

# 
#endif # ifdef ISA

#ifdef HAS_INTS
# -----------------------------------------------
#    Interrupts
# -----------------------------------------------

# Reference to PCI_INT_RESULT to keep it alive
# WARNING: Without this the program will crash upon handler call
gf_intResult = PCI_INT_RESULT()

# Interrupt handler routine
def PCI_IntHandler(pData):
    pDev = cast(PVOID(pData), PWDC_DEVICE)
    pDevCtx = cast(wdapi.WDC_GetDevContext(PVOID(pData)), PPCI_DEV_CTX)

    gf_intResult.dwCounter = pDev.contents.Int.dwCounter
    gf_intResult.dwLost = pDev.contents.Int.dwLost
    gf_intResult.waitResult = pDev.contents.Int.fStopped
    gf_intResult.dwEnabledIntType = WDC_GET_ENABLED_INT_TYPE(pDev)
    gf_intResult.dwLastMessage = WDC_GET_ENABLED_INT_LAST_MSG(pDev)

    # Execute the diagnostics application's interrupt handler routine
    pDevCtx.contents.funcDiagIntHandler(cast(pData, WDC_DEVICE_HANDLE),
        gf_intResult)

gf_intHandler = DLLCALLCONV(None, PVOID)(PCI_IntHandler)

# Check whether a given device contains an item of the specified type
def IsItemExists(pDev, item):
    for i in range(pDev.contents.cardReg.Card.dwItems):
        if pDev.contents.cardReg.Card.Item[i].item == item:
            return True

    return False

#defined as global to keep the variable alive
gpTrans = (WD_TRANSFER * WD_TRANSFER_CMDS)()

# Enable interrupts
def PCI_IntEnable(hDev, funcIntHandler):
    pDev = cast(hDev, PWDC_DEVICE)
    dwNumTransCmds = 0
    dwOptions = 0
    global gpTrans

    TraceLog("PCI_IntEnable: Entered. Device handle 0x%lx\n" % hDev.value)

    # Validate the device handle
    if not IsValidDevice(hDev, "PCI_IntEnable"):
        return WD_INVALID_PARAMETER

    # Verify that the device has an interrupt item
    if not IsItemExists(pDev, ITEM_INTERRUPT):
        return WD_OPERATION_FAILED

    pDevCtx = cast(wdapi.WDC_GetDevContext(hDev), PPCI_DEV_CTX)

    # Check whether interrupts are already enabled
    if wdapi.WDC_IntIsEnabled(hDev):
        ErrLog("Interrupts are already enabled ...\n")
        return WD_OPERATION_ALREADY_DONE

    # 

    # When using a Kernel PlugIn, acknowledge interrupts in kernel mode
    if not WDC_IS_KP(pDev):
        # TODO: Change this value, if needed
        dwNumTransCmds = 2

        # This sample demonstrates how to set up two transfer commands, one
        # for reading the device's INTCSR register (as defined in gPCI_Regs)
        # and one for writing to it to acknowledge the interrupt. The transfer
        # commands will be executed by WinDriver in the kernel when an
        # interrupt occurs.
#ifndef ISA
        # TODO: If PCI interrupts are level sensitive interrupts, they must be
        # acknowledged in the kernel immediately when they are received. Since
        # the information for acknowledging the interrupts is
        # hardware-specific, YOU MUST MODIFY THE CODE below and set up transfer
        # commands in order to correctly acknowledge the interrupts on your
        # device, as dictated by your hardware's specifications.
        # If the device supports both MSI/MSI-X and level sensitive interrupts,
        # you must set up transfer commands in order to allow your code to run
        # correctly on systems other than Windows Vista and higher and Linux.
        # Since MSI/MXI-X does not require acknowledgment of the interrupt, to
        # support only MSI/MSI-X handling (for hardware and OSs that support
        # this), you can avoid defining transfer commands, or specify
        # kernel-mode commands to be performed upon interrupt generation
        # according to your specific needs.
#endif # ifndef ISA

        #*****************************************************************
        # NOTE: If you attempt to use this code without first modifying it in
        # order to correctly acknowledge your device's level-sensitive
        # interrupts, as explained above, the OS will HANG when a level
        # sensitive interrupt occurs!
        #******************************************************************

        # Prepare the interrupt transfer commands.
        #
        # The transfer commands will be executed by WinDriver's ISR
        # which runs in kernel mode at interrupt level.

        # TODO: Change the offset of INTCSR and the PCI address space, if
        #       needed
        # #1: Read status from the INTCSR register
        pAddrDesc = WDC_GET_ADDR_DESC(pDev, INTCSR_ADDR_SPACE)
        gpTrans[0].pPort = pAddrDesc.pAddr + INTCSR
        # Read from a 32-bit register
        gpTrans[0].cmdTrans = RM_DWORD if WDC_ADDR_IS_MEM(pAddrDesc) else \
            RP_DWORD

        # #2: Write ALL_INT_MASK to the INTCSR register to acknowledge the
        #     interrupt
        gpTrans[1].pPort = gpTrans[0].pPort # In this example both commands
                                    # access the same address
                                    # (register)
        # Write to a 32-bit register
        gpTrans[1].cmdTrans = WM_DWORD if WDC_ADDR_IS_MEM(pAddrDesc) else \
            WP_DWORD
        gpTrans[1].Data.Dword = ALL_INT_MASK

        # Copy the results of "read" transfer commands back to user mode
        dwOptions = INTERRUPT_CMD_COPY

    # Store the diag interrupt handler routine, which will be executed by
    #   PCI_IntHandler() when an interrupt is received
    pDevCtx.contents.funcDiagIntHandler = funcIntHandler

    # Enable interrupts
    dwStatus = wdapi.WDC_IntEnable(hDev, gpTrans, dwNumTransCmds, dwOptions,
        gf_intHandler, hDev, WDC_IS_KP(pDev))
    if WD_STATUS_SUCCESS != dwStatus:
        ErrLog("Failed enabling interrupts. Error 0x%lx - %s\n" % (dwStatus,
            Stat2Str(dwStatus)))
        if gpTrans:
            gpTrans = (WD_TRANSFER * WD_TRANSFER_CMDS)()

        return dwStatus

    # Store the interrupt transfer commands in the device context
    pDevCtx.pIntTransCmds = gpTrans
    # 
    # TODO: You can add code here to write to the device in order to
    #         physically enable the hardware interrupts.

    TraceLog("PCI_IntEnable: Interrupts enabled\n")

    return WD_STATUS_SUCCESS

# Disable interrupts
def PCI_IntDisable(hDev):
    TraceLog("PCI_IntDisable entered. Device handle 0x%lx\n" % hDev.value)

    # Validate the device handle
    if not IsValidDevice(hDev, "PCI_IntDisable"):
        return WD_INVALID_PARAMETER

    pDevCtx = cast(wdapi.WDC_GetDevContext(hDev), PPCI_DEV_CTX)

    # Check whether interrupts are already enabled
    if not wdapi.WDC_IntIsEnabled(hDev):
        ErrLog("Interrupts are already disabled ...\n")
        return WD_OPERATION_ALREADY_DONE

    # TODO: You can add code here to write to the device in order to
    #         physically disable the hardware interrupts.

    # Disable interrupts
    dwStatus = wdapi.WDC_IntDisable(hDev)
    if WD_STATUS_SUCCESS != dwStatus:
        ErrLog("Failed disabling interrupts. Error 0x%lx - %s\n" %
            (dwStatus, Stat2Str(dwStatus)))

    return dwStatus

# Check whether interrupts are enabled for the given device
def PCI_IntIsEnabled(hDev):
    # Validate the device handle
    if not IsValidDevice(cast(hDev, PWDC_DEVICE), "PCI_IntIsEnabled"):
        return False

    # Check whether interrupts are already enabled
    return wdapi.WDC_IntIsEnabled(hDev)
#endif # ifdef HAS_INTS

#ifndef ISA
# -----------------------------------------------
#    Plug-and-play and power management events
# -----------------------------------------------

# Plug-and-play or power management event handler routine
def PCI_EventHandler(pEvent, pData):
    hDev = cast(pData, WDC_DEVICE_HANDLE)
    pDevCtx = cast(wdapi.WDC_GetDevContext(hDev), PPCI_DEV_CTX)

    TraceLog("PCI_EventHandler entered, pData 0x%lx, dwAction 0x%lx\n" % \
        (pData, pEvent.contents.dwAction))

    # Execute the diagnostics application's event handler function
    pDevCtx.contents.funcDiagEventHandler(hDev, pEvent.contents.dwAction)

# Reference to callback to keep it alive
# WARNING: Without this the program will crash upon handler call!
gf_PCI_EventHandler = \
    DLLCALLCONV(None, POINTER(WD_EVENT), PVOID)(PCI_EventHandler)

# Register a plug-and-play or power management event
def PCI_EventRegister(hDev, funcEventHandler):
    pDev = cast(hDev, PWDC_DEVICE)
    dwActions = WD_ACTIONS_ALL
    # TODO: Modify the above to set up the plug-and-play/power management
    #       events for which you wish to receive notifications.
    #       dwActions can be set to any combination of the WD_EVENT_ACTION
    #       flags defined in windrvr.py

    TraceLog("PCI_EventRegister entered. Device handle 0x%lx\n" % hDev.value)

    # Validate the device handle
    if not IsValidDevice(hDev, "PCI_EventRegister"):
        return WD_INVALID_PARAMETER

    pDevCtx = cast(wdapi.WDC_GetDevContext(hDev), PPCI_DEV_CTX)

    # Check whether the event is already registered
    if wdapi.WDC_EventIsRegistered(hDev):
        ErrLog("Events are already registered ...\n")
        return WD_OPERATION_ALREADY_DONE

    # Store the diag event handler routine to be executed from
    # PCI_EventHandler() upon an event
    pDevCtx.contents.funcDiagEventHandler = funcEventHandler

    # Register the event
    pDev = cast(hDev, PWDC_DEVICE)

    dwStatus = wdapi.WDC_EventRegister(hDev, dwActions,
        gf_PCI_EventHandler, hDev, WDC_IS_KP(pDev))

    if WD_STATUS_SUCCESS != dwStatus:
        ErrLog("Failed to register events. Error 0x%lx - %s\n" %
            (dwStatus, Stat2Str(dwStatus)))
        return dwStatus

    TraceLog("Events registered\n")
    return WD_STATUS_SUCCESS

# Unregister a plug-and-play or power management event
def PCI_EventUnregister(hDev):
    TraceLog("PCI_EventUnregister entered. Device handle 0x%lx\n" % hDev.value)

    # Validate the device handle
    if not IsValidDevice(cast(hDev, PWDC_DEVICE), "PCI_EventUnregister"):
        return WD_INVALID_PARAMETER

    # Check whether the event is currently registered */
    if not wdapi.WDC_EventIsRegistered(hDev):
        ErrLog("Cannot unregister events - no events currently "
            "registered ...\n")
        return WD_OPERATION_ALREADY_DONE

    # Unregister the event
    dwStatus = wdapi.WDC_EventUnregister(hDev)
    if WD_STATUS_SUCCESS != dwStatus:
        ErrLog("Failed to unregister events. Error 0x%lx - %s\n" % (dwStatus,
            Stat2Str(dwStatus)))

    return dwStatus

# Check whether a given plug-and-play or power management event is registered

def PCI_EventIsRegistered(hDev):
    # Validate the device handle
    if not IsValidDevice(hDev, "PCI_EventIsRegistered"):
        return False

    # Check whether the event is registered #
    return wdapi.WDC_EventIsRegistered(hDev)

#endif # ifndef ISA

#if !defined(ISA) && defined(LINUX)
def PCI_SriovEnable(hDev, dwNumVFs):
    pDev = cast(hDev, PWDC_DEVICE)

    TraceLog("PCI_SriovEnable entered. Device handle: 0x%lx\n" % hDev.value)

    # Validate the device handle
    if not IsValidDevice(pDev, "PCI_SriovEnable"):
        return WD_INVALID_PARAMETER

    return wdapi.WDC_PciSriovEnable(hDev, dwNumVFs)

def PCI_SriovDisable(hDev):
    pDev = cast(hDev, PWDC_DEVICE)

    TraceLog("PCI_SriovDisable entered. Device handle: 0x%lx\n" %  hDev.value)

    # Validate the device handle
    if not IsValidDevice(pDev, "PCI_SriovDisable"):
        return WD_INVALID_PARAMETER;

    return wdapi.WDC_PciSriovDisable(hDev)

def PCI_SriovGetNumVFs(hDev, pdwNumVFs):
    pDev = cast(hDev, PWDC_DEVICE)
    TraceLog("PCI_SriovGetNumVFs entered. Device handle: 0x%lx\n" %  hDev.value)
    if pdwNumVFs.value:
        pdwNumVFs.value = 0

    # Validate the device handle
    if not IsValidDevice(pDev, "PCI_SriovGetNumVfs"):
        return WD_INVALID_PARAMETER
    return wdapi.WDC_PciSriovGetNumVFs(hDev, byref(pdwNumVFs))

#endif

def PCI_DmaAllocContig(hDev, pDmaMenusCtx):
    dwStatus = wdapi.WDC_DMAContigBufLock(hDev, byref(pDmaMenusCtx.pBuf),
        pDmaMenusCtx.dwOptions, pDmaMenusCtx.size, byref(pDmaMenusCtx.pDma))
    if WD_STATUS_SUCCESS != dwStatus:
        ErrLog("Failed allocating contiguous memory.size[% lu], "
            "Error [0x%lx - %s]\n" % (pDmaMenusCtx.size, dwStatus,
            Stat2Str(dwStatus)))

    return dwStatus
def PCI_DmaAllocSg(hDev, pDmaMenusCtx):
    dwStatus = wdapi.WDC_DMASGBufLock(hDev, byref(pDmaMenusCtx.pBuf), 0,
        pDmaMenusCtx.size, byref(pDmaMenusCtx.pDma))
    if WD_STATUS_SUCCESS != dwStatus:
        ErrLog("Failed allocating SG memory. size [%ld], Error "
            "[0x%lx - %s]\n" % (pDmaMenusCtx.size, dwStatus,
            Stat2Str(dwStatus)))

    return dwStatus

def PCI_DmaAllocReserved(hDev, pDmaMenusCtx):
    dwStatus = wdapi.WDC_DMAReservedBufLock(hDev, pDmaMenusCtx.qwAddr,
        byref(pCbCtx.pBuf), 0,  pDmaMenusCtx.size, byref(pDmaMenusCtx.pDma))
    if WD_STATUS_SUCCESS != dwStatus:
        ErrLog("Failed claiming reserved memory. size [%ld], "
            "Error [0x%lx - %s]\n" % (pDmaMenusCtx.size.value, dwStatus,
            Stat2Str(dwStatus)))

    return dwStatus

def PCI_DmaBufUnlock(pDma):
    dwStatus = wdapi.WDC_DMABufUnlock(pDma)
    if (WD_STATUS_SUCCESS != dwStatus):
        ErrLog("Failed trying to free DMA memory. Error [0x%lx - %s]\n" %
            (dwStatus, Stat2Str(dwStatus)))

    return dwStatus
