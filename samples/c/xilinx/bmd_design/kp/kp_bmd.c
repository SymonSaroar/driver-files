/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/***************************************************************************
*  File: kp_bmd.c
*
*  Sample Kernel PlugIn driver for accessing Xilinx PCI Express cards with
*  BMD design, using the WinDriver WDC API.
*  The sample was tested with Xilinx's Virtex and Spartan development kits.
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
****************************************************************************/

#include "kpstdlib.h"
#include "wd_kp.h"
#include "utils.h"
#include "wdc_defs.h"
#include "status_strings.h"
#include "pci_regs.h"
#include "bits.h"
#include "../bmd_lib.h"

/*************************************************************
  Functions prototypes
 *************************************************************/
BOOL __cdecl KP_BMD_Open(KP_OPEN_CALL *kpOpenCall, HANDLE hWD, PVOID pOpenData,
    PVOID *ppDrvContext);
BOOL __cdecl KP_BMD_Open_32_64(KP_OPEN_CALL *kpOpenCall, HANDLE hWD,
    PVOID pOpenData, PVOID *ppDrvContext);
void __cdecl KP_BMD_Close(PVOID pDrvContext);
void __cdecl KP_BMD_Call(PVOID pDrvContext, WD_KERNEL_PLUGIN_CALL *kpCall);
BOOL __cdecl KP_BMD_IntEnable(PVOID pDrvContext, WD_KERNEL_PLUGIN_CALL *kpCall,
    PVOID *ppIntContext);
void __cdecl KP_BMD_IntDisable(PVOID pIntContext);
BOOL __cdecl KP_BMD_IntAtIrql(PVOID pIntContext, BOOL *pfIsMyInterrupt);
DWORD __cdecl KP_BMD_IntAtDpc(PVOID pIntContext, DWORD dwCount);
BOOL __cdecl KP_BMD_IntAtIrqlMSI(PVOID pIntContext, ULONG dwLastMessage,
    DWORD dwReserved);
DWORD __cdecl KP_BMD_IntAtDpcMSI(PVOID pIntContext, DWORD dwCount,
    ULONG dwLastMessage, DWORD dwReserved);
BOOL __cdecl KP_BMD_Event(PVOID pDrvContext, WD_EVENT *wd_event);
static void KP_BMD_Err(const CHAR *sFormat, ...);
static void KP_BMD_Trace(const CHAR *sFormat, ...);

#define PTR32 UINT32

typedef struct {
    UINT32 dwNumAddrSpaces; /* Total number of device address spaces */
    PTR32  pAddrDesc;       /* Array of device address spaces information */
} BMD_DEV_ADDR_DESC_32B;

/*************************************************************
  Functions implementation
 *************************************************************/

/* KP_Init is called when the Kernel PlugIn driver is loaded.
   This function sets the name of the Kernel PlugIn driver and the driver's
   open callback function. */
BOOL __cdecl KP_Init(KP_INIT *kpInit)
{
    /* Verify that the version of the WinDriver Kernel PlugIn library
       is identical to that of the windrvr.h and wd_kp.h files */
    if (WD_VER != kpInit->dwVerWD)
    {
        /* Re-build your Kernel PlugIn driver project with the compatible
           version of the WinDriver Kernel PlugIn library (kp_nt<version>.lib)
           and windrvr.h and wd_kp.h files */

        return FALSE;
    }

    kpInit->funcOpen = KP_BMD_Open;
    kpInit->funcOpen_32_64 = KP_BMD_Open_32_64;
    strcpy(kpInit->cDriverName, KP_BMD_DRIVER_NAME);

    return TRUE;
}

/* KP_BMD_Open is called when WD_KernelPlugInOpen() is called from
   the user mode.
   pDrvContext will be passed to rest of the Kernel PlugIn  callback
   functions. */
BOOL __cdecl KP_BMD_Open(KP_OPEN_CALL *kpOpenCall, HANDLE hWD, PVOID pOpenData,
    PVOID *ppDrvContext)
{
    BMD_DEV_ADDR_DESC *pDevAddrDesc;
    WDC_ADDR_DESC *pAddrDesc;
    DWORD dwSize;
    DWORD dwStatus;

    /* Initialize the BMD library */
    dwStatus = BMD_LibInit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        KP_BMD_Err("KP_BMD_Open: Failed to initialize the BMD library: %s",
            BMD_GetLastErr());
        return FALSE;
    }

    KP_BMD_Trace("KP_BMD_Open entered. BMD library initialized.\n");

    kpOpenCall->funcClose = KP_BMD_Close;
    kpOpenCall->funcCall = KP_BMD_Call;
    kpOpenCall->funcIntEnable = KP_BMD_IntEnable;
    kpOpenCall->funcIntDisable = KP_BMD_IntDisable;
    kpOpenCall->funcIntAtIrql = KP_BMD_IntAtIrql;
    kpOpenCall->funcIntAtDpc = KP_BMD_IntAtDpc;
    kpOpenCall->funcIntAtIrqlMSI = KP_BMD_IntAtIrqlMSI;
    kpOpenCall->funcIntAtDpcMSI = KP_BMD_IntAtDpcMSI;
    kpOpenCall->funcEvent = KP_BMD_Event;

    /* Create a copy of device information in the driver context */
    dwSize = sizeof(BMD_DEV_ADDR_DESC);
    pDevAddrDesc = malloc(dwSize);
    if (!pDevAddrDesc)
        goto malloc_error;

    COPY_FROM_USER(pDevAddrDesc, pOpenData, dwSize);

    dwSize = sizeof(WDC_ADDR_DESC) * pDevAddrDesc->dwNumAddrSpaces;
    pAddrDesc = malloc(dwSize);
    if (!pAddrDesc)
        goto malloc_error;

    COPY_FROM_USER(pAddrDesc, pDevAddrDesc->pAddrDesc, dwSize);
    pDevAddrDesc->pAddrDesc = pAddrDesc;

    *ppDrvContext = pDevAddrDesc;

    KP_BMD_Trace("KP_BMD_Open: Kernel PlugIn driver opened successfully\n");

    return TRUE;

malloc_error:
    KP_BMD_Err("KP_BMD_Open: Failed allocating %ld bytes\n", dwSize);
    if (pDevAddrDesc)
        free(pDevAddrDesc);
    BMD_LibUninit();
    return FALSE;
}

/* KP_BMD_Open_32_64 is called when WD_KernelPlugInOpen() is called from a
   32-bit user mode application to open a handle to a 64-bit Kernel PlugIn.
   pDrvContext will be passed to the rest of the Kernel PlugIn callback
   functions. */
BOOL __cdecl KP_BMD_Open_32_64(KP_OPEN_CALL *kpOpenCall, HANDLE hWD,
    PVOID pOpenData, PVOID *ppDrvContext)
{
    BMD_DEV_ADDR_DESC *pDevAddrDesc;
    BMD_DEV_ADDR_DESC_32B devAddrDesc_32;
    WDC_ADDR_DESC *pAddrDesc;
    DWORD dwSize;
    DWORD dwStatus;

    /* Initialize the BMD library */
    dwStatus = BMD_LibInit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        KP_BMD_Err("KP_BMD_Open_32_64: Failed to initialize the BMD library: %s",
            BMD_GetLastErr());
        return FALSE;
    }

    KP_BMD_Trace("KP_BMD_Open_32_64 entered. BMD library initialized.\n");

    kpOpenCall->funcClose = KP_BMD_Close;
    kpOpenCall->funcCall = KP_BMD_Call;
    kpOpenCall->funcIntEnable = KP_BMD_IntEnable;
    kpOpenCall->funcIntDisable = KP_BMD_IntDisable;
    kpOpenCall->funcIntAtIrql = KP_BMD_IntAtIrql;
    kpOpenCall->funcIntAtDpc = KP_BMD_IntAtDpc;
    kpOpenCall->funcIntAtIrqlMSI = KP_BMD_IntAtIrqlMSI;
    kpOpenCall->funcIntAtDpcMSI = KP_BMD_IntAtDpcMSI;
    kpOpenCall->funcEvent = KP_BMD_Event;

    /* Copy device information that sent from a 32-bit user application */
    COPY_FROM_USER(&devAddrDesc_32, pOpenData, sizeof(BMD_DEV_ADDR_DESC_32B));

    /* Create a copy of the device information in the driver context */
    dwSize = sizeof(BMD_DEV_ADDR_DESC);
    pDevAddrDesc = malloc(dwSize);
    if (!pDevAddrDesc)
        goto malloc_error;

    /* Copy the 32-bit data to a 64-bit struct */
    pDevAddrDesc->dwNumAddrSpaces = devAddrDesc_32.dwNumAddrSpaces;

    dwSize = sizeof(WDC_ADDR_DESC) * pDevAddrDesc->dwNumAddrSpaces;
    pAddrDesc = malloc(dwSize);
    if (!pAddrDesc)
        goto malloc_error;

    COPY_FROM_USER(pAddrDesc, (PVOID)(KPTR)devAddrDesc_32.pAddrDesc, dwSize);
    pDevAddrDesc->pAddrDesc = pAddrDesc;

    *ppDrvContext = pDevAddrDesc;

    KP_BMD_Trace("KP_BMD_Open_32_64: Kernel PlugIn driver opened successfully\n");

    return TRUE;

malloc_error:
    KP_BMD_Err("KP_BMD_Open_32_64: Failed allocating %ld bytes\n", dwSize);
    if (pDevAddrDesc)
        free(pDevAddrDesc);
    BMD_LibUninit();
    return FALSE;
}

/* KP_BMD_Close is called when WD_KernelPlugInClose() is called from the
   user mode */
void __cdecl KP_BMD_Close(PVOID pDrvContext)
{
    DWORD dwStatus;

    KP_BMD_Trace("KP_BMD_Close entered\n");

    /* Uninit the BMD library */
    dwStatus = BMD_LibUninit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        KP_BMD_Err("KP_BMD_Close: Failed to uninit the BMD library: %s",
            BMD_GetLastErr());
    }

    /* Free the memory allocated for the driver context */
    if (pDrvContext)
    {
        if (((BMD_DEV_ADDR_DESC *)pDrvContext)->pAddrDesc)
            free(((BMD_DEV_ADDR_DESC *)pDrvContext)->pAddrDesc);
        free(pDrvContext);
    }
}

/* KP_BMD_Call is called when WD_KernelPlugInCall() is called from the
   user mode */
void __cdecl KP_BMD_Call(PVOID pDrvContext, WD_KERNEL_PLUGIN_CALL *kpCall)
{
    KP_BMD_Trace("KP_BMD_Call: Entered. Message [0x%lx]\n", kpCall->dwMessage);

    kpCall->dwResult = KP_BMD_STATUS_OK;

    switch (kpCall->dwMessage)
    {
    case KP_BMD_MSG_VERSION:  /* Get the version of the Kernel PlugIn driver */
        {
            KP_BMD_VERSION *pUserKPVer = (KP_BMD_VERSION *)(kpCall->pData);
            KP_BMD_VERSION kernelKPVer;

            BZERO(kernelKPVer);
            kernelKPVer.dwVer = 100;
#define DRIVER_VER_STR "My Driver V1.00"
            memcpy(kernelKPVer.cVer, DRIVER_VER_STR, sizeof(DRIVER_VER_STR));
            COPY_TO_USER(pUserKPVer, &kernelKPVer, sizeof(KP_BMD_VERSION));
            kpCall->dwResult = KP_BMD_STATUS_OK;
        }
        break;
    default:
        kpCall->dwResult = KP_BMD_STATUS_MSG_NO_IMPL;
    }

    /* NOTE: You can modify the messages above and/or add your own
             Kernel PlugIn messages.
             When changing/adding messages, be sure to also update the
             messages definitions in ../bmd_lib.h. */
}

/* KP_BMD_IntEnable is called when WD_IntEnable() is called from the user
   mode with a Kernel PlugIn handle. The interrupt context (pIntContext) will
   be passed to the rest of the Kernel PlugIn interrupt functions.
   The function returns TRUE if interrupts are enabled successfully. */
BOOL __cdecl KP_BMD_IntEnable(PVOID pDrvContext, WD_KERNEL_PLUGIN_CALL *kpCall,
    PVOID *ppIntContext)
{
    KP_BMD_Trace("KP_BMD_IntEnable: Entered\n");

    /* You can allocate specific memory for each interrupt in *ppIntContext */

    /* In this sample we will set the interrupt context to the driver context,
       which has been set in KP_BMD_Open to hold the device information. */
    *ppIntContext = pDrvContext;

    /* TODO: You can add code here to write to the device in order
             to physically enable the hardware interrupts */

    return TRUE;
}

/* KP_BMD_IntDisable is called when WD_IntDisable() is called from the user
   mode with a Kernel PlugIn handle */
void __cdecl KP_BMD_IntDisable(PVOID pIntContext)
{
    /* Free any memory allocated in KP_BMD_IntEnable() here */
}

/* KP_BMD_IntAtIrql returns TRUE if deferred interrupt processing (DPC) for
   level-sensitive interrupt is required.
   The function is called at HIGH IRQL - at physical interrupt handler.
   Most library calls are NOT allowed at this level, for example:
   NO   WDC_xxx() or WD_xxx calls, apart from the WDC read/write address or
        register functions, WDC_MultiTransfer(), WD_Transfer(),
        WD_MultiTransfer() or WD_DebugAdd().
   NO   malloc().
   NO   free().
   YES  WDC read/write address or configuration space functions,
        WDC_MultiTransfer(), WD_Transfer(), WD_MultiTransfer() or
        WD_DebugAdd(), or wrapper functions that call these functions.
   YES  specific kernel OS functions (such as WinDDK functions) that can
        be called from HIGH IRQL. [Note that the use of such functions may
        break the code's portability to other OSs.] */
BOOL __cdecl KP_BMD_IntAtIrql(PVOID pIntContext, BOOL *pfIsMyInterrupt)
{
    static DWORD dwIntCount = 0; /* Interrupts count */
    BMD_DEV_ADDR_DESC *pDevAddrDesc = (BMD_DEV_ADDR_DESC *)pIntContext;
    /* NOTE: To correctly handle level-sensitive PCI interrupts, you
             need to ADD CODE HERE to read/write the
             relevant register(s) in order to correctly acknowledge the interrupts,
             as dictated by your hardware's specifications. */

    /* If the data read from the hardware indicates that the interrupt belongs
       to you, you must set *pfIsMyInterrupt to TRUE;
       otherwise, set it to FALSE (this will allow ISRs of other drivers to be
       invoked). */
    *pfIsMyInterrupt = FALSE;

    /* This sample schedules a DPC once in every 5 interrupts.
       TODO: You can modify the implementation to schedule the DPC as needed. */
    dwIntCount++;
    if (!(dwIntCount % 5))
        return TRUE;

    return FALSE;
}

/* KP_BMD_IntAtDpc is a Deferred Procedure Call for additional
   level-sensitive interrupt processing. This function is called if
   KP_BMD_IntAtIrql returned TRUE. KP_BMD_IntAtDpc returns the number of
   times to notify the user mode of the interrupt (i.e., return from
   WD_IntWait).
 */
DWORD __cdecl KP_BMD_IntAtDpc(PVOID pIntContext, DWORD dwCount)
{
    return dwCount;
}

/* KP_BMD_IntAtIrqlMSI returns TRUE if deferred interrupt processing (DPC)
   for Message-Signaled Interrupts (MSI) or Extended Message-Signaled Interrupts
   (MSI-X) is required.
   The function is called at HIGH IRQL - at physical interrupt handler.
   Note: Do not use the dwReserved parameter.
   Most library calls are NOT allowed at this level, for example:
   NO   WDC_xxx() or WD_xxx calls, apart from the WDC read/write address or
        register functions, WDC_MultiTransfer(), WD_Transfer(),
        WD_MultiTransfer() or WD_DebugAdd().
   NO   malloc().
   NO   free().
   YES  WDC read/write address or configuration space functions,
        WDC_MultiTransfer(), WD_Transfer(), WD_MultiTransfer() or
        WD_DebugAdd(), or wrapper functions that call these functions.
   YES  specific kernel OS functions (such as WinDDK functions) that can
        be called from HIGH IRQL. [Note that the use of such functions may
        break the code's portability to other OSs.] */
BOOL __cdecl KP_BMD_IntAtIrqlMSI(PVOID pIntContext, ULONG dwLastMessage,
    DWORD dwReserved)
{
    static DWORD dwIntCount = 0; /* Interrupts count */

    /* There is no need to acknowledge MSI/MSI-X. However, you can implement
     the same functionality here as done in the KP_BMD_IntAtIrql handler
     to read/write data from/to registers at HIGH IRQL. */

    /* This sample schedules a DPC once in every 5 interrupts.
       TODO: You can modify the implementation to schedule the DPC as needed. */
    dwIntCount++;
    if (!(dwIntCount % 5))
        return TRUE;

    return FALSE;
}

/* KP_BMD_IntAtDpcMSI is a Deferred Procedure Call for additional
   Message-Signaled Interrupts (MSI) or Extended Message-Signaled Interrupts
   (MSI-X) processing.
   This function is called if KP_BMD_IntAtIrqlMSI returned TRUE.
   KP_BMD_IntAtDpcMSI returns the number of times to notify the user mode of
   the interrupt (i.e. return from WD_IntWait). */
DWORD __cdecl KP_BMD_IntAtDpcMSI(PVOID pIntContext, DWORD dwCount,
    ULONG dwLastMessage, DWORD dwReserved)
{
    return dwCount;
}

/* KP_BMD_Event is called when a Plug-and-Play/power management event for
   the device is received, if EventRegister() was first called from the
   user mode with the Kernel PlugIn handle. */
BOOL __cdecl KP_BMD_Event(PVOID pDrvContext, WD_EVENT *wd_event)
{
    return TRUE; /* Return TRUE to notify the user mode of the event */
}

/* -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- */
static void KP_BMD_Err(const CHAR *sFormat, ...)
{
#if defined(DEBUG)
    CHAR sMsg[256];
    va_list argp;

    va_start(argp, sFormat);
    vsnprintf(sMsg, sizeof(sMsg) - 1, sFormat, argp);
    WDC_Err("%s: %s", KP_BMD_DRIVER_NAME, sMsg);
    va_end(argp);
#endif
}

static void KP_BMD_Trace(const CHAR *sFormat, ...)
{
#if defined(DEBUG)
    CHAR sMsg[256];
    va_list argp;

    va_start(argp, sFormat);
    vsnprintf(sMsg, sizeof(sMsg) - 1, sFormat, argp);
    WDC_Trace("%s: %s", KP_BMD_DRIVER_NAME, sMsg);
    va_end(argp);
#endif
}

