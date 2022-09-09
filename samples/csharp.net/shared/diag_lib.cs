using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using Jungo.wdapi_dotnet;

using static Jungo.wdapi_dotnet.WD_ERROR_CODES;

using DWORD = System.UInt32;

namespace Jungo.diag_lib
{
    public delegate WD_ERROR_CODES MenuDelegate(object ctx);
    public delegate bool IsHiddenMenuDelegate(DiagMenuOption Menu);

    public class DiagMenuOption
    {
        public string OptionName { get; init; }
        public string TitleName { get; init; }
        public MenuDelegate CbEntry;
        public MenuDelegate CbExit;
        public object Context { get; set; }
        public bool IsHidden { get; set; }
        public IsHiddenMenuDelegate CbIsHidden;
        public IList<DiagMenuOption> ChildMenus = new List<DiagMenuOption>();
        public DiagMenuOption ParentMenu { get; set; }

        public void AddChild(DiagMenuOption childMenu)
        {
            childMenu.ParentMenu = this;
            ChildMenus.Add(childMenu);
        }
        public void AddChildren(IList<DiagMenuOption> childrenMenus)
        {
            foreach (DiagMenuOption menu in childrenMenus)
            {
                AddChild(menu);
            }
        }

        public void SetHiddenChildren()
        {
            foreach (DiagMenuOption menu in this.ChildMenus)
            {
                if (menu.CbIsHidden != null)
                    menu.IsHidden = menu.CbIsHidden(menu);
            }
        }

        public int GetUserInput()
        {
            int selectedItemMenuIndex = 0;
            bool isValidInput = false;

            while (!isValidInput)
            {
                Console.Write("Enter option: ");
                bool isNumeric = int.TryParse(Console.ReadLine(),
                    out selectedItemMenuIndex);
                bool isExit = (selectedItemMenuIndex == DiagMenu.EXIT_MENU);
                bool isInRange = selectedItemMenuIndex > 0 &&
                    selectedItemMenuIndex <= ChildMenus.Count;

                if (!isNumeric)
                {
                    Console.WriteLine("Invalid input, please enter a " +
                        "numeric value");
                }
                else if (!isExit && (!isInRange ||
                    ChildMenus[selectedItemMenuIndex - 1].IsHidden))
                {
                    Console.WriteLine("Invalid selected menu item, please " +
                        "try again");
                }
                else
                {
                    isValidInput = true;
                    if (selectedItemMenuIndex != DiagMenu.EXIT_MENU)
                        selectedItemMenuIndex--;
                }
            }

            return selectedItemMenuIndex;
        }

        public void PrintTitle()
        {
            string separator =
                string.Concat(Enumerable.Repeat("-", TitleName.Length));

            Console.WriteLine("{0}{1}{0}{2}", Environment.NewLine,
                TitleName, separator);
        }

        public void PrintOptions()
        {
            int counter = 0;

            for (int i = 0; i < ChildMenus.Count; i++)
            {
                if (!ChildMenus[i].IsHidden)
                {
                    Console.WriteLine("{0}. {1}", (i + 1),
                        ChildMenus[i].OptionName);

                    counter++;
                }
            }

            Console.WriteLine("{0}. Exit Menu{1}", DiagMenu.EXIT_MENU,
                Environment.NewLine);
        }
    }

    public static class DiagMenu
    {
        public const int EXIT_NO_CHILDREN = -1;
        public const int EXIT_MENU = 99;

        public static int RunOnce(in DiagMenuOption currentMenu)
        {
            int option;
            WD_ERROR_CODES status = WD_STATUS_SUCCESS;

            try
            {
                if (currentMenu.CbEntry != null)
                {
                    status = currentMenu.CbEntry(currentMenu.Context);
                    if (status != WD_STATUS_SUCCESS)
                        throw new WinDriverException(status);
                }

                if (currentMenu.ChildMenus.Count == 0)
                {
                    option = EXIT_NO_CHILDREN;
                }
                else
                {
                    if (currentMenu.TitleName != null)
                        currentMenu.PrintTitle();

                    currentMenu.SetHiddenChildren();
                    currentMenu.PrintOptions();

                    option = currentMenu.GetUserInput();
                    if (option == EXIT_MENU)
                    {
                        if (currentMenu.CbExit != null)
                        {
                            status = currentMenu.CbExit(currentMenu.Context);
                            if (status != WD_STATUS_SUCCESS)
                                throw new WinDriverException(status);
                        }
                    }
                }
            }

            catch (WinDriverException)
            {
                option = EXIT_MENU;
            }

            return option;
        }
        public static void Run(DiagMenuOption menu)
        {
            int option;
            DiagMenuOption currentMenu = menu;

            while (currentMenu != null)
            {
                option = RunOnce(in currentMenu);
                if (option == EXIT_MENU || option == EXIT_NO_CHILDREN)
                    currentMenu = currentMenu.ParentMenu;
                else
                    currentMenu = currentMenu.ChildMenus[option];
            }
        }
    }

    public static class Diag
    {
        const int BYTES_IN_LINE = 16;
        const int HEX_CHARS_PER_BYTE = 2;

        public static uint InputDword(bool hexNumber,
            string message = "Enter input", DWORD min = 0, DWORD max = 0)
        {
            uint number;
            NumberStyles numberStyle = hexNumber ?
                NumberStyles.HexNumber : NumberStyles.Number;


            Console.Write("{0} (to cancel press 'x'): {1}", message,
                hexNumber ? "0x" : "");


            bool isNumeric = uint.TryParse(Console.ReadLine(), numberStyle,
                CultureInfo.InvariantCulture, out number);

            ValidateInput(number, isNumeric, hexNumber, min, max);

            return number;
        }

        public static UInt64 InputQword(bool hexNumber,
            string message = "Enter input", DWORD min = 0, DWORD max = 0)
        {
            UInt64 number;
            NumberStyles numberStyle = hexNumber ?
                NumberStyles.HexNumber : NumberStyles.Number;

            Console.Write("{0} (to cancel press 'x'): {1}", message,
                hexNumber ? "0x" : "");

            bool isNumeric = UInt64.TryParse(Console.ReadLine(), numberStyle,
                CultureInfo.InvariantCulture, out number);

            ValidateInput(number, isNumeric, hexNumber, min, max);

            return number;
        }

        private static void ValidateInput(UInt64 number, bool isNumeric,
            bool hexNumber, DWORD min = 0, DWORD max = 0)
        {
            if (!isNumeric)
                throw new WinDriverException(WD_INVALID_PARAMETER);

            if (min < max && (number < min || number > max))
            {
                string rangeStr = hexNumber ?
                    $"0x{min:x} and 0x{max:x}" : $"{min} and {max}";
                throw new WinDriverException(WD_INVALID_PARAMETER,
                    $"Invalid input: Input must be between {rangeStr}");
            }
        }

        public static DWORD GetHexBuffer(in Byte[] pBuffer)
        {
            string str = Console.ReadLine();
            str = str.Replace(" ", "").Trim();

            int bytes = Math.Max(1, Math.Min(str.Length / 2,
                 pBuffer.Length));

            DWORD dwBytesRead = 0;
            for (int i = 0; i < bytes; i++)
            {
                if (byte.TryParse(str.Substring(i * 2, Math.Min(str.Length, 2)),
                    NumberStyles.HexNumber, CultureInfo.InvariantCulture,
                    out byte number))
                {
                    pBuffer[dwBytesRead++] = number;
                }
                else
                {
                    dwBytesRead = 0;
                    break;
                }
            }

            return dwBytesRead;
        }

        private static String BytesToHex(byte[] bytes)
        {
            char[] hexArray = "0123456789ABCDEF".ToCharArray();
            char[] hexChars = new char[bytes.Length * 2];

            for (int j = 0; j < bytes.Length; j++)
            {
                int v = bytes[j] & 0xFF;
                hexChars[j * 2] = hexArray[v >> 4];
                hexChars[j * 2 + 1] = hexArray[v & 0x0F];
            }

            return new String(hexChars) + Environment.NewLine;
        }

        public static void PrintHexBuffer(byte[] pbData)
        {
            int dwLineOffset;
            String buf = BytesToHex(pbData);

            if (pbData == null)
            {
                throw new WinDriverException(WD_INVALID_PARAMETER,
                       "NULL buffer pointer");
            }
            if (pbData.Length == 0)
            {
                throw new WinDriverException(WD_INVALID_PARAMETER,
                    "Empty buffer");
            }

            for (int offset = 0; offset < buf.Length; offset++)
            {
                dwLineOffset = offset % BYTES_IN_LINE;
                if (offset > 0 && dwLineOffset == 0)
                    Console.WriteLine();
                if (offset % HEX_CHARS_PER_BYTE == 0 && dwLineOffset > 0)
                    Console.Write(" ");

                Console.Write(buf[offset]);
            }
        }
    }
    public class WinDriverException : Exception
    {
        public WinDriverException(WD_ERROR_CODES status)
     : base()
        {
            Console.WriteLine("Error [0x{0:x} - {1}]", status,
                utils.Stat2Str((DWORD)status));
        }
        public WinDriverException(DWORD dwStatus, string message)
            : this((WD_ERROR_CODES)dwStatus, message)
        {
        }
        public WinDriverException(WD_ERROR_CODES status, string message)
            : base(message)
        {
            Console.WriteLine("{0}. Error [0x{1:x} - {2}]", message,
                status, utils.Stat2Str((DWORD)status));
        }
    }

}


