/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _WD_USB_H_
#define _WD_USB_H_

#include "windrvr.h"

#if defined(__cplusplus)
    extern "C" {
#endif

typedef PVOID WDU_DRIVER_HANDLE;
typedef PVOID WDU_DEVICE_HANDLE;
typedef PVOID WDU_STREAM_HANDLE;

typedef WORD WDU_LANGID;

/*
 * User Callbacks
 */

/**
 *   WinDriver calls this function with any new device that is attached,
 *   matches the given criteria, and if WD_ACKNOWLEDGE was passed to WDU_Init()
 *   in dwOptions - not controlled yet by another driver.
 *   This callback is called once for each matching interface.
 *   @param [in] hDevice:     A unique identifier for the device/interface
 *   @param [in] pDeviceInfo: Pointer to device configuration details.
 *                     This pointer is valid until the end of the function.
 *   @param [in] pUserData:   Pointer to user data that was passed to WDU_Init
 *                     (in the event table).
 *   @return   If WD_ACKNOWLEDGE was passed to WDU_Init(), the implementor
 *             should check & return if he wants to control the device.
 */
typedef BOOL (DLLCALLCONV *WDU_ATTACH_CALLBACK)(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ WDU_DEVICE *pDeviceInfo, _In_ PVOID pUserData);

/**
 *   WinDriver calls this function when a controlled device has been detached
 *   from the system.
 *   @param [in] hDevice:      A unique identifier for the device/interface.
 *   @param [in] pUserData:    Pointer to user data that was passed to WDU_Init
 *   @return   None.
 */
typedef void (DLLCALLCONV *WDU_DETACH_CALLBACK)(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ PVOID pUserData);

/**
 *   @param [in] hDevice:      A unique identifier for the device/interface.
 *   @param [in] dwPowerState: Number of the power state selected.
 *   @param [in] pUserData:    Pointer to user data that was passed to WDU_Init
 *                             (in the event table).
 *   @return   TRUE/FALSE; Currently there is no significance to the return
 *                         value.
 */
typedef BOOL (DLLCALLCONV *WDU_POWER_CHANGE_CALLBACK)(
    _In_ WDU_DEVICE_HANDLE hDevice, _In_ DWORD dwPowerState,
    _In_ PVOID pUserData);

/*
 * struct definitions
 */
typedef struct
{
    WDU_ATTACH_CALLBACK pfDeviceAttach;
    WDU_DETACH_CALLBACK pfDeviceDetach;
    WDU_POWER_CHANGE_CALLBACK pfPowerChange;
    PVOID pUserData;  /* pointer to pass in each callback */
} WDU_EVENT_TABLE;

/*
 * API Functions
 */

/**  Starts listening to devices matching a criteria, and registers
 *   notification callbacks for those devices.
 *   @param [out] phDriver:        Handle to this registration of events &
 *       criteria.
 *   @param [in] pMatchTables:     Array of match tables defining the
 *       devices-criteria.
 *   @param [in] dwNumMatchTables: Number of elements in pMatchTables.
 *   @param [in] pEventTable:      Notification callbacks when the device's
 *       status changes.
 *   @param [in] pcLicense:         WinDriver's license string.
 *   @param [in] dwOptions:        Can be 0 or:
 *                          WD_ACKNOWLEDGE - The user can seize control over
 *                              the device in WDU_ATTACH_CALLBACK return value.
 *   @return   WinDriver Error Code
 */
DWORD DLLCALLCONV WDU_Init(_Outptr_ WDU_DRIVER_HANDLE *phDriver,
    _In_ WDU_MATCH_TABLE *pMatchTables, _In_ DWORD dwNumMatchTables,
    _In_ WDU_EVENT_TABLE *pEventTable, _In_ const char *pcLicense,
    _In_ DWORD dwOptions);

/**  Stops listening to devices matching the criteria, and unregisters the
 *   notification callbacks for those devices.
 *   @param [in] hDriver: Handle to the registration, received from WDU_Init.
 *   @return   None
 */
void DLLCALLCONV WDU_Uninit(_In_ WDU_DRIVER_HANDLE hDriver);

/**  Gets USB address that the device uses. The address number is written to
 *   the caller supplied pAddress.
 * @param [in] hDevice:   A unique identifier for the device/interface.
 * @param [out] pAddress: Pointer to DWORD, in which the result will be
 *                        returned.
 * @return   WinDriver Error Code
 * @note: This function is supported only on Windows.
 *
 * @snippet highlevel_examples.c WDU_GetDeviceAddr
 */
DWORD DLLCALLCONV WDU_GetDeviceAddr(_In_ WDU_DEVICE_HANDLE hDevice,
    _Out_ DWORD *pAddress);

/**  Gets the specified registry property of a given device.
 * @param [in] hDevice:     A unique identifier of the device/interface.
 * @param [out] pBuffer:    Pointer to a user allocated buffer to be filled
 *                    with the requested registry property. The function will
 *                    fill  the buffer only if the buffer size, as indicated in
 *                    the input value of the pdwSize parameter, is sufficient -
 *                    i.e >= the property's size, as returned via pdwSize.
 *                    pBuffer can be set to NULL when using the function
 *                    only to retrieve the size of the registry property (see
 *                    pdwSize).
 *  @param [in,out] pdwSize: As input, points to a value indicating the size of
 *                    the user-supplied buffer (pBuffer); if pBuffer is set to
 *                    NULL, the input value of this parameter is ignored.
 *                    As output, points to a value indicating the required
 *                    buffer size for storing the registry property.
 *  @param [in] property:    The ID of the registry property to be retrieved -
 *                    see the WD_DEVICE_REGISTRY_PROPERTY enumeration in
 *                    windrvr.h.
 *                    Note: String registry properties are in WCHAR format.
 *   @return   WinDriver Error Code.
 * @note When the size of the provided user buffer (pBuffer) - *pdwSize (input)
 *     is not sufficient to hold the requested registry property, the function
 *     returns WD_INVALID_PARAMETER.
 *
 * @snippet highlevel_examples.c WDU_GetDeviceRegistryProperty
 */
DWORD DLLCALLCONV WDU_GetDeviceRegistryProperty(_In_ WDU_DEVICE_HANDLE hDevice,
    _Outptr_ PVOID pBuffer, _Inout_ PDWORD pdwSize,
    _In_ WD_DEVICE_REGISTRY_PROPERTY property);

/**  Gets configuration information from the device including all the
 *   descriptors in a WDU_DEVICE struct. The caller should free *ppDeviceInfo
 *   after using it by calling WDU_PutDeviceInfo().
 *   @param [in]  hDevice:      A unique identifier for the device/interface.
 *   @param [out] ppDeviceInfo: Pointer to a pointer to a buffer containing the
 *                       device information.
 *   @return   WinDriver Error Code
 *
 * @snippet highlevel_examples.c WDU_GetDeviceInfo
 */
DWORD DLLCALLCONV WDU_GetDeviceInfo(_In_ WDU_DEVICE_HANDLE hDevice,
    _Outptr_ WDU_DEVICE **ppDeviceInfo);

/**  Receives a device information pointer, allocated with a previous
 *   WDU_GetDeviceInfo() call, in order to perform the necessary cleanup.
 *   @param [in] pDeviceInfo: Pointer to a buffer containing the device
 *                     information, as returned by a previous call to
 *                     WDU_GetDeviceInfo().
 *   @return   None
 */
void DLLCALLCONV WDU_PutDeviceInfo(_In_ WDU_DEVICE *pDeviceInfo);

/**  Sets the alternate setting for the specified interface.
 * @param [in] hDevice:           A unique identifier for the device/interface.
 * @param [in] dwInterfaceNum:    The interface's number.
 * @param [in] dwAlternateSetting:   The desired alternate setting value.
 * @return   WinDriver Error Code
 *
 * @snippet highlevel_examples.c WDU_SetInterface
 */
DWORD DLLCALLCONV WDU_SetInterface(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwInterfaceNum, _In_ DWORD dwAlternateSetting);

/* NOT IMPLEMENTED YET */
DWORD DLLCALLCONV WDU_SetConfig(WDU_DEVICE_HANDLE hDevice, DWORD dwConfigNum);
/* NOT IMPLEMENTED YET */

/**  Resets a pipe.
 *   @param [in] hDevice:      A unique identifier for the device/interface.
 *   @param [in] dwPipeNum:    Pipe number.
 *   @return   WinDriver Error Code
 *
 * @snippet highlevel_examples.c WDU_ResetPipe
 */
DWORD DLLCALLCONV WDU_ResetPipe(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwPipeNum);

/**  Resets a device (supported only on Windows).
 *   @param [in] hDevice:   A unique identifier for the device/interface
 *   @param [in] dwOptions: Can be 0 or:
 *                   WD_USB_HARD_RESET - will reset the device even if it is
 *                       not disabled.
 *                       After using this option it is advised to set the
 *                       interface of the device (WDU_SetInterface()).
 *                   WD_USB_CYCLE_PORT - will simulate unplugging and
 *                       replugging the device, prompting the operating
 *                       system to enumerate the device without resetting it.
 *                       This option is available only on Windows.
 *   @return   WinDriver Error Code
 *       The CYCLE_PORT option is supported only on Windows
 */
DWORD DLLCALLCONV WDU_ResetDevice(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwOptions);

/**  Enables/Disables wakeup feature.
 *   @param [in] hDevice:      A unique identifier for the device/interface
 *   @param [in] dwOptions:    Can be set to either of the following options:
 *                      WDU_WAKEUP_ENABLE - will enable wakeup.
 *                      or:
 *                      WDU_WAKEUP_DISABLE - will disable wakeup.
 *   @return   WinDriver Error Code
 * @note This function is supported only on Windows.
 */
DWORD DLLCALLCONV WDU_Wakeup(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwOptions);

/**  Submits a request to suspend a given device (selective suspend), or
 *   cancels a previous suspend request.
 *   @param [in] hDevice:   A unique identifier for the device/interface.
 *   @param [in] dwOptions: Can be set to either of the following
 *                   WDU_SELECTIVE_SUSPEND_OPTIONS enumeration values:
 *                   WDU_SELECTIVE_SUSPEND_SUBMIT - submit a request to
 *                       suspend the device.
 *                   WDU_SELECTIVE_SUSPEND_CANCEL - cancel a previous
 *                       suspend request for the device.
 *   @return   WinDriver Error Code.
 *   If a suspend request is received while the device is busy, the function
 *   returns WD_OPERATION_FAILED.
 * @note This function is supported only on Windows.
 *
 * @snippet highlevel_examples.c WDU_SelectiveSuspend
 */
DWORD DLLCALLCONV WDU_SelectiveSuspend(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwOptions);

/**  Transfers data to/from a device.
 *   @param [in] hDevice:   A unique identifier for the device/interface.
 *   @param [in] dwPipeNum: The number of the pipe through which the data is
 *                   transferred.
 *   @param [in] fRead:     TRUE for read, FALSE for write.
 *   @param [in] dwOptions: Can be a bit-mask of any of the following options:
 *                   USB_ISOCH_NOASAP - For isochronous data transfers.
 *                       Setting this option instructs the lower driver
 *                       (usbd.sys) to use a preset frame number (instead of
 *                       the next available frame) while performing the data
 *                       transfer. Use this flag if you notice unused frames
 *                       during the transfer, on low-speed or full-speed
 *                       devices (USB 1.1 only) and on Windows only.
 *                   USB_ISOCH_RESET - Resets the isochronous pipe before the
 *                       data transfer. It also resets the pipe after minor
 *                       errors, consequently allowing the transfer to
 *                       continue.
 *                   USB_ISOCH_FULL_PACKETS_ONLY - When set, do not transfer
 *                       less than packet size on isochronous pipes.
 *                   USB_BULK_INT_URB_SIZE_OVERRIDE_128K - Limits the size of
 *                       the USB Request Block (URB) to 128KB.
 * @param [in] pBuffer: location of the data buffer
 * @param [in] dwBufferSize:         Number of the bytes to transfer.
 * @param [out] pdwBytesTransferred: Number of bytes actually transferred.
 * @param [in] pSetupPacket:         8-bytes packet to transfer to control
 *                                   pipes.
 * @param [in] dwTimeout:            Maximum time, in milliseconds, to complete
 *                           a transfer. Zero = infinite wait.
 *   @return   WinDriver Error Code
 *
 * @snippet highlevel_examples.c WDU_Transfer
 */
DWORD DLLCALLCONV WDU_Transfer(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwPipeNum, _In_ DWORD fRead, _In_ DWORD dwOptions,
    _In_ PVOID pBuffer, _In_ DWORD dwBufferSize,
    _Outptr_ PDWORD pdwBytesTransferred, _In_ PBYTE pSetupPacket,
    _In_ DWORD dwTimeout);

/**  Halts the transfer on the specified pipe (only one simultaneous transfer
 *   per-pipe is allowed by WinDriver).
 *   @param [in] hDevice:      A unique identifier for the device/interface.
 *   @param [in] dwPipeNum:    Pipe number.
 *   @return   WinDriver Error Code
 *
 * @snippet highlevel_examples.c WDU_HaltTransfer
 */
DWORD DLLCALLCONV WDU_HaltTransfer(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwPipeNum);

/*
 * Simplified transfers - for a specific pipe
 */

DWORD DLLCALLCONV WDU_TransferDefaultPipe(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD fRead, _In_ DWORD dwOptions,
    _In_ PVOID pBuffer, _In_ DWORD dwBufferSize,
    _Outptr_ PDWORD pdwBytesTransferred,
    _In_ PBYTE pSetupPacket, _In_ DWORD dwTimeout);

DWORD DLLCALLCONV WDU_TransferBulk(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwPipeNum, _In_ DWORD fRead, _In_ DWORD dwOptions,
    _In_ PVOID pBuffer, _In_ DWORD dwBufferSize,
    _Outptr_ PDWORD pdwBytesTransferred, _In_ DWORD dwTimeout);

DWORD DLLCALLCONV WDU_TransferIsoch(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwPipeNum, _In_ DWORD fRead, _In_ DWORD dwOptions,
    _In_ PVOID pBuffer, _In_ DWORD dwBufferSize,
    _Outptr_ PDWORD pdwBytesTransferred, _In_ DWORD dwTimeout);

DWORD DLLCALLCONV WDU_TransferInterrupt(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwPipeNum, _In_ DWORD fRead, _In_ DWORD dwOptions,
    _In_ PVOID pBuffer, _In_ DWORD dwBufferSize,
    _Outptr_ PDWORD pdwBytesTransferred, _In_ DWORD dwTimeout);

/**  Reads a list of supported language IDs and/or the number of supported
 *   language IDs from a device.
 *   @param [in] hDevice:                A unique identifier for the
 *                                       device/interface.
 *   @param [out] pbNumSupportedLangIDs: Pointer to the number of supported
 *                                language IDs, to be filled by the function.
 *                                Can be NULL if bNumLangIDs is not 0 and
 *                                pLangIDs is not NULL. If NULL, the function
 *                                will not return the number of supported
 *                                language IDs for the device.
 *   @param [out] pLangIDs:              Array of language IDs. If bNumLangIDs
 *                                is not is not 0 the function will fill this
 *                                array with the supported language IDs for the
 *                                device. If bNumLangIDs < the number of
 *                                supported language IDs for the device, only
 *                                the first bNumLangIDs supported language IDs
 *                                will be read from the device and returned in
 *                                the pLangIDs array.
 *   @param [in] bNumLangIDs:            Number of IDs in pLangIDs array. If 0,
 *                                the function will only return the number of
 *                                supported language IDs.
 *   @return   WinDriver Error Code
 * @note If no language IDs are supported for the device
 *       (*pbNumSupportedLangIDs == 0) the function returns
 *       WD_STATUS_SUCCESS.
 */
DWORD DLLCALLCONV WDU_GetLangIDs(_In_ WDU_DEVICE_HANDLE hDevice,
    _Outptr_ PBYTE pbNumSupportedLangIDs, _Outptr_ WDU_LANGID *pLangIDs,
    _In_ BYTE bNumLangIDs);

/**  Reads a string descriptor from a device by string index.
 *   @param [in]  hDevice:     A unique identifier for the device/interface.
 *   @param [in]  bStrIndex:   Index of the string descriptor to read.
 *   @param [out] pbBuf:       Pointer to a buffer to be filled with the string
 *                      descriptor that is read from the device.
 *                      If the buffer is smaller than the string descriptor
 *                      (dwBufSize < *pdwDescSize), the returned descriptor
 *                      will be truncated to dwBufSize bytes.
 * @param [in]  dwBufSize: The size of the pbBuf buffer, in bytes.
 * @param [in]  langID:    The language ID to be used in the get string
 *    descriptor request that is sent to the device. If langID is 0, the
 *    request will use the first supported language ID returned by the device.
 * @param [out] pdwDescSize: An optional DWORD pointer to be filled with the
 *                      size of the string descriptor read from the device.
 *                      If this parameter is NULL, the funWDU_StreamReadction
 *                      will not return the size of the string descriptor.
 *   @return   WinDriver Error Code
 *
 * @snippet highlevel_examples.c WDU_GetStringDesc
 */
DWORD DLLCALLCONV WDU_GetStringDesc(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ BYTE bStrIndex, _Outptr_ PBYTE pbBuf, _In_ DWORD dwBufSize,
    _In_ WDU_LANGID langID, _Outptr_ PDWORD pdwDescSize);

/*
 * Streaming Functions
 */

/**
 * @note The streaming functions are currently supported only on
 *       Windows.
 */

/**  Opens a new data stream for the specified pipe.
 *   A stream can be associated with any pipe except the control pipe (Pipe 0).
 *   The stream's data transfer direction -- read/write -- is derived from the
 *   direction of its pipe.
 *   @param [in] hDevice:   A unique identifier for the device/interface.
 *   @param [in] dwPipeNum: The number of the pipe for which to open the stream
 *   @param [in] dwBufferSize: The size, in bytes, of the stream's data buffer.
 *   @param [in] dwRxSize:     The size, in bytes, of the data blocks that the
 *                          stream reads from the device.
 *                          This parameter is relevant only for read streams
 *                          and must be <= dwBufferSize.
 *   @param [in] fBlocking: TRUE for a blocking stream (performs blocking I/O);
 *                          FALSE for a non-blocking stream (non-blocking I/O).
 *   @param [in] dwOptions: Can be a bit-mask of any of the following options:
 *                      USB_ISOCH_NOASAP - For isochronous data transfers.
 *                          Setting this option instructs the lower driver
 *                          (usbd.sys) to use a preset frame number (instead of
 *                          the next available frame) while performing the data
 *                          transfer. Use this flag if you notice unused frames
 *                          during the transfer, on low-speed or full-speed
 *                          devices (USB 1.1 only) and only on Windows.
 *                      USB_ISOCH_RESET - Resets the isochronous pipe before
 *                          the data transfer. It also resets the pipe after
 *                          minor errors, consequently allowing the transfer to
 *                          continue.
 *                      USB_ISOCH_FULL_PACKETS_ONLY - When set, do not transfer
 *                          less than packet size on isochronous pipes.
 *                      USB_BULK_INT_URB_SIZE_OVERRIDE_128K - Limits the size
 *                          of the USB Request Block (URB) to 128KB.
 *                      USB_STREAM_OVERWRITE_BUFFER_WHEN_FULL - When there is
 *                          not enough free space in a read stream's data
 *                          buffer to complete the transfer, overwrite old data
 *                          in the buffer. (Applicable only to read streams).
 *   @param [in]  dwRxTxTimeout:   Maximum time, in milliseconds, for the
 *                          completion of a data transfer between the stream
 *                          and the device. Zero = infinite wait.
 *   @param [out] phStream: Pointer to a unique identifier for the
 *                           stream, to be returned by the function and passed
 *                           to the other WDU_StreamXXX() functions.
 *
 *   @return   WinDriver Error Code
 *
 * @snippet highlevel_examples.c WDU_StreamOpen
 */
DWORD DLLCALLCONV WDU_StreamOpen(_In_ WDU_DEVICE_HANDLE hDevice,
    _In_ DWORD dwPipeNum, _In_ DWORD dwBufferSize, _In_ DWORD dwRxSize,
    _In_ BOOL fBlocking, _In_ DWORD dwOptions, _In_ DWORD dwRxTxTimeout,
    _Outptr_ WDU_STREAM_HANDLE *phStream);

/**  Closes an open stream.
 *   The function stops the stream, including flushing its data to the device
 *   (in the case of a write stream), before closing it.
 *   @param [in] hStream:  A unique identifier for the stream, as returned by
 *                  WDU_StreamOpen().
 *   @return   WinDriver Error Code
 *
 * @snippet highlevel_examples.c WDU_StreamClose
 */
DWORD DLLCALLCONV WDU_StreamClose(_In_ WDU_STREAM_HANDLE hStream);

/**  Starts a stream, i.e starts transfers between the stream and the device.
 *   Data will be transferred according to the stream's direction - read/write.
 *   @param [in] hStream:  A unique identifier for the stream, as returned by
 *                  WDU_StreamOpen().
 *   @return   WinDriver Error Code
 *
 * @snippet highlevel_examples.c WDU_StreamStart
 */
DWORD DLLCALLCONV WDU_StreamStart(_In_ WDU_STREAM_HANDLE hStream);

/**  Stops a stream, i.e stops transfers between the stream and the device.
 *   In the case of a write stream, the function flushes the stream - i.e
 *   writes its contents to the device - before stopping it.
 *   @param [in] hStream:  A unique identifier for the stream, as returned by
 *                  WDU_StreamOpen().
 *   @return   WinDriver Error Code
 */
DWORD DLLCALLCONV WDU_StreamStop(_In_ WDU_STREAM_HANDLE hStream);

/**  Flushes a stream, i.e writes the entire contents of the stream's data
 *   buffer to the device (relevant for a write stream), and blocks until the
 *   completion of all pending I/O on the stream.
 *   This function can be called for both blocking and non-blocking streams.
 *   @param [in] hStream:  A unique identifier for the stream, as returned by
 *                  WDU_StreamOpen().
 *   @return   WinDriver Error Code
 *
 * @snippet highlevel_examples.c WDU_StreamFlush
 */
DWORD DLLCALLCONV WDU_StreamFlush(_In_ WDU_STREAM_HANDLE hStream);

/**  Reads data from a read stream to the application.
 *   For a blocking stream (fBlocking=TRUE - see WDU_StreamOpen()), the call
 *   to this function is blocked until the specified amount of data is read, or
 *   until the stream's attempt to read from the device times out (i.e the
 *   timeout period for transfers between the stream and the device, as set in
 *   the dwRxTxTimeout WDU_StreamOpen() parameter, expires).
 *   For a non-blocking stream, the function transfers to the application as
 *   much of the requested data as possible, subject to the amount of data
 *   currently available in the stream's data buffer, and returns immediately.
 *   For both blocking and non-blocking transfers, the function returns the
 *   amount of bytes that were actually read from the stream within the
 *   pdwBytesRead parameter.
 *   @param [in] hStream:   A unique identifier for the stream, as returned by
 *                          WDU_StreamOpen().
 *   @param [out] pBuffer:  Pointer to a data buffer to be filled with the data
 *                          read from the stream.
 *   @param [in] bytes:     Number of bytes to read from the stream.
 *   @param [out] pdwBytesRead: Pointer to a value indicating the number of
 *                              bytes actually read from the stream.
 *   @return   WinDriver Error Code
 *
 * @snippet highlevel_examples.c WDU_StreamRead
 */
DWORD DLLCALLCONV WDU_StreamRead(_In_ HANDLE hStream, _Outptr_ PVOID pBuffer,
    _In_ DWORD bytes, _Outptr_ DWORD *pdwBytesRead);

/**  Writes data from the application to a write stream.
 *   For a blocking stream (fBlocking=TRUE - see WDU_StreamOpen()), the call
 *   to this function is blocked until the entire data (*pBuffer) is written to
 *   the stream's data buffer, or until the stream's attempt to write to the
 *   device times out (i.e the timeout period for transfers between the stream
 *   and the device, as set in the dwRxTxTimeout WDU_StreamOpen() parameter,
 *   expires).
 *   For a non-blocking stream (fBlocking=FALSE), the function writes as much
 *   of the write data as currently possible to the stream's data buffer, and
 *   returns immediately.
 *   For both blocking and non-blocking transfers, the function returns the
 *   amount of bytes that were actually written to the stream within the
 *   pdwBytesWritten parameter.
 *   @param [in] hStream:   A unique identifier for the stream, as returned by
 *                          WDU_StreamOpen().
 *   @param [in] pBuffer:   Pointer to a data buffer containing the data
 *                           to write to the stream.
 *   @param [in] bytes:     Number of bytes to write to the stream.
 *   @param [out] pdwBytesWritten: Pointer to a value indicating the number of
 *                                 bytes actually written to the stream.
 *   @return   WinDriver Error Code
 *
 * @snippet highlevel_examples.c WDU_StreamWrite
 */
DWORD DLLCALLCONV WDU_StreamWrite(_In_ HANDLE hStream,
    _In_ const PVOID pBuffer, _In_ DWORD bytes,
    _Outptr_ DWORD *pdwBytesWritten);

/**  Returns a stream's current status.
 *   This function can be called for both blocking and non-blocking streams.
 *   @param [in] hStream:           A unique identifier for the stream,
 *                                  as returned by WDU_StreamOpen().
 *   @param [out] pfIsRunning       Pointer to a boolean value indicating the
 *                                  stream's current state:
 *                                  TRUE - the stream is currently running;
 *                                  FALSE - the stream is currently stopped.
 *   @param [out] pdwLastError:     Pointer to the last error associated with
 *                                  the stream. Note: Calling this function
 *                                  also resets the stream's last error.
 *   @param [out] pdwBytesInBuffer: Pointer to the current bytes count in
 *                                  the stream's data buffer.
 *
 *   @return   WinDriver Error Code
 *
 * @snippet highlevel_examples.c WDU_StreamGetStatus
 */
DWORD DLLCALLCONV WDU_StreamGetStatus(_In_ WDU_STREAM_HANDLE hStream,
    _Outptr_ BOOL *pfIsRunning, _Outptr_ DWORD *pdwLastError,
    _Outptr_ DWORD *pdwBytesInBuffer);

#ifdef __cplusplus
}
#endif

#endif /* _WD_USB_H_ */

