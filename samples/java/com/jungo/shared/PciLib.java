/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/************************************************************************
 * File: PciLib.java
 *
 * Library for accessing PCI devices, possibly using a Kernel PlugIn driver. The
 * code accesses hardware using WinDriver's WDC library.
 *
 * 
 *
 *              Note: This code sample is provided AS-IS and as a guiding
 *                sample only.
 *************************************************************************/

package com.jungo.shared;

import com.jungo.*;
import com.jungo.wdapi.*;
import com.jungo.shared.*;
import com.jungo.shared.PciMenusCommon.*;

import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;

public class PciLib
{
    /*************************************************************
      General definitions
     *************************************************************/
    /* Kernel PlugIn messages - used in WDC_CallKerPlug() calls (user mode) /
     * KP_PCI_Call() (kernel mode) */
    public final static int KP_PCI_MSG_VERSION = 1;
                 /* Query the version of the Kernel PlugIn */

    /* Kernel PlugIn messages status */
    public final static int
        KP_PCI_STATUS_OK          = 0x1,
        KP_PCI_STATUS_MSG_NO_IMPL = 0x1000,
        KP_PCI_STATUS_FAIL        = 0x1001;
    //#ifndef ISA

    /* Default vendor and device IDs (0 == all) */
    /* TODO: Replace the ID values with your device's vendor and device IDs */
    public final static int PCI_DEFAULT_VENDOR_ID = 0x0; /* Vendor ID */
    public final static int PCI_DEFAULT_DEVICE_ID = 0x0; /* Device ID */

  //#else /* ifdef ISA */

    /* ISA address spaces information */
    public final static int PCI_ADDR_SPACES_NUM = 0; /* Number of address spaces */

    /* Base address spaces numbers */

    /* Physical base addresses */

    /* Size (in bytes) of address spaces */

  //#ifdef HAS_INTS
    /* Interrupts information */
    public final static int PCI_INTS_NUM = 1; /* Number of interrupts */

    /* Interrupt Request (IRQ) */

    /* Interrupt registration options */
  //#endif /* ifdef HAS_INTS */

    // TODO: Add address space info
  //#endif /* ifdef ISA */

  //#ifdef ISA
//  /* Total number of items - address spaces, interrupts and bus items */
//  public final public static int PCI_ITEMS_NUM = PCI_ADDR_SPACES_NUM + 1;
  //#endif /* ifdef ISA */


    /* Interrupt acknowledgment information */
    /* TODO: Use correct values according to the specification of your device. */
    public final static int INTCSR = 0x00;                     /* Interrupt register */
    public final static int INTCSR_ADDR_SPACE = 0; //AD_PCI_BAR0;   /* Interrupt register's address space */
    public final static int ALL_INT_MASK = 0xFFFFFFFF;         /* Interrupt acknowledgment command */



    /* Kernel PlugIn version information struct */
    public static class KP_PCI_VERSION
    {
        public ByteBuffer data;

        public KP_PCI_VERSION()
        {
            // Make sure that size of buffers that are sent to kp_pci are big
            // enough, otherwise this can lead to BSOD.
            data = ByteBuffer.allocateDirect(wdapi.isWindows ? 120 : 112);
            data.order(java.nio.ByteOrder.nativeOrder());
        }
        public String getVersionString()
        {
            return StandardCharsets.UTF_8.decode(data).toString().substring(
                wdapi.isWindows ? 0 : 8).trim();
        }
        public long getVersion()
        {
            data.position(0);
            return wdapi.getDWORD(data);
        }
    }

    public static class PCI_DEV_ADDR_DESC extends WdcDiagLib.WDC_DEV_ADDR_DESC
    {

        public PCI_DEV_ADDR_DESC()
        {
            // Make sure that size and alignments of buffers that are sent to
            // KP_PCI are correct otherwise this can lead to a BSOD.
            data = ByteBuffer.allocateDirect(wdapi.is64bit ? 16 : 8);
            data.order(java.nio.ByteOrder.nativeOrder());
        }
        public void setdwNumAddrSpaces(int dwNumAddrSpaces)
        {
            wdapi.putDWORD(data, 0, dwNumAddrSpaces);
        }
        public void setpAddrDesc(long pAddrDesc)
        {
            wdapi.putDWORD(data, wdapi.is64bit ? 8 : 4, pAddrDesc);
        }

        public long getdwNumAddrSpaces()
        {
            data.position(0);
            return wdapi.getDWORD(data);
        }
        public long getpAddrDesc()
        {
            data.position(wdapi.is64bit ? 8 : 4);
            return wdapi.getDWORD(data);
        }

    }

  //#ifdef HAS_INTS

    /* TODO: You can add fields to PCI_INT_RESULT to store any additional
             information that you wish to pass to your diagnostics interrupt
             handler routine (DiagIntHandler() in pci_diag.c). */

    /* PCI diagnostics interrupt handler function type */
//  typedef void (*PCI_INT_HANDLER)(WDC_DEVICE_HANDLE hDev,
//      PCI_INT_RESULT *pIntResult);
  //#endif /* ifdef HAS_INTS */


    /*************************************************************
      Internal definitions
     *************************************************************/
    /* WinDriver license registration string */
    /* TODO: When using a registered WinDriver version, replace the license
             string below with the development license in order to use on the
             development machine.
             Once you require to distribute the driver's package to other
             machines, please replace the string with a distribution license */
    public static final String PCI_DEFAULT_LICENSE_STRING = "12345abcde1234.license",

    PCI_DEFAULT_DRIVER_NAME = wdapi.WD_DEFAULT_DRIVER_NAME_BASE,
    KP_PCI_DRIVER_NAME = "KP_PCI";
    public static final int MAX_TYPE = 8;
    public static class PCI_ADDR_SPACE_INFO
    {
        public int dwAddrSpace;
        public String sType;
        public String sName;
        public String sDesc;
    }

    /* PCI device information struct */
    public static class PCI_DEV_CTX extends WdcDiagLib.WDCContext
    {
    //#ifdef HAS_INTS
        public wdapi.WD_TRANSFER[]       pIntTransCmds;
        public wdapi.PCI_INT_HANDLER   funcDiagIntHandler;   /* Interrupt handler routine */
    //#endif /* ifdef HAS_INTS */
    //#ifndef ISA
        public wdapi.PCI_EVENT_HANDLER funcDiagEventHandler; /* Event handler routine */
    //#else /* ifdef ISA */
        //#ifndef HAS_INTS
//        long pData;
        /* TODO: Set pData to point to any device-specific data that you wish to
                 store or replace pData with your own device context information */
        //#endif /* ifndef HAS_INTS */
    //#endif /* ifdef ISA */

    };
    /* TODO: You can add fields to store additional device-specific information. */
	
	public static class MENU_CTX_DMA extends MenuCtx{
        public WD_DMA pDma;
        public ByteBuffer pBuf;
        public long option = 0;
        public int size = 0;
        public long qwAddr = 0;

        public MENU_CTX_DMA(WDC_DEVICE[] devs)
        {
            super(devs);
            pBuf = null;
            pDma = null;
        }
    }


    /*************************************************************
      Global variables definitions
     *************************************************************/
    public static PCI_DEV_CTX devCtx = null;
    public static PCI_DEV_ADDR_DESC devAddrDesc = null;
    /* Last error information string */
    public static String gsPCI_LastErr = "";

    /* Library initialization reference count */
    public static long LibInit_count = 0;

    /*************************************************************
      Static functions prototypes and inline implementation
     *************************************************************/

    /* Validate a device handle */
    public static boolean IsValidDevice(wdapi.WDC_DEVICE pDev, String sFunc)
    {
        if (pDev == null)
        {
            String.format(gsPCI_LastErr, "%s: null device handle\n", sFunc);
            ErrLog(gsPCI_LastErr);
            return false;
        }

        return true;
    }

    /*************************************************************
      Functions implementation
     *************************************************************/
    /* -----------------------------------------------
        PCI and WDC libraries initialize/uninitialize
       ----------------------------------------------- */
    /* Initialize the PCI and WDC libraries */
    public static long PCI_LibInit()
    {
        long dwStatus = 0;

        /* Increase the library's reference count; initialize the library only once
         */
        if (++LibInit_count > 1)
            return wdapi.WD_STATUS_SUCCESS;

        /* Set the driver name */
        if (wdapi.WD_DriverName(PCI_DEFAULT_DRIVER_NAME) == "")
        {
            ErrLog("Failed to set the driver name for WDC library.\n");
            return wdapi.WD_SYSTEM_INTERNAL_ERROR;
        }

        /* Set WDC library's debug options
         * (default: level=TRACE; redirect output to the Debug Monitor) */
        dwStatus = wdapi.WDC_SetDebugOptions(wdapi.WDC_DBG_DEFAULT, "");
        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("Failed to initialize debug options for WDC library.\n" +
                "Error 0x%x - %s\n", dwStatus, wdapi.Stat2Str(dwStatus));
            return dwStatus;
        }

        /* Open a handle to the driver and initialize the WDC library */
        dwStatus = wdapi.WDC_DriverOpen(wdapi.WDC_DRV_OPEN_DEFAULT,
            PCI_DEFAULT_LICENSE_STRING);
        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("Failed to initialize the WDC library. Error 0x%x - %s\n",
                dwStatus, wdapi.Stat2Str(dwStatus));
            return dwStatus;
        }

        return wdapi.WD_STATUS_SUCCESS;
    }

    /* Uninitialize the PCI and WDC libraries */
    public static long PCI_LibUninit()
    {
        long dwStatus;

        /* Decrease the library's reference count; uninitialize the library only
         * when there are no more open handles to the library */
        if (--LibInit_count > 0)
            return wdapi.WD_STATUS_SUCCESS;

        /* Uninitialize the WDC library and close the handle to WinDriver */
        dwStatus = wdapi.WDC_DriverClose();
        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("Failed to uninit the WDC library. Error 0x%x - %s\n",
                dwStatus, wdapi.Stat2Str(dwStatus));
        }

        return dwStatus;
    }

    /* -----------------------------------------------
        Device open/close
       ----------------------------------------------- */
    /* Open a device handle */
    //#ifndef ISA
    public static long PCI_DeviceOpen(long dwVendorId, long dwDeviceId)
    {
        long hDev;
        WDC_DEVICE dev;

        /* Allocate memory for the PCI device context */
        devCtx = new PCI_DEV_CTX();

        /* Allocate memory for the PCI device KP address context */
        devAddrDesc = new PCI_DEV_ADDR_DESC();

        hDev = WdcDiagLib.WDC_DIAG_DeviceFindAndOpen(dwVendorId, dwDeviceId,
            KP_PCI_DRIVER_NAME, devCtx, devAddrDesc);

        if (hDev == 0)
            return 0;

        dev = new WDC_DEVICE(hDev);
        /* Validate device information */
        if (!DeviceValidate(dev))
        {
            PCI_DeviceClose(dev);
            return 0;
        }

        /* Return handle to the new device */
        TraceLog("PCI_DeviceOpen: Opened a PCI device (handle 0x%x)\n" +
            "Device is %s using a Kernel PlugIn driver (%s)\n", hDev,
            (dev.isKP())? "" : "not" , KP_PCI_DRIVER_NAME);
        return hDev;
    }
    //#else /* ifdef ISA */
//  long PCI_DeviceOpen(void)
//    {
//       long dwStatus;
//         WDCResultLong result;
//         WDC_DEVICE dev;
//         Long hDev = null;
//     /*  */
//         PCI_DEV_ADDR_DESC devAddrDesc = new PCI_DEV_ADDR_DESC();
//     /*  */
//
//         /* Allocate memory for the PCI device context */
//         devCtx = new PCI_DEV_CTX();
//
//         /* Set the device's resources information */
//         SetDeviceResources(&deviceInfo);
//
//         /* Open a device handle */
//         dwStatus = wdapi.WDC_IsaDeviceOpen(&hDev, &deviceInfo, pDevCtx);
//
//         if (hDev == 0)
//             return 0;
//
//         dev = new WDC_DEVICE(hDev);
//         if (wdapi.WD_STATUS_SUCCESS != dwStatus)
//         {
//             ErrLog("Failed opening a WDC device handle. Error 0x%x - %s\n",
//                 dwStatus, wdapi.Stat2Str(dwStatus));
//             PCI_DeviceClose(dev);
//            return 0;
//        }
//         System.out.println("Device successfully opened!");
//
//         dev = new WDC_DEVICE(hDev);
//
//     /*  */
//        devAddrDesc.setdwNumAddrSpaces(dev.dwNumAddrSpaces);
//        devAddrDesc.setpAddrDesc(dev.pAddrDesc_addr);
//         /* Open a handle to a Kernel PlugIn driver */
//         dwStatus = wdapi.WDC_KernelPlugInOpen(hDev, KP_PCI_DRIVER_NAME, devAddrDesc.data);
//
//         if (dwStatus == wdapi.WD_STATUS_SUCCESS)
//             dev.updateDev(hDev);
//
//     /*  */
//
//         /* Return handle to the new device */
//         TraceLog("PCI_DeviceOpen: Opened a PCI device (handle 0x%x)\n" +
//             "Device is %s using a Kernel PlugIn driver (%s)\n", hDev,
//             (dev.isKP())? "" : "not" , KP_PCI_DRIVER_NAME);
//         return hDev;
//    }
  //#endif /* ifdef ISA */

    /* Close device handle */
    public static boolean PCI_DeviceClose(WDC_DEVICE pDev)
    {
        long hDev = pDev.hDev;
        long dwStatus;

        TraceLog("PCI_DeviceClose: Entered. Device handle 0x%x\n", hDev);

        /* Validate the device handle */
        if (hDev == 0)
        {
            ErrLog("PCI_DeviceClose: Error - null device handle\n");
            return false;
        }

    //#ifdef HAS_INTS
        /* Disable interrupts (if enabled) */
        if (wdapi.WDC_IntIsEnabled(hDev))
        {
            dwStatus = PCI_IntDisable(pDev);
            if (wdapi.WD_STATUS_SUCCESS != dwStatus)
            {
                ErrLog("Failed disabling interrupts. Error 0x%x - %s\n", dwStatus,
                    wdapi.Stat2Str(dwStatus));
            }
        }
    //#endif /* ifdef HAS_INTS */

        /* Detach PCI device context memory */
        devCtx = null;
        devAddrDesc = null;

        return WdcDiagLib.WDC_DIAG_DeviceClose(hDev);
    }

    //#ifndef ISA

    /* Validate device information */
    public static boolean DeviceValidate(WDC_DEVICE pDev)
    {
        int i, dwNumAddrSpaces = pDev.dwNumAddrSpaces;

        /* NOTE: You can modify the implementation of this function in order to
                 verify that the device has the resources you expect to find. */

        /* Verify that the device has at least one active address space */
        for (i = 0; i < dwNumAddrSpaces; i++)
        {
            if (wdapi.WDC_AddrSpaceIsActive(pDev, i))
                return true;
        }

        /* In this sample we accept the device even if it doesn't have any
         * address spaces */
        TraceLog("Device does not have any active memory or I/O address spaces\n");
        return true;
    }
    //#else /* ifdef ISA */
//  public static void SetDeviceResources(WD_CARD *pDeviceInfo)
//  {
//      wdapi.WD_ITEMS *pItem;
//
//      BZERO(*pDeviceInfo);
//
//      pDeviceInfo->dwItems = PCI_ITEMS_NUM;
//      pItem = &pDeviceInfo->Item[0];
//      /* Bus */
//      pItem[0].item = ITEM_BUS;
//      pItem[0].I.Bus.dwBusType = WD_BUS_ISA;
//
//  /*  */
//  }
  //#endif /* ifdef ISA */


    // #if !defined(ISA) && defined(LINUX)
    public static long PCI_SriovEnable(WDC_DEVICE pDev, long dwNumVFs)
    {
        long hDev = pDev.hDev;
        TraceLog("PCI_SriovEnable entered. Device handle: 0x%x\n", hDev);

        /* Validate the device handle */
        if (!IsValidDevice(pDev, "PCI_SriovEnable"))
            return wdapi.WD_INVALID_PARAMETER;

        return wdapi.WDC_PciSriovEnable(hDev, dwNumVFs);
    }

    public static long PCI_SriovDisable(WDC_DEVICE pDev)
    {
        long hDev = pDev.hDev;
        TraceLog("PCI_SriovDisable entered. Device handle: 0x%x\n", hDev);

        /* Validate the device handle */
        if (!IsValidDevice(pDev, "PCI_SriovDisable"))
            return wdapi.WD_INVALID_PARAMETER;

        return wdapi.WDC_PciSriovDisable(hDev);
    }

    public static WDCResultLong PCI_SriovGetNumVFs(WDC_DEVICE pDev)
    {
        WDCResultLong result = new WDCResultLong(0, 0);
        long hDev = pDev.hDev;

        TraceLog("PCI_SriovGetNumVFs entered. Device handle: 0x%x\n", hDev);

        /* Validate the device handle */
        if (!IsValidDevice(pDev, "PCI_SriovGetNumVfs"))
        {
            result.dwStatus = wdapi.WD_INVALID_PARAMETER;
            return result;
        }
        result = wdapi.WDC_PciSriovGetNumVFs(hDev);

        return result;
    }
    //#endif
	
    //#ifdef HAS_INTS

    /* -----------------------------------------------
        Interrupts
       ----------------------------------------------- */
    /* Check whether a given device contains an item of the specified type */
    public static boolean IsItemExists(WDC_DEVICE pDev, long item)
    {
        long dwNumItems = pDev.cardReg.Card.dwItems;

        for (int i = 0; i < dwNumItems; i++)
        {
            if ((pDev.cardReg.Card.Item[i].item) == item)
                return true;
        }

        return false;
    }

    /* Enable interrupts */
    public static long PCI_IntEnable(WDC_DEVICE pDev,
        PCI_INT_HANDLER funcIntHandler)
    {
        long dwStatus;
        long hDev = pDev.hDev;

        WDC_ADDR_DESC pAddrDesc;
        WD_TRANSFER[] pTrans = null;
        long dwNumTransCmds = 0;
        long dwOptions = 0;


        TraceLog("PCI_IntEnable: Entered. Device handle 0x%x\n", hDev);

        /* Validate the device handle */
        if (!IsValidDevice(pDev, "PCI_IntEnable"))
            return wdapi.WD_INVALID_PARAMETER;

        /* Verify that the device has an interrupt item */
        if (!IsItemExists(pDev, wdapi.ITEM_INTERRUPT))
            return wdapi.WD_OPERATION_FAILED;

//      pDevCtx = PciDiag.dev.pCtx;

        /* Check whether interrupts are already enabled */
        if (wdapi.WDC_IntIsEnabled(hDev))
        {
            ErrLog("Interrupts are already enabled ...\n");
            return wdapi.WD_OPERATION_ALREADY_DONE;
        }



        /* When using a Kernel PlugIn, acknowledge interrupts in kernel mode */
        if (!pDev.isKP())
        {
            /* TODO: Change this value, if needed */
            dwNumTransCmds = 2;

            /* This sample demonstrates how to set up two transfer commands, one
             * for reading the device's INTCSR register (as defined in gPCI_Regs)
             * and one for writing to it to acknowledge the interrupt. The transfer
             * commands will be executed by WinDriver in the kernel when an
             * interrupt occurs.*/
    //#ifndef ISA
            /* TODO: If PCI interrupts are level sensitive interrupts, they must be
             * acknowledged in the kernel immediately when they are received. Since
             * the information for acknowledging the interrupts is
             * hardware-specific, YOU MUST MODIFY THE CODE below and set up transfer
             * commands in order to correctly acknowledge the interrupts on your
             * device, as dictated by your hardware's specifications.
             * If the device supports both MSI/MSI-X and level sensitive interrupts,
             * you must set up transfer commands in order to allow your code to run
             * correctly on systems other than Windows Vista and higher and Linux.
             * Since MSI/MSI-X does not require acknowledgment of the interrupt, to
             * support only MSI/MSI-X handling (for hardware and OSs that support
             * this), you can avoid defining transfer commands, or specify
             * kernel-mode commands to be performed upon interrupt generation
             * according to your specific needs. */
    //#endif /* ifndef ISA */

            /******************************************************************
             * NOTE: If you attempt to use this code without first modifying it in
             * order to correctly acknowledge your device's level-sensitive
             * interrupts, as explained above, the OS will HANG when a level
             * sensitive interrupt occurs!
             ********************************************************************/

            /* Allocate memory for the interrupt transfer commands */
            pTrans = new WD_TRANSFER[(int)dwNumTransCmds];
            pTrans[0] = new WD_TRANSFER();
            pTrans[1] = new WD_TRANSFER();

            /* Prepare the interrupt transfer commands.
             *
             * The transfer commands will be executed by WinDriver's ISR
             * which runs in kernel mode at interrupt level.
             */

            /* TODO: Change the offset of INTCSR and the PCI address space, if
             *       needed */
            /* #1: Read status from the INTCSR register */
            pAddrDesc = pDev.getAddrDesc(INTCSR_ADDR_SPACE);

            pTrans[0].pPort = pAddrDesc.pAddr + INTCSR;
            /* Read from a 32-bit register */
            pTrans[0].cmdTrans = wdapi.WDC_ADDR_IS_MEM(pAddrDesc) ?
                    wdapi.RM_long : wdapi.RP_long;

            /* #2: Write ALL_INT_MASK to the INTCSR register to acknowledge the
             *     interrupt */
            pTrans[1].pPort = pTrans[0].pPort; /* In this example both commands
                                                    access the same address
                                                    (register) */
            /* Write to a 32-bit register */
            pTrans[1].cmdTrans = wdapi.WDC_ADDR_IS_MEM(pAddrDesc) ?
                    wdapi.WM_long : wdapi.WP_long;
            pTrans[1].Data = ALL_INT_MASK;

            /* Copy the results of "read" transfer commands back to user mode */
            dwOptions = wdapi.INTERRUPT_CMD_COPY;
        }

        /* Store the diag interrupt handler routine, which will be executed
         * when an interrupt is received */
        devCtx.funcDiagIntHandler = funcIntHandler;

        /* Enable interrupts */
        dwStatus = wdapi.WDC_IntEnable(hDev, pTrans, 0 /*dwNumTransCmds*/,
            dwOptions, devCtx.funcDiagIntHandler,
            new String("Customizable Device Context"), pDev.isKP());
        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("Failed enabling interrupts. Error 0x%x - %s\n", dwStatus,
                wdapi.Stat2Str(dwStatus));

//          if (pTrans)
//              free(pTrans);

            return dwStatus;
        }

        /* Store the interrupt transfer commands in the device context */
//      pDevCtx->pIntTransCmds = pTrans;


        /* TODO: You can add code here to write to the device in order to
                 physically enable the hardware interrupts. */

        TraceLog("PCI_IntEnable: Interrupts enabled\n");

        return wdapi.WD_STATUS_SUCCESS;
    }

    /* Disable interrupts */
    public static long PCI_IntDisable(WDC_DEVICE pDev)
    {
        long hDev = pDev.hDev;
        long dwStatus;

        TraceLog("PCI_IntDisable entered. Device handle 0x%x\n", hDev);

        /* Validate the device handle */
        if (!IsValidDevice(pDev, "PCI_IntDisable"))
            return wdapi.WD_INVALID_PARAMETER;

        /* Check whether interrupts are already enabled */
        if (!wdapi.WDC_IntIsEnabled(hDev))
        {
            ErrLog("Interrupts are already disabled ...\n");
            return wdapi.WD_OPERATION_ALREADY_DONE;
        }

        /* TODO: You can add code here to write to the device in order to
                 physically disable the hardware interrupts. */

        /* Disable interrupts */
        dwStatus = wdapi.WDC_IntDisable(hDev);
        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("Failed disabling interrupts. Error 0x%x - %s\n",
                dwStatus, wdapi.Stat2Str(dwStatus));
        }
        /* Free the memory allocated for the interrupt transfer commands */
//      if (pDevCtx.pIntTransCmds)
//      {
//          free(pDevCtx->pIntTransCmds);
//          pDevCtx.pIntTransCmds = null;
//      }

        return dwStatus;
    }

    /* Check whether interrupts are enabled for the given device */
    public static boolean PCI_IntIsEnabled(long hDev)
    {
        /* Validate the device handle */
        if (hDev == 0)
            return false;

        /* Check whether interrupts are already enabled */
        return wdapi.WDC_IntIsEnabled(hDev);
    }
  //#endif /* ifdef HAS_INTS */

  //#ifndef ISA

    /* -----------------------------------------------
        Plug-and-play and power management events
       ----------------------------------------------- */

    /* Register a plug-and-play or power management event */
    public static long PCI_EventRegister(WDC_DEVICE pDev,
        PCI_EVENT_HANDLER funcEventHandler)
    {
        long dwStatus;

        long dwActions = wdapi.WD_ACTIONS_ALL;
        /* TODO: Modify the above to set up the plug-and-play/power management
                 events for which you wish to receive notifications.
                 dwActions can be set to any combination of the WD_EVENT_ACTION
                 flags defined in windrvr.h. */

        TraceLog("PCI_EventRegister entered. Device handle 0x%x\n", pDev.hDev);

        /* Validate the device handle */
        if (!IsValidDevice(pDev, "PCI_EventRegister"))
            return wdapi.WD_INVALID_PARAMETER;

//      devCtx = (PPCI_DEV_CTX)(pDev->pCtx);

        /* Check whether the event is already registered */
        if (wdapi.WDC_EventIsRegistered(pDev.hDev))
        {
            ErrLog("Events are already registered ...\n");
            return wdapi.WD_OPERATION_ALREADY_DONE;
        }

        /* Store the diag event handler routine to be executed from
         * PCI_EventHandler() upon an event */
        devCtx.funcDiagEventHandler = funcEventHandler;

        /* Register the event */
        dwStatus = wdapi.WDC_EventRegister(pDev.hDev, dwActions,
            funcEventHandler, pDev.hDev, pDev.isKP());

        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("Failed to register events. Error 0x%x - %s\n",
                dwStatus, wdapi.Stat2Str(dwStatus));
            return dwStatus;
        }

        TraceLog("Events registered\n");
        return wdapi.WD_STATUS_SUCCESS;
    }

    /* Unregister a plug-and-play or power management event */
    public static long PCI_EventUnregister(WDC_DEVICE pDev)
    {
        long dwStatus;

        TraceLog("PCI_EventUnregister entered. Device handle 0x%x\n",
            pDev.hDev);

        /* Validate the device handle */
        if (!IsValidDevice(pDev, "PCI_EventUnregister"))
            return wdapi.WD_INVALID_PARAMETER;

        /* Check whether the event is currently registered */
        if (!wdapi.WDC_EventIsRegistered(pDev.hDev))
        {
            ErrLog("Cannot unregister events - no events currently " +
                "registered ...\n");
            return wdapi.WD_OPERATION_ALREADY_DONE;
        }

        /* Unregister the event */
        dwStatus = wdapi.WDC_EventUnregister(pDev.hDev);

        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("Failed to unregister events. Error 0x%x - %s\n", dwStatus,
                wdapi.Stat2Str(dwStatus));
        }

        return dwStatus;
    }

    /* Check whether a given plug-and-play or power management event is registered
     */
    public static boolean PCI_EventIsRegistered(long hDev)
    {
        /* Validate the device handle */
        if (hDev == 0)
            return false;

        /* Check whether the event is registered */
        return wdapi.WDC_EventIsRegistered(hDev);
    }
    //#endif /* ifndef ISA */

	
	/* -----------------------------------------------
	DMA
   ----------------------------------------------- */
	public static WDCResult PCI_DmaAllocContig(MENU_CTX_DMA pDmaMenusCtx)
	{
		WDCResult result = wdapi.WDC_DMAContigBufLock(pDmaMenusCtx.devs[0].hDev,
            0 /* dwOptions */, pDmaMenusCtx.size);
        pDmaMenusCtx.pBuf = ((WDCResultDMA)result).pBuf;
        if (wdapi.WD_STATUS_SUCCESS != result.dwStatus ||
            pDmaMenusCtx.pBuf == null)
		{
			ErrLog("Failed allocating contiguous memory."
                + " size [%d], Error [0x%x - %s]\n", pDmaMenusCtx.size,
                result.dwStatus, wdapi.Stat2Str(result.dwStatus));
		}

		return result;
	}

	public static WDCResult PCI_DmaAllocSg(MENU_CTX_DMA pDmaMenusCtx)
	{
		WDCResult result = wdapi.WDC_DMASGBufLock(pDmaMenusCtx.devs[0].hDev,
           pDmaMenusCtx.pBuf, 0 /* dwOptions */, pDmaMenusCtx.size);
			
		if (wdapi.WD_STATUS_SUCCESS != result.dwStatus)
		{
			 ErrLog("Failed allocating SG memory. size [%d], "
                + "Error [0x%x - %s]\n", pDmaMenusCtx.size, result.dwStatus,
                wdapi.Stat2Str(result.dwStatus));
		}

		return result;
	}

	public static WDCResult PCI_DmaAllocReserved(MENU_CTX_DMA pDmaMenusCtx)
	{
		WDCResult result = wdapi.WDC_DMAReservedBufLock(
            pDmaMenusCtx.devs[0].hDev, pDmaMenusCtx.qwAddr, 0 /* dwOptions */,
            pDmaMenusCtx.size);
        pDmaMenusCtx.pBuf = ((WDCResultDMA)result).pBuf;
	
        if (wdapi.WD_STATUS_SUCCESS != result.dwStatus ||
            pDmaMenusCtx.pBuf == null)
        {
            ErrLog("Failed claiming reserved memory. size" +
                " [%d], Error [0x%x - %s]\n", pDmaMenusCtx.size, result.dwStatus,
                wdapi.Stat2Str(result.dwStatus));
        }
		return result;
	}

	public static long PCI_DmaBufUnlock(WD_DMA dma)
	{
		long dwStatus = wdapi.WDC_DMABufUnlock(dma);
		if (wdapi.WD_STATUS_SUCCESS != dwStatus)
		{
            ErrLog("Failed trying to free DMA memory. Error [0x%x "
                + "- %s]\n", dwStatus, wdapi.Stat2Str(dwStatus));
        }
		
		return dwStatus;
	}

    /* -----------------------------------------------
        Debugging and error handling
       ----------------------------------------------- */
    /* Log a debug error message */
    public static void ErrLog(String sFormat, Object ... argp)
    {
        gsPCI_LastErr = String.format(sFormat, argp);
        wdapi.WDC_Err("PCI lib (Java): "+ gsPCI_LastErr);
    }

    /* Log a debug trace message */
    public static void TraceLog(String sFormat, Object ... argp)
    {
        String sMsg = String.format(sFormat, argp);
        wdapi.WDC_Trace("PCI lib (Java): " + sMsg);
    }

    /* Get last error */
    public static String PCI_GetLastErr()
    {
        return gsPCI_LastErr;
    }

}


