/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

using System;
using System.Runtime.InteropServices;

namespace Jungo
{
    using DWORD = System.UInt32;
    using CBOOL = System.UInt32;
    using BOOL = System.Boolean;
    using UINT64 = System.UInt64;
    using KPTR = System.UInt64;
    using UPTR = System.UInt64;
    using PHYS_ADDR = System.UInt64;
    using WORD = System.UInt16;
    using BYTE = System.Byte;
    using HANDLE = System.IntPtr;
    using WDC_DEVICE_HANDLE = System.IntPtr;
    using USHORT = System.Int16;
    using UCHAR = System.Byte;

    namespace wdapi_dotnet
    {
        public enum WD_EVENT_ACTION
        {
            WD_INSERT = 0x1,
            WD_REMOVE = 0x2,
            WD_OBSOLETE = 0x8,   // Obsolete
            WD_POWER_CHANGED_D0 = 0x10,  // Power states for the power
                                         // management
            WD_POWER_CHANGED_D1 = 0x20,
            WD_POWER_CHANGED_D2 = 0x40,
            WD_POWER_CHANGED_D3 = 0x80,
            WD_POWER_SYSTEM_WORKING = 0x100,
            WD_POWER_SYSTEM_SLEEPING1 = 0x200,
            WD_POWER_SYSTEM_SLEEPING2 = 0x400,
            WD_POWER_SYSTEM_SLEEPING3 = 0x800,
            WD_POWER_SYSTEM_HIBERNATE = 0x1000,
            WD_POWER_SYSTEM_SHUTDOWN = 0x2000,
            WD_IPC_UNICAST_MSG = 0x4000,
            WD_IPC_MULTICAST_MSG = 0x8000,
            WD_IPC_ALL_MSG = WD_IPC_UNICAST_MSG | WD_IPC_MULTICAST_MSG
        };
        public static class windrvr_consts
        {

            public const short WD_ACKNOWLEDGE = 0x1;
            public const int DMA_OPTIONS_ADDRESS_WIDTH_SHIFT = 24;
            public const int WD_KERNEL_PLUGIN_MAX_LENGTH = 128;
            public const int WD_CARD_ITEMS = 128; //enum
            public const int WD_PCI_CARDS = 256; // Slots max X Functions max
            public const int WD_PCI_MAX_CAPS = 50; //enum
            public const int WD_PCI_CAP_ID_ALL = 0x0; //enum
            public const int SLEEP_NON_BUSY = 1; //enum
            public const int WD_DMA_PAGES = 256; //enum
            public const int WD_LICENSE_LENGTH = 3072;
            public const int WD_FORCE_CLEANUP = 0x1; //enum
            public const int WD_IPC_MAX_PROCS = 0x40;
            public const int WD_PROCESS_NAME_LENGTH = 256;

            public const int WD_ACTIONS_POWER = (int)
                (WD_EVENT_ACTION.WD_POWER_CHANGED_D0 |
                    WD_EVENT_ACTION.WD_POWER_CHANGED_D1 |
                    WD_EVENT_ACTION.WD_POWER_CHANGED_D2 |
                    WD_EVENT_ACTION.WD_POWER_CHANGED_D3 |
                    WD_EVENT_ACTION.WD_POWER_SYSTEM_WORKING |
                    WD_EVENT_ACTION.WD_POWER_SYSTEM_SLEEPING1 |
                    WD_EVENT_ACTION.WD_POWER_SYSTEM_SLEEPING3 |
                    WD_EVENT_ACTION.WD_POWER_SYSTEM_HIBERNATE |
                    WD_EVENT_ACTION.WD_POWER_SYSTEM_SHUTDOWN);
            public const int WD_ACTIONS_ALL = WD_ACTIONS_POWER |
                (int)(WD_EVENT_ACTION.WD_INSERT |
                    WD_EVENT_ACTION.WD_REMOVE);
        };

        public enum WD_BUS_TYPE
        {
            WD_BUS_USB = -1,
            WD_BUS_UNKNOWN = 0,
            WD_BUS_ISA = 1,
            WD_BUS_EISA = 2, // ISA PnP belongs to this classification
            WD_BUS_PCI = 5,
        };

        public enum WD_INTERRUPT_OPTIONS
        {
            INTERRUPT_LEVEL_SENSITIVE = 1,
            INTERRUPT_CMD_COPY = 2,
            INTERRUPT_CE_INT_ID = 4
        };

        public delegate void INT_HANDLER(IntPtr pData);
        public delegate void EVENT_HANDLER_DOTNET(IntPtr pEvent, IntPtr pData);
        public delegate void DMA_TRANSACTION_CALLBACK(IntPtr pData);

        //enumurations
        public enum TRANSFER_OPTIONS
        {
            USB_ISOCH_RESET = 0x10,
            USB_ISOCH_FULL_PACKETS_ONLY = 0x20,
            USB_ABORT_PIPE = 0x40,
            USB_ISOCH_NOASAP = 0x80,
            USB_BULK_INT_URB_SIZE_OVERRIDE_128K = 0x100,
        };
        public enum USB_PIPE_TYPE
        {
            PIPE_TYPE_CONTROL = 0,
            PIPE_TYPE_ISOCHRONOUS = 1,
            PIPE_TYPE_BULK = 2,
            PIPE_TYPE_INTERRUPT = 3
        };

        public enum WDU_DIR
        {
            WDU_DIR_IN = 1,
            WDU_DIR_OUT = 2,
            WDU_DIR_IN_OUT = 3
        };

        public enum WDU_WAKEUP_OPTIONS
        {
            WDU_WAKEUP_ENABLE = 0x1,
            WDU_WAKEUP_DISABLE = 0x2
        };

        public enum WDU_SELECTIVE_SUSPEND_OPTIONS
        {
            WDU_SELECTIVE_SUSPEND_SUBMIT = 0x1,
            WDU_SELECTIVE_SUSPEND_CANCEL = 0x2
        };

        public enum WD_TRANSFER_CMD : DWORD
        {
            CMD_NONE = 0,       // No command
            CMD_END = 1,        // End command
            CMD_MASK = 2,       // Interrupt Mask

            RP_BYTE = 10,       // Read port byte
            RP_WORD = 11,       // Read port word
            RP_DWORD = 12,      // Read port dword
            WP_BYTE = 13,       // Write port byte
            WP_WORD = 14,       // Write port word
            WP_DWORD = 15,      // Write port dword
            RP_QWORD = 16,      // Read port qword
            WP_QWORD = 17,      // Write port qword


            RP_SBYTE = 20,      // Read port string byte
            RP_SWORD = 21,      // Read port string word
            RP_SDWORD = 22,     // Read port string dword
            WP_SBYTE = 23,      // Write port string byte
            WP_SWORD = 24,      // Write port string word
            WP_SDWORD = 25,     // Write port string dword
            RP_SQWORD = 26,     // Read port string qword
            WP_SQWORD = 27,     // Write port string qword

            RM_BYTE = 30,       // Read memory byte
            RM_WORD = 31,       // Read memory word
            RM_DWORD = 32,      // Read memory dword
            WM_BYTE = 33,       // Write memory byte
            WM_WORD = 34,       // Write memory word
            WM_DWORD = 35,      // Write memory dword
            RM_QWORD = 36,      // Read memory qword
            WM_QWORD = 37,      // Write memory qword

            RM_SBYTE = 40,      // Read memory string byte
            RM_SWORD = 41,      // Read memory string word
            RM_SDWORD = 42,     // Read memory string dword
            WM_SBYTE = 43,      // Write memory string byte
            WM_SWORD = 44,      // Write memory string word
            WM_SDWORD = 45,     // Write memory string dword
            RM_SQWORD = 46,     // Read memory string quad word
            WM_SQWORD = 47,     // Write memory string quad word
        };

        public enum WD_ERROR_CODES : int
        {
            WD_STATUS_SUCCESS = (int)0x0L,
            WD_STATUS_INVALID_WD_HANDLE = unchecked((int)0xFFFFFFFF),
            WD_WINDRIVER_STATUS_ERROR = (int)0x20000000L,
            WD_INVALID_HANDLE = (int)0x20000001L,
            WD_INVALID_PIPE_NUMBER = (int)0x20000002L,
            //Request to read from an OUT
            //(write) pipe or request to write to an IN (read) pipe
            WD_READ_WRITE_CONFLICT = (int)0x20000003L,
            //Maximum packet size is zero
            WD_ZERO_PACKET_SIZE = (int)0x20000004L,
            WD_INSUFFICIENT_RESOURCES = (int)0x20000005L,
            WD_UNKNOWN_PIPE_TYPE = (int)0x20000006L,
            WD_SYSTEM_INTERNAL_ERROR = (int)0x20000007L,
            WD_DATA_MISMATCH = (int)0x20000008L,
            WD_NO_LICENSE = (int)0x20000009L,
            WD_NOT_IMPLEMENTED = (int)0x2000000AL,
            WD_KERPLUG_FAILURE = (int)0x2000000BL,
            WD_FAILED_ENABLING_INTERRUPT = (int)0x2000000CL,
            WD_INTERRUPT_NOT_ENABLED = (int)0x2000000DL,
            WD_RESOURCE_OVERLAP = (int)0x2000000EL,
            WD_DEVICE_NOT_FOUND = (int)0x2000000FL,
            WD_WRONG_UNIQUE_ID = (int)0x20000010L,
            WD_OPERATION_ALREADY_DONE = (int)0x20000011L,
            WD_INTERFACE_DESCRIPTOR_ERROR = (int)0x20000012L,
            WD_SET_CONFIGURATION_FAILED = (int)0x20000013L,
            WD_CANT_OBTAIN_PDO = (int)0x20000014L,
            WD_TIME_OUT_EXPIRED = (int)0x20000015L,
            WD_IRP_CANCELED = (int)0x20000016L,
            WD_FAILED_USER_MAPPING = (int)0x20000017L,
            WD_FAILED_KERNEL_MAPPING = (int)0x20000018L,
            WD_NO_RESOURCES_ON_DEVICE = (int)0x20000019L,
            WD_NO_EVENTS = (int)0x2000001AL,
            WD_INVALID_PARAMETER = (int)0x2000001BL,
            WD_INCORRECT_VERSION = (int)0x2000001CL,
            WD_TRY_AGAIN = (int)0x2000001DL,
            WD_WINDRIVER_NOT_FOUND = (int)0x2000001EL,
            WD_INVALID_IOCTL = (int)0x2000001FL,
            WD_OPERATION_FAILED = (int)0x20000020L,
            WD_INVALID_32BIT_APP = (int)0x20000021L,
            WD_TOO_MANY_HANDLES = (int)0x20000022L,
            WD_NO_DEVICE_OBJECT = (int)0x20000023L,
            WD_MORE_PROCESSING_REQUIRED = unchecked((int)0xC0000016L),

            // USBD Status Codes
            // The following status codes are returned by USBD:
            // USBD status types:
            WD_USBD_STATUS_SUCCESS = (int)0x0L,
            WD_USBD_STATUS_PENDING = (int)0x40000000L,
            WD_USBD_STATUS_ERROR = unchecked((int)0x80000000),
            WD_USBD_STATUS_HALTED = unchecked((int)0xC0000000),
            /* USBD status codes:
                * NOTE: The following status codes are comprised of one of the
                * status types above and an error code [i.e. XYYYYYYY - where:
                * X = status type; YYYYYYY = error code].
                * The same error codes may also appear with one of the other status
                * types. */

            /* HC (Host Controller) status codes.
                * [NOTE: These status codes use the WD_USBD_STATUS_HALTED status
                * type]: */
            WD_USBD_STATUS_CRC = unchecked((int)0xC0000001),
            WD_USBD_STATUS_BTSTUFF = unchecked((int)0xC0000002),
            WD_USBD_STATUS_DATA_TOGGLE_MISMATCH = unchecked((int)0xC0000003),
            WD_USBD_STATUS_STALL_PID = unchecked((int)0xC0000004),
            WD_USBD_STATUS_DEV_NOT_RESPONDING = unchecked((int)0xC0000005),
            WD_USBD_STATUS_PID_CHECK_FAILURE = unchecked((int)0xC0000006),
            WD_USBD_STATUS_UNEXPECTED_PID = unchecked((int)0xC0000007),
            WD_USBD_STATUS_DATA_OVERRUN = unchecked((int)0xC0000008),
            WD_USBD_STATUS_DATA_UNDERRUN = unchecked((int)0xC0000009),
            WD_USBD_STATUS_RESERVED1 = unchecked((int)0xC000000A),
            WD_USBD_STATUS_RESERVED2 = unchecked((int)0xC000000B),
            WD_USBD_STATUS_BUFFER_OVERRUN = unchecked((int)0xC000000C),
            WD_USBD_STATUS_BUFFER_UNDERRUN = unchecked((int)0xC000000D),
            WD_USBD_STATUS_NOT_ACCESSED = unchecked((int)0xC000000F),
            WD_USBD_STATUS_FIFO = unchecked((int)0xC0000010),

            WD_USBD_STATUS_XACT_ERROR = unchecked((int)0xC0000011),
            WD_USBD_STATUS_BABBLE_DETECTED = unchecked((int)0xC0000012),
            WD_USBD_STATUS_DATA_BUFFER_ERROR = unchecked((int)0xC0000013),
            WD_USBD_STATUS_CANCELED = unchecked((int)0xC0010000),
            //Returned by HCD (Host Controller Driver) if a transfer is
            //submitted to an endpoint that is stalled:
            WD_USBD_STATUS_ENDPOINT_HALTED = unchecked((int)0xC0000030),
            //Software status codes
            //[NOTE: The following status codes have only the error bit set]:
            WD_USBD_STATUS_NO_MEMORY = unchecked((int)0x80000100),
            WD_USBD_STATUS_INVALID_URB_FUNCTION = unchecked((int)0x80000200),
            WD_USBD_STATUS_INVALID_PARAMETER = unchecked((int)0x80000300),
            //Returned if client driver attempts to close an endpoint/interface
            //or configuration with outstanding transfers:
            WD_USBD_STATUS_ERROR_BUSY = unchecked((int)0x80000400),
            //Returned by USBD if it cannot complete a URB request. Typically
            //this will be returned in the URB status field when the Irp is
            //completed with a more specific NT error code. [The Irp statuses
            //are indicated in WinDriver//s Monitor Debug Messages (wddebug_gui)
            //tool]:
            WD_USBD_STATUS_REQUEST_FAILED = unchecked((int)0x80000500),

            WD_USBD_STATUS_INVALID_PIPE_HANDLE = unchecked((int)0x80000600),
            //Returned when there is not enough bandwidth available
            //to open a requested endpoint:
            WD_USBD_STATUS_NO_BANDWIDTH = unchecked((int)0x80000700),
            // Generic HC (Host Controller) error:
            WD_USBD_STATUS_INTERNAL_HC_ERROR = unchecked((int)0x80000800),
            //Returned when a short packet terminates the transfer
            //i.e. USBD_SHORT_TRANSFER_OK bit not set:
            WD_USBD_STATUS_ERROR_SHORT_TRANSFER = unchecked((int)0x80000900),
            //Returned if the requested start frame is not within
            //USBD_ISO_START_FRAME_RANGE of the current USB frame,
            //NOTE: that the stall bit is set:
            WD_USBD_STATUS_BAD_START_FRAME = unchecked((int)0xC0000A00),
            //Returned by HCD (Host Controller Driver) if all packets in an iso
            //transfer complete with an error:
            WD_USBD_STATUS_ISOCH_REQUEST_FAILED = unchecked((int)0xC0000B00),
            //Returned by USBD if the frame length control for a given
            //HC (Host Controller) is already taken by another driver:
            WD_USBD_STATUS_FRAME_CONTROL_OWNED = unchecked((int)0xC0000C00),
            //Returned by USBD if the caller does not own frame length control
            //and attempts to relee or modify the HC frame length:
            WD_USBD_STATUS_FRAME_CONTROL_NOT_OWNED = unchecked((int)0xC0000D00),
            //Additional USB 2.0 software error codes added for USB 2.0:
            WD_USBD_STATUS_NOT_SUPPORTED = unchecked((int)0xC0000E00), //Returned for APIS not
                                                            // supported/implemented
            WD_USBD_STATUS_INAVLID_CONFIGURATION_DESCRIPTOR = unchecked((int)0xC0000F00),
            WD_USBD_STATUS_INSUFFICIENT_RESOURCES = unchecked((int)0xC0001000),
            WD_USBD_STATUS_SET_CONFIG_FAILED = unchecked((int)0xC0002000),
            WD_USBD_STATUS_BUFFER_TOO_SMALL = unchecked((int)0xC0003000),
            WD_USBD_STATUS_INTERFACE_NOT_FOUND = unchecked((int)0xC0004000),
            WD_USBD_STATUS_INAVLID_PIPE_FLAGS = unchecked((int)0xC0005000),
            WD_USBD_STATUS_TIMEOUT = unchecked((int)0xC0006000),
            WD_USBD_STATUS_DEVICE_GONE = unchecked((int)0xC0007000),
            WD_USBD_STATUS_STATUS_NOT_MAPPED = unchecked((int)0xC0008000),
            //Extended isochronous error codes returned by USBD.
            //These errors appear in the packet status field of an isochronous
            //transfer.
            //For some reon the controller did not access the TD sociated
            //with this packet:
            WD_USBD_STATUS_ISO_NOT_ACCESSED_BY_HW = unchecked((int)0xC0020000),
            //Controller reported an error in the TD.
            //Since TD errors are controller specific they are reported
            //generically with this error code:
            WD_USBD_STATUS_ISO_TD_ERROR = unchecked((int)0xC0030000),
            //The packet w submitted in time by the client but failed to reach
            //the miniport in time:
            WD_USBD_STATUS_ISO_NA_LATE_USBPORT = unchecked((int)0xC0040000),
            //The packet w not sent because the client submitted it too late
            //to transmit:
            WD_USBD_STATUS_ISO_NOT_ACCESSED_LATE = unchecked((int)0xC0050000)
        };

        public enum WD_DMA_OPTIONS
        {
            DMA_KERNEL_BUFFER_ALLOC = 0x1, // The system allocates a contiguous
                                           // buffer.
            // The user does not need to supply linear address.

            DMA_KBUF_BELOW_16M = 0x2, // If DMA_KERNEL_BUFFER_ALLOC is used,
            // this will make sure it is under 16M.

            DMA_LARGE_BUFFER = 0x4, // If DMA_LARGE_BUFFER is used,
            // the maximum number of pages are dwPages, and not
            // WD_DMA_PAGES. If you lock a user buffer (not a kernel
            // allocated buffer) that is larger than 1MB, then use this
            // option and allocate memory for pages.

            DMA_ALLOW_CACHE = 0x8,  // allow caching of contiguous memory

            DMA_KERNEL_ONLY_MAP = 0x10, // Only map to kernel, dont map to
                                        // user-mode.
            // relevant with DMA_KERNEL_BUFFER_ALLOC flag only

            DMA_FROM_DEVICE = 0x20, // memory pages are locked to be written by
                                    // device

            DMA_TO_DEVICE = 0x40, // memory pages are locked to be read by
                                  // device

            DMA_TO_FROM_DEVICE = (DMA_FROM_DEVICE | DMA_TO_DEVICE), // memory
                                  // pages are locked for both read and write

            DMA_ALLOW_64BIT_ADDRESS = 0x80, // use this for devices that support
            // 64bit DMA address

            DMA_ALLOW_NO_HCARD = 0x100, // Allow memory lock without hCard

            DMA_GET_EXISTING_BUF = 0x200, // Get existing buffer by hDma handle

            DMA_RESERVED_MEM = 0x400,

            DMA_KBUF_ALLOC_SPECIFY_ADDRESS_WIDTH = 0x800, // When using this
                // flag, the width of the address must be entered in the fourth
                // byte of dwOptions and then the allocated address will be
                // limited to this width.
                // Linux: works with contiguous buffers only.

            DMA_GET_PREALLOCATED_BUFFERS_ONLY = 0x1000, // Windows: Try to
                // allocate buffers from preallocated buffers pool ONLY (if
                // none of them are available, function will fail).

            DMA_TRANSACTION = 0x2000, // Windows: for DMA transactions only

            DMA_GPUDIRECT = 0x4000, // Linux only

            DMA_DISABLE_MERGE_ADJACENT_PAGES = 0x8000, // Disable merge adjacent
                // pages. In case the flag is omitted, the merge will take
                // place automatically. Used for scatter gather mode only.

            // backward compatibility
            DMA_READ_FROM_DEVICE = DMA_FROM_DEVICE,
            DMA_WRITE_TO_DEVICE = DMA_TO_DEVICE,
            DMA_DIRECTION_MASK = DMA_TO_FROM_DEVICE

        };

        public enum WD_INTERRUPT_WAIT_RESULT
        {
            INTERRUPT_RECEIVED = 0, /* Interrupt was received */
            INTERRUPT_STOPPED,      /* Interrupt was disabled during wait */
            INTERRUPT_INTERRUPTED,  /* Wait was interrupted before an actual
                                     * hardware interrupt was received */
        };

        public enum DEBUG_LEVEL
        {
            D_OFF = 0,
            D_ERROR = 1,
            D_WARN = 2,
            D_INFO = 3,
            D_TRACE = 4
        };

        public enum DEBUG_SECTION : long
        {
            S_ALL = 0xFFFFFFFF,
            S_IO = 0x8,
            S_MEM = 0x10,
            S_INT = 0x20,
            S_PCI = 0x40,
            S_DMA = 0x80,
            S_MISC = 0x100,
            S_LICENSE = 0x200,
            S_PNP = 0x1000,
            S_CARD_REG = 0x2000,
            S_KER_DRV = 0x4000,
            S_USB = 0x8000,
            S_KER_PLUG = 0x10000,
            S_EVENT = 0x20000,
            S_IPC = 0x40000,
            S_KER_BUF = 0x80000,
        };

        public enum DEBUG_COMMAND
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
        };

        public enum ITEM_TYPE
        {
            ITEM_NONE = 0,
            ITEM_INTERRUPT = 1,
            ITEM_MEMORY = 2,
            ITEM_IO = 3,
            ITEM_BUS = 5,
        };

        // Structures (managed)
        public struct WD_VERSION
        {
            public DWORD dwVer;
            public String cVer;
        };

        public struct WD_LICENSE
        {
            public String cLicense; // Buffer with license string to put.
                                    // If empty string then get current license setting
                                    // into dwLicense.
            public DWORD dwLicense; // Returns license settings: LICENSE_DEMO,
                                    // LICENSE_WD etc..., or 0 for invalid license.
            public DWORD dwLicense2; // Returns additional license settings, if
                                     // dwLicense could not hold all the information.

            // Then dwLicense will return 0.
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct WD_DMA_PAGE
        {
            public UINT64 pPhysicalAddr;   // Physical address of page.
            public DWORD dwBytes;          // Size of page.
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct WD_DMA
        {
            public DWORD hDma;             // Handle of DMA buffer
            public IntPtr pUserAddr;       // Beginning of buffer.
            public KPTR pKernelAddr;       // Kernel mapping of kernel allocated buffer
            public DWORD dwBytes;          // Size of buffer.
            public DWORD dwOptions;        // Allocation options:
                                           // DMA_KERNEL_BUFFER_ALLOC, DMA_KBUF_BELOW_16M, DMA_LARGE_BUFFER
                                           // DMA_ALLOW_CACHE, DMA_KERNEL_ONLY_MAP,
                                           // DMA_TO_DEVICE, DMA_ALLOW_64BIT_ADDRESS
            public DWORD dwPages;          // Number of pages in buffer.
            public DWORD hCard;            // Handle of relevant card as received from

            // The following 6 parameters are used for DMA transaction only

            [MarshalAs(UnmanagedType.FunctionPtr)]
            public DMA_TRANSACTION_CALLBACK DMATransactionCallback;
            public IntPtr DMATransactionCallbackCtx;

            public DWORD dwAlignment;  // required alignment, used for contiguous
                                       // mode only
            public DWORD dwMaxTransferSize;        // used for scatter gather mode
                                                   // only
            public DWORD dwTransferElementSize;    // used for scatter gather mode
                                                   //  only
            public DWORD dwBytesTransferred;       // bytes transferred count

            [MarshalAs(UnmanagedType.ByValArray,
                SizeConst = windrvr_consts.WD_DMA_PAGES)]
            public WD_DMA_PAGE[] Page;

        };

        [StructLayout(LayoutKind.Sequential)]
        public struct WD_BUS
        {
            public WD_BUS_TYPE dwBusType;  // Bus Type: ISA, EISA, PCI.
            public DWORD dwDomainNum;      // Domain number.
            public DWORD dwBusNum;         // Bus number.
            public DWORD dwSlotFunc;       // Slot number on Bus.
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct MEM
        {
            public PHYS_ADDR pPhysicalAddr;  // Physical address on card.
            public UINT64 qwBytes;           // Address range.
            public KPTR pTransAddr;          // Returns the address to pass on to
                                             // transfer commands.
            public UPTR pUserDirectAddr;     // Returns the address for direct user
                                             // read/write.
            public DWORD dwBar;              // Base Address Register number of PCI
                                             // card.
            public DWORD dwOptions;          // WD_ITEM_MEM_OPTIONS
            public KPTR pReserved;           // For internal use
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct I_O
        {

            public KPTR pAddr;           // Beginning of I/O address.
            public DWORD dwBytes;        // I/O range
            public DWORD dwBar;          // Base Address Register number of PCI card.
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct INT
        {
            public DWORD dwInterrupt; // Number of interrupt to install.
            public DWORD dwOptions;   // Interrupt options. For level sensitive
                                      // interrupts - set to: INTERRUPT_LEVEL_SENSITIVE
            public DWORD hInterrupt;  // Returns the handle of the interrupt installed.
            public DWORD dwReserved1; // For internal use
            public KPTR pReserved2;  // For internal use
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct WD_ITEMS
        {
            public DWORD item; // ITEM_TYPE
            public DWORD fNotSharable;
            public _I I;
        };

        [StructLayout(LayoutKind.Explicit)]
        public struct _I
        {
            [FieldOffset(0)]
            public MEM Mem;      // ITEM_MEMORY

            [FieldOffset(0)]
            public I_O IO;       // ITEM_IO

            [FieldOffset(0)]
            public INT Int;      // ITEM_INTERRUPT

            [FieldOffset(0)]
            public WD_BUS Bus;   // ITEM_BUS
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct WD_CARD
        {
            public DWORD dwItems;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst =
                windrvr_consts.WD_CARD_ITEMS)]
            public WD_ITEMS[] Item;
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct WD_CARD_REGISTER
        {
            public WD_CARD Card;           // Card to register.
            public DWORD fCheckLockOnly;   // Only check if card is lockable, return
                                           // hCard=1 if OK.
            public DWORD hCard;            // Handle of card.
            public DWORD dwOptions;        // Should be zero.
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
            public String cName;          // Name of card.
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 100)]
            public String cDescription;   // Description.
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct WD_CARD_CLEANUP
        {
            public DWORD hCard;            // Handle of card.
            public IntPtr Cmd;             // Buffer with WD_TRANSFER commands
            public DWORD dwCmds;           // Number of commands.
            public DWORD dwOptions;        // 0 (default) or WD_FORCE_CLEANUP
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct WD_KERNEL_PLUGIN_CALL
        {
            public DWORD hKernelPlugIn;
            public DWORD dwMessage;
            public IntPtr pData;
            public DWORD dwResult;
        };

        public struct WD_SLEEP
        {
            public DWORD dwMicroSeconds; // Sleep time in Micro Seconds (1/1,000,000
                                         // Second)
            public DWORD dwOptions;      // Can be: SLEEP_NON_BUSY (10000 uSec +)
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct WD_KERNEL_PLUGIN
        {
            public DWORD hKernelPlugIn;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
            public String cDriverName;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
            public String cDriverPath;
            // Should be NULL (exists for backward compatibility).
            // The driver will be searched in the operating
            // system's drivers/modules directory.
            public IntPtr pOpenData;
        };

        public struct WD_TRANSFER
        {
            public KPTR pPort;        // I/O port for transfer or kernel memory address
            public DWORD cmdTrans;    // Transfer command WD_TRANSFER_CMD.
                                      // Parameters used for string transfers:
            public DWORD dwBytes;     // For string transfer.
            public DWORD fAutoinc;    // Transfer from one port/address
                                      // or use incremental range of addresses.
            public DWORD dwOptions;   // Must be 0.

            [StructLayout(LayoutKind.Explicit)]
            public struct DATA_T
            {

                [FieldOffset(0)]
                public Byte Byte;     // Use for 8 bit transfer.

                [FieldOffset(0)]
                public UInt16 Word;     // Use for 16 bit transfer.

                [FieldOffset(0)]
                public UInt32 Dword;   // Use for 32 bit transfer.

                [FieldOffset(0)]
                public UINT64 Qword;  // Use for 64 bit transfer.

                [FieldOffset(0)]
                public IntPtr pBuffer; // Use for string transfer. /* PVOID */
            };

            public DATA_T Data;
        };

        public enum WD_INTERRUPT_TYPE
        {
            INTERRUPT_LATCHED = 0x00,
            INTERRUPT_LEVEL_SENSITIVE = 0x01,
            INTERRUPT_CMD_COPY = 0x02,
            INTERRUPT_CE_INT_ID = 0x04,
            INTERRUPT_CMD_RETURN_VALUE = 0x08,
            INTERRUPT_MESSAGE = 0x10,
            INTERRUPT_MESSAGE_X = 0x20,
			INTERRUPT_DONT_GET_MSI_MESSAGE = 0x40
        };

        [StructLayout(LayoutKind.Sequential)]
        public class WD_INTERRUPT
        {
            public DWORD hInterrupt;    // Handle of interrupt.
            public DWORD dwOptions;     // Interrupt options: can be INTERRUPT_CMD_COPY

            public IntPtr Cmd;          // Commands to do on interrupt (WD_TRANSFER *)
            public DWORD dwCmds;        // Number of commands.

            // For WD_IntEnable():
            public WD_KERNEL_PLUGIN_CALL kpCall; // Kernel PlugIn call.
            public DWORD fEnableOk;     // TRUE if interrupt was enabled
                                        // (WD_IntEnable() succeed).

            // For WD_IntWait() and WD_IntCount():
            public DWORD dwCounter;     // Number of interrupts received.
            public DWORD dwLost;        // Number of interrupts not yet dealt with.
            public DWORD fStopped;      // Was interrupt disabled during wait.

            public DWORD dwLastMessage; // Message data of the last received MSI
                                        // (Windows)
            public DWORD dwEnabledIntType; // Interrupt type that was actually enabled
        };

        public struct WDC_INTERRUPT_PARAMS
        {
            public WD_TRANSFER[] pTransCmds;
            public DWORD dwNumCmds;
            public DWORD dwOptions;
            public IntPtr funcIntHandler;
            public IntPtr pData;
            public CBOOL fUseKP;
        };


        [StructLayout(LayoutKind.Sequential)]
        public struct WD_PCI_SLOT
        {
            public DWORD dwDomain;
            public DWORD dwBus;
            public DWORD dwSlot;
            public DWORD dwFunction;
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct WD_PCI_ID
        {
            public DWORD dwVendorId;
            public DWORD dwDeviceId;
        };

        public struct WD_PCI_CAP
        {
            public DWORD dwCapId;
            public DWORD dwCapOffset;
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct WD_PCI_CARD_INFO
        {
            public WD_PCI_SLOT pciSlot;    // Pci slot.
            public WD_CARD Card; // Get card parameters for PCI slot
        };

        public struct WD_PCI_CONFIG_DUMP
        {
            public WD_PCI_SLOT pciSlot;    // PCI bus, slot and function number.
            public IntPtr pBuffer;    // Buffer for read/write. /* PVOID */
            public DWORD dwOffset;   // Offset in PCI configuration space to
                                     // read/write from.
            public DWORD dwBytes;    // Bytes to read/write from/to buffer.
                                     // Returns the number of bytes read/written.
            public DWORD fIsRead;    // If 1 then read PCI config, 0 write PCI
                                     // config.
            public DWORD dwResult;   // PCI_ACCESS_RESULT
        };

        public struct PCI
        {
            public WD_PCI_ID cardId;
            public WD_PCI_SLOT pciSlot;
        };

        public struct USB
        {
            public DWORD dwUniqueID;
        };

        public struct IPC
        {
            public DWORD hIpc;         /* Acts as a unique identifier */
            public DWORD dwSubGroupID; /* Might be identical to same process running
                                 * twice (User implementation dependant) */
            public DWORD dwGroupID;

            public DWORD dwSenderUID;
            public DWORD dwMsgID;
            public UINT64 qwMsgData;
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct WDU_MATCH_TABLE
        {
            public WORD wVendorId;
            public WORD wProductId;
            public BYTE bDeviceClass;
            public BYTE bDeviceSubClass;
            public BYTE bInterfaceClass;
            public BYTE bInterfaceSubClass;
            public BYTE bInterfaceProtocol;

        };

        [StructLayout(LayoutKind.Sequential)]
        public struct WD_EVENT
        {
            public DWORD hEvent;
            public DWORD dwEventType; // WD_EVENT_TYPE

            public DWORD dwAction; // WD_EVENT_ACTION
            public DWORD dwEventId;
            public DWORD hKernelPlugIn;
            public DWORD dwOptions; // WD_EVENT_OPTION

            [StructLayout(LayoutKind.Explicit)]
            public struct U
            {
                [FieldOffset(0)]
                PCI Pci;
                [FieldOffset(0)]
                USB Usb;
                [FieldOffset(0)]
                IPC Ipc;
            };
            public U u;
            public DWORD dwNumMatchTables;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst =
                1)]
            public WDU_MATCH_TABLE[] matchTables;
        };

        [StructLayout(LayoutKind.Sequential)]
        public class WDC_DEVICE_N
        {
            public WD_PCI_ID id;              /* PCI device ID */
            public WD_PCI_SLOT slot;            /* PCI device slot location
                                                * information */
            public DWORD dwNumAddrSpaces; /* Total number of device's
                                                * address spaces */
            public IntPtr pAddrDesc;     /* Array of device's address
                                                *  spaces info */
            public WD_CARD_REGISTER cardReg;         /* Device's resources
                                                * information */
            public WD_KERNEL_PLUGIN kerPlug;         /* Kernel PlugIn information */

            public WD_INTERRUPT Int;             /* Interrupt information */
            public HANDLE hIntThread;

            public WD_EVENT Event;           /* Event information */
            public HANDLE hEvent;
            public IntPtr pCtx;
        };

        public class WDC_DEVICE
        {
            public WD_PCI_ID id;              /* PCI device ID */
            public WD_PCI_SLOT slot;            /* PCI device slot location
                                                * information */
            public DWORD dwNumAddrSpaces; /* Total number of device's
                                                * address spaces */
            public WDC_ADDR_DESC[] pAddrDesc;     /* Array of device's address
                                                *  spaces info */
            public WD_CARD_REGISTER cardReg;         /* Device's resources
                                                * information */
            public WD_KERNEL_PLUGIN kerPlug;         /* Kernel PlugIn information */

            public WD_INTERRUPT Int;             /* Interrupt information */
            public HANDLE hIntThread;

            public WD_EVENT Event;           /* Event information */
            public HANDLE hEvent;
            public IntPtr pCtx;

            public WDC_DEVICE_HANDLE hDev;
            public GCHandle hIntCmdsPin;

            public static implicit operator WDC_DEVICE(WDC_DEVICE_HANDLE hDev)
            {
                return hDevToWdcDevice(hDev);
            }

            public static WDC_DEVICE hDevToWdcDevice(WDC_DEVICE_HANDLE hDev)
            {
                WDC_DEVICE_N wdcDeviceNative =
                    Marshal.PtrToStructure<WDC_DEVICE_N>(hDev);
                WDC_DEVICE wdcDevice = new WDC_DEVICE();

                int size = Marshal.SizeOf(typeof(WDC_ADDR_DESC));
                IntPtr pAddrDesc = wdcDeviceNative.pAddrDesc;
                wdcDevice.pAddrDesc =
                    new WDC_ADDR_DESC[wdcDeviceNative.dwNumAddrSpaces];
                for (int i = 0; i < (int)wdcDeviceNative.dwNumAddrSpaces; i++)
                {
                    wdcDevice.pAddrDesc[i] =
                        Marshal.PtrToStructure<WDC_ADDR_DESC>(pAddrDesc);
                    pAddrDesc += size;
                }

                wdcDevice.id = wdcDeviceNative.id;
                wdcDevice.slot = wdcDeviceNative.slot;
                wdcDevice.dwNumAddrSpaces = wdcDeviceNative.dwNumAddrSpaces;
                wdcDevice.cardReg = wdcDeviceNative.cardReg;
                wdcDevice.kerPlug = wdcDeviceNative.kerPlug;
                wdcDevice.Int = wdcDeviceNative.Int;
                wdcDevice.Event = wdcDeviceNative.Event;
                wdcDevice.hDev = hDev;

                return wdcDevice;
            }
        };

        public struct WDU_PIPE_INFO
        {
            public DWORD dwNumber;
            public DWORD dwMaximumPacketSize;
            public DWORD type;
            public DWORD direction;
            public DWORD dwInterval;
        };

        public struct WDU_INTERFACE_DESCRIPTOR
        {
            public UCHAR bLength;
            public UCHAR bDescriptorType;
            public UCHAR bInterfaceNumber;
            public UCHAR bAlternateSetting;
            public UCHAR bNumEndpoints;
            public UCHAR bInterfaceCls;
            public UCHAR bInterfaceSubCls;
            public UCHAR bInterfaceProtocol;
            public UCHAR iInterface;
        };

        public struct WDU_ENDPOINT_DESCRIPTOR
        {
            public UCHAR bLength;
            public UCHAR bDescriptorType;
            public UCHAR bEndpointAddress;
            public UCHAR bmAttributes;
            public USHORT wMaxPacketSize;
            public UCHAR bInterval;
        };

        public struct WDU_CONFIGURATION_DESCRIPTOR
        {
            public UCHAR bLength;
            public UCHAR bDescriptorType;
            public USHORT wTotalLength;
            public UCHAR bNumInterfaces;
            public UCHAR bConfigurationValue;
            public UCHAR iConfiguration;
            public UCHAR bmAttributes;
            public UCHAR MaxPower;
        };

        public struct WDU_DEVICE_DESCRIPTOR
        {
            public UCHAR bLength;
            public UCHAR bDescriptorType;
            public USHORT bcdUSB;
            public UCHAR bDeviceCls;
            public UCHAR bDeviceSubCls;
            public UCHAR bDeviceProtocol;
            public UCHAR bMaxPacketSize0;
            public USHORT idVendor;
            public USHORT idProduct;
            public USHORT bcdDevice;
            public UCHAR iManufacturer;
            public UCHAR iProduct;
            public UCHAR iSerialNumber;
            public UCHAR bNumConfigurations;
        };

        public struct WDU_ALTERNATE_SETTING
        {
            public WDU_INTERFACE_DESCRIPTOR Descriptor;
            public IntPtr pEndpointDescriptors; // Pointer to WDU_ENDPOINT_DESCRIPTOR
            public IntPtr pPipes;               // Pointer to WDU_PIPE_INFO
        };

        public struct WDU_INTERFACE
        {
            public IntPtr pAlternateSettings;  //Pointer to WDU_ALTERNATE_SETTING
            public DWORD dwNumAltSettings;
            public IntPtr pActiveAltSetting; //Pointer to WDU_ALTERNATE_SETTING
        };

        public struct WDU_CONFIGURATION
        {
            public WDU_CONFIGURATION_DESCRIPTOR Descriptor;
            public DWORD dwNumInterfaces;
            public IntPtr pInterfaces;  //Pointer to WDU_INTERFACE
        };

        public struct WDU_DEVICE
        {
            public WDU_DEVICE_DESCRIPTOR Descriptor;
            public WDU_PIPE_INFO Pipe0;
            public IntPtr pConfigs;  //Pointer to WDU_CONFIGURATION
            public IntPtr pActiveConfig;  //Pointer to WDU_CONFIGURATION

            public IntPtr pActiveInterface(uint index)
            {
                WDU_CONFIGURATION wduConfig =
                        Marshal.PtrToStructure<WDU_CONFIGURATION>(pConfigs);

                if (index >= wduConfig.dwNumInterfaces)
                    return IntPtr.Zero;

                return new IntPtr(wduConfig.pInterfaces.ToInt64() +
                    index * Marshal.SizeOf<WDU_INTERFACE>());
            }
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct WD_IPC_PROCESS
        {
            [MarshalAs(UnmanagedType.ByValTStr,
                SizeConst = windrvr_consts.WD_PROCESS_NAME_LENGTH)]
            public string cProcessName;
            public DWORD dwSubGroupID; // Identifier of the processes type
            public DWORD dwGroupID;  // Unique identifier of the processes group for
                                     // discarding unrelated process. WinDriver developers are
                                     // encouraged to change their driver name before
                                     // distribution to avoid this issue entirely. */
            public DWORD hIpc;       // Returned from WD_IpcRegister()
        }

        public enum WD_KER_BUF_OPTION
        {
            /* KER_BUF_ALLOC_NON_CONTIG and KER_BUF_GET_EXISTING_BUF options are valid
             * only as part of "WinDriver for Server" API and require
             * "WinDriver for Server" license.
             * @note "WinDriver for Server" APIs are included in WinDriver evaluation
             * version. */
            KER_BUF_ALLOC_NON_CONTIG = 0x0001,
            KER_BUF_ALLOC_CONTIG = 0x0002,
            KER_BUF_ALLOC_CACHED = 0x0004,
            KER_BUF_GET_EXISTING_BUF = 0x0008
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct WD_KERNEL_BUFFER
        {
            public DWORD hKerBuf;    // Handle of Kernel Buffer
            public DWORD dwOptions;  // Refer to WD_KER_BUF_OPTION
            public UINT64 qwBytes;   // Size of buffer
            public KPTR pKernelAddr; // Kernel address
            public UPTR pUserAddr;   // User address
            public IntPtr phKerBuf;   // Native handle, used in free to avoid leaks
        }

        public class windrvr_decl
        {
            public const String WD_VERSION = "1511";
            public const String DLL_NAME = "wdapi" + WD_VERSION;
            public const String WD_VER_STRING = "15.1.1";
            public const String COPYRIGHTS_YEAR_STR = "2022";
            public const String JUNGO_COPYRIGHT =
                "Jungo Connectivity Confidential. Copyright (c) " +
                COPYRIGHTS_YEAR_STR +
                " Jungo Connectivity Ltd.  https://www.jungo.com";

            private class windrvr_decl_nonmanaged
            {
                // API declaration
                [DllImport(DLL_NAME)]
                public static unsafe extern char *WD_DriverName(String sName);
            }

            // API wrapper
            public static unsafe String WD_DriverName(String sName)
            {
                return Marshal.PtrToStringAnsi(new IntPtr(
                    windrvr_decl_nonmanaged.WD_DriverName(sName)));
            }
        }
    }
}
