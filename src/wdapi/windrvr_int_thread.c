/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/*
 * File - windrvr_int_thread.c
 *
 * Implementation of a thread that waits for WinDriver events.
 */

#if defined(__KERNEL__)
    #include "kpstdlib.h"
#else
    #include "utils.h"
#endif

#include "windrvr_int_thread.h"
#include "wdc_lib.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#if !defined(__KERNEL__)
    typedef struct
    {
        HANDLE hWD;
        INT_HANDLER func;
        PVOID pData;
        WD_INTERRUPT *pInt;
        void *thread;
    } INT_THREAD_DATA;

    static void DLLCALLCONV interrupt_thread_handler(void *data)
    {
        DWORD dwStatus;
        INT_THREAD_DATA *pThread = (INT_THREAD_DATA *)data;
        volatile DWORD is_stopped = 0;

        for (;;)
        {
            dwStatus = WD_IntWait(pThread->hWD, pThread->pInt);
            if (dwStatus == WD_OPERATION_ALREADY_DONE)
            {
                WDC_Err("WD_IntWait: Failed.\n"
                    "Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
                return;
            }

            is_stopped = pThread->pInt->fStopped;
            if (is_stopped == INTERRUPT_STOPPED)
                break;

            if (is_stopped == INTERRUPT_INTERRUPTED)
                continue;

            pThread->func(pThread->pData);
        }
    }

    DWORD DLLCALLCONV InterruptEnable(HANDLE *phThread, HANDLE hWD,
        WD_INTERRUPT *pInt, INT_HANDLER func, PVOID pData)
    {
        INT_THREAD_DATA *pThread;
        DWORD dwStatus;

        *phThread = NULL;

        pThread = (INT_THREAD_DATA *)malloc(sizeof(INT_THREAD_DATA));
        if (!pThread)
            return WD_INSUFFICIENT_RESOURCES;

        dwStatus = WD_IntEnable(hWD, pInt);
        if (dwStatus)
            goto Error;

        BZERO(*pThread);
        pThread->func = func;
        pThread->pData = pData;
        pThread->hWD = hWD;
        pThread->pInt = pInt;

        dwStatus = ThreadStart(&pThread->thread, interrupt_thread_handler,
            (void *)pThread);
        if (dwStatus)
        {
            WD_IntDisable(hWD, pInt);
            goto Error;
        }

        *phThread = (HANDLE)pThread;

        return WD_STATUS_SUCCESS;

Error:
        if (pThread)
            free(pThread);
        return dwStatus;
    }

    DWORD DLLCALLCONV InterruptDisable(HANDLE hThread)
    {
        WD_INTERRUPT tmpInt;
        INT_THREAD_DATA *pThread = (INT_THREAD_DATA *)hThread;
        DWORD dwStatus;

        if (!pThread)
            return WD_INVALID_HANDLE;

        /* Copy pInt to a local variable to prevent a data race with data that
         * is returned from WD_IntWait */
        tmpInt = *pThread->pInt;
        dwStatus = WD_IntDisable(pThread->hWD, &tmpInt);

        ThreadWait(pThread->thread);
        free(pThread);
        return dwStatus;
    }
#endif

#ifdef __cplusplus
}
#endif  // __cplusplus

