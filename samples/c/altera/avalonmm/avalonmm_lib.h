/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _AVALONMM_LIB_H_
#define _AVALONMM_LIB_H_

/************************************************************************
*  File: avalonmm_lib.h
*
*  Header file of a sample library for accessing Altera PCI Express cards
*  with Avalon-MM design, using the WinDriver WDC API.
*************************************************************************/

#include "wdc_lib.h"
#include "wdc_defs.h"
#include "wdc_diag_lib.h"
#include "pci_regs.h"

#include "status_strings.h"
#include "avalonmm_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
  General definitions
 *************************************************************/
/* Default vendor and device IDs (0 == all) */
/* TODO: Replace the ID values with your device's vendor and device IDs */
#define AVALONMM_DEFAULT_VENDOR_ID 0x1172 /* Vendor ID */
#define AVALONMM_DEFAULT_DEVICE_ID 0xE003 /* Device ID */


#define DMA_ADDR_LOW(addr)   ((UINT32)((addr) & 0xffffffff))
#define DMA_ADDR_HIGH(addr)  ((UINT32)((addr) >> 32))

typedef struct MENU_CTX_DMA {
    WDC_DEVICE_HANDLE *phDev;
    BOOL fToDevice;
    BOOL fRunVerificationCheck;
    BOOL fIsDmaExecuted;
    DWORD dwBytes;
    DWORD dwOptions;
    DWORD dwNumPackets;
    UINT32 u32Pattern;
    UINT64 u64FPGAOffset;

    WD_DMA *pDmaBuffer;
    PVOID pBuf;

    WD_DMA *pDmaDescriptors;
    struct AVALONMM_DESCRIPTOR_TABLE *pDescriptorTable;
} MENU_CTX_DMA;

/* Address space information struct */
#define MAX_TYPE 8
typedef struct {
    DWORD dwAddrSpace;
    CHAR sType[MAX_TYPE];
    CHAR sName[MAX_NAME];
    CHAR sDesc[MAX_DESC];
} AVALONMM_ADDR_SPACE_INFO;

#ifdef HAS_INTS
/* Interrupt result information struct */
typedef struct {
    DWORD dwCounter; /* Number of interrupts received */
    DWORD dwLost;    /* Number of interrupts not yet handled */
    WD_INTERRUPT_WAIT_RESULT waitResult; /* See WD_INTERRUPT_WAIT_RESULT values
                                            in windrvr.h */
    DWORD dwEnabledIntType; /* Interrupt type that was actually enabled
                               (MSI/MSI-X / Level Sensitive / Edge-Triggered) */
    DWORD dwLastMessage; /* Message data of the last received MSI/MSI-X
                            (Windows Vista and higher); N/A to line-based
                            interrupts. */
} AVALONMM_INT_RESULT;
/* TODO: You can add fields to AVALONMM_INT_RESULT to store any additional
         information that you wish to pass to your diagnostics interrupt
         handler routine (DiagIntHandler() in AVALONMM_diag.c). */

/* AVALONMM diagnostics interrupt handler function type */
typedef void (*AVALONMM_INT_HANDLER)(WDC_DEVICE_HANDLE hDev,
    AVALONMM_INT_RESULT *pIntResult);
#endif /* ifdef HAS_INTS */

/* AVALONMM diagnostics plug-and-play and power management events handler
 * function type */
typedef void (*AVALONMM_EVENT_HANDLER)(WDC_DEVICE_HANDLE hDev, DWORD dwAction);


/*************************************************************
  Function prototypes
 *************************************************************/
/* -----------------------------------------------
    AVALONMM and WDC libraries initialize/uninitialize
   ----------------------------------------------- */
/* Initialize the AVALONMM and WDC libraries */
DWORD AVALONMM_LibInit(void);
/* Uninitialize the AVALONMM and WDC libraries */
DWORD AVALONMM_LibUninit(void);

#ifndef __KERNEL__
/* -----------------------------------------------
    Device open/close
   ----------------------------------------------- */
/* Open a device handle */
WDC_DEVICE_HANDLE AVALONMM_DeviceOpen(DWORD dwVendorId, DWORD dwDeviceId);
/* Close a device handle */
BOOL AVALONMM_DeviceClose(WDC_DEVICE_HANDLE hDev);

#ifdef HAS_INTS
/* -----------------------------------------------
    Interrupts
   ----------------------------------------------- */
/* Enable interrupts */
DWORD AVALONMM_IntEnable(WDC_DEVICE_HANDLE hDev,
    AVALONMM_INT_HANDLER funcIntHandler);
/* Disable interrupts */
DWORD AVALONMM_IntDisable(WDC_DEVICE_HANDLE hDev);
/* Check whether interrupts are enabled for the given device */
BOOL AVALONMM_IntIsEnabled(WDC_DEVICE_HANDLE hDev);
#endif /* ifdef HAS_INTS */

/* -----------------------------------------------
    Direct Memory Access (DMA)
   ----------------------------------------------- */
/* Initialize DMA transaction */
DWORD AVALONMM_DmaInit(MENU_CTX_DMA *pDmaMenusCtx);
/* Execute DMA transaction */
DWORD AVALONMM_DmaTransactionExecute(MENU_CTX_DMA *pDmaMenusCtx);
/* Release DMA transaction */
DWORD AVALONMM_DmaTransactionRelease(MENU_CTX_DMA *pDmaMenusCtx);
/* Mark DMA transfer as completed */
DWORD AVALONMM_DmaTransactionTransferEnded(MENU_CTX_DMA *pDmaMenusCtx);
/* Free DMA memory */
void AVALONMM_FreeDmaMem(MENU_CTX_DMA *pDmaMenusCtx);

/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */
/* Register a plug-and-play or power management event */
DWORD AVALONMM_EventRegister(WDC_DEVICE_HANDLE hDev,
    AVALONMM_EVENT_HANDLER funcEventHandler);
/* Unregister a plug-and-play or power management event */
DWORD AVALONMM_EventUnregister(WDC_DEVICE_HANDLE hDev);
/* Check whether a given plug-and-play or power management event is registered
 */
BOOL AVALONMM_EventIsRegistered(WDC_DEVICE_HANDLE hDev);
#endif /* ifndef __KERNEL */

/* -----------------------------------------------
    Address spaces information
   ----------------------------------------------- */
/* Get number of address spaces */
DWORD AVALONMM_GetNumAddrSpaces(WDC_DEVICE_HANDLE hDev);
/* Get address space information */
BOOL AVALONMM_GetAddrSpaceInfo(WDC_DEVICE_HANDLE hDev,
    AVALONMM_ADDR_SPACE_INFO *pAddrSpaceInfo);

/* -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- */
/* Get last error */
const char *AVALONMM_GetLastErr(void);

#ifdef __cplusplus
}
#endif

#endif /* _AVALONMM_LIB_H_ */


