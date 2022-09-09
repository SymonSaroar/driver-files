/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

////////////////////////////////////////////////////////////////
//File - PciScan.java
//
//A utility for getting a list of the PCI cards installed
//and the resources allocated for each one of them (memory
//ranges, I/O ranges and interrupts).
//
//Note: This code sample is provided AS-IS and as a guiding sample only.
////////////////////////////////////////////////////////////////

package com.jungo.PciScan;

import com.jungo.*;
import com.jungo.shared.*;

import java.io.File;
import java.io.PrintStream;

public class PciScan {

    public static void main(String[] args)
    {
        long dwStatus = wdapi.WD_WINDRIVER_STATUS_ERROR;
        PrintStream o = null;
        int argi = 0, argc = args.length + 1;

        if (argc != 1 && argc != 2)
        {
            System.out.printf("USAGE: %s [filename]\n", args[0]);
            System.exit(-1);
        }
        if (argc == 2)
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

        System.out.printf("pci bus scan (using WD_PciConfigDump)\n");

        dwStatus = wdapi.WDC_DriverOpen(wdapi.WDC_DRV_OPEN_CHECK_VER, "");
        if (dwStatus != 0)
            exitApp(dwStatus, o);

        WdcDiagLib.WDC_DIAG_PciDevicesInfoPrintAll(false, o == null);

        exitApp(dwStatus, o);
    }

    static void exitApp(long dwStatus, PrintStream o)
    {
        wdapi.WDC_DriverClose();
        if (dwStatus != 0)
        {
            System.out.printf("PciScan failed, 0x%lx - %s\n", dwStatus,
                wdapi.Stat2Str(dwStatus));
        }
        if (o != null)
            o.close();

        System.exit(dwStatus == wdapi.WD_STATUS_SUCCESS ? 0 : -1);
    }
}
