/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/************************************************************************
*  File: driver_lib.c
*
*  Library for accessing PCI devices, possibly using a Kernel PlugIn driver.
*  The code accesses hardware using WinDriver's WDC library.
*  Code was generated by DriverWizard v15.1.1
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*************************************************************************/

#include "wdc_defs.h"
#include "utils.h"
#include "driver_lib.h"

/*************************************************************
  Internal definitions
 *************************************************************/
/* WinDriver license registration string */
/* TODO: When using a registered WinDriver version, replace the license string
         below with the development license in order to use on the development
         machine.
         Once you require to distribute the driver's package to other machines,
         please replace the string with a distribution license */
#define DRIVER_DEFAULT_LICENSE_STRING "12345abcde12345.abcde"

#define DRIVER_DEFAULT_DRIVER_NAME WD_DEFAULT_DRIVER_NAME_BASE

/* PCI device information struct */
typedef struct {
    
    
    DRIVER_INT_HANDLER   funcDiagIntHandler;   /* Interrupt handler routine */
    DRIVER_EVENT_HANDLER funcDiagEventHandler; /* Event handler routine */

} DRIVER_DEV_CTX, *PDRIVER_DEV_CTX;
/* TODO: You can add fields to store additional device-specific information. */

/*************************************************************
  Global variables definitions
 *************************************************************/
/* Last error information string */
static CHAR gsDRIVER_LastErr[256];

/* Library initialization reference count */
static DWORD LibInit_count = 0;

/*************************************************************
  Static functions prototypes and inline implementation
 *************************************************************/

#ifndef __KERNEL__
    static BOOL DeviceValidate(const PWDC_DEVICE pDev);
    static void DRIVER_EventHandler(WD_EVENT *pEvent, PVOID pData);
#endif
static void DLLCALLCONV DRIVER_IntHandler(PVOID pData);
static void ErrLog(const CHAR *sFormat, ...);
static void TraceLog(const CHAR *sFormat, ...);

/* Validate a device handle */
static inline BOOL IsValidDevice(PWDC_DEVICE pDev, const CHAR *sFunc)
{
    if (!pDev || !(PDRIVER_DEV_CTX)(pDev->pCtx))
    {
        snprintf(gsDRIVER_LastErr, sizeof(gsDRIVER_LastErr) - 1,
            "%s: NULL device %s\n", sFunc, !pDev ? "handle" : "context");
        ErrLog(gsDRIVER_LastErr);
        return FALSE;
    }

    return TRUE;
}

/*************************************************************
  Functions implementation
 *************************************************************/
/* -----------------------------------------------
    DRIVER and WDC libraries initialize/uninitialize
   ----------------------------------------------- */
/* Initialize the DRIVER and WDC libraries */
DWORD DRIVER_LibInit(void)
{
    DWORD dwStatus;

    /* Increase the library's reference count; initialize the library only once
     */
    if (++LibInit_count > 1)
        return WD_STATUS_SUCCESS;

#ifdef WD_DRIVER_NAME_CHANGE
    /* Set the driver name */
    if (!WD_DriverName(DRIVER_DEFAULT_DRIVER_NAME))
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
    dwStatus = WDC_DriverOpen(WDC_DRV_OPEN_DEFAULT, DRIVER_DEFAULT_LICENSE_STRING);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed to initialize the WDC library. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    return WD_STATUS_SUCCESS;
}

/* Uninitialize the DRIVER and WDC libraries */
DWORD DRIVER_LibUninit(void)
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
WDC_DEVICE_HANDLE DRIVER_DeviceOpen(DWORD dwVendorId, DWORD dwDeviceId)
{
    WDC_DEVICE_HANDLE hDev;

    hDev = WDC_DIAG_DeviceFindAndOpen(dwVendorId,
        dwDeviceId, KP_DRIVER_DRIVER_NAME, sizeof(DRIVER_DEV_CTX));

    if (!hDev || !DeviceValidate((PWDC_DEVICE)hDev))
        goto Error;

    return hDev;

Error:
    if (hDev)
        DRIVER_DeviceClose(hDev);

    ErrLog("DRIVER_DeviceOpen: Failed opening PCI device\n");

    return NULL;
}

/* Close device handle */
BOOL DRIVER_DeviceClose(WDC_DEVICE_HANDLE hDev)
{
    TraceLog("DRIVER_DeviceClose: Entered. Device handle 0x%p\n", hDev);

    /* Disable interrupts (if enabled) */
    if (WDC_IntIsEnabled(hDev))
    {
        DWORD dwStatus = DRIVER_IntDisable(hDev);
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
static void DLLCALLCONV DRIVER_IntHandler(PVOID pData)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PDRIVER_DEV_CTX pDevCtx = (PDRIVER_DEV_CTX)(pDev->pCtx);
    DRIVER_INT_RESULT intResult;

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
DWORD DRIVER_IntEnable(WDC_DEVICE_HANDLE hDev, DRIVER_INT_HANDLER funcIntHandler)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PDRIVER_DEV_CTX pDevCtx;

    
    

    TraceLog("DRIVER_IntEnable: Entered. Device handle 0x%p\n", hDev);

    /* Validate the device handle */
    if (!IsValidDevice(pDev, "DRIVER_IntEnable"))
        return WD_INVALID_PARAMETER;

    /* Verify that the device has an interrupt item */
    if (!IsItemExists(pDev, ITEM_INTERRUPT))
        return WD_OPERATION_FAILED;

    pDevCtx = (PDRIVER_DEV_CTX)(pDev->pCtx);

    /* Check whether interrupts are already enabled */
    if (WDC_IntIsEnabled(hDev))
    {
        ErrLog("Interrupts are already enabled ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Define the number of interrupt transfer commands to use */
    #define NUM_TRANS_CMDS 0
    /* NOTE: In order to correctly handle PCI interrupts, you need to
             ADD CODE HERE to set up transfer commands to read/write the
             relevant register(s) in order to correctly acknowledge the
             interrupts, as dictated by your hardware's specifications.
             When adding transfer commands, be sure to also modify the
             definition of NUM_TRANS_CMDS (above) accordingly. */

    /* Store the diag interrupt handler routine, which will be executed by
       DRIVER_IntHandler() when an interrupt is received */
    pDevCtx->funcDiagIntHandler = funcIntHandler;

    /* Enable the interrupts */
    /* NOTE: When adding read transfer commands, set the INTERRUPT_CMD_COPY flag
             in the 4th argument (dwOptions) passed to WDC_IntEnable() */
    dwStatus = WDC_IntEnable(hDev, NULL, 0, 0,
        DRIVER_IntHandler, (PVOID)pDev, WDC_IS_KP(hDev));

    if (WD_STATUS_SUCCESS != dwStatus)
    {
        ErrLog("Failed enabling interrupts. Error 0x%x - %s\n",
            dwStatus, Stat2Str(dwStatus));

        return dwStatus;
    }
        
    /* TODO: You can add code here to write to the device in order to
             physically enable the hardware interrupts. */

    TraceLog("DRIVER_IntEnable: Interrupts enabled\n");

    return WD_STATUS_SUCCESS;
}

/* Disable interrupts */
DWORD DRIVER_IntDisable(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PDRIVER_DEV_CTX pDevCtx;

    TraceLog("DRIVER_IntDisable entered. Device handle 0x%p\n", hDev);

    /* Validate the device handle */
    if (!IsValidDevice(pDev, "DRIVER_IntDisable"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PDRIVER_DEV_CTX)(pDev->pCtx);

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

    return dwStatus;
}

/* Check whether interrupts are enabled for the given device */
BOOL DRIVER_IntIsEnabled(WDC_DEVICE_HANDLE hDev)
{
    /* Validate the device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "DRIVER_IntIsEnabled"))
        return FALSE;

    /* Check whether interrupts are already enabled */
    return WDC_IntIsEnabled(hDev);
}


#if defined(LINUX)
/* -----------------------------------------------
   SRIOV handling
   ----------------------------------------------- */

DWORD DRIVER_SriovEnable(WDC_DEVICE_HANDLE hDev, DWORD dwNumVFs)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    TraceLog("DRIVER_SriovEnable entered. Device handle: 0x%p\n", hDev);

    /* Validate the device handle */
    if (!IsValidDevice(pDev, "DRIVER_SriovEnable"))
        return WD_INVALID_PARAMETER;

    return WDC_PciSriovEnable(hDev, dwNumVFs);

}

DWORD DRIVER_SriovDisable(WDC_DEVICE_HANDLE hDev)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    TraceLog("DRIVER_SriovDisable entered. Device handle: 0x%p\n", hDev);

    /* Validate the device handle */
    if (!IsValidDevice(pDev, "DRIVER_SriovDisable"))
        return WD_INVALID_PARAMETER;

    return WDC_PciSriovDisable(hDev);
}

DWORD DRIVER_SriovGetNumVFs(WDC_DEVICE_HANDLE hDev, DWORD* pdwNumVFs)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    TraceLog("DRIVER_SriovGetNumVFs entered. Device handle: 0x%p\n", hDev);
    if (pdwNumVFs)
        *pdwNumVFs = 0;

    /* Validate the device handle */
    if (!IsValidDevice(pDev, "DRIVER_SriovGetNumVfs"))
        return WD_INVALID_PARAMETER;

    return WDC_PciSriovGetNumVFs(hDev, pdwNumVFs);
}
#endif

/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */
/* Plug-and-play or power management event handler routine */
static void DRIVER_EventHandler(WD_EVENT *pEvent, PVOID pData)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
    PDRIVER_DEV_CTX pDevCtx = (PDRIVER_DEV_CTX)(pDev->pCtx);

    TraceLog("DRIVER_EventHandler entered, pData 0x%p, dwAction 0x%x\n", pData,
        pEvent->dwAction);

    /* Execute the diagnostics application's event handler function */
    pDevCtx->funcDiagEventHandler((WDC_DEVICE_HANDLE)pDev, pEvent->dwAction);
}

/* Register a plug-and-play or power management event */
DWORD DRIVER_EventRegister(WDC_DEVICE_HANDLE hDev,
    DRIVER_EVENT_HANDLER funcEventHandler)
{
    DWORD dwStatus;
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    PDRIVER_DEV_CTX pDevCtx;
    DWORD dwActions = WD_ACTIONS_ALL;
    /* TODO: Modify the above to set up the plug-and-play/power management
             events for which you wish to receive notifications.
             dwActions can be set to any combination of the WD_EVENT_ACTION
             flags defined in windrvr.h. */

    TraceLog("DRIVER_EventRegister entered. Device handle 0x%p\n", hDev);

    /* Validate the device handle */
    if (!IsValidDevice(pDev, "DRIVER_EventRegister"))
        return WD_INVALID_PARAMETER;

    pDevCtx = (PDRIVER_DEV_CTX)(pDev->pCtx);

    /* Check whether the event is already registered */
    if (WDC_EventIsRegistered(hDev))
    {
        ErrLog("Events are already registered ...\n");
        return WD_OPERATION_ALREADY_DONE;
    }

    /* Store the diag event handler routine to be executed from
     * DRIVER_EventHandler() upon an event */
    pDevCtx->funcDiagEventHandler = funcEventHandler;

    /* Register the event */
    dwStatus = WDC_EventRegister(hDev, dwActions, DRIVER_EventHandler, hDev,
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
DWORD DRIVER_EventUnregister(WDC_DEVICE_HANDLE hDev)
{
    DWORD dwStatus;

    TraceLog("DRIVER_EventUnregister entered. Device handle 0x%p\n", hDev);

    /* Validate the device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "DRIVER_EventUnregister"))
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
BOOL DRIVER_EventIsRegistered(WDC_DEVICE_HANDLE hDev)
{
    /* Validate the device handle */
    if (!IsValidDevice((PWDC_DEVICE)hDev, "DRIVER_EventIsRegistered"))
        return FALSE;

    /* Check whether the event is registered */
    return WDC_EventIsRegistered(hDev);
}

/* -----------------------------------------------
    DMA
   ----------------------------------------------- */
DWORD DRIVER_DmaAllocContig(MENU_CTX_DMA *pDmaMenusCtx)
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

DWORD DRIVER_DmaAllocSg(MENU_CTX_DMA *pDmaMenusCtx)
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

DWORD DRIVER_DmaAllocReserved(MENU_CTX_DMA *pDmaMenusCtx)
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

DWORD DRIVER_DmaBufUnlock(WD_DMA *pDma)
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


/* -----------------------------------------------
    Address spaces information
   ----------------------------------------------- */
/* Get number of address spaces */
DWORD DRIVER_GetNumAddrSpaces(WDC_DEVICE_HANDLE hDev)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

    /* Validate the device handle */
    if (!IsValidDevice(pDev, "DRIVER_GetNumAddrSpaces"))
        return 0;

    /* Return the number of address spaces for the device */
    return pDev->dwNumAddrSpaces;
}

/* Get address space information */
BOOL DRIVER_GetAddrSpaceInfo(WDC_DEVICE_HANDLE hDev,
    DRIVER_ADDR_SPACE_INFO *pAddrSpaceInfo)
{
    PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
    WDC_ADDR_DESC *pAddrDesc;
    DWORD dwAddrSpace;
    BOOL fIsMemory;

    if (!IsValidDevice(pDev, "DRIVER_GetAddrSpaceInfo"))
        return FALSE;

#if defined(DEBUG)
    if (!pAddrSpaceInfo)
    {
        ErrLog("DRIVER_GetAddrSpaceInfo: Error - NULL address space information pointer\n");
        return FALSE;
    }
#endif

    dwAddrSpace = pAddrSpaceInfo->dwAddrSpace;

    if (dwAddrSpace > pDev->dwNumAddrSpaces - 1)
    {
        ErrLog("DRIVER_GetAddrSpaceInfo: Error - Address space %d is "
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
    vsnprintf(gsDRIVER_LastErr, sizeof(gsDRIVER_LastErr) - 1, sFormat, argp);
#ifdef DEBUG
        WDC_Err("DRIVER lib: %s", gsDRIVER_LastErr);
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
    WDC_Trace("DRIVER lib: %s", sMsg);
    va_end(argp);
#else
    UNUSED_VAR(sFormat);
#endif
}

/* Get last error */
const char *DRIVER_GetLastErr(void)
{
    return gsDRIVER_LastErr;
}
