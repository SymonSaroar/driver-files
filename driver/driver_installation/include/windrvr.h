/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/*
 * W i n D r i v e r
 * =================
 *
 * FOR DETAILS ON THE WinDriver FUNCTIONS, PLEASE SEE THE WinDriver MANUAL
 * OR INCLUDED HELP FILES.
 *
 * This file may not be distributed, it may only be used for development
 * or evaluation purposes. The only exception is distribution to Linux.
 * For details refer to \WinDriver\docs\license.txt.
 *
 * Web site: https://www.jungo.com
 * Email:    support@jungo.com
 */
#ifndef _WINDRVR_H_
#define _WINDRVR_H_

#if defined(__cplusplus)
    extern "C" {
#endif

#include "wd_ver.h"

#if defined(WIN32) && !defined(__MINGW32__)
    #define DLLCALLCONV __stdcall
    #if !defined(_SAL_VERSION)
            #include <sal.h>
    #endif
#else
    #define DLLCALLCONV

    /* the following macros are part of SAL annotations macros in Windows and
     are used in prototype of API functions, therefore, must be defined in
     Unix as well */
    #define _In_
    #define _Inout_
    #define _Out_
    #define _Outptr_
#endif

#if defined(WIN32)
    #define WD_DRIVER_NAME_PREFIX "\\\\.\\"
#elif defined(LINUX)
    #define WD_DRIVER_NAME_PREFIX "/dev/"
#else
    #define WD_DRIVER_NAME_PREFIX ""
#endif

#if !defined(WIN32)
    #define __FUNCTION__ __func__
#endif

#define WD_DEFAULT_DRIVER_NAME_BASE "windrvr" WD_VER_ITOA
#define WD_DEFAULT_DRIVER_NAME \
    WD_DRIVER_NAME_PREFIX WD_DEFAULT_DRIVER_NAME_BASE

#define WD_MAX_DRIVER_NAME_LENGTH 128
#define WD_MAX_KP_NAME_LENGTH 128
#define WD_VERSION_STR_LENGTH 128

#if defined(WD_DRIVER_NAME_CHANGE)
    /**  Sets the name of the WinDriver kernel module, which will be used by
     *   the calling application.
     * 
     *   The default driver name, which is used if the function is not called,
     *   is `windrvr1511`.
     *
     *   This function must be called once, and only once, from the beginning
     *   of your application,
     *   before calling any other WinDriver function
     *   (including WD_Open() / WDC_DriverOpen() / WDC_PciDeviceOpen() / WDC_IsaDeviceOpen()),
     *   as demonstrated in the sample and generated DriverWizard
     *   WinDriver applications, which include a call to this function with
     *   the default driver name - `windrvr1511`
     *
     *   On Windows and Linux, if you select to modify the name of the
     *   WinDriver kernel module (`windrvr1511.sys/.dll/.o/.ko`),
     *   as explained in @ref ch17_2_renaming_the_windriver_kernel_driver,
     *   you must ensure that your application calls WD_DriverName() with your new driver name.
     *
     *  In order to use the WD_DriverName() function, your user-mode driver
     *  project must be built with WD_DRIVER_NAME_CHANGE preprocessor flag 
     *  (e.g.: -DWD_DRIVER_NAME_CHANGE — for MS Visual Studio, Windows GCC,
     *  and GCC). The sample and generated DriverWizard Windows and Linux
     *  WinDriver projects/makefiles already set this preprocessor flag.
     * 
     *   @param [in] sName:  The name of the WinDriver kernel module to be
     *                       used by the application.
     *                       NOTE: The driver name should be indicated without
     *                       the driver file's extension.
     *                       For example, use `windrvr1511`,
     *                       not `windrvr1511.sys` or
     *                       `windrvr1511.o`.
     *
     *   @return Returns the selected driver name on success;
     *            returns NULL on failure (e.g., if the function is
     *            called twice from the same application).
     *
     * @snippet highlevel_examples.c WD_DriverName
     */
    const char* DLLCALLCONV WD_DriverName(const char *sName);
    /** Get driver name */
    #define WD_DRIVER_NAME WD_DriverName(NULL)
#else
    #define WD_DRIVER_NAME WD_DEFAULT_DRIVER_NAME
#endif

#define WD_PROD_NAME "WinDriver"

#if !defined(ARM) && \
    !defined(ARM64) && \
    !defined(x86) && \
    (defined(LINUX) || (defined(WIN32)))
        #define x86
#endif

#if !defined(x86_64) && \
    (defined(x86) && (defined(KERNEL_64BIT) || defined(__x86_64__)))
    #define x86_64
#endif

#if defined(x86_64)
    #define WD_CPU_SPEC " x86_64"
#elif defined(ARM)
    #define WD_CPU_SPEC " ARM"
#elif defined(ARM64)
    #define WD_CPU_SPEC " ARM64"
#else
    #define WD_CPU_SPEC " X86"
#endif

#if defined(WINNT)
    #define WD_FILE_FORMAT " sys"
#elif defined (APPLE)
    #define WD_FILE_FORMAT " kext"
#elif defined (LINUX) 
    #define WD_FILE_FORMAT " ko"
#endif

#if defined(KERNEL_64BIT)
    #define WD_DATA_MODEL " 64bit"
#else
    #define WD_DATA_MODEL " 32bit"
#endif

#define WD_VER_STR  WD_PROD_NAME " v" WD_VERSION_STR \
    " Jungo Connectivity (c) 1997 - " COPYRIGHTS_YEAR_STR \
        " Build Date: " __DATE__ \
    WD_CPU_SPEC WD_DATA_MODEL WD_FILE_FORMAT

#if !defined(POSIX) && defined(LINUX)
    #define POSIX
#endif

#if !defined(UNIX) && defined(POSIX)
    #define UNIX
#endif

#if !defined(WIN32) && defined(WINNT)
    #define WIN32
#endif

#if !defined(WIN32) && !defined(UNIX) && !defined(APPLE)
    #define WIN32
#endif

#if defined(_KERNEL_MODE) && !defined(KERNEL)
    #define KERNEL
#endif

#if defined(KERNEL) && !defined(__KERNEL__)
    #define __KERNEL__
#endif

#if defined(_KERNEL) && !defined(__KERNEL__)
    #define __KERNEL__
#endif

#if defined( __KERNEL__) && !defined(_KERNEL)
    #define _KERNEL
#endif

#if defined(LINUX) && defined(__x86_64__) && !defined(__KERNEL__)
    /* This fixes binary compatibility with older version of GLIBC
     * (64bit only) */
    __asm__(".symver memcpy,memcpy@GLIBC_2.2.5");
#endif

#if defined(UNIX)


    #if !defined(__P_TYPES__)
        #define __P_TYPES__
        #include <wd_types.h>

        typedef void VOID;
        typedef unsigned char UCHAR;
        typedef unsigned short USHORT;
        typedef unsigned int UINT;
        #if !defined (APPLE_USB)
            typedef unsigned long ULONG;
        #endif
        typedef u32 BOOL;
        typedef void *PVOID;
        typedef unsigned char *PBYTE;
        typedef char CHAR;
        typedef char *PCHAR;
        typedef unsigned short *PWORD;
        typedef u32 DWORD, *PDWORD;
        typedef PVOID HANDLE;
    #endif
    #if !defined(__KERNEL__)
        #include <string.h>
        #include <ctype.h>
        #include <stdlib.h>
    #endif
    #ifndef TRUE
        #define TRUE 1
    #endif
    #ifndef FALSE
        #define FALSE 0
    #endif
    #define __cdecl
    #define WINAPI

    #if defined(__KERNEL__)
        #if defined(LINUX)
            /* For _IO macros and for mapping Linux status codes
             * to WD status codes */
            #include <asm-generic/ioctl.h>
            #include <asm-generic/errno.h>
        #endif
    #else
        #include <unistd.h>
        #if defined(LINUX)
            #include <sys/ioctl.h> /* for BSD ioctl() */
            #include <sys/mman.h>
        #endif
        #include <sys/types.h>
        #include <sys/stat.h>
        #include <fcntl.h>
    #endif
        typedef unsigned long long UINT64;
    #if defined(APPLE)
        #include <libkern/OSTypes.h>
        #include <libkern/OSAtomic.h>
        typedef UInt16 UINT16;
        typedef UInt8 UINT8;
        typedef UInt8 u8;
        typedef UInt16 u16;
        typedef UInt32 u32;
        #include <IOKit/IOTypes.h>
        #if defined(__KERNEL__)
            typedef struct OSObject* FILEHANDLE;
            typedef UInt32 PRCHANDLE;
        #else
            #include <IOKit/IOKitLib.h>
        #endif
        enum {
            kWinDriverMethodSyncIoctl = 0,
            kWinDriverNumOfMethods,
        };
        #if !defined(__KERNEL__)
             #include <string.h>
             #include <ctype.h>
        #endif
     #ifndef TRUE
         #define TRUE true
     #endif
     #ifndef FALSE
         #define FALSE false
     #endif
     #define __cdecl
     #define WINAPI
     #ifdef __LP64__
         #define PTR2INT(value) ((UInt64)(value))
     #else
         #define PTR2INT(value) ((UInt32)(value))
     #endif
     #define MAC_UC_MAGIC_MASK 0xFF0000FF
     #define MAC_UC_MAGIC_NUM  0x7E000041
     #define MAC_UC_64BIT_APP  0x100
#endif

#elif defined(WIN32)
    #if defined(__KERNEL__)
        #if !defined (CMAKE_WD_BUILD)
            int sprintf(char *buffer, const char *format, ...);
        #endif
    #else
        #include <windows.h>
        #include <winioctl.h>
    #endif
    #define stricmp _stricmp
    typedef unsigned __int64 UINT64;
#endif

#if !defined(__KERNEL__)
    #include <stdarg.h>
    #if !defined(va_copy) && !defined(__va_copy)
        #define va_copy(ap2,ap1) (ap2)=(ap1)
    #endif
    #if !defined(va_copy) && defined(__va_copy)
        #define va_copy __va_copy
    #endif
#endif

#ifndef WINAPI
    #define WINAPI
#endif

#if !defined(_WINDEF_)
    typedef unsigned char BYTE;
    typedef unsigned short int WORD;
#endif

#if !defined(_BASETSD_H_)
    typedef unsigned int UINT32;
#endif


/** formatting for printing a 64bit variable */
#if defined(UNIX)
    #define PRI64       "ll"
#elif defined(WIN32)
    #define PRI64       "I64"
#endif

/** formatting for printing a kernel pointer */
#if defined(KERNEL_64BIT)
    #define KPRI PRI64
    #if defined(WIN32)
        #define UPRI KPRI
    #else
        #define UPRI "l"
    #endif
#else
    #define KPRI ""
    #define UPRI "l"
#endif

/*
 * The KPTR is guaranteed to be the same size as a kernel-mode pointer
 * The UPTR is guaranteed to be the same size as a user-mode pointer
 */
#if defined(KERNEL_64BIT)
    typedef UINT64 KPTR;
#else
    typedef UINT32 KPTR;
#endif

#if defined(UNIX)
    typedef unsigned long UPTR;
#else
    typedef size_t UPTR;
#endif

typedef UINT64 DMA_ADDR;
typedef UINT64 PHYS_ADDR;

#include "windrvr_usb.h"

/** IN WD_TRANSFER_CMD and WD_Transfer() DWORD stands for 32 bits and QWORD is
 *  64 bit. */
typedef enum
{
    CMD_NONE = 0,       /**< No command */
    CMD_END = 1,        /**< End command */
    CMD_MASK = 2,       /**< Interrupt Mask */

    RP_BYTE = 10,       /**< Read port byte */
    RP_WORD = 11,       /**< Read port word */
    RP_DWORD = 12,      /**< Read port dword */
    WP_BYTE = 13,       /**< Write port byte */
    WP_WORD = 14,       /**< Write port word */
    WP_DWORD = 15,      /**< Write port dword */
    RP_QWORD = 16,      /**< Read port qword */
    WP_QWORD = 17,      /**< Write port qword */

    RP_SBYTE = 20,      /**< Read port string byte */
    RP_SWORD = 21,      /**< Read port string word */
    RP_SDWORD = 22,     /**< Read port string dword */
    WP_SBYTE = 23,      /**< Write port string byte */
    WP_SWORD = 24,      /**< Write port string word */
    WP_SDWORD = 25,     /**< Write port string dword */
    RP_SQWORD = 26,     /**< Read port string qword */
    WP_SQWORD = 27,     /**< Write port string qword */

    RM_BYTE = 30,       /**< Read memory byte */
    RM_WORD = 31,       /**< Read memory word */
    RM_DWORD = 32,      /**< Read memory dword */
    WM_BYTE = 33,       /**< Write memory byte */
    WM_WORD = 34,       /**< Write memory word */
    WM_DWORD = 35,      /**< Write memory dword */
    RM_QWORD = 36,      /**< Read memory qword */
    WM_QWORD = 37,      /**< Write memory qword */

    RM_SBYTE = 40,      /**< Read memory string byte */
    RM_SWORD = 41,      /**< Read memory string word */
    RM_SDWORD = 42,     /**< Read memory string dword */
    WM_SBYTE = 43,      /**< Write memory string byte */
    WM_SWORD = 44,      /**< Write memory string word */
    WM_SDWORD = 45,     /**< Write memory string dword */
    RM_SQWORD = 46,     /**< Read memory string quad word */
    WM_SQWORD = 47      /**< Write memory string quad word */
} WD_TRANSFER_CMD;

enum { WD_DMA_PAGES = 256 };

#ifndef DMA_BIT_MASK
    #define DMA_BIT_MASK(n) (((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))
#endif

typedef enum {
    DMA_KERNEL_BUFFER_ALLOC = 0x1, /**< The system allocates a contiguous
         buffer. The user does not need to supply linear address. */

    DMA_KBUF_BELOW_16M = 0x2, /**< If DMA_KERNEL_BUFFER_ALLOC is used,
        * this will make sure it is under 16M. */

    DMA_LARGE_BUFFER = 0x4, /**< If DMA_LARGE_BUFFER is used,
        the maximum number of pages are dwPages, and not
        WD_DMA_PAGES. If you lock a user buffer (not a kernel
        allocated buffer) that is larger than 1MB, then use this
        option and allocate memory for pages. */

    DMA_ALLOW_CACHE = 0x8,  /**< Allow caching of contiguous memory. */

    DMA_KERNEL_ONLY_MAP = 0x10, /**< Only map to kernel, dont map to user-mode.
        relevant with DMA_KERNEL_BUFFER_ALLOC flag only */

    DMA_FROM_DEVICE = 0x20, /**< memory pages are locked to be written by
                              device */

    DMA_TO_DEVICE = 0x40, /**< memory pages are locked to be read by device */

    DMA_TO_FROM_DEVICE = (DMA_FROM_DEVICE | DMA_TO_DEVICE), /**< memory pages
        are locked for both read and write */

    DMA_ALLOW_64BIT_ADDRESS = 0x80, /**< Use this value for devices that support
                                    * 64-bit DMA addressing. */

    DMA_ALLOW_NO_HCARD = 0x100, /**< Allow memory lock without hCard */

    DMA_GET_EXISTING_BUF = 0x200, /**< Get existing buffer by hDma handle */

    DMA_RESERVED_MEM = 0x400,

    DMA_KBUF_ALLOC_SPECIFY_ADDRESS_WIDTH = 0x800, /**< When using this flag, the
        width of the address must be entered in the fourth byte of dwOptions and
        then the allocated address will be limited to this width.
        Linux: works with contiguous buffers only. */

     DMA_GET_PREALLOCATED_BUFFERS_ONLY = 0x1000, /**< Windows: Try to allocate
        buffers from preallocated buffers pool ONLY (if none of them are
        available, function will fail). */

     DMA_TRANSACTION = 0x2000, /**< Use this flag to use the DMA transaction
                              * mechanism */

     DMA_GPUDIRECT = 0x4000, /**< Linux only */

     DMA_DISABLE_MERGE_ADJACENT_PAGES = 0x8000, /**< Disable merge adjacent
        pages. In case the flag is omitted, the merge will take place
        automatically. Used for scatter gather mode only. */

} WD_DMA_OPTIONS;

#define DMA_ADDRESS_WIDTH_MASK 0x7f000000

#define DMA_OPTIONS_ALL \
    (DMA_KERNEL_BUFFER_ALLOC | DMA_KBUF_BELOW_16M | DMA_LARGE_BUFFER \
    | DMA_ALLOW_CACHE | DMA_KERNEL_ONLY_MAP | DMA_FROM_DEVICE | DMA_TO_DEVICE \
    | DMA_ALLOW_64BIT_ADDRESS | DMA_ALLOW_NO_HCARD | DMA_GET_EXISTING_BUF \
    | DMA_RESERVED_MEM | DMA_KBUF_ALLOC_SPECIFY_ADDRESS_WIDTH \
    | DMA_ADDRESS_WIDTH_MASK)

#define DMA_DIRECTION_MASK DMA_TO_FROM_DEVICE

/** Macros for backward compatibility */
#define DMA_READ_FROM_DEVICE DMA_FROM_DEVICE
#define DMA_WRITE_TO_DEVICE DMA_TO_DEVICE

#define DMA_OPTIONS_ADDRESS_WIDTH_SHIFT 24 /** 3 bytes (24 bits) are needed for
                                    WD_DMA_OPTIONS so the fourth byte will be
                                    used for storing the address width of the
                                    requested buffer */

enum {
    WD_MATCH_EXCLUDE = 0x1 /**< Exclude if there is a match */
};

/* Use it to pad struct size to 64 bit, when using 32 on 64 bit applications */
#if defined (i386) && defined(KERNEL_64BIT)
    #define PAD_TO_64(pName) DWORD dwPad_##pName;
#else
    #define PAD_TO_64(pName)
#endif

typedef struct
{
    DMA_ADDR pPhysicalAddr; /**< Physical address of page. */
    DWORD dwBytes;          /**< Size of page. */
    PAD_TO_64(dwBytes)
} WD_DMA_PAGE, WD_DMA_PAGE_V80;

typedef void (DLLCALLCONV * DMA_TRANSACTION_CALLBACK)(PVOID pData);

typedef struct
{
    DWORD hDma;        /**< Handle of DMA buffer */
    PAD_TO_64(hDma)

    PVOID pUserAddr;   /**< User address */
    PAD_TO_64(pUserAddr)

    KPTR  pKernelAddr; /**< Kernel address */
    DWORD dwBytes;     /**< Size of buffer */
    DWORD dwOptions;   /**< The first three bytes refer to WD_DMA_OPTIONS
                            The fourth byte is used for specifying the
                            amount of DMA bits in the requested buffer */
    DWORD dwPages;     /**< Number of pages in buffer. */
    DWORD hCard;       /**< Handle of relevant card as received from
                            WD_CardRegister() */

    /* Windows: The following 6 parameters are used for DMA transaction only */

    DMA_TRANSACTION_CALLBACK DMATransactionCallback;
    PAD_TO_64(DMATransactionCallback)

    PVOID DMATransactionCallbackCtx;
    PAD_TO_64(DMATransactionCallbackCtx)

    DWORD dwAlignment; /**< required alignment, used for contiguous mode only */
    DWORD dwMaxTransferSize;     /**< used for scatter gather mode only */
    DWORD dwTransferElementSize; /**< used for scatter gather mode only */
    DWORD dwBytesTransferred;    /**< bytes transferred count */

    WD_DMA_PAGE Page[WD_DMA_PAGES];
} WD_DMA, WD_DMA_V80;

typedef enum {
    /* KER_BUF_ALLOC_NON_CONTIG and KER_BUF_GET_EXISTING_BUF options are valid
     * only as part of "WinDriver for Server" API and require
     * "WinDriver for Server" license.
     * @note "WinDriver for Server" APIs are included in WinDriver evaluation
     * version. */
    KER_BUF_ALLOC_NON_CONTIG = 0x0001,
    KER_BUF_ALLOC_CONTIG     = 0x0002,
    KER_BUF_ALLOC_CACHED     = 0x0004,
    KER_BUF_GET_EXISTING_BUF = 0x0008,
} WD_KER_BUF_OPTION;

typedef struct
{
    DWORD hKerBuf;    /**< Handle of Kernel Buffer */
    DWORD dwOptions;  /**< Refer to WD_KER_BUF_OPTION */
    UINT64 qwBytes;   /**< Size of buffer */
    KPTR pKernelAddr; /**< Kernel address */
    UPTR pUserAddr;   /**< User address */
    PAD_TO_64(pUserAddr)
} WD_KERNEL_BUFFER, WD_KERNEL_BUFFER_V121;

typedef struct
{
    KPTR  pPort;       /**< I/O port for transfer or kernel memory address. */
    DWORD cmdTrans;    /**< Transfer command WD_TRANSFER_CMD. */

    /*  Parameters used for string transfers: */
    DWORD dwBytes;     /**< For string transfer. */
    DWORD fAutoinc;    /**< Transfer from one port/address
                       *  or use incremental range of addresses. */
    DWORD dwOptions;   /**< Must be 0. */
    union
    {
        BYTE Byte;     /**< Use for 8 bit transfer. */
        WORD Word;     /**< Use for 16 bit transfer. */
        UINT32 Dword;  /**< Use for 32 bit transfer. */
        UINT64 Qword;  /**< Use for 64 bit transfer. */
        PVOID pBuffer; /**< Use for string transfer. */
    } Data;
} WD_TRANSFER, WD_TRANSFER_V61;

enum {
    INTERRUPT_LATCHED = 0x00,             /**< Legacy Latched Interrupts . */
    INTERRUPT_LEVEL_SENSITIVE = 0x01,     /**< Legacy Level Sensitive 
                                               Interrupts. */
    INTERRUPT_CMD_COPY = 0x02,            /**< Copy any data read in the kernel
                                               as a result of a read transfer
                                               command, and return it to the
                                               user within the relevant
                                               transfer command structure. */
    INTERRUPT_CE_INT_ID = 0x04,           /**< Obsolete. */
    INTERRUPT_CMD_RETURN_VALUE = 0x08,    /**< Obsolete. */
    INTERRUPT_MESSAGE = 0x10,             /**< Message-Signaled Interrupts (MSI). */
    INTERRUPT_MESSAGE_X = 0x20,           /**< Extended Message-Signaled 
                                               Interrupts (MSI-X). */
    INTERRUPT_DONT_GET_MSI_MESSAGE = 0x40 /**< Linux Only. Do not read the
                                               MSI/MSI-X message from the card. */
};

typedef struct
{
    DWORD hKernelPlugIn;
    DWORD dwMessage;
    PVOID pData;
    PAD_TO_64(pData)
    DWORD dwResult;
    PAD_TO_64(dwResult)
} WD_KERNEL_PLUGIN_CALL, WD_KERNEL_PLUGIN_CALL_V40;

typedef enum {
    INTERRUPT_RECEIVED = 0, /**< Interrupt was received */
    INTERRUPT_STOPPED,      /**< Interrupt was disabled during wait */
    INTERRUPT_INTERRUPTED   /**< Wait was interrupted before an actual hardware
                                 interrupt was received */
} WD_INTERRUPT_WAIT_RESULT;

typedef struct
{
    DWORD hInterrupt;    /**< Handle of interrupt. */
    DWORD dwOptions;     /**<  Interrupt options: can be INTERRUPT_CMD_COPY */

    WD_TRANSFER *Cmd;    /**<  Commands to do on interrupt. */
    PAD_TO_64(Cmd)
    DWORD dwCmds;        /**<  Number of commands. */
    PAD_TO_64(dwCmds)

    /* For WD_IntEnable(): */
    WD_KERNEL_PLUGIN_CALL kpCall; /**<  Kernel PlugIn call. */
    DWORD fEnableOk;     /**<  TRUE if interrupt was enabled (WD_IntEnable()
                               succeeded). */

    /* For WD_IntWait() and WD_IntCount(): */
    DWORD dwCounter;     /**< Number of interrupts received. */
    DWORD dwLost;        /**< Number of interrupts not yet dealt with. */
    DWORD fStopped;      /**< Was interrupt disabled during wait. */
    DWORD dwLastMessage; /**< Message data of the last received MSI (Windows) */
    DWORD dwEnabledIntType; /**< Interrupt type that was actually enabled */
} WD_INTERRUPT, WD_INTERRUPT_V91;

typedef struct
{
    DWORD dwVer;
    CHAR cVer[WD_VERSION_STR_LENGTH];
} WD_VERSION, WD_VERSION_V30;

#define WD_LICENSE_LENGTH 3072
typedef struct
{
    CHAR cLicense[WD_LICENSE_LENGTH]; /**< Buffer with license string to put */
} WD_LICENSE, WD_LICENSE_V122;

enum
{
    WD_BUS_USB = (int)0xfffffffe,   /**< USB */
    WD_BUS_UNKNOWN = 0,             /**< Unknown bus type */
    WD_BUS_ISA = 1,                 /**< ISA */
    WD_BUS_EISA = 2,                /**< EISA, including ISA PnP */
    WD_BUS_PCI = 5,                 /**< PCI */
};
typedef DWORD WD_BUS_TYPE;

typedef struct
{
    WD_BUS_TYPE dwBusType;  /**< Bus Type: WD_BUS_PCI/ISA/EISA */
    DWORD dwDomainNum;      /**< Domain number */
    DWORD dwBusNum;         /**< Bus number */
    DWORD dwSlotFunc;       /**< Slot number on the bus */
} WD_BUS, WD_BUS_V30;

typedef enum
{
    ITEM_NONE         = 0,
    ITEM_INTERRUPT    = 1, /**< Interrupt */
    ITEM_MEMORY       = 2, /**< Memory */
    ITEM_IO           = 3, /**< I/O */
    ITEM_BUS          = 5  /**< Bus */
} ITEM_TYPE;

typedef enum
{
    WD_ITEM_MEM_DO_NOT_MAP_KERNEL = 0x1,/**< Skip the mapping of physical memory
                                         to the kernel address space */
    WD_ITEM_MEM_ALLOW_CACHE =      0x2, /**< Map physical memory as cached;
                                         applicable only to host RAM, not to
                                         local memory on the card */
    WD_ITEM_MEM_USER_MAP =         0x4, /**< Map physical memory from user mode,
                                         Linux only */
} WD_ITEM_MEM_OPTIONS;

typedef struct
{
    DWORD item;         /** ITEM_TYPE */
    DWORD fNotSharable;
    union
    {
        /** ITEM_MEMORY */
        struct
        {
            PHYS_ADDR pPhysicalAddr; /**< Physical address on card */
            UINT64 qwBytes;          /**< Address range */
            KPTR pTransAddr;         /**< Kernel-mode mapping of the physical
                                        base address, to be used for transfer
                                        commands; returned by WD_CardRegister()
                                      */
            UPTR pUserDirectAddr;    /**< User-mode mapping of the physical base
                                        address, for direct user read/write
                                        transfers; returned by WD_CardRegister()
                                      */
            PAD_TO_64(pUserDirectAddr)
            DWORD dwBar;             /**< PCI Base Address Register number */
            DWORD dwOptions;         /**< Bitmask of WD_ITEM_MEM_OPTIONS flags
                                      */
            KPTR pReserved;          /**< Reserved for internal use */
        } Mem;

        /** ITEM_IO */
        struct
        {
            KPTR  pAddr;    /**< Beginning of I/O address */
            DWORD dwBytes;  /**< I/O range */
            DWORD dwBar;    /**< PCI Base Address Register number */
        } IO;

        /** ITEM_INTERRUPT */
        struct
        {
            DWORD dwInterrupt;  /**< Number of interrupt to install */
            DWORD dwOptions;    /**< Interrupt options:
                        INTERRUPT_LATCHED -- latched
                        INTERRUPT_LEVEL_SENSITIVE -- level sensitive
                        INTERRUPT_MESSAGE -- Message-Signaled Interrupts (MSI)
                        INTERRUPT_MESSAGE_X -- Extended MSI (MSI-X) */
            DWORD hInterrupt;   /**< Handle of the installed interrupt; returned
                                   by WD_CardRegister() */
            DWORD dwReserved1;  /**< For internal use */
            KPTR  pReserved2;   /**< For internal use */
        } Int;
        WD_BUS Bus; /**< ITEM_BUS */
    } I;
} WD_ITEMS, WD_ITEMS_V118;

enum { WD_CARD_ITEMS = 128 };

typedef struct
{
    DWORD dwItems;
    PAD_TO_64(dwItems)
    WD_ITEMS Item[WD_CARD_ITEMS];
} WD_CARD, WD_CARD_V118;

typedef struct
{
    WD_CARD Card;           /**< Card to register. */
    DWORD fCheckLockOnly;   /**< Only check if card is lockable, return hCard=1
                              if OK. */
    DWORD hCard;            /**< Handle of card. */
    DWORD dwOptions;        /**< Should be zero. */
    CHAR cName[32];         /**< Name of card. */
    CHAR cDescription[100]; /**< Description. */
} WD_CARD_REGISTER, WD_CARD_REGISTER_V118;

#define WD_PROCESS_NAME_LENGTH 128
typedef struct
{
    CHAR  cProcessName[WD_PROCESS_NAME_LENGTH];
    DWORD dwSubGroupID; /**< Identifier of the processes type */
    DWORD dwGroupID;  /**< Unique identifier of the processes group for
                        discarding unrelated process. WinDriver developers are
                        encouraged to change their driver name before
                        distribution to avoid this issue entirely. */
    DWORD hIpc;       /**< Returned from WD_IpcRegister() */
} WD_IPC_PROCESS, WD_IPC_PROCESS_V121;

typedef struct
{
    WD_IPC_PROCESS procInfo;
    DWORD          dwOptions;  /**< Reserved for future use; set to 0 */
} WD_IPC_REGISTER, WD_IPC_REGISTER_V121;

enum { WD_IPC_MAX_PROCS = 0x40 };

typedef struct
{
    DWORD          hIpc;       /**< Returned from WD_IpcRegister() */

    /** Result processes */
    DWORD          dwNumProcs; /**< Number of matching processes */
    WD_IPC_PROCESS procInfo[WD_IPC_MAX_PROCS];
} WD_IPC_SCAN_PROCS, WD_IPC_SCAN_PROCS_V121;

enum
{
    WD_IPC_UID_UNICAST = 0x1,
    WD_IPC_SUBGROUP_MULTICAST = 0x2,
    WD_IPC_MULTICAST = 0x4,
};

typedef struct
{
    DWORD hIpc;           /**< Returned from WD_IpcRegister() */
    DWORD dwOptions;      /**< WD_IPC_SUBGROUP_MULTICAST, WD_IPC_UID_UNICAST,
                           * WD_IPC_MULTICAST */

    DWORD  dwRecipientID; /**< used only on WD_IPC_UNICAST */
    DWORD  dwMsgID;
    UINT64 qwMsgData;
} WD_IPC_SEND, WD_IPC_SEND_V121;

typedef struct
{
    DWORD hCard;            /**< Handle of card. */
    PAD_TO_64(hCard)
    WD_TRANSFER *Cmd;       /**< Buffer with WD_TRANSFER commands */
    PAD_TO_64(Cmd)
    DWORD dwCmds;           /**< Number of commands. */
    DWORD dwOptions;        /**< 0 (default) or WD_FORCE_CLEANUP */
} WD_CARD_CLEANUP;

enum { WD_FORCE_CLEANUP = 0x1 };

enum { WD_PCI_CARDS = 256 }; /**< Slots max X Functions max */

typedef struct
{
    DWORD dwDomain; /**< Domain number. currently only applicable for Linux
                      systems. Zero by default */
    DWORD dwBus; /**< Bus number */
    DWORD dwSlot; /**< Slot number */
    DWORD dwFunction; /**< Function number */
} WD_PCI_SLOT;

typedef struct
{
    DWORD dwVendorId;
    DWORD dwDeviceId;
} WD_PCI_ID;

typedef struct
{
    /** Scan Parameters */
    WD_PCI_ID searchId;     /**< PCI vendor and/or device IDs to search for;
                              - dwVendorId==0 -- scan all PCI vendor IDs;
                              - dwDeviceId==0 -- scan all PCI device IDs */
    DWORD dwCards;          /**< Number of matching PCI cards */

    /** Scan Results */
    WD_PCI_ID cardId[WD_PCI_CARDS];     /**< Array of matching card IDs */
    WD_PCI_SLOT cardSlot[WD_PCI_CARDS]; /**< Array of matching PCI slots info */

    /** Scan Options */
    DWORD dwOptions;        /**< Scan options -- WD_PCI_SCAN_OPTIONS */
} WD_PCI_SCAN_CARDS, WD_PCI_SCAN_CARDS_V124;

typedef enum {
    WD_PCI_SCAN_DEFAULT = 0x0,
    WD_PCI_SCAN_BY_TOPOLOGY = 0x1,
    WD_PCI_SCAN_REGISTERED = 0x2,
    WD_PCI_SCAN_INCLUDE_DOMAINS = 0x4,
} WD_PCI_SCAN_OPTIONS;

enum { WD_PCI_MAX_CAPS = 50 };

enum { WD_PCI_CAP_ID_ALL = 0x0 };

typedef struct
{
    DWORD dwCapId;      /**< PCI capability ID */
    DWORD dwCapOffset;  /**< PCI capability register offset */
} WD_PCI_CAP;

typedef enum {
    WD_PCI_SCAN_CAPS_BASIC = 0x1,   /**< Scan basic PCI capabilities */
    WD_PCI_SCAN_CAPS_EXTENDED = 0x2 /**< Scan extended (PCIe) PCI capabilities
                                     */
} WD_PCI_SCAN_CAPS_OPTIONS;

typedef struct
{
    /** Scan Parameters */
    WD_PCI_SLOT pciSlot;   /**< PCI slot information */
    DWORD       dwCapId;   /**< PCI capability ID to search for, or
                              WD_PCI_CAP_ID_ALL to scan all PCI capabilities */
    DWORD       dwOptions; /**< Scan options -- WD_PCI_SCAN_CAPS_OPTIONS;
                              default -- WD_PCI_SCAN_CAPS_BASIC */

    /** Scan Results */
    DWORD       dwNumCaps; /**< Number of matching PCI capabilities */
    WD_PCI_CAP  pciCaps[WD_PCI_MAX_CAPS]; /**< Array of matching PCI
                                            capabilities */
} WD_PCI_SCAN_CAPS, WD_PCI_SCAN_CAPS_V118;

typedef struct
{
    WD_PCI_SLOT pciSlot;    /**< PCI slot information */
    DWORD       dwNumVFs;   /**< Number of Virtual Functions */
} WD_PCI_SRIOV, WD_PCI_SRIOV_V122;

typedef struct
{
    WD_PCI_SLOT pciSlot;    /**< PCI slot information */
    WD_CARD Card;           /**< Card information */
} WD_PCI_CARD_INFO, WD_PCI_CARD_INFO_V118;

typedef enum
{
    PCI_ACCESS_OK = 0,
    PCI_ACCESS_ERROR = 1,
    PCI_BAD_BUS = 2,
    PCI_BAD_SLOT = 3
} PCI_ACCESS_RESULT;

typedef struct
{
    WD_PCI_SLOT pciSlot;    /**< PCI slot information */
    PVOID       pBuffer;    /**< Pointer to a read/write data buffer */
    PAD_TO_64(pBuffer)

    DWORD       dwOffset;   /**< PCI configuration space offset to read/write */
    DWORD       dwBytes;    /**< Input -- number of bytes to read/write;
                               Output -- number of bytes read/written */
    DWORD       fIsRead;    /**< 1 -- read data; 0 -- write data */
    DWORD       dwResult;   /**< PCI_ACCESS_RESULT */
} WD_PCI_CONFIG_DUMP, WD_PCI_CONFIG_DUMP_V30;

enum { SLEEP_BUSY = 0, SLEEP_NON_BUSY = 1 };
typedef struct
{
    DWORD dwMicroSeconds; /**< Sleep time in Micro Seconds (1/1,000,000
                            Second) */
    DWORD dwOptions;      /**< can be: SLEEP_NON_BUSY (10000 uSec +) */
} WD_SLEEP, WD_SLEEP_V40;

typedef enum
{
    D_OFF       = 0,
    D_ERROR     = 1,
    D_WARN      = 2,
    D_INFO      = 3,
    D_TRACE     = 4
} DEBUG_LEVEL;

typedef enum
{
    S_ALL       = (int)0xffffffff,
    S_IO        = 0x00000008,
    S_MEM       = 0x00000010,
    S_INT       = 0x00000020,
    S_PCI       = 0x00000040,
    S_DMA       = 0x00000080,
    S_MISC      = 0x00000100,
    S_LICENSE   = 0x00000200,
    S_PNP       = 0x00001000,
    S_CARD_REG  = 0x00002000,
    S_KER_DRV   = 0x00004000,
    S_USB       = 0x00008000,
    S_KER_PLUG  = 0x00010000,
    S_EVENT     = 0x00020000,
    S_IPC       = 0x00040000,
    S_KER_BUF   = 0x00080000,
} DEBUG_SECTION;

typedef enum
{
    DEBUG_STATUS = 1,
    DEBUG_SET_FILTER = 2,
    DEBUG_SET_BUFFER = 3,
    DEBUG_CLEAR_BUFFER = 4,
    DEBUG_DUMP_SEC_ON = 5,
    DEBUG_DUMP_SEC_OFF = 6,
    KERNEL_DEBUGGER_ON = 7,
    KERNEL_DEBUGGER_OFF = 8,
    DEBUG_DUMP_CLOCK_ON = 9,
    DEBUG_DUMP_CLOCK_OFF = 10,
    DEBUG_CLOCK_RESET = 11
} DEBUG_COMMAND;

typedef struct
{
    DWORD dwCmd;     /**< DEBUG_COMMAND: DEBUG_STATUS, DEBUG_SET_FILTER,
                      DEBUG_SET_BUFFER, DEBUG_CLEAR_BUFFER */
    /* used for DEBUG_SET_FILTER */
    DWORD dwLevel;   /**< DEBUG_LEVEL: D_ERROR, D_WARN..., or D_OFF to turn
                       debugging off */
    DWORD dwSection; /**< DEBUG_SECTION: for all sections in driver: S_ALL
                      for partial sections: S_IO, S_MEM... */
    DWORD dwLevelMessageBox; /**< DEBUG_LEVEL to print in a message box */
    /* used for DEBUG_SET_BUFFER */
    DWORD dwBufferSize; /**< size of buffer in kernel */
} WD_DEBUG, WD_DEBUG_V40;

#define DEBUG_USER_BUF_LEN 2048
typedef struct
{
    CHAR cBuffer[DEBUG_USER_BUF_LEN]; /**< buffer to receive debug messages */
} WD_DEBUG_DUMP, WD_DEBUG_DUMP_V40;

typedef struct
{
    CHAR pcBuffer[256];
    DWORD dwLevel;
    DWORD dwSection;
} WD_DEBUG_ADD, WD_DEBUG_ADD_V503;

typedef struct
{
    DWORD hKernelPlugIn;
    CHAR cDriverName[WD_MAX_KP_NAME_LENGTH];
    CHAR cDriverPath[WD_MAX_KP_NAME_LENGTH]; /**< Should be NULL (exists for backward compatibility).
                        * The driver will be searched in the operating
                        * system's drivers/modules directory. */

    PAD_TO_64(hKernelPlugIn) /* 64 bit app as a 4 byte hole here */
    PVOID pOpenData;
    PAD_TO_64(pOpenData)
} WD_KERNEL_PLUGIN, WD_KERNEL_PLUGIN_V40;

/** IOCTL Structures */
typedef enum
{
    WD_DEVICE_PCI = 0x1,
    WD_DEVICE_USB = 0x2
} WD_GET_DEVICE_PROPERTY_OPTION;

typedef struct
{
    union
    {
        HANDLE hDevice;
        PAD_TO_64(hDevice)
        DWORD dwUniqueID;
    } h;
    PVOID pBuf;
    PAD_TO_64(pBuf)
    DWORD dwBytes;
    DWORD dwProperty;
    DWORD dwOptions; /**< WD_GET_DEVICE_PROPERTY_OPTION */
} WD_GET_DEVICE_PROPERTY;

typedef enum {
    WD_STATUS_SUCCESS = 0, /**< [0] Operation completed successfully */
    WD_STATUS_INVALID_WD_HANDLE = (int)0xffffffff, /**< [0xffffffff] */

    WD_WINDRIVER_STATUS_ERROR = 0x20000000L, /**< [0x20000000] */

    WD_INVALID_HANDLE = 0x20000001L, /**< [0x20000001] Invalid WinDriver handle.
                                      This usually occurs when
                                      1. The WinDriver kernel module is
                                         not installed.
                                      2. The user application is trying to
                                         access a WinDriver kernel module
                                         with the wrong name
                                      3. A wrong handle was provided to an
                                         API. */
    WD_INVALID_PIPE_NUMBER = 0x20000002L, /**< [0x20000002] USB: Invalid Pipe
                                           Number. Occurs when the provided
                                           pipe number does not exist or is
                                           not available on the USB device. */
    WD_READ_WRITE_CONFLICT = 0x20000003L, /**< [0x20000003] Request to read from
                                           an OUT (write) pipe or request to
                                           write to an IN (read) pipe */
    WD_ZERO_PACKET_SIZE = 0x20000004L, /**< [0x20000004] Maximum packet size is
                                            zero */
    WD_INSUFFICIENT_RESOURCES = 0x20000005L, /**< [0x20000005] Insufficient
                                              resources.
                                              Occurs when WinDriver is trying
                                              to allocate more memory
                                              than currently available. */
    WD_UNKNOWN_PIPE_TYPE = 0x20000006L, /**< [0x20000006] Obsolete. */
    WD_SYSTEM_INTERNAL_ERROR = 0x20000007L, /**< [0x20000007] Internal System
                                              Error */
    WD_DATA_MISMATCH = 0x20000008L, /**< [0x20000008] Data Mismatch. Occurs when
                                         compiling a 32 bit user application
                                         on a 64-bit platform without adding
                                         the KERNEL_64BIT define. */
    WD_NO_LICENSE = 0x20000009L, /**< 0x[20000009] No License. Occurs either
                                      when no license was entered, license has
                                      expired, license was not entered
                                      correctly.
                                      Contact wd_license@jungo.com for more
                                      assistance. */
    WD_NOT_IMPLEMENTED = 0x2000000aL, /**< [0x2000000a] Function not
                                       implemented.
                                       Occurs when a certain API is not
                                       implemented for the platform that
                                       is being used. */
    WD_KERPLUG_FAILURE = 0x2000000bL, /**< [0x2000000b] Kernel PlugIn failure.
                                       Occurs when trying to open a KP which is
                                       not currently installed, or when the KP
                                       was not implemented correctly. */
    WD_FAILED_ENABLING_INTERRUPT = 0x2000000cL, /**< [0x2000000c]
                                                 Failed Enabling Interrupts.
                                                 Occurs when the OS fails to
                                                 enable interrupts for the
                                                 device or when trying to
                                                 enable MSI interrupts
                                                 for a device that doesn't
                                                 support them.
                                                 Note that an INF file MUST be
                                                 installed in order to enable
                                                 interrupts under Windows.*/
    WD_INTERRUPT_NOT_ENABLED = 0x2000000dL, /**< [0x2000000d] Interrupt not
                                              enabled.
                                              Occurs when calling  WD_IntWait()
                                              with an invalid interrupt
                                              handle, or when trying
                                              to disable a shared
                                              interrupt that was not enabled. */
    WD_RESOURCE_OVERLAP = 0x2000000eL, /**< [0x2000000e] Resource Overlap.
                                            Happens when trying to open the
                                            same device with more than one user
                                            application (when that device was
                                            not defined to be opened by more
                                            than one process). */
    WD_DEVICE_NOT_FOUND = 0x2000000fL, /**< [0x2000000f] Device not found.
                                            Occurs when trying to open a device
                                            that was not detected by the OS. */
    WD_WRONG_UNIQUE_ID = 0x20000010L, /**< [0x20000010] Wrong unique ID (USB).
                                           Occurs when trying to access a USB
                                           device with an invalid unique ID.*/
    WD_OPERATION_ALREADY_DONE = 0x20000011L, /**< [0x20000011] Operation Already
                                                  Done.
                                            Occurs when trying to perform
                                            twice an operation that
                                            should be done once (i.e.
                                            Enabling interrupts) */
    WD_USB_DESCRIPTOR_ERROR = 0x20000012L, /**< [0x20000012] USB Descriptor
                                                Error.
                                            Occurs when WinDriver recieves
                                            faulty USB descriptor data, might
                                            be a result of an invalid parameter
                                            to the WinDriver API or a
                                            result of a bug in the
                                            device's firmware. */
    WD_SET_CONFIGURATION_FAILED = 0x20000013L, /**< [0x20000013] Set
                                                Configuration Failed (USB).
                                                Occurs when WinDriver failed to
                                                set a configuration for a USB
                                                device. */
    WD_CANT_OBTAIN_PDO = 0x20000014L, /**< [0x20000014] Obsolete */
    WD_TIME_OUT_EXPIRED = 0x20000015L, /**< [0x20000015] Time Out Expired.
                                            Occurs when operations that have a
                                            timeout limit run out of time (i.e.
                                            DMA or USB transfers) */
    WD_IRP_CANCELED = 0x20000016L, /**< [0x20000016] IRP Cancelled. Occurs on
                                    Windows when an interrupt was cancelled by
                                    the user in the middle of an operation. */
    WD_FAILED_USER_MAPPING = 0x20000017L, /**< [0x20000017] Failed to map memory
                                           to User Space. Occurs when there is
                                           not enough available memory to map a
                                           PCI device's BARs to user space
                                           memory.
                                           Possible ways to deal with this are
                                           to decrease the BAR size through the
                                           firmware, add more RAM to your
                                           computer or free up more RAM. */
    WD_FAILED_KERNEL_MAPPING = 0x20000018L, /**< [0x20000018] Failed to map
                                            memory to Kernel Space.
                                            Occurs when there is not enough
                                            available memory to map a PCI
                                            device's BARs to kernel space
                                            memory. Possible ways to deal with
                                            this are to decrease the BAR size
                                            through the firmware, add more RAM
                                            to your computer or free up more
                                            RAM. */
    WD_NO_RESOURCES_ON_DEVICE = 0x20000019L, /**< [0x20000019] No Resources On
                                              Device.
                                              Occurs when a user application
                                              expects a resource (i.e.
                                              BAR, Interrupt...) to be
                                              available on a device but
                                              that resource is unavailable.
                                              This is usually a result of how
                                              the device was
                                              designed/programmed. */
    WD_NO_EVENTS = 0x2000001aL, /**< [0x2000001a] No Events. Occurs when calling
                                WD_EventSend(), WD_EventPull() without
                                registering events using
                                WD_EventRegister() beforehand. */
    WD_INVALID_PARAMETER = 0x2000001bL, /**< [0x2000001b] Invalid Parameter.
                                        Occurs when providing an invalid
                                        parameter to an API. Try fixing the
                                        parameters provided to the function. */
    WD_INCORRECT_VERSION = 0x2000001cL, /**< [0x2000001c] Incorrect version.
                                         Occurs when
                                         1. Mixing together components from
                                            different WinDriver versions.
                                         2. Failure to obtain the version of a
                                            Kernel PlugIn */
    WD_TRY_AGAIN = 0x2000001dL, /**< [0x2000001d] Try Again.
                                 Occurs when an operation has failed due to a
                                 device being busy. When trying again
                                 later the operation may succeed. */
    WD_WINDRIVER_NOT_FOUND = 0x2000001eL, /**< [0x2000001e] Obsolete */
    WD_INVALID_IOCTL = 0x2000001fL, /**< [0x2000001f] Invalid IOCTL. Occurs when
                                    providing an invalid API to the
                                    WinDriver kernel module. Use only
                                    APIs that are defined in this user
                                    manual. */
    WD_OPERATION_FAILED = 0x20000020L, /**< [0x20000020] Operation Failed.
                                        Occurs either because of an invalid
                                        input from user, or an internal
                                        system error. */
    WD_INVALID_32BIT_APP = 0x20000021L, /**< [0x20000021] Invalid 32 Bit User
                                         Application.
                                         Occurs when compiling a 32 bit user
                                         application on a 64-bit platform
                                         without adding the KERNEL_64BIT
                                         define. */
    WD_TOO_MANY_HANDLES = 0x20000022L, /**< [0x20000022] Too Many Handles.
                                        Occurs when too many handles to
                                        WinDriver or to a certain WinDriver
                                        feature have been opened. Try closing
                                        some handles to prevent this from
                                        happening. */
    WD_NO_DEVICE_OBJECT = 0x20000023L, /**< [0x20000023] No Device Object.
                                        1. On Windows this usually means that no
                                        INF file was installed for the device.
                                        Please install an INF using the
                                        DriverWizard to resolve this.
                                        2. On Linux this may mean that the
                                        device is controlled by a different
                                        driver and not by WinDriver. A possible
                                        solution would be to blacklist the
                                        other driver in order to allow
                                        WinDriver to take control of the
                                        device. */

    WD_MORE_PROCESSING_REQUIRED = (int)0xC0000016L, /**< [0xC0000016] More
                                                      Processing Required.
                                                    Occurs when a part of the
                                                    operation was complete but
                                                    more calls to an API for
                                                    continuing to process the
                                                    data will be required. */

    /* The following status codes are returned by USBD:
     USBD status types: */
    WD_USBD_STATUS_SUCCESS = 0x00000000L, /**< [0x00000000] Success */
    WD_USBD_STATUS_PENDING = 0x40000000L, /**< [0x40000000] Operation
                                            pending */
    WD_USBD_STATUS_ERROR = (int)0x80000000L, /**< [0x80000000] Error */
    WD_USBD_STATUS_HALTED = (int)0xC0000000L, /**< [0xC0000000] Halted */

    /* USBD status codes: */
    /* @note The following status codes are comprised of one of the status
    * types above and an error code [i.e. 0xXYYYYYYYL - where: X = status type;
    * YYYYYYY = error code].
    * The same error codes may also appear with one of the other status types
    * as well. */

    /* HC (Host Controller) status codes.
    @note These status codes use the WD_USBD_STATUS_HALTED status type]: */
    WD_USBD_STATUS_CRC = (int)0xC0000001L, /**< [0xC0000001] HC status:
                                                 CRC */
    WD_USBD_STATUS_BTSTUFF = (int)0xC0000002L, /**< [0xC0000002] HC status:
                                                Bit stuffing */
    WD_USBD_STATUS_DATA_TOGGLE_MISMATCH = (int)0xC0000003L, /**< [0xC0000003]
                                                            HC status:
                                                            Data toggle
                                                            mismatch */
    WD_USBD_STATUS_STALL_PID = (int)0xC0000004L, /**< [0xC0000004] HC status:
                                                    PID stall */
    WD_USBD_STATUS_DEV_NOT_RESPONDING = (int)0xC0000005L, /**< [0xC0000005]
                                                          HC status:
                                                     Device not responding */
    WD_USBD_STATUS_PID_CHECK_FAILURE = (int)0xC0000006L, /**< [0xC0000006]
                                                         HC status:
                                                        PID check failed */
    WD_USBD_STATUS_UNEXPECTED_PID = (int)0xC0000007L, /**< [0xC0000007]
                                                      HC status:
                                                      Unexpected PID */
    WD_USBD_STATUS_DATA_OVERRUN = (int)0xC0000008L, /**< [0xC0000008]
                                                    HC status:
                                                    Data overrun */
    WD_USBD_STATUS_DATA_UNDERRUN = (int)0xC0000009L, /**< [0xC0000009]
                                                     HC status:
                                                    Data underrrun */
    WD_USBD_STATUS_RESERVED1 = (int)0xC000000AL, /**< [0xC000000A] HC status:
                                                    Reserved1 */
    WD_USBD_STATUS_RESERVED2 = (int)0xC000000BL, /**< [0xC000000B] HC status:
                                                    Reserved1 */
    WD_USBD_STATUS_BUFFER_OVERRUN = (int)0xC000000CL, /**< [0xC000000C]
                                                      HC status:
                                                      Buffer overrun */
    WD_USBD_STATUS_BUFFER_UNDERRUN = (int)0xC000000DL, /**< [0xC000000D]
                                                       HC status:
                                                       Buffer underrun */
    WD_USBD_STATUS_NOT_ACCESSED = (int)0xC000000FL, /**< [0xC000000F] HC status:
                                                Not accessed */
    WD_USBD_STATUS_FIFO = (int)0xC0000010L, /**< [0xC0000010] HC status:
                                                FIFO */

#if defined(WIN32)
    WD_USBD_STATUS_XACT_ERROR = (int)0xC0000011L, /**< [0xC0000011] HC status:
                                                The host controller has
                                                set the Transaction Error
                                                (XactErr)
                                                bit in the transfer
                                                descriptor's status field */
    WD_USBD_STATUS_BABBLE_DETECTED = (int)0xC0000012L, /**< [0xC0000012]
                                                       HC status:
                                                       Babble detected */
    WD_USBD_STATUS_DATA_BUFFER_ERROR = (int)0xC0000013L, /**< [0xC0000013]
                                                         HC status:
                                                         Data buffer error */
#endif

    WD_USBD_STATUS_CANCELED = (int)0xC0010000L, /**< [0xC0010000]
                                                         USBD:
                                                         Transfer canceled */

    /** Returned by HCD (Host Controller Driver) if a transfer is submitted to
    * an endpoint that is stalled: */
    WD_USBD_STATUS_ENDPOINT_HALTED = (int)0xC0000030L,

    /* Software status codes
    @note The following status codes have only the error bit set: */
    WD_USBD_STATUS_NO_MEMORY = (int)0x80000100L, /**< [0x80000100]
                                                         USBD:
                                                         Out of memory */
    WD_USBD_STATUS_INVALID_URB_FUNCTION = (int)0x80000200L, /**< [0x80000200]
                                                         USBD:
                                                         Invalid URB function */
    WD_USBD_STATUS_INVALID_PARAMETER = (int)0x80000300L, /**< [0x80000300]
                                                         USBD:
                                                         Invalid parameter */

    /** [0x80000400] Returned if client driver attempts to close an
        endpoint/interface or configuration with outstanding transfers: */
    WD_USBD_STATUS_ERROR_BUSY = (int)0x80000400L,

    /**  [0x80000500] Returned by USBD if it cannot complete a URB request.
    Typically this will be returned in the URB status field when the Irp is
    completed  with a more specific error code. [The Irp status codes are
    indicated  in WinDriver's Debug Monitor tool (wddebug/wddebug_gui): */
    WD_USBD_STATUS_REQUEST_FAILED = (int)0x80000500L,
    /** [80000600] USBD: Invalid pipe handle */
    WD_USBD_STATUS_INVALID_PIPE_HANDLE = (int)0x80000600L,

    /** [0x80000700] Returned when there is not enough bandwidth available */
    /** to open a requested endpoint: */
    WD_USBD_STATUS_NO_BANDWIDTH = (int)0x80000700L,

    /** [0x80000800] Generic HC (Host Controller) error: */
    WD_USBD_STATUS_INTERNAL_HC_ERROR = (int)0x80000800L,

    /** [0x80000900] Returned when a short packet terminates the transfer */
    /** i.e. USBD_SHORT_TRANSFER_OK bit not set: */
    WD_USBD_STATUS_ERROR_SHORT_TRANSFER = (int)0x80000900L,

    /** [0x80000A00] Returned if the requested start frame is not within */
    /** USBD_ISO_START_FRAME_RANGE of the current USB frame, */
    /** @note The stall bit is set: */
    WD_USBD_STATUS_BAD_START_FRAME = (int)0xC0000A00L,

    /** [0xC0000B00] Returned by HCD (Host Controller Driver) if all packets
        in an isochronous transfer complete with an error: */
    WD_USBD_STATUS_ISOCH_REQUEST_FAILED = (int)0xC0000B00L,

    /** [0xC0000C00] Returned by USBD if the frame length control for a given */
    /** HC (Host Controller) is already taken by another driver: */
    WD_USBD_STATUS_FRAME_CONTROL_OWNED = (int)0xC0000C00L,

    /** [0xC0000D00] Returned by USBD if the caller does not own frame length
        control and attempts to release or modify the HC frame length: */
    WD_USBD_STATUS_FRAME_CONTROL_NOT_OWNED = (int)0xC0000D00L

#if defined(WIN32)
    ,
    /* Additional USB 2.0 software error codes added for USB 2.0: */
    WD_USBD_STATUS_NOT_SUPPORTED = (int)0xC0000E00L,
                   /**< [0xC0000E00] Returned for APIS not
                        supported/implemented */
    WD_USBD_STATUS_INAVLID_CONFIGURATION_DESCRIPTOR = (int)0xC0000F00L, /**<
                                                                  [0xC0000F00]
                                                                USBD:
                                                                Invalid
                                                                configuration
                                                                descriptor */
    WD_USBD_STATUS_INSUFFICIENT_RESOURCES = (int)0xC0001000L,  /**< [0xC0001000]
                                                                USBD:
                                                                Insufficient
                                                                resources */
    WD_USBD_STATUS_SET_CONFIG_FAILED = (int)0xC0002000L,  /**< [0xC0002000]
                                                                USBD:
                                                               Set
                                                               configuration
                                                               failed */
    WD_USBD_STATUS_BUFFER_TOO_SMALL = (int)0xC0003000L,  /**< [0xC0003000]
                                                                USBD:
                                                                Buffer too
                                                                small */
    WD_USBD_STATUS_INTERFACE_NOT_FOUND = (int)0xC0004000L,  /**< [0xC0004000]
                                                                USBD:
                                                                 Interface
                                                                 not found */
    WD_USBD_STATUS_INAVLID_PIPE_FLAGS = (int)0xC0005000L,  /**< [0xC0005000]
                                                                USBD:
                                                                Invalid pipe
                                                                flags */
    WD_USBD_STATUS_TIMEOUT = (int)0xC0006000L,  /**< [0xC0006000]
                                                                USBD:
                                                                Timeout */
    WD_USBD_STATUS_DEVICE_GONE = (int)0xC0007000L,  /**< [0xC0007000]
                                                                USBD:
                                                                Device Gone */
    WD_USBD_STATUS_STATUS_NOT_MAPPED = (int)0xC0008000L,  /**< [0xC0008000]
                                                                USBD:
                                                                 Status not
                                                                 mapped */

     /** Extended isochronous error codes returned by USBD.
     These errors appear in the packet status field of an isochronous
     transfer. */

    /** [0xC0020000] For some reason the controller did not access the TD
        associated with this packet: */
    WD_USBD_STATUS_ISO_NOT_ACCESSED_BY_HW = (int)0xC0020000L,
    /** [0xC0030000] Controller reported an error in the TD. */
    /** Since TD errors are controller specific they are reported */
    /** generically with this error code: */
    WD_USBD_STATUS_ISO_TD_ERROR = (int)0xC0030000L,
    /** [0xC0040000] The packet was submitted in time by the client but */
    /** failed to reach the miniport in time: */
    WD_USBD_STATUS_ISO_NA_LATE_USBPORT = (int)0xC0040000L,
    /** [0xC0050000] The packet was not sent because the client submitted it
        too late to transmit: */
    WD_USBD_STATUS_ISO_NOT_ACCESSED_LATE = (int)0xC0050000L
#endif
} WD_ERROR_CODES;

typedef enum
{
    WD_INSERT                  = 0x1,
    WD_REMOVE                  = 0x2,
    WD_OBSOLETE                = 0x8,  /**< Obsolete */
    WD_POWER_CHANGED_D0        = 0x10, /**< Power states for the power
                                         management */
    WD_POWER_CHANGED_D1        = 0x20,
    WD_POWER_CHANGED_D2        = 0x40,
    WD_POWER_CHANGED_D3        = 0x80,
    WD_POWER_SYSTEM_WORKING    = 0x100,
    WD_POWER_SYSTEM_SLEEPING1  = 0x200,
    WD_POWER_SYSTEM_SLEEPING2  = 0x400,
    WD_POWER_SYSTEM_SLEEPING3  = 0x800,
    WD_POWER_SYSTEM_HIBERNATE  = 0x1000,
    WD_POWER_SYSTEM_SHUTDOWN   = 0x2000,
    WD_IPC_UNICAST_MSG         = 0x4000,
    WD_IPC_MULTICAST_MSG       = 0x8000,
} WD_EVENT_ACTION;

#define WD_IPC_ALL_MSG (WD_IPC_UNICAST_MSG | WD_IPC_MULTICAST_MSG)

typedef enum
{
    WD_ACKNOWLEDGE              = 0x1,
    WD_ACCEPT_CONTROL           = 0x2/**< used in WD_EVENT_SEND (acknowledge) */
} WD_EVENT_OPTION;

#define WD_ACTIONS_POWER (WD_POWER_CHANGED_D0 | WD_POWER_CHANGED_D1 | \
    WD_POWER_CHANGED_D2 | WD_POWER_CHANGED_D3 | WD_POWER_SYSTEM_WORKING | \
    WD_POWER_SYSTEM_SLEEPING1 | WD_POWER_SYSTEM_SLEEPING3 | \
    WD_POWER_SYSTEM_HIBERNATE | WD_POWER_SYSTEM_SHUTDOWN)
#define WD_ACTIONS_ALL (WD_ACTIONS_POWER | WD_INSERT | WD_REMOVE)

enum
{
    WD_EVENT_TYPE_UNKNOWN = 0,
    WD_EVENT_TYPE_PCI     = 1,
    WD_EVENT_TYPE_USB     = 3,
    WD_EVENT_TYPE_IPC     = 4,
};
typedef DWORD WD_EVENT_TYPE;

typedef struct
{
    DWORD hEvent;
    DWORD dwEventType;      /**< WD_EVENT_TYPE */

    DWORD dwAction;         /**< WD_EVENT_ACTION */
    DWORD dwEventId;
    DWORD hKernelPlugIn;
    DWORD dwOptions;        /**< WD_EVENT_OPTION */
    union
    {
        struct
        {
            WD_PCI_ID cardId;
            WD_PCI_SLOT pciSlot;
        } Pci;
        struct
        {
            DWORD dwUniqueID;
        } Usb;
        struct
        {
            DWORD hIpc;        /**< Acts as a unique identifier */
            DWORD dwSubGroupID; /**< Might be identical to same process running
                                 * twice (User implementation dependant) */
            DWORD dwGroupID;

            DWORD dwSenderUID;
            DWORD dwMsgID;
            PAD_TO_64(dwMsgID)
            UINT64 qwMsgData;
        } Ipc;
    } u;

    DWORD dwNumMatchTables;
    WDU_MATCH_TABLE matchTables[1];
} WD_EVENT, WD_EVENT_V121;

typedef struct
{
    DWORD applications_num;
    DWORD devices_num;
} WD_USAGE;

enum
{
    WD_USB_HARD_RESET = 1,
    WD_USB_CYCLE_PORT = 2
};

#ifndef BZERO
    #define BZERO(buf) memset(&(buf), 0, sizeof(buf))
#endif

#ifndef INVALID_HANDLE_VALUE
    #define INVALID_HANDLE_VALUE ((HANDLE)(-1))
#endif

#ifndef CTL_CODE
    #define CTL_CODE(DeviceType, Function, Method, Access) ( \
        ((DeviceType)<<16) | ((Access)<<14) | ((Function)<<2) | (Method) \
    )

    #define METHOD_BUFFERED   0
    #define METHOD_IN_DIRECT  1
    #define METHOD_OUT_DIRECT 2
    #define METHOD_NEITHER    3
    #define FILE_ANY_ACCESS   0
    #define FILE_READ_ACCESS  1    /**< file & pipe */
    #define FILE_WRITE_ACCESS 2    /**< file & pipe */
#endif

#if defined(LINUX) && defined(KERNEL_64BIT)
    #define WD_TYPE 0
    #define WD_CTL_CODE(wFuncNum) \
        _IOC(_IOC_READ|_IOC_WRITE, WD_TYPE, wFuncNum, 0)
    #define WD_CTL_DECODE_FUNC(IoControlCode) _IOC_NR(IoControlCode)
    #define WD_CTL_DECODE_TYPE(IoControlCode) _IOC_TYPE(IoControlCode)
#elif defined(UNIX)
    #define WD_TYPE 0
    #define WD_CTL_CODE(wFuncNum) (wFuncNum)
    #define WD_CTL_DECODE_FUNC(IoControlCode) (IoControlCode)
    #define WD_CTL_DECODE_TYPE(IoControlCode) (WD_TYPE)
#else
    /** Device type */
    #define WD_TYPE 38200
    #if defined(KERNEL_64BIT)
        #define FUNC_MASK 0x400
    #else
        #define FUNC_MASK 0x0
    #endif
    #define WD_CTL_CODE(wFuncNum) CTL_CODE(WD_TYPE, (wFuncNum | FUNC_MASK), \
        METHOD_NEITHER, FILE_ANY_ACCESS)
    #define WD_CTL_DECODE_FUNC(IoControlCode) ((IoControlCode >> 2) & 0xfff)
    #define WD_CTL_DECODE_TYPE(IoControlCode) \
        DEVICE_TYPE_FROM_CTL_CODE(IoControlCode)
#endif

#if defined(LINUX)
    #define WD_CTL_IS_64BIT_AWARE(IoControlCode) \
        (_IOC_DIR(IoControlCode) & (_IOC_READ|_IOC_WRITE))
#elif defined(UNIX)
    #define WD_CTL_IS_64BIT_AWARE(IoControlCode) TRUE
#else
    #define WD_CTL_IS_64BIT_AWARE(IoControlCode) \
        (WD_CTL_DECODE_FUNC(IoControlCode) & FUNC_MASK)
#endif

/* WinDriver function IOCTL calls. For details on the WinDriver functions, */
/* see the WinDriver manual or included help files. */

#define IOCTL_WD_KERNEL_BUF_LOCK                    WD_CTL_CODE(0x9f3)
#define IOCTL_WD_KERNEL_BUF_UNLOCK                  WD_CTL_CODE(0x9f4)
#define IOCTL_WD_DMA_LOCK                           WD_CTL_CODE(0x9be)
#define IOCTL_WD_DMA_UNLOCK                         WD_CTL_CODE(0x902)
#define IOCTL_WD_TRANSFER                           WD_CTL_CODE(0x98c)
#define IOCTL_WD_MULTI_TRANSFER                     WD_CTL_CODE(0x98d)
#define IOCTL_WD_PCI_SCAN_CARDS                     WD_CTL_CODE(0x9fa)
#define IOCTL_WD_PCI_GET_CARD_INFO                  WD_CTL_CODE(0x9e8)
#define IOCTL_WD_VERSION                            WD_CTL_CODE(0x910)
#define IOCTL_WD_PCI_CONFIG_DUMP                    WD_CTL_CODE(0x91a)
#define IOCTL_WD_KERNEL_PLUGIN_OPEN                 WD_CTL_CODE(0x91b)
#define IOCTL_WD_KERNEL_PLUGIN_CLOSE                WD_CTL_CODE(0x91c)
#define IOCTL_WD_KERNEL_PLUGIN_CALL                 WD_CTL_CODE(0x91d)
#define IOCTL_WD_INT_ENABLE                         WD_CTL_CODE(0x9b6)
#define IOCTL_WD_INT_DISABLE                        WD_CTL_CODE(0x9bb)
#define IOCTL_WD_INT_COUNT                          WD_CTL_CODE(0x9ba)
#define IOCTL_WD_SLEEP                              WD_CTL_CODE(0x927)
#define IOCTL_WD_DEBUG                              WD_CTL_CODE(0x928)
#define IOCTL_WD_DEBUG_DUMP                         WD_CTL_CODE(0x929)
#define IOCTL_WD_CARD_UNREGISTER                    WD_CTL_CODE(0x9e7)
#define IOCTL_WD_CARD_REGISTER                      WD_CTL_CODE(0x9e6)
#define IOCTL_WD_INT_WAIT                           WD_CTL_CODE(0x9b9)
#define IOCTL_WD_LICENSE                            WD_CTL_CODE(0x9f9)
#define IOCTL_WD_EVENT_REGISTER                     WD_CTL_CODE(0x9ef)
#define IOCTL_WD_EVENT_UNREGISTER                   WD_CTL_CODE(0x9f0)
#define IOCTL_WD_EVENT_PULL                         WD_CTL_CODE(0x9f1)
#define IOCTL_WD_EVENT_SEND                         WD_CTL_CODE(0x9f2)
#define IOCTL_WD_DEBUG_ADD                          WD_CTL_CODE(0x964)
#define IOCTL_WD_USAGE                              WD_CTL_CODE(0x976)
#define IOCTL_WDU_GET_DEVICE_DATA                   WD_CTL_CODE(0x9a7)
#define IOCTL_WDU_SET_INTERFACE                     WD_CTL_CODE(0x981)
#define IOCTL_WDU_RESET_PIPE                        WD_CTL_CODE(0x982)
#define IOCTL_WDU_TRANSFER                          WD_CTL_CODE(0x983)
#define IOCTL_WDU_HALT_TRANSFER                     WD_CTL_CODE(0x985)
#define IOCTL_WDU_WAKEUP                            WD_CTL_CODE(0x98a)
#define IOCTL_WDU_RESET_DEVICE                      WD_CTL_CODE(0x98b)
#define IOCTL_WD_GET_DEVICE_PROPERTY                WD_CTL_CODE(0x990)
#define IOCTL_WD_CARD_CLEANUP_SETUP                 WD_CTL_CODE(0x995)
#define IOCTL_WD_DMA_SYNC_CPU                       WD_CTL_CODE(0x99f)
#define IOCTL_WD_DMA_SYNC_IO                        WD_CTL_CODE(0x9a0)
#define IOCTL_WDU_STREAM_OPEN                       WD_CTL_CODE(0x9a8)
#define IOCTL_WDU_STREAM_CLOSE                      WD_CTL_CODE(0x9a9)
#define IOCTL_WDU_STREAM_START                      WD_CTL_CODE(0x9af)
#define IOCTL_WDU_STREAM_STOP                       WD_CTL_CODE(0x9b0)
#define IOCTL_WDU_STREAM_FLUSH                      WD_CTL_CODE(0x9aa)
#define IOCTL_WDU_STREAM_GET_STATUS                 WD_CTL_CODE(0x9b5)
#define IOCTL_WDU_SELECTIVE_SUSPEND                 WD_CTL_CODE(0x9ae)
#define IOCTL_WD_PCI_SCAN_CAPS                      WD_CTL_CODE(0x9e5)
#define IOCTL_WD_IPC_REGISTER                       WD_CTL_CODE(0x9eb)
#define IOCTL_WD_IPC_UNREGISTER                     WD_CTL_CODE(0x9ec)
#define IOCTL_WD_IPC_SCAN_PROCS                     WD_CTL_CODE(0x9ed)
#define IOCTL_WD_IPC_SEND                           WD_CTL_CODE(0x9ee)
#define IOCTL_WD_PCI_SRIOV_ENABLE                   WD_CTL_CODE(0x9f5)
#define IOCTL_WD_PCI_SRIOV_DISABLE                  WD_CTL_CODE(0x9f6)
#define IOCTL_WD_PCI_SRIOV_GET_NUMVFS               WD_CTL_CODE(0x9f7)
#define IOCTL_WD_IPC_SHARED_INT_ENABLE              WD_CTL_CODE(0x9fc)
#define IOCTL_WD_IPC_SHARED_INT_DISABLE             WD_CTL_CODE(0x9fd)
#define IOCTL_WD_DMA_TRANSACTION_INIT               WD_CTL_CODE(0x9fe)
#define IOCTL_WD_DMA_TRANSACTION_EXECUTE            WD_CTL_CODE(0x9ff)
#define IOCTL_WD_DMA_TRANSFER_COMPLETED_AND_CHECK   WD_CTL_CODE(0xa00)
#define IOCTL_WD_DMA_TRANSACTION_RELEASE            WD_CTL_CODE(0xa01)

#if defined(UNIX)
    typedef struct
    {
        DWORD dwHeader;
        DWORD dwSize;
        PVOID pData;
        PAD_TO_64(pData)
    } WD_IOCTL_HEADER;

    #define WD_IOCTL_HEADER_CODE 0xa410b413UL
#endif

#if defined(__KERNEL__)
    HANDLE __cdecl WD_Open(void);
    void __cdecl WD_Close(HANDLE hWD);
    DWORD __cdecl KP_DeviceIoControl(DWORD dwFuncNum, HANDLE h, PVOID pParam,
        DWORD dwSize);
    #define WD_FUNCTION(wFuncNum, h, pParam, dwSize, fWait) \
        KP_DeviceIoControl((DWORD)wFuncNum, h, (PVOID)pParam, (DWORD)dwSize)
#else
    #define REGKEY_BUFSIZE 256
    #define OS_CAN_NOT_DETECT_TEXT "OS CAN NOT DETECT"
    #define INSTALLATION_TYPE_NOT_DETECT_TEXT "unknown"
    typedef struct
    {
        CHAR cProdName[REGKEY_BUFSIZE];
        CHAR cInstallationType[REGKEY_BUFSIZE];
        #ifdef WIN32
            CHAR cCurrentVersion[REGKEY_BUFSIZE];
            CHAR cBuild[REGKEY_BUFSIZE];
            CHAR cCsdVersion[REGKEY_BUFSIZE];
            DWORD dwMajorVersion;
            DWORD dwMinorVersion;
        #else
            CHAR cRelease[REGKEY_BUFSIZE];
            CHAR cReleaseVersion[REGKEY_BUFSIZE];
        #endif
   } WD_OS_INFO;

    /**
     *   Retrieves the type of the operating system in section.
     *
     *   @return
     *    Returns the type of the running operating system.
     *    If the operating system type is not detected,
     *    cProdName field will get a OS_CAN_NOT_DETECT_TEXT value.
     */
    WD_OS_INFO DLLCALLCONV get_os_type(void);

    /**
     * Checks whether the Secure Boot feature is enabled on the system.
     * Developing drivers while Secure Boot is enabled can result in driver
     * installation problems.
     *
     *   @return
     *    WD_STATUS_SUCCESS (0) when Secure Boot is enabled.
     *    WD_WINDRIVER_STATUS_ERROR when Secure Boot is disabled.
     *    Any other status when Secure Boot is not supported.
     */
    DWORD DLLCALLCONV check_secureBoot_enabled(void);
    #if defined(APPLE)
        DWORD WD_FUNCTION_LOCAL(int wFuncNum, HANDLE h,
            PVOID pParam, DWORD dwSize, BOOL fWait);

        HANDLE WD_OpenLocal(void);

        void WD_CloseLocal(HANDLE h);

        #define WD_OpenStreamLocal(read,sync) INVALID_HANDLE_VALUE

        #define WD_UStreamRead(hFile, pBuffer, dwNumberOfBytesToRead, \
            dwNumberOfBytesRead)\
            WD_NOT_IMPLEMENTED

        #define WD_UStreamWrite(hFile, pBuffer, dwNumberOfBytesToWrite, \
            dwNumberOfBytesWritten)\
            WD_NOT_IMPLEMENTED

    #elif defined(UNIX)
        static inline ULONG WD_FUNCTION_LOCAL(int wFuncNum, HANDLE h,
            PVOID pParam, DWORD dwSize, BOOL fWait)
        {
            WD_IOCTL_HEADER ioctl_hdr;

            BZERO(ioctl_hdr);
            ioctl_hdr.dwHeader = WD_IOCTL_HEADER_CODE;
            ioctl_hdr.dwSize = dwSize;
            ioctl_hdr.pData = pParam;
            (void)fWait;
            #if defined(LINUX)
                return (ULONG)ioctl((int)(long)h, wFuncNum, &ioctl_hdr);
            #endif
        }

            #define WD_OpenLocal()\
                ((HANDLE)(long)open(WD_DRIVER_NAME, O_RDWR | O_SYNC))
            #define WD_OpenStreamLocal(read,sync) \
                ((HANDLE)(long)open(WD_DRIVER_NAME, \
                    ((read) ? O_RDONLY : O_WRONLY) | \
                    ((sync) ? O_SYNC : O_NONBLOCK)))

        #define WD_CloseLocal(h) close((int)(long)(h))

        #define WD_UStreamRead(hFile, pBuffer, dwNumberOfBytesToRead, \
            dwNumberOfBytesRead)\
            WD_NOT_IMPLEMENTED

        #define WD_UStreamWrite(hFile, pBuffer, dwNumberOfBytesToWrite, \
            dwNumberOfBytesWritten)\
            WD_NOT_IMPLEMENTED

    #elif defined(WIN32)
        #define WD_CloseLocal(h) CloseHandle(h)

        #define WD_UStreamRead(hFile, pBuffer, dwNumberOfBytesToRead, \
            dwNumberOfBytesRead)\
            ReadFile(hFile, pBuffer, dwNumberOfBytesToRead, \
                dwNumberOfBytesRead, NULL) ? WD_STATUS_SUCCESS : \
                WD_OPERATION_FAILED

        #define WD_UStreamWrite(hFile, pBuffer, dwNumberOfBytesToWrite, \
            dwNumberOfBytesWritten)\
            WriteFile(hFile, pBuffer, dwNumberOfBytesToWrite, \
                dwNumberOfBytesWritten, NULL) ? WD_STATUS_SUCCESS : \
                WD_OPERATION_FAILED

        #if defined(WIN32)
            #define WD_OpenLocal()\
                CreateFileA(\
                    WD_DRIVER_NAME,\
                    GENERIC_READ,\
                    FILE_SHARE_READ | FILE_SHARE_WRITE,\
                    NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)

            #define WD_OpenStreamLocal(read,sync) \
                CreateFileA(\
                    WD_DRIVER_NAME,\
                    (read) ? GENERIC_READ : GENERIC_WRITE,\
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, \
                    (sync) ? 0 : FILE_FLAG_OVERLAPPED, NULL)

            static DWORD WD_FUNCTION_LOCAL(int wFuncNum, HANDLE h, PVOID pParam,
                DWORD dwSize, BOOL fWait)
            {
                DWORD dwTmp;
                HANDLE hWD = fWait ? WD_OpenLocal() : h;
                DWORD rc = WD_WINDRIVER_STATUS_ERROR;

                if (hWD == INVALID_HANDLE_VALUE)
                    return (DWORD)WD_STATUS_INVALID_WD_HANDLE;
		
                DeviceIoControl(hWD, (DWORD)wFuncNum, pParam, dwSize, &rc,
                    sizeof(DWORD), &dwTmp, NULL);

                if (fWait)
                    WD_CloseLocal(hWD);

                return rc;
            }
        #endif
    #endif

    #define WD_FUNCTION WD_FUNCTION_LOCAL
    #define WD_Close WD_CloseLocal
    #define WD_Open WD_OpenLocal
    #define WD_StreamOpen WD_OpenStreamLocal
    #define WD_StreamClose WD_CloseLocal
#endif

#define SIZE_OF_WD_DMA(pDma) \
    ((DWORD)(sizeof(WD_DMA) + ((pDma)->dwPages <= WD_DMA_PAGES ? \
        0 : ((pDma)->dwPages - WD_DMA_PAGES) * sizeof(WD_DMA_PAGE))))
#define SIZE_OF_WD_EVENT(pEvent) \
    ((DWORD)(sizeof(WD_EVENT) + ((pEvent)->dwNumMatchTables > 0 ? \
    sizeof(WDU_MATCH_TABLE) * ((pEvent)->dwNumMatchTables - 1) : 0)))

/**
* Sets debugging level for collecting debug messages.
*   @param [in] h:      Handle to WinDriver's kernel-mode driver as received
*                       from WD_Open().
*   @param [in] pDebug: Pointer to a debug information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet lowlevel_examples.c WD_Debug
*/
#define WD_Debug(h,pDebug)\
    WD_FUNCTION(IOCTL_WD_DEBUG, h, pDebug, sizeof(WD_DEBUG), FALSE)

/**
* Retrieves debug messages buffer.
*   @param [in] h:      Handle to WinDriver's kernel-mode driver as received
*                       from WD_Open().
*   @param [in] pDebugDump: Pointer to a debug information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet lowlevel_examples.c WD_DebugDump
*/
#define WD_DebugDump(h,pDebugDump)\
    WD_FUNCTION(IOCTL_WD_DEBUG_DUMP, h, pDebugDump, sizeof(WD_DEBUG_DUMP), \
        FALSE)

/**
* Sends debug messages to the debug log. Used by the driver code.
*   @param [in] h:         Handle to WinDriver's kernel-mode driver as received
*                          from WD_Open().
*   @param [in] pDebugAdd: Pointer to an additional debug information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet lowlevel_examples.c WD_DebugAdd
*/
#define WD_DebugAdd(h, pDebugAdd)\
    WD_FUNCTION(IOCTL_WD_DEBUG_ADD, h, pDebugAdd, sizeof(WD_DEBUG_ADD), FALSE)

/**
* Executes a single read/write instruction to an I/O port or to a memory
* address.
*   @param [in] h:         Handle to WinDriver's kernel-mode driver as received
*                          from WD_Open().
*   @param [in,out] pTransfer: Pointer to a transfer information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet lowlevel_examples.c WD_Transfer
*/
#define WD_Transfer(h,pTransfer)\
    WD_FUNCTION(IOCTL_WD_TRANSFER, h, pTransfer, sizeof(WD_TRANSFER), FALSE)

/**
* Executes multiple read/write instructions to I/O ports and/or memory
* addresses.
*   @param [in] h:                  Handle to WinDriver's kernel-mode driver
*                                   as received from WD_Open().
*   @param [in] pTransferArray:     Pointer to a beginning of an array of
*                                   transfer information structures
*   @param [in,out] dwNumTransfers: The number of transfers to perform
*                                   (the pTransferArray array should contain at
*                                   least dwNumTransfers elements)
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet lowlevel_examples.c WD_MultiTransfer
*/
#define WD_MultiTransfer(h, pTransferArray, dwNumTransfers) \
    WD_FUNCTION(IOCTL_WD_MULTI_TRANSFER, h, pTransferArray, \
        sizeof(WD_TRANSFER) * (dwNumTransfers), FALSE)

/**
* Allocates a contiguous or non-contiguous non-paged kernel buffer, and maps
* it to user address space. This buffer should be used ONLY for shared buffer
* purposes (The buffer should NOT beused for DMA).
*   @param [in] h:           Handle to WinDriver's kernel-mode driver as
*                            received from WD_Open().
*   @param [in,out] pKerBuf: Pointer to a WD_KERNEL_BUFFER information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet lowlevel_examples.c WD_KernelBufLock
*/
#define WD_KernelBufLock(h, pKerBuf)\
    WD_FUNCTION(IOCTL_WD_KERNEL_BUF_LOCK, h, pKerBuf, \
        sizeof(WD_KERNEL_BUFFER), FALSE)

/**
* Frees kernel buffer.
*   @param [in] h:       Handle to WinDriver's kernel-mode driver as received
*                        from WD_Open().
*   @param [in] pKerBuf: Pointer to a WD_KERNEL_BUFFER information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet lowlevel_examples.c WD_KernelBufUnlock
*/
#define WD_KernelBufUnlock(h, pKerBuf)\
    WD_FUNCTION(IOCTL_WD_KERNEL_BUF_UNLOCK, h, pKerBuf, \
        sizeof(WD_KERNEL_BUFFER), FALSE)

/**
* Enables contiguous-buffer or Scatter/Gather DMA.
*
* For contiguous-buffer DMA, the function allocates a DMA buffer and returns
* mappings of the allocated buffer to physical address space and to user-mode
* and kernel virtual address spaces.
*
* For Scatter/Gather DMA, the function receives the address of a data buffer
* allocated in the usermode, locks it for DMA, and returns the corresponding
* physical mappings of the locked DMA pages. On Windows the function also
* returns a kernel-mode mapping of the buffer.
*
*   @param [in] h:        Handle to WinDriver's kernel-mode driver as received
*                         from WD_Open().
*   @param [in,out] pDma: Pointer to a DMA information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @remarks
*  WinDriver supports both Scatter/Gather and contiguous-buffer DMA on Windows
*  and Linux. On Linux, Scatter/Gather DMA is only supported for 2.4 kernels
*  and above (since the 2.2 Linux kernels require a patch to support this type
*  of DMA).
*
*  You should NOT use the physical memory address returned by the function
*  (dma.Page[i].pPhysicalAddr) directly in order to access the DMA buffer from
*  your driver.
*  To access the memory directly from a user-mode process, use the user-mode
*  virtual mapping of the DMA buffer dma.pUserAddr.
*  To access the memory in the kernel, either directly from within a
*  Kernel PlugIn driver (see the WinDriver PCI Manual) or when calling
*  WD_Transfer() / WD_MultiTransfer(), use the kernel mapping of
*  the DMA buffer. For contiguous-buffer DMA
*  (dma.dwOptions | DMA_KERNEL_BUFFER_ALLOC) and for Scatter/Gather DMA on
*  Windows, this mapping is returned by WD_DMALock() within the dma.pKernelAddr
*  field. For Scatter/Gather DMA on other platforms, you can acquire a kernel
*  mapping of the buffer by calling WD_CardRegister() with a card
*  structure that contains a memory item defined with the physical DMA buffer
*  address returned from WD_DMALock() (dma.Page[i].pPhysicalAddr).
*  WD_CardRegister() will return a kernel mapping of the physical buffer within
*  the pCardReg->Card.Item[i].I.Mem.pTransAddr field.
*
*  On Windows x86 and x86_64 platforms, you should normally set the
*  DMA_ALLOW_CACHE flag in the DMA options bitmask parameter (pDma->dwOptions).
*
*  If the device supports 64-bit DMA addresses, it is recommended to set the
*  DMA_ALLOW_64BIT_ADDRESS flag in pDma->dwOptions. Otherwise, when the
*  physical memory on the target platform is larger than 4GB, the operating
*  system may only allow allocation of relatively small 32-bit DMA buffers
*  (such as 1MB buffers, or even smaller).
*
*  When using the DMA_LARGE_BUFFER flag, dwPages is an input/output parameter.
*  As input to WD_DMALock(), dwPages should be set to the maximum number of
*  pages that can be used for the DMA buffer (normally this would be the number
*  of elements in the dma.Page array). As an output value of WD_DMALock(),
*  dwPages holds the number of actual physical blocks allocated for the DMA
*  buffer. The returned dwPages may be smaller than the input value because
*  adjacent pages are returned as one block.
*
* @snippet lowlevel_examples.c WD_DMALock
*/
#define WD_DMALock(h,pDma)\
    WD_FUNCTION(IOCTL_WD_DMA_LOCK, h, pDma, SIZE_OF_WD_DMA(pDma), FALSE)

/**
* Unlocks a DMA buffer
*
*   @param [in] h:    Handle to WinDriver's kernel-mode driver as received
*                     from WD_Open().
*   @param [in] pDma: Pointer to a DMA information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet lowlevel_examples.c WD_DMAUnlock
*/
#define WD_DMAUnlock(h,pDma)\
    WD_FUNCTION(IOCTL_WD_DMA_UNLOCK, h, pDma, SIZE_OF_WD_DMA(pDma), FALSE)

/**
*  Initializes the transaction, allocates a DMA buffer,
*  locks it in physical memory, and returns mappings of the allocated buffer
*  to physical address space and to user-mode and kernel
*  virtual address spaces.
*
*   @param [in] h:    Handle to WinDriver's kernel-mode driver as received
*                     from WD_Open().
*   @param [in,out] pDma:    Pointer to a DMA information structure
*
* @return  Returns WD_STATUS_SUCCESS (0) on success,
*   or an appropriate error code otherwise
*/
#define WD_DMATransactionInit(h,pDma)\
    WD_FUNCTION(IOCTL_WD_DMA_TRANSACTION_INIT, h, pDma, SIZE_OF_WD_DMA(pDma), \
        FALSE)

/**
*  Begins the execution of a specified DMA transaction.
*
*   @param [in] h:    Handle to WinDriver's kernel-mode driver as received
*                     from WD_Open().
*   @param [in] pDma: Pointer to a DMA information structure
*
* @return  Returns WD_STATUS_SUCCESS (0) on success,
*   or an appropriate error code otherwise
*/
#define WD_DMATransactionExecute(h,pDma)\
    WD_FUNCTION(IOCTL_WD_DMA_TRANSACTION_EXECUTE, h, pDma, \
        SIZE_OF_WD_DMA(pDma), FALSE)

/**
*  Notifies WinDriver that a device's DMA transfer operation is completed.
*
*   @param [in] h:    Handle to WinDriver's kernel-mode driver as received
*                     from WD_Open().
*   @param [in] pDma: Pointer to a DMA information structure
*
* @return  Returns WD_STATUS_SUCCESS (0) on success,
*   or an appropriate error code otherwise
*/
#define WD_DMATransferCompletedAndCheck(h,pDma)\
    WD_FUNCTION(IOCTL_WD_DMA_TRANSFER_COMPLETED_AND_CHECK, h, pDma, \
        SIZE_OF_WD_DMA(pDma), FALSE)

/**
*  Terminates a specified DMA transaction without deleting the associated
*  WD_DMA transaction structure.
*
*   @param [in] h:    Handle to WinDriver's kernel-mode driver as received
*                     from WD_Open().
*   @param [in] pDma: Pointer to a DMA information structure
*
* @return  Returns WD_STATUS_SUCCESS (0) on success,
*   or an appropriate error code otherwise
*/
#define WD_DMATransactionRelease(h,pDma)\
    WD_FUNCTION(IOCTL_WD_DMA_TRANSACTION_RELEASE, h, pDma, \
        SIZE_OF_WD_DMA(pDma), FALSE)

/**
*  Unlocks and frees the memory allocated for a DMA buffer transaction by a
*  previous call to WD_DMATransactionInit()
*
*   @param [in] h:    Handle to WinDriver's kernel-mode driver as received
*                     from WD_Open().
*   @param [in] pDma: Pointer to a DMA information structure
*
* @return  Returns WD_STATUS_SUCCESS (0) on success,
*   or an appropriate error code otherwise
*/
#define WD_DMATransactionUninit(h,pDma)\
    WD_FUNCTION(IOCTL_WD_DMA_UNLOCK, h, pDma, SIZE_OF_WD_DMA(pDma), FALSE)

/**
* Synchronizes the cache of all CPUs with the DMA buffer, by flushing the data
* from the CPU caches.
*
*   @param [in] h:    Handle to WinDriver's kernel-mode driver as received
*                     from WD_Open()
*   @param [in] pDma: Pointer to a DMA information structure, received from a
*                     previous call to WD_DMALock()
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @remarks
*  An asynchronous DMA read or write operation accesses data in memory, not in
*  the processor (CPU) cache, which resides between the CPU and the host's
*  physical memory. Unless the CPU cache has been flushed, by calling
*  WD_DMASyncCpu(), just before a read transfer, the data transferred into
*  system memory by the DMA operation could be overwritten with stale data if
*  the CPU cache is flushed later. Unless the CPU cache has been flushed by
*  calling WD_DMASyncCpu() just before a write transfer, the data in the CPU
*  cache might be more upto-date than the copy in memory.
*
* @snippet lowlevel_examples.c WD_DMASyncCpu
*/
#define WD_DMASyncCpu(h,pDma)\
    WD_FUNCTION(IOCTL_WD_DMA_SYNC_CPU, h, pDma, SIZE_OF_WD_DMA(pDma), FALSE)

/**
* Synchronizes the I/O caches with the DMA buffer, by flushing the data from
*  the I/O caches and updating the CPU caches.
*
*   @param [in] h:    Handle to WinDriver's kernel-mode driver as received
*                     from WD_Open()
*   @param [in] pDma: Pointer to a DMA information structure, received from a
*                     previous call to WD_DMALock()
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @remarks
*  After a DMA transfer has been completed, the data can still be in the I/O
*  cache, which resides between the host's physical memory and the bus-master
*  DMA device, but not yet in the host's main memory. If the CPU accesses the
*  memory, it might read the wrong data from the CPU cache. To ensure a
*  consistent view of the memory for the CPU, you should call WD_DMASyncIo()
*  after a DMA transfer in order to flush the data from the I/O cache and
*  update the CPU cache with the new data. The function also flushes additional
*  caches and buffers between the device and memory, such as caches associated
*  with bus extenders or bridges.
*
* @snippet lowlevel_examples.c WD_DMASyncIo
*/
#define WD_DMASyncIo(h,pDma)\
    WD_FUNCTION(IOCTL_WD_DMA_SYNC_IO, h, pDma, SIZE_OF_WD_DMA(pDma), FALSE)

/**
* Card registration function.
* The function:
*   Maps the physical memory ranges to be accessed by kernel-mode processes
*   and user-mode applications.
*
*   Verifies that none of the registered device resources
*   (set in pCardReg->Card.Item) are already locked for exclusive use.
*   A resource can be locked for exclusive use by setting the fNotSharable
*   field of its WD_ITEMS structure (pCardReg->Card.Item[i]) to 1, before
*   calling WD_CardRegister().
*
*   Saves data regarding the interrupt request (IRQ) number and the interrupt
*   type in internal data structures; this data will later be used by
*   InterruptEnable() and/or WD_IntEnable()
*
*   @param [in] h:         Handle to WinDriver's kernel-mode driver as received
*                          from WD_Open()
*   @param [in,out] pCard: Pointer to a card registration information
*                           structure
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @remarks
* 1. For PCI the cardReg.Card input resources information should be retrieved
*    from the Plug-and-Play Manager via WD_PciGetCardInfo()
* 2. If your card has a large memory range that cannot be fully mapped to the
*    kernel address space, you can set the WD_ITEM_MEM_DO_NOT_MAP_KERNEL
*    flag in the I.Mem.dwOptions field of the relevant WD_ITEMS
*    memory resource structure that you pass to the card registration function
*    (pCardReg->Card.Item[i].I.Mem.dwOptions). This flag instructs the function
*    to map the memory range only to the user-mode virtual address space,
*    and not to the kernel address space. (For PCI devices, you can modify the
*    relevant item in the card information structure (pCard) that you received
*    from WD_PciGetCardInfo() before passing this structure to
*    WD_CardRegister().)
*
*     Note that if you select to set the WD_ITEM_MEM_DO_NOT_MAP_KERNEL flag,
*     WD_CardRegister() will not update the item's pTransAddr field with a
*     kernel mapping of the memory's base address, and you will therefore not
*     be able to rely on this mapping in calls to WinDriver APIs namely
*     interrupt handling APIs or any API called from a Kernel PlugIn driver
*
* 3. WD_CardRegister() enables the user to map the card memory resources
*    into virtual memory and access them as regular pointers.
*
* @snippet lowlevel_examples.c WD_CardRegister
*/
#define WD_CardRegister(h,pCard)\
    WD_FUNCTION(IOCTL_WD_CARD_REGISTER, h, pCard, sizeof(WD_CARD_REGISTER),\
        FALSE)

/**
* Unregisters a device and frees the resources allocated to it
*
*   @param [in] h:     Handle to WinDriver's kernel-mode driver as received
*                      from WD_Open()
*   @param [in] pCard: Pointer to a card registration information
*                      structure
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet lowlevel_examples.c WD_CardUnregister
*/
#define WD_CardUnregister(h,pCard)\
    WD_FUNCTION(IOCTL_WD_CARD_UNREGISTER, h, pCard, sizeof(WD_CARD_REGISTER),\
        FALSE)

/**
*  Registers an application with WinDriver IPC.
*
*   @param [in] h:     Handle to WinDriver's kernel-mode driver as received
*                      from WD_Open()
*   @param [in,out] pIpcRegister:  Pointer to the IPC information struct
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @remarks
*         You should choose your user applications a unique group ID parameter.
*         This is done as a precaution to prevent several applications that use
*         WinDriver with its default driver name (windrvrXXXX) to get mixing
*         messages. We strongly recommend that you rename your driver before
*         distributing it to avoid this issue entirely, among other issue
*         (See Section 15.2 on renaming you driver name).
*         The sub-group id parameter should identify your user application
*         type in case you have several types that may work simultaneously.
*/
#define WD_IpcRegister(h, pIpcRegister) \
    WD_FUNCTION(IOCTL_WD_IPC_REGISTER, h, pIpcRegister, \
        sizeof(WD_IPC_REGISTER), FALSE)

/**
*  This function unregisters the user application from WinDriver IPC.
*
*   @param [in] h:     Handle to WinDriver's kernel-mode driver as received
*                      from WD_Open()
*   @param [in] pProcInfo:  Pointer to the process information struct
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*/
#define WD_IpcUnRegister(h, pProcInfo) \
    WD_FUNCTION(IOCTL_WD_IPC_UNREGISTER, h, pProcInfo, sizeof(WD_IPC_PROCESS), \
        FALSE)

/**
*  Scans and returns information of all WinDriver IPC registered processes that
*  share the application process groupID
*
*   @param [in] h:     Handle to WinDriver's kernel-mode driver as received
*                      from WD_Open()
*   @param [in] pIpcScanProcs:  Pointer to the processes info struct
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*/
#define WD_IpcScanProcs(h, pIpcScanProcs) \
    WD_FUNCTION(IOCTL_WD_IPC_SCAN_PROCS, h, pIpcScanProcs, \
        sizeof(WD_IPC_SCAN_PROCS), FALSE)

/**
*  Sends a message to other processes, depending on the content of the pIpcSend
*  struct
*
*   @param [in] h:     Handle to WinDriver's kernel-mode driver as received
*                      from WD_Open()
*   @param [in] pIpcSend:  Pointer to the IPC message info struct
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*/
#define WD_IpcSend(h, pIpcSend) \
    WD_FUNCTION(IOCTL_WD_IPC_SEND, h, pIpcSend, sizeof(WD_IPC_SEND), FALSE)

/**
*  Enables the shared interrupts mechanism of WinDriver.
*  If the mechanism is already enabled globally (for all processes)
*  then the mechanism is also enabled for the current process.
*
*   @param [in] h:     Handle to WinDriver's kernel-mode driver as received
*                      from WD_Open()
*   @param [in] pIpcRegister:  Pointer to the IPC information struct
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @remarks
*          This function is currently only supported from the user mode.
*          This function is supported only for Windows and Linux.
*/
#define WD_SharedIntEnable(h, pIpcRegister) \
    WD_FUNCTION(IOCTL_WD_IPC_SHARED_INT_ENABLE, h, pIpcRegister, \
        sizeof(WD_IPC_REGISTER), FALSE)

/**
*  Disables the Shared Interrupts mechanism of WinDriver for all processes.
*
*   @param [in] h:     Handle to WinDriver's kernel-mode driver as received
*                      from WD_Open()
*
* @return
*  Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
*/
#define WD_SharedIntDisable(h) \
    WD_FUNCTION(IOCTL_WD_IPC_SHARED_INT_DISABLE, h, 0, 0, FALSE)

/**
*  Enables SR-IOV for a supported device.
*
*   @param [in] h:    Handle to WinDriver's kernel-mode driver as received
*                     from WD_Open().
*   @param [in] pPciSRIOV: Pointer to SR-IOV information struct
*
* @return  Returns WD_STATUS_SUCCESS (0) on success,
*   or an appropriate error code otherwise
*
* @remarks
*  Only supported in Linux
*/
#define WD_PciSriovEnable(h,pPciSRIOV) \
    WD_FUNCTION(IOCTL_WD_PCI_SRIOV_ENABLE, h, pPciSRIOV, \
        sizeof(WD_PCI_SRIOV), FALSE)

/**
*  Disables SR-IOV for a supported device and removes all the assigned VFs.
*
*   @param [in] h:    Handle to WinDriver's kernel-mode driver as received
*                     from WD_Open().
*   @param [in] pPciSRIOV: Pointer to SR-IOV information struct
*
* @return  Returns WD_STATUS_SUCCESS (0) on success,
*   or an appropriate error code otherwise
*
* @remarks
*  Only supported in Linux
*/
#define WD_PciSriovDisable(h,pPciSRIOV) \
    WD_FUNCTION(IOCTL_WD_PCI_SRIOV_DISABLE, h, pPciSRIOV, \
        sizeof(WD_PCI_SRIOV), FALSE)

/**
*  Gets the number of virtual functions assigned to a supported device.
*
*   @param [in] h:    Handle to WinDriver's kernel-mode driver as received
*                     from WD_Open().
*   @param [in,out] pPciSRIOV: Pointer to SR-IOV information struct
*
* @return  Returns WD_STATUS_SUCCESS (0) on success,
*   or an appropriate error code otherwise
*
* @remarks
*  The number of virtual functions will be stored in the pciSlot->dwNumVFs
*  field.
*  Only supported in Linux.
*/
#define WD_PciSriovGetNumVFs(h,pPciSRIOV) \
    WD_FUNCTION(IOCTL_WD_PCI_SRIOV_GET_NUMVFS, h, pPciSRIOV, \
        sizeof(WD_PCI_SRIOV), FALSE)

/**
* Sets a list of transfer cleanup commands to be performed for the specified
* card on any of the following occasions:
*  The application exits abnormally.
*
*  The application exits normally but without unregistering the specified card.
*
*  If the WD_FORCE_CLEANUP flag is set in the dwOptions parameter, the cleanup
* commands will also be performed when the specified card is unregistered.
*
*   @param [in] h:         Handle to WinDriver's kernel-mode driver as received
*                          from WD_Open()
*   @param [in] pCardCleanup: Pointer to a card clean-up information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @remarks
*  You should call this function right after calling WD_CardRegister()
*
* @snippet lowlevel_examples.c WD_CardCleanupSetup
*/
#define WD_CardCleanupSetup(h,pCardCleanup)\
    WD_FUNCTION(IOCTL_WD_CARD_CLEANUP_SETUP, h, pCardCleanup, \
        sizeof(WD_CARD_CLEANUP), FALSE)

/**
* Detects PCI devices installed on the PCI bus, which conform to the input
* criteria (vendor ID and/or card ID), and returns the number and location
* (bus, slot and function) of the detected devices.
*
*   @param [in] h:         Handle to WinDriver's kernel-mode driver as received
*                          from WD_Open()
*   @param [in,out] pPciScan: Pointer to a PCI bus scan information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet lowlevel_examples.c WD_PciScanCards
*/
#define WD_PciScanCards(h,pPciScan)\
    WD_FUNCTION(IOCTL_WD_PCI_SCAN_CARDS, h, pPciScan,\
        sizeof(WD_PCI_SCAN_CARDS), FALSE)

/**
* Scans the specified PCI capabilities group of the given PCI slot for the
* specified capability (or for all capabilities).
*
*   @param [in] h:         Handle to WinDriver's kernel-mode driver as received
*                          from WD_Open()
*   @param [in,out] pPciScanCaps: Pointer to a PCI capabilities scan structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet lowlevel_examples.c WD_PciScanCaps
*/
#define WD_PciScanCaps(h,pPciScanCaps) \
    WD_FUNCTION(IOCTL_WD_PCI_SCAN_CAPS, h, pPciScanCaps, \
        sizeof(WD_PCI_SCAN_CAPS), FALSE)

/**
* Retrieves PCI device's resource information (i.e., Memory ranges, I/O ranges,
* Interrupt lines).
*
*   @param [in] h:         Handle to WinDriver's kernel-mode driver as received
*                          from WD_Open()
*   @param [in,out] pPciCard: Pointer to a PCI card information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet lowlevel_examples.c WD_PciGetCardInfo
*/
#define WD_PciGetCardInfo(h,pPciCard)\
    WD_FUNCTION(IOCTL_WD_PCI_GET_CARD_INFO, h, pPciCard, \
        sizeof(WD_PCI_CARD_INFO), FALSE)

/**
* Reads/writes from/to the PCI configuration space of a selected PCI card or
* the extended configuration space of a selected PCI Express card.
*
*   @param [in] h:         Handle to WinDriver's kernel-mode driver as received
*                          from WD_Open()
*   @param [in,out] pPciConfigDump: Pointer to a PCI configuration space
*                                   information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet lowlevel_examples.c WD_PciConfigDump
*/
#define WD_PciConfigDump(h,pPciConfigDump)\
    WD_FUNCTION(IOCTL_WD_PCI_CONFIG_DUMP, h, pPciConfigDump, \
        sizeof(WD_PCI_CONFIG_DUMP), FALSE)

/**
* Returns the version number of the WinDriver kernel module currently running
*
*   @param [in] h:         Handle to WinDriver's kernel-mode driver as received
*                          from WD_Open()
*   @param [out] pVerInfo: Pointer to a WinDriver version information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet lowlevel_examples.c WD_Version
*/
#define WD_Version(h,pVerInfo)\
    WD_FUNCTION(IOCTL_WD_VERSION, h, pVerInfo, sizeof(WD_VERSION), FALSE)

/**
* Transfers the license string to the WinDriver kernel module
* When using the high-level WDC library APIs, described in the WinDriver
* PCI Manual, the license registration is done via the WDC_DriverOpen()
* function, so you do not need to call WD_License() directly.
*
*   @param [in] h:         Handle to WinDriver's kernel-mode driver as received
*                          from WD_Open()
*   @param [in] pLicense:  Pointer to a WinDriver license information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @remarks
*  When using a registered version, this function must be called before any
*  other WinDriver API call, apart from WD_Open(), in order to register
*  the license from the code.
*
* @snippet lowlevel_examples.c WD_License
*/
#define WD_License(h,pLicense)\
    WD_FUNCTION(IOCTL_WD_LICENSE, h, pLicense, sizeof(WD_LICENSE), FALSE)

/**
* Obtain a valid handle to the Kernel PlugIn
*
*   @param [in] h:         Handle to WinDriver's kernel-mode driver as received
*                          from WD_Open().
*   @param [in,out] pKernelPlugIn: Pointer to Kernel PlugIn information
*                                  structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet lowlevel_examples.c WD_KernelPlugInOpen
*/
#define WD_KernelPlugInOpen(h,pKernelPlugIn)\
    WD_FUNCTION(IOCTL_WD_KERNEL_PLUGIN_OPEN, h, pKernelPlugIn, \
        sizeof(WD_KERNEL_PLUGIN), FALSE)

/**
* Closes the WinDriver Kernel PlugIn handle obtained from WD_KernelPlugInOpen()
*
*   @param [in] h:         Handle to WinDriver's kernel-mode driver as received
*                          from WD_Open()
*   @param [in] pKernelPlugIn: Pointer to Kernel PlugIn information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet lowlevel_examples.c WD_KernelPlugInClose
*/
#define WD_KernelPlugInClose(h,pKernelPlugIn)\
    WD_FUNCTION(IOCTL_WD_KERNEL_PLUGIN_CLOSE, h, pKernelPlugIn, \
        sizeof(WD_KERNEL_PLUGIN), FALSE)

/**
* Calls a routine in the Kernel PlugIn to be executed
*
*   @param [in] h:         Handle to WinDriver's kernel-mode driver as received
*                          from WD_Open()
*   @param [in,out] pKernelPlugInCall: Pointer to Kernel PlugIn information
*                                      structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @remarks
*  Calling the WD_KernelPlugInCall() function in the user mode will call
*  your KP_Call callback function in the kernel. The KP_Call Kernel PlugIn
*  function will determine what routine to execute according to the message
*  passed to it in the WD_KERNEL_PLUGIN_CALL structure.
*
* @snippet lowlevel_examples.c WD_KernelPlugInCall
*/
#define WD_KernelPlugInCall(h,pKernelPlugInCall)\
    WD_FUNCTION(IOCTL_WD_KERNEL_PLUGIN_CALL, h, pKernelPlugInCall, \
        sizeof(WD_KERNEL_PLUGIN_CALL), FALSE)

/**
* Registers an interrupt service routine (ISR) to be called upon interrupt
*   @param [in] h:         Handle to WinDriver's kernel-mode driver as received
*                          from WD_Open()
*   @param [in,out] pInterrupt: Pointer to an interrupt information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @remarks
*  For more information regarding interrupt handling please refer to the
*  Interrupts section in the WinDriver PCI Manual.
*
*  kpCall is relevant for Kernel PlugIn implementation.
*
*  WinDriver must be registered with the OS as the driver of the device before
*  enabling interrupts. For Plug-and-Play hardware (PCI/PCI Express) on Windows
*  platforms, this association is made by installing an INF file for the
*  device. If the INF file is not installed, WD_IntEnable() will fail with a
*  WD_NO_DEVICE_OBJECT error.
*
* @snippet lowlevel_examples.c WD_IntEnable
*/
#define WD_IntEnable(h,pInterrupt)\
    WD_FUNCTION(IOCTL_WD_INT_ENABLE, h, pInterrupt, sizeof(WD_INTERRUPT), FALSE)

/**
* Disables interrupt processing
*   @param [in] h:         Handle to WinDriver's kernel-mode driver as received
*                          from WD_Open()
*   @param [in] pInterrupt: Pointer to an interrupt information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet lowlevel_examples.c WD_IntDisable
*/
#define WD_IntDisable(h,pInterrupt)\
    WD_FUNCTION(IOCTL_WD_INT_DISABLE, h, pInterrupt, sizeof(WD_INTERRUPT), \
        FALSE)

/**
* Retrieves the interrupts count since the call to WD_IntEnable()
*   @param [in] h:         Handle to WinDriver's kernel-mode driver as received
*                          from WD_Open()
*   @param [in,out] pInterrupt: Pointer to an interrupt information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @snippet lowlevel_examples.c WD_IntCount
*/
#define WD_IntCount(h,pInterrupt)\
    WD_FUNCTION(IOCTL_WD_INT_COUNT, h, pInterrupt, sizeof(WD_INTERRUPT), FALSE)

/**
* Waits for an interrupt.
*   @param [in] h:         Handle to WinDriver's kernel-mode driver as received
*                          from WD_Open()
*   @param [in,out] pInterrupt: Pointer to an interrupt information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @remarks
*  The INTERRUPT_INTERRUPTED status (set in pInterrupt->fStopped) can occur on
*  Linux if the application that waits on the interrupt is stopped
*  (e.g., by pressing CTRL+Z).
*
* @snippet lowlevel_examples.c WD_IntWait
*/
#define WD_IntWait(h,pInterrupt)\
    WD_FUNCTION(IOCTL_WD_INT_WAIT, h, pInterrupt, sizeof(WD_INTERRUPT), TRUE)

/**
* Delays execution for a specific duration of time.
*   @param [in] h:         Handle to WinDriver's kernel-mode driver as received
*                          from WD_Open()
*   @param [in] pSleep: Pointer to a sleep information structure
*
* @return Returns WD_STATUS_SUCCESS (0) on success,
*  or an appropriate error code otherwise
*
* @remarks
*  Example usage: to access slow response hardware.
*
* @snippet lowlevel_examples.c WD_Sleep
*/
#define WD_Sleep(h,pSleep)\
    WD_FUNCTION(IOCTL_WD_SLEEP, h, pSleep, sizeof(WD_SLEEP), FALSE)


#define WD_EventRegister(h, pEvent) \
    WD_FUNCTION(IOCTL_WD_EVENT_REGISTER, h, pEvent, SIZE_OF_WD_EVENT(pEvent), \
        FALSE)
#define WD_EventUnregister(h, pEvent) \
    WD_FUNCTION(IOCTL_WD_EVENT_UNREGISTER, h, pEvent, \
        SIZE_OF_WD_EVENT(pEvent), FALSE)
#define WD_EventPull(h,pEvent) \
    WD_FUNCTION(IOCTL_WD_EVENT_PULL, h, pEvent, SIZE_OF_WD_EVENT(pEvent), FALSE)
#define WD_EventSend(h,pEvent) \
    WD_FUNCTION(IOCTL_WD_EVENT_SEND, h, pEvent, SIZE_OF_WD_EVENT(pEvent), FALSE)
#define WD_Usage(h, pStop) \
    WD_FUNCTION(IOCTL_WD_USAGE, h, pStop, sizeof(WD_USAGE), FALSE)

#define WD_UGetDeviceData(h, pGetDevData) \
    WD_FUNCTION(IOCTL_WDU_GET_DEVICE_DATA, h, pGetDevData, \
        sizeof(WDU_GET_DEVICE_DATA), FALSE);
#define WD_GetDeviceProperty(h, pGetDevProperty) \
    WD_FUNCTION(IOCTL_WD_GET_DEVICE_PROPERTY, h, pGetDevProperty, \
        sizeof(WD_GET_DEVICE_PROPERTY), FALSE);
#define WD_USetInterface(h, pSetIfc) \
    WD_FUNCTION(IOCTL_WDU_SET_INTERFACE, h, pSetIfc, \
        sizeof(WDU_SET_INTERFACE), FALSE);
#define WD_UResetPipe(h, pResetPipe) \
    WD_FUNCTION(IOCTL_WDU_RESET_PIPE, h, pResetPipe, sizeof(WDU_RESET_PIPE), \
        FALSE);
#define WD_UTransfer(h, pTrans) \
    WD_FUNCTION(IOCTL_WDU_TRANSFER, h, pTrans, sizeof(WDU_TRANSFER), TRUE);
#define WD_UHaltTransfer(h, pHaltTrans) \
    WD_FUNCTION(IOCTL_WDU_HALT_TRANSFER, h, pHaltTrans, \
        sizeof(WDU_HALT_TRANSFER), FALSE);
#define WD_UWakeup(h, pWakeup) \
    WD_FUNCTION(IOCTL_WDU_WAKEUP, h, pWakeup, sizeof(WDU_WAKEUP), FALSE);
#define WD_USelectiveSuspend(h, pSelectiveSuspend) \
    WD_FUNCTION(IOCTL_WDU_SELECTIVE_SUSPEND, h, pSelectiveSuspend, \
        sizeof(WDU_SELECTIVE_SUSPEND), FALSE);
#define WD_UResetDevice(h, pResetDevice) \
    WD_FUNCTION(IOCTL_WDU_RESET_DEVICE, h, pResetDevice, \
        sizeof(WDU_RESET_DEVICE), FALSE);
#define WD_UStreamOpen(h, pStream) \
    WD_FUNCTION(IOCTL_WDU_STREAM_OPEN, h, pStream, sizeof(WDU_STREAM), FALSE);
#define WD_UStreamClose(h, pStream) \
    WD_FUNCTION(IOCTL_WDU_STREAM_CLOSE, h, pStream, sizeof(WDU_STREAM), FALSE);
#define WD_UStreamStart(h, pStream) \
    WD_FUNCTION(IOCTL_WDU_STREAM_START, h, pStream, sizeof(WDU_STREAM), FALSE);
#define WD_UStreamStop(h, pStream) \
    WD_FUNCTION(IOCTL_WDU_STREAM_STOP, h, pStream, sizeof(WDU_STREAM), FALSE);
#define WD_UStreamFlush(h, pStream) \
    WD_FUNCTION(IOCTL_WDU_STREAM_FLUSH, h, pStream, sizeof(WDU_STREAM), FALSE);
#define WD_UStreamGetStatus(h, pStreamStatus) \
    WD_FUNCTION(IOCTL_WDU_STREAM_GET_STATUS, h, pStreamStatus, \
        sizeof(WDU_STREAM_STATUS), FALSE);

#define __ALIGN_DOWN(val,alignment) ( (val) & ~((alignment) - 1) )
#define __ALIGN_UP(val,alignment) \
    ( ((val) + (alignment) - 1) & ~((alignment) - 1) )

#ifdef WDLOG
    #include "wd_log.h"
#endif

#ifndef MIN
    #define MIN(a,b) ((a) > (b) ? (b) : (a))
#endif
#ifndef MAX
    #define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#define SAFE_STRING(s) ((s) ? (s) : "")

#define UNUSED_VAR(x) (void)x

#ifdef __cplusplus
}
#endif

#endif /* _WINDRVR_H_ */

