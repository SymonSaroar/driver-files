/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/****************************************************************************
*  File: qdma_lib.c
*
*  Implementation of a sample library for accessing Xilinx PCI Express cards
*  with QDMA design, using the WinDriver WDC API.
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*****************************************************************************/

#if defined(__KERNEL__)
#include "kpstdlib.h"
#endif
#include "utils.h"
#include "status_strings.h"
#include "qdma_lib.h"
#include "qdma_internal.h"
#include "wdc_diag_lib.h"

static CHAR gsQDMA_LastErr[256]; /* Last error information string */

/*************************************************************
  Internal definitions
 *************************************************************/
/* WinDriver license registration string */
/* TODO: When using a registered WinDriver version, replace the license string
         below with the development license in order to use on the development
         machine.
         Once you require to distribute the driver's package to other machines,
         please replace the string with a distribution license */
#define QDMA_DEFAULT_LICENSE_STRING "12345abcde12345.abcde"

#define QDMA_DEFAULT_DRIVER_NAME WD_DEFAULT_DRIVER_NAME_BASE

/*************************************************************
  Static functions prototypes and inline implementation
 *************************************************************/

static void QDMA_EventHandler(WD_EVENT *pEvent, PVOID pData);

/* -----------------------------------------------
    Open/close handle functions
   ----------------------------------------------- */
static DWORD QDMA_InitPfContext(WDC_DEVICE_HANDLE hDev);
static DWORD QDMA_SetBarTypesIndexes(WDC_DEVICE_HANDLE hDev);
static DWORD QDMA_SetDeviceAttributes(WDC_DEVICE_HANDLE hDev,
    QDMA_DEVICE_ATTRIBUTES *deviceInfo);
static void QDMA_InitQdmaGlobal(WDC_DEVICE_HANDLE hDev, PQDMA_DEV_CTX pDevCtx);
static DWORD QDMA_InitPf(WDC_DEVICE_HANDLE hDev);

/* -----------------------------------------------
    DMA request tracker functions
   ----------------------------------------------- */
static DWORD QDMA_DmaRequestTrackerInit(DMA_REQUEST_TRACKER *requestTracker,
    DWORD dwEntries);
static DWORD QDMA_DmaRequestTrackerEnqueue(DMA_REQUEST_TRACKER *requestTracker,
    REQ_CTX *req);
static DWORD QDMA_DmaRequestTrackerDequeue(DMA_REQUEST_TRACKER *requestTracker);
static DWORD QDMA_DmaRequestTrackerPeek(DMA_REQUEST_TRACKER *requestTracker,
    REQ_CTX **req);
static void QDMA_DmaRequestTrackerUnlock(DMA_REQUEST_TRACKER *requestTracker);

/* -----------------------------------------------
    Threads functions
   ----------------------------------------------- */
static DWORD QDMA_ThreadsCreate(THREAD_MANAGER *thread_manager);
static QDMA_THREAD *QDMA_ThreadAssociate(WDC_DEVICE_HANDLE hDev,
    DWORD dwQueueId);
void QDMA_ThreadsTerminate(THREAD_MANAGER *threadManager);
void QDMA_ThreadNotify(QUEUE_PAIR *queuePair);
static void DLLCALLCONV QDMA_ThreadPoll(PVOID context);
static void QDMA_FreePollThread(WDC_DEVICE_HANDLE hDev, QDMA_THREAD *thread,
    DWORD dwQueueId);
static void QDMA_DrvMmCompletion_cb(WDC_DEVICE_HANDLE hDev, DWORD dwQueueId,
    BOOL fIsH2c, void *pCtx);
static void DLLCALLCONV QDMA_ProgramMmDma_cb(PVOID pData);
static DWORD QDMA_EnqueueMmRequest(DMA_REQUEST_CONTEXT *dmaRequestContext);
static DWORD QDMA_MmChannelEnable(WDC_DEVICE_HANDLE hDev, DWORD channel,
    BOOL fIsH2C);

/* -----------------------------------------------
    Queue pairs functions
   ----------------------------------------------- */
static DWORD QDMA_QueuesPairsInit(WDC_DEVICE_HANDLE hDev);
static DWORD QDMA_QueuePairCreate(WDC_DEVICE_HANDLE hDev, QUEUE_PAIR *queuePair,
    QUEUE_CONFIG *queueConfig);
static void QDMA_QueuePairService(QUEUE_PAIR *queuePair);
static void QDMA_QueuePairMMService(QUEUE_PAIR *queuePair, BOOL fIsH2C);
static DWORD QDMA_QueuePairEnqueueMmRequest(
    DMA_REQUEST_CONTEXT *dmaRequestContext, QUEUE_PAIR *queuePair);
static DWORD QDMA_SetContext(WDC_DEVICE_HANDLE hDev, QUEUE_PAIR *queuePair,
    BOOL fIsH2C);
static DWORD QDMA_IndexDelta(UINT32 u32LongIdx, UINT32 u32ShortIdx,
    DWORD dwCapacity);
static void QDMA_QueuePairUnlock(QUEUE_PAIR *queuePair);

/* -----------------------------------------------
    H2C/C2H queues functions
   ----------------------------------------------- */
static DWORD QDMA_QueueCreate(WDC_DEVICE_HANDLE hDev, BOOL fIsH2C, QUEUE *queue,
    QUEUE_CONFIG *queueConfig);
static DWORD QDMA_QueueProgram(WDC_DEVICE_HANDLE hDev, DWORD dwQueueId);
static void QDMA_QueueUnlock(QUEUE *queue);

/* -----------------------------------------------
    Ring buffer functions
   ----------------------------------------------- */
static DWORD QDMA_RingBufferInit(RING_BUFFER *ringBuffer, DWORD dwSize);
static DWORD QDMA_RingBufferAllocate(WDC_DEVICE_HANDLE hDev,
    RING_BUFFER *ringBuffer, DWORD dwRingBufferSize);
static void QDMA_RingBufferUnlock(RING_BUFFER *ringBuffer);
static void QDMA_RingBufferAdvanceIdx(RING_BUFFER *ringBuffer, UINT32 *u32Index,
    DWORD dwNum);
static void QDMA_RegistersAccessLock(WDC_DEVICE_HANDLE hDev);
static void QDMA_RegistersAccessUnlock(WDC_DEVICE_HANDLE hDev);
static DWORD QDMA_GetGlobalRingSize(WDC_DEVICE_HANDLE hDev, UINT8 u8Index,
    UINT8 u8Count, UINT32 *pu32GlobalRingSize);

/* -----------------------------------------------
    Read/write/clear registers functions
   ----------------------------------------------- */
static void QDMA_CsrInit(WDC_DEVICE_HANDLE hDev, QUEUE_PAIR *queuePair);
static DWORD QDMA_CsrSetDefaultGlobal(WDC_DEVICE_HANDLE hDev);
static void QDMA_CsrReadValues(WDC_DEVICE_HANDLE hDev, UINT32 u32RegisterOffset,
    DWORD dwId, DWORD dwCount, UINT32 *values);
static void QDMA_CsrWriteValues(WDC_DEVICE_HANDLE hDev, UINT32 u32RegisterOffset,
    DWORD dwId, DWORD dwCount, const UINT32 *values);
static void QDMA_CsrUpdatePidx(WDC_DEVICE_HANDLE hDev, BOOL fIsH2C,
    QUEUE_PAIR *queuePair, UINT32 newPidx);
static DWORD QDMA_FunctionMapWrite(WDC_DEVICE_HANDLE hDev, DWORD dwFuncId,
    const QDMA_FMAP_CONFIG *fmapConfig);
static DWORD QDMA_ClearContexts(WDC_DEVICE_HANDLE hDev, DWORD dwQueueId);
static DWORD QDMA_HwMonitorReg(WDC_DEVICE_HANDLE hDev,
    UINT32 u32RegisterOffset, UINT32 u32Mask, UINT32 u32Value,
    DWORD dwIntervalUs, DWORD dwTimeoutUs);
static DWORD QDMA_ContextClear(WDC_DEVICE_HANDLE hDev, CLEAR_TYPE clearType,
    BOOL fIsH2c, UINT16 u16QueueId);
static DWORD QDMA_SwContextWrite(WDC_DEVICE_HANDLE hDev, BOOL fIsH2c,
    UINT16 u16QueueId, const QDMA_DESCQ_SW_CTX *pSwCtx);
static UINT32 QDMA_RegRead(WDC_DEVICE_HANDLE hDev, UINT32 u32RegisterOffset);
static void QDMA_RegWrite(WDC_DEVICE_HANDLE hDev, UINT32 u32RegisterOffset,
    UINT32 u32Value);
static DWORD QDMA_IndirectRegWrite(WDC_DEVICE_HANDLE hDev, IND_CTX_CMD_SEL sel,
    DWORD dwHwQueueId, UINT32 *pu32Data, UINT16 u16Count);
static DWORD QDMA_IndirectRegClear(WDC_DEVICE_HANDLE hDev,
    IND_CTX_CMD_SEL sel, UINT16 u16QueueId);

/* -----------------------------------------------
    Dump functions
   ----------------------------------------------- */
static void QDMA_DumpDescriptorMm(const MM_DESCRIPTOR *desc);
static void QDMA_DumpCsrQueueReg(WDC_DEVICE_HANDLE hDev);
static void QDMA_DumpGlobalReg(WDC_DEVICE_HANDLE hDev);
static void QDMA_DumpContexts(WDC_DEVICE_HANDLE hDev, const DWORD dwQueueId);

#ifdef DEBUG
static DWORD QDMA_IndirectRegRead(WDC_DEVICE_HANDLE hDev, IND_CTX_CMD_SEL sel,
    UINT16 dwHwQueueId, UINT32 u32Count, UINT32 *pu32Data);
static DWORD QDMA_SwContextRead(WDC_DEVICE_HANDLE hDev, BOOL fIsH2c,
    UINT16 u16QueueId, QDMA_DESCQ_SW_CTX *pSwCtx);
static DWORD QDMA_HwContextRead(WDC_DEVICE_HANDLE hDev, BOOL fIsH2c,
    UINT16 u16QueueId, QDMA_DESCQ_HW_CTX *pHwCtx);
static void QDMA_DumpCtxDescqHw(const char *dir,
    const QDMA_DESCQ_HW_CTX *pHwCtx);
static void QDMA_DumpCtxDescqSw(const char *dir,
    const QDMA_DESCQ_SW_CTX *pSwCtx);
#endif

/* Validate a WDC device handle */
static inline BOOL IsValidDevice(PWDC_DEVICE pDev, const CHAR *sFunc)
{
    if (!pDev || !WDC_GetDevContext(pDev))
    {
        ErrLog("%s: NULL device %s\n", sFunc, !pDev ? "handle" : "context");
        return FALSE;
    }

    return TRUE;
}

/*************************************************************
  Functions implementation
 *************************************************************/

 /* -----------------------------------------------
     QDMA and WDC libraries initialize/uninitialize
    ----------------------------------------------- */

/* Initialize the Xilinx QDMA and WDC libraries */
DWORD QDMA_LibInit(const CHAR *sLicense)
{
    DWORD dwStatus;

#if defined(WD_DRIVER_NAME_CHANGE)
    /* Set the driver name */
    if (!WD_DriverName(QDMA_DEFAULT_DRIVER_NAME))
    {
        ErrLog("%s: Failed to set the driver name for WDC library.\n",
            __FUNCTION__);
        return WD_SYSTEM_INTERNAL_ERROR;
    }
#endif

    /* Set WDC library's debug options
     * (default: level=TRACE; redirect output to the Debug Monitor) */
    dwStatus = WDC_SetDebugOptions(WDC_DBG_DEFAULT, NULL);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("%s: Failed to initialize debug options for WDC library.\n"
            "Error 0x%x - %s\n", __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    /* Open a handle to the driver and initialize the WDC library */
    dwStatus = WDC_DriverOpen(WDC_DRV_OPEN_DEFAULT,
        sLicense ? sLicense : QDMA_DEFAULT_LICENSE_STRING);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("%s: Failed to initialize the WDC library. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    return WD_STATUS_SUCCESS;
}

/* Uninitialize the Xilinx QDMA and WDC libraries */
DWORD QDMA_LibUninit(void)
{
    DWORD dwStatus;

    /* Uninitialize the WDC library and close the handle to WinDriver */
    dwStatus = WDC_DriverClose();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("%s: Failed to uninit the WDC library. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

#if !defined(__KERNEL__)

/* -----------------------------------------------
    Open/close handle functions
   ----------------------------------------------- */
BOOL QDMA_DeviceInit(WDC_DEVICE_HANDLE hDev)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(
        (PWDC_DEVICE)hDev);
    DWORD dwStatus = QDMA_InitPfContext(hDev);

    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Failed initializing physical function context. Error 0x%x "
            "- %s\n", __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    QDMA_InitQdmaGlobal(hDev, pDevCtx);

    dwStatus = QDMA_InitPf(hDev);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Failed initializing physical function. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    dwStatus = QDMA_QueuesPairsInit(hDev);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Failed initializing queues pairs. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    dwStatus = QDMA_ThreadsCreate(&pDevCtx->threadManager);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Failed creating threads. Error 0x%x - %s\n", __FUNCTION__,
            dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    return TRUE;

Error:
    return FALSE;
}

WDC_DEVICE_HANDLE QDMA_DeviceOpen(DWORD dwVendorID, DWORD dwDeviceID)
{
    WDC_DEVICE_HANDLE hDev = WDC_DIAG_DeviceFindAndOpen(dwVendorID,
        dwDeviceID, NULL, sizeof(QDMA_DEV_CTX));

    if (!hDev || !QDMA_DeviceInit(hDev))
        goto Error;

    return hDev;

Error:
    if (hDev)
        QDMA_DeviceClose(hDev);

    printf("QDMA_DeviceOpen: Failed opening PCI device: %s",
        QDMA_GetLastErr());

    return NULL;
}

/* Close a device handle */
BOOL QDMA_DeviceClose(WDC_DEVICE_HANDLE hDev)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PQDMA_DEV_CTX pDevCtx;

    TraceLog("%s: Entered. Device handle [0x%p]\n", __FUNCTION__, hDev);

    /* Validate the WDC device handle */
    if (!hDev)
    {
        ErrLog("%s: Error - NULL device handle\n", __FUNCTION__);
        return FALSE;
    }

    QDMA_QueuesClear(hDev);

    pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(pDev);

    QDMA_ThreadsTerminate(&pDevCtx->threadManager);

    return WDC_DIAG_DeviceClose(hDev);
}

/* Initialize physical function context */
static DWORD QDMA_InitPfContext(WDC_DEVICE_HANDLE hDev)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    static DWORD dwActivePfCount = 0;
    QDMA_DEVICE_ATTRIBUTES *deviceInfo;
    DWORD dwStatus;

    /* TODO: should be determined by device handle */
    pDevCtx->dwFunctionId = 0;

    dwStatus = QDMA_SetBarTypesIndexes(hDev);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("%s: Failed set bar types. Error 0x%x - %s\n", __FUNCTION__,
            dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    pDevCtx->mode = POLLING_MODE;
    pDevCtx->deviceConf.dwPfNumber = dwActivePfCount;

    if (pDevCtx->deviceConf.dwPfNumber == 0)
        pDevCtx->deviceConf.fIsMasterPf = TRUE;

    dwActivePfCount++;

    TraceLog("%s: dwActivePfCount [%d], dwPfNumber [%d]\n", __FUNCTION__,
        dwActivePfCount, pDevCtx->deviceConf.dwPfNumber);

    deviceInfo = &pDevCtx->deviceConf.deviceInfo;
    dwStatus = QDMA_SetDeviceAttributes(hDev, deviceInfo);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Failed set device attributes. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    TraceLog("%s: numPfs [%d], fIsMmEnabled [%d], "
        "fIsStreamingEnabled [%d]\n", __FUNCTION__, deviceInfo->numPfs,
        deviceInfo->fIsMmEnabled, deviceInfo->fIsStreamingEnabled);

    if (pDevCtx->deviceConf.deviceInfo.fIsMmEnabled == 0)
    {
        ErrLog("%s: MM mode is disabled\n", __FUNCTION__);
        goto Error;
    }

Error:
    return dwStatus;
}

/* Set bar types indexes (fmapConfig and user) */
static DWORD QDMA_SetBarTypesIndexes(WDC_DEVICE_HANDLE hDev)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    UINT32 u32BarLite = 0, u32FuncId = 0, u32VersionReg;
    DWORD dwStatus, i, dwNumAddrSpaces = pDev->dwNumAddrSpaces;
    DWORD dwUserBarId = ULONG_MAX, dwConfigBarId = ULONG_MAX;

    /* DWORD dwBypassBarId = ULONG_MAX; */

    for (i = 0; i < dwNumAddrSpaces; i++)
    {
        u32VersionReg = 0;

        dwStatus = WDC_ReadAddr32(hDev, i, QDMA_OFFSET_CONFIG_BLOCK_ID,
            &u32VersionReg);
        if (dwStatus != WD_STATUS_SUCCESS)
        {
            ErrLog("%s: Failed to read version register. Error 0x%x - %s\n",
                __FUNCTION__, dwStatus, Stat2Str(dwStatus));
            goto Exit;
        }

        if ((u32VersionReg & MASK_QDMA_ID) == VAL_QDMA_ID)
        {
            dwConfigBarId = i;
            pDevCtx->dwConfigBarNum = dwConfigBarId;
            TraceLog("%s: Found config bar at %d\n", __FUNCTION__, i);
            break;
        }
    }

    if (dwConfigBarId == ULONG_MAX)
    {
        ErrLog("%s: Failed to find config bar!\n", __FUNCTION__);
        dwStatus = WD_INVALID_HANDLE;
        goto Exit;
    }

    dwStatus = WDC_ReadAddr32(hDev, i, QDMA_GLBL2_PF_BARLITE_EXT, &u32BarLite);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Failed to read bar lite register!\n", __FUNCTION__);
        goto Exit;
    }

    dwStatus = WDC_ReadAddr32(hDev, i, QDMA_GLBL2_CHANNEL_FUNC_RET, &u32FuncId);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Failed to read channel function id!\n", __FUNCTION__);
        goto Exit;
    }

    u32BarLite = ((u32BarLite >> (6 * u32FuncId)) & 0x3F);

    for (i = 0; i < dwNumAddrSpaces; i++)
    {
        if (u32BarLite & (1 << i))
        {
            dwUserBarId = i / 2;
            pDevCtx->dwUserBarNum = dwUserBarId;
            TraceLog("%s: Found user bar at %d\n", __FUNCTION__,  i / 2);
            break;
        }
    }

    for (i = 0; i < dwNumAddrSpaces; i++)
    {
        if (i == dwUserBarId || i == dwConfigBarId)
            continue;

        /* dwBypassBarId = i; */
        TraceLog("%s: Found bypass bar at %d\n", __FUNCTION__, i);
        break;
    }

Exit:
    return dwStatus;
}

/* Set device attributes */
static DWORD QDMA_SetDeviceAttributes(WDC_DEVICE_HANDLE hDev,
    QDMA_DEVICE_ATTRIBUTES *deviceInfo)
{
    DWORD dwCount = 0;
    UINT32 registerValue = 0;

    if (!hDev)
        return WD_INVALID_PARAMETER;

    /* number of PFs */
    registerValue = QDMA_RegRead(hDev, QDMA_OFFSET_GLBL2_PF_BARLITE_INT);
    if (FIELD_GET(QDMA_GLBL2_PF0_BAR_MAP_MASK, registerValue))
        dwCount++;
    if (FIELD_GET(QDMA_GLBL2_PF1_BAR_MAP_MASK, registerValue))
        dwCount++;
    if (FIELD_GET(QDMA_GLBL2_PF2_BAR_MAP_MASK, registerValue))
        dwCount++;
    if (FIELD_GET(QDMA_GLBL2_PF3_BAR_MAP_MASK, registerValue))
        dwCount++;
    deviceInfo->numPfs = (BYTE)dwCount;

    /* Number of queues */
    registerValue = QDMA_RegRead(hDev, QDMA_OFFSET_GLBL2_CHANNEL_QDMA_CAP);
    deviceInfo->numQueues = FIELD_GET(QDMA_GLBL2_MULTQ_MAX_MASK, registerValue);

    /* FLR present */
    registerValue = QDMA_RegRead(hDev, QDMA_OFFSET_GLBL2_MISC_CAP);
    deviceInfo->fIsMailboxEnabled = FIELD_GET(QDMA_GLBL2_MAILBOX_EN_MASK,
        registerValue);
    deviceInfo->fIsFlrPresent = FIELD_GET(QDMA_GLBL2_FLR_PRESENT_MASK,
        registerValue);
    deviceInfo->fIsMmCompletionsSupported = FIELD_GET(
        QDMA_GLBL2_MM_CMPT_EN_MASK, registerValue);

    /* ST/MM enabled? */
    registerValue = QDMA_RegRead(hDev, QDMA_OFFSET_GLBL2_CHANNEL_MDMA);
    deviceInfo->fIsMmEnabled = (FIELD_GET(QDMA_GLBL2_MM_C2H_MASK, registerValue)
        && FIELD_GET(QDMA_GLBL2_MM_H2C_MASK, registerValue)) ? 1 : 0;
    deviceInfo->fIsStreamingEnabled = (FIELD_GET(QDMA_GLBL2_ST_C2H_MASK,
        registerValue) && FIELD_GET(QDMA_GLBL2_ST_H2C_MASK, registerValue)) ?
        1 : 0;

    /* num of mm channels */
    deviceInfo->mmChannelMax = 1;

    return WD_STATUS_SUCCESS;
}

/* Initialize CSR default values and set global masks registers values */
static void QDMA_InitQdmaGlobal(WDC_DEVICE_HANDLE hDev, PQDMA_DEV_CTX pDevCtx)
{
    if (pDevCtx->deviceConf.fIsMasterPf)
    {
        TraceLog("%s: Setting Global CSR\n", __FUNCTION__);
        QDMA_CsrSetDefaultGlobal(hDev);
    }

    QDMA_RegWrite(hDev, QDMA_OFFSET_GLBL_ERR_MASK, 0xFFFFFFFF);
    QDMA_RegWrite(hDev, QDMA_OFFSET_GLBL_DSC_ERR_MSK, 0xFFFFFFFF);
    QDMA_RegWrite(hDev, QDMA_OFFSET_GLBL_TRQ_ERR_MSK, 0xFFFFFFFF);
    QDMA_RegWrite(hDev, QDMA_OFFSET_C2H_ERR_MASK, 0xFFFFFFFF);

    OsMutexCreate(&pDevCtx->hRegisterAccessMutex);

    QDMA_DumpGlobalReg(hDev);
}

/* Initialize physical function */
static DWORD QDMA_InitPf(WDC_DEVICE_HANDLE hDev)
{
    DWORD i, dwStatus = WD_STATUS_SUCCESS;

    /* map 512 queues to each function */
    for (i = 0; i < QDMA_NUM_PF_FUNC; i++)
    {
        QDMA_FMAP_CONFIG config = { 0 };
        config.qbase = (i * QDMA_MAX_QUEUES_PER_PF);
        config.qmax = QDMA_MAX_QUEUES_PER_PF;

        dwStatus = QDMA_FunctionMapWrite(hDev, i, &config);
        if (dwStatus != WD_STATUS_SUCCESS)
        {
            TraceLog("%s: Failed to write function map. Error 0x%x - %s\n",
                __FUNCTION__, dwStatus, Stat2Str(dwStatus));
            goto Exit;
        }
    }

Exit:
    return dwStatus;
}

/* -----------------------------------------------
    Public functions
   ----------------------------------------------- */

/* Add queue */
DWORD QDMA_AddQueue(WDC_DEVICE_HANDLE hDev, DWORD dwQueueId,
    QUEUE_CONFIG *queueConfig)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    DWORD dwStatus = WD_STATUS_SUCCESS;
    QUEUE_PAIR *queuePair;

    if (dwQueueId >= QDMA_MAX_QUEUES_PER_PF)
    {
        ErrLog("%s: Invalid queue index provided\n", __FUNCTION__);
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;

    }

    if (!queueConfig->fIsSt && !pDevCtx->deviceConf.deviceInfo.fIsMmEnabled)
    {
        ErrLog("%s: MM mode is disabled in the device\n", __FUNCTION__);
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    TraceLog("%s: Adding the queue %d\n", __FUNCTION__, dwQueueId);

    OsMutexLock(pDevCtx->hQueueAccessMutex);

    queuePair = &pDevCtx->queuePairs[dwQueueId];
    if (queuePair->state != QUEUE_STATE_QUEUE_AVAILABLE)
    {
        ErrLog("%s: Queue %d is not available!\n", __FUNCTION__, dwQueueId);
        dwStatus = WD_INVALID_PARAMETER;
        goto ExitWithUnlock;
    }

    dwStatus = QDMA_QueuePairCreate(hDev, queuePair, queueConfig);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Failed to allocate resource. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto ExitWithUnlock;
    }

    queuePair->h2cQueue.libConfig.fIrqEnabled = FALSE;
    queuePair->c2hQueue.libConfig.fIrqEnabled = FALSE;

    queuePair->thread = QDMA_ThreadAssociate(hDev, dwQueueId);

    dwStatus = QDMA_QueueProgram(hDev, dwQueueId);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Failed to program the queue. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto ExitWithUnlock;
    }

    QDMA_DumpContexts(hDev, queuePair->dwIdAbsolute);

    QDMA_CsrInit(hDev, queuePair);

    queuePair->state = QUEUE_STATE_QUEUE_PROGRAMMED;

ExitWithUnlock:
    OsMutexUnlock(pDevCtx->hQueueAccessMutex);
Exit:
    return dwStatus;
}

/* Start queue */
DWORD QDMA_StartQueue(WDC_DEVICE_HANDLE hDev, DWORD dwQueueId)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    DWORD dwStatus = WD_STATUS_SUCCESS;
    QUEUE_PAIR *queuePair;

    if (dwQueueId >= QDMA_MAX_QUEUES_PER_PF)
    {
        ErrLog("%s: Invalid queue index provided\n", __FUNCTION__);
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    TraceLog("%s: Starting the queue %d\n", __FUNCTION__, dwQueueId);

    OsMutexLock(pDevCtx->hQueueAccessMutex);

    queuePair = &pDevCtx->queuePairs[dwQueueId];
    if (queuePair->state != QUEUE_STATE_QUEUE_PROGRAMMED)
    {
        ErrLog("%s: Queue %d is not available!\n", __FUNCTION__, dwQueueId);
        dwStatus = WD_INVALID_PARAMETER;
        goto ExitWithUnlock;
    }

    queuePair->state = QUEUE_STATE_QUEUE_STARTED;

ExitWithUnlock:
    OsMutexUnlock(pDevCtx->hQueueAccessMutex);
Exit:
    return dwStatus;
}

/* Input/Output (read/write) function */
DWORD QDMA_IoMmDma(DMA_REQUEST_CONTEXT *dmaRequestContext)
{
    DWORD dwStatus;
    DWORD direction = dmaRequestContext->fIsH2C ? DMA_TO_DEVICE :
        DMA_FROM_DEVICE;

    dmaRequestContext->status = DMA_REQUEST_STATUS_UNITIALIZED;
    memset(&dmaRequestContext->timeStart, 0, sizeof(TIME_TYPE));
    memset(&dmaRequestContext->timeEnd, 0, sizeof(TIME_TYPE));

    dwStatus = WDC_DMATransactionSGInit(dmaRequestContext->hDev,
        dmaRequestContext->pBuf, DMA_ALLOW_64BIT_ADDRESS | direction,
        dmaRequestContext->dwBytes, &dmaRequestContext->pDma, NULL,
        QDMA_TRANSACTION_SAMPLE_MAX_TRANSFER_SIZE, sizeof(MM_DESCRIPTOR));
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: WDC_DMATransactionSGInit() failed. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto Exit;
    }

    get_cur_time(&dmaRequestContext->timeStart);
    dwStatus = WDC_DMATransactionExecute(dmaRequestContext->pDma,
        QDMA_ProgramMmDma_cb, dmaRequestContext);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: DMATransactionExecute() failed. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto Exit;
    }

    TraceLog("%s: DMA request executed\n", __FUNCTION__);

Exit:
    if (dwStatus != WD_STATUS_SUCCESS)
        dmaRequestContext->status = DMA_REQUEST_STATUS_ERROR;
    if (dmaRequestContext->status == DMA_REQUEST_STATUS_ERROR)
    {
        if (dwStatus == WD_STATUS_SUCCESS)
            dwStatus = WD_WINDRIVER_STATUS_ERROR;
    }

    return dwStatus;
}

/* Stop queue */
DWORD QDMA_StopQueue(WDC_DEVICE_HANDLE hDev, DWORD dwQueueId)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    DWORD dwStatus = WD_STATUS_SUCCESS;
    QUEUE_PAIR *queuePair;

    if (dwQueueId >= QDMA_MAX_QUEUES_PER_PF)
    {
        ErrLog("%s: Invalid queue index provided\n", __FUNCTION__);
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    TraceLog("%s: Stopping the queue %d\n", __FUNCTION__, dwQueueId);

    OsMutexLock(pDevCtx->hQueueAccessMutex);
    queuePair = &pDevCtx->queuePairs[dwQueueId];

    if (queuePair->state != QUEUE_STATE_QUEUE_STARTED)
    {
        ErrLog("%s: Queue %d is not available!\n", __FUNCTION__, dwQueueId);
        dwStatus = WD_INVALID_PARAMETER;
        goto ExitWithUnlock;
    }

    queuePair->state = QUEUE_STATE_QUEUE_PROGRAMMED;

ExitWithUnlock:
    OsMutexUnlock(pDevCtx->hQueueAccessMutex);
Exit:
    return dwStatus;
}

/* Remove queue */
DWORD QDMA_RemoveQueue(WDC_DEVICE_HANDLE hDev, DWORD dwQueueId)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    DWORD dwStatus = WD_STATUS_SUCCESS;
    QUEUE_PAIR *queuePair;

    if (dwQueueId >= QDMA_MAX_QUEUES_PER_PF)
    {
        ErrLog("%s: Invalid queue index provided\n", __FUNCTION__);
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    TraceLog("%s: Removing the queue %d\n", __FUNCTION__, dwQueueId);

    OsMutexLock(pDevCtx->hQueueAccessMutex);
    queuePair = &pDevCtx->queuePairs[dwQueueId];

    if (queuePair->state != QUEUE_STATE_QUEUE_PROGRAMMED)
    {
        ErrLog("%s: Queue %d is still active!\n", __FUNCTION__, dwQueueId);
        dwStatus = WD_INVALID_PARAMETER;
        goto ExitWithUnlock;
    }

    QDMA_ClearContexts(hDev, dwQueueId);
    QDMA_FreePollThread(hDev, queuePair->thread, dwQueueId);

    QDMA_QueuePairUnlock(queuePair);
    queuePair->state = QUEUE_STATE_QUEUE_AVAILABLE;

ExitWithUnlock:
    OsMutexUnlock(pDevCtx->hQueueAccessMutex);
Exit:
    return dwStatus;
}

/* Clear all queues */
void QDMA_QueuesClear(WDC_DEVICE_HANDLE hDev)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    QUEUE_PAIR *queuePair;
    DWORD dwQueueId;

    for (dwQueueId = 0; dwQueueId < QDMA_MAX_QUEUES_PER_PF; dwQueueId++)
    {
        queuePair = &pDevCtx->queuePairs[dwQueueId];

        if (queuePair->state == QUEUE_STATE_QUEUE_STARTED)
            QDMA_StopQueue(hDev, dwQueueId);
        if (queuePair->state == QUEUE_STATE_QUEUE_PROGRAMMED)
            QDMA_RemoveQueue(hDev, dwQueueId);
    }
}

/* Get queue queue */
DWORD QDMA_GetQueueState(WDC_DEVICE_HANDLE hDev, DWORD dwQueueId,
    QUEUE_STATE *queueState)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    QUEUE_PAIR *queuePair;

    if (!queueState || !hDev)
    {
        ErrLog("%s: Invalid parameters, device handle and queue state pointer "
            "must be provided\n", __FUNCTION__);
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }
    if (dwQueueId >= QDMA_MAX_QUEUES_PER_PF)
    {
        ErrLog("%s: Invalid queue index provided\n", __FUNCTION__);
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;

    }

    OsMutexLock(pDevCtx->hQueueAccessMutex);
    queuePair = &pDevCtx->queuePairs[dwQueueId];
    *queueState = queuePair->state;
    OsMutexUnlock(pDevCtx->hQueueAccessMutex);

Exit:
    return dwStatus;
}

/* -----------------------------------------------
    DMA request tracker functions
   ----------------------------------------------- */

/* Initialize DMA request tracker */
static DWORD QDMA_DmaRequestTrackerInit(DMA_REQUEST_TRACKER *requestTracker,
    DWORD dwEntries)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;
    requestTracker->dwCapacity = dwEntries;

    requestTracker->requests = (REQ_CTX *)calloc(dwEntries, sizeof(REQ_CTX));
    if (!requestTracker->requests)
    {
        dwStatus = WD_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    requestTracker->u16Pidx = 0;
    requestTracker->u16Cidx = 0;

Exit:
    return dwStatus;
}

/* Enqueue to DMA request tracker */
static DWORD QDMA_DmaRequestTrackerEnqueue(DMA_REQUEST_TRACKER *requestTracker,
    REQ_CTX *req)
{
    if (((requestTracker->u16Pidx + 1) % requestTracker->dwCapacity) ==
        requestTracker->u16Cidx)
    {
        return WD_INSUFFICIENT_RESOURCES;
    }

    requestTracker->requests[requestTracker->u16Pidx].dwNumDescriptors =
        req->dwNumDescriptors;
    requestTracker->requests[requestTracker->u16Pidx].pCtx = req->pCtx;
    requestTracker->u16Pidx =
        (requestTracker->u16Pidx + 1) % requestTracker->dwCapacity;

    return WD_STATUS_SUCCESS;
}

/* Dequeue from DMA request tracker */
static DWORD QDMA_DmaRequestTrackerDequeue(DMA_REQUEST_TRACKER *requestTracker)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;

    if (requestTracker->u16Pidx == requestTracker->u16Cidx)
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    if (NULL == requestTracker->requests)
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    requestTracker->requests[requestTracker->u16Cidx].dwNumDescriptors = 0;
    requestTracker->requests[requestTracker->u16Cidx].pCtx = NULL;
    requestTracker->u16Cidx =
        (requestTracker->u16Cidx + 1) % requestTracker->dwCapacity;

Exit:
    return dwStatus;
}

/* Peek DMA request tracker */
static DWORD QDMA_DmaRequestTrackerPeek(DMA_REQUEST_TRACKER *requestTracker,
    REQ_CTX **req)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;

    if (requestTracker->u16Pidx == requestTracker->u16Cidx)
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    if (!requestTracker->requests || !req)
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    *req = &requestTracker->requests[requestTracker->u16Cidx];

Exit:
    return dwStatus;
}

/* Destroy DMA request tracker */
static void QDMA_DmaRequestTrackerUnlock(DMA_REQUEST_TRACKER *requestTracker)
{
    if (requestTracker->requests)
    {
        free(requestTracker->requests);
        requestTracker->requests = NULL;
        requestTracker->u16Pidx = 0;
        requestTracker->u16Cidx = 0;
    }
}


/* -----------------------------------------------
    Threads functions
   ----------------------------------------------- */

/* Create threads which will later execute all the DMA I/O requests */
static DWORD QDMA_ThreadsCreate(THREAD_MANAGER *thread_manager)
{
    DWORD dwStatus = WD_STATUS_SUCCESS, i;
    DWORD dwNumberOfProcessors = (DWORD)GetNumberOfProcessors();
    DWORD dwNumberOfThreads = MIN(dwNumberOfProcessors, MAX_PROCESSORS);

    for (i = 0; i < dwNumberOfThreads; i++)
    {
        dwStatus = OsEventCreate(&thread_manager->threads[i].hOsEvent);
        if (dwStatus != WD_STATUS_SUCCESS)
        {
            ErrLog("%s: Failed creating event. Error 0x%x - %s\n",
                __FUNCTION__, dwStatus, Stat2Str(dwStatus));
            goto Exit;
        }

        thread_manager->threads[i].dwWeight = QDMA_MAX_QUEUES_PER_PF;
        thread_manager->threads[i].dwIndex = i;
        thread_manager->threads[i].fTerminate = FALSE;
        thread_manager->threads[i].queueListHead = NULL;

        dwStatus = OsMutexCreate(&thread_manager->threads[i].hMutex);
        if (dwStatus != WD_STATUS_SUCCESS)
        {
            ErrLog("%s: Failed to create thread %d. Error 0x%x - %s\n",
                __FUNCTION__, i, dwStatus, Stat2Str(dwStatus));
            goto Exit;
        }

        ThreadStart(&thread_manager->threads[i].hThread, QDMA_ThreadPoll,
            &thread_manager->threads[i]);
    }

    dwStatus = OsMutexCreate(&thread_manager->hMutex);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Failed to create thread manager lock. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto Exit;
    }

    thread_manager->dwActiveThreads = dwNumberOfThreads;
    TraceLog("%s: Active threads %d\n", __FUNCTION__,
        thread_manager->dwActiveThreads);

Exit:
    return dwStatus;
}

/* Associates the thread with the highest weight to the queue id received as
    input */
static QDMA_THREAD *QDMA_ThreadAssociate(WDC_DEVICE_HANDLE hDev,
    DWORD dwQueueId)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    QDMA_THREAD *threads = pDevCtx->threadManager.threads;
    DWORD i, dwMaxWeight, dwMaxWeightId;

    OsMutexLock(pDevCtx->threadManager.hMutex);

    /* Finds the thread with the highest weight */
    dwMaxWeight = pDevCtx->threadManager.threads[0].dwWeight;
    dwMaxWeightId = 0;
    for (i = 1; i < pDevCtx->threadManager.dwActiveThreads; i++)
    {
        if (dwMaxWeight < threads[i].dwWeight)
        {
            dwMaxWeight = threads[i].dwWeight;
            dwMaxWeightId = i;
        }
    }
    threads[dwMaxWeightId].dwWeight--;

    OsMutexUnlock(pDevCtx->threadManager.hMutex);

    /* Push the queue pair to the head of the list */
    OsMutexLock(threads[dwMaxWeightId].hMutex);
    pDevCtx->queuePairs[dwQueueId].next = threads[dwMaxWeightId].queueListHead;
    threads[dwMaxWeightId].queueListHead = &pDevCtx->queuePairs[dwQueueId];
    OsMutexUnlock(threads[dwMaxWeightId].hMutex);

    TraceLog("%s: Allocated thread: %lu\n", __FUNCTION__, dwMaxWeightId);
    return &threads[dwMaxWeightId];
}

/* Terminate all the active threads */
void QDMA_ThreadsTerminate(THREAD_MANAGER *threadManager)
{
    DWORD i;

    for (i = 0; i < threadManager->dwActiveThreads; i++)
    {
        threadManager->threads[i].fTerminate = TRUE;
        OsEventSignal(threadManager->threads[i].hOsEvent);
    }

    // TODO: wait for all the events to be signaled?

    OsMutexClose(threadManager->hMutex);
}

/* Set the thread object to the signaled state */
void QDMA_ThreadNotify(QUEUE_PAIR *queuePair)
{
    OsEventSignal(queuePair->thread->hOsEvent);
}

/* Start thread function */
static void DLLCALLCONV QDMA_ThreadPoll(PVOID context)
{
    QUEUE_PAIR *pCurrentQueueList;
    QDMA_THREAD *thread = (QDMA_THREAD *)context;
    DWORD dwStatus;

    TraceLog("%s: Active thread index: %d\n", __FUNCTION__, thread->dwIndex);

    while (TRUE)
    {
        dwStatus = OsEventWait(thread->hOsEvent, INFINITE);
        if (dwStatus != WD_STATUS_SUCCESS)
        {
            ErrLog("%s: Failed waiting for completion event. "
                "Error 0x%x - %s\n", __FUNCTION__, dwStatus,
                Stat2Str(dwStatus));
            continue;
        }

        if (thread->fTerminate)
        {
            break;
        }

        OsMutexLock(thread->hMutex);
        pCurrentQueueList = thread->queueListHead;

        while (pCurrentQueueList)
        {
            QDMA_QueuePairService(pCurrentQueueList);
            pCurrentQueueList = pCurrentQueueList->next;
        }

        OsMutexUnlock(thread->hMutex);
    }
}

/* Free the node with the provided queue id from a thread's list */
static void QDMA_FreePollThread(WDC_DEVICE_HANDLE hDev, QDMA_THREAD *thread,
    DWORD dwQueueId)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    QUEUE_PAIR *pPrevQueueList, *pCurrentQueueList;

    OsMutexLock(pDevCtx->threadManager.hMutex);
    thread->dwWeight++;
    OsMutexUnlock(pDevCtx->threadManager.hMutex);

    OsMutexLock(thread->hMutex);
    pPrevQueueList = thread->queueListHead;
    pCurrentQueueList = thread->queueListHead;

    if (pCurrentQueueList != NULL && pCurrentQueueList->dwId == dwQueueId)
    {
        thread->queueListHead = pCurrentQueueList->next;
        pCurrentQueueList->next = NULL;
    }
    else
    {
        while (pCurrentQueueList && pCurrentQueueList->dwId != dwQueueId)
        {
            pPrevQueueList = pCurrentQueueList;
            pCurrentQueueList = pCurrentQueueList->next;
        }

        if (pCurrentQueueList != NULL)
        {
            pPrevQueueList->next = pCurrentQueueList->next;
            pCurrentQueueList->next = NULL;
        }
    }

    OsMutexUnlock(thread->hMutex);
}

/* Memory mapped completion callback */
static void QDMA_DrvMmCompletion_cb(WDC_DEVICE_HANDLE hDev, DWORD dwQueueId,
    BOOL fIsH2c, void *pCtx)
{
    DWORD dwStatus;
    DMA_REQUEST_CONTEXT *dmaRequestContext = (DMA_REQUEST_CONTEXT *)pCtx;
    WD_DMA *pDma;
    char *pcDirection = fIsH2c ? "H2C" : "C2H";

    UNUSED_VAR(hDev);
    UNUSED_VAR(dwQueueId);

    if (pCtx == NULL)
    {
        ErrLog("%s: Context is NULL, not possible to proceed\n", __FUNCTION__);
        return;
    }

    pDma = dmaRequestContext->pDma;
    if (pDma == NULL)
    {
        ErrLog("%s: pDma is NULL, not possible to proceed\n", __FUNCTION__);
        return;
    }

    dwStatus = WDC_DMATransferCompletedAndCheck(pDma, TRUE);
    if (dwStatus == WD_STATUS_SUCCESS)
    {
        get_cur_time(&dmaRequestContext->timeEnd);
        dmaRequestContext->status = DMA_REQUEST_STATUS_FINISHED;
        WDC_DMATransactionUninit(pDma);
        TraceLog("%s: %s DMA transaction completed\n", __FUNCTION__,
            pcDirection);
    }
    else if (dwStatus != (DWORD)WD_MORE_PROCESSING_REQUIRED)
    {
        get_cur_time(&dmaRequestContext->timeEnd);
        dmaRequestContext->status = DMA_REQUEST_STATUS_ERROR;
        ErrLog("%s: %s DMA transfer failed\n", __FUNCTION__, pcDirection);
    }
    //TODO: uncomment for debug purposes. might flood the screen
#if 0
    else
    {
        TraceLog("%s: %s DMA transfer completed\n", __FUNCTION__,
            directionString);
    }
#endif
}

/* Execution callback */
static void DLLCALLCONV QDMA_ProgramMmDma_cb(PVOID pData)
{
    DMA_REQUEST_CONTEXT *dmaRequestContext = (DMA_REQUEST_CONTEXT *)pData;
    WD_DMA *pDma = dmaRequestContext->pDma;
    DWORD dwStatus = QDMA_EnqueueMmRequest(dmaRequestContext);

    if (dwStatus != WD_STATUS_SUCCESS)
    {
        dmaRequestContext->status = DMA_REQUEST_STATUS_ERROR;
        ErrLog("%s: QDMA_EnqueueMmRequest() failed. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));

        WDC_DMATransactionUninit(pDma);
        return /*FALSE*/;
    }

    return /*TRUE*/;
}

/* Enqueue DMA request */
static DWORD QDMA_EnqueueMmRequest(DMA_REQUEST_CONTEXT *dmaRequestContext)
{
    QUEUE_PAIR *queuePair;
    DWORD dwStatus;
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(
        dmaRequestContext->hDev);

    if (dmaRequestContext->dwQueueId >= QDMA_MAX_QUEUES_PER_PF)
    {
        ErrLog("%s: Invalid queue index provided\n", __FUNCTION__);
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    TraceLog("%s: MM enqueue the request %d\n", __FUNCTION__,
        dmaRequestContext->dwQueueId);

    OsMutexLock(pDevCtx->hQueueAccessMutex);
    queuePair = &pDevCtx->queuePairs[dmaRequestContext->dwQueueId];
    OsMutexUnlock(pDevCtx->hQueueAccessMutex);

    if (queuePair->state != QUEUE_STATE_QUEUE_STARTED)
    {
        ErrLog("%s: Queue %d is not started!\n", __FUNCTION__,
            dmaRequestContext->dwQueueId);
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    dwStatus = QDMA_QueuePairEnqueueMmRequest(dmaRequestContext, queuePair);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Failed to enqueue the MM request\n", __FUNCTION__);
        goto Exit;
    }

Exit:
    return dwStatus;
}

/* Enable memory mapped channel */
static DWORD QDMA_MmChannelEnable(WDC_DEVICE_HANDLE hDev, DWORD channel,
    BOOL fIsH2C)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    UINT32 reg_addr = (fIsH2C) ? QDMA_OFFSET_H2C_MM_CONTROL :
        QDMA_OFFSET_C2H_MM_CONTROL;
    QDMA_DEVICE_ATTRIBUTES *deviceInfo;
    DWORD dwStatus = WD_STATUS_SUCCESS;

    if (!hDev)
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    deviceInfo = &pDevCtx->deviceConf.deviceInfo;
    if (deviceInfo->fIsMmEnabled)
    {
        QDMA_RegWrite(hDev, reg_addr + (channel * QDMA_MM_CONTROL_STEP),
            QDMA_MM_CONTROL_RUN);
    }

Exit:
    return dwStatus;
}

/* -----------------------------------------------
    Queue pairs functions
   ----------------------------------------------- */

/* Initlaze all queue pairs for the provided physical function handle */
static DWORD QDMA_QueuesPairsInit(WDC_DEVICE_HANDLE hDev)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    DWORD dwQueueId, dwStatus = WD_STATUS_SUCCESS;

    dwStatus = OsMutexCreate(&pDevCtx->hQueueAccessMutex);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Queue access lock creation failed. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto Exit;
    }

    for (dwQueueId = 0; dwQueueId < QDMA_MAX_QUEUES_PER_PF; dwQueueId++)
    {
        QUEUE_PAIR *queuePair = &pDevCtx->queuePairs[dwQueueId];
        queuePair->dwId = dwQueueId;
        queuePair->dwIdAbsolute = dwQueueId +
            (pDevCtx->dwFunctionId * QDMA_MAX_QUEUES_PER_PF);
        queuePair->hDev = hDev;
        queuePair->state = QUEUE_STATE_QUEUE_AVAILABLE;

        /* clear all context fields for this queue */
        dwStatus = QDMA_ClearContexts(hDev, dwQueueId);
        if (dwStatus != WD_STATUS_SUCCESS)
        {
            TraceLog("%s: QDMA_ClearContexts() failed. Error 0x%x - %s\n",
                __FUNCTION__, dwStatus, Stat2Str(dwStatus));
            goto Exit;
        }
    }

Exit:
    return dwStatus;
}

/* Create queue pair */
static DWORD QDMA_QueuePairCreate(WDC_DEVICE_HANDLE hDev, QUEUE_PAIR *queuePair,
    QUEUE_CONFIG *queueConfig)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;

    queuePair->type = QUEUE_TYPE_MEMORY_MAPPED;

    /* h2c ring buffer */
    dwStatus = QDMA_QueueCreate(hDev, TRUE, &queuePair->h2cQueue, queueConfig);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Failed to create H2C ring buffer. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto Exit;
    }

    /* h2c ring buffer */
    dwStatus = QDMA_QueueCreate(hDev, FALSE, &queuePair->c2hQueue, queueConfig);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Failed to create C2H ring buffer. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto Exit;
    }

Exit:
    return dwStatus;
}

/* Service for the provided queue pair */
static void QDMA_QueuePairService(QUEUE_PAIR *queuePair)
{
    QDMA_QueuePairMMService(queuePair, TRUE);  /* H2C */
    QDMA_QueuePairMMService(queuePair, FALSE); /* C2H */
}

/* h2c/h2c memory mapped service for the provided queue pair */
static void QDMA_QueuePairMMService(QUEUE_PAIR *queuePair, BOOL fIsH2C)
{
    RING_BUFFER *descriptorsRing;
    DMA_REQUEST_TRACKER *tracker;
    HANDLE hMutex;
    BOOL fPolling;
    UINT16 u16WbCidx;
    DWORD dwCompletedDescriptors, dwStatus;
    REQ_CTX *req;
    PVOID pCtx;
    char *pcDirection = fIsH2C ? "H2C" : "C2H";

    if (fIsH2C)
    {
        descriptorsRing = &queuePair->h2cQueue.descriptorsRing;
        tracker = &queuePair->h2cQueue.requestTracker;
        hMutex = queuePair->h2cQueue.hMutex;
        fPolling = queuePair->h2cQueue.libConfig.fIrqEnabled ? FALSE : TRUE;
    }
    else
    {
        descriptorsRing = &queuePair->c2hQueue.descriptorsRing;
        tracker = &queuePair->c2hQueue.requestTracker;
        hMutex = queuePair->c2hQueue.hMutex;
        fPolling = queuePair->c2hQueue.libConfig.fIrqEnabled ? FALSE : TRUE;
    }

    OsMutexLock(hMutex);

    u16WbCidx = descriptorsRing->writebackStatus->u16Cidx;
    if (u16WbCidx != descriptorsRing->u32HwIdx)
    {
        TraceLog("%s: %s descriptors ring: u16WbCidx [%u] swidx [%u] hwidx %u\n",
            __FUNCTION__, pcDirection, u16WbCidx, descriptorsRing->u32SwIdx,
            descriptorsRing->u32HwIdx);

        dwCompletedDescriptors = QDMA_IndexDelta(u16WbCidx,
            descriptorsRing->u32HwIdx, descriptorsRing->dwCapacity);
        descriptorsRing->u32Credits += dwCompletedDescriptors;
        descriptorsRing->stats.dwTotalProcessedDescriptors +=
            dwCompletedDescriptors;
        TraceLog("%s: Completed descriptors: %d\n", __FUNCTION__,
            dwCompletedDescriptors);

        while (dwCompletedDescriptors)
        {
            dwStatus = QDMA_DmaRequestTrackerPeek(tracker, &req);
            if (WD_STATUS_SUCCESS == dwStatus)
            {
                TraceLog("%s: Total pending descriptors: %d\n", __FUNCTION__,
                    req->dwNumDescriptors);
                if (req->dwNumDescriptors <= dwCompletedDescriptors)
                {
                    pCtx = req->pCtx;
                    dwCompletedDescriptors -= req->dwNumDescriptors;
                    QDMA_DmaRequestTrackerDequeue(tracker);

                    QDMA_DrvMmCompletion_cb(queuePair->hDev, queuePair->dwId,
                        fIsH2C, pCtx);
                }
                else
                {
                    req->dwNumDescriptors -= dwCompletedDescriptors;
                    break;
                }
            }
            else
            {
                descriptorsRing->stats.dwTotalDropeedDescriptors +=
                    dwCompletedDescriptors;
                TraceLog("%s: No pending request but got interrupt\n",
                    __FUNCTION__);
                break;
            }
        }

        /* Update HW index */
        descriptorsRing->u32HwIdx = u16WbCidx;
    }

    if ((TRUE == fPolling) &&
        (descriptorsRing->u32Credits != (descriptorsRing->dwCapacity - 1)))
    {
        QDMA_ThreadNotify(queuePair);
    }

    OsMutexUnlock(hMutex);
}


/* Enqueue memory mapped request */
static DWORD QDMA_QueuePairEnqueueMmRequest(
    DMA_REQUEST_CONTEXT *dmaRequestContext, QUEUE_PAIR *queuePair)
{
    BOOL fPolling;
    RING_BUFFER *descriptorsRing;
    DMA_REQUEST_TRACKER *tracker;
    HANDLE hMutex;
    WD_DMA *pDma = dmaRequestContext->pDma;
    BOOL fIsH2C = (pDma->dwOptions & DMA_TO_DEVICE) ? 1 : 0;
    UINT64 u64Offset = dmaRequestContext->u64Offset + pDma->dwBytesTransferred;
    REQ_CTX req;
    DWORD i, dwStatus = WD_STATUS_SUCCESS;
    UINT32 u32RingIdx;
    MM_DESCRIPTOR *desc;
    char *pcDirection = fIsH2C ? "H2C" : "C2H";

    if (fIsH2C)
    {
        descriptorsRing = &queuePair->h2cQueue.descriptorsRing;
        tracker = &queuePair->h2cQueue.requestTracker;
        fPolling = !queuePair->h2cQueue.libConfig.fIrqEnabled;
        hMutex = queuePair->h2cQueue.hMutex;
    }
    else
    {
        descriptorsRing = &queuePair->c2hQueue.descriptorsRing;
        tracker = &queuePair->c2hQueue.requestTracker;
        fPolling = !queuePair->c2hQueue.libConfig.fIrqEnabled;
        hMutex = queuePair->c2hQueue.hMutex;
    }

    OsMutexLock(hMutex);

    if (pDma->dwPages > descriptorsRing->u32Credits)
    {
        TraceLog("%s: Not enough space in descriptor buffer for number of "
            "blocks in scatter gather dma list\n", __FUNCTION__);
        dwStatus = WD_INSUFFICIENT_RESOURCES;
        goto ExitWithUnlock;
    }

    u32RingIdx = descriptorsRing->u32SwIdx;

    TraceLog("%s: queue %d mm enqueueing: descriptors [%d] ring index [%d] "
        "Free desc [%u]\n", __FUNCTION__, queuePair->dwIdAbsolute,
        pDma->dwPages, u32RingIdx, descriptorsRing->u32Credits);

    desc = (MM_DESCRIPTOR *)descriptorsRing->pDma->pUserAddr;

    for (i = 0; i < pDma->dwPages; i++)
    {
        desc[u32RingIdx].length = pDma->Page[i].dwBytes;
        desc[u32RingIdx].valid = TRUE;
        desc[u32RingIdx].isStartOfPacket = (i == 0);
        desc[u32RingIdx].isEndOfPacket = (i == (pDma->dwPages - 1));

        if (fIsH2C)
        {
            desc[u32RingIdx].addr = pDma->Page[i].pPhysicalAddr;
            desc[u32RingIdx].u64DestinationAddress = u64Offset;
        }
        else
        {
            desc[u32RingIdx].addr = u64Offset;
            desc[u32RingIdx].u64DestinationAddress = pDma->Page[i].pPhysicalAddr;
        }

        u64Offset += pDma->Page[i].dwBytes;

        QDMA_DumpDescriptorMm(&desc[u32RingIdx]);
        QDMA_RingBufferAdvanceIdx(descriptorsRing, &u32RingIdx, 1UL);
    }

    descriptorsRing->stats.dwTotalAcceptedDescriptors += pDma->dwPages;

    req.dwNumDescriptors = pDma->dwPages;
    req.pCtx = dmaRequestContext;

    dwStatus = QDMA_DmaRequestTrackerEnqueue(tracker, &req);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Failed to track request\n", __FUNCTION__);
        goto ExitWithUnlock;
    }

    OsMemoryBarrier();

    QDMA_MmChannelEnable(queuePair->hDev, 0, fIsH2C);
    descriptorsRing->u32Credits -= pDma->dwPages;
    dmaRequestContext->status = DMA_REQUEST_STATUS_STARTED;
    descriptorsRing->u32SwIdx = u32RingIdx;
    QDMA_CsrUpdatePidx(dmaRequestContext->hDev, fIsH2C, queuePair, u32RingIdx);

    TraceLog("%s: csr[%d] %s descriptor pidx [%u]\n", __FUNCTION__,
        queuePair->dwId, pcDirection, u32RingIdx);

    OsMemoryBarrier();

    if (fPolling)
        QDMA_ThreadNotify(queuePair);

ExitWithUnlock:
    OsMutexUnlock(hMutex);
    QDMA_DumpCsrQueueReg(dmaRequestContext->hDev);

    return dwStatus;
}

/* Set h2c/h2c context */
static DWORD QDMA_SetContext(WDC_DEVICE_HANDLE hDev, QUEUE_PAIR *queuePair,
    BOOL fIsH2C)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;
    DESC_SIZE_T size;
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    BOOL fIsIrqEnabled;
    QUEUE *queue;
    QDMA_DESCQ_SW_CTX sw_ctx = { 0 };

    TraceLog("%s: queue %d setting h2c contexts...\n", __FUNCTION__,
        queuePair->dwIdAbsolute);

    fIsIrqEnabled = (pDevCtx->mode == POLLING_MODE) ? FALSE : TRUE;

    if (fIsH2C)
    {
        queue = &queuePair->h2cQueue;
        size = queuePair->type == QUEUE_TYPE_MEMORY_MAPPED ?
            bytes_32 : /* memory_mapped -> sizeof(mm_descriptor) */
            bytes_16;  /* streaming -> sizeof(h2c_descriptor) */
    }
    else
    {
        queue = &queuePair->c2hQueue;
        size = queuePair->type == QUEUE_TYPE_MEMORY_MAPPED ?
            bytes_32 : /* memory_mapped -> sizeof(mm_descriptor) */
            bytes_8; /* streaming -> sizeof(c2h_descriptor) */
    }

    if (fIsIrqEnabled)
        TraceLog("%s: Programming with IRQ\n", __FUNCTION__);

    sw_ctx.pidx = 0;
    sw_ctx.qEnabled = TRUE;
    sw_ctx.enableFetchCredit = FALSE;
    sw_ctx.wbInterruptAfterPendingCheck = TRUE;
    sw_ctx.wbInterruptInterval = fIsH2C;
    sw_ctx.functionId = pDevCtx->dwFunctionId & 0xff;
    sw_ctx.descriptorRingSizeIdx = fIsH2C ? queue->userConfig.h2cRingSizeIdx :
        queue->userConfig.c2hRingSizeIdx;
    sw_ctx.bypassEnabled = queue->userConfig.descBypassEnabled;
    sw_ctx.wbEnabled = TRUE;
    sw_ctx.fIrqEnabled = fIsIrqEnabled;
    sw_ctx.isMm = (queuePair->type == QUEUE_TYPE_MEMORY_MAPPED) ? 1 : 0;
    sw_ctx.descriptorFetchSize = size;
    sw_ctx.addressTanslation = 0;
    sw_ctx.ringBaseAddress = queue->descriptorsRing.pDma->Page[0].pPhysicalAddr;

    if (queuePair->type == QUEUE_TYPE_STREAMING)
    {
        sw_ctx.enableFetchCredit = TRUE;
    }

    if (fIsIrqEnabled)
    {
        sw_ctx.vector = queuePair->c2hQueue.libConfig.u32VectorId;
        sw_ctx.interruptAggregationEnabled =
            (pDevCtx->mode == INTERRUPT_COALESCE_MODE) ? TRUE : FALSE;
    }

    dwStatus = QDMA_SwContextWrite(hDev, fIsH2C,
        (UINT16)queuePair->dwIdAbsolute,&sw_ctx);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Failed to program %s Software context!\n", __FUNCTION__,
            fIsH2C ? "H2C" : "C2H");
        goto Exit;
    }

Exit:
    return dwStatus;
}

/* Calculate the delta between the short and long indexes */
static DWORD QDMA_IndexDelta(UINT32 u32LongIdx, UINT32 u32ShortIdx,
    DWORD dwCapacity)
{
    if (u32ShortIdx < u32LongIdx)
        return (u32LongIdx - u32ShortIdx);
    else
        return dwCapacity + (u32LongIdx - u32ShortIdx);
}

/* Unlock queue pair */
static void QDMA_QueuePairUnlock(QUEUE_PAIR *queuePair)
{
    QDMA_QueueUnlock(&queuePair->h2cQueue);
    QDMA_QueueUnlock(&queuePair->c2hQueue);
}

/* -----------------------------------------------
    H2C/C2H queues functions
   ----------------------------------------------- */

/* Create queue */
static DWORD QDMA_QueueCreate(WDC_DEVICE_HANDLE hDev, BOOL fIsH2C, QUEUE *queue,
    QUEUE_CONFIG *queueConfig)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    MM_DESCRIPTOR *descriptorsRing;
    PVOID descriptorsRingVa;
    DWORD dwStatus, dwRingBufferSize;
    UINT32 u32RingSizeIdx, u32Size = 0;

    queue->userConfig = *queueConfig;
    u32RingSizeIdx = fIsH2C ? queue->userConfig.h2cRingSizeIdx :
        queue->userConfig.c2hRingSizeIdx;

    dwStatus = QDMA_GetGlobalRingSize(hDev, u32RingSizeIdx, 1, &u32Size);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Invalid global ring index passed\n", __FUNCTION__);
        goto Exit;
    }

    dwRingBufferSize = QDMA_RingBufferInit(&queue->descriptorsRing, u32Size);

    dwStatus = QDMA_RingBufferAllocate(hDev, &queue->descriptorsRing,
        dwRingBufferSize);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: QDMA_RingBufferAllocate() failed. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto Exit;
    }

    descriptorsRingVa = queue->descriptorsRing.pDma->pUserAddr;

    dwStatus = QDMA_DmaRequestTrackerInit(&queue->requestTracker, u32Size);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: QDMA_DmaRequestTrackerInit() failed. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto Exit;
    }

    dwStatus = OsMutexCreate(&pDevCtx->hQueueAccessMutex);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        TraceLog("%s: OsMutexCreate failed: . Error 0x%x - %s\n", __FUNCTION__,
            dwStatus, Stat2Str(dwStatus));
        goto Exit;
    }

    if (!queueConfig->fIsSt)
    {
        descriptorsRing = (MM_DESCRIPTOR *)descriptorsRingVa;
        queue->descriptorsRing.writebackStatus = (volatile WB_STATUS_BASE *)
            (descriptorsRing + queue->descriptorsRing.dwCapacity);
        queue->descriptorsRing.writebackStatus->u16Cidx = 0;
        queue->descriptorsRing.writebackStatus->u16Pidx = 0;
    }

    OsMutexCreate(&queue->hMutex);
    queue->fIsH2C = fIsH2C;

Exit:
    return dwStatus;
}

/* Program queue */
static DWORD QDMA_QueueProgram(WDC_DEVICE_HANDLE hDev, DWORD dwQueueId)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    DWORD dwStatus;

    TraceLog("%s: queue %d programming...\n", __FUNCTION__, dwQueueId);

    dwStatus = QDMA_SetContext(hDev, &pDevCtx->queuePairs[dwQueueId], TRUE);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        goto Exit;
    }

    dwStatus = QDMA_SetContext(hDev, &pDevCtx->queuePairs[dwQueueId], FALSE);

Exit:
    return dwStatus;
}

/* Unlock queue */
static void QDMA_QueueUnlock(QUEUE *queue)
{
    QDMA_RingBufferUnlock(&queue->descriptorsRing);
    OsMutexClose(queue->hMutex);
    QDMA_DmaRequestTrackerUnlock(&queue->requestTracker);
}


/* -----------------------------------------------
    Ring buffer functions
   ----------------------------------------------- */

/* initialize the ring buffer structure and return the required buffer size */
static DWORD QDMA_RingBufferInit(RING_BUFFER *ringBuffer, DWORD dwSize)
{
    DWORD dwBufSize = (DWORD)(dwSize * 32);

    ringBuffer->dwSize = dwSize;
    ringBuffer->dwCapacity = ringBuffer->dwSize - 1;
    ringBuffer->u32HwIdx = ringBuffer->u32SwIdx = 0u;
    ringBuffer->u32Credits = ringBuffer->dwCapacity - 1;
    ringBuffer->stats.dwTotalAcceptedDescriptors = 0;
    ringBuffer->stats.dwTotalDropeedDescriptors = 0;
    ringBuffer->stats.dwTotalProcessedDescriptors = 0;

    return dwBufSize;
}

/* Allocate the ring buffer memory */
static DWORD QDMA_RingBufferAllocate(WDC_DEVICE_HANDLE hDev,
    RING_BUFFER *ringBuffer, DWORD dwRingBufferSize)
{
    PVOID pBuf = NULL;
    DWORD dwStatus;

    TraceLog("%s: Allocating buffer: size [%d] ring depth [%d] capacity [%d]"
        "\n", __FUNCTION__, dwRingBufferSize, ringBuffer->dwSize,
        ringBuffer->dwCapacity);

    dwStatus = WDC_DMAContigBufLock(hDev, &pBuf, DMA_ALLOW_64BIT_ADDRESS,
        dwRingBufferSize, &ringBuffer->pDma);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: WDC_DMAContigBufLock() failed. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto Exit;
    }

    memset(pBuf, 0, dwRingBufferSize);

Exit:
    return dwStatus;
}

/* Unlock ring buffer memory */
static void QDMA_RingBufferUnlock(RING_BUFFER *ringBuffer)
{
    if (ringBuffer->u32Credits != ringBuffer->dwCapacity)
    {
        TraceLog("%s: Few descriptors are already pending: %u\n", __FUNCTION__,
            ringBuffer->u32Credits);
    }

    if (ringBuffer->pDma)
    {
        TraceLog("%s: Deleting buffer object\n", __FUNCTION__);
        WDC_DMABufUnlock(ringBuffer->pDma);
        ringBuffer->pDma = NULL;
    }
}

/* Advance ring buffer index */
static void QDMA_RingBufferAdvanceIdx(RING_BUFFER *ringBuffer, UINT32 *u32Index,
    DWORD dwNum)
{
    *u32Index = (*u32Index + dwNum) % ringBuffer->dwCapacity;
}

/* Lock the registers access lock */
static void QDMA_RegistersAccessLock(WDC_DEVICE_HANDLE hDev)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    OsMutexLock(pDevCtx->hRegisterAccessMutex);
}

/* Unlock the registers access lock */
static void QDMA_RegistersAccessUnlock(WDC_DEVICE_HANDLE hDev)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    OsMutexUnlock(pDevCtx->hRegisterAccessMutex);
}

/* Get the global ring size */
static DWORD QDMA_GetGlobalRingSize(WDC_DEVICE_HANDLE hDev, UINT8 u8Index,
    UINT8 u8Count, UINT32 *pu32GlobalRingSize)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;

    if (!hDev || !pu32GlobalRingSize || !u8Count)
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }
    if ((u8Index + u8Count) > QDMA_NUM_RING_SIZES)
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    QDMA_CsrReadValues(hDev, QDMA_OFFSET_GLBL_RNG_SZ, u8Index, u8Count,
        pu32GlobalRingSize);

Exit:
    return dwStatus;
}

/* -----------------------------------------------
    Read/write/clear registers functions
   ----------------------------------------------- */

/* Initialize control/status registers */
static void QDMA_CsrInit(WDC_DEVICE_HANDLE hDev, QUEUE_PAIR *queuePair)
{
    if (queuePair->type == QUEUE_TYPE_MEMORY_MAPPED)
    {
        QDMA_CsrUpdatePidx(hDev, TRUE, queuePair, 0);
        QDMA_CsrUpdatePidx(hDev, FALSE, queuePair, 0);
    }
}

/* Set defualt values for control/status registers */
static DWORD QDMA_CsrSetDefaultGlobal(WDC_DEVICE_HANDLE hDev)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    UINT32 configValue = 0, registerValue = 0;
    QDMA_DEVICE_ATTRIBUTES *deviceInfo;
    UINT32 rng_sz[QDMA_NUM_RING_SIZES] = { 2049, 65, 129, 193, 257, 385,
        513, 769, 1025, 1537, 3073, 4097, 6145, 8193, 12289, 16385 };
    DWORD dwStatus = WD_STATUS_SUCCESS;

    if (!hDev)
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    deviceInfo = &pDevCtx->deviceConf.deviceInfo;

    /* Configuring CSR registers */
    /* Global ring sizes */
    QDMA_CsrWriteValues(hDev, QDMA_OFFSET_GLBL_RNG_SZ, 0,
        QDMA_NUM_RING_SIZES, rng_sz);

    if (deviceInfo->fIsStreamingEnabled ||
        deviceInfo->fIsMmCompletionsSupported)
    {
        UINT32 tmr_cnt[QDMA_NUM_C2H_TIMERS] = { 1, 2, 4, 5, 8, 10, 15, 20, 25,
            30, 50, 75, 100, 125, 150, 200 };
        UINT32 cnt_th[QDMA_NUM_C2H_COUNTERS] = { 64, 2, 4, 8, 16, 24, 32, 48,
            80, 96, 112, 128, 144, 160, 176, 192 };

        /* Counter thresholds */
        QDMA_CsrWriteValues(hDev, QDMA_OFFSET_C2H_CNT_TH, 0,
            QDMA_NUM_C2H_COUNTERS, cnt_th);

        /* Timer Counters */
        QDMA_CsrWriteValues(hDev, QDMA_OFFSET_C2H_TIMER_CNT, 0,
            QDMA_NUM_C2H_TIMERS, tmr_cnt);

        /* Writeback Interval */
        registerValue =
            FIELD_SET(QDMA_GLBL_DSC_CFG_MAX_DSC_FETCH_MASK,
                DEFAULT_MAX_DSC_FETCH) |
            FIELD_SET(QDMA_GLBL_DSC_CFG_WB_ACC_INT_MASK,
                DEFAULT_WRB_INT);
        QDMA_RegWrite(hDev, QDMA_OFFSET_GLBL_DSC_CFG, registerValue);
    }

    if (deviceInfo->fIsStreamingEnabled)
    {
        UINT32 pu32BufSizes[QDMA_NUM_C2H_BUFFER_SIZES] = { 4096, 256, 512, 1024,
            2048, 3968, 4096, 4096, 4096, 4096, 4096, 4096, 4096, 8192,
            9018, 16384 };

        /* Buffer Sizes */
        QDMA_CsrWriteValues(hDev, QDMA_OFFSET_C2H_BUF_SZ, 0,
            QDMA_NUM_C2H_BUFFER_SIZES, pu32BufSizes);

        /* Prefetch Configuration
         * The Prefetch Engine is responsible for calculating the number of
         * descriptors needed for the DMA that is writing the packet. */
        configValue = QDMA_RegRead(hDev,
            QDMA_OFFSET_C2H_PREFETCH_CACHE_DEPTH);
        registerValue =
            FIELD_SET(QDMA_C2H_PFCH_FL_TH_MASK,
                DEFAULT_PFCH_STOP_THRESH) |
            FIELD_SET(QDMA_C2H_NUM_PFCH_MASK,
                DEFAULT_PFCH_NUM_ENTRIES_PER_Q) |
            FIELD_SET(QDMA_C2H_PFCH_QCNT_MASK, (configValue >> 1)) |
            FIELD_SET(QDMA_C2H_EVT_QCNT_TH_MASK,
                ((configValue >> 1) - 2));
        QDMA_RegWrite(hDev, QDMA_OFFSET_C2H_PREFETCH_CFG, registerValue);

        /* C2H interrupt timer tick */
        QDMA_RegWrite(hDev, QDMA_OFFSET_C2H_INT_TIMER_TICK,
            DEFAULT_C2H_INTR_TIMER_TICK);

        /* C2h Completion Coalesce Configuration
         * The Completion (CMPT) Engine is used to write to the completion
         * queues */
        configValue = QDMA_RegRead(hDev,
            QDMA_OFFSET_C2H_CMPT_COAL_BUF_DEPTH);
        registerValue =
            FIELD_SET(QDMA_C2H_TICK_CNT_MASK,
                DEFAULT_CMPT_COAL_TIMER_CNT) |
            FIELD_SET(QDMA_C2H_TICK_VAL_MASK,
                DEFAULT_CMPT_COAL_TIMER_TICK) |
            FIELD_SET(QDMA_C2H_MAX_BUF_SZ_MASK, configValue);
        QDMA_RegWrite(hDev, QDMA_OFFSET_C2H_WRB_COAL_CFG, registerValue);

        /* H2C throttle Configuration*/
        registerValue =
            FIELD_SET(QDMA_H2C_DATA_THRESH_MASK,
                DEFAULT_H2C_THROT_DATA_THRESH) |
            FIELD_SET(QDMA_H2C_REQ_THROT_EN_DATA_MASK,
                DEFAULT_THROT_EN_DATA) |
            FIELD_SET(QDMA_H2C_REQ_THRESH_MASK,
                DEFAULT_H2C_THROT_REQ_THRESH) |
            FIELD_SET(QDMA_H2C_REQ_THROT_EN_REQ_MASK,
                DEFAULT_THROT_EN_REQ);
        QDMA_RegWrite(hDev, QDMA_OFFSET_H2C_REQ_THROT, registerValue);
    }

Exit:
    return dwStatus;
}

/* Read control/status registers values */
static void QDMA_CsrReadValues(WDC_DEVICE_HANDLE hDev,
    UINT32 u32RegisterOffset, DWORD dwId, DWORD dwCount, UINT32 *values)
{
    DWORD i;
    UINT32 u32RegisterAddress;

    u32RegisterAddress = u32RegisterOffset + (dwId * sizeof(UINT32));
    for (i = 0; i < dwCount; i++)
    {
        values[i] = QDMA_RegRead(hDev, u32RegisterAddress +
            (i * sizeof(UINT32)));
    }
}

/* Write control/status registers values */
static void QDMA_CsrWriteValues(WDC_DEVICE_HANDLE hDev, UINT32 u32RegisterOffset,
    DWORD dwId, DWORD dwCount, const UINT32 *values)
{
    DWORD i;
    UINT32 u32RegisterAddress;

    for (i = dwId; i < (dwId + dwCount); i++)
    {
        u32RegisterAddress = u32RegisterOffset + (i * sizeof(UINT32));
        QDMA_RegWrite(hDev, u32RegisterAddress, values[i - dwId]);
    }
}

/* Update control/status registers producer index */
static void QDMA_CsrUpdatePidx(WDC_DEVICE_HANDLE hDev, BOOL fIsH2C,
    QUEUE_PAIR *queuePair, UINT32 newPidx)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    CSR_DESC_PIDX csr_pidx = { 0 };

    TraceLog("%s: queue %d updating %s pidx to %u\n", __FUNCTION__,
        queuePair->dwIdAbsolute, (fIsH2C ? "h2c" : "h2c"), newPidx);

    csr_pidx.bits.fIrqEnabled = fIsH2C ?
        queuePair->h2cQueue.libConfig.fIrqEnabled :
        queuePair->c2hQueue.libConfig.fIrqEnabled;
    csr_pidx.bits.pidx = newPidx;

    WDC_WriteAddr32(hDev, pDevCtx->dwConfigBarNum,
        (QDMA_TRQ_SEL_QUEUE_PF_BASE + (UINT64)(fIsH2C ? 0x4 : 0x8) +
            ((UINT64)queuePair->dwId * 16)), csr_pidx.u32);
}

/* Write function map to the config bar
 * Function map is used to map a consecutive block of queue(s) to a function */
static DWORD QDMA_FunctionMapWrite(WDC_DEVICE_HANDLE hDev, DWORD dwFuncId,
    const QDMA_FMAP_CONFIG *fmapConfig)
{
    UINT32 pu32Fmap[QDMA_FMAP_NUM_WORDS] = { 0 };
    UINT16 u16NumWordsCount = 0;
    IND_CTX_CMD_SEL sel = QDMA_CTX_SEL_FMAP;
    DWORD dwStatus;

    if (!hDev || !fmapConfig)
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    pu32Fmap[u16NumWordsCount++] =
        FIELD_SET(QDMA_FMAP_CTX_W0_QID_MASK, fmapConfig->qbase);
    pu32Fmap[u16NumWordsCount++] =
        FIELD_SET(QDMA_FMAP_CTX_W1_QID_MAX_MASK, fmapConfig->qmax);

    dwStatus = QDMA_IndirectRegWrite(hDev, sel, dwFuncId, pu32Fmap,
        u16NumWordsCount);

Exit:
    return dwStatus;
}

/* Clear all following contexts:
 * software h2c, software h2c, hardware h2c, hardware h2c,
 * credit h2c and credit h2c.
 * for streaming mode, completion (CMPT) and prefetch engines contexts will be
 * cleared */
static DWORD QDMA_ClearContexts(WDC_DEVICE_HANDLE hDev, DWORD dwQueueId)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    DWORD dwStatus = WD_STATUS_SUCCESS;
    DWORD dwIdAbsolute = pDevCtx->queuePairs[dwQueueId].dwIdAbsolute;

    TraceLog("%s: queue %d clearing all contexts...\n", __FUNCTION__,
        dwIdAbsolute);

    dwStatus = QDMA_ContextClear(hDev, CLEAR_TYPE_SW, FALSE,
        (UINT16)dwIdAbsolute);
    if (dwStatus != WD_STATUS_SUCCESS)
        goto Exit;

    dwStatus = QDMA_ContextClear(hDev, CLEAR_TYPE_SW, TRUE,
        (UINT16)dwIdAbsolute);
    if (dwStatus != WD_STATUS_SUCCESS)
        goto Exit;

    dwStatus = QDMA_ContextClear(hDev, CLEAR_TYPE_HW, FALSE,
        (UINT16)dwIdAbsolute);
    if (dwStatus != WD_STATUS_SUCCESS)
        goto Exit;

    dwStatus = QDMA_ContextClear(hDev, CLEAR_TYPE_HW, TRUE,
        (UINT16)dwIdAbsolute);
    if (dwStatus != WD_STATUS_SUCCESS)
        goto Exit;

    dwStatus = QDMA_ContextClear(hDev, CLEAR_TYPE_CREDIT, FALSE,
        (UINT16)dwIdAbsolute);
    if (dwStatus != WD_STATUS_SUCCESS)
        goto Exit;

    dwStatus = QDMA_ContextClear(hDev, CLEAR_TYPE_CREDIT, TRUE,
        (UINT16)dwIdAbsolute);
    if (dwStatus != WD_STATUS_SUCCESS)
        goto Exit;

Exit:
    return dwStatus;
}

/* Polling a register until (the register value & mask) == value or time is up*/
static DWORD QDMA_HwMonitorReg(WDC_DEVICE_HANDLE hDev,
    UINT32 u32RegisterOffst, UINT32 u32Mask, UINT32 u32Value,
    DWORD dwIntervalUs, DWORD dwTimeoutUs)
{
    DWORD dwCount;
    UINT32 u32CurrentRegValue;

    if (!dwIntervalUs)
        dwIntervalUs = QDMA_REG_POLL_DFLT_INTERVAL_US;
    if (!dwTimeoutUs)
        dwTimeoutUs = QDMA_REG_POLL_DFLT_TIMEOUT_US;

    dwCount = dwTimeoutUs / dwIntervalUs;
    do {
        u32CurrentRegValue = QDMA_RegRead(hDev, u32RegisterOffst);
        if ((u32CurrentRegValue & u32Mask) == u32Value)
            return WD_STATUS_SUCCESS;
        WDC_Sleep(dwIntervalUs, WDC_SLEEP_BUSY);
    } while (--dwCount);

    u32CurrentRegValue = QDMA_RegRead(hDev, u32RegisterOffst);
    if ((u32CurrentRegValue & u32Mask) == u32Value)
        return WD_STATUS_SUCCESS;

    return WD_TIME_OUT_EXPIRED;
}

/* Clear context by the provided clear type */
static DWORD QDMA_ContextClear(WDC_DEVICE_HANDLE hDev, CLEAR_TYPE clearType,
    BOOL fIsH2c, UINT16 u16QueueId)
{
    DWORD dwStatus;
    IND_CTX_CMD_SEL sel;

    if (!hDev)
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    switch (clearType)
    {
        case CLEAR_TYPE_HW:
            sel = fIsH2c ? QDMA_CTX_SEL_HW_H2C : QDMA_CTX_SEL_HW_C2H;
            break;
        case CLEAR_TYPE_SW:
            sel = fIsH2c ? QDMA_CTX_SEL_SW_H2C : QDMA_CTX_SEL_SW_C2H;
            break;
        case CLEAR_TYPE_CREDIT:
            sel = fIsH2c ? QDMA_CTX_SEL_CR_H2C : QDMA_CTX_SEL_CR_C2H;
            break;
        default:
            dwStatus = WD_INVALID_PARAMETER;
            goto Exit;
    }

    dwStatus = QDMA_IndirectRegClear(hDev, sel, u16QueueId);

Exit:
    return dwStatus;
}

/* Write software context */
static DWORD QDMA_SwContextWrite(WDC_DEVICE_HANDLE hDev, BOOL fIsH2c,
    UINT16 u16QueueId, const QDMA_DESCQ_SW_CTX *pSwCtx)
{
    UINT32 sw_ctxt[QDMA_SW_CONTEXT_NUM_WORDS] = { 0 };
    UINT16 numWordsCount = 0;
    IND_CTX_CMD_SEL sel = fIsH2c ? QDMA_CTX_SEL_SW_H2C : QDMA_CTX_SEL_SW_C2H;

    /* Input args check */
    if (!hDev || !pSwCtx)
        return WD_INVALID_PARAMETER;

    sw_ctxt[numWordsCount++] =
        FIELD_SET(QDMA_SW_CTX_W0_PIDX, pSwCtx->pidx) |
        FIELD_SET(QDMA_SW_CTX_W0_IRQ_ARM_MASK, pSwCtx->interruptArm) |
        FIELD_SET(QDMA_SW_CTX_W0_FUNC_ID_MASK, pSwCtx->functionId);

    sw_ctxt[numWordsCount++] =
        FIELD_SET(QDMA_SW_CTX_W1_QEN_MASK, pSwCtx->qEnabled) |
        FIELD_SET(QDMA_SW_CTX_W1_FCRD_EN_MASK, pSwCtx->enableFetchCredit) |
        FIELD_SET(QDMA_SW_CTX_W1_WBI_CHK_MASK, pSwCtx->wbInterruptAfterPendingCheck) |
        FIELD_SET(QDMA_SW_CTX_W1_WB_INT_EN_MASK, pSwCtx->wbInterruptInterval) |
        FIELD_SET(QDMA_SW_CTX_W1_AT_MASK, pSwCtx->addressTanslation) |
        FIELD_SET(QDMA_SW_CTX_W1_FETCH_MAX_MASK, pSwCtx->fetchMax) |
        FIELD_SET(QDMA_SW_CTX_W1_RNG_SZ_MASK, pSwCtx->descriptorRingSizeIdx) |
        FIELD_SET(QDMA_SW_CTX_W1_DSC_SZ_MASK, pSwCtx->descriptorFetchSize) |
        FIELD_SET(QDMA_SW_CTX_W1_BYP_MASK, pSwCtx->bypassEnabled) |
        FIELD_SET(QDMA_SW_CTX_W1_MM_CHN_MASK, pSwCtx->mmChannel) |
        FIELD_SET(QDMA_SW_CTX_W1_WBK_EN_MASK, pSwCtx->wbEnabled) |
        FIELD_SET(QDMA_SW_CTX_W1_IRQ_EN_MASK, pSwCtx->fIrqEnabled) |
        FIELD_SET(QDMA_SW_CTX_W1_PORT_ID_MASK, pSwCtx->portId) |
        FIELD_SET(QDMA_SW_CTX_W1_IRQ_NO_LAST_MASK, pSwCtx->irqNoLast) |
        FIELD_SET(QDMA_SW_CTX_W1_ERR_MASK, pSwCtx->error) |
        FIELD_SET(QDMA_SW_CTX_W1_ERR_WB_SENT_MASK, pSwCtx->errorWbSent) |
        FIELD_SET(QDMA_SW_CTX_W1_IRQ_REQ_MASK, pSwCtx->irqReq) |
        FIELD_SET(QDMA_SW_CTX_W1_MRKR_DIS_MASK, pSwCtx->markerDisabled) |
        FIELD_SET(QDMA_SW_CTX_W1_IS_MM_MASK, pSwCtx->isMm);

    sw_ctxt[numWordsCount++] = pSwCtx->ringBaseAddress & 0xffffffff;
    sw_ctxt[numWordsCount++] = (pSwCtx->ringBaseAddress >> 32) & 0xffffffff;

    sw_ctxt[numWordsCount++] =
        FIELD_SET(QDMA_SW_CTX_W4_VEC_MASK, pSwCtx->vector) |
        FIELD_SET(QDMA_SW_CTX_W4_INTR_AGGR_MASK,
            pSwCtx->interruptAggregationEnabled);

    return QDMA_IndirectRegWrite(hDev, sel, u16QueueId, sw_ctxt,
        numWordsCount);
}

/* Read value from the provided offset in the config bar */
static UINT32 QDMA_RegRead(WDC_DEVICE_HANDLE hDev, UINT32 u32RegisterOffset)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);
    UINT32 u32Value = 0u;

    WDC_ReadAddr32(hDev, pDevCtx->dwConfigBarNum, u32RegisterOffset, &u32Value);

    return u32Value;
}

/* Write value to the provided offset in the config bar */
static void QDMA_RegWrite(WDC_DEVICE_HANDLE hDev, UINT32 u32RegisterOffset,
    UINT32 u32Value)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);

    WDC_WriteAddr32(hDev, pDevCtx->dwConfigBarNum, u32RegisterOffset, u32Value);
}

/* Write indirect value to the config bar */
static DWORD QDMA_IndirectRegWrite(WDC_DEVICE_HANDLE hDev, IND_CTX_CMD_SEL sel,
    DWORD dwHwQueueId, UINT32 *pu32Data, UINT16 u16Count)
{
    DWORD i, dwStatus;
    UINT32 u32RegAddr;
    QDMA_INDIRECT_CTX_REGS regs;
    UINT32 *pu32WriteData = (UINT32 *)&regs;

    QDMA_RegistersAccessLock(hDev);

    /* write the context data */
    for (i = 0; i < QDMA_IND_CTX_DATA_NUM_REGS; i++)
    {
        if (i < u16Count)
            regs.pu32IndirectRegistersCtx[i] = pu32Data[i];
        else
            regs.pu32IndirectRegistersCtx[i] = 0;
        regs.pu32IndirectRegistersMask[i] = 0xFFFFFFFF;
    }

    regs.cmd.word = 0;
    regs.cmd.bits.queueId = dwHwQueueId;
    regs.cmd.bits.op = QDMA_CTX_CMD_WR;
    regs.cmd.bits.sel = sel;
    u32RegAddr = QDMA_OFFSET_IND_CTX_DATA;

    for (i = 0; i < ((2 * QDMA_IND_CTX_DATA_NUM_REGS) + 1);
        i++, u32RegAddr += sizeof(UINT32))
    {
        QDMA_RegWrite(hDev, u32RegAddr, pu32WriteData[i]);
    }

    /* check if the operation went through well */
    dwStatus = QDMA_HwMonitorReg(hDev, QDMA_OFFSET_IND_CTX_CMD,
        QDMA_IND_CTX_CMD_BUSY_MASK, 0,
        QDMA_REG_POLL_DFLT_INTERVAL_US,
        QDMA_REG_POLL_DFLT_TIMEOUT_US);

    QDMA_RegistersAccessUnlock(hDev);

    return dwStatus;
}

/* Clear indirect value in the config bar */
static DWORD QDMA_IndirectRegClear(WDC_DEVICE_HANDLE hDev,
    IND_CTX_CMD_SEL sel, UINT16 u16QueueId)
{
    QDMA_IND_CTXT_CMD cmd;
    DWORD dwStatus;

    QDMA_RegistersAccessLock(hDev);

    /* set command register */
    cmd.word = 0;
    cmd.bits.queueId = u16QueueId;
    cmd.bits.op = QDMA_CTX_CMD_CLR;
    cmd.bits.sel = sel;
    QDMA_RegWrite(hDev, QDMA_OFFSET_IND_CTX_CMD, cmd.word);

    /* check if the operation went through well */
    dwStatus = QDMA_HwMonitorReg(hDev, QDMA_OFFSET_IND_CTX_CMD,
        QDMA_IND_CTX_CMD_BUSY_MASK, 0, QDMA_REG_POLL_DFLT_INTERVAL_US,
        QDMA_REG_POLL_DFLT_TIMEOUT_US);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        goto Exit;
    }

Exit:
    QDMA_RegistersAccessUnlock(hDev);
    return WD_STATUS_SUCCESS;
}

/* -----------------------------------------------
    Utility functions
   ----------------------------------------------- */

/* Get config bar index */
DWORD QDMA_ConfigBarNumGet(WDC_DEVICE_HANDLE hDev)
{
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);

    return pDevCtx ? pDevCtx->dwConfigBarNum :
        (DWORD)WD_STATUS_INVALID_WD_HANDLE;
}

/* Get vendor id by device handle */
DWORD QDMA_GetVendorId(WDC_DEVICE_HANDLE hDev)
{
    return WDC_GET_PPCI_ID(hDev)->dwVendorId;
}

/* Get device id by device handle */
DWORD QDMA_GetDeviceId(WDC_DEVICE_HANDLE hDev)
{
    return WDC_GET_PPCI_ID(hDev)->dwDeviceId;
}

/* Get the required queue queueState that the queue needs to be in order to
 * perform the operation */
QUEUE_STATE QDMA_GetRequiredStateByOperation(QUEUE_OPERATION operation)
{
    QUEUE_STATE state = QUEUE_STATE_QUEUE_AVAILABLE;

    switch (operation)
    {
        case QUEUE_OPERATION_ADD:
            state = QUEUE_STATE_QUEUE_AVAILABLE;
            break;
        case QUEUE_OPERATION_REMOVE:
            state = QUEUE_STATE_QUEUE_PROGRAMMED;
            break;
        case QUEUE_OPERATION_START:
            state = QUEUE_STATE_QUEUE_PROGRAMMED;
            break;
        case QUEUE_OPERATION_STOP:
            state = QUEUE_STATE_QUEUE_STARTED;
            break;
    }

    return state;
}

/* -----------------------------------------------
    Plug-and-play/power management functions
    ----------------------------------------------- */

/* Plug-and-play or power management event handler routine */
static void QDMA_EventHandler(WD_EVENT *pEvent, PVOID pData)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(pDev);

    TraceLog("%s: Entered. pData [0x%p], dwAction [0x%x]\n",
        __FUNCTION__, pData, pEvent->dwAction);

    /* Execute the diagnostics application's event handler function */
    pDevCtx->funcDiagEventHandler((WDC_DEVICE_HANDLE)pDev, pEvent->dwAction);
}

/* Register a plug-and-play or power management event */
DWORD QDMA_EventRegister(WDC_DEVICE_HANDLE hDev,
    QDMA_EVENT_HANDLER funcEventHandler)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PQDMA_DEV_CTX pDevCtx;
    DWORD dwActions = WD_ACTIONS_ALL;
    /* TODO: Modify the above to set up the plug-and-play/power management
             events for which you wish to receive notifications.
             dwActions can be set to any combination of the WD_EVENT_ACTION
             flags defined in windrvr.h. */

    TraceLog("%s: Entered. Device handle [0x%p]\n", __FUNCTION__, hDev);

    /* Validate the WDC device handle */
    if (!IsValidDevice(pDev, "QDMA_EventRegister"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(pDev);

    /* Check whether the event is already registered */
    if (WDC_EventIsRegistered(hDev))
    {
        ErrLog("%s: Events are already registered ...\n", __FUNCTION__);
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Store the diag event handler routine to be executed from
     * QDMA_EventHandler() upon an event */
    pDevCtx->funcDiagEventHandler = funcEventHandler;

    /* Register the event */
    dwStatus = WDC_EventRegister(hDev, dwActions, QDMA_EventHandler, hDev,
        WDC_IS_KP(hDev));
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("%s: Failed to register events. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    TraceLog("%s: Events registered\n", __FUNCTION__);

    return WD_STATUS_SUCCESS;
}

/* Unregister a plug-and-play or power management event */
DWORD QDMA_EventUnregister(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;

    TraceLog("%s: Entered. Device handle [0x%p]\n", __FUNCTION__, hDev);

    /* Validate the WDC device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "QDMA_EventUnregister"))
        return WD_INVALID_PARAMETER;

    /* Check whether the event is currently registered */
    if (!WDC_EventIsRegistered(hDev))
    {
        ErrLog("%s: Cannot unregister events. No events currently registered"
            "...\n", __FUNCTION__);
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Unregister the event */
    dwStatus = WDC_EventUnregister(hDev);

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("%s: Failed to unregister events. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

/* Check whether a given plug-and-play or power management event is registered
 */
BOOL QDMA_EventIsRegistered(WDC_DEVICE_HANDLE hDev)
{
    /* Validate the WDC device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "QDMA_EventIsRegistered"))
        return FALSE;

    /* Check whether the event is registered */
    return WDC_EventIsRegistered(hDev);
}
#endif

/* -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- */

/* Log a debug error message */
void ErrLog(const CHAR *sFormat, ...)
{
    va_list argp;

    va_start(argp, sFormat);
    vsnprintf(gsQDMA_LastErr, sizeof(gsQDMA_LastErr) - 1, sFormat, argp);
#if defined(DEBUG)
#if defined(__KERNEL__)
    WDC_Err("KP QDMA lib: %s", gsQDMA_LastErr);
#else
    WDC_Err("QDMA lib: %s", gsQDMA_LastErr);
#endif
#endif
    va_end(argp);
}

/* Log a debug trace message */
void TraceLog(const CHAR *sFormat, ...)
{
#if defined(DEBUG)
    CHAR sMsg[256];
    va_list argp;

    va_start(argp, sFormat);
    vsnprintf(sMsg, sizeof(sMsg) - 1, sFormat, argp);
#if defined(__KERNEL__)
    WDC_Trace("KP QDMA lib: %s", sMsg);
#else
    WDC_Trace("QDMA lib: %s", sMsg);
#endif
    va_end(argp);
#else
    UNUSED_VAR(sFormat);
#endif
}

/* Get last error */
const char *QDMA_GetLastErr(void)
{
    return gsQDMA_LastErr;
}


/* -----------------------------------------------
    Dump functions
   ----------------------------------------------- */

static void QDMA_DumpDescriptorMm(const MM_DESCRIPTOR *desc)
{
#ifdef DEBUG
    TraceLog("MM descriptor: valid [%u], is start of packet [%u], "
        "is end of packet [%u], length [%u] addr [0x%llX], destination "
        "address [0x%llX]\n", desc->valid, desc->isStartOfPacket,
        desc->isEndOfPacket, desc->length, desc->addr,
        desc->u64DestinationAddress);
#else
    UNUSED_VAR(desc);
#endif
}

#ifdef DEBUG

/* Read indirect value from the config bar */
static DWORD QDMA_IndirectRegRead(WDC_DEVICE_HANDLE hDev, IND_CTX_CMD_SEL sel,
    UINT16 u32HwQueueId, UINT32 u32Count, UINT32 *pu32Data)
{
    DWORD index = 0, dwStatus = WD_STATUS_SUCCESS;
    UINT32 registerAddress = QDMA_OFFSET_IND_CTX_DATA;
    QDMA_IND_CTXT_CMD cmd;

    QDMA_RegistersAccessLock(hDev);

    /* set command register */
    cmd.word = 0;
    cmd.bits.queueId = u32HwQueueId;
    cmd.bits.op = QDMA_CTX_CMD_RD;
    cmd.bits.sel = sel;
    QDMA_RegWrite(hDev, QDMA_OFFSET_IND_CTX_CMD, cmd.word);

    /* check if the operation went through well */
    dwStatus = QDMA_HwMonitorReg(hDev, QDMA_OFFSET_IND_CTX_CMD,
        QDMA_IND_CTX_CMD_BUSY_MASK, 0,
        QDMA_REG_POLL_DFLT_INTERVAL_US,
        QDMA_REG_POLL_DFLT_TIMEOUT_US);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        goto Exit;
    }

    for (index = 0; index < u32Count; index++,
            registerAddress += sizeof(UINT32))
    {
        pu32Data[index] = QDMA_RegRead(hDev, registerAddress);
    }

Exit:
    QDMA_RegistersAccessUnlock(hDev);
    return dwStatus;
}

/* Read software context */
static DWORD QDMA_SwContextRead(WDC_DEVICE_HANDLE hDev, BOOL fIsH2c,
    UINT16 u16QueueId, QDMA_DESCQ_SW_CTX *pSwCtx)
{
    DWORD dwStatus;
    UINT32 sw_ctxt[QDMA_SW_CONTEXT_NUM_WORDS] = { 0 };
    IND_CTX_CMD_SEL sel = fIsH2c ?
        QDMA_CTX_SEL_SW_H2C : QDMA_CTX_SEL_SW_C2H;

    if (!hDev || !pSwCtx)
        return WD_INVALID_PARAMETER;

    dwStatus = QDMA_IndirectRegRead(hDev, sel, u16QueueId,
        QDMA_SW_CONTEXT_NUM_WORDS, sw_ctxt);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        goto Exit;
    }

    pSwCtx->pidx = FIELD_GET(QDMA_SW_CTX_W0_PIDX, sw_ctxt[0]);
    pSwCtx->interruptArm = FIELD_GET(QDMA_SW_CTX_W0_IRQ_ARM_MASK, sw_ctxt[0]);
    pSwCtx->functionId = FIELD_GET(QDMA_SW_CTX_W0_FUNC_ID_MASK, sw_ctxt[0]);

    pSwCtx->qEnabled = FIELD_GET(QDMA_SW_CTX_W1_QEN_MASK, sw_ctxt[1]);
    pSwCtx->enableFetchCredit = FIELD_GET(QDMA_SW_CTX_W1_FCRD_EN_MASK,
        sw_ctxt[1]);
    pSwCtx->wbInterruptAfterPendingCheck =
        FIELD_GET(QDMA_SW_CTX_W1_WBI_CHK_MASK, sw_ctxt[1]);
    pSwCtx->wbInterruptInterval =
        FIELD_GET(QDMA_SW_CTX_W1_WB_INT_EN_MASK, sw_ctxt[1]);
    pSwCtx->addressTanslation = FIELD_GET(QDMA_SW_CTX_W1_AT_MASK, sw_ctxt[1]);
    pSwCtx->fetchMax =
        FIELD_GET(QDMA_SW_CTX_W1_FETCH_MAX_MASK, sw_ctxt[1]);
    pSwCtx->descriptorRingSizeIdx =
        FIELD_GET(QDMA_SW_CTX_W1_RNG_SZ_MASK, sw_ctxt[1]);
    pSwCtx->descriptorFetchSize = FIELD_GET(QDMA_SW_CTX_W1_DSC_SZ_MASK,
        sw_ctxt[1]);
    pSwCtx->bypassEnabled = FIELD_GET(QDMA_SW_CTX_W1_BYP_MASK, sw_ctxt[1]);
    pSwCtx->mmChannel = FIELD_GET(QDMA_SW_CTX_W1_MM_CHN_MASK, sw_ctxt[1]);
    pSwCtx->wbEnabled = FIELD_GET(QDMA_SW_CTX_W1_WBK_EN_MASK, sw_ctxt[1]);
    pSwCtx->fIrqEnabled = FIELD_GET(QDMA_SW_CTX_W1_IRQ_EN_MASK, sw_ctxt[1]);
    pSwCtx->portId = FIELD_GET(QDMA_SW_CTX_W1_PORT_ID_MASK, sw_ctxt[1]);
    pSwCtx->irqNoLast =
        FIELD_GET(QDMA_SW_CTX_W1_IRQ_NO_LAST_MASK, sw_ctxt[1]);
    pSwCtx->error = FIELD_GET(QDMA_SW_CTX_W1_ERR_MASK, sw_ctxt[1]);
    pSwCtx->errorWbSent =
        FIELD_GET(QDMA_SW_CTX_W1_ERR_WB_SENT_MASK, sw_ctxt[1]);
    pSwCtx->irqReq = FIELD_GET(QDMA_SW_CTX_W1_IRQ_REQ_MASK, sw_ctxt[1]);
    pSwCtx->markerDisabled = FIELD_GET(QDMA_SW_CTX_W1_MRKR_DIS_MASK,
        sw_ctxt[1]);
    pSwCtx->isMm = FIELD_GET(QDMA_SW_CTX_W1_IS_MM_MASK, sw_ctxt[1]);

    pSwCtx->ringBaseAddress = ((UINT64)sw_ctxt[3] << 32) | (sw_ctxt[2]);

    pSwCtx->vector = FIELD_GET(QDMA_SW_CTX_W4_VEC_MASK, sw_ctxt[4]);
    pSwCtx->interruptAggregationEnabled =
        FIELD_GET(QDMA_SW_CTX_W4_INTR_AGGR_MASK, sw_ctxt[4]);

Exit:
    return dwStatus;
}

/* Read hardware context */
static DWORD QDMA_HwContextRead(WDC_DEVICE_HANDLE hDev, BOOL fIsH2c,
    UINT16 u16QueueId, QDMA_DESCQ_HW_CTX *pHwCtx)
{
    UINT32 hwCtx[QDMA_HW_CONTEXT_NUM_WORDS] = { 0 };
    IND_CTX_CMD_SEL sel = fIsH2c ? QDMA_CTX_SEL_HW_H2C : QDMA_CTX_SEL_HW_C2H;
    DWORD dwStatus = WD_STATUS_SUCCESS;

    if (!hDev || !pHwCtx)
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    dwStatus = QDMA_IndirectRegRead(hDev, sel, u16QueueId,
        QDMA_HW_CONTEXT_NUM_WORDS, hwCtx);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        goto Exit;
    }

    pHwCtx->cidx = FIELD_GET(QDMA_HW_CTX_W0_CIDX_MASK, hwCtx[0]);
    pHwCtx->creditsConsumed = FIELD_GET(QDMA_HW_CTX_W0_CRD_USE_MASK, hwCtx[0]);

    pHwCtx->descriptorsPending = FIELD_GET(QDMA_HW_CTX_W1_DSC_PND_MASK,
        hwCtx[1]);
    pHwCtx->invalidAndNoDescPending = FIELD_GET(QDMA_HW_CTX_W1_IDL_STP_B_MASK,
        hwCtx[1]);
    pHwCtx->eventPending = FIELD_GET(QDMA_HW_CTX_W1_EVENT_PEND_MASK, hwCtx[1]);
    pHwCtx->descriptorFetchPending =
        FIELD_GET(QDMA_HW_CTX_W1_FETCH_PEND_MASK, hwCtx[1]);

Exit:
    return dwStatus;
}

static void QDMA_DumpCtxDescqHw(const char *dir,
    const QDMA_DESCQ_HW_CTX *pHwCtx)
{
    TraceLog("%s hardware descriptors context: cidx [%u], credits consumed [%u]"
        ", descriptors pending [%u], invalid and no desc pending [%u], event "
        "pending [%u], fetch pending [%u], reserved1 [0x%x]\n", dir,
        pHwCtx->cidx, pHwCtx->creditsConsumed, pHwCtx->descriptorsPending,
        pHwCtx->invalidAndNoDescPending, pHwCtx->eventPending,
        pHwCtx->descriptorFetchPending, pHwCtx->reserved1);
}

static void QDMA_DumpCtxDescqSw(const char *dir,
    const QDMA_DESCQ_SW_CTX *pSwCtx)
{
    TraceLog("%s software descriptors context: pidx [%u], interrupt arm [%u], "
        "function id [%u], qEnabled [%u], enable fetch credit [%u], "
        "write back interrupt after pending check [%u], write back interrupt "
        "interval [%u] address tanslation [%u], fetch max [%u], "
        "descriptor ring size index [%u], descriptor fetch size [%u], "
        "bypass enabled [%u], mm_ch [%u] wb_en [%u] irq enabled [%u], "
        "port id [%u], irq no last [%u], error [%u], err_wb [%u], irq req [%u]",
        dir, pSwCtx->pidx, pSwCtx->interruptArm, pSwCtx->functionId,
        pSwCtx->qEnabled, pSwCtx->enableFetchCredit,
        pSwCtx->wbInterruptAfterPendingCheck, pSwCtx->wbInterruptInterval,
        pSwCtx->addressTanslation, pSwCtx->fetchMax,
        pSwCtx->descriptorRingSizeIdx, pSwCtx->descriptorFetchSize,
        pSwCtx->bypassEnabled, pSwCtx->mmChannel, pSwCtx->wbEnabled,
        pSwCtx->fIrqEnabled, pSwCtx->portId, pSwCtx->irqNoLast, pSwCtx->error,
        pSwCtx->errorWbSent, pSwCtx->irqReq);

    TraceLog(" marker disabled [%u] isMm [%u], ring base address [%llX], "
        "vector [%u], interrupt aggregration [%u]\n", pSwCtx->markerDisabled,
        pSwCtx->isMm, pSwCtx->ringBaseAddress, pSwCtx->vector,
        pSwCtx->interruptAggregationEnabled);
}
#endif

static void QDMA_DumpCsrQueueReg(WDC_DEVICE_HANDLE hDev)
{
#ifdef DEBUG
    union csr_wb_reg wb = { 0 };
    UINT32 c2hDescriptorPidx, h2cDescriptorPidx, interruptCidx;
    PQDMA_DEV_CTX pDevCtx = (PQDMA_DEV_CTX)WDC_GetDevContext(hDev);

    WDC_ReadAddr32(hDev, pDevCtx->dwConfigBarNum,
        QDMA_TRQ_SEL_QUEUE_PF_BASE + 0xC, &wb.u32);
    WDC_ReadAddr32(hDev, pDevCtx->dwConfigBarNum,
        QDMA_TRQ_SEL_QUEUE_PF_BASE + 0x4, &h2cDescriptorPidx);
    WDC_ReadAddr32(hDev, pDevCtx->dwConfigBarNum,
        QDMA_TRQ_SEL_QUEUE_PF_BASE + 0x8, &c2hDescriptorPidx);
    WDC_ReadAddr32(hDev, pDevCtx->dwConfigBarNum,
        QDMA_TRQ_SEL_QUEUE_PF_BASE, &interruptCidx);

    TraceLog("CSR queue register: c2h descriptor pidx [%u], h2c descriptor "
        "pidx [%u], interrupt cidx [%u], writeback cidx [%u], count idx [%u], "
        "timer id [%u], trigger mode [%u], status desc enable[%u], "
        "interrupt enable[%u]\n", c2hDescriptorPidx, h2cDescriptorPidx,
        interruptCidx, wb.bits.writebackCidx, wb.bits.countIdx, wb.bits.timerId,
        wb.bits.triggerMode, wb.bits.statusDescEnable, wb.bits.interruptEnable);
#else
    UNUSED_VAR(hDev);
#endif
}

static void QDMA_DumpGlobalReg(WDC_DEVICE_HANDLE hDev)
{
#ifdef DEBUG
    volatile QDMA_GLOBAL_REG t_glbl;
    volatile QDMA_GLOBAL_REG *glbl = &t_glbl;

    glbl->u32Scratch = QDMA_RegRead(hDev, QDMA_OFFSET_GLBL_SCRATCH);
    glbl->u32GlobalErrorStatus = QDMA_RegRead(hDev, QDMA_OFFSET_GLBL_ERR_STAT);
    glbl->u32GlobalErrorMask = QDMA_RegRead(hDev, QDMA_OFFSET_GLBL_ERR_MASK);
    glbl->u32WritebackAccumulation = QDMA_RegRead(hDev, QDMA_OFFSET_GLBL_DSC_CFG);
    glbl->u32DescErrorStatus = QDMA_RegRead(hDev, QDMA_OFFSET_GLBL_DSC_ERR_STS);

    glbl->u32DescErrorMask = QDMA_RegRead(hDev, QDMA_OFFSET_GLBL_DSC_ERR_MSK);
    glbl->u32DescErrorLog0 = QDMA_RegRead(hDev, QDMA_OFFSET_GLBL_DSC_ERR_LOG0);
    glbl->u32TrqErrorStatus = QDMA_RegRead(hDev, QDMA_OFFSET_GLBL_TRQ_ERR_STS);
    glbl->u32TrqErrorMask = QDMA_RegRead(hDev, QDMA_OFFSET_GLBL_TRQ_ERR_MSK);
    glbl->u32TrqErrorLog = QDMA_RegRead(hDev, QDMA_OFFSET_GLBL_TRQ_ERR_LOG);

    TraceLog("Global Registers: u32Scratch [0x%x], global error status [0x%x], "
        "global error mask [0x%x], writeback accumulation [0x%x], desc error "
        "status [0x%x], desc error mask [0x%x], desc error log0 [0x%x], trq "
        "error status [0x%x], trq error mask [0x%x], trq error log [0x%x]\n",
        glbl->u32Scratch, glbl->u32GlobalErrorStatus, glbl->u32GlobalErrorMask,
        glbl->u32WritebackAccumulation, glbl->u32DescErrorStatus,
        glbl->u32DescErrorMask, glbl->u32DescErrorLog0, glbl->u32TrqErrorStatus,
        glbl->u32TrqErrorMask, glbl->u32TrqErrorLog);
#else
    UNUSED_VAR(hDev);
#endif
}

static void QDMA_DumpContexts(WDC_DEVICE_HANDLE hDev, const DWORD dwQueueId)
{
#ifdef DEBUG
    QDMA_DESCQ_HW_CTX hwCtx = { 0 };
    QDMA_DESCQ_SW_CTX swCtx = { 0 };

    TraceLog("------------ QUEUE %d CONTEXT DUMPS ------------\n", dwQueueId);

    QDMA_SwContextRead(hDev, TRUE, (UINT16)dwQueueId, &swCtx);
    QDMA_DumpCtxDescqSw("H2C", &swCtx);
    QDMA_SwContextRead(hDev, FALSE, (UINT16)dwQueueId, &swCtx);
    QDMA_DumpCtxDescqSw("C2H", &swCtx);

    QDMA_HwContextRead(hDev, TRUE, (UINT16)dwQueueId, &hwCtx);
    QDMA_DumpCtxDescqHw("H2C", &hwCtx);
    QDMA_HwContextRead(hDev, TRUE, (UINT16)dwQueueId, &hwCtx);
    QDMA_DumpCtxDescqHw("C2H", &hwCtx);

    TraceLog("------------  ------------\n");
#else
    UNUSED_VAR(hDev);
    UNUSED_VAR(dwQueueId);
#endif
}



