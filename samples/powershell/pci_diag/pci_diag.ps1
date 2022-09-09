<# Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com #>

#***********************************************************************
# File: pci_diag.c
#
# Sample user-mode diagnostics application for accessing PCI
# devices, possibly via a Kernel PlugIn driver using WinDriver's API.
#
#
# Note: This code sample is provided AS-IS and as a guiding sample only.
#***********************************************************************
using namespace Jungo.wdapi_dotnet
using namespace System
using namespace System.Runtime.InteropServices;

using module .\diag_lib.psm1
using module .\pci_lib.psm1
using module .\wdc_diag_lib.psm1

$WD_PS_PATH = Split-Path $MyInvocation.MyCommand.Path -Parent
$WD_PROD_NAME = "WinDriver"


# --------------------------------------------------
#    PCI configuration registers information
# --------------------------------------------------
# Configuration registers information array

class PCI_ADDR_SPACE_INFO {
    [uint32]$dwAddrSpace
    [string]$sType
    [string]$sName
    [string]$sDesc
}

Class WDC_REG
{
    [uint32]$dwAddrSpace      # Number of address space in which the register
                              # resides
                              # For PCI configuration registers, use
                              # WDC_AD_CFG_SPACE
    [uint32]$dwOffset         # Offset of the register in the dwAddrSpace
                              # address space
    [uint32]$dwSize           # Register's size (in bytes)
    [int]$Direction # Read/write access mode - see WDC_DIRECTION
                              # options
    [String]$sName            # Register's name
    [String]$sDescription     # Register's description

    WDC_REG([uint32]$dwAddrSpace, [uint32]$dwOffset, [uint32]$dwSize,
        [uint32]$direction, [String]$sName, [String]$sDesc)
    {
        $this.dwAddrSpace = $dwAddrSpace
        $this.dwOffset = $dwOffset
        $this.dwSize = $dwSize
        $this.Direction = $direction
        $this.sName = $sName
        $this.sDescription = $sDesc
    }
}
$gPCI_CfgRegs = @(0) * 26
$gPCI_CfgRegs[0] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_VID, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "VID", "Vendor ID" )
$gPCI_CfgRegs[1] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_DID, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "DID", "Device ID" )
$gPCI_CfgRegs[2] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_CR, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "CMD", "Command" )
$gPCI_CfgRegs[3] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_SR, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "STS", "Status" )
$gPCI_CfgRegs[4] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_REV, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "RID_CLCD",
     "Revision ID & Class Code" )
$gPCI_CfgRegs[5] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_CCSC, [wdc_lib_consts]::WDC_SIZE_8,
     [WDC_DIRECTION]::WDC_READ_WRITE, "SCC", "Sub Class Code" )
$gPCI_CfgRegs[6] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_CCBC, [wdc_lib_consts]::WDC_SIZE_8,
     [WDC_DIRECTION]::WDC_READ_WRITE, "BCC", "Base Class Code" )
$gPCI_CfgRegs[7] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_CLSR, [wdc_lib_consts]::WDC_SIZE_8,
     [WDC_DIRECTION]::WDC_READ_WRITE, "CALN", "Cache Line Size" )
$gPCI_CfgRegs[8] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_LTR, [wdc_lib_consts]::WDC_SIZE_8,
     [WDC_DIRECTION]::WDC_READ_WRITE, "LAT", "Latency Timer" )
$gPCI_CfgRegs[9] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_HDR, [wdc_lib_consts]::WDC_SIZE_8,
     [WDC_DIRECTION]::WDC_READ_WRITE, "HDR", "Header Type" )
$gPCI_CfgRegs[10] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_BISTR, [wdc_lib_consts]::WDC_SIZE_8,
     [WDC_DIRECTION]::WDC_READ_WRITE, "BIST", "Built-in Self Test" )
$gPCI_CfgRegs[11] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_BAR0, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "BADDR0", "Base Address 0" )
$gPCI_CfgRegs[12] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_BAR1, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "BADDR1", "Base Address 1" )
$gPCI_CfgRegs[13] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_BAR2, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "BADDR2", "Base Address 2" )
$gPCI_CfgRegs[14] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_BAR3, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "BADDR3", "Base Address 3" )
$gPCI_CfgRegs[15] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_BAR4, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "BADDR4", "Base Address 4" )
$gPCI_CfgRegs[16] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_BAR5, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "BADDR5", "Base Address 5" )
$gPCI_CfgRegs[17] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_CIS, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "CIS", "CardBus CISpointer" )
$gPCI_CfgRegs[18] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_SVID, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "SVID", "Sub-system Vendor ID" )
$gPCI_CfgRegs[19] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_SDID, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "SDID", "Sub-system Device ID" )
$gPCI_CfgRegs[20] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_EROM, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "EROM", "Expansion ROM Base Address" )
$gPCI_CfgRegs[21] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_CAP, [wdc_lib_consts]::WDC_SIZE_8,
     [WDC_DIRECTION]::WDC_READ_WRITE, "NEW_CAP", "New Capabilities Pointer" )
$gPCI_CfgRegs[22] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_ILR, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "INTLN", "Interrupt Line" )
$gPCI_CfgRegs[23] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_IPR, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "INTPIN", "Interrupt Pin" )
$gPCI_CfgRegs[24] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_MGR, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "MINGNT", "Minimum Required Burst Period" )
$gPCI_CfgRegs[25] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG]::PCI_MLR, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "MAXLAT", "Maximum Latency" )

$gPCI_ext_CfgRegs = @(0) * 24
$gPCI_ext_CfgRegs[0] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::PCIE_CAP_ID, [wdc_lib_consts]::WDC_SIZE_8,
     [WDC_DIRECTION]::WDC_READ_WRITE, "PCIE_CAP_ID", "PCI Express Capability ID" )
$gPCI_ext_CfgRegs[1] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::NEXT_CAP_PTR, [wdc_lib_consts]::WDC_SIZE_8,
     [WDC_DIRECTION]::WDC_READ_WRITE, "NEXT_CAP_PTR", "Next Capabiliy Pointer" )
$gPCI_ext_CfgRegs[2] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::CAP_REG, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "CAP_REG", "Capabilities Register" )
$gPCI_ext_CfgRegs[3] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::DEV_CAPS, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "DEV_CAPS", "Device Capabilities" )
$gPCI_ext_CfgRegs[4] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::DEV_CTL, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "DEV_CTL", "Device Control" )
$gPCI_ext_CfgRegs[5] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::DEV_STS, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "DEV_STS", "Device Status" )
$gPCI_ext_CfgRegs[6] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::LNK_CAPS, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "LNK_CAPS", "Link Capabilities" )
$gPCI_ext_CfgRegs[7] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::LNK_CTL, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "LNK_CTL", "Link Control" )
$gPCI_ext_CfgRegs[8] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::LNK_STS, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "LNK_STS", "Link Status" )
$gPCI_ext_CfgRegs[9] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::SLOT_CAPS, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "SLOT_CAPS", "Slot Capabilities" )
$gPCI_ext_CfgRegs[10] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::SLOT_CTL, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "SLOT_CTL", "Slot Control" )
$gPCI_ext_CfgRegs[11] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::SLOT_STS, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "SLOT_STS", "Slot Status" )
$gPCI_ext_CfgRegs[12] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::ROOT_CAPS, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "ROOT_CAPS", "Root Capabilities" )
$gPCI_ext_CfgRegs[13] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::ROOT_CTL, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "ROOT_CTL", "Root Control" )
$gPCI_ext_CfgRegs[14] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::ROOT_STS, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "ROOT_STS", "Root Status" )
$gPCI_ext_CfgRegs[15] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::DEV_CAPS2, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "DEV_CAPS2", "Device Capabilities 2" )
$gPCI_ext_CfgRegs[16] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::DEV_CTL2, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "DEV_CTL2", "Device Control 2" )
$gPCI_ext_CfgRegs[17] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::DEV_STS2, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "DEV_STS2", "Device Status 2" )
$gPCI_ext_CfgRegs[18] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::LNK_CAPS2, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "LNK_CAPS2", "Link Capabilities 2" )
$gPCI_ext_CfgRegs[19] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::LNK_CTL2, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "LNK_CTL2", "Link Control 2" )
$gPCI_ext_CfgRegs[20] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::LNK_STS2, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "LNK_STS2", "Link Status 2" )
$gPCI_ext_CfgRegs[21] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::SLOT_CAPS2, [wdc_lib_consts]::WDC_SIZE_32,
     [WDC_DIRECTION]::WDC_READ_WRITE, "SLOT_CAPS2", "Slot Capabilities 2" )
$gPCI_ext_CfgRegs[22] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::SLOT_CTL2, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "SLOT_CTL2", "Slot Control 2" )
$gPCI_ext_CfgRegs[23] = [WDC_REG]::new( [wdc_lib_consts]::WDC_AD_CFG_SPACE,
     [PCI_CFG_REG_EXPRESS]::SLOT_STS2, [wdc_lib_consts]::WDC_SIZE_16,
     [WDC_DIRECTION]::WDC_READ_WRITE, "SLOT_STS2", "Slot Status 2" )

# 
#************************************************************
#  Functions implementation
#***********************************************************

# -----------------------------------------------
#    Main diagnostics menu
# -----------------------------------------------
# Main menu options
enum MENU_MAIN{
    SCAN_BUS = 1
    FIND_AND_OPEN
    SHARED_BUFFER
    IPC
    RW_CFG_SPACE
    EVENTS
    RW_ADDR
    ENABLE_DISABLE_INT
    ALLOC_FREE_DMA
    # 
    EXIT = 99
}

# -----------------------------------------------
#    Device find, open and close
# -----------------------------------------------
# Find and open a PCI device

# Find a PCI device
function DeviceFind($dwVendorId, $dwDeviceId, [ref]$pSlot)
{
    # Scan PCI devices
    $scanResult = New-Object -TypeName "Jungo.wdapi_dotnet.WDC_PCI_SCAN_RESULT"
    $dwStatus = [wdc_lib_decl]::WDC_PciScanDevices($dwVendorId, $dwDeviceId,
        [ref]$scanResult)

    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
    {
        PCI_ERR("DeviceFind: Failed scanning the PCI bus." +
            "Error [0x{0:x} - {1}]" -f $dwStatus, [utils]::Stat2Str($dwStatus))
        return $FALSE
    }

    $dwNumDevices = $scanResult.dwNumDevices
    if (!$dwNumDevices)
    {
        Write-Host "No matching PCI device was found for search criteria " +
            "(Vendor ID 0x{0:X}, Device ID 0x{1:X})" -f $dwVendorId, $dwDeviceId

        return $FALSE
    }

    # Display matching devices information
    Write-Host (("Found {0} matching device{1} " +
        "[ Vendor ID 0x{2:X}{3}, Device ID 0x{4:X}{5} ]:") -f
        $dwNumDevices, $(If ($dwNumDevices -gt 1) {"s"} Else {""}),
        $dwVendorId, $(If ($dwVendorId) {""} Else {" (ALL)"}),
        $dwDeviceId, $(If ($dwDeviceId) {""} Else {" (ALL)"}))

    for ($i = 0; $i -lt $dwNumDevices; $i++)
    {
        Write-Host("")
        Write-Host (("{0}. Vendor ID: 0x{1:X}, Device ID: 0x{2:X}") -f ($i + 1),
            $scanResult.deviceId[$i].dwVendorId,
            $scanResult.deviceId[$i].dwDeviceId)

        WDC_DIAG_PciDeviceInfoPrint $scanResult.deviceSlot[$i] $FALSE
    }
    Write-Host("")

    # Select device
    if ($dwNumDevices -gt 1)
    {
        Write-Host("Select a device (1 - {0}): " -f $dwNumDevices)
        $i = 0

        if ([DIAG_INPUT_RESULT]::DIAG_INPUT_SUCCESS -ne
            (DIAG_GetMenuOption ([ref]$i) $dwNumDevices))
        {
            return $FALSE
        }
    }

    $pSlot.value = $scanResult.deviceSlot[$i - 1]

    return $TRUE
}

# 
<#
function CheckKPDriverVer($hDev)
{
    [KP_PCI_VERSION]$kpVer
    $dwKPResult = 0

    # Get Kernel PlugIn Driver version
    $dwStatus = [wdc_lib_decl]::WDC_CallKerPlug($hDev, $KP_PCI_MSG_VERSION,
        [ref]$kpVer, [ref]$dwKPResult)
    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
    {
        PCI_ERR(("Failed sending a \'Get Version\' message [0x{0:x}] to the " +
            "Kernel-PlugIn driver [{1}]. Error [0x{2:x} - {3}]" ) -f
            $KP_PCI_MSG_VERSION, $KP_PCI_DRIVER_NAME, $dwStatus,
            [utils]::Stat2Str($dwStatus))
    }
    elseif ($KP_PCI_STATUS_OK -ne $dwKPResult)
    {
        PCI_ERR(("Kernel-PlugIn \'Get Version\' message [0x{0:x}] failed. " +
            "Kernel PlugIn status [0x{1:x}]" ) -f $KP_PCI_MSG_VERSION, $dwKPResult)
        $dwStatus = [int][WD_ERROR_CODES]::WD_INCORRECT_VERSION
    }
    else
    {
        Write-Host("Using [{0}] Kernel-Plugin driver version [{1}.{2} - {3}]" -f
            $KP_PCI_DRIVER_NAME, $kpVer.dwVer / 100, $kpVer.dwVer % 100,
            $kpVer.cVer)
    }

    return $dwStatus
} #>
# 

# Open a handle to a PCI device
function DeviceOpen($pSlot)
{
    # Retrieve the device's resources information
    $deviceInfo = New-Object -TypeName "Jungo.wdapi_dotnet.WD_PCI_CARD_INFO"
    $deviceInfo.pciSlot = $pSlot
    $dwStatus = [wdc_lib_decl]::WDC_PciGetDeviceInfo([ref]$deviceInfo)
    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
    {
        PCI_ERR("DeviceOpen: Failed retrieving the device's resources " +
            "information. Error [0x{0:x} - {1}]" -f $dwStatus,
            [utils]::Stat2Str($dwStatus))
        return $NULL
    }

    # NOTE: If necessary, you can modify the device's resources information
    #   here - mainly the information stored in the deviceInfo.Card.Items array,
    #   and the number of array items stored in deviceInfo.Card.dwItems.
    #   For example:
    #   - Edit the deviceInfo.Card.Items array and/or deviceInfo.Card.dwItems,
    #     to register only some of the resources or to register only a portion
    #     of a specific address space.
    #   - Set the fNotSharable field of one or more items in the
    #     deviceInfo.Card.Items array to 1, to block sharing of the related
    #     resources and ensure that they are locked for exclusive use.

    # Open a handle to the device
    $hDev = PCI_DeviceOpen $deviceInfo
    if (!$hDev)
    {
        PCI_ERR("DeviceOpen: Failed opening a handle to the device: " +
            [utils]::Stat2Str($dwStatus))
        return $NULL
    }

# 
    # Get Kernel PlugIn driver version
    if ([wdc_defs_macros]::WDC_IS_KP($hDev))
    {
        CheckKPDriverVer $hDev
    }
# 

    return $hDev
}

function DeviceFindAndOpen($dwVendorId, $dwDeviceId)
{
    $slot = New-Object -TypeName "Jungo.wdapi_dotnet.WD_PCI_SLOT"
    # Find device
    if (!(DeviceFind $dwVendorId $dwDeviceId ([ref]$slot)))
    {
        return $NULL
    }

    # Open a device handle
    return DeviceOpen $slot
}

# Close handle to a PCI device
function DeviceClose($Dev)
{
    # Validate the WDC device handle
    if ($hDev -eq [System.IntPtr]::Zero)
    {
        return
    }
    # Close the WDC device handle
    if (!(PCI_DeviceClose $Dev))
    {
        PCI_ERR("DeviceClose: Failed closing PCI device: " + [utils]::Stat2Str($dwStatus))
    }
}

# -----------------------------------------------
#   Read/write memory and I/O addresses
#   -----------------------------------------------
# Read/write address menu options
enum MENU_RW_ADDR
{
    SET_ADDR_SPACE = 1
    SET_MODE
    SET_TRANS_TYPE
    READ
    WRITE
    EXIT = 99
}

$ACTIVE_ADDR_SPACE_NEEDS_INIT = 0xFF

# Read/write memory or I/O space address menu
function MenuReadWriteAddr($Dev)
{
    $hDev = $Dev.hDev
    $option = 0
    $dwAddrSpace = $ACTIVE_ADDR_SPACE_NEEDS_INIT
    [WDC_ADDR_MODE]$mode = [WDC_ADDR_MODE]::WDC_MODE_32
    $fBlock = $FALSE

    # Initialize active address space
    if ($ACTIVE_ADDR_SPACE_NEEDS_INIT -eq $dwAddrSpace)
    {
        $dwNumAddrSpaces = $Dev.dwNumAddrSpaces
        # Find the first active address space
        for ($dwAddrSpace = 0; $dwAddrSpace -lt $dwNumAddrSpaces; $dwAddrSpace++)
        {
            if ([wdc_lib_decl]::WDC_AddrSpaceIsActive($hDev, $dwAddrSpace))
            {
                break
            }
        }

        # Sanity check
        if ($dwAddrSpace -eq $dwNumAddrSpaces)
        {
            PCI_ERR("MenuReadWriteAddr: Error - No active address spaces " +
                "found")
            $dwAddrSpace = $ACTIVE_ADDR_SPACE_NEEDS_INIT
            return
        }
    }

    do
    {
        Write-Host("")
        Write-Host("Read/write the device's memory and I/O ranges")
        Write-Host("----------------------------------------------")
        Write-Host("{0}. Change active address space for read/write " -f
            [int][MENU_RW_ADDR]::SET_ADDR_SPACE)

        Write-Host("(currently: BAR {0})" -f $dwAddrSpace)
        Write-Host("{0}. Change active read/write mode (currently: {1})" -f
            [int][MENU_RW_ADDR]::SET_MODE,
            $(if ([WDC_ADDR_MODE]::WDC_MODE_8 -eq $mode) { "8 bit" }
                Else {
                $(if([WDC_ADDR_MODE]::WDC_MODE_16 -eq $mode) { "16 bit" }
                    Else {
                    $(if([WDC_ADDR_MODE]::WDC_MODE_32 -eq $mode) { "32 bit" }
                    Else {
                        "64 bit"
                    })
                })
            })
        )
        Write-Host("{0}. Toggle active transfer type (currently: {1})" -f
            [int][MENU_RW_ADDR]::SET_TRANS_TYPE,
            $(if ($fBlock) { "block transfers" } Else { "non-block transfers" }))
        Write-Host("{0}. Read from active address space" -f [int][MENU_RW_ADDR]::READ)
        Write-Host("{0}. Write to active address space" -f [int][MENU_RW_ADDR]::WRITE)
        Write-Host("{0}. Exit menu" -f [int][MENU_RW_ADDR]::EXIT)
        Write-Host("")

        if ([DIAG_INPUT_RESULT]::DIAG_INPUT_FAIL -eq (DIAG_GetMenuOption `
            ([ref]$option) ([int][MENU_RW_ADDR]::WRITE)) `
             -or !$option)
        {
            continue
        }

        switch ([MENU_RW_ADDR]$option)
        {
        EXIT { # Exit menu
        }
        SET_ADDR_SPACE { # Set active address space for
                         #  read/write address requests
            SetAddrSpace $Dev ([ref]$dwAddrSpace)
        }
        SET_MODE { # Set active $mode for read/write address
                   #                    requests
            WDC_DIAG_SetMode ([ref]$mode)
        }
        SET_TRANS_TYPE { # Toggle active transfer type
             $fBlock = -not $fBlock
        }
        READ {  # Read from a memory or I/O address
            if ($fBlock)
            {
                WDC_DIAG_ReadWriteBlock $hDev ([WDC_DIRECTION]::WDC_READ) $dwAddrSpace
            }
            else
            {
                WDC_DIAG_ReadWriteAddr $hDev ([WDC_DIRECTION]::WDC_READ) $dwAddrSpace $mode
            }
        }
        WRITE { # Write to a memory or I/O address
            if ($fBlock)
            {
                WDC_DIAG_ReadWriteBlock $hDev ([WDC_DIRECTION]::WDC_WRITE) `
                    $dwAddrSpace
            }
            else
            {
                WDC_DIAG_ReadWriteAddr $hDev ([WDC_DIRECTION]::WDC_WRITE) `
                    $dwAddrSpace $mode
            }
        }
    }
    } while ([int][MENU_RW_ADDR]::EXIT -ne $option)
}

# Set address space
function SetAddrSpace($Dev, [ref]$pdwAddrSpace)
{
    $hDev = $Dev.hDev

    $dwNumAddrSpaces = $Dev.dwNumAddrSpaces

    [PCI_ADDR_SPACE_INFO]$addrSpaceInfo = New-Object -TypeName "PCI_ADDR_SPACE_INFO"

    Write-Host("")
    Write-Host("Select an active address space:")
    Write-Host("-------------------------------")

    for ($dwAddrSpace = 0; $dwAddrSpace -lt $dwNumAddrSpaces; $dwAddrSpace++)
    {
        $addrSpaceInfo.dwAddrSpace = $dwAddrSpace
        if (!(PCI_GetAddrSpaceInfo $Dev ([ref]$addrSpaceInfo)))
        {
            PCI_ERR("SetAddrSpace: Error - Failed to get address space " +
                "information. Last error [{0}]" -f ([utils]::Stat2Str($dwStatus)))
            return
        }

        Write-Host("{0}. {1:22} {2:10} {3}" -f ($dwAddrSpace + 1),
            $addrSpaceInfo.sName, $addrSpaceInfo.sType,
            $addrSpaceInfo.sDesc)
    }
    Write-Host("")

    if ([DIAG_INPUT_RESULT]::DIAG_INPUT_SUCCESS -ne (DIAG_InputNum ([ref]$dwAddrSpace) `
        "Enter option" $FALSE 1 $dwNumAddrSpaces ))
    {
        return
    }

    $dwAddrSpace--
    if (![wdc_lib_decl]::WDC_AddrSpaceIsActive($hDev, $dwAddrSpace))
    {
        Write-Host("You have selected an inactive address space")
        return
    }

    $pdwAddrSpace.value = $dwAddrSpace
}

# -----------------------------------------------
#    Read/write the configuration space
# -----------------------------------------------
# Read/write the configuration space menu options
enum MENU_RW_CFG_SPACE {
    READ_OFFSET = 1
    WRITE_OFFSET
    READ_ALL_REGS
    READ_REG
    WRITE_REG
    EXT_READ_REG
    EXT_WRITE_REG
    SCAN_CAP
    EXIT = 99
}

# Read/write configuration space menu
function MenuReadWriteCfgSpace($hDev)
{
    $option = 0
    $fExpress = [wdc_lib_decl]::WDC_PciGetExpressGen($hDev) -ne 0
    do {
        # Display predefined registers information
        Write-Host("")
        Write-Host("Configuration registers:")
        Write-Host("------------------------")

        WDC_DIAG_RegsInfoPrint $hDev $gPCI_CfgRegs ($gPCI_CfgRegs.Length) `
            ([WDC_DIAG_REG_PRINT]::ALL) $FALSE

        if ($fExpress)
        {
            WDC_DIAG_RegsInfoPrint $hDev $gPCI_ext_CfgRegs ($gPCI_ext_CfgRegs.Length) `
                ([WDC_DIAG_REG_PRINT]::ALL) $TRUE
        }
        Write-Host ""
        Write-Host "Read/write the device's configuration space"
        Write-Host "--------------------------------------------"
        Write-Host("{0}. Read from an offset" -f ([int][MENU_RW_CFG_SPACE]::READ_OFFSET))
        Write-Host("{0}. Write to an offset" -f ([int][MENU_RW_CFG_SPACE]::WRITE_OFFSET))
        Write-Host("{0}. Read all configuration registers defined for the device (see list above)" -f ([int][MENU_RW_CFG_SPACE]::READ_ALL_REGS))
        Write-Host("{0}. Read from a named register" -f [int][MENU_RW_CFG_SPACE]::READ_REG)
        Write-Host("{0}. Write to a named register" -f [int][MENU_RW_CFG_SPACE]::WRITE_REG)
        if ($fExpress)
        {
            Write-Host("{0}. Read from a named PCI Express register" -f
                [int][MENU_RW_CFG_SPACE]::EXT_READ_REG)
            Write-Host("{0}. Write to a named PCI Express register" -f
                [int][MENU_RW_CFG_SPACE]::EXT_WRITE_REG)
        }
        Write-Host("{0}. Scan PCI/PCIe capabilities" -f [int][MENU_RW_CFG_SPACE]::SCAN_CAP)
        Write-Host("{0}. Exit menu" -f [int][MENU_RW_CFG_SPACE]::EXIT)
        Write-Host("")

        if ([DIAG_INPUT_RESULT]::DIAG_INPUT_FAIL -eq (DIAG_GetMenuOption ([ref]$option) `
            ([int][MENU_RW_CFG_SPACE]::SCAN_CAP)) -or !$option)
        {
            continue
        }

        switch ([MENU_RW_CFG_SPACE]$option)
        {
        EXIT { # Exit menu
        }
        READ_OFFSET { # Read from a configuration space offset
            WDC_DIAG_ReadWriteBlock $hDev ([WDC_DIRECTION]::WDC_READ) `
                ([wdc_lib_consts]::[wdc_lib_consts]::WDC_AD_CFG_SPACE)
        }
        WRITE_OFFSET { # Write to a configuration space offset
            WDC_DIAG_ReadWriteBlock $hDev ([WDC_DIRECTION]::WDC_WRITE) `
                ([wdc_lib_consts]::[wdc_lib_consts]::WDC_AD_CFG_SPACE)
        }
        READ_ALL_REGS {
            WDC_DIAG_ReadRegsAll $hDev $gPCI_CfgRegs ($gPCI_CfgRegs.Length) `
                $TRUE $FALSE
            if ($fExpress)
            {
                WDC_DIAG_ReadRegsAll $hDev $gPCI_ext_CfgRegs `
                    ($gPCI_ext_CfgRegs.Length) $TRUE $TRUE
            }
        }
        READ_REG {  # Read from a configuration register
            WDC_DIAG_ReadWriteReg $hDev $gPCI_CfgRegs ($gPCI_CfgRegs.Length) `
                ([WDC_DIRECTION]::WDC_READ) $TRUE $FALSE
        }
        WRITE_REG { # Write to a configuration register
            WDC_DIAG_ReadWriteReg $hDev $gPCI_CfgRegs ($gPCI_CfgRegs.Length)`
                ([WDC_DIRECTION]::WDC_WRITE) $TRUE $FALSE
        }
        EXT_WRITE_REG { # Write to a configuration PCI Express register
            WDC_DIAG_ReadWriteReg $hDev $gPCI_ext_CfgRegs `
                ($gPCI_ext_CfgRegs.Length) ([WDC_DIRECTION]::WDC_WRITE) $TRUE `
                $TRUE
        }
        EXT_READ_REG { # Read from a configuration PCI Express register
            WDC_DIAG_ReadWriteReg $hDev $gPCI_ext_CfgRegs `
                ($gPCI_ext_CfgRegs.Length) ([WDC_DIRECTION]::WDC_READ) $TRUE `
                $TRUE
        }
        SCAN_CAP { # Scan PCI/PCIe capabilities
            WDC_DIAG_ScanPCICapabilities $hDev
        }
        }
    } while ([int][MENU_RW_CFG_SPACE]::EXIT -ne $option)
}

<# -----------------------------------------------
    Read/write the run-time registers
   -----------------------------------------------#>
# Read/write the run-time registers menu options
enum MENU_RW_REGS {
    READ_ALL = 1
    READ_REG
    WRITE_REG
    EXIT = 99
}
#  #>
#ifdef HAS_INTS

# -----------------------------------------------
#    DMA memory handling
# -----------------------------------------------
# DMA menu options
enum MENU_DMA {
    ALLOCATE_CONTIG = 1
    ALLOCATE_SG
    RESERVED_MEM
#    SHARE_CONTIG_BUF
    FREE_MEM
    EXIT = 99
}

function FreeDmaMem([ref]$ppBuf, [ref]$ppDma)
{
    [WD_DMA]$wdDma = New-Object "WD_DMA"

    if ($ppDma.value -eq [IntPtr]::Zero)
    {
        return
    }

    $wdDma = [Marshal]::PtrToStructure($ppDma.value,
        [System.Type]$wdDma.GetType())

    $fIsSG = !($wdDma.dwOptions -band [WD_DMA_OPTIONS]::DMA_KERNEL_BUFFER_ALLOC)

    $dwStatus = [wdc_lib_decl]::WDC_DMABufUnlock($ppDma.value)
    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -eq $dwStatus)
    {
        Write-Host("DMA memory freed")
    }
    else
    {
        PCI_ERR("Failed trying to free DMA memory. Error [0x{0:x} - {1}]" -f
            $dwStatus, [utils]::Stat2Str($dwStatus))
    }

    if ($fIsSG)
    {
        [System.Runtime.InteropServices.Marshal]::FreeHGlobal($ppBuf.value)
    }
    $ppBuf.value = [IntPtr]::Zero
    $ppDma.value = [IntPtr]::Zero
}

# Allocate/free DMA memory menu
function MenuDma($Dev)
{
    $hDev = $Dev.hDev
    $option = $size = $dwStatus = $dwOptions = 0
    [UINT64] $qwAddr = 0
    [IntPtr]$pBuf = New-Object "System.IntPtr"
    [IntPtr]$pDma = New-Object "System.IntPtr"
    [WD_DMA]$wdDma = New-Object "WD_DMA"

    $dwDmaAddressWidth = 0

    do
    {
        Write-Host("")
        Write-Host("DMA memory")
        Write-Host("-----------")
        Write-Host("{0}. Allocate contiguous memory" -f [int][MENU_DMA]::ALLOCATE_CONTIG)
        Write-Host("{0}. Allocate scatter-gather memory" -f [int][MENU_DMA]::ALLOCATE_SG)
        Write-Host("{0}. Use reserved memory" -f [int][MENU_DMA]::RESERVED_MEM)
<#
        if ($pDma -and $pDma.dwOptions -band [WD_DMA_OPTIONS]::DMA_KERNEL_BUFFER_ALLOC)
        {
            Write-Host("{0}. Send buffer through IPC to all group processes" -f
                [int][MENU_DMA]::SHARE_CONTIG_BUF)
        }
#>
        Write-Host("{0}. Free DMA memory" -f [int][MENU_DMA]::FREE_MEM)
        Write-Host("{0}. Exit menu" -f [int][MENU_DMA]::EXIT)
        Write-Host("")

        if ([DIAG_INPUT_RESULT]::DIAG_INPUT_FAIL -eq
            (DIAG_GetMenuOption ([ref]$option) ([int][MENU_DMA]::FREE_MEM) ) `
            -or !$option )
        {
            continue
        }

        if ($option -eq [int][MENU_DMA]::RESERVED_MEM)
        {
            Write-Host("Warning: The address for the reserved memory should be " +
                "calculated according to the values listed in registry key" +
                "HKLM/HARDWARE/RESOURCEMAP/System Resources/Physical Memory." +
                "Any other address may result in a BSOD. For more details " +
                "please refer to Tech Doc #129")
            $sMsg = "Enter reserved memory address (64 bit hex uint) "
            $qwAddr = 0
            if ([DIAG_INPUT_RESULT]::DIAG_INPUT_SUCCESS -ne
                (DIAG_InputNUM ([ref]$qwAddr) $sMsg $TRUE 1 ([UInt64]::MaxValue) ))
            {
                continue
            }
        }

        if ($option -eq [int][MENU_DMA]::ALLOCATE_CONTIG -or
            $option -eq [int][MENU_DMA]::ALLOCATE_SG -or
            $option -eq [int][MENU_DMA]::RESERVED_MEM)
        {
            $sMsg = "Enter memory allocation size in bytes " +
                "(32 bit uint) "
            $size = 0
            if ([DIAG_INPUT_RESULT]::DIAG_INPUT_SUCCESS -ne
                (DIAG_InputNUM ([ref]$size) $sMsg $FALSE 1 ([UInt32]::MaxValue) ))
            {
                continue
            }
            if ($option -eq [int][MENU_DMA]::ALLOCATE_CONTIG)
            {
                $sMsg = "Enter DMA address width of an address " +
                    "that your device supports, use 0 for default value " +
                    "(32 bit uint)"
                if ([DIAG_INPUT_RESULT]::DIAG_INPUT_SUCCESS -ne
                    (DIAG_InputNum ([ref]$dwDmaAddressWidth) $sMsg $FALSE 0 64) `
                    -and $dwDmaAddressWidth)
                {
                     continue
                }

                $dwOptions = `
                    [WD_DMA_OPTIONS]::DMA_KBUF_ALLOC_SPECIFY_ADDRESS_WIDTH `
                    -bor ($dwDmaAddressWidth -shl `
                    [int][windrvr_consts]::DMA_OPTIONS_ADDRESS_WIDTH_SHIFT)
            }

            # Free DMA memory before trying the new allocation
            FreeDmaMem ([ref]$pBuf) ([ref]$pDma)
        }

        switch ([MENU_DMA]$option)
        {
        ALLOCATE_CONTIG { # Allocate contiguous memory
            $dwStatus = [wdc_lib_decl]::WDC_DMAContigBufLock($hDev, [ref]$pBuf,
                $dwOptions, $size, [ref]$pDma)
            if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -eq $dwStatus)
            {
                $wdDma = [Marshal]::PtrToStructure($pDma,
                    [System.Type]$wdDma.GetType())
                Write-Host(("Contiguous memory allocated. user addr [{0:x}], " +
                    "physical addr [0x{1:x}], size [{2}(0x{3:x})]") -f $pBuf,
                    $wdDma.Page[0].pPhysicalAddr, $wdDma.Page[0].dwBytes,
                    $wdDma.Page[0].dwBytes)
            }
            else
            {
                PCI_ERR(("Failed allocating contiguous memory. size [{0}], " +
                    "Error [0x{1:x} - {2}]") -f $size, $dwStatus,
                    [utils]::Stat2Str($dwStatus))
            }
            }

        ALLOCATE_SG { # Allocate scatter-gather memory
            $pBuf = [System.Runtime.InteropServices.Marshal]::AllocHGlobal([int]$size)
            if ($pBuf -eq [IntPtr]::Zero)
            {
                PCI_ERR("PCI_Sample: Failed to allocate buffer for " +
                    "Scatter-Gather DMA");
                continue
            }

            $dwStatus = [wdc_lib_decl]::WDC_DMASGBufLock($hDev, $pBuf,
                0 <# $dwOptions #>, $size, [ref]$pDma)
            if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -eq $dwStatus)
            {
                $wdDma = [Marshal]::PtrToStructure($pDma,
                    [System.Type]$wdDma.GetType())
                Write-Host("SG memory allocated. user addr [{0:x}], size [{1}]" -f
                    $pBuf, $size)

                Write-Host("Pages physical addresses:")
                for ($i = 0; $i -lt $wdDma.dwPages; $i++)
                {
                    Write-Host(("{0}) physical addr [0x{1:x}], " +
                        "size [{2}(0x{3:x})]") -f ($i + 1),
                        $wdDma.Page[$i].pPhysicalAddr, $wdDma.Page[$i].dwBytes,
                        $wdDma.Page[$i].dwBytes)
                }
            }
            else
            {
                PCI_ERR(("Failed allocating SG memory. size [{0}], " +
                    "Error [0x{1:x} - {2}]") -f $size, $dwStatus,
                    [utils]::Stat2Str($dwStatus))
                [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pBuf)
            }
        }
        RESERVED_MEM {
            $pBuf = `
                [System.Runtime.InteropServices.Marshal]::AllocHGlobal([int]$size)
            $dwStatus = [wdc_lib_decl]::WDC_DMAReservedBufLock($hDev, $qwAddr,
                $pBuf, 0 <# $dwOptions #>, $size, [ref]$pDma)
            if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -eq $dwStatus)
            {
                $wdDma = [Marshal]::PtrToStructure($pDma,
                    [System.Type]$wdDma.GetType())
                Write-Host(("Reserved memory claimed. user addr [{0:x}], " +
                    "bus addr [0x{1:x}], size [{2}(0x{3:x})]") -f $pBuf,
                    $wdDma.Page[0].pPhysicalAddr, $wdDma.Page[0].dwBytes,
                    $wdDma.Page[0].dwBytes)
            }
            else
            {
                PCI_ERR(("Failed claiming reserved memory. size [{0}], " +
                    "Error [0x{1:x} - {2}]") -f $size, $dwStatus,
                    [utils]::Stat2Str($dwStatus))
            }
            }
<#        SHARE_CONTIG_BUF {
            WDS_DIAG_IpcSendDmaContigToGroup $pDma
            } #>
        FREE_MEM { # Free DMA memory
            FreeDmaMem ([ref]$pBuf) ([ref]$pDma)
            }
    }
    } while ([int][MENU_DMA]::EXIT -ne $option)

    # Free DMA memory before exiting
    FreeDmaMem ([ref]$pBuf) ([ref]$pDma)
}

# ----------------------------------------------------
# Plug-and-play and power management events handling
# ----------------------------------------------------
# Events menu options
enum MENU_EVENTS{
    REGISTER_UNREGISTER = 1
    EXIT = 99
}

# Diagnostics plug-and-play and power management events handler routine
function DiagEventHandler($dwAction)
{
    # TODO: You can modify this function in order to implement your own
    #       diagnostics events handler routine.

    Write-Host("Received event notification (device handle 0x{0:x}): " -f $hDev)
    switch ([WD_EVENT_ACTION]$dwAction)
    {
    WD_INSERT {
        Write-Host("WD_INSERT")
        }
    WD_REMOVE {
        Write-Host("WD_REMOVE")
        }
    WD_POWER_CHANGED_D0 {
        Write-Host("WD_POWER_CHANGED_D0")
        }
    WD_POWER_CHANGED_D1 {
        Write-Host("WD_POWER_CHANGED_D1")
        }
    WD_POWER_CHANGED_D2 {
        Write-Host("WD_POWER_CHANGED_D2")
        }
    WD_POWER_CHANGED_D3 {
        Write-Host("WD_POWER_CHANGED_D3")
        }
    WD_POWER_SYSTEM_WORKING {
        Write-Host("WD_POWER_SYSTEM_WORKING")
        }
    WD_POWER_SYSTEM_SLEEPING1 {
        Write-Host("WD_POWER_SYSTEM_SLEEPING1")
        }
    WD_POWER_SYSTEM_SLEEPING2 {
        Write-Host("WD_POWER_SYSTEM_SLEEPING2")
        }
    WD_POWER_SYSTEM_SLEEPING3 {
        Write-Host("WD_POWER_SYSTEM_SLEEPING3")
        }
    WD_POWER_SYSTEM_HIBERNATE {
        Write-Host("WD_POWER_SYSTEM_HIBERNATE")
        }
    WD_POWER_SYSTEM_SHUTDOWN {
        Write-Host("WD_POWER_SYSTEM_SHUTDOWN")
        }
    default {
        Write-Host("0x{0:x}" -f $dwAction)
        }
    }
}

# Register/unregister plug-and-play and power management events menu
function MenuEvents($Dev)
{
    $option = 0
    do
    {
        $fRegister = !(PCI_EventIsRegistered $Dev.hDev )

        Write-Host("")
        Write-Host("Plug-and-play and power management events")
        Write-Host("------------------------------------------")
        Write-Host("{0}. {1} events" -f [int][MENU_EVENTS]::REGISTER_UNREGISTER,
            $(if ($fRegister) { "Register" } Else { "Unregister" }))
        Write-Host("{0}. Exit menu" -f [int][MENU_EVENTS]::EXIT)
        Write-Host("")

        if ([DIAG_INPUT_RESULT]::DIAG_INPUT_FAIL -eq
            (DIAG_GetMenuOption ([ref]$option) ([MENU_EVENTS]::REGISTER_UNREGISTER)) `
            -or !$option)
        {
            continue
        }

        switch ([MENU_EVENTS]$option)
        {
        EXIT { # Exit menu
        }
        REGISTER_UNREGISTER { # Register/unregister events
            if ($fRegister)
            {
                $dwStatus = (PCI_EventRegister $Dev $function:DiagEventHandler)
                if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -eq $dwStatus)
                {
                    Write-Host("Events registered")
                }
                else
                {
                    PCI_ERR("Failed to register events. Last error [{0}]" -f
                        [utils]::Stat2Str([int]$dwStatus))
                }
            }
            else
            {
                if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -eq
                    (PCI_EventUnregister $Dev))
                {
                    Write-Host("Events unregistered")
                }
                else
                {
                    PCI_ERR("Failed to unregister events. Last error [{0}]" -f
                        [utils]::Stat2Str($dwStatus))
                }
            }
        }
        }
    } while ([int][MENU_EVENTS]::EXIT -ne $option)
}

# Main diagnostics menu
function MenuMain()
{
    $Dev = $NULL
    $option = 0
    do
    {
        Write-Host("")
        Write-Host("PCI main menu")
        Write-Host("-------------")
        Write-Host("{0}. Scan PCI bus" -f [int][MENU_MAIN]::SCAN_BUS)
        Write-Host("{0}. Find and open a PCI device" -f `
            [int][MENU_MAIN]::FIND_AND_OPEN)
        <# Write-Host("{0}. Allocate/free Shared Buffer" -f `
            [int][MENU_MAIN]::SHARED_BUFFER)
        Write-Host("{0}. Manage IPC" -f [int][MENU_MAIN]::IPC)
        #>
        if ($Dev.hDev)
        {
            Write-Host("{0}. Read/write the PCI configuration space" -f
                [int][MENU_MAIN]::RW_CFG_SPACE)
            <#Write-Host(("{0}. Register/unregister plug-and-play and " +
                "power management events") -f [int][MENU_MAIN]::EVENTS)#>
            Write-Host("{0}. Read/write memory and I/O addresses on the device" -f
                [int][MENU_MAIN]::RW_ADDR)
            Write-Host("{0}. Allocate/free memory for DMA" -f
                [int][MENU_MAIN]::ALLOC_FREE_DMA)
            # 
        }

        Write-Host("{0}. Exit" -f [int][MENU_MAIN]::EXIT)

    #ifndef PCI_REGS_NUM
        if ([DIAG_INPUT_RESULT]::DIAG_INPUT_FAIL -eq (DIAG_GetMenuOption ([ref]$option) `
            $(if ($Dev) {[int][MENU_MAIN]::ALLOC_FREE_DMA} else {[int][MENU_MAIN]::IPC})) `
            -or !$option)
        {
            continue
        }

   <#
   else # ifdef PCI_REGS_NUM
       if (DIAG_INPUT_FAIL -eq DIAG_GetMenuOption([ref]$option,
           $phDev ? [int][MENU_MAIN]::RW_REGS : [int][MENU_MAIN]::IPC))
        {
            continue
        }
    endif # ifdef PCI_REGS_NUM
#>
        switch ([MENU_MAIN]$option)
        {
        SCAN_BUS { # Scan bus
            WDC_DIAG_PciDevicesInfoPrintAll $FALSE
        }
        FIND_AND_OPEN { # Find and open a PCI device
            if ($Dev)
            {
               DeviceClose $Dev
            }
            $Dev = DeviceFindAndOpen 0 0
        }
<#      SHARED_BUFFER { # Handle Shared Buffer Operations
            MenuSharedBuffer
        }

        IPC { # Register/unregister Inter-Process Communication
            MenuIpc
        }
#>
        RW_CFG_SPACE { # Read/write the device's configuration space
            MenuReadWriteCfgSpace $Dev.hDev
        }
<#        EVENTS { # Register/unregister plug-and-play and power
                 #              management events
            MenuEvents $Dev
        }#>
        RW_ADDR { # Read/write memory and I/O addresses
            MenuReadWriteAddr $Dev
        }
        ALLOC_FREE_DMA { # Allocate/free DMA memory
            MenuDma $Dev
        }
        # 
        }
    } while ([int][MENU_MAIN]::EXIT -ne $option)

}

function main()
{
    Write-Host("")
    Write-Host("PCI diagnostic utility.")
    Write-Host("Application accesses hardware using " + $WD_PROD_NAME + "")
    # 
    Write-Host("and a Kernel PlugIn driver ({0})." -f $KP_PCI_DRIVER_NAME)
    # 

    # Initialize the PCI library
    $dwStatus = PCI_LibInit

    if([int][WD_ERROR_CODES]::WD_OPERATION_ALREADY_DONE -eq $dwStatus)
    {
        PCI_LibUninit
        $dwStatus = PCI_LibInit
    }

    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
    {
            PCI_ERR("pci_diag: Failed to initialize the PCI library: {0}" -f
                    [utils]::Stat2Str($dwStatus))

        return $dwStatus
    }

    TraceLog("WinDriver user mode version {0} operating from PowerShell {1}" -f
        $WD_VERSION_STR, $PSversionTable.PSversion.ToString())

    # Find and open a PCI device (by default ID)
    if ($PCI_DEFAULT_VENDOR_ID)
    {
            $Dev = DeviceFindAndOpen $PCI_DEFAULT_VENDOR_ID $PCI_DEFAULT_DEVICE_ID
    }

    # Display main diagnostics menu for communicating with the device
    MenuMain

    # Perform necessary cleanup before exiting the program:
    # Close the device handle

    if ($Dev.hDev)
    {
            DeviceClose $Dev
    }

<#
    if (WDS_IsIpcRegistered)
        WDS_IpcUnRegister
#>

    # Uninitialize libraries
    $dwStatus = PCI_LibUninit
    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
    {
            PCI_ERR "pci_diag: Failed to uninitialize the PCI library: " +
                    [utils]::Stat2Str($dwStatus)
    }
    return "pci_diag Exit status: {0} [{1:x}] " -f $dwStatus, [utils]::Stat2Str($dwStatus)
}

main



