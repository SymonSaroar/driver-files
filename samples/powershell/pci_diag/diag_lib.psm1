<# Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com #>

<#*****************************************************************************
*  File: diag_lib.psm1 - Implementation of shared WD user-mode diagnostics API.
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
******************************************************************************#>

<#  NOTE: pci_diag.ps1 imports diag_lib.psm1 from
          $(MY_DOCUMENTS)/WindowsPowerShell\Modules\diag_lib
          if you wish to modify the script, you can copy this script to the that
          path #>

<#************************************************************
  General definitions
 ************************************************************#>
<# Error messages display #>
#define DIAG_ERR Write-Output
enum DIAG_INPUT_RESULT{
    DIAG_INPUT_CANCEL = -1
    DIAG_INPUT_FAIL = 0
    DIAG_INPUT_SUCCESS = 1
}

$DIAG_CANCEL = 'x'

<# Exit menu #>
$DIAG_EXIT_MENU = 99

function DIAG_ERR{ $Host.UI.WriteErrorLine($args) }
<#************************************************************
  Functions implementation
 ************************************************************#>
$BYTES_IN_LINE = 16
$HEX_CHARS_PER_BYTE = 3
$HEX_STOP_POS = $BYTES_IN_LINE * $HEX_CHARS_PER_BYTE

function DIAG_PrintHexBuffer($pbData, $dwBytes, [BOOL]$fAscii)
{
    $pHex = [Char[]]::new($HEX_STOP_POS + 1)
    $pAscii = [Char[]]::new($BYTES_IN_LINE + 1)
    #dwLineOffset

    if (!$pbData -or !$dwBytes)
    {
        DIAG_ERR("DIAG_PrintHexBuffer: Error - {0}" -f
            $(if (!$pBuf) { "NULL buffer pointer" } Else { "Empty buffer"}))
        return
    }

    for ($dwOffset = 0 ;$dwOffset -lt $dwBytes; $dwOffset+=$BYTES_IN_LINE)
    {
        ($pbData[($dwOffset)..($dwOffset + $BYTES_IN_LINE)]|ForEach-Object ToString X2) -join ' '
    }
}

function DIAG_GetHexBuffer([ref][Byte[]]$pBuffer)
{
    $dwBytesRead = 0
    $number = 0

    $str = Read-Host
    $str = $str.Replace(" ", "").Trim()
    $dwBytes = [Math]::Max(1, [Math]::Min($str.Length / 2,
        $pBuffer.value.Length))

    for ($i = 0; $i -lt $dwBytes; $i++)
    {
        if ([BYTE]::TryParse($str.Substring($i * 2, [Math]::Min($str.Length, 2)),
            [Globalization.NumberStyles]::HexNumber,
            [IFormatProvider]::InvariantCulture, [ref]$number))
        {
            $pBuffer.value[$dwBytesRead] = [BYTE]$number
            $dwBytesRead++
        }
        else
        {
            return -1;
        }
    }

    <# Return the number of bytes that was read #>
    return $dwBytesRead
}

<# Get menu option from user #>
function DIAG_GetMenuOption([ref]$pdwOption, $dwMax)
{
    if (!$pdwOption)
    {
        PCI_ERR("DIAG_GetMenuOption: Error - NULL option pointer")
        return [DIAG_INPUT_RESULT]::DIAG_INPUT_FAIL
    }

    $sInput = Read-Host -Prompt "Enter option"

    $pdwOption.value = $sInput

    if ($sInput -eq $DIAG_EXIT_MENU) {
        return [DIAG_INPUT_RESULT]::DIAG_INPUT_SUCCESS
    }

    if ((1..$dwMax) -notcontains $sInput)
    {
        Write-Output("Invalid option: Option must be {0}{1:X}, or {2} to exit" -f
        $(if (1 -eq $dwMax) {""} Else {"between 1 - "}), $dwMax, $DIAG_EXIT_MENU)
        $pdwOption.value = 0
        return [DIAG_INPUT_RESULT]::DIAG_INPUT_FAIL
    }

    return [DIAG_INPUT_RESULT]::DIAG_INPUT_SUCCESS
}

function DIAG_InputNum([ref]$pInput, [string]$sInputText, [BOOL]$fHex,
    [UINT64]$min, [UINT64]$max)
{
    [BOOL]$fCheckRange = ($max -gt $min)
    $iRet = 0

    if (!$pInput)
    {
        DIAG_ERR("DIAG_InputData: Error - NULL input pointer")
        return [DIAG_INPUT_RESULT]::DIAG_INPUT_FAIL
    }

    $sMsg = ("{0} (to cancel press '{1}'): {2}" -f
        $(if (!$sInputText -or ($sInputText.Equals(""))) { "Enter input" } Else { $sInputText }),
        $DIAG_CANCEL, $(if ($fHex) { "0x" } Else { "" }))

    $sInput = Read-Host $sMsg
    if ($sInput.Equals("") -or $DIAG_CANCEL -eq $sInput[0])
    {
        return [DIAG_INPUT_RESULT]::DIAG_INPUT_CANCEL
    }

    if ($fHex)
    {
        if(![UInt32]::TryParse($sInput, [Globalization.NumberStyles]::HexNumber,
            [IFormatProvider]::InvariantCulture, [ref]$iRet))
        {
            Write-Output("Invalid input")
            return [DIAG_INPUT_RESULT]::DIAG_INPUT_FAIL
        }
    }
    else
    {
        $iRet = [UInt64]$sInput
    }

    if (!$fHex -and $iRet -lt 1)
    {
        Write-Output("Invalid input")
        return [DIAG_INPUT_RESULT]::DIAG_INPUT_FAIL
    }

    if ($fCheckRange)
    {
        [UINT64]$tmp = $iRet

        if ($tmp -lt $min -or $tmp -gt $max)
        {
            Write-Output("Invalid input: Input must be between ")
            if ($fHex)
            {
                Write-Output("0x{0:X} and 0x{1:X}" -f $min, $max)
            }
            else
            {
                Write-Output("{0} and {1}" -f $min, $max)
            }
            return [DIAG_INPUT_RESULT]::DIAG_INPUT_FAIL
        }
    }
    $pInput.value = $iRet

    return [DIAG_INPUT_RESULT]::DIAG_INPUT_SUCCESS
}



