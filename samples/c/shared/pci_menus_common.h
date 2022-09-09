/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _PCI_MENUS_COMMON_H_
#define _PCI_MENUS_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__KERNEL__)
#include "diag_lib.h"
#include "status_strings.h"
#include "wdc_defs.h"
#include "pci_strings.h"
#include "wdc_diag_lib.h"
#include "wds_diag_lib.h"

#ifndef ISA

BOOL MenuCommonIsDeviceNull(DIAG_MENU_OPTION *pMenu);

DIAG_MENU_OPTION *MenuCommonScanBusInit(DIAG_MENU_OPTION *pParentMenu);

typedef struct {
    WDC_DEVICE_HANDLE *phDev;
    const WDC_REG *pCfgRegs;
    DWORD dwCfgRegsNum;
    const WDC_REG *pCfgExpRegs;
    DWORD dwCfgExpRegsNum;
} MENU_CTX_CFG;

DIAG_MENU_OPTION *MenuCommonCfgInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_CFG *pCfgCtx);

typedef struct {
    MENU_CALLBACK eventsEnableCb;
    MENU_CALLBACK eventsDisableCb;
    MENU_CALLBACK eventsMenuEntryCb;
    MENU_CALLBACK eventsMenuExitCb;
} MENU_EVENTS_CALLBACKS;

typedef void (*DIAG_EVENT_HANDLER)(WDC_DEVICE_HANDLE hDev, DWORD dwAction);

typedef struct {
    WDC_DEVICE_HANDLE *phDev;
    BOOL fRegistered;
    DIAG_EVENT_HANDLER DiagEventHandler;
} MENU_CTX_EVENTS;

DIAG_MENU_OPTION *MenuCommonEventsInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_EVENTS *pEventsMenusCtx, MENU_EVENTS_CALLBACKS *pEventsCallbacks);

#ifdef LINUX

typedef struct {
    WDC_DEVICE_HANDLE *phDev;
    DWORD dwNumVFs;
    BOOL *pfIsEnabledSRIOV;
} MENU_CTX_SRIOV;

typedef struct {
    MENU_CALLBACK sriovEnableCb;
    MENU_CALLBACK sriovDisableCb;
    MENU_CALLBACK sriovMenuEntryCb;
    MENU_CALLBACK sriovMenuExitCb;
} MENU_SRIOV_CALLBACKS;

DIAG_MENU_OPTION* MenuCommonSriovInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_SRIOV *pSriovCtx, MENU_SRIOV_CALLBACKS *pSriovCallbacks);

#endif /* ifdef LINUX */
#endif /* ifndef ISA */

typedef struct {
    WDC_DEVICE_HANDLE *phDev;
    DWORD dwAddrSpace;
    WDC_ADDR_MODE mode;
    BOOL fBlock;
} MENU_CTX_READ_WRITE_ADDR;

DIAG_MENU_OPTION *MenuCommonRwAddrInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_READ_WRITE_ADDR *pRwAddrMenusCtx);

typedef struct {
    MENU_CALLBACK interruptsEnableCb;
    MENU_CALLBACK interruptsDisableCb;
    MENU_CALLBACK interruptsMenuEntryCb;
    MENU_CALLBACK interruptsMenuExitCb;
} MENU_INTERRUPTS_CALLBACKS;

typedef void (*DIAG_INT_HANDLER)(WDC_DEVICE_HANDLE hDev, PVOID pIntResult);

typedef struct {
    WDC_DEVICE_HANDLE *phDev;
    BOOL fIntsEnabled;
    PVOID pData; /* Optional additional data. If needed, pass to
                    xxx_IntEnable in the xxx_lib.h file */
    DIAG_INT_HANDLER funcIntHandler;
} MENU_CTX_INTERRUPTS;

DIAG_MENU_OPTION *MenuCommonInterruptsInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_INTERRUPTS *pInterruptsMenusCtx,
    MENU_INTERRUPTS_CALLBACKS *pInterruptsCallbacks);


typedef struct {
    WDC_DEVICE_HANDLE *phDev;
    const WDC_REG *pRegsArr;
    DWORD dwRegsNum;
    BOOL fIsConfig;
    CHAR sModuleName[MAX_NAME];
} MENU_CTX_RW_REGS;

DIAG_MENU_OPTION *MenuCommonRwRegsInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_RW_REGS *pRegsMenusCtx);

#endif

#ifdef __cplusplus
}
#endif

#endif /* _PCI_MENUS_COMMON_H_ */
