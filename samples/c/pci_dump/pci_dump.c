/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

////////////////////////////////////////////////////////////////
// File - PCI_DUMP.C
//
// A utility for getting a dump of all the PCI configuration
// registers of the PCI cards installed.
//
// Note: This code sample is provided AS-IS and as a guiding sample only.
////////////////////////////////////////////////////////////////

#include "windrvr.h"
#include "wdc_diag_lib.h"
#include "status_strings.h"

#if defined(LINUX)
#include <stdarg.h>
#endif

int main(int argc, char *argv[])
{

    int argi = 1;
    WD_PCI_SLOT pciSlot = {0};
    BOOL single = FALSE;
    DWORD dwStatus = WD_WINDRIVER_STATUS_ERROR;
    FILE *fp = stdout;

    if (argc !=1 && argc != 2 && argc != 4 && argc != 5)
    {
        printf("USAGE: %s [filename] [bus# slot# function#]\n", argv[0]);
        return -1;
    }
    if (argc == 2 || argc == 5)
    {
        if (NULL == (fp = fopen(argv[argi++], "w")))
        {
            perror(argv[1]);
            goto Exit;
        }
    }
    if (argc >= 4)
    {
        sscanf(argv[argi++], "%ld", (long *)&pciSlot.dwBus);
        sscanf(argv[argi++], "%ld", (long *)&pciSlot.dwSlot);
        sscanf(argv[argi++], "%ld", (long *)&pciSlot.dwFunction);
        single = TRUE;
    }

    fprintf(fp, "pci bus scan (using WD_PciConfigDump)\n");

    dwStatus = WDC_DriverOpen(WDC_DRV_OPEN_CHECK_VER, NULL);
    if (dwStatus)
        goto Exit;

    if (single)
        WDC_DIAG_PciDeviceInfoPrintFile(&pciSlot, fp, TRUE);
    else
        WDC_DIAG_PciDevicesInfoPrintAllFile(fp, TRUE);

Exit:
    WDC_DriverClose();
    if (dwStatus)
    {
        fprintf(fp, "%s failed, 0x%x - %s\n", argv[0], dwStatus,
            Stat2Str(dwStatus));
    }

    if (fp)
        fclose(fp);
    return dwStatus == WD_STATUS_SUCCESS ? 0 : -1;
}

