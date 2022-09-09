/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/*******************************************************************************
*  File: plx_diag_lib.c - Implementation of shared PLX diagnostics library for
*                         accessing PLX devices from the user-mode.
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*******************************************************************************/

#include "pci_menus_common.h"
#include "status_strings.h"
#include "diag_lib.h"
#include "wdc_diag_lib.h"
#include "plx_diag_lib.h"

/*************************************************************
  Global variables
 *************************************************************/
/* User's input command */
CHAR gsInput[256];

/* Configuration registers information array */
static const WDC_REG gPLX_CfgRegs[] = {
    { WDC_AD_CFG_SPACE, PCI_VID, WDC_SIZE_16, WDC_READ_WRITE, "VID", "Vendor ID" },
    { WDC_AD_CFG_SPACE, PCI_DID, WDC_SIZE_16, WDC_READ_WRITE, "DID", "Device ID" },
    { WDC_AD_CFG_SPACE, PCI_CR, WDC_SIZE_16, WDC_READ_WRITE, "CMD", "Command" },
    { WDC_AD_CFG_SPACE, PCI_SR, WDC_SIZE_16, WDC_READ_WRITE, "STS", "Status" },
    { WDC_AD_CFG_SPACE, PCI_REV, WDC_SIZE_32, WDC_READ_WRITE, "RID_CLCD", "Revision ID & Class Code" },
    { WDC_AD_CFG_SPACE, PCI_CCSC, WDC_SIZE_8, WDC_READ_WRITE, "SCC", "Sub Class Code" },
    { WDC_AD_CFG_SPACE, PCI_CCBC, WDC_SIZE_8, WDC_READ_WRITE, "BCC", "Base Class Code" },
    { WDC_AD_CFG_SPACE, PCI_CLSR, WDC_SIZE_8, WDC_READ_WRITE, "CALN", "Cache Line Size" },
    { WDC_AD_CFG_SPACE, PCI_LTR, WDC_SIZE_8, WDC_READ_WRITE, "LAT", "Latency Timer" },
    { WDC_AD_CFG_SPACE, PCI_HDR, WDC_SIZE_8, WDC_READ_WRITE, "HDR", "Header Type" },
    { WDC_AD_CFG_SPACE, PCI_BISTR, WDC_SIZE_8, WDC_READ_WRITE, "BIST", "Built-in Self Test" },
    { WDC_AD_CFG_SPACE, PCI_BAR0, WDC_SIZE_32, WDC_READ_WRITE, "BADDR0", "Base Address 0" },
    { WDC_AD_CFG_SPACE, PCI_BAR1, WDC_SIZE_32, WDC_READ_WRITE, "BADDR1", "Base Address 1" },
    { WDC_AD_CFG_SPACE, PCI_BAR2, WDC_SIZE_32, WDC_READ_WRITE, "BADDR2", "Base Address 2" },
    { WDC_AD_CFG_SPACE, PCI_BAR3, WDC_SIZE_32, WDC_READ_WRITE, "BADDR3", "Base Address 3" },
    { WDC_AD_CFG_SPACE, PCI_BAR4, WDC_SIZE_32, WDC_READ_WRITE, "BADDR4", "Base Address 4" },
    { WDC_AD_CFG_SPACE, PCI_BAR5, WDC_SIZE_32, WDC_READ_WRITE, "BADDR5", "Base Address 5" },
    { WDC_AD_CFG_SPACE, PCI_CIS, WDC_SIZE_32, WDC_READ_WRITE, "CIS", "CardBus CIS pointer" },
    { WDC_AD_CFG_SPACE, PCI_SVID, WDC_SIZE_16, WDC_READ_WRITE, "SVID", "Sub-system Vendor ID" },
    { WDC_AD_CFG_SPACE, PCI_SDID, WDC_SIZE_16, WDC_READ_WRITE, "SDID", "Sub-system Device ID" },
    { WDC_AD_CFG_SPACE, PCI_EROM, WDC_SIZE_32, WDC_READ_WRITE, "EROM", "Expansion ROM Base Address" },
    { WDC_AD_CFG_SPACE, PCI_CAP, WDC_SIZE_8, WDC_READ_WRITE, "NEW_CAP", "New Capabilities Pointer" },
    { WDC_AD_CFG_SPACE, PCI_ILR, WDC_SIZE_32, WDC_READ_WRITE, "INTLN", "Interrupt Line" },
    { WDC_AD_CFG_SPACE, PCI_IPR, WDC_SIZE_32, WDC_READ_WRITE, "INTPIN", "Interrupt Pin" },
    { WDC_AD_CFG_SPACE, PCI_MGR, WDC_SIZE_32, WDC_READ_WRITE, "MINGNT", "Minimum Required Burst Period" },
    { WDC_AD_CFG_SPACE, PCI_MLR, WDC_SIZE_32, WDC_READ_WRITE, "MAXLAT", "Maximum Latency" },
    /* PLX-specific configuration registers */
    { WDC_AD_CFG_SPACE, 0x40, WDC_SIZE_8, WDC_READ_WRITE, "PMCAPID", "Power Management Capability ID" },
    { WDC_AD_CFG_SPACE, 0x41, WDC_SIZE_8, WDC_READ_WRITE, "PMNEXT", "Power Management Next Capability Pointer" },
    { WDC_AD_CFG_SPACE, 0x42, WDC_SIZE_16, WDC_READ_WRITE, "PMCAP", "Power Management Capabilities" },
    { WDC_AD_CFG_SPACE, 0x44, WDC_SIZE_16, WDC_READ_WRITE, "PMCSR", "Power Management Control/Status" },
    { WDC_AD_CFG_SPACE, 0x48, WDC_SIZE_8, WDC_READ_WRITE, "HS_CAPID", "Hot Swap Capability ID" },
    { WDC_AD_CFG_SPACE, 0x49, WDC_SIZE_8, WDC_READ_WRITE, "HS_NEXT", "Hot Swap Next Capability Pointer" },
    { WDC_AD_CFG_SPACE, 0x4A, WDC_SIZE_8, WDC_READ_WRITE, "HS_CSR", "Hot Swap Control/Status" },
    { WDC_AD_CFG_SPACE, 0x4C, WDC_SIZE_8, WDC_READ_WRITE, "VPD_CAPID", "PCI Vital Product Data Control" },
    { WDC_AD_CFG_SPACE, 0x4D, WDC_SIZE_8, WDC_READ_WRITE, "VPD_NEXT", "PCI Vital Product Next Capability Pointer" },
    { WDC_AD_CFG_SPACE, 0x4E, WDC_SIZE_16, WDC_READ_WRITE, "VPD_ADDR", "PCI Vital Product Data Address" },
    { WDC_AD_CFG_SPACE, 0x50, WDC_SIZE_32, WDC_READ_WRITE, "VPD_DATA", "PCI VPD Data" },
};

/* PLX run-time registers information array */
static const WDC_REG gPLX_M_Regs[] = {
    { PLX_ADDR_REG, 0x00, WDC_SIZE_32, WDC_READ_WRITE, "LAS0RR", "Local Addr Space 0 Range for PCI-to-Local Bus" },
    { PLX_ADDR_REG, 0x04, WDC_SIZE_32, WDC_READ_WRITE, "LAS0BA", "Local BAR (Remap) for PCI-to-Local Addr Space 0" },
    { PLX_ADDR_REG, 0x08, WDC_SIZE_32, WDC_READ_WRITE, "MARBR", "Mode/DMA Arbitration" },
    { PLX_ADDR_REG, 0x0C, WDC_SIZE_8, WDC_READ_WRITE, "BIGEND", "Big/Little Endian Descriptor" },
    { PLX_ADDR_REG, 0x0D, WDC_SIZE_8, WDC_READ_WRITE, "LMISC", "Local Miscellananeous Control" },
    { PLX_ADDR_REG, 0x0E, WDC_SIZE_8, WDC_READ_WRITE, "PROT_AREA", "Serial EEPROM Write-Protected Addr Boundary" },
    { PLX_ADDR_REG, 0x10, WDC_SIZE_32, WDC_READ_WRITE, "EROMRR", "Expansion ROM Range" },
    { PLX_ADDR_REG, 0x14, WDC_SIZE_32, WDC_READ_WRITE, "EROMBA", "EROM Local BAR (Remap) & BREQ0 Control" },
    { PLX_ADDR_REG, 0x18, WDC_SIZE_32, WDC_READ_WRITE, "LBRD0", "Local Addr Space 0 Bus Region Descriptor" },
    { PLX_ADDR_REG, 0x1C, WDC_SIZE_32, WDC_READ_WRITE, "DMRR", "Local Range for PCI initiatior-to-PCI" },
    { PLX_ADDR_REG, 0x20, WDC_SIZE_32, WDC_READ_WRITE, "DMLBAM", "Local Bus Addr for PCI Initiatior-to-PCI Mem" },
    { PLX_ADDR_REG, 0x24, WDC_SIZE_32, WDC_READ_WRITE, "DMLBAI", "Local BAR for PCI Initiatior-to-PCI I/O" },
    { PLX_ADDR_REG, 0x28, WDC_SIZE_32, WDC_READ_WRITE, "DMPBAM", "PCI BAR (Remap) for Initiatior-to-PCI Mem" },
    { PLX_ADDR_REG, 0x2C, WDC_SIZE_32, WDC_READ_WRITE, "DMCFGA", "PCI Config Addr for PCI Initiatior-to-PCI I/O" },
    { PLX_ADDR_REG, 0x30, WDC_SIZE_32, WDC_READ_WRITE, "OPQIS", "Outbound Post Queue Interrupt Status" },
    { PLX_ADDR_REG, 0x34, WDC_SIZE_32, WDC_READ_WRITE, "OPQIM", "Outbound Post Queue Interrupt Mask" },
    { PLX_ADDR_REG, 0x40, WDC_SIZE_32, WDC_READ_WRITE, "IQP", "Inbound Queue Post" },
    { PLX_ADDR_REG, 0x44, WDC_SIZE_32, WDC_READ_WRITE, "OQP", "Outbound Queue Post" },
    { PLX_ADDR_REG, 0x40, WDC_SIZE_32, WDC_READ_WRITE, "MBOX0_NO_I2O", "Mailbox 0 (I2O disabled)" },
    { PLX_ADDR_REG, 0x44, WDC_SIZE_32, WDC_READ_WRITE, "MBOX1_NO_I2O", "Mailbox 1 (I2O disabled)" },
    { PLX_ADDR_REG, 0x78, WDC_SIZE_32, WDC_READ_WRITE, "MBOXO", "Mailbox 0" },
    { PLX_ADDR_REG, 0x7C, WDC_SIZE_32, WDC_READ_WRITE, "MBOX1", "Mailbox 1" },
    { PLX_ADDR_REG, 0x48, WDC_SIZE_32, WDC_READ_WRITE, "MBOX2", "Mailbox 2" },
    { PLX_ADDR_REG, 0x4C, WDC_SIZE_32, WDC_READ_WRITE, "MBOX3", "Mailbox 3" },
    { PLX_ADDR_REG, 0x50, WDC_SIZE_32, WDC_READ_WRITE, "MBOX4", "Mailbox 4" },
    { PLX_ADDR_REG, 0x54, WDC_SIZE_32, WDC_READ_WRITE, "MBOX5", "Mailbox 5" },
    { PLX_ADDR_REG, 0x58, WDC_SIZE_32, WDC_READ_WRITE, "MBOX6", "Mailbox 6" },
    { PLX_ADDR_REG, 0x5C, WDC_SIZE_32, WDC_READ_WRITE, "MBOX7", "Mailbox 7" },
    { PLX_ADDR_REG, 0x60, WDC_SIZE_32, WDC_READ_WRITE, "P2LDBELL", "PCI-to-Local Doorbell" },
    { PLX_ADDR_REG, 0x64, WDC_SIZE_32, WDC_READ_WRITE, "L2PDBELL", "Local-to-PCI Doorbell" },
    { PLX_ADDR_REG, 0x68, WDC_SIZE_32, WDC_READ_WRITE, "INTCSR", "Interrupt Control/Status"  },
    { PLX_ADDR_REG, 0x6C, WDC_SIZE_32, WDC_READ_WRITE, "CNTRL", "Serial EEPROM/User I/O/Init Ctr & PCI Cmd Codes" },
    { PLX_ADDR_REG, 0x70, WDC_SIZE_32, WDC_READ_WRITE, "PCIHIDR", "PCI Hardcoded Configuration ID" },
    { PLX_ADDR_REG, 0x74, WDC_SIZE_16, WDC_READ_WRITE, "PCIHREV", "PCI Hardcoded Revision ID" },
    { PLX_ADDR_REG, 0x80, WDC_SIZE_32, WDC_READ_WRITE, "DMAMODE0", "DMA Channel 0 Mode" },
    { PLX_ADDR_REG, 0x84, WDC_SIZE_32, WDC_READ_WRITE, "DMAPADR0", "DMA Channel 0 PCI Address" },
    { PLX_ADDR_REG, 0x88, WDC_SIZE_32, WDC_READ_WRITE, "DMALADR0", "DMA Channel 0 Local Address" },
    { PLX_ADDR_REG, 0x8C, WDC_SIZE_32, WDC_READ_WRITE, "DMASIZ0", "DMA Channel 0 Transfer Size (bytes)" },
    { PLX_ADDR_REG, 0x90, WDC_SIZE_32, WDC_READ_WRITE, "DMADPR0", "DMA Channel 0 Descriptor Pointer" },
    { PLX_ADDR_REG, 0x94, WDC_SIZE_32, WDC_READ_WRITE, "DMAMODE1", "DMA Channel 1 Mode" },
    { PLX_ADDR_REG, 0x98, WDC_SIZE_32, WDC_READ_WRITE, "DMAPADR1", "DMA Channel 1 PCI Address" },
    { PLX_ADDR_REG, 0x9C, WDC_SIZE_32, WDC_READ_WRITE, "DMALADR1", "DMA Channel 1 Local Address" },
    { PLX_ADDR_REG, 0xA0, WDC_SIZE_32, WDC_READ_WRITE, "DMASIZ1", "DMA Channel 1 Transfer Size (bytes)" },
    { PLX_ADDR_REG, 0xA4, WDC_SIZE_32, WDC_READ_WRITE, "DMADPR1", "DMA Channel 1 Descriptor Pointer" },
    { PLX_ADDR_REG, 0xA8, WDC_SIZE_8, WDC_READ_WRITE, "DMACSR0", "DMA Channel 0 Command/Status" },
    { PLX_ADDR_REG, 0xA9, WDC_SIZE_8, WDC_READ_WRITE, "DMACSR1", "DMA Channel 1 Command/Status" },
    { PLX_ADDR_REG, 0xAC, WDC_SIZE_32, WDC_READ_WRITE, "DMAARB", "DMA Arbitration" },
    { PLX_ADDR_REG, 0xB0, WDC_SIZE_32, WDC_READ_WRITE, "DMATHR", "DMA Threshold (Channel 0 only)" },
    { PLX_ADDR_REG, 0xB4, WDC_SIZE_32, WDC_READ_WRITE, "DMADAC0", "DMA 0 PCI Dual Address Cycle Address" },
    { PLX_ADDR_REG, 0xB8, WDC_SIZE_32, WDC_READ_WRITE, "DMADAC1", "DMA 1 PCI Dual Address Cycle Address" },
    { PLX_ADDR_REG, 0xC0, WDC_SIZE_32, WDC_READ_WRITE, "MQCR", "Messaging Queue Configuration" },
    { PLX_ADDR_REG, 0xC4, WDC_SIZE_32, WDC_READ_WRITE, "QBAR", "Queue Base Address" },
    { PLX_ADDR_REG, 0xC8, WDC_SIZE_32, WDC_READ_WRITE, "IFHPR", "Inbound Free Head Pointer" },
    { PLX_ADDR_REG, 0xCC, WDC_SIZE_32, WDC_READ_WRITE, "IFTPR", "Inbound Free Tail Pointer" },
    { PLX_ADDR_REG, 0xD0, WDC_SIZE_32, WDC_READ_WRITE, "IPHPR", "Inbound Post Head Pointer" },
    { PLX_ADDR_REG, 0xD4, WDC_SIZE_32, WDC_READ_WRITE, "IPTPR", "Inbound Post Tail Pointer" },
    { PLX_ADDR_REG, 0xD8, WDC_SIZE_32, WDC_READ_WRITE, "OFHPR", "Outbound Free Head Pointer" },
    { PLX_ADDR_REG, 0xDC, WDC_SIZE_32, WDC_READ_WRITE, "OFTPR", "Outbound Free Tail Pointer" },
    { PLX_ADDR_REG, 0xE0, WDC_SIZE_32, WDC_READ_WRITE, "OPHPR", "Outbound Post Head Pointer" },
    { PLX_ADDR_REG, 0xE4, WDC_SIZE_32, WDC_READ_WRITE, "OPTPR", "Outbound Post Tail Pointer" },
    { PLX_ADDR_REG, 0xE8, WDC_SIZE_32, WDC_READ_WRITE, "QSR", "Queue Status/Control" },
    { PLX_ADDR_REG, 0xF0, WDC_SIZE_32, WDC_READ_WRITE, "LAS1RR", "Local Addr Space 1 Range for PCI-to-Local Bus" },
    { PLX_ADDR_REG, 0xF4, WDC_SIZE_32, WDC_READ_WRITE, "LAS1BA", "Local Addr Space 1 Local BAR (Remap)" },
    { PLX_ADDR_REG, 0xF8, WDC_SIZE_32, WDC_READ_WRITE, "LBRD1", "Local Addr Space 1 Bus Region Descriptor" },
    { PLX_ADDR_REG, 0xFC, WDC_SIZE_32, WDC_READ_WRITE, "DMDAC", "PCI Initiatior PCI Dual Address Cycle" },
};

static const WDC_REG gPLX_T_Regs[] = {
    { PLX_ADDR_REG, 0x00, WDC_SIZE_32, WDC_READ_WRITE, "LAS0RR", "Local Addr Space 0 Range" },
    { PLX_ADDR_REG, 0x04, WDC_SIZE_32, WDC_READ_WRITE, "LAS1RR", "Local Addr Space 1 Range" },
    { PLX_ADDR_REG, 0x08, WDC_SIZE_32, WDC_READ_WRITE, "LAS2RR", "Local Addr Space 2 Range" },
    { PLX_ADDR_REG, 0x0C, WDC_SIZE_32, WDC_READ_WRITE, "LAS3RR", "Local Addr Space 3 Range" },
    { PLX_ADDR_REG, 0x10, WDC_SIZE_32, WDC_READ_WRITE, "EROMRR", "Expansion ROM Range" },
    { PLX_ADDR_REG, 0x14, WDC_SIZE_32, WDC_READ_WRITE, "LAS0BA", "Local Addr Space 0 Local BAR (Remap)" },
    { PLX_ADDR_REG, 0x18, WDC_SIZE_32, WDC_READ_WRITE, "LAS1BA", "Local Addr Space 1 Local BAR (Remap)" },
    { PLX_ADDR_REG, 0x1C, WDC_SIZE_32, WDC_READ_WRITE, "LAS2BA", "Local Addr Space 2 Local BAR (Remap)" },
    { PLX_ADDR_REG, 0x20, WDC_SIZE_32, WDC_READ_WRITE, "LAS3BA", "Local Addr Space 3 Local BAR (Remap)" },
    { PLX_ADDR_REG, 0x24, WDC_SIZE_32, WDC_READ_WRITE, "EROMBA", "Expansion ROM Local BAR (Remap)" },
    { PLX_ADDR_REG, 0x28, WDC_SIZE_32, WDC_READ_WRITE, "LAS0BRD", "Local Addr Space 0 Bus Region Descriptors" },
    { PLX_ADDR_REG, 0x2C, WDC_SIZE_32, WDC_READ_WRITE, "LAS1BRD", "Local Addr Space 1 Bus Region Descriptors" },
    { PLX_ADDR_REG, 0x30, WDC_SIZE_32, WDC_READ_WRITE, "LAS2BRD", "Local Addr Space 2 Bus Region Descriptors" },
    { PLX_ADDR_REG, 0x34, WDC_SIZE_32, WDC_READ_WRITE, "LAS3BRD", "Local Addr Space 3 Bus Region Descriptors" },
    { PLX_ADDR_REG, 0x38, WDC_SIZE_32, WDC_READ_WRITE, "EROMBRD", "Expansion ROM Bus Region Descriptors" },
    { PLX_ADDR_REG, 0x3C, WDC_SIZE_32, WDC_READ_WRITE, "CS0BASE", "Chip Select 0 Base Address" },
    { PLX_ADDR_REG, 0x40, WDC_SIZE_32, WDC_READ_WRITE, "CS1BASE", "Chip Select 1 Base Address" },
    { PLX_ADDR_REG, 0x44, WDC_SIZE_32, WDC_READ_WRITE, "CS2BASE", "Chip Select 2 Base Address" },
    { PLX_ADDR_REG, 0x48, WDC_SIZE_32, WDC_READ_WRITE, "CS3BASE", "Chip Select 3 Base Address" },
    { PLX_ADDR_REG, 0x4C, WDC_SIZE_16, WDC_READ_WRITE, "INTCSR", "Interrupt Control/Status" },
    { PLX_ADDR_REG, 0x4E, WDC_SIZE_16, WDC_READ_WRITE, "PROT_AREA", "Serial EEPROM Write-Protected Addr Boundary" },
    { PLX_ADDR_REG, 0x50, WDC_SIZE_32, WDC_READ_WRITE, "CNTRL", "PCI Target Response; Serial EEPROM; Init Ctr" },
    { PLX_ADDR_REG, 0x54, WDC_SIZE_32, WDC_READ_WRITE, "GPIOC", "General Purpose I/O Control" },
    { PLX_ADDR_REG, 0x70, WDC_SIZE_32, WDC_READ_WRITE, "PMDATASEL", "Hidden 1 Power Management Data Select" },
    { PLX_ADDR_REG, 0x74, WDC_SIZE_32, WDC_READ_WRITE, "PMDATASCALE", "Hidden 2 Power Management Data Scale" },
};

/*************************************************************
  Static functions prototypes
 *************************************************************/
static BOOL DMAOpenGetInput(PLX_DIAG_DMA *pDma, BOOL *pfPolling,
    UINT32 *pu32LocalAddr, PDWORD pdwBytes, PDWORD pdwOptions);

/*************************************************************
  Functions implementation
 *************************************************************/

 /* -----------------------------------------------
     Device Open
    ----------------------------------------------- */
static DWORD MenuDeviceOpenCb(PVOID pCbCtx)
{
    PLX_MENU_CTX *pPlxMenuCtx = (PLX_MENU_CTX *)pCbCtx;
    WDC_DEVICE_HANDLE *phDev = pPlxMenuCtx->phDev;

    /* Close open device handle (if exists) */
    if (*phDev && !PLX_DeviceClose(*phDev))
        PLX_DIAG_ERR("p9656_diag: Failed closing PLX device: %s",
            PLX_GetLastErr());

    /* Open a new device handle */
    *phDev = PLX_DeviceOpen(0, 0, pPlxMenuCtx->fIsMaster);
    if (!*phDev)
    {
        /* TODO: Remove VID DID from print? */
        PLX_DIAG_ERR("plx_diag: Failed locating and opening a handle "
            "to device (VID 0x%x DID 0x%x)\n", 0,
            0);
    }

    return WD_STATUS_SUCCESS;
}

void PLX_DIAG_MenuDeviceOpenInit(DIAG_MENU_OPTION *pParentMenu,
    PLX_MENU_CTX *pPlxMenuCtx)
{
    static DIAG_MENU_OPTION deviceOpenMenu = { 0 };

    strcpy(deviceOpenMenu.cOptionName, "Find and open a PLX device");
    deviceOpenMenu.cbEntry = MenuDeviceOpenCb;

    DIAG_MenuSetCtxAndParentForMenus(&deviceOpenMenu, 1, pPlxMenuCtx,
        pParentMenu);
}

/* -----------------------------------------------
    Read/write memory and I/O addresses
   ----------------------------------------------- */

static DWORD ReadWrtieAddrLocal(MENU_CTX_READ_WRITE_ADDR *pRwAddrMenusCtx,
    WDC_DIRECTION direction)
{
    if (pRwAddrMenusCtx->fBlock)
    {
        PLX_DIAG_ReadWriteAddrLocalBlock(*(pRwAddrMenusCtx->phDev),
            direction, pRwAddrMenusCtx->dwAddrSpace);
    }
    else
    {
        PLX_DIAG_ReadWriteAddrLocal(*(pRwAddrMenusCtx->phDev), direction,
            pRwAddrMenusCtx->dwAddrSpace, pRwAddrMenusCtx->mode);
    }

    return WD_STATUS_SUCCESS;
}

static DWORD MenuRwAddrReadLocalOptionCb(PVOID pCbCtx)
{
    MENU_CTX_READ_WRITE_ADDR *pRwAddrMenusCtx =
        (MENU_CTX_READ_WRITE_ADDR *)pCbCtx;

    return ReadWrtieAddrLocal(pRwAddrMenusCtx, WDC_READ);
}

static DWORD MenuRwAddrWriteLocalOptionCb(PVOID pCbCtx)
{
    MENU_CTX_READ_WRITE_ADDR *pRwAddrMenusCtx =
        (MENU_CTX_READ_WRITE_ADDR *)pCbCtx;

    return ReadWrtieAddrLocal(pRwAddrMenusCtx, WDC_WRITE);
}

static void PLX_DIAG_MenuRwAddrLocalOptionsInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_READ_WRITE_ADDR *pRwAddrMenusCtx)
{
    static DIAG_MENU_OPTION readLocalAddressMenu = { 0 };
    static DIAG_MENU_OPTION writeLocalAddressMenu = { 0 };
    static DIAG_MENU_OPTION options[2] = { 0 };

    strcpy(readLocalAddressMenu.cOptionName, "Read from a local address in the active address "
        "space");
    readLocalAddressMenu.cbEntry = MenuRwAddrReadLocalOptionCb;

    strcpy(writeLocalAddressMenu.cOptionName, "Write to a local address in the active address "
        "space");
    writeLocalAddressMenu.cbEntry = MenuRwAddrWriteLocalOptionCb;

    options[0] = readLocalAddressMenu;
    options[1] = writeLocalAddressMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        pRwAddrMenusCtx, pParentMenu);
}

/* Read/write memory or I/O space address menu */
void PLX_DIAG_MenuReadWriteAddrInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static MENU_CTX_READ_WRITE_ADDR rwAddrMenusCtx;
    DIAG_MENU_OPTION *pRwAddrMenuRoot;

    rwAddrMenusCtx.phDev = phDev;
    rwAddrMenusCtx.fBlock = FALSE;
    rwAddrMenusCtx.mode = WDC_MODE_32;
    rwAddrMenusCtx.dwAddrSpace = ACTIVE_ADDR_SPACE_NEEDS_INIT;

    pRwAddrMenuRoot = MenuCommonRwAddrInit(pParentMenu, &rwAddrMenusCtx);

    /* Init local addr read/write options */
    PLX_DIAG_MenuRwAddrLocalOptionsInit(pRwAddrMenuRoot, &rwAddrMenusCtx);
}

void PLX_DIAG_ReadWriteAddrLocal(WDC_DEVICE_HANDLE hDev,
    WDC_DIRECTION direction, PLX_ADDR addrSpace, WDC_ADDR_MODE mode)
{
    DWORD dwStatus;
    DWORD dwLocalAddr;
    BYTE bData = 0;
    WORD wData = 0;
    UINT32 u32Data = 0;
    UINT64 u64Data = 0;

    if (!hDev)
    {
        PLX_DIAG_ERR("PLX_DIAG_ReadWriteAddrLocal: Error - NULL WDC device "
            "handle\n");
        return;
    }

    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(&dwLocalAddr,
        "Enter local address", TRUE, 0, 0))
    {
        return;
    }

    if ((WDC_WRITE == direction) &&
        !WDC_DIAG_InputWriteData((WDC_MODE_8 == mode) ? (PVOID)&bData :
            (WDC_MODE_16 == mode) ? (PVOID)&wData :
            (WDC_MODE_32 == mode) ? (PVOID)&u32Data : (PVOID)&u64Data,
            WDC_ADDR_MODE_TO_SIZE(mode)))
    {
        return;
    }

    switch (mode)
    {
    case WDC_MODE_8:
        dwStatus = (WDC_READ == direction) ?
            PLX_ReadAddrLocal8(hDev, addrSpace, dwLocalAddr, &bData) :
            PLX_WriteAddrLocal8(hDev, addrSpace, dwLocalAddr, bData);
        if (WD_STATUS_SUCCESS == dwStatus)
        {
            printf("%s 0x%X %s local address 0x%x in BAR %d\n",
                (WDC_READ == direction) ? "Read" : "Wrote", (UINT32)bData,
                (WDC_READ == direction) ? "from" : "to", dwLocalAddr,
                addrSpace);
        }
        break;
    case WDC_MODE_16:
        dwStatus = (WDC_READ == direction) ?
            PLX_ReadAddrLocal16(hDev, addrSpace, dwLocalAddr, &wData) :
            PLX_WriteAddrLocal16(hDev, addrSpace, dwLocalAddr, wData);
        if (WD_STATUS_SUCCESS == dwStatus)
        {
            printf("%s 0x%hX %s local address 0x%x in BAR %d\n",
                (WDC_READ == direction) ? "Read" : "Wrote", wData,
                (WDC_READ == direction) ? "from" : "to", dwLocalAddr,
                addrSpace);
        }
        break;
    case WDC_MODE_32:
        dwStatus = (WDC_READ == direction) ?
            PLX_ReadAddrLocal32(hDev, addrSpace, dwLocalAddr, &u32Data) :
            PLX_WriteAddrLocal32(hDev, addrSpace, dwLocalAddr, u32Data);
        if (WD_STATUS_SUCCESS == dwStatus)
        {
            printf("%s 0x%X %s local address 0x%x in BAR %d\n",
                (WDC_READ == direction) ? "Read" : "Wrote", u32Data,
                (WDC_READ == direction) ? "from" : "to", dwLocalAddr,
                addrSpace);
        }
        break;
    case WDC_MODE_64:
        dwStatus = (WDC_READ == direction) ?
            PLX_ReadAddrLocal64(hDev, addrSpace, dwLocalAddr, &u64Data) :
            PLX_WriteAddrLocal64(hDev, addrSpace, dwLocalAddr, u64Data);
        if (WD_STATUS_SUCCESS == dwStatus)
        {
            printf("%s 0x%" PRI64 "X %s local address 0x%x in BAR %d\n",
                (WDC_READ == direction) ? "Read" : "Wrote", u64Data,
                (WDC_READ == direction) ? "from" : "to", dwLocalAddr,
                addrSpace);
        }
        break;
    default:
        PLX_DIAG_ERR("PLX_DIAG_ReadWriteAddrLocal: Error - Invalid mode (%d)\n",
            mode);
        return;
    }

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        printf("Failed to %s local address 0x%x in BAR %d. "
            "Error 0x%x - %s\n", (WDC_READ == direction) ? "read from" :
            "write to", dwLocalAddr, addrSpace, dwStatus, Stat2Str(dwStatus));
    }
}

void PLX_DIAG_ReadWriteAddrLocalBlock(WDC_DEVICE_HANDLE hDev,
    WDC_DIRECTION direction, PLX_ADDR addrSpace)
{
    DWORD dwStatus;
    DWORD dwLocalAddr, dwBytes;
    const CHAR *sDir = (WDC_READ == direction) ? "read" : "write";
    PVOID pBuf = NULL;
    WDC_ADDR_MODE mode;
    WDC_ADDR_RW_OPTIONS options;
    BOOL fAutoInc;

    if (!hDev)
    {
        PLX_DIAG_ERR("PLX_DIAG_ReadWriteAddrLocalBlock: Error - NULL WDC "
            "device handle\n");
        return;
    }

    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(&dwLocalAddr,
        "Enter local address", TRUE, 0, 0))
    {
        return;
    }

    sprintf(gsInput, "Enter number of bytes to %s", sDir);
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(&dwBytes, gsInput, TRUE, 0, 0))
        return;

    if (!dwBytes)
    {
        printf("The number of bytes to %s must be bigger than 0", sDir);
        goto Exit;
    }

    pBuf = malloc(dwBytes);
    if (!pBuf)
    {
        PLX_DIAG_ERR("PLX_DIAG_ReadWriteAddrLocalBlock: Failed allocating %s "
            "data buffer\n", sDir);
        goto Exit;
    }
    memset(pBuf, 0, dwBytes);

    if (WDC_WRITE == direction)
    {
        printf("Enter data to write (hex format): 0x");
        if (!DIAG_GetHexBuffer(pBuf, dwBytes))
            goto Exit;
    }

    if (!WDC_DIAG_SetMode(&mode))
        goto Exit;

    sprintf(gsInput, "Do you wish to increment the address after each %s block "
        "(%d bytes)\n(0 - No, Otherwise - Yes)? ",
        sDir, WDC_ADDR_MODE_TO_SIZE(mode));
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(&fAutoInc, gsInput,
        FALSE, 0, 0))
    {
        goto Exit;
    }

    options = fAutoInc ? WDC_ADDR_RW_DEFAULT : WDC_ADDR_RW_NO_AUTOINC;

    dwStatus = direction == WDC_READ ?
        PLX_ReadAddrLocalBlock(hDev, addrSpace, dwLocalAddr, dwBytes, pBuf,
            mode, options) :
        PLX_WriteAddrLocalBlock(hDev, addrSpace, dwLocalAddr, dwBytes, pBuf,
            mode, options);

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("%s %d bytes %s local address 0x%x\n",
            (WDC_READ == direction) ? "Read" : "Wrote", dwBytes,
            (WDC_READ == direction) ? "from" : "to", dwLocalAddr);

        if (WDC_READ == direction)
        {
            printf("Data read from local address 0x%x (hex format):\n",
                dwLocalAddr);
            DIAG_PrintHexBuffer(pBuf, dwBytes, FALSE);
        }
    }
    else
    {
        printf("Failed to %s %d bytes %s local address 0x%x. "
            "Error 0x%x - %s\n", sDir, dwBytes, (WDC_READ == direction) ?
            "from" : "to", dwLocalAddr, dwStatus, Stat2Str(dwStatus));
    }

Exit:
    if (pBuf)
        free(pBuf);

    printf("\n");
    printf("Press ENTER to return to the menu\n");
    fgets(gsInput, sizeof(gsInput), stdin);
}

/* -----------------------------------------------
    Read/write the configuration space
   ----------------------------------------------- */

void PLX_DIAG_MenuReadWriteCfgSpaceInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static MENU_CTX_CFG cfgCtx;

    BZERO(cfgCtx);
    cfgCtx.phDev = phDev;
    cfgCtx.pCfgRegs = gPLX_CfgRegs;
    cfgCtx.dwCfgRegsNum = sizeof(gPLX_CfgRegs) / sizeof(*gPLX_CfgRegs);

    MenuCommonCfgInit(pParentMenu, &cfgCtx);
}

/* -----------------------------------------------
    Read/write the run-time registers
   ----------------------------------------------- */

static DWORD MenuRwRegsCb(PVOID pCbCtx)
{
    MENU_CTX_RW_REGS *pRegsMenusCtx = (MENU_CTX_RW_REGS *)pCbCtx;
    BOOL fIsMater = PLX_IsMaster(*(pRegsMenusCtx->phDev));

    pRegsMenusCtx->pRegsArr = fIsMater ? gPLX_M_Regs : gPLX_T_Regs;
    pRegsMenusCtx->dwRegsNum = (fIsMater ?
        sizeof(gPLX_M_Regs) :
        sizeof(gPLX_T_Regs)) / sizeof(*(pRegsMenusCtx->pRegsArr));

    WDC_DIAG_RegsInfoPrint(*(pRegsMenusCtx->phDev),
        pRegsMenusCtx->pRegsArr, pRegsMenusCtx->dwRegsNum,
        WDC_DIAG_REG_PRINT_ALL, FALSE);

    return WD_STATUS_SUCCESS;
}


/* Display read/write run-time registers menu */
void PLX_DIAG_MenuReadWriteRegsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static MENU_CTX_RW_REGS regsMenusCtx;
    DIAG_MENU_OPTION *pRwReagsMenuRoot;

    BZERO(regsMenusCtx);
    regsMenusCtx.phDev = phDev;
    regsMenusCtx.pRegsArr = gPLX_M_Regs;
    regsMenusCtx.dwRegsNum = sizeof(gPLX_M_Regs);
    strcpy(regsMenusCtx.sModuleName, "PLX");

    pRwReagsMenuRoot = MenuCommonRwRegsInit(pParentMenu, &regsMenusCtx);

    pRwReagsMenuRoot->cbEntry = MenuRwRegsCb;
}

/* -----------------------------------------------
    Direct Memory Access (DMA)
   ----------------------------------------------- */

/* DMA menu options */
enum {
    PLX_DIAG_MENU_DMA_OPEN_CH_0 = 1,
    PLX_DIAG_MENU_DMA_OPEN_CH_1,
    PLX_DIAG_MENU_DMA_CLOSE = 1,
    PLX_DIAG_MENU_DMA_EXIT = DIAG_EXIT_MENU,
};

static BOOL MenuDmaIsDeviceNull(DIAG_MENU_OPTION *pMenu)
{
    return *((PLX_MENU_DMA_CTX *)pMenu->pCbCtx)->phDev == NULL;
}

static BOOL MenuDmaIsDmaOpened(DIAG_MENU_OPTION *pMenu)
{
    return ((PLX_MENU_DMA_CTX *)pMenu->pCbCtx)->pDma->hDma != NULL;
}

static BOOL MenuDmaIsDmaClosed(DIAG_MENU_OPTION *pMenu)
{
    return ((PLX_MENU_DMA_CTX *)pMenu->pCbCtx)->pDma->hDma == NULL;
}

static DWORD MenuDmaOpenChZeroOptionCb(PVOID pCbCtx)
{
    PLX_MENU_DMA_CTX *pPlxMenuDmaCtx = (PLX_MENU_DMA_CTX *)pCbCtx;

    pPlxMenuDmaCtx->dmaChannel = PLX_DMA_CHANNEL_0;

    return PLX_DIAG_DMAOpen(pPlxMenuDmaCtx);
}

static DWORD MenuDmaOpenChOneOptionCb(PVOID pCbCtx)
{
    PLX_MENU_DMA_CTX *pPlxMenuDmaCtx = (PLX_MENU_DMA_CTX *)pCbCtx;

    pPlxMenuDmaCtx->dmaChannel = PLX_DMA_CHANNEL_1;

    return PLX_DIAG_DMAOpen(pPlxMenuDmaCtx);
}

static DWORD MenuDmaCloseOptionCb(PVOID pCbCtx)
{
    PLX_MENU_DMA_CTX *pPlxMenuDmaCtx = (PLX_MENU_DMA_CTX *)pCbCtx;

    return PLX_DIAG_DMAClose(*(pPlxMenuDmaCtx->phDev), pPlxMenuDmaCtx->pDma);
}

static void PLX_DIAG_MenuDmaSubMenusInit(DIAG_MENU_OPTION *pParentMenu,
    PLX_MENU_DMA_CTX *pPlxMenuDmaCtx)
{
    static DIAG_MENU_OPTION openDmaChannel0Menu = { 0 };
    static DIAG_MENU_OPTION openDmaChannel1Menu = { 0 };
    static DIAG_MENU_OPTION closeDmaHandleMenu = { 0 };
    static DIAG_MENU_OPTION options[3] = { 0 };


    strcpy(openDmaChannel0Menu.cOptionName, "Open DMA - channel 0");
    openDmaChannel0Menu.cbEntry = MenuDmaOpenChZeroOptionCb;
    openDmaChannel0Menu.cbIsHidden = MenuDmaIsDmaOpened;

    strcpy(openDmaChannel1Menu.cOptionName, "Open DMA - channel 1");
    openDmaChannel1Menu.cbEntry = MenuDmaOpenChOneOptionCb;
    openDmaChannel1Menu.cbIsHidden = MenuDmaIsDmaOpened;

    strcpy(closeDmaHandleMenu.cOptionName, "Close DMA handle");
    closeDmaHandleMenu.cbEntry = MenuDmaCloseOptionCb;
    closeDmaHandleMenu.cbIsHidden = MenuDmaIsDmaClosed;

    options[0] = openDmaChannel0Menu;
    options[1] = openDmaChannel1Menu;
    options[2] = closeDmaHandleMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        pPlxMenuDmaCtx, pParentMenu);
}

void PLX_DIAG_MenuDmaInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev, PLX_DIAG_DMA *pDma,
    PLX_INT_HANDLER MasterDiagDmaIntHandler, BOOL fIsDACSupported)
{
    static DIAG_MENU_OPTION dmaMenuRoot = { 0 };
    static PLX_MENU_DMA_CTX plxMenuDmaCtx = { 0 };

    strcpy(dmaMenuRoot.cTitleName, "Direct Memory Access (DMA)");
    strcpy(dmaMenuRoot.cOptionName, "Direct Memory Access (DMA) - polling or "
        "interrupt");
    dmaMenuRoot.cbIsHidden = MenuDmaIsDeviceNull;

    plxMenuDmaCtx.phDev = phDev;
    plxMenuDmaCtx.pDma = pDma;
    plxMenuDmaCtx.DmaIntHandler = MasterDiagDmaIntHandler;
    plxMenuDmaCtx.fIsDACSupported = fIsDACSupported;

    PLX_DIAG_MenuDmaSubMenusInit(&dmaMenuRoot, &plxMenuDmaCtx);
    DIAG_MenuSetCtxAndParentForMenus(&dmaMenuRoot, 1, &plxMenuDmaCtx,
        pParentMenu);
}

DWORD PLX_DIAG_DMAOpen(PLX_MENU_DMA_CTX *pPlxMenuDmaCtx)
{
    WDC_DEVICE_HANDLE hDev = *(pPlxMenuDmaCtx->phDev);
    PLX_DIAG_DMA *pDma = pPlxMenuDmaCtx->pDma;
    PLX_DMA_CHANNEL dmaChannel = pPlxMenuDmaCtx->dmaChannel;
    DWORD dwStatus;
    UINT32 u32LocalAddr;
    WDC_ADDR_MODE mode = WDC_MODE_32; /* Local bus width */
    DWORD dwBytes;
    DWORD dwOptions;
    BOOL fPolling;
    PPLX_DEV_CTX pDevCtx;
    PLX_DMA_STRUCT *pPLXDma;

    /* Get input for user */
    if (!DMAOpenGetInput(pDma, &fPolling, &u32LocalAddr, &dwBytes, &dwOptions))
        return WD_INVALID_PARAMETER;

    /* Allocate buffer for Scatter/Gather DMA (if selected) */
    if (pDma->fSG)
    {
        pDma->pBuf = malloc(dwBytes);
        if (!pDma->pBuf)
        {
            PLX_DIAG_ERR("PLX_DIAG_DMAOpen: Failed allocating Scatter/Gather "
                "DMA data buffer\n");
            return WD_INSUFFICIENT_RESOURCES;
        }
        memset(pDma->pBuf, 0, dwBytes);
    }
    else
    {
        dwOptions |= DMA_KERNEL_BUFFER_ALLOC;
    }

    /* Open DMA for selected channel */
    dwStatus = PLX_DMAOpen(hDev, u32LocalAddr, &pDma->pBuf, dwOptions,
        dwBytes, mode, dmaChannel, &pDma->hDma,
        pPlxMenuDmaCtx->fIsDACSupported);

    printf("\n");
    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("DMA for channel %d opened successfully (handle [%p])\n",
            dmaChannel, (void *)pDma->hDma);
    }
    else
    {
        printf("Failed to open DMA for channel %d. Error 0x%x - %s\n",
            dmaChannel, dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    /* Enable DMA interrupts (if not polling) */
    if (!fPolling && !PLX_IntIsEnabled(hDev))
    {
        dwStatus = PLX_IntEnable(hDev, pPlxMenuDmaCtx->DmaIntHandler,
            pDma->hDma);
        printf("\n");
        if (WD_STATUS_SUCCESS == dwStatus)
        {
            printf("DMA interrupts enabled\n");
        }
        else
        {
            printf("Failed enabling DMA interrupts. Error 0x%x - %s\n",
                dwStatus, Stat2Str(dwStatus));
            goto Error;
        }
    }

    /* Start DMA */
    PLX_DMAStart(hDev, pDma->hDma);
    printf("Started DMA on channel %d\n", dmaChannel);

    /* Poll for completion (if polling selected) */
    if (fPolling)
    {
        printf("\nPolling hardware for channel %d DMA completion ...\n",
            dmaChannel);
        if (PLX_DMAPollCompletion(hDev, pDma->hDma))
        {
            pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(hDev);
            pPLXDma = pDevCtx->pPLXDma;

            printf("Channel %d DMA completed\n", dmaChannel);
            DIAG_PrintPerformance(dwBytes, &pPLXDma->dmaStartTime);
        }
        else
        {
            printf("Channel %d DMA aborted\n", dmaChannel);
        }
    }
    DIAG_PrintHexBuffer(pDma->pBuf, dwBytes, FALSE);

    return dwStatus;

Error:
    return PLX_DIAG_DMAClose(*(pPlxMenuDmaCtx->phDev), pPlxMenuDmaCtx->pDma);
}

/* Dma transaction */
void DLLCALLCONV PLX_DMA_program(WDC_DEVICE_HANDLE hDev)
{
    PPLX_DEV_CTX pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(hDev);
    PLX_DMA_STRUCT *pPLXDma = pDevCtx->pPLXDma;
    WD_DMA *pDma = pPLXDma->pDma;
    WD_DMA *pDmaDescriptors = pPLXDma->pDmaList;
    BOOL fIsRead = pDma->dwOptions & DMA_FROM_DEVICE;
    UINT32 offset = pPLXDma->u32LocalAddr + pDma->dwBytesTransferred;
    UINT32 u32DmaMode = 0;
    UINT32 u32DMADPR = fIsRead ? BIT3 : 0;

    // (BIT0 | BIT1) - Local bus data width: 32
    // BIT6 - Enable Ready input
    // BIT8 - Local burst
    // BIT10 - Done interrupt enable
    // BIT17 - interrupt to pci
    if (!pPLXDma->fPolling)
        u32DmaMode |= BIT10 | BIT17;

    u32DmaMode |= (BIT0 | BIT1) | BIT6 | BIT8;

    if (!pDma->dwPages)
    {
        PLX_DIAG_ERR("%s: Error- Zero pages\n", __FUNCTION__);
        return;
    }
    else if (pDma->dwPages == 1)
    {
        PLX_WriteReg32(hDev, pPLXDma->dwDMAPADR,
            (UINT32)pDma->Page[0].pPhysicalAddr);
        PLX_WriteReg32(hDev, pPLXDma->dwDMALADR, offset);
        PLX_WriteReg32(hDev, pPLXDma->dwDMASIZ,
            (UINT32)pDma->Page[0].dwBytes);
    }
    else
    {
        DWORD dwPage;
        // DMA transfer element logical address
        DMA_ADDR dteLA = pDmaDescriptors->Page[0].pPhysicalAddr +
            sizeof(PLX_DTE);

        // DMA transfer element virtual address
        PLX_DTE *dteVA = (PLX_DTE *)pDmaDescriptors->pUserAddr;

        for (dwPage = 0; dwPage < pDma->dwPages; dwPage++)
        {
            dteVA[dwPage].u32PADR = (UINT32)pDma->Page[dwPage].pPhysicalAddr;
            dteVA[dwPage].u32LADR = (UINT32)offset;
            dteVA[dwPage].u32SIZ = pDma->Page[dwPage].dwBytes;
            dteVA[dwPage].u32DPR = (UINT32)dteLA | BIT0 | (fIsRead ? BIT3 : 0);

            if (dwPage + 1 == pDma->dwPages)
                dteVA[dwPage].u32DPR |= BIT1; /* Mark end of chain */

            offset += pDma->Page[dwPage].dwBytes;
            dteLA += sizeof(PLX_DTE);
        }

        u32DmaMode |= BIT9; // SG bit
        u32DMADPR |= (UINT32)pDmaDescriptors->Page[0].pPhysicalAddr | BIT0;
    }

    PLX_WriteReg32(hDev, pPLXDma->dwDMAMODE, u32DmaMode);
    PLX_WriteReg32(hDev, pPLXDma->dwDMADPR, u32DMADPR);

    // Start DMA
    PLX_WriteReg8(hDev, pPLXDma->dwDMACSR, (BYTE)(BIT0 | BIT1));
}

void PLX_DIAG_DMATransactionPolling(WDC_DEVICE_HANDLE hDev,
    PLX_DIAG_DMA *pPlxDiagDma)
{

    DWORD dwStatus;
    PPLX_DEV_CTX pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(hDev);
    PLX_DMA_STRUCT *pPLXDma = pDevCtx->pPLXDma;
    WD_DMA *pDma = pPLXDma->pDma;

    /* Poll for completion */
    printf("\nPolling hardware for channel %d DMA completion ...\n",
        pPLXDma->dmaChannel);

    do
    {
        if (!PLX_DMAPollCompletion(hDev, pPlxDiagDma->hDma))
        {
            printf("Channel %d DMA aborted\n", pPLXDma->dmaChannel);
            return;
        }

        printf("DMA transfer has been finished\n");
        dwStatus = WDC_DMATransferCompletedAndCheck(pDma, TRUE);
    } while (dwStatus == (DWORD)WD_MORE_PROCESSING_REQUIRED);

    if (dwStatus == WD_STATUS_SUCCESS)
    {
        printf("Channel %d DMA transaction completed\n", pPLXDma->dmaChannel);
        DIAG_PrintPerformance(pDma->dwBytes, &pPLXDma->dmaStartTime);
    }
    else
    {
        printf("Channel %d DMA transaction failed\n", pPLXDma->dmaChannel);
    }
}

DWORD PLX_DIAG_DMATransactionExecute(PVOID pCbCtx)
{
    PLX_MENU_DMA_CTX *pPlxMenuDmaCtx = (PLX_MENU_DMA_CTX *)pCbCtx;
    WDC_DEVICE_HANDLE hDev = *(pPlxMenuDmaCtx->phDev);
    PLX_DIAG_DMA *pPlxDiagDma = pPlxMenuDmaCtx->pDma;
    DWORD dwStatus;
    PPLX_DEV_CTX pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(hDev);
    PLX_DMA_STRUCT *pPLXDma = pDevCtx->pPLXDma;
    WD_DMA *pDma = pPLXDma->pDma;
    UINT32 u32INTCSR;

    printf("\n%s: Executing DMA transaction for channel %d.\n", __FUNCTION__,
        pPLXDma->dmaChannel);

    if (!pPLXDma->fPolling)
    {
        /* Physically enable the interrupts on the board */
        PLX_ReadReg32(hDev, pDevCtx->dwINTCSR, &u32INTCSR);
        PLX_WriteReg32(hDev, pDevCtx->dwINTCSR, u32INTCSR | BIT8 |
            ((pPLXDma->dmaChannel == PLX_DMA_CHANNEL_0) ? BIT18 : BIT19));
    }

    get_cur_time(&pPLXDma->dmaStartTime);
    dwStatus = WDC_DMATransactionExecute(pDma,
        pPLXDma->funcProgramDMATransaction, hDev);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        printf("%s: Failed to execute DMA transaction for channel %d. Error "
            "0x%x - %s\n", __FUNCTION__, pPLXDma->dmaChannel, dwStatus,
            Stat2Str(dwStatus));
    }
    else if (pPLXDma->fPolling)
    {
        PLX_DIAG_DMATransactionPolling(hDev, pPlxDiagDma);
    }

    return dwStatus;
}

DWORD PLX_DIAG_DMATransactionRelease(PVOID pCbCtx)
{
    PLX_MENU_DMA_CTX *pPlxMenuDmaCtx = (PLX_MENU_DMA_CTX *)pCbCtx;
    WDC_DEVICE_HANDLE hDev = *(pPlxMenuDmaCtx->phDev);
    DWORD dwStatus;
    PPLX_DEV_CTX pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(hDev);
    PLX_DMA_STRUCT *pPLXDma = pDevCtx->pPLXDma;
    WD_DMA *pDma = pPLXDma->pDma;

    dwStatus = WDC_DMATransactionRelease(pDma);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        printf("%s: Failed to release DMA transaction for channel %d. Error "
            "0x%x - %s\n", __FUNCTION__, pPLXDma->dmaChannel, dwStatus,
            Stat2Str(dwStatus));
    }
    else
    {
        printf("%s: DMA transaction of channel %d was released. \n",
            __FUNCTION__, pPLXDma->dmaChannel);
    }

    return dwStatus;
}

/* DMA transaction menu options */
enum {
    PLX_DIAG_MENU_DMA_TRANSACTION_INIT_0 = 1,
    PLX_DIAG_MENU_DMA_TRANSACTION_INIT_1,
    PLX_DIAG_MENU_DMA_TRANSACTION_EXECUTE = 1,
    PLX_DIAG_MENU_DMA_TRANSACTION_COMPLETE,
    PLX_DIAG_MENU_DMA_TRANSACTION_DISPLAY_BUFFER_CONTENT,
    PLX_DIAG_MENU_DMA_TRANSACTION_UNINIT,
};

static DWORD MenuDmaTransactionOpenChZeroOptionCb(PVOID pCbCtx)
{
    PLX_MENU_DMA_CTX *pPlxMenuDmaCtx = (PLX_MENU_DMA_CTX *)pCbCtx;

    pPlxMenuDmaCtx->dmaChannel = PLX_DMA_CHANNEL_0;
    return PLX_DIAG_DMATransactionInit(pPlxMenuDmaCtx);
}

static DWORD MenuDmaTransactionOpenChOneOptionCb(PVOID pCbCtx)
{
    PLX_MENU_DMA_CTX *pPlxMenuDmaCtx = (PLX_MENU_DMA_CTX *)pCbCtx;

    pPlxMenuDmaCtx->dmaChannel = PLX_DMA_CHANNEL_1;
    return PLX_DIAG_DMATransactionInit(pPlxMenuDmaCtx);
}

static DWORD MenuDmaTransactionPrintBufferOptionCb(PVOID pCbCtx)
{
    PLX_DIAG_DMA *pPlxDiagDma = ((PLX_MENU_DMA_CTX *)pCbCtx)->pDma;
    WD_DMA *pDma = ((PLX_DMA_STRUCT *)pPlxDiagDma->hDma)->pDma;

    DIAG_PrintHexBuffer(pDma->pUserAddr, pDma->dwBytes, FALSE);

    return WD_STATUS_SUCCESS;
}

static void PLX_DIAG_MenuDmaTransactionSubMenusInit(
    DIAG_MENU_OPTION *pParentMenu, PLX_MENU_DMA_CTX *pPlxMenuDmaCtx)
{
    static DIAG_MENU_OPTION initTransactionChannel0DmaMenu = { 0 };
    static DIAG_MENU_OPTION initTransactionChannel1DmaMenu = { 0 };
    static DIAG_MENU_OPTION executeTransactionMenu = { 0 };
    static DIAG_MENU_OPTION releaseTransactionMenu = { 0 };
    static DIAG_MENU_OPTION displayBufferMenu = { 0 };
    static DIAG_MENU_OPTION uninitTransactionDmaMenu = { 0 };
    static DIAG_MENU_OPTION options[6] = { 0 };

    strcpy(initTransactionChannel0DmaMenu.cOptionName, "Initialize "
        "transaction DMA - channel 0");
    initTransactionChannel0DmaMenu.cbEntry =
        MenuDmaTransactionOpenChZeroOptionCb;
    initTransactionChannel0DmaMenu.cbIsHidden = MenuDmaIsDmaOpened;

    strcpy(initTransactionChannel1DmaMenu.cOptionName, "Initialize "
        "transaction DMA - channel 1");
    initTransactionChannel1DmaMenu.cbEntry =
        MenuDmaTransactionOpenChOneOptionCb;
    initTransactionChannel1DmaMenu.cbIsHidden = MenuDmaIsDmaOpened;

    strcpy(executeTransactionMenu.cOptionName, "Execute transaction");
    executeTransactionMenu.cbEntry = PLX_DIAG_DMATransactionExecute;
    executeTransactionMenu.cbIsHidden = MenuDmaIsDmaClosed;

    strcpy(releaseTransactionMenu.cOptionName, "Release transaction");
    releaseTransactionMenu.cbEntry = PLX_DIAG_DMATransactionRelease;
    releaseTransactionMenu.cbIsHidden = MenuDmaIsDmaClosed;

    strcpy(displayBufferMenu.cOptionName, "Display transferred buffer "
        "content");
    displayBufferMenu.cbEntry = MenuDmaTransactionPrintBufferOptionCb;
    displayBufferMenu.cbIsHidden = MenuDmaIsDmaClosed;

    strcpy(uninitTransactionDmaMenu.cOptionName, "Uninitialize DMA "
        "transaction");
    uninitTransactionDmaMenu.cbEntry = MenuDmaCloseOptionCb;
    uninitTransactionDmaMenu.cbIsHidden = MenuDmaIsDmaClosed;

    options[0] = initTransactionChannel0DmaMenu;
    options[1] = initTransactionChannel1DmaMenu;
    options[2] = executeTransactionMenu;
    options[3] = releaseTransactionMenu;
    options[4] = displayBufferMenu;
    options[5] = uninitTransactionDmaMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        pPlxMenuDmaCtx, pParentMenu);
}

void PLX_DIAG_MenuDmaTransactionInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev, PLX_DIAG_DMA *pDma,
    PLX_INT_HANDLER DiagDmaTransactionIntHandler)
{
    static DIAG_MENU_OPTION dmaMenuRoot = { 0 };
    static PLX_MENU_DMA_CTX plxMenuDmaCtx = { 0 };

    strcpy(dmaMenuRoot.cTitleName, "Transaction Direct Memory Access (DMA)");
    strcpy(dmaMenuRoot.cOptionName, "Direct Memory Access (DMA) transaction");
    dmaMenuRoot.cbIsHidden = MenuDmaIsDeviceNull;

    plxMenuDmaCtx.phDev = phDev;
    plxMenuDmaCtx.pDma = pDma;
    plxMenuDmaCtx.DmaIntHandler = DiagDmaTransactionIntHandler;

    PLX_DIAG_MenuDmaTransactionSubMenusInit(&dmaMenuRoot, &plxMenuDmaCtx);
    DIAG_MenuSetCtxAndParentForMenus(&dmaMenuRoot, 1, &plxMenuDmaCtx,
        pParentMenu);
}

DWORD PLX_DIAG_DMATransactionInit(PLX_MENU_DMA_CTX *pPlxMenuDmaCtx)
{
    WDC_DEVICE_HANDLE hDev = *(pPlxMenuDmaCtx->phDev);
    PLX_DIAG_DMA *pPlxDiagDma = pPlxMenuDmaCtx->pDma;
    PLX_DMA_CHANNEL dmaChannel = pPlxMenuDmaCtx->dmaChannel;
    DWORD dwStatus, dwBytes, dwOptions;
    UINT32 u32LocalAddr;
    BOOL fPolling;
    PPLX_DEV_CTX pDevCtx;
    PLX_DMA_STRUCT *pPLXDma;

    /* Get input for user */
    if (!DMAOpenGetInput(pPlxDiagDma, &fPolling, &u32LocalAddr, &dwBytes,
        &dwOptions))
    {
        return WD_INVALID_PARAMETER;
    }

    /* Allocate buffer for Scatter/Gather DMA (if selected) */
    if (pPlxDiagDma->fSG)
    {
        pPlxDiagDma->pBuf = malloc(dwBytes);
        if (!pPlxDiagDma->pBuf)
        {
            PLX_DIAG_ERR("%s: Failed allocating Scatter/Gather DMA data buffer"
                "\n", __FUNCTION__);
            return WD_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        dwOptions |= DMA_KERNEL_BUFFER_ALLOC;
    }

    /* Initializing DMA transaction for selected channel */

    dwStatus = PLX_DMATransactionInit(hDev, &pPlxDiagDma->pBuf, dwOptions,
        dwBytes, dmaChannel, &pPlxDiagDma->hDma, pPlxMenuDmaCtx->DmaIntHandler,
        PLX_TRANSACTION_CONTIG_ALIGNMENT,
        PLX_TRANSACTION_SAMPLE_MAX_TRANSFER_SIZE, sizeof(PLX_DTE));

    printf("\n");
    if (dwStatus == WD_STATUS_SUCCESS)
    {
        printf("DMA for channel %d initialized successfully (handle [%p])\n",
            dmaChannel, (void *)pPlxDiagDma->hDma);
    }
    else
    {
        printf("Failed to initialize DMA for channel %d. Error 0x%x - %s\n",
            dmaChannel, dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    if (dwOptions & DMA_TO_DEVICE)
        DIAG_FillHexBuffer(pPlxDiagDma->pBuf, dwBytes, 0, TRUE);
    else
        memset(pPlxDiagDma->pBuf, 0, dwBytes);

    pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(hDev);
    pPLXDma = pDevCtx->pPLXDma;
    pPLXDma->u32LocalAddr = u32LocalAddr;
    pPLXDma->fPolling = fPolling;
    pPLXDma->funcProgramDMATransaction = PLX_DMA_program;

    return dwStatus;

Error:
    return PLX_DIAG_DMAClose(*(pPlxMenuDmaCtx->phDev), pPlxMenuDmaCtx->pDma);
}

DWORD PLX_DIAG_DMAClose(WDC_DEVICE_HANDLE hDev, PLX_DIAG_DMA *pDma)
{
    DWORD dwStatus;

    if (!pDma)
        return WD_WINDRIVER_STATUS_ERROR;

    if (PLX_IntIsEnabled(hDev))
    {
        dwStatus = PLX_IntDisable(hDev);
        printf("DMA interrupts disable%s\n",
            (WD_STATUS_SUCCESS == dwStatus) ? "d" : " failed");
    }

    if (pDma->hDma)
    {
        PLX_DMAClose(hDev, pDma->hDma);
        printf("DMA closed (handle [%p])\n", (void *)pDma->hDma);
    }

    if (pDma->fSG && pDma->pBuf)
        free(pDma->pBuf);

    BZERO(*pDma);

    return WD_STATUS_SUCCESS;
}

static BOOL DMAOpenGetInput(PLX_DIAG_DMA *pDma, BOOL *pfPolling,
    UINT32 *pu32LocalAddr, PDWORD pdwBytes, PDWORD pdwOptions)
{
    DWORD tmp;

    printf("\n");
    printf("Select DMA allocation type:\n");
    printf("1. Scatter/Gather\n");
    printf("2. Contiguous Buffer\n");
    printf("%d. Cancel\n", DIAG_EXIT_MENU);
    if ((DIAG_INPUT_SUCCESS != DIAG_GetMenuOption(&tmp, 2)) ||
        (DIAG_EXIT_MENU == tmp))
    {
        return FALSE;
    }
    pDma->fSG = (1 == tmp);

    printf("\n");
    printf("Select DMA completion method:\n");
    printf("1. Interrupts\n");
    printf("2. Polling\n");
    printf("%d. Cancel\n", DIAG_EXIT_MENU);
    if ((DIAG_INPUT_SUCCESS != DIAG_GetMenuOption(&tmp, 2)) ||
        (DIAG_EXIT_MENU == tmp))
    {
        return FALSE;
    }
    *pfPolling = (2 == tmp);

    printf("\n");
    printf("Select DMA direction:\n");
    printf("1. from device\n");
    printf("2. to device\n");
    printf("%d. Cancel\n", DIAG_EXIT_MENU);
    if ((DIAG_INPUT_SUCCESS != DIAG_GetMenuOption(&tmp, 2)) ||
        (DIAG_EXIT_MENU == tmp))
    {
        return FALSE;
    }
    *pdwOptions = (1 == tmp) ? DMA_FROM_DEVICE : DMA_TO_DEVICE;

    printf("\n");
    if (DIAG_INPUT_SUCCESS != DIAG_InputUINT32(pu32LocalAddr,
        "Enter local DMA address", TRUE, 0, 0))
    {
        return FALSE;
    }

    printf("\n");
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(pdwBytes,
        "Enter size of DMA buffer, in bytes", TRUE, 0, 0))
    {
        return FALSE;
    }
    if (!*pdwBytes)
    {
        printf("Invalid input: Buffer size must be larger than 0\n");
        return FALSE;
    }
    printf("\n");

    return TRUE;
}

/* -----------------------------------------------
    Interrupt handling
   ----------------------------------------------- */
static DWORD MenuInterruptsEnableOptionCb(PVOID pCbCtx)
{
    MENU_CTX_INTERRUPTS *pInterruptsMenusCtx = (MENU_CTX_INTERRUPTS *)pCbCtx;
    DWORD dwStatus = PLX_IntEnable(*(pInterruptsMenusCtx->phDev),
        (PLX_INT_HANDLER)pInterruptsMenusCtx->funcIntHandler,
        pInterruptsMenusCtx->pData);

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Interrupts enabled\n");
        pInterruptsMenusCtx->fIntsEnabled = TRUE;
    }
    else
    {
        printf("Failed enabling interrupts. Error [0x%x - %s]\n",
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

static DWORD MenuInterruptsDisableOptionCb(PVOID pCbCtx)
{
    MENU_CTX_INTERRUPTS *pInterruptsMenusCtx = (MENU_CTX_INTERRUPTS *)pCbCtx;
    DWORD dwStatus = PLX_IntDisable(*(pInterruptsMenusCtx->phDev));

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Interrupts disabled\n");
        pInterruptsMenusCtx->fIntsEnabled = FALSE;
    }
    else
    {
        printf("Failed disabling interrupts: %s\n", PLX_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuInterruptsCb(PVOID pCbCtx)
{
    MENU_CTX_INTERRUPTS *pInterruptsMenusCtx = (MENU_CTX_INTERRUPTS *)pCbCtx;

    pInterruptsMenusCtx->fIntsEnabled = PLX_IntIsEnabled(
        *pInterruptsMenusCtx->phDev);

    return WD_STATUS_SUCCESS;
}

/* Enable/Disable interrupts menu */
void PLX_DIAG_MenuInterruptsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev, PLX_INT_HANDLER DiagIntHandler, PVOID pData)
{
    MENU_INTERRUPTS_CALLBACKS interruptsMenuCbs = { 0 };
    interruptsMenuCbs.interruptsMenuEntryCb = MenuInterruptsCb;
    interruptsMenuCbs.interruptsEnableCb = MenuInterruptsEnableOptionCb;
    interruptsMenuCbs.interruptsDisableCb = MenuInterruptsDisableOptionCb;

    static MENU_CTX_INTERRUPTS interruptsMenusCtx = { 0 };
    interruptsMenusCtx.phDev = phDev;
    interruptsMenusCtx.funcIntHandler = (DIAG_INT_HANDLER)DiagIntHandler;
    interruptsMenusCtx.pData = pData;

    MenuCommonInterruptsInit(pParentMenu, &interruptsMenusCtx,
        &interruptsMenuCbs);
}

/* ----------------------------------------------------
    Plug-and-play and power management events handling
   ---------------------------------------------------- */

static DWORD MenuEventsRegisterOptionCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    DWORD dwStatus = PLX_EventRegister(*(pEventsMenusCtx->phDev),
        (PLX_EVENT_HANDLER)pEventsMenusCtx->DiagEventHandler);

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Events registered\n");
        pEventsMenusCtx->fRegistered = TRUE;
    }
    else
    {
        printf("Failed to register events. Last error [%s]\n",
            PLX_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuEventsUnregisterOptionCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    DWORD dwStatus = PLX_EventUnregister(*(pEventsMenusCtx->phDev));

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Events unregistered\n");
        pEventsMenusCtx->fRegistered = FALSE;
    }
    else
    {
        printf("Failed to unregister events. Last error [%s]\n",
            PLX_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuEventsCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    pEventsMenusCtx->fRegistered = PLX_EventIsRegistered(
        *pEventsMenusCtx->phDev);

#ifdef WIN32
    if (!pEventsMenusCtx->fRegistered)
    {
        printf("NOTICE: An INF must be installed for your device in order to \n"
            "        call your user-mode event handler.\n"
            "        You can generate an INF file using the DriverWizard.");
    }
#endif

    return WD_STATUS_SUCCESS;
}

void PLX_DIAG_MenuEventsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev, PLX_EVENT_HANDLER DiagEventHandler)
{
    static MENU_EVENTS_CALLBACKS eventsMenuCbs = { 0 };
    static MENU_CTX_EVENTS eventsMenusCtx = { 0 };

    eventsMenuCbs.eventsMenuEntryCb = MenuEventsCb;
    eventsMenuCbs.eventsEnableCb = MenuEventsRegisterOptionCb;
    eventsMenuCbs.eventsDisableCb = MenuEventsUnregisterOptionCb;

    eventsMenusCtx.phDev = phDev;
    eventsMenusCtx.DiagEventHandler = (DIAG_EVENT_HANDLER)DiagEventHandler;

    MenuCommonEventsInit(pParentMenu, &eventsMenusCtx, &eventsMenuCbs);
}

/* -----------------------------------------------
    Access the serial EEPROM
   ----------------------------------------------- */
#define EEPROM_MAX_OFFSET 0xFF

static BOOL MenuIsDeviceNullOrNoEeprom(DIAG_MENU_OPTION *pMenu)
{
    PLX_MENU_EEPROM_CTX *pEepromCtx = (PLX_MENU_EEPROM_CTX *)pMenu->pCbCtx;

    return (*(pEepromCtx->phDev) == NULL) ||
        !PLX_EEPROMIsPresent(*(pEepromCtx->phDev));
}

static DWORD MenuEepromDisplayOptionCb(PVOID pCbCtx)
{
    DWORD dwStatus, dwOffset;
    PLX_MENU_EEPROM_CTX *pEepromCtx = (PLX_MENU_EEPROM_CTX *)pCbCtx;
    UINT32 u32Data;

    for (dwOffset = 0; dwOffset < EEPROM_MAX_OFFSET; dwOffset += 4)
    {
        if (!(dwOffset % 0x10))
            printf("\n %02X: ", dwOffset);
        dwStatus = pEepromCtx->fVPDSupported ?
            PLX_EEPROM_VPD_Read32(*(pEepromCtx->phDev), dwOffset, &u32Data) :
            PLX_EEPROM_RT_Read32(*(pEepromCtx->phDev), dwOffset, &u32Data,
                pEepromCtx->EEPROMmsb);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            printf("\nError occurred while reading offset 0x%x of the "
                "serial EEPROM.\nError 0x%x - %s\n", dwOffset,
                dwStatus, Stat2Str(dwStatus));
            break;
        }
        printf("%08X  ", u32Data);
    }
    printf("\n");

    return dwStatus;
}

static DWORD MenuEepromReadOptionCb(PVOID pCbCtx)
{
    DWORD dwStatus, dwOffset;
    PLX_MENU_EEPROM_CTX *pEepromCtx = (PLX_MENU_EEPROM_CTX *)pCbCtx;
    UINT32 u32Data;
    WORD wData;

    sprintf(gsInput, "Enter offset to read from (must be a multiple of "
        "%d)", pEepromCtx->fVPDSupported ? WDC_SIZE_32 : WDC_SIZE_16);
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(&dwOffset, gsInput, TRUE,
        0, EEPROM_MAX_OFFSET))
    {
        return WD_INVALID_PARAMETER;
    }

    dwStatus = pEepromCtx->fVPDSupported ? PLX_EEPROM_VPD_Read32(
        *(pEepromCtx->phDev), dwOffset,
        &u32Data) : PLX_EEPROM_RT_Read16(*(pEepromCtx->phDev), dwOffset,
            &wData, pEepromCtx->EEPROMmsb);

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Read 0x%X from offset 0x%x of the serial EEPROM\n",
            pEepromCtx->fVPDSupported ? u32Data : (UINT32)wData, dwOffset);
    }
    else
    {
        printf("Failed reading from offset 0x%x of the serial "
            "EEPROM.\n", dwOffset);
    }

    return dwStatus;
}

static DWORD MenuEepromWriteOptionCb(PVOID pCbCtx)
{
    DWORD dwStatus, dwOffset;
    PLX_MENU_EEPROM_CTX *pEepromCtx = (PLX_MENU_EEPROM_CTX *)pCbCtx;
    UINT32 u32Data;
    WORD wData;

    sprintf(gsInput, "Enter offset to write to (must be a multiple of "
        "%d)", pEepromCtx->fVPDSupported ? WDC_SIZE_32 : WDC_SIZE_16);
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(&dwOffset, gsInput, TRUE,
        0, EEPROM_MAX_OFFSET))
    {
        return WD_INVALID_PARAMETER;
    }

    if (DIAG_INPUT_SUCCESS != DIAG_InputNum(pEepromCtx->fVPDSupported ?
        (PVOID)&u32Data : (PVOID)&wData, "Enter data to write", TRUE,
        pEepromCtx->fVPDSupported ? sizeof(u32Data) : sizeof(wData), 0, 0))
    {
        return WD_INVALID_PARAMETER;
    }

    dwStatus = pEepromCtx->fVPDSupported ?
        PLX_EEPROM_VPD_Write32(*(pEepromCtx->phDev), dwOffset, u32Data) :
        PLX_EEPROM_RT_Write16(*(pEepromCtx->phDev), dwOffset, wData,
            pEepromCtx->EEPROMmsb);

    printf("%s 0x%X to offset 0x%x of the serial EEPROM\n",
        (WD_STATUS_SUCCESS == dwStatus) ? "Wrote" : "Failed to write",
        pEepromCtx->fVPDSupported ? u32Data : (UINT32)wData, dwOffset);

    return WD_STATUS_SUCCESS;
}

static void PLX_DIAG_MenuEEPROMSubMenusInit(DIAG_MENU_OPTION *pParentMenu,
    PLX_MENU_EEPROM_CTX *pEepromCtx)
{
    static DIAG_MENU_OPTION displayEepromMenu = { 0 };
    strcpy(displayEepromMenu.cOptionName, "Display EEPROM content");
    displayEepromMenu.cbEntry = MenuEepromDisplayOptionCb;

    static DIAG_MENU_OPTION readEepromMenu = { 0 };
    strcpy(readEepromMenu.cOptionName, "Read from the serial EEPROM");
    readEepromMenu.cbEntry = MenuEepromReadOptionCb;

    static DIAG_MENU_OPTION writeEepromMenu = { 0 };
    strcpy(writeEepromMenu.cOptionName, "Write to the serial EEPROM");
    writeEepromMenu.cbEntry = MenuEepromWriteOptionCb;

    static DIAG_MENU_OPTION options[3] = { 0 };
    options[0] = displayEepromMenu;
    options[1] = readEepromMenu;
    options[2] = writeEepromMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        pEepromCtx, pParentMenu);
}

static DWORD MenuEEPROMCb(PVOID pCbCtx)
{
    PLX_MENU_EEPROM_CTX *pEepromCtx = (PLX_MENU_EEPROM_CTX *)pCbCtx;

    pEepromCtx->fVPDSupported = PLX_EEPROM_VPD_Validate(*(pEepromCtx->phDev));

    if (pEepromCtx->fVPDSupported)
    {
        printf("NOTE: EEPROM data is accessed via Vital Product Data (VPD) "
            "as DWORDs\n");
    }
    else
    {
        printf("NOTE: EEPROM data is accessed via run-time (RT) registers "
            "as WORDs\n");
    }

    return WD_STATUS_SUCCESS;
}

void PLX_DIAG_MenuEEPROMInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev, DWORD EEPROMmsb)
{
    static DIAG_MENU_OPTION eepromMenuRoot = { 0 };
    static PLX_MENU_EEPROM_CTX eepromCtx = { 0 };

    strcpy(eepromMenuRoot.cOptionName, "Access the serial EEPROM on the "
        "board");
    strcpy(eepromMenuRoot.cTitleName, "Access the board's serial EEPROM");
    eepromMenuRoot.cbEntry = MenuEEPROMCb;
    eepromMenuRoot.cbIsHidden = MenuIsDeviceNullOrNoEeprom;

    eepromCtx.phDev = phDev;
    eepromCtx.EEPROMmsb = EEPROMmsb;

    PLX_DIAG_MenuEEPROMSubMenusInit(&eepromMenuRoot, &eepromCtx);

    DIAG_MenuSetCtxAndParentForMenus(&eepromMenuRoot, 1, &eepromCtx,
        pParentMenu);
}

/* -----------------------------------------------
    Reset board
   ----------------------------------------------- */

static DWORD MenuResetBoardCb(PVOID pCbCtx)
{
    printf("\n");
    printf("Performing soft reset ...\n");
    PLX_SoftResetMaster(*((WDC_DEVICE_HANDLE *)pCbCtx));

    return WD_STATUS_SUCCESS;
}

/* NOTE: Currently supported for master devices only (PLX 9054, 9056, 9080,
 * 9656) */
void PLX_DIAG_MenuResetBoardInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static DIAG_MENU_OPTION resetBoardMenu = { 0 };

    strcpy(resetBoardMenu.cTitleName, "Reset Board");
    strcpy(resetBoardMenu.cOptionName, "Reset board");
    resetBoardMenu.cbIsHidden = MenuCommonIsDeviceNull;
    resetBoardMenu.cbEntry = MenuResetBoardCb;

    DIAG_MenuSetCtxAndParentForMenus(&resetBoardMenu, 1,
        phDev, pParentMenu);
}

