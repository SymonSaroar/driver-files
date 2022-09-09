/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/*  This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation.
 * This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
   v2 for more details.
 * You should have received a copy of the GNU General Public License
   along with this program. If not, contact Jungo Connectivity Ltd. at
   support@jungo.com */

#ifndef _WDSRIOV_INTERFACE_H_
#define _WDSRIOV_INTERFACE_H_

#if defined(LINUX_SRIOV_MODULE) || defined(LINUX_SRIOV_SUPPORT)
    #ifdef WD_DRIVER_NAME_CHANGE
        #define WD_SRIOV_FUNC_NAME(func) driver_sriov_##func
    #else
        #define WD_SRIOV_FUNC_NAME(func) windrvr1511_##func
    #endif
#else
    #define WD_SRIOV_FUNC_NAME(func) func
#endif

#if defined(__cplusplus)
    extern "C" {
#endif

int WD_SRIOV_FUNC_NAME(OS_pci_enable_sriov)(void *pdev, int nr_virtfn);

void WD_SRIOV_FUNC_NAME(OS_pci_disable_sriov)(void *pdev);

int WD_SRIOV_FUNC_NAME(OS_pci_sriov_is_vf)(void *pdev);

int WD_SRIOV_FUNC_NAME(OS_pci_sriov_is_assigned)(void *pdev);

int WD_SRIOV_FUNC_NAME(OS_pci_sriov_get_num_vf)(void *pdev);

int WD_SRIOV_FUNC_NAME(OS_pci_sriov_vf_get_owner)(void *pdev,
    unsigned int *dwDomain, unsigned int *dwBus, unsigned int *dwSlot,
    unsigned int *dwFunc);

#ifdef __cplusplus
}
#endif

#endif /* _WDSRIOV_INTERFACE_H_ */

