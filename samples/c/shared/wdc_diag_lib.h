/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _WDC_DIAG_LIB_H_
#define _WDC_DIAG_LIB_H_

/******************************************************************************
*  File: wdc_diag_lib.h - Shared WDC PCI and ISA devices' user-mode           *
*  diagnostics API header.                                                    *
*******************************************************************************/

#if !defined(__KERNEL__)

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include "windrvr.h"
#include "wdc_lib.h"
#include "pci_regs.h"
#include "wdc_defs.h"

/* -----------------------------------------------
    PCI/ISA
   ----------------------------------------------- */
#define ACTIVE_ADDR_SPACE_NEEDS_INIT 0xFF

/* Print run-time registers and PCI configuration registers information */
#define READ_REGS_ALL_DESC_INDENT      39
#define READ_REGS_ALL_DESC_WIDTH       22
#define READ_REGS_ALL_DETAILS_INDENT   61
#define READ_REGS_ALL_DETAILS_WIDTH    20
#define REGS_INFO_PRINT_DETAILS_INDENT 54
#define REGS_INFO_PRINT_DETAILS_WIDTH  22
#define PCIE_REGS_NUM                  68

#define WDC_DIAG_REG_PRINT_NAME       0x1
#define WDC_DIAG_REG_PRINT_DESC       0x2
#define WDC_DIAG_REG_PRINT_ADDR_SPACE 0x4
#define WDC_DIAG_REG_PRINT_OFFSET     0x8
#define WDC_DIAG_REG_PRINT_SIZE       0x10
#define WDC_DIAG_REG_PRINT_DIRECTION  0x12
#define WDC_DIAG_REG_PRINT_ALL \
    (WDC_DIAG_REG_PRINT_NAME | WDC_DIAG_REG_PRINT_DESC | \
    WDC_DIAG_REG_PRINT_ADDR_SPACE | WDC_DIAG_REG_PRINT_OFFSET | \
    WDC_DIAG_REG_PRINT_SIZE | WDC_DIAG_REG_PRINT_DIRECTION)
#define WDC_DIAG_REG_PRINT_DEFAULT \
    (WDC_DIAG_REG_PRINT_NAME | WDC_DIAG_REG_PRINT_DIRECTION | \
        WDC_DIAG_REG_PRINT_DESC)
typedef DWORD WDC_DIAG_REG_PRINT_OPTIONS;

/* Device configuration space identifier (PCI configuration space) */
#define WDC_AD_CFG_SPACE 0xFF

/* Register information struct */
typedef struct {
    DWORD dwAddrSpace;       /* Number of address space in which the register
                                resides */
                             /* For PCI configuration registers, use
                              * WDC_AD_CFG_SPACE */
    DWORD dwOffset;          /* Offset of the register in the dwAddrSpace
                              * address space */
    DWORD dwSize;            /* Register's size (in bytes) */
    WDC_DIRECTION direction; /* Read/write access mode - see WDC_DIRECTION
                              * options */
    CHAR  sName[MAX_NAME];   /* Register's name */
    CHAR  sDesc[MAX_DESC];   /* Register's description */
} WDC_REG;


/* Device address description struct */
typedef struct {
    DWORD dwNumAddrSpaces;    /* Total number of device address spaces */
    WDC_ADDR_DESC *pAddrDesc; /* Array of device address spaces information */
} DEV_ADDR_DESC;

/* Address space information struct */
#define MAX_TYPE 8
typedef struct {
    DWORD dwAddrSpace;
    CHAR sType[MAX_TYPE];
    CHAR sName[MAX_NAME];
    CHAR sDesc[MAX_DESC];
} ADDR_SPACE_INFO;

void WDC_DIAG_RegsInfoPrint(WDC_DEVICE_HANDLE hDev, const WDC_REG *pRegs,
    DWORD dwNumRegs, WDC_DIAG_REG_PRINT_OPTIONS options, BOOL isExpress);

/* Set address access mode */
BOOL WDC_DIAG_SetMode(WDC_ADDR_MODE *pMode);

/* Get data for address write operation from user */
/* Data size (dwSize) should be WDC_SIZE_8, WDC_SIZE_16, WDC_SIZE_32 or
 * WDC_SIZE_64 */
BOOL WDC_DIAG_InputWriteData(PVOID pData, WDC_ADDR_SIZE dwSize);

/* Read/write a memory or I/O address */
void WDC_DIAG_ReadWriteAddr(WDC_DEVICE_HANDLE hDev, WDC_DIRECTION direction,
    DWORD dwAddrSpace, WDC_ADDR_MODE mode);

/* Read/write a memory or I/O address OR an offset in the PCI configuration
 * space (dwAddrSpace == WDC_AD_CFG_SPACE) */
void WDC_DIAG_ReadWriteBlock(WDC_DEVICE_HANDLE hDev, WDC_DIRECTION direction,
    DWORD dwAddrSpace);

/* Read all pre-defined run-time or PCI configuration registers and display
 * results */
void WDC_DIAG_ReadRegsAll(WDC_DEVICE_HANDLE hDev, const WDC_REG *pRegs,
    DWORD dwNumRegs, BOOL fPciCfg, BOOL extended);

/* Display a list of pre-defined run-time or PCI configuration registers
   and let user select to read/write from/to a specific register */
void WDC_DIAG_ReadWriteReg(WDC_DEVICE_HANDLE hDev, const WDC_REG *pRegs,
    DWORD dwNumRegs, WDC_DIRECTION direction, BOOL fPciCfg, BOOL fExpressReg);

/* Display available PCI/PCIe capabilities */
void WDC_DIAG_ScanPCICapabilities(PVOID pData);

/* Get Interrupt Type description */
char *WDC_DIAG_IntTypeDescriptionGet(DWORD dwIntType);

/* -----------------------------------------------
    PCI
   ----------------------------------------------- */
/* Print PCI device location information */
void WDC_DIAG_PciSlotPrint(WD_PCI_SLOT *pPciSlot);
/* Print PCI device location information to file*/
void WDC_DIAG_PciSlotPrintFile(WD_PCI_SLOT *pPciSlot, FILE *fp);
/* Print PCI device location and resources information */
void WDC_DIAG_PciDeviceInfoPrint(WD_PCI_SLOT *pPciSlot, BOOL dump_cfg);
/* Print PCI device location and resources information to file */
void WDC_DIAG_PciDeviceInfoPrintFile(WD_PCI_SLOT *pPciSlot, FILE *fp,
    BOOL dump_cfg);
/* Print location and resources information for all connected PCI devices */
void WDC_DIAG_PciDevicesInfoPrintAll(BOOL dump_cfg);
/* Print location and resources information for all connected PCI devices to
 * file */
void WDC_DIAG_PciDevicesInfoPrintAllFile(FILE *fp, BOOL dump_cfg);

#ifndef ISA
/* Find and open a PCI device */
WDC_DEVICE_HANDLE WDC_DIAG_DeviceFindAndOpen(DWORD dwVendorId,
    DWORD dwDeviceId, PCHAR pcKpName, const DWORD dwDevCtxSize);
/* Find a PCI device */
BOOL WDC_DIAG_DeviceFind(DWORD dwVendorId, DWORD dwDeviceId,
    WD_PCI_SLOT *pSlot);
/* Open a handle to a device */
WDC_DEVICE_HANDLE WDC_DIAG_DeviceOpen(const WD_PCI_SLOT *pSlot, PCHAR pcKpName,
    const DWORD dwDevCtxSize);
/* Open a PCI device */
WDC_DEVICE_HANDLE WDC_DIAG_PCI_DeviceOpen(const WD_PCI_CARD_INFO *pDeviceInfo,
    PCHAR pcKpName, const DWORD dwDevCtxSize);
#endif
/* Close a handle to a PCI device */
BOOL WDC_DIAG_DeviceClose(WDC_DEVICE_HANDLE hDev);

BOOL WDC_DIAG_IsValidDevice(PWDC_DEVICE pDev, const CHAR *sFunc);

#ifndef ISA
DWORD WDC_DIAG_GetNumAddrSpaces(WDC_DEVICE_HANDLE hDev);
#endif

BOOL WDC_DIAG_GetAddrSpaceInfo(WDC_DEVICE_HANDLE hDev,
    ADDR_SPACE_INFO *pAddrSpaceInfo);

#ifdef __cplusplus
}
#endif

#endif /* !defined(__KERNEL__) */

#endif /* _WDC_DIAG_LIB_H_ */

