/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */
/*
 * Source file for WinDriver Linux.
 *
 * This file may be distributed only as part of the
 * application you are distributing, and only if it
 * significantly contributes to the functionality of your
 * application. (see \windriver\docs\license.txt for details).
 *
 * Web site: https://www.jungo.com
 * Email:    support@jungo.com
 */

#include "linux_common.h"

/* If CONFIG_AMD_MEM_ENCRYPT, CONFIG_ARCH_HAS_CC_PLATFORM is defined it causes
 * a GPL compilation error when trying to compile WinDriver */
#undef CONFIG_AMD_MEM_ENCRYPT
#undef CONFIG_ARCH_HAS_CC_PLATFORM

#include <linux/compiler.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/pagemap.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <asm/cacheflush.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,12,0)
    #include <asm/set_memory.h>
#endif
#include <linux/efi.h>
#include <linux/dmi.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,18,0)
    #include <linux/dma-mapping.h>
#endif

#ifdef JETSON_TK1
    #include <linux/slab.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39)
    #include <linux/smp_lock.h>
#endif
#include <linux/vmalloc.h>
#include <asm/mman.h>
#include "linux_wrappers.h"
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)) && defined(CONFIG_COMPAT)
        #include <linux/ioctl32.h>
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,1,0))
        #include <uapi/linux/mman.h>
#endif

#if defined(UDEV_SUPPORT)
    #include <linux/devfs_fs_kernel.h>
#endif

#include "wdusb_interface.h"
#include "wdsriov_interface.h"

#if defined(WINDRIVER_KERNEL)
    #define REGISTER_FUNC_MOD
#else
    /* When building the kernel plugin */
    #define REGISTER_FUNC_MOD extern
#endif

#if defined(WD_DRIVER_NAME_CHANGE)
REGISTER_FUNC_MOD kp_register_mod_func_t driver_register_kp_module_func;
#else
REGISTER_FUNC_MOD kp_register_mod_func_t windrvr1511_register_kp_module_func;
#endif

#if defined(WINDRIVER_KERNEL)
    #define EXPORT_SYMTAB
#endif
#include <linux/module.h> // must come after #define EXPORT_SYMTAB

#if defined(MODVERSIONS)
    #include <linux/modversions.h>
#endif

#if defined(MODULE_LICENSE)
    MODULE_LICENSE("Proprietary");
#endif
#if defined(MODULE_AUTHOR)
    MODULE_AUTHOR("Jungo Connectivity");
#endif
#if defined(MODULE_DESCRIPTION)
    #include "wd_ver.h"
    MODULE_DESCRIPTION("WinDriver v" WD_VERSION_STR
        " Jungo Connectivity (C) 1999 - " COPYRIGHTS_YEAR_STR);
#endif

void generic_pci_remove(void *dev_h, int notify);
int generic_pci_probe(void *dev_h, int notify);

#define VM_PG_OFFSET(vma) (vma)->vm_pgoff
#define VM_BYTE_OFFSET(vma) ((vma)->vm_pgoff << PAGE_SHIFT)


#if defined(SLI_MX6) || defined(RPI_3BP)
void __aeabi_unwind_cpp_pr0(void)
{
};
EXPORT_SYMBOL(__aeabi_unwind_cpp_pr0);

void __aeabi_unwind_cpp_pr1(void)
{
};
EXPORT_SYMBOL(__aeabi_unwind_cpp_pr1);

void __aeabi_unwind_cpp_pr2(void)
{
};
EXPORT_SYMBOL(__aeabi_unwind_cpp_pr2);
#endif

static struct pci_dev *pci_root_dev;

typedef struct
{
    struct page **pages;
    int page_count;
    unsigned long first_page_offset;
    unsigned long byte_count;
} LINUX_page_list;

typedef struct
{
    unsigned char *map;
    unsigned long flags;
} LINUX_page_addr_param;

static inline int is_high_memory(unsigned long addr)
{
    return (addr >= (unsigned long)high_memory);
}

void LINUX_wmb(void)
{
#if defined (__aarch64__)
    wmb();
#endif
}

void LINUX_rmb(void)
{
#if defined (__aarch64__)
    rmb();
#endif
}

int LINUX_down_interruptible(struct semaphore *sem)
{
    return down_interruptible(sem);
}

void LINUX_down(struct semaphore *sem)
{
    down(sem);
}

void LINUX_up(struct semaphore *sem)
{
    up(sem);
}

struct semaphore *LINUX_create_mutex(void)
{
    struct semaphore *sem = (struct semaphore *)kmalloc(
        sizeof(struct semaphore), GFP_ATOMIC);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37) || \
    (defined CONFIG_PREEMPT_RT && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
    sema_init(sem, 1);
#else
    init_MUTEX(sem);
#endif
    return sem;
}

void LINUX_free_mutex(struct semaphore *sem)
{
    kfree(sem);
}

void *LINUX_vmalloc(unsigned long size)
{
    return (void *)vmalloc(size);
}

void LINUX_vfree(void *addr)
{
    vfree(addr);
}

void *LINUX_kmalloc(unsigned int size, int flag)
{
    int is_atomic = flag & ATOMIC;
    int is_dma = flag & DMA;
    int is_user = flag & USER;

    if (size > 128 * 1024)
        return NULL;

    flag = is_atomic ? GFP_ATOMIC : GFP_KERNEL;
    flag |= is_dma ? GFP_DMA : 0;
    flag |= is_user ? GFP_USER : 0;
    return kmalloc((size_t)size, flag);
}

void *LINUX_dma_contig_alloc(void *pdev, unsigned int size, int flag,
    _u64 *phys)
{
    struct pci_dev *hwdev = pdev;
    int gfp = ((flag & DMA) ? GFP_DMA : 0) | __GFP_NOWARN;
    dma_addr_t dma_handle = 0;
    void *res;

    #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,6)
        gfp |= __GFP_COMP;
    #endif

    #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,2,0)
        if (!hwdev)
        {
            printk("%s: This Linux kernel doesn't support allocating DMA with a"
                " NULL device node\n", __FUNCTION__);
            return NULL;
        }
    #endif
    res = dma_alloc_coherent(hwdev ? &hwdev->dev : NULL, size, &dma_handle,
        GFP_KERNEL | gfp);

    /* dma_addr_t is u32/u64 depending on kenrel CONFIG_ARCH_DMA_ADDR_T_64BIT
     * config. Therefore casting should be done only by value */
    *phys = (_u64)dma_handle;

    return res;
}

void LINUX_dma_contig_free(void *pdev, void *va, unsigned int size, _u64 phys)
{
    struct pci_dev *hwdev = pdev;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,16,0)
    dma_free_coherent(hwdev ? &hwdev->dev : NULL, size, va, phys);
#else
    if (!hwdev)
        return;

    dma_free_coherent(&hwdev->dev, size, va, phys);
#endif
}

void LINUX_kfree(const void *addr)
{
    kfree(addr);
}

unsigned long LINUX_copy_from_user(void *to, const void *from, unsigned long n)
{
    return copy_from_user(to, from, n);
}

unsigned long LINUX_copy_to_user(void *to, const void *from, unsigned long n)
{
    return copy_to_user(to, from, n);
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,24)
struct page *windriver_vma_nopage(struct vm_area_struct *vma,
    unsigned long address, int *type)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
int windriver_vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,17,0)
int windriver_vma_fault(struct vm_fault *vmf)
#else
vm_fault_t windriver_vma_fault(struct vm_fault *vmf)
#endif
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,24)
    unsigned long off = address - vma->vm_start;
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0)
    unsigned long off = (unsigned long)vmf->virtual_address - vma->vm_start;
#else
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,0)
        struct vm_area_struct *vma = vmf->vma;
    #endif
    unsigned long off = (unsigned long)vmf->address - vma->vm_start;
#endif
    LINUX_mmap_private *private_data; 
    unsigned long va; 
    struct page *page;

    private_data =
        (LINUX_mmap_private *)vma->vm_private_data;
    if (!private_data)
    {
        printk("%s: private_data is NULL\n", __FUNCTION__);
        return 0;
    }
    va = (unsigned long)private_data->kernel_addr + off;

    if (private_data->is_vmalloced)
        page = vmalloc_to_page((const void *)va);
    else
        page = virt_to_page(va);

    get_page(page); /* increment page count */

    #if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,24)
    if (type)
        *type = VM_FAULT_MINOR;
    #endif
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,24)
    return page;
#else
    vmf->page = page;
    return 0;
#endif
}

void windriver_vma_open(struct vm_area_struct *area)
{
    if (area->vm_private_data)
    {
        LINUX_mmap_private *private_data =
            (LINUX_mmap_private *)area->vm_private_data;
        LINUX_atomic_inc(&private_data->ref_count);
    }
}

void windriver_vma_close(struct vm_area_struct *area)
{
    if (area->vm_private_data)
    {
        LINUX_mmap_private *private_data =
            (LINUX_mmap_private *)area->vm_private_data;

        /* Free the data allocated by wd_mmap() */
        if (!LINUX_atomic_dec(&private_data->ref_count))
        {
            LINUX_kfree(area->vm_private_data);
            area->vm_private_data = NULL;
        }
    }
}

#ifdef CONFIG_HAVE_IOREMAP_PROT
int windriver_vma_access(struct vm_area_struct *area, unsigned long addr,
    void *buf, int len, int write)
{
    void __iomem *maddr;
    unsigned long offset;
    unsigned long phys;

    if (!(area->vm_flags & (VM_IO | VM_PFNMAP)))
        return -EINVAL;

    offset = offset_in_page(addr);
    phys = area->vm_pgoff << PAGE_SHIFT;
    maddr = ioremap(phys, PAGE_ALIGN(len + offset));
    if (!maddr)
        return -ENOMEM;

    if (write)
        memcpy_toio(maddr + offset, buf, len);
    else
        memcpy_fromio(buf, maddr + offset, len);

    iounmap(maddr);

    return len;
}
#endif

struct vm_operations_struct windriver_vm_ops = {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,24)
    nopage:  windriver_vma_nopage,
#else
    fault:   windriver_vma_fault,
#endif
    close:   windriver_vma_close,
    open:    windriver_vma_open,
#ifdef CONFIG_HAVE_IOREMAP_PROT
    access:  windriver_vma_access,
#endif
};

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,9)
    #define REMAP_PAGE_RANGE remap_page_range
    #define REMAP_OFFSET(vma) VM_BYTE_OFFSET(vma)
#else
    #define REMAP_PAGE_RANGE remap_pfn_range
    #define REMAP_OFFSET(vma) VM_PG_OFFSET(vma)
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,5,13)
    #define LINUX_timespec_to_jiffies timespec_to_jiffies
    typedef struct timespec LINUX_timespec_t;
#else
    #define LINUX_timespec_to_jiffies timespec64_to_jiffies
    typedef struct timespec64 LINUX_timespec_t;
#endif

#ifndef pgprot_noncached /* Define only for architectures supported by
                          * WinDriver */
#if defined(__x86_64__)
   #define pgprot_noncached(prot) \
    (__pgprot(pgprot_val(prot) | _PAGE_PCD | _PAGE_PWT))
#elif defined(__i386__)
   #define pgprot_noncached(prot) \
    (boot_cpu_data.x86 > 3 ? \
        __pgprot(pgprot_val(prot) | _PAGE_PCD | _PAGE_PWT) : prot)
#endif
#endif

#if defined(__i386__)
#ifndef cpu_has_mtrr
   #define cpu_has_mtrr \
    (test_bit(X86_FEATURE_MTRR, (unsigned long *)boot_cpu_data.x86_capability))
#endif
#ifndef cpu_has_k6_mtrr
   #define cpu_has_k6_mtrr \
    (test_bit(X86_FEATURE_K6_MTRR, \
        (unsigned long *)boot_cpu_data.x86_capability))
#endif
#ifndef cpu_has_cyrix_arr
   #define cpu_has_cyrix_arr \
    (test_bit(X86_FEATURE_CYRIX_ARR, \
        (unsigned long *)boot_cpu_data.x86_capability))
#endif
#ifndef cpu_has_centaur_mcr
   #define cpu_has_centaur_mcr \
    (test_bit(X86_FEATURE_CENTAUR_MCR, \
        (unsigned long *)boot_cpu_data.x86_capability))
#endif
#endif

static inline int uncached_access(struct file *file, unsigned long addr)
{
#if defined(__i386__)
    if (file->f_flags & O_SYNC)
        return 1;
    return !(cpu_has_mtrr || cpu_has_k6_mtrr || cpu_has_cyrix_arr ||
        cpu_has_centaur_mcr) && (addr >= __pa(high_memory));
#elif defined(__x86_64__)
    if (file->f_flags & O_SYNC)
        return 1;
    return 0;
#else
    if (file->f_flags & O_SYNC)
        return 1;
    return addr >= __pa(high_memory);
#endif
}

#if defined(WINDRIVER_KERNEL)
    static int wd_mmap(struct file *file, struct vm_area_struct *vma)
    {
        LINUX_mmap_private *private_data;
        void *kernel_addr = NULL;

    #if LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0)
        vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP; /* don't swap out */
    #else
        vma->vm_flags |= VM_RESERVED; /* don't swap out */
    #endif

        private_data = (LINUX_mmap_private *)file->private_data;
        if (private_data)
            kernel_addr = private_data->kernel_addr;

        if (private_data && private_data->is_vmalloced)
        {

            /* Increase the ref count for the private_data that was allocated
               in LINUX_do_mmap. The data will be freed in windriver_vm_ops->close()
               when the ref count will be zero */
            LINUX_atomic_inc(&private_data->ref_count);

            /* Mapping Mem Buffer - Virtual address of allocation */
            vma->vm_file = file;
            vma->vm_private_data = private_data;
            vma->vm_ops = &windriver_vm_ops; /* Use "nopage" method for system
                                              * memory */
            file->private_data = NULL;
        }
        else
        {
            /* Mapping I/O buffer - Physical address mapping
             * Mapping DMA Buffer - Bus address */

            /* Free the private_data that was allocated in LINUX_do_mmap */
            if (private_data)
                LINUX_kfree(private_data);

            if (!kernel_addr)
            {
                vma->vm_flags |= VM_IO;
                vma->vm_ops = &windriver_vm_ops;
            }
            #if defined(__i386__) || defined(__x86_64__)
                #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25)
                else
                {
                    set_memory_uc((unsigned long)kernel_addr,
                        (vma->vm_end - vma->vm_start) / PAGE_SIZE);
                }
                #endif

                if (uncached_access(file, VM_BYTE_OFFSET(vma)) || kernel_addr)
                    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
            #else //ARM

            if (!kernel_addr && uncached_access(file, VM_BYTE_OFFSET(vma)))
                vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
            else //DMA
                vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

            #endif

            if (REMAP_PAGE_RANGE(vma, vma->vm_start, REMAP_OFFSET(vma),
                vma->vm_end - vma->vm_start, vma->vm_page_prot))
            {
                return -EAGAIN;
            }
        }
        return 0;
    }

    int LINUX_register_ioctl32_conversion(unsigned int cmd)
    {
    #if defined(CONFIG_COMPAT)
        #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
            return register_ioctl32_conversion(cmd, LINUX_ioctl_compat);
        #else
            return 0;
        #endif
    #else
        return -EINVAL;
    #endif
    }

    int LINUX_unregister_ioctl32_conversion(unsigned int cmd)
    {
    #if defined(CONFIG_COMPAT)
        #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
            return unregister_ioctl32_conversion(cmd);
        #else
            return 0;
        #endif
    #else
        return -EINVAL;
    #endif
    }

    #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
        static long LINUX_unlocked_ioctl(struct file *filp, unsigned int cmd,
            unsigned long args)
        {
            int ret;
            #if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
                struct inode *inode = filp->f_dentry->d_inode;
            #else
                struct inode *inode = filp->f_path.dentry->d_inode;
            #endif

        #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39)
            lock_kernel();
        #endif

            ret = WDlinuxIoctl(inode, filp, cmd, args);

        #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39)
            unlock_kernel();
        #endif

            return ret;
        }
    #endif

    struct file_operations windriver_fops = {
        owner: THIS_MODULE,
        #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
            ioctl: WDlinuxIoctl,
        #else
            unlocked_ioctl: LINUX_unlocked_ioctl,
        #endif
        mmap: wd_mmap,
        open: WDlinuxOpen,
        release: WDlinuxClose,
        #if defined(CONFIG_COMPAT) && \
            (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13))
            compat_ioctl: LINUX_unlocked_ioctl,
        #endif
    };

#if defined(UDEV_SUPPORT)
    static struct class_simple *windrvr_class = NULL;
#endif
    int LINUX_register_chrdev(unsigned int major, const char *name)
    {
#if defined(UDEV_SUPPORT)
        int err, new_major;

        new_major = register_chrdev(major, name, &windriver_fops);
        if (new_major < 0)
            return new_major;

        windrvr_class = class_simple_create(THIS_MODULE, (char *)name);
        if (IS_ERR(windrvr_class))
        {
            err = PTR_ERR(windrvr_class);
            printk("WinDriver: Failed to create class. err [%d]\n", err);
            goto Error;
        }

        class_simple_device_add(windrvr_class, MKDEV(new_major, 0), NULL, name);
        err = devfs_mk_cdev(MKDEV(new_major, 0), S_IFCHR | S_IRUSR | S_IWUSR,
            name);
        if (err)
        {
            printk("WinDriver: Failed to make devfs node. err [%d]\n", err);
            goto Error;
        }

        return new_major;

Error:
        if (windrvr_class)
        {
            class_simple_device_remove(MKDEV(new_major, 0));
            class_simple_destroy(windrvr_class);
        }
        unregister_chrdev(new_major, name);
        return err;
#else
        return register_chrdev(major, name, &windriver_fops);
#endif
    }

    void LINUX_unregister_chrdev(unsigned int major, const char *name)
    {
#if defined(UDEV_SUPPORT)
        devfs_remove(name);
        class_simple_device_remove(MKDEV(major, 0));
        class_simple_destroy(windrvr_class);
#endif
        unregister_chrdev(major, name);
    }
#endif

const char *LINUX_get_driver_name(void)
{
#if defined(WD_DRIVER_NAME_CHANGE)
    /* This section should only be compiled when building the
     * wizard generated kernel module */
    return "driver";
#else
    return "windrvr1511";
#endif
}

#if !defined(WINDRIVER_KERNEL)
    kp_register_mod_func_t LINUX_get_register_kp_module_func(void)
    {
    #if defined(WD_DRIVER_NAME_CHANGE)
        return driver_register_kp_module_func;
    #else
        return windrvr1511_register_kp_module_func;
    #endif
    }

    void LINUX_kp_inc_ref_count(void)
    {
        try_module_get(THIS_MODULE);
    }

    void LINUX_kp_dec_ref_count(void)
    {
        module_put(THIS_MODULE);
    }

    /* Dummy implementation of __stack_chk_fail() and __stack_chk_guard in case
       these symbols aren't defined */
    #if UINT_MAX == ULONG_MAX
        #define STACK_CHK_GUARD 0xe2dee396
    #else
        #define STACK_CHK_GUARD 0x595e9fbd94fda766
    #endif
    __attribute__((weak)) uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

    __attribute__((weak)) void __stack_chk_fail(void)
    {
    }

#else
    void unix_get_register_kp_module(kp_register_mod_func_t *func);

    void LINUX_init_register_kp_module_func(void)
    {
    #if defined(WD_DRIVER_NAME_CHANGE)
        unix_get_register_kp_module(&(driver_register_kp_module_func));
#else
        unix_get_register_kp_module(&(windrvr1511_register_kp_module_func));
#endif
    }
#endif

#if !defined(WINDRIVER_KERNEL)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
/* New work item API introduced kernel 2.6.20 */

struct work_handler_ctx
{
     struct work_struct bh;
     void (*func)(void *);
     void *func_data;
};

void work_handler_wrapper(struct work_struct *work)
{
    struct work_handler_ctx *handler_ctx = container_of(work,
        struct work_handler_ctx, bh);

    if (!handler_ctx || !handler_ctx->func)
        return;

    handler_ctx->func(handler_ctx->func_data);
}

void *LINUX_bh_alloc(void (*routine)(void *), void *data)
{
    struct work_handler_ctx *ctx = (struct work_handler_ctx *)
        vmalloc(sizeof(*ctx));

    if (!ctx)
        return NULL;

    memset(ctx, 0, sizeof(*ctx));
    ctx->func = routine;
    ctx->func_data = data;
    INIT_WORK(&ctx->bh, work_handler_wrapper);

    return (void *)(&ctx->bh);
}

void LINUX_bh_free(void *bh)
{
    struct work_struct *work = (struct work_struct *)bh;
    struct work_handler_ctx *ctx = container_of(work, struct work_handler_ctx,
        bh);

    if (ctx)
        vfree(ctx);
}

#else

void *LINUX_bh_alloc(void (*routine)(void *), void *data)
{
    struct work_struct *bh = (struct work_struct *)vmalloc(sizeof(*bh));

    if (!bh)
        return NULL;

    memset(bh, 0, sizeof(*bh));
    bh->data = data;
    bh->func = routine;
    bh->entry.next = &bh->entry;
    bh->entry.prev = &bh->entry;

    return (void *)bh;
}

void LINUX_bh_free(void *bh)
{
    vfree(bh);
}

#endif /* New work item API introduced kernel 2.6.20 */

void LINUX_flush_scheduled_tasks(void)
{
    flush_scheduled_work();
}

void LINUX_schedule_task(void *bh)
{
    schedule_work(bh);
}

#if defined(WINDRIVER_KERNEL)

extern int wd_intr_handler_linux(void *context, int irq);

irqreturn_t
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
wrapper_handler(int irq, void *ctx, struct pt_regs *pt)
#else
wrapper_handler(int irq, void *ctx)
#endif
{
    return wd_intr_handler_linux(ctx, irq);
}

int LINUX_request_irq(unsigned int irq, int is_shared, const char *device,
    void *ctx)
{
    unsigned long flag_disabled;
    unsigned long flag_shared;
    unsigned long flags;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
    flag_disabled = SA_INTERRUPT;
    flag_shared = SA_SHIRQ;
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,1,0)
    flag_disabled = IRQF_DISABLED;
    flag_shared = IRQF_SHARED;
#else
    flag_disabled = 0; // IRQF_DISABLED is now deprecated
    flag_shared = IRQF_SHARED;
#endif
    flags = flag_disabled;
    if (is_shared)
        flags |= flag_shared;

    return request_irq(irq, wrapper_handler, flags, device, ctx);
}

void LINUX_free_irq(unsigned int irq, void *ctx)
{
    free_irq(irq, ctx);
}

int LINUX_get_irq(void *pdev)
{
    return ((struct pci_dev *)pdev)->irq;
}

unsigned int LINUX_pci_is_domains_supported(void)
{
#ifdef CONFIG_PCI_DOMAINS_GENERIC
    return 1;
#else
    return 0;
#endif
}

unsigned int LINUX_pci_get_domain(void *pdev)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,71)
    return 0;
#else
    return pci_domain_nr(((struct pci_dev *)pdev)->bus);
#endif
}

unsigned int LINUX_pci_get_bus(void *pdev)
{
    return ((struct pci_dev *)pdev)->bus->number;
}

unsigned int LINUX_pci_get_slotfunc(void *pdev)
{
    return ((struct pci_dev *)pdev)->devfn;
}

unsigned short LINUX_pci_get_vendor_id(void *pdev)
{
     return ((struct pci_dev *)pdev)->vendor;
}

unsigned short LINUX_pci_get_device_id(void *pdev)
{
     return ((struct pci_dev *)pdev)->device;
}

void* LINUX_pci_get_device(unsigned int vendor, unsigned int device, void *pdev)
{
    return (void *)pci_get_device(vendor, device, pdev);
}

void LINUX_pci_put_device(void *pdev)
{
    if (pdev)
        pci_dev_put((struct pci_dev *)pdev);
}

int LINUX_pci_enable_msi(void *pdev)
{
#ifdef CONFIG_PCI_MSI
    int nvec, ret;
    struct pci_dev *dev = (struct pci_dev *)pdev;

    #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
        ret = pci_enable_msi(dev);
        nvec = 1;
    #else
        unsigned short msgctl;
        int msi_cap = pci_find_capability(dev, PCI_CAP_ID_MSI);

        pci_read_config_word(dev, msi_cap + PCI_MSI_FLAGS, &msgctl);
        ret = 1 << ((msgctl & PCI_MSI_FLAGS_QMASK) >> 1);

        #if LINUX_VERSION_CODE < KERNEL_VERSION(3,16,0)
            do {
                nvec = ret;
                ret = pci_enable_msi_block(dev, nvec);
            } while (ret > 0);
        #else
            #if LINUX_VERSION_CODE <= KERNEL_VERSION(4,11,0)
                ret = pci_enable_msi_range(dev, 1, ret);
            #else
                ret = pci_alloc_irq_vectors(dev, 1, ret, PCI_IRQ_MSI);
            #endif
            nvec = ret;
        #endif
    #endif

    if (ret < 0)
        return ret;

    return nvec;
#else
    return -1;
#endif
}

void LINUX_pci_disable_msi(void *pdev)
{
    #ifdef CONFIG_PCI_MSI
        pci_disable_msi((struct pci_dev *)pdev);
    #endif
}

#if defined(CONFIG_PCI_MSI)
    typedef struct msix_vectors_t
    {
        struct msix_vectors_t *next;
        struct msix_entry *entries;
        int num_entries;
        void *pdev;
    } msix_vectors_t;

    typedef struct
    {
        msix_vectors_t *list;
        spinlock_t lock;
    } msix_vectors_list_t;

    static msix_vectors_list_t msix_vectors_list = {NULL,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39)
        SPIN_LOCK_UNLOCKED
#else
        __SPIN_LOCK_UNLOCKED()
#endif
    };
#endif

int LINUX_request_irq_msix(void *pdev, int is_shared, const char *device,
    void *ctx)
{
#if defined(CONFIG_PCI_MSI)
    msix_vectors_t *v;
    int i, rc = 0;

    spin_lock_irq(&msix_vectors_list.lock);
    for (v = msix_vectors_list.list; v && v->pdev != pdev; v = v->next);
    spin_unlock_irq(&msix_vectors_list.lock);
    if (!v)
        return -1;

    for (i = 0; i < v->num_entries && !rc; i++)
        rc = LINUX_request_irq(v->entries[i].vector, is_shared, device, ctx);

    return rc;
#else
    return -1;
#endif
}

void LINUX_free_irq_msix(void *pdev, void *ctx)
{
#if defined(CONFIG_PCI_MSI)
    msix_vectors_t *v;
    int i;

    spin_lock_irq(&msix_vectors_list.lock);
    for (v = msix_vectors_list.list; v && v->pdev != pdev; v = v->next);
    spin_unlock_irq(&msix_vectors_list.lock);

    if (!v)
        return;

    for (i = 0; i < v->num_entries; i++)
        free_irq(v->entries[i].vector, ctx);
#endif
}

#if defined(CONFIG_PCI_MSI)
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,14,0)
#define msi_control_reg(base)           (base + PCI_MSI_FLAGS)
#define msix_table_size(control)        ((control & PCI_MSIX_FLAGS_QSIZE)+1)
#define multi_msix_capable(control)     msix_table_size((control))
static int LINUX_pci_msix_table_size(struct pci_dev *dev)
{
    int pos;
    u16 control;

    pos = pci_find_capability(dev, PCI_CAP_ID_MSIX);
    if (!pos)
        return 0;

    pci_read_config_word(dev, msi_control_reg(pos), &control);
    return multi_msix_capable(control);
}
#endif
#endif

int LINUX_pci_msix_vec_count(void *pdev)
{
#if defined(CONFIG_PCI_MSI)
    #if LINUX_VERSION_CODE < KERNEL_VERSION(3,14,0)
        return LINUX_pci_msix_table_size((struct pci_dev *)pdev);
    #else
        return pci_msix_vec_count((struct pci_dev *)pdev);
    #endif
#else
    return -1;
#endif
}

int LINUX_pci_enable_msix(void *pdev, int num_entries)
{
#if defined(CONFIG_PCI_MSI)
    int rc, i;
    struct msix_entry *entries;
    msix_vectors_t *v;

    entries = (struct msix_entry *)
        kmalloc(sizeof(struct msix_entry) * num_entries, GFP_KERNEL);
    if (!entries)
        return -ENOMEM;

    v = (msix_vectors_t *)kmalloc(sizeof(msix_vectors_t), GFP_KERNEL);
    if (!v)
    {
        kfree(entries);
        return -ENOMEM;
    }

    for (i = 0; i < num_entries; i++)
        entries[i].entry = i;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,12,0)
    /* pci_enable_msi_range() returns a negative errno if an error
     * occurs. If it succeeds, it returns the actual number of interrupts
     * allocated */
    rc = pci_enable_msix_range((struct pci_dev *)pdev, entries, 1, num_entries);
    if (rc <= 0)
#else
    /* pci_enable_msix() return value of 0 indicates the successful
     * configuration of MSI-X capability structure with new allocated MSI-X
     * irqs. A return of < 0 indicates a failure. Or a return of > 0 indicates
     * that the driver request is exceeding the number of irqs or MSI-X vectors
     * available */
    rc = pci_enable_msix((struct pci_dev *)pdev, entries, num_entries);
    if (rc > 0)
    {
        printk("%s: Failed enabling %d interrupts, retrying with %d interrupts"
            "\n", __FUNCTION__, num_entries, rc);
        num_entries = rc;
        rc = pci_enable_msix((struct pci_dev *)pdev, entries, num_entries);
    }
    if (rc)
#endif
    {
        printk("%s: Failed. rc [%d] num_entries [%d]\n", __FUNCTION__, rc,
            num_entries);
        kfree(entries);
        kfree(v);
        return rc;
    }

    v->entries = entries;
    v->num_entries = num_entries;
    v->pdev = pdev;
    spin_lock_irq(&msix_vectors_list.lock);
    v->next = msix_vectors_list.list;
    msix_vectors_list.list = v;
    spin_unlock_irq(&msix_vectors_list.lock);

    return 0;
#else
    return -1;
#endif
}

void LINUX_pci_disable_msix(void *pdev)
{
#if defined(CONFIG_PCI_MSI)
    msix_vectors_t **v, *tmp;

    spin_lock_irq(&msix_vectors_list.lock);
    for (v = &msix_vectors_list.list; *v && (*v)->pdev != pdev;
        v = &(*v)->next);
    if (!*v)
        return;

    tmp = *v;
    *v = (*v)->next;
    spin_unlock_irq(&msix_vectors_list.lock);

    kfree(tmp->entries);
        kfree(tmp);

    pci_disable_msix(pdev);
#endif
}

#endif /* WINDRIVER_KERNEL */

unsigned long LINUX_ioremap(unsigned long phys_addr, unsigned long size)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,5,13)
    return (unsigned long)ioremap_nocache(phys_addr, size);
#else
    return (unsigned long)ioremap(phys_addr, size);
#endif
}

void LINUX_iounmap(unsigned long addr)
{
    if (!is_high_memory(addr))
        return;

    iounmap((void *)addr);
}

unsigned long LINUX_do_mmap(struct file *file, unsigned long len,
    unsigned long addr_offset, void *kernel_addr, int is_vmalloced)
{
    unsigned long addr;
    unsigned long prot = PROT_READ | PROT_WRITE;
    unsigned long flag = MAP_SHARED;
    LINUX_mmap_private *private_data = NULL;

    /* When kernel address is not null, this is a mapping of DMA buffer or
     * a Mem Buffer */
    if (kernel_addr)
    {
        private_data = (LINUX_mmap_private *)kmalloc(
            sizeof(LINUX_mmap_private), GFP_KERNEL);
        if (!private_data)
            return -ENOMEM;

        private_data->is_vmalloced = is_vmalloced;
        private_data->kernel_addr = kernel_addr;
        LINUX_atomic_init(&private_data->ref_count);
    }

    /* addr_offset is the physical address when conducting user mapping by
     * physical address. (Contiguous buffer mapping)
     * When conducting user mapping by Virtual address (Vmalloc),
     * It is not used. (stored in vma->vm_pgoff and not used). */

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,5,0)
    down_write(&current->mm->mmap_sem);
    file->private_data = private_data;
    addr = do_mmap(file, 0, len, prot, flag, addr_offset);
    up_write(&current->mm->mmap_sem);
#else
    file->private_data = private_data;
    addr = vm_mmap(file, 0, len, prot, flag, addr_offset);
#endif

    return addr;
}

int LINUX_do_munmap(unsigned long addr, unsigned int len)
{
    if (!current->mm)
    {
        /* May occur on exit with Ctrl+C */
        printk("%s: mm is NULL\n", __FUNCTION__);
        return -1;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0)
    #ifdef DO_MUNMAP_API_CHANGE
        return vm_munmap(addr, (size_t)len, 0);
    #else
        return vm_munmap(addr, (size_t)len);
    #endif
#else
    #ifdef DO_MUNMAP_API_CHANGE
        return do_munmap(current->mm, addr, (size_t)len, 0);
    #else
        return do_munmap(current->mm, addr, (size_t)len);
    #endif
#endif
}

int LINUX_pcibios_present(void)
{
    return 1;
}

int LINUX_pcibios_read_config_byte(unsigned int domain, unsigned char bus,
    unsigned char dev_fn, unsigned int where, unsigned char *val)
{
    struct pci_dev *dev = LINUX_pci_find_slot(domain, bus, dev_fn);
    if (!dev)
        return -ENODEV;
    else
        return pci_read_config_byte(dev, where, val);
}

int LINUX_pcibios_read_config_word(unsigned int domain, unsigned char bus,
    unsigned char dev_fn, unsigned int where, unsigned short *val)
{
    struct pci_dev *dev = LINUX_pci_find_slot(domain, bus, dev_fn);
    if (!dev)
        return -ENODEV;
    else
        return pci_read_config_word(dev, where, val);
}

int LINUX_pcibios_read_config_dword(unsigned int domain, unsigned char bus,
    unsigned char dev_fn, unsigned int where, unsigned int *val)
{
    struct pci_dev *dev = LINUX_pci_find_slot(domain, bus, dev_fn);
    if (!dev)
        return -ENODEV;
    else
        return pci_read_config_dword(dev, where, val);
}

int LINUX_pcibios_write_config_byte(unsigned int domain, unsigned char bus,
    unsigned char dev_fn, unsigned int where, unsigned char val)
{
    struct pci_dev *dev = LINUX_pci_find_slot(domain, bus, dev_fn);
    if (!dev)
        return -ENODEV;
    else
        return pci_write_config_byte(dev, where, val);
}

int LINUX_pcibios_write_config_word(unsigned int domain, unsigned char bus,
    unsigned char dev_fn, unsigned int where, unsigned short val)
{
    struct pci_dev *dev = LINUX_pci_find_slot(domain, bus, dev_fn);
    if (!dev)
        return -ENODEV;
    else
        return pci_write_config_word(dev, where, val);
}

int LINUX_pcibios_write_config_dword(unsigned int domain, unsigned char bus,
    unsigned char dev_fn, unsigned int where, unsigned int val)
{
    struct pci_dev *dev = LINUX_pci_find_slot(domain, bus, dev_fn);
    if (!dev)
        return -ENODEV;
    else
        return pci_write_config_dword(dev, where, val);
}

int LINUX_mmconfig_enabled(void)
{
#if defined(CONFIG_PCI_MMCONFIG) || defined(CONFIG_PCI_GOMMCONFIG) || \
    defined(CONFIG_PCI_GOANY)
    return 1;
#else
    LINUX_printk("LINUX_mmconfig_enabled: mmconfig is disabled\n");
    return 0;
#endif
}

void LINUX_udelay(unsigned long usecs)
{
    udelay(usecs);
}

void LINUX_schedule(void)
{
    schedule();
}

long LINUX_schedule_timeout(long timeout)
{
    set_current_state(TASK_INTERRUPTIBLE);
    return schedule_timeout(timeout);
}

#if defined(WINDRIVER_KERNEL)

#if defined(WD_DRIVER_NAME_CHANGE)
EXPORT_SYMBOL(driver_register_kp_module_func);
#else
EXPORT_SYMBOL(windrvr1511_register_kp_module_func);
#endif

#endif /* WINDRIVER_KERNEL */

#if defined(WINDRIVER_KERNEL)
#include <linux/init.h>

#if defined(WD_DRIVER_NAME_CHANGE)
    #define WD_PCI_DRIVER_NAME "driver_pci"
#else
    #define WD_PCI_DRIVER_NAME "windrvr1511_pci"
#endif

/* OPTIONAL: Specify PCI/PCIe VID/PID Values you wish to take ownership on */
static struct pci_device_id pci_table[] =
{
  /* {0xFFFF, 0xFFFF, PCI_ANY_ID, PCI_ANY_ID}, */
};

static int wrap_generic_pci_probe(struct pci_dev *dev,
    const struct pci_device_id *id)
{
    unsigned int i;

    generic_pci_probe(dev, 0);

    /* Look the device up in the known PCI device list */
    for (i = 0; i < sizeof(pci_table) / sizeof(pci_table[0]); i++)
    {
        if (pci_table[i].vendor == dev->vendor &&
            pci_table[i].device == dev->device)
        {
            /* Take ownership */
            return 0;
        }
    }

    return -ENODEV;
}

static void wrap_generic_pci_remove(struct pci_dev *dev)
{
    generic_pci_remove(dev, 0);
}

DWORD pci_get_supported_interrupt_types(DWORD domain,
    DWORD bus, DWORD slot, DWORD func);
void LINUX_pci_get_pnp_data(void *dev_h, LINUX_pnp_data *p)
{
    struct pci_dev *dev = (struct pci_dev *)dev_h;

    if ((pci_get_supported_interrupt_types(LINUX_pci_get_domain(dev),
        dev->bus->number, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn)) != 0))
    {
        p->irq = dev->irq;
    }
    else
    {
        p->irq = -1;
    }
    p->domain = LINUX_pci_get_domain(dev);
    p->devfn = dev->devfn;
    p->bus_num = dev->bus->number;
    p->dev_h = dev;
    p->vid = dev->vendor;
    p->did = dev->device;
    p->hdr_type = dev->hdr_type;
}

void LINUX_pci_set_irq(void *dev_h, unsigned char irq)
{
    struct pci_dev *dev = (struct pci_dev *)dev_h;
    dev->irq = irq;
}

static const struct pci_device_id all_pci_ids[] = { {
    /* We want monitoring all devices */
    vendor: PCI_ANY_ID,
    device: PCI_ANY_ID,
    subvendor: PCI_ANY_ID,
    subdevice: PCI_ANY_ID,
        }, { /* End: All zeroes */ }
};

MODULE_DEVICE_TABLE(pci, all_pci_ids);

static struct pci_driver generic_pci_driver = {
    name: WD_PCI_DRIVER_NAME,
    id_table: &all_pci_ids [0],

    probe: wrap_generic_pci_probe,
    remove: wrap_generic_pci_remove,
};

#endif

int init_module(void)
{
    int ret;

    ret = init_module_cpp();
    if (ret)
    {
        printk("WinDriver: Init module failed. status [%d]\n", ret);
        return ret;
    }
    #if defined(USB_ONLY)
        printk("WinDriver: Skipping PCI init. Only usb supported on this"
            " device.\n");
        return 0;
    #endif

    #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
    pci_root_dev = pci_find_device(PCI_ANY_ID, PCI_ANY_ID, NULL);
    #else
    pci_root_dev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, NULL);
    #endif
    if (!pci_root_dev)
    {
        printk("WinDriver: Error. Unable to obtain pci_root_dev\n");
        cleanup_module_cpp();
        return -1;
    }

    #if defined(WINDRIVER_KERNEL)
        #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
            pci_module_init(&generic_pci_driver);
        #else
            ret = pci_register_driver(&generic_pci_driver);
            if (ret)
            {
                printk("WinDriver: pci_register_driver failed. status [%d]\n",
                    ret);
                return ret;
            }
        #endif
    #endif

    return 0;
}

void cleanup_module(void)
{
#if defined(WINDRIVER_KERNEL) && !defined (USB_ONLY)
    pci_unregister_driver(&generic_pci_driver);
#endif
    cleanup_module_cpp();
}

unsigned int LINUX_get_version(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,6)
    return 260;
#else
    return 266;
#endif
}

struct semaphore *LINUX_create_semaphore(void)
{
    struct semaphore *sem = (struct semaphore *)kmalloc(
        sizeof(struct semaphore), GFP_ATOMIC);

    sema_init(sem, 0);

    return sem;
}

void LINUX_free_semaphore(struct semaphore *sem)
{
    kfree((void *)sem);
}

unsigned long LINUX_jiffies()
{
    return jiffies;
}

long LINUX_get_time_in_sec()
{
    #if LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0)
        struct timeval tv;
        do_gettimeofday(&tv);
    #else
        struct timespec64 tv;
        ktime_get_real_ts64(&tv);
    #endif

    return tv.tv_sec;
}

long LINUX_get_time_in_millisec()
{
    long millisec;

    #if LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0)
        struct timeval tv;
        do_gettimeofday(&tv);
        millisec = tv.tv_usec / 1000; /* Converting microseconds to
                                       * milliseconds */
        millisec += tv.tv_sec * 1000; /* Converting seconds to milliseconds */
    #else
        struct timespec64 ts;
        ktime_get_real_ts64(&ts);
        millisec = ts.tv_nsec / NSEC_PER_MSEC; /* Converting nanoseconds
                                                * to milliseconds */
        millisec += ts.tv_sec * MSEC_PER_SEC; /* Converting seconds to
                                               * milliseconds */
    #endif

    return millisec;
}

#ifdef WINDRIVER_KERNEL
int KDBG_func_ap(long dwLevel, long dwSection, const char *format, va_list ap);

int LINUX_KDBG_func(long dwLevel, long dwSection, const char *format, ...)
{
    va_list ap;
    BOOL rc;

    va_start(ap, format);
    rc = KDBG_func_ap(dwLevel, dwSection, format, ap);
    va_end(ap);

    return rc;
}
#endif

int LINUX_printk(const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vprintk(fmt, ap);
    va_end(ap);

    return n;
}

int LINUX_sprintf(char *buf, const char *fmt, ...)
{
    int res;
    va_list args;

    va_start(args, fmt);
    res = vsprintf(buf, fmt, args);
    va_end(args);

    return res;
}

int LINUX_snprintf(char *buf, unsigned long n, const char *fmt, ...)
{
    int res;
    va_list args;

    va_start(args, fmt);
    res = vsnprintf(buf, n, fmt, args);
    va_end(args);

    return res;
}

int LINUX_vsprintf(char *buf, const char *fmt, va_list args)
{
    return vsprintf(buf, fmt, args);
}

int LINUX_vsnprintf(char *buf, unsigned long n, const char *fmt, va_list args)
{
    return vsnprintf(buf, n, fmt, args);
}

unsigned long LINUX_virt_to_phys(void *va)
{
    return virt_to_phys(va);
}

_u64 LINUX_dma_map_single(void *hwdev, void *va, unsigned long size,
    unsigned int dma_direction)
{
    /* pa - physical address (actually a bus address), va - virtual address */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
    struct device *dev = hwdev ? &((struct pci_dev *)hwdev)->dev : NULL;
    dma_addr_t pa = dma_map_single(dev, va, size, (int)dma_direction);

    if (dma_mapping_error(dev, pa))
#else
    dma_addr_t pa = pci_map_single(hwdev, va, size, (int)dma_direction);

    #if LINUX_VERSION_CODE < KERNEL_VERSION(3,6,0)
        #if defined(dma_mapping_error)
            if (pci_dma_mapping_error(pa))
        #else
            if (!pa)
        #endif
    #else
        if (!pa || (hwdev && pci_dma_mapping_error(hwdev, pa)))
    #endif
#endif
    {
        printk("%s: Failed. va [%p], pa [0x%llx], size [0x%lx]\n", __FUNCTION__,
            va, (_u64)pa, size);
        pa = 0;
    }

    return (_u64)pa;
}

void LINUX_dma_unmap_single(void *hwdev, _u64 pa, unsigned long size,
    unsigned int dma_direction)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
    dma_unmap_single(hwdev ? &((struct pci_dev *)hwdev)->dev : NULL,
        (dma_addr_t)pa, size, (int)dma_direction);
#else
    pci_unmap_single(hwdev, (dma_addr_t)pa, size, (int)dma_direction);
#endif
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,5)
    #define pci_dma_sync_single_for_cpu pci_dma_sync_single
    #define pci_dma_sync_sg_for_cpu pci_dma_sync_sg
    #define pci_dma_sync_single_for_device(x...)
    #define pci_dma_sync_sg_for_device(x...)
#endif

void LINUX_pci_dma_sync_single_for_cpu(void *dev, _u64 dma_addr,
    unsigned long size, unsigned int dma_direction)
{
    dma_sync_single_for_cpu(&((struct pci_dev *)dev)->dev, (dma_addr_t)dma_addr,
        size, (int)dma_direction);
}

void LINUX_pci_dma_sync_single_for_device(void *dev, _u64 dma_addr,
    unsigned long size, unsigned int dma_direction)
{
    dma_sync_single_for_device(&((struct pci_dev *)dev)->dev,
        (dma_addr_t)dma_addr, size, (int)dma_direction);
}

void LINUX_pci_dma_sync_sg_for_cpu(void *dev, void *dma_handle, int nelems,
    unsigned int dma_direction)
{
    dma_sync_sg_for_cpu(&((struct pci_dev *)dev)->dev, dma_handle, nelems,
        (int)dma_direction);
}

void LINUX_pci_dma_sync_sg_for_device(void *dev, void *dma_handle, int nelems,
    unsigned int dma_direction)
{
    dma_sync_sg_for_device(&((struct pci_dev *)dev)->dev, dma_handle,
        nelems, (int)dma_direction);
}

int LINUX_pci_set_dma_mask(void *dev, _u64 dma_mask)
{
    int err;

    #if LINUX_VERSION_CODE < KERNEL_VERSION(5,18,0)
        err = pci_set_dma_mask(dev, dma_mask);
    #else
        err = dma_set_mask(&((struct pci_dev *)dev)->dev, dma_mask);
    #endif
    if (err)
    {
        printk("%s: pci_set_dma_mask failed. err [%d], dma_mask [0x%llx]\n",
            __FUNCTION__, err, dma_mask);
        goto Exit;
    }

    #if LINUX_VERSION_CODE < KERNEL_VERSION(5,18,0)
        err = pci_set_consistent_dma_mask(dev, dma_mask);
    #else
        err = dma_set_coherent_mask(&((struct pci_dev *)dev)->dev, dma_mask);
    #endif
    if (err)
    {
        printk("%s: pci_set_consistent_dma_mask failed. err [%d], "
            "dma_mask [0x%llx]\n", __FUNCTION__, err, dma_mask);
        goto Exit;
    }

Exit:
    return err;
}

void LINUX_mem_map_reserve(void *addr, unsigned long size)
{
    struct page *page;

    if (!addr || !size)
        return;

    for (page = virt_to_page(addr); page <= virt_to_page(addr + size - 1);
        page++)
    {
        SetPageReserved(page);
    }
}

void LINUX_mem_map_unreserve(void *addr, unsigned long size)
{
    struct page *page;

    if (!addr || !size)
        return;

    for (page = virt_to_page(addr); page <= virt_to_page(addr + size - 1);
        page++)
    {
        ClearPageReserved(page);
    }
}

unsigned long LINUX_usecs_to_jiffies(unsigned long usecs)
{
    LINUX_timespec_t t;

    t.tv_sec = usecs / 1000000L;
    t.tv_nsec = (usecs - t.tv_sec * 1000000L) * 1000L;

    return LINUX_timespec_to_jiffies(&t);
}

unsigned long LINUX_msecs_to_jiffies(unsigned long msecs)
{
    LINUX_timespec_t t;

    t.tv_sec = msecs / 1000L;
    t.tv_nsec = (msecs - t.tv_sec * 1000L) * 1000000L;

    return LINUX_timespec_to_jiffies(&t);
}

void LINUX_add_timer(struct timer_list *timer, unsigned long timeout_msecs)
{
    timer->expires = jiffies + LINUX_msecs_to_jiffies(timeout_msecs);
    add_timer(timer);
}

void LINUX_create_timer(struct timer_list **timer,
    void *timer_cb, unsigned long ctx)
{
    *timer = vmalloc(sizeof(struct timer_list));

#if LINUX_VERSION_CODE < VERSION_CODE(4,15,0)
    init_timer(*timer);
    (*timer)->function = (void (*)(unsigned long))timer_cb;
    (*timer)->data = ctx;
#else
    timer_setup(*timer, (void (*)(struct timer_list *))timer_cb, 0);
#endif
}

void LINUX_del_timer(struct timer_list *timer)
{
    del_timer(timer);
}

void LINUX_destroy_timer(struct timer_list *timer)
{
    vfree(timer);
}

void LINUX_spin_lock_irqsave(os_spinlock_t *lock)
{
    spin_lock_irqsave((spinlock_t *)lock->spinlock, lock->flags);
}

void LINUX_spin_unlock_irqrestore(os_spinlock_t *lock)
{
    spin_unlock_irqrestore((spinlock_t *)lock->spinlock, lock->flags);
}

void LINUX_spin_lock_irq(os_spinlock_t *lock)
{
    spin_lock_irq((spinlock_t *)lock->spinlock);
}

void LINUX_spin_unlock_irq(os_spinlock_t *lock)
{
    spin_unlock_irq((spinlock_t *)lock->spinlock);
}

void LINUX_spin_lock_init(os_spinlock_t *lock)
{
    spinlock_t *sl;

    // adding 4 bytes since sizeof(spinlock_t) can be zero
    sl = kmalloc(sizeof(spinlock_t) + 4, GFP_ATOMIC);
    spin_lock_init(sl);
    lock->spinlock = (void *)sl;
}

void LINUX_spin_lock_uninit(os_spinlock_t *lock)
{
    kfree(lock->spinlock);
    lock->spinlock = NULL;
}

int LINUX_user_page_list_get(void *buf, unsigned long bytes, void **pl_h)
{
    int rc = 0, res;
    LINUX_page_list *pl;
    unsigned int page_count = PAGE_COUNT(buf, bytes);
    struct page **pages = NULL;

    *pl_h = NULL;

    /* User attempted Overflow! */
    if ((buf + bytes) < buf)
    {
        printk("%s: User attempted overflow\n", __FUNCTION__);
        return -EINVAL;
    }

    if (!(pl = kmalloc(sizeof(LINUX_page_list), GFP_KERNEL)))
        return -ENOMEM;

    memset(pl, 0, sizeof(LINUX_page_list));
    if (!(pages = kmalloc(page_count * sizeof(*pages), GFP_KERNEL)))
    {
        rc = -ENOMEM;
        goto Error;
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,8,0)
    down_read(&current->mm->mmap_sem);
#else
    down_read(&current->mm->mmap_lock);
#endif


    res = get_user_pages(
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0)
        current,
        current->mm,
#endif
        (unsigned long)buf,
        page_count,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,4,168)) || \
        (LINUX_VERSION_CODE >= KERNEL_VERSION(4,5,0) && \
         LINUX_VERSION_CODE < KERNEL_VERSION(4,9,0))
        1, /* read/write permission */
        1, /* Force: Only require the 'MAY' flag, e.g. allow "write" even to
            * readonly page */
#else
        FOLL_WRITE | FOLL_FORCE,
#endif
        pages,
        NULL);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,8,0)
    up_read(&current->mm->mmap_sem);
#else
    up_read(&current->mm->mmap_lock);
#endif

    pl->pages = pages;
    pl->page_count = page_count;
    pl->first_page_offset = (unsigned long)buf & (PAGE_SIZE - 1);
    pl->byte_count = bytes;

    if (res != page_count)
    {
        printk("%s: Error. Allocated [%d] pages, requested [%d]\n",
            __FUNCTION__, res, page_count);
        pl->page_count = res;
        rc = -EINVAL;
        goto Error;
    }

    *pl_h = pl;
    return 0;

Error:
    printk("%s: Error. rc [%d]\n", __FUNCTION__, rc);
    if (pl)
        LINUX_user_page_list_put(pl);
    return rc;
}

static void page_list_free(LINUX_page_list *pl)
{
    if (!pl)
        return;

    if (pl->pages)
        kfree(pl->pages);
    kfree(pl);
}

static void page_list_iterate(void *pl_h, void (*func)(struct page *))
{
    LINUX_page_list *pl = (LINUX_page_list *)pl_h;
    int i;

    if (!pl_h)
        return;

    for (i = 0; i < pl->page_count; i++)
        func(pl->pages[i]);
}

static void page_reserve_cb(struct page *page) { SetPageReserved(page); }
static void page_clear_cb(struct page *page) { ClearPageReserved(page); }
static void page_release_cb(struct page *page)
{
    if (!PageReserved(page))
        SetPageDirty(page);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0)
    page_cache_release(page);
#else
    put_page(page);
#endif
}

void LINUX_user_page_list_put(void *pl_h)
{
    page_list_iterate(pl_h, page_release_cb);
    page_list_free((LINUX_page_list *)pl_h);
}

void LINUX_page_list_lock(void *pl_h)
{
    page_list_iterate(pl_h, page_reserve_cb);
}

void LINUX_page_list_unlock(void *pl_h)
{
    page_list_iterate(pl_h, page_clear_cb);
}

void LINUX_get_hwinfo(char *buf, int size, int id)
{
    const char *tmp = dmi_get_system_info(id);
    if (tmp)
        strncpy(buf, tmp, size);
}

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define PL_OFFSET(initial_offset, offset) \
    (((initial_offset) + (offset)) & (~PAGE_MASK))
#define PL_INDEX(initial_offset, offset) \
    ((((initial_offset) + (offset)) & PAGE_MASK) >> PAGE_SHIFT)


static void *LINUX_page_addr_get(struct page *page,
    LINUX_page_addr_param *param)
{
    int is_page_high = PageHighMem(page);

    memset(param, 0, sizeof(LINUX_page_addr_param));

    if (is_page_high)
    {
        if (!param)
            return NULL;

        local_irq_save(param->flags);
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,4,0)
        param->map = (void *)(kmap_atomic(page, KM_IRQ0));
#else
        param->map = (void *)(kmap_atomic(page));
#endif
        if (!param->map)
        {
            printk("%s: Error. page_buf is NULL\n", __FUNCTION__);
            local_irq_restore(param->flags);
            return NULL;
        }

        return param->map;
    }

    return page_address(page);
}

void LINUX_page_addr_put(LINUX_page_addr_param *param)
{
    if (!param || !param->map)
        return;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,4,0)
    kunmap_atomic(param->map, KM_IRQ0);
#else
    kunmap_atomic(param->map);
#endif
    local_irq_restore(param->flags);
}

inline int LINUX_page_list_copy_inout(void *pl_h, unsigned long offset,
    void *buf, unsigned long bytes, int is_in)
{
    LINUX_page_list *pl = (LINUX_page_list *)pl_h;
    unsigned long page_offset, page_index, bytes_to_copy = 0;
    struct page **pages;
    void *page_buf;

    if (!bytes || !pl)
        return -1;

    pages = pl->pages;
    bytes = MIN(bytes, pl->byte_count);
    page_offset = PL_OFFSET(pl->first_page_offset, offset);
    page_index = PL_INDEX(pl->first_page_offset, offset);

    while (bytes && page_index < pl->page_count)
    {
        LINUX_page_addr_param param;

        /* The first copy is until the end of page */
        bytes_to_copy = MIN(bytes, PAGE_SIZE - page_offset);
        page_buf = LINUX_page_addr_get(pages[page_index], &param);
        if (!page_buf)
            return -1;

        page_buf+= page_offset;

        if (is_in)
            memcpy(page_buf, buf, bytes_to_copy);
        else
            memcpy(buf, page_buf, bytes_to_copy);

        LINUX_page_addr_put(&param);
        page_offset = 0;
        page_index++;
        buf += bytes_to_copy;
        bytes -= bytes_to_copy;
    }

    return 0;
}

/*
 * Copy from buffer to page list
 */
int LINUX_page_list_copyin(void *pl_h, unsigned long offset, const void *src,
    unsigned long bytes)
{
    return LINUX_page_list_copy_inout(pl_h, offset, (void *)src, bytes, 1);
}

/*
 * Copy from page list to buffer
 */
int LINUX_page_list_copyout(void *pl_h, unsigned long offset, void *dst,
    unsigned long bytes)
{
    return LINUX_page_list_copy_inout(pl_h, offset, dst, bytes, 0);
}

static struct page *LINUX_sg_page(struct scatterlist *sgl)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
     return sg_page(sgl);
#else
     return sgl->page;
#endif
}

static void LINUX_sg_set_page(struct scatterlist *sgl, struct page *page,
    unsigned int len, unsigned int offset)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
    sg_set_page(sgl, page, len, offset);
#else
    sgl->offset = offset;
    sgl->page = page;
    sgl->length = len;
#endif
}

static void LINUX_sg_init_table(struct scatterlist *sgl, unsigned int nents)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
    sg_init_table(sgl, nents);
#else
    memset(sgl, 0, sizeof(struct scatterlist) * nents);
#endif
}

int LINUX_get_sg_dma(void *buf, unsigned long size, void **sglptr)
{
    int rc = 0;
    unsigned int page_count = PAGE_COUNT(buf, size);
    struct scatterlist **sgl = (struct scatterlist **)sglptr;

    if (!(*sgl = vmalloc(sizeof(struct scatterlist) * page_count)))
    {
        rc = -ENOMEM;
    }
    return rc;
}

int LINUX_build_dma_list(LINUX_dma_page *page_list, void *buf,
    unsigned long size, unsigned int *dma_sglen, unsigned int dma_direction,
    void *dev_handle, void *sglptr)
{
    int rc, i, offset;
    struct page **pages = NULL;
    struct scatterlist *sgl = (struct scatterlist *)sglptr;
    struct scatterlist *sgEntry;
    unsigned int page_count;
    LINUX_page_list *pl = NULL;

    *dma_sglen = 0;

    rc = LINUX_user_page_list_get(buf, size, (void **)&pl);
    if (rc)
        goto Error;

    pages = pl->pages;
    page_count = pl->page_count;

    LINUX_sg_init_table(sgl, page_count);
    offset = ((unsigned long)buf) & (~PAGE_MASK);
    if (page_count > 1)
    {
        int length = PAGE_SIZE - offset;

        LINUX_sg_set_page(&sgl[0], pages[0], length, offset);

        size -= length;
        for (i = 1; i < page_count ; i++, size -= PAGE_SIZE)
        {
            LINUX_sg_set_page(&sgl[i], pages[i],
                size < PAGE_SIZE ? size : PAGE_SIZE, 0);
        }
    }
    else
    {
        LINUX_sg_set_page(&sgl[0], pages[0], size, offset);
    }

    #if LINUX_VERSION_CODE < KERNEL_VERSION(5,18,0)
    *dma_sglen = pci_map_sg((struct pci_dev *)dev_handle, sgl, page_count,
        (int)dma_direction);
    #else
    *dma_sglen = dma_map_sg(&((struct pci_dev *)dev_handle)->dev,
        sgl, page_count, (int)dma_direction);
    #endif

    if (!(*dma_sglen))
    {
        rc = -ENXIO;
        goto Error;
    }

    for_each_sg(sgl, sgEntry, *dma_sglen, i)
    {
        page_list[i].phys = sg_dma_address(sgEntry);
        page_list[i].size = sg_dma_len(sgEntry);
    }
    /* Release the page list without unlocking the user memory */
    page_list_free(pl);
    return 0;

Error:
    printk("%s: Error. rc [%d]\n", __FUNCTION__, rc);
    if (pl)
        LINUX_user_page_list_put(pl);
    if (sgl)
        LINUX_free_sg_list(sgl);

    return rc;
}

int LINUX_build_sg_dma(LINUX_dma_page *page_list, unsigned int *dma_sglen,
    void *buf, unsigned long size, unsigned int dma_direction, void *dev_handle,
    void **dma_handle)
{
    unsigned int page_count;
    int rc;
    void *sglptr;

    *dma_handle = NULL;

    rc = LINUX_get_sg_dma(buf, size, &sglptr);
    if (rc)
        goto Exit;

    rc = LINUX_build_dma_list(page_list, buf, size, &page_count,
        (int)dma_direction, dev_handle, sglptr);
    if (rc)
        goto Exit;

    *dma_sglen = page_count;
    *dma_handle = sglptr;

Exit:
    return rc;
}

void LINUX_free_sg_list(void *sgl)
{
    vfree((struct scatterlist *)sgl);
}

int LINUX_free_sg_pages(void *sglptr, unsigned int page_count,
    unsigned int dma_direction, void *dev_handle)
{
    int i;
    struct scatterlist *sgl = (struct scatterlist *)sglptr;

    #if LINUX_VERSION_CODE < KERNEL_VERSION(5,18,0)
    pci_unmap_sg((struct pci_dev *)dev_handle, sgl, page_count,
        (int)dma_direction);
    #else
    dma_unmap_sg(&((struct pci_dev *)dev_handle)->dev, sgl,
        page_count, (int)dma_direction);
    #endif

     for (i = 0; i < page_count; i++)
     {
         if (!PageReserved(LINUX_sg_page(&sgl[i])))
             SetPageDirty(LINUX_sg_page(&sgl[i]));
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0)
         page_cache_release(LINUX_sg_page(&sgl[i]));
#else
         put_page(LINUX_sg_page(&sgl[i]));
#endif
    }

    return 0;
}

int LINUX_free_sg_dma(void *dma_handle, void *buf, unsigned long size,
    unsigned int dma_direction, void *dev_handle)
{
    struct scatterlist *sgl = (struct scatterlist *)dma_handle;
    unsigned int page_count = PAGE_COUNT(buf, size);

    LINUX_free_sg_pages(sgl, page_count, dma_direction, dev_handle);
    LINUX_free_sg_list(sgl);

    return 0;
}

int LINUX_atomic_inc(os_interlocked_t *val)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
    return atomic_inc_return((atomic_t *)val);
#else
    atomic_t *v = (atomic_t *)val;
    atomic_inc(v);
    return v->counter;
#endif
}

int LINUX_atomic_dec(os_interlocked_t *val)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
    return atomic_dec_return((atomic_t *)val);
#else
    atomic_t *v = (atomic_t *)val;
    atomic_dec(v);
    return v->counter;
#endif
}

int LINUX_atomic_add(os_interlocked_t *val, int i)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
    return atomic_add_return(i, (atomic_t *)val);
#else
    atomic_t *v = (atomic_t *)val;
    atomic_add(i, v);
    return v->counter;
#endif
}

int LINUX_atomic_read(os_interlocked_t *val)
{
    return atomic_read((atomic_t *)val);
}

void LINUX_atomic_set(os_interlocked_t *val, int i)
{
    atomic_set((atomic_t *)val, i);
}

int LINUX_atomic_xchg(os_interlocked_t *val, int i)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
    return atomic_xchg((atomic_t *)val, i);
#else
    int ret = atomic_read((atomic_t *)val);

    atomic_set((atomic_t *)val, i);

    return ret;
#endif
}

#if 0
int LINUX_atomic_cmpxchg(os_interlocked_t *val, int cmp, int i)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
    return atomic_cmpxchg((atomic_t *)val, cmp, i);
#endif
    return 0;
}
#endif

void LINUX_atomic_init(os_interlocked_t *val)
{
    atomic_set((atomic_t *)val, 0);
}

void LINUX_atomic_uninit(os_interlocked_t *val)
{
    val = val;
}

void LINUX_event_wait(struct semaphore **event)
{
    int rc;

    rc = down_interruptible(*event);
}

void LINUX_event_create(struct semaphore **event)
{
    *event = LINUX_create_mutex();
}

void LINUX_event_set(struct semaphore **event)
{
   up(*event);
}

void LINUX_event_destroy(struct semaphore **event)
{
    LINUX_free_mutex(*event);
}

void LINUX_pci_set_master(void *dev_h)
{
    pci_set_master(dev_h);
}

int LINUX_pci_enable_device(void *dev_h)
{
    return pci_enable_device(dev_h);
}

void LINUX_pci_disable_device(void *dev_h)
{
    pci_disable_device(dev_h);
}

int LINUX_pci_request_regions(void *dev_h, char *modulename)
{
    return pci_request_regions(dev_h, modulename);
}

void LINUX_pci_release_regions(void *dev_h)
{
    pci_release_regions(dev_h);
}

unsigned char LINUX_inb(unsigned short port)
{
    return inb(port);
}

unsigned short LINUX_inw(unsigned short port)
{
    return inw(port);
}

unsigned int LINUX_inl(unsigned short port)
{
    return inl(port);
}

void LINUX_outb(unsigned char value, unsigned short port)
{
    outb(value, port);
}

void LINUX_outw(unsigned short value, unsigned short port)
{
    outw(value, port);
}

void LINUX_outl(unsigned int value, unsigned short port)
{
    outl(value, port);
}

void LINUX_insb(unsigned short port, void *addr, unsigned long count)
{
    insb(port, addr, count);
}
void LINUX_insw(unsigned short port, void *addr, unsigned long count)
{
    insw(port, addr, count);
}

void LINUX_insl(unsigned short port, void *addr, unsigned long count)
{
    insl(port, addr, count);
}

void LINUX_outsb(unsigned short port, void *addr, unsigned long count)
{
   outsb(port, addr, count);
}

void LINUX_outsw(unsigned short port, void *addr, unsigned long count)
{
   outsw(port, addr, count);
}

void LINUX_outsl(unsigned short port, void *addr, unsigned long count)
{
   outsl(port, addr, count);
}

char *LINUX_strcpy(char *dest, const char *src)
{
    return strcpy(dest, src);
}

char *LINUX_strcat(char *dest, const char *src)
{
    return strcat(dest, src);
}

char *LINUX_strncat(char *dest, const char *src, unsigned long count)
{
    return strncat(dest, src, count);
}

int LINUX_strcmp(const char *cs, const char *ct)
{
    return strcmp(cs, ct);
}

int LINUX_strncmp(const char *cs, const char *ct, unsigned long count)
{
   return strncmp(cs, ct, count);
}

void *LINUX_memset(void *adrr, int s, unsigned long count)
{
    return memset(adrr, s, count);
}

void *LINUX_memcpy(void *to, const void *from, unsigned long n)
{
   return memcpy(to, from, n);
}

int LINUX_memcmp(void *to, const void *from, unsigned long n)
{
   return memcmp(to, from, n);
}

unsigned long LINUX_strlen(const char *s)
{
    return strlen(s);
}

unsigned long LINUX_strnlen_s(const char *s, unsigned long n)
{
    if (!s)
        return 0;
    return strnlen(s, n);
}

char *LINUX_strncpy(char *dest, const char *src, unsigned long count)
{
    return strncpy(dest, src, count);
}

int LINUX_pci_resource_type(void *dev_h, int i)
{
    struct pci_dev *dev = (struct pci_dev *)dev_h;
    int flags = pci_resource_flags(dev, i);

    if (flags & IORESOURCE_IO)
        return WD_IO_RESOURCE_IO;

#ifdef IORESOURCE_MEM_64 /* IORESOURCE_MEM_64 not defined on older kernels */
    if (flags & IORESOURCE_MEM_64)
        return WD_IO_RESOURCE_MEM64;
#endif

    if (flags & IORESOURCE_MEM)
        return WD_IO_RESOURCE_MEM;

    return -1;
}

unsigned long LINUX_pci_resource_start(void *dev_h, int i)
{
    struct pci_dev *dev = (struct pci_dev *)dev_h;

    return pci_resource_start(dev, i);
}

unsigned long LINUX_pci_resource_len(void *dev_h, int i)
{
    struct pci_dev *dev = (struct pci_dev *)dev_h;

    return pci_resource_len(dev, i);
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,22)
struct pci_dev *LINUX_pci_get_bus_and_slot(unsigned int bus,
    unsigned int devfn)
{
    struct pci_dev *dev = NULL;

    while ((dev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, dev)) != NULL)
    {
        if (dev->bus->number == bus && dev->devfn == devfn)
            return dev;
    }
    return NULL;
}
#endif

void *LINUX_pci_find_slot(unsigned int domain, unsigned int bus,
    unsigned int devfn)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,22)
    return pci_find_slot(bus, devfn);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
    return pci_get_bus_and_slot(bus, devfn);
#else
    return pci_get_domain_bus_and_slot(domain, bus, devfn);
#endif
}

void *LINUX_pci_bus(void *dev_h)
{
    return ((struct pci_dev *)dev_h)->bus;
}

void *LINUX_pci_bus_subordinate(void *dev_h)
{
    return ((struct pci_dev *)dev_h)->subordinate;
}

#if defined(CONFIG_HOTPLUG)
void *LINUX_pci_find_bus(int domain, int busnr)
{
    return pci_find_bus(domain, busnr);
}

int LINUX_pci_scan_slot(void *bus_h, int devfn)
{
    return pci_scan_slot((struct pci_bus *)bus_h, devfn);
}

void LINUX_pci_bus_add_devices(void *bus_h)
{
    pci_bus_add_devices((struct pci_bus *)bus_h);
}

void LINUX_pci_bus_size_bridges(void *bus_h)
{
    pci_bus_size_bridges((struct pci_bus *)bus_h);
}

void LINUX_pci_bus_assign_resources(void *bus_h)
{
    pci_bus_assign_resources((struct pci_bus *)bus_h);
}

void LINUX_pci_remove_bus_device(void *dev_h)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
    pci_stop_and_remove_bus_device((struct pci_dev *)dev_h);
#else
    pci_remove_bus_device((struct pci_dev *)dev_h);
#endif
}

#else

void *LINUX_pci_find_bus(int domain, int busnr)
{
    return 0;
}

int LINUX_pci_scan_slot(void *bus_h, int devfn)
{
    return 0;
}

void LINUX_pci_bus_add_devices(void *bus_h)
{}

void LINUX_pci_bus_size_bridges(void *bus_h)
{}

void LINUX_pci_bus_assign_resources(void *bus_h)
{}

void LINUX_pci_remove_bus_device(void *dev_h)
{}
#endif

unsigned long LINUX_get_page_size(void)
{
    return PAGE_SIZE;
}

unsigned long LINUX_get_page_shift(void)
{
    return PAGE_SHIFT;
}

/*
 * The readX() and writeX() interface automatically swap bytes
 * on big endian hosts
 */
unsigned char LINUX_read8(volatile void *addr)
{
    return readb((void *)addr);
}

unsigned short LINUX_read16(volatile void *addr)
{
    return readw((void *)addr);
}

unsigned int LINUX_read32(volatile void *addr)
{
    return readl((void *)addr);
}

_u64 LINUX_read64(volatile void *addr)
{
#ifdef readq
    return readq((void *)addr);
#else /* readq is not defined for all platforms */
    return le64_to_cpu(*(volatile _u64 *)(addr));
#endif
}

void LINUX_write8(unsigned char val, volatile void *addr)
{
    writeb(val, addr);
}

void LINUX_write16(unsigned short val, volatile void *addr)
{
    writew(val, addr);
}

void LINUX_write32(unsigned int val, volatile void *addr)
{
    writel(val, addr);
}

void LINUX_write64(_u64 val, volatile void *addr)
{
#ifdef writeq
    writeq(val, addr);
#else /* writeq is not defined for all platforms */
    *(volatile _u64 *)(addr) = cpu_to_le64(val);
#endif
}

#if defined(WINDRIVER_KERNEL)

#define WD_NOT_IMPLEMENTED 0x2000000aL /* Taken from windrvr.h */

static struct pci_dev *LINUX_pci_get_next_device(struct pci_dev *dev)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
    return pci_find_device(PCI_ANY_ID, PCI_ANY_ID, dev);
#else
    return pci_get_device(PCI_ANY_ID, PCI_ANY_ID, dev);
#endif
}

int LINUX_pci_scan_devices(LINUX_device_info *devices, int size)
{
    struct pci_dev *dev = NULL;
    int i = 0;

    while ((dev = LINUX_pci_get_next_device(dev)) != NULL && i < size)

    {
        devices[i].domain = LINUX_pci_get_domain((void *)dev);
        devices[i].bus = dev->bus->number;
        devices[i].slot = PCI_SLOT(dev->devfn);
        devices[i].function = PCI_FUNC(dev->devfn);
        devices[i].vid = dev->vendor;
        devices[i].did = dev->device;
        i++;
    }

    return i;
}

#if !defined(LINUX_SRIOV_SUPPORT)

int OS_pci_enable_sriov(void *pdev, int nr_virtfn)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

void OS_pci_disable_sriov(void *pdev)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
}

int OS_pci_sriov_is_vf(void *pdev)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

int OS_pci_sriov_is_assigned(void *pdev)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

int OS_pci_sriov_get_num_vf(void *pdev)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

int OS_pci_sriov_vf_get_owner(void *pdev, unsigned int *dwDomain,
    unsigned int *dwBus, unsigned int *dwSlot, unsigned int *dwFunc)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

unsigned int LINUX_sriov_is_supported(void)
{
    return 0;
}

#else /* LINUX_SRIOV_SUPPORT */

int OS_pci_enable_sriov(void *pdev, int nr_virtfn)
{
    return WD_SRIOV_FUNC_NAME(OS_pci_enable_sriov)(pdev, nr_virtfn);
}

void OS_pci_disable_sriov(void *pdev)
{
    return WD_SRIOV_FUNC_NAME(OS_pci_disable_sriov)(pdev);
}

int OS_pci_sriov_is_vf(void *pdev)
{
    return WD_SRIOV_FUNC_NAME(OS_pci_sriov_is_vf)(pdev);
}

int OS_pci_sriov_is_assigned(void *pdev)
{
    return WD_SRIOV_FUNC_NAME(OS_pci_sriov_is_assigned)(pdev);
}

int OS_pci_sriov_get_num_vf(void *pdev)
{
    return WD_SRIOV_FUNC_NAME(OS_pci_sriov_get_num_vf(pdev));
}

int OS_pci_sriov_vf_get_owner(void *pdev, unsigned int *dwDomain,
    unsigned int *dwBus, unsigned int *dwSlot, unsigned int *dwFunc)
{
    return WD_SRIOV_FUNC_NAME(OS_pci_sriov_vf_get_owner)(pdev, dwDomain, dwBus,
        dwSlot, dwFunc);
}

unsigned int LINUX_sriov_is_supported(void)
{
    return 1;
}

#endif /* LINUX_SRIOV_SUPPORT */

#if !defined(LINUX_USB_SUPPORT)

/* Dummy OS_XXX functions in order to be able to load WinDriver when USB is not
 * supported and WDUSB module is not loaded */
DWORD OS_register_devices(void **register_ctx, WDU_MATCH_TABLE *match_tables,
    DWORD match_tabs_number)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

DWORD OS_unregister_devices(void *register_handle)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}
DWORD OS_get_device_info(HANDLE os_dev_h, void *buf, DWORD *buf_size,
    DWORD active_config, DWORD active_interface, DWORD active_setting,
    BOOL is_kernelmode, DWORD dwOptions)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

DWORD OS_set_interface(HANDLE os_dev_h,
    WDU_ALTERNATE_SETTING **alt_setting_info, DWORD interface_num,
    DWORD alt_num)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

DWORD OS_get_max_urb_transfer_size(BOOL high_speed, const pipe_t *pipe)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

DWORD OS_open_pipe(HANDLE os_dev_h,
    const WDU_ENDPOINT_DESCRIPTOR *endpoint_desc, pipe_t *pipe)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

DWORD OS_get_device_property(HANDLE os_dev_h, void *buf, DWORD *buf_size,
    WD_DEVICE_REGISTRY_PROPERTY prop)
{
    printk("OS_get_device_property: Not supported on this "
        "platform\n");
    return WD_NOT_IMPLEMENTED;
}

DWORD OS_close_device(HANDLE os_dev_h)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

DWORD OS_reset_pipe(HANDLE os_dev_h, pipe_t *pipe, PVOID ioctl_context)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

DWORD OS_selective_suspend(HANDLE os_dev_h, DWORD options)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

DWORD OS_transfer(HANDLE os_dev_h, pipe_t *pipe, void *file_h, PRCHANDLE prc_h,
    DWORD is_read, DWORD options, void *buf, DWORD bytes,
    DWORD *bytes_transferred, UCHAR *setup_packet, DWORD tout,
    PVOID ioctl_context)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

DWORD OS_halt_transfer(void *os_trans_ctx)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

BOOL OS_init(void)
{
    return TRUE;
}

void OS_uninit(void)
{
}

void OS_set_stream_context(HANDLE file_h, stream_context_t *context)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
}

stream_context_t * OS_get_stream_context(HANDLE file_h)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return (stream_context_t *)0;
}

DWORD OS_stream_request_insert(stream_t *stream, void *request)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

BOOL OS_is_stream_requests_queue_empty(stream_t *stream)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return TRUE;
}

DWORD OS_stream_transfer_create(HANDLE os_dev_h, pipe_t *pipe)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

DWORD OS_stream_transfer_start(stream_t *stream)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

void OS_stream_issue_new_transfers(void *ctx)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
}

DWORD OS_wakeup(HANDLE os_dev_h, DWORD options)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

DWORD OS_reset_device(HANDLE os_dev_h, DWORD options)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

int OS_num_pending_urbs(void *ctx)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

DWORD OS_lock_user_buff(PVOID *ptr, DWORD bytes, PVOID *lock_ctx,
    PVOID transform_context)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
    return WD_NOT_IMPLEMENTED;
}

void OS_unlock_user_buff(PVOID lock_ctx)
{
    printk("%s: Not supported on this platform\n", __FUNCTION__);
}

void wdusb_register_callbacks(wdusb_callbacks_t *callbacks, int *wdusb_ver)
{
    *wdusb_ver = WD_VER;
}

#else /* LINUX_USB_SUPPORT */

void wdusb_register_callbacks(wdusb_callbacks_t *callbacks, int *wdusb_ver)
{
    WD_USB_FUNC_NAME(wdusb_register_callbacks)(callbacks, wdusb_ver);
}

DWORD OS_register_devices(void **register_ctx, WDU_MATCH_TABLE *match_tables,
    DWORD match_tabs_number)
{
    return WD_USB_FUNC_NAME(OS_register_devices)(register_ctx, match_tables,
        match_tabs_number);
}

DWORD OS_unregister_devices(void *register_handle)
{
    return WD_USB_FUNC_NAME(OS_unregister_devices)(register_handle);
}

DWORD OS_get_device_property(HANDLE os_dev_h, void *buf, DWORD *buf_size,
    WD_DEVICE_REGISTRY_PROPERTY prop)
{
    return WD_USB_FUNC_NAME(OS_get_device_property)(os_dev_h, buf, buf_size,
        prop);
}

DWORD OS_get_device_info(HANDLE os_dev_h, void *buf, DWORD *buf_size,
    DWORD active_config, DWORD active_interface, DWORD active_setting,
    BOOL is_kernelmode, DWORD dwOptions)
{
    return WD_USB_FUNC_NAME(OS_get_device_info)(os_dev_h, buf, buf_size,
        active_config, active_interface, active_setting, is_kernelmode,
        dwOptions);
}

DWORD OS_set_interface(HANDLE os_dev_h,
    WDU_ALTERNATE_SETTING **alt_setting_info, DWORD interface_num,
    DWORD alternate_setting)
{
    return WD_USB_FUNC_NAME(OS_set_interface)(os_dev_h, alt_setting_info,
        interface_num, alternate_setting);
}

DWORD OS_get_max_urb_transfer_size(BOOL high_speed, const pipe_t *pipe)
{
    return WD_USB_FUNC_NAME(OS_get_max_urb_transfer_size)(high_speed, pipe);
}

DWORD OS_open_pipe(HANDLE os_dev_h,
    const WDU_ENDPOINT_DESCRIPTOR *endpoint_desc, pipe_t *pipe)
{
    return WD_USB_FUNC_NAME(OS_open_pipe)(os_dev_h, endpoint_desc, pipe);
}

DWORD OS_close_device(HANDLE os_dev_h)
{
    return WD_USB_FUNC_NAME(OS_close_device)(os_dev_h);
}

DWORD OS_reset_pipe(HANDLE os_dev_h, pipe_t *pipe, PVOID ioctl_context)
{
    return WD_USB_FUNC_NAME(OS_reset_pipe)(os_dev_h, pipe, ioctl_context);
}

DWORD OS_transfer(HANDLE os_dev_h, pipe_t *pipe, HANDLE file_h, PRCHANDLE prc_h,
    DWORD is_read, DWORD options, PVOID buf, DWORD bytes,
    DWORD *bytes_transferred, UCHAR *setup_packet, DWORD timeout,
    PVOID ioctl_context)
{
    return WD_USB_FUNC_NAME(OS_transfer)(os_dev_h, pipe, file_h, prc_h, is_read,
        options, buf, bytes, bytes_transferred, setup_packet, timeout,
        ioctl_context);
}

DWORD OS_stream_transfer_create(HANDLE os_dev_h, pipe_t *pipe)
{
    return WD_USB_FUNC_NAME(OS_stream_transfer_create)(os_dev_h, pipe);
}

DWORD OS_stream_transfer_start(stream_t *stream)
{
    return WD_USB_FUNC_NAME(OS_stream_transfer_start)(stream);
}

DWORD OS_halt_transfer(void *os_trans_ctx)
{
    return WD_USB_FUNC_NAME(OS_halt_transfer)(os_trans_ctx);
}

BOOL OS_init(void)
{
    return WD_USB_FUNC_NAME(OS_init)();
}

void OS_uninit(void)
{
    WD_USB_FUNC_NAME(OS_uninit)();
}

DWORD OS_wakeup(HANDLE os_dev_h, DWORD options)
{
    return WD_USB_FUNC_NAME(OS_wakeup)(os_dev_h, options);
}

DWORD OS_reset_device(HANDLE os_dev_h, DWORD options)
{
    return WD_USB_FUNC_NAME(OS_reset_device)(os_dev_h, options);
}

DWORD OS_selective_suspend(HANDLE os_dev_h, DWORD options)
{
    return WD_USB_FUNC_NAME(OS_selective_suspend)(os_dev_h, options);
}

stream_context_t *OS_get_stream_context(HANDLE file_h)
{
    return WD_USB_FUNC_NAME(OS_get_stream_context)(file_h);
}

void OS_set_stream_context(HANDLE file_h, stream_context_t *context)
{
    WD_USB_FUNC_NAME(OS_set_stream_context)(file_h, context);
}

DWORD OS_stream_request_insert(stream_t *stream, void *request)
{
    return WD_USB_FUNC_NAME(OS_stream_request_insert)(stream, request);
}

BOOL OS_is_stream_requests_queue_empty(stream_t *stream)
{
    return WD_USB_FUNC_NAME(OS_is_stream_requests_queue_empty)(stream);
}

void OS_stream_issue_new_transfers(void *ctx)
{
    WD_USB_FUNC_NAME(OS_stream_issue_new_transfers)(ctx);
}

int OS_num_pending_urbs(void *ctx)
{
    return WD_USB_FUNC_NAME(OS_num_pending_urbs)(ctx);
}

DWORD OS_lock_user_buff(PVOID *ptr, DWORD bytes, PVOID *lock_ctx,
    PVOID transform_context)
{
    return WD_USB_FUNC_NAME(OS_lock_user_buff)(ptr, bytes, lock_ctx,
        transform_context);
}

void OS_unlock_user_buff(PVOID lock_ctx)
{
    WD_USB_FUNC_NAME(OS_unlock_user_buff)(lock_ctx);
}

#endif /* LINUX_USB_SUPPORT */

#if defined (LINUX_GPUDIRECT_SUPPORT)

#include <nv-p2p.h>

static void gpudirect_free_callback(void *data)
{
    struct nvidia_p2p_dma_mapping *dma_mapping =
         (struct nvidia_p2p_dma_mapping *) data;
    nvidia_p2p_free_dma_mapping(dma_mapping);
}

#define GPU_BOUND_SHIFT   16
#define GPU_BOUND_SIZE    ((u64)1 << GPU_BOUND_SHIFT)
#define GPU_BOUND_OFFSET  (GPU_BOUND_SIZE-1)
#define GPU_BOUND_MASK    (~GPU_BOUND_OFFSET)

#endif

int LINUX_gpudirect_pin_memory(void* pdev, LINUX_dma_page *pages,
    unsigned int *num_pages, void *address, unsigned long size,
    void *page_table_ctx, void *dma_mapping_ctx)
{
    #if defined (LINUX_GPUDIRECT_SUPPORT)
        struct pci_dev *peer = (struct pci_dev *)pdev;
        struct nvidia_p2p_dma_mapping *dma_mapping;
        nvidia_p2p_page_table_t *page_table;
        int ret = 0, i = 0;
        size_t pages_size = 0;

        u64 virt_start = (u64)address & GPU_BOUND_MASK;
        size_t pin_size = (size_t)address + size - virt_start;
        if (!size)
        {
            ret = -1;
            goto Exit;
        }

        printk("%s: Entered, address [%p]\n", __FUNCTION__, address);
        ret = nvidia_p2p_get_pages(0, 0, virt_start, pin_size, &page_table,
            gpudirect_free_callback, &page_table);
        if (ret)
        {
            printk("%s: Pinning failed\n", __FUNCTION__);
            goto Exit;
        }

        ret = nvidia_p2p_dma_map_pages(peer, page_table, &dma_mapping);
        if (ret)
        {
            printk("%s: nvidia_p2p_dma_map_pages failed [%d]\n", __FUNCTION__,
                ret);
            goto Exit;
        }

        for (i = 0 ; i < dma_mapping->entries; i++)
        {
            pages[i].phys = dma_mapping->dma_addresses[i];
            switch (dma_mapping->page_size_type)
            {
            case NVIDIA_P2P_PAGE_SIZE_4KB:
                pages[i].size = 4 * 1024;
                break;
            case NVIDIA_P2P_PAGE_SIZE_64KB:
                pages[i].size = 64 * 1024;
                break;
            case NVIDIA_P2P_PAGE_SIZE_128KB:
                pages[i].size = 128 * 1024;
                break;
            default:
                pages[i].size = 0;
                break;
            }
            pages_size += pages[i].size;
            //printk("%s: Pinned page[%d] size [%ld] KBs\n", __FUNCTION__, i,
            //    pages[i].size);
        }
        *num_pages = dma_mapping->entries;
        page_table_ctx = (void *)page_table;
        dma_mapping_ctx = (void *)dma_mapping;
        printk("%s: Pinned [%d] pages, total size [%ld] KBs\n", __FUNCTION__,
            *num_pages, pages_size);
    Exit:
        return ret;
    #else
        return WD_NOT_IMPLEMENTED;
    #endif
}

void LINUX_gpudirect_unpin_memory(void *pdev, void *virt_address,
    void *page_table_ctx, void *dma_mapping_ctx)
{
    #if defined (LINUX_GPUDIRECT_SUPPORT)
        nvidia_p2p_page_table_t *page_table =
            (nvidia_p2p_page_table_t *)page_table_ctx;
        struct nvidia_p2p_dma_mapping *dma_mapping =
            (struct nvidia_p2p_dma_mapping *)dma_mapping_ctx;
        struct pci_dev *peer = (struct pci_dev *)pdev;

        printk("%s: Entered\n", __FUNCTION__);
        nvidia_p2p_dma_unmap_pages(peer, page_table, dma_mapping);

        nvidia_p2p_put_pages(0, 0, (u64)virt_address,page_table);
    #endif
}

#endif /* WINDRIVER_KERNEL */

