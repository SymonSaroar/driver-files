/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _PLX_DIAG_LIB_H_
#define _PLX_DIAG_LIB_H_

/*
 * File: plx_diag_lib.h - Header of shared PLX diagnostics library for
 *                        accessing PLX devices from the user-mode.
 *
 * Note: This code sample is provided AS-IS and as a guiding sample only.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "../lib/plx_lib.h"

/*************************************************************
  General definitions
 *************************************************************/
/* Error messages display */
#define PLX_DIAG_ERR printf

struct _PLX_DIAG_DMA;

typedef struct {
    WDC_DEVICE_HANDLE *phDev;
    struct _PLX_DIAG_DMA *pDma;
    BOOL fIsMaster;
} PLX_MENU_CTX;

typedef struct {
    WDC_DEVICE_HANDLE *phDev;
    BOOL fVPDSupported;
    DWORD EEPROMmsb;
} PLX_MENU_EEPROM_CTX;

/*************************************************************
  Functions prototypes
 *************************************************************/
 /* -----------------------------------------------
     Device Open
    ----------------------------------------------- */
void PLX_DIAG_MenuDeviceOpenInit(DIAG_MENU_OPTION *pParentMenu,
    PLX_MENU_CTX *pPlxMenuCtx);

/* -----------------------------------------------
    Read/write memory and I/O addresses
   ----------------------------------------------- */
void PLX_DIAG_MenuReadWriteAddrInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);
void PLX_DIAG_ReadWriteAddrLocal(WDC_DEVICE_HANDLE hDev,
    WDC_DIRECTION direction, PLX_ADDR addrSpace, WDC_ADDR_MODE mode);
void PLX_DIAG_ReadWriteAddrLocalBlock(WDC_DEVICE_HANDLE hDev,
    WDC_DIRECTION direction, PLX_ADDR addrSpace);

/* -----------------------------------------------
    Read/write PCI configuration registers
   ----------------------------------------------- */
void PLX_DIAG_MenuReadWriteCfgSpaceInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

/* -----------------------------------------------
    Read/write run-time registers
   ----------------------------------------------- */
void PLX_DIAG_MenuReadWriteRegsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

/* -----------------------------------------------
    Direct Memory Access (DMA)
   ----------------------------------------------- */
typedef struct _PLX_DIAG_DMA {
    PLX_DMA_HANDLE hDma;
    BOOL  fSG;
    PVOID pBuf;
} PLX_DIAG_DMA;

typedef struct {
    WDC_DEVICE_HANDLE *phDev;
    PLX_DIAG_DMA *pDma;
    PLX_INT_HANDLER DmaIntHandler;
    PLX_DMA_CHANNEL dmaChannel;
    BOOL fIsDACSupported; /* Dual Address Cycle */
} PLX_MENU_DMA_CTX;

void PLX_DIAG_MenuDmaInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev, PLX_DIAG_DMA *pDma,
    PLX_INT_HANDLER MasterDiagDmaIntHandler, BOOL fIsDACSupported);
DWORD PLX_DIAG_DMAOpen(PLX_MENU_DMA_CTX *pPlxMenuDmaCtx);
DWORD PLX_DIAG_DMAClose(WDC_DEVICE_HANDLE hDev, PLX_DIAG_DMA *pDma);

#define PLX_TRANSACTION_SAMPLE_MAX_TRANSFER_SIZE 8192

DWORD PLX_DIAG_DMATransactionInit(PLX_MENU_DMA_CTX *pPlxMenuDmaCtx);

/* -----------------------------------------------
    Interrupt handling
   ----------------------------------------------- */
void PLX_DIAG_MenuInterruptsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev, PLX_INT_HANDLER DiagIntHandler, PVOID pData);

/* ----------------------------------------------------
    Plug-and-play and power management events handling
   ---------------------------------------------------- */
void PLX_DIAG_MenuEventsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev, PLX_EVENT_HANDLER DiagEventHandler);

/* -----------------------------------------------
    Access the serial EEPROM
   ----------------------------------------------- */
void PLX_DIAG_MenuEEPROMInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev, DWORD EEPROMmsb);

/* -----------------------------------------------
    Reset board
   ----------------------------------------------- */
/* NOTE: Currently supported for master devices only (PLX 9054, 9056, 9080,
 * 9656) */
void PLX_DIAG_MenuResetBoardInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

#ifdef __cplusplus
}
#endif

#endif /* _PLX_DIAG_LIB_H_ */

