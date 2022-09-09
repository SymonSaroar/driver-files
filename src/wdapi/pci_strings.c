#include <stdio.h>
#include "pci_strings.h"
#include "status_strings.h"

#define P(str, ...) p(pBuf, dwInLen, pdwOutLen, str, ##__VA_ARGS__)

static void p(PCHAR pBuf, DWORD dwInLen, DWORD *pdwOutLen, PCHAR format, ...)
{
    va_list argptr;
    char temp[512];

    va_start(argptr, format);
    vsprintf(temp, format, argptr);
    va_end(argptr);

    *pdwOutLen += (DWORD)strlen(temp);
    if (*pdwOutLen > dwInLen || strlen(temp) + strlen(pBuf) + 1 >= dwInLen)
        return;

    strncat(pBuf, temp, strlen(pBuf));
}

static void PciCmdRegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen, DWORD *pdwOutLen,
    BOOL fIsPciExpress)
{
    P("I/O Space Response Enable: ");
    P(data & PCI_COMMAND_IO ? "True\n": "False\n");
    P("Memory Space Response Enable: ");
    P(data & PCI_COMMAND_MEMORY ? "True\n": "False\n");
    P("Bus Master Enable: ");
    P(data & PCI_COMMAND_MASTER ? "True\n": "False\n");

    if (!fIsPciExpress)
    {
        P("Special Cycle Enable: ");
        P(data & PCI_COMMAND_SPECIAL ? "True\n": "False\n");
        P("Memory Write and Invalidate: ");
        P(data & PCI_COMMAND_MEMORY ? "True\n": "False\n");
        P("VGA Palette Snoop: ");
        P(data & PCI_COMMAND_VGA_PALETTE ? "True\n": "False\n");
    }

    P("Parity Error Response: ");
    P(data & PCI_COMMAND_PARITY ? "True\n": "False\n");

    if (!fIsPciExpress)
    {
        P("IDSEL Stepping/Wait Cycle Control: ");
        P(data & PCI_COMMAND_WAIT ? "True\n": "False\n");
    }

    P("SERR# Enable: ");
    P(data & PCI_COMMAND_SERR ? "True\n": "False\n");

    if (!fIsPciExpress)
    {
        P("Fast Back-to-Back Transactions Enable: ");
        P(data & PCI_COMMAND_FAST_BACK ? "True\n": "False\n");
    }

    P("Interrupt Disable: ");
    P(data & PCI_COMMAND_INTX_DISABLE ? "True\n": "False\n");
}

static void PciStatRegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen, DWORD *pdwOutLen,
    BOOL fIsPciExpress)
{
    P("Interrupt Status: ");
    P(data & PCI_STATUS_INTERRUPT ? "True\n": "False\n");

    P("Capabilities List: ");
    P(data & PCI_STATUS_CAP_LIST ? "Exists\n" : "Doesn't exist\n");

    if (!fIsPciExpress)
    {
        P("66Mhz Capable: ");
        P(data & PCI_STATUS_66MHZ ? "True\n": "False\n");

        P("Fast Back-to-Back Transactions Capable: ");
        P(data & PCI_STATUS_FAST_BACK ? "True\n": "False\n");
    }

    P("Master Data Parity Error: ");
    P(data & PCI_STATUS_PARITY ? "True\n": "False\n");

    if (!fIsPciExpress)
    {
        P("DEVSEL Timing: ");
        switch ((data & PCI_STATUS_DEVSEL_MASK) >> PCI_STATUS_DEVSEL_SHIFT)
        {
        case 0:
            P("Fast");
            break;
        case 1:
            P("Medium");
            break;
        case 2:
            P("Slow");
            break;
        default:
            P("Undefined");
            break;
        }
        P("\n");
    }

    P("Signaled Target Abort: ");
    P(data & PCI_STATUS_SIG_TARGET_ABORT ? "True\n": "False\n");

    P("Received Target Abort: ");
    P(data & PCI_STATUS_REC_TARGET_ABORT ? "True\n": "False\n");

    P("Received Master Abort: ");
    P(data & PCI_STATUS_REC_MASTER_ABORT ? "True\n": "False\n");

    P("Signaled System Error: ");
    P(data & PCI_STATUS_SIG_SYSTEM_ERROR ? "True\n": "False\n");

    P("Detected Parity Error: ");
    P(data & PCI_STATUS_PARITY ? "True\n": "False\n");
}

static void PciCapRegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen, DWORD *pdwOutLen)
{
    P("Capability Version: 0x");
    P("%lx", (data & PCI_EXP_FLAGS_VERS));
    P("\n");

    P("Device/Port type: ");
    switch ((data & PCI_EXP_FLAGS_TYPE) >> PCI_EXP_FLAGS_TYPE_SHIFT)
    {
    case 0:
        P("PCI Express Endpoint");
        break;
    case 1:
        P("Legacy PCI Express Endpoint");
        break;
    case 4:
        P("Root Port of PCI Express Root Complex");
        break;
    case 5:
        P("Upstream Port of PCI Express Switch");
        break;
    case 6:
        P("Downstream Port of PCI Express Switch");
        break;
    case 7:
        P("PCI Express to PCI/PCI-X Bridge");
        break;
    case 8:
        P("PCI/PCI-X to PCI Express Bridge");
        break;
    case 9:
        P("Root Complex Integrated Endpoint");
        break;
    case 10:
        P("Root Complex Event Collector");
        break;
    default:
        P("Undefined");
        break;
    }
    P("\n");

    P("Slot Implemented: ");
    P(data & PCI_EXP_FLAGS_SLOT ? "True\n": "False\n");

    P("Interrupt Message Number: ");
    P("%lx", data & PCI_EXP_FLAGS_IRQ, 16);
}

static void PciDevCapsRegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen,
    DWORD *pdwOutLen)
{
    switch (data & PCI_EXP_DEVCAP_PAYLOAD)
    {
    case 0:
        P("128");
        break;
    case 1:
        P("256");
        break;
    case 2:
        P("512");
        break;
    case 3:
        P("1024");
        break;
    case 4:
        P("2048");
        break;
    case 5:
        P("4096");
        break;
    }

    P(" bytes max payload size\n");

    switch ((data & PCI_EXP_DEVCAP_PHANTOM) >> PCI_EXP_DEVCAP_PHANTOM_SHIFT)
    {
    case 0:
        P("No function number bits are used for Phantom\n Functions");
        break;
    case 1:
        P("The MSB of the function number in Requester ID\nis used for Phantom"
            " Functions");
        break;
    case 2:
        P("The 2 MSBs of the function number in Requster\nID are used for "
            "Phantom Functions");
        break;
    case 3:
        P("All 3 bits of the function number in Requster\nID are used for "
            "Phantom Functions");
        break;
    }
    P("\n");

    P(data & PCI_EXP_DEVCAP_EXT_TAG ?
        "8-bit Tag field supported" : "5-bit Tag field supported");

    P("\n");

    P("Endpoint L0s Acceptable Latency: ");
    switch ((data & PCI_EXP_DEVCAP_L0S) >> 6)
    {
    case 0:
        P("Max 64ns");
        break;
    case 1:
        P("Max 128ns");
        break;
    case 2:
        P("Max 256ns");
        break;
    case 3:
        P("Max 512ns");
        break;
    case 4:
        P("Max 1us");
        break;
    case 5:
        P("Max 2us");
        break;
    case 6:
        P("Max 4us");
        break;
    case 7:
        P("No limit");
        break;
    }
    P("\n");

    P("Endpoint L1 Acceptable Latency: ");
    switch ((data & PCI_EXP_DEVCAP_L1) >> PCI_EXP_DEVCAP_L1_SHIFT)
    {
    case 0:
        P("Max 1us");
        break;
    case 1:
        P("Max 2us");
        break;
    case 2:
        P("Max 4us");
        break;
    case 3:
        P("Max 8us");
        break;
    case 4:
        P("Max 16us");
        break;
    case 5:
        P("Max 32us");
        break;
    case 6:
        P("Max 64us");
        break;
    case 7:
        P("No limit");
        break;
    }
    P("\n");

    P("Attention Button ");
    P(data & PCI_EXP_DEVCAP_ATN_BUT ? "" : "not ");
    P("present\n");

    P("Attention Indicator ");
    P(data & PCI_EXP_DEVCAP_ATN_IND ? "" : "not ");
    P("present\n");

    P("Power Indicator ");
    P(data & PCI_EXP_DEVCAP_PWR_IND ? "" : "not ");
    P("present\n");

    P("Role-Based Error Reporting ");
    P(data & PCI_EXP_DEVCAP_RBER ? "" : "not ");
    P("implemented\n");

    P("Upper limit of power supplied by slot: ");

    switch ((data & PCI_EXP_DEVCAP_PWR_SCL) >> PCI_EXP_DEVCAP_PWD_SCL_SHIFT)
    {
    case 0:
        P("%.3f", 1.0 * ((data & PCI_EXP_DEVCAP_PWR_VAL) >>
            PCI_EXP_DEVCAP_PWR_VAL_SHIFT));
        break;
    case 1:
        P("%.3f", 0.1 * ((data & PCI_EXP_DEVCAP_PWR_VAL) >>
            PCI_EXP_DEVCAP_PWR_VAL_SHIFT));
        break;
    case 2:
        P("%.3f", 0.01 * ((data & PCI_EXP_DEVCAP_PWR_VAL) >>
            PCI_EXP_DEVCAP_PWR_VAL_SHIFT));
        break;
    case 3:
        P("%.3f", 0.001 * ((data & PCI_EXP_DEVCAP_PWR_VAL) >>
            PCI_EXP_DEVCAP_PWR_VAL_SHIFT));
        break;
    }
    P(" Watts\n");

    P("Function Level Reset Capability ");
    P(data & PCI_EXP_DEVCAP_FLR ? "" : "not ");
    P("supported");
}

static void PciDevCtlRegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen,
    DWORD *pdwOutLen)
{
    P("Correctable Error Reporting: ");
    P(data & PCI_EXP_DEVCTL_CERE ? "Enabled\n" : "Disabled\n");

    P("Non-Fatal Error Reporting: ");
    P(data & PCI_EXP_DEVCTL_NFERE ? "Enabled\n" : "Disabled\n");

    P("Fatal Error Reporting: ");
    P(data & PCI_EXP_DEVCTL_FERE ? "Enabled\n" : "Disabled\n");

    P("Unsupported Request Reporting: ");
    P(data & PCI_EXP_DEVCTL_URRE ? "Enabled\n" : "Disabled\n");

    P("Relaxed Ordering: ");
    P(data & PCI_EXP_DEVCTL_RELAX_EN ? "Enabled\n" : "Disabled\n");

    switch ((data & PCI_EXP_DEVCTL_PAYLOAD) >> PCI_EXP_DEVCTL_PAYLOAD_SHIFT)
    {
    case 0:
        P("128");
        break;
    case 1:
        P("256");
        break;
    case 2:
        P("512");
        break;
    case 3:
        P("1024");
        break;
    case 4:
        P("2048");
        break;
    case 5:
        P("4096");
        break;
    default:
        P("Undefined");
        break;
    }
    P(" bytes Max Payload size\n");

    P("Extended Tag Field: ");
    P(data & PCI_EXP_DEVCTL_EXT_TAG ? "Enabled\n" : "Disabled\n");

    P("Phantom Functions: ");
    P(data & PCI_EXP_DEVCTL_PHANTOM ? "Enabled\n" : "Disabled\n");

    P("Auxilary (AUX) Power PM: ");
    P(data & PCI_EXP_DEVCTL_AUX_PME ? "Enabled\n" : "Disabled\n");

    P("No Snoop: ");
    P(data & PCI_EXP_DEVCTL_NOSNOOP_EN ? "Enabled\n" : "Disabled\n");

    switch ((data & PCI_EXP_DEVCTL_READRQ) >> PCI_EXP_DEVCTL_READRQ_SHIFT)
    {
    case 0:
        P("128");
        break;
    case 1:
        P("256");
        break;
    case 2:
        P("512");
        break;
    case 3:
        P("1024");
        break;
    case 4:
        P("2048");
        break;
    case 5:
        P("4096");
        break;
    default:
        P("Undefined");
        break;
    }
    P(" bytes Max Read Request size\n");

    P("Bridge Configuration Retry / Initiate Function Level Reset ");
    P(data & PCI_EXP_DEVCTL_BCR_FLR ? "Enabled\n" : "Disabled\n");
}

static void PciDevStaRegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen,
    DWORD *pdwOutLen)
{
    P("Correctable Errors: ");
    P(data & PCI_EXP_DEVSTA_CED ? "" : "not ");
    P("detected\n");

    P("Non-Fatal Errors: ");
    P(data & PCI_EXP_DEVSTA_NFED ? "" : "not ");
    P("detected\n");

    P("Fatal Errors: ");
    P(data & PCI_EXP_DEVSTA_FED ? "" : "not ");
    P("detected\n");

    P("AUX Power: ");
    P(data & PCI_EXP_DEVSTA_AUXPD ? "" : "not ");
    P("detected\n");

    P("Transactions Pending: ");
    P(data & PCI_EXP_DEVSTA_TRPND ? "True\n" : "False\n");
}

static void PciLnkCapRegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen,
    DWORD *pdwOutLen)
{
    P("Link Speeds Supported: \n");
    switch (data & PCI_EXP_LNKCAP_SLS)
    {
    case 3:
        P("8.0 GT/s\n");
    case 2:
        P("5.0 GT/s\n");
    case 1:
        P("2.5 GT/s\n");
        break;
    default:
        P("Undefined Supported Link Speeds\n");
        break;
    }

    P("Max Link Width: ");
    switch ((data & PCI_EXP_LNKCAP_MLW) >> PCI_EXP_LNKCAP_MLW_SHIFT)
    {
    case 1:
    case 2:
    case 4:
    case 8:
    case 12:
    case 16:
    case 32:
        P("x%x", (data & PCI_EXP_LNKCAP_MLW) >> PCI_EXP_LNKCAP_MLW_SHIFT);
        break;
    default:
        P("Undefined ");
        break;
    }
    P("\n");

    switch ((data & PCI_EXP_LNKCAP_ASPMS) >> PCI_EXP_LNKCAP_ASPMS_SHIFT)
    {
    case 0:
        P("No ASPM Support");
        break;
    case 1:
        P("L0s Supported");
        break;
    case 2:
        P("L1 Supported");
        break;
    case 3:
        P("L0s and L1 Supported");
        break;
    }
    P("\n");

    P("L0s Exit Latency: ");
    switch ((data & PCI_EXP_LNKCAP_L0SEL) >> PCI_EXP_LNKCAP_L0SEL_SHIFT)
    {
    case 0:
        P("Less then 64 ns");
        break;
    case 1:
        P("64ns to less than 128ns");
        break;
    case 2:
        P("128ns to less than 256ns");
        break;
    case 3:
        P("256ns to less than 512ns");
        break;
    case 4:
        P("512ns to less than 1us");
        break;
    case 5:
        P("1us to less than 2us");
        break;
    case 6:
        P("2us to less than 4us");
        break;
    case 7:
        P("more than 4us");
        break;
    }
    P("\n");

    P("L1 Exit Latency: ");
    switch ((data & PCI_EXP_LNKCAP_L1EL) >> PCI_EXP_LNKCAP_L1EL_SHIFT)
    {
    case 0:
        P("Less then 1us");
        break;
    case 1:
        P("1us to less than 2us");
        break;
    case 2:
        P("2us to less than 4us");
        break;
    case 3:
        P("4us to less than 8us");
        break;
    case 4:
        P("8us to less than 16us");
        break;
    case 5:
        P("16us to less than 32us");
        break;
    case 6:
        P("32us to less than 64us");
        break;
    case 7:
        P("more than 64us");
        break;
    }
    P("\n");

    P("Clock Power Management: ");
    P(data & PCI_EXP_LNKCAP_CLKPM ? "True\n": "False\n");

    P("Surprise Down Error Reporting Capability: ");
    P(data & PCI_EXP_LNKCAP_SDERC ? "": "not ");
    P("present\n");

    P("Data Layer Link Active Reporting Capability: ");
    P(data & PCI_EXP_LNKCAP_DLLLARC ? "": "not ");
    P("present\n");

    P("Link Bandwidth Notification Capability: ");
    P(data & PCI_EXP_LNKCAP_LBNC ? "": "not ");
    P("present\n");

    P("ASPM Optionality Compliance: ");
    P(data & PCI_EXP_LNKCAP_ASPMS ? "True\n": "False\n");
    P("\n");

    P("Port Number: ");
    P("0x%lx", (data & PCI_EXP_LNKCAP_PN) >> 24);
}

static void PciLnkCtlRegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen,
    DWORD *pdwOutLen)
{
    P("Active Start Power Management (ASPM) Control: ");
    switch (data & PCI_EXP_LNKCTL_ASPMC)
    {
    case 0:
        P("Disabled");
        break;
    case 1:
        P("L0s Entry Enabled");
        break;
    case 2:
        P("L1 Entry Enabled");
        break;
    case 3:
        P("L0s and L1 Entry Enabled");
        break;
    }
    P("\n");

    P("Read Completion Boundary (RCB): ");
    P(data & PCI_EXP_LNKCTL_RCB ? "128 ": "64 ");
    P("byte\n");

    P("Link Disable: ");
    P(data & PCI_EXP_LNKCTL_RCB ? "On\n": "Off\n");

    P("Common Clock Configuration: ");
    P(data & PCI_EXP_LNKCTL_CCC ? "True\n": "False\n");
    P("\n");

    P("Extended Sync: ");
    P(data & PCI_EXP_LNKCTL_ES ? "True\n": "False\n");

    P("Clock Power Management: ");
    P(data & PCI_EXP_LNKCTL_CLKREQ_EN ? "Enabled\n": "Disabled\n");

    P("Hardware Autonomous Width Disable: ");
    P(data & PCI_EXP_LNKCTL_HAWD ? "On\n": "Off\n");

    P("Link Bandwidth Management Interrupt Enable: ");
    P(data & PCI_EXP_LNKCTL_LBMIE ? "On\n": "Off\n");

    P("Link Autonomous Bandwidth Interrupt Enable: ");
    P(data & PCI_EXP_LNKCTL_LABIE ? "On": "Off");
}

static void PciLnkStaRegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen,
    DWORD *pdwOutLen)
{
    P("Current Link Speed: ");
    switch (data & PCI_EXP_LNKSTA_CLS)
    {
    case 1:
        P("2.5 GT/s");
        break;
    case 2:
        P("5.0 GT/s");
        break;
    case 3:
        P("8.0 GT/s");
        break;
    default:
        P("Undefined");
        break;
    }
    P("\n");

    P("Negotiated Link Width: ");
    switch ((data & PCI_EXP_LNKSTA_NLW) >> PCI_EXP_LNKSTA_NLW_SHIFT)
    {
    case 1:
    case 2:
    case 4:
    case 8:
    case 12:
    case 16:
    case 32:
        P("x%x", (data & PCI_EXP_LNKSTA_NLW) >> PCI_EXP_LNKSTA_NLW_SHIFT);
        break;
    default:
        P("Undefined ");
        break;
    }
    P("\n");

    P("Link Training: ");
    P(data & PCI_EXP_LNKSTA_LT ? "True\n" : "False\n");

    P("Slot Clock Configuration: ");
    P(data & PCI_EXP_LNKSTA_SLC ? "True\n" : "False\n");

    P("Data Link Layer Link Active: ");
    P(data & PCI_EXP_LNKSTA_DLLLA ? "True\n" : "False\n");

    P("Link Bandwidth Management Status: ");
    P(data & PCI_EXP_LNKSTA_LBMS ? "True\n" : "False\n");

    P("Link Autonomous Bandwidth Status: ");
    P(data & PCI_EXP_LNKSTA_LABS ? "True\n" : "False\n");
}

static void PciSltCapRegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen,
    DWORD *pdwOutLen)
{
    P("Attention Button: ");
    P(data & PCI_EXP_SLTCAP_ABP ? "" : "not ");
    P("present\n");

    P("Power Controller: ");
    P(data & PCI_EXP_SLTCAP_PCP ? "" : "not ");
    P("present\n");

    P("MRL Sensor: ");
    P(data & PCI_EXP_SLTCAP_MRLSP ? "" : "not ");
    P("present\n");

    P("Attention Indicator: ");
    P(data & PCI_EXP_SLTCAP_AIP ? "" : "not ");
    P("present\n");

    P("Hot-Plug Surprise Capability: ");
    P(data & PCI_EXP_SLTCAP_HPS ? "" : "not ");
    P("present\n");

    P("Hot-Plug Capability: ");
    P(data & PCI_EXP_SLTCAP_HPC ? "" : "not ");
    P("present\n");

    P("Slot Power Limit Value: ");
    switch ((data & PCI_EXP_SLTCAP_SPLV) >> PCI_EXP_SLTCAP_SPLV_SHIFT)
    {
    case 0:
        P("Uninitialized");
        break;
    case 0xF0:
        P("250 Watt");
        break;
    case 0xF1:
        P("275 Watt");
        break;
    case 0xF2:
        P("300 Watt");
        break;
    default:
        P("Undefined");
        break;
    }
    P("\n");

    P("Slot Power Limit Scale: ");
    switch ((data & PCI_EXP_SLTCAP_SPLS) >> PCI_EXP_SLTCAP_SPLS_SHIFT)
    {
    case 0:
        P("1.0x");
        break;
    case 1:
        P("0.1x");
        break;
    case 2:
        P("0.01x");
        break;
    case 3:
        P("0.001x");
        break;
    default:
        P("Undefined");
        break;
    }
    P("\n");

    P("Electromechanical Interlock: ");
    P(data & PCI_EXP_SLTCAP_EIP ? "" : "not ");
    P("present\n");

    P("No Command Completed Support: ");
    P(data & PCI_EXP_SLTCAP_NCCS ? "" : "not ");
    P("present\n");

    P("Physical Slot Number: ");
    P(data & PCI_EXP_SLTCAP_PSN ? "" : "not ");
    P("present");
}

static void PciSltCtlRegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen,
    DWORD *pdwOutLen)
{
    P("Attention Button Pressed Enable: ");
    P(data & PCI_EXP_SLTCTL_ABPE ? "Enabled\n" : "Disabled\n");

    P("Power Fault Detected Enable: ");
    P(data & PCI_EXP_SLTCTL_PFDE ? "Enabled\n" : "Disabled\n");

    P("MRL Sensor Changed Enabled: ");
    P(data & PCI_EXP_SLTCTL_MRLSCE ? "Enabled\n" : "Disabled\n");

    P("Presence Detect Changed Enable: ");
    P(data & PCI_EXP_SLTCTL_PDCE ? "Enabled\n" : "Disabled\n");

    P("Command Completed Interrupt Enable: ");
    P(data & PCI_EXP_SLTCTL_CCIE ? "Enabled\n" : "Disabled\n");

    P("Hot-Plug Interrupt Enable: ");
    P(data & PCI_EXP_SLTCTL_HPIE ? "Enabled\n" : "Disabled\n");

    P("Attention Indicator Control: ");
    switch ((data & PCI_EXP_SLTCTL_AIC) >> PCI_EXP_SLTCAP_AIC_SHIFT)
    {
    case 0:
        P("Reserved");
        break;
    case 1:
        P("On");
        break;
    case 2:
        P("Blink");
        break;
    case 3:
        P("Off");
        break;
    }
    P("\n");

    P("Power Indicator Control: ");
    switch ((data & PCI_EXP_SLTCTL_PIC) >> PCI_EXP_SLTCTL_PIC_SHIFT)
    {
    case 0:
        P("Reserved");
        break;
    case 1:
        P("On");
        break;
    case 2:
        P("Blink");
        break;
    case 3:
        P("Off");
        break;
    }
    P("\n");

    P("Power Controller Control: ");
    P(data & PCI_EXP_SLTCTL_PCC ? "On\n": "Off\n");

    P("Electromechanical Interlock Control: ");
    P(data & PCI_EXP_SLTCTL_EIC ? "On\n": "Off\n");

    P("Data Link Layer State Changed: ");
    P(data & PCI_EXP_SLTCTL_DLLSCE ? "Enabled\n" : "Disabled\n");
}

static void PciSltStaRegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen,
    DWORD *pdwOutLen)
{

    P("Attention Button Pressed: ");
    P(data & PCI_EXP_SLTSTA_ABP ? "True\n": "False\n");

    P("Power Fault Detected: ");
    P(data & PCI_EXP_SLTSTA_PFD ? "True\n": "False\n");

    P("MRL Sensor Changed: ");
    P(data & PCI_EXP_SLTSTA_MRLSC ? "True\n": "False\n");

    P("Presence Detect Changed: ");
    P(data & PCI_EXP_SLTSTA_PDC ? "True\n": "False\n");

    P("Command Completed: ");
    P(data & PCI_EXP_SLTSTA_CC ? "True\n": "False\n");

    P("MRL Sensor State: ");
    P(data & PCI_EXP_SLTSTA_MRLSS ? "Open\n" : "Closed\n");

    P("Presence Detect State: ");
    P(data & PCI_EXP_SLTSTA_PDS ? "Card Present in Slot" :
        "Slot Empty");
    P("\n");

    P("Electromechanical Interlock Status: ");
    P(data & PCI_EXP_SLTSTA_EIS ? "Engaged\n" : "Disengaged\n");

    P("Data Link Layer State Changed: ");
    P(data & PCI_EXP_SLTSTA_DLLSC ? "True\n": "False\n");
}

static void PciRtCtlRegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen, DWORD *pdwOutLen)
{
    P("System Error on Correctable Error Enable: ");
    P(data & PCI_EXP_RTCTL_SECEE ? "Enabled\n" : "Disabled\n");

    P("System Error on Non-Fatal Error Enable: ");
    P(data & PCI_EXP_RTCTL_SENFEE ? "Enabled\n" : "Disabled\n");

    P("System Error on Fatal Error Enable: ");
    P(data & PCI_EXP_RTCTL_SEFEE ? "Enabled\n" : "Disabled\n");

    P("PME Interrupt Enable: ");
    P(data & PCI_EXP_RTCTL_PMEIE ? "Enabled\n" : "Disabled\n");

    P("CRS Software Visibility Enable: ");
    P(data & PCI_EXP_RTCTL_CRSSVE ? "Enabled\n" : "Disabled\n");
}

static void PciRtCapsRegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen,
    DWORD *pdwOutLen)
{
    P("CRS Software Visibility: ");
    P(data & PCI_EXP_RTCAP_CRSVIS ? "" : "not ");
    P("present");
}

static void PciRtStaRegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen,
    DWORD *pdwOutLen)
{
    P("PME Status: ");
    P(data & PCI_EXP_RTSTA_PME ? "" : "not ");
    P("present");

    if (data & PCI_EXP_RTSTA_PME)
    {
        P("\nPME Requester ID: ");
        P("0x%lx", data & PCI_EXP_RTSTA);
        P("\n");

        P("PME Pending: ");
        P(data & PCI_EXP_RTSTA_PENDING ? "True\n": "False\n");
    }
}

static void PciDevCaps2RegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen,
    DWORD *pdwOutLen)
{
    if (data & PCI_EXP_DEVCAP2_COMP_TO_RANGES_SUPP)
        P("Completion Timeout Ranges Supported:\n");
    if (data & PCI_EXP_DEVCAP2_RANGE_A)
        P("50us to 10ms\n");
    if (data & PCI_EXP_DEVCAP2_RANGE_B)
        P("10ms to 250ms\n");
    if (data & PCI_EXP_DEVCAP2_RANGE_C)
        P("250ms to 4s\n");
    if (data & PCI_EXP_DEVCAP2_RANGE_D)
        P("4s to 64s\n");

    P("Completion Timeout Disable Supported: ");
    P(data & PCI_EXP_DEVCAP2_COMP_TO_DIS_SUPP ? "": "not ");
    P("supported\n");

    P("ARI Forwarding Supported: ");
    P(data & PCI_EXP_DEVCAP2_ARI ? "" : "not ");
    P("supported\n");

    P("AtomicOp Routing Supported: ");
    P(data & PCI_EXP_DEVCAP2_ATOMIC_ROUTE ? "": "not ");
    P("supported\n");

    P("32-bit AtomicOp Completer Supported: ");
    P(data & PCI_EXP_DEVCAP2_ATOMIC_COMP32 ? "": "not ");
    P("supported\n");

    P("64-bit AtomicOp Completer Supported: ");
    P(data & PCI_EXP_DEVCAP2_ATOMIC_COMP64 ? "": "not ");
    P("supported\n");

    P("128-bit CAS Completer Supported: ");
    P(data & PCI_EXP_DEVCAP2_128_CAS_COMP_SUPP ? "": "not ");
    P("supported\n");

    P("No RO-enabled PR-PR Passing: ");
    P(data & PCI_EXP_DEVCAP2_NO_RO_ENABLED_PR ? "": "not ");
    P("supported\n");

    P("LTR Mechanism Supported: ");
    P(data & PCI_EXP_DEVCAP2_LTR ? "" : "not ");
    P("supported\n");

    P("TPH Completer Supported: ");
    P(data & PCI_EXP_DEVCAP2_TPH_COMP_SUPP ? "" : "not ");
    P("supported\n");

    P("Extended TPH Completer Supported: ");
    P(data & PCI_EXP_DEVCAP2_EXT_TPH_COMP_SUPP ? "" : "not ");
    P("supported\n");

    P("OBFF Supported using Message Signaling: ");
    P(data & PCI_EXP_DEVCAP2_OBFF_MSG ? "" : "not ");
    P("supported\n");

    P("OBFF Supported using WAKE# Signaling: ");
    P(data & PCI_EXP_DEVCAP2_OBFF_WAKE ? "" : "not ");
    P("supported\n");

    P("Extended Fmt Field Supported: ");
    P(data & PCI_EXP_DEVCAP2_EXT_FMT_FIELD_SUPP ? "3 bit" :
        "2 bit");
    P("\n");

    P("End-End TLP Prefix Supported: ");
    P(data & PCI_EXP_DEVCAP2_EE_TLP_PREFIX_SUPP ? "" : "not ");
    P("supported");

    if (data & PCI_EXP_DEVCAP2_EE_TLP_PREFIX_SUPP)
    {
        P("\nMax End-End TLP Prefixes: ");
        switch ((data & PCI_EXP_DEVCAP2_MAX_EE_TLP_PREFIXES) >>
            PCI_EXP_DEVCAP2_EE_TLP_PREFIX_SUPP_SHIFT)
        {
        case 0:
            P("4");
            break;
        case 1:
            P("1");
            break;
        case 2:
            P("2");
            break;
        case 3:
            P("3");
            break;
        }
    }
}

static void PciDevCtl2RegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen,
    DWORD *pdwOutLen)
{

    P("Completion Timeout Value: ");
    switch (data & PCI_EXP_DEVCTL2_COMP_TIMEOUT)
    {
    case 0:
        P("50us to 50ms");
        break;
    case 1:
        P("50us to 100us");
        break;
    case 2:
        P("1ms to 10ms");
        break;
    case 5:
        P("16ms to 55ms");
        break;
    case 6:
        P("65ms to 210ms");
        break;
    case 9:
        P("260ms to 900ms");
        break;
    case 10:
        P("1s to 3.5s");
        break;
    case 13:
        P("4s to 13s");
        break;
    case 14:
        P("17s to 64s");
        break;
    default:
        P("Undefined");
        break;
    }
    P("\n");

    P("Completion Timeout Disable: ");
    P((data & PCI_EXP_DEVCTL2_COMP_TIMEOUT_DISABLE) ? "" : "not ");
    P("activated\n");

    P("ARI Forwarding Enabled: ");
    P(data & PCI_EXP_DEVCTL2_ARI ? "Enabled\n" : "Disabled\n");

    P("AtomicOp Requester Enable: ");
    P(data & PCI_EXP_DEVCTL2_ATOMIC_REQ? "Enabled\n" : "Disabled\n");

    P("AtomicOp Egress Blocking: ");
    P(data & PCI_EXP_DEVCTL2_ATOMIC_EGRESS_BLOCK ? "Enabled\n" :
        "Disabled\n");

    P("IDO Request Enable: ");
    P(data & PCI_EXP_DEVCTL2_IDO_REQ_EN ? "Enabled\n" : "Disabled\n");

    P("IDO Completion Enable: ");
    P(data & PCI_EXP_DEVCTL2_IDO_CMP_EN ? "Enabled\n" : "Disabled\n");

    P("LTR Mechanism Enable: ");
    P(data & PCI_EXP_DEVCTL2_LTR_EN ? "Enabled\n" : "Disabled\n");

    P("OBFF Enable: ");
    if (!(data & PCI_EXP_DEVCTL2_OBFF_WAKE_EN))
        P("Disabled");
    else if (data & PCI_EXP_DEVCTL2_OBFF_WAKE_EN)
        P("Enabled using WAKE# signaling");
    else if (data & PCI_EXP_DEVCTL2_OBFF_MSGA_EN)
        P("Enabled using Message signaling [Variation A]");
    else if (data & PCI_EXP_DEVCTL2_OBFF_MSGB_EN)
        P("Enabled using Message signaling [Variation B]");

    P("\n");
    P("End-End TLP Prefix Blocking: ");
    P(data & PCI_EXP_DEVCTL2_EE_TLP_PREFIX_BLOCK ?
        "Forwarding Blocked" : "Forwarding Enabled");
}

static void PciLnkCap2RegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen,
    DWORD *pdwOutLen)
{
    P("Supported Link Speeds: \n");
    P(data & PCI_EXP_LNKCAP2_SLS_2_5GB ? "2.5 GT/s\n": "");
    P(data & PCI_EXP_LNKCAP2_SLS_5_0GB ? "5 GT/s\n": "");
    P(data & PCI_EXP_LNKCAP2_SLS_8_0GB ? "8 GT/s": "");
    if (!(data & PCI_EXP_LNKCTL2_LNK_SPEED_2_5) &&
        !(data & PCI_EXP_LNKCTL2_LNK_SPEED_5_0) &&
        !(data & PCI_EXP_LNKCTL2_LNK_SPEED_8_0))
    {
        P("2.5 GT/s\n");
    }
    P("\n");

    P("Crosslink Supported: ");
    P(data & PCI_EXP_LNKCAP2_CROSSLINK ? "True\n": "False\n");
}

static void PciLnkCtl2RegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen,
    DWORD *pdwOutLen)
{
    P("Target Link Speed: ");
    switch (data & PCI_EXP_LNKCTL2_TRGT_LNK_SPEED_MASK)
    {
    case 0:
    case PCI_EXP_LNKCTL2_LNK_SPEED_2_5:
        P("2.5 GT/s");
        break;
    case PCI_EXP_LNKCTL2_LNK_SPEED_5_0:
        P("5.0 GT/s");
        break;
    case PCI_EXP_LNKCTL2_LNK_SPEED_8_0:
        P("8.0 GT/s");
        break;
    default:
        P("Undefined");
        break;
    }
    P("\n");

    P("Enter Compliance: ");
    P(data & PCI_EXP_LNKCTL2_ENTER_COMP ? "True\n": "False\n");

    P("Hardware Autonomous Speed Disable: ");
    P(data & PCI_EXP_LNKCTL2_HW_AUTO_SPEED_DIS ? "True\n": "False\n");

    if ((data & PCI_EXP_LNKCTL2_TRGT_LNK_SPEED_MASK) == 2)
    {
        P("Selectable De-emphasis: ");
        P(data & PCI_EXP_LNKCTL2_SELECTABLE_DEEMPH ? "-3.5 dB" :
            "-6 dB");
        P("\n");
    }

    P("Transmit Margin: ");
    if (data & PCI_EXP_LNKCTL2_TRANS_MARGIN_MASK)
    {
        P("Operation encoding ");
        P("%lx\n", (data & PCI_EXP_LNKCTL2_TRANS_MARGIN_MASK) >>
            PCI_EXP_LNKCTL2_TRANS_MARGIN_MASK_SHIFT);
    }
    else
    {
        P("Normal operating range\n");
    }

    P("Enter Modified Compliance: ");
    P(data & PCI_EXP_LNKCTL2_ENTER_MOD_COMP ? "True\n": "False\n");

    if ((data & PCI_EXP_LNKCTL2_TRGT_LNK_SPEED_MASK) <=
        PCI_EXP_LNKCTL2_LNK_SPEED_5_0)
    {
        P("Compliance SOS: ");
        P(data & PCI_EXP_LNKCTL2_COMP_SOS ? "True\n": "False\n");
    }

    if ((data & PCI_EXP_LNKCTL2_TRGT_LNK_SPEED_MASK) ==
        PCI_EXP_LNKCTL2_LNK_SPEED_8_0)
    {
        P("Transmitter Preset in Polling: ");
        P("%lx\n", (data & PCI_EXP_LNKCTL2_TRANS_MARGIN_MASK) >>
            PCI_EXP_LNKCTL2_TRANS_MARGIN_MASK_SHIFT);
    }
    else if ((data & PCI_EXP_LNKCTL2_TRGT_LNK_SPEED_MASK) ==
        PCI_EXP_LNKCTL2_LNK_SPEED_5_0)
    {
        P("De-emphasis Level in Polling: ");
        P(data & PCI_EXP_LNKCTL2_DEEMPH_LVL_POLL ? "-3.5 dB"
            : "-6 dB");
    }
}

static void PciLnkSta2RegDetails(DWORD data, PCHAR pBuf, DWORD dwInLen,
    DWORD *pdwOutLen)
{
    P("Current De-emphasis Level \n(only applicable when link"
        " operates at 5.0 GT/s speed):\n");
    P(data & PCI_EXP_LNKSTA2_CDL ? "-3.5 dB\n" : "-6 dB\n");

    P("*** The following fields are only applicable\nwhen link operates"
        " at 8 GT/s: ***\n");
    P("Equalization Complete: ");
    P(data & PCI_EXP_LNKSTA2_EQUALIZ_COMP ? "True\n": "False\n");

    P("Equalization Phase 1 Successful: ");
    P(data & PCI_EXP_LNKSTA2_EQUALIZ_PH1 ? "True\n": "False\n");

    P("Equalization Phase 2 Successful: ");
    P(data & PCI_EXP_LNKSTA2_EQUALIZ_PH2 ? "True\n": "False\n");

    P("Equalization Phase 3 Successful: ");
    P(data & PCI_EXP_LNKSTA2_EQUALIZ_PH3 ? "True\n": "False\n");

    P("Link Equalization Request: ");
    P(data & PCI_EXP_LNKSTA2_LINE_EQ_REQ ? "True\n": "False\n");
}

/* This function is based upon the PCI Express Base Specification Revision
 * 3.0 */
DWORD DLLCALLCONV PciConfRegData2Str(_In_ WDC_DEVICE_HANDLE hDev,
    _In_ DWORD dwOffset, _Outptr_ PCHAR pBuf, _In_ DWORD dwInLen,
    _Outptr_ DWORD *pdwOutLen)
{
    BOOL fIsPciExpress;
    DWORD data = 0, status = 0;

    if (!hDev)
    {
        status = WD_INVALID_HANDLE;
        goto Exit;
    }

    fIsPciExpress = WDC_PciGetExpressGen(hDev);
    strcpy(pBuf, "");

    switch (dwOffset)
    {
    case PCI_COMMAND:
        status = WDC_PciReadCfg(hDev, PCI_COMMAND, &data, WDC_SIZE_16);
        PciCmdRegDetails(data, pBuf, dwInLen, pdwOutLen, fIsPciExpress);
        break;
    case PCI_STATUS:
        status = WDC_PciReadCfg(hDev, PCI_STATUS, &data, WDC_SIZE_16);
        PciStatRegDetails(data, pBuf, dwInLen, pdwOutLen, fIsPciExpress);
        break;
    }
Exit:
    if (status)
        return status;
    return WD_STATUS_SUCCESS;
}

DWORD DLLCALLCONV PciExpressConfRegData2Str(_In_ WDC_DEVICE_HANDLE hDev,
    _In_ DWORD dwOffset, _Outptr_ PCHAR pBuf, _In_ DWORD dwInLen,
    _Outptr_ DWORD *pdwOutLen)
{
    DWORD status = 0, data = 0, dwPciExpressOffset = 0;

    if (!pBuf)
        return WD_INVALID_PARAMETER;

    status = WDC_PciGetExpressOffset(hDev, &dwPciExpressOffset);
    if (status)
        return status;

    strcpy(pBuf, "");

    switch (dwOffset)
    {
    case PCI_EXP_FLAGS:
        status = WDC_PciReadCfg(hDev, dwPciExpressOffset + PCI_EXP_FLAGS, &data,
            WDC_SIZE_16);
        PciCapRegDetails(data, pBuf, dwInLen, pdwOutLen);
        break;
    case PCI_EXP_DEVCAP:
        status = WDC_PciReadCfg(hDev, dwPciExpressOffset + PCI_EXP_DEVCAP,
            &data, WDC_SIZE_32);
        PciDevCapsRegDetails(data, pBuf, dwInLen, pdwOutLen);
        break;
    case PCI_EXP_DEVCTL:
        status = WDC_PciReadCfg(hDev, dwPciExpressOffset + PCI_EXP_DEVCTL,
            &data, WDC_SIZE_16);
        PciDevCtlRegDetails(data, pBuf, dwInLen, pdwOutLen);
        break;
    case PCI_EXP_DEVSTA:
        status = WDC_PciReadCfg(hDev, dwPciExpressOffset + PCI_EXP_DEVSTA,
            &data, WDC_SIZE_16);
        PciDevStaRegDetails(data, pBuf, dwInLen, pdwOutLen);
        break;
    case PCI_EXP_LNKCAP:
        status = WDC_PciReadCfg(hDev, dwPciExpressOffset + PCI_EXP_LNKCAP,
            &data, WDC_SIZE_32);
        PciLnkCapRegDetails(data, pBuf, dwInLen, pdwOutLen);
        break;
    case PCI_EXP_LNKCTL:
        status = WDC_PciReadCfg(hDev, dwPciExpressOffset + PCI_EXP_LNKCTL,
            &data, WDC_SIZE_16);
        PciLnkCtlRegDetails(data, pBuf, dwInLen, pdwOutLen);
        break;
    case PCI_EXP_LNKSTA:
        status = WDC_PciReadCfg(hDev, dwPciExpressOffset + PCI_EXP_LNKSTA,
            &data, WDC_SIZE_16);
        PciLnkStaRegDetails(data, pBuf, dwInLen, pdwOutLen);
        break;
    case PCI_EXP_SLTCAP:
        status = WDC_PciReadCfg(hDev, dwPciExpressOffset + PCI_EXP_SLTCAP,
            &data, WDC_SIZE_32);
        PciSltCapRegDetails(data, pBuf, dwInLen, pdwOutLen);
        break;
    case PCI_EXP_SLTCTL:
        status = WDC_PciReadCfg(hDev, dwPciExpressOffset + PCI_EXP_SLTCTL,
            &data, WDC_SIZE_16);
        PciSltCtlRegDetails(data, pBuf, dwInLen, pdwOutLen);
        break;
    case PCI_EXP_SLTSTA:
        status = WDC_PciReadCfg(hDev, dwPciExpressOffset + PCI_EXP_SLTSTA,
            &data, WDC_SIZE_16);
        PciSltStaRegDetails(data, pBuf, dwInLen, pdwOutLen);
        break;
    case PCI_EXP_RTCTL:
        status = WDC_PciReadCfg(hDev, dwPciExpressOffset + PCI_EXP_RTCTL,
            &data, WDC_SIZE_16);
        PciRtCtlRegDetails(data, pBuf, dwInLen, pdwOutLen);
        break;
    case PCI_EXP_RTCAP:
        status = WDC_PciReadCfg(hDev, dwPciExpressOffset + PCI_EXP_RTCAP, &data,
            WDC_SIZE_16);
        PciRtCapsRegDetails(data, pBuf, dwInLen, pdwOutLen);
        break;
    case PCI_EXP_DEVCAP2:
        status = WDC_PciReadCfg(hDev, dwPciExpressOffset + PCI_EXP_DEVCAP2,
            &data, WDC_SIZE_32);
        PciDevCaps2RegDetails(data, pBuf, dwInLen, pdwOutLen);
        break;
    case PCI_EXP_DEVCTL2:
        status = WDC_PciReadCfg(hDev, dwPciExpressOffset + PCI_EXP_DEVCTL2,
            &data, WDC_SIZE_16);
        PciDevCtl2RegDetails(data, pBuf, dwInLen, pdwOutLen);
        break;
    case PCI_EXP_LNKCAP2:
        status = WDC_PciReadCfg(hDev, dwPciExpressOffset + PCI_EXP_LNKCAP2,
            &data, WDC_SIZE_32);
        PciLnkCap2RegDetails(data, pBuf, dwInLen, pdwOutLen);
        break;
    case PCI_EXP_LNKCTL2:
        status = WDC_PciReadCfg(hDev, dwPciExpressOffset + PCI_EXP_LNKCTL2,
            &data, WDC_SIZE_16);
        PciLnkCtl2RegDetails(data, pBuf, dwInLen, pdwOutLen);
        break;
    case PCI_EXP_LNKSTA2:
        status = WDC_PciReadCfg(hDev, dwPciExpressOffset + PCI_EXP_LNKSTA2,
            &data, WDC_SIZE_16);
        PciLnkSta2RegDetails(data, pBuf, dwInLen, pdwOutLen);
        break;
    }
    if (*pdwOutLen > dwInLen)
        return WD_INSUFFICIENT_RESOURCES;
    if (status)
        return status;
    return WD_STATUS_SUCCESS;
}

