/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

////////////////////////////////////////////////////////////////
//File - PciDump.java
//
//A utility for getting a dump of all the PCI configuration
//registers of the PCI cards installed.
//
//Note: This code sample is provided AS-IS and as a guiding sample only.
////////////////////////////////////////////////////////////////

package com.jungo.PciDump;

import com.jungo.*;
import com.jungo.wdapi.*;
import com.jungo.shared.*;

import java.io.File;
import java.io.PrintStream;

public class PciDump {

    public static void main(String[] args)
    {
        PrintStream o = null;
        int argi = 0, argc = args.length + 1;
        WD_PCI_SLOT pciSlot = new WD_PCI_SLOT(0, 0, 0);
        boolean single = false;
        long dwStatus = wdapi.WD_WINDRIVER_STATUS_ERROR;

        if (argc != 1 && argc != 2 && argc != 4 && argc != 5)
        {
            System.out.printf("USAGE: PciDump [filename] [bus# slot#"
                + " function#]\n");
            System.exit(-1);
        }
        if (argc == 2 || argc == 5)
        {
            try
            {
                o = new PrintStream(new File(args[argi++]));
                // Assign o to output stream
                System.setOut(o);
            }
            catch (Exception e)
            {
                System.err.println("PciDump: invalid file");
                System.exit(-1);
            }
        }
        if (argc >= 4)
        {
            try
            {
                pciSlot.dwBus = Long.valueOf(args[argi++]);
                pciSlot.dwSlot = Long.valueOf(args[argi++]);
                pciSlot.dwFunction = Long.valueOf(args[argi++]);
            }
            catch (Exception e)
            {
                System.err.println("PciDump failed: Invalid parameters");
                System.exit(-1);
            }
            single = true;
        }

        System.out.println("pci bus scan (using WD_PciConfigDump)");

        dwStatus = wdapi.WDC_DriverOpen(wdapi.WDC_DRV_OPEN_CHECK_VER, "");
        if (dwStatus == 0)
        {
            if (single)
                WdcDiagLib.WDC_DIAG_PciDeviceInfoPrint(pciSlot, true);
            else
                WdcDiagLib.WDC_DIAG_PciDevicesInfoPrintAll(true, o == null);
        }

        wdapi.WDC_DriverClose();
        if (dwStatus != 0)
        {
            System.out.printf("PciDump failed: 0x%x - %s\n", dwStatus,
                    wdapi.Stat2Str(dwStatus));
        }

        if (o != null)
            o.close();

        System.exit(dwStatus == wdapi.WD_STATUS_SUCCESS ? 0 : -1);
    }
}
