/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

using System;
using System.Runtime.InteropServices;

namespace Jungo
{
    using DWORD = System.UInt32;

    namespace wdapi_dotnet
    {
        public class utils
        {
            // API declaration
            private class utils_nonmanaged
            {
                [DllImport(windrvr_decl.DLL_NAME)]
                public static unsafe extern char *Stat2Str(DWORD dwStatus);
            };

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern void PrintDbgMessage(DWORD dwLevel,
                DWORD dwSection, String sFormat, ref Object[] args);

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD GetPageSize();

            [DllImport(windrvr_decl.DLL_NAME)]
            public static extern DWORD GetNumberOfProcessors();

            // API wrapper
            public static unsafe String Stat2Str(DWORD dwStatus)
            {
                return Marshal.PtrToStringAnsi(new IntPtr(
                    utils_nonmanaged.Stat2Str(dwStatus)));
            }
        }
    }
}
