/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _WD_UTILS_H_
#define _WD_UTILS_H_

#if defined(__KERNEL__)
    #include "kpstdlib.h"
#else
    #include <stdio.h>
#endif

#include "windrvr.h"

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(MAX_PATH)
    #define MAX_PATH 4096
#endif

#if defined(WIN32)
    #define snprintf _snprintf
    #if !defined(vsnprintf)
        #define vsnprintf _vsnprintf
    #endif
#endif

typedef void (DLLCALLCONV *HANDLER_FUNC)(void *pData);

#if !defined(WIN32) || defined(_MT)

/**
*  Creates a thread.
*
*   @param [out] phThread: Returns the handle to the created thread

*   @param [in] pFunc: Starting address of the code that the new thread is to
*                      execute. (The handler's prototype HANDLER_FUNC is
*                      defined in utils.h.)
*   @param [in] pData: Pointer to the data to be passed to the new thread
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*/
DWORD DLLCALLCONV ThreadStart(_Outptr_ HANDLE *phThread,
    _In_ HANDLER_FUNC pFunc,
    _In_ void *pData);

/**
*  Waits for a thread to exit.
*
*   @param [in] hThread: The handle to the thread whose completion is awaited
*
* @return
*  None
*
*/
void DLLCALLCONV ThreadWait(_In_ HANDLE hThread);
#endif

/**
*  Creates an event object.
*
*   @param [out] phOsEvent: The pointer to a variable that receives a
*                          handle to the newly created event object
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*/
DWORD DLLCALLCONV OsEventCreate(_Outptr_ HANDLE *phOsEvent);

/**
*  Closes a handle to an event object.
*
*   @param [in] hOsEvent: The handle to the event object to be closed
*
* @return
*  None
*
*/
void DLLCALLCONV OsEventClose(_In_ HANDLE hOsEvent);

/**
*  Waits until a specified event object is in the signaled state
*  or the time-out interval elapses.
*
*    @param [in] hOsEvent: The handle to the event object
*    @param [in] dwSecTimeout: Time-out interval of the event, in seconds.
*                              For an infinite wait, set the timeout
*                              to INFINITE.
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*/
DWORD DLLCALLCONV OsEventWait(_In_ HANDLE hOsEvent, _In_ DWORD dwSecTimeout);

/**
* Sets the specified event object to the signaled state.
*
*    @param [in] hOsEvent: The handle to the event object
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*/
DWORD DLLCALLCONV OsEventSignal(_In_ HANDLE hOsEvent);

/**
* Resets the specified event object to the non-signaled state.
*
*    @param [in] hOsEvent: The handle to the event object
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*/
DWORD DLLCALLCONV OsEventReset(_In_ HANDLE hOsEvent);

/**
* Creates a mutex object.
*
*    @param [out] phOsMutex: The pointer to a variable that
*                           receives a handle to the newly created mutex object
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*/
DWORD DLLCALLCONV OsMutexCreate(_Outptr_ HANDLE *phOsMutex);

/**
* Closes a handle to a mutex object.
*
*    @param [in] hOsMutex: The handle to the mutex object to be closed
*
* @return
*  None
*
*/
void DLLCALLCONV OsMutexClose(_In_ HANDLE hOsMutex);

/**
* Locks the specified mutex object.
*
*    @param [in] hOsMutex: The handle to the mutex object to be locked
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*/
DWORD DLLCALLCONV OsMutexLock(_In_ HANDLE hOsMutex);

/**
* Releases (unlocks) a locked mutex object.
*
*    @param [in] hOsMutex: The handle to the mutex object to be unlocked
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*/
DWORD DLLCALLCONV OsMutexUnlock(_In_ HANDLE hOsMutex);

/**
* Wrapper to WD_Sleep, Sleeps dwMicroSecs microseconds.
*
*    @param [in] dwMicroSecs: Time in microseconds to sleep
*
* @return
*  None
*
*/
void DLLCALLCONV SleepWrapper(_In_ DWORD dwMicroSecs);

#if defined(UNIX)
    #define OsMemoryBarrier() __sync_synchronize()
#elif defined(WIN32)
    #define OsMemoryBarrier() MemoryBarrier()
#endif

#if !defined(__KERNEL__)


int print2wstr(wchar_t *buffer, size_t count, const wchar_t *format , ...);

/**
* Sends debug messages to the Debug Monitor.
*
*    @param [in] dwLevel:   Assigns the level in the Debug Monitor,
*                           in which the data will be declared. If zero,
*                           D_ERROR will be declared.
*                           For more details please refer to
*                           DEBUG_LEVEL in windrvr.h.
*    @param [in] dwSection: Assigns the section in the Debug Monitor,
*                           in which the data will be declared. If zero,
*                           S_MISC will be declared. For more details please
*                           refer to DEBUG_SECTION in windrvr.h.
*    @param [in] format:    Format-control string
*    @param [in] ap:        Optional Arguments
*
* @return
*  None
*
*/
void DLLCALLCONV vPrintDbgMessage(_In_ DWORD dwLevel, _In_ DWORD dwSection,
    _In_ const char *format, _In_ va_list ap);

/**
* Sends debug messages to the Debug Monitor.
*
*    @param [in] dwLevel:   Assigns the level in the Debug Monitor,
*                           in which the data will be declared. If zero,
*                           D_ERROR will be declared.
*                           For more details please refer to
*                           DEBUG_LEVEL in windrvr.h.
*    @param [in] dwSection: Assigns the section in the Debug Monitor,
*                           in which the data will be declared. If zero,
*                           S_MISC will be declared. For more details please
*                           refer to DEBUG_SECTION in windrvr.h.
*    @param [in] format:    Format-control string
*    @param [in] ...:  Optional arguments, limited to 256 bytes
*
* @return
*  None
*
*/
void DLLCALLCONV PrintDbgMessage(DWORD dwLevel, DWORD dwSection,
    const char *format, ...);

/**
* Returns the page size in the OS.
*
* @return
*  Page size in OS
*
*/
int DLLCALLCONV GetPageSize(void);

/**
* Returns the number of processors currently online (available)
*
* @return
*  Number of processors currently online (available)
*
*/
int DLLCALLCONV GetNumberOfProcessors(void);

/**
* Writes the file size with name sFileName in dwFileSize
*
*    @param [in] sFileName:   Name of the file
*    @param [out] pdwFileSize: Pointer to DWORD that will be written with
*                             the file's size.
*    @param [in] sErrString:  Optional error message
*
* @return
*  TURE if the function succeeded in getting the file size, else FALSE
*
*/
BOOL DLLCALLCONV UtilGetFileSize(_In_ const PCHAR sFileName,
    _Outptr_ DWORD *pdwFileSize, _In_ PCHAR sErrString);

/**
* Gets a string from user input and out it in pcString
*
*    @param [out] pcString:       Pointer to buffer that will be filled with
*                                 user input
*    @param [in] dwSizeStr:       Number of bytes to read from user
*    @param [in] pcInputText:     Input text that will be written to stdout
*                                 on prompt
*    @param [in] pcDefaultString: Default String to write to pcString in case
*                                 of no input
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*/
DWORD DLLCALLCONV UtilGetStringFromUser(_Out_ PCHAR pcString,
    _In_ DWORD dwSizeStr, _In_ const CHAR *pcInputText,
    _In_ const CHAR *pcDefaultString);


/**
* Gets a file name from user input and out it in pcFileName
*
*    @param [out] pcFileName:       Pointer to buffer that will be filled with
*                                   user input
*    @param [in] dwFileNameSize:    Number of bytes in file name
*    @param [in] pcDefaultFileName: Default file name to write to pcFileName
*                                   in case of no input
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*/
DWORD DLLCALLCONV UtilGetFileName(_Out_ PCHAR pcFileName,
    _In_ DWORD dwFileNameSize, _In_ const CHAR *pcDefaultFileName);
#endif

#if defined(UNIX)
    #if !defined(stricmp)
        #define stricmp strcasecmp
    #endif
    #if !defined(strnicmp)
        #define strnicmp strncasecmp
    #endif
#endif

#if !defined(INFINITE)
    #define INFINITE 0xffffffff
#endif

#ifdef __cplusplus
}
#endif

#endif /* _WD_UTILS_H_ */

