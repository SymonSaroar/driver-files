package com.jungo.shared;
import com.jungo.wdapi;
import com.jungo.wdapi.*;
import com.jungo.shared.DiagLib.*;

/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

public class PciMenusCommon {

    public static class MenuCtx {
        public WDC_DEVICE[] devs;

        public MenuCtx(WDC_DEVICE[] devs)
        {
            this.devs = devs;
        }
    }

    public static boolean MenuCommonIsDeviceNull(DiagMenuOption pMenu)
    {
        return ((MenuCtx)pMenu.pCbCtx).devs[0] == null;
    }


    /* -----------------------------------------------
       Scan Bus
       ----------------------------------------------- */
    public static DiagMenuOption MenuCommonScanBusInit(
        DiagMenuOption pParentMenu)
    {
        DiagMenuOption menuScanBusOption =
            new DiagMenuOptionBuilder()
            .cOptionName("Scan PCI bus")
            .cbEntry((pCbCtx)->{
                WdcDiagLib.WDC_DIAG_PciDevicesInfoPrintAll(false, true);
                return wdapi.WD_STATUS_SUCCESS;
            })
            .build();


        pParentMenu.DIAG_AddMenuOptionChild(menuScanBusOption);

        return menuScanBusOption;
    }

    /* -----------------------------------------------
        Read/write the configuration space
        ----------------------------------------------- */

    public static class MenuCtxCfg extends MenuCtx{
        WdcDiagLib.WDC_REG[] pCfgRegs;
        WdcDiagLib.WDC_REG[] pCfgExpRegs;

        public MenuCtxCfg(WDC_DEVICE[] devs, WdcDiagLib.WDC_REG[] pCfgRegs,
            WdcDiagLib.WDC_REG[] pCfgExpRegs)
        {
            super(devs);
            this.pCfgRegs = pCfgRegs;
            this.pCfgExpRegs = pCfgExpRegs;
        }
    }

    static boolean MenuCfgIsDeviceNotExpress(DiagMenuOption pMenu)
    {
        return
            wdapi.WDC_PciGetExpressGen(((MenuCtx)pMenu.pCbCtx).devs[0].hDev) == 0;
    }

    static long MenuCfgReadAllOptionCb(MenuCtxCfg pCfgCtx)
    {
        WdcDiagLib.WDC_DIAG_ReadRegsAll(pCfgCtx.devs[0].hDev, pCfgCtx.pCfgRegs,
            true, false);

        if (wdapi.WDC_PciGetExpressGen(pCfgCtx.devs[0].hDev) != 0)
        {
            WdcDiagLib.WDC_DIAG_ReadRegsAll(pCfgCtx.devs[0].hDev,
                pCfgCtx.pCfgExpRegs, true, true);
        }
        return wdapi.WD_STATUS_SUCCESS;
    }


    static long MenuCfgScanCapsOptionCb(MenuCtxCfg pCfgCtx)
    {
        WdcDiagLib.WDC_DIAG_ScanPCICapabilities(pCfgCtx.devs[0].hDev);
        return wdapi.WD_STATUS_SUCCESS;
    }

    static void MenuCommonCfgSubMenusInit(DiagMenuOption pParentMenu,
            MenuCtxCfg pCfgCtx)
    {
        DiagMenuOption []options = {
            new DiagMenuOptionBuilder()
            .cOptionName("Read from an offset")
            .cbEntry((pCbCtx)->{
                WdcDiagLib.WDC_DIAG_ReadWriteBlock(pCfgCtx.devs[0].hDev,
                    wdapi.WDC_READ, wdapi.WDC_AD_CFG_SPACE);
                return wdapi.WD_STATUS_SUCCESS;
            })
            .build(),

            new DiagMenuOptionBuilder()
            .cOptionName("Write to an offset")
            .cbEntry((pCbCtx)->{
                WdcDiagLib.WDC_DIAG_ReadWriteBlock(pCfgCtx.devs[0].hDev,
                    wdapi.WDC_WRITE, wdapi.WDC_AD_CFG_SPACE);
                return wdapi.WD_STATUS_SUCCESS;
            })
            .build(),

            new DiagMenuOptionBuilder()
            .cOptionName("Read all configuration registers defined for " +
                "the device (see list above)")
            .cbEntry((pCbCtx)->MenuCfgReadAllOptionCb((MenuCtxCfg)pCbCtx))
            .build(),

            new DiagMenuOptionBuilder()
            .cOptionName("Read from a named register")
            .cbEntry((pCbCtx)->{
                WdcDiagLib.WDC_DIAG_ReadWriteReg(pCfgCtx.devs[0].hDev,
                    pCfgCtx.pCfgRegs, wdapi.WDC_READ, true, false);
                return wdapi.WD_STATUS_SUCCESS;
            })
            .build(),

            new DiagMenuOptionBuilder()
            .cOptionName("Write to a named register")
            .cbEntry((pCbCtx)->{
                WdcDiagLib.WDC_DIAG_ReadWriteReg(pCfgCtx.devs[0].hDev,
                    pCfgCtx.pCfgRegs, wdapi.WDC_WRITE, true, false);
                    return wdapi.WD_STATUS_SUCCESS;
                })
            .build(),

            new DiagMenuOptionBuilder()
            .cOptionName("Read from a named PCI Express register")
            .cbEntry((pCbCtx)->{
                WdcDiagLib.WDC_DIAG_ReadWriteReg(pCfgCtx.devs[0].hDev,
                    pCfgCtx.pCfgExpRegs, wdapi.WDC_READ, true, true);
                return wdapi.WD_STATUS_SUCCESS;
                })
            .cbIsHidden((pMenu)->MenuCfgIsDeviceNotExpress(pMenu))
            .build(),

            new DiagMenuOptionBuilder()
            .cOptionName("Write to a named PCI Express register")
            .cbEntry((pCbCtx)->{
                WdcDiagLib.WDC_DIAG_ReadWriteReg(pCfgCtx.devs[0].hDev,
                    pCfgCtx.pCfgExpRegs, wdapi.WDC_WRITE, true, true);
                return wdapi.WD_STATUS_SUCCESS;
                })
            .cbIsHidden((pMenu)->MenuCfgIsDeviceNotExpress(pMenu))
            .build(),

            new DiagMenuOptionBuilder()
            .cOptionName("Scan PCI/PCIe capabilities")
            .cbEntry((pCbCtx)->{
                WdcDiagLib.WDC_DIAG_ScanPCICapabilities(pCfgCtx.devs[0].hDev);
                return wdapi.WD_STATUS_SUCCESS;
            })
            .build()
        };

        DiagLib.DIAG_MenuSetCtxAndParentForMenus(options, pCfgCtx, pParentMenu);
    }

    static long MenuCfgCb(MenuCtxCfg pCfgCtx)
    {
        /* Display predefined registers information */
        System.out.printf("\n");
        System.out.printf("Configuration registers:\n");
        System.out.printf("------------------------\n");
        WdcDiagLib.WDC_DIAG_RegsInfoPrint(pCfgCtx.devs[0].hDev,
            pCfgCtx.pCfgRegs,
            WdcDiagLib.WDC_DIAG_REG_PRINT_ALL &
            ~WdcDiagLib.WDC_DIAG_REG_PRINT_ADDR_SPACE, false);

        if (wdapi.WDC_PciGetExpressGen(pCfgCtx.devs[0].hDev) != 0)
        {
            WdcDiagLib.WDC_DIAG_RegsInfoPrint(pCfgCtx.devs[0].hDev,
                pCfgCtx.pCfgExpRegs,
                WdcDiagLib.WDC_DIAG_REG_PRINT_ALL &
                ~WdcDiagLib.WDC_DIAG_REG_PRINT_ADDR_SPACE, true);
        }

        return wdapi.WD_STATUS_SUCCESS;
    }

    public static DiagMenuOption MenuCommonCfgInit(
            DiagMenuOption pParentMenu, MenuCtxCfg pCfgCtx)
    {
        DiagMenuOption menuCfgRoot =
            new DiagMenuOptionBuilder()
            .cOptionName("Read/write the PCI configuration space")
            .cTitleName("Read/write the device's configuration space")
            .cbEntry((pCbCtx)->MenuCfgCb((MenuCtxCfg)pCbCtx))
            .cbIsHidden((pMenu)->MenuCommonIsDeviceNull(pMenu))
            .build();

        menuCfgRoot.pCbCtx = pCfgCtx;
        pParentMenu.DIAG_AddMenuOptionChild(menuCfgRoot);

        MenuCommonCfgSubMenusInit(menuCfgRoot, pCfgCtx);
        return menuCfgRoot;
    }

    /* ----------------------------------------------------
        Plug-and-play and power management events handling
        ---------------------------------------------------- */

    public static class MenuEventsCallbacks{
        public MenuCallback eventsEnableCb = null;
        public MenuCallback eventsDisableCb = null;
        public MenuCallback eventsMenuEntryCb = null;
        public MenuCallback eventsMenuExitCb = null;
    }

    public static class MenuCtxEvents extends MenuCtx {
        public boolean fRegistered;
        public PCI_EVENT_HANDLER DiagEventHandler;

        public MenuCtxEvents(WDC_DEVICE[] devs,
            PCI_EVENT_HANDLER DiagEventHandler)
        {
            super(devs);
            this.DiagEventHandler = DiagEventHandler;
        }
    }

    static void MenuCommonEventsSubMenusInit(DiagMenuOption pParentMenu,
        MenuCtxEvents pEventsMenusCtx, MenuEventsCallbacks pEventsCallbacks)
    {
        DiagMenuOption[] options = {
            new DiagMenuOptionBuilder()
            .cOptionName("Register Events")
            .cbEntry(pEventsCallbacks.eventsEnableCb)
            .cbIsHidden((pMenu)->{
                return ((MenuCtxEvents)pMenu.pCbCtx).fRegistered;
            })
            .build(),

            new DiagMenuOptionBuilder()
            .cOptionName("Unregister Events")
            .cbEntry(pEventsCallbacks.eventsDisableCb)
            .cbIsHidden((pMenu)->{
                return !((MenuCtxEvents)pMenu.pCbCtx).fRegistered;
            })
            .build()
        };

        DiagLib.DIAG_MenuSetCtxAndParentForMenus(options, pEventsMenusCtx,
            pParentMenu);
    }

    public static DiagMenuOption MenuCommonEventsInit(
        DiagMenuOption pParentMenu, MenuCtxEvents pEventsMenusCtx,
        MenuEventsCallbacks pEventsCallbacks)
    {
        DiagMenuOption menuEventsRoot =
            new DiagMenuOptionBuilder()
            .cOptionName("Register/unregister plug-and-play and power " +
                "management events")
            .cTitleName("Plug-and-play and power management events")
            .cbEntry(pEventsCallbacks.eventsMenuEntryCb)
            .cbExit(pEventsCallbacks.eventsMenuEntryCb)
            .cbIsHidden((pMenu)->MenuCommonIsDeviceNull(pMenu))
            .build();


        menuEventsRoot.pCbCtx = pEventsMenusCtx;
        pParentMenu.DIAG_AddMenuOptionChild(menuEventsRoot);

        MenuCommonEventsSubMenusInit(menuEventsRoot, pEventsMenusCtx,
            pEventsCallbacks);
        return menuEventsRoot;
    }

    /* -----------------------------------------------
        SRIOV handling
    ----------------------------------------------- */
// #ifdef LINUX
    public static class MenuSriovCallbacks {
        public MenuCallback sriovEnableCb = null;
        public MenuCallback sriovDisableCb = null;
        public MenuCallback sriovMenuEntryCb = null;
        public MenuCallback sriovMenuExitCb = null;
    }

    public static class MenuCtxSriov extends MenuCtx {
        public long dwNumVFs;

        public MenuCtxSriov(WDC_DEVICE[] devs)
        {
            super(devs);
            this.dwNumVFs = 0;
        }
    }

    static void MenuCommonSriovSubMenusInit(DiagMenuOption pParentMenu,
        MenuCtxSriov pSriovMenusCtx, MenuSriovCallbacks pSriovCallbacks)
    {
        DiagMenuOption []options = {
            new DiagMenuOptionBuilder()
            .cOptionName("Enable SR-IOV")
            .cbEntry(pSriovCallbacks.sriovEnableCb)
            .cbIsHidden((pMenu)->{
                 return ((MenuCtxSriov)pMenu.pCbCtx).dwNumVFs > 0;
            })
            .build(),

            new DiagMenuOptionBuilder()
            .cOptionName("Disable SR-IOV")
            .cbEntry(pSriovCallbacks.sriovDisableCb)
            .cbIsHidden((pMenu)->{
            	return ((MenuCtxSriov)pMenu.pCbCtx).dwNumVFs == 0;
            })
            .build()
        };

        DiagLib.DIAG_MenuSetCtxAndParentForMenus(options, pSriovMenusCtx,
            pParentMenu);
    }

    public static DiagMenuOption MenuCommonSriovInit(
        DiagMenuOption pParentMenu, MenuCtxSriov pSriovMenusCtx,
        MenuSriovCallbacks pSriovCallbacks)
    {
        DiagMenuOption menuSriovRoot =
            new DiagMenuOptionBuilder()
            .cOptionName("Enable/disable SR-IOV capability")
            .cbEntry(pSriovCallbacks.sriovMenuEntryCb)
            .cbExit(pSriovCallbacks.sriovMenuExitCb)
            .cbIsHidden((pMenu)->MenuCommonIsDeviceNull(pMenu))
            .build();


        menuSriovRoot.pCbCtx = pSriovMenusCtx;
        pParentMenu.DIAG_AddMenuOptionChild(menuSriovRoot);

        MenuCommonSriovSubMenusInit(menuSriovRoot, pSriovMenusCtx,
            pSriovCallbacks);
        return menuSriovRoot;
    }

// #endif

    /* -----------------------------------------------
        Read/write memory and I/O addresses
        ----------------------------------------------- */
//#ifdef ISA
//   static final int PCI_ADDR_SPACES_NUM 0
//#endif /* ifdef ISA */
     static final int ACTIVE_ADDR_SPACE_NEEDS_INIT = 0xFF;

     public static class MenuCtxRwAddr extends MenuCtx{
        int dwAddrSpace = ACTIVE_ADDR_SPACE_NEEDS_INIT;
        int mode = wdapi.WDC_MODE_32;
        boolean fBlock = false;

        public MenuCtxRwAddr(WDC_DEVICE[] devs)
        {
            super(devs);
        }
    }

    static long MenuRwAddrSetAddrSpace(MenuCtxRwAddr pRwAddrCtx)
    {
        int dwAddrSpace;
     //#ifndef ISA
        long dwNumAddrSpaces = WdcDiagLib.WDC_DIAG_GetNumAddrSpaces(
            pRwAddrCtx.devs[0]);
     //#else /* ifdef ISA */
     // long dwNumAddrSpaces = PCI_ADDR_SPACES_NUM;
     //#endif /* ifdef ISA */

        WdcDiagLib.ADDR_SPACE_INFO addrSpaceInfo =
            new WdcDiagLib.ADDR_SPACE_INFO();

        System.out.printf("\n");
        System.out.printf("Select an active address space:\n");
        System.out.printf("-------------------------------\n");

        for (dwAddrSpace = 0; dwAddrSpace < dwNumAddrSpaces; dwAddrSpace++)
        {
            addrSpaceInfo.dwAddrSpace = dwAddrSpace;
            if (!WdcDiagLib.WDC_DIAG_GetAddrSpaceInfo(pRwAddrCtx.devs[0],
                addrSpaceInfo))
            {
                System.err.printf("SetAddrSpace: Error - Failed to get address"
                    + " space information.");
                return wdapi.WD_SYSTEM_INTERNAL_ERROR;
            }

            System.out.printf("%d. %-22s %-8s%s\n", dwAddrSpace + 1,
                addrSpaceInfo.sName, addrSpaceInfo.sType,
                addrSpaceInfo.sDesc);
        }
        System.out.printf("\n");

        WDCResultLong result = DiagLib.DIAG_InputNum("Enter option", false, 1,
            dwNumAddrSpaces);
        if (DiagLib.DIAG_INPUT_SUCCESS != result.dwStatus)
            return wdapi.WD_INVALID_PARAMETER;

        dwAddrSpace = (int) (result.result - 1);

        if (!wdapi.WDC_AddrSpaceIsActive(pRwAddrCtx.devs[0], dwAddrSpace))
        {
            System.out.printf("You have selected an inactive address space\n");
            return wdapi.WD_INVALID_PARAMETER;
        }

        pRwAddrCtx.dwAddrSpace = dwAddrSpace;
        return wdapi.WD_STATUS_SUCCESS;
    }

    static long ReadWriteAddr(MenuCtxRwAddr pRwAddrCtx, int direction)
    {
        if (pRwAddrCtx.fBlock)
        {
            WdcDiagLib.WDC_DIAG_ReadWriteBlock(pRwAddrCtx.devs[0].hDev,
                direction, pRwAddrCtx.dwAddrSpace);
        }
        else
        {
            WdcDiagLib.WDC_DIAG_ReadWriteAddr(pRwAddrCtx.devs[0].hDev,
                direction, pRwAddrCtx.dwAddrSpace, pRwAddrCtx.mode);
        }

        return wdapi.WD_STATUS_SUCCESS;
    }

    static void MenuCommonRwAddrSubMenusInit(
        DiagMenuOption pParentMenu, MenuCtxRwAddr pRwAddrCtx)
    {
        DiagMenuOption[] options = {
            new DiagMenuOptionBuilder()
                .cOptionName("Change active address space for read/write")
                .cbEntry((pCbCtx)->MenuRwAddrSetAddrSpace(
                    (MenuCtxRwAddr) pCbCtx))
                .build(),

            new DiagMenuOptionBuilder()
                .cOptionName("Change active read/write mode")
                .cbEntry((pCbCtx)->{
                    WDCResultInteger result = WdcDiagLib.WDC_DIAG_SetMode();
                    if (result.dwStatus == wdapi.WD_STATUS_SUCCESS)
                        pRwAddrCtx.mode = result.result;

                    return result.dwStatus;
                })
                .build(),

            new DiagMenuOptionBuilder()
                .cOptionName("Toggle active transfer type")
                .cbEntry((pCbCtx)->{
                    pRwAddrCtx.fBlock = !pRwAddrCtx.fBlock;
                    return wdapi.WD_STATUS_SUCCESS;
                })
                .build(),

            new DiagMenuOptionBuilder()
                .cOptionName("Read from active address space")
                .cbEntry((pCbCtx)->ReadWriteAddr(
                        (MenuCtxRwAddr)pCbCtx, wdapi.WDC_READ))
                .build(),

            new DiagMenuOptionBuilder()
                .cOptionName("Write to active address space")
                .cbEntry((pCbCtx)->ReadWriteAddr(
                    (MenuCtxRwAddr)pCbCtx, wdapi.WDC_WRITE))
                .build()
        };

        DiagLib.DIAG_MenuSetCtxAndParentForMenus(options, pRwAddrCtx,
                pParentMenu);
    }

    static long MenuRwAddrCb(MenuCtxRwAddr pRwAddrCtx)
    {
        /* Initialize active address space */
        if (ACTIVE_ADDR_SPACE_NEEDS_INIT == pRwAddrCtx.dwAddrSpace)
        {
            //#ifndef ISA
            int dwNumAddrSpaces = WdcDiagLib.WDC_DIAG_GetNumAddrSpaces(
                    pRwAddrCtx.devs[0]);
            //#else /* ifdef ISA */
            //              long dwNumAddrSpaces = PCI_ADDR_SPACES_NUM;
            //#endif /* ifdef ISA */
            /* Find the first active address space */
            for (pRwAddrCtx.dwAddrSpace = 0;
                pRwAddrCtx.dwAddrSpace < dwNumAddrSpaces;
                pRwAddrCtx.dwAddrSpace++)
            {
                if (wdapi.WDC_AddrSpaceIsActive(pRwAddrCtx.devs[0],
                        pRwAddrCtx.dwAddrSpace))
                {
                    break;
                }
            }

            /* Sanity check */
            if (pRwAddrCtx.dwAddrSpace == dwNumAddrSpaces)
            {
                System.err.printf("MenuReadWriteAddr: Error - No active address"
                        + " spaces found\n");
                pRwAddrCtx.dwAddrSpace = ACTIVE_ADDR_SPACE_NEEDS_INIT;
                return wdapi.WD_INSUFFICIENT_RESOURCES;
            }
        }

        System.out.printf("\nCurrent Read/Write configurations:\n");
        System.out.printf("----------------------------------\n");
        System.out.printf("Currently active address space : ");
        //#ifndef ISA
        System.out.printf("BAR %d\n", pRwAddrCtx.dwAddrSpace);
        //#else /* ifdef ISA */
        //  System.out.printf("AddrSpace %ld\n", pRwAddrCtx.dwAddrSpace);
        //#endif /* ifdef ISA */
        System.out.printf("Currently active read/write mode: %s\n",
            (wdapi.WDC_MODE_8 == pRwAddrCtx.mode) ? "8 bit" :
            (wdapi.WDC_MODE_16 == pRwAddrCtx.mode) ? "16 bit" :
            (wdapi.WDC_MODE_32 == pRwAddrCtx.mode) ? "32 bit" : "64 bit");
        System.out.printf("Currently active transfer type: %s\n",
            pRwAddrCtx.fBlock ? "block transfers" :
            "non-block transfers");
        System.out.printf("\n");

        return wdapi.WD_STATUS_SUCCESS;
    }

    public static DiagMenuOption MenuCommonRwAddrInit(
        DiagMenuOption pParentMenu, MenuCtxRwAddr pRwAddrMenusCtx)
    {
        DiagMenuOption menuRwAddrRoot =
            new DiagMenuOptionBuilder()
            .cOptionName("Read/write memory and I/O addresses on the device")
            .cTitleName("Read/write the device's memory and I/O ranges")
            .cbEntry((pCbCtx)->MenuRwAddrCb((MenuCtxRwAddr)pCbCtx))
            .cbExit((pCbCtx)->{
                pRwAddrMenusCtx.dwAddrSpace = ACTIVE_ADDR_SPACE_NEEDS_INIT;
                return wdapi.WD_STATUS_SUCCESS;
            })
            .cbIsHidden((pMenu)->MenuCommonIsDeviceNull(pMenu))
            .build();


        menuRwAddrRoot.pCbCtx = pRwAddrMenusCtx;
        pParentMenu.DIAG_AddMenuOptionChild(menuRwAddrRoot);

        MenuCommonRwAddrSubMenusInit(menuRwAddrRoot, pRwAddrMenusCtx);
        return menuRwAddrRoot;
    }


    /* -----------------------------------------------
        Interrupt handling
        ----------------------------------------------- */
    public static class MenuInterruptsCallbacks {
        public MenuCallback interruptsEnableCb = null;
        public MenuCallback interruptsDisableCb = null;
        public MenuCallback interruptsMenuEntryCb = null;
        public MenuCallback interruptsMenuExitCb = null;
    }

    public static class MenuCtxInterrupts extends MenuCtx {
        public boolean fIntsEnabled;
        public Object pData; /* Optional additional data. If needed, pass to
                        xxx_IntEnable in the xxx_lib.h file */
        public PCI_INT_HANDLER funcIntHandler;

        public MenuCtxInterrupts(WDC_DEVICE[] devs, Object pData,
            PCI_INT_HANDLER funcIntHandler)
        {
            super(devs);
            this.pData = pData;
            this.funcIntHandler = funcIntHandler;
        }
    }

    static void MenuCommonInterruptsSubMenusInit(
        DiagMenuOption pParentMenu, MenuCtxInterrupts pIntsMenusCtx,
        MenuInterruptsCallbacks pIntsCallbacks)
    {
        DiagMenuOption []options = {
            new DiagMenuOptionBuilder()
                .cOptionName("Enable Interrupts")
                .cbEntry(pIntsCallbacks.interruptsEnableCb)
                .cbIsHidden((pMenu)->{
                    return ((MenuCtxInterrupts)pMenu.pCbCtx).fIntsEnabled;
                })
                .build(),

                new DiagMenuOptionBuilder()
                .cOptionName("Disable Interrupts")
                .cbEntry(pIntsCallbacks.interruptsDisableCb)
                .cbIsHidden((pMenu)->{
                	return !((MenuCtxInterrupts)pMenu.pCbCtx).fIntsEnabled;
                })
                .build()
        };

        DiagLib.DIAG_MenuSetCtxAndParentForMenus(options, pIntsMenusCtx,
            pParentMenu);
    }

    public static DiagMenuOption MenuCommonInterruptsInit(
        DiagMenuOption pParentMenu, MenuCtxInterrupts pIntsMenusCtx,
        MenuInterruptsCallbacks pIntsCallbacks)
    {
        DiagMenuOption menuInterruptsRoot =
            new DiagMenuOptionBuilder()
                .cOptionName("Enable/disable the device's interrupts")
                .cTitleName("Interrutps")
                .cbEntry(pIntsCallbacks.interruptsMenuEntryCb)
                .cbExit(pIntsCallbacks.interruptsMenuExitCb)
                .cbIsHidden((pMenu)->MenuCommonIsDeviceNull(pMenu))
                .build();


        menuInterruptsRoot.pCbCtx = pIntsMenusCtx;
        pParentMenu.DIAG_AddMenuOptionChild(menuInterruptsRoot);

        MenuCommonInterruptsSubMenusInit(menuInterruptsRoot, pIntsMenusCtx,
            pIntsCallbacks);
        return menuInterruptsRoot;
    }

    /* -------------------------------------------------------
        Read/write the run-time/configuration block registers
    -------------------------------------------------------*/
    public static class MenuCtxRwRegs extends MenuCtx {
        WdcDiagLib.WDC_REG[] pRegsArr;
        boolean fIsConfig = false;
        String sModuleName;

        public MenuCtxRwRegs(WDC_DEVICE[] devs,
            WdcDiagLib.WDC_REG[] rwRegs, String sModuleName)
        {
            super(devs);
            this.pRegsArr = rwRegs;
            this.sModuleName = sModuleName;
        }
    }

    static void MenuCommonRwRegsSubMenusInit(
        DiagMenuOption pParentMenu, MenuCtxRwRegs pRwRegsMenusCtx)
    {
        DiagMenuOption []options = {
            new DiagMenuOptionBuilder()
            .cOptionName("Read all registers(see list above)")
            .cbEntry((pCbCtx)->{
                WdcDiagLib.WDC_DIAG_ReadRegsAll(pRwRegsMenusCtx.devs[0].hDev,
                    pRwRegsMenusCtx.pRegsArr, false, false);
                return wdapi.WD_STATUS_SUCCESS;
            })
            .build(),

            new DiagMenuOptionBuilder()
            .cOptionName("Read from a register")
            .cbEntry((pCbCtx)->{
                WdcDiagLib.WDC_DIAG_ReadWriteReg(pRwRegsMenusCtx.devs[0].hDev,
                    pRwRegsMenusCtx.pRegsArr, wdapi.WDC_READ, false, false);
                return wdapi.WD_STATUS_SUCCESS;
            })
            .build(),

            new DiagMenuOptionBuilder()
            .cOptionName("Write to a register")
            .cbEntry((pCbCtx)->{
                WdcDiagLib.WDC_DIAG_ReadWriteReg(pRwRegsMenusCtx.devs[0].hDev,
                    pRwRegsMenusCtx.pRegsArr, wdapi.WDC_WRITE, false, false);

                return wdapi.WD_STATUS_SUCCESS;
            })
            .build()
        };

        DiagLib.DIAG_MenuSetCtxAndParentForMenus(options, pRwRegsMenusCtx,
                pParentMenu);
    }

    static boolean MenuRwRegsIsDeviceNullOrRegsEmpty(
        DiagMenuOption pMenu)
    {
        return ((MenuCtxRwRegs)pMenu.pCbCtx).devs[0].hDev == 0 ||
            ((MenuCtxRwRegs)pMenu.pCbCtx).pRegsArr.length == 0;
    }

    static long MenuRwRegsCb(MenuCtxRwRegs pRwRegsMenusCtx)
    {
        WdcDiagLib.WDC_DIAG_RegsInfoPrint(pRwRegsMenusCtx.devs[0].hDev,
            pRwRegsMenusCtx.pRegsArr,
            WdcDiagLib.WDC_DIAG_REG_PRINT_ALL, false);
        System.out.println("");

        return wdapi.WD_STATUS_SUCCESS;
    }

    public static DiagMenuOption MenuCommonRwRegsInit(
        DiagMenuOption pParentMenu, MenuCtxRwRegs pRwRegsMenusCtx)
    {
        DiagMenuOption menuRwRegsRoot =
            new DiagMenuOptionBuilder()
            .cbEntry((pCbCtx)->MenuRwRegsCb((MenuCtxRwRegs)pCbCtx))
            .cbIsHidden((pMenu)->MenuRwRegsIsDeviceNullOrRegsEmpty(pMenu))
            .cOptionName("Read/write the " + (pRwRegsMenusCtx.fIsConfig ?
                "configuration block" : "run-time") + " registers")
            .cTitleName(pRwRegsMenusCtx.sModuleName +
                (pRwRegsMenusCtx.fIsConfig ? " configuration block" :
                    " run-time") + " registers")
            .build();

        menuRwRegsRoot.pCbCtx = pRwRegsMenusCtx;
        pParentMenu.DIAG_AddMenuOptionChild(menuRwRegsRoot);

        MenuCommonRwRegsSubMenusInit(menuRwRegsRoot, pRwRegsMenusCtx);
        return menuRwRegsRoot;
    }

}


