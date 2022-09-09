/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/************************************************************************
*  File: PciDiag.java
*
*  Sample user-mode diagnostics application for accessing PCI
*  devices, possibly via a Kernel PlugIn driver using WinDriver's API.

*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*************************************************************************/

package com.jungo.PciDiag;

import com.jungo.*;
import com.jungo.wdapi.*;
import com.jungo.shared.*;
import com.jungo.shared.DiagLib.*;
import com.jungo.shared.PciMenusCommon.*;
import com.jungo.shared.PciLib.*;
import java.nio.ByteBuffer;
//import sun.misc.Cleaner;
//import sun.nio.ch.DirectBuffer;

public class PciDiag {

    /*************************************************************
      Global variables
     *************************************************************/
    public static final int MAX_DEVICES = 16;

//     A member that keeps all info of the open devices
    public static WDC_DEVICE[] devs = new WDC_DEVICE[MAX_DEVICES];

     //#ifndef ISA
    /* --------------------------------------------------
        PCI configuration registers information
       -------------------------------------------------- */
    /* Configuration registers information array */
    static final WdcDiagLib.WDC_REG gPCI_CfgRegs[] = {
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_VID,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "VID", "Vendor ID"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_DID,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "DID", "Device ID"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_CR,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "CMD", "Command"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_SR,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "STS", "Status"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_REV,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "RID_CLCD",
            "Revision ID &\nClass Code"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_CCSC,
            wdapi.WDC_SIZE_8, wdapi.WDC_READ_WRITE, "SCC", "Sub Class Code"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_CCBC,
            wdapi.WDC_SIZE_8, wdapi.WDC_READ_WRITE, "BCC", "Base Class Code"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_CLSR,
            wdapi.WDC_SIZE_8, wdapi.WDC_READ_WRITE, "CALN", "Cache Line Size"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_LTR,
            wdapi.WDC_SIZE_8, wdapi.WDC_READ_WRITE, "LAT", "Latency Timer"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_HDR,
            wdapi.WDC_SIZE_8, wdapi.WDC_READ_WRITE, "HDR", "Header Type"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_BISTR,
            wdapi.WDC_SIZE_8, wdapi.WDC_READ_WRITE, "BIST",
            "Built-in Self Test"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_BAR0,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "BADDR0", "Base Address 0"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_BAR1,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "BADDR1", "Base Address 1"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_BAR2,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "BADDR2", "Base Address 2"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_BAR3,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "BADDR3", "Base Address 3"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_BAR4,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "BADDR4", "Base Address 4"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_BAR5,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "BADDR5", "Base Address 5"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_CIS,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "CIS",
            "CardBus CIS\npointer"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_SVID,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "SVID",
            "Sub-system\nVendor ID"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_SDID,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "SDID",
            "Sub-system\nDevice ID"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_EROM,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "EROM",
            "Expansion ROM\nBase Address"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_CAP,
            wdapi.WDC_SIZE_8, wdapi.WDC_READ_WRITE, "NEW_CAP",
            "New Capabilities\nPointer"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_ILR,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "INTLN", "Interrupt Line"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_IPR,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "INTPIN", "Interrupt Pin"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_MGR,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "MINGNT",
            "Minimum Required\nBurst Period"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCI_MLR,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "MAXLAT",
            "Maximum Latency"),
    };

    static final WdcDiagLib.WDC_REG gPCI_ext_CfgRegs[] = {
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.PCIE_CAP_ID,
            wdapi.WDC_SIZE_8, wdapi.WDC_READ_WRITE, "PCIE_CAP_ID",
            "PCI Express\nCapability ID"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.NEXT_CAP_PTR,
            wdapi.WDC_SIZE_8, wdapi.WDC_READ_WRITE, "NEXT_CAP_PTR",
            "Next Capabiliy Pointer"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.CAP_REG,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "CAP_REG",
            "Capabilities Register"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.DEV_CAPS,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "DEV_CAPS",
            "Device Capabilities"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.DEV_CTL,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "DEV_CTL",
            "Device Control"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.DEV_STS,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "DEV_STS",
            "Device Status"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.LNK_CAPS,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "LNK_CAPS",
            "Link Capabilities"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.LNK_CTL,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "LNK_CTL",
            "Link Control"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.LNK_STS,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "LNK_STS",
            "Link Status"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.SLOT_CAPS,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "SLOT_CAPS",
            "Slot Capabilities"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.SLOT_CTL,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "SLOT_CTL",
            "Slot Control"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.SLOT_STS,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "SLOT_STS",
            "Slot Status"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.ROOT_CAPS,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "ROOT_CAPS",
            "Root Capabilities"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.ROOT_CTL,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "ROOT_CTL",
            "Root Control"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.ROOT_STS,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "ROOT_STS",
            "Root Status"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.DEV_CAPS2,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "DEV_CAPS2",
            "Device Capabilities 2"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.DEV_CTL2,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "DEV_CTL2",
            "Device Control 2"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.DEV_STS2,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "DEV_STS2",
            "Device Status 2"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.LNK_CAPS2,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "LNK_CAPS2",
            "Link Capabilities 2"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.LNK_CTL2,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "LNK_CTL2",
            "Link Control 2"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.LNK_STS2,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "LNK_STS2",
            "Link Status 2"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.SLOT_CAPS2,
            wdapi.WDC_SIZE_32, wdapi.WDC_READ_WRITE, "SLOT_CAPS2",
            "Slot Capabilities 2"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.SLOT_CTL2,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "SLOT_CTL2",
            "Slot Control 2"),
        new WdcDiagLib.WDC_REG(wdapi.WDC_AD_CFG_SPACE, PciRegs.SLOT_STS2,
            wdapi.WDC_SIZE_16, wdapi.WDC_READ_WRITE, "SLOT_STS2",
            "Slot Status 2"),
    };


    final static String WD_PROD_NAME = "WinDriver";
    private static final long PCI_DEFAULT_VENDOR_ID = 0x0;
    private static final long PCI_DEFAULT_DEVICE_ID = 0x0;

    /*************************************************************
      Functions implementation
     *************************************************************/
    public static long PCI_Init()
    {
        long dwStatus, hDev = 0;


        /* Initialize the PCI library */
        dwStatus = PciLib.PCI_LibInit();
        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            System.err.printf("pci_diag: Failed to initialize the PCI library:"
                + " %s", PciLib.PCI_GetLastErr());
            System.exit((int)dwStatus);
        }

     //#ifndef ISA
        wdapi.PrintDbgMessage(wdapi.D_ERROR, wdapi.S_PCI,
            "WinDriver user mode version " + wdapi.WD_VERSION_STR + "\n");

        /* Find and open a PCI device (by default ID) */
        if (PCI_DEFAULT_VENDOR_ID != 0)
            hDev = PciLib.PCI_DeviceOpen(PCI_DEFAULT_VENDOR_ID,
                    PCI_DEFAULT_DEVICE_ID);
     //#else /* ifdef ISA */
//      /* Open a handle to the device */
//      hDev = PciLib.PCI_DeviceOpen();
//      if (hDev != 0)
//      {
//          System.err.printf("pci_diag: Failed opening a handle to the device: %s"/*,
//              PCI_GetLastErr()*/);
//          System.exit(1);
//      }
     //#endif /* ifdef ISA */

        if (hDev != 0)
        {
            devs[0] = new WDC_DEVICE(hDev);

                /* Get Kernel PlugIn driver version */
                if (devs[0].isKP())
                    CheckKPDriverVer(hDev);
        }

        return dwStatus;
    }

    public static void main(String[] args)
    {
        long dwStatus;
        DiagMenuOption pMenuRoot;

        System.out.printf("\n");
        System.out.printf("PCI diagnostic utility.\n");
        System.out.printf("Application accesses hardware using " + WD_PROD_NAME
            + "\n");
        System.out.printf("and a Kernel PlugIn driver (%s).\n", PciLib.KP_PCI_DRIVER_NAME);

        dwStatus = PCI_Init();
        if (dwStatus != 0)
            System.exit((int)dwStatus);

        /* Display main diagnostics menu for communicating with the device */
        pMenuRoot = MenuMainInit();

        /* Busy loop that runs the menu tree created above and communicates
        with the user */
        System.exit((int)DiagLib.DIAG_MenuRun(pMenuRoot));
    }

    static long MenuMainExitCb(Object pCbCtx)
    {
        /* Perform necessary cleanup before exiting the program: */
        /* Close the device handle */
        long dwStatus;

        for(WDC_DEVICE dev : devs)
        {
        //#ifndef ISA

            if (dev != null)
            {
                /* Disable SR-IOV if enabled */
                WDCResultLong result = PciLib.PCI_SriovGetNumVFs(dev);
                if (result.dwStatus == 0 && result.result > 0)
                {
                    dwStatus = PciLib.PCI_SriovDisable(dev);
                    if (dwStatus != 0)
                    {
                        System.err.printf("pci_diag: Failed Disabling SR-IOV: %s\n",
                                wdapi.Stat2Str(dwStatus));
                    }
                }

                PciLib.PCI_DeviceClose(dev);
            }
        //#else /* ifdef ISA */
        //      if (dev != 0 && PciLib.PCI_DeviceClose(dev))
        //      {
        //          System.err.printf("pci_diag: Failed closing ISA device: %s"/*,
        //              PCI_GetLastErr()*/);
        //      }
        //#endif /* ifdef ISA */

        }


        //#ifndef ISA
        if (wdapi.WDS_IsIpcRegistered())
            wdapi.WDS_IpcUnRegister();
        //#endif /* ifndef ISA */

        /* Uninitialize libraries */
        dwStatus = PciLib.PCI_LibUninit();
        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            System.err.printf("pci_diag: Failed to uninitialize the PCI "
                + "library: %s", PciLib.PCI_GetLastErr());
        }

        return dwStatus;
    }

    /* -----------------------------------------------
        Main diagnostics menu
        ----------------------------------------------- */
    static DiagMenuOption MenuMainInit()
    {
        DiagMenuOption menuRoot = new DiagMenuOptionBuilder()
            .cTitleName("PCI main menu")
            .cbExit((pCbCtx)->MenuMainExitCb(pCbCtx))
            .build();

        // #ifndef ISA
        PciMenusCommon.MenuCommonScanBusInit(menuRoot);
        MenuDeviceOpenInit(menuRoot);
        WdsDiagLib.MenuSharedBufferInit(menuRoot);
        WdsDiagLib.MenuIpcInit(menuRoot);
        MenuCfgInit(menuRoot);
        MenuEventsInit(menuRoot);
        //#endif /* ifndef ISA */

        MenuReadWriteAddrInit(menuRoot);

        //#ifdef HAS_INTS
        MenuInterruptsInit(menuRoot);
        //#endif /* ifdef HAS_INTS */

        MenuDmaInit(menuRoot);

        // #ifndef ISA
        // #ifdef LINUX
        if(wdapi.isLinux64 || wdapi.isLinux32)
            MenuSriovInit(menuRoot);
        //#endif /* ifdef LINUX */
        //#endif /* ifndef ISA */


        return menuRoot;
    }

    // #ifndef ISA
    static void MenuDeviceOpenInit(DiagMenuOption pParentMenu)
    {
        DiagMenuOption menuDeviceOpen =
            new DiagMenuOptionBuilder()
            .cOptionName("Find and open a PCI device")
            .cbEntry((pCbCtx)-> {
                if (devs[0] != null)
                {
                    /* Disable SR-IOV if enabled */
                    WDCResultLong result = PciLib.PCI_SriovGetNumVFs(devs[0]);
                    if (result.dwStatus == 0 && result.result > 0)
                    {
                        long dwStatus = PciLib.PCI_SriovDisable(devs[0]);
                        if (dwStatus != 0)
                        {
                            System.err.printf("pci_diag: Failed Disabling " +
                                "SR-IOV: %s\n", wdapi.Stat2Str(dwStatus));
                        }
                    }
                    PciLib.PCI_DeviceClose(devs[0]);
                    devs[0] = null;
                }

                long hDev = PciLib.PCI_DeviceOpen(0, 0);
                if (hDev != 0)
                {
                    devs[0] = new WDC_DEVICE(hDev);

                    /* Get Kernel PlugIn driver version */
                    if (devs[0].isKP())
                        CheckKPDriverVer(hDev);
                }

                return wdapi.WD_STATUS_SUCCESS;
            })
        .build();

        pParentMenu.DIAG_AddMenuOptionChild(menuDeviceOpen);
    }

    /* -----------------------------------------------
        Read/write the configuration space
        ----------------------------------------------- */
    static void MenuCfgInit(DiagMenuOption pParentMenu)
    {
        MenuCtxCfg cfgCtx = new MenuCtxCfg(
                devs, gPCI_CfgRegs, gPCI_ext_CfgRegs);

        PciMenusCommon.MenuCommonCfgInit(pParentMenu, cfgCtx);
    }

    /* ----------------------------------------------------
        Plug-and-play and power management events handling
        ---------------------------------------------------- */
    /* Diagnostics plug-and-play and power management events handler routine */
    static void DiagEventHandler(long hDev, int dwAction)
    {
        /* TODO: You can modify this function in order to implement your own
         *       diagnostics events handler routine. */

        System.out.printf("\nReceived event notification (device handle 0x%x): ",
            hDev);
        switch (dwAction)
        {
        case wdapi.WD_INSERT:
            System.out.printf("WD_INSERT\n");
            break;
        case wdapi.WD_REMOVE:
            System.out.printf("WD_REMOVE\n");
            break;
        case wdapi.WD_POWER_CHANGED_D0:
            System.out.printf("WD_POWER_CHANGED_D0\n");
            break;
        case wdapi.WD_POWER_CHANGED_D1:
            System.out.printf("WD_POWER_CHANGED_D1\n");
            break;
        case wdapi.WD_POWER_CHANGED_D2:
            System.out.printf("WD_POWER_CHANGED_D2\n");
            break;
        case wdapi.WD_POWER_CHANGED_D3:
            System.out.printf("WD_POWER_CHANGED_D3\n");
            break;
        case wdapi.WD_POWER_SYSTEM_WORKING:
            System.out.printf("WD_POWER_SYSTEM_WORKING\n");
            break;
        case wdapi.WD_POWER_SYSTEM_SLEEPING1:
            System.out.printf("WD_POWER_SYSTEM_SLEEPING1\n");
            break;
        case wdapi.WD_POWER_SYSTEM_SLEEPING2:
            System.out.printf("WD_POWER_SYSTEM_SLEEPING2\n");
            break;
        case wdapi.WD_POWER_SYSTEM_SLEEPING3:
            System.out.printf("WD_POWER_SYSTEM_SLEEPING3\n");
            break;
        case wdapi.WD_POWER_SYSTEM_HIBERNATE:
            System.out.printf("WD_POWER_SYSTEM_HIBERNATE\n");
            break;
        case wdapi.WD_POWER_SYSTEM_SHUTDOWN:
            System.out.printf("WD_POWER_SYSTEM_SHUTDOWN\n");
            break;
        default:
            System.out.printf("0x%x\n", dwAction);
            break;
        }
    }

    static long MenuEventsRegisterCb(MenuCtxEvents pEventsCtx)
    {
        if (wdapi.WD_STATUS_SUCCESS ==
                PciLib.PCI_EventRegister(pEventsCtx.devs[0],
                        pEventsCtx.DiagEventHandler))
        {
            System.out.printf("Events registered\n");
            pEventsCtx.fRegistered = true;
        }
        else
        {
            System.err.printf("Failed to register events. Last" +
                " error [%s]", PciLib.PCI_GetLastErr());
        }

        return wdapi.WD_STATUS_SUCCESS;
    }

    static long MenuEventsUnregisterCb(MenuCtxEvents pEventsCtx)
    {
        if (wdapi.WD_STATUS_SUCCESS == PciLib.PCI_EventUnregister(
            pEventsCtx.devs[0]))
        {
            System.out.printf("Events unregistered\n");
            pEventsCtx.fRegistered = false;
        }
        else
        {
            System.err.printf("Failed to unregister events. Last" +
                " error [%s]", PciLib.PCI_GetLastErr());
        }

        return wdapi.WD_STATUS_SUCCESS;
    }


    static long MenuEventsCb(MenuCtxEvents pEventsCtx)
    {
        pEventsCtx.fRegistered = PciLib.PCI_EventIsRegistered(
                pEventsCtx.devs[0].hDev);

        if (wdapi.isWindows && !pEventsCtx.fRegistered)
        {
            System.out.printf("\n");
            System.out.printf(
                    "NOTICE: An INF must be installed for your device in order"
                         + " to \n" +
                "        call your user-mode event handler.\n" +
                "        You can generate an INF file using the DriverWizard.");
        }

        return wdapi.WD_STATUS_SUCCESS;
    }

    static void MenuEventsInit(DiagMenuOption pParentMenu)
    {
        MenuCtxEvents eventsCtx =
                new MenuCtxEvents(devs,
                        new PCI_EVENT_HANDLER()
                {
                    @Override
                    public void run(PCI_EVENT_RESULT eventResult,
                        Object ctx) {
                        DiagEventHandler(eventResult.hDev,
                            eventResult.dwAction);
                    }
                });

        MenuEventsCallbacks eventsCallbacks = new
            MenuEventsCallbacks();

        eventsCallbacks.eventsEnableCb = ((pCbCtx)->MenuEventsRegisterCb(
            (MenuCtxEvents)pCbCtx));
        eventsCallbacks.eventsDisableCb = ((pCbCtx)->MenuEventsUnregisterCb(
            (MenuCtxEvents)pCbCtx));
        eventsCallbacks.eventsMenuEntryCb = ((pCbCtx)->MenuEventsCb(
            (MenuCtxEvents)pCbCtx));


        PciMenusCommon.MenuCommonEventsInit(pParentMenu, eventsCtx,
            eventsCallbacks);
    }

    /* -----------------------------------------------
        SRIOV handling
    ----------------------------------------------- */
    //#ifdef LINUX
    static long MenuSriovEnableCb(MenuCtxSriov pSriovCtx)
    {
        long dwDesiredNumVFs, dwStatus;
        WDCResultLong desiredNumVFsResult;

        desiredNumVFsResult = DiagLib.DIAG_InputNum("How many Virtual" +
            " Functions would you like to enable:", false, 0, 0);

        if (DiagLib.DIAG_INPUT_SUCCESS != desiredNumVFsResult.dwStatus)
            return desiredNumVFsResult.dwStatus;

        dwDesiredNumVFs = ((WDCResultLong)desiredNumVFsResult).result;

        dwStatus =  PciLib.PCI_SriovEnable(pSriovCtx.devs[0], dwDesiredNumVFs);
        if (wdapi.WD_STATUS_SUCCESS == dwStatus)
        {
            System.out.printf("SR-IOV enabled successfully\n");
        }
        else
        {
            System.err.printf("Failed enabling SR-IOV. Error" +
                " [0x%x - %s]\n", dwStatus, wdapi.Stat2Str(dwStatus));
        }

        return dwStatus;
    }

    static long MenuSriovDisableCb(MenuCtxSriov pSriovCtx)
    {
        long dwStatus = PciLib.PCI_SriovDisable(pSriovCtx.devs[0]);
        if (wdapi.WD_STATUS_SUCCESS == dwStatus)
        {
            System.out.printf("SR-IOV disabled successfully\n");
        }
        else
        {
            System.err.printf("Failed disabling SR-IOV. Error" +
                " [0x%x - %s]\n", dwStatus, wdapi.Stat2Str(dwStatus));
        }

        return wdapi.WD_STATUS_SUCCESS;
    }


    static long MenuSriovCb(MenuCtxSriov pSriovCtx)
    {
        WDCResultLong numVFsResult;

        System.out.printf("\n");
        numVFsResult = PciLib.PCI_SriovGetNumVFs(pSriovCtx.devs[0]);
        if (wdapi.WD_STATUS_SUCCESS != numVFsResult.dwStatus)
        {
            System.out.printf("Could not obtain dwNumVFs.\n");
        }
        else
        {
            pSriovCtx.dwNumVFs = numVFsResult.result;
            System.out.printf("SR-IOV is %s. dwNumVFs: [%d]\n",
                (pSriovCtx.dwNumVFs > 0) ? "Enabled" : "Disabled",
                pSriovCtx.dwNumVFs);
        }

        System.out.printf("-----------\n");

        return wdapi.WD_STATUS_SUCCESS;
    }

    static void MenuSriovInit(DiagMenuOption pParentMenu)
    {
        MenuCtxSriov sriovCtx =
            new MenuCtxSriov(devs);

        MenuSriovCallbacks sriovCallbacks = new
            MenuSriovCallbacks();

        sriovCallbacks.sriovEnableCb = ((pCbCtx)->MenuSriovEnableCb(
            (MenuCtxSriov)pCbCtx));
        sriovCallbacks.sriovDisableCb = ((pCbCtx)->MenuSriovDisableCb(
            (MenuCtxSriov)pCbCtx));
        sriovCallbacks.sriovMenuEntryCb = ((pCbCtx)->MenuSriovCb(
            (MenuCtxSriov)pCbCtx));


        PciMenusCommon.MenuCommonSriovInit(pParentMenu, sriovCtx,
            sriovCallbacks);
    }
    //#endif /* ifdef LINUX */
    //#endif /* ifndef ISA */

    /* -----------------------------------------------
        Read/write memory and I/O addresses
        ----------------------------------------------- */
    static void MenuReadWriteAddrInit(DiagMenuOption pParentMenu)
    {
        MenuCtxRwAddr rwAddrCtx =
            new MenuCtxRwAddr(devs);

        PciMenusCommon.MenuCommonRwAddrInit(pParentMenu, rwAddrCtx);
    }

    //#ifdef HAS_INTS
    /* -----------------------------------------------
        Interrupt handling
        ----------------------------------------------- */

    /* Diagnostics interrupt handler routine */
    static void DiagIntHandler(PCI_INT_RESULT intResult, Object ctx)
    {
        /* TODO: You can modify this function in order to implement your own
                 diagnostics interrupt handler routine */
        System.out.printf("Got interrupt number %d\n", intResult.dwCounter());
        System.out.printf("Interrupt Type: %s\n",
            WdcDiagLib.WDC_DIAG_IntTypeDescriptionGet(
                intResult.dwEnabledIntType()));
        if (wdapi.WDC_INT_IS_MSI(devs[0].Int.dwEnabledIntType))
            System.out.printf("Message Data: 0x%x\n",
                intResult.dwLastMessage());

    }

    static long MenuInterruptsEnableCb(
        MenuCtxInterrupts pIntsCtx)
    {
        long dwStatus = PciLib.PCI_IntEnable(pIntsCtx.devs[0],
            pIntsCtx.funcIntHandler);

        if (wdapi.WD_STATUS_SUCCESS == dwStatus)
        {
            System.out.printf("Interrupts enabled\n");
            pIntsCtx.fIntsEnabled = true;
        }
        else
        {
            System.err.printf("Failed enabling interrupts. Error" +
                " [0x%x - %s]\n", dwStatus,
                wdapi.Stat2Str(dwStatus));
        }

        return wdapi.WD_STATUS_SUCCESS;
    }

    static long MenuInterruptsDisableCb(
        MenuCtxInterrupts pIntsCtx)
    {
        if (wdapi.WD_STATUS_SUCCESS == PciLib.PCI_IntDisable(pIntsCtx.devs[0]))
        {
            System.out.printf("Interrupts disabled\n");
            pIntsCtx.fIntsEnabled = true;
        }
        else
        {
            System.err.printf("Failed disabling interrupts: %s",
                PciLib.PCI_GetLastErr());
        }

        return wdapi.WD_STATUS_SUCCESS;
    }


    static long MenuInterruptsCb(MenuCtxInterrupts pIntsCtx)
    {
        long dwIntOptions = pIntsCtx.devs[0].getIntOptions();
        boolean fIsMsi = wdapi.WDC_INT_IS_MSI(dwIntOptions);

        pIntsCtx.fIntsEnabled = PciLib.PCI_IntIsEnabled(pIntsCtx.devs[0].hDev);
        if ((dwIntOptions & wdapi.INTERRUPT_LEVEL_SENSITIVE) != 0 &&
                !pIntsCtx.fIntsEnabled)
        {
            /* TODO: You can remove this message after you have modified the
               implementation of PCI_IntEnable() in pci_lib.c to correctly
               acknowledge level-sensitive interrupts (see guidelines in
               PCI_IntEnable()). */
            System.out.printf("\n");
            System.out.printf("WARNING!!!\n");
            System.out.printf("----------\n");
            System.out.printf("Your hardware has level sensitive interrupts.\n");
            System.out.printf("Before enabling the interrupts, %s first modify" +
                " the source code\n of PCI_IntEnable(), in the file pci_lib.c," +
                " to correctly acknowledge\n%s interrupts when they occur, as" +
                " dictated by the hardware's specification.\n",
                fIsMsi ? "it is recommended that" : "you must",
                fIsMsi ? "level sensitive" : "");
        }


        return wdapi.WD_STATUS_SUCCESS;
    }

    static void MenuInterruptsInit(DiagMenuOption pParentMenu)
    {
        MenuCtxInterrupts intsCtx =
            new MenuCtxInterrupts(devs, null,
                    new PCI_INT_HANDLER()
                    {
                        public void run(PCI_INT_RESULT intResult, Object o)
                        {
                            DiagIntHandler(intResult, o);
                        }
                    });

        MenuInterruptsCallbacks intsCallbacks = new
            MenuInterruptsCallbacks();

        intsCallbacks.interruptsEnableCb = ((pCbCtx)->MenuInterruptsEnableCb(
            (MenuCtxInterrupts)pCbCtx));
        intsCallbacks.interruptsDisableCb = ((pCbCtx)->MenuInterruptsDisableCb(
            (MenuCtxInterrupts)pCbCtx));
        intsCallbacks.interruptsMenuEntryCb = ((pCbCtx)->MenuInterruptsCb(
            (MenuCtxInterrupts)pCbCtx));

        PciMenusCommon.MenuCommonInterruptsInit(pParentMenu, intsCtx,
            intsCallbacks);
    }

    //#endif /* ifdef HAS_INTS */


    static long CheckKPDriverVer(long hDev)
    {
        WDCResultLong result;
        long dwStatus = 0;
        long dwKPResult = 0;
        PciLib.KP_PCI_VERSION kpVer = new PciLib.KP_PCI_VERSION();

        /* Get Kernel PlugIn Driver version */
        result = wdapi.WDC_CallKerPlug(hDev, PciLib.KP_PCI_MSG_VERSION,
            kpVer.data);
        dwStatus = result.dwStatus;
        dwKPResult = result.result;
        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            System.err.printf("Failed sending a \'Get Version\' message [0x%x]"
                + " to the Kernel-PlugIn driver [%s]. Error [0x%x - %s]\n",
                PciLib.KP_PCI_MSG_VERSION, PciLib.KP_PCI_DRIVER_NAME, dwStatus,
                wdapi.Stat2Str(dwStatus));
        }
        else if (PciLib.KP_PCI_STATUS_OK != dwKPResult)
        {
            System.err.printf("Kernel-PlugIn \'Get Version\' message [0x%x]" +
                " failed. Kernel PlugIn status [0x%x]\n",
                PciLib.KP_PCI_MSG_VERSION, result.result);
            dwStatus = wdapi.WD_INCORRECT_VERSION;
        }
        else
        {
            System.out.printf("Using [%s] Kernel-Plugin driver version "
                + "[%d.%02d - %s]\n", PciLib.KP_PCI_DRIVER_NAME,
                kpVer.getVersion() / 100, kpVer.getVersion() % 100,
                kpVer.getVersionString());
        }

        return dwStatus;
    }

    /* -----------------------------------------------
        Read/write the run-time registers
       ----------------------------------------------- */


    /* -----------------------------------------------
        DMA memory handling
       ----------------------------------------------- */
    enum MENU_DMA {
        ALLOCATE_CONTIG,
        ALLOCATE_SG,
        RESERVED_MEM
    };

    static void FreeDmaMem(long hDev, WD_DMA dma, ByteBuffer pBuf)
    {
        long dwStatus;
        boolean fIsSG;

        if (dma == null)
            return;

        fIsSG = (dma.dwOptions & wdapi.DMA_KERNEL_BUFFER_ALLOC) == 0;

        dwStatus = PciLib.PCI_DmaBufUnlock(dma);
        if (wdapi.WD_STATUS_SUCCESS == dwStatus)
        {
            System.out.printf("DMA memory freed\n");
        }
        else
        {
            System.err.printf("Failed trying to free DMA memory. Error [0x%x "
                + "- %s]\n", dwStatus, wdapi.Stat2Str(dwStatus));
        }

        if (fIsSG)
        {
            if(pBuf == null)
                return;
            //Cleaner cleaner = ((DirectBuffer) pBuf).cleaner();
            //if (cleaner != null)
            //    cleaner.clean();
        }
    }

    static boolean DmaGetAllocInput(PciLib.MENU_CTX_DMA pDmaMenusCtx, MENU_DMA dwOption)
    {
        WDCResult result;
        if (dwOption == MENU_DMA.RESERVED_MEM)
        {
            if (wdapi.isWindows)
            {
                System.out.printf("Warning: The address for the reserved memory"
                    + " should be calculated according to the values listed in "
                    + "registry key\nHKLM/HARDWARE/RESOURCEMAP/System Resources/"
                    + "Physical Memory.\nAny other address may result in a BSOD."
                    + " For more details please refer to Tech Doc #129\n\n");
            }
            result = DiagLib.DIAG_InputNum("Enter reserved memory address " +
                "(64 bit hex uint) ", true, 1,
                Long.MAX_VALUE /*0xFFFFFFFFFFFFFFFF*/);

            if (DiagLib.DIAG_INPUT_SUCCESS != result.dwStatus)
                return false;

            pDmaMenusCtx.qwAddr = ((WDCResultLong)result).result;
        }

        if (dwOption == MENU_DMA.ALLOCATE_CONTIG ||
            dwOption == MENU_DMA.ALLOCATE_SG ||
            dwOption == MENU_DMA.RESERVED_MEM)
        {
            result = DiagLib.DIAG_InputNum("Enter memory allocation size"
                + " in bytes (32 bit uint) ", false, 1, 0xFFFFFFFF);
            if (DiagLib.DIAG_INPUT_SUCCESS != result.dwStatus)
                return false;

            pDmaMenusCtx.size = (int)((WDCResultLong)result).result;
            /* Free DMA memory before trying the new allocation */
            FreeDmaMem(pDmaMenusCtx.devs[0].hDev, pDmaMenusCtx.pDma,
                pDmaMenusCtx.pBuf);
            pDmaMenusCtx.pDma = null;
        }

        return true;
    }

    static long MenuDmaAllocContigOptionCb(PciLib.MENU_CTX_DMA pDmaMenuCtx)
    {
        long dwStatus = 0;

        if(!DmaGetAllocInput(pDmaMenuCtx, MENU_DMA.ALLOCATE_CONTIG))
            return wdapi.WD_INVALID_PARAMETER;

        WDCResult result = PciLib.PCI_DmaAllocContig(pDmaMenuCtx);
        if (wdapi.WD_STATUS_SUCCESS == result.dwStatus &&
            pDmaMenuCtx.pBuf != null)
        {
            pDmaMenuCtx.pDma = ((WDCResultDMA)result).pDma;
            System.out.printf("Contiguous memory allocated: " +
                "[%s],\n\t physical addr [0x%x], size [%d(0x%x)]\n",
                pDmaMenuCtx.pBuf,
                pDmaMenuCtx.pDma.Page[0].pPhysicalAddr,
                pDmaMenuCtx.pDma.Page[0].dwBytes,
                pDmaMenuCtx.pDma.Page[0].dwBytes);
        }
        else
        {
            System.err.printf("Failed allocating contiguous memory."
                + " size [%d], Error [0x%x - %s]\n", pDmaMenuCtx.size,
                result.dwStatus, wdapi.Stat2Str(result.dwStatus));
        }

        return result.dwStatus;
    }

    static long MenuDmaAllocSgOptionCb(PciLib.MENU_CTX_DMA pDmaMenuCtx)
    {
        if(!DmaGetAllocInput(pDmaMenuCtx, MENU_DMA.ALLOCATE_SG))
            return wdapi.WD_INVALID_PARAMETER;

        try
        {
            pDmaMenuCtx.pBuf = ByteBuffer.allocateDirect(pDmaMenuCtx.size);
        }
        catch (Exception e)
        {
            System.err.printf("Failed allocating user memory for SG. "
                + "size [%d]\n", pDmaMenuCtx.size);
            return wdapi.WD_OPERATION_FAILED;
        }
            
        WDCResult result = PciLib.PCI_DmaAllocSg(pDmaMenuCtx);
        if (wdapi.WD_STATUS_SUCCESS == result.dwStatus)
        {
            System.out.printf("SG memory allocated: [%s], size [%d]\n",
                pDmaMenuCtx.pBuf, pDmaMenuCtx.size);

            pDmaMenuCtx.pDma = ((WDCResultDMA)result).pDma;
            System.out.printf("Pages physical addresses:\n");
            for (int i = 0; i < pDmaMenuCtx.pDma.dwPages; i++)
            {
                System.out.printf("%d) physical addr [0x%x], " +
                    "size [%d(0x%x)]\n", i + 1,
                    pDmaMenuCtx.pDma.Page[i].pPhysicalAddr,
                    pDmaMenuCtx.pDma.Page[i].dwBytes,
                    pDmaMenuCtx.pDma.Page[i].dwBytes);
            }
        }
        else
        {
            System.err.printf("Failed allocating SG memory. size [%d], "
                + "Error [0x%x - %s]\n", pDmaMenuCtx.size, result.dwStatus,
                wdapi.Stat2Str(result.dwStatus));
//          free(pBuf);
        }

        return result.dwStatus;
    }

    static long MenuDmaReservedOptionCb(PciLib.MENU_CTX_DMA pDmaMenuCtx)
    {
        if(!DmaGetAllocInput(pDmaMenuCtx, MENU_DMA.RESERVED_MEM))
            return wdapi.WD_INVALID_PARAMETER;

        WDCResult result = PciLib.PCI_DmaAllocReserved(pDmaMenuCtx);

        if (wdapi.WD_STATUS_SUCCESS == result.dwStatus &&
            pDmaMenuCtx.pBuf != null)
        {
            pDmaMenuCtx.pDma = ((WDCResultDMA)result).pDma;
            System.out.printf("Reserved memory claimed: [%s], " +
                "bus addr [0x%x], size [%d(0x%x)]\n",
                pDmaMenuCtx.pBuf,
                pDmaMenuCtx.pDma.Page[0].pPhysicalAddr,
                pDmaMenuCtx.pDma.Page[0].dwBytes,
                pDmaMenuCtx.pDma.Page[0].dwBytes);
        }
        else
        {
            System.err.printf("Failed claiming reserved memory. size" +
                " [%d], Error [0x%x - %s]\n", pDmaMenuCtx.size, result.dwStatus,
                wdapi.Stat2Str(result.dwStatus));
        }

        return result.dwStatus;
    }

    static void MenuDmaInitSubMenusInit(DiagMenuOption pParentMenu,
            PciLib.MENU_CTX_DMA pDmaMenuCtx)
    {
        DiagMenuOption[] options = {
            new DiagMenuOptionBuilder()
            .cOptionName("Allocate contiguous memory")
            .cbEntry((pCbCtx)->MenuDmaAllocContigOptionCb((PciLib.MENU_CTX_DMA)pCbCtx))
            .build(),

            new DiagMenuOptionBuilder()
            .cOptionName("Allocate scatter-gather memory")
            .cbEntry((pCbCtx)->MenuDmaAllocSgOptionCb((PciLib.MENU_CTX_DMA)pCbCtx))
            .build(),

            new DiagMenuOptionBuilder()
            .cOptionName("Use reserved memory")
            .cbEntry((pCbCtx)->MenuDmaReservedOptionCb((PciLib.MENU_CTX_DMA)pCbCtx))
            .build(),

            //#ifndef ISA
            new DiagMenuOptionBuilder()
            .cOptionName("Send buffer through IPC to all group processes")
            .cbEntry((pCbCtx)->{
                WdsDiagLib.WDS_DIAG_IpcSendDmaContigToGroup(
                    ((PciLib.MENU_CTX_DMA)pCbCtx).pDma);
                return wdapi.WD_STATUS_SUCCESS;
            })
            .cbIsHidden((pMenu)->{
                PciLib.MENU_CTX_DMA pDmaCtx = ((PciLib.MENU_CTX_DMA)pMenu.pCbCtx);
                return (pDmaCtx.pDma == null || (pDmaCtx.pDma.dwOptions &
                            wdapi.DMA_KERNEL_BUFFER_ALLOC) == 0);
            })
            .build(),
            //#endif /* ifndef ISA */

            new DiagMenuOptionBuilder()
            .cOptionName("Free DMA memory")
            .cbEntry((pCbCtx)->{
                PciLib.MENU_CTX_DMA pDmaCtx = (PciLib.MENU_CTX_DMA)pCbCtx;
                FreeDmaMem(pDmaCtx.devs[0].hDev, pDmaCtx.pDma, pDmaCtx.pBuf);
                pDmaCtx.pDma = null;

                return wdapi.WD_STATUS_SUCCESS;
            })
            .build()
        };

        DiagLib.DIAG_MenuSetCtxAndParentForMenus(options, pDmaMenuCtx,
            pParentMenu);
    }

    static void MenuDmaInit(DiagMenuOption pParentMenu)
    {
        DiagMenuOption menuDma =
            new DiagMenuOptionBuilder()
            .cOptionName("Allocate/free memory for DMA")
            .cTitleName("DMA memory")
            .cbExit((pCbCtx)->{
                PciLib.MENU_CTX_DMA pDmaCtx = (PciLib.MENU_CTX_DMA)pCbCtx;
                FreeDmaMem(pDmaCtx.devs[0].hDev, pDmaCtx.pDma, pDmaCtx.pBuf);
                pDmaCtx.pDma = null;
                pDmaCtx.pBuf = null;

                return wdapi.WD_STATUS_SUCCESS;
            })
            .cbIsHidden((pMenu)->
                PciMenusCommon.MenuCommonIsDeviceNull(pMenu))
            .build();
        PciLib.MENU_CTX_DMA dmaMenuCtx = new PciLib.MENU_CTX_DMA(devs);

        menuDma.pCbCtx = dmaMenuCtx;
        pParentMenu.DIAG_AddMenuOptionChild(menuDma);

        MenuDmaInitSubMenusInit(menuDma, dmaMenuCtx);
    }
}



