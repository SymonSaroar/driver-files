/*  */
package com.jungo.UsbDiag;

import java.util.List;
import java.util.ArrayList;
import com.jungo.*;
import com.jungo.wdapi.*;
import com.jungo.shared.*;
import com.jungo.shared.DiagLib.*;
import static com.jungo.shared.DiagLib.DiagMenuOption;
import java.nio.ByteBuffer;

import static com.jungo.shared.DiagLib.__FUNCTION__;

import java.util.LinkedList;
import java.util.Scanner;

/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/**************************************************************************
 * This is a diagnostics application for accessing the USB device. The code
 * accesses the hardware via WinDriver functions.
 *
 * Note: This code sample is provided AS-IS and as a guiding sample only.
 *
 *************************************************************************/

public class UsbDiag
{
    static DRIVER_CONTEXT drvCtx;
    static DEVICE_CONTEXT pActiveDev = null;

    // #if defined(USB_DIAG_SAMPLE)
    /* TODO: Change the following definitions to match your device. */
    final static int DEFAULT_VENDOR_ID = 0x0;
    final static int DEFAULT_PRODUCT_ID = 0x0;

    /* WinDriver license registration string */
    /* TODO: When using a registered WinDriver version, replace the license
             string below with the development license in order to use on the
             development machine.
             Once you require to distribute the driver's package to other
             machines, please replace the string with a distribution license */
    final static String DEFAULT_LICENSE_STRING = "12345abcde1234.license";

    // #else
    // /* Use in wizard's device-specific generated code */
    // #define DEFAULT_VENDOR_ID %VID%
    // #define DEFAULT_PRODUCT_ID %PID%
    // #define DEFAULT_LICENSE_STRING "%LICENSE%"
    // #endif

    /* TODO: Change the following definition to your driver's name */
    final static String DEFAULT_DRIVER_NAME = wdapi.WD_DEFAULT_DRIVER_NAME_BASE;
    //
    final static short USE_DEFAULT = 0xff;
    final static int ATTACH_EVENT_TIMEOUT = 30, /* in seconds */
        TRANSFER_TIMEOUT = 30000; /* in msecs */

    static class DEVICE_CONTEXT {
        long hDevice;
        long dwVendorId;
        long dwProductId;
        long dwInterfaceNum;
        long dwAlternateSetting;
        WDU_DEVICE dev;

        @Override
        public String toString() {
            return "DEVICE_CONTEXT [hDevice=" + Long.toHexString(hDevice)
                + ", dwVendorId=" + dwVendorId + ", dwProductId=" + dwProductId
                + ", dwInterfaceNum=" + dwInterfaceNum + ", dwAlternateSetting="
                + dwAlternateSetting + ", dev=" + dev + "]";
        }
    }

    static class DRIVER_CONTEXT {
        public DRIVER_CONTEXT() {
            deviceContextList = new LinkedList<DEVICE_CONTEXT>();
        }

        long hEvent;
        long hMutex;
        LinkedList<DEVICE_CONTEXT> deviceContextList;
        int activeDevIx;
        long hDeviceUnusedEvent;
    }

    static class MENU_CTX_USB {
        WDU_DEVICE pDevice;
        boolean fStreamMode;
        boolean fSuspended;

        public MENU_CTX_USB(Boolean fStreamMode)
        {
            this.fStreamMode = fStreamMode;
            this.fSuspended = false;
        }
    }

    static long hDriver = 0;

    static boolean DeviceAttach(long hDevice, long pDeviceInfo,
        Object pUserData)
    {
        DRIVER_CONTEXT pDrvCtx = (DRIVER_CONTEXT) pUserData;
        DEVICE_CONTEXT pDevCtx;

        WDU_DEVICE dev = new WDU_DEVICE(pDeviceInfo);

        WDU_ALTERNATE_SETTING pActiveAltSetting =
            dev.pConfigs[dev.ActiveConfigIx].pInterfaces[dev.ActiveInterfaceIx].
            pAlternateSettings[dev.pConfigs[dev.ActiveConfigIx].
            pInterfaces[dev.ActiveInterfaceIx].ActiveAltSettingIx];

        long dwInterfaceNum = pActiveAltSetting.Descriptor.bInterfaceNumber;
        long dwAlternateSetting = pActiveAltSetting.Descriptor.bAlternateSetting;

        // NOTE: To change the alternate setting, call WDU_SetInterface() here
        long dwAttachError;

        // TODO: Replace with the requested number:
        dwAlternateSetting = 0; // %alternate_setting_number%;

        dwAttachError = wdapi.WDU_SetInterface(hDevice, dwInterfaceNum,
            dwAlternateSetting);
        if (dwAttachError != 0) {
            System.err.printf(
                "DeviceAttach: WDU_SetInterface() failed (num. %d, alternate %d) "
                    + "device 0x%x. error 0x%x (\"%s\")\n",
                dwInterfaceNum, dwAlternateSetting, hDevice, dwAttachError,
                wdapi.Stat2Str(dwAttachError));

            return false;
        }

        /* Uncomment the following code to allow only one device per process */
        /*
        boolean hasDevice;
        wdapi.OsMutexLock(pDrvCtx.hMutex);
        hasDevice = pDrvCtx.deviceContextList.size() > 0;
        wdapi.OsMutexUnlock(pDrvCtx.hMutex);
        if (hasDevice)
        {
            System.out.printf("DeviceAttach: This process already has one " +
                "device, giving this one up\n");
            return false;
        }
        */

        System.out.printf(
            "\nDeviceAttach: Received and accepted attach for vendor id 0x%x, "
                + "product id 0x%x, interface %d, device handle 0x%x\n",
            dev.Descriptor.idVendor, dev.Descriptor.idProduct, dwInterfaceNum,
            hDevice);

        /* Add our device to the device list */

        pDevCtx = new DEVICE_CONTEXT();
        pDevCtx.hDevice = hDevice;
        pDevCtx.dwInterfaceNum = dwInterfaceNum;
        pDevCtx.dwVendorId = dev.Descriptor.idVendor;
        pDevCtx.dwProductId = dev.Descriptor.idProduct;
        pDevCtx.dwAlternateSetting = dwAlternateSetting;
        pDevCtx.dev = dev;

        wdapi.OsMutexLock(pDrvCtx.hMutex);
        drvCtx.deviceContextList.add(pDevCtx);
        wdapi.OsMutexUnlock(pDrvCtx.hMutex);

        wdapi.OsEventSignal(pDrvCtx.hEvent);

        /* Accept control over this device */
        return true;
    }

    //
    static void DeviceDetach(long hDevice, Object pUserData) {
        DRIVER_CONTEXT pDrvCtx = (DRIVER_CONTEXT) pUserData;
        boolean bDetachActiveDev = false;
        int i;

        System.out.printf(
            "\nDeviceDetach: Received detach for device handle 0x%x\n",
            hDevice);

        wdapi.OsMutexLock(drvCtx.hMutex);

        for (i = 0; i < pDrvCtx.deviceContextList.size()
            && pDrvCtx.deviceContextList.get(i).hDevice != hDevice; i++)

        pDrvCtx.deviceContextList.remove(i);

        if (i == drvCtx.activeDevIx) {
            bDetachActiveDev = true;
            pDrvCtx.activeDevIx = 0;

            if (pDrvCtx.deviceContextList.size() > 0)
                pActiveDev = pDrvCtx.deviceContextList.get(0);
            else
                pActiveDev = null;
        }

        wdapi.OsMutexUnlock(drvCtx.hMutex);

        if (bDetachActiveDev) {
            /*
             * When hDeviceUnusedEvent is not signaled, hDevice is possibly in
             * use, and therefore the detach callback needs to wait on it until
             * it is certain that it cannot be used. When it is signaled -
             * hDevice is no longer used.
             */
            wdapi.OsEventWait(drvCtx.hDeviceUnusedEvent, wdapi.INFINITE);
        }
    }

    static long DriverInit(WDU_MATCH_TABLE pMatchTables,
        long dwNumMatchTables, String sDriverName, String sLicense,
        DRIVER_CONTEXT drvCtx) {
        long dwError;
        WDU_EVENT_TABLE eventTable = new WDU_EVENT_TABLE();
        WDCResultLong result;

        /* Set Driver Name */
        if (wdapi.WD_DriverName(sDriverName) == "") {
            System.err.printf(
                "DriverInit: Failed setting driver name to %s, exiting\n",
                sDriverName);
            return wdapi.WD_SYSTEM_INTERNAL_ERROR;
        }

        result = wdapi.OsEventCreate();
        dwError = result.dwStatus;
        if (dwError != 0) {
            System.err.printf(
                "DriverInit: OsEventCreate() failed on event 0x%x. error 0x%x "
                    + "(\"%s\")\n",
                drvCtx.hEvent, dwError, wdapi.Stat2Str(dwError));
            return dwError;
        }
        drvCtx.hEvent = result.result;

        result = wdapi.OsMutexCreate();
        dwError = result.dwStatus;
        if (dwError != 0) {
            System.err.printf(
                "DriverInit: OsMutexCreate() failed on mutex 0x%x. error 0x%x "
                    + "(\"%s\")\n",
                drvCtx.hMutex, dwError, wdapi.Stat2Str(dwError));
            return dwError;
        }
        drvCtx.hMutex = result.result;

        result = wdapi.OsEventCreate();
        dwError = result.dwStatus;
        if (dwError != 0) {
            System.err.printf(
                "DriverInit: OsEventCreate() failed on event 0x%x. error 0x%x "
                    + "(\"%s\")\n",
                drvCtx.hDeviceUnusedEvent, dwError, wdapi.Stat2Str(dwError));
            return dwError;
        }
        drvCtx.hDeviceUnusedEvent = result.result;

        wdapi.OsEventSignal(drvCtx.hDeviceUnusedEvent);

        eventTable.pfDeviceAttach = new WDU_ATTACH_CALLBACK()
        {
            @Override
            public boolean run(long hDevice, long pDeviceInfo, Object o)
            {
                return DeviceAttach(hDevice, pDeviceInfo, o);
            }
        };
        eventTable.pfDeviceDetach = new WDU_DETACH_CALLBACK()
        {
            @Override
            public void run(long hDevice, Object o)
            {
                DeviceDetach(hDevice, o);
            }
        };
        eventTable.pUserData = drvCtx;

        result = wdapi.WDU_Init(pMatchTables, dwNumMatchTables, eventTable,
            sLicense, wdapi.WD_ACKNOWLEDGE);
        dwError = result.dwStatus;
        if (dwError != 0) {
            System.err.printf(
                "DriverInit: Failed to initialize USB driver. error 0x%x "
                    + "(\"%s\")\n",
                dwError, wdapi.Stat2Str(dwError));
            return dwError;
        }
        hDriver = result.result;

        return wdapi.WD_STATUS_SUCCESS;
    }

    static void DriverUninit() {
        if (drvCtx.hEvent != 0)
            wdapi.OsEventClose(drvCtx.hEvent);
        if (drvCtx.hMutex != 0)
            wdapi.OsMutexClose(drvCtx.hMutex);
        if (drvCtx.hDeviceUnusedEvent != 0)
            wdapi.OsEventClose(drvCtx.hDeviceUnusedEvent);
        if (hDriver != 0)
            wdapi.WDU_Uninit(hDriver);
    }

    static long GetDevice()
    {
        if (pActiveDev == null)
        {
            System.out.printf("%s: Could not get active device\n",
                __FUNCTION__());
            return wdapi.WD_WINDRIVER_STATUS_ERROR;
        }

        wdapi.OsEventReset(drvCtx.hDeviceUnusedEvent);

        wdapi.OsMutexLock(drvCtx.hMutex);

        pActiveDev = drvCtx.deviceContextList.get(drvCtx.activeDevIx);

        wdapi.OsMutexUnlock(drvCtx.hMutex);

        return wdapi.WD_STATUS_SUCCESS;
    }
    
    /* ----------------------------*/
    /* Print Device Configurations */
    /* ----------------------------*/
    static long MenuPrintDeviceCfgsOptionCb(Object pCbCtx)
    {
        long dwStatus = GetDevice();
        if (dwStatus != 0)
            return dwStatus;
        
        dwStatus = UsbDiagLib.USB_PrintDeviceConfigurations(pActiveDev.dev);
        if (dwStatus != 0)
            System.out.printf("%s: %s\n", __FUNCTION__(), UsbDiagLib.USB_GetLastErr());
        
        return dwStatus;
    }

    /* -----------------------------------*/
    /* Change interface alternate setting */
    /* -----------------------------------*/
    static long MenuChangeInterfaceAltSettingOptionCb(Object pCbCtx)
    {
        long dwStatus = wdapi.WD_STATUS_SUCCESS;
        long dwInterfaceNumber, dwAlternateSetting;
        WDCResult result;

        dwStatus = GetDevice();
        if (dwStatus != 0)
            return dwStatus;

        result = DiagLib.DIAG_InputNum(
            "Please enter the interface number (dec): ", false, 0,
            10000);
        if (result.dwStatus != DiagLib.DIAG_INPUT_SUCCESS)
            return dwStatus;
        
        dwInterfaceNumber = ((WDCResultLong)result).result;

        result = DiagLib.DIAG_InputNum(
            "Please enter the alternate setting index (dec): ", false,
            0, 10000);
        if (result.dwStatus != DiagLib.DIAG_INPUT_SUCCESS)
            return dwStatus;
        
        dwAlternateSetting = ((WDCResultLong)result).result;
        
        dwStatus = UsbDiagLib.USB_SetInterface(pActiveDev.hDevice,
            dwInterfaceNumber, dwAlternateSetting);
        if (dwStatus != 0)
        {
            System.out.printf("%s: %s\n", __FUNCTION__(), UsbDiagLib.USB_GetLastErr());
        }
        else
        {
            pActiveDev.dev.updateDev();
            System.out
                .printf("%s: WDU_SetInterface() completed successfully\n",
                    __FUNCTION__());
            pActiveDev.dwInterfaceNum = dwInterfaceNumber;
            pActiveDev.dwAlternateSetting = dwAlternateSetting;
        }

        return dwStatus;
    }

    /* ------------*/
    /* Reset Pipe */
    /* ------------*/
    static long MenuResetPipeOptionCb(Object pCbCtx)
    {
        long dwPipeNum = 0, dwStatus = wdapi.WD_STATUS_SUCCESS;
        WDCResult result;

        dwStatus = GetDevice();
        if (dwStatus != 0)
            return dwStatus;

        UsbDiagLib.USB_PrintDevicePipesInfo(pActiveDev.dev);
        
        result = DiagLib.DIAG_InputNum(
            "Please enter the pipe number (hex): ", true, 0, 0);
        if (result.dwStatus != DiagLib.DIAG_INPUT_SUCCESS)
            return dwStatus;

        dwPipeNum = ((WDCResultLong)result).result;
        System.out.printf("\n");
        
        dwStatus = UsbDiagLib.USB_ResetPipe(pActiveDev.hDevice, dwPipeNum);
        if (dwStatus != 0) {
            System.out.printf("%s: %s\n", __FUNCTION__(),
                UsbDiagLib.USB_GetLastErr());
            return dwStatus;
        } else {
            pActiveDev.dev.updateDev();
            System.out
            .printf("%s: WDU_ResetPipe() completed successfully\n",
                __FUNCTION__());
        }

        return dwStatus;
    }
    
    /*--------------*/
    /* Read/Write Pipe */
    /*--------------*/
    static WDU_PIPE_INFO MenuRwPipeGetPipe()
    {
        long dwPipeNum;
        WDU_PIPE_INFO pPipe;

        WDCResult result = DiagLib.DIAG_InputNum(
            "Please enter the pipe number (hex):", true, 0, 0xff);
        if (result.dwStatus != DiagLib.DIAG_INPUT_SUCCESS)
        {
            System.out.printf("Error: Invalid pipe number.");
            return null;
        }

        dwPipeNum = ((WDCResultLong)result).result;

        /* Search for the pipe */
        pPipe = UsbDiagLib.USB_FindPipeInDevice(pActiveDev.dev, dwPipeNum);
        if (pPipe == null)
        {
            System.out.printf(
                "The pipe number 0x%x does not exist. Please try again.\n",
                dwPipeNum);
        }

        return pPipe;
    }

    static long MenuRwPipeReadWrite(MENU_CTX_USB pMenuCtx, boolean fRead)
    {
        long dwPipeNum, dwBytesTransferred, dwBufferSize = 0x20000,
            dwStatus = wdapi.WD_STATUS_SUCCESS;
        int dwSize;
        ByteBuffer pBuffer = null;
        byte[] SetupPacket = new byte[8];
        WDCResult result;

        WDU_PIPE_INFO pPipe = MenuRwPipeGetPipe();
        if (pPipe == null)
            return wdapi.WD_INVALID_PARAMETER;

        dwPipeNum = pPipe.dwNumber;
        if (dwPipeNum == 0
            || pPipe.type == wdapi.PIPE_TYPE_CONTROL)
        {
            if (pMenuCtx.fStreamMode)
            {
                System.err.printf(
                    "Cannot perform stream transfer using control pipe.\n"
                        + "please switch to Single Blocking Transfer mode "
                        + "(option 6) or change the pipe number\n");
                        return wdapi.WD_INVALID_PARAMETER;
            }
            System.out.printf("Please enter setup packet (hex - 8 bytes): ");
            SetupPacket = DiagLib.DIAG_GetHexBuffer(8);
        }

        result = DiagLib.DIAG_InputNum(
            "Please enter the size of the buffer (dec): ", false, 0, 0);
        if (result.dwStatus != DiagLib.DIAG_INPUT_SUCCESS)
            return result.dwStatus;

        dwSize = (int)((WDCResultLong)result).result;

        if (dwSize != 0)
        {
            pBuffer = ByteBuffer.allocateDirect(dwSize);
            if (pBuffer == null)
            {
                System.err.printf("Cannot alloc memory\n");
                return wdapi.WD_INSUFFICIENT_RESOURCES;
            }

            if (!fRead)
            {
                System.out.printf("Please enter the input buffer (hex): ");
                pBuffer.put(DiagLib.DIAG_GetHexBuffer(dwSize));
            }
        }

        if (pMenuCtx.fStreamMode)
        {
            result = UsbDiagLib.USB_ReadWriteStream(pActiveDev.hDevice,
                dwPipeNum, pBuffer, dwBufferSize, dwSize, fRead);
        }
        else
        {
            result = UsbDiagLib.USB_ReadWriteTransfer(pActiveDev.hDevice,
                dwPipeNum, pBuffer, dwSize, SetupPacket, fRead);
        }
    
        dwStatus = result.dwStatus;
        dwBytesTransferred = ((WDCResultLong)result).result;
        if (dwStatus != 0)
        {
            System.out.printf("%s: %s\n", __FUNCTION__(),
                UsbDiagLib.USB_GetLastErr());
        }
        else
        {
            System.out.printf("Transferred %d bytes\n",
                dwBytesTransferred);

            if (dwBytesTransferred > 0)
            {
                byte[] tmp = new byte[(int) dwBytesTransferred];
                pBuffer.get(tmp);

                if (fRead && pBuffer != null)
                    DiagLib.DIAG_PrintHexBuffer(tmp);
            }
        }
        return dwStatus;

    }

    static long MenuRwPipeReadOptionCb(Object pCbCtx)
    {
        return MenuRwPipeReadWrite((MENU_CTX_USB)pCbCtx, true);
    }

    static long MenuRwPipeWriteOptionCb(Object pCbCtx)
    {
        return MenuRwPipeReadWrite((MENU_CTX_USB)pCbCtx, false);
    }

    static long MenuRwPipeListenMeasure(MENU_CTX_USB pMenuCtx, boolean fListen)
    {
        long dwPipeNum, dwBufferSize = 0x20000;
        long dwStatus = wdapi.WD_STATUS_SUCCESS;
        ByteBuffer pBuffer = null;

        WDU_PIPE_INFO pPipe = MenuRwPipeGetPipe();
        if (pPipe == null)
            return wdapi.WD_INVALID_PARAMETER;
        
        dwPipeNum = pPipe.dwNumber;
        if (dwPipeNum == 0
                    || pPipe.type == wdapi.PIPE_TYPE_CONTROL)
        {
            System.out.printf(
                "Cannot perform stream transfer with control pipe\n");
            return wdapi.WD_INVALID_PARAMETER;
        }

        dwStatus = UsbDiagLib.USB_ListenToPipe(pActiveDev.hDevice, pPipe,
            pMenuCtx.fStreamMode, dwBufferSize, true, fListen);
        if (dwStatus != 0)
        {
            System.out.printf("%s: %s\n", __FUNCTION__(),
                UsbDiagLib.USB_GetLastErr());
        }

        return dwStatus;
    }

    static long MenuRwPipeListenOptionCb(Object pCbCtx)
    {
        return MenuRwPipeListenMeasure((MENU_CTX_USB)pCbCtx, true);
    }

    static long MenuRwPipeMeasureOptionCb(Object pCbCtx)
    {
        return MenuRwPipeListenMeasure((MENU_CTX_USB)pCbCtx, false);
    }


    final static int PERF_STREAM_BUFFER_SIZE = 5120000, /* In bytes */
                     PERF_DEVICE_TRANSFER_SIZE = 256 * 1024, /* In bytes */
                     PERF_TRANSFER_ITERATIONS = 1500;

    static long MenuRwPipeCheckStreamReadWrite(MENU_CTX_USB pMenuCtx,
        boolean fRead)
    {
        long dwPipeNum, dwStatus = wdapi.WD_STATUS_SUCCESS;
        ByteBuffer pBuffer = null;
        WDU_PIPE_INFO pPipe = MenuRwPipeGetPipe();

        if (pPipe == null)
            return wdapi.WD_INVALID_PARAMETER;
        
        dwPipeNum = pPipe.dwNumber;
        if (dwPipeNum == 0
                    || pPipe.type == wdapi.PIPE_TYPE_CONTROL)
        {
            System.out.printf(
                "Cannot perform stream transfer with control pipe\n");
            return wdapi.WD_INVALID_PARAMETER;
        }

        System.out.printf(
            "The size of the buffer to transfer(dec): %d\n",
            PERF_DEVICE_TRANSFER_SIZE);
        System.out.printf(
            "The size of the internal Rx/Tx stream buffer (dec): %d\n",
            PERF_STREAM_BUFFER_SIZE);
        System.out.printf(
            "Making the transfer of %d times the buffer size, please "
                + "wait ...\n",
            PERF_TRANSFER_ITERATIONS);

        pBuffer = ByteBuffer.allocateDirect(PERF_DEVICE_TRANSFER_SIZE);
        if (pBuffer == null) {
            System.err.printf("Failed allocating memory\n");
            return wdapi.WD_INSUFFICIENT_RESOURCES;
        }

        if (!fRead)
        {
            /*
            * Here you can fill pBuffer with the right data for the
            * board
            */
        }

        dwStatus = UsbDiagLib.USB_ReadWriteStreamCheck(pActiveDev.hDevice, dwPipeNum,
            pBuffer, PERF_STREAM_BUFFER_SIZE, PERF_DEVICE_TRANSFER_SIZE,
            PERF_TRANSFER_ITERATIONS, fRead);

        if (dwStatus != 0)
            System.out.printf("%s: %s\n", __FUNCTION__(), UsbDiagLib.USB_GetLastErr());
        
        return dwStatus;
    }

    static long MenuRwPipeCheckStreamReadOptionCb(Object pCbCtx)
    {
        return MenuRwPipeCheckStreamReadWrite((MENU_CTX_USB)pCbCtx, true);
    }

    static long MenuRwPipeCheckStreamWriteOptionCb(Object pCbCtx)
    {
        return MenuRwPipeCheckStreamReadWrite((MENU_CTX_USB)pCbCtx, false);
    }

    static long MenuRwPipeSwitchTransferTypeOptionCb(Object pCbCtx)
    {
        MENU_CTX_USB pMenuCtx = (MENU_CTX_USB)pCbCtx;

        pMenuCtx.fStreamMode = !pMenuCtx.fStreamMode;
        return wdapi.WD_STATUS_SUCCESS;
    }
    static void MenuRwPipeOptionsInit(DiagMenuOption pParentMenu,
        MENU_CTX_USB pMenuCtx)
    {
        List<DiagMenuOption> options = new ArrayList<DiagMenuOption>(){{
            add(new DiagMenuOptionBuilder()
            .cOptionName("Read from pipe")
            .cbEntry((pCbCtx)->MenuRwPipeReadOptionCb((MENU_CTX_USB)pCbCtx))
            .cbExit((pCbCtx)-> wdapi.OsEventSignal(drvCtx.hDeviceUnusedEvent))
            .build());
    
            add(new DiagMenuOptionBuilder()
            .cOptionName("Write from pipe")
            .cbEntry((pCbCtx)->MenuRwPipeWriteOptionCb((MENU_CTX_USB)pCbCtx))
            .cbExit((pCbCtx)-> wdapi.OsEventSignal(drvCtx.hDeviceUnusedEvent))
            .build());

            add(new DiagMenuOptionBuilder()
            .cOptionName("Listen to pipe (continuous read)")
            .cbEntry((pCbCtx)->MenuRwPipeListenOptionCb((MENU_CTX_USB)pCbCtx))
            .cbExit((pCbCtx)-> wdapi.OsEventSignal(drvCtx.hDeviceUnusedEvent))
            .build());

            add(new DiagMenuOptionBuilder()
            .cOptionName("Measure pipe speed (continuous read)")
            .cbEntry((pCbCtx)->MenuRwPipeMeasureOptionCb((MENU_CTX_USB)pCbCtx))
            .cbExit((pCbCtx)-> wdapi.OsEventSignal(drvCtx.hDeviceUnusedEvent))
            .build());
        }};

        // Stream check, only in Windows
        if (wdapi.isWindows)
        {
            List<DiagMenuOption> winOptions = new ArrayList<DiagMenuOption>(){{
                add(new DiagMenuOptionBuilder()
                .cOptionName("Check streaming READ speed")
                .cbEntry((pCbCtx)->MenuRwPipeCheckStreamReadOptionCb((MENU_CTX_USB)pCbCtx))
                .cbIsHidden((pMenu)->(!((MENU_CTX_USB)pMenu.pCbCtx).fStreamMode))
                .cbExit((pCbCtx)-> wdapi.OsEventSignal(drvCtx.hDeviceUnusedEvent))
                .build());
        
                add(new DiagMenuOptionBuilder()
                .cOptionName("Check streaming WRITE speed")
                .cbEntry((pCbCtx)->MenuRwPipeCheckStreamWriteOptionCb((MENU_CTX_USB)pCbCtx))
                .cbIsHidden((pMenu)->(!((MENU_CTX_USB)pMenu.pCbCtx).fStreamMode))
                .cbExit((pCbCtx)-> wdapi.OsEventSignal(drvCtx.hDeviceUnusedEvent))
                .build());

                add(new DiagMenuOptionBuilder()
                .cOptionName("Switch transfer mode")
                .cbEntry((pCbCtx)->MenuRwPipeSwitchTransferTypeOptionCb((MENU_CTX_USB)pCbCtx))
                .cbExit((pCbCtx)-> wdapi.OsEventSignal(drvCtx.hDeviceUnusedEvent))
                .build());
            }};

            options.addAll(winOptions);
        }

        DiagLib.DIAG_MenuSetCtxAndParentForMenus(
            options.toArray(new DiagMenuOption[0]), pMenuCtx, pParentMenu);
    }

    static long MenuRwPipeEntryCb(Object pCbCtx)
    {
        long dwStatus = GetDevice();
        if (dwStatus != 0)
            return dwStatus;

        // todo: print this only at first entry?
        System.out.printf("\n");
        UsbDiagLib.USB_PrintDevicePipesInfo(pActiveDev.dev);

        System.out.printf("\n");
        System.out.printf("Read/Write from/to device's pipes using %s\n",
        ((MENU_CTX_USB)pCbCtx).fStreamMode ? 
            "Streaming Data Transfers" : "Single Blocking Transfers");
            System.out.printf("---------------------\n");

        return wdapi.WD_STATUS_SUCCESS;
    }

    /* -------------------*/
    /* Fast streaming Read */
    /* -------------------*/
    static long MenuFastStreamingReadOptionCb(Object pCbCtx)
    {
        long dwInterfaceNumber = 0, dwAlternateSetting = 0;
        long dwPipeNum = 0;
        long dwStatus;
        long hDevice;
        WDU_DEVICE pDevice =
            drvCtx.deviceContextList.get(drvCtx.activeDevIx).dev;
        WDU_PIPE_INFO pPipe;
        long dwBufferSize = 0x20000;
        WDCResultLong result;

        result = DiagLib.DIAG_InputNum("Please enter the interface number (dec): ",
            false, 0, 0xFF);
        if (result.dwStatus != DiagLib.DIAG_INPUT_SUCCESS)
            return result.dwStatus;

        dwInterfaceNumber = result.result;

        result = DiagLib.DIAG_InputNum("Please enter the alternate setting " +
            "index (dec): ", false, 0, 0xFF);
        if (result.dwStatus != DiagLib.DIAG_INPUT_SUCCESS)
            return result.dwStatus;

        dwAlternateSetting = result.result;

        result = DiagLib.DIAG_InputNum("Please enter the pipe number (hex): 0x",
            true, 0, 0xFF);
        if (result.dwStatus != DiagLib.DIAG_INPUT_SUCCESS)
            return result.dwStatus;

        dwPipeNum = result.result;

        wdapi.OsMutexLock(drvCtx.hMutex);
        hDevice = drvCtx.deviceContextList.get(drvCtx.activeDevIx).hDevice;
        wdapi.OsMutexUnlock(drvCtx.hMutex);

        dwStatus = UsbDiagLib.USB_SetInterface(hDevice, dwInterfaceNumber,
            dwAlternateSetting);
        if (dwStatus != 0)
        {
            System.out.printf("%s: %s\n", __FUNCTION__(), UsbDiagLib.USB_GetLastErr());
            return dwStatus;
        }
        else
        {
            System.out.printf("FastStreamingRead: WDU_SetInterface() completed"
                + " successfully\n");
            drvCtx.deviceContextList.get(drvCtx.activeDevIx).dwInterfaceNum =
                dwInterfaceNumber;
            drvCtx.deviceContextList.get(drvCtx.activeDevIx).dwAlternateSetting =
                dwAlternateSetting;
        }

        /* Search for the pipe */
        pPipe = UsbDiagLib.USB_FindPipeInDevice(pDevice, dwPipeNum);
        if (pPipe == null)
        {
            System.err.printf("FastStreamingRead: Pipe number 0x%x does not"
                + " exist\n", dwPipeNum);
            return wdapi.WD_INVALID_PARAMETER;
        }

        if (dwPipeNum == 0 || pPipe.type == wdapi.PIPE_TYPE_CONTROL)
        {
            System.err.printf("FastStreamingRead: Cannot listen to control"
                + " pipes\n");
                return wdapi.WD_INVALID_PARAMETER;
        }

        dwStatus = UsbDiagLib.USB_ListenToPipe(hDevice, pPipe, true, dwBufferSize, false, true);
        if (dwStatus != 0)
            System.out.printf("%s: %s\n", __FUNCTION__(), UsbDiagLib.USB_GetLastErr());

        return dwStatus;
    }

    /* ---------------*/
    /* Select Device */
    /* ---------------*/
    static boolean MenuIsAtMostOneDeviceOpen(MENU_CTX_USB pUsbCtx)
    {
        long dwActiveDevices;
        
        wdapi.OsMutexLock(drvCtx.hMutex);
        dwActiveDevices = drvCtx.deviceContextList.size();
        wdapi.OsMutexUnlock(drvCtx.hMutex);

        return dwActiveDevices <= 1;
    }

    static long MenuSelectDeviceOptionCb(Object pCbCtx)
    {
        wdapi.OsMutexLock(drvCtx.hMutex);
        if (drvCtx.deviceContextList.size() > 1) {
            int dwDeviceNum, i;
            DEVICE_CONTEXT pCur;
            WDCResult result;

            for (i = 0; i < drvCtx.deviceContextList.size(); i++)
            {
                pCur = drvCtx.deviceContextList.get(i);
                System.out.printf(
                    "  %d. Vendor id: 0x%x, Product id: 0x%x, "
                        + "Interface number: %d, Alt. Setting: %d\n",
                    i + 1, pCur.dwVendorId, pCur.dwProductId,
                    pCur.dwInterfaceNum, pCur.dwAlternateSetting);
            }

            System.out.printf(
                "Please enter the device number (1 - %d, dec): ", i);
            result = DiagLib.DIAG_GetMenuOption(i);
            if (result.dwStatus != DiagLib.DIAG_INPUT_SUCCESS)
            {
                wdapi.OsMutexUnlock(drvCtx.hMutex);
                return result.dwStatus;
            }

            dwDeviceNum = ((WDCResultInteger)result).result - 1;

            drvCtx.activeDevIx = dwDeviceNum;
            pActiveDev = drvCtx.deviceContextList.get(dwDeviceNum);
        }
        wdapi.OsMutexUnlock(drvCtx.hMutex);
        return wdapi.WD_STATUS_SUCCESS;
    }

    /* ------------------*/
    /* Selective Suspend */
    /* ------------------*/
    static long MenuSelectiveSuspendOptionCb(Object pCbCtx)
    {
        MENU_CTX_USB pMenuCtx = (MENU_CTX_USB)pCbCtx;
        long dwStatus = UsbDiagLib.USB_SelectiveSuspend(pActiveDev.hDevice,
            pMenuCtx.fSuspended ? 
            wdapi.WDU_SELECTIVE_SUSPEND_CANCEL : wdapi.WDU_SELECTIVE_SUSPEND_SUBMIT);
        if (dwStatus != 0)
            System.out.printf("%s: %s\n", __FUNCTION__(), UsbDiagLib.USB_GetLastErr());
        else
            pMenuCtx.fSuspended = !pMenuCtx.fSuspended;

        return dwStatus;
    }

    static void MenuSelectiveSuspendOptionsInit(DiagMenuOption pParentMenu,
        MENU_CTX_USB pMenuCtx)
        {
            DiagMenuOption options[] = {
                new DiagMenuOptionBuilder()
                .cOptionName("Enter suspend mode")
                .cbEntry((pCbCtx)->MenuSelectiveSuspendOptionCb(pCbCtx))
                .cbIsHidden((pMenu)->((MENU_CTX_USB)pMenu.pCbCtx).fSuspended)
                .build(),

                new DiagMenuOptionBuilder()
                .cOptionName("Leave suspend mode")
                .cbEntry((pCbCtx)->MenuSelectiveSuspendOptionCb(pCbCtx))
                .cbIsHidden((pMenu)->!((MENU_CTX_USB)pMenu.pCbCtx).fSuspended)
                .build(),
            };

            DiagLib.DIAG_MenuSetCtxAndParentForMenus(options, pMenuCtx,
                pParentMenu);
        }

    /* ------------------*/
    /* Print Device Serial Number */
    /* ------------------*/
    static long MenuPrintDeviceSerialNumberOptionCb(Object pCbCtx)
    {
        long dwStatus = GetDevice();
        if (dwStatus != 0)
            return dwStatus;

        dwStatus = UsbDiagLib.USB_PrintDeviceSerialNumberByHandle(pActiveDev);
        if (dwStatus != 0)
            System.out.printf("%s: %s\n", __FUNCTION__(), UsbDiagLib.USB_GetLastErr());
        
        return dwStatus;
    }

    /* ------------------*/
    /* Print Device Properties */
    /* ------------------*/
    static long MenuPrintDeviceInformationOptionCb(Object pCbCtx)
    {
        long dwStatus = GetDevice();
        if (dwStatus != 0)
            return dwStatus;

        UsbDiagLib.USB_PrintDeviceProperties(pActiveDev);
        
        return dwStatus;
    }

    static long USB_Init()
    {
        long dwError;
        short wVendorId = USE_DEFAULT;
        short wProductId = USE_DEFAULT;
        drvCtx = new DRIVER_CONTEXT();
        WDU_MATCH_TABLE matchTable = new WDU_MATCH_TABLE();
        WDCResultLong result;

        wdapi.PrintDbgMessage(wdapi.D_ERROR, wdapi.S_USB,
            "WinDriver user mode version " + wdapi.WD_VERSION_STR + "\n");

        // #if defined(USB_DIAG_SAMPLE)
        result = DiagLib.DIAG_InputNum("Enter device vendor id (hex) (=0x" +
            DEFAULT_VENDOR_ID + "):", true, 0, 0);
        wVendorId = (short)result.result;
        result = DiagLib.DIAG_InputNum("Enter device product id (hex) (=0x" +
            DEFAULT_PRODUCT_ID + "):", true, 0, 0);
        wProductId = (short)result.result;
        // #endif

        /* Use defaults */
        if (wVendorId == USE_DEFAULT)
            wVendorId = DEFAULT_VENDOR_ID;
        if (wProductId == USE_DEFAULT)
            wProductId = DEFAULT_PRODUCT_ID;

        matchTable.wVendorId = wVendorId;
        matchTable.wProductId = wProductId;

        dwError = DriverInit(matchTable, 1,
            DEFAULT_DRIVER_NAME, DEFAULT_LICENSE_STRING,
            drvCtx);
        if (dwError != 0)
            System.exit(1);
        // goto Exit;

        System.out.printf("Please make sure the device is attached:\n");

        /* Wait for the device to be attached */
        dwError = wdapi.OsEventWait(drvCtx.hEvent, ATTACH_EVENT_TIMEOUT);
        if (dwError != 0) {
            if (dwError == wdapi.WD_TIME_OUT_EXPIRED) {
                System.err
                    .printf("Timeout expired for connection with the device.\n"
                        + "Check that the device is connected and try again.\n");
            } else {
                System.err.printf(
                    "main: OsEventWait() failed on event 0x%x. error 0x%x "
                        + "(\"%s\")\n",
                    drvCtx.hEvent, dwError, wdapi.Stat2Str(dwError));
            }
            System.exit(1);
        }
        return dwError;
    }
    static long MenuMainEntryCb(Object pCbCtx)
    {
        Scanner scanner = new Scanner(System. in);
        String sInput = "";
        while(true)
        {
            if (drvCtx.deviceContextList.isEmpty()) {
                System.out.printf("\n");
                System.out.printf("No Devices are currently connected.\n");
                System.out.printf("Press Enter to re check or enter EXIT to" +
                    " exit\n");
                try
                {
                    sInput = scanner.nextLine();
                } catch (Exception e) {

                }
                if (sInput.compareTo("EXIT") == 0)
                    System.exit(1);
                continue;
            }

            wdapi.OsMutexLock(drvCtx.hMutex);
            if (drvCtx.deviceContextList.isEmpty())
            {
                wdapi.OsMutexUnlock(drvCtx.hMutex);
                continue;
            }
            break;
        }
        
        if (pActiveDev == null) {
            drvCtx.activeDevIx = 0;
            pActiveDev = drvCtx.deviceContextList.get(drvCtx.activeDevIx);
        }
        
        System.out.printf("\n");
        System.out.printf(
            "Main Menu (active Dev/Prod/Interface/Alt. Setting: "
            + "0x%x/0x%x/%d/%d)\n",
            pActiveDev.dwVendorId, pActiveDev.dwProductId,
            pActiveDev.dwInterfaceNum, pActiveDev.dwAlternateSetting);
            
        wdapi.OsMutexUnlock(drvCtx.hMutex);
        return wdapi.WD_STATUS_SUCCESS;
    }

    static DiagMenuOption MenuMainInit()
    {
        MENU_CTX_USB usbMenuCtx = new MENU_CTX_USB(wdapi.isWindows);

        DiagMenuOption menuRoot = new DiagMenuOptionBuilder()
            .cbEntry((pCbCtx)->MenuMainEntryCb(pCbCtx))
            .build();

        List<DiagMenuOption> options = new ArrayList<DiagMenuOption>(){{
            add(new DiagMenuOptionBuilder()
            .cOptionName("Display device configurations")
            .cbEntry((pCbCtx)->MenuPrintDeviceCfgsOptionCb((MENU_CTX_USB)pCbCtx))
            .cbExit((pCbCtx)-> wdapi.OsEventSignal(drvCtx.hDeviceUnusedEvent))
            .build());
    
            add(new DiagMenuOptionBuilder()
            .cOptionName("Change interface alternate setting")
            .cbEntry((pCbCtx)->MenuChangeInterfaceAltSettingOptionCb((MENU_CTX_USB)pCbCtx))
            .cbExit((pCbCtx)-> wdapi.OsEventSignal(drvCtx.hDeviceUnusedEvent))
            .build());

            add(new DiagMenuOptionBuilder()
            .cOptionName("Reset Pipe")
            .cbEntry((pCbCtx)->MenuResetPipeOptionCb((MENU_CTX_USB)pCbCtx))
            .cbExit((pCbCtx)-> wdapi.OsEventSignal(drvCtx.hDeviceUnusedEvent))
            .build());

            add(new DiagMenuOptionBuilder()
            .cOptionName("Read/Write from pipes")
            .cbEntry((pCbCtx)->MenuRwPipeEntryCb((MENU_CTX_USB)pCbCtx))
            .cbExit((pCbCtx)-> wdapi.OsEventSignal(drvCtx.hDeviceUnusedEvent))
            .build());
        }};

        MenuRwPipeOptionsInit(options.get(options.size() - 1), usbMenuCtx);

        if (wdapi.isWindows)
        {
            List<DiagMenuOption> winOptions = new ArrayList<DiagMenuOption>(){{
                add(new DiagMenuOptionBuilder()
                .cOptionName("Fast streaming read")
                .cbEntry((pCbCtx)->MenuFastStreamingReadOptionCb((MENU_CTX_USB)pCbCtx))
                .cbExit((pCbCtx)-> wdapi.OsEventSignal(drvCtx.hDeviceUnusedEvent))
                .build());
        
                add(new DiagMenuOptionBuilder()
                .cOptionName("Selective Suspend")
                .cTitleName("Toggle suspend mode")
                .cbEntry((pCbCtx)->GetDevice())
                .cbExit((pCbCtx)-> wdapi.OsEventSignal(drvCtx.hDeviceUnusedEvent))
                .build());
            }};

            MenuSelectiveSuspendOptionsInit(
                winOptions.get(winOptions.size() - 1), usbMenuCtx);
            options.addAll(winOptions);
        }

        options.addAll(new ArrayList<DiagMenuOption>(){{
            add(new DiagMenuOptionBuilder()
                .cOptionName("Select Device")
                .cbEntry((pCbCtx)->MenuSelectDeviceOptionCb((MENU_CTX_USB)pCbCtx))
                .cbIsHidden((pMenu)->MenuIsAtMostOneDeviceOpen((MENU_CTX_USB)pMenu.pCbCtx))
                .cbExit((pCbCtx)-> wdapi.OsEventSignal(drvCtx.hDeviceUnusedEvent))
                .build());

            add(new DiagMenuOptionBuilder()
                .cOptionName("Display device serial number")
                .cbEntry((pCbCtx)->MenuPrintDeviceSerialNumberOptionCb((MENU_CTX_USB)pCbCtx))
                .cbExit((pCbCtx)-> wdapi.OsEventSignal(drvCtx.hDeviceUnusedEvent))
                .build());

            add(new DiagMenuOptionBuilder()
                .cOptionName("Display device information")
                .cbEntry((pCbCtx)->MenuPrintDeviceInformationOptionCb((MENU_CTX_USB)pCbCtx))
                .cbExit((pCbCtx)-> wdapi.OsEventSignal(drvCtx.hDeviceUnusedEvent))
                .build());

            add(new DiagMenuOptionBuilder()
                .cOptionName("Refresh")
                .cbExit((pCbCtx)-> wdapi.OsEventSignal(drvCtx.hDeviceUnusedEvent))
                .build());
        }});

        DiagLib.DIAG_MenuSetCtxAndParentForMenus(
            options.toArray(new DiagMenuOption[0]), usbMenuCtx, menuRoot);

        return menuRoot;
    }

    public static void main(String[] args) {
        DiagMenuOption menuRoot = MenuMainInit();
        long dwStatus = USB_Init();
        if (dwStatus != 0)
        {
            System.out.printf("USB_Init faild. error 0x%lx (\"%s\")\n",
                dwStatus, wdapi.Stat2Str(dwStatus));
            System.exit((int)dwStatus);
        }

        dwStatus = DiagLib.DIAG_MenuRun(menuRoot);

        DriverUninit();
        System.exit((int)dwStatus);
    }
}


