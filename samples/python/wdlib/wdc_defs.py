''' Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com '''

from .windrvr import *

# ************************************************************
#  General definitions
# ************************************************************
# -----------------------------------------------
#    Memory / I/O / Registers
# -----------------------------------------------

# Address space information struct
class WDC_ADDR_DESC(Structure): _fields_ = \
    [("dwAddrSpace", DWORD),        # Address space number
    ("fIsMemory", BOOL),            # TRUE: memory address space FALSE: I/O
    ("dwItemIndex", DWORD),         # Index of address space in the
                                    # pDev->cardReg.Card.Item array
    ("reserved", DWORD),
    ("qwBytes", UINT64),            # Size of address space
    ("pAddr", KPTR),                # I/O / Memory kernel mapped address -- for
                                    # WD_Transfer(), WD_MultiTransfer(), or direct
                                    # kernel access
    ("pUserDirectMemAddr", UPTR)]   # Memory address for direct user-mode access


# -----------------------------------------------
#    General
# -----------------------------------------------

# Device information struct
class WDC_DEVICE(Structure): _fields_ = \
    [("id", WD_PCI_ID),                     # PCI device ID
    ("slot", WD_PCI_SLOT),                  # PCI device slot location
                                            # information
    ("dwNumAddrSpaces", DWORD),             # Total number of device's address
                                            # spaces
    ("pAddrDesc", POINTER(WDC_ADDR_DESC)),  # Array of device's address spaces
                                            # information
    ("cardReg", WD_CARD_REGISTER),          # Device's resources information
    ("kerPlug", WD_KERNEL_PLUGIN),          # Kernel PlugIn information
    ("Int", WD_INTERRUPT),                  # Interrupt information
    ("hIntThread", HANDLE),
    ("Event", WD_EVENT),                    # Event information
    ("hEvent", HANDLE),
    ("pCtx", PVOID)]                        # User-specific context

PWDC_DEVICE = POINTER(WDC_DEVICE)

# -----------------------------------------------
#    Memory / I/O / Registers
# -----------------------------------------------
# Get direct memory address pointer

# Check if memory or I/O address */
def WDC_ADDR_IS_MEM(addrDesc):
    return addrDesc.fIsMemory

# Get address space descriptor
def WDC_GET_ADDR_DESC(pDev, dwAddrSpace):
    return pDev.contents.pAddrDesc[dwAddrSpace]

def WDC_GET_ADDR_SPACE_SIZE(pDev, dwAddrSpace):
    return pDev.contents.pAddrDesc[dwAddrSpace].qwBytes

# Get type of enabled interrupt
def WDC_GET_ENABLED_INT_TYPE(pDev):
    return pDev.contents.Int.dwEnabledIntType

# Get interrupt options field
def WDC_GET_INT_OPTIONS(pDev):
    return cast(pDev, PWDC_DEVICE).contents.Int.dwOptions

# Is the MSI/MSI-X interrupt option set
def WDC_INT_IS_MSI(dwIntType):
    mask = INTERRUPT_MESSAGE | INTERRUPT_MESSAGE_X
    return dwIntType & mask

# Get the message data of the last received MSI/MSI-X
def WDC_GET_ENABLED_INT_LAST_MSG(pDev):
    return pDev.contents.Int.dwLastMessage if \
        WDC_INT_IS_MSI(WDC_GET_ENABLED_INT_TYPE(pDev)) else 0


