/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

//////////////////////////////////////////////////////////////////////
// File - cmos_lib.c
//
// Library for accessing the CMOS on the motherboard directly,
// Code was generated by WinDriver DriverWizard.
// Application uses WinDriver to access hardware.
//
// Note: This code sample is provided AS-IS and as a guiding sample only.
//////////////////////////////////////////////////////////////////////

#include "cmos_lib.h"

// this string is set to an error message, if one occurs
CHAR CMOS_ErrorString[1024];

// internal function used by CMOS_Open()
void CMOS_SetCardElements(CMOS_HANDLE hCMOS);

BOOL CMOS_Open(CMOS_HANDLE *phCMOS)
{
    CMOS_HANDLE hCMOS = (CMOS_HANDLE)malloc(sizeof(CMOS_STRUCT));
    WD_VERSION ver;
    DWORD dwStatus;

    *phCMOS = NULL;
    CMOS_ErrorString[0] = '\0';
    BZERO(*hCMOS);

    hCMOS->hWD = WD_Open();
    if (hCMOS->hWD==INVALID_HANDLE_VALUE)
    {
        sprintf( CMOS_ErrorString, "Failed opening " WD_PROD_NAME " device\n");
        goto Exit;
    }

    BZERO(ver);
    WD_Version(hCMOS->hWD, &ver);
    if (ver.dwVer < WD_VER)
    {
        sprintf( CMOS_ErrorString, "Incorrect " WD_PROD_NAME " version\n");
        goto Exit;
    }

    CMOS_SetCardElements(hCMOS);
    hCMOS->cardReg.fCheckLockOnly = FALSE;
    dwStatus = WD_CardRegister(hCMOS->hWD, &hCMOS->cardReg);
    if (hCMOS->cardReg.hCard == 0)
    {
        sprintf(CMOS_ErrorString, "Failed locking device. Status 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
        goto Exit;
    }

    // Open finished OK
    *phCMOS = hCMOS;
    return TRUE;

Exit:
    // Error during Open
    if (hCMOS->cardReg.hCard)
        WD_CardUnregister(hCMOS->hWD, &hCMOS->cardReg);
    if (hCMOS->hWD != INVALID_HANDLE_VALUE)
        WD_Close(hCMOS->hWD);
    free(hCMOS);
    return FALSE;
}

void CMOS_Close(CMOS_HANDLE hCMOS)
{
    // unregister card
    if (hCMOS->cardReg.hCard)
        WD_CardUnregister(hCMOS->hWD, &hCMOS->cardReg);

    // close WinDriver
    WD_Close(hCMOS->hWD);

    free(hCMOS);
}

void CMOS_SetCardElements(CMOS_HANDLE hCMOS)
{
    WD_ITEMS *pItem;

    hCMOS->cardReg.Card.dwItems = CMOS_ITEMS;
    pItem = &hCMOS->cardReg.Card.Item[0];

    // CMOS I/O range
    pItem[CMOS_IO].item = ITEM_IO;
    pItem[CMOS_IO].fNotSharable = FALSE;
    pItem[CMOS_IO].I.IO.pAddr = CMOS_IO_ADDR;
    pItem[CMOS_IO].I.IO.dwBytes = CMOS_IO_BYTES;
}

void CMOS_WriteAddress(CMOS_HANDLE hCMOS, BYTE addr)
{
    WD_TRANSFER trans;

    BZERO(trans);
    trans.cmdTrans = WP_BYTE;
    trans.pPort = CMOS_IO_ADDR + CMOS_Address_OFFSET;
    trans.Data.Byte = addr;
    WD_Transfer(hCMOS->hWD, &trans);
}

BYTE CMOS_ReadData(CMOS_HANDLE hCMOS)
{
    WD_TRANSFER trans;

    BZERO(trans);
    trans.cmdTrans = RP_BYTE;
    trans.pPort = CMOS_IO_ADDR + CMOS_Data_OFFSET;
    WD_Transfer(hCMOS->hWD, &trans);
    return trans.Data.Byte;
}

void CMOS_WriteData(CMOS_HANDLE hCMOS, BYTE data)
{
    WD_TRANSFER trans;

    BZERO(trans);
    trans.cmdTrans = WP_BYTE;
    trans.pPort = CMOS_IO_ADDR + CMOS_Data_OFFSET;
    trans.Data.Byte = data;
    WD_Transfer(hCMOS->hWD, &trans);
}

BYTE CMOS_Read(CMOS_HANDLE hCMOS, BYTE addr)
{
    CMOS_WriteAddress(hCMOS, addr);
    return CMOS_ReadData(hCMOS);
}

void CMOS_Write(CMOS_HANDLE hCMOS, BYTE addr, BYTE data)
{
    CMOS_WriteAddress(hCMOS, addr);
    CMOS_WriteData(hCMOS, data);
}

