package com.jungo.shared;

import java.nio.ByteBuffer;

import com.jungo.*;
import com.jungo.wdapi.*;

/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

    /******************************************************************************
    *  File: wdc_diag_lib.c - Implementation of shared WDC PCI and ISA  devices'  *
    *  user-mode diagnostics API.                                                 *
    *                                                                             *
    *  Note: This code sample is provided AS-IS and as a guiding sample only.     *
    *******************************************************************************/

public class WdcDiagLib {

    /* Print run-time registers and PCI configuration registers information */
    public final static int
    READ_REGS_ALL_DESC_INDENT      = 39,
    READ_REGS_ALL_DESC_WIDTH       = 22,
    READ_REGS_ALL_DETAILS_INDENT   = 61,
    READ_REGS_ALL_DETAILS_WIDTH    = 20,
    REGS_INFO_PRINT_DETAILS_INDENT = 54,
    REGS_INFO_PRINT_DETAILS_WIDTH  = 22,
    PCIE_REGS_NUM                  = 68,

    WDC_DIAG_REG_PRINT_NAME       = 0x1,
    WDC_DIAG_REG_PRINT_DESC       = 0x2,
    WDC_DIAG_REG_PRINT_ADDR_SPACE = 0x4,
    WDC_DIAG_REG_PRINT_OFFSET     = 0x8,
    WDC_DIAG_REG_PRINT_SIZE       = 0x10,
    WDC_DIAG_REG_PRINT_DIRECTION  = 0x12,
    WDC_DIAG_REG_PRINT_ALL =
        (WDC_DIAG_REG_PRINT_NAME | WDC_DIAG_REG_PRINT_DESC |
        WDC_DIAG_REG_PRINT_ADDR_SPACE | WDC_DIAG_REG_PRINT_OFFSET |
        WDC_DIAG_REG_PRINT_SIZE | WDC_DIAG_REG_PRINT_DIRECTION),
    WDC_DIAG_REG_PRINT_DEFAULT =
        (WDC_DIAG_REG_PRINT_NAME | WDC_DIAG_REG_PRINT_DIRECTION |
            WDC_DIAG_REG_PRINT_DESC),

    /* Device configuration space identifier (PCI configuration space) */
    WDC_AD_CFG_SPACE = 0xFF;

    /* Register information struct */
    public static class WDC_REG {
        public int dwAddrSpace; /* Number of address space in which the register
                                    resides */
                               /* For PCI configuration registers, use
                                 * WDC_AD_CFG_SPACE */
        public int dwOffset;   /* Offset of the register in the dwAddrSpace
                                * address space */
        public int dwSize;     /* Register's size (in bytes) */
        public int direction;  /* Read/write access mode - see WDC_DIRECTION
                                * options */
        public String sName;   /* Register's name */
        public String sDesc;   /* Register's description */

        public WDC_REG(int dwAddrSpace, int dwOffset, int dwSize,
            int direction, String sName, String sDesc)
        {
            this.dwAddrSpace = dwAddrSpace;
            this.dwOffset = dwOffset;
            this.dwSize = dwSize;
            this.direction = direction;
            this.sName = sName;
            this.sDesc = sDesc;
        }
    }

    public static class ADDR_SPACE_INFO{
        public int dwAddrSpace;
        public String sType;
        public String sName;
        public String sDesc;
    }

    public static class WDCContext
    {
        public ByteBuffer data;
    }

    public static abstract class WDC_DEV_ADDR_DESC extends WDCContext
    {
        public abstract void setdwNumAddrSpaces(int dwNumAddrSpaces);
        public abstract void setpAddrDesc(long pAddrDesc);
        public abstract long getdwNumAddrSpaces();
        public abstract long getpAddrDesc();
    }
    /* -----------------------------------------------
        PCI/ISA
       ----------------------------------------------- */
    private static boolean IsValidDevice(WDC_DEVICE pDev, String sFunc)
    {
        if (pDev == null)
        {
            System.err.printf("%s: null device handle\n", sFunc);
            return false;
        }

        return true;
    }

    public static int WDC_DIAG_GetNumAddrSpaces(WDC_DEVICE pDev)
    {
        /* Validate the device handle */
        if (!IsValidDevice(pDev, "WDC_DIAG_GetNumAddrSpaces"))
            return 0;

        /* Return the number of address spaces for the device */
        return pDev.dwNumAddrSpaces;
    }

    public static boolean WDC_DIAG_GetAddrSpaceInfo(WDC_DEVICE pDev,
        ADDR_SPACE_INFO pAddrSpaceInfo)
    {
        WDC_ADDR_DESC pAddrDesc;
        int dwAddrSpace;
        boolean fIsMemory;

        if (!IsValidDevice(pDev, "WDC_DIAG_GetNumAddrSpaces"))
            return false;

        dwAddrSpace = pAddrSpaceInfo.dwAddrSpace;

        if (dwAddrSpace > pDev.dwNumAddrSpaces - 1)
        {
            System.err.printf("PCI_GetAddrSpaceInfo: Error - Address space %%d is " +
                    "out of range (0 - %d)\n", dwAddrSpace, pDev.dwNumAddrSpaces - 1);
            return false;
        }

        pAddrDesc = pDev.pAddrDesc[dwAddrSpace];

        fIsMemory = wdapi.WDC_ADDR_IS_MEM(pAddrDesc);

        //#ifndef ISA
        pAddrSpaceInfo.sName = String.format("BAR %d", dwAddrSpace);
        //#else /* ifdef ISA */
        //  System.out.printf(pAddrSpaceInfo->sName, MAX_NAME - 1, "AddrSpace %d", dwAddrSpace);
        //#endif /* ifdef ISA */
        pAddrSpaceInfo.sType = fIsMemory ? "Memory" : "I/O";

        if (wdapi.WDC_AddrSpaceIsActive(pDev, dwAddrSpace))
        {
            WD_ITEMS pItem = pDev.cardReg.Card.Item[(int)pAddrDesc.dwItemIndex];
            long pAddr = fIsMemory ? pItem.i_struct.mem_struct.pPhysicalAddr :
                pItem.i_struct.io_struct.pAddr;

            pAddrSpaceInfo.sDesc = String.format("0x%016X - 0x%016X (0x%x bytes)",
                    pAddr, pAddr + pAddrDesc.qwBytes - 1, pAddrDesc.qwBytes);
        }
        else
        {
            pAddrSpaceInfo.sDesc = "Inactive address space";
        }

        /* TODO: You can modify the code above to set a different address space
         * name/description. */

        return true;
    }

    /* Print device's resources information to file.
     * For a registered device (hCard != 0), print also kernel and user-mode
     * mappings of memory address items */
    public static void WDC_DIAG_DeviceResourcesPrint(final wdapi.WD_CARD pCard,
        long hCard)
    {
        int resources, i;
        WD_ITEMS pItem;

        if (pCard == null)
        {
            System.err.printf("WDC_DIAG_DeviceResourcesPrint: Error - NULL" +
                " card pointer\n");
            return;
        }

        for (i = 0, resources = 0; i < pCard.dwItems; i++)
        {
            pItem = pCard.Item[i];
            switch ((int)pItem.item)
            {
            case wdapi.ITEM_MEMORY:
                resources++;
                System.out.printf("    Memory range [BAR %d]: base 0x%X" +
                    ", size 0x%X\n", pItem.i_struct.mem_struct.dwBar,
                    pItem.i_struct.mem_struct.pPhysicalAddr,
                    pItem.i_struct.mem_struct.qwBytes);
                if (hCard != 0) /* Registered device */
                {
                    System.out.printf("        Kernel-mode address mapping:" +
                        " 0x%X\n", pItem.i_struct.mem_struct.pTransAddr);
                    System.out.printf("        User-mode address mapping: " +
                        "0x%X\n", pItem.i_struct.mem_struct.pUserDirectAddr);
                }
                break;

            case wdapi.ITEM_IO:
                resources++;
                System.out.printf("    I/O range [BAR %d]: base [0x%X], size " +
                    "[0x%x]\n", pItem.i_struct.io_struct.dwBar,
                    pItem.i_struct.io_struct.pAddr,
                    pItem.i_struct.io_struct.dwBytes);
                break;

            case wdapi.ITEM_INTERRUPT:
                resources++;
                System.out.printf("    Interrupt: IRQ %d\n",
                    pItem.i_struct.int_struct.dwInterrupt);
                System.out.printf("    Interrupt Options (supported interrupts):\n");
                if (wdapi.WDC_INT_IS_MSI(pItem.i_struct.int_struct.dwOptions))
                {
                    System.out.printf("        %s\n",
                        WDC_DIAG_IntTypeDescriptionGet(
                            pItem.i_struct.int_struct.dwOptions));
                }
                /* According to the MSI specification, it is recommended that
                 * a PCI device will support both MSI/MSI-X and level-sensitive
                 * interrupts, and allow the operating system to choose which
                 * type of interrupt to use. */
                if ((pItem.i_struct.int_struct.dwOptions &
                        wdapi.INTERRUPT_LEVEL_SENSITIVE) != 0)
                {
                    System.out.printf("        %s\n",
                        WDC_DIAG_IntTypeDescriptionGet(
                            wdapi.INTERRUPT_LEVEL_SENSITIVE));
                }
                else if (!wdapi.WDC_INT_IS_MSI(pItem.i_struct.int_struct.dwOptions))
                /* MSI/MSI-X interrupts are always edge-triggered, so there is no
                 * no need to display a specific edge-triggered indication for
                 * such interrupts. */
                {
                    System.out.printf("        %s\n",
                        WDC_DIAG_IntTypeDescriptionGet(wdapi.INTERRUPT_LATCHED));
                }
                break;

            case wdapi.ITEM_BUS:
                break;

            default:
                System.out.printf("    Invalid item type (0x%X)\n", pItem.item);
                break;
            }
        }

        if (resources == 0)
            System.out.printf("    Device has no resources\n");
    }

    public static void WDC_DIAG_PrintIndent(String str, long indent,
        long max_width, boolean br_at_end)
    {
        if (!str.isEmpty() && indent <= 80)
        {
            int i, j;

            for (i = 0; i < str.length(); i++)
            {
                if (str.charAt(i) == '\n')
                {
                    System.out.printf("\n");
                    for (j = 0; j < indent; j++)
                        System.out.printf(" ");
                }
                else
                {
                    System.out.printf("%c", str.charAt(i));
                }
            }
            for (; i < max_width; i++)
                System.out.printf(" ");

            if (br_at_end)
                System.out.printf("\n");
        }
    }

    /* Print run-time registers and PCI configuration registers information */
    public static void WDC_DIAG_RegsInfoPrint(long hDev, final WDC_REG pRegs[],
        int options, boolean isExpress)
    {
        boolean fName, fDesc, fAddrSpace, fOffset, fSize, fDir;
        long extended = isExpress ? PCIE_REGS_NUM : 0;
        long pciExpressOffset = 0;
        int i, headerType;
        WDCResult result;

        if (hDev == 0)
        {
            System.err.printf("WDC_DIAG_ReadRegsAll: Error - NULL WDC device " +
                "handle\n");
            return;
        }

        if (isExpress)
        {
            result = wdapi.WDC_PciGetExpressOffset(hDev);
            if (result.dwStatus != 0)
            {
                System.err.printf("WDC_DIAG_ReadRegsAll: Error getting PCI" +
                    " Express  Offset\n");
                return;
            }
            pciExpressOffset = ((WDCResultLong)result).result;
        }

        result = wdapi.WDC_PciGetHeaderType(hDev);
        if (result.dwStatus != 0)
        {
            System.err.printf("WDC_DIAG_RegsInfoPrint: Error - unable to " +
                "determine PCI header type");
            return;
        }
        headerType = (int)((WDCResultLong)result).result;

        if (pRegs == null)
        {
            System.err.printf("WDC_DIAG_RegsInfoPrint: Error - NULL registers "
                + "information pointer\n");
            return;
        }

        if (pRegs.length == 0)
        {
            System.out.printf("There are currently no pre-defined registers to"
                + " display\n");
            return;
        }

        if (options == 0)
            options = WDC_DIAG_REG_PRINT_DEFAULT;

        fName = (options & WDC_DIAG_REG_PRINT_NAME) != 0;
        fDesc = (options & WDC_DIAG_REG_PRINT_DESC) != 0;
        fAddrSpace = (options & WDC_DIAG_REG_PRINT_ADDR_SPACE) != 0;
        fOffset = (options & WDC_DIAG_REG_PRINT_OFFSET) != 0;
        fSize = (options & WDC_DIAG_REG_PRINT_SIZE) != 0;
        fDir = (options & WDC_DIAG_REG_PRINT_DIRECTION) != 0;

        System.out.printf("\n");
        System.out.printf("PCI %sRegisters\n", isExpress ? "Express " : "");
        System.out.printf("----%s---------\n", isExpress ? "--------" : "");
        System.out.printf("%3s %-22s %-4s %-10s %-5s %-4s %s\n", "",
            fName ? "Name" : "", fAddrSpace ? "BAR" : "",
            fOffset ? "Offset" : "",
            fSize ? "Size" : "", fDir ? "R/W" : "", fDesc ? "Description" : "");

        System.out.printf("%3s %-22s %-4s %-10s %-5s %-4s %s\n",
            "", fName ? "----" : "", fAddrSpace ? "---" : "",
            fOffset ? "------" : "",
            fSize ? "----" : "", fDir ? "---" : "", fDesc ? "-----------" : "");

        for (i = 0; i < pRegs.length; i++)
        {
            System.out.printf("%2d. ", i + extended + 1);

            if (fName)
                System.out.printf("%-22.22s ", pRegs[i].sName);
            else
                System.out.printf("%22s ", "");

            if (fAddrSpace && (WDC_AD_CFG_SPACE != pRegs[i].dwAddrSpace))
                System.out.printf("%2d %2s", pRegs[i].dwAddrSpace, "");
            else
                System.out.printf("%4s ", "");

            if (fOffset)
                System.out.printf("0x%-8X ",pRegs[i].dwOffset +
                    pciExpressOffset);
            else
                System.out.printf("%10s ", "");

            if (fSize)
                System.out.printf("%2d %3s", pRegs[i].dwSize, "");
            else
                System.out.printf("%5s ", "");

            if (fDir)
            {
                System.out.printf("%-4s ",
                    (wdapi.WDC_READ_WRITE == pRegs[i].direction) ? "RW" :
                    (wdapi.WDC_READ == pRegs[i].direction) ? "R" : "W");
            }
            else
            {
                System.out.printf("%4s ", "");
            }
            if (fDesc && !pRegs[i].sDesc.isEmpty())
            {
                WDC_DIAG_PrintIndent(pRegs[i].sDesc,
                    REGS_INFO_PRINT_DETAILS_INDENT,
                    REGS_INFO_PRINT_DETAILS_WIDTH, true);
            }
            else
            {
                System.out.printf("\n");
            }
        }
    }

    /* Set address access mode */
    public static WDCResultInteger WDC_DIAG_SetMode()
    {
        int option = 0, pMode;

        System.out.printf("\n");
        System.out.printf("Select read/write mode:\n");
        System.out.printf("-----------------------\n");
        System.out.printf("1. 8 bits (%d bytes)\n", wdapi.WDC_SIZE_8);
        System.out.printf("2. 16 bits (%d bytes)\n", wdapi.WDC_SIZE_16);
        System.out.printf("3. 32 bits (%d bytes)\n", wdapi.WDC_SIZE_32);
        System.out.printf("4. 64 bits (%d bytes)\n", wdapi.WDC_SIZE_64);
        System.out.printf("\n");

        WDCResultInteger result = DiagLib.DIAG_GetMenuOption(4);
        if (DiagLib.DIAG_INPUT_FAIL == result.dwStatus)
        {
            return new WDCResultInteger(wdapi.WD_INVALID_PARAMETER, -1);
        }
        option = (int)result.result;

        switch (option)
        {
        case 1:
            pMode = wdapi.WDC_MODE_8;
            break;
        case 2:
            pMode = wdapi.WDC_MODE_16;
            break;
        case 3:
            pMode = wdapi.WDC_MODE_32;
            break;
        case 4:
            pMode = wdapi.WDC_MODE_64;
            break;
        default:
            return new WDCResultInteger(wdapi.WD_INVALID_PARAMETER, -1);
        }

        return new WDCResultInteger(wdapi.WD_STATUS_SUCCESS, pMode);
    }

    /* Get data for address write operation from user */
    /* Data size (dwSize) should be WDC_SIZE_8, WDC_SIZE_16, WDC_SIZE_32 or
     * WDC_SIZE_64 */
    public static WDCResultLong WDC_DIAG_InputWriteData(int dwSize)
    {
        long dwMaxVal, dwMinVal;
        String sMaxVal;
        WDCResultLong result;

        dwMaxVal = (dwSize == wdapi.WDC_SIZE_8) ? Byte.MAX_VALUE :
            (dwSize == wdapi.WDC_SIZE_16 ) ? Short.MAX_VALUE :
            (dwSize == wdapi.WDC_SIZE_32) ? Integer.MAX_VALUE :
            (dwSize == wdapi.WDC_SIZE_64) ? Long.MAX_VALUE : -1;

        dwMinVal = (dwSize == wdapi.WDC_SIZE_8) ? Byte.MIN_VALUE :
            (dwSize == wdapi.WDC_SIZE_16 ) ? Short.MIN_VALUE :
            (dwSize == wdapi.WDC_SIZE_32) ? Integer.MIN_VALUE :
            (dwSize == wdapi.WDC_SIZE_64) ? Long.MIN_VALUE : -1;

        sMaxVal = (dwSize == wdapi.WDC_SIZE_8) ? "ff" :
            (dwSize == wdapi.WDC_SIZE_16 ) ? "ffff" :
            (dwSize == wdapi.WDC_SIZE_32) ? "ffffffff" :
            (dwSize == wdapi.WDC_SIZE_64) ? "ffffffffffffffff" : "ERROR";



        if (dwMaxVal == -1)
        {
            System.err.printf("WDC_DIAG_InputWriteData: Error - Invalid size " +
                "(%d bytes)\n", dwSize);
            return new WDCResultLong(wdapi.WD_INVALID_PARAMETER, -1);
        }

        result = DiagLib.DIAG_InputNum("Enter data to write (max value: 0x" +
             sMaxVal + ")", true, dwMinVal, dwMaxVal);

        if (result.dwStatus != DiagLib.DIAG_INPUT_SUCCESS)
        {
            System.out.printf("Invalid input\n");
            return new WDCResultLong(wdapi.WD_INVALID_PARAMETER, -1);
        }

        return new WDCResultLong(wdapi.WD_STATUS_SUCCESS, result.result);
    }

    /* Read/write a memory or I/O address */
    public static void WDC_DIAG_ReadWriteAddr(long hDev, int direction,
        long dwAddrSpace, int mode)
    {
        long dwStatus, dwOffset;
        byte bData = 0;
        short wData = 0;
        int u32Data = 0;
        long u64Data = 0;
        WDCResult result;

        if (hDev == 0)
        {
            System.err.printf("WDC_DIAG_ReadWriteAddr: Error- NULL WDC device" +
                " handle\n");
            return;
        }

        result = DiagLib.DIAG_InputNum((wdapi.WDC_READ == direction) ?
            "Enter offset to read from" : "Enter offset to write to", true, 0,
            0);
        dwStatus = result.dwStatus;
        if (DiagLib.DIAG_INPUT_SUCCESS != dwStatus)
        {
            System.err.printf("WDC_DIAG_ReadWriteAddr: %s getting the offset\n",
                dwStatus == DiagLib.DIAG_INPUT_CANCEL ? "Canceled" : "Failed");
            return;
        }
        dwOffset = ((WDCResultLong)result).result;

        if (wdapi.WDC_WRITE == direction)
        {
            result = WDC_DIAG_InputWriteData(mode);
            if (result.dwStatus != wdapi.WD_STATUS_SUCCESS)
                return;
            bData = (byte)((WDCResultLong)result).result;
            wData = (short)((WDCResultLong)result).result;
            u32Data = (int)((WDCResultLong)result).result;
            u64Data = (long)((WDCResultLong)result).result;
        }

        switch (mode)
        {
        case wdapi.WDC_MODE_8:
            if (wdapi.WDC_READ == direction)
            {
                result = wdapi.WDC_ReadAddr8(hDev, dwAddrSpace, dwOffset);
                bData = ((WDCResultByte)result).result;
            }
            dwStatus = (wdapi.WDC_READ == direction) ?
                result.dwStatus :
                wdapi.WDC_WriteAddr8(hDev, dwAddrSpace, dwOffset, bData);
            if (wdapi.WD_STATUS_SUCCESS == dwStatus)
            {
                System.out.printf("%s 0x%X %s offset 0x%x in BAR %d\n",
                    (wdapi.WDC_READ == direction) ? "Read" : "Wrote", bData,
                    (wdapi.WDC_READ == direction) ? "from" : "to", dwOffset,
                    dwAddrSpace);
            }
            break;

        case wdapi.WDC_MODE_16:
            if (wdapi.WDC_READ == direction)
            {
                result = wdapi.WDC_ReadAddr16(hDev, dwAddrSpace, dwOffset);
                wData = ((WDCResultShort)result).result;
            }
            dwStatus = (wdapi.WDC_READ == direction) ?
                result.dwStatus :
                wdapi.WDC_WriteAddr16(hDev, dwAddrSpace, dwOffset, wData);
            if (wdapi.WD_STATUS_SUCCESS == dwStatus)
            {
                System.out.printf("%s 0x%hX %s offset 0x%x in BAR %d\n",
                    (wdapi.WDC_READ == direction) ? "Read" : "Wrote", wData,
                    (wdapi.WDC_READ == direction) ? "from" : "to", dwOffset,
                    dwAddrSpace);
            }
            break;

        case wdapi.WDC_MODE_32:
            if (wdapi.WDC_READ == direction)
            {
                result = wdapi.WDC_ReadAddr32(hDev, dwAddrSpace, dwOffset);
                u32Data = ((WDCResultInteger)result).result;
            }
            dwStatus = (wdapi.WDC_READ == direction) ?
                result.dwStatus :
                wdapi.WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
            if (wdapi.WD_STATUS_SUCCESS == dwStatus)
            {
                System.out.printf("%s 0x%X %s offset 0x%x in BAR %d\n",
                    (wdapi.WDC_READ == direction) ? "Read" : "Wrote", u32Data,
                    (wdapi.WDC_READ == direction) ? "from" : "to", dwOffset,
                    dwAddrSpace);
            }
            break;

        case wdapi.WDC_MODE_64:
            if (wdapi.WDC_READ == direction)
            {
                result = wdapi.WDC_ReadAddr64(hDev, dwAddrSpace, dwOffset);
                u64Data = ((WDCResultLong)result).result;
            }
            dwStatus = (wdapi.WDC_READ == direction) ?
                result.dwStatus :
                wdapi.WDC_WriteAddr64(hDev, dwAddrSpace, dwOffset, u64Data);
            if (wdapi.WD_STATUS_SUCCESS == dwStatus)
            {
                System.out.printf("%s 0x%x %s offset 0x%x in BAR %d\n",
                    (wdapi.WDC_READ == direction) ? "Read" : "Wrote", u64Data,
                    (wdapi.WDC_READ == direction) ? "from" : "to", dwOffset,
                    dwAddrSpace);
            }
            break;

        default:
            System.err.printf("WDC_DIAG_ReadWriteAddr: Error - Invalid mode " +
                "(%d)\n",mode);
            return;
        }

        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            System.out.printf("Failed to %s offset 0x%x in BAR %d. Error "
                + "0x%x - %s\n", (wdapi.WDC_READ == direction) ? "read from" :
                "write to", dwOffset, dwAddrSpace, dwStatus,
                wdapi.Stat2Str(dwStatus));
        }
    }

    /* Read/write a memory or I/O address OR an offset in the PCI configuration
     * space (dwAddrSpace == WDC_AD_CFG_SPACE) */
    public static void WDC_DIAG_ReadWriteBlock(long hDev, int direction,
        long dwAddrSpace)
    {
        long dwStatus;
        long dwOffset, dwBytes;
        String sDir = (wdapi.WDC_READ == direction) ? "read" : "write";
        byte[] pBuf;
        WDCResult result;

        if (hDev == 0)
        {
            System.err.printf("WDC_DIAG_ReadWriteBlock: Error- NULL WDC device "
                + "handle\n");
            return;
        }

        result = DiagLib.DIAG_InputNum("offset", true, 0, 0);
        dwStatus = result.dwStatus;
        if (DiagLib.DIAG_INPUT_SUCCESS != dwStatus)
        {
            System.err.printf("WDC_DIAG_ReadWriteBlock: %s getting the offset\n",
                dwStatus == DiagLib.DIAG_INPUT_CANCEL ? "Canceled" : "Failed");
            return;
        }
        dwOffset = ((WDCResultLong)result).result;

        result = DiagLib.DIAG_InputNum("bytes", true, 0, 0);
        dwStatus = result.dwStatus;
        if (DiagLib.DIAG_INPUT_SUCCESS != dwStatus)
        {
            System.err.printf("WDC_DIAG_ReadWriteBlock: %s getting the number " +
                "of bytes to transfer\n", dwStatus == DiagLib.DIAG_INPUT_CANCEL ?
                "Canceled" : "Failed");
            return;
        }
        dwBytes = ((WDCResultLong)result).result;

        if (dwBytes == 0)
            return;


        if (wdapi.WDC_WRITE == direction)
        {
            System.out.printf("data to write (hex format): 0x");
            pBuf = DiagLib.DIAG_GetHexBuffer((int)dwBytes);
        }
        else
        {
            pBuf = new byte[(int)dwBytes];
        }

        if (wdapi.WDC_AD_CFG_SPACE == dwAddrSpace) /* Read/write a configuration/attribute
                                              * space address */
        {
            long busType = wdapi.WDC_GetBusType(hDev);

            if (wdapi.WD_BUS_PCI == busType) /* Read/write PCI configuration space
                                        * offset */
            {
                if (direction == wdapi.WDC_READ)
                    dwStatus = wdapi.WDC_PciReadCfg(hDev, dwOffset, pBuf, dwBytes);
                else
                    dwStatus = wdapi.WDC_PciWriteCfg(hDev, dwOffset, pBuf, dwBytes);
            }
            else
            {
                System.out.printf("Error - Cannot read/write configuration " +
                    "space address space for bus type [0x%x]\n", busType);
                return; //goto Exit;
            }
        }
        else /* Read/write a memory or I/O address */
        {
            int mode, options;
            boolean fAutoInc;

            result = WDC_DIAG_SetMode();
            if (result.dwStatus != wdapi.WD_STATUS_SUCCESS)
                return; // goto Exit;
            mode = ((WDCResultInteger)result).result;

            result = DiagLib.DIAG_InputNum("Do you wish to increment the " +
                    "address after each " + sDir + " block (0x" + mode +
                    " bytes) (0 - No, 1 - Yes)? ", false, 0, 1);

            if (result.dwStatus != DiagLib.DIAG_INPUT_SUCCESS)
            {
                System.out.printf("Invalid input\n");
                return; // goto Exit;
            }
            fAutoInc = (boolean)(((WDCResultLong)result).result == 1);

            options = (fAutoInc ? wdapi.WDC_ADDR_RW_DEFAULT :
                wdapi.WDC_ADDR_RW_NO_AUTOINC);

            dwStatus = direction == wdapi.WDC_READ ?
                wdapi.WDC_ReadAddrBlock(hDev, dwAddrSpace, dwOffset, dwBytes,
                    pBuf, mode, options) :
                wdapi.WDC_WriteAddrBlock(hDev, dwAddrSpace, dwOffset, dwBytes,
                    pBuf, mode, options);
        }

        if (wdapi.WD_STATUS_SUCCESS == dwStatus)
        {
            if (wdapi.WDC_READ == direction)
                DiagLib.DIAG_PrintHexBuffer(pBuf);
            else
                System.out.printf("Wrote 0x%x bytes to offset 0x%x\n", dwBytes,
                    dwOffset);
        }
        else
        {
            System.out.printf("Failed to %s 0x%x bytes %s offset 0x%x. Error " +
                "0x%x - %s\n", sDir, dwBytes, (wdapi.WDC_READ == direction) ?
                "from" : "to", dwOffset, dwStatus, wdapi.Stat2Str(dwStatus));
        }

//    Exit:
        System.out.printf("\n");
        System.out.printf("Press ENTER to continue\n");
        try
        {
            System.in.read();
        }
        catch(Exception e){}
    }

    /* Read all pre-defined run-time or PCI configuration registers and display
     * results */
    public static void WDC_DIAG_ReadRegsAll(long hDev, final WDC_REG pRegs[],
        boolean fPciCfg, boolean fExpress)
    {
        long dwStatus;
        byte bData = 0;
        short wData = 0;
        int u32Data = 0;
        long u64Data = 0;
        long pciExpressOffset = 0;
        int i;
        String details;
        WDCResult result;

        if (hDev == 0)
        {
            System.err.printf("WDC_DIAG_ReadRegsAll: Error - NULL WDC device handle\n");
            return;
        }

        if (fExpress)
        {
            result = wdapi.WDC_PciGetExpressOffset(hDev);
            if (result.dwStatus != 0)
            {
                System.err.printf("WDC_DIAG_ReadRegsAll: Error getting PCI Expres" +
                    " Offset\n");
                return;
            }
            pciExpressOffset = ((WDCResultLong)result).result;
        }

//        result = wdapi.WDC_PciGetHeaderType(hDev);
//        if (result.dwStatus != 0)
//        {
//            System.err.printf("WDC_DIAG_RegsInfoPrint: Error - unable to determine" +
//                "PCI header type");
//            return;
//        }
//        headerType = ((wdapi.WDCResultInteger)result).result;

        if (pRegs == null || pRegs.length == 0)
        {
            System.err.printf("WDC_DIAG_ReadRegsAll: %s\n", pRegs.length == 0 ?
                "No registers (dwNumRegs == 0)" : "Error - NULL registers pointer");

            return;
        }

        System.out.printf("\n");
        System.out.printf("%s registers data:\n", (fPciCfg && fExpress) ?
            "PCI Express configuration" : fPciCfg ? "PCI configuration" :
            "run-time");
        System.out.printf("---------------------------------\n\n");
        System.out.printf("%3s %-22s %-10s  %s           %s\n", "",
            "Name", "Data", "Description", "Details");
        System.out.printf("%3s %-22s %-10s  %s           %s\n", "",
            "----", "----", "-----------", "-------");

        for (i = 0; i < pRegs.length; i++)
        {
            System.out.printf("%2d. %-22.22s ", i + 1, pRegs[i].sName);

            if (wdapi.WDC_WRITE == pRegs[i].direction)
            {
                System.out.printf("Write-only register\n");
                continue;
            }

            switch (pRegs[i].dwSize)
            {
            case wdapi.WDC_SIZE_8:
                result = fPciCfg ?
                    wdapi.WDC_PciReadCfg8(hDev, pRegs[i].dwOffset +
                        pciExpressOffset) :
                    wdapi.WDC_ReadAddr8(hDev, pRegs[i].dwAddrSpace,
                        pRegs[i].dwOffset + pciExpressOffset);
                dwStatus = result.dwStatus;
                bData = ((WDCResultByte)result).result;
                if (wdapi.WD_STATUS_SUCCESS == dwStatus)
                    System.out.printf("0x%-8x  ", bData);
                break;

            case wdapi.WDC_SIZE_16:
                result = fPciCfg ?
                    wdapi.WDC_PciReadCfg16(hDev, pRegs[i].dwOffset +
                        pciExpressOffset) :
                    wdapi.WDC_ReadAddr16(hDev, pRegs[i].dwAddrSpace,
                        pRegs[i].dwOffset + pciExpressOffset);
                dwStatus = result.dwStatus;
                wData = ((WDCResultShort)result).result;
                if (wdapi.WD_STATUS_SUCCESS == dwStatus)
                    System.out.printf("0x%-8x  ", wData);
                break;

            case wdapi.WDC_SIZE_32:
                result = fPciCfg ?
                    wdapi.WDC_PciReadCfg32(hDev, pRegs[i].dwOffset +
                        pciExpressOffset) :
                    wdapi.WDC_ReadAddr32(hDev, pRegs[i].dwAddrSpace,
                        pRegs[i].dwOffset + pciExpressOffset);
                dwStatus = result.dwStatus;
                u32Data = ((WDCResultInteger)result).result;
                if (wdapi.WD_STATUS_SUCCESS == dwStatus)
                    System.out.printf("0x%-8x  ", u32Data);
                break;

            case wdapi.WDC_SIZE_64:
                result = fPciCfg ?
                    wdapi.WDC_PciReadCfg64(hDev, pRegs[i].dwOffset +
                        pciExpressOffset) :
                    wdapi.WDC_ReadAddr64(hDev, pRegs[i].dwAddrSpace,
                        pRegs[i].dwOffset + pciExpressOffset);
                dwStatus = result.dwStatus;
                u64Data = ((WDCResultLong)result).result;
                if (wdapi.WD_STATUS_SUCCESS == dwStatus)
                    System.out.printf("0x%-8x  ", u64Data);
                break;

            default:
                System.out.printf("Invalid register size (%d)\n",
                    pRegs[i].dwSize);
                return;
            }

            if (wdapi.WD_STATUS_SUCCESS != dwStatus)
                System.out.printf("Error: 0x%-9x  ", dwStatus);
            WDC_DIAG_PrintIndent(pRegs[i].sDesc, READ_REGS_ALL_DESC_INDENT,
                READ_REGS_ALL_DESC_WIDTH, false);
            if (fPciCfg)
            {
                details = fExpress ?
                    wdapi.PciExpressConfRegData2Str(hDev, pRegs[i].dwOffset) :
                    wdapi.PciConfRegData2Str(hDev, pRegs[i].dwOffset);
                if (details.isEmpty())
                {
                    System.out.printf("\n");
                }
                else
                {
                    WDC_DIAG_PrintIndent(details, READ_REGS_ALL_DETAILS_INDENT,
                        READ_REGS_ALL_DETAILS_WIDTH, true);
                }
            }
            else
            {
                System.out.printf("\n");
            }
        }

        System.out.printf("Press ENTER to continue\n");
        try
        {
            System.in.read();
        }
        catch (Exception e)
        {
        }
    }

    /* Display a list of pre-defined run-time or PCI configuration registers
       and let user select to read/write from/to a specific register */
    public static void WDC_DIAG_ReadWriteReg(long hDev, final WDC_REG pRegs[],
        int direction, boolean fPciCfg, boolean fExpressReg)
    {
        long dwStatus;
        int dwReg, headerType;
        final WDC_REG pReg;
        byte bData = 0;
        short wData = 0;
        int u32Data = 0;
        long u64Data = 0;
        boolean fExpress = wdapi.WDC_PciGetExpressGen(hDev) != 0 && fExpressReg;
        long pciExpressOffset = 0;
        String details;
        WDCResult result;

        if (hDev == 0)
        {
            System.err.printf("WDC_DIAG_ReadRegsAll: Error - NULL WDC device " +
                "handle\n");
            return;
        }

        if (fExpress)
        {
            result = wdapi.WDC_PciGetExpressOffset(hDev);
            if (result.dwStatus != 0)
            {
                System.err.printf("WDC_DIAG_ReadRegsAll: Error getting PCI" +
                    " Express Offset\n");
                return;
            }
            pciExpressOffset = ((WDCResultLong)result).result;
        }

        result = wdapi.WDC_PciGetHeaderType(hDev);
        if (result.dwStatus != 0)
        {
            System.err.printf("WDC_DIAG_RegsInfoPrint: Error - unable to" +
                " determine PCI header type");
            return;
        }
        headerType = (int)((WDCResultLong)result).result;

        if (pRegs == null || pRegs.length == 0 )
        {
            System.err.printf("WDC_DIAG_ReadWriteReg: %s\n",
                pRegs.length == 0 ? "No registers to read/write (dwNumRegs ==" +
                " 0)" : "Error - NULL registers pointer");
            return;
        }

        /* Display pre-defined registers' information */
        System.out.printf("\n");
        System.out.printf("PCI %s registers:\n", fPciCfg ? "configuration" :
            "run-time");
        System.out.printf("----------------------------\n");
        WDC_DIAG_RegsInfoPrint(hDev, pRegs, WDC_DIAG_REG_PRINT_ALL,
            false);

        /* Read/write register */
        System.out.printf("\n");
        result = DiagLib.DIAG_InputNum(("Select a register from the list above"
                + " to " + ((wdapi.WDC_READ == direction) ? "read from" :
                "write to") + " or 0 to cancel: "), false, 0, pRegs.length);

        if (DiagLib.DIAG_INPUT_SUCCESS != result.dwStatus)
            return;

        dwReg = (int)((WDCResultLong)result).result;

        if (dwReg == 0)
            return;

        if (dwReg > pRegs.length)
        {
            System.out.printf("Selection (%d) is out of range (1 - %d)\n",
                dwReg, pRegs.length);
            return;
        }

        pReg = pRegs[dwReg - 1];

        if ( ((wdapi.WDC_READ == direction) && (wdapi.WDC_WRITE == pReg.direction)) ||
            ((wdapi.WDC_WRITE == direction) && (wdapi.WDC_READ == pReg.direction)))
        {
            System.out.printf("Error - you have selected to %s a %s-only register\n",
                (wdapi.WDC_READ == direction) ? "read from" : "write to",
                (wdapi.WDC_WRITE == pReg.direction) ? "write" : "read");
            return;
        }

        if (wdapi.WDC_WRITE == direction)
        {
            result = WDC_DIAG_InputWriteData(pReg.dwSize);
            if (result.dwStatus != DiagLib.DIAG_INPUT_SUCCESS)
                return;
        }

        switch ((int)pReg.dwSize)
        {
        case wdapi.WDC_SIZE_8:
            if (wdapi.WDC_READ == direction)
            {
                result = fPciCfg ?
                    wdapi.WDC_PciReadCfg8(hDev, pReg.dwOffset +
                        pciExpressOffset) :
                    wdapi.WDC_ReadAddr8(hDev, pReg.dwAddrSpace, pReg.dwOffset +
                        pciExpressOffset);
                dwStatus = result.dwStatus;
                bData = ((WDCResultByte)result).result;
            }
            else
            {
                dwStatus = fPciCfg ? wdapi.WDC_PciWriteCfg8(hDev,
                    pReg.dwOffset + pciExpressOffset, bData) :
                    wdapi.WDC_WriteAddr8(hDev, pReg.dwAddrSpace, pReg.dwOffset
                        + pciExpressOffset, bData);
            }
            if (wdapi.WD_STATUS_SUCCESS == dwStatus)
            {
                System.out.printf("%s 0x%X %s register %s at offset [0x%x]\n",
                    (wdapi.WDC_READ == direction) ? "Read" : "Wrote", bData,
                    (wdapi.WDC_READ == direction) ? "from" : "to", pReg.sName,
                    fExpress ? pReg.dwOffset + pciExpressOffset : pReg.dwOffset);
                details = fExpress ?
                    wdapi.PciExpressConfRegData2Str(hDev, pReg.dwOffset) :
                    wdapi.PciConfRegData2Str(hDev, pReg.dwOffset);
                System.out.printf ("%s\n", wdapi.WDC_READ == direction &&
                    !details.isEmpty() ? "Decoded register data: " : "");
                System.out.printf ("%s\n", wdapi.WDC_READ == direction &&
                    !details.isEmpty() ? details : "");
            }
            break;

        case wdapi.WDC_SIZE_16:
            if (wdapi.WDC_READ == direction)
            {
                result = fPciCfg ?
                    wdapi.WDC_PciReadCfg16(hDev, pReg.dwOffset +
                        pciExpressOffset) :
                    wdapi.WDC_ReadAddr16(hDev, pReg.dwAddrSpace,
                        pReg.dwOffset + pciExpressOffset);
                dwStatus = result.dwStatus;
                wData = ((WDCResultShort)result).result;
            }
            else
            {
                dwStatus = fPciCfg ?
                    wdapi.WDC_PciWriteCfg16(hDev, pReg.dwOffset +
                        pciExpressOffset, wData) :
                    wdapi.WDC_WriteAddr16(hDev, pReg.dwAddrSpace, pReg.dwOffset
                        + pciExpressOffset, wData);
            }
            if (wdapi.WD_STATUS_SUCCESS == dwStatus)
            {
                System.out.printf("%s [0x%hX] %s register %s at offset [0x%x]\n",
                    (wdapi.WDC_READ == direction) ? "Read" : "Wrote", wData,
                    (wdapi.WDC_READ == direction) ? "from" : "to", pReg.sName,
                    fExpress ? pReg.dwOffset + pciExpressOffset : pReg.dwOffset);
                details = fExpress ?
                    wdapi.PciExpressConfRegData2Str(hDev, pReg.dwOffset) :
                    wdapi.PciConfRegData2Str(hDev, pReg.dwOffset);
                System.out.printf ("%s\n", wdapi.WDC_READ == direction &&
                    !details.isEmpty() ? "Decoded register data: " : "");
                System.out.printf ("%s\n", wdapi.WDC_READ == direction &&
                    !details.isEmpty() ? details : "");
            }
            break;

        case wdapi.WDC_SIZE_32:
            if (wdapi.WDC_READ == direction)
            {
                result = fPciCfg ?
                    wdapi.WDC_PciReadCfg32(hDev, pReg.dwOffset +
                        pciExpressOffset) :
                    wdapi.WDC_ReadAddr32(hDev, pReg.dwAddrSpace, pReg.dwOffset +
                        pciExpressOffset);
                dwStatus = result.dwStatus;
                u32Data = (int)((WDCResultInteger)result).result;
            }
            else
            {
                dwStatus = fPciCfg ?
                    wdapi.WDC_PciWriteCfg32(hDev, pReg.dwOffset +
                        pciExpressOffset, u32Data) :
                    wdapi.WDC_WriteAddr32(hDev, pReg.dwAddrSpace,
                        pReg.dwOffset + pciExpressOffset, u32Data);
            }
            if (wdapi.WD_STATUS_SUCCESS == dwStatus)
            {
                System.out.printf("%s [0x%X] %s register %s at offset [0x%x]\n",
                    (wdapi.WDC_READ == direction) ? "Read" : "Wrote", u32Data,
                    (wdapi.WDC_READ == direction) ? "from" : "to", pReg.sName,
                    fExpress ? pReg.dwOffset + pciExpressOffset : pReg.dwOffset);

                details = fExpress ?
                    wdapi.PciExpressConfRegData2Str(hDev, pReg.dwOffset) :
                    wdapi.PciConfRegData2Str(hDev, pReg.dwOffset);
                System.out.printf ("%s\n", wdapi.WDC_READ == direction &&
                    !details.isEmpty() ? "Decoded register data: " : "");
                System.out.printf ("%s\n", wdapi.WDC_READ == direction &&
                    !details.isEmpty() ? details : "");
            }
            break;

        case wdapi.WDC_SIZE_64:
            if (wdapi.WDC_READ == direction)
            {
                result = fPciCfg ?
                    wdapi.WDC_PciReadCfg64(hDev, pReg.dwOffset +
                        pciExpressOffset) :
                    wdapi.WDC_ReadAddr64(hDev, pReg.dwAddrSpace, pReg.dwOffset
                        + pciExpressOffset);
                dwStatus = result.dwStatus;
                u64Data = ((WDCResultLong)result).result;
            }
            else
            {
                dwStatus = fPciCfg ?
                    wdapi.WDC_PciWriteCfg64(hDev, pReg.dwOffset +
                        pciExpressOffset, u64Data) :
                    wdapi.WDC_WriteAddr64(hDev, pReg.dwAddrSpace,
                        pReg.dwOffset + pciExpressOffset, u64Data);
            }
            if (wdapi.WD_STATUS_SUCCESS == dwStatus)
            {
                System.out.printf("%s [0x%x] %s register %s at offset [0x%x]\n",
                    (wdapi.WDC_READ == direction) ?  "Read" : "Wrote", u64Data,
                    (wdapi.WDC_READ == direction) ? "from" : "to", pReg.sName,
                    fExpress ? pReg.dwOffset + pciExpressOffset :
                    pReg.dwOffset);
                details = fExpress ?
                    wdapi.PciExpressConfRegData2Str(hDev, pReg.dwOffset +
                        pciExpressOffset) :
                    wdapi.PciConfRegData2Str(hDev, pReg.dwOffset);
                System.out.printf ("%s\n", wdapi.WDC_READ == direction &&
                    !details.isEmpty() ? "Decoded register data: " : "");
                System.out.printf ("%s\n", wdapi.WDC_READ == direction &&
                    !details.isEmpty() ? details : "");
            }
            break;

        default:
            System.out.printf("Invalid register size (%d)\n", pReg.dwSize);
            return;
        }

        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            System.out.printf("Failed %s %s. Error [0x%x - %s]\n",
                (wdapi.WDC_READ == direction) ? "reading data from" :
                "writing data to", pReg.sName, dwStatus,
                wdapi.Stat2Str(dwStatus));
        }

        System.out.printf("\n");
        System.out.printf("Press ENTER to continue\n");
        try
        {
            System.in.read();
        }
        catch (Exception e)
        {
        }
    }

    /* Display available PCI/PCIe capabilities */
    public static void WDC_DIAG_ScanPCICapabilities(long hDev)
    {
        WDC_PCI_SCAN_CAPS_RESULT scanResult;
        long dwCapId = wdapi.WD_PCI_CAP_ID_ALL;
        int option = 0;
        int i;
        WDCResult result;

        if (hDev == 0)
        {
            System.err.printf("WDC_DIAG_ScanPCICapabilities: Error - NULL WDC "
                + "device handle\n");
            return;
        }

        System.out.printf("\n");
        System.out.printf("Select scan option (PCI/PCI-Express, all/specific):\n");
        System.out.printf("-----------------------\n");
        System.out.printf("1. Scan PCI capabilities\n");
        System.out.printf("2. Scan specific PCI capability\n");
        System.out.printf("3. Scan PCI Express extended capabilities\n");
        System.out.printf("4. Scan specific PCI Express extended capability\n");
        System.out.printf("\n");

        System.out.printf("Enter option or 0 to cancel: ");
        result = DiagLib.DIAG_GetMenuOption(4);

        if (result.dwStatus != DiagLib.DIAG_INPUT_SUCCESS)
            return;
        option = ((WDCResultInteger)result).result;

        if (option == 2 || option == 4)
        {
            result = DiagLib.DIAG_InputNum("Enter requested " + (option == 4 ?
                "extended " : "") + "capability ID (hexadecimal): 0x", true, 0,
                Long.MAX_VALUE);
            if (result.dwStatus != DiagLib.DIAG_INPUT_SUCCESS)
                return;
            dwCapId = ((WDCResultLong)result).result;
        }

        /* Scan PCI/PCIe Capabilities */
        if (option <= 2)
            result = wdapi.WDC_PciScanCaps(hDev, dwCapId);
        else
            result = wdapi.WDC_PciScanExtCaps(hDev, dwCapId);

        if (wdapi.WD_STATUS_SUCCESS != result.dwStatus)
        {
            System.err.printf("WDC_DIAG_ScanPCICapabilities: Failed scanning "
                + " PCI capabilities. Error [0x%x - %s]\n", result.dwStatus,
                wdapi.Stat2Str(result.dwStatus));
            return;
        }
        scanResult = ((WDCResultPciScanCaps)result).pPciScanCapsResult;
        System.out.printf("%sPCI %scapabilities found\n",
            (scanResult.dwNumCaps != 0) ?  "" : "No ",
            (option == 3 || option == 4) ? "Express extended " : "");
        for (i = 0; i < scanResult.dwNumCaps; i++)
        {
            System.out.printf("    %d) %s - ID [0x%x], offset [0x%x]\n", i + 1,
                (option == 3 || option == 4) ?
                PciRegs.GET_EXTENDED_CAPABILITY_STR(scanResult.pciCaps[i].dwCapId) :
                PciRegs.GET_CAPABILITY_STR(scanResult.pciCaps[i].dwCapId),
                scanResult.pciCaps[i].dwCapId,
                scanResult.pciCaps[i].dwCapOffset);
        }
    }

    public static String WDC_DIAG_IntTypeDescriptionGet(long dwIntType)
    {
        if ((dwIntType & wdapi.INTERRUPT_MESSAGE_X) != 0)
            return "Extended Message-Signaled Interrupt (MSI-X)";
        else if ((dwIntType & wdapi.INTERRUPT_MESSAGE) != 0)
            return "Message-Signaled Interrupt (MSI)";
        else if ((dwIntType & wdapi.INTERRUPT_LEVEL_SENSITIVE) != 0)
            return "Level-Sensitive Interrupt";
        return "Edge-Triggered Interrupt";
    }

    /* -----------------------------------------------
        PCI
       ----------------------------------------------- */
    /* Print PCI device location information */
    public static void WDC_DIAG_PciSlotPrint(WD_PCI_SLOT pPciSlot)
    {
        if (pPciSlot == null)
        {
            System.err.printf("WDC_DIAG_PciSlotPrint: Error - NULL PCI slot" +
                " pointer\n");
            return;
        }

        System.out.printf("Domain [0x%x], Bus [0x%x], Slot [0x%x]," +
            " Function [0x%x]\n", pPciSlot.dwDomain, pPciSlot.dwBus,
            pPciSlot.dwSlot, pPciSlot.dwFunction);
    }

    /* Print PCI device location and resources information to file */
    public static void WDC_DIAG_PciDeviceInfoPrint(WD_PCI_SLOT pPciSlot,
        boolean dump_cfg)
    {
        long dwStatus, dwExpressGen;
        WD_PCI_CARD_INFO deviceInfo = new WD_PCI_CARD_INFO(pPciSlot);

        if (pPciSlot == null)
        {
            System.err.printf("WDC_DIAG_PciDeviceInfoPrint: Error - NULL PCI" +
                " slot pointer\n");
            return;
        }

        System.out.printf("    Location: ");
        WDC_DIAG_PciSlotPrint(pPciSlot);

        if (dump_cfg)
        {
            int config;
            long dwOffset = 0;

            for (dwOffset = 0; dwOffset < 256; dwOffset += (Integer.SIZE / 8))
            {
                WDCResultInteger result =
                    wdapi.WDC_PciReadCfgBySlot32(pPciSlot, dwOffset);
                dwStatus = result.dwStatus;
                if (dwStatus != 0)
                {
                    System.err.printf("    Failed reading PCI configuration"
                        + " space.\n    Error [0x%x - %s]\n", dwStatus,
                        wdapi.Stat2Str(dwStatus));
                    return;
                }
                config = result.result;

                if ((dwOffset / 4) % 8 == 0)
                    System.out.printf("%02x ", dwOffset);
                System.out.printf("%08x ", config);

                if ((dwOffset / 4) % 8 == 7)
                    System.out.printf("\n");
            }
        }

        dwStatus = wdapi.WDC_PciGetDeviceInfo(deviceInfo);
        if ((wdapi.WD_NO_RESOURCES_ON_DEVICE != dwStatus) &&
            (wdapi.WD_STATUS_SUCCESS != dwStatus))
        {
            System.err.printf("    Failed retrieving PCI resources" +
                " information.\n    Error 0x%x - %s\n", dwStatus,
                wdapi.Stat2Str(dwStatus));
            return;
        }


        WDC_DIAG_DeviceResourcesPrint(deviceInfo.GetCard(), 0);
        dwExpressGen = wdapi.WDC_PciGetExpressGenBySlot(pPciSlot);
        if (dwExpressGen != 0)
            System.out.printf("    PCI Express Generation: Gen%d\n",
                dwExpressGen);
    }

    /* Print location and resources information for all connected PCI devices
     * to file */
    public static void WDC_DIAG_PciDevicesInfoPrintAll(boolean dump_cfg,
        boolean stdout)
    {
        long dwStatus, dwNumDevices;
        int i;
        WDCResultPciScan scanResult = wdapi.WDC_PciScanDevices(0, 0);
        dwStatus = scanResult.dwStatus;

        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            System.out.printf("Failed scanning PCI bus. Error [0x%x - %s]\n",
                dwStatus, wdapi.Stat2Str(dwStatus));
            return;
        }

        dwNumDevices = scanResult.pPciScanResult.dwNumDevices;
        if (dwNumDevices == 0)
        {
            System.out.printf("No devices were found on the PCI bus\n");
            return;
        }

        System.out.printf("\n");
        System.out.printf("Found %d devices on the PCI bus:\n", dwNumDevices);
        System.out.printf("---------------------------------\n");

        for (i = 0; i < dwNumDevices; i++)
        {
            System.out.printf("%2d. Vendor ID: [0x%X], Device ID: [0x%X]\n",
                i + 1, scanResult.pPciScanResult.deviceId[i].dwVendorId,
                scanResult.pPciScanResult.deviceId[i].dwDeviceId);
            WDC_DIAG_PciDeviceInfoPrint(scanResult.pPciScanResult.deviceSlot[i],
                dump_cfg);

            if (stdout)
            {
                System.out.printf("Press ENTER to proceed to next device");
                try
                {
                    System.in.read();
                }
                catch(Exception e)
                {
                }
            }
            System.out.printf("\n");
        }
    }

    public static long WDC_DIAG_DeviceFindAndOpen(long dwVendorId, long dwDeviceId,
            String pcKpName, WDCContext devCtx, WDC_DEV_ADDR_DESC devAddrDesc)
    {
         WD_PCI_SLOT slot;

         /* Find device */
         slot = WDC_DIAG_DeviceFind(dwVendorId, dwDeviceId);
         if (slot == null)
             return 0;

         /* Open a device handle */
         return WDC_DIAG_DeviceOpen(slot, pcKpName, devCtx, devAddrDesc);
    }

    /* Find a PCI device */
    public static WD_PCI_SLOT WDC_DIAG_DeviceFind(long dwVendorId,
            long dwDeviceId)
    {
        int i;
        long dwNumDevices;
        wdapi.WDC_PCI_SCAN_RESULT scanResult;

        if (dwVendorId == 0)
        {
            /* Get vendor ID */
            WDCResultLong result = DiagLib.DIAG_InputNum("Enter vendor ID",
                true, 0, 0);
            if (DiagLib.DIAG_INPUT_SUCCESS != result.dwStatus)
                return null;
            dwVendorId = result.result;

            /* Get device ID */
            result = DiagLib.DIAG_InputNum("Enter device ID", true, 0, 0);
            if (DiagLib.DIAG_INPUT_SUCCESS != result.dwStatus)
                return null;
            dwDeviceId = result.result;
        }

        /* Scan PCI devices */
        WDCResultPciScan scan = wdapi.WDC_PciScanDevices(dwVendorId,
            dwDeviceId);
        if (wdapi.WD_STATUS_SUCCESS != scan.dwStatus)
        {
            System.err.printf("DeviceFind: Failed scanning the PCI bus.\n" +
                "Error [0x%x - %s]\n", scan.dwStatus,
                wdapi.Stat2Str(scan.dwStatus));
            return null;
        }
        scanResult = scan.pPciScanResult;
        dwNumDevices = scanResult.dwNumDevices;
        if (0 == dwNumDevices)
        {
            System.out.printf("No matching PCI device was found for search "
                + "criteria (Vendor ID 0x%x, Device ID 0x%x)\n", dwVendorId,
                dwDeviceId);

            return null;
        }

        /* Display matching devices information */
        System.out.printf("\nFound %d matching device%s " +
            "[ Vendor ID 0x%x%s, Device ID 0x%x%s ]:\n",
            dwNumDevices, dwNumDevices > 1 ? "s" : "",
            dwVendorId, dwVendorId != 0 ? "" : " (ALL)",
            dwDeviceId, dwDeviceId != 0 ? "" : " (ALL)");

        for (i = 0; i < dwNumDevices; i++)
        {
            System.out.printf("\n");
            System.out.printf("%2d. Vendor ID: 0x%X, Device ID: 0x%X\n", i + 1,
                scanResult.deviceId[i].dwVendorId,
                scanResult.deviceId[i].dwDeviceId);

            WdcDiagLib.WDC_DIAG_PciDeviceInfoPrint(scanResult.deviceSlot[i],
                false);
        }
        System.out.printf("\n");

        /* Select device */
        if (dwNumDevices > 1)
        {
            WDCResultLong result = DiagLib.DIAG_InputNum(
                "Select a device (1 - " + dwNumDevices + "): ", false, 1,
                dwNumDevices);
            if (DiagLib.DIAG_INPUT_SUCCESS != result.dwStatus)
                return null;
            i = (int)result.result;
        }

        return scanResult.deviceSlot[i - 1];
    }

    /* Open a handle to a PCI device */
    static Long WDC_DIAG_DeviceOpen(WD_PCI_SLOT pSlot, String pcKpName,
            Object devCtx, WDC_DEV_ADDR_DESC devAddrDesc)
    {
        long hDev;
        long dwStatus;
        WD_PCI_CARD_INFO deviceInfo = new WD_PCI_CARD_INFO(pSlot);

        /* Retrieve the device's resources information */
        dwStatus = wdapi.WDC_PciGetDeviceInfo(deviceInfo);
        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            System.err.printf("DeviceOpen: Failed retrieving the device's " +
                "resources information. Error [0x%x - %s]\n", dwStatus,
                wdapi.Stat2Str(dwStatus));
            return 0L;
        }

        /* NOTE: If necessary, you can modify the device's resources information
           here - mainly the information stored in the deviceInfo.Card.Items array,
           and the number of array items stored in deviceInfo.Card.dwItems.
           For example:
           - Edit the deviceInfo.Card.Items array and/or deviceInfo.Card.dwItems,
             to register only some of the resources or to register only a portion
             of a specific address space.
           - Set the fNotSharable field of one or more items in the
             deviceInfo.Card.Items array to 1, to block sharing of the related
             resources and ensure that they are locked for exclusive use.
        */

        /* Open a handle to the device */
        hDev = WDC_DIAG_PCI_DeviceOpen(deviceInfo, pcKpName, devCtx, devAddrDesc);
        if (hDev == 0)
        {
            //System.err.printf("DeviceOpen: Failed opening a handle to the " +
                //"device: %s", PciLib.PCI_GetLastErr());
            return 0L;
        }

        return hDev;
    }

    public static long WDC_DIAG_PCI_DeviceOpen(WD_PCI_CARD_INFO pDeviceInfo,
            String pcKpName, Object devCtx, WDC_DEV_ADDR_DESC devAddrDesc)
    {
         long dwStatus;
         WDCResultLong result;
         WDC_DEVICE dev;
         Long hDev = null;

         /* Validate arguments */
         if (pDeviceInfo == null)
         {
             System.err.printf("PCI_DeviceOpen: Error - null device information " +
                 "struct pointer\n");
             return 0;
         }

         /* Open a device handle */
         result = wdapi.WDC_PciDeviceOpen(pDeviceInfo, null/* pDevCtx*/);
         dwStatus = result.dwStatus;
         hDev = result.result;

         if (hDev == 0)
             return 0;

         dev = new WDC_DEVICE(hDev);
         if (wdapi.WD_STATUS_SUCCESS != dwStatus)
         {
             System.err.printf("Failed opening a WDC device handle. Error 0x%x - %s\n",
                 dwStatus, wdapi.Stat2Str(dwStatus));
             WDC_DIAG_DeviceClose(dev.hDev);
             return 0;
         }
         System.out.println("Device successfully opened!");

         dev = new WDC_DEVICE(hDev);

     if (pcKpName != null)
     {
         devAddrDesc.setdwNumAddrSpaces(dev.dwNumAddrSpaces);
         /* Open a handle to a Kernel PlugIn driver */
         dwStatus = wdapi.WDC_KernelPlugInOpen(hDev, pcKpName, devAddrDesc.data);

         if (dwStatus == wdapi.WD_STATUS_SUCCESS)
             dev.updateDev(hDev);
     }

         /* Return handle to the new device */
         return hDev;
    }

    public static boolean WDC_DIAG_DeviceClose(long hDev)
    {
        long dwStatus;

        /* Close the device handle */
        dwStatus = wdapi.WDC_PciDeviceClose(hDev);
        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            System.err.printf("Failed closing a WDC device handle (0x%x)." +
                    "Error 0x%x - %s\n", hDev, dwStatus,
                    wdapi.Stat2Str(dwStatus));
        }

        return (wdapi.WD_STATUS_SUCCESS == dwStatus);
    }
}


