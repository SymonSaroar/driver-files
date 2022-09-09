''' Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com '''

from __future__ import print_function
import sys, os
sys.path.append(os.path.dirname(os.path.abspath(__file__)) + "/..")
from .wdc_lib import *
from .wdu_lib import *
from .diag_lib import *

TRANSFER_TIMEOUT             = 30000   # In msecs

gsUSB_LastErr = ""

def TRACE(s):
    wdapi_va.WDC_Trace("USB lib Trace: " + s)
    print ("USB lib: " + s)

def ERR(s):
    global gsUSB_LastErr

    wdapi_va.WDC_Err("USB lib Error: " + s)
    gsUSB_LastErr = s

def USB_GetLastError():
    global gsUSB_LastErr

    return gsUSB_LastErr

class USB_LISTEN_PIPE(Structure): _fields_ = \
    [("Handle", WDU_DEVICE_HANDLE),
    ("Pipe", WDU_PIPE_INFO),
    ("pContext", PVOID),
    ("fStopped", BOOL),
    ("hThread", HANDLE),
    ("dwError", DWORD),
    ("fStreamMode", BOOL),
    ("dwBytesToTransfer", DWORD),
    ("dwOptions", DWORD),
    ("fPrint", BOOL)]

##
 #  Sets the interface number and the alternate setting of the given device
 #   @param [in] hDevice: A unique identifier for the device.
 #   @param [in] dwInterfaceNum: Interface number.
 #   @param [in] dwAlternateSetting: Alterante setting.
 #
 #   @return  Returns WD_STATUS_SUCCESS (0) on success,
 #     or an appropriate error code otherwise
 #
def USB_SetInterface(hDevice, dwInterfaceNumber, dwAlternateSetting):
    dwStatus = wdapi.WDU_SetInterface(hDevice, dwInterfaceNumber,
        dwAlternateSetting)
    if dwStatus:
        ERR("USB_SetInterface: wdapi.WDU_SetInterface() "
            "failed. error 0x%lx (\"%s\")\n" % (dwStatus,
            Stat2Str(dwStatus)))

    return dwStatus

##
 #  Resets pipe number `dwPipeNum` of the given device.
 #   @param [in] hDevice: A unique identifier for the device.
 #   @param [in] dwPipeNum: Pipe number to reset.
 #
 #   @return  Returns WD_STATUS_SUCCESS (0) on success,
 #     or an appropriate error code otherwise
 #
def USB_ResetPipe(hDevice, dwPipeNum):
    dwStatus = wdapi.WDU_ResetPipe(hDevice, dwPipeNum)
    if dwStatus:
        ERR("USB_ResetPipe: wdapi.WDU_ResetPipe() "
            "failed. error 0x%lx (\"%s\")\n" % (dwStatus,
            Stat2Str(dwStatus)))

    return dwStatus

##  Gets configuration information from the device including all the
 #   descriptors in a WDU_DEVICE struct. The caller should free #ppDeviceInfo.
 #   after using it by calling WDU_PutDeviceInfo().
 #   @param [in]  hDevice: handle to the USB device
 #   @param [out] ppDeviceInfo: Pointer to a pointer to a buffer containing the
 #                       device information.
 #
 #   @return  Returns WD_STATUS_SUCCESS (0) on success,
 #     or an appropriate error code otherwise
 #
def USB_GetDeviceInfo(hDevice, ppDevice):
    dwStatus = wdapi.WDU_GetDeviceInfo(hDevice, ppDevice)
    if dwStatus:
       ERR("USB_GetDeviceInfo: wdapi.WDU_GetDeviceInfo() "
            "failed. error 0x%lx (\"%s\")\n" % (dwStatus,
            Stat2Str(dwStatus)))

    return dwStatus

##  Submits a request to suspend a given device (selective suspend), or
 #   cancels a previous suspend request.
 #   @param [in] hDevice: Handle to the USB device
 #   @param [in] dwOptions: Can be set to either of the following
 #                   WDU_SELECTIVE_SUSPEND_OPTIONS enumeration values:
 #                   WDU_SELECTIVE_SUSPEND_SUBMIT - submit a request to
 #                       suspend the device.
 #                   WDU_SELECTIVE_SUSPEND_CANCEL - cancel a previous
 #                       suspend request for the device.
 #
 #   @return  Returns WD_STATUS_SUCCESS (0) on success,
 #     or an appropriate error code otherwise
 #
def USB_SelectiveSuspend(hDevice, dwOptions):
    dwStatus = wdapi.WDU_SelectiveSuspend(hDevice, dwOptions)
    if dwStatus:
        ERR("USB_SelectiveSuspend: wdapi.WDU_SelectiveSuspend() "
            "failed. error 0x%lx (\"%s\")\n" % (dwStatus,
            Stat2Str(dwStatus)))

    return dwStatus

##  Opens a new data stream for the specified pipe and reads/writes from/to it
 #   @param [in] hDevice: handle to the USB device
 #   @param [in] dwPipeNum: Pipe number
 #   @param [in,out] pBuffer: Pointer to a data buffer
 #   @param [in] dwSize: Number of bytes to read/write
 #   @param [out] pdwBytesTransferred: Pointer to a value indicating the number of
 #                              bytes actually read from the stream.
 #   @param [in] fRead: TRUE if read operation, FALSE if write operation.
 #
 #   @return  Returns WD_STATUS_SUCCESS (0) on success,
 #     or an appropriate error code otherwise
 #
def USB_ReadWriteStream(hDevice, dwPipeNum, pBuffer, dwBufferSize, dwSize,
        pdwBytesTransferred, fRead):
    stream = WDU_STREAM_HANDLE()

    while True:
        dwStatus = wdapi.WDU_StreamOpen(hDevice, dwPipeNum,
            dwBufferSize, dwSize, True, 0, TRANSFER_TIMEOUT,
            byref(stream))
        if dwStatus:
            ERR("USB_ReadWriteStream: wdapi.WDU_StreamOpen() failed."
                " error 0x%lx (\"%s\")\n" % (dwStatus,
                Stat2Str(dwStatus)))
            break #goto End_transfer
        dwStatus = wdapi.WDU_StreamStart(stream)
        if dwStatus:
            ERR("USB_ReadWriteStream: wdapi.WDU_StreamStart() "
                "failed. error 0x%lx (\"%s\")\n" % (dwStatus,
                Stat2Str(dwStatus)))
            break #goto End_transfer
        dwStatus = wdapi.WDU_StreamRead(stream, pBuffer, dwSize,
                pdwBytesTransferred) if fRead \
            else wdapi.WDU_StreamWrite(stream, pBuffer, dwSize,
                pdwBytesTransferred)
        if dwStatus:
            fIsRunning = BOOL()
            dwLastError = DWORD()
            dwBytesInBuffer = DWORD()

            dwStatus = wdapi.WDU_StreamGetStatus(stream,
                byref(fIsRunning), byref(dwLastError),
                byref(dwBytesInBuffer))
            if not dwStatus:
                dwStatus = dwLastError.value
        break

    #End_transfer:
    if stream:
        dwCloseStatus = wdapi.WDU_StreamClose(stream)
        # Avoid overwriting the dwStatus from above, if occured
        if dwCloseStatus and not dwStatus:
            ERR("USB_ReadWriteStream: wdapi.WDU_StreamClose() "
                "failed. error 0x%lx (\"%s\")\n" % (dwCloseStatus,
                Stat2Str(dwCloseStatus)))
            dwStatus = dwCloseStatus

    return dwStatus

##  Transfers data to/from a device.
 #   @param [in] hDevice: Handle to the USB device.
 #   @param [in] dwPipeNum: Pipe number.
 #   @param [in,out] pBuffer: Pointer to a data buffer.
 #   @param [in] dwTransferSize: Number of bytes to read/write from/to the stream
 #   @param [out] pdwBytesTransferred: Pointer to a value indicating the number of
 #                              bytes actually read from the stream.
 #   @param [in] fRead: TRUE if read operation, FALSE if write operation.
 #
 #   @return  Returns WD_STATUS_SUCCESS (0) on success,
 #     or an appropriate error code otherwise
 #
def USB_ReadWriteTransfer(hDevice, dwPipeNum, pBuffer, dwTransferSize,
    pSetupPacket, pdwBytesTransferred, fRead):

    dwStatus = wdapi.WDU_Transfer(hDevice, dwPipeNum, fRead, 0, pBuffer,
        dwTransferSize, pdwBytesTransferred, pSetupPacket, TRANSFER_TIMEOUT)
    
    if dwStatus:
        ERR("USB_ReadWriteTransfer: Transfer failed. error 0x%lx (\"%s\")\n" %
           (dwStatus, Stat2Str(dwStatus)))
    elif fRead and not pdwBytesTransferred._obj.value:
        ERR("USB_ReadWriteTransfer: Transferred 0 bytes, try"
            " increasing buffer size")
        dwStatus = WD_INVALID_PARAMETER

    return dwStatus

##  Opens a new data stream for the specified pipe and reads/writes from/to it.
 #   The function will perform a performance check on the stream and will print
 #   the results to stdout
 #   @param [in] hDevice: Handle to the USB device.
 #   @param [in] dwPipeNum: Pipe number.
 #   @param [in,out] pBuffer: Pointer to a data buffer.
 #   @param [in] dwBufferSize: Size of the buffer.
 #   @param [in] dwTransferSize: Number of bytes to read/write from/to the stream.
 #   @param [in] dwIterations: Number of iterations in the performance check.
 #   @param [in] fRead: TRUE if read operation, FALSE if write operation.
 #
 # @return  Returns WD_STATUS_SUCCESS (0) on success,
 #   or an appropriate error code otherwise
 #
def USB_ReadWriteStreamCheck(hDevice, dwPipeNum, pBuffer, dwBufferSize,
        dwTransferSize, dwIterations, fRead):
    stream = WDU_STREAM_HANDLE()

    dwStatus = wdapi.WDU_StreamOpen(hDevice, dwPipeNum,
        dwBufferSize, dwTransferSize, True, 0,
        TRANSFER_TIMEOUT, byref(stream))
    if dwStatus:
        ERR("USB_ReadWriteStreamCheck: wdapi.WDU_StreamOpen() failed. error "
            "0x%lx (\"%s\")" % (dwStatus, Stat2Str(dwStatus)))
        return dwStatus

    while True:
        dwStatus = wdapi.WDU_StreamStart(stream)
        if dwStatus:
            ERR("USB_ReadWriteStreamCheck: WDU_StreamStart() failed. error "
                "0x%lx (\"%s\")\n" % (dwStatus, Stat2Str(dwStatus)))
            break #goto End_perf_test

        streaming_time_start = get_cur_time()
        (dwStatus, dwBytesTransferred) = USB_TransferBytes(fRead, stream,
            pBuffer, dwTransferSize, dwIterations)
        if dwStatus:
            break #goto End_perf_test

        # If write command, wait for all the data to be written
        if not fRead:
            dwStatus = wdapi.WDU_StreamFlush(dwStatus)
            if dwStatus:
                ERR("USB_ReadWriteStreamCheck: Transfer failed. error 0x%lx "
                    "(\"%s\")\n" % (dwStatus, Stat2Str(dwStatus)))
                break #goto End_perf_test

        DIAG_PrintPerformance(dwBytesTransferred, streaming_time_start)
        break

    #End_perf_test:
    dwStatus = wdapi.WDU_StreamClose(stream)
    if dwStatus:
        ERR("USB_ReadWriteStreamCheck: WDU_StreamClose() failed. error "
            "0x%lx (\"%s\")\n" % (dwStatus, Stat2Str(dwStatus)))
    return dwStatus

## Returns a string identifying the pipe type.
 #   @param [in] pipeType: USB_PIPE_TYPE enum value.
 #
 #   @return A string containing the description of the pipe type.
 #
def USB_PipeType2Str(pipeType):
    return "Control" if pipeType == PIPE_TYPE_CONTROL \
    else "Isochronous" if pipeType == PIPE_TYPE_ISOCHRONOUS \
    else "Bulk" if pipeType == PIPE_TYPE_BULK \
    else "Interrupt" if pipeType == PIPE_TYPE_INTERRUPT \
    else "unknown"

##  Stops listening to a USB device pipe
 #   @param [in] pParam: pointer to the pipe to which to stop listening.
 #
 #   @return None.
 #
def StopListeningToPipe(ListenPipe):
    if not ListenPipe.hThread:
        return

    print("Stop listening to pipe\n")
    ListenPipe.fStopped = True

    if ListenPipe.fStreamMode:
        wdapi.WDU_StreamClose(HANDLE(ListenPipe.Handle))
    else:
        wdapi.WDU_HaltTransfer(ListenPipe.Handle, ListenPipe.Pipe.dwNumber)

    wdapi.ThreadWait(HANDLE(ListenPipe.hThread))
    ListenPipe.hThread = None

##  Callback function that listens to a pipe continuously when there is data
 #   available on the pipe
 #   @param [in] pParam: Pointer to the pipe to which to listen.
 #
 #   @return None.
 #
def PipeListenHandler(pParam):
    pListenPipe = cast(pParam, POINTER(USB_LISTEN_PIPE))
    dwBufsize = pListenPipe.contents.dwBytesToTransfer
    fPrint = pListenPipe.contents.fPrint
    qwTotalBytesTransferred = 0
    hDevice = pListenPipe.contents.Handle
    buf = create_string_buffer(dwBufsize)

    if not fPrint:
        streaming_time_start = get_cur_time()

    while True:
        dwBytesTransferred = DWORD()

        if pListenPipe.contents.fStreamMode:
            dwError = wdapi.WDU_StreamRead(PVOID(pListenPipe.contents.Handle),
                buf, dwBufsize, byref(dwBytesTransferred))
        else:
            dwError = wdapi.WDU_Transfer(pListenPipe.contents.Handle,
                pListenPipe.contents.Pipe.dwNumber, True,
                pListenPipe.contents.dwOptions, buf, dwBufsize,
                byref(dwBytesTransferred), None, TRANSFER_TIMEOUT)

        qwTotalBytesTransferred += dwBytesTransferred.value

        if pListenPipe.contents.fStopped:
            break

        if dwError:
            pListenPipe.contents.dwError = dwError
            print("Listen ended due to an error, press <Enter> to stop.\n")
            break

        if fPrint:
            DIAG_PrintHexBuffer(buf, dwBytesTransferred)

    if not fPrint:
        DIAG_PrintPerformance(qwTotalBytesTransferred, streaming_time_start)

PIPE_THREAD_CALLBACK = DLLCALLCONV_USB(None, PVOID)
# Reference to callback to keep it alive (Without this the program will crash
# upon handler call)
_PipeListenHandler = PIPE_THREAD_CALLBACK(PipeListenHandler)

## Starts listening to a USB device pipe
 #   @param [in] hDevice: Handle to the USB device.
 #   @param [in] pListenPipe: Pointer to the pipe for which to listen.
 #
 #   @return  Returns WD_STATUS_SUCCESS (0) on success,
 #     or an appropriate error code otherwise
 #
def StartListeningToPipe(ListenPipe):
    # Start the running thread
    ListenPipe.fStopped = False
    print("Start listening to pipe")

    if ListenPipe.fStreamMode:
        ListenPipe.dwError = wdapi.WDU_StreamStart(PVOID(ListenPipe.Handle))
        if ListenPipe.dwError:
            ERR("StartListeningToPipe: wdapi.WDU_StreamStart() failed. error"
                " 0x%lx (\"%s\")" % (ListenPipe.dwError,
                Stat2Str(ListenPipe.dwError)))
            return

    ListenPipe.dwError = wdapi.ThreadStart(byref(ListenPipe,
        USB_LISTEN_PIPE.hThread.offset),
        _PipeListenHandler, PVOID(addressof(ListenPipe)))

# Function: ListenToPipe()
#     Listening to a USB device pipe
#   
#     @param [In] hDevice:      Handle to the USB device
#     @param [In] pPipe:        Pointer to the pipe for listening
#     @param [In] fStreamMode:   True - Streaming mode
#     @param [In] dwBufferSize:  Kernel buffer size on streaming mode
#     @param [In] fUserKeyWait:  True - Wait for user key before starting
#     @param [In] fPrint:        True - Print the pipe contents
#     
#     @return  Returns WD_STATUS_SUCCESS (0) on success,
#       or an appropriate error code otherwise

listenPipe = USB_LISTEN_PIPE()
def USB_ListenToPipe(hDevice, pPipe, fStreamMode, dwBufferSize, fUserKeyWait,
                     fPrint):
    global listenPipe
    listenPipe.Pipe = pPipe
    listenPipe.fStreamMode = fStreamMode
    listenPipe.fPrint = fPrint

    if pPipe.type == PIPE_TYPE_ISOCHRONOUS:
        listenPipe.dwBytesToTransfer = pPipe.dwMaximumPacketSize * 8
        # 8 minimum packets for high speed transfers
        listenPipe.dwOptions |= USB_ISOCH_FULL_PACKETS_ONLY
    else:
        listenPipe.dwBytesToTransfer = pPipe.dwMaximumPacketSize * 36

    if fStreamMode:
        dwError = wdapi.WDU_StreamOpen(hDevice, pPipe.dwNumber, dwBufferSize,
            listenPipe.dwBytesToTransfer, True, listenPipe.dwOptions,
            TRANSFER_TIMEOUT, byref(listenPipe))
        if dwError:
            ERR("ListenToPipe: WDU_StreamOpen() failed. error 0x%lx (\"%s\")\n"
                % (dwError, Stat2Str(dwError)))
            return dwError
    else:
        listenPipe.Handle = hDevice

    if fUserKeyWait:
        print("Press <Enter> to start listening. While listening, press "
            "<Enter> to stop\n\n")
        inputf()

    else:
        print("Listening Started. While listening, press <Enter> to stop\n\n")

    StartListeningToPipe(listenPipe)
    if listenPipe.dwError:
        ERR("ListenToPipe: Error listening to pipe 0x%lx. error 0x%lx (\"%s\")"
            % (pPipe.dwNumber, listenPipe.dwError,
            Stat2Str(listenPipe.dwError)))
        return listenPipe.dwError
    while inputf() != "": # Waiting for <Enter> key
        continue

    StopListeningToPipe(listenPipe)
    if listenPipe.dwError:
        ERR("ListenToPipe: Transfer failed. error 0x%lx (\"%s\")" %
            (listenPipe.dwError, Stat2Str(listenPipe.dwError)))
        return listenPipe.dwError

    return WD_STATUS_SUCCESS


##  Prints pipe information; (helper function)
#   @param [in] pPipe: Pointer to the pipe information to print.
#   @param [in] fp: File pointer to print into(usually stdout).
#
#   @return None
#
def USB_PrintPipe(pPipe, fp):
    print("  pipe num. 0x%lx: packet size %ld, type %s, dir %s, interval %ld "
        "(ms)" % (pPipe.dwNumber, pPipe.dwMaximumPacketSize,
        USB_PipeType2Str(pPipe.type), "In" if pPipe.direction == WDU_DIR_IN else
        "Out" if pPipe.direction == WDU_DIR_OUT else "In & Out",
        pPipe.dwInterval), file=fp)

## Prints the device control pipe(0) information.
 #   @param [in] pDevice: Pointer to device configuration details.
 #   @param [in] fp: File pointer to print into(usually stdout).
 #
 #   @return None.
 #
def USB_PrintPipe0Info(pDevice, fp):
    print("\nControl pipe:", file=fp)
    USB_PrintPipe(pDevice.contents.Pipe0, fp)

## Prints the pipes information for the specified alternate setting.
 #   @param [in] pAltSet: Pointer to the alternate setting information.
 #   @param [in] fp: File pointer to print into(usually stdout).
 #
 #   @return None.
 #
def USB_PrintPipesInfo(pAltSet, fp):
    pPipe = pAltSet.contents.pPipes

    if not pAltSet.contents.Descriptor.bNumEndpoints:
        print("  no pipes are defined for this device other than the default "
            "pipe (number 0).", file=fp)
        return

    print("Alternate Setting: %d" %
        pAltSet.contents.Descriptor.bAlternateSetting, file=fp)

    for p in range(pAltSet.contents.Descriptor.bNumEndpoints):
        USB_PrintPipe(pPipe[p], fp)

## Prints the device pipes information.
 #   @param [in] pDevice: Pointer to device configuration details.
 #   @param [in] fp: File pointer to print into(usually stdout).
 #
 #   @return None.
 #
def USB_PrintDevicePipesInfo(pDevice, fp):
    USB_PrintPipe0Info(pDevice, fp)
    # Iterate over interfaces and print all pipes in their active alternate
    # settings
    for k in range(pDevice.contents.pActiveConfig.contents.dwNumInterfaces):
        pIfc = pDevice.contents.pActiveInterface[k]
        if not pIfc:
            break
        print("Interface %d" %
            pIfc.contents.pActiveAltSetting.contents.Descriptor.bInterfaceNumber,
            file=fp)
        USB_PrintPipesInfo(pIfc.contents.pActiveAltSetting, fp)

## Prints the pipes information for all the active device pipes.
 #   @param [in] hDevice: Handle to a USB device.
 #   @param [in] fp: File pointer to print into(usually stdout).
 #
 #   @return None.
 #
def USB_PrintDevicePipesInfoByHandle(hDevice, fp):
    Device = WDU_DEVICE()
    pDevice = POINTER(WDU_DEVICE)(Device)

    dwError = wdapi.WDU_GetDeviceInfo(hDevice, byref(pDevice))
    if dwError:
        ERR("PrintDevicePipesInfoByHandle: WDU_GetDeviceInfo() failed. "
            "error 0x%lx (\"%s\")\n" % (dwError, Stat2Str(dwError)))
        return

    USB_PrintDevicePipesInfo(pDevice, fp)

## Prints the endpoints (pipes) information for the specified alternate
 #   setting; (helper function for USB_PrintDeviceConfigurations())
 #   @param [in] pAltSet: Pointer to the alternate setting information.
 #   @param [in] fp: file Pointer to print into(usually stdout).
 #
 #   @return None
 #
def USB_PrintEndpoints(AltSet, fp):
    for i in range(AltSet.Descriptor.bNumEndpoints):
        Endp = AltSet.pEndpointDescriptors[i]
        print("    end-point address: 0x%02x, attributes: 0x%x, max packet %d,"
            " Interval: %d" % (Endp.bEndpointAddress, Endp.bmAttributes,
            Endp.wMaxPacketSize, Endp.bInterval), file=fp)

## Prints the device's configurations information,
 #   @param [in] hDevice: Handle to the USB device.
 #   @param [in] fp: File pointer to print into(usually stdout)
 #
 #   @return  Returns WD_STATUS_SUCCESS (0) on success,
 #   or an appropriate error code otherwise
 #
def USB_PrintDeviceConfigurations(hDevice, fp):
    Device = WDU_DEVICE()
    pDevice = POINTER(WDU_DEVICE)(Device)

    # No need to use WDU_PutDeviceInfo() as Python will free the memory.
    dwError = wdapi.WDU_GetDeviceInfo(hDevice, byref(pDevice))
    if dwError:
        ERR("PrintDeviceConfigurations: WDU_GetDeviceInfo failed. error 0x%lx "
            "(\"%s\")" % (dwError, Stat2Str(dwError)))
        return

    print("This device has %d configurations:" %
        pDevice.contents.Descriptor.bNumConfigurations, file=fp)
    for iConf in range(pDevice.contents.Descriptor.bNumConfigurations):
        print("  %d. Configuration value %d (has %ld interfaces)" %
            (iConf,
            pDevice.contents.pConfigs[iConf].Descriptor.bConfigurationValue,
            pDevice.contents.pConfigs[iConf].dwNumInterfaces), file=fp)

    iConf = 0

    if pDevice.contents.Descriptor.bNumConfigurations > 1:
        print("Please enter the configuration index to display "
            "(dec - zero based): ", file=fp)

        (iConf, dwStatus) = DIAG_InputNum("Please enter the" \
            " pipe number (hex)", True, sizeof(DWORD), 0, 0)
        if DIAG_INPUT_SUCCESS != dwStatus:
            return
        iConf = iConf.value

        if iConf >= pDevice.contents.Descriptor.bNumConfigurations:
            print("ERROR: Invalid configuration index, valid values are "
                "0-%d" % pDevice.contents.Descriptor.bNumConfigurations,
                file=fp)
            return

    pConf = pDevice.contents.pConfigs[iConf]

    print("The configuration indexed %d has %ld interface(s):" % (iConf,
        pConf.dwNumInterfaces), file=fp)

    for ifc in range(pConf.dwNumInterfaces):
        pInterface = pConf.pInterfaces[ifc]
        print("interface no. %d has %ld alternate settings:" %
            (pInterface.pAlternateSettings[0].Descriptor.bInterfaceNumber,
            pInterface.dwNumAltSettings), file=fp)
        for alt in range(pInterface.dwNumAltSettings):
            pAltSet = pInterface.pAlternateSettings[alt]
            print("  alternate: %d, endpoints: %d, class: 0x%x, "
                "subclass: 0x%x, protocol: 0x%x" %
                (pAltSet.Descriptor.bAlternateSetting,
                pAltSet.Descriptor.bNumEndpoints,
                pAltSet.Descriptor.bInterfaceClass,
                pAltSet.Descriptor.bInterfaceSubClass,
                pAltSet.Descriptor.bInterfaceProtocol), file=fp)
            USB_PrintEndpoints(pAltSet, fp)
        print("", file=fp)
    print("", file=fp)

## Finds a pipe in any of the device's active interfaces
 #   @param [in] pDevice: Pointer to the USB device.
 #   @param [in] dwPipeNumber: The pipe number to look for.
 #
 #   @return A pointer to the requested pipe, or NULL if no matching pipe
 #     was found
 #
def USB_FindPipeInDevice(pDevice, dwPipeNumber):
    if dwPipeNumber == 0:
        return pDevice.contents.Pipe0

    for i in range(pDevice.contents.pActiveConfig.contents.dwNumInterfaces):
        pIfc = pDevice.contents.pActiveInterface[i]
        if not pIfc:
            break
        pAltSet = pIfc.contents.pActiveAltSetting
        for j in range(pAltSet.contents.Descriptor.bNumEndpoints):
            if pAltSet.contents.pPipes[j].dwNumber == dwPipeNumber:
                return pAltSet.contents.pPipes[j]
    return None

def USB_TransferBytes(fRead, stream, pBuffer, dwTransferSize, dwIterations):
    dwBytesTransferred = 0
    while dwBytesTransferred < dwTransferSize * \
        dwIterations:

        dwBytesTransferredSingle = DWORD()
        if fRead:
            dwError = wdapi.WDU_StreamRead(stream, pBuffer,
                dwTransferSize, byref(dwBytesTransferredSingle))
        else:
            dwError = wdapi.WDU_StreamWrite(stream, pBuffer,
                dwTransferSize, byref(dwBytesTransferredSingle))
        if dwError:
            ERR("USB_TransferBytes: Transfer failed. error 0x%lx "
                "(\"%s\")\n" % (dwError, Stat2Str(dwError)))
            return (dwError, dwBytesTransferred)
        dwBytesTransferred += dwBytesTransferredSingle.value

    return (WD_STATUS_SUCCESS, dwBytesTransferred)

## Prints the device's serial number if available
 #   @param [in] hDevice: Handle to the USB device.
 #   @param [in] fp: File pointer to print into(usually stdout).
 #
 #   @return  None
 #
def USB_PrintDeviceSerialNumberByHandle(hDevice, fp):
    dwSerialDescSize = DWORD()
    Device = WDU_DEVICE()
    pDevice = POINTER(WDU_DEVICE)(Device)
    bSerialNum = create_string_buffer(0x100)

    #No need to use WDU_PutDeviceInfo() as Python will free the memory
    dwError = wdapi.WDU_GetDeviceInfo(hDevice, byref(pDevice))
    if dwError:
        ERR("USB_PrintDeviceSerialNumberByHandle: WDU_GetDeviceInfo failed. error 0x%lx "
            "(\"%s\")\n" % (dwError, Stat2Str(dwError)))
        return

    if not pDevice.contents.Descriptor.iSerialNumber:
        print("Serial number is not available", file=fp)
        return

    print("Serial number string descriptor index: [%d]" %
        pDevice.contents.Descriptor.iSerialNumber, file=fp)

    dwError = wdapi.WDU_GetStringDesc(hDevice,
        pDevice.contents.Descriptor.iSerialNumber,
        bSerialNum, sizeof(bSerialNum), 0, byref(dwSerialDescSize))
    if dwError:
        ERR("PrintDeviceSerialNumberByHandle: WDU_GetStringDesc() failed. "
            "error 0x%lx (\"%s\")" % (dwError, Stat2Str(dwError)))
        return

    print("Serial number: length [%ld], [%s]" % (dwSerialDescSize.value,
        bSerialNum[:dwSerialDescSize.value]), file=fp)

## Prints the device's serial number if available
 #   @param [in] hDevice: Handle to the USB device.
 #   @param [in] fp: File pointer to print into(usually stdout).
 #
 #   @return  None
 #
def USB_PrintDeviceProperties(hDevice, fp):

    propertyNames = \
    [ "WdDevicePropertyDeviceDescription", "WdDevicePropertyHardwareID", \
    "WdDevicePropertyCompatibleIDs", "WdDevicePropertyBootConfiguration", \
    "WdDevicePropertyBootConfigurationTranslated", "WdDevicePropertyClassName",\
    "WdDevicePropertyClassGuid", "WdDevicePropertyDriverKeyName", \
    "WdDevicePropertyManufacturer", "WdDevicePropertyFriendlyName", \
    "WdDevicePropertyLocationInformation", "WdDevicePropertyPhysicalDeviceObjectName",\
    "WdDevicePropertyBusTypeGuid", "WdDevicePropertyLegacyBusType", \
    "WdDevicePropertyBusNumber", "WdDevicePropertyEnumeratorName", \
    "WdDevicePropertyAddress","WdDevicePropertyUINumber", \
    "WdDevicePropertyInstallState", "WdDevicePropertyRemovalPolicy" ]

    for i in range(20):
        dwSize = DWORD(256)
        if sys.platform == "win32":
            cProperty = create_unicode_buffer(256)
        else:
            cProperty = create_string_buffer(256)

        dwStatus = wdapi.WDU_GetDeviceRegistryProperty(hDevice, cProperty,
            byref(dwSize), i)

        if WD_STATUS_SUCCESS == dwStatus:
            printf("%-46s: " % propertyNames[i]),
            if i > WdDevicePropertyPhysicalDeviceObjectName and \
                i != WdDevicePropertyEnumeratorName :
                    if sys.platform == "win32":
                        bs = bytearray(cProperty)
                        print(bs[0], file=fp)
                    else:
                        print(cProperty.value[0], file=fp)
            else:
                print("%s" % cProperty.value, file=fp)


