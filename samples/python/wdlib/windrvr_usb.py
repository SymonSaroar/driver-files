''' Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com '''

from .windrvr import *

if "linux" in sys.platform:
    PRCHANDLE = c_int

# USB_PIPE_TYPE
PIPE_TYPE_CONTROL     = 0
PIPE_TYPE_ISOCHRONOUS = 1
PIPE_TYPE_BULK        = 2
PIPE_TYPE_INTERRUPT   = 3

WD_USB_MAX_PIPE_NUMBER  = 32
WD_USB_MAX_ENDPOINTS    = WD_USB_MAX_PIPE_NUMBER
WD_USB_MAX_INTERFACES   = 30
WD_USB_MAX_ALT_SETTINGS = 255

#WDU_DIR
WDU_DIR_IN     = 1
WDU_DIR_OUT    = 2
WDU_DIR_IN_OUT = 3

# USB TRANSFER options

USB_ISOCH_RESET             = 0x10
USB_ISOCH_FULL_PACKETS_ONLY = 0x20
# Windows only, ignored on other OS:
USB_ABORT_PIPE                      = 0x40
USB_ISOCH_NOASAP                    = 0x80
USB_BULK_INT_URB_SIZE_OVERRIDE_128K = 0x100 # Force a 128KB maximum
                                            # URB size
# All OS
USB_STREAM_OVERWRITE_BUFFER_WHEN_FULL = 0x200,

# The following flags are no longer used beginning with v6.0:
USB_TRANSFER_HALT  = 0x1
USB_SHORT_TRANSFER = 0x2
USB_FULL_TRANSFER  = 0x4
USB_ISOCH_ASAP     = 0x8

WDU_REGISTER_DEVICES_HANDLE = PVOID

# Descriptor types
WDU_DEVICE_DESC_TYPE      = 0x01
WDU_CONFIG_DESC_TYPE      = 0x02
WDU_STRING_DESC_STRING    = 0x03
WDU_INTERFACE_DESC_TYPE   = 0x04
WDU_ENDPOINT_DESC_TYPE    = 0x05

# Endpoint descriptor fields
WDU_ENDPOINT_TYPE_MASK      = 0x03
WDU_ENDPOINT_DIRECTION_MASK = 0x80
WDU_ENDPOINT_ADDRESS_MASK   = 0x0f
# test direction bit in the bEndpointAddress field of an endpoint
# descriptor.

def WDU_ENDPOINT_DIRECTION_OUT(addr):
    return not ((addr) & WDU_ENDPOINT_DIRECTION_MASK)
def WDU_ENDPOINT_DIRECTION_IN(addr):
    return not ((addr) & WDU_ENDPOINT_DIRECTION_MASK)
def WDU_GET_MAX_PACKET_SIZE(x):
    return USHORT(((x) & 0x7ff) * (1 + (((x) & 0x1800) >> 11)))

if "linux" not in sys.platform:
    #USB_DIR
    USB_DIR_IN     = 1
    USB_DIR_OUT    = 2
    USB_DIR_IN_OUT = 3

class WDU_PIPE_INFO(Structure): _fields_ = \
    [("dwNumber", DWORD),  # Pipe 0 is the default pipe
    ("dwMaximumPacketSize", DWORD),
    ("type", DWORD),       # USB_PIPE_TYPE
    ("direction", DWORD),  # WDU_DIR
                           # Isochronous, Bulk, Interrupt are either USB_DIR_IN
                           # or USB_DIR_OUT. Control are USB_DIR_IN_OUT
    ("dwInterval", DWORD)] # interval in ms relevant to Interrupt pipes

class WDU_INTERFACE_DESCRIPTOR(Structure): _fields_ = \
    [("bLength", UCHAR),
    ("bDescriptorType", UCHAR),
    ("bInterfaceNumber", UCHAR),
    ("bAlternateSetting", UCHAR),
    ("bNumEndpoints", UCHAR),
    ("bInterfaceClass", UCHAR),
    ("bInterfaceSubClass", UCHAR),
    ("bInterfaceProtocol", UCHAR),
    ("iInterface", UCHAR)]

class WDU_ENDPOINT_DESCRIPTOR(Structure): _fields_ = \
    [("bLength", UCHAR),
    ("bDescriptorType", UCHAR),
    ("bEndpointAddress", UCHAR),
    ("bmAttributes", UCHAR),
    ("wMaxPacketSize", USHORT),
    ("bInterval", UCHAR)]

class WDU_CONFIGURATION_DESCRIPTOR(Structure): _fields_ = \
    [("bLength", UCHAR),
    ("bDescriptorType", UCHAR),
    ("wTotalLength", USHORT),
    ("bNumInterfaces", UCHAR),
    ("bConfigurationValue", UCHAR),
    ("iConfiguration", UCHAR),
    ("bmAttributes", UCHAR),
    ("MaxPower", UCHAR)]

class WDU_DEVICE_DESCRIPTOR(Structure): _fields_ = \
    [("bLength", UCHAR),
    ("bDescriptorType", UCHAR),
    ("bcdUSB", USHORT),
    ("bDeviceClass", UCHAR),
    ("bDeviceSubClass", UCHAR),
    ("bDeviceProtocol", UCHAR),
    ("bMaxPacketSize0", UCHAR),
    ("idVendor", USHORT),
    ("idProduct", USHORT),
    ("bcdDevice", USHORT),
    ("iManufacturer", UCHAR),
    ("iProduct", UCHAR),
    ("iSerialNumber", UCHAR),
    ("bNumConfigurations", UCHAR)]

class WDU_ALTERNATE_SETTING(Structure): _fields_ = \
    [("Descriptor", WDU_INTERFACE_DESCRIPTOR),
    ("pEndpointDescriptors", POINTER(WDU_ENDPOINT_DESCRIPTOR)),
    ("pPipes", POINTER(WDU_PIPE_INFO))]

class WDU_INTERFACE(Structure): _fields_ = \
    [("pAlternateSettings", POINTER(WDU_ALTERNATE_SETTING)),
    ("dwNumAltSettings", DWORD),
    ("pActiveAltSetting", POINTER(WDU_ALTERNATE_SETTING))]

class WDU_CONFIGURATION(Structure): _fields_ = \
    [("Descriptor", WDU_CONFIGURATION_DESCRIPTOR),
    ("dwNumInterfaces", DWORD),
    ("pInterfaces", POINTER(WDU_INTERFACE))]

class WDU_DEVICE(Structure): _fields_ = \
    [("Descriptor", WDU_DEVICE_DESCRIPTOR),
    ("Pipe0", WDU_PIPE_INFO),
    ("pConfigs", POINTER(WDU_CONFIGURATION)),
    ("pActiveConfig", POINTER(WDU_CONFIGURATION)),
    ("pActiveInterface", WD_USB_MAX_INTERFACES * POINTER(WDU_INTERFACE))]

# Note: Any devices found matching this table will be controlled
class WDU_MATCH_TABLE(Structure): _fields_ = \
    [("wVendorId", USHORT),
    ("wProductId", USHORT),
    ("bDeviceClass", UCHAR),
    ("bDeviceSubClass", UCHAR),
    ("bInterfaceClass", UCHAR),
    ("bInterfaceSubClass", UCHAR),
    ("bInterfaceProtocol", UCHAR)]

class WDU_GET_DEVICE_DATA(Structure): _fields_ = \
    [("dwUniqueID", DWORD),
    ("pBuf", PVOID),
    ("dwBytes", DWORD),
    ("dwOptions", DWORD)]

# these enum values can be used as dwProperty values, see structure
# WD_GET_DEVICE_PROPERTY below.

#WD_DEVICE_REGISTRY_PROPERTY
(   WdDevicePropertyDeviceDescription,
    WdDevicePropertyHardwareID,
    WdDevicePropertyCompatibleIDs,
    WdDevicePropertyBootConfiguration,
    WdDevicePropertyBootConfigurationTranslated,
    WdDevicePropertyClassName,
    WdDevicePropertyClassGuid,
    WdDevicePropertyDriverKeyName,
    WdDevicePropertyManufacturer,
    WdDevicePropertyFriendlyName,
    WdDevicePropertyLocationInformation,
    WdDevicePropertyPhysicalDeviceObjectName,
    WdDevicePropertyBusTypeGuid,
    WdDevicePropertyLegacyBusType,
    WdDevicePropertyBusNumber,
    WdDevicePropertyEnumeratorName,
    WdDevicePropertyAddress,
    WdDevicePropertyUINumber,
    WdDevicePropertyInstallState,
    WdDevicePropertyRemovalPolicy) = range(1, 21)


class WDU_SET_INTERFACE(Structure): _fields_ = \
    [("dwUniqueID", DWORD),
    ("dwInterfaceNum", DWORD),
    ("dwAlternateSetting", DWORD),
    ("dwOptions", DWORD)]

class WDU_RESET_PIPE(Structure): _fields_ = \
    [("dwUniqueID", DWORD),
    ("dwPipeNum", DWORD),
    ("dwOptions", DWORD)]

#WDU_WAKEUP_OPTIONS
WDU_WAKEUP_ENABLE  = 0x1
WDU_WAKEUP_DISABLE = 0x2

#WDU_SELECTIVE_SUSPEND_OPTIONS
WDU_SELECTIVE_SUSPEND_SUBMIT = 0x1
WDU_SELECTIVE_SUSPEND_CANCEL = 0x2

class WDU_HALT_TRANSFER(Structure): _fields_ = \
    [("dwUniqueID", DWORD),
    ("dwPipeNum", DWORD),
    ("dwOptions", DWORD)]

class WDU_WAKEUP(Structure): _fields_ = \
    [("dwUniqueID", DWORD),
    ("dwOptions", DWORD)]

class WDU_SELECTIVE_SUSPEND(Structure): _fields_ = \
    [("dwUniqueID", DWORD),
    ("dwOptions", DWORD)]

class WDU_RESET_DEVICE(Structure): _fields_ = \
    [("dwUniqueID", DWORD),
    ("dwOptions", DWORD)]

class WDU_TRANSFER(Structure): _fields_ = \
    [("dwUniqueID", DWORD),
    ("dwPipeNum", DWORD), # Pipe number on device.
    ("fRead ", DWORD),    # TRUE for read (IN) transfers FALSE for write (OUT)
                          # transfers.
    ("dwOptions", DWORD), # USB_TRANSFER options:
                          # USB_ISOCH_FULL_PACKETS_ONLY - For isochronous
                          # transfers only. If set, only full packets will be
                          # transmitted and the transfer function will return
                          # when the amount of bytes left to transfer is less
                          # than the maximum packet size for the pipe (the
                          # function will return without transmitting the
                          # remaining bytes).
    ("pBuffer", PVOID),             # Pointer to buffer to read/write.
    ("dwBufferSize", DWORD),        # Amount of bytes to transfer.
    ("dwBytesTransferred", DWORD),  # Returns the number of bytes actually
                               # read/written
    ("SetupPacket[8]", UCHAR), # Setup packet for control pipe transfer.
    ("dwTimeout", DWORD)]  # Timeout for the transfer in milliseconds. Set to 0 
                           # for infinite wait.

class WDU_GET_DESCRIPTOR(Structure): _fields_ = \
    [("dwUniqueID", DWORD),
    ("bType", UCHAR),
    ("bIndex", UCHAR),
    ("wLength", USHORT),
    (" pBuffer", PVOID),
    ("wLanguage", USHORT)]

class WDU_STREAM(Structure): _fields_ = \
    [("dwUniqueID", DWORD),
    ("dwOptions", DWORD),
    ("dwPipeNum", DWORD),
    ("dwBufferSize", DWORD),
    ("dwRxSize", DWORD),
    ("fBlocking", BOOL),
    ("dwRxTxTimeout", DWORD),
    ("dwReserved", DWORD)]

class WDU_STREAM_STATUS(Structure): _fields_ = \
    [("dwUniqueID", DWORD),
    ("dwOptions", DWORD),
    ("fIsRunning", BOOL),
    ("dwLastError", DWORD),
    ("dwBytesInBuffer", DWORD),
    ("dwReserved", DWORD)]


