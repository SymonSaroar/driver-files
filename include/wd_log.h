/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _WD_LOG_H_
#define _WD_LOG_H_

#ifdef __cplusplus
    extern "C" {
#endif

ULONG DLLCALLCONV WdFunctionLog(DWORD wFuncNum, HANDLE h, PVOID pParam,
    DWORD dwSize, BOOL fWait);
HANDLE DLLCALLCONV WD_OpenLog(void);
void DLLCALLCONV WD_CloseLog(HANDLE hWD);

/**
*  Opens a log file.
*
*   @param [in] sFileName: Name of log file to be opened
*   @param [in] sMode:     Type of access permitted.
*                          For example, NULL or w opens an empty file for
*                          writing, and if the given file exists, its contents
*                          are destroyed; a opens a file for writing at the end
*                          of the file (i.e., append).
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @remarks
*  Once a log file is opened, all API calls are logged in this file.
*  You may add your own printouts to the log file by calling WD_LogAdd() 
*/
DWORD DLLCALLCONV WD_LogStart(const char *sFileName, const char *sMode);

/**
*  Closes a log file.
*
* @return
*  None
*
*/
VOID DLLCALLCONV WD_LogStop(void);

/**
*  Opens a log file.
*
*   @param [in] sFormat: Adds user printouts into log file.
*   @param [in] ...:   Optional format arguments
* 
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @remarks
*  Once a log file is opened, all API calls are logged in this file.
*  You may add your own printouts to the log file by calling WD_LogAdd()
*/
VOID DLLCALLCONV WD_LogAdd(const char *sFormat, ...);

#undef WD_FUNCTION
#undef WD_Close
#undef WD_Open

#define WD_FUNCTION WdFunctionLog
#define WD_Close WD_CloseLog
#define WD_Open WD_OpenLog

#ifdef __cplusplus
}
#endif

#endif /* _WD_LOG_H_ */

