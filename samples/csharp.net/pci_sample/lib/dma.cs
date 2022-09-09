/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

// Note: This code sample is provided AS-IS and as a guiding sample only.

using System;
using System.Runtime.InteropServices;

using Jungo.wdapi_dotnet;
using wdc_err = Jungo.wdapi_dotnet.WD_ERROR_CODES;
using WDC_DEVICE_HANDLE = System.IntPtr;
using DWORD = System.UInt32;
using UINT32 = System.UInt32;
using BOOL = System.Boolean;

namespace Jungo.pci_lib
{
    public struct DmaDesc
    {
        public uint u32PADR;
        public uint u32SIZ;
    };

    public abstract class DmaBuffer
    {
        internal WDC_DEVICE_HANDLE m_hDev;
        internal IntPtr m_pDma;
        internal DWORD m_dwBuffSize;
        public Log dmaLog;

        public DmaBuffer(PCI_Device dev, Log log)
        {
            m_hDev = dev.Handle;
            dmaLog = log;
        }

        public WDC_DEVICE_HANDLE DeviceHandle
        {
            get
            {
                return m_hDev;
            }
        }

        public IntPtr pWdDma
        {
            get
            {
                return m_pDma;
            }
        }


        public DWORD BuffSize
        {
            get
            {
                return m_dwBuffSize;
            }
        }


        public BOOL IsDMAOpen()
        {
            return (m_pDma != IntPtr.Zero);
        }

        internal WD_DMA MarshalDMA(IntPtr pDma)
        {
#if NET5_0
            return Marshal.PtrToStructure<WD_DMA>(pDma);
#else
            MarshalWdDma m_wdDmaMarshaler = new MarshalWdDma();
            return (WD_DMA)m_wdDmaMarshaler.MarshalNativeToManaged(pDma);
#endif
        }

        public abstract DWORD Open(DWORD dwBytes, ref IntPtr pBuffer);

        public abstract void Close();

        protected virtual void CommonClose()
        {
            DWORD dwStatus;

            if (m_pDma != IntPtr.Zero)
            {
                dwStatus = wdc_lib_decl.WDC_DMABufUnlock(m_pDma);
                if ((DWORD)wdc_err.WD_STATUS_SUCCESS != dwStatus)
                {
                    Log.ErrLog("DmaBuffer.Close: Failed unlocking DMA buffer."
                        + "Error 0x" + dwStatus.ToString("X") + ": " +
                        utils.Stat2Str(dwStatus));
                }
                m_pDma = IntPtr.Zero;
            }
            else
                Log.ErrLog("DmaBuffer.Close: DMA is not currently open ... ");

        }

        public virtual void Dispose()
        {
                Close();
        }
    }

    public class DmaBufferSG: DmaBuffer
    {
        private IntPtr m_pDmaList = IntPtr.Zero;
        public DmaBufferSG(PCI_Device dev, Log log)
            : base(dev, log){}

        public IntPtr pWdDmaList
        {
            get
            {
                return m_pDmaList;
            }
        }

        public override DWORD Open(DWORD dwBytes, ref IntPtr pBuffer)
        {
            DWORD dwStatus;
            WD_DMA wdDma = new WD_DMA();
            m_dwBuffSize = dwBytes;
            string str = "";
            string mgrStr = "";
            int i;

            if (pBuffer == IntPtr.Zero)
            {
                Log.ErrLog("DmaBuffer.Open: Scatter/Gather DMA data buffer" +
                    " is null");
                return (DWORD)wdc_err.WD_INVALID_PARAMETER;
            }

            dwStatus = wdc_lib_decl.WDC_DMASGBufLock(m_hDev, pBuffer, 0,
                m_dwBuffSize, ref m_pDma);

            if ((DWORD)wdc_err.WD_STATUS_SUCCESS != dwStatus)
            {
                Log.ErrLog("DmaBuffer.Open: Failed locking a DMA buffer. " +
                    "Error 0x" + dwStatus.ToString("X") +": " +
                    utils.Stat2Str(dwStatus)+ ". size: [" +
                        m_dwBuffSize.ToString("X") + "].");
                goto Error;
            }

            /* marshaling WD_DMA */
            wdDma = MarshalDMA(m_pDma);

            DmaDesc[] list;
            DWORD dwPageNumber;
            list = new DmaDesc[wdDma.dwPages];
            for (dwPageNumber = 0; dwPageNumber < wdDma.dwPages;
                dwPageNumber++)
            {
                list[dwPageNumber].u32PADR =
                    (uint)wdDma.Page[dwPageNumber].pPhysicalAddr;
                list[dwPageNumber].u32SIZ =
                    (uint)wdDma.Page[dwPageNumber].dwBytes;
            }

            Log.TraceLog("SG memory allocated. User address [0x" +
                pBuffer.ToString("X16") + "] size [" +
                m_dwBuffSize.ToString("X8") + "]" + Environment.NewLine +
                "Pages physical addresses:");

            for (dwPageNumber = 0, i = 0; dwPageNumber < wdDma.dwPages;
                ++i, dwPageNumber++)
            {
                str += i +")  physical addr  [0x" +
                    list[dwPageNumber].u32PADR.ToString("X8") +
                    "] size  [" + list[dwPageNumber].u32SIZ.ToString() +
                    "  (0x" + list[dwPageNumber].u32SIZ.ToString("X6") +
                    ")]\r\n";

                if (dwPageNumber == wdDma.dwPages - 1 || i % 1000 == 0)
                {
                    mgrStr += str;
                    str = "";
                }
            }
            Log.TraceLog(mgrStr);

            return (DWORD)wdc_err.WD_STATUS_SUCCESS;

Error:
            Close();
            return dwStatus;
        }

        public override void Close()
        {
            CommonClose();
        }
    }

    public class DmaBufferContig: DmaBuffer
    {
        public DmaBufferContig(PCI_Device dev, Log log)
            : base(dev, log){}

        public override DWORD Open(DWORD dwBytes, ref IntPtr pBuffer)
        {
            DWORD dwStatus;
            WD_DMA wdDma = new WD_DMA();
            m_dwBuffSize = dwBytes;
            /* Allocate and lock a DMA buffer */
            dwStatus = wdc_lib_decl.WDC_DMAContigBufLock(m_hDev, ref pBuffer,
                0, dwBytes, ref m_pDma);

            if ((DWORD)wdc_err.WD_STATUS_SUCCESS != dwStatus)
            {
                Log.ErrLog("DmaBuffer.Open: Failed locking a DMA buffer. " +
                    "Error 0x" + dwStatus.ToString("X") +": " +
                    utils.Stat2Str(dwStatus)+ ". size: [" +
                        m_dwBuffSize.ToString("X") + "].");
                goto Error;
            }

            /* marshaling WD_DMA */
            wdDma = MarshalDMA(m_pDma);

            if (wdDma.dwPages == 1)
            {
                Log.TraceLog("Contiguous memory allocated. User address [0x" +
                pBuffer.ToString("X16") + "] size [" +
                dwBytes.ToString("X") + "]");
                return (DWORD)wdc_err.WD_STATUS_SUCCESS;
            }

Error:
            Close();
            return dwStatus;
        }

        public override void Close()
        {
            CommonClose();
        }
    }
}
