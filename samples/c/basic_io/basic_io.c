/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

////////////////////////////////////////////////////////////////
// File - BASIC_IO.C
//
// This is a skeleton driver for a simple ISA card with I/O
// ports access.
//
// Note: This code sample is provided AS-IS and as a guiding sample only.
////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "windrvr.h"
#include "status_strings.h"

/* Put your I/O range here
 * In this example the range is 0x378-0x37a */
enum {
    MY_IO_BASE = 0x378,
    MY_IO_SIZE = 0x3
};

/* Global WinDriver handle and card handle */
static HANDLE hWD;
static WD_CARD_REGISTER cardReg;

BYTE IO_inp(DWORD dwIOAddr)
{
    WD_TRANSFER trns;

    BZERO(trns);
    trns.cmdTrans = RP_BYTE; // R-Read P-Port BYTE
    trns.pPort = dwIOAddr;
    WD_Transfer(hWD, &trns); // Perform read

    return trns.Data.Byte;
}

WORD IO_inpw(DWORD dwIOAddr)
{
    WD_TRANSFER trns;

    BZERO(trns);
    trns.cmdTrans = RP_WORD; // R-Read P-Port WORD
    trns.pPort = dwIOAddr;
    WD_Transfer(hWD, &trns); // Perform read

    return trns.Data.Word;
}

DWORD IO_inpd(DWORD dwIOAddr)
{
    WD_TRANSFER trns;

    BZERO(trns);
    trns.cmdTrans = RP_DWORD; // R-Read P-Port DWORD
    trns.pPort = dwIOAddr;
    WD_Transfer(hWD, &trns); // Perform read

    return trns.Data.Dword;
}

void IO_outp(DWORD dwIOAddr, BYTE bData)
{
    WD_TRANSFER trns;

    BZERO(trns);
    trns.cmdTrans = WP_BYTE; // R-Write P-Port BYTE
    trns.pPort = dwIOAddr;
    trns.Data.Byte = bData;
    WD_Transfer(hWD, &trns); // Perform write
}

void IO_outpw(DWORD dwIOAddr, WORD wData)
{
    WD_TRANSFER trns;

    BZERO(trns);
    trns.cmdTrans = WP_WORD; // R-Write P-Port WORD
    trns.pPort = dwIOAddr;
    trns.Data.Word = wData;
    WD_Transfer(hWD, &trns); // Perform write
}

void IO_outpd(DWORD dwIOAddr, DWORD dwData)
{
    WD_TRANSFER trns;

    BZERO(trns);
    trns.cmdTrans = WP_DWORD; // W-Write P-Port DWORD
    trns.pPort = dwIOAddr;
    trns.Data.Dword = dwData;
    WD_Transfer(hWD, &trns); // Perform write
}

static BOOL IO_init(void)
{
    WD_VERSION verBuf;
    DWORD dwStatus;

    hWD = WD_Open();
    if (hWD == INVALID_HANDLE_VALUE)
    {
        printf("error opening WINDRVR\n");
        return FALSE;
    }

    BZERO(verBuf);
    WD_Version(hWD, &verBuf);
    printf(WD_PROD_NAME " version - %s\n", verBuf.cVer);
    if (verBuf.dwVer < WD_VER)
    {
        printf("Error incorrect WINDRVR version. Needs ver %d\n", WD_VER);
        WD_Close(hWD);
        return FALSE;
    }

    BZERO(cardReg);
    cardReg.Card.dwItems = 1;
    cardReg.Card.Item[0].item = ITEM_IO;
    cardReg.Card.Item[0].fNotSharable = TRUE;
    cardReg.Card.Item[0].I.IO.pAddr = MY_IO_BASE;
    cardReg.Card.Item[0].I.IO.dwBytes = MY_IO_SIZE;
    cardReg.fCheckLockOnly = FALSE;
    dwStatus = WD_CardRegister(hWD, &cardReg);
    if (cardReg.hCard == 0)
    {
        printf("Failed locking device. Status 0x%x - %s\n", dwStatus,
            Stat2Str(dwStatus));
        WD_Close(hWD);
        return FALSE;
    }

    return TRUE;
}

static void IO_end(void)
{
    WD_CardUnregister(hWD, &cardReg);
    WD_Close(hWD);
}

int main(void)
{
    if (!IO_init())
        return -1;

    /* Call your device driver routines here */

    IO_end();

    return 0;
}

