/*  */
package com.jungo.UsbDiag;

import com.jungo.*;
import com.jungo.wdapi.*;
import com.jungo.shared.*;

import java.nio.ByteBuffer;

/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/**************************************************************************
 * File - USB_DIAG_LIB.JAVA
 *
 * Utility functions for communication with USB devices using WinDriver's API.
 *
 * Note: This code sample is provided AS-IS and as a guiding sample only.
 **************************************************************************/

public class UsbDiagLib {

    final static int TRANSFER_TIMEOUT = 30000, /* In msecs */
        PERF_STREAM_BUFFER_SIZE = 5120000, /* In bytes */
        PERF_DEVICE_TRANSFER_SIZE = 256 * 1024, /* In bytes */
        PERF_PERFORMANCE_SAMPLE_TIME = 10000, /* In msecs */
        PERF_TRANSFER_ITERATIONS = 1500;

    static class USB_LISTEN_PIPE {
        long Handle;
        WDU_PIPE_INFO pPipe;
        long pContext;
        boolean fStopped;
        Thread hThread;
        long dwError;
        boolean fStreamMode;
        long dwBytesToTransfer;
        long dwOptions;
        boolean fPrint;
    };

    static String gsUSB_LastErr;

    static void ErrLog(String format, Object... args)
    {
        gsUSB_LastErr = String.format(format, args);
    }

    static String USB_GetLastErr()
    {
        return gsUSB_LastErr;
    }

    static class THREAD_HANDLER_FUNCTION implements Runnable
    {
        Object pParam;

        public THREAD_HANDLER_FUNCTION(Object pParam)
        {
            this.pParam = pParam;
        }

        public void run()
        {
            PipeListenHandler(pParam);
        }
    }

    /**
     *  Sets the interface number and the alternate setting of the given device
     *   @param [in] hDevice: A unique identifier for the device.
     *   @param [in] dwInterfaceNum: Interface number.
     *   @param [in] dwAlternateSetting: Alterante setting.
     *
     *   @return  Returns WD_STATUS_SUCCESS (0) on success,
     *     or an appropriate error code otherwise
     */
    static long USB_SetInterface(long hDevice, long dwInterfaceNum,
        long dwAlternateSetting)
    {
        long dwStatus = wdapi.WDU_SetInterface(hDevice, dwInterfaceNum,
            dwAlternateSetting);
        if (dwStatus != 0)
        {
            ErrLog("WDU_SetInterface() failed. error 0x%x (\"%s\")\n",
                dwStatus, wdapi.Stat2Str(dwStatus));
        }

        return dwStatus;
    }

    /**
     *  Resets pipe number `dwPipeNum` of the given device.
     *   @param [in] hDevice: A unique identifier for the device.
     *   @param [in] dwPipeNum: Pipe number to reset.
     *
     *   @return  Returns WD_STATUS_SUCCESS (0) on success,
     *     or an appropriate error code otherwise
     */
    static long USB_ResetPipe(long hDevice, long dwPipeNum)
    {
        long dwStatus = wdapi.WDU_ResetPipe(hDevice, dwPipeNum);
        if (dwStatus != 0)
        {
            ErrLog("WDU_ResetPipe() failed. error 0x%x (\"%s\")\n",
                dwStatus, wdapi.Stat2Str(dwStatus));
        }

        return dwStatus;
    }

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
    static long USB_SelectiveSuspend(long hDevice, long dwOptions)
    {
        long dwStatus = wdapi.WDU_SelectiveSuspend(hDevice, dwOptions);
        if (dwStatus != 0)
        {
            ErrLog("WDU_SelectiveSuspen() failed. error 0x%x (\"%s\")\n",
                dwStatus, wdapi.Stat2Str(dwStatus));
        }

        return dwStatus;
    }

    /**  Opens a new data stream for the specified pipe and reads/writes from/to it 
     *   @param [in] hDevice: handle to the USB device
     *   @param [in] dwPipeNum: Pipe number
     *   @param [in,out] pBuffer: Data buffer
     *   @param [in] deBufferSize: Size of the buffer
     *   @param [in] dwTransferSize: Number of bytes to read/write
     *   @param [in] fRead: TRUE if read operation, FALSE if write operation.
     *
     *   @return  Returns WDCResultLong type.
     *              dwStatus is set to WD_STATUS_SUCCESS (0) on success,
     *              or to an appropriate error code otherwise.
     *              result is set to the number of bytes that were read/written.
     */
    static WDCResultLong USB_ReadWriteStream(long hDevice, long dwPipeNum,
        ByteBuffer pBuffer, long dwBufferSize, long dwTransferSize,
        boolean fRead)
    {
        long dwStatus, dwBytesTransferred, stream;
        WDCResult result;
        WDCResultLong resultLong = new WDCResultLong(0, 0);

        result = wdapi.WDU_StreamOpen(hDevice, dwPipeNum,dwBufferSize,
            dwTransferSize, true, 0L, (long) TRANSFER_TIMEOUT);

        if (result.dwStatus != 0)
        {
            ErrLog(
                "WDU_StreamOpen() failed. "
                    + "error 0x%x (\"%s\")\n",
                    result.dwStatus, wdapi.Stat2Str(result.dwStatus));
            return (WDCResultLong)result;
        }
        stream = ((WDCResultLong)result).result;

        dwStatus = wdapi.WDU_StreamStart(stream);
        if (dwStatus != 0)
        {
            ErrLog(
                "WDU_StreamStart() failed. error "
                    + "0x%x (\"%s\")\n",
                    dwStatus, wdapi.Stat2Str(dwStatus));
            result.dwStatus = dwStatus;
            return (WDCResultLong)result;
        }

        if (fRead)
        {
            result = wdapi.WDU_StreamRead(stream, pBuffer, dwTransferSize);
            dwStatus = result.dwStatus;
            dwBytesTransferred = ((WDCResultLong)result).result;
        }
        else
        {
            result = wdapi.WDU_StreamWrite(stream, pBuffer, dwTransferSize);
            dwStatus = result.dwStatus;
            dwBytesTransferred = ((WDCResultLong)result).result;
        }
        if (dwStatus != 0)
        {
            boolean fIsRunning;
            long dwLastError = dwStatus;
            long dwBytesInBuffer;

            result = wdapi.WDU_StreamGetStatus(stream);
            dwStatus = result.dwStatus;

            fIsRunning = ((WDCResultUsbStream)result).fIsRunning;
            dwLastError = ((WDCResultUsbStream)result).dwLastError;
            dwBytesInBuffer = ((WDCResultUsbStream)result).dwBytesInBuffer;

            if (dwStatus == 0)
                dwStatus = dwLastError;
            
            ErrLog("Transfer failed. error 0x%x (\"%s\")\n", dwStatus,
                wdapi.Stat2Str(dwStatus));
        }

        if (stream != 0) {
            long dwCloseStatus = wdapi.WDU_StreamClose(stream);
            if (dwCloseStatus != 0 && dwStatus == 0) {
                dwStatus = dwCloseStatus;
                ErrLog(
                    "WDU_StreamClose() failed. error "
                        + "0x%x (\"%s\")\n",
                        dwStatus, wdapi.Stat2Str(dwStatus));
            }
        }

        resultLong.dwStatus = dwStatus;
        resultLong.result = dwBytesTransferred;
        return resultLong;
    }

    /**  Transfers data to/from a device.
     *   @param [in] hDevice: Handle to the USB device.
     *   @param [in] dwPipeNum: Pipe number.
     *   @param [in,out] pBuffer: Data buffer.
     *   @param [in] dwTransferSize: Number of bytes to read/write from/to the stream
     *   @param [in] fRead: TRUE if read operation, FALSE if write operation.
     *
     *   @return  Returns WDCResultLong type.
     *              dwStatus is set to WD_STATUS_SUCCESS (0) on success,
     *              or to an appropriate error code otherwise.
     *              result is set to the number of bytes that were read/written.
     */
    static WDCResultLong USB_ReadWriteTransfer(long hDevice, long dwPipeNum,
        ByteBuffer pBuffer, long dwTransferSize, byte[] pSetupPacket,
        boolean fRead)
    {
        long dwStatus;
        WDCResult result;

        result = wdapi.WDU_Transfer(hDevice, dwPipeNum,fRead, 0, pBuffer,
            dwTransferSize, pSetupPacket, TRANSFER_TIMEOUT);
        dwStatus = result.dwStatus;

        if (dwStatus != 0)
        {
            ErrLog("Transfer failed. error 0x%x (\"%s\")\n", dwStatus,
                    wdapi.Stat2Str(dwStatus));
        }

        return (WDCResultLong)result;
    }

    /**  Opens a new data stream for the specified pipe and reads/writes from/to it.
     *   The function will perform a performance check on the stream and will print
     *   the results to stdout
     *   @param [in] hDevice: Handle to the USB device.
     *   @param [in] dwPipeNum: Pipe number.
     *   @param [in,out] pBuffer: Data buffer.
     *   @param [in] dwBufferSize: Size of the buffer.
     *   @param [in] dwTransferSize: Number of bytes to read/write from/to the stream.
     *   @param [in] dwIterations: Number of iterations in the performance check.
     *   @param [in] fRead: TRUE if read operation, FALSE if write operation.
     *
     * @return  Returns WD_STATUS_SUCCESS (0) on success,
     *   or an appropriate error code otherwise
     */
    static long USB_ReadWriteStreamCheck(long hDevice, long dwPipeNum,
        ByteBuffer pBuffer, long dwBufferSize, long dwTransferSize,
        long dwIterations, boolean fRead)
    {
        long dwStatus, dwBytesTransferred, stream;
        long streaming_time_start, streaming_time_end;
        long perf_time_total;
        WDCResult result;
    
        result = wdapi.WDU_StreamOpen(hDevice, dwPipeNum,
            dwBufferSize, dwTransferSize, true, 0, TRANSFER_TIMEOUT);

        if (result.dwStatus != 0)
        {
            dwStatus = result.dwStatus;
            ErrLog(
                "WDU_StreamOpen() failed. error 0x%x (\"%s\")\n",
                dwStatus, wdapi.Stat2Str(dwStatus));
            return dwStatus;
        }
        stream = ((WDCResultLong)result).result;

        pBuffer = ByteBuffer.allocateDirect(PERF_DEVICE_TRANSFER_SIZE);
        if (pBuffer == null) {
            ErrLog("Failed allocating memory\n");
            return wdapi.WD_INSUFFICIENT_RESOURCES;
        }

        dwStatus = wdapi.WDU_StreamStart(stream);
        if (dwStatus != 0)
        {
            ErrLog(
                "WDU_StreamStart() failed. error 0x%x (\"%s\")\n",
                dwStatus, wdapi.Stat2Str(dwStatus));
            return dwStatus;
        }

        dwBytesTransferred = 0;
        streaming_time_start = DiagLib.get_cur_time();
        do
        {
            long dwBytesTransferredSingle;

            if (fRead)
            {
                result = wdapi.WDU_StreamRead(stream, pBuffer,
                    PERF_DEVICE_TRANSFER_SIZE);
                dwStatus = result.dwStatus;
                dwBytesTransferredSingle =
                    ((WDCResultLong)result).result;
            }
            else
            {
                result = wdapi.WDU_StreamWrite(stream, pBuffer,
                    PERF_DEVICE_TRANSFER_SIZE);
                dwStatus = result.dwStatus;
                dwBytesTransferredSingle =
                    ((WDCResultLong)result).result;
            }

            if (dwStatus != 0)
            {
                ErrLog("Transfer failed. error 0x%x (\"%s\")\n",
                    dwStatus, wdapi.Stat2Str(dwStatus));
                return dwStatus;
            }

            dwBytesTransferred += dwBytesTransferredSingle;

        } while (dwBytesTransferred < dwTransferSize * dwIterations);

        /* If write command, wait for all the data to be written */
        if (!fRead)
        {
            dwStatus = wdapi.WDU_StreamFlush(stream);
            if (dwStatus != 0) {
                ErrLog(
                    "Transfer failed. error 0x%x (\"%s\")\n",
                        dwStatus, wdapi.Stat2Str(dwStatus));
                return dwStatus;
            }
        }

        DiagLib.DIAG_PrintPerformance(dwBytesTransferred,
            streaming_time_start);

        dwStatus = wdapi.WDU_StreamClose(stream);
        if (dwStatus != 0) {
            ErrLog(
                "WDU_StreamClose() failed. error 0x%x (\"%s\")\n",
                dwStatus, wdapi.Stat2Str(dwStatus));
        }
        return dwStatus;
    }

   /** Returns a string identifying the pipe type.
     *   @param [in] pipeType: USB_PIPE_TYPE enum value.
     *
     *   @return A string containing the description of the pipe type.
     */
    static String USB_pipeType2Str(int pipeType) {
        String res = "unknown";

        switch (pipeType) {
        case wdapi.PIPE_TYPE_CONTROL:
            res = "Control";
            break;
        case wdapi.PIPE_TYPE_ISOCHRONOUS:
            res = "Isochronous";
            break;
        case wdapi.PIPE_TYPE_BULK:
            res = "Bulk";
            break;
        case wdapi.PIPE_TYPE_INTERRUPT:
            res = "Interrupt";
            break;
        }

        return res;
    }

    /**  Stops listening to a USB device pipe
     *   @param [in] pParam: Pipe to which to stop listening.
     *
     *   @return None.
     */
    static void StopListeningToPipe(USB_LISTEN_PIPE pListenPipe)
    {
        if (pListenPipe.hThread == null)
            return;

        System.out.printf("Stop listening to pipe\n");
        pListenPipe.fStopped = true;

        if (pListenPipe.fStreamMode)
            wdapi.WDU_StreamClose(pListenPipe.Handle);
        else
            wdapi.WDU_HaltTransfer(pListenPipe.Handle,
                pListenPipe.pPipe.dwNumber);
        synchronized (pListenPipe)
        {
            try
            {
                pListenPipe.hThread.join();
            }
            catch (Exception e)
            { }
        }

        pListenPipe.hThread = null;
    }

    /**  Callback function that listens to a pipe continuously when there is data
     *   available on the pipe
     *   @param [in] pParam: Pipe to which to listen.
     *
     *   @return None.
     */
    static void PipeListenHandler(Object pParam)
    {
        USB_LISTEN_PIPE pListenPipe = (USB_LISTEN_PIPE) pParam;
        long dwBufsize = pListenPipe.dwBytesToTransfer;
        boolean fPrint = pListenPipe.fPrint;
        WDCResultLong result;
        ByteBuffer buf;
        byte[] tmp = new byte[(int) dwBufsize];
        long qwTotalBytesTransferred = 0;
        long streaming_time_start = DiagLib.get_cur_time();

        buf = ByteBuffer.allocateDirect((int) dwBufsize);
        if (buf == null) {
            ErrLog("PipeListenHandler: Memory allocation failed\n");
            return;
        }

        for (;;)
        {
            long dwError;
            long dwBytesTransferred;

            if (pListenPipe.fStreamMode)
            {
                result = wdapi.WDU_StreamRead(pListenPipe.Handle, buf,
                    dwBufsize);
            }
            else
            {
                result = wdapi.WDU_Transfer(pListenPipe.Handle,
                    pListenPipe.pPipe.dwNumber, true, pListenPipe.dwOptions,
                    buf, dwBufsize, null, TRANSFER_TIMEOUT);
            }
            dwError = result.dwStatus;
            dwBytesTransferred = result.result;
            qwTotalBytesTransferred += dwBytesTransferred;

            if (pListenPipe.fStopped)
                break;

            if (dwError != 0)
            {
                pListenPipe.dwError = dwError;
                System.out.printf(
                    "Listen ended due to an error, press <Enter> to stop.\n");
                break;
            }
            if (fPrint)
            {
                if (dwBytesTransferred > 0)
                {
                    buf.position(0);
                    buf.get(tmp);
                    DiagLib.DIAG_PrintHexBuffer(tmp);
                }
                else
                {
                    System.out.printf("0 bytes transferred");
                }
            }
        }
        if (!fPrint)
        {
            DiagLib.DIAG_PrintPerformance(qwTotalBytesTransferred,
                streaming_time_start);
        }
    }

    /** Starts listening to a USB device pipe
     *   @param [in] pListenPipe: Pipe for which to listen.
     *
     *   @return  Returns WD_STATUS_SUCCESS (0) on success,
     *     or an appropriate error code otherwise
     */
    static void StartListeningToPipe(USB_LISTEN_PIPE pListenPipe)
    {
        /* Start the running thread */
        pListenPipe.fStopped = false;
        System.out.printf("Start listening to pipe\n");

        if (pListenPipe.fStreamMode)
        {
            pListenPipe.dwError = wdapi.WDU_StreamStart(pListenPipe.Handle);
            if (pListenPipe.dwError != 0)
            {
                ErrLog(
                    "StartListeningToPipe: WDU_StreamStart() failed. error 0x%x "
                    + "(\"%s\")\n",
                    pListenPipe.dwError, wdapi.Stat2Str(pListenPipe.dwError));
                return;
            }
        }

        Runnable r = new THREAD_HANDLER_FUNCTION(pListenPipe);

        pListenPipe.hThread = new Thread(r);
        pListenPipe.hThread.start();
    }

    /** Listening to a USB device pipe
     *   @param [in] hDevice: Handle to the USB device.
     *   @param [in] pPipe: Pointer to the pipe for listening.
     *   @param [in] fStreamMode: TRUE - Streaming mode, FALSE - Transfer mode.
     *   @param [in] dwBufferSize: Buffer size on streaming mode.
     *   @param [in] fUserKeyWait: TRUE - Wait for user key before starting.
     *   @param [in] fPrint: TRUE - print info to screen, FALSE - Don't print.
     *
     *   @return  Returns WD_STATUS_SUCCESS (0) on success,
     *   or an appropriate error code otherwise
     */
    static long USB_ListenToPipe(long hDevice, WDU_PIPE_INFO pPipe,
        boolean fStreamMode, long dwBufferSize, boolean fUserKeyWait,
        boolean fPrint)
    {
        USB_LISTEN_PIPE listenPipe = new USB_LISTEN_PIPE();
        WDCResult result;

        listenPipe.pPipe = pPipe;
        listenPipe.fStreamMode = fStreamMode;
        listenPipe.fPrint = fPrint;

        if (pPipe.type == wdapi.PIPE_TYPE_ISOCHRONOUS)
        {
            listenPipe.dwBytesToTransfer = pPipe.dwMaximumPacketSize * 8;
            /* 8 minimum packets for high speed transfers */
            listenPipe.dwOptions |= wdapi.USB_ISOCH_FULL_PACKETS_ONLY;
        }
        else
        {
            listenPipe.dwBytesToTransfer = pPipe.dwMaximumPacketSize * 36;
        }

        if (fStreamMode)
        {
            result = wdapi.WDU_StreamOpen(hDevice, pPipe.dwNumber, dwBufferSize,
                listenPipe.dwBytesToTransfer, true, listenPipe.dwOptions,
                TRANSFER_TIMEOUT);

            if (result.dwStatus != 0)
            {
                ErrLog(
                    "ListenToPipe: WDU_StreamOpen() failed. "
                        + "error 0x%x (\"%s\")\n",
                    result.dwStatus, wdapi.Stat2Str(result.dwStatus));
                return result.dwStatus;
            }
            listenPipe.Handle = ((wdapi.WDCResultLong) result).result;
        }
        else
        {
            listenPipe.Handle = hDevice;
        }

        if (fUserKeyWait)
        {
            System.out.printf(
                "Press <Enter> to start listening. While listening, press "
                    + "<Enter> to stop\n\n");
            try
            {
                System.in.read();
            }
            catch (Exception e)
            { }
        }
        else
        {
            System.out.printf(
                "Listening Started. While listening, press <Enter> to stop\n\n");
        }

        StartListeningToPipe(listenPipe);
        if (listenPipe.dwError != 0)
        {
            ErrLog(
                "ListenToPipe: Error listening to pipe 0x%x. error "
                    + "0x%x (\"%s\")\n",
                pPipe.dwNumber, listenPipe.dwError,
                wdapi.Stat2Str(listenPipe.dwError));
            return listenPipe.dwError;
        }
        try
        {
            while (System.in.read() != (wdapi.isWindows ? 13 : 10));
        }
        catch (Exception e)
        {
        }

        StopListeningToPipe(listenPipe);
        if (listenPipe.dwError != 0)
        {
            ErrLog(
                "ListenToPipe: Transfer failed. error 0x%x (\"%s\")\n",
                listenPipe.dwError, wdapi.Stat2Str(listenPipe.dwError));
            return listenPipe.dwError;
        }

        return wdapi.WD_STATUS_SUCCESS;
    }

    /** Prints pipe information; (helper function).
     *   @param [in] pPipe: Pointer to the pipe information.
     *
     *   @return None.
     */
    static void USB_PrintPipe(final WDU_PIPE_INFO pPipe)
    {
        System.out.printf(
            "  pipe num. 0x%x: packet size %d, type %s, dir %s, "
                + "interval %d (ms)\n",
            pPipe.dwNumber, pPipe.dwMaximumPacketSize,
            USB_pipeType2Str((int) pPipe.type),
            pPipe.direction == wdapi.WDU_DIR_IN ? "In"
                : pPipe.direction == wdapi.WDU_DIR_OUT ? "Out" : "In & Out",
            pPipe.dwInterval);
    }

    /** Prints the device control pipe(0) information.
     *   @param [in] pDevice: Pointer to device configuration details.
     *
     *   @return None.
     */
    static void USB_PrintPipe0Info(WDU_DEVICE pDevice)
    {
        System.out.printf("Control pipe:\n");
        USB_PrintPipe(pDevice.Pipe0);
    }

    /** Prints the pipes information for the specified alternate setting.
     *   @param [in] pAltSet: Pointer to the alternate setting information.
     *   @param [in] fp: File pointer to print into(usually stdout).
     *
     *   @return None.
     */
    static void USB_PrintPipesInfo(WDU_ALTERNATE_SETTING pAltSet)
    {
        if (pAltSet.Descriptor.bNumEndpoints == 0)
        {
            System.out.printf(
                "  no pipes are defined for this device other than the default "
                    + "pipe (number 0).\n");
            return;
        }

        System.out.printf("Alternate Setting: %d\n",
            pAltSet.Descriptor.bAlternateSetting);
        for (WDU_PIPE_INFO p : pAltSet.pPipes)
            USB_PrintPipe(p);
    }

    /** Prints the pipes information for all the active device pipes.
     *   @param [in] hDevice: Handle to the USB device.
     *
     *   @return  Returns WD_STATUS_SUCCESS (0) on success,
     *   or an appropriate error code otherwise
     */
    static void USB_PrintDevicePipesInfo(WDU_DEVICE pDevice)
    {
        int k;

        USB_PrintPipe0Info(pDevice);
        /*
         * Iterate over interfaces and print all pipes in their active alternate
         * settings
         */
        for (k = 0; k < pDevice.pConfigs[pDevice.ActiveConfigIx].dwNumInterfaces;
            k++)
        {
            WDU_INTERFACE pIfc =
                pDevice.pConfigs[pDevice.ActiveConfigIx].pInterfaces[k];

            if (pIfc == null)
                break;

            System.out.printf("Interface %d\n",
                pIfc.pAlternateSettings[pIfc.ActiveAltSettingIx].
                Descriptor.bInterfaceNumber);
            USB_PrintPipesInfo(pIfc.pAlternateSettings[pIfc.ActiveAltSettingIx]);
        }
    }

    /** Prints the endpoints (pipes) information for the specified alternate
     *  setting; (helper function for USB_PrintDeviceConfigurations())
     *   @param [in] pAltSet: Pointer to the alternate setting information.
     *
     *   @return None
     */
    static void USB_PrintEndpoints(final WDU_ALTERNATE_SETTING pAltSet)
    {
        byte endp;
        WDU_ENDPOINT_DESCRIPTOR pEndp;

        for (endp = 0; endp < pAltSet.Descriptor.bNumEndpoints; endp++)
        {
            pEndp = pAltSet.pEndpointDescriptors[endp];
            System.out.printf(
                "    end-point address: 0x%02x, attributes: 0x%x, max packet %d,"
                    + " Interval: %d\n",
                pEndp.bEndpointAddress, pEndp.bmAttributes,
                pEndp.wMaxPacketSize, pEndp.bInterval);
        }
    }

    /** Prints the device's configurations information,
     *   @param [in] hDevice: Handle to the USB device.
     *
     *   @return  Returns WD_STATUS_SUCCESS (0) on success,
     *   or an appropriate error code otherwise
     */
    static long USB_PrintDeviceConfigurations(WDU_DEVICE pDevice)
    {
        int ifc;
        int iConf;
        WDCResultLong result;
        WDU_CONFIGURATION pConf;
        WDU_INTERFACE pInterface;
        WDU_ALTERNATE_SETTING pAltSet;

        System.out.printf("This device has %d configurations:\n",
            pDevice.Descriptor.bNumConfigurations);
        for (iConf = 0; iConf < pDevice.Descriptor.bNumConfigurations; iConf++)
        {
            System.out.printf(
                "  %d. Configuration value %d (has %d interfaces)\n", iConf,
                pDevice.pConfigs[iConf].Descriptor.bConfigurationValue,
                pDevice.pConfigs[iConf].dwNumInterfaces);
        }
        iConf = 0;

        if (pDevice.Descriptor.bNumConfigurations > 1) {
            result = DiagLib.DIAG_InputNum(
                "Please enter the configuration index to display (dec - zero based):",
                false, 0, pDevice.Descriptor.bNumConfigurations);
            if (result.dwStatus != DiagLib.DIAG_INPUT_SUCCESS)
                return result.dwStatus;
            iConf = (int) ((WDCResultLong) result).result;

            if (iConf >= pDevice.Descriptor.bNumConfigurations)
            {
                ErrLog(
                    "Error: Invalid configuration index, valid values are "
                        + "0-%d\n",
                    pDevice.Descriptor.bNumConfigurations);
                return wdapi.WD_INVALID_PARAMETER;
            }
        }
        pConf = pDevice.pConfigs[iConf];

        System.out.printf("The configuration indexed %d has %d interface(s):\n",
            iConf, pConf.dwNumInterfaces);

        for (ifc = 0; ifc < pConf.dwNumInterfaces; ifc++)
        {
            int alt;

            pInterface = pConf.pInterfaces[ifc];
            System.out.printf("interface no. %d has %d alternate settings:\n",
                pInterface.pAlternateSettings[0].Descriptor.bInterfaceNumber,
                pInterface.dwNumAltSettings);
            for (alt = 0; alt < pInterface.dwNumAltSettings; alt++)
            {
                pAltSet = pInterface.pAlternateSettings[alt];

                System.out.printf(
                    "  alternate: %d, endpoints: %d, class: 0x%x, "
                        + "subclass: 0x%x, protocol: 0x%x\n",
                    pAltSet.Descriptor.bAlternateSetting,
                    pAltSet.Descriptor.bNumEndpoints,
                    pAltSet.Descriptor.bInterfaceClass,
                    pAltSet.Descriptor.bInterfaceSubClass,
                    pAltSet.Descriptor.bInterfaceProtocol);

                USB_PrintEndpoints(pAltSet);
            }
            System.out.printf("\n");
        }
        System.out.printf("\n");

        return wdapi.WD_STATUS_SUCCESS;
    }

    /** Finds a pipe in any of the device's active interfaces
     *   @param [in] pDevice: Pointer to the USB device.
     *   @param [in] dwPipeNumber: The pipe number to look for.
     *
     *   @return A pointer to the requested pipe, or NULL if no matching pipe
     *     was found
     */
    static WDU_PIPE_INFO USB_FindPipeInDevice(WDU_DEVICE pDevice,
        long dwPipeNumber)
    {
        int i;

        if (dwPipeNumber == 0)
            return pDevice.Pipe0;

        for (i = 0; i < pDevice.pConfigs[pDevice.ActiveConfigIx].dwNumInterfaces; i++)
        {
            int j;
            WDU_INTERFACE pIfc = pDevice.pConfigs[pDevice.ActiveConfigIx].
                pInterfaces[pDevice.ActiveInterfaceIx];
            WDU_ALTERNATE_SETTING pAltSet;

            if (pIfc == null)
                break;

            pAltSet = pIfc.pAlternateSettings[pIfc.ActiveAltSettingIx];

            for (j = 0; j < pAltSet.Descriptor.bNumEndpoints; j++)
            {
                if (pAltSet.pPipes[j].dwNumber == dwPipeNumber)
                    return pAltSet.pPipes[j];
            }
        }

        return null;
    }

    /** Prints the device's serial number if available
     *   @param [in] hDevice: Handle to the USB device.
     *
     *   @return  Returns WD_STATUS_SUCCESS (0) on success,
     *     or an appropriate error code otherwise
     */
    static long USB_PrintDeviceSerialNumberByHandle(
        UsbDiag.DEVICE_CONTEXT pDevCtx)
    {
        String sSerialNum;

        if (pDevCtx.dev.Descriptor.iSerialNumber == 0)
        {
            ErrLog("Serial number is not available\n");
            return wdapi.WD_INVALID_PARAMETER;
        }

        System.out.printf("Serial number string descriptor index: [%d]\n",
            pDevCtx.dev.Descriptor.iSerialNumber);

        sSerialNum = wdapi.WDU_GetStringDesc(pDevCtx.hDevice,
            pDevCtx.dev.Descriptor.iSerialNumber, (short) 0);
        if (sSerialNum.isEmpty())
        {
            ErrLog("WDU_GetStringDesc() failed. String is empty.\n");
            return wdapi.WD_WINDRIVER_STATUS_ERROR;
        }

        System.out.printf("Serial number: length [%d], [%s]\n",
            sSerialNum.length(), sSerialNum);

        return wdapi.WD_STATUS_SUCCESS;
    }

    /** Prints the device properties if available
     *   @param [in] hDevice: Handle to the USB device.
     *
     *   @return  Returns WD_STATUS_SUCCESS (0) on success,
     *     or an appropriate error code otherwise
     */
    static void USB_PrintDeviceProperties(UsbDiag.DEVICE_CONTEXT pDevCtx)
    {
        long dwStatus = 0;
        long dwSize;
        String cProperty;
        String[] propertyNames =
        { "WdDevicePropertyDeviceDescription", "WdDevicePropertyHardwareID",
        "WdDevicePropertyCompatibleIDs", "WdDevicePropertyBootConfiguration",
        "WdDevicePropertyBootConfigurationTranslated", "WdDevicePropertyClassName",
        "WdDevicePropertyClassGuid", "WdDevicePropertyDriverKeyName",
        "WdDevicePropertyManufacturer", "WdDevicePropertyFriendlyName",
        "WdDevicePropertyLocationInformation", "WdDevicePropertyPhysicalDeviceObjectName",
        "WdDevicePropertyBusTypeGuid", "WdDevicePropertyLegacyBusType",
        "WdDevicePropertyBusNumber", "WdDevicePropertyEnumeratorName",
        "WdDevicePropertyAddress","WdDevicePropertyUINumber",
        "WdDevicePropertyInstallState", "WdDevicePropertyRemovalPolicy" };

        for (int i = 0; i < 20; i++)
        {
            cProperty = wdapi.WDU_GetDeviceRegistryProperty(pDevCtx.hDevice, i);
            if (!cProperty.isEmpty())
            {
                System.out.printf("%-46s: %s\n", propertyNames[i], cProperty);
            }
        }
    }
}


