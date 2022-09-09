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

#include "linux_common.h"
#include "wdsriov_interface.h"
#include "wd_ver.h"
#include <linux/pci.h>
#include <linux/module.h>

#if defined(WD_DRIVER_NAME_CHANGE)
    #define DRIVER_NAME "%DRIVER_NAME%"
#else
    #define DRIVER_NAME "windrvr1511_sriov"
#endif

#if defined(MODULE_LICENSE)
    MODULE_LICENSE("GPL");
#endif
#if defined(MODULE_AUTHOR)
    MODULE_AUTHOR("Jungo Connectivity");
#endif
#if defined(MODULE_DESCRIPTION)
    MODULE_DESCRIPTION(DRIVER_NAME " v" WD_VERSION_STR);
#endif

EXPORT_SYMBOL(WD_SRIOV_FUNC_NAME(OS_pci_enable_sriov));
int WD_SRIOV_FUNC_NAME(OS_pci_enable_sriov)(void *pdev, int nr_virtfn)
{
    int ret = 0;

#ifdef CONFIG_PCI_IOV
    if (nr_virtfn > 0)
        ret = pci_enable_sriov((struct pci_dev *)pdev, nr_virtfn);
#endif

    return ret;
}

EXPORT_SYMBOL(WD_SRIOV_FUNC_NAME(OS_pci_disable_sriov));
void WD_SRIOV_FUNC_NAME(OS_pci_disable_sriov)(void *pdev)
{
#ifdef CONFIG_PCI_IOV
    pci_disable_sriov((struct pci_dev *)pdev);
#endif
}

EXPORT_SYMBOL(WD_SRIOV_FUNC_NAME(OS_pci_sriov_is_vf));
int WD_SRIOV_FUNC_NAME(OS_pci_sriov_is_vf)(void *pdev)
{
    struct pci_dev *dev = (struct pci_dev *)pdev;

#ifdef CONFIG_PCI_IOV
    if (dev && dev->is_virtfn)
        return 1;
#endif

    return 0;
}

EXPORT_SYMBOL(WD_SRIOV_FUNC_NAME(OS_pci_sriov_is_assigned));
int WD_SRIOV_FUNC_NAME(OS_pci_sriov_is_assigned)(void *pdev)
{
    struct pci_dev *dev = (struct pci_dev *)pdev;

#ifdef CONFIG_PCI_IOV
    if (dev && pci_is_dev_assigned(dev))
        return 1;
#endif

    return 0;
}

EXPORT_SYMBOL(WD_SRIOV_FUNC_NAME(OS_pci_sriov_get_num_vf));
int WD_SRIOV_FUNC_NAME(OS_pci_sriov_get_num_vf)(void *pdev)
{
#ifdef CONFIG_PCI_IOV
     return pci_num_vf((struct pci_dev *)pdev);
#else
     return 0;
#endif
}

EXPORT_SYMBOL(WD_SRIOV_FUNC_NAME(OS_pci_sriov_vf_get_owner));
int WD_SRIOV_FUNC_NAME(OS_pci_sriov_vf_get_owner)(void *pdev,
    unsigned int *dwDomain, unsigned int *dwBus, unsigned int *dwSlot,
    unsigned int *dwFunc)
{
#ifdef CONFIG_PCI_IOV
    struct pci_dev *dev = (struct pci_dev *)pdev;
    struct pci_dev *physfn;

    if (!dwBus || !dwSlot || !dwFunc)
        return 0;

    if (!WD_SRIOV_FUNC_NAME(OS_pci_sriov_is_vf)(pdev))
        return 0;

    physfn = dev->physfn;
    if (!physfn)
        return 0;

    *dwDomain = pci_domain_nr(physfn->bus);
    *dwBus = physfn->bus->number;
    *dwSlot = PCI_SLOT(physfn->devfn);
    *dwFunc = PCI_FUNC(physfn->devfn);

    return 1;
#else
    return 0;
#endif
}

int init_module(void)
{
    printk("%s v%s loaded\n", DRIVER_NAME, WD_VERSION_STR);
    return 0;
}

void cleanup_module(void)
{
    printk("%s v%s unloaded\n", DRIVER_NAME, WD_VERSION_STR);
}

