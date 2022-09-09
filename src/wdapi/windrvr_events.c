/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/*
 * File - windrvr_events.c
 *
 * API for receiving events from WinDriver (PCI, USB)
 */

#if !defined(_WD_KERNEL_)
    #if defined(__KERNEL__)
        #include "kpstdlib.h"
    #endif
    #include "windrvr_int_thread.h"
    #include "utils.h"
#endif
#include "windrvr_events.h"

#if defined(__KERNEL__) && defined(WIN32)
    #pragma warning( disable :4013 4100)
#endif

WD_EVENT* DLLCALLCONV EventAlloc(DWORD dwNumMatchTables)
{
    DWORD dwSize;
    DWORD dwNeedAllocate;
    WD_EVENT *peNew;

    dwNeedAllocate = dwNumMatchTables > 0 ? dwNumMatchTables - 1 : 0;

    dwSize = sizeof(WD_EVENT) + sizeof(WDU_MATCH_TABLE) * dwNeedAllocate;
    peNew = (WD_EVENT *)malloc(dwSize);
    if (!peNew)
        return NULL;

    memset(peNew, 0, dwSize);
    peNew->dwNumMatchTables = dwNeedAllocate + 1;

    return peNew;
}

WD_EVENT *DLLCALLCONV PciEventCreate(WD_PCI_ID cardId, WD_PCI_SLOT pciSlot,
    DWORD dwOptions, DWORD dwAction)
{
    WD_EVENT *peNew;

    peNew = EventAlloc(1);
    if (!peNew)
        return NULL;

    peNew->dwEventType = WD_EVENT_TYPE_PCI;
    peNew->dwAction = dwAction;
    peNew->dwOptions = dwOptions;
    peNew->u.Pci.cardId = cardId;
    peNew->u.Pci.pciSlot = pciSlot;

    return peNew;
}

void DLLCALLCONV EventFree(WD_EVENT *pe)
{
    free(pe);
}

WD_EVENT* DLLCALLCONV EventDup(WD_EVENT *peSrc)
{
    WD_EVENT *peNew;
    DWORD dwNeedAllocate, dwSize;

    dwNeedAllocate = peSrc->dwNumMatchTables > 0 ?
        peSrc->dwNumMatchTables - 1 : 0;
    dwSize = sizeof(WD_EVENT) + sizeof(WDU_MATCH_TABLE) * dwNeedAllocate;

    peNew = EventAlloc(peSrc->dwNumMatchTables);
    if (peNew)
        memcpy(peNew, peSrc, dwSize);

    return peNew;
}

#if !defined(_WD_KERNEL_)
WD_EVENT* DLLCALLCONV UsbEventCreate(WDU_MATCH_TABLE *pMatchTables,
    DWORD dwNumMatchTables, DWORD dwOptions, DWORD dwAction)
{
    WD_EVENT *peNew;

    if ((pMatchTables && !dwNumMatchTables)||
        (!pMatchTables && dwNumMatchTables))
    {
        return NULL;
    }

    peNew = EventAlloc(dwNumMatchTables);
    if (!peNew)
        return NULL;

    peNew->dwEventType = WD_EVENT_TYPE_USB;
    peNew->dwAction = dwAction;
    peNew->dwOptions = dwOptions;

    if (dwNumMatchTables)
    {
        memcpy(peNew->matchTables, pMatchTables, sizeof(WDU_MATCH_TABLE) *
            dwNumMatchTables);
    }

    return peNew;
}

typedef struct
{
    HANDLE         hWD;

    EVENT_HANDLER  func;
    void          *data;

    WD_INTERRUPT   Int;        /* Int.hInterrupt holds hEvent */
    WD_EVENT_TYPE  dwEventType;

    HANDLE         thread;
} local_event_handle_t;

static void DLLCALLCONV event_handler(void *h)
{
    local_event_handle_t *handle = (local_event_handle_t *)h;
    WD_EVENT *pePulled;

    pePulled = EventAlloc(1);
    pePulled->hEvent = handle->Int.hInterrupt;
    WD_EventPull(handle->hWD, pePulled);

    if (pePulled->dwAction)
        handle->func(pePulled, handle->data);

    if (pePulled->dwOptions & WD_ACKNOWLEDGE)
        WD_EventSend(handle->hWD, pePulled);

    EventFree(pePulled);
}

DWORD DLLCALLCONV EventRegister(HANDLE *phEvent, HANDLE hWD, WD_EVENT *pEvent,
    EVENT_HANDLER pFunc, void *pData)
{
    DWORD dwStatus;
    local_event_handle_t *handle;

    *phEvent = NULL;

    handle = (local_event_handle_t *)malloc(sizeof(*handle));
    if (!handle)
    {
        dwStatus = WD_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    BZERO(*handle);
    handle->hWD = hWD;
    handle->func = pFunc;
    handle->data = pData;
    handle->dwEventType = pEvent->dwEventType;

    dwStatus = WD_EventRegister(hWD, pEvent);
    if (dwStatus)
        goto Error;

    handle->Int.hInterrupt = pEvent->hEvent;
    dwStatus = InterruptEnable(&handle->thread, hWD, &handle->Int,
        event_handler, (PVOID)handle);
    if (dwStatus)
        goto Error;

    *phEvent = (HANDLE)handle;
    return dwStatus;

Error:
    if (handle && handle->Int.hInterrupt)
        WD_EventUnregister(hWD, pEvent);
    if (handle)
        free(handle);
    return dwStatus;
}

DWORD DLLCALLCONV EventUnregister(HANDLE hEvent)
{
    local_event_handle_t *handle = (local_event_handle_t *)hEvent;
    WD_EVENT *pe;
    DWORD dwStatus;

    if (!handle)
        return WD_INVALID_HANDLE;

    pe = EventAlloc(1);
    pe->hEvent = handle->Int.hInterrupt;
    pe->dwEventType = handle->dwEventType;

    InterruptDisable(handle->thread);
    dwStatus = WD_EventUnregister(handle->hWD, pe);

    EventFree(pe);
    free(handle);
    return dwStatus;
}
#endif

