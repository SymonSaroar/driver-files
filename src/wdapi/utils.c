/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#include "utils.h"
#include "windrvr.h"

#if defined(WIN32)
    #include <process.h>

    #ifndef WAIT_OBJECT_0
        #define WAIT_OBJECT_0 0x00000000L
    #endif
    #ifndef WAIT_FAILED
        #define WAIT_FAILED 0xFFFFFFFF
    #endif
#endif

#if defined(UNIX)
    #include <pthread.h>
    #include <sys/time.h>
    #include <unistd.h>
    #include <errno.h>
#endif

#if !defined(__KERNEL__)
    #include <stdarg.h>
    #include <stdio.h>
    #include <wchar.h>
    #if !defined (APPLE)
        #include <malloc.h>
    #endif
#else
    #if defined(LINUX)
        #define sprintf LINUX_sprintf
    #endif
#endif

#if defined(UNIX)
    typedef struct {
        pthread_cond_t cond;
        pthread_mutex_t mutex;
        BOOL signaled;
    } wd_linux_event_t;
#endif

#if defined(__KERNEL__) && defined(WIN32)
    #pragma warning(disable :4013 4100)
#endif

#if defined(WIN32)
    #define THREAD_WAIT_CHECK
    #define WAIT_THREAD_PERIOD 10

    typedef struct
    {
        void *h_thread;
        BOOL is_running;
    } thread_handle_t;
#endif

#if !defined(WIN32) || defined(_MT)
/* Threads functions */
typedef struct
{
    HANDLER_FUNC func;
    void *data;
    #if defined(THREAD_WAIT_CHECK)
        thread_handle_t *h_thread;
    #endif
} thread_struct_t;

#if defined(WIN32)
    static unsigned int DLLCALLCONV thread_handler(void *data)
#else
    static void *thread_handler(void *data)
#endif
{
    thread_struct_t *t = (thread_struct_t *)data;

    t->func(t->data);
#if defined(THREAD_WAIT_CHECK)
    t->h_thread->is_running = FALSE;
#endif
    free(t);
#if defined(WIN32)
    _endthreadex(0);
#endif

    return 0;
}

DWORD DLLCALLCONV ThreadStart(_Outptr_ HANDLE *phThread,
    _In_ HANDLER_FUNC pFunc, _In_ void *pData)
{
    thread_struct_t *t;
    #if defined(WIN32)
        DWORD dwTmp;
        #if defined(THREAD_WAIT_CHECK)
            thread_handle_t *h;
        #endif
    #endif
    void *ret = NULL;

    *phThread = NULL;

    t = (thread_struct_t *)malloc(sizeof(thread_struct_t));
    if (!t)
        return WD_INSUFFICIENT_RESOURCES;

    t->func = pFunc;
    t->data = pData;
    #if defined(THREAD_WAIT_CHECK)
        h = (thread_handle_t *)malloc(sizeof(thread_handle_t));
        if (!h)
        {
           free(t);
           return WD_INSUFFICIENT_RESOURCES;
        }

        h->is_running = TRUE;
        t->h_thread = h;
    #endif

    #if defined(WIN32)
        ret = (void *)_beginthreadex(NULL, 0x1000, thread_handler, (void *)t, 0,
            (unsigned int *)&dwTmp);
    #elif defined(UNIX)
        ret = malloc(sizeof(pthread_t));
        if (ret)
        {
            int err;

            err = pthread_create((pthread_t *)ret, NULL, thread_handler,
                (PVOID)t);
            if (err)
            {
                free(ret);
                ret = NULL;
            }
        }
    #endif
    if (!ret)
    {
        free(t);
        #if defined(THREAD_WAIT_CHECK)
            free(h);
        #endif
        return WD_INSUFFICIENT_RESOURCES;
    }

    #if defined(THREAD_WAIT_CHECK)
        h->h_thread = ret;
        *phThread = (HANDLE)h;
    #else
        *phThread = ret;
    #endif

    return WD_STATUS_SUCCESS;
}

void DLLCALLCONV ThreadWait(_In_ HANDLE hThread)
{
    #if defined(WIN32)
        #if defined(THREAD_WAIT_CHECK)
            thread_handle_t *h = (thread_handle_t *)hThread;
            DWORD rc = WAIT_FAILED;

            while (h->is_running && rc != WAIT_OBJECT_0)
                rc = WaitForSingleObject(h->h_thread, WAIT_THREAD_PERIOD);

            CloseHandle(h->h_thread);
            free(h);
        #else
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
        #endif
    #elif defined(UNIX)
        pthread_join(*((pthread_t *)hThread), NULL);
        free(hThread);
    #endif
}
/* End of threads functions */

#endif /* defined(_MT) */

/* Synchronization objects */

/* Auto-reset events */
DWORD DLLCALLCONV OsEventCreate(_Outptr_ HANDLE *phOsEvent)
{
#if defined(__KERNEL__)
    return WD_NOT_IMPLEMENTED;
#else
#if defined(WIN32)
    *phOsEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    return *phOsEvent ? WD_STATUS_SUCCESS : WD_INSUFFICIENT_RESOURCES;
#elif defined(UNIX)
    wd_linux_event_t *linux_event =
        (wd_linux_event_t *)malloc(sizeof(wd_linux_event_t));

    if (!linux_event)
        return WD_INSUFFICIENT_RESOURCES;

    memset(linux_event, 0, sizeof(wd_linux_event_t));
    pthread_cond_init(&linux_event->cond, NULL);
    pthread_mutex_init(&linux_event->mutex, NULL);
    *phOsEvent = linux_event;

    return WD_STATUS_SUCCESS;
#else
    return WD_NOT_IMPLEMENTED;
#endif
#endif
}

DWORD DLLCALLCONV OsEventReset(_In_ HANDLE hOsEvent)
{
    DWORD dwStatus = OsEventWait(hOsEvent, 0);

    return (dwStatus == WD_TIME_OUT_EXPIRED) ? WD_STATUS_SUCCESS : dwStatus;
}

void DLLCALLCONV OsEventClose(_In_ HANDLE hOsEvent)
{
#if defined(__KERNEL__)
#else
#if defined(WIN32)
    if (hOsEvent)
        CloseHandle(hOsEvent);
#elif defined(UNIX)
    wd_linux_event_t *linux_event = (wd_linux_event_t *)hOsEvent;

    pthread_cond_destroy(&linux_event->cond);
    pthread_mutex_destroy(&linux_event->mutex);
    free(linux_event);
#endif
#endif
}

DWORD DLLCALLCONV OsEventWait(_In_ HANDLE hOsEvent, _In_ DWORD dwSecTimeout)
{
#if defined(__KERNEL__)
    return WD_STATUS_SUCCESS;
#else
    DWORD rc = WD_STATUS_SUCCESS;
#if defined(WIN32)
    rc = WaitForSingleObject(hOsEvent,
        (dwSecTimeout == INFINITE) ? INFINITE : dwSecTimeout * 1000);
    switch (rc)
    {
        case WAIT_OBJECT_0:
            rc = WD_STATUS_SUCCESS;
            break;
        case WAIT_TIMEOUT:
            rc = WD_TIME_OUT_EXPIRED;
            break;
        default:
            rc = WD_SYSTEM_INTERNAL_ERROR;
            break;
    }
#elif defined(UNIX)
    struct timeval now;
    struct timespec timeout;
    wd_linux_event_t *linux_event = (wd_linux_event_t *)hOsEvent;

    pthread_mutex_lock(&linux_event->mutex);
    if (!linux_event->signaled)
    {
        if (dwSecTimeout == INFINITE)
        {
            rc = pthread_cond_wait(&linux_event->cond, &linux_event->mutex);
        }
        else
        {
            gettimeofday(&now, NULL);
            timeout.tv_sec = now.tv_sec + dwSecTimeout;
            timeout.tv_nsec = now.tv_usec * 1000;

            rc = pthread_cond_timedwait(&linux_event->cond, &linux_event->mutex,
                &timeout);
        }
    }
    linux_event->signaled = FALSE;
    pthread_mutex_unlock(&linux_event->mutex);
    rc = (rc == ETIMEDOUT ? WD_TIME_OUT_EXPIRED : WD_STATUS_SUCCESS);
#endif
    return rc;
#endif
}

DWORD DLLCALLCONV OsEventSignal(_In_ HANDLE hOsEvent)
{
#if defined(__KERNEL__)
    return WD_STATUS_SUCCESS;
#else
#if defined(WIN32)
    if (!SetEvent(hOsEvent))
        return WD_SYSTEM_INTERNAL_ERROR;
#elif defined(UNIX)
    wd_linux_event_t *linux_event = (wd_linux_event_t *)hOsEvent;

    pthread_mutex_lock(&linux_event->mutex);
    linux_event->signaled = TRUE;
    pthread_cond_signal(&linux_event->cond);
    pthread_mutex_unlock(&linux_event->mutex);
#endif
    return WD_STATUS_SUCCESS;
#endif
}

DWORD DLLCALLCONV OsMutexCreate(_Outptr_ HANDLE *phOsMutex)
{
#if defined(__KERNEL__)
    return WD_NOT_IMPLEMENTED;
#else
#if defined(WIN32)
    *phOsMutex = CreateMutex(NULL, FALSE, NULL);
    return *phOsMutex ? WD_STATUS_SUCCESS : WD_INSUFFICIENT_RESOURCES;
#elif defined(UNIX)
    pthread_mutex_t *linux_mutex =
        (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));

    if (!linux_mutex)
        return WD_INSUFFICIENT_RESOURCES;

    memset(linux_mutex, 0, sizeof(pthread_mutex_t));
    pthread_mutex_init(linux_mutex, NULL);
    *phOsMutex = linux_mutex;
    return WD_STATUS_SUCCESS;
#else
    return WD_NOT_IMPLEMENTED;
#endif
#endif
}

void DLLCALLCONV OsMutexClose(_In_ HANDLE hOsMutex)
{
#if defined(__KERNEL__)
#else
#if defined(WIN32)
    if (hOsMutex)
        CloseHandle(hOsMutex);
#elif defined(UNIX)
    pthread_mutex_t *linux_mutex = (pthread_mutex_t *)hOsMutex;

    pthread_mutex_destroy(linux_mutex);
    free(linux_mutex);
#endif
#endif
}

DWORD DLLCALLCONV OsMutexLock(_In_ HANDLE hOsMutex)
{
#if defined(__KERNEL__)
    return WD_NOT_IMPLEMENTED;
#else
#if defined(WIN32)
    WaitForSingleObject(hOsMutex, INFINITE);
#elif defined(UNIX)
    pthread_mutex_t *linux_mutex = (pthread_mutex_t *)hOsMutex;

    pthread_mutex_lock(linux_mutex);
#endif
    return WD_STATUS_SUCCESS;
#endif
}

DWORD DLLCALLCONV OsMutexUnlock(_In_ HANDLE hOsMutex)
{
#if defined(__KERNEL__)
    return WD_NOT_IMPLEMENTED;
#else
#if defined(WIN32)
    ReleaseMutex(hOsMutex);
#elif defined(UNIX)
    pthread_mutex_t *linux_mutex = (pthread_mutex_t *)hOsMutex;
    pthread_mutex_unlock(linux_mutex);
#endif
    return WD_STATUS_SUCCESS;
#endif
}

void DLLCALLCONV SleepWrapper(_In_ DWORD dwMicroSecs)
{
    HANDLE hWD;
    WD_SLEEP slp;

    hWD = WD_Open();
    if (hWD == INVALID_HANDLE_VALUE)
        return;

    BZERO(slp);
    slp.dwMicroSeconds = dwMicroSecs;
    slp.dwOptions = SLEEP_NON_BUSY;
    WD_Sleep(hWD, &slp);
    WD_Close(hWD);
}

#ifdef WIN32
/* For backward compatability, no longer used */
void DLLCALLCONV FreeDllPtr(void **ptr)
{
    free(*ptr);
}
#endif

#if !defined(__KERNEL__)
int print2wstr(wchar_t *buffer, size_t count, const wchar_t *format , ...)
{
    int rc;
    va_list ap;

    va_start(ap, format);
    rc =
    #if defined(WIN32) /* Windows */
        _vsnwprintf
    #elif defined(UNIX)
        vswprintf
    #else
        #error Not implemented
    #endif
        (buffer, count, format, ap);
    va_end(ap);

    return rc;
}

void DLLCALLCONV vPrintDbgMessage(_In_ DWORD dwLevel, _In_ DWORD dwSection,
    _In_ const char *format, _In_ va_list ap)
{
    WD_DEBUG_ADD add;
    HANDLE hWD;

    hWD = WD_Open();
    if (hWD == INVALID_HANDLE_VALUE)
        return;

    BZERO(add);
    add.dwLevel = dwLevel;
    add.dwSection = dwSection;
    vsnprintf(add.pcBuffer, 255, format, ap);
    WD_DebugAdd(hWD, &add);
    WD_Close(hWD);
}

void DLLCALLCONV PrintDbgMessage(DWORD dwLevel, DWORD dwSection,
    const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    vPrintDbgMessage(dwLevel, dwSection, format, ap);
    va_end(ap);
}

int DLLCALLCONV GetPageSize(void)
{
#if defined(UNIX)
    return sysconf(_SC_PAGESIZE);
#elif defined(WIN32)
    SYSTEM_INFO si;

    GetSystemInfo(&si);
    return si.dwPageSize;
#else
    #error Not implemented
#endif
}

int DLLCALLCONV GetNumberOfProcessors(void)
{
#if defined(UNIX)
    return sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(WIN32)
    SYSTEM_INFO si;

    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
#endif
}

BOOL DLLCALLCONV UtilGetFileSize(_In_ const PCHAR pcFileName,
    _Outptr_ DWORD *pdwFileSize, _In_ PCHAR pcErrString)
{
    char err_msg[1024];
    PCHAR pc_err = pcErrString ? pcErrString : &err_msg[0];

#if defined(WIN32) /* Windows */
    HANDLE fh;

    fh = CreateFile(pcFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh == INVALID_HANDLE_VALUE)
    {
        DWORD dwLastError;
        LPTSTR lpMsgBuf;

        dwLastError = GetLastError();
        sprintf(pc_err, "Failed opening %s for reading.\n"
            "Last error: 0x%lx", pcFileName, dwLastError);

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
            dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            /* Default language */ (LPTSTR)&lpMsgBuf, 0, NULL))
        {
            /* ('\n' is already included in lpMsgBuf) */
            sprintf(pc_err, "%s - %s", pc_err, lpMsgBuf);
            LocalFree(lpMsgBuf);
        }
        else
        {
            sprintf(pc_err, "%s\n", pc_err);
        }

        return FALSE;
    }

    *pdwFileSize = GetFileSize(fh, NULL);
    if (!CloseHandle(fh))
    {
        sprintf(pc_err, "Failed closing file handle to %s (0x%p)\n", pcFileName,
            fh);
        return FALSE;
    }

    return (*pdwFileSize != INVALID_FILE_SIZE);
#elif defined(UNIX)
    struct stat file_stat;

    if (stat(pcFileName, &file_stat))
    {
        sprintf(pc_err, "Failed retrieving %s file information", pcFileName);
        return FALSE;
    }

    *pdwFileSize = (DWORD)file_stat.st_size;
    return TRUE;
#else
    sprintf(pc_err, "UtilGetFileSize: Not implemented for this OS\n");
    return FALSE;
#endif
}

DWORD DLLCALLCONV UtilGetStringFromUser(_Out_ PCHAR pcString,
    _In_ DWORD dwSizeStr, _In_ const CHAR *pcInputText,
    _In_ const CHAR *pcDefaultString)
{
    PCHAR pcRes;
    DWORD dwStrLen;

    if (!pcString || dwSizeStr <= 1)
        return WD_INVALID_PARAMETER;

    pcString[0] = '\0';

    printf("%s", pcInputText);
    if (pcDefaultString && strcmp(pcDefaultString, ""))
        printf(" (Default: %s)", pcDefaultString);
    printf(":\n > ");

    pcRes = fgets(pcString, dwSizeStr, stdin);
    if (!pcRes)
    {
        /* Error when reading input from user */
        return WD_OPERATION_FAILED;
    }

    dwStrLen = (DWORD)strlen(pcString);
    if (dwStrLen > 1)
    {
        /* Remove '\n' (may be missing if dwSizeStr < size of input string) */
        if (pcString[dwStrLen - 1] == '\n')
            pcString[dwStrLen - 1] = '\0';
    }
    else if (pcDefaultString)
    {
        strcpy(pcString, pcDefaultString);
    }
    else /* User did not provide any input and no default string exists */
    {
        return WD_OPERATION_FAILED;
    }

    return WD_STATUS_SUCCESS;
}

DWORD DLLCALLCONV UtilGetFileName(_Out_ PCHAR pcFileName,
    _In_ DWORD dwFileNameSize, _In_ const CHAR *pcDefaultFileName)
{
    return UtilGetStringFromUser(pcFileName, dwFileNameSize,
        "Please enter the file name", pcDefaultFileName);
}

#endif /* !defined(__KERNEL__) */

