/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/************************************************************************
*  File: kp_pci.c
*
*  Kernel PlugIn driver for accessing PCI devices.
*  The code accesses hardware using WinDriver's WDC library.

*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*************************************************************************/

#include "kpstdlib.h"
#include "wd_kp.h"
#include "pci_regs.h"
#include "../pci_lib.h"

/*************************************************************
  Functions prototypes
 *************************************************************/
BOOL __cdecl KP_PCI_Open(KP_OPEN_CALL *kpOpenCall, HANDLE hWD, PVOID pOpenData,
    PVOID *ppDrvContext);
BOOL __cdecl KP_PCI_Open_32_64(KP_OPEN_CALL *kpOpenCall, HANDLE hWD,
    PVOID pOpenData, PVOID *ppDrvContext);
void __cdecl KP_PCI_Close(PVOID pDrvContext);
void __cdecl KP_PCI_Call(PVOID pDrvContext, WD_KERNEL_PLUGIN_CALL *kpCall);
BOOL __cdecl KP_PCI_IntEnable(PVOID pDrvContext, WD_KERNEL_PLUGIN_CALL *kpCall,
    PVOID *ppIntContext);
void __cdecl KP_PCI_IntDisable(PVOID pIntContext);
BOOL __cdecl KP_PCI_IntAtIrql(PVOID pIntContext, BOOL *pfIsMyInterrupt);
DWORD __cdecl KP_PCI_IntAtDpc(PVOID pIntContext, DWORD dwCount);
BOOL __cdecl KP_PCI_IntAtIrqlMSI(PVOID pIntContext, ULONG dwLastMessage,
    DWORD dwReserved);
DWORD __cdecl KP_PCI_IntAtDpcMSI(PVOID pIntContext, DWORD dwCount,
    ULONG dwLastMessage, DWORD dwReserved);
BOOL __cdecl KP_PCI_Event(PVOID pDrvContext, WD_EVENT *wd_event);
static void KP_PCI_Err(const CHAR *sFormat, ...);
static void KP_PCI_Trace(const CHAR *sFormat, ...);
#define PTR32 UINT32

typedef struct {
    UINT32 dwNumAddrSpaces; /* Total number of device address spaces */
    PTR32  pAddrDesc;       /* Array of device address spaces information */
} PCI_DEV_ADDR_DESC_32B;

/*************************************************************
  Functions implementation
 *************************************************************/

/** KP_Init is called when the Kernel PlugIn driver is loaded.
   This function sets the name of the Kernel PlugIn driver and the driver's
   open callback function(s).
   @param [out] kpInit: Pointer to a pre-allocated Kernel PlugIn initialization
   information structure, whose fields should be updated by the function
   @return TRUE if successful. Otherwise FALSE.
   */
BOOL __cdecl KP_Init(KP_INIT *kpInit)
{
    /* Verify that the version of the WinDriver Kernel PlugIn library
       is identical to that of the windrvr.h and wd_kp.h files */
    if (WD_VER != kpInit->dwVerWD)
    {
        /* Rebuild your Kernel PlugIn driver project with the compatible
           version of the WinDriver Kernel PlugIn library (kp_nt<version>.lib)
           and windrvr.h and wd_kp.h files */

        return FALSE;
    }

    kpInit->funcOpen = KP_PCI_Open;
    kpInit->funcOpen_32_64 = KP_PCI_Open_32_64;
#if defined(WINNT)
    strcpy(kpInit->cDriverName, KP_PCI_DRIVER_NAME);
#else
    strncpy(kpInit->cDriverName, KP_PCI_DRIVER_NAME,
        sizeof(kpInit->cDriverName));
#endif
    kpInit->cDriverName[sizeof(kpInit->cDriverName) - 1] = 0;

    return TRUE;
}

/**
* Kernel PlugIn open function.
* This function sets the rest of the Kernel PlugIn callback functions
* (KP_Call, KP_IntEnable, etc.) and performs any other desired
* initialization (such as allocating memory for the driver context and
* filling it with data passed from the user mode).
* The returned driver context (*ppDrvContext) will be passed to rest of the
* Kernel PlugIn callback functions.
*
* The KP_Open callback is called when the WD_KernelPlugInOpen() function
* (see the WinDriver PCI Low-\Level API Reference) is called from the user
* mode — either directly (when using the low-level WinDriver API), or
* via a call to a high-level WDC function.
* WD_KernelPlugInOpen() is called from the WDC_KernelPlugInOpen(), and from
* the WDC_PciDeviceOpen() / WDC_IsaDeviceOpen() functions when they are
* called with the name of a valid Kernel PlugIn driver (set in the
* pcKPDriverName parameter).
*
*[Note]
* The WDC_xxxDeviceOpen() functions cannot be used to open a handle to a
* 64-bit Kernel PlugIn function from a 32-bit application. For this purpose,
* use WDC_KernelPlugInOpen() (or the low-level WD_KernelPlugInOpen()
* function).
* The Kernel PlugIn driver can implement two types of KP_Open() callback
* functions —
* A "standard" Kernel PlugIn open function, which is used whenever a
* user-mode application opens a handle to a Kernel PlugIn driver, except
* when a 32-bit applications opens a handle to a 64-bit driver.
* This callback function is set in the funcOpen field of the KP_INIT
* structure that is passed as a parameter to KP_Init().
* A function that will be used when a 32-bit user-mode application opens a
* handle to a 64-bit Kernel PlugIn driver.
* This callback function is set in the funcOpen_32_64 field of the KP_INIT
* structure that is passed as a parameter to KP_Init.
* A Kernel PlugIn driver can provide either one or both of these KP_Open()
* callbacks, depending on the target configuration(s).
*
*[Note]
* The KP_PCI sample (WinDriver/samples/pci_diag/kp_pci/kp_pci.c) implements
* both types of KP_Open callbacks — KP_PCI_Open() (standard) and
* KP_PCI_Open_32_64() (for opening a handle to a 64-bit Kernel PlugIn from a
* 32-bit application).
* The generated DriverWizard Kernel PlugIn code always implements a standard
* Kernel PlugIn open function — KP_XXX_Open(). When selecting the 32-bit
* application for a 64-bit Kernel PlugIn DriverWizard code-generation
* option, the wizard also implements a KP_XXX_Open_32_64() function, for
* opening a handle to a 64-bit Kernel PlugIn driver from a 32-bit application.
*
* @param [in] kpOpenCall: Structure to fill in the addresses of the
*     KP_xxx callback functions
* @param [in] hWD: The WinDriver handle that WD_KernelPlugInOpen() was
*     called with
* @param [in] pOpenData: Pointer to data passed from user mode
* @param [out] ppDrvContext: Pointer to driver context data with which
*     the KP_Close(), KP_Call(), KP_IntEnable() and KP_Event() functions
*     will be called. Use this to keep driver-specific information that will
*     be shared among these callbacks.
*
* @return TRUE if successful. If FALSE, the call to
*     WD_KernelPlugInOpen() from the user mode will fail
**/
BOOL __cdecl KP_PCI_Open(KP_OPEN_CALL *kpOpenCall, HANDLE hWD, PVOID pOpenData,
    PVOID *ppDrvContext)
{
    PCI_DEV_ADDR_DESC *pDevAddrDesc;
    DWORD dwSize;
    DWORD dwStatus;

    /* Initialize the PCI library */
    dwStatus = PCI_LibInit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        KP_PCI_Err("KP_PCI_Open: Failed to initialize the PCI library. "
            "Last error [%s]\n", PCI_GetLastErr());
        return FALSE;
    }

    KP_PCI_Trace("KP_PCI_Open: Entered. PCI library initialized\n");

    kpOpenCall->funcClose = KP_PCI_Close;
    kpOpenCall->funcCall = KP_PCI_Call;
    kpOpenCall->funcIntEnable = KP_PCI_IntEnable;
    kpOpenCall->funcIntDisable = KP_PCI_IntDisable;
    kpOpenCall->funcIntAtIrql = KP_PCI_IntAtIrql;
    kpOpenCall->funcIntAtDpc = KP_PCI_IntAtDpc;
    kpOpenCall->funcIntAtIrqlMSI = KP_PCI_IntAtIrqlMSI;
    kpOpenCall->funcIntAtDpcMSI = KP_PCI_IntAtDpcMSI;
    kpOpenCall->funcEvent = KP_PCI_Event;

    if (ppDrvContext)
    {
        if (pOpenData)
        {
            WDC_ADDR_DESC *pAddrDesc;

            /* Create a copy of device information in the driver context */
            dwSize = sizeof(PCI_DEV_ADDR_DESC);
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
        }
        else
        {
            *ppDrvContext = NULL;
        }
    }

    KP_PCI_Trace("KP_PCI_Open: Kernel PlugIn driver opened successfully\n");

    return TRUE;

malloc_error:
    KP_PCI_Err("KP_PCI_Open: Failed allocating [%ld] bytes\n", dwSize);
    if (pDevAddrDesc)
        free(pDevAddrDesc);
    PCI_LibUninit();
    return FALSE;
}


/** KP_PCI_Open_32_64 is called when WD_KernelPlugInOpen() is called from a
   32-bit user mode application to open a handle to a 64-bit Kernel PlugIn.
   pDrvContext will be passed to the rest of the Kernel PlugIn callback
   functions.
   See KP_PCI_Open() for more info.
   */
BOOL __cdecl KP_PCI_Open_32_64(KP_OPEN_CALL *kpOpenCall, HANDLE hWD,
    PVOID pOpenData, PVOID *ppDrvContext)
{
    PCI_DEV_ADDR_DESC *pDevAddrDesc;
    DWORD dwSize;
    DWORD dwStatus;

    /* Initialize the PCI library */
    dwStatus = PCI_LibInit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        KP_PCI_Err("KP_PCI_Open_32_64: Failed to initialize the PCI library. "
            "Last error [%s]\n", PCI_GetLastErr());
        return FALSE;
    }

    KP_PCI_Trace("KP_PCI_Open_32_64: Entered. PCI library initialized\n");

    kpOpenCall->funcClose = KP_PCI_Close;
    kpOpenCall->funcCall = KP_PCI_Call;
    kpOpenCall->funcIntEnable = KP_PCI_IntEnable;
    kpOpenCall->funcIntDisable = KP_PCI_IntDisable;
    kpOpenCall->funcIntAtIrql = KP_PCI_IntAtIrql;
    kpOpenCall->funcIntAtDpc = KP_PCI_IntAtDpc;
    kpOpenCall->funcIntAtIrqlMSI = KP_PCI_IntAtIrqlMSI;
    kpOpenCall->funcIntAtDpcMSI = KP_PCI_IntAtDpcMSI;
    kpOpenCall->funcEvent = KP_PCI_Event;

    if (ppDrvContext)
    {
        if (pOpenData)
        {
            PCI_DEV_ADDR_DESC_32B devAddrDesc_32;
            WDC_ADDR_DESC *pAddrDesc;

            /* Create a copy of the device information in the driver context */
            dwSize = sizeof(PCI_DEV_ADDR_DESC);
            pDevAddrDesc = malloc(dwSize);
            if (!pDevAddrDesc)
                goto malloc_error;

            /* Copy device information sent from a 32-bit user application */
            COPY_FROM_USER(&devAddrDesc_32, pOpenData,
                sizeof(PCI_DEV_ADDR_DESC_32B));

            /* Copy the 32-bit data to a 64-bit struct */
            pDevAddrDesc->dwNumAddrSpaces = devAddrDesc_32.dwNumAddrSpaces;
            dwSize = sizeof(WDC_ADDR_DESC) * pDevAddrDesc->dwNumAddrSpaces;
            pAddrDesc = malloc(dwSize);
            if (!pAddrDesc)
                goto malloc_error;

            COPY_FROM_USER(pAddrDesc, (PVOID)(KPTR)devAddrDesc_32.pAddrDesc,
                dwSize);
            pDevAddrDesc->pAddrDesc = pAddrDesc;

            *ppDrvContext = pDevAddrDesc;
        }
        else
        {
            *ppDrvContext = NULL;
        }
    }

    KP_PCI_Trace("KP_PCI_Open_32_64: Kernel PlugIn driver opened "
        "successfully\n");

    return TRUE;

malloc_error:
    KP_PCI_Err("KP_PCI_Open_32_64: Failed allocating [%ld] bytes\n", dwSize);
    if (pDevAddrDesc)
        free(pDevAddrDesc);
    PCI_LibUninit();
    return FALSE;
}

/** Called when WD_KernelPlugInClose() (see the WinDriver PCI Low-Level
* API Reference) is called from user mode.
*
* The high-level WDC_PciDeviceClose() / WDC_IsaDeviceOpen() functions
* automatically call WD_KernelPlugInClose() for devices that contain an
* open Kernel PlugIn handle.
*
* This functions can be used to perform any required clean-up for the
* Kernel PlugIn (such as freeing memory previously allocated for the driver
* context, etc.).
* @param [in] pDrvContext: Driver context data that was set by KP_Open()
*/
void __cdecl KP_PCI_Close(PVOID pDrvContext)
{
    DWORD dwStatus;

    KP_PCI_Trace("KP_PCI_Close: Entered\n");

    /* Uninit the PCI library */
    dwStatus = PCI_LibUninit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        KP_PCI_Err("KP_PCI_Close: Failed to uninit the PCI library. "
            "Last error [%s]\n", PCI_GetLastErr());
    }

    /* Free the memory allocated for the driver context */
    if (pDrvContext)
    {
        if (((PCI_DEV_ADDR_DESC *)pDrvContext)->pAddrDesc)
            free(((PCI_DEV_ADDR_DESC *)pDrvContext)->pAddrDesc);
        free(pDrvContext);
    }
}

/** Called when the user-mode application calls WDC_CallKerPlug() (or the
* low-level WD_KernelPlugInCall() function — see the WinDriver PCI
* Low-Level API Reference).
*
* This function is a message handler for your utility functions.
* @param [inout] pDrvContext: Driver context data that was set by
* KP_Open() and will also be passed to KP_Close(), KP_IntEnable() and
* KP_Event()
* @param [inout] kpCall: Structure with user-mode information received
*     from the WDC_CallKerPlug() (or from the low-level WD_KernelPlugInCall()
*     function — see the WinDriver PCI Low-Level API Reference) and/or with
*     information to return back to the user mode.
* @param [in] kpCall->dwMessage: Message number for the handler.
* @param [inout] kpCall->pData: Message context.
* @param [out] kpCall->dwResult: Message result to send back to the user
*     application.
* @remark Calling WDC_CallKerPlug()(or the low-level WD_KernelPlugInCall()
*     function — see the WinDriver PCI Low-Level API
*     Reference) in the user mode will call your KP_Call() callback
*     function in the kernel mode. The KP_Call() function in the Kernel
*     PlugIn will determine which routine to execute according to the
*     message passed to it.
*     The fIsKernelMode parameter is passed by the WinDriver kernel to the
*     KP_Call routine. The user is not required to do anything about this
*     parameter. However, notice how this parameter is passed in the
*     sample code to the macro COPY_TO_USER_OR_KERNEL — This is required
*     for the macro to function correctly.
*/
void __cdecl KP_PCI_Call(PVOID pDrvContext, WD_KERNEL_PLUGIN_CALL *kpCall)
{
    KP_PCI_Trace("KP_PCI_Call: Entered. Message [0x%lx]\n", kpCall->dwMessage);

    kpCall->dwResult = KP_PCI_STATUS_OK;

    switch (kpCall->dwMessage)
    {
    case KP_PCI_MSG_VERSION: /* Get the version of the Kernel PlugIn driver */
        {
            KP_PCI_VERSION *pUserKPVer = (KP_PCI_VERSION *)(kpCall->pData);
            KP_PCI_VERSION kernelKPVer;

            BZERO(kernelKPVer);
            kernelKPVer.dwVer = 100;
#define DRIVER_VER_STR "My Driver V1.00"
            memcpy(kernelKPVer.cVer, DRIVER_VER_STR, sizeof(DRIVER_VER_STR));
            COPY_TO_USER(pUserKPVer, &kernelKPVer, sizeof(KP_PCI_VERSION));
            kpCall->dwResult = KP_PCI_STATUS_OK;
        }
        break;

    default:
        kpCall->dwResult = KP_PCI_STATUS_MSG_NO_IMPL;
    }

    /* NOTE: You can modify the messages above and/or add your own
             Kernel PlugIn messages.
             When changing/adding messages, be sure to also update the
             messages definitions in ../pci_lib.h. */
}
/** Called when WDC_IntEnable() / WD_IntEnable() is
* called from the user mode with a Kernel PlugIn handle.
* WD_IntEnable() is called automatically from WDC_IntEnable() and
* InterruptEnable() (see WinDriver PCI Low-Level API Reference).
*
* The interrupt context that is set by this function (*ppIntContext) will be
* passed to the rest of the Kernel PlugIn interrupt functions.
*
* @param [inout] pDrvContext: Driver context data that was set by
* KP_Open() and will also be passed to KP_Close(), KP_IntEnable() and
* KP_Event()
* @param [inout] kpCall: Structure with information from WD_IntEnable()
* @param [inout] ppIntContext: Pointer to interrupt context data that will
*     be passed to KP_IntDisable() and to the Kernel PlugIn interrupt
*     handler functions. Use this context to keep interrupt specific
*     information.
* @return Returns TRUE if enable is successful; otherwise returns FALSE.
* @remark This function should contain any initialization needed for your
*     Kernel PlugIn interrupt handling.
*/
BOOL __cdecl KP_PCI_IntEnable(PVOID pDrvContext, WD_KERNEL_PLUGIN_CALL *kpCall,
    PVOID *ppIntContext)
{
    KP_PCI_Trace("KP_PCI_IntEnable: Entered\n");

    /* You can allocate specific memory for each interrupt in *ppIntContext */

    /* In this sample we will set the interrupt context to the driver context,
       which has been set in KP_PCI_Open to hold the device information. */
    *ppIntContext = pDrvContext;

    /* TODO: You can add code here to write to the device in order
             to physically enable the hardware interrupts */

    return TRUE;
}

/** Called when WDC_IntDisable() / WD_IntDisable()
* is called from the user mode for interrupts that were enabled
* in the Kernel PlugIn.
* WD_IntDisable() is called automatically from WDC_IntDisable() and
* InterruptDisable() (see WinDriver PCI Low-Level API Reference).
*
* This function should free any memory that was allocated in KP_IntEnable.
*
* @param [in] pIntContext: Interrupt context data that was set by
*    KP_IntEnable()
*/
void __cdecl KP_PCI_IntDisable(PVOID pIntContext)
{
    /* Free any memory allocated in KP_PCI_IntEnable() here */
}
/** High-priority legacy interrupt handler routine, which is run at high
* interrupt request level. This function is called upon the arrival of a
* legacy interrupt that has been enabled using a Kernel PlugIn driver —
* see the description of WDC_IntEnable() or the low-level
* InterruptEnable() and WD_IntEnable() functions (see WinDriver PCI
* Low-Level API Reference).
* @param [inout] pIntContext: Pointer to interrupt context data that was set
*     by KP_IntEnable() and will also be passed to KP_IntAtDpc() (if executed)
*     and KP_IntDisable().
* @param [out] pfIsMyInterrupt: Set *pfIsMyInterrupt to TRUE if the interrupt
*     belongs to this driver; otherwise set it to FALSE in order to enable the
*     interrupt service routines of other drivers for the same interrupt to be
*     called.
* @return TRUE if deferred interrupt processing (DPC) is required; otherwise
*     FALSE.
* @remark Code running at IRQL will only be interrupted by higher priority
* interrupts.
* Code running at high IRQL is limited in the following ways:
* It may only access non-pageable memory.
* It may only call the following functions (or wrapper functions that call * these functions):
* WDC_xxx() read/write address or configuration space functions.
* WDC_MultiTransfer(), or the low-level WD_Transfer(),
* WD_MultiTransfer(), or WD_DebugAdd() functions (see the WinDriver PCI
* Low-Level API Reference).
* Specific kernel OS functions (such as WDK functions) that can be called
* from high interrupt request level. Note that the use of such functions
* may break the code's portability to other operating systems.
* It may not call malloc(), free(), or any WDC_xxx or WD_xxx API other
* than those listed above.
* The code performed at high interrupt request level should be minimal
* (e.g., only the code that acknowledges level-sensitive interrupts), since
* it is operating at a high priority. The rest of your code should be
* written in KP_IntAtDpc(), which runs at the deferred DISPATCH level
* and is not subject to the above restrictions.
*/
BOOL __cdecl KP_PCI_IntAtIrql(PVOID pIntContext, BOOL *pfIsMyInterrupt)
{
    static DWORD dwIntCount = 0; /* Interrupts count */
    PCI_DEV_ADDR_DESC *pDevAddrDesc = (PCI_DEV_ADDR_DESC *)pIntContext;
    WDC_ADDR_DESC *pAddrDesc = &pDevAddrDesc->pAddrDesc[INTCSR_ADDR_SPACE];

#define USE_MULTI_TRANSFER
#if defined USE_MULTI_TRANSFER
    /* Define the number of interrupt transfer commands to use */
    WD_TRANSFER trans[2];

    /*
       This sample demonstrates how to set up two transfer commands, one for
       reading the device's INTCSR register (as defined in gPCI_Regs) and one
       for writing to it to acknowledge the interrupt.

       TODO: PCI interrupts are level sensitive interrupts and must be
             acknowledged in the kernel immediately when they are received.
             Since the information for acknowledging the interrupts is
             hardware-specific, YOU MUST MODIFY THE CODE below and set up
             transfer commands in order to correctly acknowledge the interrupts
             on your device, as dictated by your hardware's specifications.

       *************************************************************************
       * NOTE: If you attempt to use this code without first modifying it in   *
       *       order to correctly acknowledge your device's interrupts, as     *
       *       explained above, the OS will HANG when an interrupt occurs!     *
       *************************************************************************
    */

    BZERO(trans);

    /* Prepare the interrupt transfer commands */

    /* #1: Read status from the INTCSR register */
    trans[0].pPort = pAddrDesc->pAddr + INTCSR;
    /* 32bit read: */
    trans[0].cmdTrans = WDC_ADDR_IS_MEM(pAddrDesc) ? RM_DWORD : RP_DWORD;

    /* #2: Write ALL_INT_MASK to the INTCSR register to acknowledge the
           interrupt */
    /* In this example both commands access the same address (register): */
    trans[1].pPort = trans[0].pPort;
    /* 32bit write: */
    trans[1].cmdTrans = WDC_ADDR_IS_MEM(pAddrDesc) ? WM_DWORD : WP_DWORD;
    trans[1].Data.Dword = ALL_INT_MASK;

    /* Execute the transfer commands */
    WDC_MultiTransfer(trans, 2);
#else
    /* NOTE: For memory registers you can replace the use of WDC_MultiTransfer()
       (or any other WD_xxx/WDC_xxx read/write function call) with direct
       memory access. For example, if INTCSR is a memory register, the code
       above can be replaced with the following: */

    UINT32 readData;
    PVOID pData = (DWORD *)(pAddrDesc->pAddr + INTCSR);

    /* Read status from the PCI_INTCSR register */
    readData = WDC_ReadMem32(pData, 0);

    /* Write to the PCI_INTCSR register to acknowledge the interrupt */
    WDC_WriteMem32(pData, 0, ALL_INT_MASK);
#endif
#undef USE_MULTI_TRANSFER

    /* If the data read from the hardware indicates that the interrupt belongs
       to you, you must set *pfIsMyInterrupt to TRUE.
       Otherwise, set it to FALSE (this will let ISR's of other drivers be
       invoked). */
    *pfIsMyInterrupt = FALSE;
    /* This sample schedules a DPC once in every 5 interrupts.
       TODO: You can modify the implementation to schedule the DPC as needed. */
    dwIntCount++;
    if (!(dwIntCount % 5))
        return TRUE;

    return FALSE;
}

/**
* Deferred processing legacy interrupt handler routine.
* This function is called once the high-priority legacy interrupt handling 
* is completed, provided that KP_IntAtIrql() returned TRUE.
* @param [inout] pIntContext: Interrupt context data that was set by
*     KP_IntEnable(), passed to KP_IntAtIrql(), and will be passed to
*     KP_IntDisable().
* @param [in] dwCount: The number of times KP_IntAtIrql() returned TRUE
*     since the last DPC call. If dwCount is 1, KP_IntAtIrql requested a DPC
*     only once since the last DPC call. If the value is greater than 1,
*     KP_IntAtIrql has already requested a DPC a few times, but the interval
*     was too short, therefore KP_IntAtDpc was not called for each DPC request.
* @return Returns the number of times to notify user mode (i.e., return
*     from WD_IntWait() — see the WinDriver PCI Low-Level API Reference).
* @remark Most of the interrupt handling should be implemented within this
* function, as opposed to the high-priority KP_IntAtIrql() interrupt handler.
* If KP_IntAtDpc() returns with a value greater than zero, WD_IntWait()
* returns and the user-mode interrupt handler will be called in the amount
* of times set in the return value of KP_IntAtDpc(). If you do not want the
* user-mode interrupt handler to execute, KP_IntAtDpc() should return zero.
*/

DWORD __cdecl KP_PCI_IntAtDpc(PVOID pIntContext, DWORD dwCount)
{
    return dwCount;
}
/** High-priority Message-Signaled Interrupts (MSI) / Extended
* Message-Signaled Interrupts (MSI-X) handler routine, which is run at high
* interrupt request level. This function is called upon the arrival of an
* MSI/MSI-X that has been enabled using a Kernel PlugIn — see the
* description of WDC_IntEnable() or the low-level InterruptEnable() and
* WD_IntEnable() functions (see WinDriver PCI Low-Level API Reference).
*
* @param [inout] pIntContext: Pointer to interrupt context data that was
*     set by KP_IntEnable() and will also be passed to KP_IntAtDpcMSI() (if
*     executed) and KP_IntDisable()
* @param [in] dwLastMessage: The message data for the last received
*     interrupt.
@ @param [in] dwReserved: Reserved for future use. Do not use this
*     parameter.
*
* @return TRUE if deferred MSI/MSI-X processing (DPC) is required;
*     otherwise FALSE.
* @remark Code running at IRQL will only be interrupted by higher priority
* interrupts.
* Code running at high IRQL is limited in the following ways:
* It may only access non-pageable memory.
* It may only call the following functions (or wrapper functions that call 
* these functions):
* WDC_xxx() read/write address or configuration space functions.
* WDC_MultiTransfer(), or the low-level WD_Transfer(),
* WD_MultiTransfer(), or WD_DebugAdd() functions (see the WinDriver PCI
* Low-Level API Reference).
* Specific kernel OS functions (such as WDK functions) that can be called 
* from high interrupt request level. Note that the use of such functions may 
* break the code's portability to other operating systems.
* It may not call malloc(), free(), or any WDC_xxx or WD_xxx API other than 
* those listed above.
* The code performed at high interrupt request level should be minimal,
* since it is operating at a high priority. The rest of your code should be 
* written in KP_IntAtDpcMSI, which runs at the deferred DISPATCH
* level and is not subject to the above restrictions.
*/
BOOL __cdecl KP_PCI_IntAtIrqlMSI(PVOID pIntContext, ULONG dwLastMessage,
    DWORD dwReserved)
{
    static DWORD dwIntCount = 0; /* Interrupts count */

    /* There is no need to acknowledge MSI/MSI-X. However, you can implement
       the same functionality here as done in the KP_PCI_IntAtIrql handler
       to read/write data from/to registers at HIGH IRQL. */

    /* This sample schedules a DPC once in every 5 interrupts.
       TODO: You can modify the implementation to schedule the DPC as needed. */
    dwIntCount++;
    if (!(dwIntCount % 5))
        return TRUE;

    return FALSE;
}

/** Deferred processing Message-Signaled Interrupts (MSI) / Extended
* Message-Signaled Interrupts (MSI-X) handler routine.
* This function is called once the high-priority MSI/MSI-X handling is
* completed, provided that KP_IntAtIrqlMSI [B.8.10] returned TRUE.
*
* @param [inout] pIntContext: Interrupt context data that was set by
*     KP_IntEnable(), passed to KP_IntAtIrqlMSI(), and will be
*     passed to KP_IntDisable().
* @param [in] dwCount: The number of times KP_IntAtIrqlMSI()
*     returned TRUE since the last DPC call. If dwCount is 1, KP_IntAtIrqlMSI()
*     requested a DPC only once since the last DPC call. If the value is
*     greater than 1, KP_IntAtIrqlMSI() has already requested a DPC a few
*     times, but the interval was too short, therefore KP_IntAtDpcMSI() was not
*     called for each DPC request.
* @param [in] dwLastMessage: The message data for the last received interrupt.
* @param [in] dwReserved: Reserved for future use. Do not use this parameter.
*
* @return Returns the number of times to notify user mode (i.e., return
*     from WD_IntWait() — see the WinDriver PCI Low-Level API Reference).
* @remark Most of the MSI/MSI-X handling should be implemented within this
* function, as opposed to the high-priority KP_IntAtIrqlMSI()
* interrupt handler.
* If KP_IntAtDpcMSI returns with a value greater than zero, WD_IntWait()
* returns and the user-mode interrupt handler will be called in the amount
* of times set in the return value of KP_IntAtDpcMSI. If you do not want the
* user-mode interrupt handler to execute, KP_IntAtDpcMSI should return zero.
*/
DWORD __cdecl KP_PCI_IntAtDpcMSI(PVOID pIntContext, DWORD dwCount,
    ULONG dwLastMessage, DWORD dwReserved)
{
    return dwCount;
}


/** Called when a Plug-and-Play or power management event for the device is
* received, provided the user-mode application first called WDC_EventRegister()
* with fUseKP = TRUE (or the low-level EventRegister() function
* with a Kernel PlugIn handle — see WinDriver PCI Low-Level API Reference)
*
* @param [inout] pDrvContext: Driver context data that was set by
* KP_Open() and will also be passed to KP_Close(), KP_IntEnable() and
* KP_Event()
* @param [in] wd_event: Pointer to the PnP/power management event
* information received from the user mode
* @return TRUE in order to notify the user about the event. FALSE otherwise.
* @remark KP_Event will be called if the user mode process called
* WDC_EventRegister() with fUseKP= TRUE (or of the low-level EventRegister()
* function was called with a Kernel PlugIn handle — see the WinDriver PCI
* Low-Level API Reference).
*/
BOOL __cdecl KP_PCI_Event(PVOID pDrvContext, WD_EVENT *wd_event)
{
    return TRUE; /* Return TRUE to notify the user mode of the event */
}

/* -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- */
static void KP_PCI_Err(const CHAR *sFormat, ...)
{
#if defined(DEBUG)
    CHAR sMsg[256];
    va_list argp;

    va_start(argp, sFormat);
    vsnprintf(sMsg, sizeof(sMsg) - 1, sFormat, argp);
    WDC_Err("%s: %s", KP_PCI_DRIVER_NAME, sMsg);
    va_end(argp);
#endif
}

static void KP_PCI_Trace(const CHAR *sFormat, ...)
{
#if defined(DEBUG)
    CHAR sMsg[256];
    va_list argp;

    va_start(argp, sFormat);
    vsnprintf(sMsg, sizeof(sMsg) - 1, sFormat, argp);
    WDC_Trace("%s: %s", KP_PCI_DRIVER_NAME, sMsg);
    va_end(argp);
#endif
}

