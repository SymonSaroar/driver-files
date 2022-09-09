/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifdef WDLOG
#undef WDLOG
#endif

#include "windrvr.h"
#include <stdio.h>

#if defined(LINUX)
    #include <stdarg.h>
#endif

FILE *fpWdLog;

#define STR(s) ((s) ? (s) : "(null)")

static int print_ioctl_data(DWORD dwIoctl, PVOID src, DWORD src_bytes);

DWORD DLLCALLCONV WD_LogStart(const char *sFileName, const char *sMode)
{
    if (!sMode)
        sMode = "w";

    fpWdLog = fopen(sFileName, sMode);
    if (!fpWdLog)
        return WD_SYSTEM_INTERNAL_ERROR;

    return WD_STATUS_SUCCESS;
}

VOID DLLCALLCONV WD_LogStop(void)
{
    if (fpWdLog)
        fclose(fpWdLog);
    fpWdLog = NULL;
}

DWORD DLLCALLCONV WdFunctionLog(DWORD dwIoctl, HANDLE h, PVOID pParam,
    DWORD dwSize, BOOL fWait)
{
    DWORD rc;
    /* Don't log debug messages - too messy */
    DWORD skip = (dwIoctl == IOCTL_WD_DEBUG_ADD);

    if (fpWdLog && !skip)
    {
        fprintf(fpWdLog, "\nLogging ioctl %x (%x), handle %p, size %x\n",
            (UINT32)dwIoctl, WD_CTL_DECODE_FUNC((UINT32)dwIoctl), h, dwSize);
        print_ioctl_data(dwIoctl, pParam, dwSize);
    }

    rc = WD_FUNCTION_LOCAL(dwIoctl, h, pParam, dwSize, fWait);

    if (fpWdLog && !skip)
    {
        fprintf(fpWdLog, "ioctl %x (%x) returned status %x\n",
            (UINT32)dwIoctl, WD_CTL_DECODE_FUNC((UINT32)dwIoctl), rc);
        print_ioctl_data(dwIoctl, pParam, dwSize);
    }
    return rc;
}

HANDLE DLLCALLCONV WD_OpenLog(void)
{
    HANDLE hWD = WD_Open();

    if (fpWdLog)
        fprintf(fpWdLog, "WD_Open() returned %p\n", hWD);

    return hWD;
}

void DLLCALLCONV WD_CloseLog(HANDLE hWD)
{
    WD_Close(hWD);

    if (fpWdLog)
        fprintf(fpWdLog, "WD_Close() called for handle %p\n", hWD);
}

VOID DLLCALLCONV WD_LogAdd(const char *sFormat, ...)
{
    va_list ap;

    if (!fpWdLog)
        return;

    va_start(ap, sFormat);
    vfprintf(fpWdLog, sFormat, ap);
    va_end(ap);
}

static VOID WD_LogAddIdented(int ident, const char *sFormat, ...)
{
    va_list ap;

    if (!fpWdLog)
        return;

    va_start(ap, sFormat);
    for (; ident > 0; ident--)
    {
        fprintf(fpWdLog, "  ");
    }
    vfprintf(fpWdLog, sFormat, ap);
    va_end(ap);
    fflush(fpWdLog);
}

#define LOG WD_LogAddIdented

static void log_hexbuf(PVOID src, DWORD src_bytes, int ident)
{
    DWORD i;

    LOG(ident, "0x ");
    for (i = 0; i < src_bytes; i++)
        LOG(0, "%02x ", ((BYTE *)src)[i]);
    LOG(0, "\n");
}

static void log_WD_PCI_ID(WD_PCI_ID *p, int ident)
{
    LOG(ident, "WD_PCI_ID:\n");
    LOG(ident + 1, "dwVendorId=%x, dwDeviceId=%x\n", p->dwVendorId,
        p->dwDeviceId);
}

static void log_WD_PCI_CAP(WD_PCI_CAP *p, int ident)
{
    LOG(ident, "WD_PCI_CAP:\n");
    LOG(ident + 1, "dwCapId [0x%x], dwCapOffset [0x%x]\n", p->dwCapId,
        p->dwCapOffset);
}

static void log_WD_PCI_SLOT(WD_PCI_SLOT *p, int ident)
{
    LOG(ident, "WD_PCI_SLOT:\n");
    LOG(ident + 1, "dwBus=%x, dwSlot=%x, dwFunction=%x\n", p->dwBus,
        p->dwSlot, p->dwFunction);
}

static void log_WDU_MATCH_TABLE(WDU_MATCH_TABLE *p, int ident)
{
    LOG(ident, "WDU_MATCH_TABLE:\n");
    LOG(ident + 1, "wVendorId=%hx, wProductId=%hx, bDeviceClass=%x, "
        "bDeviceSubClass=%x\n",
        p->wVendorId, p->wProductId, (UINT32)p->bDeviceClass,
        (UINT32)p->bDeviceSubClass);
    LOG(ident + 1, "bInterfaceClass=%x, bInterfaceSubClass=%x, "
        "bInterfaceProtocol=%x\n",
        (UINT32)p->bInterfaceClass, (UINT32)p->bInterfaceSubClass,
        (UINT32)p->bInterfaceProtocol);
}

static void log_WD_ITEMS(WD_ITEMS *p, int ident)
{
    LOG(ident, "WD_ITEMS:\n");
    LOG(ident + 1, "item=%x, fNotSharable=%x\n",
        p->item, p->fNotSharable);

    switch(p->item)
    {
    case ITEM_INTERRUPT:
        LOG(ident + 1, "union I <ITEM_INTERRUPT>:\n");
        LOG(ident + 2, "dwInterrupt=%x, dwOptions=%x, hInterrupt=%x\n",
            p->I.Int.dwInterrupt, p->I.Int.dwOptions, p->I.Int.hInterrupt);
        break;

    case ITEM_MEMORY:
        LOG(ident + 1, "union I <ITEM_MEMORY>:\n");
#if defined(KERNEL_64BIT)
        LOG(ident + 2, "pPhysicalAddr=%"PRI64"x, dwBytes=%"PRI64"x, "
            "dwTransAddr=%x:%x, dwUserDirectAddr=%x, dwBar=%x, dwOptions=%x"
            "\n", p->I.Mem.pPhysicalAddr, p->I.Mem.qwBytes,
            /* TODO: use KPRI syntax to print KPTR */
            (UINT32)(p->I.Mem.pTransAddr >> 32), (UINT32)p->I.Mem.pTransAddr,
            p->I.Mem.pUserDirectAddr, p->I.Mem.dwBar, p->I.Mem.dwOptions);
#else
        LOG(ident + 2, "pPhysicalAddr=%"PRI64"x, dwBytes=%"PRI64"x, "
            "dwTransAddr=%x, dwUserDirectAddr=%x, dwBar=%x, dwOptions=%x\n",
            p->I.Mem.pPhysicalAddr, p->I.Mem.qwBytes, p->I.Mem.pTransAddr,
            p->I.Mem.pUserDirectAddr, p->I.Mem.dwBar, p->I.Mem.dwOptions);
#endif
        break;

    case ITEM_IO:
        LOG(ident + 1, "union I <ITEM_IO>:\n");
#if defined(KERNEL_64BIT)
        LOG(ident + 2, "dwAddr=%x:%x, dwBytes=%x, dwBar=%x\n",
            /* TODO: use KPRI syntax to print KPTR */
            (UINT32)(p->I.IO.pAddr >> 32), (UINT32)(p->I.IO.pAddr),
            p->I.IO.dwBytes, p->I.IO.dwBar);
#else
        LOG(ident + 2, "dwAddr=%x, dwBytes=%x, dwBar=%x\n", p->I.IO.pAddr,
            p->I.IO.dwBytes, p->I.IO.dwBar);
#endif
        break;

    case ITEM_BUS:
        LOG(ident + 1, "union I <ITEM_BUS>:\n");
        LOG(ident + 2, "dwBusType=%x, dwBusNum=%x, dwSlotFunc=%x\n",
            p->I.Bus.dwBusType, p->I.Bus.dwBusNum, p->I.Bus.dwSlotFunc);
        break;

    case ITEM_NONE:
    default:
        LOG(ident + 1, "union I <ITEM_NONE>:\n");
        log_hexbuf(&p->I, sizeof(p->I), ident + 2);
        break;
    }
}

static void log_WD_CARD(WD_CARD *p, int ident)
{
    DWORD i;
    LOG(ident, "WD_CARD:\n");
    LOG(ident + 1, "dwItems=%x\n", p->dwItems);
    for (i = 0; i < p->dwItems; i++)
    {
        LOG(ident + 1, "[%x]", i);
        log_WD_ITEMS(&p->Item[i], ident + 1);
    }
}

static void log_WD_IPC_PROCESS(WD_IPC_PROCESS *p, int ident)
{
    LOG(ident, "WD_IPC_PROCESS:\n");
    LOG(ident + 1, "cProcessName=%s\n", STR(p->cProcessName));
    LOG(ident + 1, "dwSubGroupID=0x%x\n", p->dwSubGroupID);
    LOG(ident + 1, "dwGroupID=0x%x\n", p->dwGroupID);
    LOG(ident + 1, "hIpc=%ld\n", p->hIpc);
}

static void log_WD_TRANSFER(WD_TRANSFER *p, int ident)
{
    LOG(ident, "WD_TRANSFER:\n");
#if defined(KERNEL_64BIT)
    /* TODO: use KPRI syntax to print KPTR */
    LOG(ident + 1, "pPort=%x:%x, cmdTrans=%x, dwBytes=%x, fAutoinc=%x, "
        "dwOptions=%x\n",
        (UINT32)(p->pPort >> 32), (UINT32)(p->pPort), p->cmdTrans, p->dwBytes,
        p->fAutoinc, p->dwOptions);
#else
    LOG(ident + 1, "pPort=%x, cmdTrans=%x, dwBytes=%x, fAutoinc=%x, "
        "dwOptions=%x\n",
        p->pPort, p->cmdTrans, p->dwBytes, p->fAutoinc, p->dwOptions);
#endif
    LOG(ident + 1, "Data=");
    log_hexbuf(&p->Data, sizeof(p->Data), 0);
}

static void log_WD_CLEANUP_SETUP(WD_CARD_CLEANUP *p, int ident)
{
    DWORD i;
    LOG(ident, "WD_CLEANUP_SETUP:\n");
    LOG(ident + 1, "hCard=%x, dwOptions=%x, dwCmds=%x\n",
        p->hCard, p->dwOptions, p->dwCmds);
    for (i = 0; i < p->dwCmds; i++)
    {
        LOG(ident + 1, "[%x]", i);
        log_WD_TRANSFER(&p->Cmd[i], ident + 1);
    }
}

static void log_WD_DMA_PAGE(WD_DMA_PAGE *p, int ident)
{
    LOG(ident, "WD_DMA_PAGE:\n");
#if defined(KERNEL_64BIT)
    LOG(ident + 1, "pPhysicalAddr=%x:%x, dwBytes=%x\n",
        (UINT32)(p->pPhysicalAddr >> 32), (UINT32)(p->pPhysicalAddr),
        p->dwBytes);
#else
    LOG(ident + 1, "pPhysicalAddr=%x, dwBytes=%x\n", p->pPhysicalAddr,
        p->dwBytes);
#endif
}

static void log_WD_KERNEL_PLUGIN_CALL(WD_KERNEL_PLUGIN_CALL *p, int ident)
{
    LOG(ident, "WD_KERNEL_PLUGIN_CALL:\n");
    LOG(ident + 1, "hKernelPlugIn=%x, dwMessage=%x, pData=%p, dwResult=%x\n",
        p->hKernelPlugIn, p->dwMessage, p->pData, p->dwResult);
}

#define INIT_STRUCT_LOG(s_type) \
    s_type *p = (s_type *)src; \
    LOG(ident, #s_type ", %x (%x)\n", dwIoctl, \
    WD_CTL_DECODE_FUNC(dwIoctl));

static int print_ioctl_data(DWORD dwIoctl, PVOID src, DWORD src_bytes)
{
    DWORD i;
    int ident = 0;

    switch (dwIoctl)
    {
    case IOCTL_WD_CARD_REGISTER:
    case IOCTL_WD_CARD_UNREGISTER:
        {
            INIT_STRUCT_LOG(WD_CARD_REGISTER);
            log_WD_CARD(&p->Card, ident + 1);
            LOG(ident + 1, "fCheckLockOnly=%x, hCard=%x, dwOptions=%x\n",
                p->fCheckLockOnly, p->hCard, p->dwOptions);
            LOG(ident + 1, "cName=%s\n", STR(p->cName));
            LOG(ident + 1, "cDescription=%s\n", STR(p->cDescription));
            break;
        }

    case IOCTL_WD_IPC_REGISTER:
        {
            INIT_STRUCT_LOG(WD_IPC_REGISTER);
            log_WD_IPC_PROCESS(&p->procInfo, ident + 1);
            LOG(ident + 1, "dwOptions=0x%x\n", p->dwOptions);
            break;
        }

    case IOCTL_WD_IPC_UNREGISTER:
        {
            INIT_STRUCT_LOG(WD_IPC_PROCESS);
            log_WD_IPC_PROCESS(p, ident + 1);
            break;
        }

    case IOCTL_WD_IPC_SCAN_PROCS:
        {
            DWORD i;
            INIT_STRUCT_LOG(WD_IPC_SCAN_PROCS);
            LOG(ident + 1, "hIpc=%ld, dwNumProcs=%ld\n", p->hIpc,
                p->dwNumProcs);
            for (i = 0; i < p->dwNumProcs; i++)
                log_WD_IPC_PROCESS(&p->procInfo[i], ident + 1);
            break;
        }

    case IOCTL_WD_IPC_SEND:
        {
            INIT_STRUCT_LOG(WD_IPC_SEND);
            LOG(ident + 1, "hIpc=%ld, dwOptions=0x%x, dwRecipientID=0x%x, "
                "dwMsgID=0x%x, qwMsgData=0x%llx\n", p->hIpc, p->dwOptions,
                p->dwRecipientID, p->dwMsgID, p->qwMsgData);
            break;
        }

    case IOCTL_WD_CARD_CLEANUP_SETUP:
        {
            INIT_STRUCT_LOG(WD_CARD_CLEANUP);
            log_WD_CLEANUP_SETUP(p, ident + 1);
            break;
        }
    case IOCTL_WD_DEBUG:
        {
            INIT_STRUCT_LOG(WD_DEBUG);
            LOG(ident + 1, "dwCmd=%x, dwLevel=%x, dwSection=%x, "
                "dwLevelMessageBox=%x, dwBufferSize=%x\n",
                p->dwCmd, p->dwLevel, p->dwSection, p->dwLevelMessageBox,
                p->dwBufferSize);
            break;
        }
    case IOCTL_WD_DEBUG_ADD:
        {
            INIT_STRUCT_LOG(WD_DEBUG_ADD);
            LOG(ident + 1, "pcBuffer=%p, dwLevel=%x, dwSection=%x\n",
                p->pcBuffer, p->dwLevel, p->dwSection);
            break;
        }
    case IOCTL_WD_DEBUG_DUMP:
        {
            INIT_STRUCT_LOG(WD_DEBUG_DUMP);
            LOG(ident + 1, "cBuffer=%s\n", p->cBuffer);
            break;
        }
    case IOCTL_WD_DMA_LOCK:
    case IOCTL_WD_DMA_UNLOCK:
        {
            INIT_STRUCT_LOG(WD_DMA);
#if defined(KERNEL_64BIT)
            LOG(ident + 1, "hDma=%x, pUserAddr=%p, pKernelAddr%x:%x\n",
                p->hDma, p->pUserAddr, (UINT32)(p->pKernelAddr >> 32),
                (UINT32)(p->pKernelAddr));
#else
            LOG(ident + 1, "hDma=%x, pUserAddr=%p, pKernelAddr=%x\n", p->hDma,
                p->pUserAddr, p->pKernelAddr);
#endif
            LOG(ident + 1, "dwBytes=%x, dwOptions=%x, dwPages=%x, "
                "hCard=%x\n", p->dwBytes, p->dwOptions, p->dwPages, p->hCard);
            if (p->hDma)
            {
                for (i = 0; i < p->dwPages; i++)
                {
                    LOG(ident + 1, "[%x]", i);
                    log_WD_DMA_PAGE(&p->Page[i], ident + 1);
                }
            }
            break;
        }
    case IOCTL_WD_EVENT_REGISTER:
    case IOCTL_WD_EVENT_UNREGISTER:
    case IOCTL_WD_EVENT_PULL:
    case IOCTL_WD_EVENT_SEND:
        {
            INIT_STRUCT_LOG(WD_EVENT);

            LOG(ident + 1, "hEvent=%x, dwEventType=%x, dwAction=%x, "
                "dwEventId=%x\n", p->hEvent, p->dwEventType, p->dwAction,
                p->dwEventId);
            LOG(ident + 1, "hKernelPlugIn=%x, dwOptions=%x\n",
                p->hKernelPlugIn, p->dwOptions);

            switch (p->dwEventType)
            {
            case WD_EVENT_TYPE_PCI:
                LOG(ident + 1, "union u <WD_BUS_PCI>:\n");
                log_WD_PCI_ID(&p->u.Pci.cardId, ident + 2);
                log_WD_PCI_SLOT(&p->u.Pci.pciSlot, ident + 2);
                break;

            case WD_EVENT_TYPE_USB:
                LOG(ident + 1, "union u <WD_BUS_USB>:\n");
                LOG(ident + 2, "dwUniqueID=%x\n", p->u.Usb.dwUniqueID);
                break;

            case WD_EVENT_TYPE_IPC:
                LOG(ident + 1, "union u <WD_BUS_UNKNOWN>:\n");
                LOG(ident + 2, "hIpc=%x, dwSubGroupID=%x, dwGroupID=%x, "
                    "dwSenderUID=%x, dwMsgID=%x, qwMsgData=%llx\n",
                    p->u.Ipc.hIpc, p->u.Ipc.dwSubGroupID, p->u.Ipc.dwGroupID,
                    p->u.Ipc.dwSenderUID, p->u.Ipc.dwMsgID, p->u.Ipc.qwMsgData);
                break;

            default:
                LOG(ident + 1, "ERROR: Unknown event type\n");
                break;
            }

            LOG(ident + 1, "dwNumMatchTables=%x\n", p->dwNumMatchTables);

            /* Pointer to an array of size 1 */
            log_WDU_MATCH_TABLE(p->matchTables, ident + 1);
            break;
        }

    case IOCTL_WD_INT_COUNT:
    case IOCTL_WD_INT_DISABLE:
    case IOCTL_WD_INT_ENABLE:
    case IOCTL_WD_INT_WAIT:
        {
            INIT_STRUCT_LOG(WD_INTERRUPT);
            LOG(ident + 1, "hInterrupt=%x, dwOptions=%x, dwCmds=%x\n",
                p->hInterrupt, p->dwOptions, p->dwCmds);
            for (i = 0; i < p->dwCmds; i++)
            {
                LOG(ident + 1, "[%x]", i);
                log_WD_TRANSFER(&p->Cmd[i], ident + 1);
            }
            log_WD_KERNEL_PLUGIN_CALL(&p->kpCall, ident + 1);
            LOG(ident + 1, "fEnableOk=%x, dwCounter=%x, dwLost=%x, "
                "fStopped=%x\n", p->fEnableOk, p->dwCounter, p->dwLost,
                p->fStopped);
            break;
        }
    case IOCTL_WD_KERNEL_PLUGIN_CALL:
        {
            log_WD_KERNEL_PLUGIN_CALL((WD_KERNEL_PLUGIN_CALL *)src, ident);
            break;
        }
    case IOCTL_WD_KERNEL_PLUGIN_CLOSE:
    case IOCTL_WD_KERNEL_PLUGIN_OPEN:
        {
            INIT_STRUCT_LOG(WD_KERNEL_PLUGIN);
            LOG(ident + 1, "hKernelPlugIn=%x, cDriverName=%s, "
                "cDriverPath=%s, " "pOpenData=%p\n", p->hKernelPlugIn,
                STR(p->cDriverName), STR(p->cDriverPath), p->pOpenData);
            break;
        }
    case IOCTL_WD_LICENSE:
        {
            INIT_STRUCT_LOG(WD_LICENSE);
            LOG(ident + 1, "cLicense=%s\n", STR(p->cLicense));
            break;
        }
    case IOCTL_WD_MULTI_TRANSFER:
        {
            WD_TRANSFER *p = (WD_TRANSFER *)src;

            LOG(ident, "WD_MULTI_TRANSFER:\n");
            for (i = 0; i < src_bytes / sizeof(WD_TRANSFER); i++)
            {
                LOG(ident + 1, "[%x]", i);
                log_WD_TRANSFER(&p[i], ident + 1);
            }
            break;
        }
    case IOCTL_WD_PCI_CONFIG_DUMP:
        {
            INIT_STRUCT_LOG(WD_PCI_CONFIG_DUMP);
            log_WD_PCI_SLOT(&p->pciSlot, ident + 1);
            LOG(ident + 1, "pBuffer=%p, dwOffset=%x, dwBytes=%x, "
                "fIsRead=%x, dwResult=%x\n", p->pBuffer, p->dwOffset,
                p->dwBytes, p->fIsRead, p->dwResult);
            break;
        }
    case IOCTL_WD_PCI_GET_CARD_INFO:
        {
            INIT_STRUCT_LOG(WD_PCI_CARD_INFO);
            log_WD_PCI_SLOT(&p->pciSlot, ident + 1);
            log_WD_CARD(&p->Card, ident + 1);
            break;
        }
    case IOCTL_WD_PCI_SCAN_CARDS:
        {
            INIT_STRUCT_LOG(WD_PCI_SCAN_CARDS);
            LOG(ident + 1, "searchId.dwVendorId=%x, searchId.dwDeviceId=%x, "
                "dwOptions=%x\n", p->searchId.dwVendorId,
                p->searchId.dwDeviceId, p->dwOptions);
            LOG(ident + 1, "dwCards=%x\n", p->dwCards);
            for (i = 0; i < p->dwCards; i++)
            {
                LOG(ident + 1, "[%x]", i);
                log_WD_PCI_ID(&p->cardId[i], ident + 1);
                LOG(ident + 1, "[%x]", i);
                log_WD_PCI_SLOT(&p->cardSlot[i], ident + 1);
            }
            break;
        }
    case IOCTL_WD_PCI_SCAN_CAPS:
        {
            INIT_STRUCT_LOG(WD_PCI_SCAN_CAPS);
            log_WD_PCI_SLOT(&p->pciSlot, ident + 1);
            LOG(ident + 1, "dwCapID [0x%x], dwOptions [0x%x]\n", p->dwCapId,
                p->dwOptions);
            LOG(ident + 1, "dwNumCaps [0x%x]\n", p->dwNumCaps);
            for (i = 0; i < p->dwNumCaps; i++)
            {
                LOG(ident + 1, "[0x%x]", i);
                log_WD_PCI_CAP(&p->pciCaps[i], ident + 1);
            }
            break;
        }
    case IOCTL_WD_SLEEP:
        {
            INIT_STRUCT_LOG(WD_SLEEP);
            LOG(ident + 1, "dwMicroSeconds=%x, dwOptions=%x\n",
                p->dwMicroSeconds, p->dwOptions);
            break;
        }
    case IOCTL_WD_TRANSFER:
        {
            INIT_STRUCT_LOG(WD_TRANSFER);
            log_WD_TRANSFER(p, ident + 1);
            break;
        }
    case IOCTL_WD_USAGE:
        {
            INIT_STRUCT_LOG(WD_USAGE);
            LOG(ident + 1, "applications_num=%x, devices_num=%x\n",
                p->applications_num, p->devices_num);
            break;
        }
    case IOCTL_WD_VERSION:
        {
            INIT_STRUCT_LOG(WD_VERSION);
            LOG(ident + 1, "dwVer=%x, cVer=%s\n", p->dwVer, STR(p->cVer));
            break;
        }
    case IOCTL_WDU_GET_DEVICE_DATA:
        {
            INIT_STRUCT_LOG(WDU_GET_DEVICE_DATA);
            LOG(ident + 1, "dwUniqueID=%x, pBuf=%p, dwBytes=%x, "
                "dwOptions=%x\n", p->dwUniqueID, p->pBuf, p->dwBytes,
                p->dwOptions);
            break;
        }
    case IOCTL_WDU_SET_INTERFACE:
        {
            INIT_STRUCT_LOG(WDU_SET_INTERFACE);
            LOG(ident + 1, "dwUniqueID=%x, dwInterfaceNum=%x, "
                "dwAlternateSetting=%x, dwOptions=%x\n", p->dwUniqueID,
                p->dwInterfaceNum, p->dwAlternateSetting, p->dwOptions);
            break;
        }
    case IOCTL_WDU_RESET_PIPE:
        {
            INIT_STRUCT_LOG(WDU_RESET_PIPE);
            LOG(ident + 1, "dwUniqueID=%x, dwPipeNum=%x, dwOptions=%x\n",
                p->dwUniqueID, p->dwPipeNum, p->dwOptions);
            break;
        }
    case IOCTL_WDU_TRANSFER:
        {
            INIT_STRUCT_LOG(WDU_TRANSFER);
            LOG(ident + 1, "dwUniqueID=%x, dwPipeNum=%x, fRead=%x, "
                "dwOptions=%x\n", p->dwUniqueID, p->dwPipeNum, p->fRead,
                p->dwOptions);
            LOG(ident + 1, "pBuffer=%p, dwBufferSize=%x, "
                "dwBytesTransferred=%x, " "dwTimeout=%x\n", p->pBuffer,
                p->dwBufferSize, p->dwBytesTransferred, p->dwTimeout);
            log_hexbuf(p->SetupPacket, sizeof(p->SetupPacket), ident + 1);
            break;
        }

    case IOCTL_WDU_HALT_TRANSFER:
        {
            INIT_STRUCT_LOG(WDU_HALT_TRANSFER);
            LOG(ident + 1, "dwUniqueID=%x, dwPipeNum=%x, dwOptions=%x\n",
                p->dwUniqueID, p->dwPipeNum, p->dwOptions);
            break;
        }
    case IOCTL_WDU_WAKEUP:
        {
            INIT_STRUCT_LOG(WDU_WAKEUP);
            LOG(ident + 1, "dwUniqueID=%x, dwOptions=%x\n", p->dwUniqueID,
                p->dwOptions);
            break;
        }
    case IOCTL_WDU_RESET_DEVICE:
        {
            INIT_STRUCT_LOG(WDU_RESET_DEVICE);
            LOG(ident + 1, "dwUniqueID=%x, dwOptions=%x\n", p->dwUniqueID,
                p->dwOptions);
            break;
        }
    case IOCTL_WDU_SELECTIVE_SUSPEND:
        {
            INIT_STRUCT_LOG(WDU_SELECTIVE_SUSPEND);
            LOG(ident + 1, "dwUniqueID=%x, dwOptions=%x\n", p->dwUniqueID,
                p->dwOptions);
            break;
        }
    case IOCTL_WDU_STREAM_OPEN:
        {
            INIT_STRUCT_LOG(WDU_STREAM);
            LOG(ident + 1, "dwUniqueID=%x, dwOptions=%x\n", p->dwUniqueID,
                p->dwOptions);
            break;
        }
    case IOCTL_WDU_STREAM_CLOSE:
        {
            INIT_STRUCT_LOG(WDU_STREAM);
            LOG(ident + 1, "dwUniqueID=%x, dwOptions=%x\n", p->dwUniqueID,
                p->dwOptions);
            break;
        }
    case IOCTL_WDU_STREAM_FLUSH:
        {
            INIT_STRUCT_LOG(WDU_STREAM);
            LOG(ident + 1, "dwUniqueID=%x, dwOptions=%x\n", p->dwUniqueID,
                p->dwOptions);
            break;
        }
    case IOCTL_WDU_STREAM_START:
        {
            INIT_STRUCT_LOG(WDU_STREAM);
            LOG(ident + 1, "dwUniqueID=%x, dwOptions=%x\n", p->dwUniqueID,
                p->dwOptions);
            break;
        }
    case IOCTL_WDU_STREAM_STOP:
        {
            INIT_STRUCT_LOG(WDU_STREAM);
            LOG(ident + 1, "dwUniqueID=%x, dwOptions=%x\n", p->dwUniqueID,
                p->dwOptions);
            break;
        }
    case IOCTL_WDU_STREAM_GET_STATUS:
        {
            INIT_STRUCT_LOG(WDU_STREAM_STATUS);
            LOG(ident + 1, "dwUniqueID=%x, dwOptions=%x\n", p->dwUniqueID,
                p->dwOptions);
            break;
        }

    default:
        {
            return WD_INVALID_PARAMETER;
        }
    }
    return WD_STATUS_SUCCESS;
}

