/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/************************************************************************
*  File: wdc_events.c - Implementation of WDC Plug-and-Play and power   *
*        management events handling API                                 *
*************************************************************************/

#include "wdc_lib.h"
#include "wdc_defs.h"
#include "wdc_err.h"
#include "status_strings.h"

DWORD DLLCALLCONV WDC_EventRegister(_In_ WDC_DEVICE_HANDLE hDev,
    _In_ DWORD dwActions, _In_ EVENT_HANDLER funcEventHandler,
    _In_ PVOID pData, _In_ BOOL fUseKP)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    if (!funcEventHandler || !WdcIsValidDevHandle(hDev))
    {
        WDC_Err("WDC_EventRegister: %s\n",
            !funcEventHandler ? "Error - NULL event handler callback function"
            : WdcGetLastErrStr());
        return WD_INVALID_PARAMETER;
    }

    if (!dwActions)
    {
        WDC_Err("WDC_EventRegister: Error - No events actions were set\n");
        return WD_INVALID_PARAMETER;
    }

    if (pDev->hEvent)
    {
        WDC_Err("WDC_EventRegister: Event already registered\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    BZERO(pDev->Event);

    if (fUseKP)
    {
        if (!WDC_IS_KP(pDev))
        {
            WDC_Err("WDC_EventRegister: Error - Device does not use a Kernel "
                "PlugIn\n");
            return WD_INVALID_PARAMETER;
        }

        pDev->Event.hKernelPlugIn = WDC_GET_KP_HANDLE(pDev);
    }

    pDev->Event.dwAction = dwActions;
    pDev->Event.dwEventType = WD_EVENT_TYPE_PCI;
    pDev->Event.u.Pci.cardId = pDev->id;
    pDev->Event.u.Pci.pciSlot = pDev->slot;

    dwStatus = EventRegister(&pDev->hEvent, WDC_GetWDHandle(), &pDev->Event,
        funcEventHandler, pData);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDC_Err("WDC_EventRegister: Failed to register event.\n"
            "Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));

        pDev->hEvent = NULL;
        return dwStatus;
    }

    WDC_Trace("WDC_EventRegister: Events registered successfully. "
        "event handle 0x%lx\n", pDev->hEvent);

    return WD_STATUS_SUCCESS;
}

DWORD DLLCALLCONV WDC_EventUnregister(_In_ WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    if (!WdcIsValidDevHandle(hDev))
    {
        WDC_Err("WDC_EventUnregister: %s\n", WdcGetLastErrStr());
        return WD_INVALID_PARAMETER;
    }

    if (!pDev->hEvent)
    {
        WDC_Trace("WDC_EventUnregister: Events are not registered\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    dwStatus = EventUnregister(pDev->hEvent);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDC_Err("WDC_EventUnregister: Failed to unregister events.\n"
            "Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
    }

    pDev->hEvent = NULL;

    return dwStatus;
}

BOOL DLLCALLCONV WDC_EventIsRegistered(_In_ WDC_DEVICE_HANDLE hDev)
{
#if defined(DEBUG)
    if (!WdcIsValidDevHandle(hDev))
    {
        WDC_Err("WDC_EventIsRegistered: %s\n", WdcGetLastErrStr());
        return FALSE;
    }
#endif

    return ((PWDC_DEVICE)hDev)->hEvent ? TRUE : FALSE;
}

