using System;

using Jungo.wdapi_dotnet;
using static Jungo.wdapi_dotnet.wdc_lib_decl;
using static Jungo.diag_lib.Diag;
using static Jungo.wdapi_dotnet.WD_ERROR_CODES;
using static Jungo.wdapi_dotnet.wds_lib_decl;

using System.Collections.Generic;
using ConsoleTables;
using System.Text;
using System.Runtime.InteropServices;

using DWORD = System.UInt32;

namespace Jungo.diag_lib
{
    public static class wds_diag_lib
    {

        private static string DEFAULT_PROCESS_NAME = "Diagnostic program";
        private static string DEFAULT_SHARED_INT_NAME = "WinDriver IPC Shared Interrupt";
        private static string SAMPLE_BUFFER_DATA = "This is a sample buffer data";

        /* For IPC Menu */
        private static WD_KERNEL_BUFFER pIpcKerBuf = new();
        private static DWORD dwSubGroupId = 0;

        /* For Shared Buffer Menu */
        private static WD_KERNEL_BUFFER pSharedKerBuf = new();

        /* Unique identifier of the processes group to avoid getting messages from
         * processes made under WinDriver by other developers that use the same driver
         * name.
         * WinDriver developers are encouraged to change their driver name before
         * distribution to avoid this issue entirely. */
        private static DWORD DEFAULT_PROCESS_GROUP_ID = 0x12345678;

        public enum IPC_MSG_TYPE
        {
            IPC_MSG_KERNEL_BUFFER_READY = 1,
            /* Kernel buffer (Created with
             * WDS_SharedBufferAlloc()) ready to be
             * shared between processes. Kernel Buffer
             * handle is passed in the qwMsgData */

            IPC_MSG_CONTIG_DMA_BUFFER_READY = 2,
            /* Kernel buffer (Created with
             * WDC_DMAContigBufLock()) ready to be
             * shared between processes. DMA Buffer
             * handle is passed in the qwMsgData */

            /* TODO: Modify/Add values to communicate between processes */
        };

        private static void WDS_DIAG_SharedBufferFree(
            ref WD_KERNEL_BUFFER pSharedKerBuf)
        {
            DWORD dwStatus;

            if (pSharedKerBuf.hKerBuf == 0)
                return;

            dwStatus = WDS_SharedBufferFree(pSharedKerBuf);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "WDS_DIAG_SharedBufferFree: Failed freeing shared buffer memory");
            }

            pSharedKerBuf = new();

            Console.WriteLine("Shared buffer memory freed");
        }

        private static void AllocKernelBuff(ref WD_KERNEL_BUFFER pKerBuf,
            DWORD dwOptions)
        {
            uint size;
            DWORD dwStatus;

            size = InputDword(false, "Enter memory allocation size in bytes "
                + "(32 bit uint)");
            WDS_DIAG_SharedBufferFree(ref pKerBuf);

            dwStatus = WDS_SharedBufferAlloc(size, dwOptions, out pKerBuf);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "AllocKernelBuff: Failed allocating shared memory buffer");
            }

            Console.WriteLine("Shared buffer allocated. User addr [0x{0:x}], " +
                "kernel addr [0x{1:x}], size [{2}(0x{3:x}]", pKerBuf.pUserAddr,
                pKerBuf.pKernelAddr, size, size);

        }

        private static WD_ERROR_CODES MenuSharedBufferAllocContigOptionCb(
            object ctx)
        {
            AllocKernelBuff(ref pSharedKerBuf,
                (DWORD)WD_KER_BUF_OPTION.KER_BUF_ALLOC_CONTIG);

            return WD_STATUS_SUCCESS;
        }

        private static WD_ERROR_CODES MenuSharedBufferAllocNonContigOptionCb(
            object ctx)
        {
            AllocKernelBuff(ref pSharedKerBuf,
                (DWORD)WD_KER_BUF_OPTION.KER_BUF_ALLOC_NON_CONTIG);

            return WD_STATUS_SUCCESS;
        }

        private static WD_ERROR_CODES MenuSharedBufferExitCb(object ctx)
        {
            WDS_DIAG_SharedBufferFree(ref pSharedKerBuf);

            return WD_STATUS_SUCCESS;
        }

        public static void MenuSharedBufferInit(DiagMenuOption menuRoot)
        {
            DiagMenuOption menuSharedBufferMenuOption = new()
            {
                OptionName = "Allocate/free Shared Buffer",
                TitleName = "Shared Buffer Operations" + Environment.NewLine +
                    "E.g. for communicating with Kernel-Plugin",
                CbExit = MenuSharedBufferExitCb
            };
            DiagMenuOption MenuAllocateContigOption = new()
            {
                OptionName = "Allocate contiguous shared buffer",
                CbEntry = MenuSharedBufferAllocContigOptionCb
            };
            DiagMenuOption MenuAllocateNonContigOption = new()
            {
                OptionName = "Allocate non-contiguous shared buffer",
                CbEntry = MenuSharedBufferAllocNonContigOptionCb
            };
            DiagMenuOption MenuFreeMemoryOption = new()
            {
                OptionName = "Free shared buffer",
                CbEntry = MenuSharedBufferExitCb
            };

            IList<DiagMenuOption> menSharedBufferChildren =
                new List<DiagMenuOption>()
            {
                MenuAllocateContigOption,
                MenuAllocateNonContigOption,
                MenuFreeMemoryOption
            };

            menuSharedBufferMenuOption.AddChildren(menSharedBufferChildren);
            menuRoot.AddChild(menuSharedBufferMenuOption);
        }

        private static void ipc_msg_event_cb(WDS_IPC_MSG_RX pIpcRxMsg,
            IntPtr pData)
        {
            DWORD dwStatus;
            Console.WriteLine("Received an IPC message:" +
                "msgID [0x{0:x}], msgData [0x{1:x}] from process [0x{2:x}]",
                pIpcRxMsg.dwMsgID, pIpcRxMsg.qwMsgData, pIpcRxMsg.dwSenderUID);

            /* Important: Acquiring and using any resource (E.g. kernel/DMA buffer)
             * should be done from a deferred procedure to avoid jamming the IPC
             * incoming messages.
             * Notice you can pass private context at WDS_IpcRegister() and use it here
             * (pData) for signalling local thread for example.
             *
             * The following implementation is for sample purposes only! */
            try
            {
                switch (pIpcRxMsg.dwMsgID)
                {
                    case (DWORD)IPC_MSG_TYPE.IPC_MSG_KERNEL_BUFFER_READY:
                        DWORD sample_buffer_len;
                        WD_KERNEL_BUFFER pKerBuf = new();

                        Console.WriteLine("");
                        Console.WriteLine("This is a shared kernel buffer, " +
                            "getting it...");
                        Console.WriteLine("");

                        dwStatus = WDS_SharedBufferGet(
                            (DWORD)pIpcRxMsg.qwMsgData, out pKerBuf);
                        if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
                        {
                            throw new WinDriverException(dwStatus,
                                "ipc_msg_event_cb: Failed getting shared " +
                                "kernel buffer");
                        }


                        Console.WriteLine("Got a shared kernel buffer. UserAddr [0x{0:x}], " +
                            "KernelAddr [0x{1:x}], size [{2}]", pKerBuf.pUserAddr,
                            pKerBuf.pKernelAddr, pKerBuf.qwBytes);

                        /* Here we read SAMPLE_BUFFER_DATA from the received buffer */
                        sample_buffer_len = (DWORD)SAMPLE_BUFFER_DATA.Length;
                        if (pKerBuf.qwBytes > (ulong)sample_buffer_len + 1)
                        {
                            string sMessage = Marshal.PtrToStringAnsi(
                                new IntPtr((long)pKerBuf.pUserAddr));
                            Console.WriteLine("Sample data from kernel buffer [{0}]",
                                sMessage);
                        }
                        else
                        {
                            Console.WriteLine("Kernel buffer was too short for sample data");
                        }

                        /* For sample purpose we immediately release the buffer */
                        WDS_SharedBufferFree(pKerBuf);
                        break;

                    case (DWORD)IPC_MSG_TYPE.IPC_MSG_CONTIG_DMA_BUFFER_READY:
                        IntPtr pDma = new();
                        WD_DMA dma = new();

                        Console.WriteLine("");
                        Console.WriteLine("This is a DMA buffer, getting it...");
                        Console.WriteLine("");

                        dwStatus = WDC_DMABufGet((DWORD)pIpcRxMsg.qwMsgData, ref pDma);
                        if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
                        {
                            throw new WinDriverException(dwStatus,
                                "ipc_msg_event_cb: Failed getting DMA buffer.");
                        }

                        dma = Marshal.PtrToStructure<WD_DMA>(pDma);
                        Console.WriteLine("Got a DMA buffer. UserAddr [0x{0:x}], " +
                            "pPhysicalAddr [0x{1:x}], size [{2}(0x{3:x})]",
                            dma.pUserAddr, dma.Page[0].pPhysicalAddr,
                            dma.Page[0].dwBytes, dma.Page[0].dwBytes);

                        /* For sample purpose we immediately release the buffer */
                        WDC_DMABufUnlock(pDma);
                        break;

                    default:
                        Console.WriteLine("Unknown IPC type. msgID [0x{0:x}]," +
                            "msgData [0x{1:x}] from process [0x{2:x}]",
                            pIpcRxMsg.dwMsgID, pIpcRxMsg.qwMsgData,
                            pIpcRxMsg.dwSenderUID);
                        Console.WriteLine("");
                        break;
                }
            }
            catch (Exception e)
            {
                return;
            }
        }

        private static void ipc_shared_int_msg_event_cb(WDS_IPC_MSG_RX pIpcRxMsg,
            IntPtr pData)
        {
            Console.WriteLine("Shared Interrupt via IPC arrived:");
            Console.WriteLine("msgID [0x{0:x}], msgData [0x{1:x}] from process [0x{2:x}]",
                pIpcRxMsg.dwMsgID, pIpcRxMsg.qwMsgData, pIpcRxMsg.dwSenderUID);
        }

        /* Register process to IPC service */
        private static DWORD WDS_DIAG_IpcRegister()
        {
            DWORD dwSubGroupID = 0;
            DWORD dwStatus;

            dwSubGroupID = InputDword(true, "Enter process SubGroup ID(hex)",
                0, 0XFFFFFFFF);

            dwStatus = WDS_IpcRegister(DEFAULT_PROCESS_NAME, DEFAULT_PROCESS_GROUP_ID,
                dwSubGroupID, (DWORD)WD_EVENT_ACTION.WD_IPC_ALL_MSG,
                ipc_msg_event_cb, IntPtr.Zero /* Your cb ctx */);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "WDS_DIAG_IpcRegister: Failed registering process to IPC.");
            }

            Console.WriteLine("Registration completed successfully");
            return dwSubGroupID;
        }

        /* Enable Shared Interrupts via IPC */
        private static DWORD WDS_DIAG_IpcSharedIntsEnable()
        {
            DWORD dwSubGroupID = 0;
            DWORD dwStatus;

            if (WDS_IsSharedIntsEnabledLocally())
            {
                throw new WinDriverException(WD_OPERATION_ALREADY_DONE,
                    "WDS_DIAG_IpcSharedIntsEnable: Shared interrupts already enabled locally");
            }

            dwSubGroupID = InputDword(true, "Enter shared interrupt's SubGroup ID (hex)", 0,
                0XFFFFFFFF);

            /* WDS_SharedIntEnable() is called in this sample with pFunc=
            ipc_shared_int_msg_event_cb. This will cause a shared interrupt to invoke
            both this callback and the callback passed to WDS_IpcRegister() in
            WDS_DIAG_IpcRegister(). To disable the "general" IPC callback, pass
            pFunc=NULL in the above mentioned call.
            Note you can replace pFunc here with your own callback especially designed
            to handle interrupts */
            dwStatus = WDS_SharedIntEnable(DEFAULT_SHARED_INT_NAME,
                DEFAULT_PROCESS_GROUP_ID, dwSubGroupID, (DWORD)WD_EVENT_ACTION.WD_IPC_ALL_MSG,
                ipc_shared_int_msg_event_cb, IntPtr.Zero /* Your cb ctx */);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "WDS_DIAG_IpcSharedIntsEnable: Failed enabling shared interrupts via IPC.");
            }

            Console.WriteLine("Shared interrupts via IPC enabled successfully");
            return dwSubGroupID;
        }

        private static void WDS_DIAG_IpcScanProcs()
        {
            DWORD dwStatus, i;
            WDS_IPC_SCAN_RESULT ipcScanResult = new();

            dwStatus = WDS_IpcScanProcs(ref ipcScanResult);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "WDS_DIAG_IpcScanProcs: Failed scanning registered proccess");
            }

            if (ipcScanResult.dwNumProcs != 0)
            {
                Console.WriteLine("Found {0} processes in current group",
                    ipcScanResult.dwNumProcs);
                for (i = 0; i < ipcScanResult.dwNumProcs; i++)
                {
                    Console.WriteLine("  {0}) Name: {1}, SubGroup ID: 0x{2:x}, UID: 0x{3:x}",
                        i + 1,
                        ipcScanResult.procInfo[i].cProcessName,
                        ipcScanResult.procInfo[i].dwSubGroupID,
                        ipcScanResult.procInfo[i].hIpc);
                }
            }
            else
            {
                Console.WriteLine("No processes found in current group");
            }
        }

        private static void WDS_DIAG_IpcKerBufRelease()
        {
            DWORD dwStatus;

            if (pIpcKerBuf.hKerBuf == 0)
                return;

            /* Notice that once a buffer that was acquired by a different process is
             * freed, its kernel resources are kept as long as the other processes did
             * not release the buffer. */
            dwStatus = WDS_SharedBufferFree(pIpcKerBuf);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "WDS_DIAG_IpcKerBufRelease: Failed freeing shared buffer");
            }

            pIpcKerBuf = new();

            Console.WriteLine("Kernel buffer freed");
        }

        private static void WDS_DIAG_IpcKerBufAllocAndShare()
        {
            DWORD size = 0;
            DWORD dwStatus;
            DWORD dwOptions = (DWORD)WD_KER_BUF_OPTION.KER_BUF_ALLOC_CONTIG;
            DWORD sample_buffer_len;

            /* If kernel buffer was allocated in the past, release it */
            WDS_DIAG_IpcKerBufRelease();

            size = InputDword(true,
                    "Enter new kernel buffer size to allocate and share with current group",
                    1, 0xFFFFFFFF);


            dwStatus = WDS_SharedBufferAlloc(size, dwOptions, out pIpcKerBuf);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "WDS_DIAG_IpcKerBufAllocAndShare: Failed allocating kernel shared buffer");
            }

            Console.WriteLine("Successful kernel buffer allocation. UserAddr [0x{0:x}], " +
                "KernelAddr [0x{1:x}], size [{2}]", pIpcKerBuf.pUserAddr,
                pIpcKerBuf.pKernelAddr, size);

            /* Here we write SAMPLE_BUFFER_DATA to the new allocated buffer */
            sample_buffer_len = (DWORD)SAMPLE_BUFFER_DATA.Length;
            if (size > sample_buffer_len + 1)
            {
                IntPtr destPtr = new IntPtr((long)pIpcKerBuf.pUserAddr);
                byte[] data = Encoding.ASCII.GetBytes(SAMPLE_BUFFER_DATA);
                Marshal.Copy(data, 0, destPtr, data.Length);
                Console.WriteLine("Sample data written to kernel buffer");
            }
            else
            {
                Console.WriteLine("Kernel buffer is too short for sample data");
            }

            dwStatus = WDS_IpcMulticast(
                (DWORD)IPC_MSG_TYPE.IPC_MSG_KERNEL_BUFFER_READY,
                pIpcKerBuf.hKerBuf);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "WDS_DIAG_IpcAllocAndShareBuf: Failed sending message.");
            }

            Console.WriteLine("Kernel buffer shared successfully");
        }

        enum IPC_MESSAGE_SEND_OPTION
        {
            MENU_IPC_SEND_UID_UNICAST = 1,
            MENU_IPC_SEND_SUBGROUP_MULTICAST,
            MENU_IPC_SEND_MULTICAST
        }

        private static void WDS_DIAG_IpcSend(IPC_MESSAGE_SEND_OPTION ipc_menu_option)
        {
            DWORD recipientID = 0;
            DWORD messageID;
            UInt64 messageData;
            DWORD dwStatus = (DWORD)WD_STATUS_SUCCESS;

            if (ipc_menu_option == IPC_MESSAGE_SEND_OPTION.MENU_IPC_SEND_UID_UNICAST ||
                ipc_menu_option == IPC_MESSAGE_SEND_OPTION.MENU_IPC_SEND_SUBGROUP_MULTICAST)
            {
                recipientID = InputDword(true,
                    String.Format("Enter recipient{0}ID (hex)",
                    ipc_menu_option == IPC_MESSAGE_SEND_OPTION.MENU_IPC_SEND_SUBGROUP_MULTICAST ?
                    "(s) SubGroup " : " U"), 0, 0XFFFFFFFF);
                 
            }

            messageID = InputDword(true, "Enter your message ID (32Bit hex)",
                0, 0xFFFFFFFF);
            messageData = InputQword(true, "Enter your message (64Bit hex)",
                0, 0xFFFFFFFF);
            
            switch (ipc_menu_option)
            {
                case IPC_MESSAGE_SEND_OPTION.MENU_IPC_SEND_UID_UNICAST:
                    dwStatus = WDS_IpcUidUnicast(recipientID, messageID, messageData);
                    break;

                case IPC_MESSAGE_SEND_OPTION.MENU_IPC_SEND_SUBGROUP_MULTICAST:
                    dwStatus = WDS_IpcSubGroupMulticast(recipientID, messageID,
                        messageData);
                    break;

                case IPC_MESSAGE_SEND_OPTION.MENU_IPC_SEND_MULTICAST:
                    dwStatus = WDS_IpcMulticast(messageID, messageData);
                    break;
            }

            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "WDS_DIAG_IpcSend: Failed sending message.");
            }

            Console.WriteLine("Message sent successfully");
        }

        private static bool MenuIpcIsRegistered(DiagMenuOption menu)
        {
            return WDS_IsIpcRegistered();
        }

        private static bool MenuIpcIsNotRegistered(DiagMenuOption menu)
        {
            return !WDS_IsIpcRegistered();
        }

        private static WD_ERROR_CODES MenuIpcRegisterCb(object ctx)
        {
            dwSubGroupId = WDS_DIAG_IpcRegister();

            return WD_STATUS_SUCCESS;
        }

        private static WD_ERROR_CODES MenuIpcUnRegisterCb(object ctx)
        {
            WDS_IpcUnRegister();
            Console.WriteLine("Process unregistered successfully");

            return WD_STATUS_SUCCESS;
        }

        private static WD_ERROR_CODES MenuIpcScanCb(object ctx)
        {
            WDS_DIAG_IpcScanProcs();

            return WD_STATUS_SUCCESS;
        }

        private static WD_ERROR_CODES MenuIpcUnicastCb(object ctx)
        {
            WDS_DIAG_IpcSend(IPC_MESSAGE_SEND_OPTION.MENU_IPC_SEND_UID_UNICAST);

            return WD_STATUS_SUCCESS;
        }

        private static WD_ERROR_CODES MenuIpcMulticastSubGroupCb(object ctx)
        {
            WDS_DIAG_IpcSend(IPC_MESSAGE_SEND_OPTION.MENU_IPC_SEND_SUBGROUP_MULTICAST);

            return WD_STATUS_SUCCESS;
        }

        private static WD_ERROR_CODES MenuIpcMulticastGroupCb(object ctx)
        {
            WDS_DIAG_IpcSend(IPC_MESSAGE_SEND_OPTION.MENU_IPC_SEND_MULTICAST);

            return WD_STATUS_SUCCESS;
        }

        private static WD_ERROR_CODES MenuIpcSharedIntEnableCb(object ctx)
        {
            WDS_DIAG_IpcSharedIntsEnable();

            return WD_STATUS_SUCCESS;
        }

        private static WD_ERROR_CODES MenuIpcSharedIntDisableLocalCb(object ctx)
        {
            Console.WriteLine("");
            if (WDS_SharedIntDisableLocal() == (DWORD)WD_STATUS_SUCCESS)
                Console.WriteLine("Shared ints successfully disabled locally");
            else
                Console.WriteLine("Shared ints already disabled locally");

            return WD_STATUS_SUCCESS;
        }

        private static WD_ERROR_CODES MenuIpcSharedIntDisableGlobalCb(object ctx)
        {
            Console.WriteLine("");
            /* After global disable, shared interrupts must be disabled
               locally */
            if (WDS_SharedIntDisableGlobal() == (DWORD)WD_STATUS_SUCCESS)
                Console.WriteLine("Shared ints successfully disabled globally");
            else
                Console.WriteLine("Shared ints already disabled globally");

            return MenuIpcSharedIntDisableLocalCb(ctx);
        }

        private static WD_ERROR_CODES MenuIpcSharedBufferAllocCb(object ctx)
        {
            WDS_DIAG_IpcKerBufAllocAndShare();

            return WD_STATUS_SUCCESS;
        }

        private static WD_ERROR_CODES MenuIpcSharedBufferFreeCb(object ctx)
        {
            WDS_DIAG_IpcKerBufRelease();

            return WD_STATUS_SUCCESS;
        }

        static WD_ERROR_CODES MenuIpcCb(object ctx)
        {
            Console.WriteLine("");
            Console.WriteLine("IPC management menu - ");

            if (WDS_IsIpcRegistered())
                Console.WriteLine("Registered with SubGroup ID 0x{0:x}", dwSubGroupId);
            else
                Console.WriteLine("Unregistered");

            Console.WriteLine("--------------");

            return WD_STATUS_SUCCESS;
        }

        public static void MenuIpcInit(DiagMenuOption menuRoot)
        {
            DiagMenuOption menuIpcMenuOption = new()
            {
                OptionName = "Manage IPC",
                CbEntry = MenuIpcCb,
            };

            DiagMenuOption registerProcessesMenu = new()
            {
                OptionName = "Register process",
                CbEntry = MenuIpcRegisterCb,
                CbIsHidden = MenuIpcIsRegistered
            };
            DiagMenuOption unregisterProcessesMenu = new()
            {
                OptionName = "Un-Register process",
                CbEntry = MenuIpcUnRegisterCb,
                CbIsHidden = MenuIpcIsNotRegistered
            };
            DiagMenuOption findMenu = new()
            {
                OptionName = "Find current registered group processes",
                CbEntry = MenuIpcScanCb,
                CbIsHidden = MenuIpcIsNotRegistered
            };
            DiagMenuOption unicastMenu = new()
            {
                OptionName = "Unicast- Send message to a single process by unique ID",
                CbEntry = MenuIpcUnicastCb,
                CbIsHidden = MenuIpcIsNotRegistered
            };
            DiagMenuOption multicastSubGroupMenu = new()
            {
                OptionName = "Multicast - Send message to a subgroup",
                CbEntry = MenuIpcMulticastSubGroupCb,
                CbIsHidden = MenuIpcIsNotRegistered
            };
            DiagMenuOption multicastAllMenu = new()
            {
                OptionName = "Multicast- Send message to all processes in current group",
                CbEntry = MenuIpcMulticastGroupCb,
                CbIsHidden = MenuIpcIsNotRegistered
            };
            DiagMenuOption enableIntsMenu = new()
            {
                OptionName = "Enable Shared Interrupts via IPC",
                CbEntry = MenuIpcSharedIntEnableCb,
                CbIsHidden = MenuIpcIsNotRegistered
            };
            DiagMenuOption disableLocallyIntsMenu = new()
            {
                OptionName = "Locally Disable Shared Interrupts via IPC",
                CbEntry = MenuIpcSharedIntDisableLocalCb,
                CbIsHidden = MenuIpcIsNotRegistered
            };
            DiagMenuOption disableGloballyIntsMenu = new()
            {
                OptionName = "Globally Disable Shared Interrupts via IPC",
                CbEntry = MenuIpcSharedIntDisableGlobalCb,
                CbIsHidden = MenuIpcIsNotRegistered
            };
            DiagMenuOption allocateAndShareBufferMenu = new()
            {
                OptionName = "Allocate and share a kernel buffer with all processes in current group",
                CbEntry = MenuIpcSharedBufferAllocCb,
                CbIsHidden = MenuIpcIsNotRegistered
            };
            DiagMenuOption freeSharedBufferMenu = new()
            {
                OptionName = "Free shared kernel buffer",
                CbEntry = MenuIpcSharedBufferFreeCb,
                CbIsHidden = MenuIpcIsNotRegistered
            };

            IList<DiagMenuOption> menuIpcChildren =
                new List<DiagMenuOption>()
                {
                    registerProcessesMenu,
                    unregisterProcessesMenu,
                    findMenu,
                    unicastMenu,
                    multicastSubGroupMenu,
                    multicastAllMenu,
                    enableIntsMenu,
                    disableLocallyIntsMenu,
                    disableGloballyIntsMenu,
                    allocateAndShareBufferMenu,
                    freeSharedBufferMenu,
                };

            menuIpcMenuOption.AddChildren(menuIpcChildren);
            menuRoot.AddChild(menuIpcMenuOption);
        }

        public static void WDS_DIAG_IpcSendDmaContigToGroup(IntPtr pDma)
        {
            DWORD dwStatus;
            WD_DMA dma;

            if (pDma == IntPtr.Zero)
            {
                throw new WinDriverException(WD_INVALID_PARAMETER,
                    "send_dma_contig_buf_to_group: Error - DMA ctx is NULL");
            }

            dma = Marshal.PtrToStructure<WD_DMA>(pDma);

            if ((dma.dwOptions & (DWORD)WD_DMA_OPTIONS.DMA_KERNEL_BUFFER_ALLOC) == 0)
            {

                throw new WinDriverException(WD_INVALID_PARAMETER,
                    "send_dma_contig_buf_to_group: Error - Sharing SG DMA is not supported");
            }

            dwStatus = WDS_IpcMulticast(
                (DWORD)IPC_MSG_TYPE.IPC_MSG_CONTIG_DMA_BUFFER_READY, dma.hDma);
            if ((DWORD)WD_STATUS_SUCCESS != dwStatus)
            {
                throw new WinDriverException(dwStatus,
                    "send_dma_contig_buf_to_group: Failed sending message.");
            }

            Console.WriteLine("DMA contiguous buffer handle sent successfully");
        }
    }
}





