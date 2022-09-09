/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/****************************************************************************
*  File: lscdma_lib.c
*
*  Implementation of a sample library for accessing Lattice PCI Express cards
*  with SGDMA support, using the WinDriver WDC API.
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*****************************************************************************/

#if defined(__KERNEL__)
    #include "kpstdlib.h"
#endif
#include "utils.h"
#include "lscdma_lib.h"

/*************************************************************
  Internal definitions
 *************************************************************/
/* WinDriver license registration string */
/* TODO: When using a registered WinDriver version, replace the license string
         below with the development license in order to use on the development
         machine.
         Once you require to distribute the driver's package to other machines,
         please replace the string with a distribution license */
#define LSCDMA_DEFAULT_LICENSE_STRING "12345abcde12345.abcde"

#define LSCDMA_DEFAULT_DRIVER_NAME WD_DEFAULT_DRIVER_NAME_BASE

typedef struct {
    WDC_DEVICE_HANDLE  hDev;        /* Device handle */
    DWORD              dwBytes;     /* DMA buffer size */
    DWORD              dwChannel;   /* SGDMA channel */
    BOOL               fToDevice;   /* DMA transfer direction */
    WD_DMA             *pDma;       /* DMA buffer */
    PVOID              pBuf;        /* User mode pointer to the DMA buffer */
    DWORD              dwBurstSize; /* Burst size */
    DWORD              dwStartBD;   /* First Buffer Descriptor */
} LSCDMA_DMA_STRUCT;

/* LSCDMA device information struct */
typedef struct {
    LSCDMA_INT_HANDLER   funcDiagIntHandler;
    LSCDMA_EVENT_HANDLER funcDiagEventHandler;
    LSCDMA_DMA_STRUCT    *pChannelArr[SGDMA_NUM_CHANNELS];
    UINT32               SGDMAIPVer;
} LSCDMA_DEV_CTX, *PLSCDMA_DEV_CTX;
/* TODO: You can add fields to store additional device-specific information */

static CHAR gsLSCDMA_LastErr[256];

static DWORD LibInit_count = 0;
/*************************************************************
  Static functions prototypes and inline implementation
 *************************************************************/
#if !defined(__KERNEL__)
static BOOL DeviceInit(const PWDC_DEVICE pDev);
static void DLLCALLCONV LSCDMA_IntHandler(PVOID pData);
static void LSCDMA_EventHandler(WD_EVENT *pEvent, PVOID pData);
static void SGDMAEnable(WDC_DEVICE_HANDLE hDev);
static void SGDMADisable(WDC_DEVICE_HANDLE hDev);
static void DmaInterruptsDisableAll(WDC_DEVICE_HANDLE hDev);
static void SGDMAChannelEnable(LSCDMA_DMA_STRUCT *pDmaStruct);
static void SGDMAChannelDisableAll(WDC_DEVICE_HANDLE hDev);
static void DmaStopAll(WDC_DEVICE_HANDLE hDev);
static DWORD CompleteDmatransfer(LSCDMA_DMA_STRUCT *pDmaStruct);
#endif
static void ErrLog(const CHAR *sFormat, ...);
static void TraceLog(const CHAR *sFormat, ...);

#if !defined(__KERNEL__)
/* Allocate buffer with page aligned address */
static void *__valloc(unsigned long size)
{
#if defined(WIN32)
    UPTR p = 0, buf, page_size;

    page_size = GetPageSize();
    buf = (UPTR)malloc(size + page_size);
    if (buf)
    {
        p = __ALIGN_DOWN(buf + page_size, page_size);
        *(UPTR *)(p - sizeof(UPTR)) = buf;
    }
    return (void *)p;
#else
    return valloc(size);
#endif
}

/* Free buffer with page aligned address */
static void __vfree(void *p)
{
#if defined(WIN32)
    UPTR buf = *(UPTR *)((UPTR)p - sizeof(UPTR));
    free((void *)buf);
#else
    free(p);
#endif
}
#endif

static inline BOOL IsValidDevice(PWDC_DEVICE pDev, const CHAR *sFunc)
{
    if (!pDev || !WDC_GetDevContext(pDev))
    {
        snprintf(gsLSCDMA_LastErr, sizeof(gsLSCDMA_LastErr) - 1, "%s: NULL device %s\n",
            sFunc, !pDev ? "handle" : "context");
        ErrLog(gsLSCDMA_LastErr);
        return FALSE;
    }

    return TRUE;
}

/*************************************************************
  Functions implementation
 *************************************************************/
/* -----------------------------------------------
    LSCDMA and WDC library initialize/uninit
   ----------------------------------------------- */
DWORD LSCDMA_LibInit(void)
{
    DWORD dwStatus;

    /* init only once */
    if (++LibInit_count > 1)
        return WD_STATUS_SUCCESS;

#if defined(WD_DRIVER_NAME_CHANGE)
    /* Set the driver name */
    if (!WD_DriverName(LSCDMA_DEFAULT_DRIVER_NAME))
    {
        ErrLog("Failed to set the driver name for WDC library.\n");
        return WD_SYSTEM_INTERNAL_ERROR;
    }
#endif

    /* Set WDC library's debug options (default: level TRACE, output to Debug Monitor) */
    dwStatus = WDC_SetDebugOptions(WDC_DBG_DEFAULT, NULL);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to initialize debug options for WDC library.\n"
            "Error 0x%x - %s\n", dwStatus, Stat2Str(dwStatus));

        return dwStatus;
    }

    /* Open a handle to the driver and initialize the WDC library */
    dwStatus = WDC_DriverOpen(WDC_DRV_OPEN_DEFAULT, LSCDMA_DEFAULT_LICENSE_STRING);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to initialize the WDC library. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));

        return dwStatus;
    }

    return WD_STATUS_SUCCESS;
}

DWORD LSCDMA_LibUninit(void)
{
    DWORD dwStatus;

    if (--LibInit_count > 0)
        return WD_STATUS_SUCCESS;

    /* Uninit the WDC library and close the handle to WinDriver */
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

WDC_DEVICE_HANDLE LSCDMA_DeviceOpen(DWORD dwVendorId, DWORD dwDeviceId)
{
    WDC_DEVICE_HANDLE hDev = WDC_DIAG_DeviceFindAndOpen(dwVendorId,
        dwDeviceId, KP_LSCDMA_DRIVER_NAME, sizeof(LSCDMA_DEV_CTX));

    if (!hDev || !DeviceInit((PWDC_DEVICE)hDev))
        goto Error;

    return hDev;

Error:
    if (hDev)
        LSCDMA_DeviceClose(hDev);

    printf("LSCDMA_DeviceOpen: Failed opening PCI device: %s",
        LSCDMA_GetLastErr());

    return NULL;
}

BOOL LSCDMA_DeviceClose(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;

    TraceLog("LSCDMA_DeviceClose entered. Device handle: 0x%p\n", hDev);

    if (!hDev)
    {
        ErrLog("LSCDMA_DeviceClose: Error - NULL device handle\n");
        return FALSE;
    }

    SGDMADisable(hDev); /* Disable SGDMA */

    /* Disable interrupts */
    if (LSCDMA_IntIsEnabled(hDev))
    {
        dwStatus = LSCDMA_IntDisable(hDev);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("Failed disabling interrupts. Error 0x%x - %s\n",
                dwStatus, Stat2Str(dwStatus));
        }
    }

    return WDC_DIAG_DeviceClose(hDev);
}

static BOOL DeviceInit(const PWDC_DEVICE pDev)
{
    PLSCDMA_DEV_CTX pDevCtx = (PLSCDMA_DEV_CTX)WDC_GetDevContext(pDev);
    DWORD i, active = 0;
    UINT32 val;

    /* Verify that the device has at least one active address space */
    for (i = 0; i < pDev->dwNumAddrSpaces; i++)
    {
        if (WDC_AddrSpaceIsActive(pDev, i))
            active++;
    }

    if (!active)
    {
        ErrLog("Device does not have any active address spaces (BARs)\n");
        return FALSE;
    }

    WDC_ReadAddr32(pDev, AD_PCI_BAR0, GPIO_ID_REG_OFFSET, &val);
    if (val != GPIO_ID_VALUE)
    {
        ErrLog("Invalid GPIO ID, expected [0x%08x], got [0x%08x]\n",
            GPIO_ID_VALUE, val);
        return FALSE;
    }

    WDC_ReadAddr32(pDev, AD_PCI_BAR0, INTCTL_ID_OFFSET, &val);
    if (val != INTCTL_ID_VALUE)
    {
        ErrLog("Invalid Interrupt Controller ID, expected [0x%08x], "
            "got [0x%08x]\n", INTCTL_ID_VALUE, val);
        return FALSE;
    }

    WDC_ReadAddr32(pDev, AD_PCI_BAR0, SGDMA_IPID_OFFSET, &val);
    if (val != SGDMA_IPID_VALUE)
    {
        ErrLog("Invalid SGDMA ID, expected [0x%08x], got [0x%08x]\n",
            SGDMA_IPID_VALUE, val);
        return FALSE;
    }

    WDC_ReadAddr32(pDev, AD_PCI_BAR0, SGDMA_IPVER_OFFSET, &val);
    val = SGDMA_IPVER_MAJOR_MINOR(val);
    TraceLog("SGDMA IP version: [0x%04x]\n", val);
    if (val == 0x0200 || val == 0x0201 || val == 0x0202)
    {
        ErrLog("This SGDMA version in known to be broken\n");
        return FALSE;
    }
    pDevCtx->SGDMAIPVer = val;

    SGDMADisable(pDev); /* Disable SGDMA until ready */
    /* Group 1, BD base 0, no err mask */
    WDC_WriteAddr32(pDev, AD_PCI_BAR0, SGDMA_CHANNEL_CTRL(SGDMA_WRITE_CHANNEL),
        0x00000040);
    /* Packet Buffer offset 0 */
    WDC_WriteAddr32(pDev, AD_PCI_BAR0, SGDMA_CHANNEL_PBOFF(SGDMA_WRITE_CHANNEL),
        0x00000000);
    /* Group 0, BD base 0, no err mask */
    WDC_WriteAddr32(pDev, AD_PCI_BAR0, SGDMA_CHANNEL_CTRL(SGDMA_READ_CHANNEL),
        0x00000000);
    /* Packet Buffer offset 0 */
    WDC_WriteAddr32(pDev, AD_PCI_BAR0, SGDMA_CHANNEL_PBOFF(SGDMA_READ_CHANNEL),
        0x00000000);
    /* Global Arbiter Control - enable channels 0 and 1 */
    WDC_WriteAddr32(pDev, AD_PCI_BAR0, SGDMA_GARBITER_OFFSET, 0xfffc148f);
    /* Global Event Control - enable channels 0 and 1 */
    WDC_WriteAddr32(pDev, AD_PCI_BAR0, SGDMA_GEVENT_OFFSET, 0xfffc0000);
    SGDMAEnable(pDev); /* Enable SGDMA */

    return TRUE;
}

/* -----------------------------------------------
    Interrupts
   ----------------------------------------------- */
static void DLLCALLCONV LSCDMA_IntHandler(PVOID pData)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PLSCDMA_DEV_CTX pDevCtx = (PLSCDMA_DEV_CTX)WDC_GetDevContext(pDev);
    LSCDMA_INT_RESULT intResult;
    UINT32 u32IntCtrl, u32IntStatus, u32IntEnable, u32Service;
    LSCDMA_DMA_STRUCT *pDmaStruct = NULL;

    WDC_ReadAddr32(pDev, AD_PCI_BAR0, INTCTL_CTRL_OFFSET, &u32IntCtrl);
    WDC_ReadAddr32(pDev, AD_PCI_BAR0, INTCTL_STATUS_OFFSET, &u32IntStatus);
    WDC_ReadAddr32(pDev, AD_PCI_BAR0, INTCTL_ENABLE_OFFSET, &u32IntEnable);

    /* Disable device interrupts */
    WDC_WriteAddr32(pDev, AD_PCI_BAR0, INTCTL_CTRL_OFFSET,
        u32IntCtrl & ~INTCTL_OUTPUT_EN);

    u32Service = u32IntStatus & u32IntEnable;

    if (u32Service & INTCTL_INTR_WR_CHAN)
        pDmaStruct = pDevCtx->pChannelArr[SGDMA_WRITE_CHANNEL];
    else if (u32Service & INTCTL_INTR_RD_CHAN)
        pDmaStruct = pDevCtx->pChannelArr[SGDMA_READ_CHANNEL];

    if (pDmaStruct)
    {
        DWORD dwStatus = CompleteDmatransfer(pDmaStruct);

        if (dwStatus != WD_STATUS_SUCCESS)
        {
            ErrLog("DMA transfer failed\n");
            pDmaStruct = NULL;
        }
    }

    BZERO(intResult);
    intResult.hDma = pDmaStruct;
    intResult.dwCounter = pDev->Int.dwCounter;
    intResult.dwLost = pDev->Int.dwLost;
    intResult.waitResult = (WD_INTERRUPT_WAIT_RESULT)pDev->Int.fStopped;
    intResult.dwEnabledIntType = WDC_GET_ENABLED_INT_TYPE(pDev);
    intResult.dwLastMessage = WDC_GET_ENABLED_INT_LAST_MSG(pDev);

    /* Execute the diagnostics application's interrupt handler routine */
    pDevCtx->funcDiagIntHandler((WDC_DEVICE_HANDLE)pDev, &intResult);
}

DWORD LSCDMA_IntEnable(WDC_DEVICE_HANDLE hDev, LSCDMA_INT_HANDLER funcIntHandler)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PLSCDMA_DEV_CTX pDevCtx;
    UINT32 val;

    TraceLog("LSCDMA_IntEnable entered. Device handle: 0x%p\n", hDev);

    if (!IsValidDevice(pDev, "LSCDMA_IntEnable"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PLSCDMA_DEV_CTX)WDC_GetDevContext(pDev);

    /* Check if interrupts are already enabled */
    if (WDC_IntIsEnabled(hDev))
    {
        ErrLog("Interrupts are already enabled ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* TODO: Define transfer commands in case level sensitive interrupts are
     * used */

    pDevCtx->funcDiagIntHandler = funcIntHandler;
    dwStatus = WDC_IntEnable(hDev, NULL, 0, 0, LSCDMA_IntHandler, (PVOID)pDev,
        WDC_IS_KP(hDev));
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed enabling interrupts. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));

        return dwStatus;
    }

    /* Enable device interrupts */
    WDC_ReadAddr32(hDev, AD_PCI_BAR0, INTCTL_CTRL_OFFSET, &val);
    WDC_WriteAddr32(hDev, AD_PCI_BAR0, INTCTL_CTRL_OFFSET,
        val | INTCTL_OUTPUT_EN);

    TraceLog("LSCDMA_IntEnable: Interrupts enabled\n");

    return WD_STATUS_SUCCESS;
}

DWORD LSCDMA_IntDisable(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    TraceLog("LSCDMA_IntDisable entered. Device handle: 0x%p\n", hDev);

    if (!IsValidDevice(pDev, "LSCDMA_IntDisable"))
        return WD_INVALID_PARAMETER;

    if (!WDC_IntIsEnabled(hDev))
    {
        ErrLog("Interrupts are already disabled ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    DmaInterruptsDisableAll(hDev);

    /* Disable the interrupts */
    dwStatus = WDC_IntDisable(hDev);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed disabling interrupts. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

BOOL LSCDMA_IntIsEnabled(WDC_DEVICE_HANDLE hDev)
{
    if (!IsValidDevice((PWDC_DEVICE)hDev, "LSCDMA_IntIsEnabled"))
        return FALSE;

    return WDC_IntIsEnabled(hDev);
}

/* -----------------------------------------------
    Direct Memory Access (DMA)
   ----------------------------------------------- */

static DWORD CompleteDmatransfer(LSCDMA_DMA_STRUCT *pDmaStruct)
{
    WDC_DEVICE_HANDLE hDev = pDmaStruct->hDev;
    UINT32 u32DmaStatus;

    if (!pDmaStruct->fToDevice)
        WDC_DMASyncIo(pDmaStruct->pDma);

    WDC_ReadAddr32(hDev, AD_PCI_BAR0, SGDMA_CHANNEL_STAT(pDmaStruct->dwChannel),
        &u32DmaStatus);
    WDC_WriteAddr32(hDev, AD_PCI_BAR0,
        SGDMA_CHANNEL_STAT(pDmaStruct->dwChannel),
        0x10); /* Clear Xfer complete */

    LSCDMA_DmaInterruptsDisable(pDmaStruct);

    if (u32DmaStatus & CHANNEL_STATUS_ERRORS ||
        !(u32DmaStatus & CHANNEL_STATUS_XFERCOMP) ||
        !(u32DmaStatus & CHANNEL_STATUS_ENABLED))
    {
        ErrLog("DMA transfer failed\n");
        return WD_OPERATION_FAILED;
    }

    return WD_STATUS_SUCCESS;
}

static void SGDMAEnable(WDC_DEVICE_HANDLE hDev)
{
    /* Enable SGDMA */
    WDC_WriteAddr32(hDev, AD_PCI_BAR0, SGDMA_GSTATUS_OFFSET, 0xE0000000);
}

static void SGDMADisable(WDC_DEVICE_HANDLE hDev)
{
    DmaStopAll(hDev);
    DmaInterruptsDisableAll(hDev);
    SGDMAChannelDisableAll(hDev);

    /* Disable SGDMA */
    WDC_WriteAddr32(hDev, AD_PCI_BAR0, SGDMA_GSTATUS_OFFSET, 0x00000000);
}

static void SGDMAChannelEnable(LSCDMA_DMA_STRUCT *pDmaStruct)
{
    WDC_DEVICE_HANDLE hDev = pDmaStruct->hDev;
    PLSCDMA_DEV_CTX pDevCtx = (PLSCDMA_DEV_CTX)WDC_GetDevContext(hDev);
    DWORD dwChannel = pDmaStruct->dwChannel;
    UINT32 val;

    WDC_ReadAddr32(hDev, AD_PCI_BAR0, SGDMA_CHANNEL_CTRL(dwChannel), &val);
    val |= pDevCtx->SGDMAIPVer < 0x200 ?
        pDmaStruct->dwStartBD << 8 : pDmaStruct->dwStartBD << 16;
    WDC_WriteAddr32(hDev, AD_PCI_BAR0, SGDMA_CHANNEL_CTRL(dwChannel), val);

    WDC_WriteAddr32(hDev, AD_PCI_BAR0, SGDMA_CHANNEL_PBOFF(dwChannel),
        0x00000000);

    /* Unmask channel and set default weights */
    WDC_ReadAddr32(hDev, AD_PCI_BAR0, SGDMA_GARBITER_OFFSET, &val);
    val = (val & ~(0x00010000 << dwChannel)) | 0x0000148f;
    WDC_WriteAddr32(hDev, AD_PCI_BAR0, SGDMA_GARBITER_OFFSET, val);

    /* Unmask events for this channel */
    WDC_ReadAddr32(hDev, AD_PCI_BAR0, SGDMA_GEVENT_OFFSET, &val);
    val &= ~(0x00010000 << dwChannel);
    WDC_WriteAddr32(hDev, AD_PCI_BAR0, SGDMA_GEVENT_OFFSET, val);

    /* Enabke the channel */
    WDC_ReadAddr32(hDev, AD_PCI_BAR0, SGDMA_GCONTROL_OFFSET, &val);
    val = (val & ~(0x00010000 << dwChannel)) | (1 << dwChannel);
    WDC_WriteAddr32(hDev, AD_PCI_BAR0, SGDMA_GCONTROL_OFFSET, val);

    /* Make sure that SGDMA is enabled */
    SGDMAEnable(hDev);
}

static void SGDMAChannelDisableAll(WDC_DEVICE_HANDLE hDev)
{
    /* Disable and reset all channels, mask DMA requests */
    WDC_WriteAddr32(hDev, AD_PCI_BAR0, SGDMA_GCONTROL_OFFSET, 0xFFFF0000);
}

static void SGDMAChannelDisable(LSCDMA_DMA_STRUCT *pDmaStruct)
{
    WDC_DEVICE_HANDLE hDev = pDmaStruct->hDev;
    UINT32 val;

    WDC_ReadAddr32(hDev, AD_PCI_BAR0, SGDMA_GCONTROL_OFFSET, &val);
    val &= ~(1 << pDmaStruct->dwChannel); /* Disable and reset the channel */
    val |= 1 << (16 + pDmaStruct->dwChannel); /* Mask DMA requests */
    WDC_WriteAddr32(hDev, AD_PCI_BAR0, SGDMA_GCONTROL_OFFSET, val);
}

static void DmaInterruptsDisableAll(WDC_DEVICE_HANDLE hDev)
{
    WDC_WriteAddr32(hDev, AD_PCI_BAR0, INTCTL_ENABLE_OFFSET, 0x00000000);
    WDC_WriteAddr32(hDev, AD_PCI_BAR0, INTCTL_CTRL_OFFSET, 0x00000000);
}

void LSCDMA_DmaInterruptsEnable(LSCDMA_DMA_HANDLE hDma)
{
    LSCDMA_DMA_STRUCT *pDmaStruct = (LSCDMA_DMA_STRUCT *)hDma;
    UINT32 val;

    WDC_ReadAddr32(pDmaStruct->hDev, AD_PCI_BAR0, INTCTL_ENABLE_OFFSET, &val);
    val |= 1<<pDmaStruct->dwChannel;
    WDC_WriteAddr32(pDmaStruct->hDev, AD_PCI_BAR0, INTCTL_ENABLE_OFFSET, val);
}

void LSCDMA_DmaInterruptsDisable(LSCDMA_DMA_HANDLE hDma)
{
    LSCDMA_DMA_STRUCT *pDmaStruct = (LSCDMA_DMA_STRUCT *)hDma;
    UINT32 val;

    WDC_ReadAddr32(pDmaStruct->hDev, AD_PCI_BAR0, INTCTL_ENABLE_OFFSET, &val);
    val &= ~(1<<pDmaStruct->dwChannel);
    WDC_WriteAddr32(pDmaStruct->hDev, AD_PCI_BAR0, INTCTL_ENABLE_OFFSET, val);
}

void LSCDMA_DmaInterruptsDisableAll(WDC_DEVICE_HANDLE hDev)
{
    WDC_WriteAddr32(hDev, AD_PCI_BAR0, INTCTL_ENABLE_OFFSET, 0x00000000);
    WDC_WriteAddr32(hDev, AD_PCI_BAR0, INTCTL_CTRL_OFFSET, 0x00000000);
}

void LSCDMA_DmaStart(LSCDMA_DMA_HANDLE hDma)
{
    LSCDMA_DMA_STRUCT *pDmaStruct = (LSCDMA_DMA_STRUCT *)hDma;

    if (pDmaStruct->fToDevice)
        WDC_DMASyncCpu(pDmaStruct->pDma);

    LSCDMA_DmaInterruptsEnable(pDmaStruct);
    WDC_WriteAddr32(pDmaStruct->hDev, AD_PCI_BAR0, GPIO_DMAREQ_OFFSET,
        1<<pDmaStruct->dwChannel);
}

void LSCDMA_DMAStop(LSCDMA_DMA_HANDLE hDma)
{
    LSCDMA_DMA_STRUCT *pDmaStruct = (LSCDMA_DMA_STRUCT *)hDma;
    UINT32 val;

    WDC_ReadAddr32(pDmaStruct->hDev, AD_PCI_BAR0, GPIO_DMAREQ_OFFSET, &val);
    WDC_WriteAddr32(pDmaStruct->hDev, AD_PCI_BAR0, GPIO_DMAREQ_OFFSET,
        val & ~(1<<pDmaStruct->dwChannel));
}

static void DmaToDeviceTransferBuild(LSCDMA_DMA_STRUCT *pDmaStruct)
{
    WDC_DEVICE_HANDLE hDev = pDmaStruct->hDev;
    UINT32 val, i, u32DstAddr, u32DstAddrMode, u32DstDataWidth;

    u32DstAddr = SGDMA_EBR64_BASE_ADDR(0);
    u32DstAddrMode = SGDMA_ADDR_MODE_MEM;
    u32DstDataWidth = SGDMA_DATA_64BIT;

    for (i = 0; i < pDmaStruct->pDma->dwPages; i++)
    {
        DWORD bd = pDmaStruct->dwStartBD + i;
        DWORD len = pDmaStruct->pDma->Page[i].dwBytes;

        val = SGDMA_DST_ADDR_MODE(u32DstAddrMode) |
            SGDMA_DST_SIZE(u32DstDataWidth) | SGDMA_DST_BUS(SGDMA_BUS_A) |
            SGDMA_SRC_MEM | SGDMA_SRC_SIZE(SGDMA_DATA_64BIT) |
            SGDMA_SRC_BUS(SGDMA_BUS_B) | 0xf0;
        if (i == pDmaStruct->pDma->dwPages - 1)
            val |= SGDMA_EOL;
        WDC_WriteAddr32(hDev, AD_PCI_BAR0, SGDMA_BD_CFG0(bd), val);

        val = SGDMA_BURST_SIZE(MIN(len, pDmaStruct->dwBurstSize)) | len;
        WDC_WriteAddr32(hDev, AD_PCI_BAR0, SGDMA_BD_CFG1(bd), val);

        WDC_WriteAddr32(hDev, AD_PCI_BAR0, SGDMA_BD_SRC(bd),
            (UINT32)pDmaStruct->pDma->Page[i].pPhysicalAddr);
        WDC_WriteAddr32(hDev, AD_PCI_BAR0, SGDMA_BD_DST(bd),
            SGDMA_WB(u32DstAddr));

        if (u32DstAddrMode != SGDMA_ADDR_MODE_FIFO)
            u32DstAddr += len;
    }
}

static void DmaFromDeviceTransferBuild(LSCDMA_DMA_STRUCT *pDmaStruct)
{
    WDC_DEVICE_HANDLE hDev = pDmaStruct->hDev;
    UINT32 val, i, u32SrcAddr, u32SrcAddrMode, u32SrcDataWidth;

    u32SrcAddr = SGDMA_EBR64_BASE_ADDR(0);
    u32SrcAddrMode = SGDMA_ADDR_MODE_MEM;
    u32SrcDataWidth = SGDMA_DATA_64BIT;

    for (i = 0; i < pDmaStruct->pDma->dwPages; i++)
    {
        DWORD bd = pDmaStruct->dwStartBD + i;
        DWORD len = pDmaStruct->pDma->Page[i].dwBytes;

        val = SGDMA_DST_MEM | SGDMA_DST_SIZE(SGDMA_DATA_64BIT) |
            SGDMA_DST_BUS(SGDMA_BUS_B) | SGDMA_SRC_ADDR_MODE(u32SrcAddrMode) |
            SGDMA_SRC_SIZE(u32SrcDataWidth) | SGDMA_SRC_BUS(SGDMA_BUS_A);
        if (i == pDmaStruct->pDma->dwPages - 1)
            val |= SGDMA_EOL;
        WDC_WriteAddr32(hDev, AD_PCI_BAR0, SGDMA_BD_CFG0(bd), val);

        val = SGDMA_BURST_SIZE(MIN(len, pDmaStruct->dwBurstSize)) | len;
        WDC_WriteAddr32(hDev, AD_PCI_BAR0, SGDMA_BD_CFG1(bd), val);

        WDC_WriteAddr32(hDev, AD_PCI_BAR0, SGDMA_BD_SRC(bd),
            SGDMA_WB(u32SrcAddr));
        WDC_WriteAddr32(hDev, AD_PCI_BAR0, SGDMA_BD_DST(bd),
            (UINT32)pDmaStruct->pDma->Page[i].pPhysicalAddr);

        if (u32SrcAddrMode != SGDMA_ADDR_MODE_FIFO)
            u32SrcAddr += len;
    }
}

static void DmaTransferBuild(LSCDMA_DMA_STRUCT *pDmaStruct)
{
    if (pDmaStruct->fToDevice)
        DmaToDeviceTransferBuild(pDmaStruct);
    else
        DmaFromDeviceTransferBuild(pDmaStruct);
}

static void DmaStopAll(WDC_DEVICE_HANDLE hDev)
{
    WDC_WriteAddr32(hDev, AD_PCI_BAR0, GPIO_CNTRCTRL_OFFSET, 0x00000000);
    WDC_WriteAddr32(hDev, AD_PCI_BAR0, GPIO_DMAREQ_OFFSET, 0x00000000);
}

LSCDMA_DMA_HANDLE LSCDMA_DmaOpen(WDC_DEVICE_HANDLE hDev, DWORD dwBytes,
    BOOL fToDevice)
{
    PLSCDMA_DEV_CTX pDevCtx = (PLSCDMA_DEV_CTX)WDC_GetDevContext(hDev);
    LSCDMA_DMA_STRUCT *pDmaStruct = NULL;
    DWORD dwChannel = fToDevice ? SGDMA_READ_CHANNEL : SGDMA_WRITE_CHANNEL;
    DWORD dwStatus;

    if (pDevCtx->pChannelArr[dwChannel])
    {
        TraceLog("DMA handle already open for channel %d\n", dwChannel);
        return pDevCtx->pChannelArr[dwChannel];
    }

    if ((fToDevice && dwBytes > MAX_DMA_TO_DEV_BYTES) ||
        (!fToDevice && dwBytes > MAX_DMA_FROM_DEV_BYTES))
    {
        ErrLog("Maximum allowed DMA buffer size is [%d bytes] "
            "(requested [%d bytes])\n", fToDevice ? MAX_DMA_TO_DEV_BYTES :
            MAX_DMA_FROM_DEV_BYTES, dwBytes);
        return NULL;
    }

    pDmaStruct = (LSCDMA_DMA_STRUCT *)calloc(1, sizeof(LSCDMA_DMA_STRUCT));
    if (!pDmaStruct)
    {
        ErrLog("Memory allocation failure\n");
        return NULL;
    }

    pDmaStruct->pBuf = __valloc(dwBytes);
    if (!pDmaStruct->pBuf)
    {
        ErrLog("Memory allocation failure\n");
        goto Error;
    }

    dwStatus = WDC_DMASGBufLock(hDev, pDmaStruct->pBuf,
        fToDevice ? DMA_TO_DEVICE : DMA_FROM_DEVICE, dwBytes,
        &pDmaStruct->pDma);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("Failed locking DMA buffer. Error 0x%x - %s\n", dwStatus,
            Stat2Str(dwStatus));
        goto Error;
    }

    pDmaStruct->hDev = hDev;
    pDmaStruct->dwBytes = dwBytes;
    pDmaStruct->dwChannel = dwChannel;
    pDmaStruct->fToDevice = fToDevice;
    /* TODO: Set the burst size according to PCI-E capability structure:
     * PCIeMaxPayloadSize for DMA_TO_DEVICE and PCIeMaxReadReqSize for
     * DMA_FROM_DEVICE */
    pDmaStruct->dwBurstSize = fToDevice ? 512 : 128;
    /* Set read descriptors after write descriptors */
    pDmaStruct->dwStartBD = !fToDevice ? 0 : MAX_DMA_WRITE_DESCS;
    pDevCtx->pChannelArr[dwChannel] = pDmaStruct;

    SGDMAChannelEnable(pDmaStruct);
    DmaTransferBuild(pDmaStruct);

    return (LSCDMA_DMA_HANDLE)pDmaStruct;

Error:
    if (pDmaStruct->pDma)
        WDC_DMABufUnlock(pDmaStruct->pDma);
    if (pDmaStruct->pBuf)
        __vfree(pDmaStruct->pBuf);
    free(pDmaStruct);
    return NULL;
}

void LSCDMA_DmaClose(LSCDMA_DMA_HANDLE hDma)
{
    LSCDMA_DMA_STRUCT *pDmaStruct = (LSCDMA_DMA_STRUCT *)hDma;
    WDC_DEVICE_HANDLE hDev = pDmaStruct->hDev;
    PLSCDMA_DEV_CTX pDevCtx = (PLSCDMA_DEV_CTX)WDC_GetDevContext(hDev);

    SGDMAChannelDisable(pDmaStruct);
    WDC_DMABufUnlock(pDmaStruct->pDma);
    __vfree(pDmaStruct->pBuf);
    pDevCtx->pChannelArr[pDmaStruct->dwChannel] = NULL;
    free(pDmaStruct);
}

/* Returns pointer to the allocated virtual buffer and buffer size in bytes */
PVOID LSCDMA_DmaBufferGet(LSCDMA_DMA_HANDLE hDma, DWORD *pBytes)
{
    LSCDMA_DMA_STRUCT *pDmaStruct = (LSCDMA_DMA_STRUCT *)hDma;

    if (!hDma || !pBytes)
        return NULL;

    *pBytes = pDmaStruct->dwBytes;
    return pDmaStruct->pBuf;
}

/* Returns DMA direction. */
BOOL LSCDMA_DmaIstoDevice(LSCDMA_DMA_HANDLE hDma)
{
    return ((LSCDMA_DMA_STRUCT *)hDma)->fToDevice;
}

/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */
static void LSCDMA_EventHandler(WD_EVENT *pEvent, PVOID pData)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PLSCDMA_DEV_CTX pDevCtx = (PLSCDMA_DEV_CTX)WDC_GetDevContext(pDev);

    TraceLog("LSCDMA_EventHandler entered, pData: 0x%p, dwAction 0x%x\n",
        pData, pEvent->dwAction);

    /* Execute the diagnostics application's event handler function */
    pDevCtx->funcDiagEventHandler((WDC_DEVICE_HANDLE)pDev, pEvent->dwAction);
}

DWORD LSCDMA_EventRegister(WDC_DEVICE_HANDLE hDev, LSCDMA_EVENT_HANDLER funcEventHandler)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PLSCDMA_DEV_CTX pDevCtx;
    DWORD dwActions = WD_ACTIONS_ALL;
    /* TODO: Modify the above to set up the plug-and-play/power management
             events for which you wish to receive notifications.
             dwActions can be set to any combination of the WD_EVENT_ACTION
             flags defined in windrvr.h */

    TraceLog("LSCDMA_EventRegister entered. Device handle: 0x%p\n", hDev);

    if (!IsValidDevice(pDev, "LSCDMA_EventRegister"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PLSCDMA_DEV_CTX)WDC_GetDevContext(pDev);

    /* Check if event is already registered */
    if (WDC_EventIsRegistered(hDev))
    {
        ErrLog("Events are already registered ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Store the diag event handler routine to be executed from LSCDMA_EventHandler() upon an event */
    pDevCtx->funcDiagEventHandler = funcEventHandler;

    /* Register event */
    dwStatus = WDC_EventRegister(hDev, dwActions, LSCDMA_EventHandler, hDev, TRUE);

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to register events. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    TraceLog("Events registered\n");

    return WD_STATUS_SUCCESS;
}

DWORD LSCDMA_EventUnregister(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;

    TraceLog("LSCDMA_EventUnregister entered. Device handle: 0x%p\n", hDev);

    if (!IsValidDevice((PWDC_DEVICE)hDev, "LSCDMA_EventUnregister"))
        return WD_INVALID_PARAMETER;

    if (!WDC_EventIsRegistered(hDev))
    {
        ErrLog("Cannot unregister events - no events currently registered ...\n");
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

BOOL LSCDMA_EventIsRegistered(WDC_DEVICE_HANDLE hDev)
{
    if (!IsValidDevice((PWDC_DEVICE)hDev, "LSCDMA_EventIsRegistered"))
        return FALSE;

    return WDC_EventIsRegistered(hDev);
}
#endif

/* -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- */
static void ErrLog(const CHAR *sFormat, ...)
{
#if defined(DEBUG)
    va_list argp;
    va_start(argp, sFormat);
    vsnprintf(gsLSCDMA_LastErr, sizeof(gsLSCDMA_LastErr) - 1, sFormat, argp);
#if defined(__KERNEL__)
    WDC_Err("KP LSCDMA lib: %s", LSCDMA_GetLastErr());
#else
    WDC_Err("LSCDMA lib: %s", LSCDMA_GetLastErr());
#endif
    va_end(argp);
#else
    UNUSED_VAR(sFormat);
#endif
}

static void TraceLog(const CHAR *sFormat, ...)
{
#if defined(DEBUG)
    CHAR sMsg[256];
    va_list argp;
    va_start(argp, sFormat);
    vsnprintf(sMsg, sizeof(sMsg) - 1, sFormat, argp);
#if defined(__KERNEL__)
    WDC_Trace("KP LSCDMA lib: %s", sMsg);
#else
    WDC_Trace("LSCDMA lib: %s", sMsg);
#endif
    va_end(argp);
#else
    UNUSED_VAR(sFormat);
#endif
}

const char *LSCDMA_GetLastErr(void)
{
    return gsLSCDMA_LastErr;
}
