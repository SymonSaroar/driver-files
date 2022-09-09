using System;

using Jungo.wdapi_dotnet;
using static Jungo.wdapi_dotnet.wdc_lib_decl;
using static Jungo.wdapi_dotnet.wdc_lib_consts;
using static Jungo.wdapi_dotnet.windrvr_consts;
using static Jungo.wdapi_dotnet.wdc_defs_macros;
using static Jungo.wdapi_dotnet.wdc_lib_macros;
using static Jungo.wdapi_dotnet.WD_ERROR_CODES;

using System.Collections.Generic;
using ConsoleTables;
using System.Text;

using DWORD = System.UInt32;

namespace Jungo.diag_lib
{
    public static class wdc_diag_lib
    {
        public const UInt32 ACTIVE_ADDR_SPACE_NEEDS_INIT = 0xFF;
        const UInt32 PCIE_REGS_NUM = 68;

        public class WDC_REG
        {
            public DWORD AddrSpace { get; set; }
            public DWORD Offset { get; set; }
            public UInt32 Size { get; set; }
            public WDC_DIRECTION Direction { get; set; }
            public string Name { get; set; }
            public string Description { get; set; }

            public WDC_REG(UInt32 addrSpace = 0, UInt32 offset = 0,
                UInt32 size = (UInt32)WDC_SIZE_32,
                WDC_DIRECTION direction = WDC_DIRECTION.WDC_READ_WRITE,
                string name = "", string description = "")
            {
                AddrSpace = addrSpace;
                Offset = offset;
                Size = size;
                Direction = direction;
                Name = name;
                Description = description;
            }
        }

        [Flags]
        public enum REG_PRINT
        {
            None = 0,
            Name = 1 << 0,
            Desc = 1 << 1,
            AddrSpace = 1 << 2,
            Offset = 1 << 3,
            Size = 1 << 4,
            Direction = 1 << 5,

            Default = Name | Direction | Desc,
            All = Name | Desc | AddrSpace | Offset | Size | Direction
        }

        public class ADDR_SPACE_INFO
        {
            public DWORD AddrSpace { get; set; }
            public string Type { get; set; }
            public string Name { get; set; }
            public string Desc { get; set; }
        }

        public static string IntTypeDescriptionGet(DWORD dwIntType)
        {
            if ((dwIntType & (DWORD)WD_INTERRUPT_TYPE.INTERRUPT_MESSAGE_X) > 0)
                return "Extended Message-Signaled Interrupt (MSI-X)";
            if ((dwIntType & (DWORD)WD_INTERRUPT_TYPE.INTERRUPT_MESSAGE) > 0)
                return "Message-Signaled Interrupt (MSI)";
            if ((dwIntType & (DWORD)WD_INTERRUPT_TYPE.INTERRUPT_LEVEL_SENSITIVE) > 0)
                return "Level-Sensitive Interrupt";

            return "Edge-Triggered Interrupt";
        }

        public static string DeviceResourcesPrint(in WD_CARD card,
          bool isRegisteredDevice)
        {
            string res = "";
            int resources = 0;

            for (DWORD i = 0; i < card.dwItems; i++)
            {
                WD_ITEMS item = card.Item[i];

                switch ((ITEM_TYPE)item.item)
                {
                    case ITEM_TYPE.ITEM_MEMORY:
                        resources++;
                        res += String.Format("\tMemory range [BAR {0}]: base 0x{1:X} " +
                            "size 0x{2:X}", item.I.Mem.dwBar,
                            item.I.Mem.pPhysicalAddr, item.I.Mem.qwBytes) + Environment.NewLine;

                        if (isRegisteredDevice)
                        {
                            res += String.Format("\tKernel-mode address mapping: 0x{0:X}",
                                item.I.Mem.pTransAddr) + Environment.NewLine;
                            res += String.Format("\tUser-mode address mapping: 0x{0:X}",
                                item.I.Mem.pUserDirectAddr) + Environment.NewLine;
                        }
                        break;

                    case ITEM_TYPE.ITEM_IO:
                        resources++;
                        res += String.Format("\tI/O range [BAR {0}]: base [0x{1:X}], size " +
                            "[0x{2:X}]", item.I.IO.dwBar, item.I.IO.pAddr,
                            item.I.IO.dwBytes) + Environment.NewLine;
                        break;

                    case ITEM_TYPE.ITEM_INTERRUPT:
                        resources++;
                        res += String.Format("\tInterrupt: IRQ {0}",
                            item.I.Int.dwInterrupt) + Environment.NewLine;
                        res += "\tInterrupt Options (supported interrupts):" + Environment.NewLine;

                        if (WDC_INT_IS_MSI(item.I.Int.dwOptions))
                        {
                            res += String.Format("\t\t{0}",
                                IntTypeDescriptionGet(item.I.Int.dwOptions)) + Environment.NewLine;
                        }
                        /* According to the MSI specification, it is recommended that
                         *a PCI device will support both MSI/ MSI - X and level-sensitive
                           * interrupts, and allow the operating system to choose which
                                             *type of interrupt to use. */
                        if ((item.I.Int.dwOptions & (DWORD)WD_INTERRUPT_TYPE.INTERRUPT_LEVEL_SENSITIVE) > 0)
                        {
                            String.Format("\t\t{0}",
                                IntTypeDescriptionGet((DWORD)WD_INTERRUPT_TYPE.INTERRUPT_LEVEL_SENSITIVE));
                        }
                        else if (!WDC_INT_IS_MSI(item.I.Int.dwOptions))
                        /* MSI/MSI-X interrupts are always edge-triggered, so there is no
                         * no need to display a specific edge-triggered indication for
                         * such interrupts. */
                        {
                            res += String.Format("\t\t{0}",
                                IntTypeDescriptionGet((DWORD)WD_INTERRUPT_TYPE.INTERRUPT_LATCHED));
                        }
                        break;
                    case ITEM_TYPE.ITEM_BUS:
                        break;
                    default:
                        res += String.Format("\tInvalid item type (0x{0:X})",
                            item.item) + Environment.NewLine;
                        break;
                }
            }

            if (resources == 0)
                res += "\tDevice has no resources" + Environment.NewLine;

            return res;
        }

        public static string GetPciDeviceInfo(WD_PCI_ID pciId, WD_PCI_SLOT slot)
        {
            DWORD dwStatus, dwExpressGen;
            StringBuilder sb = new();

            sb.AppendFormat("Vendor ID: 0x{0:X}, Device ID: 0x{1:X} " +
                "{2}\tLocation: ", pciId.dwVendorId, pciId.dwDeviceId,
                Environment.NewLine);
            sb.AppendFormat("Domain [0x{0:x}], Bus [0x{1:x}], Slot " +
                "[0x{2:x}], Function [0x{3:x}] {4}", slot.dwDomain, slot.dwBus,
                slot.dwSlot, slot.dwFunction, Environment.NewLine);

            WD_PCI_CARD_INFO deviceInfo = new()
            {
                pciSlot = slot
            };

            dwStatus = WDC_PciGetDeviceInfo(ref deviceInfo);
            if ((DWORD)WD_NO_RESOURCES_ON_DEVICE != dwStatus &&
                 (DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "Failed retrieving PCI resources information");
            }

            sb.AppendFormat(DeviceResourcesPrint(in deviceInfo.Card, false));

            dwExpressGen = WDC_PciGetExpressGenBySlot(ref slot);
            if (dwExpressGen != 0)
            {
                sb.AppendFormat("\tPCI Express Generation: Gen{0} {1}",
                    dwExpressGen, Environment.NewLine);
            }

            return sb.ToString();
        }

        public static void PciDevicesInfoPrintAll()
        {
            WDC_PCI_SCAN_RESULT scanResult = pci_lib.DeviceFind(0, 0);

            if (scanResult.dwNumDevices == 0)
            {
                Console.WriteLine("No devices were found on the PCI bus");
            }
            else
            {
                Console.WriteLine();
                Console.WriteLine("Found {0} devices on the PCI bus:",
                    scanResult.dwNumDevices);
                Console.WriteLine("---------------------------------");

                for (DWORD i = 0; i < scanResult.dwNumDevices; i++)
                {
                    Console.WriteLine(GetPciDeviceInfo(scanResult.deviceId[i],
                        scanResult.deviceSlot[i]));

                    Console.WriteLine("Press ENTER to continue");
                    Console.ReadLine();
                }
            }
        }

        private static void PciRegsPrint(IList<WDC_REG> cfgRegs,
            bool isExpress, DWORD pciExpressOffset)
        {
            UInt32 extended = isExpress ? PCIE_REGS_NUM : 0;

            Console.WriteLine("PCI {0}Registers", isExpress ? "Express " : "");
            Console.WriteLine("----{0}---------", isExpress ? "--------" : "");

            ConsoleTable cfgRegsTable = new("Ix", "Name", "Offset", "Size",
                "R/W", "Description");

            for (int i = 0; i < cfgRegs.Count; i++)
            {
                WDC_REG cfgReg = cfgRegs[i];
                string direction =
                    (cfgReg.Direction == WDC_DIRECTION.WDC_READ_WRITE) ?
                    "RW" : "R";
                DWORD offset = cfgReg.Offset + pciExpressOffset;

                cfgRegsTable.AddRow((i + 1) + extended, cfgReg.Name,
                    $"0x{offset:X}", cfgReg.Size, direction,
                    cfgReg.Description);
            }

            cfgRegsTable.Write(Format.Minimal);
        }

        public static void RegsInfoPrint(WDC_DEVICE wdcDevice,
            IList<WDC_REG> cfgRegs, bool isExpress)
        {
            DWORD dwStatus, pciExpressOffset = 0;
            WDC_PCI_HEADER_TYPE headerType = new();

            if (wdcDevice.hDev == IntPtr.Zero)
            {
                throw new WinDriverException(WD_INVALID_HANDLE,
                    "NULL WDC device handle");
            }

            if (isExpress)
            {
                dwStatus = WDC_PciGetExpressOffset(wdcDevice.hDev,
                    ref pciExpressOffset);
                if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
                {
                    throw new WinDriverException(dwStatus,
                        "Error getting PCI Express Offset");
                }
            }

            dwStatus = WDC_PciGetHeaderType(wdcDevice.hDev, ref headerType);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                       "Unable to determine PCI header type");
            }

            if (cfgRegs.Count == 0)
            {
                throw new WinDriverException(WD_INVALID_PARAMETER,
                  "There are currently no are currently no pre-defined " +
                  "registers to display");
            }

            Console.WriteLine();

            PciRegsPrint(cfgRegs, isExpress, pciExpressOffset);
        }

        public static void PciSlotPrint(in WD_PCI_SLOT pciSlot)
        {
            Console.Write("Domain [0x{0:x}], Bus [0x{1:x}], Slot [0x{2:x}], " +
                "Function [0x{3:x}]", pciSlot.dwDomain, pciSlot.dwBus,
                pciSlot.dwSlot, pciSlot.dwFunction);
        }

        public static void SetMode(out WDC_ADDR_MODE mode)
        {
            Console.WriteLine("");
            Console.WriteLine("Select read/write mode:");
            Console.WriteLine("-----------------------");
            Console.WriteLine("1. 8 bits (1 bytes)", WDC_ADDR_MODE.WDC_MODE_8);
            Console.WriteLine("2. 16 bits (2 bytes)", WDC_ADDR_MODE.WDC_MODE_16);
            Console.WriteLine("3. 32 bits (3 bytes)", WDC_ADDR_MODE.WDC_MODE_32);
            Console.WriteLine("4. 64 bits (4 bytes)", WDC_ADDR_MODE.WDC_MODE_64);
            Console.WriteLine("{0}. Exit Menu", DiagMenu.EXIT_MENU);

            DWORD option = Diag.InputDword(false, "Enter option", 1, 4);

            mode = option switch
            {
                1 => WDC_ADDR_MODE.WDC_MODE_8,
                2 => WDC_ADDR_MODE.WDC_MODE_16,
                3 => WDC_ADDR_MODE.WDC_MODE_32,
                4 => WDC_ADDR_MODE.WDC_MODE_64,
                _ => WDC_ADDR_MODE.WDC_MODE_32,
            };
        }

        public static void ReadWriteBlock(WDC_DEVICE wdcDevice,
            WDC_DIRECTION direction, DWORD dwAddrSpace)
        {
            IntPtr pData = IntPtr.Zero;
            WDC_ADDR_RW_OPTIONS options;
            DWORD dwStatus, dwOffset, dwBytes, yesNoAutoInc;
            byte[] pBuf;
            string directionStr = (direction == WDC_DIRECTION.WDC_READ) ?
                "read" : "write";

            if (wdcDevice.hDev == IntPtr.Zero)
            {
                throw new WinDriverException(WD_INVALID_HANDLE,
                    "Error- NULL WDC device handle");
            }

            dwOffset = Diag.InputDword(true, "Enter offset");
            dwBytes = Diag.InputDword(true, "Enter bytes", 1, UInt32.MaxValue);

            pBuf = new byte[dwBytes];

            if (WDC_DIRECTION.WDC_WRITE == direction)
            {
                Console.Write("data to write (hex format): 0x");
                if (Diag.GetHexBuffer(in pBuf) == 0)
                {
                    throw new WinDriverException(WD_INVALID_PARAMETER,
                        "Data cannot be empty");
                }
            }

            if (WDC_AD_CFG_SPACE == dwAddrSpace)
            {
                WD_BUS_TYPE busType = WDC_GetBusType(wdcDevice.hDev);

                if (WD_BUS_TYPE.WD_BUS_PCI == busType)
                {
                    pData = System.Runtime.InteropServices.Marshal.
                        AllocHGlobal((int)dwBytes);

                    if (direction == WDC_DIRECTION.WDC_READ)
                    {
                        dwStatus = WDC_PciReadCfg(wdcDevice.hDev, dwOffset,
                            pData, dwBytes);
                        System.Runtime.InteropServices.Marshal.Copy(pData, pBuf, 0,
                            (int)dwBytes);
                    }
                    else
                    {
                        System.Runtime.InteropServices.Marshal.Copy(pBuf, 0,
                            pData, (int)dwBytes);
                        dwStatus = WDC_PciWriteCfg(wdcDevice.hDev,
                            dwOffset, pData, dwBytes);
                    }
                }
                else
                {
                    throw new WinDriverException(
                        WD_NO_RESOURCES_ON_DEVICE,
                        "Cannot read/write configuration space " +
                        $"address space for bus type [0x{busType:X}]");
                }
            }
            else
            {
                SetMode(out WDC_ADDR_MODE mode);

                Console.WriteLine("Do you wish to increment the address after " +
                    "each {0} block (0x{1:X} bytes) (0 - No, Otherwise - Yes)? ",
                    directionStr, WDC_ADDR_MODE_TO_SIZE(mode));

                yesNoAutoInc = Diag.InputDword(false);
                options = (yesNoAutoInc == 0) ?
                    WDC_ADDR_RW_OPTIONS.WDC_ADDR_RW_NO_AUTOINC :
                    WDC_ADDR_RW_OPTIONS.WDC_ADDR_RW_DEFAULT;

                if (direction == WDC_DIRECTION.WDC_READ)
                {
                    dwStatus = WDC_ReadAddrBlock(wdcDevice.hDev,
                        dwAddrSpace, dwOffset, dwBytes, pBuf, mode, options);
                }
                else
                {
                    dwStatus = WDC_WriteAddrBlock(wdcDevice.hDev,
                        dwAddrSpace, dwOffset, dwBytes, pBuf, mode, options);
                }
            }

            if ((DWORD)WD_STATUS_SUCCESS == dwStatus)
            {
                if (WDC_DIRECTION.WDC_READ == direction)
                {
                    Diag.PrintHexBuffer(pBuf);
                }
                else
                {
                    Console.WriteLine($"Wrote 0x{dwBytes:X} bytes to offset 0x{dwOffset:X}");
                }
            }
            else
            {
                string fromToStr = (direction == WDC_DIRECTION.WDC_READ) ?
                    "from" : "to";
                throw new WinDriverException(dwStatus,
                    $"Failed to {directionStr} 0x{dwBytes:X} bytes {fromToStr} offset " +
                    $"0x{dwOffset:X}");
            }

            if (pData != IntPtr.Zero)
                System.Runtime.InteropServices.Marshal.FreeHGlobal(pData);

            Console.WriteLine("Press ENTER to continue");
            Console.ReadLine();
        }

        private static string ReadRegData(WDC_DEVICE wdcDevice, WDC_REG cfgReg,
            bool isCfgRegs, DWORD pciExpressOffset)
        {
            DWORD dwStatus;
            string res = "0X";
            byte bData = 0;
            UInt16 wData = 0;
            UInt32 u32Data = 0;
            UInt64 u64Data = 0;
            IntPtr hDev = wdcDevice.hDev;

            switch (cfgReg.Size)
            {
                case WDC_SIZE_8:
                    dwStatus = isCfgRegs ?
                        WDC_PciReadCfg8(hDev,
                            cfgReg.Offset + pciExpressOffset, ref bData) :
                        WDC_ReadAddr8(hDev, cfgReg.AddrSpace,
                            cfgReg.Offset + pciExpressOffset, ref bData);
                    if ((DWORD)WD_STATUS_SUCCESS == dwStatus)
                        res += bData.ToString("X2");
                    break;

                case WDC_SIZE_16:
                    dwStatus = isCfgRegs ?
                        WDC_PciReadCfg16(hDev,
                            cfgReg.Offset + pciExpressOffset, ref wData) :
                        WDC_ReadAddr16(hDev, cfgReg.AddrSpace,
                            cfgReg.Offset + pciExpressOffset, ref wData);
                    if ((DWORD)WD_STATUS_SUCCESS == dwStatus)
                        res += wData.ToString("X2");
                    break;

                case WDC_SIZE_32:
                    dwStatus = isCfgRegs ?
                        WDC_PciReadCfg32(hDev,
                            cfgReg.Offset + pciExpressOffset, ref u32Data) :
                        WDC_ReadAddr32(hDev, cfgReg.AddrSpace,
                            cfgReg.Offset + pciExpressOffset, ref u32Data);
                    if ((DWORD)WD_STATUS_SUCCESS == dwStatus)
                        res += u32Data.ToString("X2");
                    break;

                case WDC_SIZE_64:
                    dwStatus = isCfgRegs ?
                        WDC_PciReadCfg64(hDev,
                            cfgReg.Offset + pciExpressOffset, ref u64Data) :
                        WDC_ReadAddr64(hDev, cfgReg.AddrSpace,
                            cfgReg.Offset + pciExpressOffset, ref u64Data);
                    if ((DWORD)WD_STATUS_SUCCESS == dwStatus)
                        res += u64Data.ToString("X2");
                    break;
            }

            return res;
        }

        public static void ReadRegsAll(WDC_DEVICE wdcDevice,
            IList<WDC_REG> cfgRegs, bool isCfgRegs, bool isExpress)
        {
            DWORD dwStatus;
            DWORD dwPciExpressOffset = 0;
            WDC_PCI_HEADER_TYPE headerType = new();

            if (wdcDevice.hDev == IntPtr.Zero)
            {
                throw new WinDriverException(WD_INVALID_HANDLE,
                    "NULL WDC device handle");
            }
            if (cfgRegs == null)
            {
                throw new WinDriverException(WD_INVALID_PARAMETER,
                    "NULL registers pointer");
            }
            if (cfgRegs == null)
            {
                throw new WinDriverException(WD_INVALID_PARAMETER,
                    "No registers");
            }

            dwStatus = WDC_PciGetHeaderType(wdcDevice.hDev, ref headerType);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "unable to determine PCI header type");
            }

            if (isExpress)
            {
                dwStatus = WDC_PciGetExpressOffset(wdcDevice.hDev,
                    ref dwPciExpressOffset);
                if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
                {
                    throw new WinDriverException(dwStatus,
                        "Error getting PCI Express offset");
                }
            }

            Console.WriteLine("");
            Console.WriteLine("{0} registers data:", (isCfgRegs && isExpress) ?
                "PCI Express configuration" : isCfgRegs ? "PCI configuration" :
                "run-time");
            Console.WriteLine("---------------------------------");

            ConsoleTable regsTable = new("Ix", "Name", "Data", "Description");

            for (int i = 0; i < cfgRegs.Count; i++)
            {
                WDC_REG cfgReg = cfgRegs[i];
                string data = ReadRegData(wdcDevice, cfgReg, isCfgRegs,
                    dwPciExpressOffset);

                regsTable.AddRow(i + 1, cfgReg.Name, data, cfgReg.Description);
            }

            regsTable.Write(Format.Minimal);

            Console.WriteLine("Press ENTER to continue");
            Console.ReadLine();
        }

        public static UInt64 InputWriteData(UInt32 size)
        {
            UInt64 minVal, maxVal;
            DWORD data;

            switch (size)
            {
                case WDC_SIZE_8:
                    minVal = byte.MinValue;
                    maxVal = byte.MaxValue;
                    break;
                case WDC_SIZE_16:
                    minVal = UInt16.MinValue;
                    maxVal = UInt16.MaxValue;
                    break;
                case WDC_SIZE_32:
                    minVal = UInt32.MinValue;
                    maxVal = UInt32.MaxValue;
                    break;
                case WDC_SIZE_64:
                    minVal = UInt64.MinValue;
                    maxVal = UInt64.MaxValue;
                    break;
                default:
                    throw new WinDriverException(WD_INVALID_PARAMETER,
                        "Invalid size");
            }

            data = Diag.InputDword(true, $"Enter data to write(max value: 0x{maxVal:X})",
                (DWORD)minVal, (DWORD)maxVal);

            return data;
        }

        public static UInt64 ReadWriteReg(WDC_DEVICE wdcDevice,
            WDC_REG cfgReg, WDC_DIRECTION direction, bool isCfgRegs)
        {
            DWORD dwStatus = (DWORD)WD_STATUS_SUCCESS;
            byte bData = 0;
            UInt16 wData = 0;
            UInt32 u32Data = 0;
            UInt64 u64Data = 0;
            UInt64 data = 0;

            if (WDC_DIRECTION.WDC_WRITE == direction)
                data = InputWriteData(cfgReg.Size);

            switch (cfgReg.Size)
            {
                case WDC_SIZE_8:
                    if (isCfgRegs)
                    {
                        dwStatus = (WDC_DIRECTION.WDC_READ == direction) ?
                            WDC_PciReadCfg8(wdcDevice.hDev, cfgReg.Offset, ref bData) :
                            WDC_WriteAddr8(wdcDevice.hDev, cfgReg.AddrSpace,
                                cfgReg.Offset, (byte)data);
                        data = bData;
                    }
                    else
                    {
                        dwStatus = (WDC_DIRECTION.WDC_READ == direction) ?
                            WDC_ReadAddr8(wdcDevice.hDev, cfgReg.AddrSpace,
                                cfgReg.Offset, ref bData) :
                            WDC_WriteAddr8(wdcDevice.hDev, cfgReg.AddrSpace,
                                cfgReg.Offset, (byte)data);
                        data = bData;
                    }
                    break;
                case WDC_SIZE_16:
                    if (isCfgRegs)
                    {
                        dwStatus = (WDC_DIRECTION.WDC_READ == direction) ?
                            WDC_PciReadCfg16(wdcDevice.hDev, cfgReg.Offset, ref wData) :
                            WDC_WriteAddr16(wdcDevice.hDev, cfgReg.AddrSpace,
                                cfgReg.Offset, (UInt16)data);
                        data = wData;
                    }
                    else
                    {
                        dwStatus = (WDC_DIRECTION.WDC_READ == direction) ?
                        WDC_ReadAddr16(wdcDevice.hDev, cfgReg.AddrSpace,
                            cfgReg.Offset, ref wData) :
                        WDC_WriteAddr16(wdcDevice.hDev, cfgReg.AddrSpace,
                            cfgReg.Offset, (UInt16)data);
                        data = wData;
                    }
                    break;
                case WDC_SIZE_32:
                    if (isCfgRegs)
                    {
                        dwStatus = (WDC_DIRECTION.WDC_READ == direction) ?
                            WDC_PciReadCfg32(wdcDevice.hDev, cfgReg.Offset,
                                ref u32Data) :
                            WDC_WriteAddr32(wdcDevice.hDev, cfgReg.AddrSpace,
                                cfgReg.Offset, (UInt32)data);
                        data = u32Data;
                    }
                    else
                    {
                        dwStatus = (WDC_DIRECTION.WDC_READ == direction) ?
                        WDC_ReadAddr32(wdcDevice.hDev, cfgReg.AddrSpace,
                            cfgReg.Offset, ref u32Data) :
                        WDC_WriteAddr32(wdcDevice.hDev, cfgReg.AddrSpace,
                            cfgReg.Offset, (UInt32)data);
                        data = u32Data;
                    }
                    break;
                case WDC_SIZE_64:
                    if (isCfgRegs)
                    {
                        dwStatus = (WDC_DIRECTION.WDC_READ == direction) ?
                            WDC_PciReadCfg64(wdcDevice.hDev, cfgReg.Offset,
                                ref u64Data) :
                            WDC_WriteAddr64(wdcDevice.hDev, cfgReg.AddrSpace,
                                cfgReg.Offset, (UInt64)data);
                        data = u64Data;
                    }
                    else
                    {
                        dwStatus = (WDC_DIRECTION.WDC_READ == direction) ?
                        WDC_ReadAddr64(wdcDevice.hDev, cfgReg.AddrSpace,
                            cfgReg.Offset, ref u64Data) :
                        WDC_WriteAddr64(wdcDevice.hDev, cfgReg.AddrSpace,
                            cfgReg.Offset, (UInt64)data);
                        data = u64Data;
                    }
                    break;
            }

            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                string directionErrorStr = (WDC_DIRECTION.WDC_READ == direction) ?
                    "reading data from" : "writing data to";
                string regId = string.IsNullOrEmpty(cfgReg.Name) ?
                    $"offset 0x{cfgReg.Offset}" : cfgReg.Name;

                throw new WinDriverException(dwStatus,
                    $"Failed {directionErrorStr} {regId}");
            }

            return data;
        }

        public static void ReadWriteRegFromList(WDC_DEVICE wdcDevice,
            IList<WDC_REG> cfgRegs, WDC_DIRECTION direction, bool isCfgRegs,
            bool isExpress)
        {
            UInt64 data;
            DWORD dwStatus, reg;
            DWORD pciExpressOffset = 0;
            WDC_PCI_HEADER_TYPE headerType = 0;
            WDC_REG cfgReg;

            if (wdcDevice.hDev == IntPtr.Zero)
            {
                throw new WinDriverException(WD_INVALID_HANDLE,
                    "NULL WDC device handle");
            }

            isExpress &= WDC_PciGetExpressGen(wdcDevice.hDev) != 0;
            if (isExpress)
            {
                dwStatus = WDC_PciGetExpressOffset(wdcDevice.hDev,
                    ref pciExpressOffset);
                if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
                {
                    throw new WinDriverException(dwStatus,
                        "Error getting PCI Express offset");
                }
            }

            dwStatus = WDC_PciGetHeaderType(wdcDevice.hDev, ref headerType);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "unable to determine PCI header type");
            }

            if (cfgRegs.Count == 0)
            {
                throw new WinDriverException(WD_INVALID_PARAMETER,
                  "No registers to read/write");
            }

            /* Display pre-defined registers' information */
            Console.WriteLine();
            Console.WriteLine("PCI {0} registers:\n",
                isCfgRegs ? "configuration" : "run-time");
            Console.WriteLine("----------------------------\n");

            RegsInfoPrint(wdcDevice, cfgRegs, isExpress);

            /* Read/write register */
            Console.WriteLine("");
            string message = string.Format("Select a register from the list above to {0}",
                (WDC_DIRECTION.WDC_READ == direction) ? "read from" : "write to");

            reg = Diag.InputDword(false, message, 1, (DWORD)cfgRegs.Count);

            cfgReg = cfgRegs[(int)reg - 1];

            if (((WDC_DIRECTION.WDC_READ == direction) &&
                (WDC_DIRECTION.WDC_WRITE == cfgReg.Direction)) ||
                ((WDC_DIRECTION.WDC_WRITE == direction) &&
                (WDC_DIRECTION.WDC_READ == cfgReg.Direction)))
            {
                throw new WinDriverException(WD_INVALID_PARAMETER,
                    string.Format("you have selected to {0} a {1}-only register",
                    (WDC_DIRECTION.WDC_READ == direction) ? "read from" : "write to",
                    (WDC_DIRECTION.WDC_WRITE == cfgReg.Direction) ? "write" : "read"));
            }

            data = ReadWriteReg(wdcDevice, cfgReg, direction, isCfgRegs);

            string directionStr = (direction == WDC_DIRECTION.WDC_READ) ?
                "Read" : "Wrote";
            string fromToStr = (direction == WDC_DIRECTION.WDC_READ) ?
                "from" : "to";
            DWORD offset = isExpress ?
                cfgReg.Offset + pciExpressOffset : cfgReg.Offset;

            Console.WriteLine($"{directionStr} 0x{data:X} {fromToStr} " +
                $"register {cfgReg.Name} at offset [0x{offset:X}]");

            StringBuilder sbDetails = new(1024);
            DWORD outBufLen = 0;

            dwStatus = isExpress ?
                PciExpressConfRegData2Str(wdcDevice.hDev,
                    cfgReg.Offset, sbDetails, 1024, ref outBufLen) :
                PciConfRegData2Str(wdcDevice.hDev,
                    cfgReg.Offset, sbDetails, 1024, ref outBufLen);

            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    $"Failed to decode register data");
            }

            if (WDC_DIRECTION.WDC_READ == direction && !sbDetails.Equals(""))
                Console.WriteLine($"Decoded register data: {sbDetails}");

            Console.WriteLine();
            Console.WriteLine("Press ENTER to continue");
            Console.ReadLine();
        }

        public static void PciScanCaps(WDC_DEVICE wdcDevice,
            DWORD dwCapId = WD_PCI_CAP_ID_ALL)
        {
            DWORD dwStatus;
            WDC_PCI_SCAN_CAPS_RESULT scanResult = new();

            if (wdcDevice.hDev == IntPtr.Zero)
            {
                throw new WinDriverException(WD_INVALID_HANDLE,
                    "NULL WDC device handle");
            }

            dwStatus = WDC_PciScanCaps(wdcDevice.hDev, dwCapId, ref scanResult);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "Failed scanning PCI capabilities");
            }

            Console.WriteLine("{0}PCI capabilities found",
                scanResult.dwNumCaps > 0 ? "" : "No ");

            for (DWORD i = 0; i < scanResult.dwNumCaps; i++)
            {
                Console.WriteLine("\t{0}) {1} - ID [0x{2:x}], offset [0x{3:x}]",
                    i + 1,
                    GetCapabilityStr(scanResult.pciCaps[i].dwCapId),
                    scanResult.pciCaps[i].dwCapId,
                    scanResult.pciCaps[i].dwCapOffset);
            }

            Console.WriteLine();
        }

        public static void ReadWriteFromBar(WDC_DEVICE wdcDevice,
            DWORD addrSpace, WDC_ADDR_MODE mode, WDC_DIRECTION direction)
        {
            UInt64 data;
            DWORD offset = Diag.InputDword(true, "Enter offset");

            WDC_REG wdcReg = new()
            {
                AddrSpace = addrSpace,
                Size = (UInt32)mode,
                Offset = offset
            };

            data = ReadWriteReg(wdcDevice, wdcReg, direction, false);

            string directionStr = (direction == WDC_DIRECTION.WDC_READ) ?
             "Read" : "Wrote";
            string fromToStr = (direction == WDC_DIRECTION.WDC_READ) ?
                "from" : "to";

            Console.WriteLine($"{directionStr} 0x{data:X} {fromToStr} " +
                $"offset [0x{offset:X}] in BAR {addrSpace}");
        }

        public static void PciExpressScanCaps(WDC_DEVICE wdcDevice,
            DWORD dwCapId = WD_PCI_CAP_ID_ALL)
        {
            DWORD dwStatus;
            WDC_PCI_SCAN_CAPS_RESULT scanResult = new();

            if (wdcDevice.hDev == IntPtr.Zero)
            {
                throw new WinDriverException(WD_INVALID_HANDLE,
                    "NULL WDC device handle");
            }

            dwStatus = WDC_PciScanExtCaps(wdcDevice.hDev, dwCapId,
                ref scanResult);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "Failed scanning PCI express capabilities");
            }

            Console.WriteLine("{0}PCI Express extended capabilities found",
                scanResult.dwNumCaps > 0 ? "" : "No ");

            for (DWORD i = 0; i < scanResult.dwNumCaps; i++)
            {
                Console.WriteLine("\t{0}) {1} - ID [0x{2:x}], offset [0x{3:x}]",
                    i + 1,
                    GetExtendedCapabilityStr(scanResult.pciCaps[i].dwCapId),
                    scanResult.pciCaps[i].dwCapId,
                    scanResult.pciCaps[i].dwCapOffset);
            }

            Console.WriteLine();
        }

        public static void GetAddrSpaceInfo(WDC_DEVICE wdcDevice, ADDR_SPACE_INFO addrSpaceInfo)
        {
            WDC_ADDR_DESC AddrDesc;

            if (wdcDevice.hDev == IntPtr.Zero)
            {
                throw new WinDriverException(WD_INVALID_HANDLE,
                    "NULL WDC device handle");
            }

            if (addrSpaceInfo.AddrSpace > wdcDevice.dwNumAddrSpaces - 1)
            {
                throw new WinDriverException(WD_INVALID_PARAMETER,
                    $"Address space {addrSpaceInfo.AddrSpace} is out of " +
                    $"range (0 - {wdcDevice.dwNumAddrSpaces - 1})");
            }

            AddrDesc = wdcDevice.pAddrDesc[addrSpaceInfo.AddrSpace];

            addrSpaceInfo.Name = $"BAR {addrSpaceInfo.AddrSpace}";
            addrSpaceInfo.Type = WDC_ADDR_IS_MEM(AddrDesc) ? "Memory" : "I/O";

            if (AddrDesc.qwBytes > 0)
            {
                WD_ITEMS item = wdcDevice.cardReg.Card.Item[AddrDesc.dwItemIndex];
                ulong pAddr = WDC_ADDR_IS_MEM(AddrDesc) ? item.I.Mem.pPhysicalAddr :
                    item.I.IO.pAddr;

                addrSpaceInfo.Desc = $"0x{pAddr:X16} - " +
                    $"0x{pAddr + AddrDesc.qwBytes - 1:X16} (0x{AddrDesc.qwBytes:x} bytes)";
            }
            else
            {
                addrSpaceInfo.Desc = "Inactive address space";
            }
        }
    }
}


