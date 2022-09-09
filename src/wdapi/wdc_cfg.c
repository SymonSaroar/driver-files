/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/*
 *  File: wdc_cfg.c
 *
 *  Implementation of WDC functions for reading/writing
 *  PCI configuration space
 */

#include "wdc_lib.h"
#include "wdc_defs.h"
#include "wdc_err.h"
#include "status_strings.h"

#if defined(DEBUG)
static inline BOOL RWParamsValidate(WDC_DEVICE_HANDLE hDev, PVOID pData)
{
    if (!WdcIsValidDevHandle(hDev) || !WdcIsValidPtr(pData, "NULL data buffer"))
        return FALSE;
    return TRUE;
}
#endif

/*
 * Read/Write PCI configuration registers
 */

/* Read/Write by slot */
static DWORD WDC_PciReadWriteCfgBySlot(WD_PCI_SLOT *pPciSlot, DWORD dwOffset,
    PVOID pData, DWORD dwBytes, WDC_DIRECTION direction)
{
    DWORD dwStatus;
    WD_PCI_CONFIG_DUMP pciCnf;

    BZERO(pciCnf);
    pciCnf.pciSlot = *pPciSlot;
    pciCnf.pBuffer = pData;
    pciCnf.dwOffset = dwOffset;
    pciCnf.dwBytes = dwBytes;
    pciCnf.fIsRead = (WDC_READ == direction);

    dwStatus = WD_PciConfigDump(WDC_GetWDHandle(), &pciCnf);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WdcSetLastErrStr("Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    return WD_STATUS_SUCCESS;

Error:
    WDC_Err("WDC_PciReadWriteCfgBySlot: Failed %s %ld bytes %s offset"
        "0x%lx.\n%s", (WDC_READ == direction) ? "reading" : "writing",
        dwBytes, (WDC_READ == direction) ? "from" : "to", dwOffset,
        WdcGetLastErrStr());

    return dwStatus;
}

/* Read/Write by device handle */
static DWORD WDC_PciReadWriteCfg(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    PVOID pData, DWORD dwBytes, WDC_DIRECTION direction)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    return WDC_PciReadWriteCfgBySlot(&(pDev->slot), dwOffset, pData,
        dwBytes, direction);
}

DWORD DLLCALLCONV WDC_PciReadCfgBySlot(_In_ WD_PCI_SLOT *pPciSlot,
    _In_ DWORD dwOffset, _Outptr_ PVOID pData, _In_ DWORD dwBytes)
{
    return WDC_PciReadWriteCfgBySlot(pPciSlot, dwOffset, pData, dwBytes,
        WDC_READ);
}

DWORD DLLCALLCONV WDC_PciWriteCfgBySlot(_In_ WD_PCI_SLOT *pPciSlot,
    _In_ DWORD dwOffset, _In_ PVOID pData, _In_ DWORD dwBytes)
{
    return WDC_PciReadWriteCfgBySlot(pPciSlot, dwOffset, pData, dwBytes,
        WDC_WRITE);
}

DWORD DLLCALLCONV WDC_PciReadCfg(_In_ WDC_DEVICE_HANDLE hDev,
    _In_ DWORD dwOffset, _Outptr_ PVOID pData, _In_ DWORD dwBytes)
{
    return WDC_PciReadWriteCfg(hDev, dwOffset, pData, dwBytes, WDC_READ);
}

DWORD DLLCALLCONV WDC_PciWriteCfg(_In_ WDC_DEVICE_HANDLE hDev,
    _In_ DWORD dwOffset, _In_ PVOID pData, _In_ DWORD dwBytes)
{
    return WDC_PciReadWriteCfg(hDev, dwOffset, pData, dwBytes, WDC_WRITE);
}

#define DECLARE_PCI_READ_CFG_BY_SLOT(bits,type) \
    DWORD DLLCALLCONV WDC_PciReadCfgBySlot##bits( \
        _In_ WD_PCI_SLOT *pPciSlot, \
        _In_ DWORD dwOffset, _Out_ type *val) { \
            return WDC_PciReadWriteCfgBySlot(pPciSlot, dwOffset, val, \
                bits/8, WDC_READ);\
        }

#define DECLARE_PCI_WRITE_CFG_BY_SLOT(bits,type) \
    DWORD DLLCALLCONV WDC_PciWriteCfgBySlot##bits( \
        _In_ WD_PCI_SLOT *pPciSlot, _In_ DWORD dwOffset, _In_ type val) \
        { \
            return WDC_PciReadWriteCfgBySlot(pPciSlot, dwOffset, &val, \
                bits/8, WDC_WRITE);\
        }

#define DECLARE_PCI_READ_CFG(bits,type) \
    DWORD DLLCALLCONV WDC_PciReadCfg##bits( \
        _In_ WDC_DEVICE_HANDLE hDev, _In_ DWORD dwOffset, _Out_ type *val) \
        { \
        return WDC_PciReadWriteCfg(hDev, dwOffset, val, bits/8, WDC_READ); \
        }

#define DECLARE_PCI_WRITE_CFG(bits,type) \
    DWORD DLLCALLCONV WDC_PciWriteCfg##bits(_In_ WDC_DEVICE_HANDLE hDev, \
        _In_ DWORD dwOffset, _In_ type val) { \
        return WDC_PciReadWriteCfg(hDev, dwOffset, &val, bits/8, WDC_WRITE); \
    }

DECLARE_PCI_READ_CFG_BY_SLOT(8, BYTE)
DECLARE_PCI_READ_CFG_BY_SLOT(16, WORD)
DECLARE_PCI_READ_CFG_BY_SLOT(32, UINT32)
DECLARE_PCI_READ_CFG_BY_SLOT(64, UINT64)
DECLARE_PCI_WRITE_CFG_BY_SLOT(8, BYTE)
DECLARE_PCI_WRITE_CFG_BY_SLOT(16, WORD)
DECLARE_PCI_WRITE_CFG_BY_SLOT(32, UINT32)
DECLARE_PCI_WRITE_CFG_BY_SLOT(64, UINT64)

DECLARE_PCI_READ_CFG(8, BYTE)
DECLARE_PCI_READ_CFG(16, WORD)
DECLARE_PCI_READ_CFG(32, UINT32)
DECLARE_PCI_READ_CFG(64, UINT64)
DECLARE_PCI_WRITE_CFG(8, BYTE)
DECLARE_PCI_WRITE_CFG(16, WORD)
DECLARE_PCI_WRITE_CFG(32, UINT32)
DECLARE_PCI_WRITE_CFG(64, UINT64)
