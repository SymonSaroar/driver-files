''' Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com '''

from ctypes import *
from .windrvr import *
import sys, string, binascii, time

BYTES_IN_LINE      = 16
HEX_CHARS_PER_BYTE = 3
HEX_STOP_POS       = BYTES_IN_LINE * HEX_CHARS_PER_BYTE
DIAG_INPUT_CANCEL = -1
DIAG_INVALID_MENU = -1
DIAG_INPUT_FAIL = 0
DIAG_INPUT_SUCCESS = 1
DIAG_EXIT_MENU = 99
DIAG_CANCEL = 'x'

class DIAG_MENU_OPTION(object):
    def __init__(self, cOptionName = "", cTitleName = "", cbEntry = None,
            cbExit = None, pCbCtx = None, fIsHidden = False, cbIsHidden = None,
            pChildMenus = [], pParentMenu = None):
        self.cOptionName = cOptionName
        self.cTitleName = cTitleName
        self.cbEntry = cbEntry or None
        self.cbExit = cbExit or None
        self.pCbCtx = pCbCtx or None
        self.fIsHidden = False
        self.cbIsHidden = cbIsHidden
        self.pChildMenus = pChildMenus or []
        self.pParentMenu = pParentMenu or None

    def AddMenuOptionChild(self, pChild):
        self.pChildMenus.append(pChild)
        pChild.pParentMenu = self

#************************************************************
#  Functions implementation
# ***********************************************************

def DIAG_MenuSetHiddenChildren(pMenu):
    if (pMenu):
        for pChild in pMenu.pChildMenus:
            if (pChild.cbIsHidden is not None):
                pChild.fIsHidden = pChild.cbIsHidden(pChild)


def DIAG_GetNextMenuOption(pCurrentMenu):
    dwMaxOption = len(pCurrentMenu.pChildMenus)
    printf("Enter option: ")
    dwOption = inputf("")
    try:
        dwOption = int(dwOption)
    except:
        print("Invalid option\n")
        return DIAG_INVALID_MENU

    if (0 < dwOption <= dwMaxOption):

        if (pCurrentMenu.pChildMenus[dwOption - 1].fIsHidden):
            dwOption = DIAG_INVALID_MENU
        else:
            dwOption = dwOption - 1

    elif (dwOption != DIAG_EXIT_MENU):
        dwOption = DIAG_INVALID_MENU

    if (dwOption == DIAG_INVALID_MENU):
        print("Invalid option\n")

    return dwOption

def DIAG_MenuPrintTitleSeperator(cTitleName):
    print('-' * len(cTitleName))

def DIAG_MenuRun(pMenuRoot):
    pCurrentMenu = pMenuRoot
    dwOption, dwStatus = 0, 0

    while (pCurrentMenu):
        dwStatus = 0

        if (pCurrentMenu.cbEntry):
           dwStatus = pCurrentMenu.cbEntry(pCurrentMenu.pCbCtx)

        # If error occured, or we are in leaf node, go back to parent
        if (dwStatus or len(pCurrentMenu.pChildMenus) == 0):
            pCurrentMenu = pCurrentMenu.pParentMenu
            continue

        if (pCurrentMenu.cTitleName != ""):
            printf("\n%s\n", pCurrentMenu.cTitleName)
            DIAG_MenuPrintTitleSeperator(pCurrentMenu.cTitleName)

        DIAG_MenuSetHiddenChildren(pCurrentMenu)
        DIAG_MenuPrintOptions(pCurrentMenu)

        dwOption = DIAG_GetNextMenuOption(pCurrentMenu)
        if (dwOption == DIAG_EXIT_MENU):
            if (pCurrentMenu.cbExit):
                dwStatus = pCurrentMenu.cbExit(pCurrentMenu.pCbCtx)

            # Go back to parent
            pCurrentMenu = pCurrentMenu.pParentMenu

        elif (dwOption != DIAG_INVALID_MENU):
            # Go to chosen child
            pCurrentMenu = pCurrentMenu.pChildMenus[dwOption]

    return dwStatus

def DIAG_MenuPrintOptions(pMenu):
    if (pMenu is not None):
        for i, pChild in enumerate(pMenu.pChildMenus, 1):
            if (not pChild.fIsHidden):
                print("%ld. %s" % (i, pChild.cOptionName))

        printf("%d. Exit Menu\n", DIAG_EXIT_MENU)
        print("\n")

def DIAG_MenuSetCtxAndParentForMenus(pMenusArr, pCtx, pParentMenu):
    for pMenuOption in pMenusArr:
        pMenuOption.pCbCtx = pCtx
        pParentMenu.AddMenuOptionChild(pMenuOption)

def DIAG_PrintHexBuffer(pBuf, dwBytes):
    if not pBuf or not dwBytes:
        print("DIAG_PrintHexBuffer: Error - %s\n" %
            ("NULL buffer pointer" if not pBuf else "Empty buffer"))
        return
    hexStr = binascii.hexlify(pBuf).upper()
    curOffset = 0
    for lines in range(dwBytes.value // BYTES_IN_LINE):
        for offset in range(0, BYTES_IN_LINE * 2, 2):
            printf("%c%c " % (hexStr[curOffset], hexStr[curOffset+1]))
            curOffset += 2
        printf("\n")


def DIAG_GetHexChar():
    ch = sys.stdin.read(1)
    if not ch in string.hexdigits:
        return -1

    return int(ch, 16)

def DIAG_GetHexBuffer(numBytes):
    bytesRead = 0

    pData = bytearray(numBytes)
    while bytesRead < numBytes:
        ch = DIAG_GetHexChar()
        if ch < 0:
            continue
        res = ch << 4

        ch = DIAG_GetHexChar()
        if ch < 0:
            continue

        res += ch

        pData[bytesRead] = res
        bytesRead += 1

    # Advance to new line
    while ch != '\n' and ch != '\r':
        ch = sys.stdin.read(1)

    # Return the number of bytes that was read
    return pData, bytesRead

# Get menu option from user
def DIAG_GetMenuOption(dwMax):
    printf("Enter option: ")
    dwOption = inputf("")
    try:
        dwOption = int(dwOption)
    except:
        return (0, DIAG_CANCEL)

    if dwOption < 1:
        print("Invalid option\n")
        return (dwOption, DIAG_INPUT_FAIL)

    if DIAG_EXIT_MENU == dwOption:
        return (dwOption, DIAG_INPUT_SUCCESS)

    if not dwMax:
        return (dwOption, DIAG_INPUT_SUCCESS)

    if dwOption > dwMax:
        print("Invalid option: Option must be %s%ld, or %d to exit\n" %
            ("" if 1 == dwMax else "between 1 - ", dwMax, DIAG_EXIT_MENU))
        return (dwOption, DIAG_INPUT_FAIL)

    return (dwOption, DIAG_INPUT_SUCCESS)

class MaxVal:
    SI8  = 2 ** 7  - 1
    UI8  = 2 ** 8  - 1
    SI16 = 2 ** 15 - 1
    UI16 = 2 ** 16 - 1
    SI32 = 2 ** 31 - 1
    UI32 = 2 ** 32 - 1
    SI64 = 2 ** 63 - 1
    UI64 = 2 ** 64 - 1

def DIAG_InputNum(sInputText, fHex, dwSize, minInput, maxInput):
    fCheckRange = (maxInput > minInput)
    printf("%s (to cancel press '%c'): %s" %
        (("Enter input" if (not sInputText) or sInputText == "" else sInputText,
        DIAG_CANCEL, "0x" if fHex else "")))

    sInput = inputf()
    if len(sInput) < 1:
        return ("", DIAG_INPUT_FAIL)
    if DIAG_CANCEL == sInput[0].lower():
        return (sInput, DIAG_CANCEL)

    try:
        if fHex:
            pInput = int(sInput, 16)
        else:
            pInput = int(sInput)
    except:
        return (sInput, DIAG_CANCEL)

    if fCheckRange:
        if pInput < minInput or pInput > maxInput:
            printf("Invalid input: Input must be between "),
            if fHex:
                print("0x%X and 0x%X\n" % (minInput, maxInput))
            else:
                print("%d and %d\n"% (minInput, maxInput))
            return (sInput, DIAG_CANCEL)

    if sys.platform != "win32":
        if dwSize == sizeof(DWORD) and pInput < MaxVal.UI64:
            return (DWORD(pInput), DIAG_INPUT_SUCCESS)
    if dwSize == sizeof(BYTE) and pInput < MaxVal.UI8:
        return (BYTE(pInput), DIAG_INPUT_SUCCESS)
    elif dwSize == sizeof(WORD) and pInput < MaxVal.UI16:
        return (WORD(pInput), DIAG_INPUT_SUCCESS)
    elif dwSize == sizeof(UINT32) and pInput < MaxVal.UI32:
        return (UINT32(pInput), DIAG_INPUT_SUCCESS)
    elif dwSize == sizeof(UINT64) and pInput < MaxVal.UI64:
        return (UINT64(pInput), DIAG_INPUT_SUCCESS)

    return (pInput, DIAG_INPUT_SUCCESS)

# Function: get_cur_time()
#     Retrieves the current time, in OS units
#   Parameters:
#     time [out] pointer to the OS time, in OS units
#   Return Value:
#     True if successful, False otherwise
def get_cur_time():
    return time.time()

# Function: time_diff()
#     Calculates a time difference in milliseconds
#   Parameters:
#     end   [in] End time, in OS units
#     start [in] Start time, in OS units
#   Return Value:
#     The time difference, in milliseconds.
def time_diff(end, start):
    return 1000 * (end-start)

def DIAG_PrintPerformance(size, startTime):
    endTime = get_cur_time()
    perf_time_total = time_diff(endTime, startTime)

    if perf_time_total != -1:
        print("Transferred %ld bytes, elapsed time %ld[ms], "
            "rate %ld[MB/sec]\n" % (size, perf_time_total,
                # (bytes / msec) * sec / MB
                (size / (perf_time_total + 1)) * 1000 / (1024 * 1024)))



