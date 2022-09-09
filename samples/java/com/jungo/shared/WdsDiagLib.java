package com.jungo.shared;

import com.jungo.*;
import com.jungo.wdapi.*;

/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/******************************************************************************
*  File: wds_diag_lib.c - Implementation of shared WDS all devices' user-mode *
*        diagnostics API.                                                     *
*                                                                             *
*  Note: This code sample is provided AS-IS and as a guiding sample only.     *
*******************************************************************************/

public class WdsDiagLib {

    /*************************************************************
      General definitions
     *************************************************************/
    final static String DEFAULT_PROCESS_NAME =
        "WinDriver Java Diagnostic program";

    /* Unique identifier of the processes group to avoid getting messages from
     * processes made under WinDriver by other developers that use the same
     * driver name.
     * WinDriver developers are encouraged to change their driver name before
     * distribution to avoid this issue entirely. */
    final static int DEFAULT_PROCESS_GROUP_ID = 0x12345678;

    /* Identifiers for shared interrupt IPC process */
    final static String DEFAULT_SHARED_INT_NAME =
        "WinDriver IPC Shared Interrupt";

    /* You can pass any kind of Java Object to IPC and Shared Interrupt context
     * This sample uses a simple String object. */
    final static String SAMPLE_BUFFER_DATA = "This is a sample buffer data";

    /*************************************************************
      General definitions
     *************************************************************/

    /* Example IPC messages IDs */
    final static int
        IPC_MSG_KERNEL_BUFFER_READY = 1,
                                    /* Kernel buffer (Created with
                                     * WDS_SharedBufferAlloc()) ready to be
                                     * shared between processes. Kernel Buffer
                                     * handle is passed in the qwMsgData */

        IPC_MSG_CONTIG_DMA_BUFFER_READY = 2;
                                         /* Kernel buffer (Created with
                                          * WDC_DMAContigBufLock()) ready to be
                                          * shared between processes. DMA Buffer
                                          * handle is passed in the qwMsgData */

        /* TODO: Modify/Add values to communicate between processes */
/* -------------------------------------------------------------------------
   IPC
   ------------------------------------------------------------------------- */
/* IPC API functions are not part of the standard WinDriver API, and not
 * included in the standard version of WinDriver. The functions are part of
 * "WinDriver for Server" API and require "WinDriver for Server" license.
 * Note that "WinDriver for Server" APIs are included in WinDriver
 * evaluation version. */

    /*************************************************************
      Global variables
     *************************************************************/

    /* User's shared kernel buffer handle */
    static WD_KERNEL_BUFFER pSharedKerBuf = null;
                                                /* Static global pointer is used
                                                 * only for sample simplicity */

    /* -----------------------------------------------
        Shared Buffer
       ----------------------------------------------- */
    static class MenuCtxSharedBuffer{
        WD_KERNEL_BUFFER pKerBuf = null;
    }

    /* Shared Buffer menu options */
    enum MENU_SB{
        ALLOC_CONTIG,
        ALLOC_NON_CONTIG,
        FREE,
    };

    static void WDS_DIAG_SharedBufferFree(WD_KERNEL_BUFFER kerBuf)
    {
        long dwStatus;

        if (kerBuf == null)
            return;

        dwStatus = wdapi.WDS_SharedBufferFree(kerBuf);
        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            System.err.printf("WDS_DIAG_SharedBufferFree: Failed freeing shared"
            + "buffer memory. Error [0x%x - %s]\n", dwStatus,
            wdapi.Stat2Str(dwStatus));
        }

        System.out.printf("Shared buffer memory freed\n");
    }

    static long AllocKernelBuff(MenuCtxSharedBuffer pSharedBuffCtx,
        long dwOptions)
    {
        long size;
        WDCResult result = DiagLib.DIAG_InputNum("Enter memory allocation size "
                + "in bytes (32 bit uint)", false, 1,  0xFFFFFFFF);
        if (DiagLib.DIAG_INPUT_SUCCESS != result.dwStatus)
            return result.dwStatus;

        size = ((WDCResultLong)result).result;

        /* Free shared buffer memory before trying the new allocation */
        WDS_DIAG_SharedBufferFree(pSharedBuffCtx.pKerBuf);
        pSharedBuffCtx.pKerBuf = null;

        result = wdapi.WDS_SharedBufferAlloc(size, dwOptions);
        if (wdapi.WD_STATUS_SUCCESS == result.dwStatus)
        {
            pSharedBuffCtx.pKerBuf = ((WDCResultKerBuf)result).kerBuf;
            System.err.printf("%d", pSharedBuffCtx.pKerBuf.hKerBuf);
            System.out.printf("Shared buffer allocated. User addr "
                + "[0x%x], kernel addr [0x%x], size [%d(0x%x)]\n",
                pSharedBuffCtx.pKerBuf.pUserAddr,
                pSharedBuffCtx.pKerBuf.pKernelAddr, size, size);
        }
        else
        {
            System.err.printf("MenuSharedBuffer: Failed allocating " +
                "shared buffer memory. size [%d], Error [0x%x - %s]\n",
                size, result.dwStatus, wdapi.Stat2Str(result.dwStatus));
        }

        return result.dwStatus;
    }

    static void MenuSharedBufferSubMenusInit(
        DiagLib.DiagMenuOption pParentMenu, MenuCtxSharedBuffer pSharedBuffCtx)
    {
        DiagLib.DiagMenuOption[] options = {
            new DiagLib.DiagMenuOptionBuilder()
            .cOptionName("Allocate contiguous shared buffer")
            .cbEntry((pCbCtx)->AllocKernelBuff((MenuCtxSharedBuffer)pCbCtx,
                wdapi.KER_BUF_ALLOC_CONTIG))
            .build(),

            new DiagLib.DiagMenuOptionBuilder()
            .cOptionName("Allocate non-contiguous shared buffer")
            .cbEntry((pCbCtx)->AllocKernelBuff((MenuCtxSharedBuffer)pCbCtx,
                wdapi.KER_BUF_ALLOC_NON_CONTIG))
            .build(),

            new DiagLib.DiagMenuOptionBuilder()
            .cOptionName("Free shared buffer")
            .cbEntry((pCbCtx)->{
                WDS_DIAG_SharedBufferFree(
                    ((MenuCtxSharedBuffer)pCbCtx).pKerBuf);
                ((MenuCtxSharedBuffer)pCbCtx).pKerBuf = null;
                return wdapi.WD_STATUS_SUCCESS;
            })
            .build()
        };

        DiagLib.DIAG_MenuSetCtxAndParentForMenus(options, pSharedBuffCtx,
            pParentMenu);
    }

    public static DiagLib.DiagMenuOption MenuSharedBufferInit(
        DiagLib.DiagMenuOption pParentMenu)
    {
        MenuCtxSharedBuffer sharedBufferCtx = new MenuCtxSharedBuffer();
        DiagLib.DiagMenuOption menuSharedBufferRoot =
            new DiagLib.DiagMenuOptionBuilder()
            .cOptionName("Allocate/free Shared Buffer")
            .cTitleName("Shared Buffer Operations\n" +
            "    E.g. for communicating with Kernel-Plugin")
            .build();

        pParentMenu.DIAG_AddMenuOptionChild(menuSharedBufferRoot);

        MenuSharedBufferSubMenusInit(menuSharedBufferRoot, sharedBufferCtx);
        return menuSharedBufferRoot;
    }

    /* -----------------------------------------------
        IPC - Inter process Communication
       ----------------------------------------------- */

    static class MenuCtxIpc{
        long dwSubGroupID = 0;
    }

    static long MenuIpcSharedIntsDisableLocalOptionCb(Object pCbCtx)
    {
        if (wdapi.WDS_SharedIntDisableLocal() == wdapi.WD_STATUS_SUCCESS)
            System.out.printf("\nShared ints successfully disabled locally\n");
        else
            System.out.printf("\nShared ints already disabled locally\n");

        return wdapi.WD_STATUS_SUCCESS;
    }

    static void MenuIpcSubMenusInit(
        DiagLib.DiagMenuOption pParentMenu, MenuCtxIpc pIpcCtx)
    {
        DiagLib.DiagMenuOption[] options = {
            new DiagLib.DiagMenuOptionBuilder()
            .cOptionName("Register processes")
            .cbEntry((pCbCtx)->{
                ((MenuCtxIpc)pCbCtx).dwSubGroupID = WDS_DIAG_IpcRegister();
                return wdapi.WD_STATUS_SUCCESS;
            })
            .cbIsHidden((pMenu)->{
                return wdapi.WDS_IsIpcRegistered();})
            .build(),

            new DiagLib.DiagMenuOptionBuilder()
            .cOptionName("Un-Register process")
            .cbEntry((pCbCtx)->{
                wdapi.WDS_IpcUnRegister();
                System.out.printf("Process unregistered successfully\n");
                return wdapi.WD_STATUS_SUCCESS;
            })
            .cbIsHidden((pMenu)->{
                return !wdapi.WDS_IsIpcRegistered();})
            .build(),

            new DiagLib.DiagMenuOptionBuilder()
            .cOptionName("Find current registered group processes")
            .cbEntry((pCbCtx)->{
                WDS_DIAG_IpcScanProcs();
                return wdapi.WD_STATUS_SUCCESS;
            })
            .cbIsHidden((pMenu)->{
                return !wdapi.WDS_IsIpcRegistered();})
            .build(),

            new DiagLib.DiagMenuOptionBuilder()
            .cOptionName("Unicast- Send message to a single process by " +
                "unique ID")
            .cbEntry((pCbCtx)->{
                WDS_DIAG_IpcSend(MENU_IPC.SEND_UID_UNICAST.ordinal());
                return wdapi.WD_STATUS_SUCCESS;
            })
            .cbIsHidden((pMenu)->{
                return !wdapi.WDS_IsIpcRegistered();})
            .build(),

            new DiagLib.DiagMenuOptionBuilder()
            .cOptionName("Multicast - Send message to a subGroup")
            .cbEntry((pCbCtx)->{
                WDS_DIAG_IpcSend(MENU_IPC.SEND_SUBGROUP_MULTICAST.ordinal());
                return wdapi.WD_STATUS_SUCCESS;
            })
            .cbIsHidden((pMenu)->{
                return !wdapi.WDS_IsIpcRegistered();})
            .build(),

            new DiagLib.DiagMenuOptionBuilder()
            .cOptionName("Multicast- Send message to all processes in " +
                "current subgroup")
            .cbEntry((pCbCtx)->{
                WDS_DIAG_IpcSend(MENU_IPC.SEND_MULTICAST.ordinal());
                return wdapi.WD_STATUS_SUCCESS;
            })
            .cbIsHidden((pMenu)->{
                return !wdapi.WDS_IsIpcRegistered();})
            .build(),

            new DiagLib.DiagMenuOptionBuilder()
            .cOptionName("Enable Shared Interrupts via IPC")
            .cbEntry((pCbCtx)->{
                WDS_DIAG_IpcSharedIntsEnable();
                return wdapi.WD_STATUS_SUCCESS;
            })
            .cbIsHidden((pMenu)->{
                return !wdapi.WDS_IsIpcRegistered();})
            .build(),

            new DiagLib.DiagMenuOptionBuilder()
            .cOptionName("Locally Disable Shared Interrupts via IPC")
            .cbEntry((pCbCtx)->MenuIpcSharedIntsDisableLocalOptionCb(pCbCtx))
            .cbIsHidden((pMenu)->{
                return !wdapi.WDS_IsIpcRegistered();})
            .build(),

            new DiagLib.DiagMenuOptionBuilder()
            .cOptionName("Globally Disable Shared Interrupts via IPC")
            .cbEntry((pCbCtx)->{
                /* After global disable, shared interrupts must be disabled
                 * locally */
                if (wdapi.WDS_SharedIntDisableGlobal() == wdapi.WD_STATUS_SUCCESS)
                    System.out.printf("\nShared ints successfully disabled globally\n");
                else
                    System.out.printf("\nShared ints already disabled globally\n");

                return MenuIpcSharedIntsDisableLocalOptionCb(pCbCtx);
            })
            .cbIsHidden((pMenu)->{
                return !wdapi.WDS_IsIpcRegistered();})
            .build(),

            new DiagLib.DiagMenuOptionBuilder()
            .cOptionName("Allocate and share a kernel buffer with all " +
                "processes in current group")
            .cbEntry((pCbCtx)->{
                WDS_DIAG_IpcKerBufAllocAndShare();
                return wdapi.WD_STATUS_SUCCESS;
            })
            .cbIsHidden((pMenu)->{
                return !wdapi.WDS_IsIpcRegistered();})
            .build(),

            new DiagLib.DiagMenuOptionBuilder()
            .cOptionName("Free shared kernel buffer")
            .cbEntry((pCbCtx)->{
                WDS_DIAG_IpcKerBufRelease();
                return wdapi.WD_STATUS_SUCCESS;
            })
            .cbIsHidden((pMenu)->{
                return !wdapi.WDS_IsIpcRegistered();})
            .build(),
        };

        DiagLib.DIAG_MenuSetCtxAndParentForMenus(options, pIpcCtx,
                pParentMenu);
    }

    static long MenuIpcCb(MenuCtxIpc pIpcMenuCtx)
    {
        boolean isIpcRegistered = wdapi.WDS_IsIpcRegistered();

        System.out.printf("\n");
        System.out.printf("IPC management menu - ");
        if (isIpcRegistered)
        {
            System.out.printf("Registered with SubGroup ID 0x%x\n",
                pIpcMenuCtx.dwSubGroupID);
        }
        else
        {
            System.out.printf("Unregistered\n");
        }
        System.out.printf("--------------\n");

        return wdapi.WD_STATUS_SUCCESS;
    }

    public static DiagLib.DiagMenuOption MenuIpcInit(
            DiagLib.DiagMenuOption pParentMenu)
    {
        MenuCtxIpc ipcCtx = new MenuCtxIpc();
        DiagLib.DiagMenuOption ipcMenuRoot =
            new DiagLib.DiagMenuOptionBuilder()
            .cOptionName("Manage IPC")
            .cbEntry((pCbCtx)->MenuIpcCb((MenuCtxIpc)pCbCtx))
            .build();

        ipcMenuRoot.pCbCtx = ipcCtx;
        pParentMenu.DIAG_AddMenuOptionChild(ipcMenuRoot);

        MenuIpcSubMenusInit(ipcMenuRoot, ipcCtx);
        return ipcMenuRoot;
    }

    static void ipc_msg_event_cb(WDS_IPC_MSG_RX ipcRxMsg, Object pData)
    {
        System.out.printf("\n\nReceived an IPC message:\n" +
            "msgID [0x%x], msgData [0x%x] from process [0x%x]\n",
            ipcRxMsg.dwMsgID, ipcRxMsg.qwMsgData, ipcRxMsg.dwSenderUID);
        System.out.printf("IPC pData: %s\n", pData.toString());

        /* Important: Acquiring and using any resource (E.g. kernel/DMA buffer)
         * should be done from a deferred procedure to avoid jamming the IPC
         * incoming messages.
         * Notice you can pass private context at WDS_IpcRegister() and use it
         * here (pData) for signalling local thread for example.
         *
         * The following implementation is for sample purposes only! */

        switch ((int)ipcRxMsg.dwMsgID)
        {
        case IPC_MSG_KERNEL_BUFFER_READY:
        {
            WD_KERNEL_BUFFER pKerBuf = null;
            long sample_buffer_len;
            WDCResultKerBuf result;

            System.out.printf("\nThis is a shared kernel buffer, getting "
                + "it...\n");

            result = wdapi.WDS_SharedBufferGet(ipcRxMsg.qwMsgData);
            if (wdapi.WD_STATUS_SUCCESS != result.dwStatus)
            {
                System.err.printf("ipc_msg_event_cb: Failed getting shared"
                        + " kernel buffer. Error [0x%x - %s]\n", result.dwStatus,
                        wdapi.Stat2Str(result.dwStatus));
                return;
            }
            pKerBuf = result.kerBuf;

            System.out.printf("Got a shared kernel buffer. UserAddr [0x%x],"
                    + " KernelAddr [0x%x], size [%d]\n", pKerBuf.pUserAddr,
                    pKerBuf.pKernelAddr, pKerBuf.qwBytes);

            /* Here we read SAMPLE_BUFFER_DATA from the received buffer */
            sample_buffer_len = (long)SAMPLE_BUFFER_DATA.length();
            if (pKerBuf.qwBytes > sample_buffer_len + 1)
            {
                // order the buffer correctly for reading
                pKerBuf.nativeData.order(java.nio.ByteOrder.nativeOrder());

                // read buffer contents
                byte[] bytes = new byte[pKerBuf.nativeData.remaining()];
                pKerBuf.nativeData.get(bytes);

                System.out.printf("Sample data from kernel buffer [%s]\n",
                        new String(bytes));
            }
            else
            {
                System.out.printf("Kernel buffer was too short for sample" +
                        " data\n");
            }

            /* For sample purpose we immediately release the buffer */
            wdapi.WDS_SharedBufferFree(pKerBuf);
        }
        break;

        case IPC_MSG_CONTIG_DMA_BUFFER_READY:
        {
            long dwStatus;
            WD_DMA pDma = null;
            WDCResultDMA result;

            System.out.printf("\nThis is a DMA buffer, getting it...\n");

            result = wdapi.WDC_DMABufGet((long)ipcRxMsg.qwMsgData);
            dwStatus = result.dwStatus;
            if (wdapi.WD_STATUS_SUCCESS != dwStatus)
            {
                System.err.printf("ipc_msg_event_cb: Failed getting DMA buffer."
                 + " Error [0x%x - %s]\n", dwStatus, wdapi.Stat2Str(dwStatus));
                return;
            }
            pDma = result.pDma;

            System.out.printf("Got a DMA buffer. UserAddr [%x], " +
                "pPhysicalAddr [0x%x], size [%d(0x%x)]\n",
                pDma.pUserAddr, pDma.Page[0].pPhysicalAddr,
                pDma.Page[0].dwBytes, pDma.Page[0].dwBytes);

            /* For sample purpose we immediately release the buffer */
            wdapi.WDC_DMABufUnlock(pDma);
        }
        break;

        default:
            System.out.printf("Unknown IPC type. msgID [0x%x], msgData [0x%x]"
                + " from process [0x%x]\n\n", ipcRxMsg.dwMsgID,
                ipcRxMsg.qwMsgData, ipcRxMsg.dwSenderUID);
        }
    }

    static void ipc_shared_int_msg_event_cb(WDS_IPC_MSG_RX ipcRxMsg,
        Object pData)
    {
        System.out.printf("Shared Interrupt via IPC arrived:\nmsgID [0x%x], " +
            "msgData [0x%x] from process [0x%x]\n\n", ipcRxMsg.dwMsgID,
            ipcRxMsg.qwMsgData, ipcRxMsg.dwSenderUID);
        System.out.printf("Message pData:" + (String)pData);
    }

    /* Register process to IPC service */
    static long WDS_DIAG_IpcRegister()
    {
        long dwSubGroupID = 0;
        long dwStatus;
        WDCResultLong result;

        result = DiagLib.DIAG_InputNum("Enter process SubGroup ID (hex)", true,
            0, 0xFFFFFFFF);
        if (DiagLib.DIAG_INPUT_SUCCESS != result.dwStatus)
            return 0;

        dwSubGroupID = result.result;

        dwStatus = wdapi.WDS_IpcRegister(DEFAULT_PROCESS_NAME,
            DEFAULT_PROCESS_GROUP_ID, dwSubGroupID, wdapi.WD_IPC_ALL_MSG,
            new IPC_MSG_RX_HANDLER()
            {
                @Override
                public void run(WDS_IPC_MSG_RX ipcRxMsg, Object pData)
                {
                    ipc_msg_event_cb(ipcRxMsg, pData);
                }
        }, SAMPLE_BUFFER_DATA);
        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            System.err.printf("WDS_DIAG_IpcRegister: Failed registering" +
                " process to IPC. Error [0x%x - %s]\n", dwStatus,
                wdapi.Stat2Str(dwStatus));
            return 0;
        }

        System.out.printf("Registration completed successfully\n");
        return dwSubGroupID;
    }

    /* Enable Shared Interrupts via IPC */
    static long WDS_DIAG_IpcSharedIntsEnable()
    {
        long dwSubGroupID = 0;
        long dwStatus;
        WDCResultLong result;

        if (wdapi.WDS_IsSharedIntsEnabledLocally())
        {
            System.err.printf("WDS_DIAG_IpcSharedIntsEnable: Shared interrupts" +
                " already enabled locally.\n");
            return wdapi.WD_OPERATION_ALREADY_DONE;
        }

        result = DiagLib.DIAG_InputNum("Enter shared interrupt's SubGroup ID " +
            "(hex)", true, 0, 0xFFFFFFFF);
        if (DiagLib.DIAG_INPUT_SUCCESS != result.dwStatus)
            return 0;

        dwSubGroupID = result.result;

        /* WDS_SharedIntEnable() is called in this sample with pFunc=
        ipc_shared_int_msg_event_cb. This will cause a shared interrupt to invoke
        both this callback and the callback passed to WDS_IpcRegister() in
        WDS_DIAG_IpcRegister(). To disable the "general" IPC callback, pass
        pFunc=null in the above mentioned call.
        Note you can replace pFunc here with your own callback especially designed
        to handle interrupts */
        dwStatus = wdapi.WDS_SharedIntEnable(DEFAULT_SHARED_INT_NAME,
            DEFAULT_PROCESS_GROUP_ID, dwSubGroupID, wdapi.WD_IPC_ALL_MSG,
            new IPC_MSG_RX_HANDLER() {
            @Override
                public void run(WDS_IPC_MSG_RX ipcRxMsg, Object pData)
                {
                    ipc_shared_int_msg_event_cb(ipcRxMsg, pData);
                }
              }, SAMPLE_BUFFER_DATA);
        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            System.err.printf("WDS_DIAG_IpcSharedIntsEnable: Failed enabling"
                + " shared interrupts via IPC. Error [0x%x - %s]\n", dwStatus,
                wdapi.Stat2Str(dwStatus));
            return 0;
        }

        System.out.printf("Shared interrupts via IPC enabled successfully\n");
        return dwSubGroupID;
    }

    static void WDS_DIAG_IpcScanProcs()
    {
        WDCResultIpcScan ipcScanResult;
        int i;

        ipcScanResult = wdapi.WDS_IpcScanProcs();
        if (wdapi.WD_STATUS_SUCCESS != ipcScanResult.dwStatus)
        {
            System.err.printf("WDS_DIAG_IpcScanProcs: Failed scanning "
                + "registered processes. Error [0x%x - %s]\n",
                ipcScanResult.dwStatus,
                wdapi.Stat2Str(ipcScanResult.dwStatus));
            return;
        }

        if (ipcScanResult.procInfo.length != 0)
        {
            System.out.printf("Found %d processes in current group\n",
                ipcScanResult.procInfo.length);
            for (i = 0; i < ipcScanResult.procInfo.length; i++)
            {
                System.out.printf("  %d) Name: %s, SubGroup ID: 0x%x, UID: " +
                    " 0x%x\n", i + 1, ipcScanResult.procInfo[i].cProcessName,
                    ipcScanResult.procInfo[i].dwSubGroupID,
                    ipcScanResult.procInfo[i].hIpc);
            }
        }
        else
        {
            System.out.printf("No processes found in current group\n");
        }
    }

    static void WDS_DIAG_IpcKerBufRelease()
    {
        long dwStatus;

        if (pSharedKerBuf == null)
            return;

        /* Notice that once a buffer that was acquired by a different process is
         * freed, its kernel resources are kept as long as the other processes
		 * did not release the buffer. */
        dwStatus = wdapi.WDS_SharedBufferFree(pSharedKerBuf);
        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            System.err.printf("WDS_DIAG_IpcKerBufRelease: Failed freeing shared"
			+ " buffer. Error [0x%x - %s]\n", dwStatus,
			wdapi.Stat2Str(dwStatus));
        }

        pSharedKerBuf = null;

        System.out.printf("Kernel buffer freed\n");
    }

    static void WDS_DIAG_IpcKerBufAllocAndShare()
    {
        long size = 0;
        long dwStatus;
        long dwOptions = wdapi.KER_BUF_ALLOC_CONTIG;
        long sample_buffer_len;
        WDCResult result;

        /* If kernel buffer was allocated in the past, release it */
        WDS_DIAG_IpcKerBufRelease();

        result = DiagLib.DIAG_InputNum("Enter new kernel buffer size to " +
            "allocate and share with\n current group", true, 1, 0xFFFFFFFF);
        if (DiagLib.DIAG_INPUT_SUCCESS != result.dwStatus)
            return;

        size = ((WDCResultLong)result).result;

        result = wdapi.WDS_SharedBufferAlloc(size, dwOptions);
        dwStatus = result.dwStatus;
        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            System.err.printf("WDS_DIAG_IpcKerBufAllocAndShare: Failed " +
				"allocating shared kernel buffer. size [%d], Error [0x%x - %s]"
				+ "\n", size, dwStatus, wdapi.Stat2Str(dwStatus));
            return;
        }
        pSharedKerBuf = ((WDCResultKerBuf)result).kerBuf;

        System.out.printf("Successful kernel buffer allocation. UserAddr [0x%x]"
			+ ", KernelAddr [0x%x], size [%d]\n", pSharedKerBuf.pUserAddr,
            pSharedKerBuf.pKernelAddr, size);

        /* Here we write SAMPLE_BUFFER_DATA to the new allocated buffer */
        sample_buffer_len = SAMPLE_BUFFER_DATA.length();
        if (size > sample_buffer_len + 1)
        {
            pSharedKerBuf.nativeData.put(SAMPLE_BUFFER_DATA.getBytes());
            System.out.printf("Sample data written to kernel buffer\n");
        }
        else
        {
            System.out.printf("Kernel buffer is too short for sample data\n");
        }

        dwStatus = wdapi.WDS_IpcMulticast(IPC_MSG_KERNEL_BUFFER_READY,
            wdapi.WDS_SharedBufferGetGlobalHandle(pSharedKerBuf));
        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            System.err.printf("WDS_DIAG_IpcAllocAndShareBuf: Failed sending " +
				"message. Error [0x%x - %s]\n", dwStatus,
				wdapi.Stat2Str(dwStatus));
            return;
        }

        System.out.printf("Kernel buffer shared successfully\n");
    }

    /* IPC menu options */
    enum MENU_IPC
    {
        REGISTER,
        UNREGISTER,
        GET_GROUP_IDS,
        SEND_UID_UNICAST,
        SEND_SUBGROUP_MULTICAST,
        SEND_MULTICAST,
        ENABLE_SHARED_INTS,
        LOCAL_DISABLE_SHARED_INTS,
        GLOBAL_DISABLE_SHARED_INTS,
        KER_BUF_ALLOC_AND_SHARE,
        KER_BUF_RELEASE,
    };

    static void WDS_DIAG_IpcSend(int ipc_menu_option)
    {
        long recipientID = 0;
        long messageID = 0;
        long messageData = 0;
        long dwStatus = 0;
        WDCResultLong result;
        final MENU_IPC[] vals = MENU_IPC.values();

        if (ipc_menu_option == MENU_IPC.SEND_UID_UNICAST.ordinal() + 1 ||
            ipc_menu_option == MENU_IPC.SEND_SUBGROUP_MULTICAST.ordinal() + 1)
        {
            String sInputText = String.format("Enter recipient%sID (hex)",
                ipc_menu_option == MENU_IPC.SEND_SUBGROUP_MULTICAST.ordinal()
                + 1 ? "(s) SubGroup " : " U");
            result = DiagLib.DIAG_InputNum(sInputText, true, 0, 0xFFFFFFFF);
            if (result.dwStatus != DiagLib.DIAG_INPUT_SUCCESS)
                return;

            recipientID = result.result;
        }

        result = DiagLib.DIAG_InputNum("Enter your message ID (32Bit hex)",
            true, 0, 0xFFFFFFFF);
        if (result.dwStatus != DiagLib.DIAG_INPUT_SUCCESS)
            return;

        messageID = result.result;

        result = DiagLib.DIAG_InputNum("Enter your message (64Bit hex)",
              true, 0, 0xFFFFFFFF);
        if (result.dwStatus != DiagLib.DIAG_INPUT_SUCCESS)
            return;

        messageID = result.result;

        switch (vals[ipc_menu_option - 1])
        {
        case SEND_UID_UNICAST:
            dwStatus = wdapi.WDS_IpcUidUnicast(recipientID, messageID,
                messageData);
            break;

        case SEND_SUBGROUP_MULTICAST:
            dwStatus = wdapi.WDS_IpcSubGroupMulticast(recipientID, messageID,
                messageData);
            break;

        case SEND_MULTICAST:
            dwStatus = wdapi.WDS_IpcMulticast(messageID, messageData);
            break;

        default:
            break;
        }

        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            System.err.printf("WDS_DIAG_IpcSend: Failed sending message. " +
                "Error [0x%x - %s]\n", dwStatus, wdapi.Stat2Str(dwStatus));
            return;
        }

        System.out.printf("Message sent successfully\n");
    }

    public static void MenuIpc()
    {
        int option;
        long dwSubGroupID = 0;
        final MENU_IPC[] vals = MENU_IPC.values();

        do
        {
            boolean isIpcRegistered = wdapi.WDS_IsIpcRegistered();
            WDCResultInteger result;

            System.out.printf("\n");
            System.out.printf("IPC management menu - ");
            if (isIpcRegistered)
            {
                System.out.printf("Registered with SubGroup ID 0x%x\n",
                   dwSubGroupID);
            }
            else
            {
                System.out.printf("Unregistered\n");
            }
            System.out.printf("--------------\n");

            if (!isIpcRegistered)
            {
                System.out.printf("%d. Register processes\n",
                    MENU_IPC.REGISTER.ordinal() + 1);
            }
            else
            {
                System.out.printf("%d. Un-Register process\n",
                    MENU_IPC.UNREGISTER.ordinal() + 1);
                System.out.printf("%d. Find current registered group processes\n",
                    MENU_IPC.GET_GROUP_IDS.ordinal() + 1);
                System.out.printf("%d. Unicast- Send message to a single " +
                    "process by unique ID\n",
                    MENU_IPC.SEND_UID_UNICAST.ordinal() + 1);
                System.out.printf("%d. Multicast - Send message to a subGroup\n",
                    MENU_IPC.SEND_SUBGROUP_MULTICAST.ordinal() + 1);
                System.out.printf("%d. Multicast- Send message to all processes"
                    + " in current group\n",
                    MENU_IPC.SEND_MULTICAST.ordinal() + 1);
                System.out.printf("%d. Enable Shared Interrupts via IPC \n",
                    MENU_IPC.ENABLE_SHARED_INTS.ordinal() + 1);
                System.out.printf("%d. Locally Disable Shared Interrupts via"
                    + " IPC \n",
                    MENU_IPC.LOCAL_DISABLE_SHARED_INTS.ordinal() + 1);
                System.out.printf("%d. Globally Disable Shared Interrupts via" +
                    " IPC \n",
                    MENU_IPC.GLOBAL_DISABLE_SHARED_INTS.ordinal() + 1);
                System.out.printf("%d. Allocate and share a kernel buffer with"
                    + " all processes in current group\n",
                    MENU_IPC.KER_BUF_ALLOC_AND_SHARE.ordinal() + 1);
                System.out.printf("%d. Free shared kernel buffer\n",
                    MENU_IPC.KER_BUF_RELEASE.ordinal() + 1);
            }
            System.out.printf("%d. Exit\n", DiagLib.DIAG_EXIT_MENU);

            result = DiagLib.DIAG_GetMenuOption(isIpcRegistered ?
                MENU_IPC.KER_BUF_RELEASE.ordinal() + 1 :
                MENU_IPC.REGISTER.ordinal() + 1);

            if (DiagLib.DIAG_INPUT_FAIL == result.dwStatus)
                continue;

            option = (int)result.result;

            if (option == DiagLib.DIAG_EXIT_MENU)
                return;

            switch (vals[option - 1])
            {
            case REGISTER:
                if (isIpcRegistered)
                    System.out.printf("Process already registered\n");
                else
                    dwSubGroupID = WDS_DIAG_IpcRegister();
                break;

            case UNREGISTER:
                if (isIpcRegistered)
                    wdapi.WDS_IpcUnRegister();
                System.out.printf("Process unregistered successfully\n");
                break;

            case GET_GROUP_IDS:
                WDS_DIAG_IpcScanProcs();
                break;

            case SEND_UID_UNICAST:
            case SEND_SUBGROUP_MULTICAST:
            case SEND_MULTICAST:
                WDS_DIAG_IpcSend(option);
                break;

            case ENABLE_SHARED_INTS:
                WDS_DIAG_IpcSharedIntsEnable();
                break;
            case GLOBAL_DISABLE_SHARED_INTS:
                /* After global disable, shared interrupts must be disabled
                 * locally */
                if (wdapi.WDS_SharedIntDisableGlobal() == wdapi.WD_STATUS_SUCCESS)
                    System.out.printf("\nShared ints successfully disabled globally\n");
                else
                    System.out.printf("\nShared ints already disabled globally\n");
            case LOCAL_DISABLE_SHARED_INTS:
                if (wdapi.WDS_SharedIntDisableLocal() == wdapi.WD_STATUS_SUCCESS)
                    System.out.printf("\nShared ints successfully disabled locally\n");
                else
                    System.out.printf("\nShared ints already disabled locally\n");
                break;
            case KER_BUF_ALLOC_AND_SHARE:
                WDS_DIAG_IpcKerBufAllocAndShare();
                break;

            case KER_BUF_RELEASE:
                WDS_DIAG_IpcKerBufRelease();
                break;
            }
        } while (true);
    }

    public static long WDS_DIAG_IpcSendDmaContigToGroup(wdapi.WD_DMA pDma)
    {
        long dwStatus;

        if (pDma == null)
        {
            System.err.printf("send_dma_contig_buf_to_group: Error - DMA ctx "
                + "is null\n");
            return wdapi.WD_INVALID_PARAMETER;
        }

        if ((pDma.dwOptions & wdapi.DMA_KERNEL_BUFFER_ALLOC) == 0)
        {
            System.err.printf("send_dma_contig_buf_to_group: Error - Sharing "
                + " SG DMA is not supported\n");
            return wdapi.WD_INVALID_PARAMETER;
        }

        dwStatus = wdapi.WDS_IpcMulticast(IPC_MSG_CONTIG_DMA_BUFFER_READY,
            wdapi.WDC_DMAGetGlobalHandle(pDma));
        if (wdapi.WD_STATUS_SUCCESS != dwStatus)
        {
            System.err.printf("send_dma_contig_buf_to_group: Failed sending"
                + " message. Error [0x%x - %s]\n", dwStatus,
                wdapi.Stat2Str(dwStatus));
            return dwStatus;
        }

        System.out.printf("DMA contiguous buffer handle sent successfully\n");
        return wdapi.WD_STATUS_SUCCESS;
    }
}


