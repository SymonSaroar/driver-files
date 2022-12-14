/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/*
 * File - wdu_lib.c
 *
 * WinDriver USB API Declarations & Implementations
 */

#if defined(_KERNEL_)
    #include "kpstdlib.h"
#endif
#include "wdu_lib.h"
#include "windrvr_events.h"
#include "status_strings.h"
#include "utils.h"
#include <stdarg.h>
#include <stdio.h>

/* Print Functions */

#if defined(__KERNEL__) && defined(WIN32)
    #pragma warning( disable :4013 4100)
#endif

#if !defined(TRACE)
int __cdecl TRACE(const char *fmt, ...)
{
    #if defined(DEBUG)
        va_list argp;
        va_start(argp, fmt);
        vfprintf(stderr, fmt, argp);
        va_end(argp);
    #endif
    return 0;
}
#endif

#if !defined(ERR)
int __cdecl ERR(const char *fmt, ...)
{
    #if defined(DEBUG)
        va_list argp;
        va_start(argp, fmt);
        fprintf(stderr, "WDERROR: ");
        vfprintf(stderr, fmt, argp);
        va_end(argp);
    #endif
    return 0;
}
#endif

/* Structures */

#define WDU_DEVLIST_TIMEOUT 30 /* In seconds */
#define WDU_STREAM_LIST_TIMEOUT 5 /* In seconds */
#define WDU_TRANSFER_TIMEOUT 30000 /* In msecs */

typedef struct
{
    WDU_DEVICE_HANDLE hDevice;
    HANDLE hWD;
    DWORD dwPipeNum;
} WDU_STREAM_CONTEXT;

typedef struct _WDU_STREAM_LIST_ITEM
{
    struct _WDU_STREAM_LIST_ITEM *next;
    WDU_STREAM_CONTEXT *pStreamCtx;
} WDU_STREAM_LIST_ITEM;

typedef struct
{
    WDU_STREAM_LIST_ITEM *pHead;
    HANDLE hEvent;
    int iRefCount;
} WDU_STREAM_LIST;

typedef struct
{
    HANDLE hWD;
    WDU_EVENT_TABLE EventTable;
    HANDLE hEvents;
} DRIVER_CTX;

typedef struct
{
    DRIVER_CTX *pDriverCtx;
    WDU_DEVICE *pDevice; /* Not fixed size => ptr */
    DWORD dwUniqueID;
    WDU_STREAM_LIST StreamList;
} DEVICE_CTX;

typedef struct _WDU_DEVICE_LIST_ITEM
{
    struct _WDU_DEVICE_LIST_ITEM *next;
    DEVICE_CTX *pDeviceCtx;
} WDU_DEVICE_LIST_ITEM;

typedef struct
{
    WDU_DEVICE_LIST_ITEM *pHead;
    HANDLE hEvent;
    int iRefCount;
} WDU_DEVICE_LIST;

WDU_DEVICE_LIST DevList; /* Global devices list */

/* Private Functions Prototypes */
static DWORD InitStreamList(WDU_STREAM_LIST *pList);
static DWORD AddStreamToList(WDU_STREAM_LIST *pList,
    WDU_STREAM_CONTEXT *pStream);
static DWORD RemoveStreamFromList(WDU_STREAM_LIST *pList,
    WDU_STREAM_CONTEXT *pStream);
static DWORD UninitStreamList(WDU_STREAM_LIST *pList);

static DWORD AddDeviceToDevList(DEVICE_CTX *pDeviceCtx);
static DWORD RemoveDeviceFromDevList(DRIVER_CTX *pDriverCtx, DWORD dwUniqueID,
    DEVICE_CTX **ppDeviceCtx);
static DWORD RemoveAllDevicesFromDevList(DRIVER_CTX *pDriverCtx);
static DWORD FindDeviceByUniqueID(DRIVER_CTX *pDriverCtx, DWORD dwUniqueID,
    DEVICE_CTX **ppDeviceCtx);
static DWORD FindDeviceByCtx(DEVICE_CTX *pDeviceCtx);
static DWORD PutDevice(DEVICE_CTX *pDeviceCtx);

/* Translate WD_functions into IOCTLs */

#define PARAMS_SET(param) Params.param = param
#define GET_HWD(h) (((DEVICE_CTX *)(h))->pDriverCtx->hWD)

/*
 * Unique ID is passed in IOCTLs to identify the device/interface instead of
 * hDevice like in the old API
 */
#define PARAMS_INIT(T) \
    T Params; \
    \
    BZERO(Params); \
    if (!hDevice || \
        FindDeviceByCtx((DEVICE_CTX *)hDevice) != WD_STATUS_SUCCESS) \
    { \
        return WD_DEVICE_NOT_FOUND; \
    } \
    Params.dwUniqueID = ((DEVICE_CTX *)hDevice)->dwUniqueID;

DWORD DLLCALLCONV WDU_SetInterface(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwInterfaceNum, _In_ DWORD dwAlternateSetting)
{
    DWORD dwStatus;
    DWORD ifc_index;
    DEVICE_CTX *pDevCtx = (DEVICE_CTX *)hDevice;
    WDU_DEVICE *pDevice = pDevCtx->pDevice;
    PARAMS_INIT(WDU_SET_INTERFACE);

    PARAMS_SET(dwInterfaceNum);
    PARAMS_SET(dwAlternateSetting);
    dwStatus = WD_USetInterface(GET_HWD(hDevice), &Params);
    if (dwStatus)
    {
        ERR("WDU_SetInterface: Set interface (interface: %ld, alternate "
            "setting: %ld) failed. Error 0x%lx (%s)\n", dwInterfaceNum,
            dwAlternateSetting, dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    /* since WD_USetInterface succeed we assume that dwInterfaceNum and
     * dwAlternateSetting correct and exist */
    for (ifc_index = 0;
        pDevice->pActiveInterface[ifc_index]->pAlternateSettings[0].
        Descriptor.bInterfaceNumber != dwInterfaceNum; ifc_index++);

    /* Update the active alternate setting */
    pDevice->pActiveInterface[ifc_index]->pActiveAltSetting =
        &pDevice->pActiveInterface[ifc_index]->
        pAlternateSettings[dwAlternateSetting];

    return WD_STATUS_SUCCESS;
}

/* Currently not implemented */
DWORD DLLCALLCONV WDU_SetConfig(WDU_DEVICE_HANDLE hDevice, DWORD dwConfigNum);

DWORD DLLCALLCONV WDU_ResetPipe(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwPipeNum)
{
    PARAMS_INIT(WDU_RESET_PIPE);

    PARAMS_SET(dwPipeNum);

    return WD_UResetPipe(GET_HWD(hDevice), &Params);
}

/* Currently not implemented on Linux */
DWORD DLLCALLCONV WDU_ResetDevice(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwOptions)
{
    PARAMS_INIT(WDU_RESET_DEVICE);

    PARAMS_SET(dwOptions);

    return WD_UResetDevice(GET_HWD(hDevice), &Params);
}

/* Currently not implemented on Linux */
DWORD DLLCALLCONV WDU_Wakeup(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwOptions)
{
    PARAMS_INIT(WDU_WAKEUP);

    PARAMS_SET(dwOptions);

    return WD_UWakeup(GET_HWD(hDevice), &Params);
}

DWORD DLLCALLCONV WDU_SelectiveSuspend(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwOptions)
{
    PARAMS_INIT(WDU_SELECTIVE_SUSPEND);

    PARAMS_SET(dwOptions);

    return WD_USelectiveSuspend(GET_HWD(hDevice), &Params);
}

DWORD DLLCALLCONV WDU_Transfer(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwPipeNum, _In_ DWORD fRead, _In_ DWORD dwOptions,
    _In_ PVOID pBuffer, _In_ DWORD dwBufferSize,
    _Outptr_ PDWORD pdwBytesTransferred, _In_ PBYTE pSetupPacket,
    _In_ DWORD dwTimeout)
{
    DWORD dwStatus;
    PARAMS_INIT(WDU_TRANSFER);

    PARAMS_SET(dwPipeNum);
    PARAMS_SET(fRead);
    PARAMS_SET(dwOptions);
    PARAMS_SET(pBuffer);
    PARAMS_SET(dwBufferSize);
    if (pSetupPacket)
        memcpy(&Params.SetupPacket, pSetupPacket, 8);
    PARAMS_SET(dwTimeout);

    dwStatus = WD_UTransfer(GET_HWD(hDevice), &Params);
    if (pdwBytesTransferred)
        *pdwBytesTransferred = Params.dwBytesTransferred;

    return dwStatus;
}

DWORD DLLCALLCONV WDU_HaltTransfer(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwPipeNum)
{
    PARAMS_INIT(WDU_HALT_TRANSFER);

    PARAMS_SET(dwPipeNum);

    return WD_UHaltTransfer(GET_HWD(hDevice), &Params);
}

/* User-mode wrappers for kernel functions */

static void EventHandler(WD_EVENT *pEvent, void *pDriverContext);

DWORD DLLCALLCONV WDU_Init(_Outptr_ WDU_DRIVER_HANDLE *phDriver,
    _In_ WDU_MATCH_TABLE *pMatchTables, _In_ DWORD dwNumMatchTables,
    _In_ WDU_EVENT_TABLE *pEventTable, _In_ const char *pcLicense,
    _In_ DWORD dwOptions)
{
    DWORD dwStatus;
    DRIVER_CTX *pDriverCtx;
    WD_VERSION ver;
    WD_EVENT *event = NULL;
    WD_LICENSE lic;

    *phDriver = INVALID_HANDLE_VALUE;

    pDriverCtx = (DRIVER_CTX *)calloc(1, sizeof(DRIVER_CTX));
    if (!pDriverCtx)
    {
        ERR("WDU_Init: Failed allocating memory for driver ctx\n");
        dwStatus = WD_INSUFFICIENT_RESOURCES;
        goto Error;
    }

#if !defined(__KERNEL__)
    /* Init the device list event */
    if (DevList.iRefCount == 0)
    {
        dwStatus = OsEventCreate(&DevList.hEvent);
        if (dwStatus)
        {
            ERR("WDU_Init: Failed creating event. dwStatus (0x%lx) - %s\n",
                dwStatus, Stat2Str(dwStatus));
            goto Error;
        }

        dwStatus = OsEventSignal(DevList.hEvent);
        if (dwStatus)
        {
            ERR("WDU_Init: Error - Signaling device list event. "
                "dwStatus (0x%lx) - %s\n", dwStatus, Stat2Str(dwStatus));
            OsEventClose(DevList.hEvent);
            goto Error;
        }
    }
    DevList.iRefCount++;
#endif

    /* Check whether handle is valid and version OK */
    pDriverCtx->hWD = WD_Open();
    if (pDriverCtx->hWD == INVALID_HANDLE_VALUE)
    {
        ERR("WDU_Init: Failed opening " WD_PROD_NAME " device\n");
        dwStatus = WD_SYSTEM_INTERNAL_ERROR;
        goto Error;
    }

    strcpy(lic.cLicense, pcLicense);
    WD_License(pDriverCtx->hWD, &lic);

    PrintDbgMessage(D_ERROR, S_USB, "WDU_Init: User mode version - %s\n",
        WD_VER_STR);

    BZERO(ver);
    dwStatus = WD_Version(pDriverCtx->hWD, &ver);
    if (WD_STATUS_SUCCESS != dwStatus || ver.dwVer < WD_VER)
    {
        ERR("WDU_Init: Error - Incorrect " WD_PROD_NAME " version\n");
        if (WD_STATUS_SUCCESS == dwStatus)
            dwStatus = WD_INCORRECT_VERSION;
        goto Error;
    }

    pDriverCtx->EventTable = *pEventTable;

    if (pEventTable->pfDeviceAttach)
    {
        DWORD dwAction;

        dwAction = WD_INSERT |
            (pEventTable->pfDeviceDetach ? WD_REMOVE : 0) |
            (pEventTable->pfPowerChange ? WD_POWER_CHANGED_D0 |
                                          WD_POWER_CHANGED_D1 |
                                          WD_POWER_CHANGED_D2 |
                                          WD_POWER_CHANGED_D3 |
                                          WD_POWER_SYSTEM_WORKING |
                                          WD_POWER_SYSTEM_SLEEPING1 |
                                          WD_POWER_SYSTEM_SLEEPING2 |
                                          WD_POWER_SYSTEM_SLEEPING3 |
                                          WD_POWER_SYSTEM_HIBERNATE |
                                          WD_POWER_SYSTEM_SHUTDOWN : 0);
        event = UsbEventCreate(pMatchTables, dwNumMatchTables, dwOptions,
            dwAction);
        if (!event)
        {
            ERR("WDU_Init: Failed allocating memory for event\n");
            dwStatus = WD_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        dwStatus = EventRegister(&pDriverCtx->hEvents, pDriverCtx->hWD, event,
            EventHandler, pDriverCtx);
        if (dwStatus)
        {
            ERR("WDU_Init: EventRegister failed with dwStatus (0x%lx) - %s\n",
                dwStatus, Stat2Str(dwStatus));
            goto Error;
        }
    }

    *phDriver = pDriverCtx;
    goto Exit;

Error:
    if (pDriverCtx)
        WDU_Uninit(pDriverCtx);
Exit:
    if (event)
        EventFree(event);
    return dwStatus;
}

void DLLCALLCONV WDU_Uninit(_In_ WDU_DRIVER_HANDLE hDriver)
{
    DRIVER_CTX *pDriverCtx = (DRIVER_CTX *)hDriver;

    if (pDriverCtx && hDriver != INVALID_HANDLE_VALUE)
    {
        if (pDriverCtx->hWD)
        {
            if (pDriverCtx->hEvents)
                EventUnregister(pDriverCtx->hEvents);
            WD_Close(pDriverCtx->hWD);
        }
        RemoveAllDevicesFromDevList(pDriverCtx);
        free(pDriverCtx);
    }

#if !defined(__KERNEL__)
    /* This ensures that iRefCount doesn't become negative,
     * something that may cause a deadlock under Linux */
    if (DevList.iRefCount > 0)
    {
        DevList.iRefCount--;
        if (DevList.iRefCount == 0)
            OsEventClose(DevList.hEvent);
    }
#endif
}

static void EventHandler(WD_EVENT *pEvent, void *pDriverContext)
{
    DRIVER_CTX *pDriverCtx = (DRIVER_CTX *)pDriverContext;
    DEVICE_CTX *pDeviceCtx, *pDummyDeviceCtx;
    WDU_DEVICE_HANDLE hDevice;
    BOOL bControlDevice = FALSE;
    DWORD dwStatus;

    TRACE("EventHandler: Got event. Action 0x%lx, handle %p\n",
        pEvent->dwAction, pEvent->hEvent);

    switch (pEvent->dwAction)
    {
    case WD_INSERT:
        /* Create device context */
        pDeviceCtx = (DEVICE_CTX *)calloc(1, sizeof(DEVICE_CTX));
        if (!pDeviceCtx)
        {
            ERR("EventHandler: Failed allocating memory\n");
            return;
        }

        /* Init streams list */
        dwStatus = InitStreamList(&pDeviceCtx->StreamList);
        if (dwStatus)
        {
            PutDevice(pDeviceCtx);
            return;
        }

        /* DEVICE_CTX * is used as WDU_DEVICE_HANDLE */
        hDevice = pDeviceCtx;

        pDeviceCtx->dwUniqueID = pEvent->u.Usb.dwUniqueID;
        pDeviceCtx->pDriverCtx = pDriverCtx;

        /* Add the device handle to the device list for future IOCTLs */
        dwStatus = AddDeviceToDevList(pDeviceCtx);
        if (dwStatus)
        {
            PutDevice(pDeviceCtx);
            return;
        }

        /* Get device info */
        dwStatus = WDU_GetDeviceInfo(hDevice, &pDeviceCtx->pDevice);
        if (dwStatus)
        {
            ERR("EventHandler: Failed getting device info. device handle %p, "
                "dwUniqueID 0x%lx\n", hDevice, pDeviceCtx->dwUniqueID);
            RemoveDeviceFromDevList(pDriverCtx, pEvent->u.Usb.dwUniqueID,
                &pDummyDeviceCtx);
            PutDevice(pDeviceCtx);
            return;
        }

        bControlDevice = pDriverCtx->EventTable.pfDeviceAttach(hDevice,
            pDeviceCtx->pDevice, pDriverCtx->EventTable.pUserData);
        if (!bControlDevice)
        {
            TRACE("EventHandler: bControlDevice==FALSE; pDriverCtx %p\n",
                pDriverCtx);
            RemoveDeviceFromDevList(pDriverCtx, pEvent->u.Usb.dwUniqueID,
                &pDummyDeviceCtx);
            PutDevice(pDeviceCtx);
        }

        pEvent->dwOptions |= bControlDevice ? WD_ACCEPT_CONTROL : 0;
        break;

    case WD_REMOVE:
        dwStatus = RemoveDeviceFromDevList(pDriverCtx, pEvent->u.Usb.dwUniqueID,
            &pDeviceCtx);
        if (dwStatus)
            /* Device is not mine or may has been closed by WDU_Uninit */
            break;

        /* DEVICE_CTX * is used as WDU_DEVICE_HANDLE */
        pDriverCtx->EventTable.pfDeviceDetach((WDU_DEVICE_HANDLE)pDeviceCtx,
            pDriverCtx->EventTable.pUserData);

        dwStatus = PutDevice(pDeviceCtx);
        if (dwStatus)
        {
            ERR("EventHandler: Failed removing device. dwUniqueID 0x%lx\n",
                pDeviceCtx->dwUniqueID);
        }
        break;

    case WD_POWER_CHANGED_D0:
    case WD_POWER_CHANGED_D1:
    case WD_POWER_CHANGED_D2:
    case WD_POWER_CHANGED_D3:
    case WD_POWER_SYSTEM_WORKING:
    case WD_POWER_SYSTEM_SLEEPING1:
    case WD_POWER_SYSTEM_SLEEPING2:
    case WD_POWER_SYSTEM_SLEEPING3:
    case WD_POWER_SYSTEM_HIBERNATE:
    case WD_POWER_SYSTEM_SHUTDOWN:
        dwStatus = FindDeviceByUniqueID(pDriverCtx, pEvent->u.Usb.dwUniqueID,
            &pDeviceCtx);
        if (dwStatus)
        {
            /* Device is not mine or may have been closed by WDU_Uninit */
            break;
        }

        pDriverCtx->EventTable.pfPowerChange((WDU_DEVICE_HANDLE)pDeviceCtx,
            pEvent->dwAction, pDriverCtx->EventTable.pUserData);
        /*
         * XXX return in the event structure if the user says it's ok to
         * change power (not implemented yet)
         */
        break;
    }
}

/*
 * ppDeviceInfo is set to point to an allocated buffer containing the info.
 * The caller should free the buffer after the use by calling
 * WDU_PutDeviceInfo()
 */
DWORD DLLCALLCONV WDU_GetDeviceInfo(_In_ WDU_DEVICE_HANDLE hDevice,
    _Outptr_ WDU_DEVICE **ppDeviceInfo)
{
    DWORD dwStatus;
    PARAMS_INIT(WDU_GET_DEVICE_DATA);

    /* First call with pBuf NULL, return dwBytes */
    dwStatus = WD_UGetDeviceData(GET_HWD(hDevice), &Params);
    if (dwStatus != WD_STATUS_SUCCESS)
        return dwStatus;

    *ppDeviceInfo = (WDU_DEVICE *)malloc(Params.dwBytes);
    if (!ppDeviceInfo)
    {
        ERR("WDU_GetDeviceInfo: Failed allocating memory\n");
        return WD_INSUFFICIENT_RESOURCES;
    }

    Params.pBuf = *ppDeviceInfo;
    /* Second call with correct pBuf and dwBytes */
    dwStatus = WD_UGetDeviceData(GET_HWD(hDevice), &Params);
    if (dwStatus)
    {
        WDU_PutDeviceInfo(*ppDeviceInfo);
        *ppDeviceInfo = NULL;
    }

    return dwStatus;
}

/* Frees device info allocated with WDU_GetDeviceInfo() */
void DLLCALLCONV WDU_PutDeviceInfo(_In_ WDU_DEVICE *pDeviceInfo)
{
    if (pDeviceInfo)
        free(pDeviceInfo);
}

DWORD DLLCALLCONV WDU_GetDeviceAddr(_In_ WDU_DEVICE_HANDLE hDevice,
    _Out_ DWORD *pAddress)
{
    DWORD size = sizeof(*pAddress);

    return WDU_GetDeviceRegistryProperty(hDevice, pAddress, &size,
        WdDevicePropertyAddress);
}

DWORD DLLCALLCONV WDU_GetDeviceRegistryProperty(_In_ WDU_DEVICE_HANDLE hDevice,
    _Outptr_ PVOID pBuffer, _Inout_ PDWORD pdwSize,
    _In_ WD_DEVICE_REGISTRY_PROPERTY property)
{
    DWORD dwStatus;
    WD_GET_DEVICE_PROPERTY Params;

    BZERO(Params);

    if (!hDevice || FindDeviceByCtx((DEVICE_CTX *)hDevice) != WD_STATUS_SUCCESS)
        return WD_DEVICE_NOT_FOUND;

    Params.h.dwUniqueID = ((DEVICE_CTX *)hDevice)->dwUniqueID;
    Params.pBuf = pBuffer;
    Params.dwProperty = property;
    Params.dwOptions = WD_DEVICE_USB;
    Params.dwBytes = *pdwSize;
    dwStatus = WD_GetDeviceProperty(GET_HWD(hDevice), &Params);

    *pdwSize = Params.dwBytes;
    return dwStatus;
}

/*
 * Simplified transfers
 */

DWORD DLLCALLCONV WDU_TransferDefaultPipe(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD fRead, _In_ DWORD dwOptions, _In_ PVOID pBuffer,
    _In_ DWORD dwBufferSize, _Outptr_ PDWORD pdwBytesTransferred,
    _In_ PBYTE pSetupPacket, _In_ DWORD dwTimeout)
{
    return WDU_Transfer(hDevice, 0, fRead, dwOptions, pBuffer, dwBufferSize,
        pdwBytesTransferred, pSetupPacket, dwTimeout);
}

DWORD DLLCALLCONV WDU_TransferBulk(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwPipeNum, _In_ DWORD fRead, _In_ DWORD dwOptions,
    _In_ PVOID pBuffer, _In_ DWORD dwBufferSize,
    _Outptr_ PDWORD pdwBytesTransferred, _In_ DWORD dwTimeout)
{
    return WDU_Transfer(hDevice, dwPipeNum, fRead, dwOptions, pBuffer,
        dwBufferSize, pdwBytesTransferred, NULL, dwTimeout);
}

DWORD DLLCALLCONV WDU_TransferIsoch(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwPipeNum, _In_ DWORD fRead, _In_ DWORD dwOptions,
    _In_ PVOID pBuffer, _In_ DWORD dwBufferSize,
    _Outptr_ PDWORD pdwBytesTransferred, _In_ DWORD dwTimeout)
{
    return WDU_Transfer(hDevice, dwPipeNum, fRead, dwOptions, pBuffer,
        dwBufferSize, pdwBytesTransferred, NULL, dwTimeout);
}

DWORD DLLCALLCONV WDU_TransferInterrupt(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwPipeNum, _In_ DWORD fRead, _In_ DWORD dwOptions,
    _In_ PVOID pBuffer, _In_ DWORD dwBufferSize,
    _Outptr_ PDWORD pdwBytesTransferred, _In_ DWORD dwTimeout)
{
    return WDU_Transfer(hDevice, dwPipeNum, fRead, dwOptions, pBuffer,
        dwBufferSize, pdwBytesTransferred, NULL, dwTimeout);
}

/* Private Functions */

static DWORD InitStreamList(WDU_STREAM_LIST *pList)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;

    if (pList->hEvent)
        return dwStatus;

    TRACE("InitStreamList: Entered\n");

    dwStatus = OsEventCreate(&pList->hEvent);
    if (dwStatus)
    {
        ERR("InitStreamList: Failed creating event. dwStatus (0x%lx) - %s\n",
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    dwStatus = OsEventSignal(pList->hEvent);
    if (dwStatus)
    {
        ERR("InitStreamList: Error - signaling stream list event. "
            "dwStatus (0x%lx) - %s\n", dwStatus, Stat2Str(dwStatus));
        OsEventClose(pList->hEvent);
        return dwStatus;
    }

    return dwStatus;
}

static DWORD AddStreamToList(WDU_STREAM_LIST *pList,
    WDU_STREAM_CONTEXT *pItemCtx)
{
    WDU_STREAM_LIST_ITEM *pItem;
    DWORD dwStatus;

    TRACE("AddStreamToList: item %p, before %p\n", pItemCtx, pList);

    dwStatus = OsEventWait(pList->hEvent, WDU_STREAM_LIST_TIMEOUT);
    if (dwStatus)
    {
        ERR("AddStreamToList: Error - Waiting for list event. "
            "dwStatus (0x%lx) - %s\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    pItem = (WDU_STREAM_LIST_ITEM *)calloc(1, sizeof(WDU_STREAM_LIST_ITEM));
    if (!pItem)
        return WD_INSUFFICIENT_RESOURCES;

    pList->iRefCount++;

    pItem->pStreamCtx = pItemCtx;
    pItem->next = pList->pHead;
    pList->pHead = pItem;

    dwStatus = OsEventSignal(pList->hEvent);
    if (dwStatus)
    {
        ERR("AddStreamToList: Error - Signaling list event. "
            "dwStatus (0x%lx) - %s\n", dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

static DWORD RemoveStreamFromList(WDU_STREAM_LIST *pList,
    WDU_STREAM_CONTEXT *pItemCtx)
{
    WDU_STREAM_LIST_ITEM **iter, *tmp;
    DWORD dwStatus;
    BOOL Found = FALSE;

    TRACE("RemoveStreamFromList: List %p, pItemCtx 0x%lx\n", pList, pItemCtx);

    dwStatus = OsEventWait(pList->hEvent, WDU_STREAM_LIST_TIMEOUT);
    if (dwStatus)
    {
        ERR("RemoveStreamFromList: Error - Waiting for list event. "
            "dwStatus (0x%lx) - %s\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    for (iter = &pList->pHead; *iter; iter = &(*iter)->next)
    {
        if ((*iter)->pStreamCtx == pItemCtx)
        {
            Found = TRUE;
            break;
        }
    }

    if (Found)
    {
        TRACE("RemoveStreamFromList: Remove stream\n", pList, pItemCtx);
        tmp = *iter;

        /* Remove from list */
        *iter = (*iter)->next;
        free(tmp);
    }

    dwStatus = OsEventSignal(pList->hEvent);
    if (dwStatus)
    {
        ERR("RemoveStreamFromList: Error - Signaling list event. "
            "dwStatus (0x%lx) - %s\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    pList->iRefCount--;

    if (!Found)
        dwStatus = WD_OPERATION_FAILED;
    return dwStatus;
}

static DWORD UninitStreamList(WDU_STREAM_LIST *pList)
{
    WDU_STREAM_CONTEXT *pStreamCtx;
    WDU_STREAM_LIST_ITEM **ppItem = &pList->pHead;
    WDU_STREAM_LIST_ITEM *pItemTmp;
    DWORD dwStatus;

    TRACE("UninitStreamList: Entered\n");

    if (!pList->hEvent)
    {
        /* List's event doesn't exist but there are items in list - an error. */
        if (pList->iRefCount != 0)
            return WD_SYSTEM_INTERNAL_ERROR;

        return WD_STATUS_SUCCESS;
    }

    dwStatus = OsEventWait(pList->hEvent, WDU_STREAM_LIST_TIMEOUT);
    if (dwStatus)
    {
        ERR("UninitStreamList: Error - Waiting for device list event. "
            "dwStatus (0x%lx) - %s\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    while (*ppItem)
    {
        pStreamCtx = (*ppItem)->pStreamCtx;
        TRACE("UninitStreamList: Removing %p\n", pStreamCtx);

        pItemTmp = *ppItem;
        *ppItem = (*ppItem)->next;

        WD_StreamClose(pStreamCtx->hWD);
        free(pStreamCtx);
        free(pItemTmp);
    }

    dwStatus = OsEventSignal(pList->hEvent);
    if (dwStatus)
    {
        ERR("UninitStreamList: Error - Signaling list event. "
            "dwStatus (0x%lx) - %s\n", dwStatus, Stat2Str(dwStatus));
    }

    OsEventClose(pList->hEvent);

    return dwStatus;
}

static DWORD AddDeviceToDevList(DEVICE_CTX *pDeviceCtx)
{
    WDU_DEVICE_LIST_ITEM *pDevItem;
    DWORD dwStatus;

    TRACE("AddDeviceToDevList: device %p, before %p\n", pDeviceCtx, DevList);

    dwStatus = OsEventWait(DevList.hEvent, WDU_DEVLIST_TIMEOUT);
    if (dwStatus)
    {
        ERR("AddDeviceToDevList: Error - Waiting for device list event. "
            "dwStatus (0x%lx) - %s\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    pDevItem = (WDU_DEVICE_LIST_ITEM *)calloc(1, sizeof(WDU_DEVICE_LIST_ITEM));
    if (!pDevItem)
        return WD_INSUFFICIENT_RESOURCES;

    pDevItem->pDeviceCtx = pDeviceCtx;
    pDevItem->next = DevList.pHead;

    DevList.pHead = pDevItem;

    dwStatus = OsEventSignal(DevList.hEvent);
    if (dwStatus)
    {
        ERR("AddDeviceToDevList: Error - Signaling device list event. "
            "dwStatus (0x%lx) - %s\n", dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

static DWORD RemoveAllDevicesFromDevList(DRIVER_CTX *pDriverCtx)
{
#if defined(__KERNEL__)
    return WD_NOT_IMPLEMENTED;
#else
    DEVICE_CTX *pDeviceCtx;
    WDU_DEVICE_LIST_ITEM **ppDev = &DevList.pHead;
    WDU_DEVICE_LIST_ITEM *pDevTmp;
    DWORD dwStatus;

    TRACE("RemoveAllDevicesFromDevList: pDriverCtx %p\n", pDriverCtx);

    dwStatus = OsEventWait(DevList.hEvent, WDU_DEVLIST_TIMEOUT);
    if (dwStatus)
    {
        ERR("RemoveAllDevicesFromDevList: Error - Waiting for device list "
            "event. dwStatus (0x%lx) - %s\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    while (*ppDev)
    {
        pDeviceCtx = (*ppDev)->pDeviceCtx;
        if (pDeviceCtx->pDriverCtx == pDriverCtx)
        {
            pDevTmp = *ppDev;
            *ppDev = (*ppDev)->next;

            dwStatus = PutDevice(pDeviceCtx);
            if (dwStatus)
            {
                ERR("RemoveAllDevicesFromDevList: Failed releasing device. "
                    "dwUniqueID 0x%lx\n", pDeviceCtx->dwUniqueID);
                continue;
            }

            free(pDevTmp);
        }
        else
        {
            ppDev = &(*ppDev)->next;
        }
    }

    dwStatus = OsEventSignal(DevList.hEvent);
    if (dwStatus)
    {
        ERR("RemoveAllDevicesFromDevList: Error - Signaling device list event. "
            "dwStatus (0x%lx) - %s\n", dwStatus, Stat2Str(dwStatus));
    }
    return dwStatus;
#endif
}

static DWORD FindDeviceByUniqueID(DRIVER_CTX *pDriverCtx, DWORD dwUniqueID,
    DEVICE_CTX **ppDeviceCtx)
{
    WDU_DEVICE_LIST_ITEM *iter;
    DWORD dwStatus;
    BOOL Found = FALSE;

    TRACE("FindDeviceByUniqueID: DevList.pHead %p, dwUniqueID 0x%lx\n",
        DevList.pHead, dwUniqueID);

    *ppDeviceCtx = NULL;

    dwStatus = OsEventWait(DevList.hEvent, WDU_DEVLIST_TIMEOUT);
    if (dwStatus)
    {
        ERR("FindDeviceByUniqueID: Error - Waiting for device list event. "
            "dwStatus (0x%lx) - %s\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    for (iter = DevList.pHead; iter; iter = iter->next)
    {
        if (iter->pDeviceCtx->dwUniqueID == dwUniqueID &&
            iter->pDeviceCtx->pDriverCtx == pDriverCtx)
        {
            Found = TRUE;
            break;
        }
    }

    dwStatus = OsEventSignal(DevList.hEvent);
    if (dwStatus)
    {
        ERR("FindDeviceByUniqueID: Error - Signaling device list event. "
            "dwStatus (0x%lx) - %s\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    if (Found)
        *ppDeviceCtx = iter->pDeviceCtx;
    else
        dwStatus = WD_DEVICE_NOT_FOUND;

    return dwStatus;
}

static DWORD FindDeviceByCtx(DEVICE_CTX *pDeviceCtx)
{
    WDU_DEVICE_LIST_ITEM *iter;
    DWORD dwStatus;
    BOOL Found = FALSE;

    dwStatus = OsEventWait(DevList.hEvent, WDU_DEVLIST_TIMEOUT);
    if (dwStatus)
    {
        ERR("FindDeviceByCtx: Error - Waiting for device list event. "
            "dwStatus (0x%lx) - %s\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    for (iter = DevList.pHead; iter; iter = iter->next)
    {
        if (iter->pDeviceCtx == pDeviceCtx)
        {
            Found = TRUE;
            break;
        }
    }

    dwStatus = OsEventSignal(DevList.hEvent);
    if (dwStatus)
    {
        ERR("FindDeviceByCtx: Error - Signaling device list event. "
            "dwStatus (0x%lx) - %s\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    if (!Found)
        dwStatus = WD_DEVICE_NOT_FOUND;

    return dwStatus;
}

static DWORD RemoveDeviceFromDevList(DRIVER_CTX *pDriverCtx, DWORD dwUniqueID,
    DEVICE_CTX **ppDeviceCtx)
{
    WDU_DEVICE_LIST_ITEM **iter, *tmp;
    DWORD dwStatus;
    BOOL Found = FALSE;

    TRACE("RemoveDeviceFromDevList: DevList %p, dwUniqueID 0x%lx\n", DevList,
        dwUniqueID);

    *ppDeviceCtx = NULL;

    dwStatus = OsEventWait(DevList.hEvent, WDU_DEVLIST_TIMEOUT);
    if (dwStatus)
    {
        ERR("RemoveDeviceFromDevList: Error - Waiting for device list event. "
            "dwStatus (0x%lx) - %s\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    for (iter = &DevList.pHead; *iter; iter = &(*iter)->next)
    {
        if ((*iter)->pDeviceCtx->dwUniqueID == dwUniqueID &&
            (*iter)->pDeviceCtx->pDriverCtx == pDriverCtx)
        {
            Found = TRUE;
            break;
        }
    }

    if (Found)
    {
        tmp = *iter;
        *ppDeviceCtx = (*iter)->pDeviceCtx;

        /* Remove the device */
        *iter = (*iter)->next;
        free(tmp);
    }

    dwStatus = OsEventSignal(DevList.hEvent);
    if (dwStatus)
    {
        ERR("RemoveDeviceFromDevList: Error - Signaling device list event. "
            "dwStatus (0x%lx) - %s\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    if (!Found)
        dwStatus = WD_DEVICE_NOT_FOUND;
    return dwStatus;
}

static DWORD PutDevice(DEVICE_CTX *pDeviceCtx)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;

    dwStatus = UninitStreamList(&pDeviceCtx->StreamList);
    if (dwStatus)
        return dwStatus;

    WDU_PutDeviceInfo(pDeviceCtx->pDevice);
    free(pDeviceCtx);

    return dwStatus;
}

DWORD DLLCALLCONV WDU_GetLangIDs(_In_ WDU_DEVICE_HANDLE hDevice,
    _Outptr_ PBYTE pbNumSupportedLangIDs, _Outptr_ WDU_LANGID *pLangIDs,
    _In_ BYTE bNumLangIDs)
{
    DWORD dwStatus;
    DWORD dwBytesTransferred;
    BYTE bTmpNumIDs, bNumReadIDs;
    BYTE bSizeTrans; /* size in bytes */
    WDU_LANGID *pBuf = NULL;
    BYTE  setupPacket[] = {0x80, 0x6, 0x0, 0x3, 0x0, 0x0, sizeof(bSizeTrans),
        0x0};

    if (!pbNumSupportedLangIDs && !bNumLangIDs)
        return WD_INVALID_PARAMETER;

    /* Read number of supported language IDs from the device */
    dwStatus = WDU_TransferDefaultPipe(hDevice, TRUE, 0, &bSizeTrans,
        sizeof(bSizeTrans), &dwBytesTransferred, setupPacket,
        WDU_TRANSFER_TIMEOUT);
    if (dwStatus)
    {
        ERR("WDU_GetLangIDs: Failed reading number of supported language IDs "
            "from the device. Error 0x%lx (\"%s\")\n", dwStatus,
            Stat2Str(dwStatus));
        return dwStatus;
    }

    /* First 2 bytes hold length and desc type, respectively */
    bTmpNumIDs = (BYTE)((bSizeTrans - 2) / sizeof(WDU_LANGID));

    if (pbNumSupportedLangIDs)
        *pbNumSupportedLangIDs = bTmpNumIDs;

    if (!bTmpNumIDs)
    {
        TRACE("WDU_GetLangIDs: No language IDs are supported for the device\n");
        return WD_STATUS_SUCCESS;
    }

    /* Check if function was called only in order to get the number of
     * supported language IDs */
    if (!bNumLangIDs)
        return WD_STATUS_SUCCESS;

    if (!pLangIDs)
    {
        ERR("WDU_GetLangIDs: Error - Received NULL language IDs array\n");
        return WD_INVALID_PARAMETER;
    }

    /* Determine number of language IDs to read */
    if (bNumLangIDs < bTmpNumIDs)
    {
        TRACE("WDU_GetLangIDs: Size of language IDs array (%d) is smaller than "
            "the number of supported language IDs (%d) - Returning only the "
            "first %d supported language IDs\n", (int)bNumLangIDs,
            (int)bTmpNumIDs, (int)bNumLangIDs);
        bTmpNumIDs = bNumLangIDs;
        bSizeTrans = (BYTE)((bTmpNumIDs + 1) * sizeof(WDU_LANGID));
    }

    pBuf = (WDU_LANGID *)malloc(bSizeTrans);
    if (!pBuf)
    {
        ERR("WDU_GetLangIDs: Failed allocating memory for language IDs\n");
        return WD_INSUFFICIENT_RESOURCES;
    }

    /* Update wLength for expected transfer size */
    setupPacket[6] = (BYTE)(bSizeTrans & 0xFF);
    setupPacket[7] = (BYTE)((bSizeTrans & 0xFF00) >> 8);

    /* Read supported language IDs from the device */
    dwStatus = WDU_TransferDefaultPipe(hDevice, TRUE, 0, pBuf, bSizeTrans,
        &dwBytesTransferred, setupPacket, WDU_TRANSFER_TIMEOUT);
    if (dwStatus)
    {
        ERR("WDU_GetLangIDs: Failed getting supported language IDs from the "
            "device. Error 0x%lx (\"%s\")\n", dwStatus, Stat2Str(dwStatus));
        goto Exit;
    }

    /* First WORD (LANGID) holds length and desc type */
    bNumReadIDs = (BYTE)(dwBytesTransferred / sizeof(WDU_LANGID) - 1);
    if (bNumReadIDs != bTmpNumIDs)
    {
        ERR("WDU_GetLangIDs: Error - Expected to read %d language IDs, "
            "read %d\n", (int)bTmpNumIDs, (int)bNumReadIDs);
        dwStatus = WD_SYSTEM_INTERNAL_ERROR;
        goto Exit;
    }

    /* First WORD (LANGID) holds length and desc type */
    memcpy(pLangIDs, pBuf + 1, bSizeTrans - sizeof(WDU_LANGID));

    dwStatus = WD_STATUS_SUCCESS;

Exit:
    free(pBuf);
    return dwStatus;
}

#define GET_STR_DESC_LEN 4096  /* max buffer size */
DWORD DLLCALLCONV WDU_GetStringDesc(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ BYTE bStrIndex, _Outptr_ PBYTE pbBuf, _In_ DWORD dwBufSize,
    _In_ WDU_LANGID langID, _Outptr_ PDWORD pdwDescSize)
{
    DWORD dwStatus, dwBytesTransferred;
    BYTE buf[GET_STR_DESC_LEN];
    BYTE setupPacket[] = { 0x80, 0x6, 0x0, 0x3, 0x0, 0x0,
        (BYTE)(GET_STR_DESC_LEN & 0xFF),
        (BYTE)((GET_STR_DESC_LEN & 0xFF00) >> 8) };
    DWORD dwDescSize;

    if (!pbBuf)
    {
        ERR("WDU_GetStringDesc: Error - pbBuf is NULL\n");
        return WD_INVALID_PARAMETER;
    }

    if (!bStrIndex)
    {
        ERR("WDU_GetStringDesc: Error - Invalid string index (0)\n");
        return WD_INVALID_PARAMETER;
    }

    /* Set string index in the setup packet request */
    setupPacket[2] = bStrIndex;

    if (!langID)
    {
        /* Get first language ID in supported language IDs list */
        dwStatus = WDU_GetLangIDs(hDevice, NULL, &langID, 1);
        if (dwStatus)
            return dwStatus;

        if (!langID)
        {
            TRACE("WDU_GetStringDesc: No language IDs are supported for this "
                "device\n");
            return WD_OPERATION_FAILED;
        }
    }

    /* wIndex = selected language ID */
    setupPacket[4] = (BYTE)(langID & 0xFF);
    setupPacket[5] = (BYTE)((langID & 0xFF00) >> 8);

    BZERO(buf);
    dwStatus = WDU_TransferDefaultPipe(hDevice, TRUE, 0, &buf, GET_STR_DESC_LEN,
        &dwBytesTransferred, setupPacket, WDU_TRANSFER_TIMEOUT);
    if (dwStatus)
    {
        ERR("WDU_GetStringDesc: Failed reading string descriptor from the "
            "device. Error 0x%lx (\"%s\")\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    if (dwBytesTransferred > 2)
    {
        /* The descriptor string begins at the third byte */
        dwDescSize = dwBytesTransferred - 2;

        if (dwDescSize && pdwDescSize)
            *pdwDescSize = dwDescSize;

        /*
         * Return descriptor data to user.
         * If dwBufSize < dwDescSize the returned buffer will be truncated to
         * dwBufSize
         */
        memcpy(pbBuf, buf + 2, MIN(dwDescSize, dwBufSize));
    }
    else if (pdwDescSize)
    {
        *pdwDescSize = 0;
    }

    return WD_STATUS_SUCCESS;
}

DWORD DLLCALLCONV WDU_StreamOpen(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwPipeNum, _In_ DWORD dwBufferSize, _In_ DWORD dwRxSize,
    _In_ BOOL fBlocking, _In_ DWORD dwOptions, _In_ DWORD dwRxTxTimeout,
    _Outptr_ WDU_STREAM_HANDLE *phStream)
{
    DEVICE_CTX *pDevCtx = (DEVICE_CTX *)hDevice;
    WDU_STREAM_CONTEXT *pStream;
    HANDLE hWD;
    DWORD dwStatus;
    BOOL fRead = WDU_ENDPOINT_DIRECTION_IN(dwPipeNum);
    DWORD dwReserved = 0;
    PARAMS_INIT(WDU_STREAM);

    PARAMS_SET(dwPipeNum);
    PARAMS_SET(dwBufferSize);
    PARAMS_SET(dwRxSize);
    PARAMS_SET(fBlocking);
    PARAMS_SET(dwOptions);
    PARAMS_SET(dwRxTxTimeout);
    PARAMS_SET(dwReserved);

    *phStream = NULL;

    pStream = (WDU_STREAM_CONTEXT *)malloc(sizeof(*pStream));
    if (!pStream)
    {
        ERR("WDU_StreamOpen: Failed allocating memory\n");
        return WD_INSUFFICIENT_RESOURCES;
    }

    /* Note: always open in sync mode */
    hWD = WD_StreamOpen(fRead, TRUE);
    if (hWD == INVALID_HANDLE_VALUE)
    {
        ERR("WDU_StreamOpen: Failed opening stream for " WD_PROD_NAME
            " device\n");
        dwStatus = WD_SYSTEM_INTERNAL_ERROR;
        goto Error;
    }

    dwStatus = WD_UStreamOpen(hWD, &Params);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ERR("WDU_StreamOpen: Failed opening stream. Error %lx (%s)\n", dwStatus,
            Stat2Str(dwStatus));
        goto Error;
    }

    AddStreamToList(&pDevCtx->StreamList, pStream);

    pStream->hDevice = hDevice;
    pStream->hWD = hWD;
    pStream->dwPipeNum = dwPipeNum;

    *phStream = pStream;
    return WD_STATUS_SUCCESS;

Error:
    if (hWD != INVALID_HANDLE_VALUE)
        WD_StreamClose(hWD);
    free(pStream);
    return dwStatus;
}

DWORD DLLCALLCONV WDU_StreamClose(_In_ WDU_STREAM_HANDLE hStream)
{
    WDU_STREAM_CONTEXT *pStream = (WDU_STREAM_CONTEXT *)hStream;
    WDU_DEVICE_HANDLE hDevice = pStream ? pStream->hDevice : NULL;
    DEVICE_CTX *pDevCtx = (DEVICE_CTX *)hDevice;
    DWORD dwStatus;
    PARAMS_INIT(WDU_STREAM);

    RemoveStreamFromList(&pDevCtx->StreamList, pStream);

    dwStatus = WD_UStreamClose(pStream->hWD, &Params);
    if (dwStatus != WD_STATUS_SUCCESS)
        return dwStatus;

    WD_StreamClose(pStream->hWD);
    free(pStream);
    return WD_STATUS_SUCCESS;
}

DWORD DLLCALLCONV WDU_StreamFlush(_In_ WDU_STREAM_HANDLE hStream)
{
    WDU_STREAM_CONTEXT *pStream = (WDU_STREAM_CONTEXT *)hStream;
    WDU_DEVICE_HANDLE hDevice = pStream ? pStream->hDevice : NULL;
    PARAMS_INIT(WDU_STREAM);

    return WD_UStreamFlush(((WDU_STREAM_CONTEXT *)hStream)->hWD, &Params);
}

DWORD DLLCALLCONV WDU_StreamRead(_In_ HANDLE hStream, _Outptr_ PVOID pBuffer,
    _In_ DWORD bytes, _Outptr_ DWORD *pdwBytesRead)
{
    WDU_STREAM_CONTEXT *pStream = (WDU_STREAM_CONTEXT *)hStream;

    if (!pStream)
        return WD_INVALID_PARAMETER;

    return WD_UStreamRead(pStream->hWD, pBuffer, bytes, pdwBytesRead);
}

DWORD DLLCALLCONV WDU_StreamWrite(_In_ HANDLE hStream, _In_ const PVOID pBuffer,
    _In_ DWORD bytes, _Outptr_ DWORD *pdwBytesWritten)
{
    WDU_STREAM_CONTEXT *pStream = (WDU_STREAM_CONTEXT *)hStream;

    if (!pStream)
        return WD_INVALID_PARAMETER;

    return WD_UStreamWrite(pStream->hWD, pBuffer, bytes, pdwBytesWritten);
}

DWORD DLLCALLCONV WDU_StreamStart(_In_ WDU_STREAM_HANDLE hStream)
{
    WDU_STREAM_CONTEXT *pStream = (WDU_STREAM_CONTEXT *)hStream;
    WDU_DEVICE_HANDLE hDevice = pStream ? pStream->hDevice : NULL;
    PARAMS_INIT(WDU_STREAM);

    return WD_UStreamStart(pStream->hWD, &Params);
}

DWORD DLLCALLCONV WDU_StreamStop(_In_ WDU_STREAM_HANDLE hStream)
{
    WDU_STREAM_CONTEXT *pStream = (WDU_STREAM_CONTEXT *)hStream;
    WDU_DEVICE_HANDLE hDevice = pStream ? pStream->hDevice : NULL;
    PARAMS_INIT(WDU_STREAM);

    return WD_UStreamStop(pStream->hWD, &Params);
}

DWORD DLLCALLCONV WDU_StreamGetStatus(_In_ WDU_STREAM_HANDLE hStream,
    _Outptr_ BOOL *pfIsRunning, _Outptr_ DWORD *pdwLastError,
    _Outptr_ DWORD *pdwBytesInBuffer)
{
    DWORD dwStatus;
    WDU_STREAM_CONTEXT *pStream = (WDU_STREAM_CONTEXT *)hStream;
    WDU_DEVICE_HANDLE hDevice = pStream ? pStream->hDevice : NULL;
    PARAMS_INIT(WDU_STREAM_STATUS);

    dwStatus = WD_UStreamGetStatus(pStream->hWD, &Params);
    if (pfIsRunning)
        *pfIsRunning = Params.fIsRunning;
    if (pdwLastError)
        *pdwLastError = Params.dwLastError;
    if (pdwBytesInBuffer)
        *pdwBytesInBuffer = Params.dwBytesInBuffer;

    return dwStatus;
}

