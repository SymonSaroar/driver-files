/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _BMD_LIB_H_
#define _BMD_LIB_H_

/***************************************************************************
*  File: bmd_lib.h
*
*  Header file of a sample library for accessing Xilinx PCI Express cards
*  with BMD design, using the WinDriver WDC API.
*  The sample was tested with Xilinx's Virtex and Spartan development kits.
****************************************************************************/

#include "wdc_defs.h"
#include "utils.h"
#include "status_strings.h"
#include "wdc_diag_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
  General definitions
 *************************************************************/
/* Kernel PlugIn driver name (should be no more than 8 characters) */
#define KP_BMD_DRIVER_NAME "KP_BMD"

/* Kernel PlugIn messages - used in WDC_CallKerPlug() calls (user mode) /
 * KP_BMD_Call() (kernel mode) */
enum {
    KP_BMD_MSG_VERSION = 1, /* Query the version of the Kernel PlugIn */
};

/* Kernel PlugIn messages status */
enum {
    KP_BMD_STATUS_OK = 0x1,
    KP_BMD_STATUS_MSG_NO_IMPL = 0x1000,
};

/* Default vendor and device IDs (0 == all) */
#define BMD_DEFAULT_VENDOR_ID 0x10EE    /* Vendor ID */
#define BMD_DEFAULT_DEVICE_ID 0x0       /* All Xilinx devices */
  /* TODO: Change the device ID value to match your specific device. */

/* Direct Memory Access (DMA) information struct */
typedef struct {
    WD_DMA *pDma;
    WDC_DEVICE_HANDLE hDev;
} BMD_DMA_STRUCT, *BMD_DMA_HANDLE;

/* Kernel PlugIn version information struct */
typedef struct {
    DWORD dwVer;
    CHAR cVer[100];
} KP_BMD_VERSION;

/* Device address description struct */
typedef struct {
    DWORD dwNumAddrSpaces;    /* Total number of device address spaces */
    WDC_ADDR_DESC *pAddrDesc; /* Array of device address spaces information */
} BMD_DEV_ADDR_DESC;

/* Address space information struct */
#define MAX_TYPE 8
typedef struct {
    DWORD dwAddrSpace;
    CHAR sType[MAX_TYPE];
    CHAR sName[MAX_NAME];
    CHAR sDesc[MAX_DESC];
} BMD_ADDR_SPACE_INFO;

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
    PVOID pBuf;             /* Pointer to a user-mode DMA data buffer */
    UINT32 u32Pattern;      /* 32-bit data pattern (used for DMA data) */
    DWORD dwBufNumItems;    /* Size of the pBuf buffer, in units of UINT32 */
    BOOL fIsRead; /* DMA direction: host-to-device=read; device-to-host=write */
} BMD_INT_RESULT;
/* TODO: You can add fields to BMD_INT_RESULT to store any additional
         information that you wish to pass to your diagnostics interrupt
         handler routine (DiagIntHandler() in bmd_diag.c). */

/* BMD diagnostics interrupt handler function type */
typedef void (*BMD_INT_HANDLER)(WDC_DEVICE_HANDLE hDev,
    BMD_INT_RESULT *pIntResult);

/* BMD diagnostics plug-and-play and power management events handler
 * function type */
typedef void (*BMD_EVENT_HANDLER)(WDC_DEVICE_HANDLE hDev, DWORD dwAction);

/* BMD device information struct */
typedef struct {
    BMD_INT_HANDLER funcDiagIntHandler;     /* Interrupt handler routine */
    BMD_EVENT_HANDLER funcDiagEventHandler; /* Event handler routine */
    BMD_DMA_HANDLE hDma;    /* Handle to a BMD DMA information struct */
    PVOID pBuf;             /* Pointer to a user-mode DMA data buffer */
    BOOL fIsRead; /* DMA direction: host-to-device=read; device-to-host=write */
    UINT32 u32Pattern;      /* 32-bit data pattern (used for DMA data) */
    DWORD dwBufNumItems;    /* Size of the pBuf buffer, in units of UINT32 */
} BMD_DEV_CTX, *PBMD_DEV_CTX;
/* TODO: You can add fields to store additional device-specific information. */


/* BMD registers address space (BAR 0) */
#define BMD_SPACE AD_PCI_BAR0
/* BMD registers offsets */
enum {
    BMD_DSCR_OFFSET = 0x0,
    BMD_DDMACR_OFFSET = 0x4,
    BMD_WDMATLPA_OFFSET = 0x8,
    BMD_WDMATLPS_OFFSET = 0xc,
    BMD_WDMATLPC_OFFSET = 0x10,
    BMD_WDMATLPP_OFFSET = 0x14,
    BMD_RDMATLPP_OFFSET = 0x18,
    BMD_RDMATLPA_OFFSET = 0x1c,
    BMD_RDMATLPS_OFFSET = 0x20,
    BMD_RDMATLPC_OFFSET = 0x24,
    BMD_WDMAPERF_OFFSET = 0x28,
    BMD_RDMAPERF_OFFSET = 0x2c,
    BMD_RDMASTAT_OFFSET = 0x30,
    BMD_NRDCOMP_OFFSET = 0x34,
    BMD_RCOMPDSIZE_OFFSET = 0x38,
    BMD_DLWSTAT_OFFSET = 0x3c,
    BMD_DLTRSSTAT_OFFSET = 0x40,
    BMD_DMISCCONT_OFFSET = 0x44
};

/*************************************************************
  Function prototypes
 *************************************************************/
/* -----------------------------------------------
    BMD and WDC libraries initialize/uninitialize
   ----------------------------------------------- */
/* Initialize the Xilinx BMD and WDC libraries */
DWORD BMD_LibInit(void);
/* Uninitialize the Xilinx BMD and WDC libraries */
DWORD BMD_LibUninit(void);

#if !defined(__KERNEL__)
/* -----------------------------------------------
    Device open/close
   ----------------------------------------------- */
WDC_DEVICE_HANDLE BMD_DeviceOpen(DWORD dwVendorId, DWORD dwDeviceId);
/* Close a device handle */
BOOL BMD_DeviceClose(WDC_DEVICE_HANDLE hDev);

#ifdef HAS_INTS
/* -----------------------------------------------
    Interrupts
   ----------------------------------------------- */
/* Enable interrupts */
DWORD BMD_IntEnable(WDC_DEVICE_HANDLE hDev, BMD_INT_HANDLER funcIntHandler);
/* Disable interrupts */
DWORD BMD_IntDisable(WDC_DEVICE_HANDLE hDev);
/* Check whether interrupts are enabled for the given device */
BOOL BMD_IntIsEnabled(WDC_DEVICE_HANDLE hDev);
#endif /* ifdef HAS_INTS */
/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */
/* Register a plug-and-play or power management event */
DWORD BMD_EventRegister(WDC_DEVICE_HANDLE hDev,
    BMD_EVENT_HANDLER funcEventHandler);
/* Unregister a plug-and-play or power management event */
DWORD BMD_EventUnregister(WDC_DEVICE_HANDLE hDev);
/* Check whether a given plug-and-play or power management event is registered
 */
BOOL BMD_EventIsRegistered(WDC_DEVICE_HANDLE hDev);

/* -----------------------------------------------
    Direct Memory Access (DMA)
   ----------------------------------------------- */
/* Open a DMA handle: Allocate and initialize a BMD DMA information structure,
 * including allocation of a contiguous DMA buffer */
DWORD BMD_DmaOpen(WDC_DEVICE_HANDLE hDev, PVOID *ppBuf, DWORD dwOptions,
    DWORD dwBytes, BMD_DMA_HANDLE *ppDmaHandle);
/* Close a DMA handle: Unlock and free a DMA buffer and the containing BMD DMA
 * information structure */
BOOL BMD_DmaClose(BMD_DMA_HANDLE hDma);
/* Prepare a device for a DMA transfer */
/* NOTE: Call this function after BMD_DmaOpen() and before BMD_DmaStart(). */
BOOL BMD_DmaDevicePrepare(BMD_DMA_HANDLE hDma, BOOL fIsRead, WORD wTLPNumItems,
    WORD dwNumItems, UINT32 u32Pattern, BOOL fEnable64bit, BYTE bTrafficClass);
/* Get maximum DMA packet size */
WORD BMD_DmaGetMaxPacketSize(WDC_DEVICE_HANDLE hDev, BOOL fIsRead);

/* Synchronize all CPU caches with the DMA buffer */
DWORD BMD_DmaSyncCpu(BMD_DMA_HANDLE hDma);
/* Synchronize the I/O caches with the DMA buffer */
DWORD BMD_DmaSyncIo(BMD_DMA_HANDLE hDma);

/* Start a DMA transfer */
BOOL BMD_DmaStart(BMD_DMA_HANDLE hDma, BOOL fIsRead);
/* Detect the completion of a DMA transfer */
BOOL BMD_DmaIsDone(WDC_DEVICE_HANDLE hDev, BOOL fIsRead);
/* Poll for DMA transfer completion */
BOOL BMD_DmaPollCompletion(BMD_DMA_HANDLE hDma, BOOL fIsRead);
/* Verify success of a host-to-device (read) DMA transfer */
BOOL BMD_DmaIsReadSucceed(WDC_DEVICE_HANDLE hDev);

/* Enable DMA interrupts */
BOOL BMD_DmaIntEnable(WDC_DEVICE_HANDLE hDev, BOOL fIsRead);
/* Disable DMA interrupts */
BOOL BMD_DmaIntDisable(WDC_DEVICE_HANDLE hDev, BOOL fIsRead);

/* -----------------------------------------------
    Read/write registers
   ----------------------------------------------- */
/**
 * Function name:  BMD_ReadReg8 / BMD_ReadReg16 / BMD_ReadReg32
 * Description:    Read from a 8/16/32-bit register
 * Parameters:
 *     @hDev:     (IN) Handle to the card, received from BMD_DeviceOpen()
 *     @dwOffset: (IN) Offset of the register within the BMD_SPACE address space
 * Return values:  Read data
 * Scope: Global
 **/
BYTE BMD_ReadReg8(WDC_DEVICE_HANDLE hDev, DWORD dwOffset);
WORD BMD_ReadReg16(WDC_DEVICE_HANDLE hDev, DWORD dwOffset);
UINT32 BMD_ReadReg32(WDC_DEVICE_HANDLE hDev, DWORD dwOffset);

/**
 * Function name:  BMD_WriteReg8 / BMD_WriteReg16 / BMD_WriteReg32
 * Description:    Write to a 8/16/32-bit register
 * Parameters:
 *     @hDev:     (IN) Handle to the card, received from BMD_DeviceOpen()
 *     @dwOffset: (IN) Offset of the register within the BMD_SPACE address space
 *     @bData/@wData/@u32Data:
                  (IN) Data to write to the register (8/16/32 bits)
 * Return values:  None
 * Scope: Global
 **/
void BMD_WriteReg8(WDC_DEVICE_HANDLE hDev, DWORD dwOffset, BYTE bData);
void BMD_WriteReg16(WDC_DEVICE_HANDLE hDev, DWORD dwOffset, WORD wData);
void BMD_WriteReg32(WDC_DEVICE_HANDLE hDev, DWORD dwOffset, UINT32 u32Data);
#endif

/* -----------------------------------------------
    Address spaces information
   ----------------------------------------------- */
/* Get number of address spaces */
DWORD BMD_GetNumAddrSpaces(WDC_DEVICE_HANDLE hDev);
/* Get address space information */
BOOL BMD_GetAddrSpaceInfo(WDC_DEVICE_HANDLE hDev,
    BMD_ADDR_SPACE_INFO *pAddrSpaceInfo);


/* -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- */
/* Get last error */
const char *BMD_GetLastErr(void);

#ifdef __cplusplus
}
#endif

#endif /* _BMD_LIB_H_ */

