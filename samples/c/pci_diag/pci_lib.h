#ifndef _PCI_LIB_H_
#define _PCI_LIB_H_

/************************************************************************
*  File: pci_lib.h
*
*  Library for accessing PCI devices, possibly using a Kernel PlugIn driver.
*  The code accesses hardware using WinDriver's WDC library.

*
*  Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com
*************************************************************************/

#include "wdc_lib.h"
#include "wdc_defs.h"
#include "wdc_diag_lib.h"
#include "pci_regs.h"

#include "status_strings.h"
#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
  General definitions
 *************************************************************/
/* Kernel PlugIn driver name (should be no more than 8 characters) */
#define KP_PCI_DRIVER_NAME "KP_PCI"

/* Kernel PlugIn messages - used in WDC_CallKerPlug() calls (user mode) /
 * KP_PCI_Call() (kernel mode) */
enum {
    KP_PCI_MSG_VERSION = 1, /* Query the version of the Kernel PlugIn */
};

/* Kernel PlugIn messages status */
enum {
    KP_PCI_STATUS_OK          = 0x1,
    KP_PCI_STATUS_MSG_NO_IMPL = 0x1000,
    KP_PCI_STATUS_FAIL        = 0x1001,
};

/* Default vendor and device IDs (0 == all) */
/* TODO: Replace the ID values with your device's vendor and device IDs */
#define PCI_DEFAULT_VENDOR_ID 0x0 /* Vendor ID */
#define PCI_DEFAULT_DEVICE_ID 0x0 /* Device ID */



/* Interrupt acknowledgment information */
/* TODO: Use correct values according to the specification of your device. */
#define INTCSR 0x00                     /* Interrupt register */
#define INTCSR_ADDR_SPACE AD_PCI_BAR0   /* Interrupt register's address space */
#define ALL_INT_MASK 0xFFFFFFFF         /* Interrupt acknowledgment command */

/* Kernel PlugIn version information struct */
typedef struct {
    DWORD dwVer;
    CHAR cVer[100];
} KP_PCI_VERSION;

/* Device address description struct */
typedef struct {
    DWORD dwNumAddrSpaces;    /* Total number of device address spaces */
    PAD_TO_64(dwNumAddrSpaces)
    WDC_ADDR_DESC *pAddrDesc; /* Array of device address spaces information */
    PAD_TO_64(pAddrDesc)
} PCI_DEV_ADDR_DESC;

/* Address space information struct */
#define MAX_TYPE 8
typedef struct {
    DWORD dwAddrSpace;
    CHAR sType[MAX_TYPE];
    CHAR sName[MAX_NAME];
    CHAR sDesc[MAX_DESC];
} PCI_ADDR_SPACE_INFO;

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
} PCI_INT_RESULT;
/* TODO: You can add fields to PCI_INT_RESULT to store any additional
         information that you wish to pass to your diagnostics interrupt
         handler routine (DiagIntHandler() in pci_diag.c). */

/* PCI diagnostics interrupt handler function type */
typedef void (*PCI_INT_HANDLER)(WDC_DEVICE_HANDLE hDev,
    PCI_INT_RESULT *pIntResult);

/* PCI diagnostics plug-and-play and power management events handler function
 * type */
typedef void (*PCI_EVENT_HANDLER)(WDC_DEVICE_HANDLE hDev, DWORD dwAction);


typedef struct MENU_CTX_DMA {
    WDC_DEVICE_HANDLE *phDev;
    DWORD size;
    DWORD dwOptions;
    UINT64 qwAddr;
    PVOID pBuf;
    WD_DMA *pDma;
    DWORD dwDmaAddressWidth;
} MENU_CTX_DMA;

/*************************************************************
  Function prototypes
 *************************************************************/
/* -----------------------------------------------
    PCI and WDC libraries initialize/uninitialize
   ----------------------------------------------- */
/* Initialize the PCI and WDC libraries */
DWORD PCI_LibInit(void);
/* Uninitialize the PCI and WDC libraries */
DWORD PCI_LibUninit(void);

#ifndef __KERNEL__
/* -----------------------------------------------
    Device open/close
   ----------------------------------------------- */
/* Open a device handle */
WDC_DEVICE_HANDLE PCI_DeviceOpen(DWORD dwVendorId, DWORD dwDeviceId);
/* Close a device handle */
BOOL PCI_DeviceClose(WDC_DEVICE_HANDLE hDev);

#if !defined(ISA) && defined(LINUX)
/* -----------------------------------------------
    SRIOV
   ----------------------------------------------- */
/* Enable SRIOV */
DWORD PCI_SriovEnable(WDC_DEVICE_HANDLE hDev, DWORD dwNumVFs);
/* Disable SRIOV */
DWORD PCI_SriovDisable(WDC_DEVICE_HANDLE hDev);
/* Get and store number of virtual functions in *pdwNumVfs */
DWORD PCI_SriovGetNumVFs(WDC_DEVICE_HANDLE hDev, DWORD *pdwNumVFs);
#endif

/* -----------------------------------------------
    Interrupts
   ----------------------------------------------- */
/* Enable interrupts */
DWORD PCI_IntEnable(WDC_DEVICE_HANDLE hDev, PCI_INT_HANDLER funcIntHandler);
/* Disable interrupts */
DWORD PCI_IntDisable(WDC_DEVICE_HANDLE hDev);
/* Check whether interrupts are enabled for the given device */
BOOL PCI_IntIsEnabled(WDC_DEVICE_HANDLE hDev);

/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */
/* Register a plug-and-play or power management event */
DWORD PCI_EventRegister(WDC_DEVICE_HANDLE hDev,
    PCI_EVENT_HANDLER funcEventHandler);
/* Unregister a plug-and-play or power management event */
DWORD PCI_EventUnregister(WDC_DEVICE_HANDLE hDev);
/* Check whether a given plug-and-play or power management event is registered
 */
BOOL PCI_EventIsRegistered(WDC_DEVICE_HANDLE hDev);

/* -----------------------------------------------
    DMA
   ----------------------------------------------- */
DWORD PCI_DmaAllocContig(MENU_CTX_DMA *pDmaMenusCtx);
DWORD PCI_DmaAllocSg(MENU_CTX_DMA *pDmaMenusCtx);
DWORD PCI_DmaAllocReserved(MENU_CTX_DMA *pDmaMenusCtx);
DWORD PCI_DmaBufUnlock(WD_DMA *pDma);
#endif /* ifndef __KERNEL */

/* -----------------------------------------------
    Address spaces information
   ----------------------------------------------- */
/* Get number of address spaces */
DWORD PCI_GetNumAddrSpaces(WDC_DEVICE_HANDLE hDev);
/* Get address space information */
BOOL PCI_GetAddrSpaceInfo(WDC_DEVICE_HANDLE hDev,
    PCI_ADDR_SPACE_INFO *pAddrSpaceInfo);



/* -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- */
/* Get last error */
const char *PCI_GetLastErr(void);

#ifdef __cplusplus
}
#endif

#endif /* _PCI_LIB_H_ */


