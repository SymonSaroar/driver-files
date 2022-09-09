''' Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com '''

import sys, os
sys.path.append(os.path.dirname(os.path.abspath(__file__)) + "/..")
from wdlib.wdc_diag_lib import *
import argparse

def auto_int(x):
    return int(x, 0)

parser = argparse.ArgumentParser(description="PCI scan")

parser.add_argument('-f', '--filename', type=str)
args = parser.parse_args()

if args.filename:
    fp = open(args.filename, "w+")
else:
    fp = None

fprintf(fp, "PCI bus scan (using WD_PciConfigDump)\n")

dwStatus = wdapi.WDC_DriverOpen(WDC_DRV_OPEN_CHECK_VER, 0)
if dwStatus:
    print("WDC_DriverOpen: " + Stat2Str(dwStatus))
    exit

WDC_DIAG_PciDevicesInfoPrintAllFile(fp, False)

dwStatus = wdapi.WDC_DriverClose()
if dwStatus:
    print("WDC_DriverClose: " + Stat2Str(dwStatus))

if fp:
    fp.close()
