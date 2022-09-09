/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/***************************************************************************
*  File: lscdma_lib.h
*
*  Header file of a sample library for accessing Lattice PCI Express cards
*  with SGDMA support, using the WinDriver WDC API.
****************************************************************************/

#ifndef _LSCDMA_LIB_H_
#define _LSCDMA_LIB_H_

#include "wdc_defs.h"
#include "status_strings.h"
#include "wdc_diag_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
  General definitions
 *************************************************************/
/* Kernel PlugIn driver name (should be no more than 8 characters) */
#define KP_LSCDMA_DRIVER_NAME "KP_LSCDMA"

/* Kernel PlugIn messages - used in WDC_CallKerPlug() calls (user mode) / KP_LSCDMA_Call() (kernel mode) */
enum {
    KP_LSCDMA_MSG_VERSION = 1, /* Query the version of the Kernel PlugIn */
};

/* Kernel PlugIn messages status */
enum {
    KP_LSCDMA_STATUS_OK = 0x1,
    KP_LSCDMA_STATUS_MSG_NO_IMPL = 0x1000,
};

/* Default vendor and device IDs */
#define LSCDMA_DEFAULT_VENDOR_ID 0x1204 /* Vendor ID */
#define LSCDMA_DEFAULT_DEVICE_ID 0      /* Device ID */

/* Kernel PlugIn version information struct */
typedef struct {
    DWORD dwVer;
    CHAR cVer[100];
} KP_LSCDMA_VERSION;

/* Device address description struct */
typedef struct {
    DWORD dwNumAddrSpaces;    /* Total number of device address spaces */
    WDC_ADDR_DESC *pAddrDesc; /* Array of device address spaces information */
} LSCDMA_DEV_ADDR_DESC;

/* Address space information struct */
#define MAX_TYPE 8
typedef struct {
    DWORD dwAddrSpace;
    CHAR sType[MAX_TYPE];
    CHAR sName[MAX_NAME];
    CHAR sDesc[MAX_DESC];
} LSCDMA_ADDR_SPACE_INFO;

typedef void *LSCDMA_DMA_HANDLE;

/* Interrupt result information struct */
typedef struct
{
    DWORD dwCounter; /* Number of interrupts received */
    DWORD dwLost;    /* Number of interrupts not yet handled */
    WD_INTERRUPT_WAIT_RESULT waitResult; /* See WD_INTERRUPT_WAIT_RESULT values in windrvr.h */
    DWORD dwEnabledIntType; /* Interrupt type that was actually enabled
                               (MSI/MSI-X/Level Sensitive/Edge-Triggered) */
    DWORD dwLastMessage; /* Message data of the last received MSI/MSI-X
                            (Windows Vista and higher); N/A to line-based
                            interrupts) */
    LSCDMA_DMA_HANDLE hDma; /* DMA transfer handle */
} LSCDMA_INT_RESULT;
/* TODO: You can add fields to LSCDMA_INT_RESULT to store any additional
         information that you wish to pass to your diagnostics interrupt
         handler routine (DiagIntHandler() in lscdma_diag.c) */

/* LSCDMA device registers */
enum {
    /* GPIO registers */
    GPIO_ID_REG_OFFSET      = 0x0000,
    GPIO_SCRACH_OFFSET      = 0x0004,
    GPIO_LED14SEG_OFFSET    = 0x0008,
    GPIO_DIPSW_OFFSET       = 0x000a,
    GPIO_CNTRCTRL_OFFSET    = 0x000c,
    GPIO_CNTRVAL_OFFSET     = 0x0010,
    GPIO_CNTRRELOAD_OFFSET  = 0x0014,
    GPIO_DMAREQ_OFFSET      = 0x0018,
    GPIO_WR_CNTR_OFFSET     = 0x001c,
    GPIO_RD_CNTR_OFFSET     = 0x0020,

    /* Interrupt controller registers */
    INTCTL_ID_OFFSET        = 0x0100,
    INTCTL_CTRL_OFFSET      = 0x0104,
    INTCTL_STATUS_OFFSET    = 0x0108,
    INTCTL_ENABLE_OFFSET    = 0x010c,

    /* SGDMA registers */
    SGDMA_IPID_OFFSET       = 0x2000,
    SGDMA_IPVER_OFFSET      = 0x2004,
#define SGDMA_IPVER_MAJOR_MINOR(ipver) ((ipver)>>16)
    SGDMA_GCONTROL_OFFSET   = 0x2008,
    SGDMA_GSTATUS_OFFSET    = 0x200c,
    SGDMA_GEVENT_OFFSET     = 0x2010,
    SGDMA_GERROR_OFFSET     = 0x2014,
    SGDMA_GARBITER_OFFSET   = 0x2018,
    SGDMA_GAUX_OFFSET       = 0x201c,
};

#define SGDMA_CHANNEL_CTRL(n)           (((n)<<5) + 0x2200)
#define SGDMA_CHANNEL_STAT(n)           (((n)<<5) + 0x2204)
#define SGDMA_CHANNEL_CURR_SRC(n)       (((n)<<5) + 0x2208)
#define SGDMA_CHANNEL_CURR_DST(n)       (((n)<<5) + 0x220c)
#define SGDMA_CHANNEL_CURR_XFER_CNT(n)  (((n)<<5) + 0x2210)
#define SGDMA_CHANNEL_PBOFF(n)          (((n)<<5) + 0x2214)

#define SGDMA_BD_CFG0(n)      (((n)<<4) + 0x2400)
#define SGDMA_BD_CFG1(n)      (((n)<<4) + 0x2404)
#define SGDMA_BD_SRC(n)       (((n)<<4) + 0x2408)
#define SGDMA_BD_DST(n)       (((n)<<4) + 0x240c)

#define CHANNEL_STATUS_ENABLED   0x00000001
#define CHANNEL_STATUS_REQUEST   0x00000002
#define CHANNEL_STATUS_XFERCOMP  0x00000004
#define CHANNEL_STATUS_EOD       0x00000008
#define CHANNEL_STATUS_CLRCOMP   0x00000010
#define CHANNEL_STATUS_ERRORS    0x00ff0000

/* Interrupt controller bit definitions */
#define INTCTL_OUT_ACTIVE            0x01
#define INTCTL_TEST_MODE             0x02
#define INTCTL_OUTPUT_EN             0x04
#define INTCTL_INTR_TEST1            0x0100
#define INTCTL_INTR_TEST2            0x0200
#define INTCTL_TEST1_EN              0x0001
#define INTCTL_TEST2_EN              0x0002
#define INTCTL_DOWN_COUNT_EN         0x0020
#define INTCTL_INTR_DOWN_COUNT_ZERO  0x0020
#define INTCTL_INTR_WR_CHAN          0x0001
#define INTCTL_INTR_RD_CHAN          0x0002

#define GPIO_ID_VALUE     0x12043010
#define INTCTL_ID_VALUE   0x12043050
#define SGDMA_IPID_VALUE  0x12040000

#define SGDMA_ADDR_MODE_FIFO  0
#define SGDMA_ADDR_MODE_MEM   1
#define SGDMA_ADDR_MODE_LOOP  2

#define SGDMA_DATA_64BIT   3
#define SGDMA_DATA_32BIT   2
#define SGDMA_DATA_16BIT   1
#define SGDMA_DATA_8BIT    0

#define SGDMA_EOL       1
#define SGDMA_NEXT      0
#define SGDMA_SPLIT     2
#define SGDMA_LOCK      4
#define SGDMA_AUTORTY   8

#define SGDMA_BUS_A   0
#define SGDMA_BUS_B   1
#define SGDMA_PB      2

#define SGDMA_SRC_BUS(a)       ((a)<<8)
#define SGDMA_SRC_SIZE(a)      ((a)<<10) /* 0=8 bit, 1=16 bit, 2=32 bit, 3=64 bit */
#define SGDMA_SRC_ADDR_MODE(a) ((a)<<13) /* 0=FIFO, 1=MEM, 2=LOOP */
#define SGDMA_SRC_MEM          (1<<13)
#define SGDMA_SRC_FIFO         (0<<13)

#define SGDMA_DST_BUS(a)       ((a)<<16)
#define SGDMA_DST_SIZE(a)      ((a)<<18) /* 0=8 bit, 1=16 bit, 2=32 bit, 3=64 bit */
#define SGDMA_DST_ADDR_MODE(a) ((a)<<21) /* 0=FIFO, 1=MEM, 2=LOOP */
#define SGDMA_DST_MEM          (1<<21)
#define SGDMA_DST_FIFO         (0<<21)

#define SGDMA_EBR64_BASE_ADDR(a) (0x10000 + (a))
#define SGDMA_EBR64_SIZE         (64 * 1024)
#define SGDMA_BURST_SIZE(s)      ((s)<<16)

/* Convert the BAR address to the board's local Address */
#define SGDMA_WB(a) ((a) & 0x0fffffff)

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define SGDMA_NUM_CHANNELS  2
#define SGDMA_WRITE_CHANNEL 0 /* DMA_TO_DEVICE */
#define SGDMA_READ_CHANNEL  1 /* DMA_FROM_DEVICE */

/* TODO: Maximun number of descriptors  can be modified according to your DMA
 * implementation. Note that maximum value also depends on number of descriptors
 * supported by your device */
#define MAX_DMA_WRITE_DESCS     256
#define MAX_DMA_READ_DESCS      16
#define MAX_DMA_FROM_DEV_BYTES  (PAGE_SIZE * MAX_DMA_WRITE_DESCS)
#define MAX_DMA_TO_DEV_BYTES    (PAGE_SIZE * MAX_DMA_READ_DESCS)

/* LSCDMA diagnostics interrupt handler function type */
typedef void (* LSCDMA_INT_HANDLER)(WDC_DEVICE_HANDLE hDev,
    LSCDMA_INT_RESULT *pIntResult);

/* LSCDMA diagnostics plug-and-play and power management events handler function type */
typedef void (* LSCDMA_EVENT_HANDLER)(WDC_DEVICE_HANDLE hDev,
    DWORD dwAction);

/*************************************************************
  Function prototypes
 *************************************************************/
DWORD LSCDMA_LibInit(void);
DWORD LSCDMA_LibUninit(void);

#if !defined(__KERNEL__)
WDC_DEVICE_HANDLE LSCDMA_DeviceOpen(DWORD dwVendorId, DWORD dwDeviceId);
BOOL LSCDMA_DeviceClose(WDC_DEVICE_HANDLE hDev);

DWORD LSCDMA_IntEnable(WDC_DEVICE_HANDLE hDev, LSCDMA_INT_HANDLER funcIntHandler);
DWORD LSCDMA_IntDisable(WDC_DEVICE_HANDLE hDev);
BOOL LSCDMA_IntIsEnabled(WDC_DEVICE_HANDLE hDev);

LSCDMA_DMA_HANDLE LSCDMA_DmaOpen(WDC_DEVICE_HANDLE hDev, DWORD dwBytes,
    BOOL fToDevice);
void LSCDMA_DmaClose(LSCDMA_DMA_HANDLE hDma);
void LSCDMA_DmaInterruptsEnable(LSCDMA_DMA_HANDLE hDma);
void LSCDMA_DmaInterruptsDisable(LSCDMA_DMA_HANDLE hDma);
void LSCDMA_DmaStart(LSCDMA_DMA_HANDLE hDma);
void LSCDMA_DMAStop(LSCDMA_DMA_HANDLE hDma);
PVOID LSCDMA_DmaBufferGet(LSCDMA_DMA_HANDLE hDma, DWORD *pBytes);
BOOL LSCDMA_DmaIstoDevice(LSCDMA_DMA_HANDLE hDma);

DWORD LSCDMA_EventRegister(WDC_DEVICE_HANDLE hDev, LSCDMA_EVENT_HANDLER funcEventHandler);
DWORD LSCDMA_EventUnregister(WDC_DEVICE_HANDLE hDev);
BOOL LSCDMA_EventIsRegistered(WDC_DEVICE_HANDLE hDev);
#endif

DWORD LSCDMA_GetNumAddrSpaces(WDC_DEVICE_HANDLE hDev);
BOOL LSCDMA_GetAddrSpaceInfo(WDC_DEVICE_HANDLE hDev, LSCDMA_ADDR_SPACE_INFO *pAddrSpaceInfo);


const char *LSCDMA_GetLastErr(void);

#ifdef __cplusplus
}
#endif

#endif
