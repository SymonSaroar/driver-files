/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/****************************************************************
* File: wdc_sriov.c - Implementation of sriov WDC API -     *
*****************************************************************/

#include "wdc_lib.h"
#include "wdc_defs.h"
#include "wdc_err.h"
#include "status_strings.h"

DWORD DLLCALLCONV WDC_PciSriovEnable(_In_ WDC_DEVICE_HANDLE hDev,
    _In_ DWORD dwNumVFs)
{
    DWORD dwStatus;
    WD_PCI_SRIOV sriov;

    WDC_Trace("WDC_PciSriovEnable\n");

    sriov.pciSlot = *(WDC_GET_PPCI_SLOT(hDev));
    sriov.dwNumVFs = dwNumVFs;

    dwStatus = WD_PciSriovEnable(WDC_GetWDHandle(), &sriov);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDC_Err("WDC_SriovEnable: Failed enabling. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

DWORD DLLCALLCONV WDC_PciSriovDisable(_In_ WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;
    WD_PCI_SRIOV sriov;

    sriov.pciSlot = *(WDC_GET_PPCI_SLOT(hDev));

    dwStatus = WD_PciSriovDisable(WDC_GetWDHandle(), &sriov);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDC_Err("WDC_SriovDisable: Failed disabling. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

DWORD DLLCALLCONV WDC_PciSriovGetNumVFs(_In_ WDC_DEVICE_HANDLE hDev,
    _Outptr_ DWORD *pdwNumVFs)
{
    DWORD dwStatus;
    WD_PCI_SRIOV sriov;

    if (!pdwNumVFs)
    {
        WDC_Err("WDC_PciSriovGetNumVFs: Invalid param pdwNumVFs\n");
        return WD_INVALID_PARAMETER;
    }

    sriov.pciSlot = *(WDC_GET_PPCI_SLOT(hDev));

    dwStatus = WD_PciSriovGetNumVFs(WDC_GetWDHandle(), &sriov);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        WDC_Err("WDC_PciSriovGetNumVFs: Failed getting vf count. "
            "Error [0x%lx - %s]\n", dwStatus, Stat2Str(dwStatus));
        sriov.dwNumVFs = 0;
    }

    *pdwNumVFs = sriov.dwNumVFs;

    return dwStatus;
}

