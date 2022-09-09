/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _WDS_LIB_H_
#define _WDS_LIB_H_

/*********************************************************************
*  File: wds_lib.h - WD Shared (WDS) library header.                 *
*                    This file defines the WDS library's high-level  *
*                    interface                                       *
**********************************************************************/

#if defined(__KERNEL__)
    #include "kpstdlib.h"
#endif

#include "windrvr.h"
#include "wdc_lib.h"

#ifdef __cplusplus
    extern "C" {
#endif

/**************************************************************
  General definitions
 **************************************************************/

/* -------------------------------------------------------------------------
    IPC
   ------------------------------------------------------------------------- */
/* IPC API functions are not part of the standard WinDriver API, and not
 * included in the standard version of WinDriver. The functions are part of
 * "WinDriver for Server" API and require "WinDriver for Server" license.
 * Note that "WinDriver for Server" APIs are included in WinDriver evaluation
 * version. */

/** IPC scan processes results */
typedef struct {
    DWORD          dwNumProcs; /**< Number of matching processes */
    WD_IPC_PROCESS procInfo[WD_IPC_MAX_PROCS]; /**< Array of processes info */
} WDS_IPC_SCAN_RESULT;

/** IPC message received */
typedef struct {
    DWORD   dwSenderUID; /**< WinDriver IPC unique ID of the sending process*/
    DWORD   dwMsgID; /**< A 32 bit unique number defined by the
                         user application. This number should be
                         known to all user-applications that
                         work under WinDriver IPC and share the
                         same group ID */
    UINT64  qwMsgData; /**< Optional - 64 bit additional data from the sending
                           user-application process */
} WDS_IPC_MSG_RX;


/**
*  WinDriver IPC message handler callback.
*
*   @param [in] pIpcRxMsg: Pointer to the received IPC message
*   @param [in] pData:     Application specific data opaque as passed
*                          to WDS_IpcRegister()
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*/
typedef void (*IPC_MSG_RX_HANDLER)(_In_ WDS_IPC_MSG_RX *pIpcRxMsg,
    _In_ void *pData);

/**
*  Enables the application to check if it is already registered with
*  WinDriver IPC.
*
* @return
*  Returns TRUE if successful; otherwise returns FALSE.
*
*/
BOOL DLLCALLCONV WDS_IsIpcRegistered(void);

/**
*  Registers an application with WinDriver IPC.
*
*   @param [in] pcProcessName: Optional process name string
*   @param [in] dwGroupID:     A unique group ID represent the specific
*                              application. Must be a positive ID
*   @param [in] dwSubGroupID:  Sub-group ID that should identify your user
*                              application type in case you have several types
*                              that may work simultaneously.
*                              Must be a positive ID
*   @param [in] dwAction:      IPC message type to receive,
*                              which can consist one of the enumeration
*                              values listed below:
*                               WD_IPC_UNICAST_MSG: Receive a message to a
*                               specific process with WinDriver IPC unique ID
*                               WD_IPC_MULTICAST_MSG: Receive a message from
*                               all processes that were registered with the
*                               same group ID as this process
*                               WD_IPC_ALL_MSG: Receive both types of the
*                               messages above
*   @param [in] pFunc:         A user-mode IPC message handler callback
*                              function, which will be called when a message
*                              was received by WinDriver from a different
*                              process (see dwActions) occurs.
*                              (See IPC_MSG_RX_HANDLER())
*   @param [in] pData:         Data for the user-mode IPC message handler
*                              callback routine (pFunc)
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @remarks
*         You should choose your user applications a unique group ID parameter.
*         This is done as a precaution to prevent several applications that use
*         WinDriver with its default driver name (windrvrXXXX) to get mixing
*         messages. We strongly recommend that you rename your driver before
*         distributing it to avoid this issue entirely, among other issue
*         (See Section 15.2 on renaming you driver name).
*         The sub-group id parameter should identify your user application
*         type in case you have several types that may work simultaneously.
*
* @snippet highlevel_examples.c WDS_IpcRegister
*/
DWORD DLLCALLCONV WDS_IpcRegister(_In_ const CHAR *pcProcessName,
    _In_ DWORD dwGroupID, _In_ DWORD dwSubGroupID, _In_ DWORD dwAction,
    _In_ IPC_MSG_RX_HANDLER pFunc, _In_ void *pData);

/**
*  This function enables the user application to unregister with WinDriver IPC.
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet highlevel_examples.c WDS_IpcUnRegister
*
*/
void DLLCALLCONV WDS_IpcUnRegister(void);

/**
*  Scans and returns information of all registered processes that share the
*  application process groupID
*  (as was given to WDS_IpcRegister() or a specific groupID.)
*
*   @param [out] pIpcScanResult: Pointer to IpcScanResult struct
*                                that will be filled by the function.
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet highlevel_examples.c WDS_IpcScanProcs
*
*/
DWORD DLLCALLCONV WDS_IpcScanProcs(
    _Outptr_ WDS_IPC_SCAN_RESULT *pIpcScanResult);


/**
*  Sends a message to a specific process with WinDriver IPC unique ID
*
*   @param [in] dwRecipientUID: WinDriver IPC unique ID that should identify
*                               one of your user application. The reciepient
*                               UID can be obtained from the result of
*                               WDS_IpcScanProcs() or the sender ID
*                               as received in the callback registered in
*                               WDS_IpcRegister()
*   @param [in] dwMsgID:        A 32 bit unique number defined by the
*                               user application. This number should be
*                               known to all user-applications that
*                               work under  WinDriver IPC and share the
*                               same group ID
*   @param [in] qwMsgData:      Optional - 64 bit additional data from the
*                               sending user-application
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet highlevel_examples.c WDS_MessageExample
*
*/
DWORD DLLCALLCONV WDS_IpcUidUnicast(_In_ DWORD dwRecipientUID,
    _In_ DWORD dwMsgID, _In_ UINT64 qwMsgData);

/**
*  Sends a message to all processes that registered with the same sub-group ID
*
*   @param [in] dwRecipientSubGroupID:  Recipient sub-group ID that should
*                                       identify your user application type
*                                       in case you have several types that
*                                       may work simultaneously.
*   @param [in] dwMsgID:                A 32 bit unique number defined by the
*                                       user application. This number should be
*                                       known to all user-applications that
*                                       work under  WinDriver IPC and share the
*                                       same group ID
*   @param [in] qwMsgData:              Optional - 64 bit additional data from
*                                       the sending user-application
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*
* @snippet highlevel_examples.c WDS_MessageExample
*/
DWORD DLLCALLCONV WDS_IpcSubGroupMulticast(_In_ DWORD dwRecipientSubGroupID,
    _In_ DWORD dwMsgID, _In_ UINT64 qwMsgData);

/**
*  Sends a message to all processes that were registered with the same group ID
*  as the sending process. The message won't be sent to the sending process.
*
*   @param [in] dwMsgID:   A 32 bit unique number defined by the user
*                          application. This number should be known
*                          to all user-applications that work under
*                          WinDriver IPC and share the same group ID
*   @param [in] qwMsgData: Optional - 64 bit additional data from the sending
*                          user-application
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*
* @snippet highlevel_examples.c WDS_MessageExample
*/
DWORD DLLCALLCONV WDS_IpcMulticast(_In_ DWORD dwMsgID, _In_ UINT64 qwMsgData);

/* -------------------------------------------------------------------------
    Shared Buffers (User-Mode <-> Kernel Mode) / (User-Mode <-> User-Mode)
   ------------------------------------------------------------------------- */
/*
 * Kernel buffers can be used to share data between:
 * 1) User-mode application and a Kernel PlugIn driver.
 * 2) Multiple user-mode applications.
 */

/**
*  Allocates a memory buffer that can be shared between the user mode and
*  the kernel mode ("shared buffer"), and returns user-mode and kernel-mode
*  virtual address space mappings of the allocated buffer.
*
*   @param [in] qwBytes:   The size of the buffer to allocate, in bytes
*   @param [in] dwOptions: Kernel buffer options bit-mask, which can consist
*                          of a combination of the enumeration values
*                          listed below.
*                          - KER_BUF_ALLOC_NON_CONTIG: Allocates a
*                          non contiguous buffer
*                          - KER_BUF_ALLOC_CONTIG: Allocates a
*                          physically contiguous buffer
*                          - KER_BUF_ALLOC_CACHED: Allocates a cached buffer.
*                          This option can be set with
*                          KER_BUF_ALLOC_NON_CONTIG or
*                          KER_BUF_ALLOC_CONTIG buffer
*   @param [out] ppKerBuf: Pointer to a WD_KERNEL_BUFFER pointer,
*                          to be filled by the function.
*                          The caller should use *ppBuf->pUserAddr usermode
*                          mapped address. When the buffer is no longer needed,
*                          (*ppBuf) should be passed to WDS_SharedBufferFree()
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @remarks
*          This function is currently only supported from the user mode.
*          This function is supported only for Windows and Linux.
*
* @snippet highlevel_examples.c WDS_SharedBufferAlloc
*/
DWORD DLLCALLCONV WDS_SharedBufferAlloc(_In_ UINT64 qwBytes,
    _In_ DWORD dwOptions, _Outptr_ WD_KERNEL_BUFFER **ppKerBuf);


/**
*  Utility macro that returns a kernel buffer global handle that can be used
*  for buffer sharing between multiple processes.
*
*   @param [in] pKerBuf: Pointer to a kernel buffer
*                        information structure
*
* @return
*  Returns a Kernel buffer handle of pKerBuf.
*/
#define WDS_SharedBufferGetGlobalHandle(pKerBuf) ((pKerBuf)->hKerBuf)

/**
*  Retrieves a shared buffer which was allocated by another process.
*
*   @param [in] hKerBuf:   Kernel buffer handle.
*   @param [out] ppKerBuf: Pointer to a pointer to a kernel buffer information
*                          structure, which is associated with hKerBuf
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*/
DWORD DLLCALLCONV WDS_SharedBufferGet(_In_ DWORD hKerBuf,
    _Outptr_ WD_KERNEL_BUFFER **ppKerBuf);


/**
*  Frees a shared buffer that was allocated by a previous call to
*  WDS_SharedBufferAlloc().
*
*   @param [in] pKerBuf: Pointer to a WD_KERNEL_BUF structure,
*                     received within the *ppBuf parameter of a previous
*                     call to WDS_SharedBufferAlloc()
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @remarks
*          This function is currently only supported from the user mode.
*          This function is supported only for Windows and Linux.
*
* @snippet highlevel_examples.c WDS_SharedBufferFree
*/
DWORD DLLCALLCONV WDS_SharedBufferFree(_In_ WD_KERNEL_BUFFER *pKerBuf);

/**
*  Enables the shared interrupts mechanism of WinDriver.
*  If the mechanism is already enabled globally (for all processes)
*  then the mechanism is enabled for the current process.
*
*   @param [in] pcProcessName: Optional process name string
*   @param [in] dwGroupID:     A unique group ID represent the specific
*                              application. Must be a positive ID
*   @param [in] dwSubGroupID:  Sub-group ID that should identify your user
*                              application type in case you have several types
*                              that may work simultaneously.
*                              Must be a positive ID
*   @param [in] dwAction:      IPC message type to receive,
*                              which can consist one of the enumeration
*                              values listed below:
*                               WD_IPC_UNICAST_MSG: Receive a message to a
*                               specific process with WinDriver IPC unique ID
*                               WD_IPC_MULTICAST_MSG: Receive a message from
*                               all processes that were registered with the
*                               same group ID as this process
*                               WD_IPC_ALL_MSG: Receive both types of the
*                               messages above
*   @param [in] pFunc:         A user-mode IPC message handler callback
*                              function, which will be called when a message
*                              was received by WinDriver from Shared Interrupts
*                              IPC process occurs.
*                              (See IPC_MSG_RX_HANDLER())
*   @param [in] pData:         Data for the user-mode IPC message handler
*                              callback routine (pFunc)
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet highlevel_examples.c WDS_SharedIntEnable
*/
DWORD DLLCALLCONV WDS_SharedIntEnable(_In_ const CHAR *pcProcessName,
    _In_ DWORD dwGroupID, _In_ DWORD dwSubGroupID, _In_ DWORD dwAction,
    _In_ IPC_MSG_RX_HANDLER pFunc, _In_ void *pData);

/**
*  Disables the Shared Interrupts mechanism of WinDriver for all processes.
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet highlevel_examples.c WDS_SharedIntDisableGlobal
*/
DWORD DLLCALLCONV WDS_SharedIntDisableGlobal(void);


/**
*  Disables the Shared Interrupts mechanism of WinDriver for the
*  current process.
*  This function does not disable the Shared Interrupts mechanism
*  for all processes.
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet highlevel_examples.c WDS_SharedIntDisableLocal
*
*/
DWORD DLLCALLCONV WDS_SharedIntDisableLocal(void);


/**
*  Check and returns whether shared interrupts are enabled
*  for the current process
*
* @return
*  TRUE if shared interrupts are enabled, else FALSE
*
*/
BOOL DLLCALLCONV WDS_IsSharedIntsEnabledLocally(void);

#ifdef __cplusplus
}
#endif

#endif /* _WDS_LIB_H_ */

