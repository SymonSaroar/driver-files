/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

///////////////////////////////////////////////////////////////////////////
// File - print_struct.c
//
// Note: This code sample is provided AS-IS and as a guiding sample only.
///////////////////////////////////////////////////////////////////////////

#include "windrvr.h"
#include "print_struct.h"
#include <stdio.h>

// Function: WD_CARD_print()
//   Print all resources belonging to device
// Parameters:
//   pCard [in] pointer to device
//   pcPrefix [in] string prefix to add to print
// Return Value:
//   None
void WD_CARD_print(WD_CARD *pCard, char *pcPrefix)
{
    DWORD i;

    for (i = 0; i < pCard->dwItems; i++)
    {
        WD_ITEMS item = pCard->Item[i];

        printf("%sItem ", pcPrefix);
        switch (item.item)
        {
        case ITEM_MEMORY:
            printf("Memory: base 0x%"PRI64"x, size 0x%"PRI64"x",
                item.I.Mem.pPhysicalAddr, item.I.Mem.qwBytes);
            break;

        case ITEM_IO:
            printf("IO: base 0x%"KPRI"x, size 0x%x", item.I.IO.pAddr,
                item.I.IO.dwBytes);
            break;

        case ITEM_INTERRUPT:
            printf("Interrupt: irq %u", item.I.Int.dwInterrupt);
            break;

        case ITEM_BUS:
            printf("Bus: type %u, bus number %u, slot/func 0x%x",
                item.I.Bus.dwBusType, item.I.Bus.dwBusNum,
                item.I.Bus.dwSlotFunc);
            break;

        default:
            printf("Invalid item type");
            break;
        }
        printf("\n");
    }
}

