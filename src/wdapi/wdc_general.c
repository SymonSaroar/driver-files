/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/****************************************************************
* File: wdc_general.c - Implementation of general WDC API -     *
*       init/uninit driver/WDC lib; open/close device           *
*****************************************************************/

#include "utils.h"
#include "wdc_lib.h"
#include "wdc_defs.h"
#include "wdc_err.h"
#include "status_strings.h"
#include "pci_regs.h"

/*************************************************************
  General definitions
 *************************************************************/

/* Demo WinDriver license registration string */
#define WDC_DEMO_LICENSE_STR "12345abcde1234.license"

static HANDLE ghWD = INVALID_HANDLE_VALUE;

#if !defined(__KERNEL__)
static DWORD PciScanDevices(DWORD dwVendorId, DWORD dwDeviceId,
    WDC_PCI_SCAN_RESULT *pPciScanResult, DWORD dwOptions);
static void deviceDestroy(PWDC_DEVICE pDev);
static DWORD deviceOpen(WDC_DEVICE_HANDLE *phDev, const PVOID pDeviceInfo,
    const PVOID pDevCtx, WD_BUS_TYPE bus);
static DWORD deviceClose(WDC_DEVICE_HANDLE hDev);
static DWORD KernelPlugInOpen(WDC_DEVICE_HANDLE hDev,
    const CHAR *pcKPDriverName, PVOID pKPOpenData);
static DWORD SetDeviceInfo(PWDC_DEVICE pDev);
#endif

#define MAX_ADDR_SPACE_NUM WD_CARD_ITEMS

/* Get handle to WinDriver (required for WD_XXX functions) */
HANDLE DLLCALLCONV WDC_GetWDHandle(void)
{
    return ghWD;
}

PVOID DLLCALLCONV WDC_GetDevContext(_In_ WDC_DEVICE_HANDLE hDev)
{
    return hDev ? ((PWDC_DEVICE)hDev)->pCtx : NULL;
}

/* Get device's bus type */
WD_BUS_TYPE DLLCALLCONV WDC_GetBusType(_In_ WDC_DEVICE_HANDLE hDev)
{
    if (!WdcIsValidDevHandle(hDev))
    {
        WDC_Err("WDC_GetBusType: Error %s", WdcGetLastErrStr());
        return WD_BUS_UNKNOWN;
    }

    return ((PWDC_DEVICE)hDev)->cardReg.Card.Item[0].I.Bus.dwBusType;
}

/* Sleep (default option - WDC_SLEEP_BUSY) */
DWORD DLLCALLCONV WDC_Sleep(_In_ DWORD dwMicroSecs,
    _In_ WDC_SLEEP_OPTIONS options)
{
    WD_SLEEP slp;

    BZERO(slp);
    slp.dwMicroSeconds = dwMicroSecs;
    slp.dwOptions = options;

    return WD_Sleep(ghWD, &slp);
}

/* Get WinDriver's kernel module version */
DWORD DLLCALLCONV WDC_Version(_Outptr_ CHAR *pcVersion, _In_ DWORD dwLen,
    _Out_ DWORD *pdwVersion)
{
    WD_VERSION ver;
    DWORD dwStatus;
    HANDLE hWD;

    BZERO(ver);

    if (!WdcIsValidPtr(pcVersion, "NULL pointer to version results") ||
        !WdcIsValidPtr(pdwVersion, "NULL pointer to version results"))
    {
        return WD_INVALID_PARAMETER;
    }
    if (dwLen < WD_VERSION_STR_LENGTH)
        return WD_INVALID_PARAMETER;

    hWD = WD_Open();
    if (hWD == INVALID_HANDLE_VALUE)
        return WD_INVALID_HANDLE;

    dwStatus = WD_Version(hWD, &ver);
    strcpy(pcVersion, ver.cVer);
    *pdwVersion = ver.dwVer;

    WD_Close(hWD);
    return dwStatus;
}

/* -----------------------------------------------
    Open/close driver
   ----------------------------------------------- */
DWORD DLLCALLCONV WDC_DriverOpen(_In_ WDC_DRV_OPEN_OPTIONS openOptions,
    _In_ const CHAR *pcLicense)
{
#if !defined(__KERNEL__)
    DWORD dwStatus;
#endif

    if (ghWD != INVALID_HANDLE_VALUE)
        return WD_OPERATION_ALREADY_DONE;

    /* Open a handle to WinDriver */
    ghWD = WD_Open();
    if (ghWD == INVALID_HANDLE_VALUE)
        return WD_INVALID_HANDLE;

#if defined(__KERNEL__)
    /* the two parameters below are not referenced */
    openOptions = openOptions;
    pcLicense = pcLicense;
    WDC_Trace("WDC_DriverOpen: Called from Kernel PlugIn version - %s\n", WD_VER_STR);
    return WD_STATUS_SUCCESS;
#else
    /* Compare WinDriver files versions with running WinDriver kernel module
     * version */
    WDC_Trace("WDC_DriverOpen: User mode version - %s\n", WD_VER_STR);
#if defined (i386) && defined (KERNEL_64BIT)
    WDC_Trace("WDC_DriverOpen: Running a 32 bit user app on a 64 bit system\n");
#endif
    if (openOptions & WDC_DRV_OPEN_CHECK_VER)
    {
        WD_VERSION ver;

        BZERO(ver);
        dwStatus = WD_Version(ghWD, &ver);
        if (WD_STATUS_SUCCESS != dwStatus || ver.dwVer < WD_VER)
        {
            WDC_Err("WDC_DriverOpen: Version check failed. Error 0x%lx - %s\n",
                dwStatus, Stat2Str(dwStatus));
            if (WD_STATUS_SUCCESS == dwStatus)
                dwStatus = WD_INCORRECT_VERSION;
            goto Error;
        }
    }

    /* Register WinDriver license registration string */
    if (openOptions & WDC_DRV_OPEN_REG_LIC)
    {
        WD_LICENSE lic;

        BZERO(lic);

        if (pcLicense && strcmp(pcLicense, ""))
        {
            strcpy(lic.cLicense, pcLicense);
        }
        else
        {
            WDC_Trace("WDC_DriverOpen: No license to register -> attempting to "
                "register demo WinDriver license\n");
            strcpy(lic.cLicense, WDC_DEMO_LICENSE_STR);
        }

        dwStatus = WD_License(ghWD, &lic);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            WDC_Err("WDC_DriverOpen: Failed registering WinDriver license. "
                "Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
            goto Error;
        }
    }

    return WD_STATUS_SUCCESS;

Error:
    WDC_DriverClose();
    return dwStatus;
#endif
}

DWORD DLLCALLCONV WDC_DriverClose(void)
{
    WDC_SetDebugOptions(WDC_DBG_NONE, NULL);

    if (INVALID_HANDLE_VALUE != ghWD)
    {
        WD_Close(ghWD);
        ghWD = INVALID_HANDLE_VALUE;
    }

    return WD_STATUS_SUCCESS;
}

/* -----------------------------------------------
    Scan bus
   ----------------------------------------------- */
#ifndef __KERNEL__
DWORD DLLCALLCONV WDC_PciScanDevices(_In_ DWORD dwVendorId,
    _In_ DWORD dwDeviceId, _Outptr_ WDC_PCI_SCAN_RESULT *pPciScanResult)
{
    WDC_Trace("WDC_PciScanDevices: Entered\n");

    return PciScanDevices(dwVendorId, dwDeviceId, pPciScanResult,
        WD_PCI_SCAN_DEFAULT);
}

DWORD DLLCALLCONV WDC_PciScanDevicesByTopology(_In_ DWORD dwVendorId,
    _In_ DWORD dwDeviceId, _Outptr_ WDC_PCI_SCAN_RESULT *pPciScanResult)
{
    WDC_Trace("WDC_PciScanDevicesByTopology: Entered\n");

    return PciScanDevices(dwVendorId, dwDeviceId, pPciScanResult,
        WD_PCI_SCAN_BY_TOPOLOGY);
}

DWORD DLLCALLCONV WDC_PciScanRegisteredDevices(_In_ DWORD dwVendorId,
    _In_ DWORD dwDeviceId, _Outptr_ WDC_PCI_SCAN_RESULT *pPciScanResult)
{
    WDC_Trace("WDC_PciScanRegisteredDevices: Entered\n");

    return PciScanDevices(dwVendorId, dwDeviceId, pPciScanResult,
        WD_PCI_SCAN_REGISTERED);
}

/* -----------------------------------------------
    Get device's resources information
   ----------------------------------------------- */
DWORD DLLCALLCONV WDC_PciGetDeviceInfo(_Inout_ WD_PCI_CARD_INFO *pDeviceInfo)
{
    if (!WdcIsValidPtr(pDeviceInfo, "NULL device information pointer"))
    {
        WDC_Err("WDC_PciGetDeviceInfo: %s", WdcGetLastErrStr());
        return WD_INVALID_PARAMETER;
    }

    return WD_PciGetCardInfo(ghWD, pDeviceInfo);
}

/* -----------------------------------------------
    Scan PCI devices according to options
   ----------------------------------------------- */
static DWORD PciScanDevices(DWORD dwVendorId, DWORD dwDeviceId,
    WDC_PCI_SCAN_RESULT *pPciScanResult, DWORD dwOptions)
{
    DWORD dwStatus, i, dwOptionsCheck = dwOptions;
    WD_PCI_SCAN_CARDS scanDevices;

    if (!WdcIsValidPtr(pPciScanResult,
        "NULL pointer to device scan results struct"))
    {
        return WD_INVALID_PARAMETER;
    }

    BZERO(scanDevices);
    scanDevices.searchId.dwVendorId = dwVendorId;
    scanDevices.searchId.dwDeviceId = dwDeviceId;
    scanDevices.dwOptions = dwOptions;

    if (dwOptions & WD_PCI_SCAN_INCLUDE_DOMAINS)
        dwOptionsCheck = dwOptions ^ WD_PCI_SCAN_INCLUDE_DOMAINS;

    if (dwOptionsCheck != WD_PCI_SCAN_DEFAULT &&
        dwOptionsCheck != WD_PCI_SCAN_BY_TOPOLOGY &&
        dwOptionsCheck != WD_PCI_SCAN_REGISTERED)
    {
        WDC_Err("PciScanDevices: Error. Invalid dwOptions [0x%lx]\n",
            dwOptionsCheck);
        return WD_INVALID_PARAMETER;
    }

    dwStatus = WD_PciScanCards(ghWD, &scanDevices);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDC_Err("PciScanDevices: Failed scanning PCI bus. Error [0x%lx - %s]\n",
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    BZERO(*pPciScanResult);
    pPciScanResult->dwNumDevices = scanDevices.dwCards;

    for (i = 0; i < scanDevices.dwCards; i++)
    {
        pPciScanResult->deviceId[i] = scanDevices.cardId[i];
        pPciScanResult->deviceSlot[i] = scanDevices.cardSlot[i];
    }

    WDC_Trace("PciScanDevices: PCI bus scanned successfully.\n"
        "Found [%ld] matching cards (vendor ID [0x%lx], device ID [0x%lx])\n",
        pPciScanResult->dwNumDevices, dwVendorId, dwDeviceId);

    return WD_STATUS_SUCCESS;
}
#endif

/* -----------------------------------------------
    Scan PCI/PCIe capabilities according to options
   ----------------------------------------------- */
DWORD DLLCALLCONV WDC_PciGetExpressOffset(_In_ WDC_DEVICE_HANDLE hDev,
    _Outptr_ DWORD *pdwOffset)
{
    WDC_PCI_SCAN_CAPS_RESULT scanResult;
    DWORD status, i;

    BZERO(scanResult);
    status = WDC_PciScanCaps(hDev, WD_PCI_CAP_ID_ALL, &scanResult);
    if (status)
    {
        WDC_Err("%s: Failed scanning PCI capabilities. error 0x%lx (\"%s\")\n",
            __FUNCTION__, status, Stat2Str(status));
        return status;
    }

    for (i = 0; i < scanResult.dwNumCaps; i++)
    {
        DWORD cap_id = scanResult.pciCaps[i].dwCapId;
        DWORD cap_offset = scanResult.pciCaps[i].dwCapOffset;

        if (cap_id == PCI_CAP_ID_EXP)
        {
            *pdwOffset = cap_offset;
            return WD_STATUS_SUCCESS;
        }
    }
    return WD_OPERATION_FAILED;
}

DWORD DLLCALLCONV WDC_PciGetHeaderType(_In_ WDC_DEVICE_HANDLE hDev,
    _Outptr_ WDC_PCI_HEADER_TYPE *pHeaderType)
{
    DWORD status;
    BYTE tmp;

    if (!pHeaderType)
    {
        WDC_Err ("%s: Pointer to header_type is NULL\n", __FUNCTION__);
        return WD_INVALID_PARAMETER;
    }
    if (!hDev)
    {
        WDC_Err ("%s: hDev is NULL\n", __FUNCTION__);
        return WD_INVALID_HANDLE;
    }

    status = WDC_PciReadCfg8(hDev, PCI_HDR, &tmp);
    if (status)
    {
        WDC_Err("%s: Could not get header type. "
            "error 0x%lx (\"%s\")\n", __FUNCTION__, status, Stat2Str(status));
        return status;
    }

    /* ignore the 7th bit of the register because it is not relevant to the
     * header type (See PCI System Architecture book page 554)*/
    tmp &= 0x7f;

    switch (tmp)
    {
    case PCI_HEADER_TYPE_NORMAL:
        *pHeaderType = HEADER_TYPE_NORMAL;
        break;

    case PCI_HEADER_TYPE_BRIDGE:
        *pHeaderType = HEADER_TYPE_BRIDGE;
        break;

    case PCI_HEADER_TYPE_CARDBUS:
        *pHeaderType = HEADER_TYPE_CARDBUS;
        break;

    default:
        WDC_Err("%s: Unknown Header Type [%x] assuming type 0\n", tmp,
            __FUNCTION__);
        *pHeaderType = HEADER_TYPE_NORMAL;
    }

    return WD_STATUS_SUCCESS;
}

static DWORD PCIScanCapsBySlot(WD_PCI_SLOT *pSlot, DWORD dwCapId,
    WDC_PCI_SCAN_CAPS_RESULT *pScanCapsResult, DWORD dwOptions)
{
    DWORD dwStatus, i;
    WD_PCI_SCAN_CAPS scanCaps;

    if (!WdcIsValidPtr(pScanCapsResult,
        "NULL pointer to device capabilities scan results struct"))
    {
        return WD_INVALID_PARAMETER;
    }

    BZERO(scanCaps);
    memcpy(&scanCaps.pciSlot, pSlot, sizeof(WD_PCI_SLOT));
    scanCaps.dwCapId = dwCapId;
    scanCaps.dwOptions = dwOptions;

    if (dwOptions != WD_PCI_SCAN_CAPS_BASIC &&
        dwOptions != WD_PCI_SCAN_CAPS_EXTENDED)
    {
        WDC_Err("PCIScanCapsBySlot: Error. Invalid dwOptions [0x%lx]\n",
            dwOptions);
        return WD_INVALID_PARAMETER;
    }

    dwStatus = WD_PciScanCaps(ghWD, &scanCaps);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDC_Err("PCIScanCapsBySlot: Failed scanning PCI capabilities. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    BZERO(*pScanCapsResult);
    pScanCapsResult->dwNumCaps = scanCaps.dwNumCaps;

    for (i = 0; i < scanCaps.dwNumCaps; i++)
        pScanCapsResult->pciCaps[i] = scanCaps.pciCaps[i];

    WDC_Trace("PCIScanCapsBySlot: PCI capabilities scanned successfully.\n"
        "Found [%ld] matching capabilities\n", pScanCapsResult->dwNumCaps);

    return WD_STATUS_SUCCESS;
}

static DWORD PCIScanCaps(WDC_DEVICE_HANDLE hDev, DWORD dwCapId,
    WDC_PCI_SCAN_CAPS_RESULT *pScanCapsResult, DWORD dwOptions)
{
    if (!WdcIsValidDevHandle(hDev))
    {
        WDC_Err("PCIScanCaps: Invalid device handle");
        return WD_INVALID_HANDLE;
    }

    return PCIScanCapsBySlot(WDC_GET_PPCI_SLOT(hDev), dwCapId, pScanCapsResult,
        dwOptions);
}

DWORD DLLCALLCONV WDC_PciScanCaps(_In_ WDC_DEVICE_HANDLE hDev,
    _In_ DWORD dwCapId, _Outptr_ WDC_PCI_SCAN_CAPS_RESULT *pScanCapsResult)
{
    return PCIScanCaps(hDev, dwCapId, pScanCapsResult, WD_PCI_SCAN_CAPS_BASIC);
}

DWORD DLLCALLCONV WDC_PciScanCapsBySlot(_In_ WD_PCI_SLOT *pPciSlot,
    _In_ DWORD dwCapId, _Outptr_ WDC_PCI_SCAN_CAPS_RESULT *pScanCapsResult)
{
    return PCIScanCapsBySlot(pPciSlot, dwCapId, pScanCapsResult,
        WD_PCI_SCAN_CAPS_BASIC);
}

DWORD DLLCALLCONV WDC_PciScanExtCaps(_In_ WDC_DEVICE_HANDLE hDev,
    _In_ DWORD dwCapId, _Outptr_ WDC_PCI_SCAN_CAPS_RESULT *pScanCapsResult)
{
    return PCIScanCaps(hDev, dwCapId, pScanCapsResult,
        WD_PCI_SCAN_CAPS_EXTENDED);
}

DWORD DLLCALLCONV WDC_PciGetExpressGenBySlot(_In_ WD_PCI_SLOT *pPciSlot)
{
    DWORD dwExpressOffset = 0, dwStatus;
    BYTE bLnkCap = 0;
    WDC_PCI_SCAN_CAPS_RESULT scanResult;

    BZERO(scanResult);
    if (WD_STATUS_SUCCESS != WDC_PciScanCapsBySlot(pPciSlot,
        PCI_CAP_ID_EXP, &scanResult))
    {
        return 0;
    }

    dwExpressOffset = scanResult.pciCaps[0].dwCapOffset;
    if (!dwExpressOffset)
        return 0;

    dwStatus = WDC_PciReadCfgBySlot(pPciSlot, dwExpressOffset + PCI_EXP_LNKCAP,
        &bLnkCap, sizeof(BYTE));
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        return 0;
    }

    if (!bLnkCap)
        return 1;

    return (DWORD)(bLnkCap % 0x10);
}

DWORD DLLCALLCONV WDC_PciGetExpressGen(_In_ WDC_DEVICE_HANDLE hDev)
{
    if (!WdcIsValidDevHandle(hDev))
    {
        WDC_Err("WDC_PciGetExpressGen: Invalid device handle");
        return 0;
    }

    return WDC_PciGetExpressGenBySlot(WDC_GET_PPCI_SLOT(hDev));
}

/* -----------------------------------------------
    Open/close device
   ----------------------------------------------- */
#if !defined(__KERNEL__)
DWORD DLLCALLCONV WDC_PciDeviceOpen(_Outptr_ WDC_DEVICE_HANDLE *phDev,
    _In_ const WD_PCI_CARD_INFO *pDeviceInfo, _In_ const PVOID pDevCtx)
{
    DWORD dwStatus;

    dwStatus = deviceOpen(phDev, (const PVOID)pDeviceInfo, pDevCtx,
        WD_BUS_PCI);
    if (WD_STATUS_SUCCESS != dwStatus)
        WDC_Err("WDC_PciDeviceOpen: Error %s", WdcGetLastErrStr());

    return dwStatus;
}

DWORD DLLCALLCONV WDC_IsaDeviceOpen(_Outptr_ WDC_DEVICE_HANDLE *phDev,
    _In_ const WD_CARD *pDeviceInfo, _In_ const PVOID pDevCtx)
{
    DWORD dwStatus;

    dwStatus = deviceOpen(phDev, (const PVOID)pDeviceInfo, pDevCtx,
        WD_BUS_ISA);
    if (WD_STATUS_SUCCESS != dwStatus)
        WDC_Err("WDC_IsaDeviceOpen: Error %s", WdcGetLastErrStr());

    return dwStatus;
}

DWORD DLLCALLCONV WDC_PciDeviceClose(_In_ WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;

    dwStatus = deviceClose(hDev);
    if (WD_STATUS_SUCCESS != dwStatus)
        WDC_Err("WDC_PciDeviceClose: Error %s", WdcGetLastErrStr());

    return dwStatus;
}

DWORD DLLCALLCONV WDC_IsaDeviceClose(_In_ WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;

    dwStatus = deviceClose(hDev);
    if (WD_STATUS_SUCCESS != dwStatus)
        WDC_Err("WDC_IsaDeviceClose: Error %s", WdcGetLastErrStr());

    return dwStatus;
}

/* -----------------------------------------------
    Open Kernel PlugIn Driver
   ----------------------------------------------- */
DWORD DLLCALLCONV WDC_KernelPlugInOpen(_In_ WDC_DEVICE_HANDLE hDev,
    _In_ const CHAR *pcKPDriverName, _In_ PVOID pKPOpenData)
{
    if (!WdcIsValidDevHandle(hDev))
    {
        WDC_Err("WDC_KernelPlugInOpen: Error %s", WdcGetLastErrStr());
        return WD_INVALID_HANDLE;
    }

    if (!pcKPDriverName || !strcmp(pcKPDriverName, ""))
    {
        WdcSetLastErrStr("Error - Empty Kernel-Plugin driver name\n");
        return WD_INVALID_PARAMETER;
    }

    return KernelPlugInOpen(hDev, pcKPDriverName, pKPOpenData);
}

static DWORD PciMatchDevice(const WD_PCI_SLOT *pSlot,
    WD_PCI_SCAN_CARDS *scanDevices, WD_PCI_ID *pId)
{
    DWORD i;

    for (i = 0; i < scanDevices->dwCards; i++)
    {
        if ((scanDevices->cardSlot[i].dwBus == pSlot->dwBus) &&
            (scanDevices->cardSlot[i].dwSlot == pSlot->dwSlot) &&
            (scanDevices->cardSlot[i].dwFunction == pSlot->dwFunction))
        {
            pId->dwVendorId = scanDevices->cardId[i].dwVendorId;
            pId->dwDeviceId = scanDevices->cardId[i].dwDeviceId;
            return WD_STATUS_SUCCESS;
        }
    }

    return WD_DEVICE_NOT_FOUND;
}

static DWORD PciSlotToId(const WD_PCI_SLOT *pSlot, WD_PCI_ID *pId)
{
    DWORD dwStatus;
    WD_PCI_SCAN_CARDS scanDevices;
    WD_PCI_CONFIG_DUMP pciCfg;
    USHORT vid_did_buff[2];

    BZERO(pciCfg);
    pciCfg.pciSlot = *pSlot;
    pciCfg.pBuffer = vid_did_buff;
    pciCfg.dwOffset = 0;
    pciCfg.dwBytes = sizeof(vid_did_buff);
    pciCfg.fIsRead = TRUE;
    dwStatus = WD_PciConfigDump(ghWD, &pciCfg);
    if (dwStatus == WD_STATUS_SUCCESS && pciCfg.dwResult == PCI_ACCESS_OK)
    {
        pId->dwVendorId = vid_did_buff[0];
        pId->dwDeviceId = vid_did_buff[1];
        return WD_STATUS_SUCCESS;
    }

    /* scanCards.searchId.dwVendorId/dwDeviceId = 0 - all cards */
    BZERO(scanDevices);

    /* Scan in registered devices list to improve performance */
    scanDevices.dwOptions = WD_PCI_SCAN_REGISTERED;
    dwStatus = WD_PciScanCards(ghWD, &scanDevices);
    if (WD_STATUS_SUCCESS == dwStatus)
    {
        /* Registered devices found, check if the requested device is one of
         * them */
        if (PciMatchDevice(pSlot, &scanDevices, pId) == WD_STATUS_SUCCESS)
            return WD_STATUS_SUCCESS;
    }
    else if (WD_DEVICE_NOT_FOUND != dwStatus)
    {
        WdcSetLastErrStr("Failed scanning PCI bus. Error 0x%lx - %s\n",
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    /* At this point we are sure that the requested device was not found in
     * registered devices - scan the entire bus */
    scanDevices.dwOptions = WD_PCI_SCAN_DEFAULT;
    dwStatus = WD_PciScanCards(ghWD, &scanDevices);

    if (PciMatchDevice(pSlot, &scanDevices, pId) == WD_STATUS_SUCCESS)
        return WD_STATUS_SUCCESS;

    /* The requested device was not found in registered devices and also was
     * not found by default bus scan */

    scanDevices.dwOptions = WD_PCI_SCAN_BY_TOPOLOGY;
    dwStatus = WD_PciScanCards(ghWD, &scanDevices);

    if (PciMatchDevice(pSlot, &scanDevices, pId) == WD_STATUS_SUCCESS)
        return WD_STATUS_SUCCESS;

    WdcSetLastErrStr("Failed to locate the device "
        "(bus: 0x%lx, slot: 0x%lx, function: 0x%lx)\n",
        pSlot->dwBus, pSlot->dwSlot, pSlot->dwFunction);

    return WD_DEVICE_NOT_FOUND;
}

static PWDC_DEVICE deviceCreate(const PVOID pDeviceInfo, const PVOID pDevCtx,
    WD_BUS_TYPE bus)
{
    PWDC_DEVICE pDev;

    pDev = (PWDC_DEVICE)malloc(sizeof(WDC_DEVICE));
    if (!pDev)
    {
        WdcSetLastErrStr("deviceCreate: Failed memory allocation\n");
        return NULL;
    }

    BZERO(*pDev);

    switch (bus)
    {
    case WD_BUS_PCI:
    {
        WD_PCI_CARD_INFO *pInfo = (WD_PCI_CARD_INFO *)pDeviceInfo;

        pDev->slot = pInfo->pciSlot;
        pDev->cardReg.Card = pInfo->Card;

        if (WD_STATUS_SUCCESS != PciSlotToId(&pDev->slot,
            &pDev->id))
        {
            goto Error;
        }

        break;
    }
    case WD_BUS_ISA:
    {
        pDev->cardReg.Card = *(WD_CARD *)pDeviceInfo;
        break;
    }
    default:
        WdcSetLastErrStr("Error - Invalid bus type (0x%lx)\n", bus);
        goto Error;
        break;
    }

    pDev->pCtx = pDevCtx;

    return pDev;

Error:
    deviceDestroy(pDev);
    return NULL;
}

static DWORD deviceOpen(WDC_DEVICE_HANDLE *phDev, const PVOID pDeviceInfo,
    const PVOID pDevCtx, WD_BUS_TYPE bus)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev;

    if (!WdcIsValidPtr(phDev, "NULL device handle pointer") ||
        !WdcIsValidPtr(pDeviceInfo, "NULL device information pointer"))
    {
        return WD_INVALID_PARAMETER;
    }

    *phDev = NULL;

    pDev = deviceCreate(pDeviceInfo, pDevCtx, bus);
    if (!pDev)
        return WD_INSUFFICIENT_RESOURCES;

    dwStatus = WD_CardRegister(ghWD, &pDev->cardReg);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WdcSetLastErrStr("Failed registering the device. Error [0x%lx - %s]\n",
            dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    dwStatus = SetDeviceInfo(pDev);
    if (WD_STATUS_SUCCESS != dwStatus)
        goto Error;

    *phDev = (WDC_DEVICE_HANDLE)pDev;

    return WD_STATUS_SUCCESS;

Error:
    deviceClose((WDC_DEVICE_HANDLE)pDev);
    *phDev = NULL;
    return dwStatus;
}

static void deviceDestroy(PWDC_DEVICE pDev)
{
    if (!pDev)
        return;

    if (pDev->pAddrDesc)
        free(pDev->pAddrDesc);

    free(pDev);
}

static DWORD deviceClose(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    if (!WdcIsValidDevHandle(hDev))
        return WD_INVALID_PARAMETER;

    if (pDev->hIntThread)
    {
        dwStatus = WDC_IntDisable(hDev);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            WdcSetLastErrStr("Failed disabling interrupt. Error [0x%lx - %s]\n",
                dwStatus, Stat2Str(dwStatus));
        }
    }

    if (pDev->hEvent)
    {
        dwStatus = WDC_EventUnregister(hDev);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            WdcSetLastErrStr("Failed unregistering events. Error "
                "[0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
        }
    }

    if (WDC_IS_KP(pDev))
    {
        dwStatus = WD_KernelPlugInClose(ghWD, &pDev->kerPlug);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            WdcSetLastErrStr("Failed closing Kernel PlugIn handle [%p]\n"
                "Error [0x%lx - %s]\n", WDC_GET_KP_HANDLE(pDev), dwStatus,
                Stat2Str(dwStatus));
        }
    }

    if (WDC_GET_CARD_HANDLE(pDev))
    {
        dwStatus = WD_CardUnregister(ghWD, &pDev->cardReg);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            WdcSetLastErrStr("Failed unregistering the device (handle %p). "
                "Error [0x%lx - %s]\n", WDC_GET_CARD_HANDLE(pDev), dwStatus,
                Stat2Str(dwStatus));
        }

#if defined (LINUX)
        {
            DWORD dwNumItems = pDev->cardReg.Card.dwItems;
            WD_ITEMS* pItem = pDev->cardReg.Card.Item;
            WDC_ADDR_DESC addrDescs[MAX_ADDR_SPACE_NUM];
            WDC_ADDR_DESC* pAddrDesc;
            DWORD i;

            BZERO(addrDescs);
            for (i = 0; i < dwNumItems; i++, pItem++)
            {
                if (pItem->item == ITEM_MEMORY &&
                    pItem->I.Mem.dwOptions & WD_ITEM_MEM_USER_MAP)
                {
                    pAddrDesc = &addrDescs[pItem->I.Mem.dwBar];
                    munmap((PVOID)pAddrDesc->pUserDirectMemAddr,
                        pItem->I.Mem.qwBytes);
                }
            }
        }
#endif
    }

    deviceDestroy(pDev);

    return dwStatus;
}

static DWORD KernelPlugInOpen(WDC_DEVICE_HANDLE hDev,
    const CHAR *pcKPDriverName, PVOID pKPOpenData)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    strncpy(pDev->kerPlug.cDriverName, pcKPDriverName,
        sizeof(pDev->kerPlug.cDriverName) - 1);
    pDev->kerPlug.pOpenData = pKPOpenData;

    dwStatus = WD_KernelPlugInOpen(ghWD, &pDev->kerPlug);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WdcSetLastErrStr("Failed opening a Kernel PlugIn handle. "
            "Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

static DWORD SetDeviceInfo(PWDC_DEVICE pDev)
{
    DWORD i;
    DWORD dwNumAddrs = 0;
    DWORD dwNumItems = pDev->cardReg.Card.dwItems;
    WD_ITEMS *pItem = pDev->cardReg.Card.Item;
    WDC_ADDR_DESC addrDescs[MAX_ADDR_SPACE_NUM];
    WDC_ADDR_DESC *pAddrDesc;

    BZERO(addrDescs);
    for (i = 0; i < dwNumItems; i++, pItem++)
    {
        switch (pItem->item)
        {
        case ITEM_INTERRUPT:
            pDev->Int.hInterrupt = pItem->I.Int.hInterrupt;
            pDev->Int.dwOptions = pItem->I.Int.dwOptions;
            break;

        case ITEM_MEMORY:
            pAddrDesc = &addrDescs[pItem->I.Mem.dwBar];
            pAddrDesc->fIsMemory = TRUE;
            pAddrDesc->dwAddrSpace = pItem->I.Mem.dwBar;
            pAddrDesc->pAddr = pItem->I.Mem.pTransAddr;
            pAddrDesc->pUserDirectMemAddr = pItem->I.Mem.pUserDirectAddr;

#if defined (LINUX)

    #if 0
            /* on ARM64 on certain platforms, only this method works
            currently for non-block transfer */
            pItem->I.Mem.dwOptions |= WD_ITEM_MEM_USER_MAP;
    #endif
            if (pItem->I.Mem.dwOptions & WD_ITEM_MEM_USER_MAP)
            {
                int fd = open("/dev/mem", O_RDWR | O_SYNC);
                if (fd < 0)
                    return WD_FAILED_USER_MAPPING;

                pAddrDesc->pUserDirectMemAddr = (UPTR)mmap(NULL,
                    pItem->I.Mem.qwBytes, PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, pItem->I.Mem.pPhysicalAddr);
                if (pAddrDesc->pUserDirectMemAddr == (UPTR)MAP_FAILED)
                    return WD_FAILED_USER_MAPPING;

                WDC_Trace("%s: Mapping item [%d] from user mode. "
                    "pUserDirectMemAddr[0x%lx]\n", __FUNCTION__, i,
                    pAddrDesc->pUserDirectMemAddr);
                close(fd);
            }
#endif
            pAddrDesc->qwBytes = pItem->I.Mem.qwBytes;

            pAddrDesc->dwItemIndex = i;
            dwNumAddrs = MAX(pAddrDesc->dwAddrSpace + 1, dwNumAddrs);
            break;

        case ITEM_IO:
            pAddrDesc = &addrDescs[pItem->I.IO.dwBar];
            pAddrDesc->fIsMemory = FALSE;
            pAddrDesc->dwAddrSpace = pItem->I.IO.dwBar;
            pAddrDesc->pAddr = pItem->I.IO.pAddr;
            pAddrDesc->qwBytes = pItem->I.IO.dwBytes;

            pAddrDesc->dwItemIndex = i;
            dwNumAddrs = MAX(pAddrDesc->dwAddrSpace + 1, dwNumAddrs);
            break;

        default:
            break;
        }
    }

    if (dwNumAddrs)
    {
        pDev->pAddrDesc = (WDC_ADDR_DESC *)malloc(dwNumAddrs *
            sizeof(addrDescs[0]));
        if (!pDev->pAddrDesc)
        {
            WdcSetLastErrStr("SetDeviceInfo: Failed memory allocation\n");
            return WD_INSUFFICIENT_RESOURCES;
        }
        memcpy(pDev->pAddrDesc, addrDescs, dwNumAddrs * sizeof(addrDescs[0]));
        pDev->dwNumAddrSpaces = dwNumAddrs;
    }

    return WD_STATUS_SUCCESS;
}

#endif

/* -----------------------------------------------
    Set card cleanup commands
   ----------------------------------------------- */
DWORD WDC_CardCleanupSetup(_In_ WDC_DEVICE_HANDLE hDev,
    _In_ WD_TRANSFER *pTransCmds, _In_ DWORD dwCmds, _In_ BOOL fForceCleanup)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    WD_CARD_CLEANUP cardCleanup;

    if (!pTransCmds || !dwCmds)
        return WD_INVALID_PARAMETER;

    cardCleanup.hCard = WDC_GET_CARD_HANDLE(pDev);
    cardCleanup.Cmd = pTransCmds;
    cardCleanup.dwCmds = dwCmds;
    cardCleanup.dwOptions = fForceCleanup ? WD_FORCE_CLEANUP : 0;

    return WD_CardCleanupSetup(ghWD, &cardCleanup);
}

/* -----------------------------------------------
    Send Kernel PlugIn messages
   ----------------------------------------------- */
DWORD DLLCALLCONV WDC_CallKerPlug(_In_ WDC_DEVICE_HANDLE hDev, _In_ DWORD dwMsg,
    _Inout_ PVOID pData, _Outptr_ PDWORD pdwResult)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    WD_KERNEL_PLUGIN_CALL kpCall;

    BZERO(kpCall);
    kpCall.hKernelPlugIn = WDC_GET_KP_HANDLE(pDev);
    kpCall.dwMessage = dwMsg;
    kpCall.pData = pData;

    dwStatus = WD_KernelPlugInCall(ghWD, &kpCall);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDC_Err("WDC_CallKerPlug: Failed sending 0x%lx message to the Kernel "
            "PlugIn (%s). Error 0x%lx - %s\n", dwMsg,
            pDev->kerPlug.cDriverName, dwStatus, Stat2Str(dwStatus));
    }
    else if (pdwResult)
    {
        *pdwResult = kpCall.dwResult;
    }

    return dwStatus;
}

