/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/**********************************************************************
 * File - USB_DIAG_LIB.H
 *
 * Library for USB diagnostics and samples, using WinDriver functions.
 **********************************************************************/

#ifndef _USB_DIAG_LIB_H_
#define _USB_DIAG_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "windrvr.h"
#include "diag_lib.h"

enum {MAX_BUFFER_SIZE = 4096};

/**
 *  Sets the interface number and the alternate setting of the given device
 *   @param [in] hDevice: A unique identifier for the device.
 *   @param [in] dwInterfaceNum: Interface number.
 *   @param [in] dwAlternateSetting: Alterante setting.
 *
 *   @return  Returns WD_STATUS_SUCCESS (0) on success,
 *     or an appropriate error code otherwise
 */
DWORD USB_SetInterface(WDU_DEVICE_HANDLE hDevice, DWORD dwInterfaceNum,
    DWORD dwAlternateSetting);

/**
 *  Resets pipe number `dwPipeNum` of the given device.
 *   @param [in] hDevice: A unique identifier for the device.
 *   @param [in] dwPipeNum: Pipe number to reset.
 *
 *   @return  Returns WD_STATUS_SUCCESS (0) on success,
 *     or an appropriate error code otherwise
 */
DWORD USB_ResetPipe(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum);

/**  Gets configuration information from the device including all the
 *   descriptors in a WDU_DEVICE struct. The caller should free *ppDeviceInfo.
 *   after using it by calling WDU_PutDeviceInfo().
 *   @param [in]  hDevice: handle to the USB device
 *   @param [out] ppDeviceInfo: Pointer to a pointer to a buffer containing the
 *                       device information.
 * 
 *   @return  Returns WD_STATUS_SUCCESS (0) on success,
 *     or an appropriate error code otherwise
 */
DWORD USB_GetDeviceInfo(WDU_DEVICE_HANDLE hDevice,
    WDU_DEVICE **ppDeviceInfo);

/**  Submits a request to suspend a given device (selective suspend), or
 *   cancels a previous suspend request.
 *   @param [in] hDevice: Handle to the USB device
 *   @param [in] dwOptions: Can be set to either of the following
 *                   WDU_SELECTIVE_SUSPEND_OPTIONS enumeration values:
 *                   WDU_SELECTIVE_SUSPEND_SUBMIT - submit a request to
 *                       suspend the device.
 *                   WDU_SELECTIVE_SUSPEND_CANCEL - cancel a previous
 *                       suspend request for the device.
 * 
 *   @return  Returns WD_STATUS_SUCCESS (0) on success,
 *     or an appropriate error code otherwise 
 */
DWORD USB_SelectiveSuspend(WDU_DEVICE_HANDLE hDevice, DWORD dwOptions);

/**  Opens a new data stream for the specified pipe and reads/writes from/to it 
 *   @param [in] hDevice: handle to the USB device
 *   @param [in] dwPipeNum: Pipe number
 *   @param [in,out] pBuffer: Pointer to a data buffer
 *   @param [in] dwTransferSize: Number of bytes to read/write
 *   @param [out] pdwBytesTransferred: Pointer to a value indicating the number of
 *                              bytes actually read from the stream.
 *   @param [in] fRead: TRUE if read operation, FALSE if write operation.
 *
 *   @return  Returns WD_STATUS_SUCCESS (0) on success,
 *     or an appropriate error code otherwise
 */
DWORD USB_ReadWriteStream(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum,
    PVOID pBuffer, DWORD dwBufferSize, DWORD dwTransferSize,
    DWORD *pdwBytesTransferred, BOOL fRead);

/**  Transfers data to/from a device.
 *   @param [in] hDevice: Handle to the USB device.
 *   @param [in] dwPipeNum: Pipe number.
 *   @param [in,out] pBuffer: Pointer to a data buffer.
 *   @param [in] dwBufferSize: Size of the given buffer.
 *   @param [in] dwTransferSize: Number of bytes to read/write from/to the stream
 *   @param [out] pdwBytesTransferred: Pointer to a value indicating the number of
 *                              bytes actually read from the stream.
 *   @param [in] fRead: TRUE if read operation, FALSE if write operation.
 * 
 *   @return  Returns WD_STATUS_SUCCESS (0) on success,
 *     or an appropriate error code otherwise
 */
DWORD USB_ReadWriteTransfer(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum,
    PVOID pBuffer, DWORD dwTransferSize, PBYTE pSetupPacket,
    DWORD *pdwBytesTransferred, BOOL fRead);

/**  Opens a new data stream for the specified pipe and reads/writes from/to it.
 *   The function will perform a performance check on the stream and will print
 *   the results to stdout
 *   @param [in] hDevice: Handle to the USB device.
 *   @param [in] dwPipeNum: Pipe number.
 *   @param [in,out] pBuffer: Pointer to a data buffer.
 *   @param [in] dwBufferSize: Size of the buffer.
 *   @param [in] dwTransferSize: Number of bytes to read/write from/to the stream.
 *   @param [in] dwIterations: Number of iterations in the performance check.
 *   @param [in] fRead: TRUE if read operation, FALSE if write operation.
 *
 * @return  Returns WD_STATUS_SUCCESS (0) on success,
 *   or an appropriate error code otherwise
 */
DWORD USB_ReadWriteStreamCheck(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum,
    PVOID pBuffer, DWORD dwBufferSize, DWORD dwTransferSize,
    DWORD dwIterations, BOOL fRead);

/** Returns a string identifying the pipe type.
 *   @param [in] pipeType: USB_PIPE_TYPE enum value.
 *
 *   @return A string containing the description of the pipe type.
 */
char *USB_PipeType2Str(ULONG pipeType);

/** Prints the pipes information for the specified alternate setting.
 *   @param [in] pAltSet: Pointer to the alternate setting information.
 *   @param [in] fp: File pointer to print into(usually stdout).
 * 
 *   @return None.
 */
void USB_PrintPipesInfo(WDU_ALTERNATE_SETTING *pAltSet, FILE *fp);

/** Prints the device pipes information.
 *   @param [in] pDevice: Pointer to device configuration details.
 *   @param [in] fp: File pointer to print into(usually stdout).
 * 
 *   @return None.
 */
void USB_PrintDevicePipesInfo(WDU_DEVICE *pDevice, FILE *fp);

/** Prints the device control pipe(0) information.
 *   @param [in] pDevice: Pointer to device configuration details.
 *   @param [in] fp: File pointer to print into(usually stdout).
 * 
 *   @return None.
 */
void USB_PrintPipe0Info(WDU_DEVICE *pDevice, FILE *fp);

/** Prints the pipes information for all the active device pipes.
 *   @param [in] hDevice: Handle to the USB device.
 *   @param [in] fp: File pointer to print into(usually stdout).
 * 
 *   @return  Returns WD_STATUS_SUCCESS (0) on success,
 *   or an appropriate error code otherwise
 */
DWORD USB_PrintDevicePipesInfoByHandle(HANDLE hDevice, FILE *fp);

/** Prints the device's configurations information,
 *   @param [in] hDevice: Handle to the USB device.
 *   @param [in] fpIn: File pointer to scan for user input(usually stdin)
 *   @param [in] fpOut: File pointer to print into(usually stdout)
 *
 *   @return  Returns WD_STATUS_SUCCESS (0) on success,
 *   or an appropriate error code otherwise
 */
DWORD USB_PrintDeviceConfigurations(HANDLE hDevice, FILE *fpIn,
    FILE *fpOut);

/** Listening to a USB device pipe
 *   @param [in] hDevice: Handle to the USB device.
 *   @param [in] pPipe: Pointer to the pipe for listening.
 *   @param [in] fStreamMode: TRUE - Streaming mode, FALSE - Transfer mode.
 *   @param [in] dwBufferSize: Buffer size on streaming mode.
 *   @param [in] fUserKeyWait: TRUE - Wait for user key before starting.
 *   @param [in] fpIn: File pointer to scan for user input(usually stdin).
 *   @param [in] fpOut: File pointer to print into(usually stdout).
 *   @param [in] fPrint: TRUE - print info to fpOut, FALSE - Don't print.
 *
 *   @return  Returns WD_STATUS_SUCCESS (0) on success,
 *   or an appropriate error code otherwise
 */
DWORD USB_ListenToPipe(HANDLE hDevice, WDU_PIPE_INFO *pPipe,
    BOOL fStreamMode, DWORD dwBufferSize, BOOL fUserKeyWait, FILE *fpIn,
    FILE *fpOut, BOOL fPrint);

/** Finds a pipe in any of the device's active interfaces
 *   @param [in] pDevice: Pointer to the USB device.
 *   @param [in] dwPipeNumber: The pipe number to look for.
 *
 *   @return A pointer to the requested pipe, or NULL if no matching pipe
 *     was found
 */
WDU_PIPE_INFO *USB_FindPipeInDevice(WDU_DEVICE *pDevice,
    DWORD dwPipeNumber);

/** Prints the device's serial number if available
 *   @param [in] hDevice: Handle to the USB device.
 *   @param [in] fp: File pointer to print into(usually stdout).
 *
 *   @return  Returns WD_STATUS_SUCCESS (0) on success,
 *     or an appropriate error code otherwise
 */
DWORD USB_PrintDeviceSerialNumberByHandle(HANDLE hDevice, FILE *fp);

/** Prints the device properties if available
 *   @param [in] hDevice: Handle to the USB device.
 *   @param [in] fp: File pointer to print into(usually stdout).
 *
 *   @return  Returns WD_STATUS_SUCCESS (0) on success,
 *     or an appropriate error code otherwise
 */
void USB_PrintDeviceProperties(HANDLE hDevice, FILE *fp);

/** Returns the last error written to the log buffer
 *
 *   @return The last error written to the log buffer
 */
const char *USB_GetLastErr(void);

#ifdef __cplusplus
}
#endif

#endif /* _USB_DIAG_LIB_H_ */

