''' @JUNGO_COPYRIGHT '''

from __future__ import print_function
from .wdc_lib import *
from .diag_lib import *
from .pci_regs import *
from ctypes import *
import sys

# Print run-time registers and PCI configuration registers information
READ_REGS_ALL_DESC_INDENT      = 39
READ_REGS_ALL_DESC_WIDTH       = 22
READ_REGS_ALL_DETAILS_INDENT   = 61
READ_REGS_ALL_DETAILS_WIDTH    = 0
REGS_INFO_PRINT_DETAILS_INDENT = 54
REGS_INFO_PRINT_DETAILS_WIDTH  = 22
PCIE_REGS_NUM                  = 68

WDC_DIAG_REG_PRINT_NAME       = 0x1
WDC_DIAG_REG_PRINT_DESC       = 0x2
WDC_DIAG_REG_PRINT_ADDR_SPACE = 0x4
WDC_DIAG_REG_PRINT_OFFSET     = 0x8
WDC_DIAG_REG_PRINT_SIZE       = 0x10
WDC_DIAG_REG_PRINT_DIRECTION  = 0x12
WDC_DIAG_REG_PRINT_ALL = \
    (WDC_DIAG_REG_PRINT_NAME | WDC_DIAG_REG_PRINT_DESC | \
    WDC_DIAG_REG_PRINT_ADDR_SPACE | WDC_DIAG_REG_PRINT_OFFSET | \
    WDC_DIAG_REG_PRINT_SIZE | WDC_DIAG_REG_PRINT_DIRECTION)
WDC_DIAG_REG_PRINT_DEFAULT = \
    (WDC_DIAG_REG_PRINT_NAME | WDC_DIAG_REG_PRINT_DIRECTION | \
        WDC_DIAG_REG_PRINT_DESC)

WDC_DIAG_REG_PRINT_OPTIONS = DWORD

# Device configuration space identifier (PCI configuration space)
WDC_AD_CFG_SPACE = 0xFF

# Register information struct
class WDC_REG():
    dwAddrSpace = 0    # Number of address space in which the register
                       # resides
                       # For PCI configuration registers, use
                       # WDC_AD_CFG_SPACE
    dwOffset = 0       # Offset of the register in the dwAddrSpace
                       # address space
    dwSize = 0         # Register's size (in bytes)
    direction = 0      # Read/write access mode - see WDC_DIRECTION
                       # options
    sName = ""         # Register's name
    sDesc = ""         # Register's description
    def __init__(self, dwAddrSpace, dwOffset, dwSize, direction, sName, sDesc):
        self.dwAddrSpace = dwAddrSpace
        self.dwOffset = dwOffset
        self.dwSize = dwSize
        self.direction = direction
        self.sName = sName
        self.sDesc = sDesc

MAX_TYPE = 8
class WDC_DIAG_ADDR_SPACE_INFO(object):
    dwAddrSpace = 0
    sType = ""
    sName = ""
    sDesc = ""

def WDC_DIAG_ERR(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs),

def fprintf(fp, string):
    if fp == None:
        print(string, end = '')
    else:
        fp.write(string)

# -----------------------------------------------
#    PCI/ISA
# -----------------------------------------------

# Print device's resources information to file.
# For a registered device (hCard != 0), print also kernel and user-mode
# mappings of memory address items
def WDC_DIAG_DeviceResourcesPrint(pCard, hCard, fp):
    resources = 0

    for i in range(pCard.dwItems):
        pItem = pCard.Item[i]
        if pItem.item == ITEM_MEMORY:
            resources += 1
            fprintf(fp, "    Memory range [BAR %ld]: base 0x%X, "
                "size 0x%X\n" % (pItem.I.Mem.dwBar,
                pItem.I.Mem.pPhysicalAddr, pItem.I.Mem.qwBytes))

            if hCard: # Registered device
                fprintf(fp, "        Kernel-mode address mapping: 0x%X\n" %
                    pItem.I.Mem.pTransAddr)
                fprintf(fp, "        User-mode address mapping: 0x%X\n" %
                    pItem.I.Mem.pUserDirectAddr)

        elif pItem.item == ITEM_IO:
            resources += 1
            fprintf(fp, "    I/O range [BAR %ld]: base [0x%X], size "
                "[0x%lX]\n" % (pItem.I.IO.dwBar, pItem.I.IO.pAddr,
                pItem.I.IO.dwBytes))

        elif pItem.item == ITEM_INTERRUPT:
            resources += 1
            fprintf(fp, "    Interrupt: IRQ %ld\n" % pItem.I.Int.dwInterrupt)
            fprintf(fp, "    Interrupt Options (supported interrupts):\n")

            if WDC_INT_IS_MSI(pItem.I.Int.dwOptions):
                fprintf(fp, "        %s\n" %
                    WDC_DIAG_IntTypeDescriptionGet(pItem.I.Int.dwOptions))

            # According to the MSI specification, it is recommended that
            # a PCI device will support both MSI/MSI-X and level-sensitive
            # interrupts, and allow the operating system to choose which
            # type of interrupt to use.
            if pItem.I.Int.dwOptions & INTERRUPT_LEVEL_SENSITIVE:
                fprintf(fp, "        %s\n" %
                    WDC_DIAG_IntTypeDescriptionGet(INTERRUPT_LEVEL_SENSITIVE))
            elif not WDC_INT_IS_MSI(pItem.I.Int.dwOptions):
            # MSI/MSI-X interrupts are always edge-triggered, so there is no
            # no need to display a specific edge-triggered indication for
            # such interrupts.
                fprintf(fp, "        %s\n" %
                    WDC_DIAG_IntTypeDescriptionGet(INTERRUPT_LATCHED))

        elif pItem.item == ITEM_BUS:
            pass
        else:
            fprintf(fp, "    Invalid item type (0x%lx)\n" % pItem.item)

    if not resources:
        fprintf(fp, "    Device has no resources\n")

def WDC_DIAG_PrintIndent(string, indent, max_width, br_at_end):
    if len(string) > 0 and indent <= 80:
        for i in range(len(string)):
            fprintf(None, string[i])
            if string[i] == '\n' and i < len(string) - 1 :
                for j in range(indent):
                    fprintf(None, " ")

        for k in range(len(string), max_width):
            fprintf(None, " ")

        if br_at_end:
            fprintf(None, "\n")

# Print run-time registers and PCI configuration registers information
def WDC_DIAG_RegsInfoPrint(hDev, pRegs, dwNumRegs, options, isExpress):
    extended = PCIE_REGS_NUM if isExpress else 0
    pciExpressOffset = DWORD(0)
    headerType = WDC_PCI_HEADER_TYPE()

    if not hDev:
        WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: Error - NULL WDC device handle\n")
        return

    if isExpress:
        dwStatus = wdapi.WDC_PciGetExpressOffset(hDev, byref(pciExpressOffset))
        if dwStatus:
            WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: Error getting PCI Express"
                " Offset\n")
            return
    dwStatus = wdapi.WDC_PciGetHeaderType(hDev, byref(headerType))
    if dwStatus:
        WDC_DIAG_ERR("WDC_DIAG_RegsInfoPrint: Error - unable to determine"
            "PCI header type")
        return

    if not dwNumRegs:
        print("There are currently no pre-defined registers to display\n")
        return

    if not pRegs:
        WDC_DIAG_ERR("WDC_DIAG_RegsInfoPrint: Error - NULL registers "
            "information pointer\n")
        return

    if not options:
        options = WDC_DIAG_REG_PRINT_DEFAULT

    fName = options & WDC_DIAG_REG_PRINT_NAME
    fDesc = options & WDC_DIAG_REG_PRINT_DESC
    fAddrSpace = options & WDC_DIAG_REG_PRINT_ADDR_SPACE
    fOffset = options & WDC_DIAG_REG_PRINT_OFFSET
    fSize = options & WDC_DIAG_REG_PRINT_SIZE
    fDir = options & WDC_DIAG_REG_PRINT_DIRECTION

    fprintf(None, "\n")
    fprintf(None, "PCI %sRegisters\n" % ("Express " if isExpress else "")),
    fprintf(None, "----%s---------\n" % ("--------" if isExpress else "")),
    fprintf(None, "%3s %-*s %-*s %-*s %-*s %-*s %s\n" % ("", MAX_NAME_DISPLAY,
        "Name" if fName else "", 4, "BAR" if fAddrSpace else "",
        WDC_SIZE_32 * 2 + 2, "Offset" if fOffset else "", 5,
        "Size" if fSize else "", 4, "R/W" if fDir else "",
        "Description" if fDesc else "")),

    fprintf(None, "%3s %-*s %-*s %-*s %-*s %-*s %s\n" % (
        "", MAX_NAME_DISPLAY, "----" if fName else "", 4,
        "---" if fAddrSpace else "", WDC_SIZE_32 * 2 + 2,
        "------" if fOffset else "", 5, "----" if fSize else "", 4,
        "---" if fDir else "", "-----------" if fDesc else "")),

    for i, pReg in enumerate(pRegs, 1):
        fprintf(None, "%2ld. " % (i + extended)),

        if fName:
            fprintf(None, "%-*.*s " % (MAX_NAME_DISPLAY, MAX_NAME_DISPLAY,
                pReg.sName)),
        else:
            fprintf(None, "%*s " % (MAX_NAME_DISPLAY, "")),

        if fAddrSpace and WDC_AD_CFG_SPACE != pReg.dwAddrSpace:
            fprintf(None, "%2ld %*s" % (pReg.dwAddrSpace, 2, "")),
        else:
            fprintf(None, "%4s " % ""),

        if fOffset:
            fprintf(None, "0x%-*lX " % (WDC_SIZE_32 * 2, pReg.dwOffset +
                pciExpressOffset.value)),
        else:
            fprintf(None, "%*s " % (WDC_SIZE_32 * 2 + 2, "")),

        if fSize:
            fprintf(None, "%*ld %*s" % (2, pReg.dwSize, 3, "")),
        else:
            fprintf(None, "%*s " % (5, "")),

        if fDir:
            fprintf(None, "%-*s " % (4, \
                "RW" if WDC_READ_WRITE == pReg.direction else
                "R" if WDC_READ == pReg.direction else "W")),
        else:
            fprintf(None, "%*s" % (4, "")),
        if fDesc and pReg.sDesc != "":
            WDC_DIAG_PrintIndent(pReg.sDesc, REGS_INFO_PRINT_DETAILS_INDENT,
                REGS_INFO_PRINT_DETAILS_WIDTH, True)
        else:
            fprintf(None, "\n")

# Set address access mode
def WDC_DIAG_SetMode():
    print("")
    print("Select read/write mode:")
    print("-----------------------")
    print("1. 8 bits (%ld bytes)" % WDC_SIZE_8)
    print("2. 16 bits (%ld bytes)" % WDC_SIZE_16)
    print("3. 32 bits (%ld bytes)" % WDC_SIZE_32)
    print("4. 64 bits (%ld bytes)" % WDC_SIZE_64)
    print("99. Cancel")

    (option, dwStatus) = DIAG_GetMenuOption(4)

    if dwStatus != DIAG_INPUT_SUCCESS:
        return (-1, DIAG_INPUT_FAIL)

    if option == 1:
        return (WDC_MODE_8, DIAG_INPUT_SUCCESS)
    elif option == 2:
        return (WDC_MODE_16, DIAG_INPUT_SUCCESS)
    elif option == 3:
        return (WDC_MODE_32, DIAG_INPUT_SUCCESS)
    elif option == 4:
        return (WDC_MODE_64, DIAG_INPUT_SUCCESS)

    print("Invalid selection")
    return (-1, DIAG_INPUT_FAIL)

# Read/write a memory or I/O address
def WDC_DIAG_ReadWriteAddr(hDev, direction, dwAddrSpace, mode):
    bData = BYTE()
    wData = WORD()
    u32Data = UINT32()
    u64Data = UINT64()

    if not hDev:
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteAddr: Error- NULL WDC device handle\n")
        return

    (dwOffset, dwStatus) = DIAG_InputNum("Enter offset to read from"
        if (WDC_READ == direction) else "Enter offset to write to", True, \
            sizeof(DWORD), 0, 0)
    if DIAG_INPUT_SUCCESS != dwStatus:
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteAddr: %s getting the offset\n" %
            "Canceled" if dwStatus == DIAG_INPUT_CANCEL else "Failed")
        return

    if WDC_WRITE == direction:
        if WDC_MODE_8 == mode:
            msg = "Enter data to write (max value: 0x%lX)" % MaxVal.UI8
            (bData, dwStatus) = DIAG_InputNum(msg, True, sizeof(BYTE), 0, 0)
        elif WDC_MODE_16 == mode:
            msg = "Enter data to write (max value: 0x%lX)" % MaxVal.UI16
            (wData, dwStatus) = DIAG_InputNum(msg, True, sizeof(WORD), 0, 0)
        elif WDC_MODE_32 == mode:
            msg = "Enter data to write (max value: 0x%lX)" % MaxVal.UI32
            (u32Data, dwStatus) = DIAG_InputNum(msg, True, sizeof(UINT32), 0, 0)
        else:
            msg = "Enter data to write (max value: 0x%lX)" % MaxVal.UI64
            (u64Data, dwStatus) = DIAG_InputNum(msg, True, sizeof(UINT64), 0, 0)

        if DIAG_INPUT_SUCCESS != dwStatus:
            WDC_DIAG_ERR("WDC_DIAG_ReadWriteAddr: Failed getting data to"
                " write\n")
            return

    if mode == WDC_MODE_8:
        dwStatus = \
        wdapi.WDC_ReadAddr8(hDev, dwAddrSpace, dwOffset, byref(bData)) \
            if WDC_READ == direction \
            else wdapi.WDC_WriteAddr8(hDev, dwAddrSpace, dwOffset, bData)
        if WD_STATUS_SUCCESS == dwStatus:
            print("%s 0x%X %s offset 0x%lX in BAR %ld\n" %
                ("Read" if WDC_READ == direction else "Wrote",
                bData.value, "from" if WDC_READ == direction
                else "to", dwOffset.value, dwAddrSpace))

    elif mode == WDC_MODE_16:
        dwStatus = \
            wdapi.WDC_ReadAddr16(hDev, dwAddrSpace, dwOffset, byref(wData)) \
            if WDC_READ == direction \
            else wdapi.WDC_WriteAddr16(hDev, dwAddrSpace, dwOffset, wData)
        if WD_STATUS_SUCCESS == dwStatus:
            print("%s 0x%hX %s offset 0x%lX in BAR %ld\n" %
                ("Read" if WDC_READ == direction else "Wrote", wData.value,
                "from" if WDC_READ == direction else "to", dwOffset.value,
                dwAddrSpace))

    elif mode == WDC_MODE_32:
        dwStatus = \
            wdapi.WDC_ReadAddr32(hDev, dwAddrSpace, dwOffset, byref(u32Data)) \
            if WDC_READ == direction \
            else wdapi.WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data)
        if WD_STATUS_SUCCESS == dwStatus:
            print("%s 0x%X %s offset 0x%lX in BAR %ld\n" %
                ("Read" if WDC_READ == direction else "Wrote", u32Data.value,
                "from" if WDC_READ == direction else "to", dwOffset.value,
                dwAddrSpace))

    elif mode == WDC_MODE_64:
        dwStatus = \
            wdapi.WDC_ReadAddr64(hDev, dwAddrSpace, dwOffset, byref(u64Data)) \
            if WDC_READ == direction \
            else wdapi.WDC_WriteAddr64(hDev, dwAddrSpace, dwOffset, u64Data)
        if WD_STATUS_SUCCESS == dwStatus:
            print(("%s 0x%lX %s offset 0x%lX in BAR %ld\n") %
                ("Read" if WDC_READ == direction else "Wrote", u64Data.value,
                "from" if WDC_READ == direction else "to", dwOffset.value,
                dwAddrSpace))

    else:
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteAddr: Error - Invalid mode (%d)\n",
            mode)
        return

    if WD_STATUS_SUCCESS != dwStatus:
        print("Failed to %s offset 0x%lX in BAR %ld. Error 0x%lx - %s\n" %
            ("read from" if WDC_READ == direction else "write to", dwOffset,
            dwAddrSpace, dwStatus, Stat2Str(dwStatus)))

# Read/write a memory or I/O address OR an offset in the PCI configuration
# space (dwAddrSpace == WDC_AD_CFG_SPACE)
def WDC_DIAG_ReadWriteBlock(hDev, direction, dwAddrSpace):
    sDir = "read" if WDC_READ == direction else "write"

    if not hDev:
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteBlock: Error- NULL WDC device "
            "handle\n")
        return

    (dwOffset, dwStatus) = DIAG_InputNum("Enter offset to read from"
        if WDC_READ == direction else "Enter offset to write to", True, \
            sizeof(DWORD), 0, 0)
    if DIAG_INPUT_SUCCESS != dwStatus:
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteBlock: %s getting the offset\n" %
            "Cancelled" if dwStatus == DIAG_INPUT_CANCEL else "Failed")
        return

    (dwBytes, dwStatus) = DIAG_InputNum("Enter number of bytes to transfer",
        True, sizeof(DWORD), 0, 0)
    if DIAG_INPUT_SUCCESS != dwStatus:
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteBlock: %s getting the number of bytes\n"
            % "Cancelled" if dwStatus == DIAG_INPUT_CANCEL else "Failed")
        return

    if not dwBytes:
        return

    if WDC_WRITE == direction:
        print("data to write (hex format): 0x")
        (sInput, dwBytesRead) = DIAG_GetHexBuffer(dwBytes.value)
        if not dwBytesRead:
            return

    pBuf = create_string_buffer(dwBytes.value) if WDC_READ == direction else \
        (c_char * len(sInput)).from_buffer(sInput)

    if WDC_AD_CFG_SPACE == dwAddrSpace: # Read/write a configuration/attribute
                                        # space address
        busType = wdapi.WDC_GetBusType(hDev)
        if WD_BUS_PCI == busType: # Read/write PCI configuration space
                                  # offset
            if direction == WDC_READ:
                dwStatus = wdapi.WDC_PciReadCfg(hDev, dwOffset, pBuf, dwBytes)
            else:
                dwStatus = wdapi.WDC_PciWriteCfg(hDev, dwOffset, pBuf, dwBytes)
        else:
            print("Error - Cannot read/write configuration space address "
                "space for bus type [0x%lx]\n" % busType)
            return
    else: # Read/write a memory or I/O address
        (mode, dwStatus) = WDC_DIAG_SetMode()
        if DIAG_INPUT_SUCCESS != dwStatus:
            print("Invalid input\n")
            return

        (fAutoInc, dwStatus) = DIAG_InputNum("Do you wish to increment the "
            "address after each %s block(0x%lX bytes) (0 - No, Otherwise -"
            " Yes)? " % (sDir, mode), False, sizeof(DWORD), 0, 0)
        if DIAG_INPUT_SUCCESS != dwStatus:
            print("Invalid input\n")
            return

        fAutoInc = bool(fAutoInc.value)
        options = WDC_ADDR_RW_DEFAULT if fAutoInc else WDC_ADDR_RW_NO_AUTOINC

        dwStatus = \
            wdapi.WDC_ReadAddrBlock(hDev, dwAddrSpace, dwOffset, dwBytes,
                byref(pBuf), mode, options) if direction == WDC_READ else \
            wdapi.WDC_WriteAddrBlock(hDev, dwAddrSpace, dwOffset, dwBytes,
                byref(pBuf), mode, options)

    if WD_STATUS_SUCCESS == dwStatus:
        if WDC_READ == direction:
            DIAG_PrintHexBuffer(pBuf, dwBytes)
            print()
        else:
            print("Wrote 0x%lX bytes to offset 0x%lX\n" % (dwBytes.value,
                dwOffset.value))
    else:
        print("Failed to %s 0x%lX bytes %s offset 0x%lX. Error 0x%lx - %s\n" %
            (sDir, dwBytes.value, "from" if WDC_READ == direction else "to",
            dwOffset.value, dwStatus, Stat2Str(dwStatus)))

#Exit:
    print("Press ENTER to continue\n")
    inputf()

# Read all pre-defined run-time or PCI configuration registers and display
# results
def WDC_DIAG_ReadRegsAll(hDev, pRegs, dwNumRegs, fPciCfg, fExpress):
    bData = BYTE()
    wData = WORD()
    u32Data = UINT32()
    u64Data = UINT64()
    outBufLen = DWORD()
    pciExpressOffset = DWORD()
    headerType = WDC_PCI_HEADER_TYPE()
    details = create_string_buffer(1024)

    if not hDev:
        WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: Error - NULL WDC device handle\n")
        return

    if fExpress:
        dwStatus = wdapi.WDC_PciGetExpressOffset(hDev, byref(pciExpressOffset))
        if dwStatus:
            WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: Error getting PCI Express"
                " Offset\n")
            return
    dwStatus = wdapi.WDC_PciGetHeaderType(hDev, byref(headerType))
    if dwStatus:
        WDC_DIAG_ERR("WDC_DIAG_RegsInfoPrint: Error - unable to determine"
            "PCI header type")
        return

    if not dwNumRegs or not pRegs:
        WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: %s\n" %
            ("No registers (dwNumRegs == 0)" if not dwNumRegs
            else "Error - NULL registers pointer"))
        return

    fprintf(None, "\n")
    fprintf(None, "%s registers data:\n" % ("PCI Express configuration" \
        if fPciCfg and fExpress else "PCI configuration" if fPciCfg else
        "run-time")),
    fprintf(None, "---------------------------------\n\n"),
    fprintf(None, "%3s %-*s %-*s  %s           %s\n" % ("" , MAX_NAME_DISPLAY,
        "Name", 4 * 2 + 2, "Data" , "Description" , "Details")),
    fprintf(None, "%3s %-*s %-*s  %s           %s\n" % ("" , MAX_NAME_DISPLAY,
        "----", 4 * 2 + 2, "----" , "-----------" , "-------")),

    for i, pReg in enumerate(pRegs, 1):
        fprintf(None, "%2ld. %-*.*s " % (i, MAX_NAME_DISPLAY, MAX_NAME_DISPLAY,
            pReg.sName)),
        if WDC_WRITE == pReg.direction:
            fprintf(None, "Write-only register\n"),
            continue

        if pReg.dwSize == WDC_SIZE_8:
            dwStatus = \
                wdapi.WDC_PciReadCfg8(hDev, pReg.dwOffset +
                    pciExpressOffset.value, byref(bData)) if fPciCfg else \
                wdapi.WDC_ReadAddr8(hDev, pReg.dwAddrSpace, pReg.dwOffset +
                    pciExpressOffset, byref(bData))
            if WD_STATUS_SUCCESS == dwStatus:
                fprintf(None, "0x%-*X  " % (WDC_SIZE_64 , bData.value))

        elif pReg.dwSize == WDC_SIZE_16:
            dwStatus = \
                wdapi.WDC_PciReadCfg16(hDev, pReg.dwOffset +
                    pciExpressOffset.value, byref(wData)) if fPciCfg else \
                wdapi.WDC_ReadAddr16(hDev, pReg.dwAddrSpace, pReg.dwOffset +
                    pciExpressOffset.value, byref(wData))
            if WD_STATUS_SUCCESS == dwStatus:
                fprintf(None, "0x%-*hX  " % (WDC_SIZE_64 , wData.value))

        elif pReg.dwSize == WDC_SIZE_32:
            dwStatus = \
                wdapi.WDC_PciReadCfg32(hDev, pReg.dwOffset +
                    pciExpressOffset.value, byref(u32Data)) if fPciCfg else \
                wdapi.WDC_ReadAddr32(hDev, pReg.dwAddrSpace, pReg.dwOffset +
                    pciExpressOffset.value, byref(u32Data))
            if WD_STATUS_SUCCESS == dwStatus:
                fprintf(None, "0x%-*X  " % (WDC_SIZE_64, u32Data.value))

        elif pReg.dwSize == WDC_SIZE_64:
            dwStatus = \
                wdapi.WDC_PciReadCfg64(hDev, pReg.dwOffset +
                    pciExpressOffset.value, byref(u64Data)) if fPciCfg else \
                wdapi.WDC_ReadAddr64(hDev, pReg.dwAddrSpace, pReg.dwOffset +
                    pciExpressOffset.value, byref(u64Data))
            if WD_STATUS_SUCCESS == dwStatus:
                fprintf(None, "0x%-*lX  " % (WDC_SIZE_64, u64Data.value))

        else:
            print("Invalid register size (%ld)\n" % pReg.dwSize),
            return

        if WD_STATUS_SUCCESS != dwStatus:
            print("Error: 0x%-*lx  " % (WDC_SIZE_64 * 2 - 7, dwStatus)),
            return

        WDC_DIAG_PrintIndent(pReg.sDesc, READ_REGS_ALL_DESC_INDENT,
            READ_REGS_ALL_DESC_WIDTH, False)
        if fPciCfg:
            dwStatus = \
                wdapi.PciExpressConfRegData2Str(hDev, pReg.dwOffset, details,
                    sizeof(details), byref(outBufLen)) if fExpress else \
                wdapi.PciConfRegData2Str(hDev, pReg.dwOffset, details,
                    sizeof(details), byref(outBufLen))
            if outBufLen.value > 0:
                WDC_DIAG_PrintIndent(details.value.decode("utf-8").strip() +
                    "\n", READ_REGS_ALL_DETAILS_INDENT, 0, True)

            else:
                fprintf(None, "\n")
        else:
            fprintf(None, "\n")

        details = create_string_buffer(1024)
        outBufLen = DWORD(0)

    print("\nPress ENTER to continue")
    inputf()

#   Display a list of pre-defined run-time or PCI configuration registers
#   and let user select to read/write from/to a specific register
def WDC_DIAG_ReadWriteReg(hDev, pRegs, dwNumRegs, direction, fPciCfg,
        fExpressReg):
    bData = BYTE()
    wData = WORD()
    u32Data = UINT32()
    u64Data = UINT64()
    outBufLen = DWORD()
    fExpress = wdapi.WDC_PciGetExpressGen(hDev) != 0 and fExpressReg
    pciExpressOffset = DWORD()
    headerType = WDC_PCI_HEADER_TYPE()
    details = create_string_buffer(1024)

    if not hDev:
        WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: Error - NULL WDC device handle\n")
        return

    if fExpress:
        dwStatus = wdapi.WDC_PciGetExpressOffset(hDev, byref(pciExpressOffset))
        if dwStatus:
            WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: Error getting PCI Express"
                " Offset\n")
            return
    dwStatus = wdapi.WDC_PciGetHeaderType(hDev, byref(headerType))
    if dwStatus:
        WDC_DIAG_ERR("WDC_DIAG_RegsInfoPrint: Error - unable to determine"
            "PCI header type")
        return
    if not dwNumRegs or not pRegs:
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteReg: %s\n" %
            "No registers to read/write (dwNumRegs == 0)" if not dwNumRegs else
            "Error - NULL registers pointer")
        return

    # Display pre-defined registers' information
    print("")
    print("PCI %s registers:" % "configuration" if fPciCfg else "run-time")
    print("----------------------------")
    WDC_DIAG_RegsInfoPrint(hDev, pRegs, dwNumRegs, WDC_DIAG_REG_PRINT_ALL,
        False)

    # Read/write register
    (dwReg, dwStatus) = DIAG_InputNum("\nSelect a register from the list"
        " above to %s or 0 to cancel: " % ("read from" if \
        WDC_READ == direction else "write to"), False, \
            sizeof(DWORD), 1, dwNumRegs)
    if DIAG_INPUT_SUCCESS != dwStatus:
        print("Invalid selection")
        return

    pReg = pRegs[dwReg.value - 1]

    if (WDC_READ == direction and WDC_WRITE == pReg.direction) or \
        (WDC_WRITE == direction and WDC_READ == pReg.direction):
        print("Error - you have selected to %s a %s-only register" %
            ("read from" if WDC_READ == direction else "write to" ,
            "write" if WDC_WRITE == pReg.direction else "read"))
        return

    if WDC_WRITE == direction:
        (tmpData, dwStatus) = \
            DIAG_InputNum("Enter data to write to register", True, \
                sizeof(bData if WDC_SIZE_8 == pReg.dwSize else
                wData if WDC_SIZE_16 == pReg.dwSize else
                u32Data if WDC_SIZE_32 == pReg.dwSize else u64Data), 0, 0)

    if pReg.dwSize == WDC_SIZE_8:
        if WDC_READ == direction:
            dwStatus = \
                wdapi.WDC_PciReadCfg8(hDev, pReg.dwOffset +
                    pciExpressOffset.value, byref(bData)) if fPciCfg else \
                wdapi.WDC_ReadAddr8(hDev, pReg.dwAddrSpace, pReg.dwOffset +
                    pciExpressOffset.value, byref(bData))
        else:
            bData = BYTE(tmpData.value)
            dwStatus = wdapi.WDC_PciWriteCfg8(hDev, pReg.dwOffset +
                    pciExpressOffset.value, bData) if fPciCfg else \
                wdapi.WDC_WriteAddr8(hDev, pReg.dwAddrSpace, pReg.dwOffset +
                    pciExpressOffset.value, bData)
        if WD_STATUS_SUCCESS == dwStatus:
            print("%s 0x%X %s register %s at offset [0x%lx]\n" % (
                "Read" if WDC_READ == direction else "Wrote", bData.value,
                "from" if WDC_READ == direction else "to", pReg.sName,
                pReg.dwOffset + pciExpressOffset.value if fExpress else
                pReg.dwOffset))

    elif pReg.dwSize == WDC_SIZE_16:
        if WDC_READ == direction:
            dwStatus = \
                wdapi.WDC_PciReadCfg16(hDev, pReg.dwOffset +
                    pciExpressOffset.value, byref(wData)) if fPciCfg else \
                wdapi.WDC_ReadAddr16(hDev, pReg.dwAddrSpace, pReg.dwOffset +
                    pciExpressOffset.value, byref(wData))
        else:
            wData = WORD(tmpData.value)
            dwStatus = \
                wdapi.WDC_PciWriteCfg16(hDev, pReg.dwOffset +
                    pciExpressOffset.value, wData) if fPciCfg else \
                wdapi.WDC_WriteAddr16(hDev, pReg.dwAddrSpace, pReg.dwOffset +
                    pciExpressOffset.value, wData)
        if WD_STATUS_SUCCESS == dwStatus:
            print("%s [0x%hX] %s register %s at offset [0x%lx]\n" %
                ("Read" if WDC_READ == direction else "Wrote", wData.value,
                "from" if WDC_READ == direction else "to", pReg.sName,
                pReg.dwOffset + pciExpressOffset.value if fExpress else
                pReg.dwOffset))

    elif pReg.dwSize == WDC_SIZE_32:
        if WDC_READ == direction:
            dwStatus = \
                wdapi.WDC_PciReadCfg32(hDev, pReg.dwOffset +
                    pciExpressOffset.value, byref(u32Data)) if fPciCfg else \
                wdapi.WDC_ReadAddr32(hDev, pReg.dwAddrSpace, pReg.dwOffset +
                    pciExpressOffset.value, byref(u32Data))
        else:
            u32Data = UINT32(tmpData.value)
            dwStatus = \
                wdapi.WDC_PciWriteCfg32(hDev, pReg.dwOffset +
                    pciExpressOffset.value, u32Data) if fPciCfg else \
                wdapi.WDC_WriteAddr32(hDev, pReg.dwAddrSpace, pReg.dwOffset +
                    pciExpressOffset.value, u32Data)
        if WD_STATUS_SUCCESS == dwStatus:
            print("%s [0x%X] %s register %s at offset [0x%lx]\n" %
                ("Read" if WDC_READ == direction else "Wrote", u32Data.value,
                "from" if WDC_READ == direction else "to", pReg.sName,
                pReg.dwOffset + pciExpressOffset.value if fExpress else
                pReg.dwOffset))

    elif pReg.dwSize == WDC_SIZE_64:
        if WDC_READ == direction:
            dwStatus = \
                wdapi.WDC_PciReadCfg64(hDev, pReg.dwOffset +
                    pciExpressOffset.value, byref(u64Data)) if fPciCfg else \
                wdapi.WDC_ReadAddr64(hDev, pReg.dwAddrSpace, pReg.dwOffset +
                    pciExpressOffset.value, byref(u64Data))
        else:
            u64Data = UINT64(tmpData.value)
            dwStatus = \
                wdapi.WDC_PciWriteCfg64(hDev, pReg.dwOffset +
                    pciExpressOffset.value, u64Data) if fPciCfg else \
                wdapi.WDC_WriteAddr64(hDev, pReg.dwAddrSpace, pReg.dwOffset +
                    pciExpressOffset.value, u64Data)
        if WD_STATUS_SUCCESS == dwStatus:
            print("%s [0x%lX] %s register %s at offset [0x%lx]\n" %
                ("Read" if WDC_READ == direction else "Wrote", u64Data.value,
                "from" if WDC_READ == direction else "to", pReg.sName,
                pReg.dwOffset + pciExpressOffset.value if fExpress
                else pReg.dwOffset))
    else:
        print("Invalid register size (%ld)" % pReg.dwSize)
        return

    if WD_STATUS_SUCCESS != dwStatus:
        print("Failed %s %s. Error [0x%lx - %s]" %
            ("reading data from" if WDC_READ == direction else
            "writing data to", pReg.sName, dwStatus,
            Stat2Str(dwStatus)))

    if WDC_READ == direction:
        dwStatus = \
            wdapi.PciExpressConfRegData2Str(hDev, pReg.dwOffset, details,
                sizeof(details), byref(outBufLen)) if fExpress else \
            wdapi.PciConfRegData2Str(hDev, pReg.dwOffset, details,
                sizeof(details), byref(outBufLen))
        print("Decoded register data:\n%s" % (details.value.decode("utf-8")
            if len(details) else ""))

    print("Press ENTER to continue")
    inputf()

# Display available PCI/PCIe capabilities
def WDC_DIAG_ScanPCICapabilities(hDev, fExpress):
    scanResult = WDC_PCI_SCAN_CAPS_RESULT()
    dwCapId = DWORD(WD_PCI_CAP_ID_ALL)
    option = 0

    if not hDev:
        WDC_DIAG_ERR("WDC_DIAG_ScanPCICapabilities: Error - NULL WDC device "
            "handle\n")
        return
    while True:
        print("\nSelect scan option (PCI/PCI-Express, all/specific):")
        print("-----------------------")
        print("1. Scan PCI capabilities")
        print("2. Scan specific PCI capability")
        if fExpress:
            print("3. Scan PCI Express extended capabilities")
            print("4. Scan specific PCI Express extended capability")
        print("99. Exit menu")
        print("")
        (option, dwStatus) = DIAG_GetMenuOption(4 if fExpress else 2)

        if dwStatus != DIAG_INPUT_SUCCESS or option == 99:
            print("Invalid input")
            return (-1, DIAG_INPUT_FAIL)

        if option == 2 or option == 4:
            (dwCapId, dwStatus) = DIAG_InputNum("Enter requested %scapability"
                " ID (hexadecimal): 0x" % ("extended " if option == 4 else ""),
                False, sizeof(DWORD), 0, 0)
            if DIAG_INPUT_SUCCESS != dwStatus:
                print("Invalid selection\n")
                return

        # Scan PCI/PCIe Capabilities
        if option <= 2:
            dwStatus = wdapi.WDC_PciScanCaps(hDev, dwCapId, byref(scanResult))
        else:
            dwStatus = wdapi.WDC_PciScanExtCaps(hDev, dwCapId,
                byref(scanResult))

        if WD_STATUS_SUCCESS != dwStatus:
            WDC_DIAG_ERR("WDC_DIAG_ScanPCICapabilities: Failed scanning PCI "
                "capabilities. Error [0x%lx - %s]\n" % (dwStatus,
                Stat2Str(dwStatus)))
        else:
            print("%sPCI %scapabilities found\n" % ("" if
                scanResult.dwNumCaps else "No ", "Express extended "
                if option == 3 or option == 4 else ""))
            for i in range(scanResult.dwNumCaps):
                print("    %ld) %s - ID [0x%lx], offset [0x%lx]" % (i + 1,
                    GET_EXTENDED_CAPABILITY_STR(scanResult.pciCaps[i].dwCapId)
                    if option == 3 or option == 4 else
                    GET_CAPABILITY_STR(scanResult.pciCaps[i].dwCapId),
                    scanResult.pciCaps[i].dwCapId,
                    scanResult.pciCaps[i].dwCapOffset))

        print("\nPress ENTER to proceed to next device")
        inputf()

def WDC_DIAG_IntTypeDescriptionGet(dwIntType):
    if dwIntType & INTERRUPT_MESSAGE_X:
        return "Extended Message-Signaled Interrupt (MSI-X)"
    elif dwIntType & INTERRUPT_MESSAGE:
        return "Message-Signaled Interrupt (MSI)"
    elif dwIntType & INTERRUPT_LEVEL_SENSITIVE:
        return "Level-Sensitive Interrupt"
    return "Edge-Triggered Interrupt"

# -----------------------------------------------
#    PCI
# -----------------------------------------------

# Print location and resources information for all connected PCI devices
def WDC_DIAG_PciDevicesInfoPrintAll(dump_cfg):
    WDC_DIAG_PciDevicesInfoPrintAllFile(None, dump_cfg)

# Print location and resources information for all connected PCI devices to
# file
def WDC_DIAG_PciDevicesInfoPrintAllFile(fp, dump_cfg):
    scanResult = WDC_PCI_SCAN_RESULT()
    dwStatus = wdapi.WDC_PciScanDevices(0, 0, byref(scanResult))

    if (WD_STATUS_SUCCESS != dwStatus):
        fprintf(fp, "Failed scanning PCI bus. Error [0x%lx - %s]\n" % (dwStatus,
            Stat2Str(dwStatus)))
        return

    dwNumDevices = scanResult.dwNumDevices
    if not dwNumDevices:
        fprintf(fp, "No devices were found on the PCI bus\n")
        return

    fprintf(fp, "\n")
    fprintf(fp, "Found %ld devices on the PCI bus:\n" % dwNumDevices)
    fprintf(fp, "---------------------------------\n")

    for i in range (dwNumDevices):
        fprintf(fp, "%2ld. Vendor ID: [0x%lX], Device ID: [0x%lX]\n" % (i + 1,
            scanResult.deviceId[i].dwVendorId,
            scanResult.deviceId[i].dwDeviceId))

        WDC_DIAG_PciDeviceInfoPrintFile(scanResult.deviceSlot[i], fp, dump_cfg)
        if not fp:
            print("Press ENTER to proceed to next device")
            inputf()

# Print PCI device location information to file
def WDC_DIAG_PciSlotPrintFile(pPciSlot, fp):
    fprintf(fp, "Domain [0x%lx], Bus [0x%lx], Slot [0x%lx], Function [0x%lx]\n" %
        (pPciSlot.dwDomain, pPciSlot.dwBus, pPciSlot.dwSlot, pPciSlot.dwFunction))

# Print PCI device location and resources information
def WDC_DIAG_PciDeviceInfoPrint(pPciSlot, dump_cfg):
    WDC_DIAG_PciDeviceInfoPrintFile(pPciSlot, None, dump_cfg)

# Print PCI device location and resources information to file
def WDC_DIAG_PciDeviceInfoPrintFile(pPciSlot, fp, dump_cfg):
    deviceInfo = WD_PCI_CARD_INFO()

    fprintf(fp, "    Location: ")
    WDC_DIAG_PciSlotPrintFile(pPciSlot, fp)

    if dump_cfg:
        config = UINT32()
        for dwOffset in range(0, 256, sizeof(UINT32)):
            dwStatus = wdapi.WDC_PciReadCfgBySlot32(byref(pPciSlot), dwOffset,
                    byref(config))

            if dwStatus:
                WDC_DIAG_ERR("    Failed reading PCI configuration space.\n"
                    "    Error [0x%lx - %s]\n" % (dwStatus,
                    Stat2Str(dwStatus)))
                return

            if (dwOffset / 4) % 8 == 0:
                fprintf(fp, "%02lx " % dwOffset)

            fprintf(fp, "%08x " % config.value)
            if (dwOffset / 4) % 8 == 7:
                fprintf(fp, "\n")

    deviceInfo.pciSlot = pPciSlot
    dwStatus = wdapi.WDC_PciGetDeviceInfo(byref(deviceInfo))
    if ((WD_NO_RESOURCES_ON_DEVICE != dwStatus) and
        (WD_STATUS_SUCCESS != dwStatus)):
        WDC_DIAG_ERR("    Failed retrieving PCI resources information.\n"
            "    Error 0x%lx - %s\n" % (dwStatus, Stat2Str(dwStatus)))
        return

    WDC_DIAG_DeviceResourcesPrint(deviceInfo.Card, 0, fp)

    dwExpressGen = wdapi.WDC_PciGetExpressGenBySlot(byref(pPciSlot))
    if (dwExpressGen != 0):
        fprintf(fp, "    PCI Express Generation: Gen%ld\n" % dwExpressGen)
    fprintf(fp, "\n")

def WDC_DIAG_DeviceFindAndOpen(dwVendorId, dwDeviceId, gpDevCtx, pcKpName,
    PCI_DEV_ADDR_DESC):

    # Find device
    slot = WDC_DIAG_DeviceFind(dwVendorId, dwDeviceId)
    if not slot:
        return None

    # Open a device handle
    return WDC_DIAG_DeviceOpen(slot, gpDevCtx, pcKpName, PCI_DEV_ADDR_DESC)

# Find a PCI device
def WDC_DIAG_DeviceFind(dwVendorId, dwDeviceId):
    scanResult = WDC_PCI_SCAN_RESULT()

    if not dwVendorId:
        # Get vendor ID
        (dwVendorId, dwStatus) = DIAG_InputNum("Enter vendor ID", True,
            sizeof(DWORD), 0, 0)
        if DIAG_INPUT_SUCCESS != dwStatus:
            return None

        # Get device ID
        (dwDeviceId, dwStatus) = DIAG_InputNum("Enter device ID", True,
            sizeof(DWORD), 0, 0)
        if DIAG_INPUT_SUCCESS != dwStatus:
            return None

    # Scan PCI devices
    dwStatus = wdapi.WDC_PciScanDevices(dwVendorId, dwDeviceId,
        byref(scanResult))
    if WD_STATUS_SUCCESS != dwStatus:
        WDC_DIAG_ERR("DeviceFind: Failed scanning the PCI bus.\n"
            "Error [0x%lx - %s]\n" % (dwStatus, Stat2Str(dwStatus)))
        return None

    dwNumDevices = scanResult.dwNumDevices
    if not dwNumDevices:
        print("No matching PCI device was found for search criteria "
            "(Vendor ID 0x%lX, Device ID 0x%lX)\n" % (dwVendorId, dwDeviceId))
        return None

    # Display matching devices information
    print("\nFound %ld matching device%s "
        "[ Vendor ID 0x%lX%s, Device ID 0x%lX%s ]:\n" %
        (dwNumDevices, "s" if dwNumDevices > 1 else "",
        dwVendorId.value, "" if dwVendorId else " (ALL)",
        dwDeviceId.value, "" if dwDeviceId else " (ALL)"))

    for i in range(dwNumDevices):
        print("%2ld. Vendor ID: 0x%lX, Device ID: 0x%lX" % (i + 1,
            scanResult.deviceId[i].dwVendorId,
            scanResult.deviceId[i].dwDeviceId))
        WDC_DIAG_PciDeviceInfoPrint(scanResult.deviceSlot[i], False)

    # Select device
    i = 1
    if dwNumDevices > 1:
        gs = "Select a device (1 - %ld): " % dwNumDevices
        (i, dwStatus) = DIAG_InputNum(gs, False, 0, 1, dwNumDevices)
        if DIAG_INPUT_SUCCESS != dwStatus:
            return None

    return scanResult.deviceSlot[i - 1]

# Open a handle to a PCI device
def WDC_DIAG_DeviceOpen(slot, gpDevCtx, pcKpName, PCI_DEV_ADDR_DESC):
    hDev = WDC_DEVICE_HANDLE()
    deviceInfo = WD_PCI_CARD_INFO()

    # Retrieve the device's resources information
    deviceInfo.pciSlot = slot
    dwStatus = wdapi.WDC_PciGetDeviceInfo(byref(deviceInfo))
    if WD_STATUS_SUCCESS != dwStatus:
        print("DeviceOpen: Failed retrieving the device's resources "
            "information. Error [0x%lx - %s]\n" % (dwStatus,
            Stat2Str(dwStatus)))
        return None

    # NOTE: If necessary, you can modify the device's resources information
    #   here - mainly the information stored in the deviceInfo.Card.Items array,
    #   and the number of array items stored in deviceInfo.Card.dwItems.
    #   For example:
    #   - Edit the deviceInfo.Card.Items array and/or deviceInfo.Card.dwItems,
    #    to register only some of the resources or to register only a portion
    #    of a specific address space.
    #   - Set the fNotSharable field of one or more items in the
    #    deviceInfo.Card.Items array to 1, to block sharing of the related
    #    resources and ensure that they are locked for exclusive use.

    # Open a handle to the device
    hDev = WDC_DIAG_PCI_DeviceOpen(deviceInfo, gpDevCtx, pcKpName, PCI_DEV_ADDR_DESC)
    if not hDev:
        print("DeviceOpen: Failed opening a handle to the device")
        return None

    return hDev

def WDC_DIAG_PCI_DeviceOpen(deviceInfo, gpDevCtx, pcKpName, PCI_DEV_ADDR_DESC):
    hDev = WDC_DEVICE_HANDLE()
    devAddrDesc = PCI_DEV_ADDR_DESC()

    # Validate arguments
    if not deviceInfo:
        print("PCI_DeviceOpen: Error - NULL device information "
            "struct pointer\n")
        return None

    # Open a device handle
    dwStatus = wdapi.WDC_PciDeviceOpen(byref(hDev), byref(deviceInfo),
        byref(gpDevCtx))

    if WD_STATUS_SUCCESS != dwStatus:
        print("Failed opening a WDC device handle. Error 0x%lx - %s\n" %
            (dwStatus, Stat2Str(dwStatus)))
        return PCI_DeviceOpen_err(hDev)
    pDev = cast(hDev, PWDC_DEVICE)

    if pcKpName:
        devAddrDesc.dwNumAddrSpaces = pDev.contents.dwNumAddrSpaces
        devAddrDesc.pAddrDesc = pDev.contents.pAddrDesc

        # Open a handle to a Kernel PlugIn driver
        wdapi.WDC_KernelPlugInOpen(hDev, pcKpName.encode('utf-8'),
            byref(devAddrDesc))

    # Return handle to the new device
    wdapi_va.WDC_Trace("PCI_DeviceOpen: Opened a PCI device (handle 0x%lx)\n"
        "Device is %susing a Kernel PlugIn driver (%s)\n" % (hDev.value,
        "" if WDC_IS_KP(pDev) else "not " , pcKpName))

    return hDev

def WDC_DIAG_DeviceClose(hDev):
    if not hDev:
        return True

    # Close the device handle
    dwStatus = wdapi.WDC_PciDeviceClose(hDev)
    if WD_STATUS_SUCCESS != dwStatus:
        print("Failed closing a WDC device handle (0x%lx). Error 0x%lx - %s\n",
            hDev, dwStatus, Stat2Str(dwStatus))

    return WD_STATUS_SUCCESS == dwStatus


def IsValidDevice(hDev, sFunc):
    if not hDev or not wdapi.WDC_GetDevContext(hDev):
        ErrLog ("%s: NULL device %s\n" % (sFunc, "handle" if not pDev else
            "context"))
        return False

    return True

def WDC_DIAG_GetNumAddrSpaces(hDev):
    pDev = cast(hDev, PWDC_DEVICE)
    # Validate the device handle
    if not IsValidDevice(hDev, "WDC_DIAG_GetNumAddrSpaces"):
        return 0

    # Return the number of address spaces for the device
    return pDev.contents.dwNumAddrSpaces

def WDC_DIAG_GetAddrSpaceInfo(hDev, addrSpaceInfo):
    pDev = cast(hDev, PWDC_DEVICE)

    if not IsValidDevice(hDev, "WDC_DIAG_GetAddrSpaceInfo"):
        return False

    dwAddrSpace = addrSpaceInfo.dwAddrSpace

    if dwAddrSpace > pDev.contents.dwNumAddrSpaces - 1:
        ErrLog("PCI_GetAddrSpaceInfo: Error - Address space %ld is "
            "out of range (0 - %ld)\n" % (dwAddrSpace,
            pDev.contents.dwNumAddrSpaces - 1))
        return False

    addrDesc = pDev.contents.pAddrDesc[dwAddrSpace]

    fIsMemory = WDC_ADDR_IS_MEM(addrDesc)

#ifndef ISA
    addrSpaceInfo.sName = "BAR %ld" % int(dwAddrSpace)
#else # ifdef ISA
#    addrSpaceInfo.sName = "AddrSpace %ld" % dwAddrSpace
#endif # ifdef ISA
    addrSpaceInfo.sType = "Memory" if fIsMemory else "I/O"

    if wdapi.WDC_AddrSpaceIsActive(pDev, dwAddrSpace):
        item = pDev.contents.cardReg.Card.Item[addrDesc.dwItemIndex]
        pAddr = item.I.Mem.pPhysicalAddr if fIsMemory \
            else item.I.IO.pAddr

        addrSpaceInfo.sDesc = (("0x%0*lX - 0x%0*lX (0x%lx bytes)") %
            (WDC_SIZE_64 * 2, pAddr, WDC_SIZE_64 * 2,
            pAddr + addrDesc.qwBytes - 1, addrDesc.qwBytes))
    else:
        addrSpaceInfo.sDesc = "Inactive address space"

    # TODO: You can modify the code above to set a different address space
    # name/description.

    return True


