/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/****************************************************************************
*  File: AVALONMM_lib.c
*
*  Implementation of a sample library for accessing Intel Altera PCI Express
*  cards with AVALONMM design, using the WinDriver WDC API.
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*****************************************************************************/

#include "wdc_defs.h"
#include "utils.h"
#include "avalonmm_lib.h"

/*************************************************************
  Internal definitions
 *************************************************************/
 /* WinDriver license registration string */
 /* TODO: When using a registered WinDriver version, replace the license string
          below with the development license in order to use on the development
          machine.
          Once you require to distribute the driver's package to other machines,
          please replace the string with a distribution license */
#define AVALONMM_DEFAULT_LICENSE_STRING "12345abcde12345.abcde"

#define AVALONMM_DEFAULT_DRIVER_NAME WD_DEFAULT_DRIVER_NAME_BASE

/* PCI device information struct */
typedef struct {
#ifdef HAS_INTS
    AVALONMM_INT_HANDLER   funcDiagIntHandler;   /* Interrupt handler routine */
#endif /* ifdef HAS_INTS */
    AVALONMM_EVENT_HANDLER funcDiagEventHandler; /* Event handler routine */
} AVALONMM_DEV_CTX, *PAVALONMM_DEV_CTX;
/* TODO: You can add fields to store additional device-specific information. */

/*************************************************************
  Global variables definitions
 *************************************************************/
 /* Last error information string */
static CHAR gsAVALONMM_LastErr[256];

/* Library initialization reference count */
static DWORD LibInit_count = 0;

/*************************************************************
  Static functions prototypes and inline implementation
 *************************************************************/

#ifndef __KERNEL__
static BOOL DeviceValidate(const PWDC_DEVICE pDev);
static void AVALONMM_EventHandler(WD_EVENT *pEvent, PVOID pData);
#endif
static void DLLCALLCONV AVALONMM_IntHandler(PVOID pData);
static void ErrLog(const CHAR *sFormat, ...);
static void TraceLog(const CHAR *sFormat, ...);

/* Validate a device handle */
static inline BOOL IsValidDevice(PWDC_DEVICE pDev, const CHAR *sFunc)
{
    if (!pDev || !(PAVALONMM_DEV_CTX)(pDev->pCtx))
    {
        snprintf(gsAVALONMM_LastErr, sizeof(gsAVALONMM_LastErr) - 1,
            "%s: NULL device %s\n", sFunc, !pDev ? "handle" : "context");
        ErrLog(gsAVALONMM_LastErr);
        return FALSE;
    }

    return TRUE;
}

/*************************************************************
  Functions implementation
 *************************************************************/
 /* -----------------------------------------------
     AVALONMM and WDC libraries initialize/uninitialize
    ----------------------------------------------- */
    /* Initialize the AVALONMM and WDC libraries */
DWORD AVALONMM_LibInit(void)
{
    DWORD dwStatus;

    /* Increase the library's reference count; initialize the library only once
     */
    if (++LibInit_count > 1)
        return WD_STATUS_SUCCESS;

#ifdef WD_DRIVER_NAME_CHANGE
    /* Set the driver name */
    if (!WD_DriverName(AVALONMM_DEFAULT_DRIVER_NAME))
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
        AVALONMM_DEFAULT_LICENSE_STRING);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to initialize the WDC library. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    return WD_STATUS_SUCCESS;
}

/* Uninitialize the AVALONMM and WDC libraries */
DWORD AVALONMM_LibUninit(void)
{
    DWORD dwStatus;

    /* Decrease the library's reference count; uninitialize the library only
     * when there are no more open handles to the library */
    if (--LibInit_count > 0)
        return WD_STATUS_SUCCESS;

    /* Uninitialize the WDC library and close the handle to WinDriver */
    dwStatus = WDC_DriverClose();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to uninit the WDC library. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

/* -----------------------------------------------
    Device open/close
   ----------------------------------------------- */
   /* Open a device handle */
WDC_DEVICE_HANDLE AVALONMM_DeviceOpen(DWORD dwVendorId, DWORD dwDeviceId)
{
    WDC_DEVICE_HANDLE hDev;

    hDev = WDC_DIAG_DeviceFindAndOpen(dwVendorId, dwDeviceId, NULL,
        sizeof(AVALONMM_DEV_CTX));

    if (!hDev || !DeviceValidate((PWDC_DEVICE)hDev))
        goto Error;

    return hDev;

Error:
    if (hDev)
        AVALONMM_DeviceClose(hDev);

    ErrLog("AVALONMM_DeviceOpen: Failed opening PCI device\n");

    return NULL;
}

/* Close device handle */
BOOL AVALONMM_DeviceClose(WDC_DEVICE_HANDLE hDev)
{
    TraceLog("AVALONMM_DeviceClose: Entered. Device handle 0x%p\n", hDev);

#ifdef HAS_INTS
    /* Disable interrupts (if enabled) */
    if (WDC_IntIsEnabled(hDev))
    {
        DWORD dwStatus = AVALONMM_IntDisable(hDev);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("Failed disabling interrupts. Error 0x%x - %s\n", dwStatus,
                Stat2Str(dwStatus));
        }
    }
#endif /* ifdef HAS_INTS */
    return WDC_DIAG_DeviceClose(hDev);
}

/* Validate device information */
static BOOL DeviceValidate(const PWDC_DEVICE pDev)
{
    DWORD i, dwNumAddrSpaces = pDev->dwNumAddrSpaces;

    /* NOTE: You can modify the implementation of this function in order to
             verify that the device has the resources you expect to find. */

             /* Verify that the device has at least one active address space */
    for (i = 0; i < dwNumAddrSpaces; i++)
    {
        if (WDC_AddrSpaceIsActive(pDev, i))
            return TRUE;
    }

    /* In this sample we accept the device even if it doesn't have any
     * address spaces */
    TraceLog("Device does not have any active memory or I/O address spaces\n");
    return TRUE;
}
#ifdef HAS_INTS
/* -----------------------------------------------
    Interrupts
   ----------------------------------------------- */
   /* Interrupt handler routine */
static void DLLCALLCONV AVALONMM_IntHandler(PVOID pData)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PAVALONMM_DEV_CTX pDevCtx = (PAVALONMM_DEV_CTX)(pDev->pCtx);
    AVALONMM_INT_RESULT intResult;

    BZERO(intResult);
    intResult.dwCounter = pDev->Int.dwCounter;
    intResult.dwLost = pDev->Int.dwLost;
    intResult.waitResult = (WD_INTERRUPT_WAIT_RESULT)pDev->Int.fStopped;
    intResult.dwEnabledIntType = WDC_GET_ENABLED_INT_TYPE(pDev);
    intResult.dwLastMessage = WDC_GET_ENABLED_INT_LAST_MSG(pDev);

    /* Execute the diagnostics application's interrupt handler routine */
    pDevCtx->funcDiagIntHandler((WDC_DEVICE_HANDLE)pDev, &intResult);
}
#endif /* ifdef HAS_INTS */

/* Check whether a given device contains an item of the specified type */
static BOOL IsItemExists(PWDC_DEVICE pDev, ITEM_TYPE item)
{
    DWORD i, dwNumItems = pDev->cardReg.Card.dwItems;

    for (i = 0; i < dwNumItems; i++)
    {
        if ((ITEM_TYPE)(pDev->cardReg.Card.Item[i].item) == item)
            return TRUE;
    }

    return FALSE;
}

static void verifyDmaOperation(MENU_CTX_DMA *pDmaMenusCtx)
{
    BOOL fIsIdentical = TRUE;
    UINT32 u32RootPortData; /* PCIe host */
    UINT32 u32EndPointData; /* on-chip memory */
    DWORD i, dwBytesCurrentTransfer = 0;
    DWORD dwDwordsTransferred =
        pDmaMenusCtx->pDmaBuffer->dwBytesTransferred / sizeof(UINT32);

    for (i = 0; i < pDmaMenusCtx->pDmaBuffer->dwPages; i++)
    {
        dwBytesCurrentTransfer += pDmaMenusCtx->pDmaBuffer->Page[i].dwBytes;
    }

    for (i = 0; i < dwBytesCurrentTransfer / sizeof(UINT32); i++)
    {
        UINT32 u32Offset = dwDwordsTransferred + i;

        u32RootPortData = *((UINT32*)pDmaMenusCtx->pBuf + u32Offset);
        WDC_ReadAddr32(*pDmaMenusCtx->phDev, AVALONMM_RXM_BAR4,
            u32Offset * sizeof(UINT32), &u32EndPointData);

        if (u32RootPortData != u32EndPointData)
        {
            printf("MISMATCH at offset %d. root port data [0x%x] != end point"
                " data [0x%x]\n", i, u32RootPortData, u32EndPointData);
            fIsIdentical = FALSE;
        }
    }

    printf("The verification %s\n",
        fIsIdentical ? "finished successfully" : "failed");

}

static void setReadWriteDescriptor(AVALONMM_DESCRIPTOR *desc, DMA_ADDR source,
    DMA_ADDR dest, DWORD dwBytes, UINT32 id)
{
    desc->u32SrcAddrLow = DMA_ADDR_LOW(source);
    desc->u32SrcAddrHigh = DMA_ADDR_HIGH(source);
    desc->u32DestAddrLow = DMA_ADDR_LOW(dest);
    desc->u32DestAddrHigh = DMA_ADDR_HIGH(dest);

    desc->controlAndLength.bits.u32Id = id;
    desc->controlAndLength.bits.u32Size = dwBytes / sizeof(UINT32);

    desc->u32Reserved1 = 0;
    desc->u32Reserved2 = 0;
    desc->u32Reserved3 = 0;
}

static DWORD setDescriptors(MENU_CTX_DMA *pDmaMenusCtx)
{
    WD_DMA *pDmaBuffer = pDmaMenusCtx->pDmaBuffer;
    DWORD i;
    DWORD dwPages = pDmaBuffer->dwPages;
    DWORD dwStatus = WD_STATUS_SUCCESS;
    UINT64 u64Offset = pDmaMenusCtx->u64FPGAOffset +
        pDmaBuffer->dwBytesTransferred;
    AVALONMM_DESCRIPTOR *desc;
    AVALONMM_DESCRIPTOR_TABLE *pDescriptorTable =
        (AVALONMM_DESCRIPTOR_TABLE *)pDmaMenusCtx->pDescriptorTable;

    memset(pDescriptorTable, 0, sizeof(AVALONMM_DESCRIPTOR_TABLE));

    if (dwPages >= AVALONMM_MAX_DESCRIPTORS)
    {
        ErrLog("%s: Got more pages [%d] than allowed descriptors [%d].\n",
            __FUNCTION__, dwPages, AVALONMM_MAX_DESCRIPTORS);
        dwStatus = WD_OPERATION_FAILED;
        goto Exit;
    }

    for (i = 0; i < dwPages; i++)
    {
        desc = &pDescriptorTable->descriptors[i];

        if (pDmaMenusCtx->fToDevice)
        {
            setReadWriteDescriptor(desc, pDmaBuffer->Page[i].pPhysicalAddr,
                u64Offset, pDmaBuffer->Page[i].dwBytes, i);
        }
        else
        {
            setReadWriteDescriptor(desc, u64Offset,
                pDmaBuffer->Page[i].pPhysicalAddr, pDmaBuffer->Page[i].dwBytes,
                i);
        }

        u64Offset += pDmaBuffer->Page[i].dwBytes;
    }

Exit:
    return dwStatus;
}

static void initEndpointMemory(MENU_CTX_DMA *pDmaMenusCtx)
{
    UINT32 *pu32Buf;
    DWORD i;

    if (!pDmaMenusCtx->fToDevice && pDmaMenusCtx->fRunVerificationCheck)
    {
        pu32Buf = (UINT32 *)pDmaMenusCtx->pBuf;
        for (i = 0; i < pDmaMenusCtx->dwBytes / sizeof(UINT32); i++)
        {
            WDC_ReadAddr32(*pDmaMenusCtx->phDev, AVALONMM_RXM_BAR4,
                i * sizeof(UINT32), &pu32Buf[i]);
        }
    }
}

static void DLLCALLCONV DmaTransferBuild(PVOID pData)
{
    MENU_CTX_DMA *pDmaMenusCtx = (MENU_CTX_DMA *)pData;
    DMA_ADDR descriptorsPhysicalAddr =
        pDmaMenusCtx->pDmaDescriptors->Page[0].pPhysicalAddr;
    WD_DMA *pDmaBuffer = pDmaMenusCtx->pDmaBuffer;
    DWORD dwStatus = WD_STATUS_SUCCESS;
    DWORD dwPages = pDmaBuffer->dwPages;
    UINT32 u32DescriptorTableAddrLow, u32DescriptorTableAddrHigh,
        u32DescriptorFifoAddrLow, u32DescriptorFifoAddrHigh,
        u32TableSize;
    UINT32 u32ControlRegistersOffset =
        pDmaMenusCtx->fToDevice ? AVALONMM_CONTROL_REGISTERS_TO_DEVICE_OFFSET :
        AVALONMM_CONTROL_REGISTERS_FROM_DEVICE_OFFSET;

    TraceLog("%s Entered. dwPages %d\n", __FUNCTION__, dwPages);

    dwStatus = setDescriptors(pDmaMenusCtx);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("Failed to set descriptors. Error 0x%x - %s\n", dwStatus,
            Stat2Str(dwStatus));
        return;
    }

    u32DescriptorTableAddrLow = DMA_ADDR_LOW(descriptorsPhysicalAddr);
    u32DescriptorTableAddrHigh = DMA_ADDR_HIGH(descriptorsPhysicalAddr);

    if (pDmaMenusCtx->fToDevice)
    {
        u32DescriptorFifoAddrLow = AVALONMM_ENDPOINT_READ_BASE_LOW;
        u32DescriptorFifoAddrHigh = AVALONMM_ENDPOINT_READ_BASE_HIGH;
    }
    else
    {
        u32DescriptorFifoAddrLow = AVALONMM_ENDPOINT_WRITE_BASE_LOW;
        u32DescriptorFifoAddrHigh = AVALONMM_ENDPOINT_WRITE_BASE_HIGH;
    }

    u32TableSize = dwPages - 1;

    /* Write to Root Complex Read/Write Status and Descriptor Base registers */
    WDC_WriteAddr32(*pDmaMenusCtx->phDev, AVALONMM_RXM_BAR0,
        AVALONMM_OFFSET_DESCRIPTORS_LOW + u32ControlRegistersOffset,
        u32DescriptorTableAddrLow);
    WDC_WriteAddr32(*pDmaMenusCtx->phDev, AVALONMM_RXM_BAR0,
        AVALONMM_OFFSET_DESCRIPTORS_HIGH +u32ControlRegistersOffset,
        u32DescriptorTableAddrHigh);

    /* Write to endpoint Read/Write Descriptor FIFO Base registers */
    WDC_WriteAddr32(*pDmaMenusCtx->phDev, AVALONMM_RXM_BAR0,
        AVALONMM_OFFSET_FIFO_LOW + u32ControlRegistersOffset,
        u32DescriptorFifoAddrLow);
    WDC_WriteAddr32(*pDmaMenusCtx->phDev, AVALONMM_RXM_BAR0,
        AVALONMM_OFFSET_FIFO_HIGH + u32ControlRegistersOffset,
        u32DescriptorFifoAddrHigh);

    /* Write to WR_TABLE_SIZE/RD_TABLE_SIZE register */
    WDC_WriteAddr32(*pDmaMenusCtx->phDev, AVALONMM_RXM_BAR0,
        AVALONMM_OFFSET_TABLE_SIZE + u32ControlRegistersOffset,
        u32TableSize);

    /* Write to WR_CONTROL/RD_CONTROL register */

    /* When the 0th bit of this register is set, the Descriptor Controller sets
     * the Done bit for each descriptor in the Done status table.*/

    /* When the 0th bit of this register is not set, the Descriptor Controller
     * sets the Done bit only for the final descriptor, as specified by
     * RD_DMA_LAST_PTR. */

    /* In both cases, the Descriptor Controller sends an MSI to the host after
     * the completion of the last descriptor along with the status update for
     * the last descriptor. */

    //WDC_WriteAddr32(*pDmaMenusCtx->phDev, AVALONMM_RXM_BAR0,
    //    AVALONMM_OFFSET_CONTROL + u32ControlRegistersOffset, 1);

    WDC_DMASyncCpu(pDmaMenusCtx->pDmaDescriptors);
}

static DWORD DmaBuildDescBuffer(MENU_CTX_DMA *pDmaMenusCtx)
{
    DWORD dwStatus;

    dwStatus = WDC_DMAContigBufLock(*pDmaMenusCtx->phDev,
        (PVOID *)&pDmaMenusCtx->pDescriptorTable, pDmaMenusCtx->dwOptions,
        sizeof(AVALONMM_DESCRIPTOR_TABLE), &pDmaMenusCtx->pDmaDescriptors);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("Failed locking DMA descriptors buffer. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

DWORD AVALONMM_DmaPollCompletion(MENU_CTX_DMA *pDmaMenusCtx)
{
    AVALONMM_DESCRIPTOR_TABLE *pDescriptorTable =
        pDmaMenusCtx->pDescriptorTable;
    DWORD dwPages = pDmaMenusCtx->pDmaBuffer->dwPages;
    volatile AVALONMM_DESCRIPTOR_STATUS *pLastDescriptorStatus =
        &pDescriptorTable->descriptorsStatus[dwPages - 1];

    /* For more info about polling: read the WR_CONTROL/RD_CONTROL register
     * comment in DmaTransferBuild() */
    while (!pLastDescriptorStatus->bits.u32Done);

    return WD_STATUS_SUCCESS;
}

DWORD AVALONMM_DmaInit(MENU_CTX_DMA *pDmaMenusCtx)
{
    DWORD dwStatus;

    dwStatus = DmaBuildDescBuffer(pDmaMenusCtx);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        goto Exit;
    }

    initEndpointMemory(pDmaMenusCtx);

    dwStatus = WDC_DMATransactionSGInit(*pDmaMenusCtx->phDev,
        pDmaMenusCtx->pBuf, pDmaMenusCtx->dwOptions, pDmaMenusCtx->dwBytes,
        &pDmaMenusCtx->pDmaBuffer, NULL,
        AVALONMM_TRANSACTION_MAX_TRANSFER_SIZE, sizeof(AVALONMM_DESCRIPTOR));
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        printf("WDC_DMATransactionSGInit() Failed. Error 0x%x\n", dwStatus);
        goto Exit;
    }

Exit:
    return dwStatus;
}

DWORD AVALONMM_DmaTransferStart(MENU_CTX_DMA *pDmaMenusCtx)
{
    UINT32 controlRegistersOffset =
        pDmaMenusCtx->fToDevice ? AVALONMM_CONTROL_REGISTERS_TO_DEVICE_OFFSET :
        AVALONMM_CONTROL_REGISTERS_FROM_DEVICE_OFFSET;

    /* start the DMA operation */
    WDC_WriteAddr32(*pDmaMenusCtx->phDev, AVALONMM_RXM_BAR0,
        controlRegistersOffset + AVALONMM_OFFSET_LAST_PTR,
        pDmaMenusCtx->pDmaBuffer->dwPages - 1);

    return WD_STATUS_SUCCESS;
}

DWORD AVALONMM_DmaTransactionExecute(MENU_CTX_DMA *pDmaMenusCtx)
{
    DWORD dwStatus = WD_STATUS_SUCCESS;

    TraceLog("\n%s: Executing DMA transaction.\n", __FUNCTION__);

    dwStatus = WDC_DMATransactionExecute(pDmaMenusCtx->pDmaBuffer,
        DmaTransferBuild, pDmaMenusCtx);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Failed to execute DMA transaction. Error 0x%x - %s\n",
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
        goto Exit;
    }

    do {
        AVALONMM_DmaTransferStart(pDmaMenusCtx);
        AVALONMM_DmaPollCompletion(pDmaMenusCtx);
		TraceLog("DMA transfer has been finished\n");

        if (pDmaMenusCtx->fRunVerificationCheck)
            verifyDmaOperation(pDmaMenusCtx);

        dwStatus = AVALONMM_DmaTransactionTransferEnded(pDmaMenusCtx);
    } while (dwStatus == (DWORD)WD_MORE_PROCESSING_REQUIRED);

Exit:
    return dwStatus;
}

DWORD AVALONMM_DmaTransactionTransferEnded(MENU_CTX_DMA *pDmaMenusCtx)
{
    DWORD dwStatus = WDC_DMATransferCompletedAndCheck(pDmaMenusCtx->pDmaBuffer,
        TRUE);
    if (dwStatus == WD_STATUS_SUCCESS)
        TraceLog("DMA transaction completed\n");
    else if (dwStatus != (DWORD)WD_MORE_PROCESSING_REQUIRED)
        ErrLog("DMA transfer failed\n");

    return dwStatus;
}

DWORD AVALONMM_DmaTransactionRelease(MENU_CTX_DMA *pDmaMenusCtx)
{
    DWORD dwStatus;

    dwStatus = WDC_DMATransactionRelease(pDmaMenusCtx->pDmaBuffer);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("%s: Failed to release DMA transaction. Error 0x%x - %s\n",\
            __FUNCTION__, dwStatus, Stat2Str(dwStatus));
    }
    else
    {
        TraceLog("%s: DMA transaction was released. \n", __FUNCTION__);
    }

    return dwStatus;
}

void AVALONMM_FreeDmaMem(MENU_CTX_DMA *pDmaMenusCtx)
{
    DWORD dwStatus;

    if (!pDmaMenusCtx)
        return;

    if (pDmaMenusCtx->pDmaDescriptors)
    {
        dwStatus = WDC_DMABufUnlock(pDmaMenusCtx->pDmaDescriptors);
        if (WD_STATUS_SUCCESS == dwStatus)
        {
            TraceLog("DMA memory freed\n");
        }
        else
        {
            ErrLog("Failed trying to free DMA memory. Error [0x%x - %s]\n",
                dwStatus, Stat2Str(dwStatus));
        }
    }

    if (pDmaMenusCtx->pDmaBuffer)
    {
        dwStatus = WDC_DMATransactionUninit(pDmaMenusCtx->pDmaBuffer);
        if (WD_STATUS_SUCCESS == dwStatus)
        {
            TraceLog("DMA transaction uninitialized\n");
        }
        else
        {
            ErrLog("Failed trying to uninitialize DMA transaction. Error "
                "[0x%x - %s]\n", dwStatus, Stat2Str(dwStatus));
        }
    }

    pDmaMenusCtx->pDmaBuffer = NULL;
    pDmaMenusCtx->pDmaDescriptors = NULL;
}

#ifdef HAS_INTS
/* Enable interrupts */
DWORD AVALONMM_IntEnable(WDC_DEVICE_HANDLE hDev,
    AVALONMM_INT_HANDLER funcIntHandler)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PAVALONMM_DEV_CTX pDevCtx;

    TraceLog("AVALONMM_IntEnable: Entered. Device handle 0x%p\n", hDev);

    /* Validate the device handle */
    if (!IsValidDevice(pDev, "AVALONMM_IntEnable"))
        return WD_INVALID_PARAMETER;

    /* Verify that the device has an interrupt item */
    if (!IsItemExists(pDev, ITEM_INTERRUPT))
        return WD_OPERATION_FAILED;

    pDevCtx = (PAVALONMM_DEV_CTX)(pDev->pCtx);

    /* Check whether interrupts are already enabled */
    if (WDC_IntIsEnabled(hDev))
    {
        ErrLog("Interrupts are already enabled ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Store the diag interrupt handler routine, which will be executed by
     * AVALONMM_IntHandler() when an interrupt is received */
    pDevCtx->funcDiagIntHandler = funcIntHandler;

    /* Enable the interrupts */
    /* NOTE: When adding read transfer commands, set the INTERRUPT_CMD_COPY flag
             in the 4th argument (dwOptions) passed to WDC_IntEnable() */
    dwStatus = WDC_IntEnable(hDev, NULL, 0, INTERRUPT_MESSAGE,
        AVALONMM_IntHandler, (PVOID)pDev, WDC_IS_KP(hDev));

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed enabling interrupts. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));

        return dwStatus;
    }

	TraceLog("AVALONMM_IntEnable: Interrupts enabled\n");

    return WD_STATUS_SUCCESS;
}

/* Disable interrupts */
DWORD AVALONMM_IntDisable(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    TraceLog("AVALONMM_IntDisable entered. Device handle 0x%p\n", hDev);

    /* Validate the device handle */
    if (!IsValidDevice(pDev, "AVALONMM_IntDisable"))
        return WD_INVALID_PARAMETER;

    /* Check whether interrupts are already enabled */
    if (!WDC_IntIsEnabled(hDev))
    {
        ErrLog("Interrupts are already disabled ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

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
BOOL AVALONMM_IntIsEnabled(WDC_DEVICE_HANDLE hDev)
{
    /* Validate the device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "AVALONMM_IntIsEnabled"))
        return FALSE;

    /* Check whether interrupts are already enabled */
    return WDC_IntIsEnabled(hDev);
}
#endif /* ifdef HAS_INTS */
/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */
/* Plug-and-play or power management event handler routine */
static void AVALONMM_EventHandler(WD_EVENT *pEvent, PVOID pData)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PAVALONMM_DEV_CTX pDevCtx = (PAVALONMM_DEV_CTX)(pDev->pCtx);

    TraceLog("AVALONMM_EventHandler entered, pData 0x%p, dwAction 0x%x\n",
        pData, pEvent->dwAction);

    /* Execute the diagnostics application's event handler function */
    pDevCtx->funcDiagEventHandler((WDC_DEVICE_HANDLE)pDev, pEvent->dwAction);
}

/* Register a plug-and-play or power management event */
DWORD AVALONMM_EventRegister(WDC_DEVICE_HANDLE hDev,
    AVALONMM_EVENT_HANDLER funcEventHandler)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PAVALONMM_DEV_CTX pDevCtx;
    DWORD dwActions = WD_ACTIONS_ALL;
    /* TODO: Modify the above to set up the plug-and-play/power management
             events for which you wish to receive notifications.
             dwActions can be set to any combination of the WD_EVENT_ACTION
             flags defined in windrvr.h. */

    TraceLog("AVALONMM_EventRegister entered. Device handle 0x%p\n", hDev);

    /* Validate the device handle */
    if (!IsValidDevice(pDev, "AVALONMM_EventRegister"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PAVALONMM_DEV_CTX)(pDev->pCtx);

    /* Check whether the event is already registered */
    if (WDC_EventIsRegistered(hDev))
    {
        ErrLog("Events are already registered ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Store the diag event handler routine to be executed from
     * AVALONMM_EventHandler() upon an event */
    pDevCtx->funcDiagEventHandler = funcEventHandler;

    /* Register the event */
    dwStatus = WDC_EventRegister(hDev, dwActions, AVALONMM_EventHandler, hDev,
        WDC_IS_KP(hDev));

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to register events. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    TraceLog("Events registered\n");
    return WD_STATUS_SUCCESS;
}

/* Unregister a plug-and-play or power management event */
DWORD AVALONMM_EventUnregister(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;

    TraceLog("AVALONMM_EventUnregister entered. Device handle 0x%p\n", hDev);

    /* Validate the device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "AVALONMM_EventUnregister"))
        return WD_INVALID_PARAMETER;

    /* Check whether the event is currently registered */
    if (!WDC_EventIsRegistered(hDev))
    {
        ErrLog("Cannot unregister events - no events currently "
            "registered ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Unregister the event */
    dwStatus = WDC_EventUnregister(hDev);

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to unregister events. Error 0x%x - %s\n", dwStatus,
            Stat2Str(dwStatus));
    }

    return dwStatus;
}

/* Check whether a given plug-and-play or power management event is registered
 */
BOOL AVALONMM_EventIsRegistered(WDC_DEVICE_HANDLE hDev)
{
    /* Validate the device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "AVALONMM_EventIsRegistered"))
        return FALSE;

    /* Check whether the event is registered */
    return WDC_EventIsRegistered(hDev);
}

/* -----------------------------------------------
    Address spaces information
   ----------------------------------------------- */
   /* Get number of address spaces */
DWORD AVALONMM_GetNumAddrSpaces(WDC_DEVICE_HANDLE hDev)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    /* Validate the device handle */
    if (!IsValidDevice(pDev, "AVALONMM_GetNumAddrSpaces"))
        return 0;

    /* Return the number of address spaces for the device */
    return pDev->dwNumAddrSpaces;
}

/* Get address space information */
BOOL AVALONMM_GetAddrSpaceInfo(WDC_DEVICE_HANDLE hDev,
    AVALONMM_ADDR_SPACE_INFO *pAddrSpaceInfo)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    WDC_ADDR_DESC *pAddrDesc;
    DWORD dwAddrSpace;
    BOOL fIsMemory;

    if (!IsValidDevice(pDev, "AVALONMM_GetAddrSpaceInfo"))
        return FALSE;

#if defined(DEBUG)
    if (!pAddrSpaceInfo)
    {
        ErrLog("AVALONMM_GetAddrSpaceInfo: Error - NULL address space "
            "information pointer\n");
        return FALSE;
    }
#endif

    dwAddrSpace = pAddrSpaceInfo->dwAddrSpace;

    if (dwAddrSpace > pDev->dwNumAddrSpaces - 1)
    {
        ErrLog("AVALONMM_GetAddrSpaceInfo: Error - Address space %d is "
            "out of range (0 - %d)\n", dwAddrSpace, pDev->dwNumAddrSpaces - 1);
        return FALSE;
    }

    pAddrDesc = &pDev->pAddrDesc[dwAddrSpace];

    fIsMemory = WDC_ADDR_IS_MEM(pAddrDesc);

    snprintf(pAddrSpaceInfo->sName, MAX_NAME - 1, "BAR %d", dwAddrSpace);
    snprintf(pAddrSpaceInfo->sType, MAX_TYPE - 1, fIsMemory ? "Memory" : "I/O");

    if (WDC_AddrSpaceIsActive(pDev, dwAddrSpace))
    {
        WD_ITEMS *pItem = &pDev->cardReg.Card.Item[pAddrDesc->dwItemIndex];
        PHYS_ADDR pAddr = fIsMemory ? pItem->I.Mem.pPhysicalAddr :
            pItem->I.IO.pAddr;

        snprintf(pAddrSpaceInfo->sDesc, MAX_DESC - 1,
            "0x%0*" PRI64 "X - 0x%0*" PRI64 "X (0x%" PRI64 "x bytes)",
            (int)WDC_SIZE_64 * 2, pAddr,
            (int)WDC_SIZE_64 * 2, pAddr + pAddrDesc->qwBytes - 1,
            pAddrDesc->qwBytes);
    }
    else
    {
        snprintf(pAddrSpaceInfo->sDesc, MAX_DESC - 1, "Inactive address space");
    }

    /* TODO: You can modify the code above to set a different address space
     * name/description. */

    return TRUE;
}


/* -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- */
/* Log a debug error message */
static void ErrLog(const CHAR *sFormat, ...)
{
    va_list argp;

    va_start(argp, sFormat);
    vsnprintf(gsAVALONMM_LastErr, sizeof(gsAVALONMM_LastErr) - 1, sFormat,
        argp);
#ifdef DEBUG
    WDC_Err("AVALONMM lib: %s", gsAVALONMM_LastErr);
#endif
    va_end(argp);
}

/* Log a debug trace message */
static void TraceLog(const CHAR *sFormat, ...)
{
#ifdef DEBUG
    CHAR sMsg[256];
    va_list argp;

    va_start(argp, sFormat);
    vsnprintf(sMsg, sizeof(sMsg) - 1, sFormat, argp);
    WDC_Trace("AVALONMM lib: %s", sMsg);
    va_end(argp);
#else
    UNUSED_VAR(sFormat);
#endif
}

/* Get last error */
const char *AVALONMM_GetLastErr(void)
{
    return gsAVALONMM_LastErr;
}


