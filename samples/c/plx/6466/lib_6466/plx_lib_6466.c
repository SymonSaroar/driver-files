/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/************************************************************************
*  File: plx_lib_6466.c
*
*  Library for accessing PLX devices.
*  The code accesses hardware using WinDriver's WDC library.
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*************************************************************************/

#include "wdc_defs.h"
#include "utils.h"
#include "status_strings.h"
#include "../../lib/plx_lib.h"
#include "plx_lib_6466.h"

/*************************************************************
  Internal definitions
 *************************************************************/
/* VPD EEPROM delay */
#define EEPROM_VPD_Delay() WDC_Sleep(20000, WDC_SLEEP_BUSY)

/* PLX device information struct */
typedef struct {
    WD_PCI_ID id;
    PLX_EVENT_HANDLER funcDiagEventHandler;

    /* TODO: You can add fields to store additional device-specific
     * information */
} PLX6466_DEV_CTX, *PPLX6466_DEV_CTX;

/*************************************************************
  Global variables definitions
 *************************************************************/
/* Last PLX library error string */
static CHAR gsPLX_LastErr[256];

/*************************************************************
  Static functions prototypes and inline implementation
 *************************************************************/
#if !defined(__KERNEL__)
static DWORD DeviceInit(PWDC_DEVICE pDev);
#endif

static void ErrLog(const CHAR *sFormat, ...);
static void TraceLog(const CHAR *sFormat, ...);

static inline BOOL IsValidDevice(PWDC_DEVICE pDev, const CHAR *sFunc)
{
    if (!pDev || !(PPLX6466_DEV_CTX)(pDev->pCtx))
    {
        snprintf(gsPLX_LastErr, sizeof(gsPLX_LastErr) - 1,
            "%s: NULL device %s\n", sFunc,
            !pDev ? "handle" : "context");
        ErrLog(gsPLX_LastErr);
        return FALSE;
    }

    return TRUE;
}

/*************************************************************
  Functions implementation
 *************************************************************/
#if !defined(__KERNEL__)
/* -----------------------------------------------
    Device open/close
   ----------------------------------------------- */
WDC_DEVICE_HANDLE PLX6466_DeviceOpen(DWORD dwVendorID, DWORD dwDeviceID)
{
    WDC_DEVICE_HANDLE hDev = WDC_DIAG_DeviceFindAndOpen(dwVendorID,
        dwDeviceID, NULL, sizeof(PLX_DEV_CTX));

    if (!hDev || (DeviceInit((PWDC_DEVICE)hDev) != WD_STATUS_SUCCESS))
        goto Error;

    return hDev;

Error:
    if (hDev)
        PLX_DeviceClose(hDev);

    printf("PLX6466_DeviceOpen: Failed opening PCI device: %s",
        PLX_GetLastErr());

    return NULL;
}

static DWORD DeviceInit(PWDC_DEVICE pDev)
{
    /* PPLX6466_DEV_CTX pDevCtx = (PPLX6466_DEV_CTX)WDC_GetDevContext(pDev); */

    /* NOTE: You can modify the implementation of this function in order to
             perform any device initialization you require */

    UNUSED_VAR(pDev);

    return WD_STATUS_SUCCESS;
}
#endif

/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */
static void PLX6466_EventHandler(WD_EVENT *pEvent, PVOID pData)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PPLX6466_DEV_CTX pDevCtx = (PPLX6466_DEV_CTX)WDC_GetDevContext(pDev);

    TraceLog("PLX6466_EventHandler entered, pData 0x%p, dwAction 0x%x\n",
        pData, pEvent->dwAction);

    /* Execute the diagnostics application's event handler function */
    pDevCtx->funcDiagEventHandler(pDev, pEvent->dwAction);
}

DWORD PLX6466_EventRegister(WDC_DEVICE_HANDLE hDev,
    PLX_EVENT_HANDLER funcEventHandler)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PPLX6466_DEV_CTX pDevCtx;
    DWORD dwActions = WD_ACTIONS_ALL;
    /* TODO: Modify the above to set up the plug-and-play/power management
             events for which you wish to receive notifications.
             dwActions can be set to any combination of the WD_EVENT_ACTION
             flags defined in windrvr.h */

    TraceLog("PLX6466_EventRegister entered. Device handle: 0x%p\n", hDev);

    if (!IsValidDevice(pDev, "PLX6466_EventRegister"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PPLX6466_DEV_CTX)WDC_GetDevContext(pDev);

    /* Check if event is already registered */
    if (WDC_EventIsRegistered(hDev))
    {
        ErrLog("Events are already registered ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Store the diag event handler routine to be executed from
     * PLX6466_EventHandler() upon an event */
    pDevCtx->funcDiagEventHandler = funcEventHandler;

    /* Register event */
    dwStatus = WDC_EventRegister(hDev, dwActions, PLX6466_EventHandler, hDev,
        FALSE);

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to register events. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    TraceLog("Events registered\n");

    return WD_STATUS_SUCCESS;
}

DWORD PLX6466_EventUnregister(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;

    TraceLog("PLX6466_EventUnregister entered. Device handle: 0x%p\n", hDev);

    if (!IsValidDevice((PWDC_DEVICE)hDev, "PLX6466_EventUnregister"))
        return WD_INVALID_PARAMETER;

    if (!WDC_EventIsRegistered(hDev))
    {
        ErrLog("Cannot unregister events - no events currently registered ..."
            "\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    dwStatus = WDC_EventUnregister(hDev);

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to unregister events. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

BOOL PLX6466_EventIsRegistered(WDC_DEVICE_HANDLE hDev)
{
    if (!IsValidDevice((PWDC_DEVICE)hDev, "PLX6466_EventIsRegistered"))
        return FALSE;

    return WDC_EventIsRegistered(hDev);
}

/* -----------------------------------------------
    Access the serial EEPROM
   ----------------------------------------------- */
/* VPD EEPROM access */
BOOL PLX6466_EEPROM_VPD_Validate(WDC_DEVICE_HANDLE hDev)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    BYTE bData = 0;

    TraceLog("PLX6466_EEPROM_VPD_Validate entered. Device handle 0x%p\n",
        hDev);

    if (!IsValidDevice(pDev, "PLX6466_EEPROM_VPD_Validate"))
        return FALSE;

    /* Check the next capability pointers */
    WDC_PciReadCfg8(hDev, PLX6466_CAP, &bData);
    if (bData != (BYTE)PLX6466_PMCI)
    {
        ErrLog("PLX6466_EEPROM_VPD_Validate: NEW_CAP register validation "
            "failed\n");
        return FALSE;
    }

    WDC_PciReadCfg8(hDev, PLX6466_PMNCP, &bData);
    if (bData != (BYTE)PLX6466_HSCL)
    {
        ErrLog("PLX6466_EEPROM_VPD_Validate: PMNEXT register validation "
            "failed\n");
        return FALSE;
    }

    WDC_PciReadCfg8(hDev, PLX6466_HSNCP, &bData);
    if (bData != (BYTE)PLX6466_VCI)
    {
        ErrLog("PLX6466_EEPROM_VPD_Validate: HS_NEXT register validation "
            "failed\n");
        return FALSE;
    }

    WDC_PciReadCfg8(hDev, PLX6466_VNCP, &bData);
    if (bData != 0xf0)
    {
        ErrLog("PLX6466_EEPROM_VPD_Validate: VPD_NEXT register validation "
            "failed\n"
            );
        return FALSE;
    }

    return TRUE;
}

DWORD PLX6466_EEPROM_VPD_Read32(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    UINT32 *pu32Data)
{
    DWORD i;
    WORD wAddr, wData;

    if (!IsValidDevice((PWDC_DEVICE)hDev, "PLX6466_EEPROM_VPD_Read32"))
        return WD_INVALID_PARAMETER;

    if (dwOffset % 4)
    {
        ErrLog("PLX6466_EEPROM_VPD_Read32: Error - offset (0x%x) is not a "
            "multiple of 4 (device handle: 0x%p)\n", dwOffset, hDev);
        return WD_INVALID_PARAMETER;
    }

    /* Write a destination serial EEPROM address and flag of operation, value
     * of 0 */
    wAddr = (WORD)(dwOffset & ~BIT15);
    WDC_PciWriteCfg16(hDev, PLX6466_VPDA, wAddr);

    /* Probe a flag of operation until it changes to a 1 to ensure the Read
     * data is available */
    for (i = 0; i < 10; i++)
    {
        EEPROM_VPD_Delay();
        WDC_PciReadCfg16(hDev, PLX6466_VPDA, &wData);

        if (wData & BIT15)
            break;
    }

    if (i == 10)
    {
        ErrLog("PLX6466_EEPROM_VPD_Read32: Error - Acknowledge to EEPROM read "
            "was not received (device handle 0x%p)\n", hDev);
        return WD_OPERATION_FAILED;
    }

    /* Read back the requested data from PVPDATA register */
    WDC_PciReadCfg32(hDev, PLX6466_VPDD, pu32Data);

    return WD_STATUS_SUCCESS;
}

DWORD PLX6466_EEPROM_VPD_Write32(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    UINT32 u32Data)
{
    DWORD i;
    UINT32 u32ReadBack;
    WORD wAddr, wData;

    if (!IsValidDevice((PWDC_DEVICE)hDev, "PLX6466_EEPROM_VPD_Write32"))
        return WD_INVALID_PARAMETER;

    if (dwOffset % 4)
    {
        ErrLog("PLX6466_EEPROM_VPD_Write32: Error - offset (0x%x) is not a "
            "multiple of 4 (device handle: 0x%p)\n", dwOffset, hDev);
        return WD_INVALID_PARAMETER;
    }

    wAddr = (WORD)dwOffset;

    EEPROM_VPD_Delay();

    /* Write desired data to PVPDATA register */
    WDC_PciWriteCfg32(hDev, PLX6466_VPDD, u32Data);

    /* Write a destination serial EEPROM address and flag of operation, value
     * of 1 */
    wAddr = (WORD)(wAddr | BIT15);
    WDC_PciWriteCfg16(hDev, PLX6466_VPDA, wAddr);

    /* Probe a flag of operation until it changes to a 0 to ensure the write
     * completes */
    for (i = 0; i < 10; i++)
    {
        EEPROM_VPD_Delay();
        WDC_PciReadCfg16(hDev, PLX6466_VPDA, &wData);
        if (!(wData & BIT15))
            break;
    }

    PLX6466_EEPROM_VPD_Read32(hDev, dwOffset, &u32ReadBack);

    if (u32ReadBack != u32Data)
    {
        ErrLog("PLX6466_EEPROM_VPD_Write32: Error - Wrote 0x%08X, read back "
            "0x%08X (device handle 0x%p)\n", u32Data, u32ReadBack);
        return WD_OPERATION_FAILED;
    }

    return WD_STATUS_SUCCESS;
}

/* -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- */
static void ErrLog(const CHAR *sFormat, ...)
{
    va_list argp;
    va_start(argp, sFormat);
    vsnprintf(gsPLX_LastErr, sizeof(gsPLX_LastErr) - 1, sFormat, argp);
#if defined(DEBUG)
    WDC_Err("PLX lib: %s", gsPLX_LastErr);
#endif
    va_end(argp);
}

static void TraceLog(const CHAR *sFormat, ...)
{
#if defined(DEBUG)
    CHAR sMsg[256];
    va_list argp;
    va_start(argp, sFormat);
    vsnprintf(sMsg, sizeof(sMsg) - 1, sFormat, argp);
    WDC_Trace("PLX lib: %s", sMsg);
    va_end(argp);
#else
    UNUSED_VAR(sFormat);
#endif
}

