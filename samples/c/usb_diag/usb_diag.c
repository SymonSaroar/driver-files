/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/******************************************************************
 * This is a diagnostics application for accessing the USB device.
 * The code accesses the hardware via WinDriver functions.
 *
 * Note: This code sample is provided AS-IS and as a guiding sample only.
 *
 ******************************************************************/

#include "wdu_lib.h"
#include "status_strings.h"
#include "utils.h"
#include "usb_diag_lib.h"

#if defined(USB_DIAG_SAMPLE)
    /* TODO: Change the following definitions to match your device. */
    #define DEFAULT_VENDOR_ID 0x1234
    #define DEFAULT_PRODUCT_ID 0x5678

    /* WinDriver license registration string */
    /* TODO: When using a registered WinDriver version, replace the license
             string below with the development license in order to use on the
             development machine.
             Once you require to distribute the driver's package to other
             machines, please replace the string with a distribution license */
    #define DEFAULT_LICENSE_STRING "12345abcde1234.license"
#else
    /* Use in wizard's device-specific generated code */
    #define DEFAULT_VENDOR_ID         %VID%
    #define DEFAULT_PRODUCT_ID        %PID%
    #define DEFAULT_LICENSE_STRING    "%LICENSE%"
#endif

/* TODO: Change the following definition to your driver's name */
#define DEFAULT_DRIVER_NAME WD_DEFAULT_DRIVER_NAME_BASE

#define USE_DEFAULT 0xffff
#define ATTACH_EVENT_TIMEOUT 30 /* in seconds */

#if !defined(TRACE)
    #define TRACE printf
#endif
#if !defined(ERR)
    #define ERR printf
#endif


typedef struct DEVICE_CONTEXT
{
    struct DEVICE_CONTEXT *pNext;
    WDU_DEVICE_HANDLE hDevice;
    DWORD dwVendorId;
    DWORD dwProductId;
    DWORD dwInterfaceNum;
    DWORD dwAlternateSetting;
} DEVICE_CONTEXT;

typedef struct DRIVER_CONTEXT
{
    HANDLE hEvent;
    HANDLE hMutex;
    DWORD dwDeviceCount;
    DEVICE_CONTEXT *deviceContextList;
    DEVICE_CONTEXT *pActiveDev;
    HANDLE hDeviceUnusedEvent;
} DRIVER_CONTEXT;

typedef struct MENU_CTX_USB
{
    DRIVER_CONTEXT *pDrvCtx;
    WDU_DEVICE_HANDLE *phDevice;
    WDU_DEVICE *pDevice;
    BOOL fStreamMode;
    BOOL fSuspended;
} MENU_CTX_USB;

static char line[250];
static WDU_DRIVER_HANDLE hDriver = 0;


static BOOL DLLCALLCONV DeviceAttach(WDU_DEVICE_HANDLE hDevice,
    WDU_DEVICE *pDeviceInfo, PVOID pUserData)
{
    DRIVER_CONTEXT *pDrvCtx = (DRIVER_CONTEXT *)pUserData;
    DEVICE_CONTEXT *pDevCtx, **ppDevCtx;
    WDU_ALTERNATE_SETTING *pActiveAltSetting =
        pDeviceInfo->pActiveInterface[0]->pActiveAltSetting;
    DWORD dwInterfaceNum = pActiveAltSetting->Descriptor.bInterfaceNumber;
    DWORD dwAlternateSetting = pActiveAltSetting->Descriptor.bAlternateSetting;

    /*
    // NOTE: To change the alternate setting, call WDU_SetInterface() here
    DWORD dwAttachError;

    // TODO: Replace with the requested number:
    dwAlternateSetting = %alternate_setting_number%;

    dwAttachError = WDU_SetInterface(hDevice, dwInterfaceNum,
        dwAlternateSetting);
    if (dwAttachError)
    {
        ERR("DeviceAttach: WDU_SetInterface() failed (num. %d, alternate %d) "
            "device 0x%p. error 0x%x (\"%s\")\n", dwInterfaceNum,
            dwAlternateSetting, hDevice, dwAttachError,
            Stat2Str(dwAttachError));

        return FALSE;
    }
    */

    /* Uncomment the following code to allow only one device per process */
    /*
    BOOL hasDevice;
    OsMutexLock(pDrvCtx->hMutex);
    hasDevice = pDrvCtx->dwDeviceCount > 0;
    OsMutexUnlock(pDrvCtx->hMutex);
    if (hasDevice)
    {
        TRACE("DeviceAttach: This process already has one device,"
            " giving this one up\n");
        return FALSE;
    }
    */

    TRACE("\nDeviceAttach: Received and accepted attach for vendor id 0x%x, "
        "product id 0x%x, interface %d, device handle 0x%p\n",
        pDeviceInfo->Descriptor.idVendor, pDeviceInfo->Descriptor.idProduct,
        dwInterfaceNum, hDevice);

    /* Add our device to the device list */
    pDevCtx = (DEVICE_CONTEXT *)malloc(sizeof(DEVICE_CONTEXT));
    if (!pDevCtx)
    {
        ERR("DeviceAttach: Failed allocating memory\n");
        return FALSE;
    }

    BZERO(*pDevCtx);
    pDevCtx->hDevice = hDevice;
    pDevCtx->dwInterfaceNum = dwInterfaceNum;
    pDevCtx->dwVendorId = pDeviceInfo->Descriptor.idVendor;
    pDevCtx->dwProductId = pDeviceInfo->Descriptor.idProduct;
    pDevCtx->dwAlternateSetting = dwAlternateSetting;

    OsMutexLock(pDrvCtx->hMutex);
    for (ppDevCtx = &pDrvCtx->deviceContextList; *ppDevCtx;
        ppDevCtx = &((*ppDevCtx)->pNext));
    *ppDevCtx = pDevCtx;
    pDrvCtx->dwDeviceCount++;
    OsMutexUnlock(pDrvCtx->hMutex);

    OsEventSignal(pDrvCtx->hEvent);

    /* Accept control over this device */
    return TRUE;
}

static VOID DLLCALLCONV DeviceDetach(WDU_DEVICE_HANDLE hDevice, PVOID pUserData)
{
    DRIVER_CONTEXT *pDrvCtx = (DRIVER_CONTEXT *)pUserData;
    DEVICE_CONTEXT **pCur;
    DEVICE_CONTEXT *pTmpDev;
    BOOL bDetachActiveDev = FALSE;

    TRACE("\nDeviceDetach: Received detach for device handle 0x%p\n", hDevice);

    OsMutexLock(pDrvCtx->hMutex);
    for (pCur = &pDrvCtx->deviceContextList;
        *pCur && (*pCur)->hDevice != hDevice;
        pCur = &((*pCur)->pNext));

    if (*pCur == pDrvCtx->pActiveDev)
    {
        bDetachActiveDev = TRUE;
        pDrvCtx->pActiveDev = NULL;
    }

    pTmpDev = *pCur;
    *pCur = pTmpDev->pNext;
    free(pTmpDev);

    pDrvCtx->dwDeviceCount--;
    OsMutexUnlock(pDrvCtx->hMutex);

    if (bDetachActiveDev)
    {
        /* When hDeviceUnusedEvent is not signaled, hDevice is possibly in use,
         * and therefore the detach callback needs to wait on it until it is
         * certain that it cannot be used.
         * When it is signaled - hDevice is no longer used. */
        OsEventWait(pDrvCtx->hDeviceUnusedEvent, INFINITE);
    }
}

static DWORD GetDevice(DRIVER_CONTEXT *pDrvCtx, WDU_DEVICE_HANDLE *phDevice)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;
    if (!pDrvCtx->pActiveDev)
    {
        dwStatus = WD_WINDRIVER_STATUS_ERROR;
        ERR("%s: Could not get active device\n", __FUNCTION__);
        goto Exit;
    }

    OsEventReset(pDrvCtx->hDeviceUnusedEvent);

    OsMutexLock(pDrvCtx->hMutex);
    *phDevice = pDrvCtx->pActiveDev->hDevice;
    OsMutexUnlock(pDrvCtx->hMutex);

Exit:
    return dwStatus;
}

static DWORD GetInterfaceAndAltSettings(DWORD *pdwInterfaceNumber,
    DWORD *pdwAlternateSetting)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;

    sprintf(line, "Please enter the interface number (dec): ");
    if (DIAG_INPUT_SUCCESS !=
        DIAG_InputDWORD(pdwInterfaceNumber, line, FALSE, 0, 0))
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }


    sprintf(line, "Please enter the alternate setting index (dec): ");
    if (DIAG_INPUT_SUCCESS !=
        DIAG_InputDWORD(pdwAlternateSetting, line, FALSE, 0, 0))
    {
        dwStatus = WD_INVALID_PARAMETER;
    }

Exit:
    return dwStatus;
}

static DWORD DriverInit(WDU_MATCH_TABLE *pMatchTables, DWORD dwNumMatchTables,
    const PCHAR sDriverName, const PCHAR sLicense, DRIVER_CONTEXT *pDrvCtx)
{
    DWORD dwError;
    WDU_EVENT_TABLE eventTable;

    /* Set Driver Name */
    if (!WD_DriverName(sDriverName))
    {
        ERR("DriverInit: Failed setting driver name to %s, exiting\n",
            sDriverName);
        return WD_SYSTEM_INTERNAL_ERROR;
    }

    dwError = OsEventCreate(&pDrvCtx->hEvent);
    if (dwError)
    {
        ERR("DriverInit: OsEventCreate() failed on event 0x%p. error 0x%x "
            "(\"%s\")\n", pDrvCtx->hEvent, dwError, Stat2Str(dwError));
        return dwError;
    }

    dwError = OsMutexCreate(&pDrvCtx->hMutex);
    if (dwError)
    {
        ERR("DriverInit: OsMutexCreate() failed on mutex 0x%p. error 0x%x "
            "(\"%s\")\n", pDrvCtx->hMutex, dwError, Stat2Str(dwError));
        return dwError;
    }

    dwError = OsEventCreate(&pDrvCtx->hDeviceUnusedEvent);
    if (dwError)
    {
        ERR("DriverInit: OsEventCreate() failed on event 0x%p. error 0x%x "
            "(\"%s\")\n", pDrvCtx->hDeviceUnusedEvent, dwError,
            Stat2Str(dwError));
        return dwError;
    }

    OsEventSignal(pDrvCtx->hDeviceUnusedEvent);
    BZERO(eventTable);
    eventTable.pfDeviceAttach = DeviceAttach;
    eventTable.pfDeviceDetach = DeviceDetach;
    eventTable.pUserData = pDrvCtx;

    dwError = WDU_Init(&hDriver, pMatchTables, dwNumMatchTables, &eventTable,
        sLicense, WD_ACKNOWLEDGE);
    if (dwError)
    {
        ERR("DriverInit: Failed to initialize USB driver. error 0x%x "
            "(\"%s\")\n", dwError, Stat2Str(dwError));
        return dwError;
    }

    return WD_STATUS_SUCCESS;
}

static VOID DriverUninit(DRIVER_CONTEXT *pDrvCtx)
{
    DEVICE_CONTEXT *pCur, *pTmpDev;

    if (pDrvCtx->hEvent)
        OsEventClose(pDrvCtx->hEvent);
    if (pDrvCtx->hMutex)
        OsMutexClose(pDrvCtx->hMutex);
    if (pDrvCtx->hDeviceUnusedEvent)
        OsEventClose(pDrvCtx->hDeviceUnusedEvent);
    if (hDriver)
        WDU_Uninit(hDriver);

    /* Release any remaining devices */
    pCur = pDrvCtx->deviceContextList;
    while (pCur)
    {
        pTmpDev = pCur;
        pCur = pCur->pNext;
        free(pTmpDev);
    }
}

static DWORD USB_Init(DRIVER_CONTEXT *pDrvCtx, WDU_MATCH_TABLE *pMatchTable)
{
    DWORD dwStatus;
    WORD wVendorId = USE_DEFAULT;
    WORD wProductId = USE_DEFAULT;

    PrintDbgMessage(D_ERROR, S_USB, "WinDriver user mode version %s\n",
        WD_VERSION_STR);

#if defined(USB_DIAG_SAMPLE)
    TRACE("Enter device vendor id (hex) (=0x%x):\n", DEFAULT_VENDOR_ID);
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(&wVendorId, line, TRUE, 0, 0))
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    TRACE("Enter device product id (hex) (=0x%x):\n", DEFAULT_PRODUCT_ID);
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(&wProductId, line, TRUE, 0, 0))
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }
#endif

    /* Use defaults */
    if (wVendorId == USE_DEFAULT)
        wVendorId = DEFAULT_VENDOR_ID;
    if (wProductId == USE_DEFAULT)
        wProductId = DEFAULT_PRODUCT_ID;

    BZERO(*pMatchTable);
    pMatchTable->wVendorId = wVendorId;
    pMatchTable->wProductId = wProductId;

    BZERO(*pDrvCtx);
    dwStatus = DriverInit(pMatchTable, 1, DEFAULT_DRIVER_NAME,
        DEFAULT_LICENSE_STRING, pDrvCtx);
    if (dwStatus)
        goto Exit;

    TRACE("Please make sure the device is attached:\n");
    TRACE("(The application is waiting for device attachment...)\n");

    /* Wait for the device to be attached */
    dwStatus = OsEventWait(pDrvCtx->hEvent, ATTACH_EVENT_TIMEOUT);
    if (dwStatus)
    {
        if (dwStatus == WD_TIME_OUT_EXPIRED)
        {
            ERR("Timeout expired for connection with the device.\n"
                "Check that the device is connected and try again.\n");
        }
        else
        {
            ERR("USB_Init: OsEventWait() failed on event 0x%p. error 0x%x "
                "(\"%s\")\n", pDrvCtx->hEvent, dwStatus, Stat2Str(dwStatus));
        }
        goto Exit;
    }

Exit:
    return dwStatus;
}

#if defined(WIN32)
static DWORD MenuUsbEntryCb(PVOID pCbCtx)
{
    return GetDevice(((MENU_CTX_USB *)pCbCtx)->pDrvCtx,
        ((MENU_CTX_USB *)pCbCtx)->phDevice);
}
#endif

static DWORD MenuUsbExitCb(PVOID pCbCtx)
{
    return OsEventSignal(((MENU_CTX_USB *)pCbCtx)->
        pDrvCtx->hDeviceUnusedEvent);
}

/* ----------------------------*/
/* Print Device Configurations */
/* ----------------------------*/
static DWORD MenuPrintDeviceCfgsOptionCb(PVOID pCbCtx)
{
    DWORD dwStatus = GetDevice(((MENU_CTX_USB *)pCbCtx)->pDrvCtx,
        ((MENU_CTX_USB *)pCbCtx)->phDevice);

    if (dwStatus)
        goto Exit;

    dwStatus = USB_PrintDeviceConfigurations(
        *((MENU_CTX_USB *)pCbCtx)->phDevice, stdin, stdout);

    if (dwStatus)
        ERR("%s: %s", __FUNCTION__, USB_GetLastErr());
Exit:
    return dwStatus;
}


/* -----------------------------------*/
/* Change interface alternate setting */
/* -----------------------------------*/
static DWORD MenuChangeInterfaceAltSettingOptionCb(PVOID pCbCtx)
{
    DWORD dwInterfaceNumber, dwAlternateSetting, dwStatus;
    DRIVER_CONTEXT *pDrvCtx = ((MENU_CTX_USB *)pCbCtx)->pDrvCtx;
    WDU_DEVICE_HANDLE *phDevice = ((MENU_CTX_USB *)pCbCtx)->phDevice;

    dwStatus = GetDevice(pDrvCtx, phDevice);
    if (dwStatus)
        goto Exit;

    dwStatus = GetInterfaceAndAltSettings(&dwInterfaceNumber,
        &dwAlternateSetting);
    if (dwStatus)
        goto Exit;

    dwStatus = USB_SetInterface(*phDevice, dwInterfaceNumber,
        dwAlternateSetting);
    if (dwStatus)
    {
        goto Error;
    }
    else
    {
        TRACE("MenuChangeInterfaceAltSettingOptionCb: WDU_SetInterface() "
            "completed successfully\n");

        pDrvCtx->pActiveDev->dwInterfaceNum = dwInterfaceNumber;
        pDrvCtx->pActiveDev->dwAlternateSetting = dwAlternateSetting;
        goto Exit;
    }

Error:
    ERR("%s: %s", __FUNCTION__, USB_GetLastErr());
Exit:
    return dwStatus;
}

/* ------------*/
/* Reset Pipe */
/* ------------*/
static DWORD MenuResetPipeOptionCb(PVOID pCbCtx)
{
    DWORD dwPipeNum, dwStatus = WD_STATUS_SUCCESS;
    DRIVER_CONTEXT *pDrvCtx = ((MENU_CTX_USB *)pCbCtx)->pDrvCtx;
    WDU_DEVICE_HANDLE *phDevice = ((MENU_CTX_USB *)pCbCtx)->phDevice;

    dwStatus = GetDevice(pDrvCtx, phDevice);
    if (dwStatus)
        goto Exit;

    dwStatus = USB_PrintDevicePipesInfoByHandle(*phDevice, stdout);
    if (dwStatus)
        goto Error;

    sprintf(line, "Please enter the pipe number (hex): 0x");
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(&dwPipeNum, line, TRUE, 0, 0))
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    TRACE("\n");

    dwStatus = USB_ResetPipe(*phDevice, dwPipeNum);
    if (dwStatus)
        goto Error;

    TRACE("MenuResetPipeOptionCb: WDU_ResetPipe() completed "
        "successfully\n");
    goto Exit;

Error:
    ERR("%s: %s", __FUNCTION__, USB_GetLastErr());
Exit:
    return dwStatus;
}

/* ---------------*/
/* Read/Write Pipe */
/* ---------------*/
static DWORD MenuRwPipeGetPipeNum(WDU_PIPE_INFO **ppPipe, WDU_DEVICE *pDevice,
    DWORD *pdwPipeNum)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;

    sprintf(line, "Please enter the pipe number (hex): 0x");
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(pdwPipeNum, line, TRUE, 0, 0))
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    /* Search for the pipe */
    *ppPipe = USB_FindPipeInDevice(pDevice, *pdwPipeNum);
    if (!*ppPipe)
    {
        ERR("The pipe number 0x%x does not exist. Please try again.\n",
            *pdwPipeNum);
        dwStatus = WD_INVALID_PARAMETER;
    }

Exit:
    return dwStatus;
}

/* Read/Write pipe */
static DWORD MenuRwPipeReadWrite(MENU_CTX_USB *pMenuCtx, BOOL fRead)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;
    DWORD dwSize, dwBytesTransferred, dwPipeNum = 0, dwBufferSize = 0x20000;
    PVOID pBuffer = NULL;
    WDU_PIPE_INFO *pPipe;
    BYTE SetupPacket[8];

    dwStatus = MenuRwPipeGetPipeNum(&pPipe, pMenuCtx->pDevice, &dwPipeNum);
    if (dwStatus)
        goto End_transfer;

    if (!dwPipeNum || pPipe->type == PIPE_TYPE_CONTROL)
    {
        if (pMenuCtx->fStreamMode)
        {
            ERR("Cannot perform stream transfer using control pipe.\n"
                "please switch to Single Blocking Transfer mode "
                "or change the pipe number\n");
            dwStatus = WD_INVALID_PARAMETER;
            goto End_transfer;
        }

        TRACE("Please enter setup packet (hex - 8 bytes): ");
        DIAG_GetHexBuffer((PVOID)SetupPacket, 8);
    }

    sprintf(line, "Please enter the size of the buffer (dec): ");
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(&dwSize, line, FALSE, 0, 0))
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto End_transfer;
    }

    if (dwSize)
    {
        pBuffer = malloc(dwSize);
        if (!pBuffer)
        {
            ERR("Cannot alloc memory\n");
            dwStatus = WD_INSUFFICIENT_RESOURCES;
            goto End_transfer;
        }

        if (!fRead)
        {
            TRACE("Please enter the input buffer (hex): ");
            DIAG_GetHexBuffer(pBuffer, dwSize);
        }
    }

    if (pMenuCtx->fStreamMode)
    {
        dwStatus = USB_ReadWriteStream(*(pMenuCtx->phDevice), dwPipeNum,
            pBuffer, dwBufferSize, dwSize, &dwBytesTransferred, fRead);
    }
    else
    {
        dwStatus = USB_ReadWriteTransfer(*(pMenuCtx->phDevice), dwPipeNum,
            pBuffer, dwSize, SetupPacket, &dwBytesTransferred, fRead);
    }

    if (dwStatus)
    {
        ERR("%s: %s", __FUNCTION__, USB_GetLastErr());
    }
    else
    {
        TRACE("Transferred %d bytes\n", dwBytesTransferred);
        if (fRead && pBuffer)
            DIAG_PrintHexBuffer(pBuffer, dwBytesTransferred, TRUE);
    }

End_transfer:
    if (pBuffer)
        free(pBuffer);

    return dwStatus;

}

static DWORD MenuRwPipeReadOptionCb(PVOID pCbCtx)
{
    return MenuRwPipeReadWrite((MENU_CTX_USB *)pCbCtx, TRUE);
}

static DWORD MenuRwPipeWriteOptionCb(PVOID pCbCtx)
{
    return MenuRwPipeReadWrite((MENU_CTX_USB *)pCbCtx, FALSE);
}

/* Listen/Measure Pipe */
static DWORD MenuRwPipeListenMeasure(MENU_CTX_USB *pMenuCtx,
    BOOL fListen)
{
    DWORD dwStatus = WD_STATUS_SUCCESS, dwPipeNum = 0, dwBufferSize = 0x20000;
    WDU_PIPE_INFO *pPipe;

    dwStatus = MenuRwPipeGetPipeNum(&pPipe, pMenuCtx->pDevice, &dwPipeNum);
    if (dwStatus)
        goto Exit;

    if (!dwPipeNum || pPipe->type == PIPE_TYPE_CONTROL)
    {
        TRACE("Cannot listen to control pipes.\n");
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    dwStatus = USB_ListenToPipe(*(pMenuCtx->phDevice), pPipe,
        pMenuCtx->fStreamMode, dwBufferSize, TRUE, stdin, stdout, fListen);

    if (dwStatus)
        ERR("%s: %s", __FUNCTION__, USB_GetLastErr());
Exit:
    return dwStatus;
}

static DWORD MenuRwPipeListenOptionCb(PVOID pCbCtx)
{
    return MenuRwPipeListenMeasure((MENU_CTX_USB *)pCbCtx, TRUE);
}

static DWORD MenuRwPipeMeasureOptionCb(PVOID pCbCtx)
{
    return MenuRwPipeListenMeasure((MENU_CTX_USB *)pCbCtx, FALSE);
}

#if defined(WIN32)
#define PERF_STREAM_BUFFER_SIZE 5120000 /* In bytes */
#define PERF_DEVICE_TRANSFER_SIZE 256*1024 /* In bytes */
#define PERF_PERFORMANCE_SAMPLE_TIME 10000 /* In msecs */
#define PERF_TRANSFER_ITERATIONS 1500

/* Check Read/Write Stream */
static BOOL MenuRwPipeIsStreamingNotActivated(DIAG_MENU_OPTION *pMenu)
{
    return !((MENU_CTX_USB *)pMenu->pCbCtx)->fStreamMode;
}

static DWORD MenuRwPipeCheckStreamReadWrite(MENU_CTX_USB *pMenuCtx, BOOL fRead)
{
    DWORD  dwPipeNum = 0, dwStatus = WD_STATUS_SUCCESS;
    PVOID pBuffer = NULL;
    WDU_PIPE_INFO *pPipe;

    dwStatus = MenuRwPipeGetPipeNum(&pPipe, pMenuCtx->pDevice, &dwPipeNum);
    if (dwStatus)
        goto End_perf_test;

    if (!dwPipeNum || pPipe->type == PIPE_TYPE_CONTROL)
    {
        TRACE("Cannot perform stream transfer with control pipe\n");
        dwStatus = WD_INVALID_PARAMETER;
        goto End_perf_test;
    }

    TRACE("The size of the buffer to transfer(dec): %d\n",
        PERF_DEVICE_TRANSFER_SIZE);
    TRACE("The size of the internal Rx/Tx stream buffer (dec): %d\n",
        PERF_STREAM_BUFFER_SIZE);
    TRACE("Making the transfer of %d times the buffer size, please "
        "wait ...\n", PERF_TRANSFER_ITERATIONS);

    pBuffer = malloc(PERF_DEVICE_TRANSFER_SIZE);
    if (!pBuffer)
    {
        ERR("Failed allocating memory\n");
        dwStatus = WD_INSUFFICIENT_RESOURCES;
        goto End_perf_test;
    }

    if (!fRead)
    {
        /* Here you can fill pBuffer with the right data for the
         * board */
    }

    dwStatus = USB_ReadWriteStreamCheck(*(pMenuCtx->phDevice), dwPipeNum,
        pBuffer, PERF_STREAM_BUFFER_SIZE, PERF_DEVICE_TRANSFER_SIZE,
        PERF_TRANSFER_ITERATIONS, fRead);

    if (dwStatus)
        ERR("%s: %s", __FUNCTION__, USB_GetLastErr());

End_perf_test:
    if (pBuffer)
        free(pBuffer);

    return dwStatus;
}

static DWORD MenuRwPipeCheckStreamReadOptionCb(PVOID pCbCtx)
{
    return MenuRwPipeCheckStreamReadWrite((MENU_CTX_USB *)pCbCtx, TRUE);
}

static DWORD MenuRwPipeCheckStreamWriteOptionCb(PVOID pCbCtx)
{
    return MenuRwPipeCheckStreamReadWrite((MENU_CTX_USB *)pCbCtx, FALSE);
}

static DWORD MenuRwPipeSwitchTransferTypeOptionCb(PVOID pCbCtx)
{
    MENU_CTX_USB *pMenuCtx = (MENU_CTX_USB *)pCbCtx;

    pMenuCtx->fStreamMode = !pMenuCtx->fStreamMode;
    return WD_STATUS_SUCCESS;
}
#endif

static void MenuRwPipeOptionsInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_USB *pMenuCtx)
{
    static DIAG_MENU_OPTION readPipeMenu = { 0 };
    static DIAG_MENU_OPTION writePipeMenu = { 0 };
    static DIAG_MENU_OPTION listenPipeMenu = { 0 };
    static DIAG_MENU_OPTION measurePipeMenu = { 0 };
#if defined(WIN32)
    static DIAG_MENU_OPTION options[7] = { 0 };
    static DIAG_MENU_OPTION checkStreamingReadSpeedMenu = { 0 };
    static DIAG_MENU_OPTION checkStreamingWriteSpeedMenu = { 0 };
    static DIAG_MENU_OPTION switchTransferModeMenu = { 0 };


#else
    static DIAG_MENU_OPTION options[4] = { 0 };
#endif

    strcpy(readPipeMenu.cOptionName, "Read from pipe");
    readPipeMenu.cbEntry = MenuRwPipeReadOptionCb;
    readPipeMenu.cbExit = MenuUsbExitCb;

    strcpy(writePipeMenu.cOptionName, "Write to pipe");
    writePipeMenu.cbEntry = MenuRwPipeWriteOptionCb;
    writePipeMenu.cbExit = MenuUsbExitCb;

    strcpy(listenPipeMenu.cOptionName, "Listen to pipe (continuous read)");
    listenPipeMenu.cbEntry = MenuRwPipeListenOptionCb;
    listenPipeMenu.cbExit = MenuUsbExitCb;

    strcpy(measurePipeMenu.cOptionName, "Measure pipe speed (continuous read)");
    measurePipeMenu.cbEntry = MenuRwPipeMeasureOptionCb;
    measurePipeMenu.cbExit = MenuUsbExitCb;

#if defined(WIN32)
    strcpy(checkStreamingReadSpeedMenu.cOptionName, "Check streaming READ "
        "speed");
    checkStreamingReadSpeedMenu.cbEntry = MenuRwPipeCheckStreamReadOptionCb;
    checkStreamingReadSpeedMenu.cbIsHidden = MenuRwPipeIsStreamingNotActivated;
    checkStreamingReadSpeedMenu.cbExit = MenuUsbExitCb;

    strcpy(checkStreamingWriteSpeedMenu.cOptionName, "Check streaming WRITE "
        "speed");
    checkStreamingWriteSpeedMenu.cbEntry = MenuRwPipeCheckStreamWriteOptionCb;
    checkStreamingWriteSpeedMenu.cbIsHidden = MenuRwPipeIsStreamingNotActivated;
    checkStreamingWriteSpeedMenu.cbExit = MenuUsbExitCb;

    strcpy(switchTransferModeMenu.cOptionName, "Switch transfer mode");
    switchTransferModeMenu.cbEntry = MenuRwPipeSwitchTransferTypeOptionCb;
    switchTransferModeMenu.cbExit = MenuUsbExitCb;
#else
#endif

    options[0] = readPipeMenu;
    options[1] = writePipeMenu;
    options[2] = listenPipeMenu;
    options[3] = measurePipeMenu;

#if defined(WIN32)
    options[4] = checkStreamingReadSpeedMenu;
    options[5] = checkStreamingWriteSpeedMenu;
    options[6] = switchTransferModeMenu;
#endif

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        pMenuCtx, pParentMenu);
}

static DWORD MenuRwPipeEntryCb(PVOID pCbCtx)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;
    MENU_CTX_USB *pMenuCtx = (MENU_CTX_USB *)pCbCtx;

    dwStatus = GetDevice(pMenuCtx->pDrvCtx, pMenuCtx->phDevice);
    if (dwStatus)
        goto Exit;

    if (!pMenuCtx->pDevice)
    {
        dwStatus = USB_GetDeviceInfo(*(pMenuCtx->phDevice),
            &(pMenuCtx->pDevice));
        if (dwStatus)
        {
            ERR("%s: %s", __FUNCTION__, USB_GetLastErr());
            goto Exit;
        }


        USB_PrintDevicePipesInfo(pMenuCtx->pDevice, stdout);
    }

    TRACE("\n");
    TRACE("Read/Write from/to device's pipes using %s\n",
        pMenuCtx->fStreamMode ?
        "Streaming Data Transfers" : "Single Blocking Transfers");
    TRACE("---------------------\n");

Exit:
    return dwStatus;
}

#if defined(WIN32)
/* -------------------*/
/* Fast streaming Read */
/* -------------------*/
static DWORD MenuFastStreamingReadOptionCb(PVOID pCbCtx)
{
    DRIVER_CONTEXT *pDrvCtx = ((MENU_CTX_USB *)pCbCtx)->pDrvCtx;
    WDU_DEVICE_HANDLE *phDevice = ((MENU_CTX_USB *)pCbCtx)->phDevice;
    DWORD dwInterfaceNumber = 0, dwAlternateSetting = 0;
    DWORD dwPipeNum = 0;
    DWORD dwStatus = WD_STATUS_SUCCESS;
    WDU_DEVICE *pDevice = NULL;
    WDU_PIPE_INFO *pPipe;
    DWORD dwBufferSize = 0x20000;

    dwStatus = GetDevice(pDrvCtx, phDevice);
    if (dwStatus)
        goto Exit;

    dwStatus = GetInterfaceAndAltSettings(&dwInterfaceNumber,
        &dwAlternateSetting);
    if (dwStatus)
        goto Exit;

    sprintf(line, "Please enter the pipe number (hex): 0x");
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(&dwPipeNum, line, TRUE, 0, 0))
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    dwStatus = USB_SetInterface(*phDevice, dwInterfaceNumber,
        dwAlternateSetting);
    if (dwStatus)
    {
        goto Error;
    }
    else
    {
        TRACE("%s: USB_SetInterface() completed successfully\n",
            __FUNCTION__);
        pDrvCtx->pActiveDev->dwInterfaceNum = dwInterfaceNumber;
        pDrvCtx->pActiveDev->dwAlternateSetting = dwAlternateSetting;
    }

    dwStatus = USB_GetDeviceInfo(*phDevice, &pDevice);
    if (dwStatus)
        goto Error;

    /* Search for the pipe */
    pPipe = USB_FindPipeInDevice(pDevice, dwPipeNum);
    if (!pPipe)
    {
        ERR("%s: Pipe number 0x%x does not exist\n",
            __FUNCTION__, dwPipeNum);
        goto Exit;
    }

    if (!dwPipeNum || pPipe->type == PIPE_TYPE_CONTROL)
    {
        ERR("%s: Cannot listen to control pipes\n", __FUNCTION__);
        goto Exit;
    }

    dwStatus = USB_ListenToPipe(*phDevice, pPipe, TRUE, dwBufferSize,
        FALSE, stdin, stdout, TRUE);

    if (dwStatus == WD_STATUS_SUCCESS)
        goto Exit;

Error:
    ERR("%s: %s", __FUNCTION__, USB_GetLastErr());
Exit:
    WDU_PutDeviceInfo(pDevice);

    return dwStatus;
}
#endif

/* ---------------*/
/* Select Device */
/* ---------------*/
static BOOL MenuIsAtMostOneDeviceOpen(DIAG_MENU_OPTION *pMenu)
{
    DRIVER_CONTEXT *pDrvCtx = ((MENU_CTX_USB *)pMenu->pCbCtx)->pDrvCtx;
    DWORD dwDeviceCount;

    OsMutexLock(pDrvCtx->hMutex);
    dwDeviceCount = pDrvCtx->dwDeviceCount;
    OsMutexUnlock(pDrvCtx->hMutex);

    return dwDeviceCount <= 1;
}

static DWORD MenuSelectDeviceOptionCb(PVOID pCbCtx)
{
    DRIVER_CONTEXT *pDrvCtx = ((MENU_CTX_USB *)pCbCtx)->pDrvCtx;
    WDU_DEVICE_HANDLE *phDevice = ((MENU_CTX_USB *)pCbCtx)->phDevice;

    DWORD dwStatus = GetDevice(pDrvCtx, phDevice);
    if (dwStatus)
        goto Exit;

    OsMutexLock(pDrvCtx->hMutex);
    if (pDrvCtx->dwDeviceCount > 1)
    {
        DWORD dwDeviceNum, i = 1;
        DEVICE_CONTEXT *pCur;

        for (pCur = pDrvCtx->deviceContextList; pCur;
            pCur = pCur->pNext)
        {
            TRACE("  %d. Vendor id: 0x%x, Product id: 0x%x, "
                "Interface number: %d, Alt. Setting: %d\n", i,
                pCur->dwVendorId, pCur->dwProductId,
                pCur->dwInterfaceNum, pCur->dwAlternateSetting);
            i++;
        }

        sprintf(line, "Please enter the device number (1 - %d, dec): ",
            i - 1);
        if (DIAG_INPUT_SUCCESS !=
            DIAG_InputDWORD(&dwDeviceNum, line, FALSE, 0, 0))
        {
            goto Unlock;
        }

        for (pCur = pDrvCtx->deviceContextList, i = 1;
            pCur && i < dwDeviceNum; pCur = pCur->pNext, i++);

        pDrvCtx->pActiveDev = pCur;
    }

Unlock:
    OsMutexUnlock(pDrvCtx->hMutex);
Exit:
    return dwStatus;
}

#if defined(WIN32)
/* ------------------*/
/* Selective Suspend */
/* ------------------*/
static BOOL MenuIsSuspended(DIAG_MENU_OPTION *pMenu)
{
    return ((MENU_CTX_USB *)pMenu->pCbCtx)->fSuspended;
}

static BOOL MenuIsNotSuspended(DIAG_MENU_OPTION *pMenu)
{
    return !((MENU_CTX_USB *)pMenu->pCbCtx)->fSuspended;
}

static DWORD MenuSelectiveSuspendOptionCb(PVOID pCbCtx)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;
    MENU_CTX_USB *pMenuCtx = (MENU_CTX_USB *)pCbCtx;

    dwStatus = USB_SelectiveSuspend(*(pMenuCtx->phDevice),
        pMenuCtx->fSuspended ?
        WDU_SELECTIVE_SUSPEND_CANCEL : WDU_SELECTIVE_SUSPEND_SUBMIT);

    if (dwStatus)
    {
        ERR("%s: %s", __FUNCTION__, USB_GetLastErr());
    }
    else
    {
        pMenuCtx->fSuspended = !pMenuCtx->fSuspended;
    }

    return dwStatus;
}

static void MenuSelectiveSuspendOptionsInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_USB *pUsbMenuCtx)
{
    static DIAG_MENU_OPTION enterSuspendModeMenu = { 0 };
    static DIAG_MENU_OPTION leaveSuspendModeMenu = { 0 };
    static DIAG_MENU_OPTION options[2] = { 0 };

    strcpy(enterSuspendModeMenu.cOptionName, "Enter suspend mode");
    enterSuspendModeMenu.cbEntry = MenuSelectiveSuspendOptionCb;
    enterSuspendModeMenu.cbIsHidden = MenuIsSuspended;

    strcpy(leaveSuspendModeMenu.cOptionName, "Leave suspend mode");
    leaveSuspendModeMenu.cbEntry = MenuSelectiveSuspendOptionCb;
    leaveSuspendModeMenu.cbIsHidden = MenuIsNotSuspended;

    options[0] = enterSuspendModeMenu;
    options[1] = leaveSuspendModeMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        pUsbMenuCtx, pParentMenu);
}
#endif

/* ----------------------------*/
/* Print Device Serial Number */
/* ----------------------------*/
static DWORD MenuPrintDeviceSerialNumberOptionCb(PVOID pCbCtx)
{
    DRIVER_CONTEXT *pDrvCtx = ((MENU_CTX_USB *)pCbCtx)->pDrvCtx;
    WDU_DEVICE_HANDLE *phDevice = ((MENU_CTX_USB *)pCbCtx)->phDevice;

    DWORD dwStatus = GetDevice(pDrvCtx, phDevice);
    if (dwStatus)
        goto Exit;

    dwStatus = USB_PrintDeviceSerialNumberByHandle(*phDevice, stdout);
    if (dwStatus)
        ERR("%s: %s", __FUNCTION__, USB_GetLastErr());

Exit:
    return dwStatus;
}

/* ----------------------------*/
/* Print Device Properties */
/* ----------------------------*/
static DWORD MenuPrintDeviceInformationOptionCb(PVOID pCbCtx)
{
    DRIVER_CONTEXT *pDrvCtx = ((MENU_CTX_USB *)pCbCtx)->pDrvCtx;
    WDU_DEVICE_HANDLE *phDevice = ((MENU_CTX_USB *)pCbCtx)->phDevice;
    DWORD dwStatus = GetDevice(pDrvCtx, phDevice);
    if (dwStatus)
        goto Exit;

    USB_PrintDeviceProperties(*phDevice, stdout);

Exit:
    return dwStatus;
}

static DWORD MenuMainEntryCb(PVOID pCbCtx)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;
    DRIVER_CONTEXT *pDrvCtx = ((MENU_CTX_USB *)pCbCtx)->pDrvCtx;

    while (TRUE)
    {
        if (!pDrvCtx->dwDeviceCount)
        {
            TRACE("\n");
            TRACE("No Devices are currently connected.\n");
            TRACE("Press Enter to re check or enter EXIT to exit\n");
            fgets(line, sizeof(line), stdin);
            /* Removing the '\n' character from the end */
            line[strlen(line) - 1] = '\0';

            if (!stricmp(line, "EXIT"))
            {
                dwStatus = WD_WINDRIVER_STATUS_ERROR;
                goto Exit;
            }
            continue;
        }

        OsMutexLock(pDrvCtx->hMutex);
        if (!pDrvCtx->dwDeviceCount)
        {
            OsMutexUnlock(pDrvCtx->hMutex);
            continue;
        }

        break;
    }

    if (!pDrvCtx->pActiveDev)
        pDrvCtx->pActiveDev = pDrvCtx->deviceContextList;


    TRACE("\n");
    TRACE("Main Menu (active Dev/Prod/Interface/Alt. Setting: "
        "0x%x/0x%x/%d/%d)\n", pDrvCtx->pActiveDev->dwVendorId,
        pDrvCtx->pActiveDev->dwProductId,
        pDrvCtx->pActiveDev->dwInterfaceNum,
        pDrvCtx->pActiveDev->dwAlternateSetting);
    TRACE("----------\n");

    OsMutexUnlock(pDrvCtx->hMutex);
Exit:
    return dwStatus;
}

#define MENU_RW_PIPE_INDEX 3
#define MENU_SELECTIVE_SUSPEND_INDEX 5
static void MenuMainOptionsInit(DIAG_MENU_OPTION *pMainMenu,
    MENU_CTX_USB *pUsbMenuCtx)
{
    static DIAG_MENU_OPTION displayDeviceConfigurationsMenu = { 0 };
    static DIAG_MENU_OPTION changeInterfaceSettingMenu = { 0 };
    static DIAG_MENU_OPTION resetPipeMenu = { 0 };
    static DIAG_MENU_OPTION ioPipeMenu = { 0 };
#if defined(WIN32)
    #define MENU_RW_PIPE_WIN_INCREASE 2
    static DIAG_MENU_OPTION options[10] = { 0 };
    static DIAG_MENU_OPTION fastStreamingReadMenu = { 0 };
    static DIAG_MENU_OPTION selectiveSuspendMenu = { 0 };
#else
    #define MENU_RW_PIPE_WIN_INCREASE 0
    static DIAG_MENU_OPTION options[8] = { 0 };
#endif
    static DIAG_MENU_OPTION selectDeviceMenu = { 0 };
    static DIAG_MENU_OPTION displayDeviceSerialMenu = { 0 };
    static DIAG_MENU_OPTION displayDeviceInfoMenu = { 0 };
    static DIAG_MENU_OPTION refreshMenu = { 0 };


    strcpy(displayDeviceConfigurationsMenu.cOptionName, "Display device "
        "configurations");
    displayDeviceConfigurationsMenu.cbEntry = MenuPrintDeviceCfgsOptionCb;
    displayDeviceConfigurationsMenu.cbExit = MenuUsbExitCb;

    strcpy(changeInterfaceSettingMenu.cOptionName, "Change interface alternate"
        " setting");
    changeInterfaceSettingMenu.cbEntry = MenuChangeInterfaceAltSettingOptionCb;
    changeInterfaceSettingMenu.cbExit = MenuUsbExitCb;

    strcpy(resetPipeMenu.cOptionName, "Reset Pipe");
    resetPipeMenu.cbEntry = MenuResetPipeOptionCb;
    resetPipeMenu.cbExit = MenuUsbExitCb;

    strcpy(ioPipeMenu.cOptionName, "Read/Write from pipes");
    ioPipeMenu.cbEntry = MenuRwPipeEntryCb;
    ioPipeMenu.cbExit = MenuUsbExitCb;

#if defined(WIN32)
    strcpy(fastStreamingReadMenu.cOptionName, "Fast streaming read");
    fastStreamingReadMenu.cbEntry = MenuFastStreamingReadOptionCb;
    fastStreamingReadMenu.cbExit = MenuUsbExitCb;

    strcpy(selectiveSuspendMenu.cOptionName, "Selective Suspend");
    strcpy(selectiveSuspendMenu.cTitleName, "Toggle suspend mode");
    selectiveSuspendMenu.cbEntry = MenuUsbEntryCb;
    selectiveSuspendMenu.cbExit = MenuUsbExitCb;
#endif

    strcpy(selectDeviceMenu.cOptionName, "Select Device");
    selectDeviceMenu.cbEntry = MenuSelectDeviceOptionCb;
    selectDeviceMenu.cbIsHidden = MenuIsAtMostOneDeviceOpen;
    selectDeviceMenu.cbExit = MenuUsbExitCb;

    strcpy(displayDeviceSerialMenu.cOptionName, "Display device serial "
        "number");
    displayDeviceSerialMenu.cbEntry = MenuPrintDeviceSerialNumberOptionCb;
    displayDeviceSerialMenu.cbExit = MenuUsbExitCb;

    strcpy(displayDeviceInfoMenu.cOptionName, "Display device information");
    displayDeviceInfoMenu.cbEntry = MenuPrintDeviceInformationOptionCb;
    displayDeviceInfoMenu.cbExit = MenuUsbExitCb;

    strcpy(refreshMenu.cOptionName, "Refresh");
    /* No real entry callback, refresh does nothing */
    refreshMenu.cbExit = MenuUsbExitCb;

    options[0] = displayDeviceConfigurationsMenu;
    options[1] = changeInterfaceSettingMenu;
    options[2] = resetPipeMenu;
    options[3] = ioPipeMenu;

#if defined(WIN32)
    options[4] = fastStreamingReadMenu;
    options[5] = selectiveSuspendMenu;
#endif
    options[4 + MENU_RW_PIPE_WIN_INCREASE] = selectDeviceMenu;
    options[5 + MENU_RW_PIPE_WIN_INCREASE] = displayDeviceSerialMenu;
    options[6 + MENU_RW_PIPE_WIN_INCREASE] = displayDeviceInfoMenu;
    options[7 + MENU_RW_PIPE_WIN_INCREASE] = refreshMenu;

    MenuRwPipeOptionsInit(&options[MENU_RW_PIPE_INDEX], pUsbMenuCtx);

#if defined(WIN32)
    MenuSelectiveSuspendOptionsInit(&options[MENU_SELECTIVE_SUSPEND_INDEX],
        pUsbMenuCtx);
#endif

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        pUsbMenuCtx, pMainMenu);
}

static DIAG_MENU_OPTION *MenuMainInit(DRIVER_CONTEXT *pDrvCtx)
{
    static WDU_DEVICE_HANDLE hDevice;
    static MENU_CTX_USB usbMenuCtx = { 0 };
    static DIAG_MENU_OPTION menuRoot = { 0 };

    usbMenuCtx.pDrvCtx = pDrvCtx;
    usbMenuCtx.phDevice = &hDevice;
#if defined(WIN32)
    usbMenuCtx.fStreamMode = TRUE;
#endif

    menuRoot.cbEntry = MenuMainEntryCb;
    menuRoot.pCbCtx = &usbMenuCtx;

    MenuMainOptionsInit(&menuRoot, &usbMenuCtx);
    return &menuRoot;
}

int main()
{
    DWORD dwStatus;
    DRIVER_CONTEXT drvCtx;
    WDU_MATCH_TABLE matchTable;
    DIAG_MENU_OPTION *pMenuRoot = MenuMainInit(&drvCtx);

    dwStatus = USB_Init(&drvCtx, &matchTable);
    if (dwStatus)
    {
        TRACE("USB_Init faild. error 0x%x (\"%s\")\n", dwStatus,
            Stat2Str(dwStatus));
        goto Exit;
    }

    dwStatus = DIAG_MenuRun(pMenuRoot);

Exit:
    DriverUninit(&drvCtx);
    return dwStatus;
}


