/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/******************************************************************************
*  File: wdc_diag_lib.c - Implementation of shared WDC PCI and ISA  devices'  *
*  user-mode diagnostics API.                                                 *
*                                                                             *
*  Note: This code sample is provided AS-IS and as a guiding sample only.     *
*******************************************************************************/

#if !defined(__KERNEL__)

#include "status_strings.h"
#include "pci_strings.h"
#include "diag_lib.h"
#include "wdc_diag_lib.h"
#include "wdc_defs.h"

/*************************************************************
  General definitions
 *************************************************************/
/* Error messages display */
#define WDC_DIAG_ERR printf

/*************************************************************
  Global variables
 *************************************************************/
/* User's input command */
static CHAR gsInput[256];

/* -----------------------------------------------
    PCI/ISA
   ----------------------------------------------- */

/* Print device's resources information to file.
 * For a registered device (hCard != 0), print also kernel and user-mode
 * mappings of memory address items */
static void WDC_DIAG_DeviceResourcesPrint(const WD_CARD *pCard, DWORD hCard,
    FILE *fp)
{
    int resources;
    DWORD i;
    const WD_ITEMS *pItem;

    if (!pCard)
    {
        WDC_DIAG_ERR("WDC_DIAG_DeviceResourcesPrint: Error - NULL card "
            "pointer\n");
        return;
    }

    for (i = 0, resources = 0; i < pCard->dwItems; i++)
    {
        pItem = &pCard->Item[i];
        switch (pItem->item)
        {
        case ITEM_MEMORY:
            resources++;
            fprintf(fp, "    Memory range [BAR %d]: base 0x%" PRI64 "X, "
                "size 0x%" PRI64 "X\n", pItem->I.Mem.dwBar,
                pItem->I.Mem.pPhysicalAddr, pItem->I.Mem.qwBytes);
            if (hCard) /* Registered device */
            {
                fprintf(fp, "        Kernel-mode address mapping: 0x%" KPRI "X\n",
                    pItem->I.Mem.pTransAddr);
                fprintf(fp, "        User-mode address mapping: 0x%" UPRI "X\n",
                    pItem->I.Mem.pUserDirectAddr);
            }
            break;

        case ITEM_IO:
            resources++;
            fprintf(fp, "    I/O range [BAR %d]: base [0x%" KPRI "X], size "
                "[0x%x]\n", pItem->I.IO.dwBar, pItem->I.IO.pAddr,
                pItem->I.IO.dwBytes);
            break;

        case ITEM_INTERRUPT:
            resources++;
            fprintf(fp, "    Interrupt: IRQ %d\n", pItem->I.Int.dwInterrupt);
            fprintf(fp, "    Interrupt Options (supported interrupts):\n");
            if (WDC_INT_IS_MSI(pItem->I.Int.dwOptions))
            {
                fprintf(fp, "        %s\n",
                    WDC_DIAG_IntTypeDescriptionGet(pItem->I.Int.dwOptions));
            }
            /* According to the MSI specification, it is recommended that
             * a PCI device will support both MSI/MSI-X and level-sensitive
             * interrupts, and allow the operating system to choose which
             * type of interrupt to use. */
            if (pItem->I.Int.dwOptions & INTERRUPT_LEVEL_SENSITIVE)
            {
                fprintf(fp, "        %s\n",
                    WDC_DIAG_IntTypeDescriptionGet(INTERRUPT_LEVEL_SENSITIVE));
            }
            else if (!WDC_INT_IS_MSI(pItem->I.Int.dwOptions))
            /* MSI/MSI-X interrupts are always edge-triggered, so there is no
             * no need to display a specific edge-triggered indication for
             * such interrupts. */
            {
                fprintf(fp, "        %s\n",
                    WDC_DIAG_IntTypeDescriptionGet(INTERRUPT_LATCHED));
            }
            break;

        case ITEM_BUS:
            break;

        default:
            fprintf(fp, "    Invalid item type (0x%x)\n", pItem->item);
            break;
        }
    }

    if (!resources)
        fprintf(fp, "    Device has no resources\n");
}

void WDC_DIAG_PrintIndent(const char* str, DWORD indent, DWORD max_width,
    BOOL br_at_end)
{
    if (strcmp(str, "") && indent <= 80)
    {
        DWORD i, j;

        for (i = 0; i < strlen(str); i++)
        {
            if (str[i] == '\n')
            {
                printf("\n");
                for (j = 0; j < indent; j++)
                    printf(" ");
            }
            else
            {
                printf ("%c", str[i]);
            }
        }
        for (; i < max_width; i++)
            printf(" ");

        if (br_at_end)
            printf ("\n");
    }
}

/* Print run-time registers and PCI configuration registers information */
void WDC_DIAG_RegsInfoPrint(WDC_DEVICE_HANDLE hDev, const WDC_REG *pRegs,
    DWORD dwNumRegs, WDC_DIAG_REG_PRINT_OPTIONS options, BOOL isExpress)
{
    const WDC_REG *pReg;
    BOOL fName, fDesc, fAddrSpace, fOffset, fSize, fDir;

    DWORD i, dwStatus = WD_STATUS_SUCCESS;
    DWORD extended = isExpress ? PCIE_REGS_NUM : 0;
    DWORD pciExpressOffset = 0;
    WDC_PCI_HEADER_TYPE headerType;

    if (!hDev)
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: Error - NULL WDC device handle\n");
        return;
    }

    if (isExpress)
    {
        dwStatus = WDC_PciGetExpressOffset(hDev, &pciExpressOffset);
        if (dwStatus)
        {
            WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: Error getting PCI Expres"
                " Offset\n");
            return;
        }
    }

    dwStatus = WDC_PciGetHeaderType(hDev, &headerType);
    if (dwStatus)
    {
        WDC_DIAG_ERR("WDC_DIAG_RegsInfoPrint: Error - unable to determine"
            "PCI header type");
        return;
    }

    if (!dwNumRegs)
    {
        printf("There are currently no pre-defined registers to display\n");
        return;
    }

    if (!pRegs)
    {
        WDC_DIAG_ERR("WDC_DIAG_RegsInfoPrint: Error - NULL registers "
            "information pointer\n");
        return;
    }

    if (!options)
        options = WDC_DIAG_REG_PRINT_DEFAULT;

    fName = options & WDC_DIAG_REG_PRINT_NAME,
    fDesc = options & WDC_DIAG_REG_PRINT_DESC,
    fAddrSpace = options & WDC_DIAG_REG_PRINT_ADDR_SPACE,
    fOffset = options & WDC_DIAG_REG_PRINT_OFFSET,
    fSize = options & WDC_DIAG_REG_PRINT_SIZE;
    fDir = options & WDC_DIAG_REG_PRINT_DIRECTION;

    printf("\n");
    printf("PCI %sRegisters\n", isExpress ? "Express " : "");
    printf("----%s---------\n", isExpress ? "--------" : "");
    printf("%3s %-*s %-*s %-*s %-*s %-*s %s\n", "", MAX_NAME_DISPLAY,
        fName ? "Name" : "", 4, fAddrSpace ? "BAR" : "",
        (int)WDC_SIZE_32 * 2 + 2, fOffset ? "Offset" : "", 5,
        fSize ? "Size" : "", 4, fDir ? "R/W" : "", fDesc ? "Description" : "");

    printf("%3s %-*s %-*s %-*s %-*s %-*s %s\n",
        "", MAX_NAME_DISPLAY, fName ? "----" : "", 4, fAddrSpace ? "---" : "",
        (int)WDC_SIZE_32 * 2 + 2, fOffset ? "------" : "", 5,
        fSize ? "----" : "", 4, fDir ? "---" : "", fDesc ? "-----------" : "");

    for (i = 1, pReg = pRegs; i <= dwNumRegs; i++, pReg++)
    {
        printf("%2d. ", i + extended);

        if (fName)
            printf("%-*.*s ", MAX_NAME_DISPLAY, MAX_NAME_DISPLAY, pReg->sName);
        else
            printf("%*s ", MAX_NAME_DISPLAY, "");

        if (fAddrSpace && (WDC_AD_CFG_SPACE != pReg->dwAddrSpace))
            printf("%2d %*s", pReg->dwAddrSpace, 2, "");
        else
            printf("%4s ", "");

        if (fOffset)
            printf("0x%-*X ", (int)WDC_SIZE_32 * 2, pReg->dwOffset +
                pciExpressOffset);
        else
            printf("%*s ", (int)WDC_SIZE_32 * 2 + 2, "");

        if (fSize)
            printf("%*d %*s", 2, pReg->dwSize, 3, "");
        else
            printf("%*s ", 5, "");

        if (fDir)
        {
            printf("%-*s ", 4,
                (WDC_READ_WRITE == pReg->direction) ? "RW" :
                (WDC_READ == pReg->direction) ? "R" : "W");
        }
        else
        {
            printf("%*s ", 4, "");
        }
        if (fDesc && strcmp(pReg->sDesc, ""))
        {
            WDC_DIAG_PrintIndent(pReg->sDesc, REGS_INFO_PRINT_DETAILS_INDENT,
                REGS_INFO_PRINT_DETAILS_WIDTH, TRUE);
        }
        else
        {
            printf("\n");
        }
    }
}

/* Set address access mode */
BOOL WDC_DIAG_SetMode(WDC_ADDR_MODE *pMode)
{
    int option = 0;

    if (!pMode)
    {
        WDC_DIAG_ERR("WDC_DIAG_SetMode: Error - NULL mode pointer\n");
        return FALSE;
    }

    printf("\n");
    printf("Select read/write mode:\n");
    printf("-----------------------\n");
    printf("1. 8 bits (%d bytes)\n", WDC_SIZE_8);
    printf("2. 16 bits (%d bytes)\n", WDC_SIZE_16);
    printf("3. 32 bits (%d bytes)\n", WDC_SIZE_32);
    printf("4. 64 bits (%d bytes)\n", WDC_SIZE_64);
    printf("\n");

    printf("Enter option or 0 to cancel: ");
    fgets(gsInput, sizeof(gsInput), stdin);
    if (sscanf(gsInput, "%d", &option) < 1)
    {
        printf("Invalid input\n");
        return FALSE;
    }

    if (!option)
        return FALSE;

    switch (option)
    {
    case 1:
        *pMode = WDC_MODE_8;
        break;
    case 2:
        *pMode = WDC_MODE_16;
        break;
    case 3:
        *pMode = WDC_MODE_32;
        break;
    case 4:
        *pMode = WDC_MODE_64;
        break;
    default:
        printf("Invalid selection\n");
        return FALSE;
    }

    return TRUE;
}

/* Get data for address write operation from user */
/* Data size (dwSize) should be WDC_SIZE_8, WDC_SIZE_16, WDC_SIZE_32 or
 * WDC_SIZE_64 */
BOOL WDC_DIAG_InputWriteData(PVOID pData, WDC_ADDR_SIZE dwSize)
{
    UINT64 u64Data, u64MaxVal;

    if (!pData)
    {
        WDC_DIAG_ERR("WDC_DIAG_InputWriteData: Error - NULL data pointer\n");
        return FALSE;
    }

    u64MaxVal = (dwSize >= WDC_SIZE_64) ? ~((UINT64)0) :
        ((UINT64)1 << (dwSize * 8)) - 1;

    printf("Enter data to write (max value: 0x%" PRI64 "X) or '%c' to cancel: 0x",
        u64MaxVal, DIAG_CANCEL);
    fgets(gsInput, sizeof(gsInput), stdin);
    if (DIAG_CANCEL == tolower(gsInput[0]))
        return FALSE;

    BZERO(u64Data);
    if (sscanf(gsInput, "%" PRI64 "x", &u64Data) < 1)
    {
        printf("Invalid input\n");
        return FALSE;
    }

    if (u64Data > u64MaxVal)
    {
        printf("Error: Value is too big (max legal value is 0x%" PRI64 "X)\n",
            u64MaxVal);
        return FALSE;
    }

    switch (dwSize)
    {
    case WDC_SIZE_8:
        *((BYTE *)pData) = (BYTE)u64Data;
        return TRUE;
    case WDC_SIZE_16:
        *((WORD *)pData) = (WORD)u64Data;
        return TRUE;
    case WDC_SIZE_32:
        *((UINT32 *)pData) = (UINT32)u64Data;
        return TRUE;
    case WDC_SIZE_64:
        *((UINT64 *)pData) = (UINT64)u64Data;
        return TRUE;
    default:
        WDC_DIAG_ERR("WDC_DIAG_InputWriteData: Error - Invalid size "
            "(%d bytes)\n", dwSize);
    }

    return FALSE;
}

/* Read/write a memory or I/O address */
void WDC_DIAG_ReadWriteAddr(WDC_DEVICE_HANDLE hDev, WDC_DIRECTION direction,
    DWORD dwAddrSpace, WDC_ADDR_MODE mode)
{
    DIAG_INPUT_RESULT inputResult;
    DWORD dwStatus;
    DWORD dwOffset;
    BYTE bData = 0;
    WORD wData = 0;
    UINT32 u32Data = 0;
    UINT64 u64Data = 0;

    if (!hDev)
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteAddr: Error- NULL WDC device handle\n");
        return;
    }

    inputResult = DIAG_InputDWORD(&dwOffset, (WDC_READ == direction) ?
        "Enter offset to read from" : "Enter offset to write to", TRUE, 0, 0);
    if (DIAG_INPUT_SUCCESS != inputResult)
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteAddr: %s getting the offset\n",
            inputResult == DIAG_INPUT_CANCEL ? "Canceled" : "Failed");
        return;
    }

    if ((WDC_WRITE == direction) &&
        !WDC_DIAG_InputWriteData((WDC_MODE_8 == mode) ? (PVOID)&bData :
        (WDC_MODE_16 == mode) ? (PVOID)&wData :
        (WDC_MODE_32 == mode) ? (PVOID)&u32Data : (PVOID)&u64Data,
        WDC_ADDR_MODE_TO_SIZE(mode)))
    {
            return;
    }

    switch (mode)
    {
    case WDC_MODE_8:
        dwStatus = (WDC_READ == direction) ?
            WDC_ReadAddr8(hDev, dwAddrSpace, dwOffset, &bData) :
            WDC_WriteAddr8(hDev, dwAddrSpace, dwOffset, bData);
        if (WD_STATUS_SUCCESS == dwStatus)
        {
            printf("%s 0x%X %s offset 0x%x in BAR %d\n",
                (WDC_READ == direction) ? "Read" : "Wrote", (UINT32)bData,
                (WDC_READ == direction) ? "from" : "to", dwOffset, dwAddrSpace);
        }
        break;

    case WDC_MODE_16:
        dwStatus = (WDC_READ == direction) ?
            WDC_ReadAddr16(hDev, dwAddrSpace, dwOffset, &wData) :
            WDC_WriteAddr16(hDev, dwAddrSpace, dwOffset, wData);
        if (WD_STATUS_SUCCESS == dwStatus)
        {
            printf("%s 0x%hX %s offset 0x%x in BAR %d\n",
                (WDC_READ == direction) ? "Read" : "Wrote", wData,
                (WDC_READ == direction) ? "from" : "to", dwOffset, dwAddrSpace);
        }
        break;

    case WDC_MODE_32:
        dwStatus = (WDC_READ == direction) ?
            WDC_ReadAddr32(hDev, dwAddrSpace, dwOffset, &u32Data) :
            WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
        if (WD_STATUS_SUCCESS == dwStatus)
        {
            printf("%s 0x%X %s offset 0x%x in BAR %d\n",
                (WDC_READ == direction) ? "Read" : "Wrote", u32Data,
                (WDC_READ == direction) ? "from" : "to", dwOffset, dwAddrSpace);
        }
        break;

    case WDC_MODE_64:
        dwStatus = (WDC_READ == direction) ?
            WDC_ReadAddr64(hDev, dwAddrSpace, dwOffset, &u64Data) :
            WDC_WriteAddr64(hDev, dwAddrSpace, dwOffset, u64Data);
        if (WD_STATUS_SUCCESS == dwStatus)
        {
            printf("%s 0x%" PRI64 "X %s offset 0x%x in BAR %d\n",
                (WDC_READ == direction) ? "Read" : "Wrote", u64Data,
                (WDC_READ == direction) ? "from" : "to", dwOffset, dwAddrSpace);
        }
        break;

    default:
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteAddr: Error - Invalid mode (%d)\n",
            mode);
        return;
    }

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        printf("Failed to %s offset 0x%x in BAR %d. Error 0x%x - %s\n",
            (WDC_READ == direction) ? "read from" : "write to", dwOffset,
            dwAddrSpace, dwStatus, Stat2Str(dwStatus));
    }
}

/* Read/write a memory or I/O address OR an offset in the PCI configuration
 * space (dwAddrSpace == WDC_AD_CFG_SPACE) */
void WDC_DIAG_ReadWriteBlock(WDC_DEVICE_HANDLE hDev, WDC_DIRECTION direction,
    DWORD dwAddrSpace)
{
    DWORD dwStatus;
    DIAG_INPUT_RESULT inputResult;
    DWORD dwOffset, dwBytes;
    const CHAR *sDir = (WDC_READ == direction) ? "read" : "write";
    PVOID pBuf = NULL;

    if (!hDev)
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteBlock: Error- NULL WDC device "
            "handle\n");
        return;
    }

    inputResult = DIAG_InputDWORD(&dwOffset, "offset", TRUE, 0, 0);
    if (DIAG_INPUT_SUCCESS != inputResult)
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteBlock: %s getting the offset\n",
            inputResult == DIAG_INPUT_CANCEL ? "Canceled" : "Failed");
        return;
    }

    inputResult = DIAG_InputDWORD(&dwBytes, "bytes", TRUE, 0, 0);
    if (DIAG_INPUT_SUCCESS != inputResult)
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteBlock: %s getting the number of bytes "
            "to transfer\n", inputResult == DIAG_INPUT_CANCEL ? "Canceled" :
            "Failed");
        return;
    }

    if (!dwBytes)
        return;

    pBuf = malloc(dwBytes);
    if (!pBuf)
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteBlock: Failed allocating [%s] data "
            "buffer\n", sDir);
        goto Exit;
    }
    memset(pBuf, 0, dwBytes);

    if (WDC_WRITE == direction)
    {
        printf("data to write (hex format): 0x");
        if (!DIAG_GetHexBuffer(pBuf, dwBytes))
            goto Exit;
    }

    if (WDC_AD_CFG_SPACE == dwAddrSpace) /* Read/write a configuration/attribute
                                          * space address */
    {
        WD_BUS_TYPE busType = WDC_GetBusType(hDev);

        if (WD_BUS_PCI == busType) /* Read/write PCI configuration space
                                    * offset */
        {
            if (direction == WDC_READ)
                dwStatus = WDC_PciReadCfg(hDev, dwOffset, pBuf, dwBytes);
            else
                dwStatus = WDC_PciWriteCfg(hDev, dwOffset, pBuf, dwBytes);
        }
        else
        {
            printf("Error - Cannot read/write configuration space address "
                "space for bus type [0x%x]\n", busType);
            goto Exit;
        }
    }
    else /* Read/write a memory or I/O address */
    {
        WDC_ADDR_MODE mode;
        WDC_ADDR_RW_OPTIONS options;
        BOOL fAutoInc;

        if (!WDC_DIAG_SetMode(&mode))
            goto Exit;

        printf("Do you wish to increment the address after each %s block "
            "(0x%x bytes) (0 - No, Otherwise - Yes)? ", sDir,
            WDC_ADDR_MODE_TO_SIZE(mode));
        fgets(gsInput, sizeof(gsInput), stdin);
        if (sscanf(gsInput, "%u", &fAutoInc) < 1)
        {
            printf("Invalid input\n");
            goto Exit;
        }

        options = (fAutoInc ? WDC_ADDR_RW_DEFAULT : WDC_ADDR_RW_NO_AUTOINC);

        dwStatus = direction == WDC_READ ?
            WDC_ReadAddrBlock(hDev, dwAddrSpace, dwOffset, dwBytes, pBuf, mode,
                options) :
            WDC_WriteAddrBlock(hDev, dwAddrSpace, dwOffset, dwBytes, pBuf, mode,
                options);
    }

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        if (WDC_READ == direction)
            DIAG_PrintHexBuffer(pBuf, dwBytes, FALSE);
        else
            printf("Wrote 0x%x bytes to offset 0x%x\n", dwBytes, dwOffset);
    }
    else
    {
        printf("Failed to %s 0x%x bytes %s offset 0x%x. Error 0x%x - %s\n",
            sDir, dwBytes, (WDC_READ == direction) ? "from" : "to", dwOffset,
            dwStatus, Stat2Str(dwStatus));
    }

Exit:
    if (pBuf)
        free(pBuf);

    printf("\n");
    printf("Press ENTER to continue\n");
    fgets(gsInput, sizeof(gsInput), stdin);
}

/* Read all pre-defined run-time or PCI configuration registers and display
 * results */
void WDC_DIAG_ReadRegsAll(WDC_DEVICE_HANDLE hDev, const WDC_REG *pRegs,
    DWORD dwNumRegs, BOOL fPciCfg, BOOL fExpress)
{
    DWORD i, dwStatus, outBufLen;
    const WDC_REG *pReg;
    BYTE bData = 0;
    WORD wData = 0;
    UINT32 u32Data = 0;
    UINT64 u64Data = 0;
    DWORD pciExpressOffset = 0;
    WDC_PCI_HEADER_TYPE headerType;
    CHAR details[1024];

    if (!hDev)
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: Error - NULL WDC device handle\n");
        return;
    }

    if (fExpress)
    {
        dwStatus = WDC_PciGetExpressOffset(hDev, &pciExpressOffset);
        if (dwStatus)
        {
            WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: Error getting PCI Expres"
                " Offset\n");
            return;
        }
    }

    dwStatus = WDC_PciGetHeaderType(hDev, &headerType);
    if (dwStatus)
    {
        WDC_DIAG_ERR("WDC_DIAG_RegsInfoPrint: Error - unable to determine"
            "PCI header type");
        return;
    }

    if (!dwNumRegs || !pRegs)
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: %s\n", !dwNumRegs ?
            "No registers (dwNumRegs == 0)" : "Error - NULL registers pointer");

        return;
    }

    printf("\n");
    printf("%s registers data:\n", (fPciCfg && fExpress) ?
        "PCI Express configuration" : fPciCfg ? "PCI configuration" :
        "run-time");
    printf("---------------------------------\n\n");
    printf("%3s %-*s %-*s  %s           %s\n", "", MAX_NAME_DISPLAY, "Name",
        4 * 2 + 2, "Data", "Description", "Details");
    printf("%3s %-*s %-*s  %s           %s\n", "", MAX_NAME_DISPLAY, "----",
        4 * 2 + 2, "----", "-----------", "-------");

    for (i = 1, pReg = pRegs; i <= dwNumRegs; i++, pReg++)
    {
        printf("%2d. %-*.*s ", i, MAX_NAME_DISPLAY, MAX_NAME_DISPLAY,
            pReg->sName);

        if (WDC_WRITE == pReg->direction)
        {
            printf("Write-only register\n");
            continue;
        }

        switch (pReg->dwSize)
        {
        case WDC_SIZE_8:
            dwStatus = fPciCfg ?
                WDC_PciReadCfg8(hDev, pReg->dwOffset + pciExpressOffset,
                    &bData) :
                WDC_ReadAddr8(hDev, pReg->dwAddrSpace, pReg->dwOffset +
                    pciExpressOffset, &bData);
            if (WD_STATUS_SUCCESS == dwStatus)
                printf("0x%-*X  ", (int)WDC_SIZE_64 , (UINT32)bData);
            break;

        case WDC_SIZE_16:
            dwStatus = fPciCfg ?
                WDC_PciReadCfg16(hDev, pReg->dwOffset + pciExpressOffset,
                    &wData) :
                WDC_ReadAddr16(hDev, pReg->dwAddrSpace, pReg->dwOffset +
                    pciExpressOffset, &wData);
            if (WD_STATUS_SUCCESS == dwStatus)
                printf("0x%-*hX  ", (int)WDC_SIZE_64 , wData);
            break;

        case WDC_SIZE_32:
            dwStatus = fPciCfg ?
                WDC_PciReadCfg32(hDev, pReg->dwOffset + pciExpressOffset,
                    &u32Data) :
                WDC_ReadAddr32(hDev, pReg->dwAddrSpace, pReg->dwOffset +
                    pciExpressOffset, &u32Data);
            if (WD_STATUS_SUCCESS == dwStatus)
                printf("0x%-*X  ", (int)WDC_SIZE_64, u32Data);
            break;

        case WDC_SIZE_64:
            dwStatus = fPciCfg ?
                WDC_PciReadCfg64(hDev, pReg->dwOffset + pciExpressOffset,
                    &u64Data) :
                WDC_ReadAddr64(hDev, pReg->dwAddrSpace, pReg->dwOffset +
                    pciExpressOffset, &u64Data);
            if (WD_STATUS_SUCCESS == dwStatus)
                printf("0x%-*" PRI64 "X  ", (int)WDC_SIZE_64, u64Data);
            break;

        default:
            printf("Invalid register size (%d)\n", pReg->dwSize);
            return;
        }

        if (WD_STATUS_SUCCESS != dwStatus)
            printf("Error: 0x%-*x  ", (int)WDC_SIZE_64 * 2 - 7, dwStatus);
        WDC_DIAG_PrintIndent(pReg->sDesc, READ_REGS_ALL_DESC_INDENT,
            READ_REGS_ALL_DESC_WIDTH, FALSE);

        if (fPciCfg)
        {
            dwStatus = fExpress ?
                PciExpressConfRegData2Str(hDev, pReg->dwOffset, details,
                    sizeof(details), &outBufLen) :
                PciConfRegData2Str(hDev, pReg->dwOffset, details,
                    sizeof(details), &outBufLen);
            if (!strcmp(details, ""))
            {
                printf("\n");
            }
            else
            {
                WDC_DIAG_PrintIndent(details, READ_REGS_ALL_DETAILS_INDENT,
                    READ_REGS_ALL_DETAILS_WIDTH, TRUE);
            }
        }
        else
        {
            printf("\n");
        }
        outBufLen = 0;
    }

    printf("\n");
    printf("Press ENTER to continue\n");
    fgets(gsInput, sizeof(gsInput), stdin);
}

/* Display a list of pre-defined run-time or PCI configuration registers
   and let user select to read/write from/to a specific register */
void WDC_DIAG_ReadWriteReg(WDC_DEVICE_HANDLE hDev, const WDC_REG *pRegs,
    DWORD dwNumRegs, WDC_DIRECTION direction, BOOL fPciCfg, BOOL fExpressReg)
{
    DWORD dwStatus;
    DWORD dwReg, outBufLen = 0;
    const WDC_REG *pReg;
    BYTE bData = 0;
    WORD wData = 0;
    UINT32 u32Data = 0;
    UINT64 u64Data = 0;
    BOOL fExpress;
    DWORD pciExpressOffset = 0;
    WDC_PCI_HEADER_TYPE headerType;
    CHAR details[1024];

    if (!hDev)
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: Error - NULL WDC device handle\n");
        return;
    }

    fExpress = fExpressReg && WDC_PciGetExpressGen(hDev) != 0;
    if (fExpress)
    {
        dwStatus = WDC_PciGetExpressOffset(hDev, &pciExpressOffset);
        if (dwStatus)
        {
            WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: Error getting PCI Express"
                " Offset\n");
            return;
        }
    }

    dwStatus = WDC_PciGetHeaderType(hDev, &headerType);
    if (dwStatus)
    {
        WDC_DIAG_ERR("WDC_DIAG_RegsInfoPrint: Error - unable to determine"
            "PCI header type");
        return;
    }

    if (!dwNumRegs || !pRegs)
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteReg: %s\n",
            !dwNumRegs ? "No registers to read/write (dwNumRegs == 0)" :
            "Error - NULL registers pointer");
        return;
    }

    /* Display pre-defined registers' information */
    printf("\n");
    printf("PCI %s registers:\n", fPciCfg ? "configuration" : "run-time");
    printf("----------------------------\n");
    WDC_DIAG_RegsInfoPrint(hDev, pRegs, dwNumRegs, WDC_DIAG_REG_PRINT_ALL,
        FALSE);

    /* Read/write register */
    printf("\n");
    printf("Select a register from the list above to %s or 0 to cancel: ",
        (WDC_READ == direction) ? "read from" : "write to");
    fgets(gsInput, sizeof(gsInput), stdin);
    if (sscanf(gsInput, "%ld", (long int *)&dwReg) < 1)
    {
        printf("Invalid selection\n");
        goto Exit;
    }

    if (!dwReg)
        return;

    if (dwReg > dwNumRegs)
    {
        printf("Selection (%d) is out of range (1 - %d)\n",
            dwReg, dwNumRegs);
        goto Exit;
    }

    pReg = &pRegs[dwReg - 1];

    if ( ((WDC_READ == direction) && (WDC_WRITE == pReg->direction)) ||
        ((WDC_WRITE == direction) && (WDC_READ == pReg->direction)))
    {
        printf("Error - you have selected to %s a %s-only register\n",
            (WDC_READ == direction) ? "read from" : "write to",
            (WDC_WRITE == pReg->direction) ? "write" : "read");
        goto Exit;
    }

    if ((WDC_WRITE == direction) &&
        !WDC_DIAG_InputWriteData((WDC_SIZE_8 == pReg->dwSize) ? (PVOID)&bData :
        (WDC_SIZE_16 == pReg->dwSize) ? (PVOID)&wData :
        (WDC_SIZE_32 == pReg->dwSize) ? (PVOID)&u32Data : (PVOID)&u64Data,
        pReg->dwSize))
    {
        goto Exit;
    }

    switch (pReg->dwSize)
    {
    case WDC_SIZE_8:
        if (WDC_READ == direction)
        {
            dwStatus = fPciCfg ?
                WDC_PciReadCfg8(hDev, pReg->dwOffset + pciExpressOffset,
                    &bData) :
                WDC_ReadAddr8(hDev, pReg->dwAddrSpace, pReg->dwOffset +
                    pciExpressOffset, &bData);
        }
        else
        {
            dwStatus = fPciCfg ? WDC_PciWriteCfg8(hDev, pReg->dwOffset +
                pciExpressOffset, bData) :
                WDC_WriteAddr8(hDev, pReg->dwAddrSpace, pReg->dwOffset +
                pciExpressOffset, bData);
        }
        if (WD_STATUS_SUCCESS == dwStatus)
        {
            printf("%s 0x%X %s register %s at offset [0x%x]\n",
                (WDC_READ == direction) ? "Read" : "Wrote", (UINT32)bData,
                (WDC_READ == direction) ? "from" : "to", pReg->sName,
                fExpress ? pReg->dwOffset + pciExpressOffset : pReg->dwOffset);
            dwStatus = fExpress ?
                PciExpressConfRegData2Str(hDev, pReg->dwOffset, details,
                    sizeof(details), &outBufLen) :
                PciConfRegData2Str(hDev, pReg->dwOffset, details,
                    sizeof(details), &outBufLen);
            printf ("%s\n", WDC_READ == direction && strcmp(details, "") ?
                "Decoded register data: " : "");
            printf ("%s\n", WDC_READ == direction && strcmp(details, "") ?
                details : "");
        }
        break;

    case WDC_SIZE_16:
        if (WDC_READ == direction)
        {
            dwStatus = fPciCfg ?
                WDC_PciReadCfg16(hDev, pReg->dwOffset + pciExpressOffset,
                    &wData) :
                WDC_ReadAddr16(hDev, pReg->dwAddrSpace, pReg->dwOffset +
                    pciExpressOffset, &wData);
        }
        else
        {
            dwStatus = fPciCfg ?
                WDC_PciWriteCfg16(hDev, pReg->dwOffset + pciExpressOffset,
                    wData) :
                WDC_WriteAddr16(hDev, pReg->dwAddrSpace, pReg->dwOffset +
                    pciExpressOffset, wData);
        }
        if (WD_STATUS_SUCCESS == dwStatus)
        {
            printf("%s [0x%hX] %s register %s at offset [0x%x]\n",
                (WDC_READ == direction) ? "Read" : "Wrote", wData,
                (WDC_READ == direction) ? "from" : "to", pReg->sName,
                fExpress ? pReg->dwOffset + pciExpressOffset : pReg->dwOffset);
            dwStatus = fExpress ?
                PciExpressConfRegData2Str(hDev, pReg->dwOffset, details,
                    sizeof(details), &outBufLen) :
                PciConfRegData2Str(hDev, pReg->dwOffset, details,
                    sizeof(details), &outBufLen);
            printf ("%s\n", WDC_READ == direction && strcmp(details, "") ?
                "Decoded register data: " : "");
            printf ("%s\n", WDC_READ == direction && strcmp(details, "") ?
                details : "");
        }
        break;

    case WDC_SIZE_32:
        if (WDC_READ == direction)
        {
            dwStatus = fPciCfg ?
                WDC_PciReadCfg32(hDev, pReg->dwOffset + pciExpressOffset,
                    &u32Data) :
                WDC_ReadAddr32(hDev, pReg->dwAddrSpace, pReg->dwOffset +
                    pciExpressOffset, &u32Data);
        }
        else
        {
            dwStatus = fPciCfg ?
                WDC_PciWriteCfg32(hDev, pReg->dwOffset + pciExpressOffset,
                    u32Data) :
                WDC_WriteAddr32(hDev, pReg->dwAddrSpace, pReg->dwOffset +
                    pciExpressOffset, u32Data);
        }
        if (WD_STATUS_SUCCESS == dwStatus)
        {
            printf("%s [0x%X] %s register %s at offset [0x%x]\n",
                (WDC_READ == direction) ? "Read" : "Wrote", u32Data,
                (WDC_READ == direction) ? "from" : "to", pReg->sName,
                fExpress ? pReg->dwOffset + pciExpressOffset : pReg->dwOffset);
            dwStatus = fExpress ?
                PciExpressConfRegData2Str(hDev, pReg->dwOffset, details,
                    sizeof(details), &outBufLen) :
                PciConfRegData2Str(hDev, pReg->dwOffset, details,
                    sizeof(details), &outBufLen);
            printf ("%s\n", WDC_READ == direction && strcmp(details, "") ?
                "Decoded register data: " : "");
            printf ("%s\n", WDC_READ == direction && strcmp(details, "") ?
                details : "");
        }
        break;

    case WDC_SIZE_64:
        if (WDC_READ == direction)
        {
            dwStatus = fPciCfg ?
                WDC_PciReadCfg64(hDev, pReg->dwOffset + pciExpressOffset,
                    &u64Data) :
                WDC_ReadAddr64(hDev, pReg->dwAddrSpace, pReg->dwOffset +
                    pciExpressOffset, &u64Data);
        }
        else
        {
            dwStatus = fPciCfg ?
                WDC_PciWriteCfg64(hDev, pReg->dwOffset + pciExpressOffset,
                    u64Data) :
                WDC_WriteAddr64(hDev, pReg->dwAddrSpace, pReg->dwOffset +
                    pciExpressOffset, u64Data);
        }
        if (WD_STATUS_SUCCESS == dwStatus)
        {
            printf("%s [0x%" PRI64 "X] %s register %s at offset [0x%x]\n",
                (WDC_READ == direction) ?  "Read" : "Wrote", u64Data,
                (WDC_READ == direction) ? "from" : "to", pReg->sName,
                fExpress ? pReg->dwOffset + pciExpressOffset : pReg->dwOffset);
            dwStatus = fExpress ?
                PciExpressConfRegData2Str(hDev, pReg->dwOffset +
                    pciExpressOffset, details, sizeof(details), &outBufLen) :
                PciConfRegData2Str(hDev, pReg->dwOffset, details,
                    sizeof(details), &outBufLen);
            printf ("%s\n", WDC_READ == direction && strcmp(details, "") ?
                "Decoded register data: " : "");
            printf ("%s\n", WDC_READ == direction && strcmp(details, "") ?
                details : "");
        }
        break;

    default:
        printf("Invalid register size (%d)\n", pReg->dwSize);
        return;
    }

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        printf("Failed %s %s. Error [0x%x - %s]\n", (WDC_READ == direction) ?
            "reading data from" : "writing data to", pReg->sName, dwStatus,
            Stat2Str(dwStatus));
    }

Exit:
    printf("\n");
    printf("Press ENTER to continue\n");
    fgets(gsInput, sizeof(gsInput), stdin);
}

/* Display available PCI/PCIe capabilities */
void WDC_DIAG_ScanPCICapabilities(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;
    WDC_PCI_SCAN_CAPS_RESULT scanResult;
    DWORD dwCapId = WD_PCI_CAP_ID_ALL;
    int option = 0;
    DWORD i;

    if (!hDev)
    {
        WDC_DIAG_ERR("WDC_DIAG_ScanPCICapabilities: Error - NULL WDC device "
            "handle\n");
        return;
    }

    printf("\n");
    printf("Select scan option (PCI/PCI-Express, all/specific):\n");
    printf("-----------------------\n");
    printf("1. Scan PCI capabilities\n");
    printf("2. Scan specific PCI capability\n");
    printf("3. Scan PCI Express extended capabilities\n");
    printf("4. Scan specific PCI Express extended capability\n");
    printf("\n");

    printf("Enter option or 0 to cancel: ");
    fgets(gsInput, sizeof(gsInput), stdin);
    if (sscanf(gsInput, "%d", &option) < 1)
    {
        printf("Invalid input\n");
        return;
    }

    if (!option)
        return;

    if (option > 4)
    {
        printf("Invalid selection\n");
        return;
    }

    if (option == 2 || option == 4)
    {
        printf("Enter requested %scapability ID (hexadecimal): 0x",
            option == 4 ? "extended " : "");
        fgets(gsInput, sizeof(gsInput), stdin);
        if (sscanf(gsInput, "%x", &dwCapId) < 1)
        {
            printf("Invalid input\n");
            return;
        }
    }

    /* Scan PCI/PCIe Capabilities */
    BZERO(scanResult);

    if (option <= 2)
        dwStatus = WDC_PciScanCaps(hDev, dwCapId, &scanResult);
    else
        dwStatus = WDC_PciScanExtCaps(hDev, dwCapId, &scanResult);

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDC_DIAG_ERR("WDC_DIAG_ScanPCICapabilities: Failed scanning PCI "
            "capabilities. Error [0x%x - %s]\n", dwStatus, Stat2Str(dwStatus));
        return;
    }

    printf("%sPCI %scapabilities found\n", scanResult.dwNumCaps ?  "" : "No ",
        (option == 3 || option == 4) ? "Express extended " : "");
    for (i = 0; i < scanResult.dwNumCaps; i++)
    {
        printf("    %d) %s - ID [0x%x], offset [0x%x]\n", i + 1,
            (option == 3 || option == 4) ?
            GET_EXTENDED_CAPABILITY_STR(scanResult.pciCaps[i].dwCapId) :
            GET_CAPABILITY_STR(scanResult.pciCaps[i].dwCapId),
            scanResult.pciCaps[i].dwCapId, scanResult.pciCaps[i].dwCapOffset);
    }
}

char *WDC_DIAG_IntTypeDescriptionGet(DWORD dwIntType)
{
    if (dwIntType & INTERRUPT_MESSAGE_X)
        return "Extended Message-Signaled Interrupt (MSI-X)";
    else if (dwIntType & INTERRUPT_MESSAGE)
        return "Message-Signaled Interrupt (MSI)";
    else if (dwIntType & INTERRUPT_LEVEL_SENSITIVE)
        return "Level-Sensitive Interrupt";
    return "Edge-Triggered Interrupt";
}

/* -----------------------------------------------
    PCI
   ----------------------------------------------- */
/* Print PCI device location information */
void WDC_DIAG_PciSlotPrint(WD_PCI_SLOT *pPciSlot)
{
    WDC_DIAG_PciSlotPrintFile(pPciSlot, stdout);
}

/* Print PCI device location information to file */
void WDC_DIAG_PciSlotPrintFile(WD_PCI_SLOT *pPciSlot, FILE *fp)
{
    if (!pPciSlot)
    {
        WDC_DIAG_ERR("WDC_DIAG_PciSlotPrint: Error - NULL PCI slot pointer\n");
        return;
    }

    fprintf(fp, "Domain [0x%x], Bus [0x%x], Slot [0x%x], Function [0x%x]\n",
        pPciSlot->dwDomain, pPciSlot->dwBus, pPciSlot->dwSlot,
        pPciSlot->dwFunction);
}

/* Print PCI device location and resources information */
void WDC_DIAG_PciDeviceInfoPrint(WD_PCI_SLOT *pPciSlot, BOOL dump_cfg)
{
    WDC_DIAG_PciDeviceInfoPrintFile(pPciSlot, stdout, dump_cfg);
}

/* Print PCI device location and resources information to file */
void WDC_DIAG_PciDeviceInfoPrintFile(WD_PCI_SLOT *pPciSlot, FILE *fp,
    BOOL dump_cfg)
{
    DWORD dwStatus, dwExpressGen;
    WD_PCI_CARD_INFO deviceInfo;

    if (!pPciSlot)
    {
        WDC_DIAG_ERR("WDC_DIAG_PciDeviceInfoPrint: Error - NULL PCI slot "
            "pointer\n");
        return;
    }

    fprintf(fp, "    Location: ");
    WDC_DIAG_PciSlotPrintFile(pPciSlot, fp);

    if (dump_cfg)
    {
        UINT32 config;
        DWORD dwOffset = 0;

        for (dwOffset = 0; dwOffset < 256; dwOffset += sizeof(UINT32))
        {
            dwStatus = WDC_PciReadCfgBySlot32(pPciSlot, dwOffset, &config);
            if (dwStatus)
            {
                WDC_DIAG_ERR("    Failed reading PCI configuration space.\n"
                    "    Error [0x%x - %s]\n", dwStatus, Stat2Str(dwStatus));
                return;
            }

            if ((dwOffset / 4) % 8 == 0)
                fprintf(fp, "%02x ", dwOffset);
            fprintf(fp, "%08x ", config);
            if ((dwOffset / 4) % 8 == 7)
                fprintf(fp, "\n");
        }
    }

    BZERO(deviceInfo);
    deviceInfo.pciSlot = *pPciSlot;
    dwStatus = WDC_PciGetDeviceInfo(&deviceInfo);
    if ((WD_NO_RESOURCES_ON_DEVICE != dwStatus) &&
        (WD_STATUS_SUCCESS != dwStatus))
    {
        WDC_DIAG_ERR("    Failed retrieving PCI resources information.\n"
            "    Error 0x%x - %s\n", dwStatus, Stat2Str(dwStatus));
        return;
    }

    WDC_DIAG_DeviceResourcesPrint(&deviceInfo.Card, 0, fp);

    dwExpressGen = WDC_PciGetExpressGenBySlot(pPciSlot);
    if (dwExpressGen)
        fprintf(fp, "    PCI Express Generation: Gen%d\n", dwExpressGen);
}

/* Print location and resources information for all connected PCI devices */
void WDC_DIAG_PciDevicesInfoPrintAll(BOOL dump_cfg)
{
    WDC_DIAG_PciDevicesInfoPrintAllFile(stdout, dump_cfg);
}

/* Print location and resources information for all connected PCI devices to
 * file */
void WDC_DIAG_PciDevicesInfoPrintAllFile(FILE *fp, BOOL dump_cfg)
{
    DWORD dwStatus;
    DWORD i, dwNumDevices;
    WDC_PCI_SCAN_RESULT scanResult;

    BZERO(scanResult);
    dwStatus = WDC_PciScanDevices(0, 0, &scanResult);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        fprintf(fp, "Failed scanning PCI bus. Error [0x%x - %s]\n", dwStatus,
            Stat2Str(dwStatus));
        return;
    }

    dwNumDevices = scanResult.dwNumDevices;
    if (!dwNumDevices)
    {
        fprintf(fp, "No devices were found on the PCI bus\n");
        return;
    }

    fprintf(fp, "\n");
    fprintf(fp, "Found %d devices on the PCI bus:\n", dwNumDevices);
    fprintf(fp, "---------------------------------\n");

    for (i = 0; i < dwNumDevices; i++)
    {
        fprintf(fp, "%2d. Vendor ID: [0x%x], Device ID: [0x%x]\n", i + 1,
            scanResult.deviceId[i].dwVendorId,
            scanResult.deviceId[i].dwDeviceId);
        WDC_DIAG_PciDeviceInfoPrintFile(&scanResult.deviceSlot[i], fp,
            dump_cfg);

        if (fp == stdout)
        {
            printf("Press ENTER to proceed to next device");
            fgets(gsInput, sizeof(gsInput), stdin);
        }
        fprintf(fp, "\n");
    }
}

#ifndef ISA
/* Find and open a PCI device */
WDC_DEVICE_HANDLE WDC_DIAG_DeviceFindAndOpen(DWORD dwVendorId,
    DWORD dwDeviceId, PCHAR pcKpName, const DWORD dwDevCtxSize)
{
    WD_PCI_SLOT slot;

    /* Find device */
    if (!WDC_DIAG_DeviceFind(dwVendorId, dwDeviceId, &slot))
        return NULL;

    /* Open a device handle */
    return WDC_DIAG_DeviceOpen(&slot, pcKpName, dwDevCtxSize);
}

/* Find a PCI device */
BOOL WDC_DIAG_DeviceFind(DWORD dwVendorId, DWORD dwDeviceId,
    WD_PCI_SLOT *pSlot)
{
    DWORD dwStatus;
    DWORD i, dwNumDevices;
    WDC_PCI_SCAN_RESULT scanResult;

    if (!dwVendorId)
    {
        /* Get vendor ID */
        if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&dwVendorId,
            "Enter vendor ID", TRUE, 0, 0))
        {
            return FALSE;
        }

        /* Get device ID */
        if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&dwDeviceId,
            "Enter device ID", TRUE, 0, 0))
        {
            return FALSE;
        }
    }

    /* Scan PCI devices */
    BZERO(scanResult);
    dwStatus = WDC_PciScanDevices(dwVendorId, dwDeviceId, &scanResult);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDC_DIAG_ERR("DeviceFind: Failed scanning the PCI bus.\n"
            "Error [0x%x - %s]\n", dwStatus, Stat2Str(dwStatus));
        return FALSE;
    }

    dwNumDevices = scanResult.dwNumDevices;
    if (!dwNumDevices)
    {
        printf("No matching PCI device was found for search criteria "
            "(Vendor ID 0x%x, Device ID 0x%x)\n", dwVendorId, dwDeviceId);

        return FALSE;
    }

    /* Display matching devices information */
    printf("\nFound %d matching device%s "
        "[ Vendor ID 0x%x%s, Device ID 0x%x%s ]:\n",
        dwNumDevices, dwNumDevices > 1 ? "s" : "",
        dwVendorId, dwVendorId ? "" : " (ALL)",
        dwDeviceId, dwDeviceId ? "" : " (ALL)");

    for (i = 0; i < dwNumDevices; i++)
    {
        printf("\n");
        printf("%2d. Vendor ID: 0x%x, Device ID: 0x%x\n", i + 1,
            scanResult.deviceId[i].dwVendorId,
            scanResult.deviceId[i].dwDeviceId);

        WDC_DIAG_PciDeviceInfoPrint(&scanResult.deviceSlot[i], FALSE);
    }
    printf("\n");

    /* Select device */
    if (dwNumDevices > 1)
    {
        sprintf(gsInput, "Select a device (1 - %d): ", dwNumDevices);
        i = 0;
        if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&i, gsInput, FALSE, 1,
            dwNumDevices))
        {
            return FALSE;
        }
    }

    *pSlot = scanResult.deviceSlot[i - 1];

    return TRUE;
}

/* Open a handle to a PCI device */
WDC_DEVICE_HANDLE WDC_DIAG_DeviceOpen(const WD_PCI_SLOT *pSlot, PCHAR pcKpName,
    const DWORD dwDevCtxSize)
{
    WDC_DEVICE_HANDLE hDev;
    DWORD dwStatus;
    WD_PCI_CARD_INFO deviceInfo;

    /* Retrieve the device's resources information */
    BZERO(deviceInfo);
    deviceInfo.pciSlot = *pSlot;
    dwStatus = WDC_PciGetDeviceInfo(&deviceInfo);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDC_DIAG_ERR("DeviceOpen: Failed retrieving the device's resources "
            "information. Error [0x%x - %s]\n", dwStatus, Stat2Str(dwStatus));
        return NULL;
    }

    /* NOTE: If necessary, you can modify the device's resources information
       here - mainly the information stored in the deviceInfo.Card.Items array,
       and the number of array items stored in deviceInfo.Card.dwItems.
       For example:
       - Edit the deviceInfo.Card.Items array and/or deviceInfo.Card.dwItems,
         to register only some of the resources or to register only a portion
         of a specific address space.
       - Set the fNotSharable field of one or more items in the
         deviceInfo.Card.Items array to 1, to block sharing of the related
         resources and ensure that they are locked for exclusive use.
    */

    /* Open a handle to the device */
    hDev = WDC_DIAG_PCI_DeviceOpen(&deviceInfo, pcKpName, dwDevCtxSize);

    if (!hDev)
        return NULL;

    return hDev;
}

WDC_DEVICE_HANDLE WDC_DIAG_PCI_DeviceOpen(const WD_PCI_CARD_INFO *pDeviceInfo,
    PCHAR pcKpName, const DWORD dwDevCtxSize)
{
    DWORD dwStatus;
    PVOID pDevCtx = NULL;
    WDC_DEVICE_HANDLE hDev = NULL;
    DEV_ADDR_DESC devAddrDesc = { 0 };
    PWDC_DEVICE pDev;

    /* Validate arguments */
    if (!pDeviceInfo)
    {
        WDC_DIAG_ERR("PCI_DeviceOpen: Error - NULL device information "
            "struct pointer\n");
        return NULL;
    }

    /* Allocate memory for the PCI device context */
    pDevCtx = malloc(dwDevCtxSize);
    if (!pDevCtx)
    {
        WDC_DIAG_ERR("Failed allocating memory for PCI device context\n");
        return NULL;
    }

    memset(pDevCtx, 0, dwDevCtxSize);

    /* Open a device handle */
    dwStatus = WDC_PciDeviceOpen(&hDev, pDeviceInfo, pDevCtx);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDC_DIAG_ERR("Failed opening a WDC device handle. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    if (pcKpName)
    {
        pDev = (PWDC_DEVICE)hDev;
        devAddrDesc.dwNumAddrSpaces = pDev->dwNumAddrSpaces;
        devAddrDesc.pAddrDesc = pDev->pAddrDesc;

        /* Open a handle to a Kernel PlugIn driver */
        WDC_KernelPlugInOpen(hDev, pcKpName, &devAddrDesc);
    }

    /* Return handle to the new device */
    WDC_Trace("PCI_DeviceOpen: Opened a PCI device (handle 0x%p)\n"
        "Device is %s using a Kernel PlugIn driver (%s)\n", hDev,
        (WDC_IS_KP(hDev)) ? "" : "not", pcKpName ? pcKpName : "");
    return hDev;

Error:
    if (hDev)
        WDC_DIAG_DeviceClose(hDev);
    else
        free(pDevCtx);

    return NULL;
}

#endif
/* Close handle to a PCI device */
BOOL WDC_DIAG_DeviceClose(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;
    PVOID pDevCtx;

    /* Validate the WDC device handle */
    if (!hDev)
        return FALSE;

    pDevCtx = WDC_GetDevContext(hDev);

    /* Close the device handle */
    dwStatus = WDC_PciDeviceClose(hDev);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDC_DIAG_ERR("Failed closing a WDC device handle (0x%p). Error 0x%x "
            "- %s\n", hDev, dwStatus, Stat2Str(dwStatus));
    }

    /* Free PCI device context memory */
    if (pDevCtx)
        free(pDevCtx);

    return (WD_STATUS_SUCCESS == dwStatus);
}

BOOL WDC_DIAG_IsValidDevice(PWDC_DEVICE pDev, const CHAR *sFunc)
{
    if (!pDev || !(pDev->pCtx))
    {
        WDC_DIAG_ERR("%s: NULL device %s\n", sFunc,
            !pDev ? "handle" : "context");
        return FALSE;
    }

    return TRUE;
}

#ifndef ISA
DWORD WDC_DIAG_GetNumAddrSpaces(WDC_DEVICE_HANDLE hDev)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    /* Validate the device handle */
    if (!WDC_DIAG_IsValidDevice(pDev, __FUNCTION__))
        return 0;

    /* Return the number of address spaces for the device */
    return pDev->dwNumAddrSpaces;
}
#endif /* ifndef ISA*/

BOOL WDC_DIAG_GetAddrSpaceInfo(WDC_DEVICE_HANDLE hDev,
    ADDR_SPACE_INFO *pAddrSpaceInfo)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    WDC_ADDR_DESC *pAddrDesc;
    DWORD dwAddrSpace;
    BOOL fIsMemory;

    if (!WDC_DIAG_IsValidDevice(pDev, __FUNCTION__))
        return FALSE;

    dwAddrSpace = pAddrSpaceInfo->dwAddrSpace;

    if (dwAddrSpace > pDev->dwNumAddrSpaces - 1)
    {
        WDC_DIAG_ERR("WDC_DIAG_GetAddrSpaceInfo: Error - Address space %d is "
            "out of range (0 - %d)\n", dwAddrSpace,
            pDev->dwNumAddrSpaces - 1);
        return FALSE;
    }

    pAddrDesc = &pDev->pAddrDesc[dwAddrSpace];

    fIsMemory = WDC_ADDR_IS_MEM(pAddrDesc);

#ifndef ISA
    snprintf(pAddrSpaceInfo->sName, MAX_NAME - 1, "BAR %d", dwAddrSpace);
#else /* ifdef ISA */
    snprintf(pAddrSpaceInfo->sName, MAX_NAME - 1, "AddrSpace %d", dwAddrSpace);
#endif /* ifdef ISA */
    snprintf(pAddrSpaceInfo->sType, MAX_TYPE - 1, fIsMemory ? "Memory" : "I/O");

    if (WDC_AddrSpaceIsActive(pDev, dwAddrSpace))
    {
        WD_ITEMS *pItem = &pDev->cardReg.Card.Item[pAddrDesc->dwItemIndex];
        PHYS_ADDR pAddr = fIsMemory ? pItem->I.Mem.pPhysicalAddr :
            pItem->I.IO.pAddr;

        snprintf(pAddrSpaceInfo->sDesc, MAX_DESC - 1,
            "0x%0*" PRI64 "X - 0x%0*" PRI64 "X (0x%" PRI64 "x bytes)",
            (int)WDC_SIZE_64 * 2, pAddr,
            (int)WDC_SIZE_64 * 2, pAddr + pAddrDesc->qwBytes - 1,
            pAddrDesc->qwBytes);
    }
    else
    {
        snprintf(pAddrSpaceInfo->sDesc, MAX_DESC - 1, "Inactive address space");
    }

    /* TODO: You can modify the code above to set a different address space
     * name/description. */

    return TRUE;
}

#endif /* !defined(__KERNEL__) */

