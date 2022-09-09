/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/********************************************************************
*  File: wdc_ints.c - Implementation of WDC interrupt handling API  *
*********************************************************************/

#include "wdc_lib.h"
#include "wdc_defs.h"
#include "wdc_err.h"
#include "status_strings.h"

/*************************************************************
  Functions implementations
 *************************************************************/

#define INTERRUPT_TYPE_ALL          \
    (INTERRUPT_LEVEL_SENSITIVE |    \
     INTERRUPT_MESSAGE |            \
     INTERRUPT_MESSAGE_X)

#define OPTION_INTERRUPT(x) ((x) & INTERRUPT_TYPE_ALL)

#if !defined (__KERNEL__)
DWORD DLLCALLCONV WDC_IntEnable(_In_ WDC_DEVICE_HANDLE hDev,
    _In_ WD_TRANSFER *pTransCmds, _In_ DWORD dwNumCmds, _In_ DWORD dwOptions,
    _In_ INT_HANDLER funcIntHandler, _In_ PVOID pData, _In_ BOOL fUseKP)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    DWORD dwIntOptions;
    DWORD dwStatus;

    WDC_Trace("WDC_IntEnable: Entered\n");

    if (!funcIntHandler || !WdcIsValidDevHandle(hDev))
    {
        WDC_Err("WDC_IntEnable: %s",
            !funcIntHandler ? "Error - NULL event handler callback function\n" :
            WdcGetLastErrStr());
        return WD_INVALID_PARAMETER;
    }

    if (pDev->hIntThread)
    {
        WDC_Trace("WDC_IntEnable: Interrupt is already enabled\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    if (!pDev->Int.hInterrupt)
    {
        WDC_Trace("WDC_IntEnable: Error - No interrupt handle\n");
        return WD_INVALID_PARAMETER;
    }

    if (fUseKP)
    {
        if (!WDC_IS_KP(pDev))
        {
            WDC_Err("WDC_IntEnable: Error - No Kernel PlugIn handle\n");
            return WD_INVALID_PARAMETER;
        }

        pDev->Int.kpCall.hKernelPlugIn = WDC_GET_KP_HANDLE(pDev);
    }
    else
    {
        pDev->Int.kpCall.hKernelPlugIn = 0;
    }

    if (dwNumCmds && !pTransCmds)
    {
        WDC_Err("WDC_IntEnable: Error - No interrupt transfer commands "
            "(expecting %ld commands)\n", dwNumCmds);
        return WD_INVALID_PARAMETER;
    }

    /* Check for specific interrupt type selection */
    dwIntOptions = OPTION_INTERRUPT(dwOptions);
    if (dwIntOptions)
    {
        /* Verify that selected interrupt type is supported */
        if (dwIntOptions & pDev->Int.dwOptions)
        {
            pDev->Int.dwOptions &= !INTERRUPT_TYPE_ALL;
            pDev->Int.dwOptions |= dwIntOptions;
        }
        else
        {
            WDC_Err("WDC_IntEnable: Error - selected interrupt type is not "
                "supported by device\n");
        }
    }

    pDev->Int.dwOptions |= dwOptions;
    pDev->Int.Cmd = pTransCmds;
    pDev->Int.dwCmds = dwNumCmds;

    dwStatus = InterruptEnable(&pDev->hIntThread, WDC_GetWDHandle(), &pDev->Int,
        funcIntHandler, pData);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDC_Err("WDC_IntEnable: Failed enabling interrupt.\n"
            "Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));

        pDev->hIntThread = NULL;
        return dwStatus;
    }

    WDC_Trace("WDC_IntEnable: Interrupt enabled successfully\n");

    return WD_STATUS_SUCCESS;
}

DWORD DLLCALLCONV WDC_IntDisable(_In_ WDC_DEVICE_HANDLE hDev)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    DWORD dwStatus;

    WDC_Trace("WDC_IntDisable: Entered\n");

    if (!WdcIsValidDevHandle(hDev))
    {
        WDC_Err("WDC_IntDisable: %s\n", WdcGetLastErrStr());
        return WD_INVALID_PARAMETER;
    }

    if (!pDev->hIntThread)
    {
        WDC_Trace("WDC_IntDisable: Interrupt is already disabled\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    dwStatus = InterruptDisable(pDev->hIntThread);
    if (WD_STATUS_SUCCESS == dwStatus)
    {
        WDC_Trace("WDC_IntDisable: Interrupt disabled successfully\n");
    }
    else
    {
        WDC_Err("WDC_IntDisable: Failed disabling interrupt.\n"
            "Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
    }

    pDev->hIntThread = NULL;
    pDev->Int.kpCall.hKernelPlugIn = 0;

    return dwStatus;
}
#endif

BOOL DLLCALLCONV WDC_IntIsEnabled(_In_ WDC_DEVICE_HANDLE hDev)
{
#if defined(DEBUG)
    if (!WdcIsValidDevHandle(hDev))
    {
        WDC_Err("WDC_IntIsEnabled: %s", WdcGetLastErrStr());
        return FALSE;
    }
#endif

    return ((PWDC_DEVICE)hDev)->hIntThread ? TRUE : FALSE;
}

const CHAR * DLLCALLCONV WDC_IntType2Str(_In_ DWORD dwIntType)
{
    switch (dwIntType)
    {
    case INTERRUPT_MESSAGE_X:
        return "Extended Message-Signaled Interrupt (MSI-X)";
    case INTERRUPT_MESSAGE:
        return "Message-Signaled Interrupt (MSI)";
    case INTERRUPT_LEVEL_SENSITIVE:
        return "Level-Sensitive Interrupt";
    case INTERRUPT_LATCHED:
        return "Edge-Triggered Interrupt";
    }

    return "Unknown";
}

