<# Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com #>

<#*****************************************************************************
*  File: wdc_diag_lib.ps1 - Implementation of shared WDC PCI and ISA  devices'*
*  user-$mode diagnostics API.                                                *
*                                                                             *
*  Note: This code sample is provided AS-IS and as a guiding sample only.     *
******************************************************************************#>

<#  NOTE: pci_diag.ps1 imports wdc_diag_lib.psm1 from
          $(MY_DOCUMENTS)/WindowsPowerShell\Modules\wdc_diag_lib
          if you wish to modify the script, you can copy this script to the that
          path #>

using namespace Jungo.wdapi_dotnet
using namespace System

using module ./diag_lib.psm1

<#************************************************************
  General definitions
 ************************************************************#>
<# Error messages display #>
function WDC_DIAG_ERR{ $Host.UI.WriteErrorLine($args) }

[Flags()]
Enum WDC_DIAG_REG_PRINT
{
    NAME            = 0x1
    DESC            = 0x2
    ADDR_SPACE      = 0x4
    OFFSET          = 0x8
    SIZE            = 0x10
    PRINT_DIRECTION = 0x12
    DEFAULT         = 0x15
    ALL             = 0x31
}

enum READ_REGS_ALL
{
    DESC_INDENT    =  39
    DESC_WIDTH     =  22
    DETAILS_INDENT =  61
    DETAILS_WIDTH  =  20
}
enum REGS_INFO_PRINT_DETAILS
{
    INDENT = 54
    WIDTH  = 22
}
enum PCIE_REGS
{
    NUM    = 68
}

<# -----------------------------------------------
    PCI/ISA
   ----------------------------------------------- #>

<# Print device's resources information to file.
 * For a registered device (hCard -ne 0), print also kernel and user-mode
 * mappings of memory address items #>

function WDC_DIAG_DeviceResourcesPrint($pCard, $hCard, $fp)
{
    if (!$pCard)
    {
        WDC_DIAG_ERR "WDC_DIAG_DeviceResourcesPrint: Error - NULL card " +
            "pointer"
        return
    }

    for ($($resources = 0; $i = 0) ;$i -lt $pCard.dwItems; $i++)
    {
        $pItem = $pCard.Item[$i]

        switch ([ITEM_TYPE]$pItem.item)
        {
        ITEM_MEMORY {
            $resources++
            fprintf $fp (("    Memory range [BAR {0}]: base 0x{1:X} " +
                "size 0x{2:X}") -f $pItem.I.Mem.dwBar,
                $pItem.I.Mem.pPhysicalAddr, $pItem.I.Mem.qwBytes)

            if ($hCard) <# Registered device #>
            {
                fprintf $fp ("        Kernel-mode address mapping: 0x{0:X}" -f
                    $pItem.I.Mem.pTransAddr)
                fprintf $fp ("        User-mode address mapping: 0x{0:X}" -f
                    $pItem.I.Mem.pUserDirectAddr)
            }
        }

        ITEM_IO {
            $resources++
            fprintf $fp (("    I/O range [BAR {0}]: base [0x{1:X}], size " +
                "[0x{2:X}]") -f $pItem.I.IO.dwBar, $pItem.I.IO.pAddr,
                $pItem.I.IO.dwBytes)
        }

        ITEM_INTERRUPT {
            $resources++
            fprintf $fp ("    Interrupt: IRQ {0}" -f $pItem.value.I.Int.dwInterrupt)
            fprintf $fp "    Interrupt Options (supported interrupts):"
            if ([wdc_defs_macros]::WDC_INT_IS_MSI($pItem.I.Int.dwOptions))
            {
                fprintf $fp ("        {0}" -f
                    (WDC_DIAG_IntTypeDescriptionGet $pItem.I.Int.dwOptions))
            }
            <# According to the MSI specification, it is recommended that
             * a PCI device will support both MSI/MSI-X and level-sensitive
             * interrupts, and allow the operating system to choose which
             * type of interrupt to use. #>
            if ([int]$pItem.I.Int.dwOptions -band [WD_INTERRUPT_TYPE]::INTERRUPT_LEVEL_SENSITIVE)
            {
                fprintf $fp ("        {0}" -f
                    (WDC_DIAG_IntTypeDescriptionGet ([WD_INTERRUPT_TYPE]::INTERRUPT_LEVEL_SENSITIVE)))
            }
            elseif (![wdc_defs_macros]::WDC_INT_IS_MSI($pItem.I.Int.dwOptions))
            <# MSI/MSI-X interrupts are always edge-triggered, so there is no
             * no need to display a specific edge-triggered indication for
             * such interrupts. #>
            {
                fprintf $fp ("        {0}" -f
                    (WDC_DIAG_IntTypeDescriptionGet ([WD_INTERRUPT_TYPE]::INTERRUPT_LATCHED)))
            }
        }

        ITEM_BUS {
        }

        default: {
            fprintf $fp "    Invalid item type (0x{0:X})" -f $pItem.value.item
        }
        }
    }

    if (!$resources)
    {
        fprintf $fp "    Device has no resources"
    }
}

<# Print run-time registers and PCI configuration registers information #>

function WDC_DIAG_RegsInfoPrint($hDev, $pRegs, $dwNumRegs, $options,
    $isExpress)
{
    $extended = $(If ($isExpress) {([PCIE_REGS]::NUM)} Else {0})
    $pciExpressOffset = 0
    $headerType = New-Object -TypeName "Jungo.wdapi_dotnet.WDC_PCI_HEADER_TYPE"

    if (!$hDev)
    {
        WDC_DIAG_ERR("WDC_DIAG_RegsInfoPrint: Error - NULL WDC device handle")
        return
    }

    if ($isExpress)
    {
        $dwStatus = [wdc_lib_decl]::WDC_PciGetExpressOffset($hDev, [ref]$pciExpressOffset)
        if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
        {
            WDC_DIAG_ERR("WDC_DIAG_RegsInfoPrint: Error getting PCI Express " +
                " Offset")
            return
        }
    }

    $dwStatus = [wdc_lib_decl]::WDC_PciGetHeaderType($hDev, [ref]$headerType)
    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
    {
        WDC_DIAG_ERR("WDC_DIAG_RegsInfoPrint: Error - unable to determine " +
            "PCI header type")
        return
    }

    if (!$dwNumRegs)
    {
        Write-Host("There are currently no pre-defined registers to display")
        return
    }

    if (!$pRegs)
    {
        WDC_DIAG_ERR("WDC_DIAG_RegsInfoPrint: Error - NULL registers " +
            "information pointer")
        return
    }

    if (!$options)
    {
        $options = [WDC_DIAG_REG_PRINT]::DEFAULT
    }

    $fName = $options -band [WDC_DIAG_REG_PRINT]::NAME
    $fDesc = $options -band [WDC_DIAG_REG_PRINT]::DESC
    $fAddrSpace = $options -band [WDC_DIAG_REG_PRINT]::ADDR_SPACE
    $fOffset = $options -band [WDC_DIAG_REG_PRINT]::OFFSET
    $fSize = $options -band [WDC_DIAG_REG_PRINT]::SIZE
    $fDir = $options -band [WDC_DIAG_REG_PRINT]::DIRECTION

    Write-Host("")
    Write-Host("PCI {0}Registers" -f $(If ($isExpress) {"Express "} Else {""}))
    Write-Host("----{0}---------" -f $(If ($isExpress) {"--------"} Else {""}))

    $pRegs | Format-Table -Property `
        @{Label="Ix"; Expression={$pRegs.IndexOf($_) + $extended + 1}},
        @{Label="Name"; Expression={$_.sName}; Width=30},
        @{Label="Offset"; Expression={"0x{0:X}" -f $_.dwOffset}},
        @{Label="Size"; Expression={$_.dwSize}; Width=30},
        @{Label="R/W"; Expression={$(If ($_.DIRECTION -eq [WDC_DIRECTION]::WDC_READ_WRITE) {"RW"} Else {"R"})}},
        @{Label="Description"; Expression={$_.sDescription}; Width=30}
}

<# Set address access $mode #>
function WDC_DIAG_SetMode([ref]$pMode)
{
    $option = 0

    if (!$pMode)
    {
        WDC_DIAG_ERR("WDC_DIAG_SetMode: Error - NULL mode pointer")
        return $FALSE
    }

    Write-Host("")
    Write-Host("Select read/write mode:")
    Write-Host("-----------------------")
    Write-Host("1. 8 bits ({0} bytes)" -f [wdc_lib_consts]::WDC_SIZE_8)
    Write-Host("2. 16 bits ({0} bytes)" -f [wdc_lib_consts]::WDC_SIZE_16)
    Write-Host("3. 32 bits ({0} bytes)" -f [wdc_lib_consts]::WDC_SIZE_32)
    Write-Host("4. 64 bits ({0} bytes)" -f [wdc_lib_consts]::WDC_SIZE_64)
    Write-Host("{0}. Exit Menu" -f $DIAG_EXIT_MENU)

    if([DIAG_INPUT_RESULT]::DIAG_INPUT_SUCCESS -ne (DIAG_GetMenuOption ([ref]$option) 4))
    {
        return $FALSE
    }

    switch ($option)
    {
    1 {
        $pMode.value = [WDC_ADDR_MODE]::WDC_MODE_8
        }
    2 {
        $pMode.value = [WDC_ADDR_MODE]::WDC_MODE_16
        }
    3 {
        $pMode.value = [WDC_ADDR_MODE]::WDC_MODE_32
        }
    4 {
        $pMode.value = [WDC_ADDR_MODE]::WDC_MODE_64
        }
    default {
        Write-Host("Invalid selection")
        return $FALSE
    }
    }
    return $TRUE
}

<# Get data for address write operation from user #>
<# Data size ($dwSize) should be [wdc_lib_consts]::WDC_SIZE_8, [wdc_lib_consts]::WDC_SIZE_16, [wdc_lib_consts]::WDC_SIZE_32 or
 * [wdc_lib_consts]::WDC_SIZE_64 #>
function WDC_DIAG_InputWriteData([ref]$pData, $dwSize)
{
    if (!$pData)
    {
        WDC_DIAG_ERR("WDC_DIAG_InputWriteData: Error - NULL data pointer")
        return $FALSE
    }


    $u64MaxVal = $(if($dwSize -eq [wdc_lib_consts]::WDC_SIZE_64) {
        [System.uInt64]::MaxValue } Else
        {
            $(if($dwSize -eq [wdc_lib_consts]::WDC_SIZE_32)
            {
                [System.uInt32]::MaxValue } Else
                {
                    $(if($dwSize -eq [wdc_lib_consts]::WDC_SIZE_16) {
                        [System.uInt16]::MaxValue } Else
                        {
                            [System.uInt8]::MaxValue
                        }
                    )
                }
            )
        })


    $sMsg = "Enter data to write (max value: 0x{0:X}) or '{1}' to cancel: 0x" -f
           $u64MaxVal, $DIAG_CANCEL

    if ([DIAG_INPUT_RESULT]::DIAG_INPUT_SUCCESS -ne `
        (DIAG_InputNum ([ref]$u64Data) $sMsg $TRUE 0 $u64MaxVal))
    {
        Write-Host("Invalid input")
        return $FALSE
    }

    switch ($dwSize)
    {
    ([wdc_lib_consts]::WDC_SIZE_8) {
        $pData.value = $u64Data
        return $TRUE
        }
    ([wdc_lib_consts]::WDC_SIZE_16) {
        $pData.value = $u64Data
        return $TRUE
        }
    ([wdc_lib_consts]::WDC_SIZE_32) {
        $pData.value = $u64Data
        return $TRUE
        }
    ([wdc_lib_consts]::WDC_SIZE_64) {
        $pData.value = $u64Data
        return $TRUE
        }
    default {
        WDC_DIAG_ERR("WDC_DIAG_InputWriteData: Error - Invalid size " +
            "({0} bytes)" -f $dwSize)
        }
    }
    return $FALSE
}

<# Read/write a memory or I/O address #>
function WDC_DIAG_ReadWriteAddr($hDev, $direction, $dwAddrSpace, $mode)
{
    $dwOffset = 0
    $bData = 0
    $wData = 0
    $u32Data = 0
    $u64Data = 0

    if (!$hDev)
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteAddr: Error- NULL WDC device handle")
        return
    }

    $dwStatus = (DIAG_InputNum ([ref]$dwOffset) `
        $(if ([WDC_DIRECTION]::WDC_READ -eq $direction) {
        "Enter offset to read from" } Else { "Enter offset to write to" }) `
        $TRUE 0 0)
    if ([DIAG_INPUT_RESULT]::DIAG_INPUT_SUCCESS -ne $dwStatus)
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteAddr: {0} getting the offset" -f
            $(if ($dwStatus -eq [DIAG_INPUT_RESULT]::DIAG_INPUT_CANCEL) {"Canceled"} Else {"Failed"}))
        return
    }

    if (([WDC_DIRECTION]::WDC_WRITE -eq $direction) -and
        !(WDC_DIAG_InputWriteData `
        $(if ([WDC_ADDR_MODE]::WDC_MODE_8 -eq $mode) { [ref]$bData }
            Else {
            $(if ([WDC_ADDR_MODE]::WDC_MODE_16 -eq $mode) { [ref]$wData }
                Else {
                $(if ([WDC_ADDR_MODE]::WDC_MODE_32 -eq $mode) { [ref]$u32Data }
                    Else {
                    [ref]$u64Data
                })
            })
        }) ([wdc_lib_macros]::WDC_ADDR_MODE_TO_SIZE($mode)) ))
    {
            return
    }

    switch ([WDC_ADDR_MODE]$mode)
    {
    WDC_MODE_8 {
        $dwStatus = $(if ([WDC_DIRECTION]::WDC_READ -eq $direction) {
            [wdc_lib_decl]::WDC_ReadAddr8($hDev, $dwAddrSpace, $dwOffset, [ref]$bData) } Else
            { [wdc_lib_decl]::WDC_WriteAddr8($hDev, $dwAddrSpace, $dwOffset, $bData) })
        if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -eq $dwStatus)
        {
            Write-Host("{0} 0x{1:X} {2} offset 0x{3:X} in BAR {4}" -f
                $(if ([WDC_DIRECTION]::WDC_READ -eq $direction) { "Read" } Else { "Wrote" }), 
                $bData, $(if ([WDC_DIRECTION]::WDC_READ -eq $direction) { "from" } Else { "to" }),
                $dwOffset, $dwAddrSpace)
        }
    }
    WDC_MODE_16 {
        $dwStatus = $(if ([WDC_DIRECTION]::WDC_READ -eq $direction) {
            [wdc_lib_decl]::WDC_ReadAddr16($hDev, $dwAddrSpace, $dwOffset, [ref]$wData) } Else {
            [wdc_lib_decl]::WDC_WriteAddr16($hDev, $dwAddrSpace, $dwOffset, $wData) })
        if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -eq $dwStatus)
        {
            Write-Host("{0} 0x{1:X} {2} offset 0x{3:X} in BAR {4}" -f
                $(if ([WDC_DIRECTION]::WDC_READ -eq $direction) { "Read" } Else { "Wrote" }),
                $wData,
                $(if ([WDC_DIRECTION]::WDC_READ -eq $direction) { "from" } Else { "to" }),
                $dwOffset, $dwAddrSpace)
        }
    }
    WDC_MODE_32 {
        $dwStatus = $(if([WDC_DIRECTION]::WDC_READ -eq $direction) {
            [wdc_lib_decl]::WDC_ReadAddr32($hDev, $dwAddrSpace, $dwOffset, [ref]$u32Data) }
            Else { [wdc_lib_decl]::WDC_WriteAddr32($hDev, $dwAddrSpace, $dwOffset, $u32Data) })
        if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -eq $dwStatus)
        {
            Write-Host("{0} 0x{1:X} {2} offset 0x{3:X} in BAR {4}" -f
                $(if ([WDC_DIRECTION]::WDC_READ -eq $direction) { "Read" } Else { "Wrote" }),
                $u32Data,
                $(if ([WDC_DIRECTION]::WDC_READ -eq $direction) { "from" } Else { "to" }),
                $dwOffset, $dwAddrSpace)
        }
    }
    WDC_MODE_64 {
        $dwStatus = $(if ([WDC_DIRECTION]::WDC_READ -eq $direction) {
            [wdc_lib_decl]::WDC_ReadAddr64($hDev, $dwAddrSpace, $dwOffset, [ref]$u64Data) } Else {
            [wdc_lib_decl]::WDC_WriteAddr64($hDev, $dwAddrSpace, $dwOffset, $u64Data) })
        if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -eq $dwStatus)
        {
            Write-Host("{0} 0x{1:X} {2} offset 0x{3:X} in BAR {4}" -f
                $(if ([WDC_DIRECTION]::WDC_READ -eq $direction) { "Read" } Else { "Wrote" }),
                $u64Data,
                $(if ([WDC_DIRECTION]::WDC_READ -eq $direction) { "from" } Else { "to" }),
                $dwOffset, $dwAddrSpace)
        }
    }
    default {
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteAddr: Error - Invalid mode ({0})" -f
            $mode)
        return
    }
    }

    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
    {
        Write-Host("Failed to {0} offset 0x{1:X} in BAR {2}. Error 0x{3:x} - {4}" -f
            $(if([WDC_DIRECTION]::WDC_READ -eq $direction) { "read from" } Else { "write to" }),
            $dwOffset, $dwAddrSpace, $dwStatus, [utils]::Stat2Str($dwStatus))
    }
}

<# Read/write a memory or I/O address OR an offset in the PCI configuration
 * space ($dwAddrSpace -eq [wdc_lib_consts]::WDC_AD_CFG_SPACE) #>
function WDC_DIAG_ReadWriteBlock($hDev, $direction, $dwAddrSpace)
{
    $dwOffset = 0
    $dwBytes = 0
    $sDir = $(if([WDC_DIRECTION]::WDC_READ -eq $direction) { "read" } Else { "write" })

    if (!$hDev)
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteBlock: Error- NULL WDC device " +
            "handle")
        return
    }

    if ([DIAG_INPUT_RESULT]::DIAG_INPUT_SUCCESS -ne (DIAG_InputNum ([ref]$dwOffset) "Enter Offset" $TRUE 0 0))
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteBlock: {0} getting the offset" -f
            $(if ($dwStatus -eq $DIAG_INPUT_CANCEL) {"Canceled"} Else {"Failed"}))
        return
    }

    if ([DIAG_INPUT_RESULT]::DIAG_INPUT_SUCCESS -ne
        (DIAG_InputNum ([ref]$dwBytes) "Enter number of bytes" $FALSE 0 0))
    {
        WDC_DIAG_ERR(("WDC_DIAG_ReadWriteBlock: {0} getting the number of bytes " +
            "to transfer") -f $(if ($dwStatus -eq $DIAG_INPUT_CANCEL) {"Canceled"} Else
            {"Failed"}))
        return
    }

    if (!$dwBytes)
    {
        return
    }
    $pBuf = [Byte[]]::new($dwBytes)

    if ([WDC_DIRECTION]::WDC_WRITE -eq $direction)
    {
        Write-Host("data to write (hex format): 0x")
        if (!(DIAG_GetHexBuffer ([ref]$pBuf)))
        {
            Pause
            return
        }
    }

    if ([wdc_lib_consts]::WDC_AD_CFG_SPACE -eq $dwAddrSpace) <# Read/write a configuration/attribute
                                           space address #>
    {
        [WD_BUS_TYPE]$busType = [wdc_lib_decl]::WDC_GetBusType($hDev)

        if ([WD_BUS_TYPE]::WD_BUS_PCI -eq $busType)
             <# Read/write PCI configuration space offset #>
        {
            $pData = [System.Runtime.InteropServices.Marshal]::
                AllocHGlobal($dwBytes)

            if ($direction -eq [WDC_DIRECTION]::WDC_READ)
            {
                $dwStatus = [wdc_lib_decl]::WDC_PciReadCfg($hDev, $dwOffset,
                    $pData, $dwBytes)
                [System.Runtime.InteropServices.Marshal]::Copy($pData, $pBuf, 0,
                    $dwBytes)
            }
            else
            {
                [System.Runtime.InteropServices.Marshal]::Copy($pBuf, 0,
                    $pData, $dwBytes)
                $dwStatus = [wdc_lib_decl]::WDC_PciWriteCfg($hDev, $dwOffset,
                    $pData, $dwBytes)
            }
        }
        else
        {
            Write-Host("Error - Cannot read/write configuration space address " +
                "space for bus type [0x{0:X}]" -f $busType)
            Pause
            return
        }
    }
    else <# Read/write a memory or I/O address #>
    {
        $mode = 0
        [bool]$fAutoInc = $TRUE

        if (!(WDC_DIAG_SetMode ([ref]$mode) ))
        {
            return
        }

        $sMsg = ("Do you wish to increment the address after each {0} block " +
            "(0x{1:X} bytes) (0 - No, Otherwise - Yes)? ") -f $sDir,
            [wdc_lib_macros]::WDC_ADDR_MODE_TO_SIZE($mode)
        if ([DIAG_INPUT_RESULT]::DIAG_INPUT_SUCCESS -ne (DIAG_InputNum ([ref]$fAutoInc) $sMsg $FALSE 0 0))
        {

        }

        $options = $(if($fAutoInc) { [WDC_ADDR_RW_OPTIONS]::WDC_ADDR_RW_DEFAULT } Else
            { [WDC_ADDR_RW_OPTIONS]::WDC_ADDR_RW_NO_AUTOINC })

        $dwStatus = $(if($direction -eq [WDC_DIRECTION]::WDC_READ) {
            [wdc_lib_decl]::WDC_ReadAddrBlock($hDev,
                $dwAddrSpace, $dwOffset, $dwBytes, $pBuf, $mode, $options) } Else {
            [wdc_lib_decl]::WDC_WriteAddrBlock($hDev,
                $dwAddrSpace, $dwOffset, $dwBytes, $pBuf, $mode, $options)
            })
    }

    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -eq $dwStatus)
    {
        if ([WDC_DIRECTION]::WDC_READ -eq $direction)
        {
            DIAG_PrintHexBuffer $pBuf $dwBytes $FALSE
        }
        else
        {
            Write-Host("Wrote 0x{0:X} bytes to offset 0x{1:X}" -f $dwBytes, $dwOffset)
        }
    }
    else
    {
        Write-Host("Failed to {0} 0x{1:X} bytes {2} offset 0x{3:X}. Error 0x{4:X} - {5}" -f
            $sDir, $dwBytes,
            $(if([WDC_DIRECTION]::WDC_READ -eq $direction) { "from" } Else { "to" }),
            $dwOffset, $dwStatus, [utils]::Stat2Str($dwStatus))
    }

    if ($pData)
    {
        [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pData)
        $pData = $null
    }

    Pause
}

<# Read all pre-defined run-time or PCI configuration registers and display
 * results #>
function WDC_DIAG_ReadRegsAll($hDev, $pRegs, $dwNumRegs, $fPciCfg, $fExpress)
{
    [UINT32]$outBufLen = 0
    [byte]$bData = 0
    [uint16]$wData = 0
    [uint32]$u32Data = 0
    [uint64]$u64Data = 0
    $pciExpressOffset = 0
    $headerType = New-Object -TypeName "Jungo.wdapi_dotnet.WDC_PCI_HEADER_TYPE"
    $details = New-Object -TypeName "System.Text.StringBuilder" -ArgumentList 10240

    if (!$hDev)
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: Error - NULL WDC device handle")
        return
    }

    if ($fExpress)
    {
        $dwStatus = [wdc_lib_decl]::WDC_PciGetExpressOffset($hDev,
            [ref]$pciExpressOffset)
        if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
        {
            WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: Error getting PCI Express" +
                " Offset")
            return
        }
    }

    $dwStatus = [wdc_lib_decl]::WDC_PciGetHeaderType($hDev, [ref]$headerType)
    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
    {
        WDC_DIAG_ERR("WDC_DIAG_RegsInfoPrint: Error - unable to determine" +
            "PCI header type")
        return
    }

    if (!$dwNumRegs -or !$pRegs)
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: {0}" -f $(if (!$dwNumRegs)
            {"No registers ($dwNumRegs = 0)"} Else {"Error - NULL registers pointer"}))

        return
    }

    Write-Host("")
    Write-Host("{0} registers data:" -f $(if ($fPciCfg -and $fExpress)
        {"PCI Express configuration"} Else {$(if ($fPciCfg) { "PCI configuration" }
        Else {"run-time"}) }))
    Write-Host("---------------------------------")

    $pRegs | Format-Table -Wrap -Property `
        @{Label="Ix"; Expression={$pRegs.IndexOf($_) + $extended + 1}},
        @{Label="Name"; Expression={$_.sName}},
        @{Label="Data"; Expression={"0X{1:X}" -f
            $(if ($_.dwSize -eq [wdc_lib_consts]::WDC_SIZE_8)
            { [wdc_lib_decl]::WDC_PciReadCfg8($hDev, $_.dwOffset + $pciExpressOffset,
                [ref]$bData)
                 } Else
            { $(if ($_.dwSize -eq [wdc_lib_consts]::WDC_SIZE_16)
                { [wdc_lib_decl]::WDC_PciReadCfg16($hDev, $_.dwOffset + $pciExpressOffset, 
                    [ref]$wData) }
                Else
                { $(if ($_.dwSize -eq [wdc_lib_consts]::WDC_SIZE_32)
                    { [wdc_lib_decl]::WDC_PciReadCfg32($hDev,
                        $_.dwOffset + $pciExpressOffset, [ref]$u32Data) }
                    Else
                    { [wdc_lib_decl]::WDC_PciReadCfg64($hDev,
                        $_.dwOffset + $pciExpressOffset, [ref]$u64Data) }
                   )
                }
               )
            }),
            $(if ($_.dwSize -eq [wdc_lib_consts]::WDC_SIZE_8)
            { $bData } Else
            { $(if ($_.dwSize -eq [wdc_lib_consts]::WDC_SIZE_16)
                { $wData } Else
                { $(if ($_.dwSize -eq [wdc_lib_consts]::WDC_SIZE_32)
                    { $u32Data }
                    Else
                    { $u64Data }
                   )
                }
               )
            }), ($u64Data = $u32data = $wData = $bData = 0)}},
        @{Label="Description"; Expression={$_.sDescription}},
        @{Label="Details"; Expression={"{1}" -f
            $(if ($fPciCfg -eq $TRUE) {
                $(if ($fExpress -eq $TRUE) {
                    [wdc_lib_decl]::PciExpressConfRegData2Str($hDev,
                        $_.dwOffset, $details, 10240, [ref]$outBufLen)
                } Else {
                    [wdc_lib_decl]::PciConfRegData2Str($hDev, $_.dwOffset,
                        $details, 10240, [ref]$outBufLen)
                })
            }), $details.toString(), $details.Clear(), ($outBufLen = 0) }}

    Pause
}

<# Display a list of pre-defined run-time or PCI configuration registers
   and let user select to read/write from/to a specific register #>
function WDC_DIAG_ReadWriteReg([IntPtr]$hDev, $pRegs,
     [uint32]$dwNumRegs, [WDC_DIRECTION]$direction, [BOOL]$fPciCfg, [BOOL]$fExpress)
{
     $dwReg = 0
     $outBufLen = 0
     [byte]$bData = 0
     [uint16]$wData = 0
     [uint32]$u32Data = 0
     [uint64]$u64Data = 0
     $pciExpressOffset = 0
     $headerType = New-Object -TypeName "Jungo.wdapi_dotnet.WDC_PCI_HEADER_TYPE"
     $details = New-Object -TypeName "System.Text.StringBuilder" -ArgumentList 10240

    if (!$hDev)
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: Error - NULL WDC device handle")
        return
    }

    if ($fExpress -and [wdc_lib_decl]::WDC_PciGetExpressGen($hDev) -ne 0)
    {
        $dwStatus = [wdc_lib_decl]::WDC_PciGetExpressOffset($hDev,
            [ref]$pciExpressOffset)
        if ($dwStatus)
        {
            WDC_DIAG_ERR("WDC_DIAG_ReadRegsAll: Error getting PCI Express" +
                " Offset")
            return
        }
    }

    $dwStatus = [wdc_lib_decl]::WDC_PciGetHeaderType($hDev, [ref]$headerType)
    if ($dwStatus)
    {
        WDC_DIAG_ERR("WDC_DIAG_RegsInfoPrint: Error - unable to determine" +
            "PCI header type")
        return
    }

    if (!$dwNumRegs -or !$pRegs)
    {
        WDC_DIAG_ERR("WDC_DIAG_ReadWriteReg: {0}" -f
            $(if (!$dwNumRegs) {"No registers to read/write ($dwNumRegs == 0)"} Else
            {"Error - NULL registers pointer"}))
        return
    }

    <# Display pre-defined registers' information #>
    Write-Host("")
    Write-Host("PCI {0} registers:" -f $(if($fPciCfg) { "configuration" }
        Else { "run-time" } ))
    Write-Host("----------------------------")
    WDC_DIAG_RegsInfoPrint $hDev $pRegs $dwNumRegs ([WDC_DIAG_REG_PRINT]::ALL) `
        $FALSE

    <# Read/write register #>
    Write-Host("")
    $sMsg = "Select a register from the list above to {0} or 0 to cancel: " -f
        $(if([WDC_DIRECTION]::WDC_READ -eq $direction) { "read from" } Else
        { "write to" })

    if ([DIAG_INPUT_RESULT]::DIAG_INPUT_SUCCESS -ne (DIAG_InputNum ([ref]$dwReg) $sMsg $FALSE 1 $dwNumRegs))
    {
        Write-Host "Invalid Selection"
        Pause
        return
    }

    $pReg  = $pRegs[$dwReg - 1]

    if ( (([WDC_DIRECTION]::WDC_READ -eq $direction) -and ([WDC_DIRECTION]::WDC_WRITE -eq $pReg.direction)) -or
        (([WDC_DIRECTION]::WDC_WRITE -eq $direction) -and ([WDC_DIRECTION]::WDC_READ -eq $pReg.direction)))
    {
        Write-Host("Error - you have selected to {0} a {1}-only register" -f
            $(if([WDC_DIRECTION]::WDC_READ -eq $direction) { "read from" } Else { "write to" }),
            $(if([WDC_DIRECTION]::WDC_WRITE -eq $pReg.direction) { "write" } Else { "read" }))
        Pause
        return
    }

    if (([WDC_DIRECTION]::WDC_WRITE -eq $direction) -and (
        !(WDC_DIAG_InputWriteData (
        $(if([wdc_lib_consts]::WDC_SIZE_8 -eq $pReg.dwSize) { [ref]$bData }
        Else {
            $(if([wdc_lib_consts]::WDC_SIZE_16 -eq $pReg.dwSize) { [ref]$wData } Else
            {
                $(if([wdc_lib_consts]::WDC_SIZE_32 -eq $pReg.dwSize) { [ref]$u32Data } Else
                    { [ref]$u64Data })
            })
         })) $pReg.dwSize)))
    {
        Write-Host "WDC_DIAG_InputWriteData failed"
        return
    }

    switch ($pReg.dwSize)
    {
    ([wdc_lib_consts]::WDC_SIZE_8)
    {
        if ([WDC_DIRECTION]::WDC_READ -eq $direction)
        {
            $dwStatus = $(if ($fPciCfg) {
                [wdc_lib_decl]::WDC_PciReadCfg8($hDev, $pReg.dwOffset + $pciExpressOffset,
                    [ref]$bData) } Else {
                [wdc_lib_decl]::WDC_ReadAddr8($hDev, $pReg.dwAddrSpace, $pReg.dwOffset +
                    $pciExpressOffset, [ref]$bData) })
        }
        else
        {
            $dwStatus = $(if ($fPciCfg) {
                [wdc_lib_decl]::WDC_PciWriteCfg8($hDev, ($pReg.dwOffset +
                    $pciExpressOffset), $bData) } Else {
                [wdc_lib_decl]::WDC_WriteAddr8($hDev, $pReg.dwAddrSpace, ($pReg.dwOffset +
                    $pciExpressOffset), $bData) })
        }
        if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -eq $dwStatus)
        {
            Write-Host("{0} 0x{1:x} {2} register {3} at offset [0x{4:x}]" -f
                $(if([WDC_DIRECTION]::WDC_READ -eq $direction) { "Read" } Else { "Wrote" }),
                $bData,
                $(if([WDC_DIRECTION]::WDC_READ -eq $direction) { "from" } Else { "to" }),
                $pReg.sName,
                $(if($fExpress) { $pReg.dwOffset + $pciExpressOffset } Else { $pReg.dwOffset }))

            $dwStatus = $(if ($fExpress)
                { [wdc_lib_decl]::PciExpressConfRegData2Str($hDev, $pReg.dwOffset, $details,
                    10240, [ref]$outBufLen) } Else {
                [wdc_lib_decl]::PciConfRegData2Str($hDev, $pReg.dwOffset, $details,
                    10240, [ref]$outBufLen) })
            Write-Host ("{0}" -f $(if([WDC_DIRECTION]::WDC_READ -eq $direction -and $details.Equals(""))
                { "Decoded register data: " } Else { "" }))
            Write-Host ("{0}" -f $(if([WDC_DIRECTION]::WDC_READ -eq $direction -and $details.Equals(""))
                { $details } Else { "" } ))
        }
    }
    ([wdc_lib_consts]::WDC_SIZE_16)
    {
        if ([WDC_DIRECTION]::WDC_READ -eq $direction)
        {
            $dwStatus = $(if ($fPciCfg) {
                [wdc_lib_decl]::WDC_PciReadCfg16($hDev, ($pReg.dwOffset + $pciExpressOffset),
                    [ref]$wData) } Else {
                [wdc_lib_decl]::WDC_ReadAddr16($hDev, $pReg.dwAddrSpace, ($pReg.dwOffset +
                    $pciExpressOffset), [ref]$wData) })
        }
        else
        {
            $dwStatus = $(if ($fPciCfg) {
                [wdc_lib_decl]::WDC_PciWriteCfg16($hDev, ($pReg.dwOffset + $pciExpressOffset),
                    $wData) } Else {
                [wdc_lib_decl]::WDC_WriteAddr16($hDev, $pReg.dwAddrSpace, ($pReg.dwOffset +
                    $pciExpressOffset), $wData) })
        }
        if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -eq $dwStatus)
        {
            Write-Host("{0} [0x{1:x}] {2} register {3} at offset [0x{4:x}]" -f
                $(if([WDC_DIRECTION]::WDC_READ -eq $direction) { "Read" } Else { "Wrote" }),
                $wData,
                $(if([WDC_DIRECTION]::WDC_READ -eq $direction) { "from" } Else { "to" }),
                $pReg.sName,
                $(if($fExpress) { $pReg.dwOffset + $pciExpressOffset } Else { $pReg.dwOffset }))
            $dwStatus = $(if($fExpress) {
                [wdc_lib_decl]::PciExpressConfRegData2Str($hDev, $pReg.dwOffset, $details,
                    10240, [ref]$outBufLen) } Else {
                [wdc_lib_decl]::PciConfRegData2Str($hDev, $pReg.dwOffset, $details,
                    10240, [ref]$outBufLen) })
            Write-Host ("{0}" -f $(if([WDC_DIRECTION]::WDC_READ -eq $direction -and $details.Equals("")) {
                "Decoded register data: " } Else { "" }))
            Write-Host ("{0}" -f $(if([WDC_DIRECTION]::WDC_READ -eq $direction -and $details.Equals("")) {
                $details } Else { "" }))
        }
    }
    ([wdc_lib_consts]::WDC_SIZE_32)
    {
        if ([WDC_DIRECTION]::WDC_READ -eq $direction)
        {
            $dwStatus = $(if($fPciCfg) {
                [wdc_lib_decl]::WDC_PciReadCfg32($hDev, $pReg.dwOffset + $pciExpressOffset,
                    [ref]$u32Data) } Else {
                [wdc_lib_decl]::WDC_ReadAddr32($hDev, $pReg.dwAddrSpace, $pReg.dwOffset +
                    $pciExpressOffset, [ref]$u32Data) })
        }
        else
        {
            $dwStatus = $(if($fPciCfg) {
                [wdc_lib_decl]::WDC_PciWriteCfg32($hDev, $pReg.dwOffset + $pciExpressOffset,
                    $u32Data) } Else {
                [wdc_lib_decl]::WDC_WriteAddr32($hDev, $pReg.dwAddrSpace, $pReg.dwOffset +
                    $pciExpressOffset, $u32Data) })
        }
        if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -eq $dwStatus)
        {
            Write-Host("{0} [0x{1:X}] {2} register {3} at offset [0x{4:x}]" -f
                $(if([WDC_DIRECTION]::WDC_READ -eq $direction) { "Read" } Else { "Wrote" }),
                $u32Data,
                $(if([WDC_DIRECTION]::WDC_READ -eq $direction) { "from" } Else { "to" }),
                $pReg.sName,
                $(if($fExpress) { $pReg.dwOffset + $pciExpressOffset } Else { $pReg.dwOffset }))
            $dwStatus = $(if($fExpress) {
                [wdc_lib_decl]::PciExpressConfRegData2Str($hDev, $pReg.dwOffset, $details,
                    10240, [ref]$outBufLen) } Else {
                [wdc_lib_decl]::PciConfRegData2Str($hDev, $pReg.dwOffset, $details,
                    10240, [ref]$outBufLen)})
            Write-Host ("{0}" -f $(if([WDC_DIRECTION]::WDC_READ -eq $direction -and $details.Equals(""))
                { "Decoded register data: " } Else { "" }))
            Write-Host ("{0}" -f $(if([WDC_DIRECTION]::WDC_READ -eq $direction -and $details.Equals(""))
                { $details } Else { "" } ))
        }
        }
    ([wdc_lib_consts]::WDC_SIZE_64)
    {
        if ([WDC_DIRECTION]::WDC_READ -eq $direction)
        {
            $dwStatus = $(if($fPciCfg) {
                [wdc_lib_decl]::WDC_PciReadCfg64($hDev, $pReg.dwSizedwOffset + $pciExpressOffset,
                    [ref]$u64Data) } Else {
                [wdc_lib_decl]::WDC_ReadAddr64($hDev, $pReg.dwAddrSpace, $pReg.dwOffset +
                    $pciExpressOffset, [ref]$u64Data) })
        }
        else
        {
            $dwStatus = $(if($fPciCfg) {
                [wdc_lib_decl]::WDC_PciWriteCfg64($hDev, $pReg.dwOffset + $pciExpressOffset,
                    $u64Data) } Else {
                [wdc_lib_decl]::WDC_WriteAddr64($hDev, $pReg.dwAddrSpace, $pReg.dwOffset +
                    $pciExpressOffset, $u64Data) })
        }
        if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -eq $dwStatus)
        {
            Write-Host("{0} [0x{1:X}] {2} register {3} at offset [0x{4:x}]",
                $(if([WDC_DIRECTION]::WDC_READ -eq $direction) { "Read" } Else { "Wrote" }),
                $u64Data,
                $(if([WDC_DIRECTION]::WDC_READ -eq $direction) { "from" } Else { "to" }),
                $pReg.sName,
                $(if($fExpress) { $pReg.dwOffset + $pciExpressOffset } Else { $pReg.dwOffset }))
            $dwStatus = $(if($fExpress) {
                [wdc_lib_decl]::PciExpressConfRegData2Str($hDev, $pReg.dwOffset +
                    $pciExpressOffset, $details, 10240, [ref]$outBufLen) } Else {
                [wdc_lib_decl]::PciConfRegData2Str($hDev, $pReg.dwOffset, $details,
                    10240, [ref]$outBufLen) })
            Write-Host ("{0}" -f $(if([WDC_DIRECTION]::WDC_READ -eq $direction -and $details.Equals("")) {
                "Decoded register data: " } Else { "" }))
            Write-Host ("{0}" -f $(if([WDC_DIRECTION]::WDC_READ -eq $direction -and $details.Equals("")) {
                $details } Else { "" }))
        }
        }

    default
    {
        Write-Host("Invalid register size ({0})" -f $pReg.dwSize)
    }
    }

    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
    {
        Write-Host("Failed {0} {1}. Error [0x{2:x} - {3}]" -f
            $(if([WDC_DIRECTION]::WDC_READ -eq $direction) { "reading data from" } Else { "writing data to" },
            $pReg.sName, $dwStatus, [utils]::Stat2Str($dwStatus)))
    }

    Write-Host("")
    Pause
}

<# Display available PCI/PCIe capabilities #>
function WDC_DIAG_ScanPCICapabilities($hDev)
{
    $scanResult = New-Object -TypeName "Jungo.wdapi_dotnet.WDC_PCI_SCAN_CAPS_RESULT"
    $dwCapId = [windrvr_consts]::WD_PCI_CAP_ID_ALL
    $option = 0

    if (!$hDev)
    {
        WDC_DIAG_ERR("WDC_DIAG_ScanPCICapabilities: Error - NULL WDC device " +
            "handle")
        return
    }

    Write-Host("")
    Write-Host("Select scan option (PCI/PCI-Express, all/specific):")
    Write-Host("-----------------------")
    Write-Host("1. Scan PCI capabilities")
    Write-Host("2. Scan specific PCI capability")
    Write-Host("3. Scan PCI Express extended capabilities")
    Write-Host("4. Scan specific PCI Express extended capability")
    Write-Host("")

    if ($DIAG_INPUT_FAIL -eq (DIAG_GetMenuOption ([ref]$option) 4))
    {
        return
    }

    if ($option -eq 2 -or $option -eq 4)
    {
        $sMsg = ("Enter requested {0}capability ID (hexadecimal): 0x" -f
            $(if ($option -eq 4) {"extended "} Else {""}))

        if ($DIAG_INPUT_FAIL -eq (DIAG_InputNum ([ref]$dwCapId) $sMsg $FALSE 0 1024))
        {
            return
        }
    }

    <# Scan PCI/PCIe Capabilities #>
    if ($option -le 2)
    {
        $dwStatus = [wdc_lib_decl]::WDC_PciScanCaps($hDev, $dwCapId,
            [ref]$scanResult)
    }
    else
    {
        $dwStatus = [wdc_lib_decl]::WDC_PciScanExtCaps($hDev, $dwCapId,
            [ref]$scanResult)
    }

    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
    {
        WDC_DIAG_ERR("WDC_DIAG_ScanPCICapabilities: Failed scanning PCI " +
            "capabilities. Error [0x{0:X} - {1}]" -f $dwStatus, [utils]::Stat2Str($dwStatus))
        return
    }

    Write-Host("{0} PCI{1}capabilities found" -f `
        $(if ($scanResult.dwNumCaps) { $scanResult.dwNumCaps } Else { "No" }),
        $(if ($option -eq 3 -or $option -eq 4) { " Express extended " } Else { " " }))

    for ($i = 0; $i -lt $scanResult.dwNumCaps; $i++)
    {
        Write-Host("    {0}) {1} - ID [0x{2:x}], offset [0x{3:x}]" -f ($i + 1),
            $(if ($option -eq 3 -or $option -eq 4)
            { [wdc_lib_macros]::GetExtendedCapabilityStr($scanResult.pciCaps[$i].dwCapId) } Else
            { [wdc_lib_macros]::GetExtendedCapabilityStr($scanResult.pciCaps[$i].dwCapId) }),
            $scanResult.pciCaps[$i].dwCapId, $scanResult.pciCaps[$i].dwCapOffset)
    }
}

function WDC_DIAG_IntTypeDescriptionGet($dwIntType)
{
    if ($dwIntType -band [int][WD_INTERRUPT_TYPE]::INTERRUPT_MESSAGE_X)
    {
        return "Extended Message-Signaled Interrupt (MSI-X)"
    }
    if ($dwIntType -band [int][WD_INTERRUPT_TYPE]::INTERRUPT_MESSAGE)
    {
        return "Message-Signaled Interrupt (MSI)"
    }
    if ($dwIntType -band [int][WD_INTERRUPT_TYPE]::INTERRUPT_LEVEL_SENSITIVE)
    {
        return "Level-Sensitive Interrupt"
    }
    return "Edge-Triggered Interrupt"
}

<# -----------------------------------------------
    PCI
   ----------------------------------------------- #>

function fprintf()
{
    if ($args[0])
    {
        Write-Output $args[1] >> $args[0]
    }
    else
    {
        Write-Host $args[1]
    }
}

<# Print PCI device location information #>
function WDC_DIAG_PciSlotPrint($pPciSlot)
{
    WDC_DIAG_PciSlotPrintFile $pPciSlot $FALSE
}

<# Print PCI device location information to file #>
function WDC_DIAG_PciSlotPrintFile($pPciSlot, $fp)
{
    if (!$pPciSlot)
    {
        WDC_DIAG_ERR("WDC_DIAG_PciSlotPrint: Error - NULL PCI slot pointer")
        return
    }

    fprintf $fp ("Domain [0x{0:x}], Bus [0x{1:x}], Slot [0x{2:x}], Function [0x{3:x}]" -f
        $pPciSlot.dwDomain, $pPciSlot.dwBus, $pPciSlot.dwSlot, $pPciSlot.dwFunction)
}

<# Print PCI device location and $resources information #>
function WDC_DIAG_PciDeviceInfoPrint($pPciSlot, $dump_cfg)
{
    WDC_DIAG_PciDeviceInfoPrintFile $pPciSlot $NULL $dump_cfg
}

<# Print PCI device location and $resources information to file #>
function WDC_DIAG_PciDeviceInfoPrintFile([WD_PCI_SLOT]$pPciSlot, $fp, $dump_cfg)
{
    if (!$pPciSlot)
    {
        WDC_DIAG_ERR("WDC_DIAG_PciDeviceInfoPrint: Error - NULL PCI slot " +
            "pointer")
        return
    }

    fprintf $fp "    Location: "

    WDC_DIAG_PciSlotPrintFile $pPciSlot $fp

    if ($dump_cfg)
    {
        $dwOffset = 0

        for ($dwOffset = 0; $dwOffset -lt 256; $dwOffset += 8)
        {
            $dwStatus = [wdc_lib_decl]::WDC_PciReadCfgBySlot32($pPciSlot, $dwOffset, [ref]$config)
            if ($dwStatus)
            {
                WDC_DIAG_ERR ("    Failed reading PCI configuration space." +
                    "    Error [0x{0:x} - {1}]" -f $dwStatus, [utils]::Stat2Str($dwStatus))
                return
            }

            if (($dwOffset / 4) % 8 -eq 0)
            {
                fprintf $fp ("%02lx " -f $dwOffset)
            }
            fprintf $fp "%08x " -f $config
            if (($dwOffset / 4) % 8 -eq 7)
            {
                fprintf $fp ""
            }
        }
    }

    $deviceInfo = New-Object -TypeName "Jungo.wdapi_dotnet.WD_PCI_CARD_INFO"
    $deviceInfo.pciSlot = $pPciSlot

    $dwStatus = [wdc_lib_decl]::WDC_PciGetDeviceInfo([ref]$deviceInfo)
    if (([int][WD_ERROR_CODES]::WD_NO_RESOURCES_ON_DEVICE -ne $dwStatus) -and
        ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus))
    {
        WDC_DIAG_ERR "    Failed retrieving PCI resources information." +
            "    Error 0x{0:x} - {1}" -f $dwStatus, [utils]::Stat2Str($dwStatus)
        return
    }

    WDC_DIAG_DeviceResourcesPrint $deviceInfo.Card 0 $fp

    $dwExpressGen = [wdc_lib_decl]::WDC_PciGetExpressGenBySlot([ref]$pPciSlot)
    if ($dwExpressGen)
    {
        fprintf $fp ("    PCI Express Generation: Gen{0}" -f $dwExpressGen)
    }
}

<# Print location and $resources information for all connected PCI devices to
 * file #>
function WDC_DIAG_PciDevicesInfoPrintAllFile
{
    $scanResult = New-Object -TypeName "Jungo.wdapi_dotnet.WDC_PCI_SCAN_RESULT"
    $dwStatus = [wdc_lib_decl]::WDC_PciScanDevices(0, 0, [ref]$scanResult)

    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
    {
        fprintf $fp ("Failed scanning PCI bus. Error [0x{0:x} - {1}]" -f $dwStatus,
            ([utils]::Stat2Str($dwStatus)))
        return
    }

    $dwNumDevices = $scanResult.dwNumDevices
    if (!$dwNumDevices)
    {
        fprintf $fp "No devices were found on the PCI bus"
        return
    }

    fprintf $fp ""
    fprintf $fp ("Found {0} devices on the PCI bus:" -f $dwNumDevices)
    fprintf $fp "---------------------------------"

    for ($i = 0; $i -lt $dwNumDevices; $i++)
    {
        fprintf $fp ("{0}. Vendor ID: [0x{1:X}], Device ID: [0x{2:X}]" `
            -f ($i + 1), $scanResult.deviceId[$i].dwVendorId,
            $scanResult.deviceId[$i].dwDeviceId)

        WDC_DIAG_PciDeviceInfoPrintFile $scanResult.deviceSlot[$i] $fp `
            $dump_cfg

        if (!$fp)
        {
            Read-Host -Prompt "Press ENTER to proceed to next device"
        }
        fprintf $fp ""
    }
}

<# Print location and $resources information for all connected PCI devices #>
function WDC_DIAG_PciDevicesInfoPrintAll($dump_cfg)
{
    WDC_DIAG_PciDevicesInfoPrintAllFile $False $dump_cfg
}
