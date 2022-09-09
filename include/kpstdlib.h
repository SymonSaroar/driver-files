/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _KPSTDLIB_H_
#define _KPSTDLIB_H_

#ifndef __KERNEL__
    #define __KERNEL__
#endif

#if !defined(UNIX) && defined(LINUX)
    #define UNIX
#endif

#if defined(UNIX)
    #include "windrvr.h" // for use of KDBG DWORD parameter.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Spinlocks and interlocked operations */

/** Kernel PlugIn spinlock object structure */
typedef struct _KP_SPINLOCK KP_SPINLOCK;
/**
*  Initializes a new Kernel PlugIn spinlock object.
*
* @return
*  If successful, returns a pointer to the new Kernel PlugIn spinlock object,
*  otherwise returns NULL.
*
*/
KP_SPINLOCK *kp_spinlock_init(void);

/**
*  Waits on a Kernel PlugIn spinlock object.
*
*   @param [in] spinlock: Pointer to the Kernel PlugIn spinlock
*                         object on which to wait
*
* @return
*  None
*
*/
void kp_spinlock_wait(KP_SPINLOCK *spinlock);

/**
*  Releases a Kernel PlugIn spinlock object.
*
*   @param [in] spinlock: Pointer to the Kernel PlugIn spinlock
*                         object on which to release
*
* @return
*  None
*
*/
void kp_spinlock_release(KP_SPINLOCK *spinlock);

/**
*  Uninitializes a Kernel PlugIn spinlock object.
*
*   @param [in] spinlock: Pointer to the Kernel PlugIn spinlock
*                         object on which to uninitialize
*
* @return
*  None
*
*/
void kp_spinlock_uninit(KP_SPINLOCK *spinlock);

/** a Kernel PlugIn interlocked operations counter */
typedef volatile int KP_INTERLOCKED;

/**
*  Initializes a Kernel PlugIn interlocked counter.
*
*   @param [in,out] target: Pointer to the Kernel PlugIn interlocked counter
*                           to initialize
*
* @return
*  None
*
*/
void kp_interlocked_init(KP_INTERLOCKED *target);

/**
*  Uninitializes  a Kernel PlugIn interlocked counter.
*
*   @param [in,out] target: Pointer to the Kernel PlugIn interlocked counter
*                           to uninitialize
*
* @return
*  None
*
*/
void kp_interlocked_uninit(KP_INTERLOCKED *target);

/**
*  Increments the value of a Kernel PlugIn interlocked counter by one.
*
*   @param [in,out] target: Pointer to the Kernel PlugIn interlocked counter
*                           to increment
*
* @return
*  Returns the new value of the interlocked counter (target).
*
*/
int kp_interlocked_increment(KP_INTERLOCKED *target);

/**
*  Decrements the value of a Kernel PlugIn interlocked counter by one.
*
*   @param [in,out] target: Pointer to the Kernel PlugIn interlocked counter
*                           to decrement
*
* @return
*  Returns the new value of the interlocked counter (target).
*
*/
int kp_interlocked_decrement(KP_INTERLOCKED *target);

/**
*  Adds a specified value to the current value of a Kernel PlugIn
*  interlocked counter.
*
*   @param [in,out] target: Pointer to the Kernel PlugIn interlocked counter
*                           to which to add
*   @param [in] val:        The value to add to the interlocked counter
*                           (target)
*
* @return
*  Returns the new value of the interlocked counter (target).
*
*/
int kp_interlocked_add(KP_INTERLOCKED *target, int val);

/**
*  Reads to the value of a Kernel PlugIn interlocked counter.
*
*   @param [in] target: Pointer to the Kernel PlugIn interlocked counter
*                           to read
*
* @return
*  Returns the value of the interlocked counter (target).
*
*/
int kp_interlocked_read(KP_INTERLOCKED *target);

/**
*  Sets the value of a Kernel PlugIn interlocked counter to the
*  specified value.
*
*   @param [in,out] target: Pointer to the Kernel PlugIn interlocked counter
*                           to set
*   @param [in] val:        The value to set for the interlocked counter (target)
*
* @return
*  None
*
*/
void kp_interlocked_set(KP_INTERLOCKED *target, int val);

/**
*  Sets the value of a Kernel PlugIn interlocked counter to the specified
*  value and returns the previous value of the counter.
*
*   @param [in,out] target: Pointer to the Kernel PlugIn interlocked counter
*                           to exchange
*   @param [in] val:        The new value to set for the interlocked counter
*                           (target)

*
* @return
*  Returns the previous value of the interlocked counter (target).
*
*/
int kp_interlocked_exchange(KP_INTERLOCKED *target, int val);

#if defined(WINNT) || defined(WIN32)
    #if defined(_WIN64) && !defined(KERNEL_64BIT)
        #define KERNEL_64BIT
    #endif
    typedef unsigned long ULONG;
    typedef unsigned short USHORT;
    typedef unsigned char UCHAR;
    typedef long LONG;
    typedef short SHORT;
    typedef char CHAR;
    typedef ULONG DWORD;
    typedef DWORD *PDWORD;
    typedef unsigned char *PBYTE;
    typedef USHORT WORD;
    typedef void *PVOID;
    typedef char *PCHAR;
    typedef PVOID HANDLE;
    typedef ULONG BOOL;
    #ifndef WINAPI
        #define WINAPI
    #endif
#elif defined(UNIX)
    #ifndef __cdecl
        #define __cdecl
    #endif
#endif

#if defined(WINNT) || defined(WIN32)
    #define OS_needs_copy_from_user(fKernelMode) FALSE
    #define COPY_FROM_USER(dst,src,n) memcpy(dst,src,n)
    #define COPY_TO_USER(dst,src,n) memcpy(dst,src,n)
#elif defined(LINUX)
    #define OS_needs_copy_from_user(fKernelMode) (!fKernelMode)
    #define COPY_FROM_USER(dst,src,n) LINUX_copy_from_user(dst,src,n)
    #define COPY_TO_USER(dst,src,n) LINUX_copy_to_user(dst,src,n)
#endif

/**
* Macro for copying data from the user mode to the Kernel PlugIn.
* @remark The COPY_TO_USER_OR_KERNEL() and COPY_FROM_USER_OR_KERNEL() are
* macros used for copying data (when necessary) to/from user-mode memory
* addresses (respectively), when accessing such addresses from within the
* Kernel PlugIn. Copying the data ensures that the user-mode address can be
* used correctly, even if the context of the user-mode process changes in
* the midst of the I/O operation. This is particularly relevant for long
* operations, during which the context of the user-mode process may change.
* The use of macros to perform the copy provides a generic solution for all
* supported operating systems.
* Note that if you wish to access the user-mode data from within the Kernel
* PlugIn interrupt handler functions, you should first copy the data into
* some variable in the Kernel PlugIn before the execution of the kernel-mode
* interrupt handler routines.
* To safely share a data buffer between the user-mode and Kernel PlugIn
* routines (e.g., KP_IntAtIrql() and KP_IntAtDpc()), consider
* using the technique outlined in the technical document titled "How do I
* share a memory buffer between Kernel PlugIn and user-mode projects for DMA
* or other purposes?" found under the "Kernel PlugIn" technical documents
* section of the "Support" section.
*/
#define COPY_FROM_USER_OR_KERNEL(dst, src, n, fKernelMode) \
{ \
    if (OS_needs_copy_from_user(fKernelMode)) \
        COPY_FROM_USER(dst, src, n); \
    else \
        memcpy(dst, src, n); \
}

/**
* Macro copying data from the Kernel PlugIn to user mode.
* See COPY_FROM_USER_OR_KERNEL() for more info.
*/
#define COPY_TO_USER_OR_KERNEL(dst, src, n, fKernelMode) \
{ \
    if (OS_needs_copy_from_user(fKernelMode)) \
        COPY_TO_USER(dst, src, n); \
    else \
        memcpy(dst, src, n); \
}

#ifndef FALSE
    #define FALSE 0
#endif

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef NULL
    #define NULL 0UL
#endif

int __cdecl KDBG(DWORD dwLevel, DWORD dwSection, const char *format, ...);

#if defined(WIN32)
    #if defined(KERNEL_64BIT)
        #include <stdarg.h>
    #else
        #define _INTSIZEOF(n) ((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))
        // Define varargs ANSI style
        typedef char * va_list;
        #define va_start(ap,v) ( ap = (va_list)&v + _INTSIZEOF(v) )
        #define va_arg(ap,t) ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
        #define va_end(ap) ( ap = (va_list)0 )

    #endif

    int __cdecl _snprintf(char *buffer, unsigned long Limit, const char *format,
        ...);
    int __cdecl _vsnprintf(char *buffer, unsigned long Limit, const char
        *format, va_list Next);
#endif

char* __cdecl strcpy(char *s1, const char *s2);
void* __cdecl malloc(unsigned long size);
void __cdecl free(void *buf);

#if defined(LINUX)
    #include <linux_wrappers.h>
    #define memset LINUX_memset
    #define strncmp LINUX_strncmp
    #define strcpy LINUX_strcpy
    #define strcmp LINUX_strcmp
    #define strncpy LINUX_strncpy
    #define strcat LINUX_strcat
    #define strncat LINUX_strncat
    #define strlen LINUX_strlen
    #define memcpy LINUX_memcpy
    #define memcmp LINUX_memcmp
    #define sprintf LINUX_sprintf
    #define vsprintf LINUX_vsprintf
    #define snprintf LINUX_snprintf
    #define vsnprintf LINUX_vsnprintf
#elif defined(WINNT)
    #if !defined size_t
        #if defined(KERNEL_64BIT)
            typedef unsigned __int64 size_t;
        #else
            typedef unsigned int size_t;
        #endif
    #endif
    void* __cdecl memcpy(void *dest, const void *src, size_t count);
    void* __cdecl memset(void *dest, int c, size_t count);
#if !defined(_STRNCPY)
    char* _strncpy(char* s1, const char* s2, size_t limit);
#endif
    #define snprintf _snprintf
    #define vsnprintf _vsnprintf
    #define strncpy _strncpy
#endif

#ifdef __cplusplus
}
#endif

#endif /* _KPSTDLIB_H_ */

