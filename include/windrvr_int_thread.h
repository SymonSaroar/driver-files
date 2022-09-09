/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _WINDRVR_INT_THREAD_H_
#define _WINDRVR_INT_THREAD_H_

#include "windrvr.h"
#include "windrvr_events.h"
#include "status_strings.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (DLLCALLCONV * INT_HANDLER)(PVOID pData);
typedef INT_HANDLER INT_HANDLER_FUNC;

DWORD DLLCALLCONV InterruptEnable(HANDLE *phThread, HANDLE hWD,
    WD_INTERRUPT *pInt, INT_HANDLER func, PVOID pData);

DWORD DLLCALLCONV InterruptDisable(HANDLE hThread);

#ifdef __cplusplus
}
#endif

#endif /* _WINDRVR_INT_THREAD_H_ */

