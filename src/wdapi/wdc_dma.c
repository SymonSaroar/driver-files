/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/*
 *  File: wdc_dma.c
 *  Implementation of WDC DMA API
 */

#include "utils.h"
#include "wdc_lib.h"
#include "wdc_defs.h"
#include "wdc_err.h"
#include "status_strings.h"

#if !defined (__KERNEL__)
/*
 * Static and inline functions implementations
 */
static DWORD DMABufLock(PWDC_DEVICE pDev, PHYS_ADDR qwAddr, PVOID *ppBuf,
    DWORD dwOptions, DWORD dwDMABufSize, WD_DMA **ppDma, DWORD dwAlignment,
    DWORD dwMaxTransferSize, DWORD dwTransferElementSize)
{
    DWORD dwStatus;
    WD_DMA *pDma;
    DWORD dwPagesNeeded = 0, dwAllocSize;
    BOOL fIsSG = !(dwOptions & DMA_KERNEL_BUFFER_ALLOC);
    BOOL fReserved = (dwOptions & DMA_RESERVED_MEM);
    BOOL fTransaction = (dwOptions & DMA_TRANSACTION);

    if (!WdcIsValidPtr(ppDma, "NULL address of DMA struct pointer") ||
        !WdcIsValidPtr(ppBuf, "NULL address of DMA buffer pointer"))
    {
        return WD_INVALID_PARAMETER;
    }

    dwAllocSize = sizeof(WD_DMA);
    if (fIsSG)
    {
        /*
         * Since the first and last page do not necessarily start (or end) on a
         * PAGE_SIZE boundary, add 1 page to support the worst case scenario
         */
        if (fTransaction)
        {
            dwPagesNeeded = ((dwMaxTransferSize + GetPageSize() - 1) /
                GetPageSize()) + 1;
        }
        else
        {
            dwPagesNeeded = ((dwDMABufSize + GetPageSize() - 1) /
                GetPageSize()) + 1;
        }

        if (WD_DMA_PAGES < dwPagesNeeded)
        {
            dwAllocSize += ((DWORD)sizeof(WD_DMA_PAGE) * (dwPagesNeeded -
                WD_DMA_PAGES));
            dwOptions |= DMA_LARGE_BUFFER;
        }
    }

    pDma = (WD_DMA *)malloc(dwAllocSize);
    if (!pDma)
    {
        WdcSetLastErrStr("Failed allocating memory for a DMA struct\n");
        return WD_INSUFFICIENT_RESOURCES;
    }

    memset(pDma, 0, dwAllocSize);
    pDma->dwBytes = dwDMABufSize;
    pDma->dwOptions = dwOptions;
    pDma->hCard = pDev ? WDC_GET_CARD_HANDLE(pDev) : 0;

    if (fIsSG)
    {
        pDma->pUserAddr = *ppBuf;

        if (dwOptions & DMA_LARGE_BUFFER)
            pDma->dwPages = dwPagesNeeded;
    }

    if (fTransaction)
    {
        if (fIsSG)
        {
            pDma->dwMaxTransferSize = dwMaxTransferSize;
            pDma->dwTransferElementSize = dwTransferElementSize;
        }
        else
        {
            pDma->dwAlignment = dwAlignment;
        }

        dwStatus = WD_DMATransactionInit(WDC_GetWDHandle(), pDma);
    }

    if (!fTransaction)
    {
        if (fReserved)
            pDma->Page[0].pPhysicalAddr = qwAddr;

        dwStatus = WD_DMALock(WDC_GetWDHandle(), pDma);
    }

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        free(pDma);
        WdcSetLastErrStr("Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    /* If a contiguous/reserved kernel buffer was locked, update buffer */
    if (!fIsSG)
        *ppBuf = pDma->pUserAddr;

    *ppDma = pDma;

    return WD_STATUS_SUCCESS;
}

typedef enum
{
    TRANSACTION_EXECUTE,
    TRANSFER_COMPLETED_AND_CHECK,
    TRANSACTION_RELEASE,
    TRANSACTION_UNINIT,
    TRANSACTION_FUNCTIONS_COUNT
} WD_TRANSACTION_FUNC_NAME;

static const char *DMA_TRANSACTION_FUNC_STRING[] =
{
    "TransactionExecute", "TransferCompletedAndCheck", "TransactionRelease",
    "TransactionUninit"
};

static DWORD DLLCALLCONV WDC_DMATransaction(WD_DMA *pDma,
    WD_TRANSACTION_FUNC_NAME funcName)
{
    DWORD dwStatus = WD_OPERATION_FAILED;
    BOOL fIsSG = !(pDma->dwOptions & DMA_KERNEL_BUFFER_ALLOC);
    DWORD dwPagesNeeded = 0;

    if (!WdcIsValidPtr(pDma, "NULL pointer to DMA struct"))
    {
        WDC_Err("%s: %s\n", __FUNCTION__, WdcGetLastErrStr());
        return WD_INVALID_PARAMETER;
    }

    if (fIsSG && (funcName == TRANSACTION_EXECUTE ||
        funcName == TRANSFER_COMPLETED_AND_CHECK))
    {
        dwPagesNeeded = ((pDma->dwMaxTransferSize + GetPageSize() - 1) /
            GetPageSize()) + 1;
        if (WD_DMA_PAGES < dwPagesNeeded)
        {
            pDma->dwPages = dwPagesNeeded;
            pDma->dwOptions |= DMA_LARGE_BUFFER;
        }
    }

    if (pDma->hDma)
    {
        switch (funcName)
        {
        case TRANSACTION_EXECUTE:
            dwStatus = WD_DMATransactionExecute(WDC_GetWDHandle(), pDma);
            break;
        case TRANSFER_COMPLETED_AND_CHECK:
            dwStatus = WD_DMATransferCompletedAndCheck(WDC_GetWDHandle(), pDma);
            break;
        case TRANSACTION_RELEASE:
            dwStatus = WD_DMATransactionRelease(WDC_GetWDHandle(), pDma);
            break;
        case TRANSACTION_UNINIT:
            dwStatus = WD_DMATransactionUninit(WDC_GetWDHandle(), pDma);
            break;
        default:
            return WD_INVALID_PARAMETER;
        }

        if (dwStatus != WD_STATUS_SUCCESS &&
            dwStatus != WD_MORE_PROCESSING_REQUIRED)
        {
            WDC_Err("%s: Failed to perform [%s] function.\n Error 0x%lx - %s\n",
                __FUNCTION__, DMA_TRANSACTION_FUNC_STRING[funcName], dwStatus,
                Stat2Str(dwStatus));
        }
    }

    return dwStatus;
}

/*
 * Functions implementations
 */


DWORD DLLCALLCONV WDC_DMATransactionInit(WDC_DEVICE_HANDLE hDev, PVOID *ppBuf,
    DWORD dwOptions, DWORD dwDMABufSize, WD_DMA **ppDma,
    WDC_INTERRUPT_PARAMS *interruptParams, DWORD dwAlignment,
    DWORD dwMaxTransferSize, DWORD dwTransferElementSize)
{
    DWORD dwStatus;

    dwOptions |= DMA_TRANSACTION;

    if (interruptParams)
    {
        if (!WDC_IntIsEnabled(hDev))
        {
            dwStatus = WDC_IntEnable(hDev, interruptParams->pTransCmds,
                interruptParams->dwNumCmds, interruptParams->dwOptions,
                interruptParams->funcIntHandler, interruptParams->pData,
                interruptParams->fUseKP);
            if (dwStatus != WD_STATUS_SUCCESS)
                return dwStatus;
        }
        else
        {
            WDC_Trace("%s: Interrupts are already enabled\n", __FUNCTION__);
        }
    }

    dwStatus = DMABufLock((PWDC_DEVICE)hDev, 0, ppBuf, dwOptions,
        dwDMABufSize, ppDma, dwAlignment, dwMaxTransferSize,
        dwTransferElementSize);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        WDC_Err("%s: Failed initializing DMA transaction. %s\n", __FUNCTION__,
            WdcGetLastErrStr());
    }

    return dwStatus;
}

DWORD DLLCALLCONV WDC_DMATransactionContigInit(_In_ WDC_DEVICE_HANDLE hDev,
    _Outptr_ PVOID *ppBuf, _In_ DWORD dwOptions, _In_ DWORD dwDMABufSize,
    _Outptr_ WD_DMA **ppDma, _In_ WDC_INTERRUPT_PARAMS *pInterruptParams,
    _In_ DWORD dwAlignment)
{
    dwOptions |= DMA_KERNEL_BUFFER_ALLOC;

    return WDC_DMATransactionInit(hDev, ppBuf, dwOptions, dwDMABufSize,
        ppDma, pInterruptParams, dwAlignment, 0, 0);
}

DWORD DLLCALLCONV WDC_DMATransactionSGInit(_In_ WDC_DEVICE_HANDLE hDev,
    _In_ PVOID pBuf, _In_ DWORD dwOptions, _In_ DWORD dwDMABufSize,
    _Outptr_ WD_DMA **ppDma, _In_ WDC_INTERRUPT_PARAMS *pInterruptParams,
    _In_ DWORD dwMaxTransferSize, _In_ DWORD dwTransferElementSize)
{
   return WDC_DMATransactionInit(hDev, &pBuf, dwOptions, dwDMABufSize,
       ppDma, pInterruptParams, 0, dwMaxTransferSize, dwTransferElementSize);
}

DWORD DLLCALLCONV WDC_DMATransactionExecute(_Inout_ WD_DMA *pDma,
    _In_ DMA_TRANSACTION_CALLBACK funcDMATransactionCallback,
    _In_ PVOID DMATransactionCallbackCtx)
{
    DWORD dwStatus;

    pDma->DMATransactionCallback = funcDMATransactionCallback;
    pDma->DMATransactionCallbackCtx = DMATransactionCallbackCtx;

    dwStatus = WDC_DMATransaction(pDma, TRANSACTION_EXECUTE);
    if (dwStatus == WD_STATUS_SUCCESS && funcDMATransactionCallback)
        funcDMATransactionCallback(DMATransactionCallbackCtx);

    return dwStatus;
}

DWORD DLLCALLCONV WDC_DMATransferCompletedAndCheck(_Inout_ WD_DMA *pDma,
    _In_ BOOL fRunCallback)
{
    DWORD dwStatus = WDC_DMATransaction(pDma, TRANSFER_COMPLETED_AND_CHECK);
    if (dwStatus == WD_MORE_PROCESSING_REQUIRED && fRunCallback)
    {
        if (pDma->DMATransactionCallback)
        {
            pDma->DMATransactionCallback(pDma->DMATransactionCallbackCtx);
        }
        else
        {
            WDC_Trace("%s: callback was not provided to the WD_DMA structure"
                "\n", __FUNCTION__);
        }
    }

    return dwStatus;
}

DWORD DLLCALLCONV WDC_DMATransactionRelease(_In_ WD_DMA *pDma)
{
    return WDC_DMATransaction(pDma, TRANSACTION_RELEASE);
}

DWORD DLLCALLCONV WDC_DMATransactionUninit(_In_ WD_DMA *pDma)
{
    DWORD dwStatus = WDC_DMATransaction(pDma, TRANSACTION_UNINIT);

    if (pDma)
        free(pDma);

    return dwStatus;
}

DWORD DLLCALLCONV WDC_DMAContigBufLock(_In_ WDC_DEVICE_HANDLE hDev,
    _Outptr_ PVOID *ppBuf, _In_ DWORD dwOptions, _In_ DWORD dwDMABufSize,
    _Outptr_ WD_DMA **ppDma)
{
    DWORD dwStatus;

    dwOptions |= DMA_KERNEL_BUFFER_ALLOC;

    dwStatus = DMABufLock((PWDC_DEVICE)hDev, 0, ppBuf, dwOptions, dwDMABufSize,
        ppDma, 0, 0, 0);
    if (WD_STATUS_SUCCESS != dwStatus)
        WDC_Err("WDC_DMAContigBufLock: %s", WdcGetLastErrStr());

    return dwStatus;
}

DWORD DLLCALLCONV WDC_DMASGBufLock(_In_ WDC_DEVICE_HANDLE hDev,
    _In_ PVOID pBuf, _In_ DWORD dwOptions, _In_ DWORD dwDMABufSize,
    _Outptr_ WD_DMA **ppDma)
{
    DWORD dwStatus;

    if (dwOptions & DMA_KERNEL_BUFFER_ALLOC)
    {
        WDC_Err("WDC_DMASGBufLock: Error - The DMA_KERNEL_BUFFER_ALLOC flag "
            "should not be set when locking a Scatter/Gather DMA buffer\n");
        return WD_INVALID_PARAMETER;
    }

    dwStatus = DMABufLock((PWDC_DEVICE)hDev, 0, &pBuf, dwOptions, dwDMABufSize,
        ppDma, 0 ,0 ,0);
    if (WD_STATUS_SUCCESS != dwStatus)
        WDC_Err("WDC_DMASGBufLock: %s\n", WdcGetLastErrStr());

    return dwStatus;
}

DWORD DLLCALLCONV WDC_DMAReservedBufLock(_In_ WDC_DEVICE_HANDLE hDev,
    _In_ PHYS_ADDR qwAddr, _Outptr_ PVOID *ppBuf, _In_ DWORD dwOptions,
    _In_ DWORD dwDMABufSize, _Outptr_ WD_DMA **ppDma)
{
    DWORD dwStatus;

    dwOptions |= DMA_RESERVED_MEM;
    dwOptions |= DMA_KERNEL_BUFFER_ALLOC;

    dwStatus = DMABufLock((PWDC_DEVICE)hDev, qwAddr, ppBuf, dwOptions,
        dwDMABufSize, ppDma, 0, 0, 0);
    if (WD_STATUS_SUCCESS != dwStatus)
        WDC_Err("WDC_DMAReservedBufLock: %s\n", WdcGetLastErrStr());

    return dwStatus;
}

DWORD DLLCALLCONV WDC_DMASyncCpu(_In_ WD_DMA *pDma)
{
    DWORD dwStatus;

    dwStatus = WD_DMASyncCpu(WDC_GetWDHandle(), pDma);
    if (WD_STATUS_SUCCESS != dwStatus)
        WDC_Err("WDC_DMASyncCpu: %s\n", WdcGetLastErrStr());

    return dwStatus;
}

DWORD DLLCALLCONV WDC_DMASyncIo(_In_ WD_DMA *pDma)
{
    DWORD dwStatus;

    dwStatus = WD_DMASyncIo(WDC_GetWDHandle(), pDma);
    if (WD_STATUS_SUCCESS != dwStatus)
        WDC_Err("WDC_DMASyncIo: %s\n", WdcGetLastErrStr());

    return dwStatus;
}

DWORD DLLCALLCONV WDC_DMABufGet(_In_ DWORD hDma, _Outptr_ WD_DMA **ppDma)
{
    DWORD dwStatus;
    WD_DMA *pDma;

    if (!WdcIsValidPtr(ppDma, "NULL address of WD_DMA struct pointer"))
    {
        return WD_INVALID_PARAMETER;
    }

    *ppDma = NULL;

    if (!hDma)
    {
        WDC_Err("WDC_DMABufGet: Invalid shared buffer handle\n");
        return WD_INVALID_HANDLE;
    }

    pDma = (WD_DMA *)malloc(sizeof(WD_DMA));
    if (!pDma)
    {
        WDC_Err("WDC_DMABufGet: Failed allocating memory\n");
        return WD_INSUFFICIENT_RESOURCES;
    }

    BZERO(*pDma);
    pDma->hDma = hDma;
    pDma->dwOptions = DMA_GET_EXISTING_BUF;

    dwStatus = WD_DMALock(WDC_GetWDHandle(), pDma);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDC_Err("WDC_DMABufGet: DMA lock failed. Error 0x%lx - %s\n", dwStatus,
            Stat2Str(dwStatus));
        free(pDma);
        return dwStatus;
    }

    *ppDma = pDma;

    return WD_STATUS_SUCCESS;
}

DWORD DLLCALLCONV WDC_DMABufUnlock(_In_ WD_DMA *pDma)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;

    if (!WdcIsValidPtr(pDma, "NULL pointer to DMA struct"))
    {
        WDC_Err("WDC_DMABufUnlock: %s\n", WdcGetLastErrStr());
        return WD_INVALID_PARAMETER;
    }

    if (pDma->hDma)
    {
        dwStatus = WD_DMAUnlock(WDC_GetWDHandle(), pDma);

        if (WD_STATUS_SUCCESS != dwStatus)
        {
            WDC_Err("WDC_DMABufUnlock: Failed unlocking DMA buffer.\n"
                "Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
        }
    }

    free(pDma);

    return dwStatus;
}
#endif
