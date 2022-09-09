''' Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com '''

from .windrvr_usb import *
from .wdc_lib import *

WDU_DRIVER_HANDLE = PVOID
WDU_DEVICE_HANDLE = PVOID
WDU_STREAM_HANDLE = PVOID
WDU_LANGID = WORD

# WinDriver USB API Declarations & Implementations
#
# User Callbacks
#
# Function typedef: WDU_ATTACH_CALLBACK()
#   WinDriver calls this function with any new device that is attached,
#   matches the given criteria, and if WD_ACKNOWLEDGE was passed to WDU_Init()
#   in dwOptions - not controlled yet by another driver.
#   This callback is called once for each matching interface.
# Parameters:
#   [in] hDevice:     A unique identifier for the device/interface
#   [in] pDeviceInfo: Pointer to device configuration details.
#                     This pointer is valid until the end of the function.
#   [in] pUserData:   Pointer to user data that was passed to WDU_Init (in the
#                     event table).
# Return Value:
#   If WD_ACKNOWLEDGE was passed to WDU_Init(), the implementor should check &
#   return if he wants to control the device.
WDU_ATTACH_CALLBACK = DLLCALLCONV_USB(BOOL, WDU_DEVICE_HANDLE, POINTER(WDU_DEVICE),
    PVOID)
# Corresponds to C function type:
#typedef BOOL (DLLCALLCONV *WDU_ATTACH_CALLBACK)(WDU_DEVICE_HANDLE hDevice,
#    WDU_DEVICE *pDeviceInfo, PVOID pUserData);

#
# Function typedef: WDU_DETACH_CALLBACK()
#   WinDriver calls this function when a controlled device has been detached
#   from the system.
# Parameters:
#   [in] hDevice:      A unique identifier for the device/interface.
#   [in] pUserData:    Pointer to user data that was passed to WDU_Init.
# Return Value:
#   None.
#    pDevToRemove = DRIVER_CONTEXT()
WDU_DETACH_CALLBACK = DLLCALLCONV_USB(None, WDU_DEVICE_HANDLE, PVOID)
# Corresponds to C function type:
#typedef void (DLLCALLCONV *WDU_DETACH_CALLBACK)(WDU_DEVICE_HANDLE hDevice,
#    PVOID pUserData);

#
# Function typedef: WDU_POWER_CHANGE_CALLBACK()
# Parameters:
#   [in] hDevice:      A unique identifier for the device/interface.
#   [in] dwPowerState: Number of the power state selected.
#   [in] pUserData:    Pointer to user data that was passed to WDU_Init (in the
#                      event table).
# Return Value:
#   TRUE/FALSE; Currently there is no significance to the return value.
WDU_POWER_CHANGE_CALLBACK = DLLCALLCONV_USB(BOOL, WDU_DEVICE_HANDLE, DWORD, PVOID)
# Corresponds to C function type:
#typedef BOOL (DLLCALLCONV *WDU_POWER_CHANGE_CALLBACK)(WDU_DEVICE_HANDLE hDevice,
#    DWORD dwPowerState, PVOID pUserData);

# Structures

WDU_DEVLIST_TIMEOUT     = 30    # In seconds
WDU_STREAM_LIST_TIMEOUT = 5     # In seconds
WDU_TRANSFER_TIMEOUT    = 30000 # In msecs

class WDU_EVENT_TABLE(Structure): _fields_ = \
    [("pfDeviceAttach", WDU_ATTACH_CALLBACK),
    ("pfDeviceDetach", WDU_DETACH_CALLBACK),
    ("pfPowerChange", WDU_POWER_CHANGE_CALLBACK),
    ("pUserData", PVOID)]  # pointer to pass in each callback

class WDU_STREAM_CONTEXT(Structure): _fields_ = \
    [("hDevice", WDU_DEVICE_HANDLE),
    ("hWD", HANDLE),
    ("dwPipeNum", DWORD)]

class WDU_STREAM_LIST_ITEM(Structure):
    pass

WDU_STREAM_LIST_ITEM._fields_ = \
    [("next", POINTER(WDU_STREAM_LIST_ITEM)),
    ("pStreamCtx", POINTER(WDU_STREAM_CONTEXT))]

class WDU_STREAM_LIST(Structure): _fields_ = \
    [("pHead", POINTER(WDU_STREAM_LIST_ITEM)),
    ("hEvent", HANDLE),
    ("iRefCount", c_int)]

class DRIVER_CTX(Structure): _fields_ = \
    [("hWD", HANDLE),
    ("EventTable", WDU_EVENT_TABLE),
    ("hEvents", HANDLE)]

class DEVICE_CTX(Structure): _fields_ = \
    [("pDriverCtx", POINTER(DRIVER_CTX)),
    ("pDevice", POINTER(WDU_DEVICE)), # Not fixed size => ptr
    ("dwUniqueID", DWORD),
    ("StreamList", WDU_STREAM_LIST)]

class WDU_DEVICE_LIST_ITEM(Structure):
    pass

WDU_DEVICE_LIST_ITEM._fields_ = \
    [("next", POINTER(WDU_DEVICE_LIST_ITEM)),
    ("pDeviceCtx", POINTER(DEVICE_CTX))]

class WDU_DEVICE_LIST(Structure): _fields_ = \
    [("pHead", POINTER(WDU_DEVICE_LIST_ITEM)),
    ("hEvent", HANDLE), # Not fixed size => ptr
    ("iRefCount", c_int)]

DevList = WDU_DEVICE_LIST()  # Global devices list

wdapi.WDU_Transfer.argtypes = [WDU_DEVICE_HANDLE, DWORD, DWORD, DWORD, PVOID, \
    DWORD, POINTER(DWORD), POINTER(CHAR), DWORD]


