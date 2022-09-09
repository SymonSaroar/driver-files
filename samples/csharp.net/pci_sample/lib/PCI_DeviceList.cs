/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

// Note: This code sample is provided AS-IS and as a guiding sample only.

using System;
using System.Collections;

using Jungo.wdapi_dotnet;
using wdc_err = Jungo.wdapi_dotnet.WD_ERROR_CODES;
using DWORD = System.UInt32;
using BOOL = System.Boolean;
using WDC_DRV_OPEN_OPTIONS = System.UInt32;

namespace Jungo.pci_lib
{
    public class PCI_DeviceList: ArrayList
    {
        /* WinDriver license registration string */
        /* TODO: When using a registered WinDriver version, replace the license
                 string below with the development license in order to use on
                 the development machine.
                 Once you require to distribute the driver's package to other
                 machines, please replace the string with a distribution
                 license */
        private string PCI_DEFAULT_LICENSE_STRING = "12345abcde1234.license";

        // TODO: If you have renamed the WinDriver kernel module
        // (windrvr1511.sys), change the driver name below accordingly
        private string PCI_DEFAULT_DRIVER_NAME = null;
        private DWORD PCI_DEFAULT_VENDOR_ID = 0x0; /* any vendor */
        private DWORD PCI_DEFAULT_DEVICE_ID = 0x0; /* any device */

        private static PCI_DeviceList instance;

        public static PCI_DeviceList TheDeviceList()
        {
            if (instance == null)
            {
                instance = new PCI_DeviceList();
            }
            return instance;
        }

        private PCI_DeviceList(){}

        public DWORD Init()
        {
            if (PCI_DEFAULT_DRIVER_NAME != null &&
                windrvr_decl.WD_DriverName(PCI_DEFAULT_DRIVER_NAME) == null)
            {
                Log.ErrLog("PCI_DeviceList.Init: Failed to set driver name for "
                    + "the WDC library.");
                return (DWORD)wdc_err.WD_SYSTEM_INTERNAL_ERROR;
            }

            DWORD dwStatus = wdc_lib_decl.WDC_SetDebugOptions(
                    wdc_lib_consts.WDC_DBG_DEFAULT, null);
            if (dwStatus != (DWORD)wdc_err.WD_STATUS_SUCCESS)
            {
                Log.ErrLog("PCI_DeviceList.Init: Failed to initialize debug " +
                    "options for the WDC library. Error 0x" +
                    dwStatus.ToString("X") + utils.Stat2Str(dwStatus));
                return dwStatus;
            }

            dwStatus = wdc_lib_decl.WDC_DriverOpen(
                (WDC_DRV_OPEN_OPTIONS)wdc_lib_consts.WDC_DRV_OPEN_DEFAULT,
                PCI_DEFAULT_LICENSE_STRING);
            if (dwStatus != (DWORD)wdc_err.WD_STATUS_SUCCESS)
            {
                Log.ErrLog("PCI_DeviceList.Init: Failed to initialize the " +
                    "WDC library. Error 0x" + dwStatus.ToString("X") +
                    utils.Stat2Str(dwStatus));
                return dwStatus;
            }
            return Populate();
        }

        public PCI_Device Get(int index)
        {
            if (index >= this.Count || index < 0)
                return null;
            return (PCI_Device)this[index];
        }

        public PCI_Device Get(WD_PCI_SLOT slot)
        {
            foreach(PCI_Device device in this)
            {
                if (device.IsMySlot(ref slot))
                    return device;
            }
            return null;
        }

        private DWORD Populate()
        {
            DWORD dwStatus;
            WDC_PCI_SCAN_RESULT scanResult = new WDC_PCI_SCAN_RESULT();
#if NET5_0
            dwStatus = wdc_lib_decl.WDC_PciScanDevices(PCI_DEFAULT_VENDOR_ID,
                PCI_DEFAULT_DEVICE_ID, ref scanResult);
#else
            dwStatus = wdc_lib_decl.WDC_PciScanDevices(PCI_DEFAULT_VENDOR_ID,
                PCI_DEFAULT_DEVICE_ID, scanResult);
#endif

            if ((DWORD)wdc_err.WD_STATUS_SUCCESS != dwStatus)
            {
                Log.ErrLog("PCI_DeviceList.Populate: Failed scanning "
                    + "the PCI bus. Error 0x" + dwStatus.ToString("X") +
                    utils.Stat2Str(dwStatus));
                return dwStatus;
            }

            if (scanResult.dwNumDevices == 0)
            {
                Log.ErrLog("PCI_DeviceList.Populate: No matching PCI " +
                    "device was found for search criteria " +
                    PCI_DEFAULT_VENDOR_ID.ToString("X") + ", " +
                    PCI_DEFAULT_DEVICE_ID.ToString("X"));
                return (DWORD)wdc_err.WD_INVALID_PARAMETER;
            }

            for (DWORD i = 0; i < scanResult.dwNumDevices; ++i)
            {
                PCI_Device device;
                WD_PCI_SLOT slot = scanResult.deviceSlot[i];

                device = new PCI_Device(scanResult.deviceId[i].dwVendorId,
                            scanResult.deviceId[i].dwDeviceId, slot);

                this.Add(device);
            }
            return (DWORD)wdc_err.WD_STATUS_SUCCESS;
        }

        public void Dispose()
        {
            foreach (PCI_Device device in this)
                device.Dispose();
            this.Clear();

            DWORD dwStatus = wdc_lib_decl.WDC_DriverClose();

            if (dwStatus != (DWORD)wdc_err.WD_STATUS_SUCCESS)
            {
                Exception excp = new Exception("PCI_DeviceList.Dispose: " +
                    "Failed to uninit the WDC library. Error 0x" +
                    dwStatus.ToString("X") + utils.Stat2Str(dwStatus));
                throw excp;
            }
        }
    };
}

