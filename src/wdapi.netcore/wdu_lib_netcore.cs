/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

using System;
using System.Runtime.InteropServices;

namespace Jungo
{
    using DWORD = System.UInt32;
    using WORD = System.UInt16;
    using BYTE = System.Byte;
    using WDU_DEVICE_HANDLE = System.IntPtr;
    using WDU_DRIVER_HANDLE = System.IntPtr;

    namespace wdapi_dotnet
    {
        public delegate bool D_WDU_ATTACH_CALLBACK(WDU_DEVICE_HANDLE hDevice,
            ref WDU_DEVICE pDeviceInfo, IntPtr pUserData);

        public delegate void D_WDU_DETACH_CALLBACK(WDU_DEVICE_HANDLE hDevice,
            IntPtr pUserData);

        public delegate bool D_WDU_POWER_CHANGE_CALLBCAK(
            WDU_DEVICE_HANDLE hDevice, int dwPowerState, IntPtr pUserData);

        public struct WDU_EVENT_TABLE
        {
            public D_WDU_ATTACH_CALLBACK pfDeviceAttach;
            public D_WDU_DETACH_CALLBACK pfDeviceDetach;
            public D_WDU_POWER_CHANGE_CALLBCAK pfPowerChange;
            public IntPtr pUserData;

            public WDU_EVENT_TABLE(D_WDU_ATTACH_CALLBACK _pfDeviceAttach,
                D_WDU_DETACH_CALLBACK _pfDeviceDetach)
            {
                pfDeviceAttach = _pfDeviceAttach;
                pfDeviceDetach = _pfDeviceDetach;
                pfPowerChange = null;
                pUserData = IntPtr.Zero;
            }

            public WDU_EVENT_TABLE(D_WDU_ATTACH_CALLBACK _pfDeviceAttach,
                D_WDU_DETACH_CALLBACK _pfDeviceDetach, IntPtr _pUserData)
            {
                pfDeviceAttach = _pfDeviceAttach;
                pfDeviceDetach = _pfDeviceDetach;
                pUserData = _pUserData;
                pfPowerChange = null;
            }

            public WDU_EVENT_TABLE(ref D_WDU_ATTACH_CALLBACK _pfDeviceAttach,
                ref D_WDU_DETACH_CALLBACK _pfDeviceDetach,
                ref D_WDU_POWER_CHANGE_CALLBCAK _pfPowerChange,
                IntPtr _pUserData)
            {
                pfDeviceAttach = _pfDeviceAttach;
                pfDeviceDetach = _pfDeviceDetach;
                pUserData = _pUserData;
                pfPowerChange = _pfPowerChange;
            }
        };

        //" API declarations "

        public class wdu_lib_decl
        {
            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDU_Init(ref WDU_DRIVER_HANDLE phDriver,
                WDU_MATCH_TABLE[] pMatchTables,
                DWORD dwNumMatchTables, ref WDU_EVENT_TABLE pEventTable,
                String sLicense, DWORD dwOptions);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern void WDU_Uninit(WDU_DRIVER_HANDLE hDriver);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDU_GetDeviceAddr(
                WDU_DEVICE_HANDLE hDevice, ref DWORD pAddress);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDU_GetDeviceInfo(
                WDU_DEVICE_HANDLE hDevice, ref IntPtr ppDeviceInfo);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern void WDU_PutDeviceInfo(IntPtr pDeviceInfo);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDU_SetInterface(
                WDU_DEVICE_HANDLE hDevice, DWORD dwInterfaceNum,
                DWORD dwAlternateSetting);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDU_ResetPipe(WDU_DEVICE_HANDLE hDevice,
                    DWORD dwPipeNum);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDU_ResetDevice(
                WDU_DEVICE_HANDLE hDevice, DWORD dwOptions);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDU_Wakeup(WDU_DEVICE_HANDLE hDevice,
                DWORD dwOptions);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDU_SelectiveSuspend(
                WDU_DEVICE_HANDLE hDevice, DWORD dwOptions);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDU_Transfer(WDU_DEVICE_HANDLE hDevice,
                DWORD dwPipeNum, DWORD fRead, DWORD dwOptions,
                BYTE[] pBuffer, DWORD dwBufferSize,
                ref DWORD pdwBytesTransferred, BYTE[] pSetupPacket,
                DWORD dwTimeout);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDU_TransferDefaultPipe(
                WDU_DEVICE_HANDLE hDevice,
                DWORD fRead, DWORD dwOptions, BYTE[] pBuffer,
                DWORD dwBufferSize, ref DWORD pdwBytesTransferred,
                BYTE[] pSetupPacket, DWORD dwTimeout);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDU_TransferBulk(
                WDU_DEVICE_HANDLE hDevice,
                DWORD dwPipeNum, DWORD fRead, DWORD dwOptions,
                BYTE[] pBuffer, DWORD dwBufferSize,
                ref DWORD pdwBytesTransferred, DWORD dwTimeout);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDU_TransferIsoch(
                WDU_DEVICE_HANDLE hDevice,
                DWORD dwPipeNum, DWORD fRead, DWORD dwOptions,
                BYTE[] pBuffer, DWORD dwBufferSize,
                ref DWORD pdwBytesTransferred, DWORD dwTimeout);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDU_TransferInterrupt(
                WDU_DEVICE_HANDLE hDevice,
                DWORD dwPipeNum, DWORD fRead, DWORD dwOptions,
                BYTE[] pBuffer, DWORD dwBufferSize,
                ref DWORD pdwBytesTransferred, DWORD dwTimeout);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDU_HaltTransfer(
                WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDU_GetLangIDs(
                WDU_DEVICE_HANDLE hDevice,
                ref BYTE pbNumSupportedLangIDs, WORD[] pLangIDs,
                BYTE bNumLangIDs);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD WDU_GetStringDesc
                (WDU_DEVICE_HANDLE hDevice,
                BYTE bStrIndex, BYTE[] pbBuf, DWORD dwBufferSize,
                WORD lang, ref DWORD pdwDescSize);
        }
    }
}
