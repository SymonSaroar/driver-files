/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

////////////////////////////////////////////////////////////////
// File - cmos_date.c
//
// This application reads the date directly from the CMOS
// chip on the motherboard.
//
// Note: This code sample is provided AS-IS and as a guiding sample only.
////////////////////////////////////////////////////////////////

#include "cmos_lib.h"

enum {
    OFFSET_Seconds = 0,
    OFFSET_Minutes = 2,
    OFFSET_Hours   = 4,
    OFFSET_Day     = 7,
    OFFSET_Month   = 8,
    OFFSET_Year    = 9,
} CMOS_REGISTER_OFFSETS;

int main()
{
    CMOS_HANDLE hCMOS = NULL;

    printf("CMOS diagnostic utility.\n");
    printf("Application accesses hardware using " WD_PROD_NAME ".\n");

    if (!CMOS_Open(&hCMOS))
    {
        printf("error while opening CMOS:\n");
        printf("%s", CMOS_ErrorString);
        return 0;
    }

    printf("Date and Time read directly from CMOS:\n");
    printf("Time: %x:%02x:%02x\n",
        CMOS_Read(hCMOS, OFFSET_Hours),
        CMOS_Read(hCMOS, OFFSET_Minutes),
        CMOS_Read(hCMOS, OFFSET_Seconds));
    printf("Date: %x/%02x/%02x\n",
        CMOS_Read(hCMOS, OFFSET_Month),
        CMOS_Read(hCMOS, OFFSET_Day),
        CMOS_Read(hCMOS, OFFSET_Year));

    CMOS_Close(hCMOS);

    return 0;
}

