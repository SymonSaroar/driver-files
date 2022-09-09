''' Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com '''

import sys, os
sys.path.append(os.path.dirname(os.path.abspath(__file__)) + "/..")
from wdlib.wdc_diag_lib import *
import argparse

def auto_int(x):
    return int(x, 0)

parser = argparse.ArgumentParser(description='PCI dump')

parser.add_argument('-f', '--filename', type=str)
parser.add_argument('-b', '--bus', type=auto_int)
parser.add_argument('-s', '--slot', type=auto_int)
parser.add_argument('-fu', '--function', type=auto_int)
args = parser.parse_args()

single = (args.bus != None and args.slot != None and args.function != None)

if not single:
    args.bus = 0
    args.slot = 0
    args.function = 0

if args.filename:
    fp = open(args.filename, "w+")
else:
    fp = None

fprintf(fp, "PCI dump (using WD_PciConfigDump)\n")

dwStatus = wdapi.WDC_DriverOpen(WDC_DRV_OPEN_CHECK_VER, 0)
if dwStatus:
    print("WDC_DriverOpen: " + Stat2Str(dwStatus))
    exit

if single:
    pSlot = WD_PCI_SLOT(args.bus, args.slot, args.function)
    WDC_DIAG_PciDeviceInfoPrintFile(pSlot, fp, True)
else:
    WDC_DIAG_PciDevicesInfoPrintAllFile(fp, True)

dwStatus = wdapi.WDC_DriverClose()
if dwStatus:
    print("WDC_DriverClose: " + Stat2Str(dwStatus))

if fp:
    fp.close()
