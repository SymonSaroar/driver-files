/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/****************************************************************
* File: wds_ipc.c - Implementation of WDS IPC API -             *
*       register/unregister, scan processes, send message.      *
*       WD IPC is implemented as a singleton to simplify its    *
*       use.                                                    *
*****************************************************************/

#include "wds_lib.h"
#include "wdc_lib.h"
#include "status_strings.h"
#include "wdc_err.h"

/*************************************************************
  General definitions
 *************************************************************/
#define WDS_Err WDC_Err
#define WDS_Trace WDC_Trace
#define WDS_GetWDHandle WDC_GetWDHandle

typedef struct
{
    WD_IPC_PROCESS      procInfo;

    HANDLE              hEvent;
    IPC_MSG_RX_HANDLER  pFunc; /* User messages handler cb function */
    void               *pData; /* User private ctx */
} IPC_CTX, *PIPC_CTX;

static PIPC_CTX gpIPC = NULL;
static PIPC_CTX gpSharedIntIPC = NULL;

/* -----------------------------------------------
    Register/unregister IPC
   ----------------------------------------------- */
BOOL DLLCALLCONV WDS_IsIpcRegistered(void)
{
    return gpIPC ? TRUE : FALSE;
}

BOOL DLLCALLCONV WDS_IsSharedIntsEnabledLocally(void)
{
    return gpSharedIntIPC ? TRUE : FALSE;
}

static void ipcEventsHandler(WD_EVENT *pEvent, void *pData)
{
    PIPC_CTX pIpc = (PIPC_CTX)pData;
    WDS_IPC_MSG_RX rxMsg;

    if (!pIpc)
    {
        WDS_Err("ipcEventsHandler: Error - NULL IPC ctx\n");
        return;
    }

    WDS_Trace("ipcEventsHandler: We got IPC msgID [0x%lx], msgData [0x%llx] "
        "from process [0x%lx]\n", pEvent->u.Ipc.dwMsgID,
        pEvent->u.Ipc.qwMsgData, pEvent->u.Ipc.dwSenderUID);

    if (!pIpc->pFunc)
    {
        WDS_Trace("ipcEventsHandler: User did not supply messages callback\n");
        return;
    }

    rxMsg.dwSenderUID = pEvent->u.Ipc.dwSenderUID;
    rxMsg.dwMsgID = pEvent->u.Ipc.dwMsgID;
    rxMsg.qwMsgData = pEvent->u.Ipc.qwMsgData;

    /* Calling user IPC callback */
    pIpc->pFunc(&rxMsg, pIpc->pData);
}

static DWORD ipcEventRegister(PIPC_CTX pIpc, DWORD dwAction,
    IPC_MSG_RX_HANDLER pFunc, void *pData)
{
    WD_EVENT Event; /* Event information */
    DWORD dwStatus;

    if (dwAction & ~WD_IPC_ALL_MSG)
    {
        WDS_Err("ipcEventRegister: dwAction has IPC unrelated bits. "
            "dwAction [0x%lx]\n", dwAction);
        return WD_INVALID_PARAMETER;
    }

    BZERO(Event);
    Event.dwEventType = WD_EVENT_TYPE_IPC;
    Event.dwAction = dwAction;
    Event.u.Ipc.hIpc = pIpc->procInfo.hIpc; /* Unique identifier of this
                                             * process */
    Event.u.Ipc.dwSubGroupID = pIpc->procInfo.dwSubGroupID;
    Event.u.Ipc.dwGroupID = pIpc->procInfo.dwGroupID;

    dwStatus = EventRegister(&pIpc->hEvent, WDS_GetWDHandle(), &Event,
        ipcEventsHandler, pIpc);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_Err("ipcEventRegister: Failed registering event. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    pIpc->pFunc = pFunc;
    pIpc->pData = pData;

    WDS_Trace("ipcEventRegister: Registration to IPC messages completed "
        "successfully. dwAction [0x%lx], hIpc [0x%lx], dwSubGroupID [0x%lx], "
        "dwGroupID [0x%lx]\n", Event.dwAction, Event.u.Ipc.hIpc,
        Event.u.Ipc.dwSubGroupID, Event.u.Ipc.dwGroupID);

    return dwStatus;
}

static void ipcEventUnRegister(PIPC_CTX pIpc)
{
    DWORD dwStatus;

    if (!pIpc->hEvent)
        return;

    dwStatus = EventUnregister(pIpc->hEvent);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_Err("ipcEventUnRegister: Unregister events failed. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
    }

    pIpc->hEvent = NULL;
    pIpc->pFunc = NULL;
    pIpc->pData = NULL;
}

static DWORD ipcCtxCreate(PIPC_CTX *ppIPC, const CHAR *pcProcessName,
    DWORD dwGroupID, DWORD dwSubGroupID)
{
    PIPC_CTX pIpc;

    *ppIPC = NULL;

    pIpc = (PIPC_CTX)malloc(sizeof(IPC_CTX));
    if (!pIpc)
    {
        WDS_Err("ipcCtxCreate: Failed memory allocation\n");
        return WD_INSUFFICIENT_RESOURCES;
    }

    BZERO(*pIpc);

    if (pcProcessName)
    {
        strncpy(pIpc->procInfo.cProcessName, pcProcessName,
            sizeof(pIpc->procInfo.cProcessName));
    }

    pIpc->procInfo.dwGroupID = dwGroupID;
    pIpc->procInfo.dwSubGroupID = dwSubGroupID;

    *ppIPC = pIpc;

    return WD_STATUS_SUCCESS;
}

static void ipcCtxDestroy(PIPC_CTX pIpc)
{
    if (!pIpc)
        return;

    free(pIpc);
}

/*
 * const CHAR *pcProcessName - In - Optional process name string
 * DWORD dwGroupID - In - 0 (Zero) not allowed
 * DWORD dwSubGroupID - In - 0 (Zero) not allowed
 * DWORD dwAction - In - IPC messages types to receive- WD_IPC_UNICAST_MSG,
 *                       WD_IPC_MULTICAST_MSG, WD_IPC_ALL_MSG
 * IPC_MSG_RX_HANDLER pFunc - IN - Function to be called back once event happens
 * void* pData - IN - Private data for the event handler
 */
DWORD DLLCALLCONV WDS_IpcRegister(_In_ const CHAR *pcProcessName,
    _In_ DWORD dwGroupID, _In_ DWORD dwSubGroupID, _In_ DWORD dwAction,
    _In_ IPC_MSG_RX_HANDLER pFunc, _In_ void *pData)
{
    DWORD dwStatus, dwErrorStatus;
    PIPC_CTX pIpc = NULL;
    WD_IPC_REGISTER ipcRegister;

    if (gpIPC)
    {
        WDS_Err("WDS_IpcRegister: IPC register was already done\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    if (!dwGroupID || !dwSubGroupID)
    {
        WDS_Err("WDS_IpcRegister: ID Zero (0) is not allowed. "
            "dwGroupID [0x%lx], dwSubGroupID [0x%lx]\n", dwGroupID,
            dwSubGroupID);
        return WD_INVALID_PARAMETER;
    }

    dwStatus = ipcCtxCreate(&pIpc, pcProcessName, dwGroupID, dwSubGroupID);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_Err("WDS_IpcRegister: Failed creating IPC ctx. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    /* Fill IPC register structure */
    memcpy(&ipcRegister.procInfo, &pIpc->procInfo,
        sizeof(ipcRegister.procInfo));
    dwStatus = WD_IpcRegister(WDS_GetWDHandle(), &ipcRegister);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_Err("WDS_IpcRegister: Failed registering process to IPC. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    /* Save IPC handle in context */
    pIpc->procInfo.hIpc = ipcRegister.procInfo.hIpc;

    /* Event register must be done after IPC register since we use hIpc as the
     * process unique identifier */
    dwStatus = ipcEventRegister(pIpc, dwAction, pFunc, pData);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_Err("WDS_IpcRegister: Failed registering IPC event. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    gpIPC = pIpc;
    return WD_STATUS_SUCCESS;

Error:
    if (pIpc->procInfo.hIpc)
    {
        dwErrorStatus = WD_IpcUnRegister(WDS_GetWDHandle(), &pIpc->procInfo);
        if (WD_STATUS_SUCCESS != dwErrorStatus)
        {
            WDS_Err("WDS_IpcRegister: Failed unregistering process. "
                "Error [0x%lx - %s]\n", dwErrorStatus, Stat2Str(dwErrorStatus));
        }
    }
    ipcCtxDestroy(pIpc);
    return dwStatus;
}

void DLLCALLCONV WDS_IpcUnRegister(void)
{
    DWORD dwStatus;

    if (!gpIPC)
    {
        WDS_Err("WDS_IpcUnRegister: Error - IPC was not registered\n");
        return;
    }

    ipcEventUnRegister(gpIPC);

    dwStatus = WD_IpcUnRegister(WDS_GetWDHandle(), &gpIPC->procInfo);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_Err("WDS_IpcUnRegister: Failed unregistering process. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
    }

    ipcCtxDestroy(gpIPC);
    gpIPC = NULL;
}

/* -----------------------------------------------
    Scan processes, Send messages
   ----------------------------------------------- */

/* Description: Scan and return info of all registered processes that share this
 *              process groupID.
 *
 * pIpcScanResult - OUT - Result array of processes registered with the groupID
 */
DWORD DLLCALLCONV WDS_IpcScanProcs(_Outptr_ WDS_IPC_SCAN_RESULT *pIpcScanResult)
{
    WD_IPC_SCAN_PROCS ipcScanProcs;
    DWORD dwStatus, i;

    if (!WdcIsValidPtr(pIpcScanResult,
        "NULL pointer to process scan results struct"))
    {
        return WD_INVALID_PARAMETER;
    }

    if (!gpIPC)
    {
        WDS_Err("WDS_IpcScanProcs: Error - IPC not registered\n");
        return WD_INVALID_PARAMETER;
    }

    BZERO(ipcScanProcs);
    ipcScanProcs.hIpc = gpIPC->procInfo.hIpc;

    dwStatus = WD_IpcScanProcs(WDS_GetWDHandle(), &ipcScanProcs);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_Err("WDS_IpcScanProcs: Failed scanning registered "
            "processes for groupID [0x%lx]. Error [0x%lx - %s]\n",
            gpIPC->procInfo.dwGroupID, dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    BZERO(*pIpcScanResult);
    pIpcScanResult->dwNumProcs = ipcScanProcs.dwNumProcs;

    for (i = 0; i < ipcScanProcs.dwNumProcs; i++)
    {
        memcpy(&pIpcScanResult->procInfo[i], &ipcScanProcs.procInfo[i],
            sizeof(WD_IPC_PROCESS));
    }

    WDS_Trace("WDS_IpcScanProcs: IPC processes scanned successfully. "
        "Found [%ld] matching processes in groupID [0x%lx]\n",
        pIpcScanResult->dwNumProcs, gpIPC->procInfo.dwGroupID);

    return dwStatus;
}

/* WDS_IpcUidUnicast()  - The message will be sent to a single processes by
 *                        the given UID which is actually the hIpc
 *                        returned from WD_IpcRegister().
 * Note: Actually, a processes may register several times to the IPC mechanism
 *       if it uses different WD handle (returned from WD_Open()). The High
 *       level API (WDAPI) does not normally allows this as it always uses the
 *       original handle achieved in WDC_DriverOpen() using WDS_GetWDHandle() */
DWORD DLLCALLCONV WDS_IpcUidUnicast(_In_ DWORD dwRecipientUID,
    _In_ DWORD dwMsgID, _In_ UINT64 qwMsgData)
{
    WD_IPC_SEND ipcSend;
    DWORD dwStatus;

    if (!gpIPC)
    {
        WDS_Err("WDS_IpcUidUnicast: Error - IPC was not registered\n");
        return WD_WINDRIVER_STATUS_ERROR;
    }

    BZERO(ipcSend);
    ipcSend.hIpc = gpIPC->procInfo.hIpc;
    ipcSend.dwOptions = WD_IPC_UID_UNICAST;
    ipcSend.dwRecipientID = dwRecipientUID;
    ipcSend.dwMsgID = dwMsgID;
    ipcSend.qwMsgData = qwMsgData;

    dwStatus = WD_IpcSend(WDS_GetWDHandle(), &ipcSend);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_Err("WDS_IpcUidUnicast: Failed sending message. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    WDS_Trace("WDS_IpcUidUnicast: Message sent successfully\n");

    return dwStatus;
}


/* WDS_IpcSubGroupMulticast() - The message will be sent to all processes that
 *                              registered with this subGroup ID. */
DWORD DLLCALLCONV WDS_IpcSubGroupMulticast(_In_ DWORD dwRecipientSubGroupID,
    _In_ DWORD dwMsgID, _In_ UINT64 qwMsgData)
{
    WD_IPC_SEND ipcSend;
    DWORD dwStatus;

    if (!gpIPC)
    {
        WDS_Err("WDS_IpcSubGroupMulticast: Error - IPC was not registered\n");
        return WD_WINDRIVER_STATUS_ERROR;
    }

    BZERO(ipcSend);
    ipcSend.hIpc = gpIPC->procInfo.hIpc;
    ipcSend.dwOptions = WD_IPC_SUBGROUP_MULTICAST;
    ipcSend.dwRecipientID = dwRecipientSubGroupID;
    ipcSend.dwMsgID = dwMsgID;
    ipcSend.qwMsgData = qwMsgData;

    dwStatus = WD_IpcSend(WDS_GetWDHandle(), &ipcSend);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_Err("WDS_IpcSubGroupMulticast: Failed sending message. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    WDS_Trace("WDS_IpcSubGroupMulticast: Message sent successfully\n");

    return dwStatus;
}

/* WDS_IpcMulticast() - The message will be sent to all processes that
 *                      registered with the same Group ID as the sender. */
DWORD DLLCALLCONV WDS_IpcMulticast(_In_ DWORD dwMsgID, _In_ UINT64 qwMsgData)
{
    WD_IPC_SEND ipcSend;
    DWORD dwStatus;

    if (!gpIPC)
    {
        WDS_Err("WDS_IpcUnicast: Error - IPC was not registered\n");
        return WD_WINDRIVER_STATUS_ERROR;
    }

    BZERO(ipcSend);
    ipcSend.hIpc = gpIPC->procInfo.hIpc;
    ipcSend.dwOptions = WD_IPC_MULTICAST;
    ipcSend.dwMsgID = dwMsgID;
    ipcSend.qwMsgData = qwMsgData;

    dwStatus = WD_IpcSend(WDS_GetWDHandle(), &ipcSend);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_Err("WDS_IpcMulticast: Failed sending message. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    WDS_Trace("WDS_IpcMulticast: Message sent successfully\n");

    return dwStatus;
}

DWORD DLLCALLCONV WDS_SharedIntEnable(_In_ const CHAR *pcProcessName,
    _In_ DWORD dwGroupID, _In_ DWORD dwSubGroupID, _In_ DWORD dwAction,
    _In_ IPC_MSG_RX_HANDLER pFunc, _In_ void *pData)
{
    DWORD dwStatus;
    PIPC_CTX pIpc = NULL;
    WD_IPC_REGISTER ipcRegister;

    if (!gpIPC)
    {
        WDS_Err("%s: This process is not registered to IPC. Please register "
            "first.\n", __FUNCTION__);
        return WD_OPERATION_ALREADY_DONE;
    }

    if (gpSharedIntIPC)
    {
        WDS_Err("%s: Shared interrupts were already enabled in this process\n",
            __FUNCTION__);
        return WD_OPERATION_ALREADY_DONE;
    }

    if (!dwGroupID || !dwSubGroupID)
    {
        WDS_Err("%s: ID Zero (0) is not allowed. dwGroupID [0x%lx], "
            "dwSubGroupID [0x%lx]\n", __FUNCTION__, dwGroupID, dwSubGroupID);
        return WD_INVALID_PARAMETER;
    }

    dwStatus = ipcCtxCreate(&pIpc, pcProcessName, dwGroupID, dwSubGroupID);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_Err("%s: Failed creating IPC ctx. Error [0x%lx - %s]\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    /* Fill IPC register structure */
    memcpy(&ipcRegister.procInfo, &pIpc->procInfo,
        sizeof(ipcRegister.procInfo));

    dwStatus = WD_SharedIntEnable(WDS_GetWDHandle(), &ipcRegister);
    if (WD_OPERATION_ALREADY_DONE == dwStatus)
    {
        WDS_Trace("%s: Shared interrupts already enabled globally, enabling"
            " for this process\n", __FUNCTION__);
    }
    else if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_Err("%s: Failed enabling shared interrupts. Error [0x%lx - %s]\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    /* Save IPC of the current process handle in context
     (NOT the Shared Interrupt IPC handle)*/
    pIpc->procInfo.hIpc = gpIPC->procInfo.hIpc;

    /* Event registration must be done after IPC register since we use hIpc as
     * the process unique identifier */
    dwStatus = ipcEventRegister(pIpc, dwAction, pFunc, pData);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_Err("%s: Failed registering IPC event. Error [0x%lx - %s]\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    gpSharedIntIPC = pIpc;
    return WD_STATUS_SUCCESS;

Error:
    if (pIpc->procInfo.hIpc)
    {
        dwStatus = WD_SharedIntDisable(WDS_GetWDHandle());
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            WDS_Err("%s: Failed disabling shared interrupts. Error [0x%lx - "
                "%s]\n", __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        }
    }
    ipcCtxDestroy(pIpc);
    return dwStatus;
}

DWORD DLLCALLCONV WDS_SharedIntDisableLocal(void)
{
    if (!gpSharedIntIPC)
    {
        WDS_Err("%s: Error - shared interrupts were not enabled on this "
            "process\n", __FUNCTION__);
        return WD_INTERRUPT_NOT_ENABLED;
    }

    ipcEventUnRegister(gpSharedIntIPC);
    ipcCtxDestroy(gpSharedIntIPC);
    gpSharedIntIPC = NULL;

    WDS_Trace("%s: Shared interrupts successfully disabled in this process\n",
        __FUNCTION__);

    return WD_STATUS_SUCCESS;
}

DWORD DLLCALLCONV WDS_SharedIntDisableGlobal(void)
{
    DWORD dwStatus = WD_SharedIntDisable(WDS_GetWDHandle());
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_Err("%s: Failed disabling shared interrupts. Error [0x%lx - %s]\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}
