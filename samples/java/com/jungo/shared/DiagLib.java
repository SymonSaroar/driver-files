package com.jungo.shared;

import com.jungo.wdapi.*;
import java.util.Scanner;

/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

public class DiagLib {

    final public static int
    DIAG_INVALID_MENU = -1,
    DIAG_INPUT_CANCEL = -1,
    DIAG_INPUT_FAIL = 0,
    DIAG_INPUT_SUCCESS = 1;

    final public static char DIAG_CANCEL = 'x';

    final public static int
    DIAG_EXIT_MENU = 99,
    BYTES_IN_LINE = 32,
    HEX_CHARS_PER_BYTE = 2,
    HEX_STOP_POS = BYTES_IN_LINE * HEX_CHARS_PER_BYTE;

    final public static Scanner scanner = new Scanner(System.in);

    final public static int MENU_MAX_CHILDREN = 16;

    public interface MenuCallback {
        public long run(Object pCbCtx);
    }

    public interface MenuIsHiddenCallback {
        public boolean run(DiagMenuOption pMenu);
    }

    public static class DiagMenuOption {
        public String cOptionName; /**< The name of the option that will be
                                printed before selecting this menu */
        public String cTitleName; /**< The title name that will be printed at
                                the top of the menu(can be empty) */

        public MenuCallback cbEntry; /**< Callback function that runs when entering
                                    this menu */
        public MenuCallback cbExit;  /**< Callback function that runs when exiting
                                    this menu */
        public Object pCbCtx;               /**< The context needed for this menu
                                        (varibles that the menu needs to use)*/

        public boolean fIsHidden;  /**< Determine if this option should be visible
                            to the user, depending on its context*/
        public MenuIsHiddenCallback cbIsHidden; /**< Callback that returns whether
										        this menu should be hidden,
										        depending on the context.
										        This function is called
										        each time inside the menu loop */


        public int dwCurrentNumberOfChildren; /**< Current number of child menus in
                                             the pChildMenus array */
        public DiagMenuOption[] pChildMenus; /**< Array of all
                                           menus that can be
                                           accessed from this
                                           menu */
        public DiagMenuOption pParentMenu; /**< Parent of this menu. When
                           exiting the menu, the parent
                           menu will be shown */

        public DiagMenuOption(String cOptionName, String cTitleName,
                MenuCallback cbEntry, MenuCallback cbExit,
                MenuIsHiddenCallback cbIsHidden)
        {
            this.cOptionName = cOptionName;
            this.cTitleName = cTitleName;
            this.cbEntry = cbEntry;
            this.cbExit = cbExit;
            this.cbIsHidden = cbIsHidden;
            this.dwCurrentNumberOfChildren = 0;
            this.pChildMenus = new DiagMenuOption[MENU_MAX_CHILDREN];
            this.pParentMenu = null;
            this.pCbCtx = null;
        }

        public void DIAG_AddMenuOptionChild(DiagMenuOption pChildMenu)
        {
            if (pChildMenu != null)
            {
                if (this.dwCurrentNumberOfChildren < MENU_MAX_CHILDREN)
                {
                    this.pChildMenus[this.dwCurrentNumberOfChildren] =
                        pChildMenu;
                    this.dwCurrentNumberOfChildren++;
                    pChildMenu.pParentMenu = this;
                }
                else
                {
                    System.err.printf("DIAG_AddMenuOptionChild: %s",
                            "Error! children menu array is full.");
                }
            }
        }
    }

    // Builder class for DiagMenuOption, to simplify object creation
    public static class DiagMenuOptionBuilder
    {
        String _cOptionName = "";
        String _cTitleName = "";
        MenuCallback _cbEntry = null;
        MenuCallback _cbExit = null;
        MenuIsHiddenCallback _cbIsHidden = null;

        public DiagMenuOption build()
        {
            return new DiagMenuOption(_cOptionName, _cTitleName, _cbEntry,
                    _cbExit, _cbIsHidden);
        }

        public DiagMenuOptionBuilder cOptionName(String _cOptionName)
        {
            this._cOptionName = _cOptionName;
            return this;
        }

        public DiagMenuOptionBuilder cTitleName(String _cTitleName)
        {
            this._cTitleName = _cTitleName;
            return this;
        }

        public DiagMenuOptionBuilder cbEntry(MenuCallback _cbEntry)
        {
            this._cbEntry = _cbEntry;
            return this;
        }

        public DiagMenuOptionBuilder cbExit(MenuCallback _cbExit)
        {
            this._cbExit = _cbExit;
            return this;
        }

        public DiagMenuOptionBuilder cbIsHidden(
                MenuIsHiddenCallback _cbIsHidden)
        {
            this._cbIsHidden = _cbIsHidden;
            return this;
        }
    }

    public static void DIAG_MenuSetCtxAndParentForMenus(
            DiagMenuOption[] pMenusArr, Object pCtx,
            DiagMenuOption pParentMenu)
    {
        for (DiagMenuOption pMenu : pMenusArr)
        {
            if (pMenu != null)
            {
                pMenu.pCbCtx = pCtx;
                pParentMenu.DIAG_AddMenuOptionChild(pMenu);
            }
        }
    }

    private static void DIAG_MenuSetHiddenChildren(DiagMenuOption pMenu)
    {
        if (pMenu != null)
        {
            for (DiagMenuOption pChild : pMenu.pChildMenus)
            {
                if (pChild != null && pChild.cbIsHidden != null)
                	pChild.fIsHidden = pChild.cbIsHidden.run(pChild);
            }
        }
    }

    private static int DIAG_GetNextMenuOption(DiagMenuOption pCurrentMenu)
    {
        int dwOption = 0, dwStatus = 0;
        int dwMaxOption = pCurrentMenu.dwCurrentNumberOfChildren;

        System.out.println("Enter option: ");

        try
        {
            if (scanner.hasNext())
                dwOption = scanner.nextInt();
        }
        catch (Exception e)
        {
            System.out.printf("Invalid option\n");
            scanner.next();
            return DIAG_INVALID_MENU;
        }

        if (0 < dwOption && dwOption <= dwMaxOption)
        {
            if (pCurrentMenu.pChildMenus[dwOption - 1].fIsHidden)
                dwOption = DIAG_INVALID_MENU;
            else
                dwOption = dwOption - 1;
        }

        else if (dwOption != DIAG_EXIT_MENU)
        {
            dwOption = DIAG_INVALID_MENU;
        }

        if (dwOption == DIAG_INVALID_MENU)
            System.out.println("Invalid option");

        return dwOption;
    }

    private static void DIAG_MenuPrintTitleSeperator(String pcTitleName)
    {
        for(int i = 0; i < pcTitleName.length(); i++)
                System.out.print("-");

        System.out.println();
    }

    private static void DIAG_MenuPrintOptions(DiagMenuOption pMenu)
    {
        DiagMenuOption[] pChildren;

        if (pMenu != null)
        {
            pChildren = pMenu.pChildMenus;

            for (int i = 0; i < MENU_MAX_CHILDREN && pChildren[i] != null; i++)
            {
                if (!pChildren[i].fIsHidden)
                {
                    System.out.printf("%d. %s\n", (i + 1),
                        pChildren[i].cOptionName);
                }
            }

            System.out.printf("%d. Exit Menu\n", DIAG_EXIT_MENU);
            System.out.printf("\n");
        }
    }

    public static long DIAG_MenuRun(DiagMenuOption pMenuRoot)
    {
        DiagMenuOption pCurrentMenu = pMenuRoot;
        long dwOption, dwStatus = 0;

        while (pCurrentMenu != null)
        {
            dwStatus = 0;

            if (pCurrentMenu.cbEntry != null)
               dwStatus = pCurrentMenu.cbEntry.run(pCurrentMenu.pCbCtx);

            /* If error occurred, or we are in leaf node, go back to parent */
            if (dwStatus != 0 || pCurrentMenu.dwCurrentNumberOfChildren == 0)
            {
                pCurrentMenu = pCurrentMenu.pParentMenu;
                continue;
            }

            if (!pCurrentMenu.cTitleName.isEmpty())
            {
                System.out.printf("\n%s\n", pCurrentMenu.cTitleName);
                DIAG_MenuPrintTitleSeperator(pCurrentMenu.cTitleName);
            }

            DIAG_MenuSetHiddenChildren(pCurrentMenu);
            DIAG_MenuPrintOptions(pCurrentMenu);

            dwOption = DIAG_GetNextMenuOption(pCurrentMenu);
            if (dwOption == DIAG_EXIT_MENU)
            {
                if (pCurrentMenu.cbExit != null)
                    dwStatus = pCurrentMenu.cbExit.run(pCurrentMenu.pCbCtx);

                /* Go back to parent */
                pCurrentMenu = pCurrentMenu.pParentMenu;
            }
            else if (dwOption != DIAG_INVALID_MENU)
            {
                /* Go to chosen child */
                pCurrentMenu = pCurrentMenu.pChildMenus[(int) dwOption];
            }
        }

        return dwStatus;
    }

    // Mimic C __FUNCTION__ macro, will return the calling(!) function name
    public static String __FUNCTION__()
    {
        return Thread.currentThread().getStackTrace()[2].getMethodName();
    }

    public static void DIAG_PrintHexBuffer(byte[] pbData)
    {
        String buf = bytesToHex(pbData);
        int dwOffset, dwLineOffset;

        if (buf.isEmpty())
        {
            System.err.printf("DIAG_PrintHexBuffer: Error - %s\n",
                "Empty buffer");
            return;
        }

        for (dwOffset = 0; dwOffset < buf.length(); dwOffset++)
        {
            dwLineOffset = dwOffset % BYTES_IN_LINE;
            if (dwOffset != 0 && dwLineOffset == 0)
            {
                System.out.println();
            }
            if (dwOffset % HEX_CHARS_PER_BYTE == 0 && dwLineOffset > 0)
                System.out.print(" ");
            System.out.print(buf.charAt(dwOffset));

        }
    }

    public static int DIAG_GetHexChar()
    {
        int ch;
        try {
            ch = System.in.read();
        }
        catch (Exception e)
        {
            return -1;
        }

        if (Character.digit((char)ch, 16) == -1)
            return -1;

        if (Character.isDigit(ch))
            return ch - '0';
        else
            return Character.toUpperCase(ch) - 'A' + 10;
    }

    public static byte[] DIAG_GetHexBuffer(int dwBytes)
    {
        int dwBytesRead = 0;
        int res;
        int ch;
        byte[] pData = new byte[dwBytes];

        while (dwBytesRead < dwBytes)
        {
            ch = DIAG_GetHexChar();
            if (ch < 0)
                continue;

            res = ch << 4;

            ch = DIAG_GetHexChar();
            if (ch < 0)
                continue;

            res += ch;
            pData[dwBytesRead] = (byte)res;
            dwBytesRead++;
        }

        /* Advance to new line */
        do {
            try {
                ch = System.in.read();
            }
            catch (Exception e)
            {
                return pData;
            }
        } while (ch != '\n' && ch != '\r');

        /* Return the number of bytes that was read */
        return pData;
    }

    /* Get menu option from user */
    public static WDCResultInteger DIAG_GetMenuOption(int iMax)
    {
        int iRet = 0;

        System.out.printf("Enter option: ");

        try
        {
            if (scanner.hasNext())
                iRet = scanner.nextInt();
        }
        catch (Exception e)
        {
            System.out.printf("Invalid option\n");
            scanner.next();
            return new WDCResultInteger(DIAG_INPUT_FAIL, 0);
        }
        if (iRet < 1)
        {
            System.out.printf("Invalid option\n");
            return new WDCResultInteger(DIAG_INPUT_FAIL, 0);
        }

        if (iRet == DIAG_EXIT_MENU)
            return new WDCResultInteger(DIAG_INPUT_SUCCESS, DIAG_EXIT_MENU);

        if (iRet > iMax)
        {
            System.out.printf("Invalid option: Option must be %s%d, or %d to" +
                " exit\n", (1 == iMax) ? "" : "between 1 - ", iMax,
                DIAG_EXIT_MENU);
            return new WDCResultInteger(DIAG_INPUT_FAIL, 0);
        }

        return new WDCResultInteger(DIAG_INPUT_SUCCESS, iRet);
    }

    public static WDCResultLong DIAG_InputNum(String sInputText, boolean fHex,
        long min, long max)
    {
        long dwRet = 0;
        boolean fCheckRange = (max > min);
        String s;

        System.out.printf("%s (to cancel press 'x'): %s",
            (sInputText == null || sInputText.isEmpty()) ? "Enter input" :
            sInputText, fHex ? "0x" : "");
        try
        {
            s = scanner.next();
            dwRet = Long.parseUnsignedLong(s, fHex ? 16 : 10);
        }
        catch (Exception e)
        {
            System.out.printf("Input cancelled / invalid\n");
            return new WDCResultLong(DIAG_INPUT_FAIL, -1);
        }

        if (fCheckRange)
        {
            if (dwRet < min || dwRet > max)
            {
                System.out.printf("Invalid input: Input must be between ");
                if (fHex)
                    System.out.printf("0x%x and 0x%x\n", min, max);
                else
                    System.out.printf("%d and %d\n", min, max);
                return new WDCResultLong(DIAG_INPUT_FAIL, -1);
            }
        }

        return new WDCResultLong(DIAG_INPUT_SUCCESS, dwRet);
    }

    private final static char[] hexArray = "0123456789ABCDEF".toCharArray();
    public static String bytesToHex(byte[] bytes)
    {
        char[] hexChars = new char[bytes.length * 2];
        for ( int j = 0; j < bytes.length; j++ ) {

            int v = bytes[j] & 0xFF;
            hexChars[j * 2] = hexArray[v >>> 4];
            hexChars[j * 2 + 1] = hexArray[v & 0x0F];
        }
        return new String(hexChars) + "\n";
    }

    /*
     * Function: get_cur_time() Retrieves the current time, in OS units
     * Parameters: time [out] pointer to the OS time, in OS units Return Value:
     * true if successful, false otherwise
     */
    public static long get_cur_time()
    {
        return System.currentTimeMillis();
    }

    /*
     * Function: time_diff() Calculates a time difference in milliseconds
     * Parameters: end [in] End time, in OS units start [in] Start time, in OS
     * units Return Value: The time difference, in milliseconds.
     */
    private static long time_diff(final long end, final long start)
    {
        return end - start;
    }

    public static void DIAG_PrintPerformance(long size, long startTime)
    {
        long endTime = get_cur_time();
        long perf_time_total = time_diff(endTime, startTime);
        if (perf_time_total != -1)
        {
            System.out.printf("Transferred %d bytes, elapsed time %d[ms], " +
                "rate %d[MB/sec]\n", size, perf_time_total,
                /* (bytes / msec) * sec / MB */
                (size / (perf_time_total + 1)) * 1000 / (1024 * 1024));
        }
    }
}


