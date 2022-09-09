''' Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com '''

from .diag_lib import *
from .wdc_lib import *

#*****************************************************************************
#  File: wds_diag_lib.py - Implementation of shared WDS all devices' user-mode
#        diagnostics API.
#
#  Note: This code sample is provided AS-IS and as a guiding sample only.
#*****************************************************************************

# -------------------------------------------------------------------------
#    IPC
# -------------------------------------------------------------------------
# IPC API functions are not part of the standard WinDriver API, and not
# included in the standard version of WinDriver. The functions are part of
# "WinDriver for Server" API and require "WinDriver for Server" license.
# Note that "WinDriver for Server" APIs are included in WinDriver evaluation
# version.

g_dwSubGroupID = 0

# IPC scan processes results
class WDS_IPC_SCAN_RESULT(Structure): _fields_ = \
    [("dwNumProcs", DWORD), # Number of matching processes
     ("procInfo", WD_IPC_PROCESS * WD_IPC_MAX_PROCS)] # Array of processes info

# IPC message received
class WDS_IPC_MSG_RX(Structure): _fields_ = \
    [("dwSenderUID", DWORD), # Number of matching processes
    ("dwMsgID", DWORD),
    ("qwMsgData", UINT64)]

IPC_MSG_RX_HANDLER = DLLCALLCONV(None, POINTER(WDS_IPC_MSG_RX), PVOID)
#typedef void (*IPC_MSG_RX_HANDLER)(WDS_IPC_MSG_RX *pIpcRxMsg, void *pData);

# Example IPC messages IDs
IPC_MSG_KERNEL_BUFFER_READY     = 1
                                 # Kernel buffer (Created with
                                 # WDS_SharedBufferAlloc()) ready to be
                                 # shared between processes. Kernel Buffer
                                 # handle is passed in the qwMsgData

IPC_MSG_CONTIG_DMA_BUFFER_READY = 2
                                 # Kernel buffer (Created with
                                 # wdapi.WDC_DMAContigBufLock()) ready to be
                                 # shared between processes. DMA Buffer
                                 # handle is passed in the qwMsgData

# TODO: Modify/Add values to communicate between processes

#************************************************************
#  General definitions
#************************************************************
# Error messages display

def WDS_DIAG_ERR(s):
    wdapi_va.WDC_Err("PCI lib ERROR: %s\n" % s)
    print ("WDS_DIAG_ERR: " + s)

# Identifiers for shared interrupt IPC process */
DEFAULT_SHARED_INT_NAME = b"WinDriver IPC Shared Interrupt"

DEFAULT_PROCESS_NAME = b"Diagnostic program"

# Unique identifier of the processes group to avoid getting messages from
# processes made under WinDriver by other developers that use the same driver
# name.
# WinDriver developers are encouraged to change their driver name before
# distribution to avoid this issue entirely.

DEFAULT_PROCESS_GROUP_ID = 0x12345678
SAMPLE_BUFFER_DATA = b"This is a sample buffer data"

#***********************************************************
#  Global variables
#************************************************************

# User's shared kernel buffer handle
pSharedKerBuf = None # Static global pointer is used
                     # only for sample simplicity

# Get kernel buffer global handle
def WDS_SharedBufferGetGlobalHandle(pKerBuf):
    return pKerBuf.contents.hKerBuf

# -----------------------------------------------
#    Shared Buffer
# -----------------------------------------------

# Shared Buffer menu options
(   MENU_SB_ALLOC_CONTIG,
    MENU_SB_ALLOC_NON_CONTIG,
    MENU_SB_FREE ) = range(1,4)
MENU_SB_EXIT = DIAG_EXIT_MENU

def WDS_DIAG_SharedBufferFree(pKerBuf):
    if not pKerBuf:
        return

    dwStatus = wdapi.WDS_SharedBufferFree(pKerBuf)
    if WD_STATUS_SUCCESS != dwStatus:
        WDS_DIAG_ERR("WDS_DIAG_SharedBufferFree: Failed freeing shared buffer "
            "memory. Error [0x%lx - %s]" % (dwStatus, Stat2Str(dwStatus)))
        return

    pKerBuf = None
    print("Shared buffer memory freed")

gpKerBuf = None

def MenuSharedBufferExitCb(pCbCtx):
    global gpKerBuf
    WDS_DIAG_SharedBufferFree(gpKerBuf)
    gpKerBuf = None

    return WD_STATUS_SUCCESS

def AllocKernelBuff(dwOptions):
    global gpKerBuf

    (size, dwStatus) = DIAG_InputNum("Enter memory allocation size in "
        "bytes (32 bit uint) ", True, sizeof(DWORD), 1, 0xFFFFFFFF)
    if DIAG_INPUT_SUCCESS != dwStatus:
        return WD_INVALID_PARAMETER

    # Free shared buffer memory before trying the new allocation
    WDS_DIAG_SharedBufferFree(gpKerBuf)
    gpKerBuf = POINTER(WD_KERNEL_BUFFER)()

    dwStatus = wdapi.WDS_SharedBufferAlloc(UINT64(size.value),
        dwOptions, byref(gpKerBuf))
    if WD_STATUS_SUCCESS == dwStatus:
        print("Shared buffer allocated. User addr [0x%x], "
            "kernel addr [0x%x], size [%lu(0x%x)]" %
            (gpKerBuf.contents.pUserAddr, gpKerBuf.contents.pKernelAddr,
            size.value, size.value))
    else:
        WDS_DIAG_ERR("MenuSharedBuffer: Failed allocating shared "
            "buffer memory. size [%ld], Error [0x%x - %s]" %
            (size.value, dwStatus, Stat2Str(dwStatus)))

    return dwStatus

def MenuSharedBufferContigOptionCb(pCbCtx):
    return AllocKernelBuff(KER_BUF_ALLOC_CONTIG)

def MenuSharedBufferContigOptionCb(pCbCtx):
    return AllocKernelBuff(KER_BUF_ALLOC_NON_CONTIG)

def MenuSharedBufferSubMenusInit(pParentMenu):
    options = [
        DIAG_MENU_OPTION (
            cOptionName = "Allocate contiguous shared buffer",
            cbEntry = MenuSharedBufferContigOptionCb),

        DIAG_MENU_OPTION (
            cOptionName = "Allocate non-contiguous shared buffer",
            cbEntry = MenuSharedBufferContigOptionCb ),

        DIAG_MENU_OPTION (
            cOptionName = "Free shared buffer",
            cbEntry = MenuSharedBufferExitCb )
    ]

    DIAG_MenuSetCtxAndParentForMenus(options, None, pParentMenu)

def MenuSharedBufferInit(pParentMenu):
    sharedBufferMenuRoot = DIAG_MENU_OPTION(
        cOptionName = "Allocate/free Shared Buffer",
        cTitleName = "Shared Buffer Operations\n"
            "    E.g. for communicating with Kernel-Plugin",
        cbExit = MenuSharedBufferExitCb
    )

    MenuSharedBufferSubMenusInit(sharedBufferMenuRoot)
    DIAG_MenuSetCtxAndParentForMenus([sharedBufferMenuRoot], None, pParentMenu)

    return sharedBufferMenuRoot

#   -----------------------------------------------
#    IPC - Inter process Communication
#   -----------------------------------------------

def ipc_msg_event_cb(pIpcRxMsg, pData):
    print("\nReceived an IPC message:\n"
        "msgID [0x%lx], msgData [0x%lx] from process [0x%lx]" %
        (pIpcRxMsg.contents.dwMsgID, pIpcRxMsg.contents.qwMsgData,
        pIpcRxMsg.contents.dwSenderUID))

    # Important: Acquiring and using any resource (E.g. kernel/DMA buffer)
    # should be done from a deferred procedure to avoid jamming the IPC
    # incoming messages.
    # Notice you can pass private context at WDS_IpcRegister() and use it here
    # (pData) for signalling local thread for example.

    # The following implementation is for sample purposes only!

    if pIpcRxMsg.contents.dwMsgID == IPC_MSG_KERNEL_BUFFER_READY:
        pKerBuf = POINTER(WD_KERNEL_BUFFER)()

        print("This is a shared kernel buffer, getting it...")
        dwStatus = wdapi.WDS_SharedBufferGet(pIpcRxMsg.contents.qwMsgData,
            byref(pKerBuf))
        if WD_STATUS_SUCCESS != dwStatus:
            WDS_DIAG_ERR("ipc_msg_event_cb: Failed getting shared kernel "
                "buffer. Error [0x%lx - %s]" % (dwStatus,
                Stat2Str(dwStatus)))
            return

        print("Got a shared kernel buffer. UserAddr [0x%lx], "
            "KernelAddr [0x%lx], size [%lu]" % (pKerBuf.contents.pUserAddr,
            pKerBuf.contents.pKernelAddr, pKerBuf.contents.qwBytes))

        # Here we read SAMPLE_BUFFER_DATA from the received buffer
        sample_buffer_len = len(SAMPLE_BUFFER_DATA)
        if pKerBuf.contents.qwBytes > sample_buffer_len + 1:
            print("Sample data from kernel buffer [%s]" %
                string_at(pKerBuf.contents.pUserAddr, sample_buffer_len).
                decode('utf-8'))
        else:
            print("Kernel buffer was too short for sample data")

        # For sample purpose we immediately release the buffer
        wdapi.WDS_SharedBufferFree(pKerBuf)

    elif pIpcRxMsg.contents.dwMsgID == IPC_MSG_CONTIG_DMA_BUFFER_READY:
        pDma = POINTER(WD_DMA)()
        print("This is a DMA buffer, getting it...")

        dwStatus = wdapi.WDC_DMABufGet(DWORD(pIpcRxMsg.contents.qwMsgData),
            byref(pDma))
        if WD_STATUS_SUCCESS != dwStatus:
            WDS_DIAG_ERR("ipc_msg_event_cb: Failed getting DMA buffer. "
                "Error [0x%lx - %s]" % (dwStatus, Stat2Str(dwStatus)))
            return

        print("Got a DMA buffer. UserAddr [%lx], pPhysicalAddr [0x%lx], size "
            "[%ld(0x%lx)]" % (pDma.contents.pUserAddr,
            pDma.contents.Page[0].pPhysicalAddr, pDma.contents.Page[0].dwBytes,
            pDma.contents.Page[0].dwBytes))

        # For sample purpose we immediately release the buffer
        wdapi.WDC_DMABufUnlock(pDma)
    else:
        print("Unknown IPC type. msgID [0x%lx], msgData [0x%lx] from "
            "process [0x%lx]" % (pIpcRxMsg.contents.dwMsgID,
            pIpcRxMsg.contents.qwMsgData, pIpcRxMsg.contents.dwSenderUID))

gf_ipc_msg_event_cb = IPC_MSG_RX_HANDLER(ipc_msg_event_cb)

def ipc_shared_int_msg_event_cb(pIpcRxMsg, pData):
    print("Shared Interrupt via IPC arrived:\nmsgID [0x%lx], msgData [0x%lx]"
        " from process [0x%lx]\n" % (pIpcRxMsg.contents.dwMsgID,
        pIpcRxMsg.contents.qwMsgData, pIpcRxMsg.contents.dwSenderUID))

gf_ipc_shared_int_event_cb = IPC_MSG_RX_HANDLER(ipc_shared_int_msg_event_cb)

# Register process to IPC service
def WDS_DIAG_IpcRegister():
    (dwSubGroupID, dwStatus) = DIAG_InputNum("Enter process SubGroup ID (hex)",
        True, sizeof(DWORD), 0, 0xFFFFFFFF)
    if DIAG_INPUT_SUCCESS != dwStatus:
        return 0

    dwStatus = wdapi.WDS_IpcRegister(DEFAULT_PROCESS_NAME,
        DEFAULT_PROCESS_GROUP_ID, dwSubGroupID, WD_IPC_ALL_MSG,
        gf_ipc_msg_event_cb, None ) # Your cb ctx
    if WD_STATUS_SUCCESS != dwStatus:
        WDS_DIAG_ERR("WDS_DIAG_IpcRegister: Failed registering process to IPC. "
            "Error [0x%lx - %s]" % (dwStatus, Stat2Str(dwStatus)))
        return 0

    print("Registration completed successfully")
    return dwSubGroupID.value

# Enable Shared Interrupts via IPC
def WDS_DIAG_IpcSharedIntsEnable():
    if (wdapi.WDS_IsSharedIntsEnabledLocally()):
        WDS_DIAG_ERR("WDS_DIAG_IpcSharedIntsEnable: Shared interrupts already"
             " enabled locally.\n")
        return WD_OPERATION_ALREADY_DONE

    (dwSubGroupID, dwStatus) = DIAG_InputNum("Enter shared interrupt's SubGroup"
        " ID (hex)", True, sizeof(DWORD), 1, 0xFFFFFFFF)
    if DIAG_INPUT_SUCCESS != dwStatus:
        return 0

    # WDS_SharedIntEnable() is called in this sample with pFunc=
    # ipc_shared_int_msg_event_cb. This will cause a shared interrupt to invoke
    # both this callback and the callback passed to WDS_IpcRegister() in
    # WDS_DIAG_IpcRegister(). To disable the "general" IPC callback, pass
    # pFunc=NULL in the above mentioned call.
    # Note you can replace pFunc here with your own callback especially designed
    # to handle interrupts
    dwStatus = wdapi.WDS_SharedIntEnable(DEFAULT_SHARED_INT_NAME,
        DEFAULT_PROCESS_GROUP_ID, dwSubGroupID, WD_IPC_ALL_MSG,
        gf_ipc_shared_int_event_cb, None) # Your cb ctx
    if (WD_STATUS_SUCCESS != dwStatus):
        WDS_DIAG_ERR("WDS_DIAG_IpcSharedIntsEnable: Failed enabling shared"
            " interrupts via IPC. Error [0x%lx - %s]\n" %
            (dwStatus, Stat2Str(dwStatus)))
        return 0

    print("Shared interrupts via IPC enabled successfully\n")
    return dwSubGroupID

def WDS_DIAG_IpcScanProcs():
    ipcScanResult = WDS_IPC_SCAN_RESULT()

    dwStatus = wdapi.WDS_IpcScanProcs(byref(ipcScanResult))
    if WD_STATUS_SUCCESS != dwStatus:
        WDS_DIAG_ERR("WDS_DIAG_IpcScanProcs: Failed scanning registered "
            "processes. Error [0x%lx - %s]" % (dwStatus,
            Stat2Str(dwStatus)))
        return

    if ipcScanResult.dwNumProcs:
        print("Found %ld processes in current group" % ipcScanResult.dwNumProcs)
        for i in range(ipcScanResult.dwNumProcs):
            print("  %lu) Name: %s, SubGroup ID: 0x%lx, UID: 0x%lx" % (i + 1,
                ipcScanResult.procInfo[i].cProcessName.decode('utf-8'),
                ipcScanResult.procInfo[i].dwSubGroupID,
                ipcScanResult.procInfo[i].hIpc))
    else:
        print("No processes found in current group")

    return WD_STATUS_SUCCESS

def WDS_DIAG_IpcKerBufRelease():
    global pSharedKerBuf

    if not pSharedKerBuf:
        return WD_OPERATION_ALREADY_DONE

    # Notice that once a buffer that was acquired by a different process is
    # freed, its kernel resources are kept as long as the other processes did
    # not release the buffer.
    dwStatus = wdapi.WDS_SharedBufferFree(pSharedKerBuf)
    if WD_STATUS_SUCCESS != dwStatus:
        WDS_DIAG_ERR("WDS_DIAG_IpcKerBufRelease: Failed freeing shared "
            "buffer. Error [0x%lx - %s]" % (dwStatus, Stat2Str(dwStatus)))
        return dwStatus

    pSharedKerBuf = None
    print("Kernel buffer freed")
    return dwStatus

def WDS_DIAG_IpcKerBufAllocAndShare():
    global pSharedKerBuf
    dwOptions = DWORD(KER_BUF_ALLOC_CONTIG)

    # If kernel buffer was allocated in the past, release it
    WDS_DIAG_IpcKerBufRelease()

    pSharedKerBuf = POINTER(WD_KERNEL_BUFFER)()

    (size, dwStatus) = DIAG_InputNum("Enter new kernel buffer size to allocate"
        " and share with current group", True, sizeof(UINT64), 1, 0xFFFFFFFF)
    if DIAG_INPUT_SUCCESS != dwStatus:
        return dwStatus

    dwStatus = wdapi.WDS_SharedBufferAlloc(size, dwOptions,
        byref(pSharedKerBuf))
    if WD_STATUS_SUCCESS != dwStatus:
        WDS_DIAG_ERR("WDS_DIAG_IpcKerBufAllocAndShare: Failed allocating "
            "shared kernel buffer. size [%lu], Error [0x%lx - %s]" % size.value,
            dwStatus, Stat2Str(dwStatus))
        return dwStatus

    print("Successful kernel buffer allocation. UserAddr [%#lx], "
        "KernelAddr [%#lx], size [%#lx]" % (pSharedKerBuf.contents.pUserAddr,
        pSharedKerBuf.contents.pKernelAddr, size.value))

    # Here we write SAMPLE_BUFFER_DATA to the new allocated buffer
    sample_buffer_len = len(SAMPLE_BUFFER_DATA)
    if size.value > sample_buffer_len + 1:
        memmove(pSharedKerBuf.contents.pUserAddr, SAMPLE_BUFFER_DATA,
            sample_buffer_len)
        print("Sample data written to kernel buffer\0")
    else:
        print("Kernel buffer is too short for sample data")

    dwStatus = wdapi.WDS_IpcMulticast(IPC_MSG_KERNEL_BUFFER_READY,
        WDS_SharedBufferGetGlobalHandle(pSharedKerBuf))
    if WD_STATUS_SUCCESS != dwStatus:
        WDS_DIAG_ERR("WDS_DIAG_IpcAllocAndShareBuf: Failed sending message. "
            "Error [0x%lx - %s]" % (dwStatus, Stat2Str(dwStatus)))
        return dwStatus

    print("Kernel buffer shared successfully")
    return dwStatus

# IPC menu options
(   MENU_IPC_REGISTER,
    MENU_IPC_UNREGISTER,
    MENU_IPC_GET_GROUP_IDS,
    MENU_IPC_SEND_UID_UNICAST,
    MENU_IPC_SEND_SUBGROUP_MULTICAST,
    MENU_IPC_SEND_MULTICAST,
    MENU_IPC_ENABLE_SHARED_INTS,
    MENU_IPC_LOCAL_DISABLE_SHARED_INTS,
    MENU_IPC_GLOBAL_DISABLE_SHARED_INTS,
    MENU_IPC_KER_BUF_ALLOC_AND_SHARE,
    MENU_IPC_KER_BUF_RELEASE ) = range(1, 12)
MENU_IPC_EXIT = DIAG_EXIT_MENU

def WDS_DIAG_IpcSend(ipc_menu_option):
    if ipc_menu_option == MENU_IPC_SEND_UID_UNICAST or \
        ipc_menu_option == MENU_IPC_SEND_SUBGROUP_MULTICAST:
        (recipientID, dwStatus) = DIAG_InputNum("Enter recipient%sID (hex)" %
            ("(s) SubGroup " if ipc_menu_option ==
            MENU_IPC_SEND_SUBGROUP_MULTICAST else " U"), True, sizeof(DWORD), 0,
            0xFFFFFFFF)
        if DIAG_INPUT_SUCCESS != dwStatus:
            return dwStatus

    (messageID, dwStatus) = DIAG_InputNum("Enter your message ID (32Bit hex)",
        True, sizeof(DWORD), 0, 0xFFFFFFFF)
    if DIAG_INPUT_SUCCESS != dwStatus:
        return dwStatus

    (messageData, dwStatus) = DIAG_InputNum("Enter your message (64Bit hex)",
        True, sizeof(UINT64), 0, 0xFFFFFFFFFFFFFFFF)
    if DIAG_INPUT_SUCCESS != dwStatus:
        return dwStatus

    if ipc_menu_option == MENU_IPC_SEND_UID_UNICAST:
        dwStatus = wdapi.WDS_IpcUidUnicast(recipientID, messageID, messageData)
    elif ipc_menu_option == MENU_IPC_SEND_SUBGROUP_MULTICAST:
        dwStatus = wdapi.WDS_IpcSubGroupMulticast(recipientID, messageID,
            messageData)
    elif ipc_menu_option == MENU_IPC_SEND_MULTICAST:
        dwStatus = wdapi.WDS_IpcMulticast(messageID, messageData)

    if WD_STATUS_SUCCESS != dwStatus:
        WDS_DIAG_ERR("WDS_DIAG_IpcSend: Failed sending message. "
            "Error [0x%lx - %s]" % (dwStatus, Stat2Str(dwStatus)))
        return dwStatus

    print("Message sent successfully")
    return dwStatus

def WDS_DIAG_IpcSendDmaContigToGroup(pDma):
    if not pDma:
        WDS_DIAG_ERR("send_dma_contig_buf_to_group: Error - DMA ctx is None")
        return WD_INVALID_PARAMETER

    if not (pDma.contents.dwOptions & DMA_KERNEL_BUFFER_ALLOC):
        WDS_DIAG_ERR("send_dma_contig_buf_to_group: Error - Sharing SG DMA is "
            "not supported")
        return WD_INVALID_PARAMETER

    dwStatus = wdapi.WDS_IpcMulticast(IPC_MSG_CONTIG_DMA_BUFFER_READY,
        WDC_DMAGetGlobalHandle(pDma))
    if WD_STATUS_SUCCESS != dwStatus:
        WDS_DIAG_ERR("send_dma_contig_buf_to_group: Failed sending message. "
            "Error [0x%lx - %s]" % (dwStatus, Stat2Str(dwStatus)))
        return dwStatus

    print("DMA contiguous buffer handle sent successfully")
    return WD_STATUS_SUCCESS

def MenuIpcIsRegistered(pMenu):
    return wdapi.WDS_IsIpcRegistered()

def MenuIpcIsNotRegistered(pMenu):
    return not wdapi.WDS_IsIpcRegistered()

def MenuIpcRegisterCb(pCbCtx):
    global g_dwSubGroupID
    g_dwSubGroupID = WDS_DIAG_IpcRegister()

    return WD_STATUS_SUCCESS

def MenuIpcUnRegisterCb(pCbCtx):
    wdapi.WDS_IpcUnRegister()
    printf("Process unregistered successfully\n")
    return WD_STATUS_SUCCESS

def MenuIpcScanCb(pCbCtx):
    return WDS_DIAG_IpcScanProcs()

def MenuIpcUnicastCb(pCbCtx):
    return WDS_DIAG_IpcSend(MENU_IPC_SEND_UID_UNICAST)

def MenuIpcMulticastSubGroupCb(pCbCtx):
    return WDS_DIAG_IpcSend(MENU_IPC_SEND_SUBGROUP_MULTICAST)

def MenuIpcMulticastGroupCb(pCbCtx):
    return WDS_DIAG_IpcSend(MENU_IPC_SEND_MULTICAST)

def MenuIpcSharedIntEnableCb(pCbCtx):
    return WDS_DIAG_IpcSharedIntsEnable()

def MenuIpcSharedIntDisableLocalCb(pCbCtx):
    if (wdapi.WDS_SharedIntDisableLocal() == WD_STATUS_SUCCESS):
        printf("\nShared ints successfully disabled locally\n")
    else:
        printf("\nShared ints already disabled locally\n")

    return WD_STATUS_SUCCESS

def MenuIpcSharedIntDisableGlobalCb(pCbCtx):
    # After global disable, shared interrupts must be disabled
    #    locally
    if (wdapi.WDS_SharedIntDisableGlobal() == WD_STATUS_SUCCESS):
        printf("\nShared ints successfully disabled globally\n")
    else:
        printf("\nShared ints already disabled globally\n")

    return MenuIpcSharedIntDisableLocalCb(pCbCtx)

def MenuIpcSharedBufferAllocCb(pCbCtx):
    return WDS_DIAG_IpcKerBufAllocAndShare()

def MenuIpcSharedBufferFreeCb(pCbCtx):
    return WDS_DIAG_IpcKerBufRelease()

def MenuIpcSubMenusInit(pParentMenu):
    options = [
        DIAG_MENU_OPTION(
            cOptionName = "Register processes",
            cbEntry = MenuIpcRegisterCb,
            cbIsHidden = MenuIpcIsRegistered ),

        DIAG_MENU_OPTION (
            cOptionName = "Un-Register process",
            cbEntry = MenuIpcUnRegisterCb,
            cbIsHidden = MenuIpcIsNotRegistered ),

        DIAG_MENU_OPTION (
            cOptionName = "Find current registered group processes",
            cbEntry = MenuIpcScanCb,
            cbIsHidden = MenuIpcIsNotRegistered ),

        DIAG_MENU_OPTION (
            cOptionName = "Unicast- Send message to a single process by "
                    "unique ID",
            cbEntry = MenuIpcUnicastCb,
            cbIsHidden = MenuIpcIsNotRegistered ) ,

        DIAG_MENU_OPTION (
            cOptionName = "Multicast - Send message to a subGroup",
            cbEntry = MenuIpcMulticastSubGroupCb,
            cbIsHidden = MenuIpcIsNotRegistered ),

        DIAG_MENU_OPTION (
            cOptionName = "Multicast- Send message to all processes in current "
                    "group",
            cbEntry = MenuIpcMulticastGroupCb,
            cbIsHidden = MenuIpcIsNotRegistered ),

        DIAG_MENU_OPTION (
            cOptionName = "Enable Shared Interrupts via IPC",
            cbEntry = MenuIpcSharedIntEnableCb,
            cbIsHidden = MenuIpcIsNotRegistered ),

        DIAG_MENU_OPTION (
            cOptionName = "Locally Disable Shared Interrupts via IPC",
            cbEntry = MenuIpcSharedIntDisableLocalCb,
            cbIsHidden = MenuIpcIsNotRegistered ),

        DIAG_MENU_OPTION (
            cOptionName = "Globally Disable Shared Interrupts via IPC",
            cbEntry = MenuIpcSharedIntDisableGlobalCb,
            cbIsHidden = MenuIpcIsNotRegistered ),

        DIAG_MENU_OPTION (
            cOptionName = "Allocate and share a kernel buffer with all "
                    "processes in current group",
            cbEntry = MenuIpcSharedBufferAllocCb,
            cbIsHidden = MenuIpcIsNotRegistered ),

        DIAG_MENU_OPTION (
            cOptionName = "Free shared kernel buffer",
            cbEntry = MenuIpcSharedBufferFreeCb,
            cbIsHidden = MenuIpcIsNotRegistered )
    ]

    DIAG_MenuSetCtxAndParentForMenus(options, None, pParentMenu)

def MenuIpcCb(pCbCtx):
    global g_dwSubGroupID

    printf("\n")
    printf("IPC management menu - ")

    if (wdapi.WDS_IsIpcRegistered()):
        printf("Registered with SubGroup ID 0x%lx\n", g_dwSubGroupID)
    else:
        printf("Unregistered\n")

    printf("--------------\n")

    return WD_STATUS_SUCCESS

def MenuIpcInit(pParentMenu):
    ipcMenuRoot = DIAG_MENU_OPTION(
        cOptionName = "Manage IPC",
        cbEntry = MenuIpcCb
    )

    MenuIpcSubMenusInit(ipcMenuRoot)
    DIAG_MenuSetCtxAndParentForMenus([ipcMenuRoot], None, pParentMenu)

    return ipcMenuRoot


