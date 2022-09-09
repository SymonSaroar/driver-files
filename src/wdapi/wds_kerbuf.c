/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/*
 *  File: wds_kerbuf.c
 *  Implementation of WDS KERNEL BUFFER API
 */

#include "utils.h"
#include "wds_lib.h"
#include "wdc_defs.h"
#include "wdc_err.h"
#include "status_strings.h"

DWORD DLLCALLCONV WDS_SharedBufferAlloc(_In_ UINT64 qwBytes,
    _In_ DWORD dwOptions, _Outptr_ WD_KERNEL_BUFFER **ppKerBuf)
{
    WD_KERNEL_BUFFER *pKerBuf;
    DWORD dwStatus;

    WDC_Trace("WDS_SharedBufferAlloc Entered. bytes [%"PRI64"d] "
        "dwOptions [0x%lx]\n", qwBytes, dwOptions);

    if (!WdcIsValidPtr(ppKerBuf, "NULL address of WD_KERNEL_BUFFER struct "
        "pointer"))
    {
        return WD_INVALID_PARAMETER;
    }

    pKerBuf = (WD_KERNEL_BUFFER *)malloc(sizeof(WD_KERNEL_BUFFER));
    if (!pKerBuf)
    {
        WDC_Err("WDS_SharedBufferAlloc: Memory allocation failed\n");
        return WD_INSUFFICIENT_RESOURCES;
    }

    pKerBuf->qwBytes = qwBytes;
    pKerBuf->dwOptions = dwOptions;

    dwStatus = WD_KernelBufLock(WDC_GetWDHandle(), pKerBuf);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDC_Err("WDS_SharedBufferAlloc: Failed kernel buffer alloc. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
        free(pKerBuf);
        return dwStatus;
    }

    *ppKerBuf = pKerBuf;

    return WD_STATUS_SUCCESS;
}

DWORD DLLCALLCONV WDS_SharedBufferGet(_In_ DWORD hKerBuf,
    _Outptr_ WD_KERNEL_BUFFER **ppKerBuf)
{
    WD_KERNEL_BUFFER *pKerBuf;
    DWORD dwStatus;

    if (!WdcIsValidPtr(ppKerBuf, "NULL address of WD_KERNEL_BUFFER struct "
        "pointer"))
    {
        return WD_INVALID_PARAMETER;
    }

    *ppKerBuf = NULL;

    if (!hKerBuf)
    {
        WDC_Err("WDS_SharedBufferGet: Invalid kernel buffer handle\n");
        return WD_INVALID_HANDLE;
    }

    pKerBuf = (WD_KERNEL_BUFFER *)malloc(sizeof(WD_KERNEL_BUFFER));
    if (!pKerBuf)
    {
        WDC_Err("WDS_SharedBufferGet: Memory allocation failed\n");
        return WD_INSUFFICIENT_RESOURCES;
    }

    BZERO(*pKerBuf);
    pKerBuf->hKerBuf = hKerBuf;
    pKerBuf->dwOptions = KER_BUF_GET_EXISTING_BUF;

    dwStatus = WD_KernelBufLock(WDC_GetWDHandle(), pKerBuf);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDC_Err("WDS_SharedBufferGet: Failed getting existing kernel buffer. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
        free(pKerBuf);
        return dwStatus;
    }

    *ppKerBuf = pKerBuf;

    return WD_STATUS_SUCCESS;
}

DWORD DLLCALLCONV WDS_SharedBufferFree(_In_ WD_KERNEL_BUFFER *pKerBuf)
{
    DWORD dwStatus;

    WDC_Trace("WDS_SharedBufferFree Entered\n");

    if (!WdcIsValidPtr(pKerBuf, "NULL address of KERNEL_BUFFER struct pointer"))
        return WD_INVALID_PARAMETER;

    dwStatus = WD_KernelBufUnlock(WDC_GetWDHandle(), pKerBuf);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDC_Err("WDS_SharedBufferFree: Failed unlocking kernel buffer. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
    }

    free(pKerBuf);

    return dwStatus;
}

