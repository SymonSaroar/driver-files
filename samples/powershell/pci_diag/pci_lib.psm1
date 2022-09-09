<# Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com #>

<#***********************************************************************
*  File: pci_lib.c
*
*  Library for accessing PCI devices, possibly using a Kernel PlugIn driver.
*  The code accesses hardware using WinDriver's WDC library.

*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
************************************************************************#>

<#  NOTE: pci_diag.ps1 imports pci_lib.psm1 from
          $(MY_DOCUMENTS)/WindowsPowerShell\Modules\pci_lib
          if you wish to modify the script, you can copy this script to the that
          path #>

using namespace Jungo.wdapi_dotnet

if ($IsWindows)
{
    [Reflection.Assembly]::LoadFile(($pwd).path +
        "/../../../lib/amd64/wdapi_netcore1511.dll")
}
else
{
    [Reflection.Assembly]::LoadFile(($pwd).path +
        "/../../../lib/wdapi_netcore1511.dll")
}


<# *************************************************************
  General definitions
   ************************************************************* #>
# Kernel PlugIn driver name (should be no more than 8 characters)
$KP_PCI_DRIVER_NAME = "KP_PCI"

# Kernel PlugIn messages - used in WDC_CallKerPlug() calls (user mode)
# KP_PCI_Call() (kernel mode)
$KP_PCI_MSG_VERSION = 1 # Query the version of the Kernel PlugIn

# Kernel PlugIn messages status
enum KP_PCI_STATUS {
    OK          = 0x1
    MSG_NO_IMPL = 0x1000
    FAIL        = 0x1001
}

# Default vendor and device IDs (0 == all)
# TODO: Replace the ID values with your device's vendor and device IDs
$PCI_DEFAULT_VENDOR_ID = 0x0 # Vendor ID
$PCI_DEFAULT_DEVICE_ID = 0x0 # Device ID

# Interrupt acknowledgment information
# TODO: Use correct values according to the specification of your device.
$INTCSR = 0x00                    # Interrupt register
$INTCSR_ADDR_SPACE = $AD_PCI_BAR0 # Interrupt register's address space
$ALL_INT_MASK = 0xFFFFFFFF        # Interrupt acknowledgment command

# Device address description struct
class PCI_DEV_ADDR_DESC {
    [UInt32]$dwNumAddrSpaces # Total number of device address spaces
    [IntPtr]$pAddrDesc       # Array of device address spaces information
}

# Kernel PlugIn version information struct
class KP_PCI_VERSION {
    [UInt32]$dwVer
    [String]$cVer
}

<#************************************************************
  Internal definitions
 ************************************************************#>
$WD_VERSION_STR = "1511"
$WD_PROD_NAME = "windrvr" + $WD_VERSION_STR
$WD_DEFAULT_DRIVER_NAME_BASE = $WD_PROD_NAME
<# WinDriver license registration string #>
<# TODO: When using a registered WinDriver version, replace the license string
         below with the development license in order to use on the development
         machine.
         Once you require to distribute the driver's package to other machines,
         please replace the string with a distribution license #>
$PCI_DEFAULT_LICENSE_STRING = "12345abcde1234.license"

$PCI_DEFAULT_DRIVER_NAME = $WD_DEFAULT_DRIVER_NAME_BASE

class PCI_INT_RESULT {
    [UInt32]$Counter    # Number of interrupts received
    [UInt32]$dwLost     # Number of interrupts not yet handled
    [Uint32]$waitResult # See WD_INTERRUPT_WAIT_RESULT values
                        #  in windrvr.h
    [UInt32]$dwEnabledIntType # Interrupt type that was actually enabled
                              #  (MSI/MSI-X / Level Sensitive / Edge-Triggered)
    [UInt32]$dwLastMessage # Message data of the last received MSI/MSI-X
                           #  (Windows Vista and higher); N/A to line-based
                           #  interrupts.
}

<#************************************************************
  Global variables definitions
 ************************************************************#>
<# Last error information string #>
[string]$gsPCI_LastErr = ""

<# Library initialization reference count #>
[int]$LibInit_count = 0

<#************************************************************
  Inline implementation
 ************************************************************#>
function PCI_ERR{ $Host.UI.WriteErrorLine($args) }

<# Validate a device handle #>
function IsValidDevice($pDev, $sFunc)
{
    if (!$pDev)
    {
       $gsPCI_LastErr = "{0}: NULL device {1}" -f $sFunc,
           $(if (!$pDev) { "handle" } Else { "context" })
       ErrLog $gsPCI_LastErr
       return $FALSE
    }

    return $TRUE
}

<#************************************************************
  Functions implementation
 ************************************************************#>
<# -----------------------------------------------
    PCI and WDC libraries initialize/uninitialize
   ----------------------------------------------- #>
<# Initialize the PCI and WDC libraries #>
function PCI_LibInit()
{
    <# Increase the library's reference count; initialize the library only once
     #>
    if (++$LibInit_count -gt 1)
    {
        return [int][WD_ERROR_CODES]::WD_STATUS_SUCCESS
    }

    <# Set the driver name #>
    if (![windrvr_decl]::WD_DriverName($PCI_DEFAULT_DRIVER_NAME))
    {
        ErrLog("Failed to set the driver name for WDC library.")
        return [int][WD_ERROR_CODES]::WD_SYSTEM_INTERNAL_ERROR
    }

    <# Set WDC library's debug options
       (default: level=TRACE; redirect output to the Debug Monitor) #>
    $dwStatus = [wdc_lib_decl]::WDC_SetDebugOptions(`
        [wdc_lib_consts]::WDC_DBG_DEFAULT, $NULL)
    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
    {
        ErrLog("Failed to initialize debug options for WDC library." +
            "Error 0x{0:x} - {1}" -f ($dwStatus, [utils]::Stat2Str($dwStatus)))
        return $dwStatus
    }

    <# Open a handle to the driver and initialize the WDC library #>
    $dwStatus = [wdc_lib_decl]::WDC_DriverOpen([wdc_lib_consts]::WDC_DRV_OPEN_DEFAULT,
        $PCI_DEFAULT_LICENSE_STRING)
    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
    {
        ErrLog("Failed to initialize the WDC library. Error 0x{0:x} - {1}" -f
            $dwStatus, [utils]::Stat2Str($dwStatus))
        return $dwStatus
    }
    return [int][WD_ERROR_CODES]::WD_STATUS_SUCCESS
}

<# Uninitialize the PCI and WDC libraries #>
function PCI_LibUninit()
{
    <# Decrease the library's reference count; uninitialize the library only
     * when there are no more open handles to the library #>
    if (--$LibInit_count -gt 0)
    {
        return [int][WD_ERROR_CODES]::WD_STATUS_SUCCESS
    }

    <# Uninitialize the WDC library and close the handle to WinDriver #>
    $dwStatus = [wdc_lib_decl]::WDC_DriverClose()
    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
    {
        ErrLog "Failed to uninit the WDC library. Error 0x{0:x} - {1}" -f
            $dwStatus, [utils]::Stat2Str($dwStatus)
    }
    return $dwStatus
}

<# -----------------------------------------------
    Device open/close
   ----------------------------------------------- #>
<# Close device handle #>
function PCI_DeviceClose([WDC_DEVICE]$Dev)
{
    TraceLog ("PCI_DeviceClose: Entered. Device handle 0x{0:x}" -f $Dev.hDev)

    <# Validate the device handle #>
    if (!$Dev)
    {
        ErrLog "PCI_DeviceClose: Error - NULL device object"
        return $FALSE
    }

    $pDevCtx = [wdc_lib_decl]::WDC_GetDevContext($Dev.hDev)

    <# Disable interrupts (if enabled) #>
    if ([wdc_lib_decl]::WDC_IntIsEnabled($Dev.hDev))
    {
        $dwStatus = PCI_IntDisable $Dev
        if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
        {
            ErrLog("Failed disabling interrupts. Error 0x{0:x} - {1}" -f $dwStatus,
                [utils]::Stat2Str($dwStatus))
        }
    }

    <# Close the device handle #>
    $dwStatus = [wdc_lib_decl]::WDC_PciDeviceClose($Dev.hDev)
    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
    {
        ErrLog("Failed closing a WDC device handle (0x{0:x}). Error 0x{1:x} - {2}" -f
            $Dev.hDev, $dwStatus, [utils]::Stat2Str($dwStatus))
    }

    return ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -eq  $dwStatus)
}

<# Open a device handle #>
function PCI_DeviceOpen($pDeviceInfo)
{
    #PPCI_DEV_CTX $pDevCtx = $NULL
    $wdcDevice = New-Object -TypeName "Jungo.wdapi_dotnet.WDC_DEVICE"
    $hDev = [System.IntPtr]::Zero
<#  #>
    #[PCI_DEV_ADDR_DESC]$devAddrDesc = New-Object -TypeName "PCI_DEV_ADDR_DESC"
<#  #>

    <# Validate arguments #>
    if (!$pDeviceInfo)
    {
        ErrLog("PCI_DeviceOpen: Error - NULL device information " +
            "struct pointer")
        return $NULL
    }

    <# Allocate memory for the PCI device context #>
    $pDevCtx = [System.IntPtr]::Zero
    #$pDevCtx = (PPCI_DEV_CTX)malloc(sizeof(PCI_DEV_CTX))
    #if (!$pDevCtx)
    #{
    #    ErrLog("Failed allocating memory for PCI device context")
    #    return [System.IntPtr]::Zero
    #}

    <# Open a device handle #>
    $dwStatus = [wdc_lib_decl]::WDC_PciDeviceOpen([ref]$hDev,
        [ref]$pDeviceInfo, $pDevCtx)
    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
    {
        ErrLog("Failed opening a WDC device handle. Error 0x{0:x} - {1}" -f
            $dwStatus, [utils]::Stat2Str($dwStatus))
        return
    }

    $wdcDevice = [WDC_DEVICE]::hDevToWdcDevice($hDev)
<#  #>
    #$devAddrDesc.dwNumAddrSpaces = $Dev.dwNumAddrSpaces
    #$devAddrDesc.pAddrDesc = $Dev

    <# Open a handle to a Kernel PlugIn driver #>
    #[wdc_lib_decl]::WDC_KernelPlugInOpen($hDev, $KP_PCI_DRIVER_NAME, [ref]$devAddrDesc)
<#  #>

    <# Validate device information #>
    #if (!(DeviceValidate $hDev))
    #{
    #    return $NULL
    #}

    <# Return handle to the new device #>
    #TraceLog(("PCI_DeviceOpen: Opened a PCI device (handle 0x{0:x})" +
    #    "Device is {1} using a Kernel PlugIn driver ({2})" ) -f $hDev,
    #    $(if ([wdc_defs_macros]::WDC_IS_KP($hDev)) { "" } Else { "not" }),
    #    $KP_PCI_DRIVER_NAME)
    return $wdcDevice

Error:
    if (!$hDev)
    {
        PCI_DeviceClose $hDev
    }
    #else
    #    free(pDevCtx)

    return $NULL
}

<# Validate device information #>
function DeviceValidate($pDev)
{
    $dwNumAddrSpaces = $pDev.dwNumAddrSpaces

    <# NOTE: You can modify the implementation of this function in order to
             verify that the device has the resources you expect to find. #>

    <# Verify that the device has at least one active address space #>
    for ($i = 0; $i -lt $dwNumAddrSpaces; $i++)
    {
        if ([wdc_lib_decl]::WDC_AddrSpaceIsActive($pDev, $i))
        {
            return $TRUE
        }
    }

    <# In this sample we accept the device even if it doesn't have any
     * address spaces #>
    TraceLog("Device does not have any active memory or I/O address spaces")
    return $TRUE
}

<# -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- #>
<#
$wdcEventMarshaler = New-Object "MarshalWdEvent"

# Plug-and-play or power management event handler routine
function PCI_EventHandler([IntPtr]$pEvent, [Object]$pDevCtx)
{
    #[WD_EVENT]$wdEvent = [WD_EVENT]$wdcEventMarshaler.MarshalNativeToManaged($pEvent)
    #PWDC_DEVICE $pDev = (PWDC_DEVICE)$pData
    #PPCI_DEV_CTX $pDevCtx = (PPCI_DEV_CTX)(pDev.pCtx)

    #TraceLog("PCI_EventHandler entered, pData 0x{0}, dwAction 0x{1:x}" -f $pData,
    #    $pEvent.dwAction)
    #Write-Host $wdEvent
    Write-Host "PCI_EVENT_HANDLER"
    Write-Host "PCI_EventHandler entered, dwAction 0x{0:x}" -f $pEvent

    # Execute the diagnostics application's event handler function
    #& $pDevCtx($pEvent.dwAction)
}
#>

<#
$g_eventHandler = ${function:PCI_EventHandler}
# Register a plug-and-play or power management event
function PCI_EventRegister($Dev, $funcEventHandler)
{
    $hDev = $dev.hDev
    $dwActions = [windrvr_consts]::WD_ACTIONS_ALL
    # TODO: Modify the above to set up the plug-and-play/power management
    #         events for which you wish to receive notifications.
    #         $dwActions can be set to any combination of the WD_EVENT_ACTION
    #         flags defined in windrvr.h.

    TraceLog("PCI_EventRegister entered. Device handle 0x{0:x}" -f $hDev)

    # Validate the device handle
    if (!(IsValidDevice $hDev "PCI_EventRegister"))
    {
        return [int][WD_ERROR_CODES]::WD_INVALID_PARAMETER
    }

    # Check whether the event is already registered
    if ([wdc_lib_decl]::WDC_EventIsRegistered($hDev))
    {
        ErrLog("Events are already registered ...")
        return [int][WD_ERROR_CODES]::WD_OPERATION_ALREADY_DONE
    }

    # Store the diag event handler routine to be executed from
    #   PCI_EventHandler() upon an event
    $PS_Event = [wdapi_ps.PS_EventHandler]::new($funcEventHandler)

    # Register the event

    $dwStatus = $PS_Event.WDC_EventRegister($Dev, $dwActions,
        $g_eventHandler, $hDev, [wdc_defs_macros]::WDC_IS_KP($Dev))

    #$dwStatus = [wdc_lib_decl]::WDC_EventRegister($hDev, $dwActions,
    #    $PCI_EventHandler, $hDev, [wdc_defs_macros]::WDC_IS_KP($hDev))

    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
    {
        ErrLog("Failed to register events. Error 0x{0:X} - {1}" -f
            $dwStatus, [utils]::Stat2Str($dwStatus))
        return $dwStatus
    }

    TraceLog("Events registered")
    return [int][WD_ERROR_CODES]::WD_STATUS_SUCCESS
}
#>

# Unregister a plug-and-play or power management event
<#function PCI_EventUnregister($Dev)
{
    $hDev = $Dev.hDev
    TraceLog("PCI_EventUnregister entered. Device handle 0x{0:x}" -f
        $hDev)

    # Validate the device handle
    if (!(IsValidDevice $hDev "PCI_EventUnregister"))
    {
        return [WD_ERROR_CODES]::WD_INVALID_PARAMETER
    }
    # Check whether the event is currently registered
    if (![wdc_lib_decl]::WDC_EventIsRegistered($hDev))
    {
        ErrLog("Cannot unregister events - no events currently " +
            "registered ...")
        return [WD_ERROR_CODES]::WD_OPERATION_ALREADY_DONE
    }

    # Unregister the event
    $dwStatus = [wdc_lib_decl]::WDC_EventUnregister($Dev)
    if ([int][WD_ERROR_CODES]::WD_STATUS_SUCCESS -ne $dwStatus)
    {
        ErrLog("Failed to unregister events. Error 0x{0:X} - {1}", $dwStatus,
            [utils]::Stat2Str($dwStatus))
    }

    return $dwStatus
}#>

# Check whether a given plug-and-play or power management event is registered
<#
function PCI_EventIsRegistered($hDev)
{
    # Validate the device handle
    if (!(IsValidDevice $hDev "PCI_EventIsRegistered"))
    {
        return $FALSE
    }
    # Check whether the event is registered
    return [wdc_lib_decl]::WDC_EventIsRegistered($hDev)
}
#>
<# -----------------------------------------------
    Address spaces information
   ----------------------------------------------- #>

<# Get number of address spaces #>
function PCI_GetNumAddrSpaces($Dev)
{
    <# Validate the device handle #>
    if (!(IsValidDevice $Dev.hDev "PCI_GetNumAddrSpaces"))
    {
        return 0
    }
    <# Return the number of address spaces for the device #>
    return $Dev.dwNumAddrSpaces
}
#endif <# ifndef ISA #>

<# Get address space information #>
function PCI_GetAddrSpaceInfo($Dev, [ref]$pAddrSpaceInfo)
{
    $hDev = $Dev.hDev

    <#  #>
    $dwAddrSpace = $pAddrSpaceInfo.value.dwAddrSpace

    if ($dwAddrSpace -gt ($Dev.dwNumAddrSpaces - 1))
    {
        ErrLog("PCI_GetAddrSpaceInfo: Error - Address space" +
            " {0:x} is out of range (0 - {1:x})" -f $dwAddrSpace,
             ($Dev.dwNumAddrSpaces - 1))
        return $FALSE
    }

    $pAddrDesc = $Dev.pAddrDesc[$dwAddrSpace]

    $fIsMemory = [wdc_defs_macros]::WDC_ADDR_IS_MEM($pAddrDesc)


    $pAddrSpaceInfo.value.sName = "BAR {0}" -f $dwAddrSpace
    $pAddrSpaceInfo.value.sType = $(if ($fIsMemory) { "Memory" } Else { "I/O" })

    if ([wdc_lib_decl]::WDC_AddrSpaceIsActive($hDev, $dwAddrSpace))
    {
        [WD_ITEMS]$pItem = $Dev.cardReg.Card.Item[$pAddrDesc.dwItemIndex]
        $pAddr = $(if ($fIsMemory) { $pItem.I.Mem.pPhysicalAddr} Else
            { $pItem.I.IO.pAddr })

        $pAddrSpaceInfo.value.sDesc = "0x{0:x} - 0x{1:x} (0x{2:x} bytes)" -f
            $pAddr, ($pAddr + $pAddrDesc.qwBytes - 1), $pAddrDesc.qwBytes
    }
    else
    {
        $pAddrSpaceInfo.value.sDesc = "Inactive address space"
    }

    <# TODO: You can modify the code above to set a different address space
     * name/description. #>

    return $TRUE
}
<#  #>

<# -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- #>
<# Log a debug error message #>
function ErrLog()
{
   [wdc_lib_decl]::WDC_Err("PCI lib(PS): {0}\n" -f $gsPCI_LastErr)
}

<# Log a debug trace message #>
function TraceLog($sMsg)
{
    [wdc_lib_decl]::WDC_Trace("PCI lib(PS): {0}`n" -f $sMsg)
}

<# Get last error #>
function PCI_GetLastErr()
{
    return $gsPCI_LastErr
}



