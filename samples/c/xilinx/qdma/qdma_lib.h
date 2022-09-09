/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _QDMA_LIB_H_
#define _QDMA_LIB_H_

/***************************************************************************
*  File: qdma_lib.h
*
*  Header file of a sample library for accessing Xilinx PCI Express cards
*  with QDMA design, using the WinDriver WDC API.
****************************************************************************/

#include "wdc_defs.h"
#include "diag_lib.h"

/*************************************************************
  General definitions
 *************************************************************/

#define MAX_PROCESSORS 16

#define QDMA_DEV_PF     0
#define QDMA_DEV_VF     1

#define QDMA_NUM_PF_FUNC        4
#define QDMA_MAX_QUEUES_PER_PF  512
#define QDMA_TRANSACTION_SAMPLE_MAX_TRANSFER_SIZE 0X0FFFFFFF

#if defined(UNIX)
    #define ULONG_MAX     0xffffffffUL  // maximum unsigned long value
#endif

typedef enum {
    QUEUE_STATE_QUEUE_AVAILABLE,
    QUEUE_STATE_QUEUE_PROGRAMMED,
    QUEUE_STATE_QUEUE_STARTED,
    QUEUE_STATE_QUEUE_MAX
} QUEUE_STATE;

typedef enum {
    QUEUE_OPERATION_ADD,
    QUEUE_OPERATION_REMOVE,
    QUEUE_OPERATION_START,
    QUEUE_OPERATION_STOP,
} QUEUE_OPERATION;

typedef enum {
    DMA_REQUEST_STATUS_UNITIALIZED,
    DMA_REQUEST_STATUS_STARTED,
    DMA_REQUEST_STATUS_FINISHED,
    DMA_REQUEST_STATUS_ERROR,
} DMA_REQUEST_STATUS;

typedef struct {
    UINT32 h2cRingSizeIdx : 4;
    UINT32 c2hRingSizeIdx : 4;
    UINT32 c2hBufferSizeIdx : 4;
    UINT32 c2hThCntIdx : 4;
    UINT32 c2hTimerCntIdx : 4;
    UINT32 fIsSt : 1;
    UINT32 comtSize : 2;
    UINT32 triggerMode : 4;
    UINT32 descBypassEnabled : 1;
    UINT32 pfchEnabled : 1;
    UINT32 pfchBypassEnabled : 1;
    UINT32 unused : 2;
    /* TODO: Add more queue configurable parameters */
} QUEUE_CONFIG;

typedef struct {
    WDC_DEVICE_HANDLE hDev;    /* Device handle */
    WD_DMA *pDma;              /* Pointer to a DMA information structure */
    PVOID pBuf;                /* Pointer to a user-mode DMA data buffer */
    DWORD dwBytes;             /* Size of the pBuf DMA buffer, in bytes */
    BOOL fIsH2C;               /* Is host to card direction? */
    UINT32 dwQueueId;          /* Queue index */
    UINT64 u64Offset;          /* Device offset */
    DMA_REQUEST_STATUS status; /* DMA status */
    TIME_TYPE timeStart;       /* DMA start time */
    TIME_TYPE timeEnd;         /* DMA end time */
} DMA_REQUEST_CONTEXT;

/*************************************************************
  Function prototypes
 *************************************************************/
 /* -----------------------------------------------
     QDMA and WDC libraries initialize/uninitialize
    ----------------------------------------------- */

/* Initialize the Xilinx QDMA and WDC libraries */
DWORD QDMA_LibInit(const CHAR *sLicense);
/* Uninitialize the Xilinx QDMA and WDC libraries */
DWORD QDMA_LibUninit(void);

/* Address space information struct */
#define MAX_TYPE 8
typedef struct {
    DWORD dwAddrSpace;
    CHAR sType[MAX_TYPE];
    CHAR sName[MAX_NAME];
    CHAR sDesc[MAX_DESC];
} QDMA_ADDR_SPACE_INFO;

typedef void *QDMA_DMA_HANDLE;

/* Interrupt result information struct */
typedef struct
{
    DWORD dwCounter;        /* Number of interrupts received */
    DWORD dwLost;           /* Number of interrupts not yet handled */
    WD_INTERRUPT_WAIT_RESULT waitResult; /* See WD_INTERRUPT_WAIT_RESULT values
                                            in windrvr.h */
    BOOL fIsMessageBased;
    DWORD dwLastMessage;    /* Message data of the last received MSI/MSI-X
                             * (Windows Vista and higher); N/A to line-based
                             * interrupts) */
    UINT32 u32DmaStatus;    /* Status of the completed DMA transfer */
    UINT32 u32IntStatus;    /* Interrupt status */
    QDMA_DMA_HANDLE hDma;   /* Completed DMA handle */
    PVOID pData;            /* Custom context */
} QDMA_INT_RESULT;

/* TODO: You can add fields to QDMA_INT_RESULT to store any additional
         information that you wish to pass to your diagnostics interrupt
         handler routine (DiagIntHandler() in qdma_diag.c). */

/* QDMA diagnostics interrupt handler function type */
typedef void (*QDMA_INT_HANDLER)(WDC_DEVICE_HANDLE hDev,
    QDMA_INT_RESULT *pIntResult);

/* QDMA diagnostics plug-and-play and power management events handler
 * function type */
typedef void (*QDMA_EVENT_HANDLER)(WDC_DEVICE_HANDLE hDev, DWORD dwAction);

#if !defined(__KERNEL__)

/* -----------------------------------------------
    Device open/close
   ----------------------------------------------- */

BOOL QDMA_DeviceInit(WDC_DEVICE_HANDLE hDev);
/* Open a device handle */
WDC_DEVICE_HANDLE QDMA_DeviceOpen(DWORD dwVendorID, DWORD dwDeviceID);
/* Close a device handle */
BOOL QDMA_DeviceClose(WDC_DEVICE_HANDLE hDev);
/* Get vendor id by device handle */
DWORD QDMA_GetVendorId(WDC_DEVICE_HANDLE hDev);
/* Get device id by device handle */
DWORD QDMA_GetDeviceId(WDC_DEVICE_HANDLE hDev);

/* -----------------------------------------------
    Direct Memory Access (DMA)
   ----------------------------------------------- */
/* Add queue */
DWORD QDMA_AddQueue(WDC_DEVICE_HANDLE hDev, DWORD dwQueueId,
    QUEUE_CONFIG *queueConfig);
/* Start queue */
DWORD QDMA_StartQueue(WDC_DEVICE_HANDLE hDev, DWORD dwQueueId);
/* Remove queue */
DWORD QDMA_RemoveQueue(WDC_DEVICE_HANDLE hDev, DWORD dwQueueId);
/* Stop queue */
DWORD QDMA_StopQueue(WDC_DEVICE_HANDLE hDev, DWORD dwQueueId);
/* Input/Output (read/write) function */
DWORD QDMA_IoMmDma(DMA_REQUEST_CONTEXT *dmaRequestContext);
/* Get queue queue */
DWORD QDMA_GetQueueState(WDC_DEVICE_HANDLE hDev, DWORD dwQueueId,
    QUEUE_STATE *state);
/* Clear all queues */
void QDMA_QueuesClear(WDC_DEVICE_HANDLE hDev);
/* Get the required queue queueState that the queue needs to be in order to
 * perform the operation */
QUEUE_STATE QDMA_GetRequiredStateByOperation(QUEUE_OPERATION operation);

/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */

/* Register a plug-and-play or power management event */
DWORD QDMA_EventRegister(WDC_DEVICE_HANDLE hDev,
    QDMA_EVENT_HANDLER funcEventHandler);
/* Unregister a plug-and-play or power management event */
DWORD QDMA_EventUnregister(WDC_DEVICE_HANDLE hDev);
/* Check whether a given plug-and-play or power management event is registered
 */
BOOL QDMA_EventIsRegistered(WDC_DEVICE_HANDLE hDev);

#endif

/* -----------------------------------------------
    Address spaces information
   ----------------------------------------------- */
/* Get number of address spaces */
DWORD QDMA_GetNumAddrSpaces(WDC_DEVICE_HANDLE hDev);
/* Get address space information */
BOOL QDMA_GetAddrSpaceInfo(WDC_DEVICE_HANDLE hDev,
    QDMA_ADDR_SPACE_INFO *pAddrSpaceInfo);
/* Get configuration BAR number */
DWORD QDMA_ConfigBarNumGet(WDC_DEVICE_HANDLE hDev);

/* -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- */
   /* Get last error */
const char *QDMA_GetLastErr(void);

#endif /* _QDMA_LIB_H_ */



