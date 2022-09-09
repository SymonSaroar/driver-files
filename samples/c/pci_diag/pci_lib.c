/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/************************************************************************
*  File: pci_lib.c
*
*  Library for accessing PCI devices, possibly using a Kernel PlugIn driver.
*  The code accesses hardware using WinDriver's WDC library.

*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*************************************************************************/

#ifdef __KERNEL__
#include "kpstdlib.h"
#endif
#include "wdc_defs.h"
#include "utils.h"
#include "pci_lib.h"

/*************************************************************
  Internal definitions
 *************************************************************/
/* WinDriver license registration string */
/* TODO: When using a registered WinDriver version, replace the license string
         below with the development license in order to use on the development
         machine.
         Once you require to distribute the driver's package to other machines,
         please replace the string with a distribution license */
#define PCI_DEFAULT_LICENSE_STRING "12345abcde1234.license"

#define PCI_DEFAULT_DRIVER_NAME WD_DEFAULT_DRIVER_NAME_BASE

/* PCI device information struct */
typedef struct {
    WD_TRANSFER       *pIntTransCmds;
    PCI_INT_HANDLER   funcDiagIntHandler;   /* Interrupt handler routine */
    PCI_EVENT_HANDLER funcDiagEventHandler; /* Event handler routine */

} PCI_DEV_CTX, *PPCI_DEV_CTX;
/* TODO: You can add fields to store additional device-specific information. */

/*************************************************************
  Global variables definitions
 *************************************************************/
/* Last error information string */
static CHAR gsPCI_LastErr[256];

/* Library initialization reference count */
static DWORD LibInit_count = 0;

/*************************************************************
  Static functions prototypes and inline implementation
 *************************************************************/

#ifndef __KERNEL__
    static BOOL DeviceValidate(const PWDC_DEVICE pDev);
    static void PCI_EventHandler(WD_EVENT *pEvent, PVOID pData);
#endif
static void DLLCALLCONV PCI_IntHandler(PVOID pData);
static void ErrLog(const CHAR *sFormat, ...);
static void TraceLog(const CHAR *sFormat, ...);

/* Validate a device handle */
static inline BOOL IsValidDevice(PWDC_DEVICE pDev, const CHAR *sFunc)
{
    if (!pDev || !(PPCI_DEV_CTX)(pDev->pCtx))
    {
        snprintf(gsPCI_LastErr, sizeof(gsPCI_LastErr) - 1,
            "%s: NULL device %s\n", sFunc, !pDev ? "handle" : "context");
        ErrLog(gsPCI_LastErr);
        return FALSE;
    }

    return TRUE;
}

/*************************************************************
  Functions implementation
 *************************************************************/
/* -----------------------------------------------
    PCI and WDC libraries initialize/uninitialize
   ----------------------------------------------- */
/* Initialize the PCI and WDC libraries */
DWORD PCI_LibInit(void)
{
    DWORD dwStatus;

    /* Increase the library's reference count; initialize the library only once
     */
    if (++LibInit_count > 1)
        return WD_STATUS_SUCCESS;

#ifdef WD_DRIVER_NAME_CHANGE
    /* Set the driver name */
    if (!WD_DriverName(PCI_DEFAULT_DRIVER_NAME))
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
    dwStatus = WDC_DriverOpen(WDC_DRV_OPEN_DEFAULT, PCI_DEFAULT_LICENSE_STRING);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to initialize the WDC library. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    return WD_STATUS_SUCCESS;
}

/* Uninitialize the PCI and WDC libraries */
DWORD PCI_LibUninit(void)
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

#ifndef __KERNEL__
/* -----------------------------------------------
    Device open/close
   ----------------------------------------------- */
/* Open a device handle */
WDC_DEVICE_HANDLE PCI_DeviceOpen(DWORD dwVendorId, DWORD dwDeviceId)
{
    WDC_DEVICE_HANDLE hDev;

    hDev = WDC_DIAG_DeviceFindAndOpen(dwVendorId,
        dwDeviceId, KP_PCI_DRIVER_NAME, sizeof(PCI_DEV_CTX));

    if (!hDev || !DeviceValidate((PWDC_DEVICE)hDev))
        goto Error;

    return hDev;

Error:
    if (hDev)
        PCI_DeviceClose(hDev);

    ErrLog("PCI_DeviceOpen: Failed opening PCI device\n");

    return NULL;
}

/* Close device handle */
BOOL PCI_DeviceClose(WDC_DEVICE_HANDLE hDev)
{
    TraceLog("PCI_DeviceClose: Entered. Device handle 0x%p\n", hDev);

    /* Disable interrupts (if enabled) */
    if (WDC_IntIsEnabled(hDev))
    {
        DWORD dwStatus = PCI_IntDisable(hDev);
        if (WD_STATUS_SUCCESS != dwStatus)
        {
            ErrLog("Failed disabling interrupts. Error 0x%x - %s\n", dwStatus,
                Stat2Str(dwStatus));
        }
    }

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

/* -----------------------------------------------
    Interrupts
   ----------------------------------------------- */
/* Interrupt handler routine */
static void DLLCALLCONV PCI_IntHandler(PVOID pData)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PPCI_DEV_CTX pDevCtx = (PPCI_DEV_CTX)(pDev->pCtx);
    PCI_INT_RESULT intResult;

    BZERO(intResult);
    intResult.dwCounter = pDev->Int.dwCounter;
    intResult.dwLost = pDev->Int.dwLost;
    intResult.waitResult = (WD_INTERRUPT_WAIT_RESULT)pDev->Int.fStopped;
    intResult.dwEnabledIntType = WDC_GET_ENABLED_INT_TYPE(pDev);
    intResult.dwLastMessage = WDC_GET_ENABLED_INT_LAST_MSG(pDev);

    /* Execute the diagnostics application's interrupt handler routine */
    pDevCtx->funcDiagIntHandler((WDC_DEVICE_HANDLE)pDev, &intResult);
}

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
/* Enable interrupts */
DWORD PCI_IntEnable(WDC_DEVICE_HANDLE hDev, PCI_INT_HANDLER funcIntHandler)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PPCI_DEV_CTX pDevCtx;

    WDC_ADDR_DESC *pAddrDesc;
    WD_TRANSFER *pTrans = NULL;
    DWORD dwNumTransCmds = 0;
    DWORD dwOptions = 0;

    TraceLog("PCI_IntEnable: Entered. Device handle 0x%p\n", hDev);

    /* Validate the device handle */
    if (!IsValidDevice(pDev, "PCI_IntEnable"))
        return WD_INVALID_PARAMETER;

    /* Verify that the device has an interrupt item */
    if (!IsItemExists(pDev, ITEM_INTERRUPT))
        return WD_OPERATION_FAILED;

    pDevCtx = (PPCI_DEV_CTX)(pDev->pCtx);

    /* Check whether interrupts are already enabled */
    if (WDC_IntIsEnabled(hDev))
    {
        ErrLog("Interrupts are already enabled ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }


    /* When using a Kernel PlugIn, acknowledge interrupts in kernel mode */
    if (!WDC_IS_KP(pDev))
    {
        /* TODO: Change this value, if needed */
        dwNumTransCmds = 2;

        /* This sample demonstrates how to set up two transfer commands, one
         * for reading the device's INTCSR register (as defined in gPCI_Regs)
         * and one for writing to it to acknowledge the interrupt. The transfer
         * commands will be executed by WinDriver in the kernel when an
         * interrupt occurs.*/
        /* TODO: If PCI interrupts are level sensitive interrupts, they must be
         * acknowledged in the kernel immediately when they are received. Since
         * the information for acknowledging the interrupts is
         * hardware-specific, YOU MUST MODIFY THE CODE below and set up transfer
         * commands in order to correctly acknowledge the interrupts on your
         * device, as dictated by your hardware's specifications.
         * If the device supports both MSI/MSI-X and level sensitive interrupts,
         * you must set up transfer commands in order to allow your code to run
         * correctly on systems other than Windows Vista and higher and Linux.
         * Since MSI/MSI-X does not require acknowledgment of the interrupt, to
         * support only MSI/MSI-X handling (for hardware and OSs that support
         * this), you can avoid defining transfer commands, or specify
         * kernel-mode commands to be performed upon interrupt generation
         * according to your specific needs. */
        /******************************************************************
         * NOTE: If you attempt to use this code without first modifying it in
         * order to correctly acknowledge your device's level-sensitive
         * interrupts, as explained above, the OS will HANG when a level
         * sensitive interrupt occurs!
         ********************************************************************/

        /* Allocate memory for the interrupt transfer commands */
        pTrans = (WD_TRANSFER *)calloc(dwNumTransCmds, sizeof(WD_TRANSFER));
        if (!pTrans)
        {
            ErrLog("Failed allocating memory for interrupt transfer "
                "commands\n");
            return WD_INSUFFICIENT_RESOURCES;
        }

        /* Prepare the interrupt transfer commands.
         *
         * The transfer commands will be executed by WinDriver's ISR
         * which runs in kernel mode at interrupt level.
         */

        /* TODO: Change the offset of INTCSR and the PCI address space, if
         *       needed */
        /* #1: Read status from the INTCSR register */
        pAddrDesc = WDC_GET_ADDR_DESC(pDev, INTCSR_ADDR_SPACE);
        if (pAddrDesc)
        {
            pTrans[0].pPort = pAddrDesc->pAddr + INTCSR;
            /* Read from a 32-bit register */
            pTrans[0].cmdTrans = WDC_ADDR_IS_MEM(pAddrDesc) ? RM_DWORD :
                RP_DWORD;

            /* #2: Write ALL_INT_MASK to the INTCSR register to acknowledge the
             *     interrupt */
            pTrans[1].pPort = pTrans[0].pPort; /* In this example both commands
                                                access the same address
                                                (register) */
            /* Write to a 32-bit register */
            pTrans[1].cmdTrans = WDC_ADDR_IS_MEM(pAddrDesc) ? WM_DWORD :
                WP_DWORD;
            pTrans[1].Data.Dword = ALL_INT_MASK;

            /* Copy the results of "read" transfer commands back to user mode */
            dwOptions = INTERRUPT_CMD_COPY;
        }
    }

    /* Store the diag interrupt handler routine, which will be executed by
       PCI_IntHandler() when an interrupt is received */
    pDevCtx->funcDiagIntHandler = funcIntHandler;

    /* Enable interrupts */
    dwStatus = WDC_IntEnable(hDev, pTrans, dwNumTransCmds, dwOptions,
        PCI_IntHandler, (PVOID)pDev, WDC_IS_KP(pDev));
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed enabling interrupts. Error 0x%x - %s\n", dwStatus,
            Stat2Str(dwStatus));

        if (pTrans)
            free(pTrans);

        return dwStatus;
    }

    /* Store the interrupt transfer commands in the device context */
    pDevCtx->pIntTransCmds = pTrans;

    /* TODO: You can add code here to write to the device in order to
             physically enable the hardware interrupts. */

    TraceLog("PCI_IntEnable: Interrupts enabled\n");

    return WD_STATUS_SUCCESS;
}

/* Disable interrupts */
DWORD PCI_IntDisable(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PPCI_DEV_CTX pDevCtx;

    TraceLog("PCI_IntDisable entered. Device handle 0x%p\n", hDev);

    /* Validate the device handle */
    if (!IsValidDevice(pDev, "PCI_IntDisable"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PPCI_DEV_CTX)(pDev->pCtx);

    /* Check whether interrupts are already enabled */
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
    /* Free the memory allocated for the interrupt transfer commands */
    if (pDevCtx->pIntTransCmds)
    {
        free(pDevCtx->pIntTransCmds);
        pDevCtx->pIntTransCmds = NULL;
    }

    return dwStatus;
}

/* Check whether interrupts are enabled for the given device */
BOOL PCI_IntIsEnabled(WDC_DEVICE_HANDLE hDev)
{
    /* Validate the device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "PCI_IntIsEnabled"))
        return FALSE;

    /* Check whether interrupts are already enabled */
    return WDC_IntIsEnabled(hDev);
}


#if defined(LINUX)
/* -----------------------------------------------
   SRIOV handling
   ----------------------------------------------- */

DWORD PCI_SriovEnable(WDC_DEVICE_HANDLE hDev, DWORD dwNumVFs)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    TraceLog("PCI_SriovEnable entered. Device handle: 0x%p\n", hDev);

    /* Validate the device handle */
    if (!IsValidDevice(pDev, "PCI_SriovEnable"))
        return WD_INVALID_PARAMETER;

    return WDC_PciSriovEnable(hDev, dwNumVFs);

}

DWORD PCI_SriovDisable(WDC_DEVICE_HANDLE hDev)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    TraceLog("PCI_SriovDisable entered. Device handle: 0x%p\n", hDev);

    /* Validate the device handle */
    if (!IsValidDevice(pDev, "PCI_SriovDisable"))
        return WD_INVALID_PARAMETER;

    return WDC_PciSriovDisable(hDev);
}

DWORD PCI_SriovGetNumVFs(WDC_DEVICE_HANDLE hDev, DWORD* pdwNumVFs)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    TraceLog("PCI_SriovGetNumVFs entered. Device handle: 0x%p\n", hDev);
    if (pdwNumVFs)
        *pdwNumVFs = 0;

    /* Validate the device handle */
    if (!IsValidDevice(pDev, "PCI_SriovGetNumVfs"))
        return WD_INVALID_PARAMETER;

    return WDC_PciSriovGetNumVFs(hDev, pdwNumVFs);
}
#endif

/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */
/* Plug-and-play or power management event handler routine */
static void PCI_EventHandler(WD_EVENT *pEvent, PVOID pData)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PPCI_DEV_CTX pDevCtx = (PPCI_DEV_CTX)(pDev->pCtx);

    TraceLog("PCI_EventHandler entered, pData 0x%p, dwAction 0x%x\n", pData,
        pEvent->dwAction);

    /* Execute the diagnostics application's event handler function */
    pDevCtx->funcDiagEventHandler((WDC_DEVICE_HANDLE)pDev, pEvent->dwAction);
}

/* Register a plug-and-play or power management event */
DWORD PCI_EventRegister(WDC_DEVICE_HANDLE hDev,
    PCI_EVENT_HANDLER funcEventHandler)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PPCI_DEV_CTX pDevCtx;
    DWORD dwActions = WD_ACTIONS_ALL;
    /* TODO: Modify the above to set up the plug-and-play/power management
             events for which you wish to receive notifications.
             dwActions can be set to any combination of the WD_EVENT_ACTION
             flags defined in windrvr.h. */

    TraceLog("PCI_EventRegister entered. Device handle 0x%p\n", hDev);

    /* Validate the device handle */
    if (!IsValidDevice(pDev, "PCI_EventRegister"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PPCI_DEV_CTX)(pDev->pCtx);

    /* Check whether the event is already registered */
    if (WDC_EventIsRegistered(hDev))
    {
        ErrLog("Events are already registered ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Store the diag event handler routine to be executed from
     * PCI_EventHandler() upon an event */
    pDevCtx->funcDiagEventHandler = funcEventHandler;

    /* Register the event */
    dwStatus = WDC_EventRegister(hDev, dwActions, PCI_EventHandler, hDev,
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
DWORD PCI_EventUnregister(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;

    TraceLog("PCI_EventUnregister entered. Device handle 0x%p\n", hDev);

    /* Validate the device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "PCI_EventUnregister"))
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
BOOL PCI_EventIsRegistered(WDC_DEVICE_HANDLE hDev)
{
    /* Validate the device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "PCI_EventIsRegistered"))
        return FALSE;

    /* Check whether the event is registered */
    return WDC_EventIsRegistered(hDev);
}

/* -----------------------------------------------
    DMA
   ----------------------------------------------- */
DWORD PCI_DmaAllocContig(MENU_CTX_DMA *pDmaMenusCtx)
{
    DWORD dwStatus;

    dwStatus = WDC_DMAContigBufLock(*(pDmaMenusCtx->phDev),
        &(pDmaMenusCtx->pBuf), pDmaMenusCtx->dwOptions, pDmaMenusCtx->size,
        &(pDmaMenusCtx->pDma));
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("Failed allocating contiguous memory.size[% lu], "
            "Error [0x%x - %s]\n", pDmaMenusCtx->size, dwStatus,
            Stat2Str(dwStatus));
    }

    return dwStatus;
}

DWORD PCI_DmaAllocSg(MENU_CTX_DMA *pDmaMenusCtx)
{
    DWORD dwStatus, dwOptions = 0;

    dwStatus = WDC_DMASGBufLock(*(pDmaMenusCtx->phDev), pDmaMenusCtx->pBuf,
        dwOptions, pDmaMenusCtx->size, &(pDmaMenusCtx->pDma));
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("Failed allocating user memory for SG. size [%d]\n",
            "Error [0x%x - %s]\n", pDmaMenusCtx->size,
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

DWORD PCI_DmaAllocReserved(MENU_CTX_DMA *pDmaMenusCtx)
{
    DWORD dwStatus, dwOptions = 0;

    dwStatus = WDC_DMAReservedBufLock(*(pDmaMenusCtx->phDev),
        pDmaMenusCtx->qwAddr, &pDmaMenusCtx->pBuf, dwOptions,
        pDmaMenusCtx->size, &pDmaMenusCtx->pDma);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("Failed claiming reserved memory. size [%d], "
            "Error [0x%x - %s]\n", pDmaMenusCtx->size,
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

DWORD PCI_DmaBufUnlock(WD_DMA *pDma)
{
    DWORD dwStatus;

    dwStatus = WDC_DMABufUnlock(pDma);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        ErrLog("Failed trying to free DMA memory. Error [0x%x - %s]\n",
            dwStatus, Stat2Str(dwStatus));
    }

    return dwStatus;
}

#endif /* __KERNEL__ */

/* -----------------------------------------------
    Address spaces information
   ----------------------------------------------- */
/* Get number of address spaces */
DWORD PCI_GetNumAddrSpaces(WDC_DEVICE_HANDLE hDev)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    /* Validate the device handle */
    if (!IsValidDevice(pDev, "PCI_GetNumAddrSpaces"))
        return 0;

    /* Return the number of address spaces for the device */
    return pDev->dwNumAddrSpaces;
}

/* Get address space information */
BOOL PCI_GetAddrSpaceInfo(WDC_DEVICE_HANDLE hDev,
    PCI_ADDR_SPACE_INFO *pAddrSpaceInfo)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    WDC_ADDR_DESC *pAddrDesc;
    DWORD dwAddrSpace;
    BOOL fIsMemory;

    dwAddrSpace = pAddrSpaceInfo->dwAddrSpace;

    if (dwAddrSpace > pDev->dwNumAddrSpaces - 1)
    {
        ErrLog("PCI_GetAddrSpaceInfo: Error - Address space %d is "
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
    vsnprintf(gsPCI_LastErr, sizeof(gsPCI_LastErr) - 1, sFormat, argp);
#ifdef DEBUG
    #ifdef __KERNEL__
        WDC_Err("KP PCI lib: %s", gsPCI_LastErr);
    #else
        WDC_Err("PCI lib: %s", gsPCI_LastErr);
    #endif
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
#ifdef __KERNEL__
    WDC_Trace("KP PCI lib: %s", sMsg);
#else
    WDC_Trace("PCI lib: %s", sMsg);
#endif
    va_end(argp);
#else
    UNUSED_VAR(sFormat);
#endif
}

/* Get last error */
const char *PCI_GetLastErr(void)
{
    return gsPCI_LastErr;
}

