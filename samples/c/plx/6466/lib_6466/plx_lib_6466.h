/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _PLX_LIB_6466_H_
#define _PLX_LIB_6466_H_

/************************************************************************
*  File: plx_lib_6466.h
*
*  Library for accessing PLX_6466 device.
*  The code accesses hardware using WinDriver's WDC library.
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*************************************************************************/

#include "pci_regs.h"
#include "wdc_lib.h"
#include "plx_regs_6466.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
  Function prototypes
 *************************************************************/
/* -----------------------------------------------
    Device open
   ----------------------------------------------- */
WDC_DEVICE_HANDLE PLX6466_DeviceOpen(DWORD dwVendorID, DWORD dwDeviceID);

/* ----------------------------------------------------
    Plug-and-play and power management events handling
   ---------------------------------------------------- */
DWORD PLX6466_EventRegister(WDC_DEVICE_HANDLE hDev,
    PLX_EVENT_HANDLER funcEventHandler);
DWORD PLX6466_EventUnregister(WDC_DEVICE_HANDLE hDev);
BOOL PLX6466_EventIsRegistered(WDC_DEVICE_HANDLE hDev);

/* -----------------------------------------------
    Access the serial EEPROM
   ----------------------------------------------- */
/* Access EEPROM via VPD */
BOOL PLX6466_EEPROM_VPD_Validate(WDC_DEVICE_HANDLE hDev);
DWORD PLX6466_EEPROM_VPD_Read32(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    UINT32 *pu32Data);
DWORD PLX6466_EEPROM_VPD_Write32(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    UINT32 u32Data);

#ifdef __cplusplus
}
#endif

#endif
