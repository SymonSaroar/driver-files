/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _QSYS_LIB_H_
#define _QSYS_LIB_H_

/*****************************************************************************
*  File: qsys_lib.h
*
*  Header file of a sample library for accessing Altera PCI Express cards
*  with Qsys design, using the WinDriver WDC_API.
*  The sample was tested with Altera's Stratix IV GX development kit.
*  For more information on the Qsys design, refer to Altera's
*  "PCI Express in Qsys Example Designs" wiki page:
*  http://alterawiki.com/wiki/PCI_Express_in_Qsys_Example_Designs
*  You can download the WinDriver Development kit from here:
*  https://www.jungo.com/st/do/download_new.php?product=WinDriver&tag=GrandMenu
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
******************************************************************************/

#include "wdc_lib.h"
#include "wdc_defs.h"
#include "pci_regs.h"
#include "bits.h"
#include "wdc_diag_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
  General definitions
 *************************************************************/
/* Kernel PlugIn driver name (should beno more than 8 characters) */
#define KP_QSYS_DRIVER_NAME "KP_QSYS"

/* Kernel PlugIn messages - used in WDC_CallKerPlug() calls (user mode) /
 * KP_QSYS_Call() (kernel mode) */
enum {
    KP_QSYS_MSG_VERSION = 1, /* Query the version of the Kernel PlugIn */
};

/* Kernel PlugIn messages status */
enum {
    KP_QSYS_STATUS_OK = 0x1,
    KP_QSYS_STATUS_MSG_NO_IMPL = 0x1000,
};

/* Default vendor and device IDs (0 == all) */
#define QSYS_DEFAULT_VENDOR_ID 0x1172 /* Vendor ID */
#define QSYS_DEFAULT_DEVICE_ID 0x0    /* All Altera devices */
  /* TODO: Change the device ID value to match your specific device. */

/* Qsys revision */
#define QSYS_REVISION               9

/* Number of DMA descriptors */
#define QSYS_DMA_NUM_DESCRIPTORS    2

/* DMA item size, in bytes */
#define QSYS_DMA_ITEM_BYTES     sizeof(UINT32)              /* 4 bytes */
/* DMA packet size, in bytes */
#define QSYS_DMA_PKT_BYTES      32                          /* 32 bytes */
/* DMA packet size, in number of UINT32 items */
#define QSYS_DMA_PKT_NUM_ITEMS  QSYS_DMA_PKT_BYTES / QSYS_DMA_ITEM_BYTES
                                                            /* 8 UINT32 items */

/* Qsys DMA transfer completion detection descriptor size: */
/* Descriptor size in number of packets */
#define QSYS_DMA_DONE_NUM_PKTS  1                           /* 1 packet */
/* Descriptor size in number of UINT32 items */
#define QSYS_DMA_DONE_NUM_ITEMS QSYS_DMA_DONE_NUM_PKTS * QSYS_DMA_PKT_NUM_ITEMS
                                                            /* 8 UINT32 items */
/* Descriptor size in bytes */
#define QSYS_DMA_DONE_BYTES     QSYS_DMA_DONE_NUM_PKTS * QSYS_DMA_PKT_BYTES
                                                            /* 32 bytes */

/* Qsys DMA transfer completion detection data size (the portion of the DMA
 * transfer completion detection descriptor used to verify DMA completion): */
/* Completion-done data size in number of UINT32 items */
# define QSYS_DMA_DONE_DETECT_NUM_ITEMS 1                   /* 1 UINT32 item */
/* Completion-done data size in bytes */
#define QSYS_DMA_DONE_DETECT_BYTES \
    QSYS_DMA_DONE_DETECT_NUM_ITEMS * QSYS_DMA_ITEM_BYTES    /* 4 bytes */

/* Qsys BAR access for DMA */
#define QSYS_DMA_BAR_ACCESS 0x07000000

/* Direct Memory Access (DMA) information struct */
typedef struct {
    WD_DMA *pDma;           /* Pointer to a DMA information structure */
    WDC_DEVICE_HANDLE hDev; /* WDC device handle */
    PVOID pBuf;             /* Pointer to a user-mode DMA data buffer */
    DWORD dwBufSize;        /* Size of the pBuf DMA buffer, in bytes */
    DWORD dwTargetAddr;     /* Target device DMA address */
    /* Array of mSGDMA descriptor sizes, in bytes */
    DWORD dwDescrptSizes[QSYS_DMA_NUM_DESCRIPTORS];
    /* Array of mSGDMA descriptor host-to-device (read) DMA base addresses */
    DWORD dwReadAddresses[QSYS_DMA_NUM_DESCRIPTORS];
    /* Array of mSGDMA descriptor device-to-host (write) DMA base addresses */
    DWORD dwWriteAddresses[QSYS_DMA_NUM_DESCRIPTORS];
} QSYS_DMA_STRUCT, *QSYS_PDMA_STRUCT;

/* Kernel PlugIn version information struct */
typedef struct {
    DWORD dwVer;
    CHAR cVer[100];
} KP_QSYS_VERSION;

/* Device address description struct */
typedef struct {
    DWORD dwNumAddrSpaces;    /* Total number of device address spaces */
    WDC_ADDR_DESC *pAddrDesc; /* Array of device address spaces information */
} QSYS_DEV_ADDR_DESC;

/* Address space information struct */
#define MAX_TYPE 8
typedef struct {
    DWORD dwAddrSpace;
    CHAR sType[MAX_TYPE];
    CHAR sName[MAX_NAME];
    CHAR sDesc[MAX_DESC];
} QSYS_ADDR_SPACE_INFO;

/* Interrupt result information struct */
typedef struct
{
    DWORD dwCounter; /* Number of interrupts received */
    DWORD dwLost;    /* Number of interrupts not yet handled */
    WD_INTERRUPT_WAIT_RESULT waitResult; /* See WD_INTERRUPT_WAIT_RESULT values
                                            in windrvr.h */
    DWORD dwEnabledIntType; /* Interrupt type that was actually enabled
                               (MSI/MSI-X/Level Sensitive/Edge-Triggered) */
    DWORD dwLastMessage; /* Message data of the last received MSI/MSI-X
                            (Windows Vista and higher); N/A to line-based
                            interrupts) */
} QSYS_INT_RESULT;
/* TODO: You can add fields to QSYS_INT_RESULT to store any additional
         information that you wish to pass to your diagnostics interrupt
         handler routine (DiagIntHandler() in qsys_diag.c). */

/* Qsys diagnostics interrupt handler function type */
typedef void (*QSYS_INT_HANDLER)(WDC_DEVICE_HANDLE hDev,
    QSYS_INT_RESULT *pIntResult);

/* Qsys diagnostics plug-and-play and power management events handler function
 * type */
typedef void (*QSYS_EVENT_HANDLER)(WDC_DEVICE_HANDLE hDev,
    DWORD dwAction);


/*************************************************************
  Function prototypes
 *************************************************************/
/* -----------------------------------------------
    Qsys and WDC libraries initialize/uninitialize
   ----------------------------------------------- */
/* Initialize the Altera Qsys and WDC libraries */
DWORD QSYS_LibInit(void);
/* Uninitialize the Altera Qsys and WDC libraries */
DWORD QSYS_LibUninit(void);

/* -----------------------------------------------
    Device open/close
   ----------------------------------------------- */
/* Open a device handle */
WDC_DEVICE_HANDLE QSYS_DeviceOpen(DWORD dwVendorID, DWORD dwDeviceID);
/* Close a device handle */
BOOL QSYS_DeviceClose(WDC_DEVICE_HANDLE hDev);

#ifdef HAS_INTS
/* -----------------------------------------------
    Interrupts
   ----------------------------------------------- */
/* Enable interrupts */
DWORD QSYS_IntEnable(WDC_DEVICE_HANDLE hDev,
    QSYS_INT_HANDLER funcIntHandler);
/* Disable interrupts */
DWORD QSYS_IntDisable(WDC_DEVICE_HANDLE hDev);
/* Check whether interrupts are enabled for the given device */
BOOL QSYS_IntIsEnabled(WDC_DEVICE_HANDLE hDev);
#endif /* ifdef HAS_INTS */

/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */
/* Register a plug-and-play or power management event */
DWORD QSYS_EventRegister(WDC_DEVICE_HANDLE hDev,
    QSYS_EVENT_HANDLER funcEventHandler);
/* Unregister a plug-and-play or power management event */
DWORD QSYS_EventUnregister(WDC_DEVICE_HANDLE hDev);
/* Check whether a given plug-and-play or power management event is registered
 */
BOOL QSYS_EventIsRegistered(WDC_DEVICE_HANDLE hDev);

/* -----------------------------------------------
    Qsys Revision Verification
   ----------------------------------------------- */
/* Verify Qsys revision */
BOOL QSYS_IsQsysRevision(WDC_DEVICE_HANDLE hDev);

/* -----------------------------------------------
    Direct Memory Access (DMA)
   ----------------------------------------------- */
/* Open a DMA handle: Allocate a contiguous DMA buffer and initialize the given
 * Qsys DMA information structure */
DWORD QSYS_DmaOpen(WDC_DEVICE_HANDLE hDev, QSYS_PDMA_STRUCT pDma,
    DWORD dwOptions, DWORD dwNumTransItems);
/* Close a DMA handle: Unlock and free a DMA buffer */
BOOL QSYS_DmaClose(QSYS_PDMA_STRUCT pDma);
/* Prepare a device for a DMA transfer */
/* NOTE: Call this function after QSYS_DmaOpen() and before QSYS_DmaStart().
 */
BOOL QSYS_DmaDevicePrepare(QSYS_PDMA_STRUCT pDma, BOOL fIsToDevice);

/* Synchronize all CPU caches with the DMA buffer */
DWORD QSYS_DmaSyncCpu(QSYS_PDMA_STRUCT pDma);
/* Synchronize the I/O caches with the DMA buffer */
DWORD QSYS_DmaSyncIo(QSYS_PDMA_STRUCT pDma);

/* Start a DMA transfer */
BOOL QSYS_DmaStart(QSYS_PDMA_STRUCT pDma, BOOL fIsToDevice);
/* Detect the completion of a DMA transfer */
BOOL QSYS_DmaIsDone(QSYS_PDMA_STRUCT pDma, UINT32 u32ExpectedData);
/* Poll for DMA transfer completion */
BOOL QSYS_DmaPollCompletion(QSYS_PDMA_STRUCT pDma, UINT32 u32ExpectedData);
/* Verify DMA transfer completion in the device */
BOOL QSYS_DmaTransferVerify(WDC_DEVICE_HANDLE hDev);

/* -----------------------------------------------
    Read register
   ----------------------------------------------- */
/**
 * Function name:  QSYS_ReadReg8 / QSYS_ReadReg16 / QSYS_ReadReg32
 * Description:    Read from a 8/16/32-bit register
 * Parameters:
 *     @hDev:        (IN) Handle to the card, received from QSYS_DeviceOpen()
 *     @dwAddrSpace: (IN) Address space containing the register
 *     @dwOffset:    (IN) Offset of the register within the address space
 * Return values:  Read data
 * Scope: Global
 **/
BYTE QSYS_ReadReg8(WDC_DEVICE_HANDLE hDev, DWORD dwAddrSpace, DWORD dwOffset);
WORD QSYS_ReadReg16(WDC_DEVICE_HANDLE hDev, DWORD dwAddrSpace, DWORD dwOffset);
UINT32 QSYS_ReadReg32(WDC_DEVICE_HANDLE hDev, DWORD dwAddrSpace,
    DWORD dwOffset);

/* -----------------------------------------------
    Address spaces information
   ----------------------------------------------- */
/* Get number of address spaces */
DWORD QSYS_GetNumAddrSpaces(WDC_DEVICE_HANDLE hDev);
/* Get address space information */
BOOL QSYS_GetAddrSpaceInfo(WDC_DEVICE_HANDLE hDev,
    QSYS_ADDR_SPACE_INFO *pAddrSpaceInfo);



/* -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- */
/* Get last error */
const char *QSYS_GetLastErr(void);

#ifdef __cplusplus
}
#endif

#endif /* _QSYS_LIB_H_ */

