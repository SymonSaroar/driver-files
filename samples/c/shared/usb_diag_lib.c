/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/**************************************************************************
 * File - USB_DIAG_LIB.C
 *
 * Utility functions for communication with USB devices
 * using WinDriver's API.
 *
 * Note: This code sample is provided AS-IS and as a guiding sample only.
 **************************************************************************/
#include <stdio.h>
#include "windrvr.h"
#include "wdu_lib.h"
#include "status_strings.h"
#include "utils.h"

/* Last error information string */
static CHAR gsUSB_LastErr[256];

#include "usb_diag_lib.h"

#if !defined(ERR)
    #define ERR printf
#endif

#define TRANSFER_TIMEOUT 30000 /* In msecs */
#define ENTER_KEY 10

/* Log a debug error message */
static void ErrLog(const CHAR *sFormat, ...);
/* Log a debug trace message */
static void TraceLog(const CHAR *sFormat, ...);

typedef struct
{
    HANDLE Handle;
    WDU_PIPE_INFO *pPipe;
    PVOID pContext;
    BOOL fStopped;
    HANDLE hThread;
    DWORD dwError;
    BOOL fStreamMode;
    DWORD dwBytesToTransfer;
    DWORD dwOptions;
    BOOL fPrint;
    FILE *fpOut;
    FILE *fpIn;
} USB_LISTEN_PIPE;

DWORD USB_SetInterface(WDU_DEVICE_HANDLE hDevice, DWORD dwInterfaceNum,
    DWORD dwAlternateSetting)
{
    DWORD dwStatus = WDU_SetInterface(hDevice, dwInterfaceNum,
        dwAlternateSetting);
    if (dwStatus)
    {
        ErrLog("WDU_SetInterface() failed. error 0x%x (\"%s\")\n", dwStatus,
            Stat2Str(dwStatus));
    }
    else
    {
        TraceLog("WDU_SetInterface() completed successfully\n");
    }

    return dwStatus;
}

DWORD USB_ResetPipe(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum)
{
    DWORD dwStatus = WDU_ResetPipe(hDevice, dwPipeNum);
    if (dwStatus)
    {
        ErrLog("WDU_ResetPipe() failed. error 0x%x (\"%s\")\n", dwStatus,
            Stat2Str(dwStatus));
    }
    else
    {
        TraceLog("WDU_ResetPipe() completed successfully\n");
    }

    return dwStatus;
}

DWORD USB_GetDeviceInfo(WDU_DEVICE_HANDLE hDevice,
    WDU_DEVICE **ppDeviceInfo)
{
    DWORD dwStatus = WDU_GetDeviceInfo(hDevice, ppDeviceInfo);
    if (dwStatus)
    {
        ErrLog("WDU_GetDeviceInfo() failed. error 0x%x (\"%s\")\n", dwStatus,
            Stat2Str(dwStatus));
    }

    return dwStatus;
}

DWORD USB_SelectiveSuspend(WDU_DEVICE_HANDLE hDevice, DWORD dwOptions)
{
    DWORD dwStatus = WDU_SelectiveSuspend(hDevice, dwOptions);
    if (dwStatus)
    {
        ErrLog("WDU_SelectiveSuspend() failed. error 0x%x (\"%s\")\n",
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

DWORD USB_ReadWriteStream(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum,
    PVOID pBuffer, DWORD dwBufferSize, DWORD dwTransferSize,
    DWORD *pdwBytesTransferred, BOOL fRead)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;
    WDU_STREAM_HANDLE stream;
    
    dwStatus = WDU_StreamOpen(hDevice, dwPipeNum,
        dwBufferSize, dwTransferSize, TRUE, 0, TRANSFER_TIMEOUT, &stream);
    if (dwStatus)
    {
        ErrLog("WDU_StreamOpen() failed. error 0x%x (\"%s\")\n", dwStatus,
            Stat2Str(dwStatus));
        goto End_transfer;
    }

    dwStatus = WDU_StreamStart(stream);
    if (dwStatus)
    {
        ErrLog("WDU_StreamStart() failed. error 0x%x (\"%s\")\n", dwStatus,
            Stat2Str(dwStatus));
        goto End_transfer;
    }

    if (fRead)
    {
        dwStatus = WDU_StreamRead(stream, pBuffer, dwTransferSize,
            pdwBytesTransferred);
    }
    else
    {
        dwStatus = WDU_StreamWrite(stream, pBuffer, dwTransferSize,
            pdwBytesTransferred);
    }
    if (dwStatus)
    {
        BOOL fIsRunning;
        DWORD dwLastError;
        DWORD dwBytesInBuffer;

        dwStatus = WDU_StreamGetStatus(stream, &fIsRunning,
            &dwLastError, &dwBytesInBuffer);
        if (!dwStatus)
            dwStatus = dwLastError;
    }
   
End_transfer:
    if (stream)
    {
        DWORD dwCloseStatus = WDU_StreamClose(stream);
        /* Avoid overwriting the dwStatus from above, if occured */
        if (dwCloseStatus && !dwStatus)
        {
            ErrLog("WDU_StreamClose() failed. error 0x%x (\"%s\")\n",
                dwStatus, Stat2Str(dwStatus));
            dwStatus = dwCloseStatus;
        }
    }

    return dwStatus;
}

DWORD USB_ReadWriteTransfer(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum,
    PVOID pBuffer, DWORD dwTransferSize, PBYTE pSetupPacket,
    DWORD *pdwBytesTransferred, BOOL fRead)
{
    DWORD dwStatus =  WDU_Transfer(hDevice, dwPipeNum, fRead, 0, pBuffer,
        dwTransferSize, pdwBytesTransferred, pSetupPacket, TRANSFER_TIMEOUT);

    if (dwStatus)
    {
        ErrLog("Transfer failed. error 0x%x (\"%s\")\n", dwStatus,
            Stat2Str(dwStatus));
    }

    return dwStatus;
}

DWORD USB_ReadWriteStreamCheck(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum,
    PVOID pBuffer, DWORD dwBufferSize, DWORD dwTransferSize,
    DWORD dwIterations, BOOL fRead)
{
    DWORD dwBytesTransferred = 0, dwStatus = WD_STATUS_SUCCESS;
    TIME_TYPE streaming_time_start;
    WDU_STREAM_HANDLE stream;

    dwStatus = WDU_StreamOpen(hDevice, dwPipeNum,
        dwBufferSize, dwTransferSize, TRUE, 0, TRANSFER_TIMEOUT, &stream);
    if (dwStatus)
    {
        ErrLog("WDU_StreamOpen() failed. error 0x%x (\"%s\")\n", dwStatus,
            Stat2Str(dwStatus));
        goto End_perf_test;
    }

    dwStatus = WDU_StreamStart(stream);
    if (dwStatus)
    {
        ErrLog("WDU_StreamStart() failed. error 0x%x (\"%s\")\n", dwStatus,
            Stat2Str(dwStatus));
        goto End_perf_test;
    }

    dwBytesTransferred = 0;
    get_cur_time(&streaming_time_start);
    do {
        DWORD dwBytesTransferredSingle;

        if (fRead)
        {
            dwStatus = WDU_StreamRead(stream, pBuffer,
                dwTransferSize, &dwBytesTransferredSingle);
        }
        else
        {
            dwStatus = WDU_StreamWrite(stream, pBuffer,
                dwTransferSize, &dwBytesTransferredSingle);
        }

        if (dwStatus)
        {
            ErrLog("Transfer failed. error 0x%x (\"%s\")\n", dwStatus,
                Stat2Str(dwStatus));
            goto End_perf_test;
        }

        dwBytesTransferred += dwBytesTransferredSingle;

    } while (dwBytesTransferred < dwTransferSize * dwIterations);

    /* If write command, wait for all the data to be written */
    if (!fRead)
    {
        dwStatus = WDU_StreamFlush(stream);
        if (dwStatus)
        {
            ErrLog("Transfer failed. error 0x%x (\"%s\")\n", dwStatus,
                Stat2Str(dwStatus));
            goto End_perf_test;
        }
    }

    DIAG_PrintPerformance(dwBytesTransferred, &streaming_time_start);

End_perf_test:
    dwStatus = WDU_StreamClose(stream);
    if (dwStatus)
    {
        ErrLog("WDU_StreamClose() failed. error 0x%x (\"%s\")\n", dwStatus,
            Stat2Str(dwStatus));
    }

    return dwStatus;
}

char *USB_PipeType2Str(ULONG pipeType)
{
    char *res = "unknown";

    switch (pipeType)
    {
        case PIPE_TYPE_CONTROL:
            res = "Control";
            break;
        case PIPE_TYPE_ISOCHRONOUS:
            res = "Isochronous";
            break;
        case PIPE_TYPE_BULK:
            res = "Bulk";
            break;
        case PIPE_TYPE_INTERRUPT:
            res = "Interrupt";
            break;
    }

    return res;
}

/* Input of command from user */
static char line[256];

/**  Stops listening to a USB device pipe
 *   @param [in] pParam: pointer to the pipe to which to stop listening.
 * 
 *   @return None.
 */
static void StopListeningToPipe(USB_LISTEN_PIPE *pListenPipe)
{
    if (!pListenPipe->hThread)
        return;

    fprintf(pListenPipe->fpOut, "Stop listening to pipe\n");
    pListenPipe->fStopped = TRUE;

    if (pListenPipe->fStreamMode)
        WDU_StreamClose(pListenPipe->Handle);
    else
        WDU_HaltTransfer(pListenPipe->Handle, pListenPipe->pPipe->dwNumber);

    ThreadWait(pListenPipe->hThread);
    pListenPipe->hThread = NULL;
}

/**  Callback function that listens to a pipe continuously when there is data
 *   available on the pipe
 *   @param [in] pParam: Pointer to the pipe to which to listen.
 *
 *   @return None.
 */
void DLLCALLCONV PipeListenHandler(void *pParam)
{
    USB_LISTEN_PIPE *pListenPipe = (USB_LISTEN_PIPE *)pParam;
    DWORD dwBufsize = pListenPipe->dwBytesToTransfer;
    PVOID buf;
    UINT64 qwTotalBytesTransferred = 0;
    TIME_TYPE streaming_time_start;

    buf = malloc(dwBufsize);
    if (!buf)
    {
        ErrLog("PipeListenHandler: Memory allocation failed\n");
        return;
    }
    if (!pListenPipe->fPrint)
        get_cur_time(&streaming_time_start);

    for (;;)
    {
        DWORD dwError;
        DWORD dwBytesTransferred;

        if (pListenPipe->fStreamMode)
        {
            dwError = WDU_StreamRead(pListenPipe->Handle, buf, dwBufsize,
                &dwBytesTransferred);
        }
        else
        {
            dwError = WDU_Transfer(pListenPipe->Handle,
                pListenPipe->pPipe->dwNumber, TRUE, pListenPipe->dwOptions, buf,
                dwBufsize, &dwBytesTransferred, NULL, TRANSFER_TIMEOUT);
        }
        qwTotalBytesTransferred += (UINT64)dwBytesTransferred;

        if (pListenPipe->fStopped)
            break;

        if (dwError)
        {
            pListenPipe->dwError = dwError;
            fprintf(pListenPipe->fpOut,
                "Listen ended due to an error, press <Enter> to stop.\n");
            break;
        }

        if (pListenPipe->fPrint)
            DIAG_PrintHexBuffer(buf, dwBytesTransferred, TRUE);
    }
    if (!pListenPipe->fPrint)
        DIAG_PrintPerformance(qwTotalBytesTransferred, &streaming_time_start);

    free(buf);
}

/** Starts listening to a USB device pipe
 *   @param [in] hDevice: Handle to the USB device.
 *   @param [in] pListenPipe: Pointer to the pipe for which to listen.
 *
 *   @return  Returns WD_STATUS_SUCCESS (0) on success,
 *     or an appropriate error code otherwise
 */
static DWORD StartListeningToPipe(USB_LISTEN_PIPE *pListenPipe)
{
    /* Start the running thread */
    pListenPipe->fStopped = FALSE;
    fprintf(pListenPipe->fpOut, "Start listening to pipe\n");

    if (pListenPipe->fStreamMode)
    {
        pListenPipe->dwError = WDU_StreamStart(pListenPipe->Handle);
        if (pListenPipe->dwError)
        {
            ErrLog("StartListeningToPipe: WDU_StreamStart() failed. error 0x%x "
                "(\"%s\")\n", pListenPipe->dwError,
                Stat2Str(pListenPipe->dwError));
            goto Exit;
        }
    }

    pListenPipe->dwError = ThreadStart(&pListenPipe->hThread, PipeListenHandler,
        (PVOID)pListenPipe);

Exit:
    return pListenPipe->dwError;
}

DWORD USB_ListenToPipe(HANDLE hDevice, WDU_PIPE_INFO *pPipe,
    BOOL fStreamMode, DWORD dwBufferSize, BOOL fUserKeyWait, FILE *fpIn,
    FILE *fpOut, BOOL fPrint)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;
    USB_LISTEN_PIPE listenPipe;

    BZERO(listenPipe);
    listenPipe.pPipe = pPipe;
    listenPipe.fStreamMode = fStreamMode;
    listenPipe.fPrint = fPrint;
    listenPipe.fpOut = fpOut;
    listenPipe.fpIn = fpIn;

    if (pPipe->type == PIPE_TYPE_ISOCHRONOUS)
    {
        listenPipe.dwBytesToTransfer = pPipe->dwMaximumPacketSize * 8;
        /* 8 minimum packets for high speed transfers */
        listenPipe.dwOptions |= USB_ISOCH_FULL_PACKETS_ONLY;
    }
    else
    {
        listenPipe.dwBytesToTransfer = pPipe->dwMaximumPacketSize * 36;
    }

    if (fStreamMode)
    {
        dwStatus = WDU_StreamOpen(hDevice, pPipe->dwNumber, dwBufferSize,
            listenPipe.dwBytesToTransfer, TRUE, listenPipe.dwOptions,
            TRANSFER_TIMEOUT, &listenPipe.Handle);
        if (dwStatus)
        {
            ErrLog("WDU_StreamOpen() failed. error 0x%x (\"%s\")\n",
                dwStatus, Stat2Str(dwStatus));
            goto Exit;
        }
    }
    else
    {
        listenPipe.Handle = hDevice;
    }

    if (fUserKeyWait)
    {
        fprintf(fpOut,
            "Press <Enter> to start listening. While listening, press "
            "<Enter> to stop\n\n");
        fgetc(fpIn);
    }
    else
    {
        fprintf(fpOut,
            "Listening Started. While listening, press <Enter> to stop\n\n");
    }

    StartListeningToPipe(&listenPipe);
    if (listenPipe.dwError)
    {
        dwStatus = listenPipe.dwError;
        ErrLog("Error listening to pipe 0x%x. error 0x%x (\"%s\")\n",
            pPipe->dwNumber, listenPipe.dwError, Stat2Str(listenPipe.dwError));
        goto Exit;
    }

    while (fgetc(fpIn) != ENTER_KEY); /* Waiting for <Enter> key */

    StopListeningToPipe(&listenPipe);
    if (listenPipe.dwError)
    {
        dwStatus = listenPipe.dwError;
        ErrLog("Transfer failed. error 0x%x (\"%s\")\n", listenPipe.dwError,
            Stat2Str(listenPipe.dwError));
    }

Exit:
    return dwStatus;
}

/** Prints pipe information; (helper function)
 *   @param [in] pPipe: Pointer to the pipe information to print.
 *   @param [in] fp: File pointer to print into(usually stdout).
 *
 *   @return None
 */
static void PrintPipe(const WDU_PIPE_INFO *pPipe, FILE *fp)
{
    fprintf(fp, "  pipe num. 0x%x: packet size %d, type %s, dir %s, "
        "interval %d (ms)\n", pPipe->dwNumber, pPipe->dwMaximumPacketSize,
        USB_PipeType2Str(pPipe->type), pPipe->direction == WDU_DIR_IN ? "In" :
        pPipe->direction == WDU_DIR_OUT ? "Out" : "In & Out",
        pPipe->dwInterval);
}

void USB_PrintPipe0Info(WDU_DEVICE *pDevice, FILE *fp)
{
    fprintf(fp, "Control pipe:\n");
    PrintPipe(&pDevice->Pipe0, fp);
}

void USB_PrintPipesInfo(WDU_ALTERNATE_SETTING *pAltSet, FILE *fp)
{
    BYTE p;
    WDU_PIPE_INFO *pPipe;

    if (!pAltSet->Descriptor.bNumEndpoints)
    {
        fprintf(fp,"  no pipes are defined for this device other than "
            "the default pipe (number 0).\n");
        return;
    }

    fprintf(fp, "Alternate Setting: %d\n",
        pAltSet->Descriptor.bAlternateSetting);
    for (p = 0, pPipe = pAltSet->pPipes; p < pAltSet->Descriptor.bNumEndpoints;
        p++, pPipe++)
    PrintPipe(pPipe, fp);
}

void USB_PrintDevicePipesInfo(WDU_DEVICE *pDevice, FILE *fp)
{
    DWORD k;

    USB_PrintPipe0Info(pDevice, fp);
    /* Iterate over interfaces and print all pipes in their active alternate
     * settings */
    for (k = 0; k < pDevice->pActiveConfig->dwNumInterfaces; k++)
    {
        WDU_INTERFACE *pIfc = pDevice->pActiveInterface[k];

        if (!pIfc)
            break;

        fprintf(fp, "Interface %d\n",
            pIfc->pActiveAltSetting->Descriptor.bInterfaceNumber);
        USB_PrintPipesInfo(pIfc->pActiveAltSetting, fp);
    }
}

DWORD USB_PrintDevicePipesInfoByHandle(HANDLE hDevice, FILE *fp)
{
    WDU_DEVICE *pDevice;
    DWORD dwStatus;

    dwStatus = WDU_GetDeviceInfo(hDevice, &pDevice);
    if (dwStatus)
    {
        ErrLog("USB_PrintDevicePipesInfoByHandle: WDU_GetDeviceInfo() "
            "failed. error 0x%x (\"%s\")\n", dwStatus, Stat2Str(dwStatus));
        goto Exit;
    }

    USB_PrintDevicePipesInfo(pDevice, fp);
    WDU_PutDeviceInfo(pDevice);

Exit:
    return dwStatus;
}

/** Prints the endpoints (pipes) information for the specified alternate
     setting; (helper function for USB_PrintDeviceConfigurations())
 *   @param [in] pAltSet: Pointer to the alternate setting information.
 *   @param [in] fp: file Pointer to print into(usually stdout).
 *
 *   @return None
 */
static void PrintEndpoints(const WDU_ALTERNATE_SETTING *pAltSet, FILE *fp)
{
    BYTE endp;
    WDU_ENDPOINT_DESCRIPTOR *pEndp;

    for (endp = 0; endp < pAltSet->Descriptor.bNumEndpoints; endp++)
    {
        pEndp = &pAltSet->pEndpointDescriptors[endp];
        fprintf(fp,
            "    end-point address: 0x%02x, attributes: 0x%x, max packet %d,"
            " Interval: %d\n", pEndp->bEndpointAddress, pEndp->bmAttributes,
            pEndp->wMaxPacketSize, pEndp->bInterval);
    }
}

DWORD USB_PrintDeviceConfigurations(HANDLE hDevice, FILE *fpIn,
    FILE *fpOut)
{
    DWORD dwStatus;
    WDU_DEVICE *pDevice = NULL;
    DWORD ifc;
    UINT32 iConf;
    WDU_CONFIGURATION *pConf;
    WDU_INTERFACE *pInterface;
    WDU_ALTERNATE_SETTING *pAltSet;

    UNUSED_VAR(fpIn);

    dwStatus = WDU_GetDeviceInfo(hDevice, &pDevice);
    if (dwStatus)
    {
        ErrLog("USB_PrintDeviceConfigurations: WDU_GetDeviceInfo failed. "
            "error 0x%x (\"%s\")\n", dwStatus, Stat2Str(dwStatus));
        goto Exit;
    }

    fprintf(fpOut, "This device has %d configurations:\n",
        pDevice->Descriptor.bNumConfigurations);
    for (iConf = 0; iConf < pDevice->Descriptor.bNumConfigurations; iConf++)
    {
        fprintf(fpOut, "  %d. Configuration value %d (has %d interfaces)\n",
            iConf, pDevice->pConfigs[iConf].Descriptor.bConfigurationValue,
            pDevice->pConfigs[iConf].dwNumInterfaces);
    }
    iConf = 0;

    if (pDevice->Descriptor.bNumConfigurations > 1)
    {
        sprintf(line, "Please enter the configuration index to display "
            "(dec - zero based):");
        if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(&iConf, line, FALSE, 0, 0))
        {
            dwStatus = WD_INVALID_PARAMETER;
            goto Exit;
        }

        if (iConf >= pDevice->Descriptor.bNumConfigurations)
        {
            ErrLog("ERROR: Invalid configuration index, valid values are "
                "0-%d\n", pDevice->Descriptor.bNumConfigurations);
            dwStatus = WD_INVALID_PARAMETER;
            goto Exit;
        }
    }
    pConf = &pDevice->pConfigs[iConf];

    fprintf(fpOut, "The configuration indexed %d has %d interface(s):\n",
        iConf, pConf->dwNumInterfaces);

    for (ifc = 0; ifc < pConf->dwNumInterfaces; ifc++)
    {
        DWORD alt;

        pInterface = &pConf->pInterfaces[ifc];
        fprintf(fpOut, "interface no. %d has %d alternate settings:\n",
            pInterface->pAlternateSettings[0].Descriptor.bInterfaceNumber,
            pInterface->dwNumAltSettings);
        for (alt = 0; alt < pInterface->dwNumAltSettings; alt++)
        {
            pAltSet = &pInterface->pAlternateSettings[alt];

            fprintf(fpOut, "  alternate: %d, endpoints: %d, class: 0x%x, "
                "subclass: 0x%x, protocol: 0x%x\n",
                pAltSet->Descriptor.bAlternateSetting,
                pAltSet->Descriptor.bNumEndpoints,
                pAltSet->Descriptor.bInterfaceClass,
                pAltSet->Descriptor.bInterfaceSubClass,
                pAltSet->Descriptor.bInterfaceProtocol);

            PrintEndpoints(pAltSet, fpOut);
        }
        fprintf(fpOut, "\n");
    }
    fprintf(fpOut, "\n");

Exit:
    if (pDevice)
        WDU_PutDeviceInfo(pDevice);

    return dwStatus;
}

WDU_PIPE_INFO *USB_FindPipeInDevice(WDU_DEVICE *pDevice,
    DWORD dwPipeNumber)
{
    DWORD i;

    if (dwPipeNumber == 0)
        return &pDevice->Pipe0;

    for (i = 0; i < pDevice->pActiveConfig->dwNumInterfaces; i++)
    {
        DWORD j;
        WDU_INTERFACE *pIfc = pDevice->pActiveInterface[i];
        WDU_ALTERNATE_SETTING *pAltSet;

        if (!pIfc)
            break;

        pAltSet = pIfc->pActiveAltSetting;

        for (j = 0; j < pAltSet->Descriptor.bNumEndpoints; j++)
        {
            if (pAltSet->pPipes[j].dwNumber == dwPipeNumber)
                return &pAltSet->pPipes[j];
        }
    }

    return NULL;
}

DWORD USB_PrintDeviceSerialNumberByHandle(HANDLE hDevice, FILE *fp)
{
    WDU_DEVICE *pDevice;
    DWORD dwStatus;
    BYTE bSerialNum[0x100];
    DWORD dwSerialDescSize = 0;
    DWORD i;

    dwStatus = WDU_GetDeviceInfo(hDevice, &pDevice);
    if (dwStatus)
    {
        ErrLog("WDU_GetDeviceInfo() failed. error 0x%x (\"%s\")\n",
            dwStatus, Stat2Str(dwStatus));
        goto Exit;
    }

    if (!pDevice->Descriptor.iSerialNumber)
    {
        fprintf(fp, "Serial number is not available\n");
        goto Exit;
    }

    fprintf(fp, "Serial number string descriptor index: [%d]\n",
        pDevice->Descriptor.iSerialNumber);

    dwStatus = WDU_GetStringDesc(hDevice, pDevice->Descriptor.iSerialNumber,
        bSerialNum, sizeof(bSerialNum), 0, &dwSerialDescSize);
    if (dwStatus)
    {
        ErrLog("WDU_GetStringDesc() failed. error 0x%x (\"%s\")\n", dwStatus,
            Stat2Str(dwStatus));
        goto Exit;
    }

    fprintf(fp, "Serial number: length [%d], [", dwSerialDescSize);
    for (i = 0; i < dwSerialDescSize; i++)
        fprintf(fp,"%c", bSerialNum[i]);
    fprintf(fp, "]\n");

Exit:
    WDU_PutDeviceInfo(pDevice);
    
    return dwStatus;
}

#define PROPERTY_LENGTH 256
#define PROPERTY_NAME_LENGTH 46
#define NUM_PROPERTIES 20
void USB_PrintDeviceProperties(HANDLE hDevice, FILE *fp)
{
    DWORD dwStatus = 0;
    DWORD dwSize, i;
#ifdef WIN32
    WCHAR cProperty[PROPERTY_LENGTH];
#else
    CHAR cProperty[PROPERTY_LENGTH];
#endif
    CHAR propertyNames[NUM_PROPERTIES][PROPERTY_NAME_LENGTH] =
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

    for (i = 0; i < NUM_PROPERTIES; i++)
    {
        dwSize = PROPERTY_LENGTH;
        BZERO(cProperty);
        dwStatus = WDU_GetDeviceRegistryProperty(hDevice, cProperty, &dwSize,
            (WD_DEVICE_REGISTRY_PROPERTY)i);
        if (WD_STATUS_SUCCESS == dwStatus)
        {
            fprintf(fp, "%-46s: ", propertyNames[i]);
            if (i > WdDevicePropertyPhysicalDeviceObjectName &&
                i != WdDevicePropertyEnumeratorName)
            {
                fprintf(fp, "%d\n", *cProperty);
            }
            else
            {
#ifdef WIN32
                fprintf(fp, "%ws\n", cProperty);
#else
                fprintf(fp, "%s\n", cProperty);
#endif
            }
        }
    }
}

/* -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- */
   /* Log a debug error message */
static void ErrLog(const CHAR *sFormat, ...)
{
    va_list argp;

    va_start(argp, sFormat);
    vsnprintf(gsUSB_LastErr, sizeof(gsUSB_LastErr) - 1, sFormat, argp);
    va_end(argp);
}

/* Log a debug trace message */
static void TraceLog(const CHAR *sFormat, ...)
{
#ifdef DEBUG
    CHAR sMsg[256];
    va_list argp;

    va_start(argp, sFormat);
    vsnprintf(sMsg, sizeof(sMsg) - 1, sFormat, argp);

    va_end(argp);
#else
    UNUSED_VAR(sFormat);
#endif
}

/* Get last error */
const char *USB_GetLastErr(void)
{
    return gsUSB_LastErr;
}
