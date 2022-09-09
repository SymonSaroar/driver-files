/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/******************************************************************************
*  File: wds_diag_lib.c - Implementation of shared WDS all devices' user-mode *
*        diagnostics API.                                                     *
*                                                                             *
*  Note: This code sample is provided AS-IS and as a guiding sample only.     *
*******************************************************************************/

#if !defined(__KERNEL__)

#include <stdio.h>
#include "wds_diag_lib.h"
#include "status_strings.h"
#include "wdc_defs.h"
#include "wds_lib.h"

/*************************************************************
  General definitions
 *************************************************************/
/* Error messages display */
#define WDS_DIAG_ERR printf

#define DEFAULT_PROCESS_NAME "Diagnostic program"

/* Unique identifier of the processes group to avoid getting messages from
 * processes made under WinDriver by other developers that use the same driver
 * name.
 * WinDriver developers are encouraged to change their driver name before
 * distribution to avoid this issue entirely. */
#define DEFAULT_PROCESS_GROUP_ID 0x12345678

/* Identifiers for shared interrupt IPC process */
#define DEFAULT_SHARED_INT_NAME "WinDriver IPC Shared Interrupt"

#define SAMPLE_BUFFER_DATA "This is a sample buffer data"

/*************************************************************
  Global variables
 *************************************************************/

/* User's input command */
static CHAR gsInput[256];

/* User's shared kernel buffer handle */
static WD_KERNEL_BUFFER *pSharedKerBuf = NULL; /* Static global pointer is used
                                                * only for sample simplicity */

/* -----------------------------------------------
    Shared Buffer
   ----------------------------------------------- */

/* Shared Buffer menu options */
enum {
    MENU_SB_ALLOC_CONTIG = 1,
    MENU_SB_ALLOC_NON_CONTIG,
    MENU_SB_FREE,
    MENU_SB_EXIT = DIAG_EXIT_MENU,
};

static void WDS_DIAG_SharedBufferFree(WD_KERNEL_BUFFER **ppKerBuf)
{
    DWORD dwStatus;

    if (!(*ppKerBuf))
        return;

    dwStatus = WDS_SharedBufferFree(*ppKerBuf);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_DIAG_ERR("WDS_DIAG_SharedBufferFree: Failed freeing shared buffer "
            "memory. Error [0x%x - %s]\n", dwStatus, Stat2Str(dwStatus));
    }

    *ppKerBuf = NULL;

    printf("Shared buffer memory freed\n");
}

static DWORD AllocKernelBuff(WD_KERNEL_BUFFER **ppKerBuf, DWORD dwOptions)
{
    DWORD size, dwStatus;

    sprintf(gsInput, "Enter memory allocation size in bytes "
        "(32 bit uint)");
    size = 0;
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&size, gsInput,
        FALSE, 1, 0xFFFFFFFF))
    {
        return WD_INVALID_PARAMETER;
    }

    /* Free shared buffer memory before trying the new allocation */
    WDS_DIAG_SharedBufferFree(ppKerBuf);

    dwStatus = WDS_SharedBufferAlloc(size, dwOptions, ppKerBuf);
    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Shared buffer allocated. User addr [0x%" UPRI "x], "
            "kernel addr [0x%" KPRI "x], size [%u(0x%x)]\n",
            (*ppKerBuf)->pUserAddr, (*ppKerBuf)->pKernelAddr, size, size);
    }
    else
    {
        WDS_DIAG_ERR("MenuSharedBuffer: Failed allocating shared "
            "buffer memory. size [%d], Error [0x%x - %s]\n", size,
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

static DWORD MenuSharedBufferAllocContigOptionCb(PVOID pCbCtx)
{
    return AllocKernelBuff((WD_KERNEL_BUFFER **)pCbCtx, KER_BUF_ALLOC_CONTIG);
}

static DWORD MenuSharedBufferAllocNonContigOptionCb(PVOID pCbCtx)
{
    return AllocKernelBuff((WD_KERNEL_BUFFER **)pCbCtx,
        KER_BUF_ALLOC_NON_CONTIG);
}

static DWORD MenuSharedBufferExitCb(PVOID pCbCtx)
{
    WD_KERNEL_BUFFER **ppKerBuf = (WD_KERNEL_BUFFER **)pCbCtx;

    WDS_DIAG_SharedBufferFree(ppKerBuf);
    return WD_STATUS_SUCCESS;
}

void MenuSharedBufferSubMenusInit(DIAG_MENU_OPTION *pParentMenu,
    WD_KERNEL_BUFFER **ppKerBuf)
{
    static DIAG_MENU_OPTION allocateContigSharedBufferMenu = { 0 };
    static DIAG_MENU_OPTION allocateNonContigSharedBufferMenu = { 0 };
    static DIAG_MENU_OPTION freeSharedBufferMenu = { 0 };
    static DIAG_MENU_OPTION options[3] = { 0 };

    strcpy(allocateContigSharedBufferMenu.cOptionName, "Allocate contiguous "
        "shared buffer");
    allocateContigSharedBufferMenu.cbEntry =
        MenuSharedBufferAllocContigOptionCb;

    strcpy(allocateNonContigSharedBufferMenu.cOptionName, "Allocate "
        "non-contiguous shared buffer");
    allocateNonContigSharedBufferMenu.cbEntry =
        MenuSharedBufferAllocNonContigOptionCb;

    strcpy(freeSharedBufferMenu.cOptionName, "Free shared buffer");
    freeSharedBufferMenu.cbEntry = MenuSharedBufferExitCb;

    options[0] = allocateContigSharedBufferMenu;
    options[1] = allocateNonContigSharedBufferMenu;
    options[2] = freeSharedBufferMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        (PVOID)ppKerBuf, pParentMenu);
}

DIAG_MENU_OPTION *MenuSharedBufferInit(DIAG_MENU_OPTION *pParentMenu)
{
    static WD_KERNEL_BUFFER *pKerBuf = NULL;

    static DIAG_MENU_OPTION sharedBufferMenuRoot = { 0 };

    strcpy(sharedBufferMenuRoot.cOptionName, "Allocate/free Shared Buffer");
    strcpy(sharedBufferMenuRoot.cTitleName, "Shared Buffer Operations\n"
        "    E.g. for communicating with Kernel-Plugin");
    sharedBufferMenuRoot.cbExit = MenuSharedBufferExitCb;

    MenuSharedBufferSubMenusInit(&sharedBufferMenuRoot, &pKerBuf);
    DIAG_MenuSetCtxAndParentForMenus(&sharedBufferMenuRoot, 1, &pKerBuf,
        pParentMenu);

    return &sharedBufferMenuRoot;
}

/* -----------------------------------------------
    IPC - Inter process Communication
   ----------------------------------------------- */

static void ipc_msg_event_cb(WDS_IPC_MSG_RX *pIpcRxMsg, void *pData)
{
    UNUSED_VAR(pData);

    printf("\n\nReceived an IPC message:\n"
        "msgID [0x%x], msgData [0x%llx] from process [0x%x]\n",
        pIpcRxMsg->dwMsgID, pIpcRxMsg->qwMsgData, pIpcRxMsg->dwSenderUID);

    /* Important: Acquiring and using any resource (E.g. kernel/DMA buffer)
     * should be done from a deferred procedure to avoid jamming the IPC
     * incoming messages.
     * Notice you can pass private context at WDS_IpcRegister() and use it here
     * (pData) for signalling local thread for example.
     *
     * The following implementation is for sample purposes only! */

    switch (pIpcRxMsg->dwMsgID)
    {
    case IPC_MSG_KERNEL_BUFFER_READY:
    {
        DWORD dwStatus;
        WD_KERNEL_BUFFER *pKerBuf = NULL;
        DWORD sample_buffer_len;

        printf("\nThis is a shared kernel buffer, getting it...\n");

        dwStatus = WDS_SharedBufferGet((DWORD)pIpcRxMsg->qwMsgData,
            &pKerBuf);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            WDS_DIAG_ERR("ipc_msg_event_cb: Failed getting shared kernel "
                "buffer. Error [0x%x - %s]\n", dwStatus,
                Stat2Str(dwStatus));
            return;
        }

        printf("Got a shared kernel buffer. UserAddr [0x%" UPRI "x], "
            "KernelAddr [0x%" KPRI "x], size [%llu]\n", pKerBuf->pUserAddr,
            pKerBuf->pKernelAddr, pKerBuf->qwBytes);

        /* Here we read SAMPLE_BUFFER_DATA from the received buffer */
        sample_buffer_len = (DWORD)strlen(SAMPLE_BUFFER_DATA);
        if (pKerBuf->qwBytes > sample_buffer_len + 1)
        {
            printf("Sample data from kernel buffer [%s]\n",
                (char *)pKerBuf->pUserAddr);
        }
        else
        {
            printf("Kernel buffer was too short for sample data\n");
        }

        /* For sample purpose we immediately release the buffer */
        WDS_SharedBufferFree(pKerBuf);
    }
    break;

    case IPC_MSG_CONTIG_DMA_BUFFER_READY:
    {
        DWORD dwStatus;
        WD_DMA *pDma = NULL;

        printf("\nThis is a DMA buffer, getting it...\n");

        dwStatus = WDC_DMABufGet((DWORD)pIpcRxMsg->qwMsgData, &pDma);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            WDS_DIAG_ERR("ipc_msg_event_cb: Failed getting DMA buffer. "
                "Error [0x%x - %s]\n", dwStatus, Stat2Str(dwStatus));
            return;
        }

        printf("Got a DMA buffer. UserAddr [%p], "
            "pPhysicalAddr [0x%" PRI64 "x], size [%d(0x%x)]\n",
            pDma->pUserAddr, pDma->Page[0].pPhysicalAddr,
            pDma->Page[0].dwBytes, pDma->Page[0].dwBytes);

        /* For sample purpose we immediately release the buffer */
        WDC_DMABufUnlock(pDma);
    }
    break;

    default:
        printf("Unknown IPC type. msgID [0x%x], msgData [0x%llx] from "
            "process [0x%x]\n\n", pIpcRxMsg->dwMsgID, pIpcRxMsg->qwMsgData,
            pIpcRxMsg->dwSenderUID);
    }
}

static void ipc_shared_int_msg_event_cb(WDS_IPC_MSG_RX *pIpcRxMsg, void *pData)
{
    UNUSED_VAR(pData);

    printf("Shared Interrupt via IPC arrived:\nmsgID [0x%x], msgData [0x%llx]"
        " from process [0x%x]\n\n", pIpcRxMsg->dwMsgID, pIpcRxMsg->qwMsgData,
        pIpcRxMsg->dwSenderUID);
}

/* Register process to IPC service */
static DWORD WDS_DIAG_IpcRegister(void)
{
    DWORD dwSubGroupID = 0;
    DWORD dwStatus;

    sprintf(gsInput, "Enter process SubGroup ID (hex)");
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&dwSubGroupID, gsInput,
        TRUE, 0, 0xFFFFFFFF))
    {
        return 0;
    }

    dwStatus = WDS_IpcRegister(DEFAULT_PROCESS_NAME, DEFAULT_PROCESS_GROUP_ID,
        dwSubGroupID, WD_IPC_ALL_MSG, ipc_msg_event_cb, NULL /* Your cb ctx */);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_DIAG_ERR("WDS_DIAG_IpcRegister: Failed registering process to IPC. "
            "Error [0x%x - %s]\n", dwStatus, Stat2Str(dwStatus));
        return 0;
    }

    printf("Registration completed successfully\n");
    return dwSubGroupID;
}

/* Enable Shared Interrupts via IPC */
static DWORD WDS_DIAG_IpcSharedIntsEnable(void)
{
    DWORD dwSubGroupID = 0;
    DWORD dwStatus;

    if (WDS_IsSharedIntsEnabledLocally())
    {
        WDS_DIAG_ERR("%s: Shared interrupts already enabled locally.\n",
            __FUNCTION__);
        return WD_OPERATION_ALREADY_DONE;
    }

    sprintf(gsInput, "Enter shared interrupt's SubGroup ID (hex)");
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&dwSubGroupID, gsInput,
        TRUE, 0, 0xFFFFFFFF))
    {
        return 0;
    }

    /* WDS_SharedIntEnable() is called in this sample with pFunc=
    ipc_shared_int_msg_event_cb. This will cause a shared interrupt to invoke
    both this callback and the callback passed to WDS_IpcRegister() in
    WDS_DIAG_IpcRegister(). To disable the "general" IPC callback, pass
    pFunc=NULL in the above mentioned call.
    Note you can replace pFunc here with your own callback especially designed
    to handle interrupts */
    dwStatus = WDS_SharedIntEnable(DEFAULT_SHARED_INT_NAME,
        DEFAULT_PROCESS_GROUP_ID, dwSubGroupID, WD_IPC_ALL_MSG,
        ipc_shared_int_msg_event_cb, NULL /* Your cb ctx */);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_DIAG_ERR("%s: Failed enabling shared interrupts via IPC. "
            "Error [0x%x - %s]\n", __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        return 0;
    }

    printf("Shared interrupts via IPC enabled successfully\n");
    return dwSubGroupID;
}

static void WDS_DIAG_IpcScanProcs(void)
{
    DWORD dwStatus;
    WDS_IPC_SCAN_RESULT ipcScanResult;
    DWORD i;

    dwStatus = WDS_IpcScanProcs(&ipcScanResult);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_DIAG_ERR("WDS_DIAG_IpcScanProcs: Failed scanning registered "
            "processes. Error [0x%x - %s]\n", dwStatus, Stat2Str(dwStatus));
        return;
    }

    if (ipcScanResult.dwNumProcs)
    {
        printf("Found %d processes in current group\n",
            ipcScanResult.dwNumProcs);
        for (i = 0; i < ipcScanResult.dwNumProcs; i++)
        {
            printf("  %u) Name: %s, SubGroup ID: 0x%x, UID: 0x%x\n", i + 1,
                ipcScanResult.procInfo[i].cProcessName,
                ipcScanResult.procInfo[i].dwSubGroupID,
                ipcScanResult.procInfo[i].hIpc);
        }
    }
    else
    {
        printf("No processes found in current group\n");
    }
}

static void WDS_DIAG_IpcKerBufRelease(void)
{
    DWORD dwStatus;

    if (!pSharedKerBuf)
        return;

    /* Notice that once a buffer that was acquired by a different process is
     * freed, its kernel resources are kept as long as the other processes did
     * not release the buffer. */
    dwStatus = WDS_SharedBufferFree(pSharedKerBuf);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_DIAG_ERR("WDS_DIAG_IpcKerBufRelease: Failed freeing shared "
            "buffer. Error [0x%x - %s]\n", dwStatus, Stat2Str(dwStatus));
    }

    pSharedKerBuf = NULL;

    printf("Kernel buffer freed\n");
}

static void WDS_DIAG_IpcKerBufAllocAndShare(void)
{
    DWORD size = 0;
    DWORD dwStatus;
    DWORD dwOptions = KER_BUF_ALLOC_CONTIG;
    DWORD sample_buffer_len;

    /* If kernel buffer was allocated in the past, release it */
    WDS_DIAG_IpcKerBufRelease();

    sprintf(gsInput, "Enter new kernel buffer size to allocate and share with "
        "current group");
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&size, gsInput, TRUE, 1,
        0xFFFFFFFF))
    {
        return;
    }

    dwStatus = WDS_SharedBufferAlloc(size, dwOptions, &pSharedKerBuf);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_DIAG_ERR("WDS_DIAG_IpcKerBufAllocAndShare: Failed allocating "
            "shared kernel buffer. size [%u], Error [0x%x - %s]\n", size,
            dwStatus, Stat2Str(dwStatus));
        return;
    }

    printf("Successful kernel buffer allocation. UserAddr [0x%" UPRI "x], "
        "KernelAddr [0x%" KPRI "x], size [%u]\n", pSharedKerBuf->pUserAddr,
        pSharedKerBuf->pKernelAddr, size);

    /* Here we write SAMPLE_BUFFER_DATA to the new allocated buffer */
    sample_buffer_len = (DWORD)strlen(SAMPLE_BUFFER_DATA);
    if (size > sample_buffer_len + 1)
    {
        memcpy((PVOID)pSharedKerBuf->pUserAddr, SAMPLE_BUFFER_DATA,
            sample_buffer_len);
        ((char *)pSharedKerBuf->pUserAddr)[sample_buffer_len] = '\0';
        printf("Sample data written to kernel buffer\n");
    }
    else
    {
        printf("Kernel buffer is too short for sample data\n");
    }

    dwStatus = WDS_IpcMulticast(IPC_MSG_KERNEL_BUFFER_READY,
        WDS_SharedBufferGetGlobalHandle(pSharedKerBuf));
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_DIAG_ERR("WDS_DIAG_IpcAllocAndShareBuf: Failed sending message. "
            "Error [0x%x - %s]\n", dwStatus, Stat2Str(dwStatus));
        return;
    }

    printf("Kernel buffer shared successfully\n");
}

/* IPC menu options */
enum {
    MENU_IPC_REGISTER = 1,
    MENU_IPC_UNREGISTER,
    MENU_IPC_GET_GROUP_IDS,
    MENU_IPC_SEND_UID_UNICAST,
    MENU_IPC_SEND_SUBGROUP_MULTICAST,
    MENU_IPC_SEND_MULTICAST,
    MENU_IPC_ENABLE_SHARED_INTS,
    MENU_IPC_LOCAL_DISABLE_SHARED_INTS,
    MENU_IPC_GLOBAL_DISABLE_SHARED_INTS,
    MENU_IPC_KER_BUF_ALLOC_AND_SHARE,
    MENU_IPC_KER_BUF_RELEASE,
    MENU_IPC_EXIT = DIAG_EXIT_MENU,
};

static void WDS_DIAG_IpcSend(DWORD ipc_menu_option)
{
    DWORD recipientID;
    DWORD messageID;
    UINT64 messageData;
    DWORD dwStatus;

    if (ipc_menu_option == MENU_IPC_SEND_UID_UNICAST ||
        ipc_menu_option == MENU_IPC_SEND_SUBGROUP_MULTICAST)
    {
        sprintf(gsInput, "Enter recipient%sID (hex)",
            ipc_menu_option == MENU_IPC_SEND_SUBGROUP_MULTICAST ?
            "(s) SubGroup " : " U");
        if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&recipientID,
            gsInput, TRUE, 0, 0xFFFFFFFF))
        {
            return;
        }
    }

    sprintf(gsInput, "Enter your message ID (32Bit hex)");
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&messageID, gsInput, TRUE,
        0, 0xFFFFFFFF))
    {
        return;
    }

    sprintf(gsInput, "Enter your message (64Bit hex)");
    if (DIAG_INPUT_SUCCESS != DIAG_InputUINT64((PVOID)&messageData, gsInput,
        TRUE, 0, 0xFFFFFFFF))
    {
        return;
    }

    switch (ipc_menu_option)
    {
    case MENU_IPC_SEND_UID_UNICAST:
        dwStatus = WDS_IpcUidUnicast(recipientID, messageID, messageData);
        break;

    case MENU_IPC_SEND_SUBGROUP_MULTICAST:
        dwStatus = WDS_IpcSubGroupMulticast(recipientID, messageID,
            messageData);
        break;

    case MENU_IPC_SEND_MULTICAST:
        dwStatus = WDS_IpcMulticast(messageID, messageData);
        break;
    }

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_DIAG_ERR("WDS_DIAG_IpcSend: Failed sending message. "
            "Error [0x%x - %s]\n", dwStatus, Stat2Str(dwStatus));
        return;
    }

    printf("Message sent successfully\n");
}

static BOOL MenuIpcIsRegistered(DIAG_MENU_OPTION *pMenu)
{
    UNUSED_VAR(pMenu);

    return WDS_IsIpcRegistered();
}

static BOOL MenuIpcIsNotRegistered(DIAG_MENU_OPTION *pMenu)
{
    UNUSED_VAR(pMenu);

    return !WDS_IsIpcRegistered();
}

static DWORD MenuIpcRegisterCb(PVOID pCbCtx)
{
    *(PDWORD)pCbCtx = WDS_DIAG_IpcRegister();

    return WD_STATUS_SUCCESS;
}

static DWORD MenuIpcUnRegisterCb(PVOID pCbCtx)
{
    UNUSED_VAR(pCbCtx);
    WDS_IpcUnRegister();
    printf("Process unregistered successfully\n");

    return WD_STATUS_SUCCESS;
}

static DWORD MenuIpcScanCb(PVOID pCbCtx)
{
    UNUSED_VAR(pCbCtx);
    WDS_DIAG_IpcScanProcs();

    return WD_STATUS_SUCCESS;
}

static DWORD MenuIpcUnicastCb(PVOID pCbCtx)
{
    UNUSED_VAR(pCbCtx);
    WDS_DIAG_IpcSend(MENU_IPC_SEND_UID_UNICAST);

    return WD_STATUS_SUCCESS;
}

static DWORD MenuIpcMulticastSubGroupCb(PVOID pCbCtx)
{
    UNUSED_VAR(pCbCtx);
    WDS_DIAG_IpcSend(MENU_IPC_SEND_SUBGROUP_MULTICAST);

    return WD_STATUS_SUCCESS;
}

static DWORD MenuIpcMulticastGroupCb(PVOID pCbCtx)
{
    UNUSED_VAR(pCbCtx);
    WDS_DIAG_IpcSend(MENU_IPC_SEND_MULTICAST);

    return WD_STATUS_SUCCESS;
}

static DWORD MenuIpcSharedIntEnableCb(PVOID pCbCtx)
{
    UNUSED_VAR(pCbCtx);
    WDS_DIAG_IpcSharedIntsEnable();

    return WD_STATUS_SUCCESS;
}

static DWORD MenuIpcSharedIntDisableLocalCb(PVOID pCbCtx)
{
    UNUSED_VAR(pCbCtx);

    if (WDS_SharedIntDisableLocal() == WD_STATUS_SUCCESS)
        printf("\nShared ints successfully disabled locally\n");
    else
        printf("\nShared ints already disabled locally\n");

    return WD_STATUS_SUCCESS;
}

static DWORD MenuIpcSharedIntDisableGlobalCb(PVOID pCbCtx)
{
    /*After global disable, shared interrupts must be disabled
        *locally */
    if (WDS_SharedIntDisableGlobal() == WD_STATUS_SUCCESS)
        printf("\nShared ints successfully disabled globally\n");
    else
        printf("\nShared ints already disabled globally\n");

    return MenuIpcSharedIntDisableLocalCb(pCbCtx);
}

static DWORD MenuIpcSharedBufferAllocCb(PVOID pCbCtx)
{
    UNUSED_VAR(pCbCtx);
    WDS_DIAG_IpcKerBufAllocAndShare();

    return WD_STATUS_SUCCESS;
}

static DWORD MenuIpcSharedBufferFreeCb(PVOID pCbCtx)
{
    UNUSED_VAR(pCbCtx);
    WDS_DIAG_IpcKerBufRelease();

    return WD_STATUS_SUCCESS;
}

static void MenuIpcSubMenusInit(DIAG_MENU_OPTION *pParentMenu,
    PDWORD pdwSubGroupID)
{
    static DIAG_MENU_OPTION registerProcessesMenu = { 0 };
    static DIAG_MENU_OPTION unregisterProcessesMenu = { 0 };
    static DIAG_MENU_OPTION findMenu = { 0 };
    static DIAG_MENU_OPTION unicastMenu = { 0 };
    static DIAG_MENU_OPTION multicastSubGroupMenu = { 0 };
    static DIAG_MENU_OPTION multicastAllMenu = { 0 };
    static DIAG_MENU_OPTION enableIntsMenu = { 0 };
    static DIAG_MENU_OPTION disableLocallyIntsMenu = { 0 };
    static DIAG_MENU_OPTION disableGloballyIntsMenu = { 0 };
    static DIAG_MENU_OPTION allocateAndShareBufferMenu = { 0 };
    static DIAG_MENU_OPTION freeSharedBufferMenu = { 0 };
    static DIAG_MENU_OPTION options[11] = { 0 };

    strcpy(registerProcessesMenu.cOptionName, "Register process");
    registerProcessesMenu.cbEntry = MenuIpcRegisterCb;
    registerProcessesMenu.cbIsHidden = MenuIpcIsRegistered;

    strcpy(unregisterProcessesMenu.cOptionName, "Un-Register process");
    unregisterProcessesMenu.cbEntry = MenuIpcUnRegisterCb;
    unregisterProcessesMenu.cbIsHidden = MenuIpcIsNotRegistered;

    strcpy(findMenu.cOptionName, "Find current registered group processes");
    findMenu.cbEntry = MenuIpcScanCb;
    findMenu.cbIsHidden = MenuIpcIsNotRegistered;

    strcpy(unicastMenu.cOptionName, "Unicast- Send message to a single process"
        " by unique ID");
    unicastMenu.cbEntry = MenuIpcUnicastCb;
    unicastMenu.cbIsHidden = MenuIpcIsNotRegistered;

    strcpy(multicastSubGroupMenu.cOptionName, "Multicast - Send message to a "
        "subGroup");
    multicastSubGroupMenu.cbEntry = MenuIpcMulticastSubGroupCb;
    multicastSubGroupMenu.cbIsHidden = MenuIpcIsNotRegistered;

    strcpy(multicastAllMenu.cOptionName, "Multicast- Send message to all "
        "processes in current group");
    multicastAllMenu.cbEntry = MenuIpcMulticastGroupCb;
    multicastAllMenu.cbIsHidden = MenuIpcIsNotRegistered;

    strcpy(enableIntsMenu.cOptionName, "Enable Shared Interrupts via IPC");
    enableIntsMenu.cbEntry = MenuIpcSharedIntEnableCb;
    enableIntsMenu.cbIsHidden = MenuIpcIsNotRegistered;

    strcpy(disableLocallyIntsMenu.cOptionName, "Locally Disable Shared "
        "Interrupts via IPC");
    disableLocallyIntsMenu.cbEntry = MenuIpcSharedIntDisableLocalCb;
    disableLocallyIntsMenu.cbIsHidden = MenuIpcIsNotRegistered;

    strcpy(disableGloballyIntsMenu.cOptionName, "Globally Disable Shared "
        "Interrupts via IPC");
    disableGloballyIntsMenu.cbEntry = MenuIpcSharedIntDisableGlobalCb;
    disableGloballyIntsMenu.cbIsHidden = MenuIpcIsNotRegistered;

    strcpy(allocateAndShareBufferMenu.cOptionName, "Allocate and share a "
        "kernel buffer with all processes in current group");
    allocateAndShareBufferMenu.cbEntry = MenuIpcSharedBufferAllocCb;
    allocateAndShareBufferMenu.cbIsHidden = MenuIpcIsNotRegistered;

    strcpy(freeSharedBufferMenu.cOptionName, "Free shared kernel buffer");
    freeSharedBufferMenu.cbEntry = MenuIpcSharedBufferFreeCb;
    freeSharedBufferMenu.cbIsHidden = MenuIpcIsNotRegistered;

    options[0] = registerProcessesMenu;
    options[1] = unregisterProcessesMenu;
    options[2] = findMenu;
    options[3] = unicastMenu;
    options[4] = multicastSubGroupMenu;
    options[5] = multicastAllMenu;
    options[6] = enableIntsMenu;
    options[7] = disableLocallyIntsMenu;
    options[8] = disableGloballyIntsMenu;
    options[9] = allocateAndShareBufferMenu;
    options[10] = freeSharedBufferMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        pdwSubGroupID, pParentMenu);
}

static DWORD MenuIpcCb(PVOID pCbCtx)
{
    PDWORD pdwSubGroupID = (PDWORD)pCbCtx;

    printf("\n");
    printf("IPC management menu - ");

    if (WDS_IsIpcRegistered())
        printf("Registered with SubGroup ID 0x%x\n", *pdwSubGroupID);
    else
        printf("Unregistered\n");

    printf("--------------\n");

    return WD_STATUS_SUCCESS;
}

DIAG_MENU_OPTION *MenuIpcInit(DIAG_MENU_OPTION *pParentMenu)
{
    static DWORD dwSubGroupID = 0;
    static DIAG_MENU_OPTION ipcMenuRoot = { 0 };

    strcpy(ipcMenuRoot.cOptionName, "Manage IPC");
    ipcMenuRoot.cbEntry = MenuIpcCb;

    MenuIpcSubMenusInit(&ipcMenuRoot, &dwSubGroupID);
    DIAG_MenuSetCtxAndParentForMenus(&ipcMenuRoot, 1, &dwSubGroupID,
        pParentMenu);

    return &ipcMenuRoot;
}

DWORD WDS_DIAG_IpcSendDmaContigToGroup(WD_DMA *pDma)
{
    DWORD dwStatus;

    if (!pDma)
    {
        WDS_DIAG_ERR("send_dma_contig_buf_to_group: Error - DMA ctx is NULL\n");
        return WD_INVALID_PARAMETER;
    }

    if (!(pDma->dwOptions & DMA_KERNEL_BUFFER_ALLOC))
    {
        WDS_DIAG_ERR("send_dma_contig_buf_to_group: Error - Sharing SG DMA is "
            "not supported\n");
        return WD_INVALID_PARAMETER;
    }

    dwStatus = WDS_IpcMulticast(IPC_MSG_CONTIG_DMA_BUFFER_READY,
        WDC_DMAGetGlobalHandle(pDma));
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDS_DIAG_ERR("send_dma_contig_buf_to_group: Failed sending message. "
            "Error [0x%x - %s]\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    printf("DMA contiguous buffer handle sent successfully\n");
    return WD_STATUS_SUCCESS;
}

#endif /* !defined(__KERNEL__) */

