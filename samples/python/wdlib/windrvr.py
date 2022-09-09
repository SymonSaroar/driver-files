''' Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com '''

from ctypes import *
import sys, subprocess, platform

IS_PYTHON_V3 = sys.version_info[0] > 2
IS_32ON64 = platform.machine().endswith('64') and \
    platform.architecture()[0] == '32bit'

if IS_32ON64:
    print("Error: 64 bit WinDriver currently does not support running user mode\n"
        "Python based applications on a 32 bit Python interpreter.\n"
        "Please run your Python application using a 64 bit Python interpreter.")
    sys.exit()

WD_VER = 1511
WD_VER_ITOA = str(WD_VER)
WD_VERSION_STR = "15.1.1"

WD_PROD_NAME = "WinDriver"
WD_DEFAULT_DRIVER_NAME_BASE = "windrvr" + WD_VER_ITOA

if IS_PYTHON_V3:
    WD_DEFAULT_DRIVER_NAME_BASE = WD_DEFAULT_DRIVER_NAME_BASE.encode('utf-8')

inputf = input if IS_PYTHON_V3 else raw_input

WD_MAX_DRIVER_NAME_LENGTH = 128
WD_MAX_KP_NAME_LENGTH = 128
WD_TRANSFER_CMDS = 128

#Type definitions
if sys.platform == "win32" and not IS_32ON64:
    from ctypes.wintypes import *
else:
    USHORT = c_ushort
    UINT = c_uint
    ULONG = c_long
    BOOL = c_uint32
    PBYTE = c_char_p
    PCHAR = c_char_p
    PWORD = c_void_p
    PDWORD = c_void_p
    HANDLE = c_void_p
    DWORD = c_uint32

CHAR = c_char
UCHAR = c_ubyte
UINT32 = c_uint32
UINT64 = c_longlong
PVOID = c_void_p
UPTR = c_size_t
BYTE = c_ubyte
WORD = c_ushort

def printf(str, *args):
    sys.stdout.write(str % args)

if sys.platform == "cygwin":
    if platform.architecture()[0] == "64bit":
        DWORD = c_uint
    else:
        try:
            TARGET_CPU = subprocess.check_output("export | grep -w ProgramW6432"
                "| sed 's/.*ProgramW6432.x86_64//'", shell=True)
        except:
            TARGET_CPU = ""

if platform.architecture()[0] == "64bit" or \
   (sys.platform == "cygwin" and TARGET_CPU == "x86_64\n") or \
   IS_32ON64:
    KPTR = UINT64
else:
    KPTR = DWORD

if sys.platform == "win32":
    PRI64 = "I64"
else :
    PRI64 = "ll"

PHYS_ADDR = UINT64
DMA_ADDR = UINT64

# IN WD_TRANSFER_CMD and WD_Transfer() DWORD stands for 32 bits and QWORD is 64
# bit.

# WD_TRANSFER_CMD
CMD_NONE = 0       # No command
CMD_END = 1        # End command
CMD_MASK = 2       # Interrupt Mask

RP_BYTE = 10       # Read port byte
RP_WORD = 11       # Read port word
RP_DWORD = 12      # Read port dword
WP_BYTE = 13       # Write port byte
WP_WORD = 14       # Write port word
WP_DWORD = 15      # Write port dword
RP_QWORD = 16      # Read port qword
WP_QWORD = 17      # Write port qword

RP_SBYTE = 20      # Read port string byte
RP_SWORD = 21      # Read port string word
RP_SDWORD = 22     # Read port string dword
WP_SBYTE = 23      # Write port string byte
WP_SWORD = 24      # Write port string word
WP_SDWORD = 25     # Write port string dword
RP_SQWORD = 26     # Read port string qword
WP_SQWORD = 27     # Write port string qword

RM_BYTE = 30       # Read memory byte
RM_WORD = 31       # Read memory word
RM_DWORD = 32      # Read memory dword
WM_BYTE = 33       # Write memory byte
WM_WORD = 34       # Write memory word
WM_DWORD = 35      # Write memory dword
RM_QWORD = 36      # Read memory qword
WM_QWORD = 37      # Write memory qword

RM_SBYTE = 40      # Read memory string byte
RM_SWORD = 41      # Read memory string word
RM_SDWORD = 42     # Read memory string dword
WM_SBYTE = 43      # Write memory string byte
WM_SWORD = 44      # Write memory string word
WM_SDWORD = 45     # Write memory string dword
RM_SQWORD = 46     # Read memory string quad word
WM_SQWORD = 47     # Write memory string quad word

WD_PCI_CARDS = 256

WD_DMA_PAGES = 256

#WD_DMA_OPTIONS
DMA_KERNEL_BUFFER_ALLOC = 0x1 # The system allocates a contiguous buffer.
    # The user does not need to supply linear address.

DMA_KBUF_BELOW_16M = 0x2 # If DMA_KERNEL_BUFFER_ALLOC is used,
    # this will make sure it is under 16M.

DMA_LARGE_BUFFER = 0x4 # If DMA_LARGE_BUFFER is used,
    # the maximum number of pages are dwPages, and not
    # WD_DMA_PAGES. If you lock a user buffer (not a kernel
    # allocated buffer) that is larger than 1MB, then use this
    # option and allocate memory for pages.

DMA_ALLOW_CACHE = 0x8  # Allow caching of contiguous memory.

DMA_KERNEL_ONLY_MAP = 0x10 # Only map to kernel, dont map to user-mode.
    # relevant with DMA_KERNEL_BUFFER_ALLOC flag only

DMA_FROM_DEVICE = 0x20 # memory pages are locked to be written by device

DMA_TO_DEVICE = 0x40 # memory pages are locked to be read by device

DMA_TO_FROM_DEVICE = DMA_FROM_DEVICE | DMA_TO_DEVICE, # memory pages are
    # locked for both read and write

DMA_ALLOW_64BIT_ADDRESS = 0x80 # Use this value for devices that support
                                # 64-bit DMA addressing.

DMA_ALLOW_NO_HCARD = 0x100 # Allow memory lock without hCard

DMA_GET_EXISTING_BUF = 0x200 # Get existing buffer by hDma handle

DMA_RESERVED_MEM = 0x400

DMA_KBUF_ALLOC_SPECIFY_ADDRESS_WIDTH = 0x800 # When using this flag, the
	# width of the address must be entered in the fourth byte of dwOptions
	# and then the allocated address will be limited to this width.
	# Linux: works with contiguous buffers only.

DMA_GET_PREALLOCATED_BUFFERS_ONLY = 0x1000, # Windows: Try to allocate
	# buffers from preallocated buffers pool ONLY (if none of them are
	# available, function will fail).

DMA_TRANSACTION = 0x2000, # Use this flag to use the DMA transaction
	# mechanism

DMA_GPUDIRECT = 0x4000, # Linux only

DMA_DISABLE_MERGE_ADJACENT_PAGES = 0x8000, # Disable merge adjacent pages.
	# In case the flag is omitted, the merge will take place
	# automatically. Used for scatter gather mode only.

DMA_OPTIONS_MAX_SHIFT = 24 # 3 bytes (24 bits) are needed for WD_DMA_OPTIONS
	# so the fourth byte will be used for storing the address width of the
	# requested buffer

#USB header (windrvr_usb)

# Note: Any devices found matching this table will be controlled
class WDU_MATCH_TABLE(Structure): _fields_ = \
    [("wVendorId", USHORT),
    ("wProductId", USHORT),
    ("bDeviceClass", UCHAR),
    ("bDeviceSubClass", UCHAR),
    ("bInterfaceClass", UCHAR),
    ("bInterfaceSubClass", UCHAR),
    ("bInterfaceProtocol", UCHAR)]

#windrvr.h


#WD_ITEM_MEM_OPTIONS
ITEM_NONE         = 0
ITEM_INTERRUPT    = 1 # Interrupt
ITEM_MEMORY       = 2 # Memory
ITEM_IO           = 3 # I/O
ITEM_BUS          = 5 # Bus

ITEM_TYPE = DWORD

class _DATA(Union): _fields_ = \
    [("Byte", BYTE),      # Use for 8 bit transfer.
    ("Word", WORD),      # Use for 16 bit transfer.
    ("Dword", UINT32),   # Use for 32 bit transfer.
    ("Qword", UINT64),   # Use for 64 bit transfer.
    ("pBuffer", PVOID)]  # Use for string transfer.

class WD_TRANSFER(Structure): _fields_ = \
    [("pPort", KPTR),     # I/O port for transfer or kernel memory address.
    ("cmdTrans", DWORD),  # Transfer command WD_TRANSFER_CMD.
    # Parameters used for string transfers:
    ("dwBytes", DWORD),   # For string transfer.
    ("fAutoinc", DWORD),  # Transfer from one port/address
                          # or use incremental range of addresses.
    ("dwOptions", DWORD), # Must be 0.
    ("Data", _DATA)]

#INTERRUPTS
INTERRUPT_LATCHED = 0x00
INTERRUPT_LEVEL_SENSITIVE = 0x01
INTERRUPT_CMD_COPY = 0x02
INTERRUPT_CE_INT_ID = 0x04
INTERRUPT_CMD_RETURN_VALUE = 0x08
INTERRUPT_MESSAGE = 0x10
INTERRUPT_MESSAGE_X = 0x20
INTERRUPT_DONT_GET_MSI_MESSAGE = 0x40

class WD_KERNEL_PLUGIN_CALL(Structure): _fields_ =\
    [("hKernelPlugIn", DWORD),
    ("dwMessage", DWORD),
    ("pData", PVOID),
    ("dwResult", DWORD)]

#WD_INTERRUPT_WAIT_RESULT
INTERRUPT_RECEIVED    = 0 # Interrupt was received
INTERRUPT_STOPPED     = 1 # Interrupt was disabled during wait
INTERRUPT_INTERRUPTED = 2 # Wait was interrupted before an actual hardware
                          # interrupt was received

WD_INTERRUPT_WAIT_RESULT = DWORD

class WD_INTERRUPT(Structure): _fields_ =\
    [("hInterrupt", DWORD),    # Handle of interrupt.
    ("dwOptions", DWORD),      # Interrupt options: can be INTERRUPT_CMD_COPY

    ("Cmd", POINTER(WD_TRANSFER)),    # Commands to do on interrupt.
    ("dwCmds", DWORD),                # Number of commands.

    # For WD_IntEnable():
    ("kpCall", WD_KERNEL_PLUGIN_CALL),  # Kernel PlugIn call.
    ("fEnableOk", DWORD),      # TRUE if interrupt was enabled (WD_IntEnable()
                               # succeed).

    # For WD_IntWait() and WD_IntCount():
    ("dwCounter", DWORD),        # Number of interrupts received.
    ("dwLost", DWORD),           # Number of interrupts not yet dealt with.
    ("fStopped", DWORD),         # Was interrupt disabled during wait.

    ("dwLastMessage", DWORD),    # Message data of the last received MSI (Windows)
    ("dwEnabledIntType", DWORD)] # Interrupt type that was actually enabled

DMA_TRANSACTION_CALLBACK = CFUNCTYPE(None, PVOID)

class WD_DMA_PAGE(Structure): _fields_ = \
    [("pPhysicalAddr", DMA_ADDR), # Physical address of page.
    ("dwBytes", DWORD)]           # Size of page.

class WD_DMA(Structure): _fields_ = \
    [("hDma", DWORD),       # Handle of DMA buffer
     ("pUserAddr", PVOID),  # User address
     ("pKernelAddr", KPTR), # Kernel address
     ("dwBytes", DWORD),    # Size of buffer
     ("dwOptions", DWORD),  # Refer to WD_DMA_OPTIONS
     ("dwPages", DWORD),    # Number of pages in buffer.
     ("hCard", DWORD),      # Handle of relevant card as received from
                            # WD_CardRegister
     ("DMATransactionCallback", DMA_TRANSACTION_CALLBACK),
     ("DMATransactionCallbackCtx", PVOID),
     ("dwAlignment", DWORD), # required alignment, used for contiguous mode only
     ("dwMaxTransferSize", DWORD), # used for scatter gather mode only
     ("dwTransferElementSize", DWORD), # used for scatter gather mode only
     ("dwBytesTransferred", DWORD), # bytes transferred count
     ("Page", WD_DMA_PAGE * WD_DMA_PAGES)] # You can increase WD_DMA_PAGES if
                            # you plan to use more than pages than the default


# KER_BUF_ALLOC_NON_CONTIG and KER_BUF_GET_EXISTING_BUF options are valid
# only as part of "WinDriver for Server" API and require
#  "WinDriver for Server" license. Note that "WinDriver for Server" APIs are
# included in WinDriver evaluation version.
# WD_KER_BUF_OPTION:
KER_BUF_ALLOC_NON_CONTIG = 0x0001
KER_BUF_ALLOC_CONTIG     = 0x0002
KER_BUF_ALLOC_CACHED     = 0x0004
KER_BUF_GET_EXISTING_BUF = 0x0008

class WD_KERNEL_BUFFER(Structure): _fields_ = \
    [("hKerBuf", DWORD),    # Handle of Kernel Buffer
    ("dwOptions", DWORD),   # Refer to WD_KER_BUF_OPTION
    ("qwBytes", UINT64),    # Size of buffer
    ("pKernelAddr", KPTR),  # Kernel address
    ("pUserAddr", UPTR)]    # User address

WD_BUS_USB = 0xfffffffe        # USB
WD_BUS_UNKNOWN = 0             # Unknown bus type
WD_BUS_ISA = 1                 # ISA
WD_BUS_EISA = 2                # EISA, including ISA PnP
WD_BUS_PCI = 5                 # PCI

class WD_BUS(Structure): _fields_ = \
    [("dwBusType", DWORD),
     ("dwDomainNum", DWORD),
     ("dwBusNum", DWORD),
     ("dwSlotFunc", DWORD)]

WD_BUS_TYPE = DWORD

class _Mem(Structure): _fields_ = \
    [("pPhysicalAddr", PHYS_ADDR),
     ("qwBytes", UINT64),
     ("pTransAddr", KPTR),
     ("pUserDirectAddr", UPTR),
     ("dwBar", DWORD),
     ("dwOptions", DWORD),
     ("pReserved", KPTR)]

class _IO(Structure): _fields_ = \
    [("pAddr", KPTR),
     ("dwBytes", DWORD),
     ("dwBar", DWORD)]

class _Int(Structure): _fields_ = \
    [("dwInterrupt", DWORD),
     ("dwOptions", DWORD),
     ("hInterrupt", DWORD),
     ("dwReserved1", DWORD),
     ("pReserved2", KPTR)]

class _I(Union) : _fields_ = \
    [("Mem", _Mem),
     ("IO", _IO),
     ("Int", _Int),
     ("Bus", WD_BUS)]

class WD_ITEMS(Structure): _fields_ = \
    [("item", DWORD),          # ITEM_TYPE
     ("fNotSharable", DWORD),
     ("I", _I)]

WD_CARD_ITEMS = 128

class WD_CARD(Structure): _fields_ = \
    [("dwItems", DWORD),
     ("Item", WD_ITEMS * WD_CARD_ITEMS)]

class WD_CARD_REGISTER(Structure): _fields_ =\
    [("Card", WD_CARD),           # Card to register.
    ("fCheckLockOnly", DWORD),    # Only check if card is lockable, return hCard=1 if
                                  # OK.
    ("hCard", DWORD),             # Handle of card.
    ("dwOptions", DWORD),         # Should be zero.
    ("cName", CHAR * 32),         # Name of card.
    ("cDescription", CHAR * 100)] # Description.

class WD_PCI_SLOT(Structure): _fields_ = \
    [("dwDomain", DWORD),
    ("dwBus", DWORD),
    ("dwSlot", DWORD),
    ("dwFunction", DWORD)]

WD_PROCESS_NAME_LENGTH = 128

class WD_IPC_PROCESS(Structure): _fields_ = \
    [("cProcessName", CHAR * WD_PROCESS_NAME_LENGTH),
    ("dwSubGroupID", DWORD), # Identifier of the processes type
    ("dwGroupID", DWORD),  # Unique identifier of the processes group for discarding
                       # unrelated process. WinDriver developers are encouraged
                       # to change their driver name before distribution to
                       # avoid this issue entirely.
    ("hIpc", DWORD)]   # Returned from WD_IpcRegister()

class WD_IPC_REGISTER(Structure): _fields_ = \
    [("procInfo", WD_IPC_PROCESS),
    ("dwOptions", DWORD)]            # Reserved for future use set to 0

WD_IPC_MAX_PROCS = 0x40

class WD_IPC_SCAN_PROCS(Structure): _fields_ = \
    [("hIpc", DWORD),                 # Returned from WD_IpcRegister()
    # Result processes
    ("dwNumProcs", DWORD),           # Number of matching processes
    ("procInfo", WD_IPC_PROCESS * WD_IPC_MAX_PROCS)]

WD_IPC_UID_UNICAST        = 0x1
WD_IPC_SUBGROUP_MULTICAST = 0x2
WD_IPC_MULTICAST          = 0x4

class WD_IPC_SEND(Structure): _fields_ = \
    [("hIpc", DWORD),      # Returned from WD_IpcRegister()
    ("dwOptions", DWORD),  # WD_IPC_SUBGROUP_MULTICAST, WD_IPC_UID_UNICAST,
                           # WD_IPC_MULTICAST
    ("dwRecipientID", DWORD),   # used only on WD_IPC_UNICAST
    ("dwMsgID", DWORD),
    ("qwMsgData", UINT64)]

class WD_PCI_ID(Structure): _fields_ = \
    [("dwVendorId", DWORD),
    ("dwDeviceId", DWORD)]

class WD_PCI_SCAN_CARDS(Structure): _fields_ = \
    [("searchId", WD_PCI_ID),  # PCI vendor and/or device IDs to search for
                               # dwVendorId==0 -- scan all PCI vendor IDs
                               # dwDeviceId==0 -- scan all PCI device IDs
    ("dwCards", DWORD),        # Number of matching PCI cards
    # Scan Results
    ("cardId", WD_PCI_ID * WD_PCI_CARDS),     # Array of matching card IDs
    ("cardSlot", WD_PCI_SLOT * WD_PCI_CARDS), # Array of matching PCI slots info
    # Scan Options
    ("dwOptions", DWORD)]        # Scan options -- WD_PCI_SCAN_OPTIONS

#WD_PCI_SCAN_OPTIONS
WD_PCI_SCAN_DEFAULT     = 0x0
WD_PCI_SCAN_BY_TOPOLOGY = 0x1
WD_PCI_SCAN_REGISTERED  = 0x2
WD_PCI_SCAN_INCLUDE_DOMAINS  = 0x4

WD_PCI_MAX_CAPS = 50

WD_PCI_CAP_ID_ALL = 0x0

class WD_PCI_CAP(Structure): _fields_ = \
    [("dwCapId", DWORD),    # PCI capability ID
    ("dwCapOffset", DWORD)] # PCI capability register offset

#WD_PCI_SCAN_CAPS_OPTIONS
WD_PCI_SCAN_CAPS_BASIC = 0x1    # Scan basic PCI capabilities
WD_PCI_SCAN_CAPS_EXTENDED = 0x2 # Scan extended (PCIe) PCI capabilities

class WD_PCI_SCAN_CAPS(Structure): _fields_ = \
    [("pciSlot", WD_PCI_SLOT), # PCI slot information
    ("dwCapId", DWORD),        # PCI capability ID to search for, or
                               # WD_PCI_CAP_ID_ALL to scan all PCI capabilities
    ("dwOptions", DWORD),      # Scan options -- WD_PCI_SCAN_CAPS_OPTIONS
                               # default -- WD_PCI_SCAN_CAPS_BASIC
    # Scan Results
    ("dwNumCaps", DWORD),      # Number of matching PCI capabilities
    ("pciCaps", WD_PCI_CAP * WD_PCI_MAX_CAPS)] # Array of matching PCI capabilities

class WD_PCI_SRIOV(Structure): _fields_ = \
    [("pciSlot", WD_PCI_SLOT), # PCI slot information
    ("dwNumVFs", DWORD)]       # Number of Virtual Functions

class WD_PCI_CARD_INFO(Structure): _fields_ = \
    [("pciSlot", WD_PCI_SLOT), # PCI slot information
    ("Card", WD_CARD)]         # Card information

#DEBUG_LEVEL
D_OFF       = 0
D_ERROR     = 1
D_WARN      = 2
D_INFO      = 3
D_TRACE     = 4

#DEBUG_SECTION
S_ALL       = 0xffffffff
S_IO        = 0x00000008
S_MEM       = 0x00000010
S_INT       = 0x00000020
S_PCI       = 0x00000040
S_DMA       = 0x00000080
S_MISC      = 0x00000100
S_LICENSE   = 0x00000200
S_PNP       = 0x00001000
S_CARD_REG  = 0x00002000
S_KER_DRV   = 0x00004000
S_USB       = 0x00008000
S_KER_PLUG  = 0x00010000
S_EVENT     = 0x00020000
S_IPC       = 0x00040000
S_KER_BUF   = 0x00080000

class WD_KERNEL_PLUGIN(Structure): _fields_ =\
    [("hKernelPlugIn", DWORD),
    ("cDriverName", CHAR * WD_MAX_KP_NAME_LENGTH),
    ("cDriverPath", CHAR * WD_MAX_KP_NAME_LENGTH), # Should be NULL (exists for backward compatibility).
                        # The driver will be searched in the operating
                        # system's drivers/modules directory.
    ("pOpenData", PVOID)]

#WD_ERROR_CODES
WD_STATUS_SUCCESS = 0x0
WD_STATUS_INVALID_WD_HANDLE = 0xffffffff

WD_WINDRIVER_STATUS_ERROR = 0x20000000

WD_INVALID_HANDLE = 0x20000001
WD_INVALID_PIPE_NUMBER = 0x20000002
WD_READ_WRITE_CONFLICT = 0x20000003 # Request to read from an OUT (write)
                                         #  pipe or request to write to an IN
                                         #  (read) pipe
WD_ZERO_PACKET_SIZE = 0x20000004 # Maximum packet size is zero
WD_INSUFFICIENT_RESOURCES = 0x20000005
WD_UNKNOWN_PIPE_TYPE = 0x20000006
WD_SYSTEM_INTERNAL_ERROR = 0x20000007
WD_DATA_MISMATCH = 0x20000008
WD_NO_LICENSE = 0x20000009
WD_NOT_IMPLEMENTED = 0x2000000a
WD_KERPLUG_FAILURE = 0x2000000b
WD_FAILED_ENABLING_INTERRUPT = 0x2000000c
WD_INTERRUPT_NOT_ENABLED = 0x2000000d
WD_RESOURCE_OVERLAP = 0x2000000e
WD_DEVICE_NOT_FOUND = 0x2000000f
WD_WRONG_UNIQUE_ID = 0x20000010
WD_OPERATION_ALREADY_DONE = 0x20000011
WD_USB_DESCRIPTOR_ERROR = 0x20000012
WD_SET_CONFIGURATION_FAILED = 0x20000013
WD_CANT_OBTAIN_PDO = 0x20000014
WD_TIME_OUT_EXPIRED = 0x20000015
WD_IRP_CANCELED = 0x20000016
WD_FAILED_USER_MAPPING = 0x20000017
WD_FAILED_KERNEL_MAPPING = 0x20000018
WD_NO_RESOURCES_ON_DEVICE = 0x20000019
WD_NO_EVENTS = 0x2000001a
WD_INVALID_PARAMETER = 0x2000001b
WD_INCORRECT_VERSION = 0x2000001c
WD_TRY_AGAIN = 0x2000001d
WD_WINDRIVER_NOT_FOUND = 0x2000001e
WD_INVALID_IOCTL = 0x2000001f
WD_OPERATION_FAILED = 0x20000020
WD_INVALID_32BIT_APP = 0x20000021
WD_TOO_MANY_HANDLES = 0x20000022
WD_NO_DEVICE_OBJECT = 0x20000023
# The following status codes are returned by USBD:
# USBD status types:
WD_USBD_STATUS_SUCCESS = 0x00000000
WD_USBD_STATUS_PENDING = 0x40000000
WD_USBD_STATUS_ERROR = 0x80000000
WD_USBD_STATUS_HALTED = 0xC0000000

# USBD status codes:
# NOTE: The following status codes are comprised of one of the status
# types above and an error code [i.e. 0xXYYYYYYYL - where: X = status type
# YYYYYYY = error code].
# The same error codes may also appear with one of the other status types
# as well.

# HC (Host Controller) status codes.
# [NOTE: These status codes use the WD_USBD_STATUS_HALTED status type]:
WD_USBD_STATUS_CRC = 0xC0000001
WD_USBD_STATUS_BTSTUFF = 0xC0000002
WD_USBD_STATUS_DATA_TOGGLE_MISMATCH = 0xC0000003
WD_USBD_STATUS_STALL_PID = 0xC0000004
WD_USBD_STATUS_DEV_NOT_RESPONDING = 0xC0000005
WD_USBD_STATUS_PID_CHECK_FAILURE = 0xC0000006
WD_USBD_STATUS_UNEXPECTED_PID = 0xC0000007
WD_USBD_STATUS_DATA_OVERRUN = 0xC0000008
WD_USBD_STATUS_DATA_UNDERRUN = 0xC0000009
WD_USBD_STATUS_RESERVED1 = 0xC000000A
WD_USBD_STATUS_RESERVED2 = 0xC000000B
WD_USBD_STATUS_BUFFER_OVERRUN = 0xC000000C
WD_USBD_STATUS_BUFFER_UNDERRUN = 0xC000000D
WD_USBD_STATUS_NOT_ACCESSED = 0xC000000F
WD_USBD_STATUS_FIFO = 0xC0000010

if sys.platform == "win32":
    WD_USBD_STATUS_XACT_ERROR = 0xC0000011
    WD_USBD_STATUS_BABBLE_DETECTED = 0xC0000012
    WD_USBD_STATUS_DATA_BUFFER_ERROR = 0xC0000013

WD_USBD_STATUS_CANCELED = 0xC0010000

# Returned by HCD (Host Controller Driver) if a transfer is submitted to
# an endpoint that is stalled:
WD_USBD_STATUS_ENDPOINT_HALTED = 0xC0000030

# Software status codes
# [NOTE: The following status codes have only the error bit set]:
WD_USBD_STATUS_NO_MEMORY = 0x80000100
WD_USBD_STATUS_INVALID_URB_FUNCTION = 0x80000200
WD_USBD_STATUS_INVALID_PARAMETER = 0x80000300

# Returned if client driver attempts to close an endpoint/interface
# or configuration with outstanding transfers:
WD_USBD_STATUS_ERROR_BUSY = 0x80000400,

# Returned by USBD if it cannot complete a URB request. Typically this
# will be returned in the URB status field when the Irp is completed
# with a more specific error code. [The Irp status codes are indicated
# in WinDriver's Debug Monitor tool (wddebug/wddebug_gui):
WD_USBD_STATUS_REQUEST_FAILED = 0x80000500

WD_USBD_STATUS_INVALID_PIPE_HANDLE = 0x80000600

# Returned when there is not enough bandwidth available
# to open a requested endpoint:
WD_USBD_STATUS_NO_BANDWIDTH = 0x80000700

# Generic HC (Host Controller) error:
WD_USBD_STATUS_INTERNAL_HC_ERROR = 0x80000800

# Returned when a short packet terminates the transfer
# i.e. USBD_SHORT_TRANSFER_OK bit not set:
WD_USBD_STATUS_ERROR_SHORT_TRANSFER = 0x80000900

# Returned if the requested start frame is not within
# USBD_ISO_START_FRAME_RANGE of the current USB frame,
# NOTE: The stall bit is set:
WD_USBD_STATUS_BAD_START_FRAME = 0xC0000A00

# Returned by HCD (Host Controller Driver) if all packets in an
# isochronous transfer complete with an error:
WD_USBD_STATUS_ISOCH_REQUEST_FAILED = 0xC0000B00

# Returned by USBD if the frame length control for a given
# HC (Host Controller) is already taken by another driver:
WD_USBD_STATUS_FRAME_CONTROL_OWNED = 0xC0000C00

# Returned by USBD if the caller does not own frame length control and
# attempts to release or modify the HC frame length:
WD_USBD_STATUS_FRAME_CONTROL_NOT_OWNED = 0xC0000D00

if sys.platform == "win32":
    # Additional USB 2.0 software error codes added for USB 2.0:
    WD_USBD_STATUS_NOT_SUPPORTED = 0xC0000E00 # Returned for APIS not
                                                     # supported/implemented
    WD_USBD_STATUS_INAVLID_CONFIGURATION_DESCRIPTOR = 0xC0000F00,
    WD_USBD_STATUS_INSUFFICIENT_RESOURCES = 0xC0001000
    WD_USBD_STATUS_SET_CONFIG_FAILED = 0xC0002000
    WD_USBD_STATUS_BUFFER_TOO_SMALL = 0xC0003000
    WD_USBD_STATUS_INTERFACE_NOT_FOUND = 0xC0004000
    WD_USBD_STATUS_INAVLID_PIPE_FLAGS = 0xC0005000
    WD_USBD_STATUS_TIMEOUT = 0xC0006000
    WD_USBD_STATUS_DEVICE_GONE = 0xC0007000
    WD_USBD_STATUS_STATUS_NOT_MAPPED = 0xC0008000

    # Extended isochronous error codes returned by USBD.
    # These errors appear in the packet status field of an isochronous
    # transfer.

    # For some reason the controller did not access the TD associated with
    # this packet:
    WD_USBD_STATUS_ISO_NOT_ACCESSED_BY_HW = 0xC0020000,
    # Controller reported an error in the TD.
    # Since TD errors are controller specific they are reported
    # generically with this error code:
    WD_USBD_STATUS_ISO_TD_ERROR = 0xC0030000,
    # The packet was submitted in time by the client but
    # failed to reach the miniport in time:
    WD_USBD_STATUS_ISO_NA_LATE_USBPORT = 0xC0040000,
    # The packet was not sent because the client submitted it too late
    # to transmit:
    WD_USBD_STATUS_ISO_NOT_ACCESSED_LATE = 0xC0050000

# WD_EVENT_ACTION
WD_INSERT                  = 0x1
WD_REMOVE                  = 0x2
WD_OBSOLETE                = 0x8  # Obsolete
WD_POWER_CHANGED_D0        = 0x10 # Power states for the power management
WD_POWER_CHANGED_D1        = 0x20
WD_POWER_CHANGED_D2        = 0x40
WD_POWER_CHANGED_D3        = 0x80
WD_POWER_SYSTEM_WORKING    = 0x100
WD_POWER_SYSTEM_SLEEPING1  = 0x200
WD_POWER_SYSTEM_SLEEPING2  = 0x400
WD_POWER_SYSTEM_SLEEPING3  = 0x800
WD_POWER_SYSTEM_HIBERNATE  = 0x1000
WD_POWER_SYSTEM_SHUTDOWN   = 0x2000
WD_IPC_UNICAST_MSG         = 0x4000
WD_IPC_MULTICAST_MSG       = 0x8000

WD_IPC_ALL_MSG = WD_IPC_UNICAST_MSG | WD_IPC_MULTICAST_MSG

#WD_EVENT_OPTION
WD_ACKNOWLEDGE              = 0x1
WD_ACCEPT_CONTROL           = 0x2  # used in WD_EVENT_SEND (acknowledge)

WD_ACTIONS_POWER = WD_POWER_CHANGED_D0 | WD_POWER_CHANGED_D1 | \
    WD_POWER_CHANGED_D2 | WD_POWER_CHANGED_D3 | WD_POWER_SYSTEM_WORKING | \
    WD_POWER_SYSTEM_SLEEPING1 | WD_POWER_SYSTEM_SLEEPING3 | \
    WD_POWER_SYSTEM_HIBERNATE | WD_POWER_SYSTEM_SHUTDOWN
WD_ACTIONS_ALL = WD_ACTIONS_POWER | WD_INSERT | WD_REMOVE

class _Pci(Structure): _fields_ =\
    [("cardId", WD_PCI_ID),
    ("pciSlot", WD_PCI_SLOT)]

class _Usb(Structure): _fields_ =\
    [("dwUniqueID", DWORD)]

class _Ipc(Structure): _fields_ =\
    [("hIpc", DWORD),        # Acts as a unique identifier
    ("dwSubGroupID", DWORD), # Might be identical to same process running
                             # twice (User implementation dependant)
    ("dwGroupID", DWORD),
    ("dwSenderUID", DWORD),
    ("dwMsgID", DWORD),
    ("qwMsgData", UINT64)]

class _u(Union): _fields_ =\
    [("Pci", _Pci),
    ("Usb", _Usb),
    ("Ipc", _Ipc)]

class WD_EVENT(Structure): _fields_ =\
    [("hEvent", DWORD),
    ("dwEventType", DWORD),      # WD_EVENT_TYPE
    ("dwAction", DWORD),         # WD_EVENT_ACTION
    ("dwEventId", DWORD),
    ("hKernelPlugIn", DWORD),
    ("dwOptions", DWORD),        # WD_EVENT_OPTION
    ("u", _u),
    ("dwNumMatchTables", DWORD),
    ("matchTables", WDU_MATCH_TABLE * 1)]


