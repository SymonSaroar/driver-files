/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/************************************************************************
*  File: plx_lib.c
*
*  Library for accessing PLX devices.
*  The code accesses hardware using WinDriver's WDC library.
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*************************************************************************/

#include "wdc_defs.h"
#include "utils.h"
#include "status_strings.h"
#include "plx_lib.h"

/*************************************************************
  Internal definitions
 *************************************************************/
/* WinDriver license registration string */
/* TODO: When using a registered WinDriver version, replace the license string
         below with the development license in order to use on the development
         machine.
         Once you require to distribute the driver's package to other machines,
         please replace the string with a distribution license */
#define PLX_DEFAULT_LICENSE_STRING "12345abcde1234.license"

#define PLX_DEFAULT_DRIVER_NAME WD_DEFAULT_DRIVER_NAME_BASE

/* VPD EEPROM delay */
#define EEPROM_VPD_Delay() WDC_Sleep(20000, WDC_SLEEP_BUSY)

/* Run-time registers EEPROM delay */
#define EEPROM_RT_Delay() WDC_Sleep(500, WDC_SLEEP_BUSY)

/* Soft board reset delay */
#define PLX_SOFT_RESET_DELAY() WDC_Sleep(5000000, WDC_SLEEP_NON_BUSY)

/*************************************************************
  Global variables definitions
 *************************************************************/
/* Last PLX library error string */
static CHAR gsPLX_LastErr[256];

/*************************************************************
  Static functions prototypes and inline implementation
 *************************************************************/
#if !defined(__KERNEL__)
static BOOL IsDeviceValid(const PWDC_DEVICE pDev);
static DWORD DeviceInit(PWDC_DEVICE pDev, BOOL fIsMaster);

static DWORD IntEnableDma(PWDC_DEVICE hDev, PLX_INT_HANDLER funcDiagIntHandler,
    PLX_DMA_HANDLE hDma);
static DWORD IntEnable(PWDC_DEVICE hDev, PLX_INT_HANDLER funcDiagIntHandler);
static DWORD IntDisableDma(WDC_DEVICE_HANDLE hDev);
static DWORD IntDisable(WDC_DEVICE_HANDLE hDev);

static void DLLCALLCONV PLX_IntHandlerDma(PVOID pData);
static void DLLCALLCONV PLX_IntHandler(PVOID pData);
static void PLX_EventHandler(WD_EVENT *pEvent, PVOID pData);
#endif

static DWORD LocalAddrSetMode(WDC_DEVICE_HANDLE hDev, PLX_ADDR addrSpace,
    DWORD dwLocalAddr);

static DWORD EEPROM_VPD_EnableAccess(WDC_DEVICE_HANDLE hDev,
    UINT32 *pu32DataOld);
static DWORD EEPROM_VPD_RestoreAccess(WDC_DEVICE_HANDLE hDev, UINT32 u32Data);
static DWORD EEPROM_VPD_RemoveWriteProtection(WDC_DEVICE_HANDLE hDev,
    WORD wAddr,
    PBYTE pbDataOld);
static DWORD EEPROM_VPD_RestoreWriteProtection(WDC_DEVICE_HANDLE Dev,
    WORD wAddr);

static void EEPROM_RT_ChipSelect(WDC_DEVICE_HANDLE hDev, BOOL fSelect);
static void EEPROM_RT_ReadBit(WDC_DEVICE_HANDLE hDev, BOOL *pBit);
static void EEPROM_RT_WriteBit(WDC_DEVICE_HANDLE hDev, BOOL bit);
static void EEPROM_RT_WriteEnableDisable(WDC_DEVICE_HANDLE hDev, BOOL fEnable);

static void ErrLog(const CHAR *sFormat, ...);
static void TraceLog(const CHAR *sFormat, ...);

static inline BOOL IsValidDevice(PWDC_DEVICE pDev, const CHAR *sFunc)
{
    if (!pDev || !(PPLX_DEV_CTX)(pDev->pCtx))
    {
        snprintf(gsPLX_LastErr, sizeof(gsPLX_LastErr) - 1, "%s: NULL "
            "device %s\n", sFunc, !pDev ? "handle" : "context");
        ErrLog(gsPLX_LastErr);
        return FALSE;
    }

    return TRUE;
}

/*************************************************************
  Functions implementation
 *************************************************************/
/* -----------------------------------------------
    PLX and WDC library initialize/uninit
   ----------------------------------------------- */
DWORD PLX_LibInit(void)
{
    DWORD dwStatus;

    /* Set the driver name */
    if (!WD_DriverName(PLX_DEFAULT_DRIVER_NAME))
    {
        ErrLog("Failed to set the driver name for WDC library.\n");
        return WD_SYSTEM_INTERNAL_ERROR;
    }

    /* Set WDC library's debug options (default: level TRACE, output to Debug
     * Monitor) */
    dwStatus = WDC_SetDebugOptions(WDC_DBG_DEFAULT, NULL);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to initialize debug options for WDC library.\n"
            "Error 0x%x - %s\n", dwStatus, Stat2Str(dwStatus));

        return dwStatus;
    }

    /* Open a handle to the driver and initialize the WDC library */
    dwStatus = WDC_DriverOpen(WDC_DRV_OPEN_DEFAULT, PLX_DEFAULT_LICENSE_STRING);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to initialize the WDC library. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));

        return dwStatus;
    }

    return WD_STATUS_SUCCESS;
}

DWORD PLX_LibUninit(void)
{
    DWORD dwStatus;

    /* Uninit the WDC library and close the handle to WinDriver */
    dwStatus = WDC_DriverClose();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to uninit the WDC library. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

BOOL PLX_IsMaster(WDC_DEVICE_HANDLE hDev)
{
    PPLX_DEV_CTX pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(hDev);
    return pDevCtx->fIsMaster;
}

#if !defined(__KERNEL__)
/* -----------------------------------------------
    Device open/close
   ----------------------------------------------- */

BOOL PLX_DeviceInit(WDC_DEVICE_HANDLE hDev, BOOL fIsMaster)
{
    /* Validate device information */
    /* Initialize device context and validate device information */
    if (!IsDeviceValid((PWDC_DEVICE)hDev) ||
        (DeviceInit((PWDC_DEVICE)hDev, fIsMaster) != WD_STATUS_SUCCESS))
        return FALSE;

    return TRUE;
}

WDC_DEVICE_HANDLE PLX_DeviceOpen(DWORD dwVendorID, DWORD dwDeviceID,
    BOOL fIsMaster)
{
    WDC_DEVICE_HANDLE hDev = WDC_DIAG_DeviceFindAndOpen(dwVendorID,
        dwDeviceID, NULL, sizeof(PLX_DEV_CTX));

    if (!hDev || !PLX_DeviceInit(hDev, fIsMaster))
        goto Error;

    return hDev;

Error:
    if (hDev)
        PLX_DeviceClose(hDev);

    printf("PLX_DeviceOpen: Failed opening PCI device: %s",
        PLX_GetLastErr());

    return NULL;
}

BOOL PLX_DeviceClose(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;

    TraceLog("PLX_DeviceClose entered. Device handle: 0x%p\n", hDev);

    if (!hDev)
    {
        ErrLog("PLX_DeviceClose: Error - NULL device handle\n");
        return FALSE;
    }

    /* Disable interrupts */
    if (PLX_IntIsEnabled(hDev))
    {
        dwStatus = PLX_IntDisable(hDev);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("Failed disabling interrupts. Error 0x%x - %s\n", dwStatus,
                Stat2Str(dwStatus));
        }
    }
    return WDC_DIAG_DeviceClose(hDev);
}

static BOOL IsDeviceValid(const PWDC_DEVICE pDev)
{
    DWORD i, dwNumAddrSpaces = pDev->dwNumAddrSpaces;

    /* TODO: Modify the implementation of this function in order to verify
             that your device has all expected resources */

    /* Verify that the device has at least one active address space */
    for (i = 0; i < dwNumAddrSpaces; i++)
    {
        if (WDC_AddrSpaceIsActive(pDev, i))
            return TRUE;
    }

    ErrLog("Device does not have any active memory or I/O address spaces\n");
    return FALSE;
}

static DWORD DeviceInit(PWDC_DEVICE pDev, BOOL fIsMaster)
{
    PPLX_DEV_CTX pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(pDev);

    /* NOTE: You can modify the implementation of this function in order to
             perform any additional device initialization you require */

    /* Set device type - master/target */
    pDevCtx->fIsMaster = fIsMaster;

    /* Set offsets for some registers */
    /* Offsets of DMA registers will be set in PLX_DMAOpen() */
    if (fIsMaster)
    {
        pDevCtx->dwINTCSR = PLX_M_INTCSR;
        pDevCtx->dwCNTRL = PLX_M_CNTRL;
        pDevCtx->dwPROT_AREA = PLX_M_PROT_AREA;
        pDevCtx->dwLAS0BA = PLX_M_LAS0BA;
    }
    else
    {
        pDevCtx->dwINTCSR = PLX_T_INTCSR;
        pDevCtx->dwCNTRL = PLX_T_CNTRL;
        pDevCtx->dwPROT_AREA = PLX_T_PROT_AREA;
        pDevCtx->dwLAS0BA = PLX_T_LAS0BA;
    }

    /* Enable target abort for master devices */
    if (fIsMaster)
    {
        UINT32 u32IntStatus = 0;

        PLX_ReadReg32(pDev, PLX_M_INTCSR, &u32IntStatus);
        PLX_WriteReg32(pDev, PLX_M_INTCSR, u32IntStatus | BIT12);
    }

    return WD_STATUS_SUCCESS;
}

/* -----------------------------------------------
    Interrupts
   ----------------------------------------------- */
static void DLLCALLCONV PLX_IntHandlerDma(PVOID pData)
{
    /* TODO: Modify the interrupt handler code to suit your specific needs */

    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PPLX_DEV_CTX pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(pDev);
    PLX_INT_RESULT intResult;

    /* Flush the DMA data from I/O cache and update the CPU caches. */
    PLX_DMASyncIo((PLX_DMA_HANDLE)pDevCtx->pPLXDma);

    BZERO(intResult);
    intResult.u32INTCSR =
        pDevCtx->pIntTransCmds[pDevCtx->bIntCsrIndex].Data.Dword;
    intResult.bDMACSR = pDevCtx->pIntTransCmds[pDevCtx->bDmaCsrIndex].Data.Byte;
    intResult.dwCounter = pDev->Int.dwCounter;
    intResult.dwLost = pDev->Int.dwLost;
    intResult.waitResult = (WD_INTERRUPT_WAIT_RESULT)pDev->Int.fStopped;

    /* Execute the diagnostics application's interrupt handler routine */
    pDevCtx->funcDiagIntHandler(pDev, &intResult);
}

static void DLLCALLCONV PLX_IntHandler(PVOID pData)
{
    /* TODO: Modify the interrupt handler code to suit your specific needs */

    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PPLX_DEV_CTX pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(pDev);
    PLX_INT_RESULT intResult;

    BZERO(intResult);
    intResult.u32INTCSR =
        pDevCtx->pIntTransCmds[pDevCtx->bIntCsrIndex].Data.Word;
    intResult.dwCounter = pDev->Int.dwCounter;
    intResult.dwLost = pDev->Int.dwLost;
    intResult.waitResult = (WD_INTERRUPT_WAIT_RESULT)pDev->Int.fStopped;

    /* Execute the diagnostics application's interrupt handler routine */
    pDevCtx->funcDiagIntHandler(pDev, &intResult);
}

/* mask for all PLX interrupts */
#define PLX_MASTER_INT_MASK ((UINT32) (\
        BIT5    /* power management */ \
        | BIT13 /* PCI Doorbell */ \
        | BIT14 /* PCI Abort */ \
        | BIT15 /* Local Input */ \
        | BIT20 /* Local Doorbell */ \
        | BIT21 /* DMA Channel 0 */ \
        | BIT22 /* DMA Channel 1 */ \
        | BIT23 /* BIST */ \
        ))

#define PLX_TARGET_INT_MASK ((WORD) (\
    BIT2    /* LINTi1 Status */ \
    | BIT5  /* LINTi2 Status */ \
    ))

static int getTransCmds(PWDC_DEVICE pDev, PLX_DMA_STRUCT *pPLXDma,
    WD_TRANSFER *pTrans)
{
    PPLX_DEV_CTX pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(pDev);

    /* The transfer commands will be executed by WinDriver in the kernel
       for each interrupt that is received */

    WDC_ADDR_DESC *pAddrDesc = &pDev->pAddrDesc[PLX_ADDR_REG];

    int i = 0;

    /* Read interrupt status from the INTCSR register */
    pTrans[i].pPort = pAddrDesc->pAddr + pDevCtx->dwINTCSR;
    pTrans[i].cmdTrans = WDC_ADDR_IS_MEM(pAddrDesc) ? RM_DWORD : RP_DWORD;
    pDevCtx->bIntCsrIndex = i;
    i++;

    /* Mask interrupt status from the INTCSR register */
    pTrans[i].cmdTrans = CMD_MASK;
    pTrans[i].Data.Dword = PLX_MASTER_INT_MASK;
    i++;

    /* Read DMA status from the DMACSR register */
    pTrans[i].pPort = pAddrDesc->pAddr + pPLXDma->dwDMACSR;
    pTrans[i].cmdTrans = WDC_ADDR_IS_MEM(pAddrDesc) ? RM_BYTE : RP_BYTE;
    pDevCtx->bDmaCsrIndex = i;
    i++;

    /* Write to the DMACSR register to clear the DMA DONE interrupt */
    pTrans[i].pPort = pTrans[pDevCtx->bDmaCsrIndex].pPort;
    pTrans[i].cmdTrans = WDC_ADDR_IS_MEM(pAddrDesc) ? WM_BYTE : WP_BYTE;
    pTrans[i].Data.Byte = (BYTE)BIT3;
    i++;

    return i;
}

static DWORD IntEnableDma(PWDC_DEVICE pDev, PLX_INT_HANDLER funcDiagIntHandler,
    PLX_DMA_HANDLE hDma)
{
    DWORD dwStatus;
    PPLX_DEV_CTX pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(pDev);
    WD_TRANSFER *pTrans;
    UINT32 u32INTCSR = 0;
    UINT32 u32DMAMODE = 0;
    PLX_DMA_STRUCT *pPLXDma = (PLX_DMA_STRUCT *)hDma;
    DWORD dwNumCmds;

    /* Validate DMA handle and verify that DMA is open */
    if (!pPLXDma || !pPLXDma->pDma)
    {
        ErrLog("IntEnableDma: Error - %s (Device handle 0x%p, "
            "DMA handle 0x%p)\n", !pPLXDma ? "NULL DMA handle (pData)" :
            "DMA is not open", pDev, pPLXDma);
        return WD_INVALID_PARAMETER;
    }

    /* Define the number of interrupt transfer commands to use */
#define NUM_TRANS_CMDS_MASTER 4

/* Allocate memory for the interrupt transfer commands */
    pTrans = (WD_TRANSFER *)calloc(NUM_TRANS_CMDS_MASTER, sizeof(WD_TRANSFER));
    if (!pTrans)
    {
        ErrLog("Failed allocating memory for interrupt transfer commands\n");
        return WD_INSUFFICIENT_RESOURCES;
    }

    dwNumCmds = getTransCmds(pDev, pPLXDma, pTrans);

    /* Init the diag interrupt handler routine, which will be executed by
       PLX_IntHandler() when an interrupt is received */
    pDevCtx->funcDiagIntHandler = funcDiagIntHandler;

    /* Enable the interrupts */
    dwStatus = WDC_IntEnable(pDev, pTrans, dwNumCmds, INTERRUPT_CMD_COPY,
        PLX_IntHandlerDma, (PVOID)pDev, FALSE);

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("IntEnableDma: Failed enabling interrupts (vid 0x%x did 0x%x, "
            "handle 0x%p).\nError 0x%x - %s\n", pDevCtx->id.dwVendorId,
            pDevCtx->id.dwDeviceId, pDev, dwStatus, Stat2Str(dwStatus));

        goto Error;
    }

    /* Physically enable the interrupts on the board */
    /* DMA DONE interrupt enable, route DMA channel interrupt to PCI interrupt
     */
    PLX_ReadReg32(pDev, pPLXDma->dwDMAMODE, &u32DMAMODE);
    PLX_WriteReg32(pDev, pPLXDma->dwDMAMODE, u32DMAMODE | BIT10 | BIT17);

    /* PCI interrupt enable, DMA channel local interrupt enable */
    PLX_ReadReg32(pDev, pDevCtx->dwINTCSR, &u32INTCSR);
    PLX_WriteReg32(pDev, pDevCtx->dwINTCSR,
        u32INTCSR | BIT8 | ((PLX_DMA_CHANNEL_0 == pPLXDma->dmaChannel) ?
        BIT18 : BIT19));

    /* Store the interrupt transfer commands in the device context */
    pDevCtx->pIntTransCmds = pTrans;

    return WD_STATUS_SUCCESS;

Error:
    if (pTrans)
        free(pTrans);

    return dwStatus;
}

static DWORD IntEnable(PWDC_DEVICE pDev, PLX_INT_HANDLER funcDiagIntHandler)
{
    DWORD dwStatus;
    PPLX_DEV_CTX pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(pDev);
    WDC_ADDR_DESC *pAddrDesc;
    WD_TRANSFER *pTrans;
    WORD wINTCSR = 0;
    int i;

    /* Define the number of interrupt transfer commands to use */
    #define NUM_TRANS_CMDS_TARGET 3

    /* Allocate memory for the interrupt transfer commands */
    pTrans = (WD_TRANSFER *)calloc(NUM_TRANS_CMDS_TARGET, sizeof(WD_TRANSFER));
    if (!pTrans)
    {
        ErrLog("Failed allocating memory for interrupt transfer commands\n");
        return WD_INSUFFICIENT_RESOURCES;
    }

    /* Read the value of the INTCSR register */
    dwStatus = PLX_ReadReg16(pDev, pDevCtx->dwINTCSR, &wINTCSR);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("IntEnable: Failed reading from the INTCSR register "
            "(vid 0x%x did 0x%x, handle 0x%p)\nError 0x%x - %s\n",
            pDevCtx->id.dwVendorId, pDevCtx->id.dwDeviceId, pDev, dwStatus,
            Stat2Str(dwStatus));
        goto Error;
    }

    /* Prepare the interrupt transfer commands */
    /* The transfer commands will be executed by WinDriver in the kernel
       for each interrupt that is received */

    i = 0;

    /* Read status from the INTCSR register */
    pAddrDesc = &pDev->pAddrDesc[PLX_ADDR_REG];
    pTrans[i].pPort = pAddrDesc->pAddr + pDevCtx->dwINTCSR;
    pTrans[i].cmdTrans = WDC_ADDR_IS_MEM(pAddrDesc) ? RM_WORD : RP_WORD;
    pDevCtx->bIntCsrIndex = i;
    i++;

    /* Mask interrupt status from the INTCSR register */
    pTrans[i].cmdTrans = CMD_MASK;
    pTrans[i].Data.Word = PLX_TARGET_INT_MASK;
    i++;

    /* Write to the INTCSR register to clear the interrupt */
    pTrans[i].pPort = pTrans[pDevCtx->bIntCsrIndex].pPort;
    pTrans[i].cmdTrans = WDC_ADDR_IS_MEM(pAddrDesc) ? WM_WORD : WP_WORD;
    pTrans[i].Data.Word = wINTCSR & ~(BIT8 | BIT10);
    i++;

    /* Store the diag interrupt handler routine, which will be executed by
       PLX_IntHandler() when an interrupt is received */
    pDevCtx->funcDiagIntHandler = funcDiagIntHandler;

    /* Enable the interrupts */
    dwStatus = WDC_IntEnable(pDev, pTrans, i,
        INTERRUPT_CMD_COPY, PLX_IntHandler, (PVOID)pDev, FALSE);

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("IntEnable: Failed enabling interrupts (vid 0x%x did 0x%x, "
            "handle 0x%p).\nError 0x%x - %s\n", pDevCtx->id.dwVendorId,
            pDevCtx->id.dwDeviceId, pDev, dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    /* Physically enable the interrupts on the board */
    dwStatus = PLX_WriteReg16(pDev, pDevCtx->dwINTCSR,
        (WORD)(wINTCSR | BIT8 | BIT10));
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("IntEnable: Faild writing to the INTCSR register to physically "
            "enable the interrupts on the board (vid 0x%x did 0x%x, "
            "handle 0x%p)\nError 0x%x - %s\n", pDevCtx->id.dwVendorId,
            pDevCtx->id.dwDeviceId, pDev, dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    /* Store the interrupt transfer commands in the device context */
    pDevCtx->pIntTransCmds = pTrans;

    return WD_STATUS_SUCCESS;

Error:
    if (pTrans)
        free(pTrans);

    return dwStatus;
}

DWORD PLX_IntEnable(WDC_DEVICE_HANDLE hDev, PLX_INT_HANDLER funcDiagIntHandler,
    PVOID pData)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    TraceLog("PLX_IntEnable entered. Device handle: 0x%p\n", hDev);

    if (!IsValidDevice(pDev, "PLX_IntEnable"))
        return WD_INVALID_PARAMETER;

    /* Check if interrupts are already enabled */
    if (WDC_IntIsEnabled(hDev))
    {
        ErrLog("Interrupts are already enabled ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    dwStatus = ((PPLX_DEV_CTX)(pDev->pCtx))->fIsMaster ?
        IntEnableDma(pDev, funcDiagIntHandler, (PLX_DMA_HANDLE)pData) :
        IntEnable(pDev, funcDiagIntHandler);

    if (WD_STATUS_SUCCESS == dwStatus)
        TraceLog("PLX_IntEnable: Interrupts enabled\n");

    return dwStatus;
}

static DWORD IntDisableDma(WDC_DEVICE_HANDLE hDev)
{
    UINT32 u32INTCSR = 0, u32DMAMODE = 0;

    PLX_ReadReg32(hDev, PLX_M_INTCSR, &u32INTCSR);
    PLX_WriteReg32(hDev, PLX_M_INTCSR,
        u32INTCSR & ~(BIT8 | BIT18 | BIT19));

    PLX_ReadReg32(hDev, PLX_M_DMAMODE0, &u32DMAMODE);
    PLX_WriteReg32(hDev, PLX_M_DMAMODE0, u32DMAMODE & ~BIT10);

    PLX_ReadReg32(hDev, PLX_M_DMAMODE1, &u32DMAMODE);
    PLX_WriteReg32(hDev, PLX_M_DMAMODE1, u32DMAMODE & ~BIT10);

    return WD_STATUS_SUCCESS;
}

static DWORD IntDisable(WDC_DEVICE_HANDLE hDev)
{
    WORD wINTCSR = 0;

    PLX_ReadReg16(hDev, PLX_T_INTCSR, &wINTCSR);
    PLX_WriteReg16(hDev, PLX_T_INTCSR, (WORD)(wINTCSR & ~(BIT8 | BIT10)));

    return WD_STATUS_SUCCESS;
}

DWORD PLX_IntDisable(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PPLX_DEV_CTX pDevCtx;

    TraceLog("PLX_IntDisable entered. Device handle: 0x%p\n", hDev);

    if (!IsValidDevice(pDev, "PLX_IntDisable"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(hDev);

    if (!WDC_IntIsEnabled(hDev))
    {
        ErrLog("Interrupts are already disabled ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Physically disable the interrupts on the board */
    dwStatus = ((PPLX_DEV_CTX)(pDev->pCtx))->fIsMaster ?
        IntDisableDma(hDev) : IntDisable(hDev);

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        TraceLog("PLX_IntDisable: Physically disabled the interrupts on the "
            "board\n");
    }

    /* Disable the interrupts */
    dwStatus = WDC_IntDisable(hDev);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed disabling interrupts. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
    }

    /* Free the memory allocated for the interrupt transfer commands */
    if (pDevCtx->pIntTransCmds)
    {
        free(pDevCtx->pIntTransCmds);
        pDevCtx->pIntTransCmds = NULL;
    }

    return dwStatus;
}

BOOL PLX_IntIsEnabled(WDC_DEVICE_HANDLE hDev)
{
    if (!IsValidDevice((PWDC_DEVICE)hDev, "PLX_IntIsEnabled"))
        return FALSE;

    return WDC_IntIsEnabled(hDev);
}

/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */
static void PLX_EventHandler(WD_EVENT *pEvent, PVOID pData)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PPLX_DEV_CTX pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(pDev);

    TraceLog("PLX_EventHandler entered, pData 0x%p, dwAction 0x%x\n",
        pData, pEvent->dwAction);

    /* Execute the diagnostics application's event handler function */
    pDevCtx->funcDiagEventHandler(pDev, pEvent->dwAction);
}

DWORD PLX_EventRegister(WDC_DEVICE_HANDLE hDev,
    PLX_EVENT_HANDLER funcEventHandler)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PPLX_DEV_CTX pDevCtx;
    DWORD dwActions = WD_ACTIONS_ALL;
    /* TODO: Modify the above to set up the plug-and-play/power management
             events for which you wish to receive notifications.
             dwActions can be set to any combination of the WD_EVENT_ACTION
             flags defined in windrvr.h */

    TraceLog("PLX_EventRegister entered. Device handle: 0x%p\n", hDev);

    if (!IsValidDevice(pDev, "PLX_EventRegister"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(hDev);

    /* Check if event is already registered */
    if (WDC_EventIsRegistered(hDev))
    {
        ErrLog("Events are already registered ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Store the diag event handler routine to be executed from
     * PLX_EventHandler() upon an event */
    pDevCtx->funcDiagEventHandler = funcEventHandler;

    /* Register event */
    dwStatus = WDC_EventRegister(hDev, dwActions, PLX_EventHandler, hDev,
        FALSE);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to register events. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    TraceLog("Events registered\n");

    return WD_STATUS_SUCCESS;
}

DWORD PLX_EventUnregister(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;

    TraceLog("PLX_EventUnregister entered. Device handle: 0x%p\n", hDev);

    if (!IsValidDevice((PWDC_DEVICE)hDev, "PLX_EventUnregister"))
        return WD_INVALID_PARAMETER;

    if (!WDC_EventIsRegistered(hDev))
    {
        ErrLog("Cannot unregister events - no events currently "
            "registered...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    dwStatus = WDC_EventUnregister(hDev);

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to unregister events. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

BOOL PLX_EventIsRegistered(WDC_DEVICE_HANDLE hDev)
{
    if (!IsValidDevice((PWDC_DEVICE)hDev, "PLX_EventIsRegistered"))
        return FALSE;

    return WDC_EventIsRegistered(hDev);
}

/* -----------------------------------------------
    DMA (Direct Memory Access)
   ----------------------------------------------- */

void setOffsetsOfDMARegisters(PLX_DMA_STRUCT *pPLXDma)
{
    PLX_DMA_CHANNEL dmaChannel = pPLXDma->dmaChannel;

    pPLXDma->dwDMACSR = (PLX_DMA_CHANNEL_0 == dmaChannel ?
        PLX_M_DMACSR0 : PLX_M_DMACSR1);
    pPLXDma->dwDMAMODE = (PLX_DMA_CHANNEL_0 == dmaChannel ?
        PLX_M_DMAMODE0 : PLX_M_DMAMODE1);
    pPLXDma->dwDMAPADR = (PLX_DMA_CHANNEL_0 == dmaChannel ?
        PLX_M_DMAPADR0 : PLX_M_DMAPADR1);
    pPLXDma->dwDMALADR = (PLX_DMA_CHANNEL_0 == dmaChannel ?
        PLX_M_DMALADR0 : PLX_M_DMALADR1);
    pPLXDma->dwDMADPR = (PLX_DMA_CHANNEL_0 == dmaChannel ?
        PLX_M_DMADPR0 : PLX_M_DMADPR1);
    pPLXDma->dwDMASIZ = (PLX_DMA_CHANNEL_0 == dmaChannel ?
        PLX_M_DMASIZ0 : PLX_M_DMASIZ1);
    pPLXDma->dwDMADAC = (PLX_DMA_CHANNEL_0 == dmaChannel ?
        PLX_M_DMADAC0 : PLX_M_DMADAC1);
    pPLXDma->u32AbortMask = (PLX_DMA_CHANNEL_0 == dmaChannel ? BIT25 : BIT26);
}

DWORD PLX_DMAOpen(WDC_DEVICE_HANDLE hDev, UINT32 u32LocalAddr, PVOID *ppBuf,
    DWORD dwOptions, DWORD dwBytes, WDC_ADDR_MODE mode,
    PLX_DMA_CHANNEL dmaChannel, PLX_DMA_HANDLE *ppDmaHandle,
    BOOL fIsDACSupported)
{
    DWORD dwStatus;
    PPLX_DEV_CTX pDevCtx;
    UINT32 u32DMAMODE;
    BOOL fAutoinc = TRUE;
    PLX_DMA_STRUCT *pPLXDma = NULL;
    BOOL fSG = !(dwOptions & DMA_KERNEL_BUFFER_ALLOC);
    BOOL fIsRead = dwOptions & DMA_FROM_DEVICE ? TRUE : FALSE;
    WD_DMA *pDma;

    TraceLog("PLX_DMAOpen entered. Device handle: 0x%p\n", hDev);

    if (!IsValidDevice((PWDC_DEVICE)hDev, "PLX_DMAOpen"))
        return WD_INVALID_PARAMETER;

    if (!ppBuf)
    {
        ErrLog("PLX_DMAOpen: Error - NULL DMA buffer pointer\n");
        return WD_INVALID_PARAMETER;
    }

    pPLXDma = (PLX_DMA_STRUCT *)malloc(sizeof(PLX_DMA_STRUCT));
    if (!pPLXDma)
    {
        ErrLog("PLX_DMAOpen: Failed allocating memory for PLX DMA struct\n");
        return WD_INSUFFICIENT_RESOURCES;
    }
    BZERO(*pPLXDma);

    /* Allocate and lock a DMA buffer */
    dwStatus = fSG ?
        WDC_DMASGBufLock(hDev, *ppBuf, dwOptions, dwBytes, &pPLXDma->pDma) :
        WDC_DMAContigBufLock(hDev, ppBuf, dwOptions, dwBytes, &pPLXDma->pDma);

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("PLX_DMAOpen: Failed locking a DMA buffer. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    pPLXDma->dmaChannel = dmaChannel;
    setOffsetsOfDMARegisters(pPLXDma);

    if (fIsDACSupported)
    {
        /* Make sure the DMA DAC (Dual Address Cycle) is set to 0 */
        PLX_WriteReg32(hDev, pPLXDma->dwDMADAC, 0);
    }

    /* Common settings for chain and direct DMA */
    u32DMAMODE =
        (fAutoinc ? 0 : BIT11)
        | BIT6 /* Enable Ready input */
        | BIT8 /* Local burst */
        | ((WDC_MODE_8 == mode) ? 0 : (WDC_MODE_16 == mode) ?
            BIT0 : (BIT1 | BIT0));

    pDma = (WD_DMA *)pPLXDma->pDma;
    if (pDma->dwPages == 1)
    {
        /* DMA of one page ==> direct DMA */
        PLX_WriteReg32(hDev, pPLXDma->dwDMAMODE, u32DMAMODE);
        PLX_WriteReg32(hDev, pPLXDma->dwDMAPADR,
            (UINT32)pDma->Page[0].pPhysicalAddr);
        PLX_WriteReg32(hDev, pPLXDma->dwDMALADR, u32LocalAddr);
        PLX_WriteReg32(hDev, pPLXDma->dwDMASIZ,
            (UINT32)pDma->Page[0].dwBytes);
        PLX_WriteReg32(hDev, pPLXDma->dwDMADPR, fIsRead ? BIT3 : 0);
    }
    else /* DMA of more then one page ==> chain DMA */
    {
        typedef struct {
            UINT32 u32PADR;
            UINT32 u32LADR;
            UINT32 u32SIZ;
            UINT32 u32DPR;
        } DMA_LIST;

        DMA_LIST *pList = NULL;
        DWORD dwPageNumber;
        UINT32 u32StartOfChain, u32AlignShift, u32MemoryCopied;

        /* Allocate a kernel buffer to hold the chain of DMA descriptors
           includes extra 0x10 bytes for quadword alignment */
        dwStatus = WDC_DMAContigBufLock(hDev, (PVOID *)&pList, DMA_TO_DEVICE,
            pDma->dwPages * sizeof(DMA_LIST) + 0x10,
            &pPLXDma->pDmaList);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("PLX_DMAOpen: Failed locking DMA list buffer. "
                "Error 0x%x - %s\n", dwStatus, Stat2Str(dwStatus));
            goto Error;
        }

#if defined(WIN32)
    // TODO: remove pragma
    #pragma warning( push )
    #pragma warning( disable : 4311 )
#endif

        /* Verification that bits 0-3 are zero (QUADWORD aligned) */
        u32AlignShift =
            0x10 - ((UINT32)(unsigned long)pPLXDma->pDmaList->pUserAddr & 0xF);
        pList = (DMA_LIST *)((UPTR)pList + u32AlignShift);
        u32StartOfChain = (UINT32)pPLXDma->pDmaList->Page[0].pPhysicalAddr +
            u32AlignShift;
        pPLXDma->u32StartOfChain = u32StartOfChain;

#if defined(WIN32)
    #pragma warning( pop )
#endif

        /* Setting chain of DMA pages in the memory */
        for (dwPageNumber = 0, u32MemoryCopied = 0;
            dwPageNumber < pDma->dwPages; dwPageNumber++)
        {
            pList[dwPageNumber].u32PADR =
                (UINT32)pDma->Page[dwPageNumber].pPhysicalAddr;
            pList[dwPageNumber].u32LADR =
                u32LocalAddr + (fAutoinc ? u32MemoryCopied : 0);
            pList[dwPageNumber].u32SIZ =
                (UINT32)pDma->Page[dwPageNumber].dwBytes;
            pList[dwPageNumber].u32DPR =
                (u32StartOfChain + (UINT32)sizeof(DMA_LIST) *
                (dwPageNumber + 1)) | BIT0 | (fIsRead ? BIT3 : 0);
            u32MemoryCopied += pDma->Page[dwPageNumber].dwBytes;
        }

        pList[dwPageNumber - 1].u32DPR |= BIT1; /* Mark end of chain */

        PLX_WriteReg32(hDev, pPLXDma->dwDMAMODE, u32DMAMODE | BIT9); /* Chain
                                                                      * transfer
                                                                      */
        PLX_WriteReg32(hDev, pPLXDma->dwDMADPR, u32StartOfChain | BIT0);

        /* Flush the list of DMA descriptors from CPU cache to system memory */
        WDC_DMASyncCpu(pPLXDma->pDmaList);
    }

    *ppDmaHandle = (PLX_DMA_HANDLE)pPLXDma;

    /* update the device context */
    pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(hDev);
    pDevCtx->pPLXDma = pPLXDma;

    return WD_STATUS_SUCCESS;

Error:
    if (pPLXDma)
        PLX_DMAClose(hDev, (PLX_DMA_HANDLE)pPLXDma);

    return dwStatus;
}

#include <math.h>

static UINT32 ROUND_TO_PAGES(DWORD dwSize)
{
    return ((UINT64)dwSize + GetPageSize() - 1) & ~(GetPageSize() - 1);
}

static UINT32 BYTES_TO_PAGES(DWORD dwSize)
{
    UINT32 pageShift = (UINT32)log2(GetPageSize());
    return (dwSize >> pageShift) + ((dwSize & (GetPageSize() - 1)) != 0);
}

DWORD PLX_DMATransactionInit(WDC_DEVICE_HANDLE hDev, PVOID *ppBuf,
    DWORD dwOptions, DWORD dwBytes, PLX_DMA_CHANNEL dmaChannel,
    PLX_DMA_HANDLE *ppDmaHandle, PLX_INT_HANDLER MasterDiagDmaIntHandler,
    DWORD dwAlignment, DWORD dwMaxTransferSize, DWORD dwTransferElementSize)
{
    DWORD dwStatus;
    PPLX_DEV_CTX pDevCtx;
    PLX_DMA_STRUCT *pPLXDma = NULL;
    BOOL fSG = !(dwOptions & DMA_KERNEL_BUFFER_ALLOC);
    WD_TRANSFER *pTrans;
    PLX_DTE *pList = NULL;
    DWORD dwNumCmds;
    WDC_INTERRUPT_PARAMS intParams;

    TraceLog("%s entered. Device handle: 0x%p\n", __FUNCTION__, hDev);

    if (!IsValidDevice((PWDC_DEVICE)hDev, __FUNCTION__))
        return WD_INVALID_PARAMETER;

    if (!ppBuf)
    {
        ErrLog("%s: Error - NULL DMA buffer pointer\n", __FUNCTION__);
        return WD_INVALID_PARAMETER;
    }

    pPLXDma = (PLX_DMA_STRUCT *)malloc(sizeof(PLX_DMA_STRUCT));
    if (!pPLXDma)
    {
        ErrLog("%s: Failed allocating memory for PLX DMA struct\n",
            __FUNCTION__);
        return WD_INSUFFICIENT_RESOURCES;
    }
    BZERO(*pPLXDma);


    pTrans = (WD_TRANSFER *)calloc(NUM_TRANS_CMDS_MASTER, sizeof(WD_TRANSFER));
    if (!pTrans)
    {
        ErrLog("%s: Failed allocating memory for interrupt transfer commands\n",
            __FUNCTION__);
        return WD_INSUFFICIENT_RESOURCES;
    }

    pPLXDma->dmaChannel = dmaChannel;
    setOffsetsOfDMARegisters(pPLXDma);

    *ppDmaHandle = (PLX_DMA_HANDLE)pPLXDma;

    /* update the device context */
    pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(hDev);
    pDevCtx->pPLXDma = pPLXDma;
    pDevCtx->funcDiagIntHandler = MasterDiagDmaIntHandler;
    pDevCtx->pIntTransCmds = pTrans;

    dwNumCmds = getTransCmds((PWDC_DEVICE)hDev, pPLXDma, pTrans);

    intParams.pTransCmds = pTrans;
    intParams.dwNumCmds = dwNumCmds;
    intParams.dwOptions = INTERRUPT_CMD_COPY;
    intParams.funcIntHandler = PLX_IntHandlerDma;
    intParams.pData = hDev;
    intParams.fUseKP = FALSE;

    if (fSG)
    {
        DWORD dwPages = BYTES_TO_PAGES((ULONG)ROUND_TO_PAGES(dwMaxTransferSize)
            + GetPageSize());

        dwStatus = WDC_DMAContigBufLock(hDev, (void **)&pList, DMA_TO_DEVICE,
            dwPages * sizeof(PLX_DTE), &pPLXDma->pDmaList);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("%s: Failed locking DMA list buffer. Error 0x%x - %s\n",
                __FUNCTION__, dwStatus, Stat2Str(dwStatus));
            goto Error;
        }

        dwStatus = WDC_DMATransactionSGInit(hDev, *ppBuf, dwOptions, dwBytes,
            &pPLXDma->pDma, &intParams, dwMaxTransferSize,
            dwTransferElementSize);
    }
    else
    {
        dwStatus = WDC_DMATransactionContigInit(hDev, ppBuf, dwOptions, dwBytes,
            &pPLXDma->pDma, &intParams, dwAlignment);
    }

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("%s: Failed initializing DMA transaction. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    return WD_STATUS_SUCCESS;

Error:
    if (pPLXDma)
        PLX_DMATransactionUninit(hDev, (PLX_DMA_HANDLE)pPLXDma);

    return dwStatus;
}

void PLX_DMATransactionUninit(WDC_DEVICE_HANDLE hDev, PLX_DMA_HANDLE hDma)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;
    PLX_DMA_STRUCT *pPLXDma = (PLX_DMA_STRUCT*)hDma;

    TraceLog("%s entered. Device handle: 0x%p\n", __FUNCTION__, hDev);

    if (!IsValidDevice((PWDC_DEVICE)hDev, __FUNCTION__))
        return;

    if (!hDma)
    {
        ErrLog("%s: NULL DMA handle (device handle 0x%p)\n", __FUNCTION__,
            hDev);
        return;
    }

    if (pPLXDma->pDma)
    {
        dwStatus = WDC_DMATransactionUninit((WD_DMA *)pPLXDma->pDma);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("%s: failed to uninitialize DMA transaction. Error 0x%x - "
                "%s\n", __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        }
    }
    else
    {
        TraceLog("%s: DMA transaction is not currently initialized... (device "
            "handle 0x%p, DMA handle 0x%p)\n", __FUNCTION__, hDev, hDma);
    }

    free(pPLXDma);
}

void PLX_DMAClose(WDC_DEVICE_HANDLE hDev, PLX_DMA_HANDLE hDma)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;
    PLX_DMA_STRUCT *pPLXDma = (PLX_DMA_STRUCT *)hDma;

    TraceLog("PLX_DMAClose entered. Device handle: 0x%p\n", hDev);

    if (!IsValidDevice((PWDC_DEVICE)hDev, "PLX_DMAClose"))
        return;

    if (!hDma)
    {
        ErrLog("PLX_DMAClose: NULL DMA handle (device handle 0x%p)\n", hDev);
        return;
    }

    if (pPLXDma->pDma)
    {
        dwStatus = WDC_DMABufUnlock(pPLXDma->pDma);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("PLX_DMAClose: Failed unlocking DMA buffer. "
                "Error 0x%x - %s\n", dwStatus, Stat2Str(dwStatus));
        }
    }
    else
    {
        TraceLog("PLX_DMAClose: DMA is not currently open ... "
            "(device handle 0x%p, DMA handle 0x%p)\n", hDev, hDma);
    }

    if (pPLXDma->pDmaList)
    {
        dwStatus = WDC_DMABufUnlock(pPLXDma->pDmaList);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("PLX_DMAClose: Failed unlocking DMA list buffer. "
                "Error 0x%x - %s\n", dwStatus, Stat2Str(dwStatus));
        }
    }

    free(pPLXDma);
}

void PLX_DMASyncCpu(PLX_DMA_HANDLE hDma)
{
    WDC_DMASyncCpu(((PLX_DMA_STRUCT *)hDma)->pDma);
}

void PLX_DMASyncIo(PLX_DMA_HANDLE hDma)
{
    WDC_DMASyncIo(((PLX_DMA_STRUCT *)hDma)->pDma);
}

void PLX_DMAStart(WDC_DEVICE_HANDLE hDev, PLX_DMA_HANDLE hDma)
{
    PLX_DMA_STRUCT *pPLXDma = (PLX_DMA_STRUCT *)hDma;

    PLX_DMASyncCpu(hDma);

    get_cur_time(&pPLXDma->dmaStartTime);

    if (pPLXDma->pDmaList)
    {
        PLX_WriteReg32(hDev, pPLXDma->dwDMADPR,
            pPLXDma->u32StartOfChain | BIT0);
    }

    PLX_WriteReg8(hDev, pPLXDma->dwDMACSR, (BYTE)(BIT0 | BIT1));
}

BOOL PLX_DMAIsAborted(WDC_DEVICE_HANDLE hDev, PLX_DMA_HANDLE hDma)
{
    UINT32 intcsr, mask = ((PLX_DMA_STRUCT *)hDma)->u32AbortMask;

    PLX_ReadReg32(hDev, PLX_M_INTCSR, &intcsr);

    return ((intcsr & mask) != mask);
}

BOOL PLX_DMAIsDone(WDC_DEVICE_HANDLE hDev, PLX_DMA_HANDLE hDma)
{
    BYTE dmacsr = 0;

    PLX_ReadReg8(hDev, ((PLX_DMA_STRUCT *)hDma)->dwDMACSR, &dmacsr);

    return (dmacsr & (BYTE)BIT4) == (BYTE)BIT4;
}

BOOL PLX_DMAPollCompletion(WDC_DEVICE_HANDLE hDev, PLX_DMA_HANDLE hDma)
{
    while (!PLX_DMAIsDone(hDev, hDma) && !PLX_DMAIsAborted(hDev, hDma))
        ;

    PLX_DMASyncIo(hDma);
    return !PLX_DMAIsAborted(hDev, hDma);
}
#endif

/* -----------------------------------------------
    Read/write local addresses
   ----------------------------------------------- */
#define MASK_LOCAL(hDev, addrSpace) \
    (((PWDC_DEVICE)hDev)->pAddrDesc[addrSpace].qwBytes - 1)
#define ADDR_SPACE_OFFSET(fIsMaster, addrSpace) \
    (((fIsMaster) ? 0xF0 : 0x4) * ((addrSpace) - PLX_ADDR_SPACE0))

static DWORD LocalAddrSetMode(WDC_DEVICE_HANDLE hDev, PLX_ADDR addrSpace,
    DWORD dwLocalAddr)
{
    PPLX_DEV_CTX pCtx = (PPLX_DEV_CTX)(((PWDC_DEVICE)hDev)->pCtx);
    KPTR pLocalBase;
    DWORD dwOffset;

    pLocalBase = (KPTR)((dwLocalAddr & ~MASK_LOCAL(hDev, addrSpace)) | BIT0);
    dwOffset = pCtx->dwLAS0BA + ADDR_SPACE_OFFSET(pCtx->fIsMaster, addrSpace);

    return WDC_WriteAddr32(hDev, PLX_ADDR_REG, dwOffset, (UINT32)pLocalBase);
}

DWORD PLX_ReadAddrLocalBlock(WDC_DEVICE_HANDLE hDev, PLX_ADDR addrSpace,
    DWORD dwLocalAddr, DWORD dwBytes, PVOID pData, WDC_ADDR_MODE mode,
    WDC_ADDR_RW_OPTIONS options)
{
    KPTR dwOffset = (KPTR)(MASK_LOCAL(hDev, addrSpace) & dwLocalAddr);

    LocalAddrSetMode(hDev, addrSpace, dwLocalAddr);

    return WDC_ReadAddrBlock(hDev, addrSpace, dwOffset, dwBytes, pData,
        mode, options);
}

DWORD PLX_WriteAddrLocalBlock(WDC_DEVICE_HANDLE hDev, PLX_ADDR addrSpace,
    DWORD dwLocalAddr, DWORD dwBytes, PVOID pData, WDC_ADDR_MODE mode,
    WDC_ADDR_RW_OPTIONS options)
{
    KPTR dwOffset = (KPTR)(MASK_LOCAL(hDev, addrSpace) & dwLocalAddr);

    LocalAddrSetMode(hDev, addrSpace, dwLocalAddr);

    return WDC_WriteAddrBlock(hDev, addrSpace, dwOffset, dwBytes, pData,
        mode, options);
}

DWORD PLX_ReadAddrLocal8(WDC_DEVICE_HANDLE hDev, PLX_ADDR addrSpace,
    DWORD dwLocalAddr, BYTE *pbData)
{
    KPTR dwOffset = (KPTR)(MASK_LOCAL(hDev, addrSpace) & dwLocalAddr);
    LocalAddrSetMode(hDev, addrSpace, dwLocalAddr);

    return WDC_ReadAddr8(hDev, addrSpace, dwOffset, pbData);
}

DWORD PLX_WriteAddrLocal8(WDC_DEVICE_HANDLE hDev, PLX_ADDR addrSpace,
    DWORD dwLocalAddr, BYTE bData)
{
    KPTR dwOffset = (KPTR)(MASK_LOCAL(hDev, addrSpace) & dwLocalAddr);
    LocalAddrSetMode(hDev, addrSpace, dwLocalAddr);

    return WDC_WriteAddr8(hDev, addrSpace, dwOffset, bData);
}

DWORD PLX_ReadAddrLocal16(WDC_DEVICE_HANDLE hDev, PLX_ADDR addrSpace,
    DWORD dwLocalAddr, WORD *pwData)
{
    KPTR dwOffset = (KPTR)(MASK_LOCAL(hDev, addrSpace) & dwLocalAddr);
    LocalAddrSetMode(hDev, addrSpace, dwLocalAddr);

    return WDC_ReadAddr16(hDev, addrSpace, dwOffset, pwData);
}

DWORD PLX_WriteAddrLocal16(WDC_DEVICE_HANDLE hDev, PLX_ADDR addrSpace,
    DWORD dwLocalAddr, WORD wData)
{
    KPTR dwOffset = (KPTR)(MASK_LOCAL(hDev, addrSpace) & dwLocalAddr);
    LocalAddrSetMode(hDev, addrSpace, dwLocalAddr);

    return WDC_WriteAddr16(hDev, addrSpace, dwOffset, wData);
}

DWORD PLX_ReadAddrLocal32(WDC_DEVICE_HANDLE hDev, PLX_ADDR addrSpace,
    DWORD dwLocalAddr, UINT32 *pu32Data)
{
    KPTR dwOffset = (KPTR)(MASK_LOCAL(hDev, addrSpace) & dwLocalAddr);
    LocalAddrSetMode(hDev, addrSpace, dwLocalAddr);

    return WDC_ReadAddr32(hDev, addrSpace, dwOffset, pu32Data);
}

DWORD PLX_WriteAddrLocal32(WDC_DEVICE_HANDLE hDev, PLX_ADDR addrSpace,
    DWORD dwLocalAddr, UINT32 u32Data)
{
    KPTR dwOffset = (KPTR)(MASK_LOCAL(hDev, addrSpace) & dwLocalAddr);
    LocalAddrSetMode(hDev, addrSpace, dwLocalAddr);

    return WDC_WriteAddr32(hDev, addrSpace, dwOffset, u32Data);
}

DWORD PLX_ReadAddrLocal64(WDC_DEVICE_HANDLE hDev, PLX_ADDR addrSpace,
    DWORD dwLocalAddr, UINT64 *pu64Data)
{
    KPTR dwOffset = (KPTR)(MASK_LOCAL(hDev, addrSpace) & dwLocalAddr);
    LocalAddrSetMode(hDev, addrSpace, dwLocalAddr);

    return WDC_ReadAddr64(hDev, addrSpace, dwOffset, pu64Data);
}

DWORD PLX_WriteAddrLocal64(WDC_DEVICE_HANDLE hDev, PLX_ADDR addrSpace,
    DWORD dwLocalAddr, UINT64 u64Data)
{
    KPTR dwOffset = (KPTR)(MASK_LOCAL(hDev, addrSpace) & dwLocalAddr);
    LocalAddrSetMode(hDev, addrSpace, dwLocalAddr);

    return WDC_WriteAddr64(hDev, addrSpace, dwOffset, u64Data);
}

/* -----------------------------------------------
    Access the serial EEPROM
   ----------------------------------------------- */
BOOL PLX_EEPROMIsPresent(WDC_DEVICE_HANDLE hDev)
{
    PPLX_DEV_CTX pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(hDev);
    UINT32 u32CNTRL = 0;

    PLX_ReadReg32(hDev, pDevCtx->dwCNTRL, &u32CNTRL);

    return (u32CNTRL & BIT28) == BIT28;
}

/* VPD EEPROM access */
BOOL PLX_EEPROM_VPD_Validate(WDC_DEVICE_HANDLE hDev)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    BYTE bData = 0;

    TraceLog("PLX_EEPROM_VPD_Validate entered. Device handle 0x%p\n", hDev);

    if (!IsValidDevice(pDev, "PLX_EEPROM_VPD_Validate"))
        return FALSE;

    /* Verify that a blank or programmed serial EEPROM is present */
    if (!PLX_EEPROMIsPresent(hDev))
    {
        ErrLog("PLX_EEPROM_VPD_Validate: Error - serial EEPROM was not found "
            "on board (handle 0x%p)\n", hDev);
        return FALSE;
    }

    /* Check the next capability pointers */
    WDC_PciReadCfg8(hDev, PCI_CAP, &bData);
    if (bData != (BYTE)PLX_PMCAPID)
    {
        ErrLog("PLX_EEPROM_VPD_Validate: NEW_CAP register validation failed\n");
        return FALSE;
    }

    WDC_PciReadCfg8(hDev, PLX_PMNEXT, &bData);
    if (bData != (BYTE)PLX_HS_CAPID)
    {
        ErrLog("PLX_EEPROM_VPD_Validate: PMNEXT register validation failed\n");
        return FALSE;
    }

    WDC_PciReadCfg8(hDev, PLX_HS_NEXT, &bData);
    if (bData != (BYTE)PLX_VPD_CAPID)
    {
        ErrLog("PLX_EEPROM_VPD_Validate: HS_NEXT register validation failed\n");
        return FALSE;
    }

    WDC_PciReadCfg8(hDev, PLX_VPD_NEXT, &bData);
    if (bData != 0)
    {
        ErrLog("PLX_EEPROM_VPD_Validate: VPD_NEXT register validation "
            "failed\n");
        return FALSE;
    }

    return TRUE;
}

DWORD PLX_EEPROM_VPD_Read32(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    UINT32 *pu32Data)
{
    DWORD i;
    UINT32 u32EnableAccess;
    WORD wAddr, wData;

    if (!IsValidDevice((PWDC_DEVICE)hDev, "PLX_EEPROM_VPD_Read32"))
        return WD_INVALID_PARAMETER;

    if (dwOffset % 4)
    {
        ErrLog("PLX_EEPROM_VPD_Read32: Error - offset (0x%x) is not a "
            "multiple of 4 (device handle: 0x%p)\n", dwOffset, hDev);
        return WD_INVALID_PARAMETER;
    }

    /* Clear EEDO Input Enable */
    EEPROM_VPD_EnableAccess(hDev, &u32EnableAccess);

    /* Write a destination serial EEPROM address and flag of operation,
     * value of 0 */
    wAddr = (WORD)(dwOffset & ~BIT15);
    WDC_PciWriteCfg16(hDev, PLX_VPD_ADDR, wAddr);

    /* Probe a flag of operation until it changes to a 1 to ensure the Read data
     * is available */
    for (i = 0; i < 10; i++)
    {
        EEPROM_VPD_Delay();
        WDC_PciReadCfg16(hDev, PLX_VPD_ADDR, &wData);

        if (wData & BIT15)
            break;
    }

    if (i == 10)
    {
        ErrLog("PLX_EEPROM_VPD_Read32: Error - Acknowledge to EEPROM read was "
            "not received (device handle 0x%p)\n", hDev);
        return WD_OPERATION_FAILED;
    }

    /* Read back the requested data from PVPDATA register */
    WDC_PciReadCfg32(hDev, PLX_VPD_DATA, pu32Data);

    /* Restore EEDO Input Enable */
    EEPROM_VPD_RestoreAccess(hDev, u32EnableAccess);

    return WD_STATUS_SUCCESS;
}

DWORD PLX_EEPROM_VPD_Write32(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    UINT32 u32Data)
{
    DWORD i;
    UINT32 u32ReadBack, u32EnableAccess;
    WORD wAddr, wData;
    BYTE bEnableOffset;

    if (!IsValidDevice((PWDC_DEVICE)hDev, "PLX_EEPROM_VPD_Write32"))
        return WD_INVALID_PARAMETER;

    if (dwOffset % 4)
    {
        ErrLog("PLX_EEPROM_VPD_Write32: Error - offset (0x%x) is not a "
            "multiple of 4 (device handle: 0x%p)\n", dwOffset, hDev);
        return WD_INVALID_PARAMETER;
    }

    /* Clear EEDO Input Enable */
    EEPROM_VPD_EnableAccess(hDev, &u32EnableAccess);

    wAddr = (WORD)dwOffset;
    EEPROM_VPD_RemoveWriteProtection(hDev, wAddr, &bEnableOffset);

    EEPROM_VPD_Delay();

    /* Write desired data to PVPDATA register */
    WDC_PciWriteCfg32(hDev, PLX_VPD_DATA, u32Data);

    /* Write a destination serial EEPROM address and flag of operation,
     * value of 1 */
    wAddr = (WORD)(wAddr | BIT15);
    WDC_PciWriteCfg16(hDev, PLX_VPD_ADDR, wAddr);

    /* Probe a flag of operation until it changes to a 0 to ensure the write
     * completes */
    for (i = 0; i < 10; i++)
    {
        EEPROM_VPD_Delay();
        WDC_PciReadCfg16(hDev, PLX_VPD_ADDR, &wData);
        if (wData & BIT15)
            break;
    }

    EEPROM_VPD_RestoreWriteProtection((PWDC_DEVICE)hDev, bEnableOffset);

    /* Restore EEDO Input Enable */
    EEPROM_VPD_RestoreAccess(hDev, u32EnableAccess);

    PLX_EEPROM_VPD_Read32(hDev, dwOffset, &u32ReadBack);

    if (u32ReadBack != u32Data)
    {
        ErrLog("PLX_EEPROM_VPD_Write32: Error - Wrote 0x%08X, read back 0x%08X "
            "(device handle 0x%p)\n", u32Data, u32ReadBack);
        return WD_OPERATION_FAILED;
    }

    return WD_STATUS_SUCCESS;
}

/* Enable EEPROM access via VPD mechanism - disable EEDO Input (CNTRL[31]=0,
 * default).  This bit is specific to PLX 9656 and PLX 9056 chips (it is
 * reserved on other boards) */
static DWORD EEPROM_VPD_EnableAccess(WDC_DEVICE_HANDLE hDev,
    UINT32 *pu32DataOld)
{
    DWORD dwCNTRL = ((PPLX_DEV_CTX)(((PWDC_DEVICE)hDev)->pCtx))->dwCNTRL;

    PLX_ReadReg32(hDev, dwCNTRL, pu32DataOld);

    return PLX_WriteReg32(hDev, dwCNTRL, *pu32DataOld & ~BIT31);
}

/* Restore EEDO Input Enable */
static DWORD EEPROM_VPD_RestoreAccess(WDC_DEVICE_HANDLE hDev, UINT32 u32Data)
{
    return PLX_WriteReg32(hDev,
        ((PPLX_DEV_CTX)((((PWDC_DEVICE)hDev)->pCtx)))->dwCNTRL, u32Data);
}

static DWORD EEPROM_VPD_RemoveWriteProtection(WDC_DEVICE_HANDLE hDev,
    WORD wAddr, PBYTE pbDataOld)
{
    PPLX_DEV_CTX pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(hDev);

    PLX_ReadReg8(hDev, pDevCtx->dwPROT_AREA, pbDataOld);

    wAddr /= 4;
    wAddr &= 0x7F;

    PLX_WriteReg8(hDev, pDevCtx->dwPROT_AREA, (BYTE)wAddr);

    *pbDataOld *= 4; /* Expand from DWORD to BYTE count */

    return WD_STATUS_SUCCESS;
}

static DWORD EEPROM_VPD_RestoreWriteProtection(WDC_DEVICE_HANDLE hDev,
    WORD wAddr)
{
    PPLX_DEV_CTX pDevCtx = (PPLX_DEV_CTX)WDC_GetDevContext(hDev);
    return PLX_WriteReg8(hDev, pDevCtx->dwPROT_AREA, (BYTE)wAddr);
}

DWORD PLX_EEPROM_RT_Read16(WDC_DEVICE_HANDLE hDev, DWORD dwOffset, PWORD pwData,
    DWORD EEPROMmsb)
{
    WORD i;
    DWORD dwAddr = dwOffset >> 1;
    BOOL bit = 0;

    if (!IsValidDevice((PWDC_DEVICE)hDev, "PLX_EEPROM_RT_Read16"))
        return WD_INVALID_PARAMETER;

    *pwData = 0;

    EEPROM_RT_ChipSelect(hDev, TRUE);
    EEPROM_RT_WriteBit(hDev, 1);
    EEPROM_RT_WriteBit(hDev, 1);
    EEPROM_RT_WriteBit(hDev, 0);

    /* CS06, CS46 EEPROM - send 6bit address
     * CS56, CS66 EEPROM - send 8bit address */
    for (i = (WORD)EEPROMmsb; i; i >>= 1)
        EEPROM_RT_WriteBit(hDev, (dwAddr & i) == i);

    for (i = BIT15; i; i >>= 1)
    {
        EEPROM_RT_ReadBit(hDev, &bit);
        *pwData |= (bit ? i : 0);
    }

    EEPROM_RT_ChipSelect(hDev, FALSE);

    return WD_STATUS_SUCCESS;
}

DWORD PLX_EEPROM_RT_Write16(WDC_DEVICE_HANDLE hDev, DWORD dwOffset, WORD wData,
    DWORD EEPROMmsb)
{
    WORD i;
    DWORD dwAddr = dwOffset >> 1;
    WORD wReadBack;

    if (!IsValidDevice((PWDC_DEVICE)hDev, "PLX_EEPROM_RT_Write16"))
        return WD_INVALID_PARAMETER;

    EEPROM_RT_WriteEnableDisable(hDev, TRUE);

    EEPROM_RT_ChipSelect(hDev, TRUE);

    /* Send a PRWRITE instruction */
    EEPROM_RT_WriteBit(hDev, 1);
    EEPROM_RT_WriteBit(hDev, 0);
    EEPROM_RT_WriteBit(hDev, 1);

    /* CS06, CS46 EEPROM - send 6bit address
     * CS56, CS66 EEPROM - send 8bit address */
    for (i = (WORD)EEPROMmsb; i; i >>= 1)
        EEPROM_RT_WriteBit(hDev, (dwAddr & i) == i);

    for (i = BIT15; i; i >>= 1)
        EEPROM_RT_WriteBit(hDev, (wData & i) == i);

    EEPROM_RT_ChipSelect(hDev, FALSE);

    EEPROM_RT_WriteEnableDisable(hDev, TRUE);

    PLX_EEPROM_RT_Read16(hDev, dwOffset, &wReadBack, EEPROMmsb);

    if (wData != wReadBack)
    {
        ErrLog("PLX_EEPROM_RT_Write16: Error - Wrote 0x%04X, read back 0x%04X "
            "(device handle 0x%p)\n", wData, wReadBack);
        return WD_OPERATION_FAILED;
    }

    return WD_STATUS_SUCCESS;
}

DWORD PLX_EEPROM_RT_Read32(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    UINT32 *pu32Data, DWORD EEPROMmsb)
{
    WORD wData1, wData2;

    if (!IsValidDevice((PWDC_DEVICE)hDev, "PLX_EEPROM_RT_Read32"))
        return WD_INVALID_PARAMETER;

    if (dwOffset % 4)
    {
        ErrLog("PLX_EEPROM_RT_Read32: Error - offset (0x%x) is not a multiple "
            "of 4 (device handle: 0x%p)\n", dwOffset, hDev);
        return WD_INVALID_PARAMETER;
    }

    PLX_EEPROM_RT_Read16(hDev, dwOffset, &wData1, EEPROMmsb);
    PLX_EEPROM_RT_Read16(hDev, dwOffset + 2, &wData2, EEPROMmsb);

    *pu32Data = (UINT32)((wData1 << 16) + wData2);

    return WD_STATUS_SUCCESS;
}

DWORD PLX_EEPROM_RT_Write32(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    UINT32 u32Data, DWORD EEPROMmsb)
{
    WORD wData1, wData2;

    if (!IsValidDevice((PWDC_DEVICE)hDev, "PLX_EEPROM_RT_Write32"))
        return WD_INVALID_PARAMETER;

    if (dwOffset % 4)
    {
        ErrLog("PLX_EEPROM_RT_Write32: Error - offset (0x%x) is not a "
            "multiple of 4 (device handle: 0x%p)\n", dwOffset, hDev);
        return WD_INVALID_PARAMETER;
    }

    wData1 = (WORD)(u32Data >> 16);
    wData2 = (WORD)(u32Data & 0xFFFF);

    PLX_EEPROM_RT_Write16(hDev, dwOffset, wData1, EEPROMmsb);
    PLX_EEPROM_RT_Write16(hDev, dwOffset + 2, wData2, EEPROMmsb);

    return WD_STATUS_SUCCESS;
}

static void EEPROM_RT_ChipSelect(WDC_DEVICE_HANDLE hDev, BOOL fSelect)
{
    DWORD dwCNTRL = ((PPLX_DEV_CTX)(((PWDC_DEVICE)hDev)->pCtx))->dwCNTRL;
    UINT32 u32CNTRL;

    PLX_ReadReg32(hDev, dwCNTRL, &u32CNTRL);
    PLX_WriteReg32(hDev, dwCNTRL,
        fSelect ? (u32CNTRL | BIT25) : (u32CNTRL & ~BIT25) );

    EEPROM_RT_Delay();
}

static void EEPROM_RT_ReadBit(WDC_DEVICE_HANDLE hDev, BOOL *pBit)
{
    DWORD dwCNTRL = ((PPLX_DEV_CTX)(((PWDC_DEVICE)hDev)->pCtx))->dwCNTRL;
    UINT32 u32CNTRL;

    PLX_ReadReg32(hDev, dwCNTRL, &u32CNTRL);

    /* clock */
    PLX_WriteReg32(hDev, dwCNTRL, u32CNTRL & ~BIT24);
    EEPROM_RT_Delay();
    PLX_WriteReg32(hDev, dwCNTRL, u32CNTRL | BIT24);
    EEPROM_RT_Delay();
    PLX_WriteReg32(hDev, dwCNTRL, u32CNTRL & ~BIT24);
    EEPROM_RT_Delay();

    /* data */
    PLX_ReadReg32(hDev, dwCNTRL, &u32CNTRL);
    *pBit = (u32CNTRL & BIT27) == BIT27;
}

static void EEPROM_RT_WriteBit(WDC_DEVICE_HANDLE hDev, BOOL bit)
{
    DWORD dwCNTRL = ((PPLX_DEV_CTX)(((PWDC_DEVICE)hDev)->pCtx))->dwCNTRL;
    UINT32 u32CNTRL;

    PLX_ReadReg32(hDev, dwCNTRL, &u32CNTRL);

    if (bit) /* data */
        u32CNTRL |= BIT26;
    else
        u32CNTRL &= ~BIT26;

    /* clock */
    PLX_WriteReg32(hDev, dwCNTRL, u32CNTRL & ~BIT24);
    EEPROM_RT_Delay();
    PLX_WriteReg32(hDev, dwCNTRL, u32CNTRL | BIT24);
    EEPROM_RT_Delay();
    PLX_WriteReg32(hDev, dwCNTRL, u32CNTRL & ~BIT24);
    EEPROM_RT_Delay();
}

static void EEPROM_RT_WriteEnableDisable(WDC_DEVICE_HANDLE hDev, BOOL fEnable)
{
    EEPROM_RT_ChipSelect(hDev, TRUE);

    /* Send a WEN instruction */
    EEPROM_RT_WriteBit(hDev, 1);
    EEPROM_RT_WriteBit(hDev, 0);
    EEPROM_RT_WriteBit(hDev, 0);
    EEPROM_RT_WriteBit(hDev, fEnable ? 1 : 0);
    EEPROM_RT_WriteBit(hDev, fEnable ? 1 : 0);

    EEPROM_RT_WriteBit(hDev, 0);
    EEPROM_RT_WriteBit(hDev, 0);
    EEPROM_RT_WriteBit(hDev, 0);
    EEPROM_RT_WriteBit(hDev, 0);

    EEPROM_RT_ChipSelect(hDev, FALSE);
}

/* -----------------------------------------------
    Reset of PLX board
   ----------------------------------------------- */
/* Definition of abort bits in commad (CMD) configuration register */
#define PLX_M_CR_ABORT_BITS (\
    BIT24   /* Detected Master Data Parity Error */ \
    | BIT27 /* Signaled Target Abort */ \
    | BIT28 /* Received Target Abort */ \
    | BIT29 /* Received Master Abort */ \
    | BIT30 /* Signaled System Error */ \
    | BIT31 /* Detected Parity Error on PCI bus */ \
    )

/* Software board reset for master devices (9054, 9056, 9080, 9656) */
void PLX_SoftResetMaster(WDC_DEVICE_HANDLE hDev)
{
    BOOL fMUEnabled, fEEPROMPresent;
    WORD wCMD = 0;
    UINT32 u32QSR = 0;
    UINT32 u32CNTRL = 0;
    UINT32 u32INTLN = 0;
    UINT32 u32MBOX0 = 0, u32MBOX1 = 0;
    UINT32 u32HS_CNTL = 0;
    UINT32 u32PMCSR = 0;

    /* Clear any PCI errors */
    WDC_PciReadCfg16(hDev, PCI_CR, &wCMD);
    if (wCMD & PLX_M_CR_ABORT_BITS)
    {
        /* Write value back to clear aborts */
        WDC_PciWriteCfg16(hDev, PCI_CR, wCMD);
    }

    /* Save state of I2O Decode Enable */
    PLX_ReadReg32(hDev, PLX_M_QSR, &u32QSR);
    fMUEnabled = u32QSR & BIT0 ? TRUE : FALSE;

    /* Make sure S/W Reset & EEPROM reload bits are clear */
    PLX_ReadReg32(hDev, PLX_M_CNTRL, &u32CNTRL);
    u32CNTRL &= ~(BIT30 | BIT29);

    /* Determine if an EEPROM is present */
    fEEPROMPresent = PLX_EEPROMIsPresent(hDev);

    /* Save some registers if EEPROM present */
    if (fEEPROMPresent)
    {
        /* Run-time registers */
        PLX_ReadReg32(hDev, PLX_M_MBOX0, &u32MBOX0);
        PLX_ReadReg32(hDev, PLX_M_MBOX1, &u32MBOX1);

        /* PCI configuration registers */
        WDC_PciReadCfg32(hDev, PCI_ILR, &u32INTLN);
        WDC_PciReadCfg32(hDev, PLX_HS_CAPID, &u32HS_CNTL);
        WDC_PciReadCfg32(hDev, PLX_PMCSR, &u32PMCSR);
    }

    /* Issue Software Reset to hold PLX chip in reset */
    PLX_WriteReg32(hDev, PLX_M_CNTRL, u32CNTRL | BIT30);

    /* Bring chip out of reset */
    PLX_SOFT_RESET_DELAY();
    PLX_WriteReg32(hDev, PLX_M_CNTRL, u32CNTRL);

    /* Issue EEPROM reload in case now programmed */
    PLX_WriteReg32(hDev, PLX_M_CNTRL, u32CNTRL | BIT29);

    /* Clear EEPROM reload */
    PLX_SOFT_RESET_DELAY();
    PLX_WriteReg32(hDev, PLX_M_CNTRL, u32CNTRL);

    /* Restore I2O Decode Enable state */
    if (fMUEnabled)
    {
        PLX_ReadReg32(hDev, PLX_M_QSR, &u32QSR);
        PLX_WriteReg32(hDev, PLX_M_QSR, u32QSR | BIT0);
    }

    /* If EEPROM was present, restore registers */
    if (fEEPROMPresent)
    {
        /* Run-time registers */
        PLX_WriteReg32(hDev, PLX_M_MBOX0, u32MBOX0);
        PLX_WriteReg32(hDev, PLX_M_MBOX1, u32MBOX1);

        /* PCI configuration registers */
        WDC_PciReadCfg32(hDev, PCI_ILR, &u32INTLN);

        /* Mask out HS bits that can be cleared */
        u32HS_CNTL &= ~(BIT23 | BIT22 | BIT17);
        WDC_PciWriteCfg32(hDev, PLX_HS_CAPID, u32HS_CNTL);

        /* Mask out PM bits that can be cleared */
        u32PMCSR &= ~BIT15;
        WDC_PciWriteCfg32(hDev, PLX_PMCSR, u32PMCSR);
    }
}

/* -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- */
static void ErrLog(const CHAR *sFormat, ...)
{
    va_list argp;
    va_start(argp, sFormat);
    vsnprintf(gsPLX_LastErr, sizeof(gsPLX_LastErr) - 1, sFormat, argp);
#if defined(DEBUG)
    WDC_Err("PLX lib: %s", gsPLX_LastErr);
#endif
    va_end(argp);
}

static void TraceLog(const CHAR *sFormat, ...)
{
#if defined(DEBUG)
    CHAR sMsg[256];
    va_list argp;
    va_start(argp, sFormat);
    vsnprintf(sMsg, sizeof(sMsg) - 1, sFormat, argp);
    WDC_Trace("PLX lib: %s", sMsg);
    va_end(argp);
#else
    UNUSED_VAR(sFormat);
#endif
}

const char *PLX_GetLastErr(void)
{
    return gsPLX_LastErr;
}

