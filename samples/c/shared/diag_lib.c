/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/******************************************************************************
*  File: diag_lib.c - Implementation of shared WD user-mode diagnostics API.
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*******************************************************************************/

#if !defined(__KERNEL__)

#include <stdio.h>
#include <time.h>
#include "diag_lib.h"

/*************************************************************
  General definitions
 *************************************************************/
/* Error messages display */
#define DIAG_ERR printf

/* Messages display */
#define DIAG_TRACE printf

/*************************************************************
  Functions implementation
 *************************************************************/
static void DIAG_AddMenuOptionChild(
    DIAG_MENU_OPTION *pParentMenu, DIAG_MENU_OPTION *pChildMenu)
{
    if (pParentMenu && pChildMenu)
    {
        if (pParentMenu->dwCurrentNumberOfChildren < MENU_MAX_CHILDREN)
        {
            pParentMenu->pChildMenus[pParentMenu->dwCurrentNumberOfChildren] =
                pChildMenu;
            pParentMenu->dwCurrentNumberOfChildren++;
            pChildMenu->pParentMenu = pParentMenu;
        }
        else
        {
            DIAG_ERR("DIAG_AddMenuOptionChild: Error! children menu array is "
                "full.");
        }
    }
}

void DIAG_MenuSetCtxAndParentForMenus(DIAG_MENU_OPTION *pMenusArr,
    DWORD dwSize, PVOID pCtx, DIAG_MENU_OPTION *pParentMenu)
{
    DWORD i;

    for (i = 0; i < dwSize; i++)
    {
        pMenusArr[i].pCbCtx = pCtx;
        DIAG_AddMenuOptionChild(pParentMenu, &pMenusArr[i]);
    }
}

static void DIAG_MenuSetHiddenChildren(DIAG_MENU_OPTION *pMenu)
{
    DWORD i;
    DIAG_MENU_OPTION **ppChildren;

    if (pMenu)
    {
        ppChildren = pMenu->pChildMenus;

        for (i = 0; i < MENU_MAX_CHILDREN && ppChildren[i]; i++)
        {
            if (ppChildren[i]->cbIsHidden)
            {
                ppChildren[i]->fIsHidden =
                    ppChildren[i]->cbIsHidden(ppChildren[i]);
            }
        }
    }
}

static DWORD DIAG_GetNextMenuOption(DIAG_MENU_OPTION *pCurrentMenu)
{
    static CHAR sInput[MAX_NAME];
    DWORD dwStatus, dwOption, dwMaxOption;

    dwMaxOption = pCurrentMenu->dwCurrentNumberOfChildren;
    dwOption = 0;

    DIAG_TRACE("Enter option: ");

    fgets(sInput, sizeof(sInput), stdin);

    dwStatus = sscanf(sInput, "%u", &dwOption);
    if (dwStatus < 1 || dwOption == 0)
        goto Error;

    if (0 < dwOption && dwOption <= dwMaxOption)
    {
        if (pCurrentMenu->pChildMenus[dwOption - 1]->fIsHidden)
            goto Error;

        dwOption = dwOption - 1;
    }
    else if (dwOption != DIAG_EXIT_MENU)
    {
        goto Error;
    }

    goto Exit;

Error:
    DIAG_TRACE("Invalid option\n");
    dwOption = DIAG_INVALID_MENU;

Exit:
    return dwOption;
}

static void DIAG_MenuPrintTitleSeperator(PCHAR pcTitleName)
{
    DWORD i = 0;

    while (pcTitleName[i++])
        DIAG_TRACE("-");

    DIAG_TRACE("\n");
}

static void DIAG_MenuPrintOptions(DIAG_MENU_OPTION *pMenu)
{
    DWORD i;
    DIAG_MENU_OPTION **ppChildren;

    if (pMenu)
    {
        ppChildren = pMenu->pChildMenus;

        for (i = 0; i < MENU_MAX_CHILDREN && ppChildren[i]; i++)
        {
            if (!ppChildren[i]->fIsHidden)
                DIAG_TRACE("%d. %s\n", (i + 1), ppChildren[i]->cOptionName);
        }

        DIAG_TRACE("%d. Exit Menu\n", DIAG_EXIT_MENU);
        DIAG_TRACE("\n");
    }
}

DWORD DIAG_MenuRun(DIAG_MENU_OPTION *pMenuRoot)
{
    DIAG_MENU_OPTION *pCurrentMenu = pMenuRoot;
    DWORD dwOption, dwStatus = 0;

    while (pCurrentMenu)
    {
        dwStatus = 0;

        if (pCurrentMenu->cbEntry)
           dwStatus = pCurrentMenu->cbEntry(pCurrentMenu->pCbCtx);

        /* If error occured, or we are in leaf node, go back to parent */
        if (dwStatus || pCurrentMenu->dwCurrentNumberOfChildren == 0)
        {
            pCurrentMenu = pCurrentMenu->pParentMenu;
            continue;
        }

        if (*pCurrentMenu->cTitleName)
        {
            DIAG_TRACE("\n%s\n", pCurrentMenu->cTitleName);
            DIAG_MenuPrintTitleSeperator(pCurrentMenu->cTitleName);
        }


        DIAG_MenuSetHiddenChildren(pCurrentMenu);
        DIAG_MenuPrintOptions(pCurrentMenu);

        dwOption = DIAG_GetNextMenuOption(pCurrentMenu);
        if (dwOption == DIAG_EXIT_MENU)
        {
            if (pCurrentMenu->cbExit)
                dwStatus = pCurrentMenu->cbExit(pCurrentMenu->pCbCtx);

            /* Go back to parent */
            pCurrentMenu = pCurrentMenu->pParentMenu;
        }
        else if (dwOption != DIAG_INVALID_MENU)
        {
            /* Go to chosen child */
            pCurrentMenu = pCurrentMenu->pChildMenus[dwOption];
        }
    }

    return dwStatus;
}

#define BYTES_IN_LINE 16
#define HEX_CHARS_PER_BYTE 3
#define HEX_STOP_POS BYTES_IN_LINE * HEX_CHARS_PER_BYTE
void DIAG_PrintHexBuffer(PVOID pBuf, DWORD dwBytes, BOOL fAscii)
{
    PBYTE pbData = (PBYTE)pBuf;
    CHAR pHex[HEX_STOP_POS + 1] = { 0 };
    CHAR pAscii[BYTES_IN_LINE + 1] = { 0 };
    DWORD dwOffset, dwLineOffset, i;

    if (!pBuf || !dwBytes)
    {
        DIAG_ERR("DIAG_PrintHexBuffer: Error - %s\n",
            !pBuf ? "NULL buffer pointer" : "Empty buffer");
        return;
    }

    for (dwOffset = 0; dwOffset < dwBytes; dwOffset++)
    {
        dwLineOffset = dwOffset % BYTES_IN_LINE;
        if (dwOffset && !dwLineOffset)
        {
            pAscii[dwLineOffset] = '\0';
            printf("%s%s%s\n", pHex, fAscii ? " | " : "", fAscii ? pAscii : "");
        }
        sprintf(pHex + dwLineOffset * HEX_CHARS_PER_BYTE, "%02X ",
            (UINT32)pbData[dwOffset]);
        if (fAscii)
        {
            pAscii[dwLineOffset] =
                (CHAR)((pbData[dwOffset] >= 0x20) ? pbData[dwOffset] : '.');
        }
    }

    /* Print the last line. Fill with blanks if needed */
    if (dwOffset % BYTES_IN_LINE)
    {
        for (i = (dwOffset % BYTES_IN_LINE) * HEX_CHARS_PER_BYTE;
            i < BYTES_IN_LINE * HEX_CHARS_PER_BYTE;
            i++)
        {
            pHex[i] = ' ';
        }
        pHex[i] = '\0';
    }
    if (fAscii)
        pAscii[dwOffset % BYTES_IN_LINE] = '\0';
    printf("%s%s%s\n", pHex, fAscii ? " | " : "", fAscii ? pAscii : "");
}

int DIAG_GetHexChar(void)
{
    int ch;

    ch = getchar();

    if (!isxdigit(ch))
        return -1;

    if (isdigit(ch))
        return ch - '0';
    else
        return toupper(ch) - 'A' + 10;
}

DWORD DIAG_GetHexBuffer(PVOID pBuffer, DWORD dwBytes)
{
    DWORD dwBytesRead = 0;
    PBYTE pData = (PBYTE)pBuffer;
    int res;
    int ch;

    while (dwBytesRead < dwBytes)
    {
        ch = DIAG_GetHexChar();
        if (ch < 0)
            continue;

        res = ch << 4;

        ch = DIAG_GetHexChar();
        if (ch < 0)
            continue;

        res += ch;
        pData[dwBytesRead] = (BYTE)res;
        dwBytesRead++;
    }

    /* Advance to new line */
    do {
        ch = getchar();
    } while (ch != '\n' && ch != '\r');

    /* Return the number of bytes that was read */
    return dwBytesRead;
}

void DIAG_FillHexBuffer(PVOID pBuffer, DWORD dwBytes, BYTE ch, BOOL fRandNum)
{
    if (fRandNum)
    {
        srand((unsigned int)time(NULL));
        ch = (rand() % 255);
    }

    memset(pBuffer, ch, dwBytes);
}

/* Get menu option from user */
DIAG_INPUT_RESULT DIAG_GetMenuOption(PDWORD pdwOption, DWORD dwMax)
{
    static CHAR sInput[256];
    int iRet;

    if (!pdwOption)
    {
        DIAG_ERR("DIAG_GetMenuOption: Error - NULL option pointer\n");
        return DIAG_INPUT_FAIL;
    }

    printf("Enter option: ");

    fgets(sInput, sizeof(sInput), stdin);

    iRet = sscanf(sInput, "%u", pdwOption);
    if (iRet < 1)
    {
        printf("Invalid option\n");
        return DIAG_INPUT_FAIL;
    }

    if (DIAG_EXIT_MENU == *pdwOption)
        return DIAG_INPUT_SUCCESS;

    if (!dwMax)
        return DIAG_INPUT_SUCCESS;

    if (*pdwOption > dwMax)
    {
        printf("Invalid option: Option must be %s%d, or %d to exit\n",
            (1 == dwMax) ? "" : "between 1 - ", dwMax, DIAG_EXIT_MENU);
        return DIAG_INPUT_FAIL;
    }

    return DIAG_INPUT_SUCCESS;
}

DIAG_INPUT_RESULT DIAG_InputNum(PVOID pInput, const CHAR *sInputText, BOOL fHex,
    DWORD dwSize, UINT64 min, UINT64 max)
{
    static CHAR sInput[256];
    const CHAR *sFormat;
    int iRet;
    BOOL fCheckRange = (max > min);

    if (!pInput)
    {
        DIAG_ERR("DIAG_InputData: Error - NULL input pointer\n");
        return DIAG_INPUT_FAIL;
    }

    printf("%s (to cancel press '%c'): %s",
        (!sInputText || !strcmp(sInputText, "")) ?  "Enter input" : sInputText,
        DIAG_CANCEL, fHex ? "0x" : "");

    fgets(sInput, sizeof(sInput), stdin);
    if (DIAG_CANCEL == tolower(sInput[0]))
        return DIAG_INPUT_CANCEL;

    switch (dwSize)
    {
    case sizeof(WORD):
        sFormat = fHex ? "%hX" : "%hu";
        break;
    case sizeof(UINT32):
        sFormat = fHex ? "%X" : "%u";
        break;
    case sizeof(UINT64):
        sFormat = fHex ? "%" PRI64 "X" : "%" PRI64 "d";
        break;
    default:
        DIAG_ERR("DIAG_InputNum: Error - Invalid size (%d)\n", dwSize);
        return DIAG_INPUT_FAIL;
    }

    iRet = sscanf(sInput, sFormat, pInput);
    if (iRet < 1)
    {
        printf("Invalid input\n");
        return DIAG_INPUT_FAIL;
    }

    if (fCheckRange)
    {
        UINT64 tmp = sizeof(WORD) == dwSize ? *(WORD *)pInput :
            sizeof(UINT32) == dwSize ? *(UINT32 *)pInput : *(UINT64 *)pInput;

        if (tmp < min || tmp > max)
        {
            printf("Invalid input: Input must be between ");
            if (fHex)
                printf("0x%" PRI64 "X and 0x%" PRI64 "X\n", min, max);
            else
                printf("%" PRI64 "d and %" PRI64 "d\n", min, max);
            return DIAG_INPUT_FAIL;
        }
    }

    return DIAG_INPUT_SUCCESS;
}

/* Function: time_diff()
Calculates a time difference in milliseconds
Parameters:
end   [in] End time, in OS units
start [in] Start time, in OS units
Return Value:
The time difference, in milliseconds. */
double time_diff(TIME_TYPE *end, TIME_TYPE *start)
{
#if defined(UNIX)
    return (double)(end->tv_usec - start->tv_usec) / 1000 +
        (end->tv_sec - start->tv_sec) * 1000;
#else
    TIME_TYPE ctr_freq;

    if (!QueryPerformanceFrequency(&ctr_freq))
    {
        DIAG_ERR("Error reading timer frequency\n");
        return -1;
    }

    return (double)((end->QuadPart - start->QuadPart) * 1000 /
        ctr_freq.QuadPart);
#endif
}

/* Function: get_cur_time()
Retrieves the current time, in OS units
Parameters:
time [out] pointer to the OS time, in OS units
Return Value:
TRUE if successful, FALSE otherwise */
BOOL get_cur_time(TIME_TYPE *time)
{
#if defined(UNIX)
    return !gettimeofday(time, NULL);
#else
    return QueryPerformanceCounter(time);
#endif
}

void DIAG_PrintPerformance(UINT64 qwBytes, TIME_TYPE *startTime)
{
    TIME_TYPE endTime;
    double perf_time_total;

    get_cur_time(&endTime);
    perf_time_total = time_diff(&endTime, startTime);

    if (perf_time_total != -1)
    {
        printf("Transferred %" PRI64 "d bytes, elapsed time %.2lf[ms], "
            "rate %.2lf [MB/sec]\n", qwBytes, perf_time_total,
            ((qwBytes / (perf_time_total + 1)) * 1000) / (1024 * 1024));
            /* ((bytes / msec) * sec) / MB */
    }
}

#endif /* !defined(__KERNEL__) */

