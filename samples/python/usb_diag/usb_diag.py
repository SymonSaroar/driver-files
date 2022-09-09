''' Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com '''

import sys, os
sys.path.append(os.path.dirname(os.path.abspath(__file__)) + "/..")
from wdlib.usb_diag_lib import *

# TODO: Change the following definitions to match your device.
DEFAULT_VENDOR_ID = 0x0
DEFAULT_PRODUCT_ID = 0x0

# WinDriver license registration string
# TODO: When using a registered WinDriver version, replace the license string
#         below with the development license in order to use on the development
#         machine.
#         Once you require to distribute the driver's package to other machines,
#         please replace the string with a distribution license
DEFAULT_LICENSE_STRING = "12345abcde1234.license"

# TODO: Change the following definition to your driver's name
DEFAULT_DRIVER_NAME  = WD_DEFAULT_DRIVER_NAME_BASE

INFINITE              = 0xffffffff
ATTACH_EVENT_TIMEOUT  = 30     # in seconds
TRANSFER_TIMEOUT      = 30000  # in msecs
WD_PYTHON_USB_DEVICES = 256

class DEVICE_CONTEXT(Structure): _fields_ = \
    [("hDevice", WDU_DEVICE_HANDLE),
    ("dwVendorId", DWORD),
    ("dwProductId", DWORD),
    ("dwInterfaceNum", DWORD),
    ("dwAlternateSetting", DWORD)]

class DRIVER_CONTEXT(Structure): _fields_ = \
    [("hEvent", HANDLE),
    ("hMutex", HANDLE),
    ("dwDeviceCount", DWORD),
    ("dwActiveDev", DWORD),
    ("hDeviceUnusedEvent", HANDLE),
    ("DevCtxArray", (DEVICE_CONTEXT * WD_PYTHON_USB_DEVICES))]

class MENU_CTX_USB(object):
    def __init__(self, fStreamMode):
        self.fStreamMode = fStreamMode
        self.pDevice = None
        self.fSuspended = False

hDriver = WDU_DRIVER_HANDLE()
hDevice = WDU_DEVICE_HANDLE()

# Driver context must be global in order for Python not to free it during
# the program's run
DrvCtx = DRIVER_CONTEXT()

def DeviceAttach(hDevice, pDeviceInfo, pUserData):
    pDrvCtx = cast(pUserData, POINTER(DRIVER_CONTEXT))
    pActiveAltSetting = \
        pDeviceInfo.contents.pActiveInterface[0].contents.pActiveAltSetting
    dwInterfaceNum = pActiveAltSetting.contents.Descriptor.bInterfaceNumber
    dwAlternateSetting = pActiveAltSetting.contents.Descriptor.bAlternateSetting

    # NOTE: To change the alternate setting, call wdapi.WDU_SetInterface() here
    '''
    # TODO: Replace with the requested number:
    #dwAlternateSetting = %alternate_setting_number%

    dwAttachError = wdapi.WDU_SetInterface(hDevice, dwInterfaceNum,
        dwAlternateSetting)
    if dwAttachError:
        ERR("DeviceAttach: WDU_SetInterface() failed (num. %ld, alternate %ld) "
            "device 0x%lx. error 0x%lx (\"%s\")\n" % (dwInterfaceNum,
            dwAlternateSetting, hDevice, dwAttachError,
            Stat2Str(dwAttachError)))
        return False
    '''

    #Uncomment the following code to allow only one device per process
    '''
    wdapi.OsMutexLock(HANDLE(pDrvCtx.contents.hMutex))
    hasDevice = (pDrvCtx.contents.dwDeviceCount > 0)
    wdapi.OsMutexUnlock(HANDLE(pDrvCtx.contents.hMutex))
    if (hasDevice):
        TRACE("DeviceAttach: This process already has one device, giving this "
             "one up\n")
        return False
    '''

    TRACE("\nDeviceAttach: Received and accepted attach for vendor id 0x%x, "
        "product id 0x%x, interface %ld, device handle 0x%lx\n" %
        (pDeviceInfo.contents.Descriptor.idVendor,
        pDeviceInfo.contents.Descriptor.idProduct, dwInterfaceNum, hDevice))

    # Add our device to device array
    wdapi.OsMutexLock(HANDLE(pDrvCtx.contents.hMutex))

    for i in range(WD_PYTHON_USB_DEVICES):
        if not pDrvCtx.contents.DevCtxArray[i].dwVendorId and \
            not pDrvCtx.contents.DevCtxArray[i].dwProductId:
            break

    pDrvCtx.contents.DevCtxArray[i].hDevice = WDU_DEVICE_HANDLE(hDevice)
    pDrvCtx.contents.DevCtxArray[i].dwInterfaceNum = DWORD(dwInterfaceNum)
    pDrvCtx.contents.DevCtxArray[i].dwVendorId = \
        pDeviceInfo.contents.Descriptor.idVendor
    pDrvCtx.contents.DevCtxArray[i].dwProductId = \
        pDeviceInfo.contents.Descriptor.idProduct
    pDrvCtx.contents.DevCtxArray[i].dwAlternateSetting = \
        DWORD(dwAlternateSetting)

    pDrvCtx.contents.dwDeviceCount += 1
    wdapi.OsMutexUnlock(HANDLE(pDrvCtx.contents.hMutex))

    wdapi.OsEventSignal(HANDLE(pDrvCtx.contents.hEvent))

    # Accept control over this device
    return True

def DeviceDetach(hDevice, pUserData):
    pDrvCtx = cast(pUserData, POINTER(DRIVER_CONTEXT))
    bDetachActiveDev = False

    TRACE("\nDeviceDetach: Received detach for device handle 0x%lx\n" % hDevice)

    wdapi.OsMutexLock(pDrvCtx.contents.hMutex)

    #find device to remove
    for i in range(WD_PYTHON_USB_DEVICES):
        if pDrvCtx.contents.DevCtxArray[i].hDevice == hDevice:
            break
    if i == WD_PYTHON_USB_DEVICES:
        ERR("\nDeviceDetach: No device to remove")
        return

    if i == DrvCtx.dwActiveDev:
        bDetachActiveDev = True
        #find other device to activate if there is more than one connected
        if pDrvCtx.contents.dwDeviceCount > 1:
            for j in range(WD_PYTHON_USB_DEVICES):
                if pDrvCtx.contents.DevCtxArray[j].hDevice:
                    DrvCtx.dwActiveDev = j
        else:
            pDrvCtx.contents.dwActiveDev = 0

    pDrvCtx.contents.DevCtxArray[i] = DEVICE_CONTEXT()
    pDrvCtx.contents.dwDeviceCount -= 1
    wdapi.OsMutexUnlock(pDrvCtx.contents.hMutex)

    if bDetachActiveDev:
        # When hDeviceUnusedEvent is not signaled, hDevice is possibly in use,
        # and therefore the detach callback needs to wait on it until it is
        # certain that it cannot be used.
        # When it is signaled - hDevice is no longer used.
        if pDrvCtx.contents.dwDeviceCount <= 0:
            TRACE("No devices are connected.\nPlease connect a device or press "
                "CTRL+C to quit application")
        wdapi.OsEventWait(pDrvCtx.contents.hDeviceUnusedEvent, INFINITE)

def GetDevice():
    global DrvCtx
    global hDevice

    if not DrvCtx.DevCtxArray[DrvCtx.dwActiveDev]:
        ERR("GetDevice: Could not get active device\n")
        return WD_WINDRIVER_STATUS_ERROR
    
    wdapi.OsEventReset(HANDLE(DrvCtx.hDeviceUnusedEvent))

    wdapi.OsMutexLock(HANDLE(DrvCtx.hMutex))
    hDevice = WDU_DEVICE_HANDLE(DrvCtx.DevCtxArray[DrvCtx.dwActiveDev].hDevice)
    wdapi.OsMutexUnlock(HANDLE(DrvCtx.hMutex))
    return WD_STATUS_SUCCESS

def GetInterfaceAndAltSettings():
    (dwInterfaceNumber, dwStatus) = DIAG_InputNum("Please enter the interface"\
        " number (dec): ", False, sizeof(DWORD), 0, 0)
    if DIAG_INPUT_SUCCESS != dwStatus:
        return WD_INVALID_PARAMETER, 0, 0

    (dwAlternateSetting, dwStatus) = DIAG_InputNum("Please enter the"\
        " alternate setting index (dec): ", False, sizeof(DWORD), 0, 0)
    if DIAG_INPUT_SUCCESS != dwStatus:
        return WD_INVALID_PARAMETER, 0, 0

    return WD_STATUS_SUCCESS, dwInterfaceNumber, dwAlternateSetting

def MenuUsbEntryCb(pCbCtx):
    return GetDevice()


def MenuUsbExitCb(pCbCtx):
    global DrvCtx

    return wdapi.OsEventSignal(HANDLE(DrvCtx.hDeviceUnusedEvent))

# ---------------------------- #
# Print Device Configurations  #
# ---------------------------- #
def MenuPrintDeviceCfgsOptionCb(pCbCtx):
    global hDevice

    dwStatus = GetDevice()
    if dwStatus:
        return dwStatus
    
    dwStatus = USB_PrintDeviceConfigurations(hDevice, sys.stdout)
    if dwStatus:
        printf("MenuPrintDeviceCfgsOptionCb: %s", USB_GetLastErr())

    return dwStatus

# ----------------------------------- #
# Change interface alternate setting  #
# ----------------------------------- #
def MenuChangeInterfaceAltSettingOptionCb(pCbCtx):
    global DrvCtx
    global hDevice

    dwStatus = GetDevice()
    if dwStatus:
        return dwStatus

    dwStatus, dwInterfaceNumber, dwAlternateSetting = GetInterfaceAndAltSettings()
    if dwStatus:
        return dwStatus
 
    dwStatus = USB_SetInterface(hDevice, dwInterfaceNumber, dwAlternateSetting)
    if dwStatus:
        printf("MenuChangeInterfaceAltSettingOptionCb: %s", USB_GetLastErr())
    else:
        TRACE("MenuChangeInterfaceAltSettingOptionCb: "
                    "USB_SetInterface() completed successfully\n")
        DrvCtx.DevCtxArray[DrvCtx.dwActiveDev].dwInterfaceNum = dwInterfaceNumber
        DrvCtx.DevCtxArray[DrvCtx.dwActiveDev].dwAlternateSetting = \
            dwAlternateSetting

    return dwStatus

# ------------ #
# Reset Pipe #
# ------------ #
def MenuResetPipeOptionCb(pCbCtx):
    global hDevice

    dwStatus = GetDevice()
    if dwStatus:
        return dwStatus

    dwStatus = USB_PrintDevicePipesInfoByHandle(hDevice, sys.stdout)
    if dwStatus:
        printf("MenuResetPipeOptionCb: %s", USB_GetLastErr())
        return dwStatus

    (dwPipeNum, dwStatus) = DIAG_InputNum("Please enter the" \
        " pipe number (hex): ", True, sizeof(DWORD), 0, 0)
    if DIAG_INPUT_SUCCESS != dwStatus:
        return dwStatus

    print("")

    dwStatus = USB_ResetPipe(hDevice, dwPipeNum)
    if dwStatus:
        printf("MenuResetPipeOptionCb: %s", USB_GetLastErr())
        return dwStatus
   
    print("MenuResetPipeOptionCb: WDU_ResetPipe() completed successfully\n")
    return dwStatus

# --------------- #
# Read/Write Pipe #
# --------------- #
def MenuRwPipeGetPipeNum(pDevice):
    (dwPipeNum, dwStatus) = DIAG_InputNum("Please enter the" \
        " pipe number (hex): ", True, sizeof(DWORD), 0, 0)
    if DIAG_INPUT_SUCCESS != dwStatus:
        return WD_INVALID_PARAMETER, None, dwPipeNum

    dwPipeNum = dwPipeNum.value
    # Search for the pipe
    pPipe = USB_FindPipeInDevice(pDevice, dwPipeNum)
    if not pPipe:
        print("The pipe number 0x%lx does not exist. Please try again." %
            dwPipeNum)
        dwStatus = WD_INVALID_PARAMETER
    else:
        dwStatus = WD_STATUS_SUCCESS

    return dwStatus, pPipe, dwPipeNum

# Read/Write pipe
def MenuRwPipeReadWrite(pMenuCtx, fRead):
    global hDevice

    dwBufferSize =  0x20000
    dwBytesTransferred = DWORD()

    dwStatus, pPipe, dwPipeNum = MenuRwPipeGetPipeNum(pMenuCtx.pDevice)
    if dwStatus:
        return dwStatus

    if (not dwPipeNum) or pPipe.type == PIPE_TYPE_CONTROL:
        if pMenuCtx.fStreamMode:
            ERR("Cannot perform stream transfer using control pipe.\n"
                "please switch to Single Blocking Transfer mode "
                "(option 6) or change the pipe number")
            return WD_INVALID_PARAMETER

        print("Please enter setup packet (hex - 8 bytes): ")
        (sInput, bytesRead) = DIAG_GetHexBuffer(8)
        SetupPacket = (c_char * len(sInput)).from_buffer(sInput)
    else:
        SetupPacket = create_string_buffer(8)

    (dwSize, dwStatus) = DIAG_InputNum("Please enter the size of the"\
        " buffer (dec): ", False, sizeof(DWORD), 0, 0)
    if DIAG_INPUT_SUCCESS != dwStatus:
        return dwStatus

    dwSize = dwSize.value
    pBuffer = create_string_buffer(dwSize)

    if dwSize:
        if not fRead:
            print("Please enter the input buffer (hex): ")
            (sInput, bytesRead) = DIAG_GetHexBuffer(dwSize)
            pBuffer = (c_char * len(sInput)).from_buffer(sInput)

    if pMenuCtx.fStreamMode:
        dwStatus = USB_ReadWriteStream(hDevice, dwPipeNum, pBuffer,
            dwBufferSize, dwSize, byref(dwBytesTransferred), fRead)
    else:
        dwStatus = USB_ReadWriteTransfer(hDevice, dwPipeNum, pBuffer, dwSize,
            SetupPacket, byref(dwBytesTransferred), fRead)

    if dwStatus:
        print("MenuRwPipeReadWrite: %s" % USB_GetLastErr())
    else:
        print("Transferred %ld bytes\n" % dwBytesTransferred.value)
        if fRead and pBuffer:
            DIAG_PrintHexBuffer(pBuffer, dwBytesTransferred)

    return dwStatus

def MenuRwPipeReadOptionCb(pCbCtx):
    return MenuRwPipeReadWrite(pCbCtx, True)

def MenuRwPipeWriteOptionCb(pCbCtx):
    return MenuRwPipeReadWrite(pCbCtx, False)

# Listen/Measure Pipe
def MenuRwPipeListenMeasure(pMenuCtx, fListen):
    global hDevice

    dwBufferSize = 0x20000

    dwStatus, pPipe, dwPipeNum = MenuRwPipeGetPipeNum(pMenuCtx.pDevice)
    if dwStatus:
        return dwStatus

    if (not dwPipeNum) or pPipe.type == PIPE_TYPE_CONTROL:
        printf("Cannot listen to control pipes.\n")
        return WD_INVALID_PARAMETER
    
    dwStatus = USB_ListenToPipe(hDevice, pPipe, pMenuCtx.fStreamMode,
        dwBufferSize, True, fListen)

    if dwStatus:
        print("MenuRwPipeListenMeasure: %s" % USB_GetLastErr())

    return dwStatus


def MenuRwPipeListenOptionCb(pCbCtx):
    return MenuRwPipeListenMeasure(pCbCtx, True)


def MenuRwPipeMeasureOptionCb(pCbCtx):
    return MenuRwPipeListenMeasure(pCbCtx, False)

# Check Read/Write Stream
def MenuRwPipeIsStreamingNotActivated(pMenu):
    return not pMenu.pCbCtx.fStreamMode

PERF_STREAM_BUFFER_SIZE      = 5120000 # In bytes
PERF_DEVICE_TRANSFER_SIZE    = 256*1024   # In bytes
PERF_TRANSFER_ITERATIONS     = 1500

def MenuRwPipeCheckStreamReadWrite(pMenuCtx, fRead):
    global hDevice

    dwStatus, pPipe, dwPipeNum = MenuRwPipeGetPipeNum(pMenuCtx.pDevice)
    if dwStatus:
        return dwStatus

    if (not dwPipeNum) or pPipe.type == PIPE_TYPE_CONTROL:
        printf("Cannot perform stream transfer with control pipe\n")
        return WD_INVALID_PARAMETER

    print("The size of the buffer to transfer(dec): %d" %
        PERF_DEVICE_TRANSFER_SIZE)
    print("The size of the internal Rx/Tx stream buffer (dec): %d" %
        PERF_STREAM_BUFFER_SIZE)
    print("Making the transfer of %d times the buffer size, please "
        "wait ..." % PERF_TRANSFER_ITERATIONS)

    pBuffer = create_string_buffer(PERF_DEVICE_TRANSFER_SIZE)
    if not fRead:
        pass
        # Here you can fill pBuffer with the right data for the
        # board

    dwStatus = USB_ReadWriteStreamCheck(hDevice, dwPipeNum,
        pBuffer , PERF_STREAM_BUFFER_SIZE, PERF_DEVICE_TRANSFER_SIZE,
        PERF_TRANSFER_ITERATIONS, fRead)

    if dwStatus:
        print("MenuRwPipeIsStreamingNotActivated: %s" % USB_GetLastErr())

    return dwStatus

def MenuRwPipeCheckStreamReadOptionCb(pCbCtx):
    return MenuRwPipeCheckStreamReadWrite(pCbCtx, True)

def MenuRwPipeCheckStreamWriteOptionCb(pCbCtx):
    return MenuRwPipeCheckStreamReadWrite(pCbCtx, False)

def MenuRwPipeSwitchTransferTypeOptionCb(pCbCtx):
    pCbCtx.fStreamMode = not pCbCtx.fStreamMode
    return WD_STATUS_SUCCESS

def MenuRwPipeOptionsInit(pParentMenu, pMenuCtx):
    options = [
        DIAG_MENU_OPTION (
            cOptionName = "Read from pipe",
            cbEntry = MenuRwPipeReadOptionCb,
            cbExit = MenuUsbExitCb ),

        DIAG_MENU_OPTION (
            cOptionName = "Write from pipe",
            cbEntry = MenuRwPipeWriteOptionCb,
            cbExit = MenuUsbExitCb ),

        DIAG_MENU_OPTION (
            cOptionName = "Listen to pipe (continuous read)",
            cbEntry = MenuRwPipeListenOptionCb,
            cbExit = MenuUsbExitCb ),

        DIAG_MENU_OPTION (
            cOptionName = "Measure pipe speed (continuous read)",
            cbEntry = MenuRwPipeMeasureOptionCb,
            cbExit = MenuUsbExitCb )
        ]
    if sys.platform == "win32":
        options += [
            DIAG_MENU_OPTION (
                cOptionName = "Check streaming READ speed",
                cbEntry = MenuRwPipeCheckStreamReadOptionCb,
                cbIsHidden = MenuRwPipeIsStreamingNotActivated,
                cbExit = MenuUsbExitCb ),

            DIAG_MENU_OPTION (
                cOptionName = "Check streaming WRITE speed",
                cbEntry = MenuRwPipeCheckStreamWriteOptionCb,
                cbIsHidden = MenuRwPipeIsStreamingNotActivated,
                cbExit = MenuUsbExitCb ),

            DIAG_MENU_OPTION (
                cOptionName = "Switch transfer mode",
                cbEntry = MenuRwPipeSwitchTransferTypeOptionCb,
                cbExit = MenuUsbExitCb )
        ]

    DIAG_MenuSetCtxAndParentForMenus(options, pMenuCtx, pParentMenu)

def MenuRwPipeEntryCb(pCbCtx):
    global hDevice

    dwStatus = GetDevice()
    if dwStatus:
        return dwStatus

    if not pCbCtx.pDevice:
        pCbCtx.pDevice = POINTER(WDU_DEVICE)(WDU_DEVICE())
        dwStatus = USB_GetDeviceInfo(hDevice, byref(pCbCtx.pDevice))
        if (dwStatus):
            print(USB_GetLastError())
            return dwStatus
        
        USB_PrintDevicePipesInfo(pCbCtx.pDevice, sys.stdout)

    print("")
    print("Read/Write from/to device's pipes using %s" %(
        "Streaming Data Transfers" if pCbCtx.fStreamMode\
        else "Single Blocking Transfers"))
    print("---------------------")

    return dwStatus


def MenuFastStreamingReadOptionCb(pCbCtx):
    global DrcCtx

    Device = WDU_DEVICE()
    pDevice = POINTER(WDU_DEVICE)(Device)
    dwBufferSize = 0x20000

    dwStatus = GetDevice()
    if dwStatus:
        return dwStatus

    dwStatus, dwInterfaceNumber, dwAlternateSetting = GetInterfaceAndAltSettings()
    if dwStatus:
        return dwStatus

    (dwPipeNum, dwStatus) = DIAG_InputNum("Please enter the" \
        " pipe number (hex): ", True, sizeof(DWORD), 0, 0)
    if DIAG_INPUT_SUCCESS != dwStatus:
        return WD_INVALID_PARAMETER

    dwStatus = USB_SetInterface(hDevice, dwInterfaceNumber, dwAlternateSetting)
    if dwStatus:
        print("MenuFastStreamingReadOptionCb: %s" % USB_GetLastErr())
        return dwStatus
    else:
        TRACE("MenuFastStreamingReadOptionCb: USB_SetInterface() completed "
            "successfully")
        DrvCtx.DevCtxArray[DrvCtx.dwActiveDev].dwInterfaceNum = \
            dwInterfaceNumber
        DrvCtx.DevCtxArray[DrvCtx.dwActiveDev].dwAlternateSetting = \
            dwAlternateSetting

    dwStatus = USB_GetDeviceInfo(hDevice, byref(pDevice))
    if dwStatus:
        print("MenuFastStreamingReadOptionCb: %s" % USB_GetLastErr())
        return dwStatus

    # Search for the pipe
    pPipe = USB_FindPipeInDevice(pDevice, dwPipeNum.value)
    if not pPipe:
        print("MenuFastStreamingReadOptionCb %s" % USB_GetLastError())
        return WD_INVALID_PARAMETER

    if not dwPipeNum or pPipe.type == PIPE_TYPE_CONTROL:
        ERR("MenuFastStreamingReadOptionCb: Cannot listen to control pipes")
        return WD_INVALID_PARAMETER

    dwStatus = USB_ListenToPipe(hDevice, pPipe, True, dwBufferSize, False,
        True)
    if dwStatus:
       print("MenuFastStreamingReadOptionCb %s" % USB_GetLastError())

    return dwStatus

# --------------- #
# Select Device  #
# --------------- #
def MenuIsAtMostOneDeviceOpen(pMenu):
    global DrvCtx

    wdapi.OsMutexLock(HANDLE(DrvCtx.hMutex))
    dwDeviceCount = DrvCtx.dwDeviceCount
    wdapi.OsMutexUnlock(HANDLE(DrvCtx.hMutex))

    return dwDeviceCount <= 1

def MenuSelectDeviceOptionCb(pCbCtx):
    global DrvCtx
    
    dwStatus = GetDevice()
    if dwStatus:
        return dwStatus
  
    wdapi.OsMutexLock(HANDLE(DrvCtx.hMutex))
    availableDevs = []
    for i in range(WD_PYTHON_USB_DEVICES):
        if DrvCtx.DevCtxArray[i].dwVendorId and \
            DrvCtx.DevCtxArray[i].dwProductId:
            print("  %ld. Vendor id: 0x%lx, Product id: 0x%lx, "
                "Interface number: %ld, Alt. Setting: %ld" % (i + 1,
                DrvCtx.DevCtxArray[i].dwVendorId,
                DrvCtx.DevCtxArray[i].dwProductId,
                DrvCtx.DevCtxArray[i].dwInterfaceNum,
                DrvCtx.DevCtxArray[i].dwAlternateSetting))
            availableDevs.append(i + 1)

    (dwDeviceNum, dwStatus) = DIAG_InputNum("Please enter the "
        "device number (1 - %ld, dec): " % max(availableDevs), \
        False, sizeof(DWORD), 1, max(availableDevs))
    if DIAG_INPUT_SUCCESS != dwStatus:
        wdapi.OsMutexUnlock(HANDLE(DrvCtx.hMutex))
        return WD_INVALID_PARAMETER

    if dwDeviceNum.value not in availableDevs:
        wdapi.OsMutexUnlock(HANDLE(DrvCtx.hMutex))
        print("Invalid selection")
        return WD_INVALID_PARAMETER

    DrvCtx.dwActiveDev = DWORD(dwDeviceNum.value - 1)
    pCbCtx.pDevice = None

    wdapi.OsMutexUnlock(HANDLE(DrvCtx.hMutex))

# ------------------ #
# Selective Suspend  #
# ------------------ #
def MenuIsSuspended(pMenu):
    return pMenu.pCbCtx.fSuspended

def MenuIsNotSuspended(pMenu):
    return not pMenu.pCbCtx.fSuspended

def MenuSelectiveSuspendOptionCb(pCbCtx):
    global hDevice

    dwStatus = USB_SelectiveSuspend(hDevice,
        WDU_SELECTIVE_SUSPEND_CANCEL if pCbCtx.fSuspended else
        WDU_SELECTIVE_SUSPEND_SUBMIT)

    if (dwStatus):
        print("MenuSelectiveSuspendOptionCb: %s" % USB_GetLastErr())
    else:
        pCbCtx.fSuspended = not pCbCtx.fSuspended

    return dwStatus

def MenuSelectiveSuspendOptionsInit(pParentMenu, pMenuCtx):
    options = [
        DIAG_MENU_OPTION (
            cOptionName = "Enter suspend mode",
            cbEntry = MenuSelectiveSuspendOptionCb,
            cbIsHidden = MenuIsSuspended ),

        DIAG_MENU_OPTION (
            cOptionName = "Leave suspend mode",
            cbEntry = MenuSelectiveSuspendOptionCb,
            cbIsHidden = MenuIsNotSuspended )
        ]

    DIAG_MenuSetCtxAndParentForMenus(options, pMenuCtx, pParentMenu)

# ---------------------------- #
# Print Device Serial Number   #
# ---------------------------- #
def MenuPrintDeviceSerialNumberOptionCb(pCbCtx):
    global hDevice

    dwStatus = GetDevice()
    if dwStatus:
        return dwStatus
    
    dwStatus = USB_PrintDeviceSerialNumberByHandle(hDevice, sys.stdout)
    if dwStatus:
        print("MenuPrintDeviceSerialNumberOptionCb: %s" % USB_GetLastErr())

    return dwStatus

# ---------------------------- #
# Print Device Properties      #
# ---------------------------- #
def MenuPrintDeviceInformationOptionCb(pCbCtx):
    global hDevice

    dwStatus = GetDevice()
    if dwStatus:
        return dwStatus

    USB_PrintDeviceProperties(hDevice, sys.stdout)
    
    return WD_STATUS_SUCCESS


def MenuMainEntryCb(pCbCtx):
    global DrvCtx

    while True:
        if not DrvCtx.dwDeviceCount:
            print("No Devices are currently connected.")
            if sys.platform == "win32":
                print("Please make sure that you've generated and installed an "
                    "INF file for this device using WinDriver DriverWizard.")
            print("Press Enter to re check or enter EXIT to exit")
            line = inputf()

            if line == "EXIT":
                return WD_WINDRIVER_STATUS_ERROR

        dwActiveDev = DrvCtx.dwActiveDev
        wdapi.OsMutexLock(HANDLE(DrvCtx.hMutex))

        if not DrvCtx.dwDeviceCount:
            wdapi.OsMutexUnlock(HANDLE(DrvCtx.hMutex))
            continue
        break

    print("")
    print("Main Menu (active Dev/Prod/Interface/Alt. Setting: "
        "0x%lx/0x%lx/%ld/%ld)" % \
        (DrvCtx.DevCtxArray[dwActiveDev].dwVendorId,
        DrvCtx.DevCtxArray[dwActiveDev].dwProductId,
        DrvCtx.DevCtxArray[dwActiveDev].dwInterfaceNum,
        DrvCtx.DevCtxArray[dwActiveDev].dwAlternateSetting))
    print("----------")

    wdapi.OsMutexUnlock(HANDLE(DrvCtx.hMutex))
    return WD_STATUS_SUCCESS

def MenuMainInit(pUsbMenuCtx):
    menuRoot = DIAG_MENU_OPTION(cbEntry = MenuMainEntryCb)

    options = [
        DIAG_MENU_OPTION (
            cOptionName = "Display device configurations",
            cbEntry = MenuPrintDeviceCfgsOptionCb,
            cbExit = MenuUsbExitCb ),

        DIAG_MENU_OPTION (
            cOptionName = "Change interface alternate setting",
            cbEntry = MenuChangeInterfaceAltSettingOptionCb,
            cbExit = MenuUsbExitCb ),
        
        DIAG_MENU_OPTION (
            cOptionName = "Reset Pipe",
            cbEntry = MenuResetPipeOptionCb,
            cbExit = MenuUsbExitCb ),
        
        DIAG_MENU_OPTION (
            cOptionName = "Read/Write from pipes",
            cbEntry = MenuRwPipeEntryCb,
            cbExit = MenuUsbExitCb )
        ]

    MENU_RW_PIPE_INDEX = 3
    MenuRwPipeOptionsInit(options[MENU_RW_PIPE_INDEX], pUsbMenuCtx)
    
    # Add additional options in Windows
    if sys.platform == "win32":
        options += [
            DIAG_MENU_OPTION(
                cOptionName = "Fast streaming read",
                cbEntry = MenuFastStreamingReadOptionCb,
                cbExit = MenuUsbExitCb ),

            DIAG_MENU_OPTION(
                cOptionName = "Selective Suspend",
                cTitleName = "Toggle suspend mode",
                cbEntry = MenuUsbEntryCb,
                cbExit = MenuUsbExitCb )
            ]
        MenuSelectiveSuspendOptionsInit(options[len(options) - 1], pUsbMenuCtx)

    options += [
            DIAG_MENU_OPTION (
                cOptionName = "Select Device",
                cbEntry = MenuSelectDeviceOptionCb,
                cbIsHidden = MenuIsAtMostOneDeviceOpen,
                cbExit = MenuUsbExitCb ),
            DIAG_MENU_OPTION (
                cOptionName = "Display device serial number",
                cbEntry = MenuPrintDeviceSerialNumberOptionCb,
                cbExit = MenuUsbExitCb ),
        
            DIAG_MENU_OPTION (
                cOptionName = "Display device information",
                cbEntry = MenuPrintDeviceInformationOptionCb,
                cbExit = MenuUsbExitCb ),
        
            DIAG_MENU_OPTION (
                cOptionName = "Refresh",
                # No real entry callback, refresh does nothing
                cbExit = MenuUsbExitCb )
        ]

    DIAG_MenuSetCtxAndParentForMenus(options, pUsbMenuCtx, menuRoot)
    return menuRoot

eventTable = WDU_EVENT_TABLE()
def DriverInit(matchTables, dwNumMatchTables, sDriverName, sLicense, DrvCtx):
    global eventTable

    # Set Driver Name
    if not wdapi.WD_DriverName(sDriverName):
        ERR("DriverInit: Failed setting driver name to %s, exiting\n" %
            sDriverName)
        return WD_SYSTEM_INTERNAL_ERROR

    dwStatus = wdapi.OsEventCreate(byref(DrvCtx, DRIVER_CONTEXT.hEvent.offset))
    if dwStatus:
        ERR("DriverInit: OsEventCreate() failed on event 0x%lx. error 0x%lx "
            "(\"%s\")\n" % (DrvCtx.hEvent, dwStatus, Stat2Str(dwStatus)))
        return dwStatus

    dwStatus = wdapi.OsMutexCreate(byref(DrvCtx, DRIVER_CONTEXT.hMutex.offset))
    if dwStatus:
        ERR("DriverInit: OsMutexCreate() failed on mutex 0x%lx. error 0x%lx "
            "(\"%s\")\n" % (DrvCtx.hMutex, dwStatus, Stat2Str(dwStatus)))
        return dwStatus

    dwStatus = wdapi.OsEventCreate(byref(DrvCtx,
        DRIVER_CONTEXT.hDeviceUnusedEvent.offset))
    if dwStatus:
        ERR("DriverInit: OsEventCreate() failed on event 0x%lx. error 0x%lx "
            "(\"%s\")\n" % (DrvCtx.hDeviceUnusedEvent, dwStatus,
            Stat2Str(dwStatus)))
        return dwStatus

    wdapi.OsEventSignal(HANDLE(DrvCtx.hDeviceUnusedEvent))

    eventTable.pfDeviceAttach = WDU_ATTACH_CALLBACK(DeviceAttach)
    eventTable.pfDeviceDetach = WDU_DETACH_CALLBACK(DeviceDetach)
    eventTable.pUserData = PVOID(addressof(DrvCtx))

    dwStatus = wdapi.WDU_Init(byref(hDriver), byref(matchTables),
        dwNumMatchTables, byref(eventTable), sLicense, WD_ACKNOWLEDGE)
    if dwStatus:
        ERR("DriverInit: Failed to initialize USB driver. error 0x%lx "
            "(\"%s\")\n" % (dwStatus, Stat2Str(dwStatus)))
        return dwStatus

    return WD_STATUS_SUCCESS

def DriverUninit(DrvCtx):
    if DrvCtx.hEvent:
        wdapi.OsEventClose(HANDLE(DrvCtx.hEvent))
    if DrvCtx.hMutex:
        wdapi.OsMutexClose(HANDLE(DrvCtx.hMutex))
    if DrvCtx.hDeviceUnusedEvent:
        wdapi.OsEventClose(HANDLE(DrvCtx.hDeviceUnusedEvent))
    if hDriver:
        wdapi.WDU_Uninit(hDriver)

def main_Exit(dwStatus, DrvCtx):
    DriverUninit(DrvCtx)
    return dwStatus

def USB_Init():
    fUseDefault = False
    matchTable = WDU_MATCH_TABLE()
    global DrvCtx

    wdapi_va.PrintDbgMessage(D_ERROR, S_USB, "WinDriver user mode version %s\n"\
        % WD_VERSION_STR)

    #if defined(USB_DIAG_SAMPLE)
    # Get vendor ID
    (wVendorId, dwStatus) = DIAG_InputNum("Enter vendor ID (Default = 0x%x):" %
        DEFAULT_VENDOR_ID, True, sizeof(WORD), 0, 0)
    if DIAG_INPUT_SUCCESS != dwStatus:
        fUseDefault = True

    # Get device ID
    (wProductId, dwStatus) = DIAG_InputNum("Enter device ID (Default = 0x%x):" %
        DEFAULT_PRODUCT_ID, True, sizeof(WORD), 0, 0)
    if DIAG_INPUT_SUCCESS != dwStatus:
        fUseDefault = True
    #endif

    # Use defaults
    if fUseDefault:
        wVendorId = DEFAULT_VENDOR_ID
        wProductId = DEFAULT_PRODUCT_ID

    matchTable.wVendorId = wVendorId
    matchTable.wProductId = wProductId

    dwStatus = DriverInit(matchTable, 1, DEFAULT_DRIVER_NAME,
        DEFAULT_LICENSE_STRING, DrvCtx)

    if dwStatus:
        return dwStatus

    print("Please make sure the device is attached:\n")
    print("(The application is waiting for device attachment...)\n")
    # Wait for the device to be attached
    dwStatus = wdapi.OsEventWait(HANDLE(DrvCtx.hEvent), ATTACH_EVENT_TIMEOUT)
    if dwStatus:
        if dwStatus == WD_TIME_OUT_EXPIRED:
            ERR("Timeout expired for connection with the device.\n"
                "Check that the device is connected and try again.\n")
        else:
            ERR("main: wdapi.OsEventWait() failed on event 0x%lx. error 0x%lx "
                "(\"%s\")\n" % (DrvCtx.hEvent, dwStatus, Stat2Str(dwStatus)))
            return dwStatus

def main():
    global DrvCtx

    usbMenuCtx = MENU_CTX_USB(fStreamMode = sys.platform == "win32")
    pMenuRoot = MenuMainInit(usbMenuCtx)

    dwStatus = USB_Init()
    if dwStatus:
        ERR("main: USB_Init() failed. error 0x%lx (\"%s\")\n" % 
             (dwStatus, Stat2Str(dwStatus)))
        return main_Exit(dwStatus, DrvCtx)

    dwStatus = DIAG_MenuRun(pMenuRoot)
    return main_Exit(dwStatus, DrvCtx)

if __name__ == "__main__":
    main()


