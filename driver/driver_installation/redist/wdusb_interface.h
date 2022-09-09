/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/*  This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation.
 * This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
   v2 for more details.
 * You should have received a copy of the GNU General Public License
   along with this program. If not, contact Jungo Connectivity Ltd. at
   support@jungo.com */

#ifndef _WDUSB_INTERFACE_H_
#define _WDUSB_INTERFACE_H_

#include "windrvr_usb.h"

#define SETUP_PACKET_LEN 8
#define MAX_URBS_TO_USE 4

#if defined(LINUX_USB_MODULE) || defined(LINUX_USB_SUPPORT)
    #ifdef WD_DRIVER_NAME_CHANGE
        #define WD_USB_FUNC_NAME(func) driver_usb_##func
    #else
        #define WD_USB_FUNC_NAME(func) windrvr1511_##func
    #endif
#else
    #define WD_USB_FUNC_NAME(func) func
#endif

#if defined(__cplusplus)
    extern "C" {
#endif

#if defined(LINUX) && !defined(__WD_LINUX_SYNC_TYPES__)
    #define __WD_LINUX_SYNC_TYPES__

    typedef struct os_spinlock
    {
        void *spinlock;
        unsigned long flags;
    } os_spinlock_t;

    typedef volatile int os_interlocked_t;
#endif

typedef struct _trans_t
{
    struct _trans_t  *next;
    struct pipe_t    *pipe;
    void             *os_trans_ctx;
    void            (*os_trans_ctx_destroy_cb)(void *);
    BOOL              is_halted;
    long              refcnt;
} trans_t;

typedef struct _stream_context_t
{
    DWORD di_unique_id;
    DWORD pipe_num;
} stream_context_t;

typedef struct
{
    /* Internal buffer */
    PBYTE buffer;
    DWORD curr_ptr;
    DWORD curr_count;
    DWORD outstanding_bytes; /* Total byte count of outstanding read/write
                              * URB's */
    DWORD bytes_to_process;  /* Number of bytes in internal buffer that is
                              * waiting to be processed by requests in
                              * requests_in_process queue */
    HANDLE file_h;
    void *requests;
    void *requests_to_finish; /* Queue of requests that need to be completed
                                 when there is enough data in the internal
                                 buffer in case of READ request, or enough free
                                 space in case of WRITE request. */
    os_interlocked_t is_running;
    BOOL is_buffer_full; /* TRUE if buffer becomes full during read operation */
    os_spinlock_t spinlock;
    trans_t *trans;
    void *action_timer;
    os_interlocked_t last_error;

    /* Set by user */
    WDU_STREAM settings;

    /* Stream context, stored in file system */
    stream_context_t *context;
} stream_t;

typedef struct pipe_t
{
    HANDLE             handle;
    trans_t           *trans_list;
    os_interlocked_t   trans_active;
    os_spinlock_t      trans_spinlock;
    UCHAR              endpoint_address;
    UCHAR              attributes;
    USHORT             max_packet_size;
    UCHAR              interval;
    DWORD              max_urb_transfer_size;
    stream_t          *stream;
    os_interlocked_t   stream_enabled;
} pipe_t;
#define GET_PIPE_TYPE(p) \
    ((USB_PIPE_TYPE)((p)->attributes & WDU_ENDPOINT_TYPE_MASK))

#if defined(LINUX)
typedef struct
{
    int (*KDBG_func)(long dwLevel, long dwSection, const char *format, ...);
    DWORD (*wd_device_attach)(HANDLE os_dev_h, DWORD interface_num,
        DWORD configuration_value);
    DWORD (*wd_device_detach)(HANDLE os_dev_h);
    int (*wd_page_list_copyout)(void *pl_h, unsigned long offset, void *dst,
        unsigned long bytes);
    int (*wd_page_list_copyin)(void *pl_h, unsigned long offset,
        const void *src, unsigned long bytes);
    int (*wd_user_page_list_get)(void *buf, unsigned long bytes, void **pl_h);
    void (*wd_user_page_list_put)(void *pl_h);
    trans_t* (*wd_create_transfer)(pipe_t *pipe, void *os_trans_ctx,
        void (*wd_os_trans_ctx_destroy_cb)(void *));
    BOOL (*wd_release_transfer)(trans_t *trans);
    DWORD (*wd_map_error_status)(int err);
    const char* (*wd_get_driver_name)(void);
} wdusb_callbacks_t;

void WD_USB_FUNC_NAME(wdusb_register_callbacks)(wdusb_callbacks_t *callbacks,
    int *wdusb_ver);

#endif

/* General Interface */
BOOL WD_USB_FUNC_NAME(OS_init)(void);
void WD_USB_FUNC_NAME(OS_uninit)(void);

/* Device Interface */
DWORD WD_USB_FUNC_NAME(OS_register_devices)(void **register_ctx,
    WDU_MATCH_TABLE *match_tables, DWORD match_tabs_number);
DWORD WD_USB_FUNC_NAME(OS_unregister_devices)(void *register_handle);

DWORD WD_USB_FUNC_NAME(OS_get_device_property)(HANDLE os_dev_h, void *buf,
    DWORD *buf_size, WD_DEVICE_REGISTRY_PROPERTY prop);
DWORD WD_USB_FUNC_NAME(OS_get_device_info)(HANDLE os_dev_h, void *buf,
    DWORD *buf_size, DWORD active_config, DWORD active_interface,
    DWORD active_setting, BOOL is_kernelmode, DWORD dwOptions);

DWORD WD_USB_FUNC_NAME(OS_close_device)(HANDLE os_dev_h);
DWORD WD_USB_FUNC_NAME(OS_reset_device)(HANDLE os_dev_h, DWORD options);

/* Power Interface */
DWORD WD_USB_FUNC_NAME(OS_wakeup)(HANDLE os_dev_h, DWORD options);
DWORD WD_USB_FUNC_NAME(OS_selective_suspend)(HANDLE os_dev_h, DWORD options);

/* Transfer Interface */
DWORD WD_USB_FUNC_NAME(OS_transfer)(HANDLE os_dev_h, pipe_t *pipe,
    HANDLE file_h, PRCHANDLE prc_h, DWORD is_read, DWORD options, PVOID buf,
    DWORD bytes, DWORD *bytes_transferred, UCHAR *setup_packet, DWORD timeout,
    PVOID ioctl_context);
DWORD WD_USB_FUNC_NAME(OS_halt_transfer)(void *os_trans_ctx);

int WD_USB_FUNC_NAME(OS_num_pending_urbs)(void *os_trans_ctx);
DWORD WD_USB_FUNC_NAME(OS_get_max_urb_transfer_size)(BOOL high_speed,
    const pipe_t *pipe);

DWORD WD_USB_FUNC_NAME(OS_lock_user_buff)(PVOID *ptr, DWORD bytes,
    PVOID *lock_ctx, PVOID transform_context);
void WD_USB_FUNC_NAME(OS_unlock_user_buff)(PVOID lock_ctx);

DWORD WD_USB_FUNC_NAME(OS_open_pipe)(HANDLE os_dev_h,
    const WDU_ENDPOINT_DESCRIPTOR *endpoint_desc, pipe_t *pipe);
DWORD WD_USB_FUNC_NAME(OS_reset_pipe)(HANDLE os_dev_h, pipe_t *pipe,
    PVOID ioctl_context);

DWORD WD_USB_FUNC_NAME(OS_set_interface)(HANDLE os_dev_h,
    WDU_ALTERNATE_SETTING **alt_setting_info, DWORD interface_num,
    DWORD alternate_setting);

/* Streaming Interface */
DWORD WD_USB_FUNC_NAME(OS_stream_transfer_create)(HANDLE os_dev_h,
    pipe_t *pipe);
DWORD WD_USB_FUNC_NAME(OS_stream_transfer_start)(stream_t *stream);
stream_context_t* WD_USB_FUNC_NAME(OS_get_stream_context)(HANDLE file_h);
void WD_USB_FUNC_NAME(OS_set_stream_context)(HANDLE file_h,
    stream_context_t *context);
DWORD WD_USB_FUNC_NAME(OS_stream_request_insert)(stream_t *stream,
    void *request);
BOOL WD_USB_FUNC_NAME(OS_is_stream_requests_queue_empty)(stream_t *stream);
void WD_USB_FUNC_NAME(OS_stream_issue_new_transfers)(void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* _WDUSB_INTERFACE_H_ */

