/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/****************************************************************************
*  File: bmd_lib.c
*
*  Implementation of a sample library for accessing Xilinx PCI Express cards
*  with BMD design, using the WinDriver WDC API.
*  The sample was tested with Xilinx's Virtex and Spartan development kits.
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*****************************************************************************/

#if defined(__KERNEL__)
    #include "kpstdlib.h"
#endif
#include "bmd_lib.h"

/*************************************************************
  Internal definitions
 *************************************************************/
/* WinDriver license registration string */
/* TODO: When using a registered WinDriver version, replace the license string
         below with the development license in order to use on the development
         machine.
         Once you require to distribute the driver's package to other machines,
         please replace the string with a distribution license */
#define BMD_DEFAULT_LICENSE_STRING "12345abcde12345.abcde"

#define BMD_DEFAULT_DRIVER_NAME WD_DEFAULT_DRIVER_NAME_BASE

/* Last error information string */
static CHAR gsBMD_LastErr[256];

/*************************************************************
  Static functions prototypes and inline implementation
 *************************************************************/
#if !defined(__KERNEL__)
static BOOL DeviceValidate(const PWDC_DEVICE pDev);
#endif
static void DLLCALLCONV BMD_IntHandler(PVOID pData);
static void BMD_EventHandler(WD_EVENT *pEvent, PVOID pData);
static void ErrLog(const CHAR *sFormat, ...);
static void TraceLog(const CHAR *sFormat, ...);

/* Validate a WDC device handle */
static inline BOOL IsValidDevice(PWDC_DEVICE pDev, const CHAR *sFunc)
{
    if (!pDev || !WDC_GetDevContext(pDev))
    {
        ErrLog("%s: NULL device %s\n", sFunc, !pDev ? "handle" : "context");
        return FALSE;
    }

    return TRUE;
}

/*************************************************************
  Functions implementation
 *************************************************************/
/* -----------------------------------------------
    BMD and WDC libraries initialize/uninitialize
   ----------------------------------------------- */
/* Initialize the Xilinx BMD and WDC libraries */
DWORD BMD_LibInit(void)
{
    DWORD dwStatus;

#if defined(WD_DRIVER_NAME_CHANGE)
    /* Set the driver name */
    if (!WD_DriverName(BMD_DEFAULT_DRIVER_NAME))
    {
        ErrLog("Failed to set the driver name for WDC library.\n");
        return WD_SYSTEM_INTERNAL_ERROR;
    }
#endif

    /* Set WDC library's debug options
     * (default: level=TRACE; redirect output to the Debug Monitor) */
    dwStatus = WDC_SetDebugOptions(WDC_DBG_DEFAULT, NULL);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to initialize debug options for WDC library.\n"
            "Error 0x%x - %s\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    /* Open a handle to the driver and initialize the WDC library */
    dwStatus = WDC_DriverOpen(WDC_DRV_OPEN_DEFAULT,
        BMD_DEFAULT_LICENSE_STRING);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to initialize the WDC library. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    return WD_STATUS_SUCCESS;
}

/* Uninitialize the Xilinx BMD and WDC libraries */
DWORD BMD_LibUninit(void)
{
    DWORD dwStatus;

    /* Uninitialize the WDC library and close the handle to WinDriver */
    dwStatus = WDC_DriverClose();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to uninit the WDC library. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

#if !defined(__KERNEL__)
/* -----------------------------------------------
    Device open/close
   ----------------------------------------------- */
/* Open a device handle */
WDC_DEVICE_HANDLE BMD_DeviceOpen(DWORD dwVendorId, DWORD dwDeviceId)
{
    WDC_DEVICE_HANDLE hDev = WDC_DIAG_DeviceFindAndOpen(dwVendorId,
        dwDeviceId, KP_BMD_DRIVER_NAME, sizeof(BMD_DEV_CTX));

    if (!hDev || !DeviceValidate((PWDC_DEVICE)hDev))
        goto Error;

    return hDev;

Error:
    if (hDev)
        BMD_DeviceClose(hDev);

    printf("BMD_DeviceOpen: Failed opening PCI device: %s",
        BMD_GetLastErr());

    return NULL;
}

/* Close a device handle */
BOOL BMD_DeviceClose(WDC_DEVICE_HANDLE hDev)
{
    /* Validate the WDC device handle */
    if (!hDev)
        return FALSE;

#ifdef HAS_INTS
    /* Disable interrupts (if enabled) */
    if (BMD_IntIsEnabled(hDev))
    {
        DWORD dwStatus = BMD_IntDisable(hDev);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("Failed disabling interrupts. Error 0x%x - %s\n",
                dwStatus, Stat2Str(dwStatus));
        }
    }
#endif /* ifdef HAS_INTS */

    return WDC_DIAG_DeviceClose(hDev);
}

/* Validate device information */
static BOOL DeviceValidate(const PWDC_DEVICE pDev)
{
    DWORD i, dwNumAddrSpaces = pDev->dwNumAddrSpaces;

    /* TODO: You can modify the implementation of this function in order to
             verify that the device has all expected resources. */

    /* Verify that the device has at least one active address space */
    for (i = 0; i < dwNumAddrSpaces; i++)
    {
        if (WDC_AddrSpaceIsActive(pDev, i))
            return TRUE;
    }

    ErrLog("Device does not have any active memory or I/O address spaces\n");
    return FALSE;
}

#ifdef HAS_INTS
/* -----------------------------------------------
    Interrupts
   ----------------------------------------------- */
/* Interrupt handler routine */
static void DLLCALLCONV BMD_IntHandler(PVOID pData)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PBMD_DEV_CTX pDevCtx = (PBMD_DEV_CTX)WDC_GetDevContext(pDev);
    BMD_INT_RESULT intResult;

    BZERO(intResult);
    intResult.dwCounter = pDev->Int.dwCounter;
    intResult.dwLost = pDev->Int.dwLost;
    intResult.waitResult = (WD_INTERRUPT_WAIT_RESULT)pDev->Int.fStopped;

    intResult.fIsMessageBased =
        (WDC_GET_ENABLED_INT_TYPE(pDev) == INTERRUPT_MESSAGE ||
        WDC_GET_ENABLED_INT_TYPE(pDev) == INTERRUPT_MESSAGE_X) ?
        TRUE : FALSE;
    intResult.dwLastMessage = WDC_GET_ENABLED_INT_LAST_MSG(pDev);

    intResult.fIsRead = pDevCtx->fIsRead;
    intResult.pBuf = pDevCtx->pBuf;
    intResult.u32Pattern = pDevCtx->u32Pattern;
    intResult.dwBufNumItems = pDevCtx->dwBufNumItems;

    /* Execute the diagnostics application's interrupt handler routine */
    pDevCtx->funcDiagIntHandler((WDC_DEVICE_HANDLE)pDev, &intResult);
}

/* Enable interrupts */
DWORD BMD_IntEnable(WDC_DEVICE_HANDLE hDev,
    BMD_INT_HANDLER funcIntHandler)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PBMD_DEV_CTX pDevCtx;

    TraceLog("BMD_IntEnable: Entered. Device handle [0x%p]\n", hDev);

    /* Validate the WDC device handle */
    if (!IsValidDevice(pDev, "BMD_IntEnable"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PBMD_DEV_CTX)WDC_GetDevContext(pDev);

    /* Check whether interrupts are already enabled */
    if (WDC_IntIsEnabled(hDev))
    {
        ErrLog("Interrupts are already enabled ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Message-Signaled Interrupts (MSI) / Extended Message-Signaled Interrupts
       (MSI-X) do not need to be acknowledged, so transfer commands are not
       required */
    #define NUM_TRANS_CMDS 0

    /* Store the diag interrupt handler routine, which will be executed by
       BMD_IntHandler() when an interrupt is received */
    pDevCtx->funcDiagIntHandler = funcIntHandler;

    /* Enable interrupts */
    dwStatus = WDC_IntEnable(hDev, NULL, NUM_TRANS_CMDS, 0, BMD_IntHandler,
        (PVOID)pDev, WDC_IS_KP(hDev));

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed enabling interrupts. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));

        return dwStatus;
    }

    /* TODO: You can add code here to write to the device in order to
             physically enable the hardware interrupts. */

    TraceLog("BMD_IntEnable: Interrupts enabled\n");

    return WD_STATUS_SUCCESS;
}

/* Disable interrupts */
DWORD BMD_IntDisable(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    TraceLog("BMD_IntDisable: Entered. Device handle [0x%p]\n", hDev);

    /* Validate the WDC device handle */
    if (!IsValidDevice(pDev, "BMD_IntDisable"))
        return WD_INVALID_PARAMETER;

    /* Check whether interrupts are already disabled */
    if (!WDC_IntIsEnabled(hDev))
    {
        ErrLog("Interrupts are already disabled ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* TODO: You can add code here to write to the device in order to
             physically disable the hardware interrupts. */

    /* Disable interrupts */
    dwStatus = WDC_IntDisable(hDev);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed disabling interrupts. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

/* Check whether interrupts are enabled for the given device */
BOOL BMD_IntIsEnabled(WDC_DEVICE_HANDLE hDev)
{
    /* Validate the WDC device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "BMD_IntIsEnabled"))
        return FALSE;

    /* Check whether interrupts are already enabled */
    return WDC_IntIsEnabled(hDev);
}
#endif /* ifdef HAS_INTS */
/* -----------------------------------------------
    Direct Memory Access (DMA)
   ----------------------------------------------- */
/* Validate a BMD DMA information structure handle */
static BOOL IsValidDmaHandle(BMD_DMA_HANDLE hDma, CHAR *sFunc)
{
    /* Validate the DMA handle and the WDC device handle */
    BOOL ret = (hDma && IsValidDevice((PWDC_DEVICE)hDma->hDev, sFunc)) ? TRUE : FALSE;

    if (!hDma)
        ErrLog("%s: NULL DMA Handle\n", sFunc);

    return ret;
}

/* Open a DMA handle: Allocate and initialize a BMD DMA information structure,
 * including allocation of a contiguous DMA buffer */
DWORD BMD_DmaOpen(WDC_DEVICE_HANDLE hDev, PVOID *ppBuf, DWORD dwOptions,
    DWORD dwBytes, BMD_DMA_HANDLE *ppDmaHandle)
{
    DWORD dwStatus;
    PBMD_DEV_CTX pDevCtx;
    BMD_DMA_HANDLE pBMDDma = NULL;

    /* Validate the WDC device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "BMD_DmaOpen"))
        return WD_INVALID_PARAMETER;

    /* Validate DMA buffer pointer */
    if (!ppBuf)
    {
        ErrLog("BMD_DmaOpen: Error - NULL DMA buffer pointer\n");
        return WD_INVALID_PARAMETER;
    }

    /* Check whether the device already has an open DMA handle */
    pDevCtx = (PBMD_DEV_CTX)WDC_GetDevContext(hDev);
    if (pDevCtx->hDma)
    {
        ErrLog("BMD_DmaOpen: Error - DMA already open\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Allocate a BMD DMA information structure */
    pBMDDma = (BMD_DMA_STRUCT *)calloc(1, sizeof(BMD_DMA_STRUCT));
    if (!pBMDDma)
    {
        ErrLog("BMD_DmaOpen: Failed allocating memory for a BMD DMA "
            "information structure\n");
        return WD_INSUFFICIENT_RESOURCES;
    }
    /* Store the device handle in the BMD DMA information structure */
    pBMDDma->hDev = hDev;

    /* Allocate and lock a contiguous DMA buffer */
    dwStatus = WDC_DMAContigBufLock(hDev, ppBuf, dwOptions, dwBytes,
        &pBMDDma->pDma);

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("BMD_DmaOpen: Failed locking a DMA buffer. "
            "Error 0x%x - %s\n", dwStatus, Stat2Str(dwStatus));
        goto Error;
    }

    *ppDmaHandle = (BMD_DMA_HANDLE)pBMDDma;

    /* Update the device context's DMA handle */
    pDevCtx->hDma = pBMDDma;
    pDevCtx->pBuf = *ppBuf;

    return WD_STATUS_SUCCESS;

Error:
    if (pBMDDma)
        BMD_DmaClose((BMD_DMA_HANDLE)pBMDDma);

    return dwStatus;
}

/* Prepare a device for a DMA transfer */
BOOL BMD_DmaDevicePrepare(BMD_DMA_HANDLE hDma, BOOL fIsRead, WORD wTLPNumItems,
    WORD dwNumItems, UINT32 u32Pattern, BOOL fEnable64bit, BYTE bTrafficClass)
{
    UINT32 u32TLPs, u32LowerAddr;
    BYTE bUpperAddr;
    WDC_DEVICE_HANDLE hDev;
    PBMD_DEV_CTX pDevCtx;

    /* Validate the DMA handle */
    if (!IsValidDmaHandle(hDma, "BMD_DmaDevicePrepare"))
        return FALSE;

    hDev = hDma->hDev;

    /* Assert Initiator Reset */
    BMD_WriteReg32(hDev, BMD_DSCR_OFFSET, 0x1);

    /* De-assert Initiator Reset */
    BMD_WriteReg32(hDev, BMD_DSCR_OFFSET, 0x0);

    /* Get the lower 32 bits of the DMA address */
    u32LowerAddr = (UINT32)hDma->pDma->Page[0].pPhysicalAddr;
    /* Get the upper 8 bits of the DMA address */
    bUpperAddr = (BYTE)((hDma->pDma->Page[0].pPhysicalAddr >> 32) & 0xFF);

    /* Set the DMA Transaction Layer Packets (TLPs) */
    u32TLPs = (wTLPNumItems & 0x1FFF) | /* u32TLPs[0:12] - TLP size */
        ((bTrafficClass & 0x7) << 16) | /* u32TLPs[16:18] - TLP traffic class */
        (fEnable64bit ? BIT19 : 0) |    /* u32TLPs[19] - enable 64-bit TLP */
        (bUpperAddr << 24);           /* u32TLPs[24:31] - the 33:39 upper bits
                                         * of the DMA address */

    if (fIsRead)
    {
        /* Set the lower 32 bits of the DMA address */
        BMD_WriteReg32(hDev, BMD_RDMATLPA_OFFSET, u32LowerAddr);

        /* Set the size, traffic class, 64-bit enable, and upper 8 bits of the
         * DMA address */
        BMD_WriteReg32(hDev, BMD_RDMATLPS_OFFSET, u32TLPs);

        /* Set TLP count */
        BMD_WriteReg16(hDev, BMD_RDMATLPC_OFFSET, dwNumItems);

        /* Set DMA read data pattern */
        BMD_WriteReg32(hDev, BMD_RDMATLPP_OFFSET, u32Pattern);
    }
    else
    {
        /* Set the lower 32 bits of the DMA address */
        BMD_WriteReg32(hDev, BMD_WDMATLPA_OFFSET, u32LowerAddr);

        /* Set the size, traffic class, 64-bit enable, and upper 8 bits of the
         * DMA address */
        BMD_WriteReg32(hDev, BMD_WDMATLPS_OFFSET, u32TLPs);

        /* Set TLP count */
        BMD_WriteReg16(hDev, BMD_WDMATLPC_OFFSET, dwNumItems);

        /* Set DMA read data pattern */
        BMD_WriteReg32(hDev, BMD_WDMATLPP_OFFSET, u32Pattern);
    }

    /* Initialize device context DMA fields: */
    pDevCtx = (PBMD_DEV_CTX)WDC_GetDevContext(hDev);
    /* Set the DMA direction (host-to-device=read; device-to-host=write) */
    pDevCtx->fIsRead = fIsRead;
    /* Set the DMA data pattern */
    pDevCtx->u32Pattern = u32Pattern;
    /* Set the total DMA transfer size, in units of UINT32 */
    pDevCtx->dwBufNumItems = (DWORD)dwNumItems * (DWORD)wTLPNumItems;

    return TRUE;
}

/* Close a DMA handle: Unlock and free a DMA buffer and the containing BMD DMA
 * information structure */
BOOL BMD_DmaClose(BMD_DMA_HANDLE hDma)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;
    PBMD_DEV_CTX pDevCtx;

    TraceLog("BMD_DmaClose: Entered\n");

    /* Validate the DMA handle */
    if (!IsValidDmaHandle(hDma, "BMD_DmaClose"))
        return FALSE;

    TraceLog("BMD_DmaClose: Device handle [0x%p]\n", hDma->hDev);

    if (hDma->pDma)
    {
        /* Unlock and free the DMA buffer */
        dwStatus = WDC_DMABufUnlock(hDma->pDma);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("BMD_DmaClose: Failed unlocking DMA buffer. "
                "Error 0x%x - %s\n", dwStatus, Stat2Str(dwStatus));
        }
    }
    else
    {
        TraceLog("BMD_DmaClose: DMA is not currently open ... "
            "(device handle 0x%p, DMA handle 0x%p)\n", hDma->hDev, hDma);
    }

    /* Clear the device context's DMA struct and DMA buffer handles */
    pDevCtx = (PBMD_DEV_CTX)WDC_GetDevContext(hDma->hDev);
    pDevCtx->hDma = NULL;
    pDevCtx->pBuf = NULL;

    /* Free the BMD DMA information structure */
    free(hDma);

    return TRUE;
}

static WORD code2size(BYTE bCode)
{
    if (bCode > 0x05)
        return 0;
    return (WORD)(128 << bCode);
}

/* Get maximum DMA packet size */
WORD BMD_DmaGetMaxPacketSize(WDC_DEVICE_HANDLE hDev, BOOL fIsRead)
{
    UINT32 u32DLTRSSTAT;
    WORD wBytes;

    /* Validate the WDC device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "BMD_DmaGetMaxPacketSize"))
        return 0;

    /* Read encoded maximum payload sizes */
    u32DLTRSSTAT = BMD_ReadReg32(hDev, BMD_DLTRSSTAT_OFFSET);

    /* Convert encoded maximum payload sizes into byte count */
    if (fIsRead)
    {
        /* Bits 18:16 */
        wBytes = code2size((BYTE)((u32DLTRSSTAT >> 16) & 0x7));
    }
    else
    {
        /* Bits 2:0 */
        WORD wMaxCapPayload = code2size((BYTE)(u32DLTRSSTAT & 0x7));
        /* Bits 10:8 */
        WORD wMaxProgPayload = code2size((BYTE)((u32DLTRSSTAT >> 8) & 0x7));

        wBytes = MIN(wMaxCapPayload, wMaxProgPayload);
    }

    return wBytes;
}

/* Synchronize all CPU caches with the DMA buffer */
DWORD BMD_DmaSyncCpu(BMD_DMA_HANDLE hDma)
{
    return WDC_DMASyncCpu(hDma->pDma);
}

/* Synchronize the I/O caches with the DMA buffer */
DWORD BMD_DmaSyncIo(BMD_DMA_HANDLE hDma)
{
    return WDC_DMASyncIo(hDma->pDma);
}

/* Start a DMA transfer */
BOOL BMD_DmaStart(BMD_DMA_HANDLE hDma, BOOL fIsRead)
{
    /* Validate the DMA handle */
    if (!IsValidDmaHandle(hDma, "BMD_DmaStart"))
        return FALSE;

    /* Synchronize CPU cache with the DMA buffer */
    BMD_DmaSyncCpu(hDma);

    /* Configure the device to start a DMA transfer */
    BMD_WriteReg32(hDma->hDev, BMD_DDMACR_OFFSET, fIsRead ? 0x10000 : 0x1);

    return TRUE;
}

/* Detect the completion of a DMA transfer */
BOOL BMD_DmaIsDone(WDC_DEVICE_HANDLE hDev, BOOL fIsRead)
{
    UINT32 ddmacr;

    /* Validate the WDC device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "BMD_DmaIsDone"))
        return FALSE;

    /* Detect DMA transfer completion */
    ddmacr = BMD_ReadReg32(hDev, BMD_DDMACR_OFFSET);
    return (fIsRead ? ddmacr & BIT24 : ddmacr & BIT8) ? TRUE : FALSE;
}

/* Poll for DMA transfer completion */
BOOL BMD_DmaPollCompletion(BMD_DMA_HANDLE hDma, BOOL fIsRead)
{
    DWORD i, dwTimeout = 10000000; /* 10 seconds */

    /* Validate the DMA handle */
    if (!IsValidDmaHandle(hDma, "BMD_DmaPollCompletion"))
        return FALSE;

    for (i = 0; i < dwTimeout; i += 2)
    {
        /* Check for DMA completion */
        if (BMD_DmaIsDone(hDma->hDev, fIsRead))
        {
            BMD_DmaSyncIo(hDma);
            return TRUE;
        }

        /* Wait */
        WDC_Sleep(2, 0);
    }

    return FALSE;
}
#ifdef HAS_INTS
/* Enable DMA interrupts */
BOOL BMD_DmaIntEnable(WDC_DEVICE_HANDLE hDev, BOOL fIsRead)
{
    UINT32 ddmacr = BMD_ReadReg32(hDev, BMD_DDMACR_OFFSET);

    /* Validate the WDC device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "BMD_DmaIntEnable"))
        return FALSE;

    ddmacr &= fIsRead ? ~BIT23 : ~BIT7;
    BMD_WriteReg32(hDev, BMD_DDMACR_OFFSET, ddmacr);

    return TRUE;
}

/* Disable DMA interrupts */
BOOL BMD_DmaIntDisable(WDC_DEVICE_HANDLE hDev, BOOL fIsRead)
{
    UINT32 ddmacr = BMD_ReadReg32(hDev, BMD_DDMACR_OFFSET);

    /* Validate the WDC device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "BMD_DmaIntDisable"))
        return FALSE;

    ddmacr |= fIsRead ? BIT23 : BIT7;
    BMD_WriteReg32(hDev, BMD_DDMACR_OFFSET, ddmacr);

    return TRUE;
}
#endif /* ifdef HAS_INTS */

/* Verify success of a host-to-device (read) DMA transfer */
BOOL BMD_DmaIsReadSucceed(WDC_DEVICE_HANDLE hDev)
{
    UINT32 ddmacr;

    /* Validate the WDC device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "BMD_DmaIsReadSucceed"))
        return FALSE;

    /* Check for a successful host-to-device (read)DMA  transfer indication */
    ddmacr = BMD_ReadReg32(hDev, BMD_DDMACR_OFFSET);
    return ddmacr & BIT31 ? FALSE : TRUE;
}

/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */
/* Plug-and-play or power management event handler routine */
static void BMD_EventHandler(WD_EVENT *pEvent, PVOID pData)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PBMD_DEV_CTX pDevCtx = (PBMD_DEV_CTX)WDC_GetDevContext(pDev);

    TraceLog("BMD_EventHandler: Entered. pData [0x%p], dwAction [0x%x]\n",
        pData, pEvent->dwAction);

    /* Execute the diagnostics application's event handler function */
    pDevCtx->funcDiagEventHandler((WDC_DEVICE_HANDLE)pDev, pEvent->dwAction);
}

/* Register a plug-and-play or power management event */
DWORD BMD_EventRegister(WDC_DEVICE_HANDLE hDev,
    BMD_EVENT_HANDLER funcEventHandler)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PBMD_DEV_CTX pDevCtx;
    DWORD dwActions = WD_ACTIONS_ALL;
    /* TODO: Modify the above to set up the plug-and-play/power management
             events for which you wish to receive notifications.
             dwActions can be set to any combination of the WD_EVENT_ACTION
             flags defined in windrvr.h. */

    TraceLog("BMD_EventRegister: Entered. Device handle [0x%p]\n", hDev);

    /* Validate the WDC device handle */
    if (!IsValidDevice(pDev, "BMD_EventRegister"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PBMD_DEV_CTX)WDC_GetDevContext(pDev);

    /* Check whether the event is already registered */
    if (WDC_EventIsRegistered(hDev))
    {
        ErrLog("BMD_EventRegister: Events are already registered ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Store the diag event handler routine to be executed from
     * BMD_EventHandler() upon an event */
    pDevCtx->funcDiagEventHandler = funcEventHandler;

    /* Register the event */
    dwStatus = WDC_EventRegister(hDev, dwActions, BMD_EventHandler, hDev,
        WDC_IS_KP(hDev));

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("BMD_EventRegister: Failed to register events. "
            "Error 0x%x - %s\n", dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    TraceLog("BMD_EventRegister: Events registered\n");

    return WD_STATUS_SUCCESS;
}

/* Unregister a plug-and-play or power management event */
DWORD BMD_EventUnregister(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;

    TraceLog("BMD_EventUnregister: Entered. Device handle [0x%p]\n", hDev);

    /* Validate the WDC device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "BMD_EventUnregister"))
        return WD_INVALID_PARAMETER;

    /* Check whether the event is currently registered */
    if (!WDC_EventIsRegistered(hDev))
    {
        ErrLog("BMD_EventUnregister: Cannot unregister events. No events "
            "currently registered ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Unregister the event */
    dwStatus = WDC_EventUnregister(hDev);

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("BMD_EventUnregister: Failed to unregister events. "
            "Error 0x%x - %s\n", dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

/* Check whether a given plug-and-play or power management event is registered
 */
BOOL BMD_EventIsRegistered(WDC_DEVICE_HANDLE hDev)
{
    /* Validate the WDC device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "BMD_EventIsRegistered"))
        return FALSE;

    /* Check whether the event is registered */
    return WDC_EventIsRegistered(hDev);
}
#endif

/* -----------------------------------------------
    Read/write registers
   ----------------------------------------------- */
/* Read from a 32-bit register */
UINT32 BMD_ReadReg32(WDC_DEVICE_HANDLE hDev, DWORD dwOffset)
{
    UINT32 u32Data;

    WDC_ReadAddr32(hDev, BMD_SPACE, dwOffset, &u32Data);
    return u32Data;
}

/* Read from a 16-bit register */
WORD BMD_ReadReg16(WDC_DEVICE_HANDLE hDev, DWORD dwOffset)
{
    WORD wData;

    WDC_ReadAddr16(hDev, BMD_SPACE, dwOffset, &wData);
    return wData;
}

/* Read from an 8-bit register */
BYTE BMD_ReadReg8(WDC_DEVICE_HANDLE hDev, DWORD dwOffset)
{
    BYTE bData;

    WDC_ReadAddr8(hDev, BMD_SPACE, dwOffset, &bData);
    return bData;
}

/* Write to a 32-bit register */
void BMD_WriteReg32(WDC_DEVICE_HANDLE hDev, DWORD dwOffset, UINT32 u32Data)
{
    WDC_WriteAddr32(hDev, BMD_SPACE, dwOffset, u32Data);
}

/* Write to a 16-bit register */
void BMD_WriteReg16(WDC_DEVICE_HANDLE hDev, DWORD dwOffset, WORD wData)
{
    WDC_WriteAddr16(hDev, BMD_SPACE, dwOffset, wData);
}

/* Write to an 8-bit register */
void BMD_WriteReg8(WDC_DEVICE_HANDLE hDev, DWORD dwOffset, BYTE bData)
{
    WDC_WriteAddr8(hDev, BMD_SPACE, dwOffset, bData);
}

/* -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- */
/* Log a debug error message */
static void ErrLog(const CHAR *sFormat, ...)
{
    va_list argp;

    va_start(argp, sFormat);
    vsnprintf(gsBMD_LastErr, sizeof(gsBMD_LastErr) - 1, sFormat, argp);
#if defined(DEBUG)
    #if defined(__KERNEL__)
        WDC_Err("KP BMD lib: %s", gsBMD_LastErr);
     #else
        WDC_Err("BMD lib: %s", gsBMD_LastErr);
    #endif
#endif
    va_end(argp);
}

/* Log a debug trace message */
static void TraceLog(const CHAR *sFormat, ...)
{
#if defined(DEBUG)
    CHAR sMsg[256];
    va_list argp;

    va_start(argp, sFormat);
    vsnprintf(sMsg, sizeof(sMsg) - 1, sFormat, argp);
    #if defined(__KERNEL__)
        WDC_Trace("KP BMD lib: %s", sMsg);
    #else
        WDC_Trace("BMD lib: %s", sMsg);
    #endif
    va_end(argp);
#else
    UNUSED_VAR(sFormat);
#endif
}

/* Get last error */
const char *BMD_GetLastErr(void)
{
    return gsBMD_LastErr;
}


