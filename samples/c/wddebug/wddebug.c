/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/*
 * File - WDDEBUG.C
 *
 * A utility that turns WinDriver's debug mode on and off.
 * Debug mode checks every I/O and memory transfer command,
 * making sure it fits in with the card's registered
 * resources. If an illegal command is given, WinDriver
 * will ignore the command and show a warning message on
 * screen. Debug mode slows down transfer operations,
 * therefore it should be used only in the development process.
 * Running this command without parameters will print the
 * version of WinDriver installed.
 *
 * If debug mode is set, this utility also enables you to print
 * out debug messages from the kernel, by the dump command.
 */

#include "windrvr.h"
#include <stdio.h>
#include <time.h>

#if defined(UNIX)
    #include <unistd.h>
    #include <fcntl.h>

    #define stricmp strcasecmp
#else
    #include <conio.h>
#endif

static const char *sDriverName = WD_PROD_NAME;
static FILE *file_h;

#if defined(UNIX)
    static void Sleep(int msec)
    {
        usleep(msec * 1000);
    }
#endif

static void Usage(void)
{
    fprintf(file_h, "WDDEBUG Debug Monitor: WinDriver debug messages logging "
        "utility.\n"
        "\n"
        "USAGE:\n"
        "       WDDEBUG\n"
        "       WDDEBUG [<driver_name>] <command> [<level>] [<sections>]\n"
        "       WDDEBUG [<driver_name>] <command> [<filename>]\n"
        "\n"
        "`WDDEBUG` (no arguments) "
        "displays this help message (<=> `WDDEBUG help`).\n"
        "\n"
        "<driver_name>: The name of the driver to which to apply the command, "
            "without\n"
            "              the driver file's extension. Default: "
            WD_DEFAULT_DRIVER_NAME_BASE ".\n"
        "\n"
        "<command>: The Debug Monitor command to execute:\n"
        "  on            - Set debugging mode on.\n"
        "  off           - Set debugging mode off.\n"
        "  dbg_on        - Log WinDriver debug messages to the OS kernel "
            "debugger, and\n"
        "                  set the debugging mode to ON.\n"
        "  dbg_off       - Stop Redirecting debug messages from the "
            "Debug Monitor to the\n"
        "                  OS kernel debugger.\n"
        "  dump          - Continuously send (\"dump\") debug messages to the "
            "command prompt.\n"
        "                  (The prompt will display instructions on how to "
            "stop the dump.)\n"
        "  status        - Display information regarding the running "
            "<driver_name>, and\n"
        "                  the current Debug Monitor status and settings.\n"
        "  clock_on      - Add a timestamp to each debug message.\n"
        "                  The timestamps are relative to the driver-load "
            "time,\n"
        "                  or to the time of the last clock_reset command.\n"
        "  clock_off     - Do not add timestamps to the debug messages.\n"
        "  clock_reset   - Reset the debug-messages timestamps clock.\n"
        "  sect_info_on  - Add section(s) information to each debug message.\n"
        "  sect_info_off - Do not add section(s) information to the debug "
            "messages.\n"
        "  help          - Display usage instructions.\n"
        "\n"
        "<filename>: Path to the file to save the log.\n"
        "            This argument is applicable with the 'dump' command.\n"
        "<level>: The debug trace level to set: ERROR, WARN, INFO, or TRACE "
            "(default).\n"
        "         This argument is applicable with the 'on' and 'dbg_on' "
            "commands.\n"
        "         Note: When <sections> is set, <level> must be set as well "
            "(no default).\n"
        "<sections>: The debug sections to monitor -- either ALL (default) to "
            "monitor all\n"
        "            sections, or a quoted string that contains a combination "
            "of any of the\n"
        "            following flags (separated by spaces):\n"
        "                CARD_REG, DMA, EVENT, IPC, KER_BUF, KER_DRV, "
            "KER_PLUG, IO, INT,\n"
        "                MEM, MISC, PCI, LICENSE, USB\n"
        "            This argument is applicable with the 'on' and 'dbg_on' "
            "commands.\n"
        "\n"
        "Examples:\n"
        "1. Turn on debug monitoring for the default driver ("
        WD_DEFAULT_DRIVER_NAME_BASE "), to monitor\n"
        "   PCI and DMA routines at trace level INFO; then start dumping "
            "debug messages:\n"
        "       WDDEBUG on INFO \"PCI DMA\"\n"
        "       WDDEBUG dump\n"
        "2. Redirect debug messages from your \"my_driver\" driver to the OS "
            "kernel\n"
        "   debugger, for ALL debug sections, at trace level TRACE:\n"
        "       WDDEBUG my_driver dbg_on TRACE ALL\n"
        "   Note: \"ALL\" can be omitted because this is the default "
            "<sections> flag.\n");
}

static BOOL LevelToDword(const char *sLevel, DWORD *pdwLevel)
{
    DWORD dwLevel;

    if (stricmp(sLevel, "ERROR") == 0)
        dwLevel = D_ERROR;
    else if (stricmp(sLevel, "WARN") == 0)
        dwLevel = D_WARN;
    else if (stricmp(sLevel, "INFO") == 0)
        dwLevel = D_INFO;
    else if (stricmp(sLevel, "TRACE") == 0)
        dwLevel = D_TRACE;
    else if (stricmp(sLevel, "OFF") == 0)
        dwLevel = D_OFF;
    else
        return FALSE;

    *pdwLevel = dwLevel;
    return TRUE;
}

static const char *LevelToString(DWORD dwLevel)
{
    if (dwLevel == D_OFF)
        return "OFF";
    if (dwLevel == D_ERROR)
        return "ERROR";
    if (dwLevel == D_WARN)
        return "WARN";
    if (dwLevel == D_INFO)
        return "INFO";
    if (dwLevel == D_TRACE)
        return "TRACE";
    return "";
}

static BOOL SectionToDword(const char *sSection, DWORD *pdwSection)
{
    char tokBuf[1024];
    char *tok;

    *pdwSection = 0;
    strcpy(tokBuf, sSection);

    for (tok = strtok(tokBuf, " "); tok; tok = strtok(NULL, " "))
    {
        if (stricmp(tok, "ALL") == 0)
            *pdwSection |= S_ALL;
        else if (stricmp(tok, "IO") == 0)
            *pdwSection |= S_IO;
        else if (stricmp(tok, "MEM") == 0)
            *pdwSection |= S_MEM;
        else if (stricmp(tok, "INT") == 0)
            *pdwSection |= S_INT;
        else if (stricmp(tok, "PCI") == 0)
            *pdwSection |= S_PCI;
        else if (stricmp(tok, "DMA") == 0)
            *pdwSection |= S_DMA;
        else if (stricmp(tok, "MISC") == 0)
            *pdwSection |= S_MISC;
        else if (stricmp(tok, "LICENSE") == 0)
            *pdwSection |= S_LICENSE;
        else if (stricmp(tok, "KER_PLUG") == 0)
            *pdwSection |= S_KER_PLUG;
        else if (stricmp(tok, "PNP") == 0)
            *pdwSection |= S_PNP;
        else if (stricmp(tok, "CARD_REG") == 0)
            *pdwSection |= S_CARD_REG;
        else if (stricmp(tok, "KER_DRV") == 0)
            *pdwSection |= S_KER_DRV;
        else if (stricmp(tok, "USB") == 0)
            *pdwSection |= S_USB;
        else if (stricmp(tok, "KER_PLUG") == 0)
            *pdwSection |= S_KER_PLUG;
        else if (stricmp(tok, "EVENT") == 0)
            *pdwSection |= S_EVENT;
        else if (stricmp(tok, "IPC") == 0)
            *pdwSection |= S_IPC;
        else if (stricmp(tok, "KER_BUF") == 0)
            *pdwSection |= S_KER_BUF;
        else if (tok[0] == '0' && toupper(tok[1]) == 'X')
        {
            DWORD dwSection;

            sscanf(tok + 2, "%x", &dwSection);
            *pdwSection |= dwSection;
        }
        else
        {
            return FALSE;
        }
    }

    return TRUE;
}

char *SectionToString(DWORD dwSection)
{
    static char sSection[1024];

    sSection[0] = '\0';
    if (dwSection == (DWORD)S_ALL)
    {
        strcat(sSection, "ALL ");
        return sSection;
    }
    if (dwSection & S_IO)
    {
        strcat(sSection, "IO ");
        dwSection &= ~S_IO;
    }
    if (dwSection & S_MEM)
    {
        strcat(sSection, "MEM ");
        dwSection &= ~S_MEM;
    }
    if (dwSection & S_INT)
    {
        strcat(sSection, "INT ");
        dwSection &= ~S_INT;
    }
    if (dwSection & S_PCI)
    {
        strcat(sSection, "PCI ");
        dwSection &= ~S_PCI;
    }
    if (dwSection & S_DMA)
    {
        strcat(sSection, "DMA ");
        dwSection &= ~S_DMA;
    }
    if (dwSection & S_KER_PLUG)
    {
        strcat(sSection, "KER_PLUG ");
        dwSection &= ~S_KER_PLUG;
    }
    if (dwSection & S_MISC)
    {
        strcat(sSection, "MISC ");
        dwSection &= ~S_MISC;
    }
    if (dwSection & S_LICENSE)
    {
        strcat(sSection, "LICENSE ");
        dwSection &= ~S_LICENSE;
    }
    if (dwSection & S_KER_PLUG)
    {
        strcat(sSection, "KER_PLUG ");
        dwSection &= ~S_KER_PLUG;
    }
    if (dwSection & S_CARD_REG)
    {
        strcat(sSection, "CARD_REG ");
        dwSection &= ~S_CARD_REG;
    }
    if (dwSection & S_KER_DRV)
    {
        strcat(sSection, "KER_DRV ");
        dwSection &= ~S_KER_DRV;
    }
    if (dwSection & S_EVENT)
    {
        strcat(sSection, "EVENT ");
        dwSection &= ~S_EVENT;
    }
    if (dwSection & S_PNP)
    {
        strcat(sSection, "PNP ");
        dwSection &= ~S_PNP;
    }
    if (dwSection & S_IPC)
    {
        strcat(sSection, "IPC ");
        dwSection &= ~S_IPC;
    }
    if (dwSection & S_KER_BUF)
    {
        strcat(sSection, "KER_BUF ");
        dwSection &= ~S_KER_BUF;
    }

    if (dwSection)
        sprintf(sSection + strlen(sSection), "0x%08x", dwSection);

    return sSection;
}

void Print_version(WD_VERSION *pVer)
{
    fprintf(file_h, "%s v%d.%02d installed (%s)\n", sDriverName,
        pVer->dwVer / 100, pVer->dwVer % 100, pVer->cVer);
}

void Print_status(HANDLE hWD)
{
    WD_DEBUG debug;

    BZERO(debug);
    debug.dwCmd = DEBUG_STATUS;
    WD_Debug(hWD, &debug);

    fprintf(file_h, "Debug level (%d) %s, Debug sections (0x%08x) %s, Buffer "
        "size %d\n", debug.dwLevel, LevelToString(debug.dwLevel),
        (UINT32)debug.dwSection, SectionToString(debug.dwSection),
        debug.dwBufferSize);
}

int main(int argc, char *argv[])
{
    WD_VERSION verBuf;
    HANDLE hWD;
    WD_DEBUG debug;
    BOOL dump_to_file = FALSE;
    WD_OS_INFO OS_info = get_os_type();
    file_h = stdout;
    char installationStringTemp[REGKEY_BUFSIZE + 20] = "";

    BZERO(debug);

    /* Check the <driver_name> option */
    if (argc > 2 &&
        stricmp(argv[1], "off") != 0 && stricmp(argv[1], "on") != 0 &&
        stricmp(argv[1], "dbg_on") != 0 && stricmp(argv[1], "dbg_off") != 0 &&
        stricmp(argv[1], "status") != 0 && stricmp(argv[1], "dump") != 0 &&
        stricmp(argv[1], "sect_info_on") != 0 &&
        stricmp(argv[1], "sect_info_off") != 0 &&
        stricmp(argv[1], "clock_on") != 0 && stricmp(argv[1], "clock_off") != 0
        && stricmp(argv[1], "clock_reset") != 0 && stricmp(argv[1], "log") != 0
        && stricmp(argv[1], "help") != 0)
    {
        sDriverName = argv[1];
        /* When using the <driver_name> option, shift argc and argv */
        argc--;
        argv++;

        /* Set Driver Name */
        if (!WD_DriverName(sDriverName))
        {
            fprintf(file_h, "Error: Cannot set driver name to %s.\n",
                sDriverName);
            return EXIT_FAILURE;
        }
    }

    hWD = WD_Open();
    if (hWD == INVALID_HANDLE_VALUE)
    {
        fprintf(file_h, "Error: %s device not installed.\n", sDriverName);
        return EXIT_FAILURE;
    }

    BZERO(verBuf);
    WD_Version(hWD, &verBuf);
    if (verBuf.dwVer < WD_VER)
    {
        Print_version(&verBuf);
        fprintf(file_h, "Please update the %s installed to v%d.%02d, "
            "or newer.\n", sDriverName, WD_VER / 100, WD_VER % 100);
        WD_Close(hWD);
        return EXIT_FAILURE;
    }

    if (argc < 2)
    {
        Print_version(&verBuf);
        fprintf(file_h, "\n");
        Usage();
        return EXIT_SUCCESS;
    }

    if (dump_to_file || !stricmp(argv[1], "dump"))
    {
        WD_DEBUG_DUMP debugDump;
        time_t ltime;

#if defined(UNIX)
        int stdin_fileno = STDIN_FILENO;
#endif

        if (argc > 3)
        {
            fprintf(file_h, "Too many arguments\n");
            Usage();
            return EXIT_FAILURE;
        }

        if (argc == 3)
        {
            file_h = fopen(argv[2], "w+");
            if (!file_h)
            {
                printf("Failed opening '%s' file for write\n", argv[2]);
                return EXIT_FAILURE;
            }
        }
        time(&ltime);

        fprintf(file_h, "WDDEBUG v%d.%02d Debug Monitor.\n", WD_VER / 100,
            WD_VER % 100);
        fprintf(file_h, "Running %s\n", verBuf.cVer);
        fprintf(file_h, "Time: %s", ctime(&ltime));

    if (strcmp(OS_info.cInstallationType, INSTALLATION_TYPE_NOT_DETECT_TEXT))
    {
        sprintf(installationStringTemp, " (%s installation)",
            OS_info.cInstallationType);
    }

    fprintf(file_h, "OS Name: %s%s\n", OS_info.cProdName,
        installationStringTemp);
#if defined(UNIX)
    if (OS_info.cReleaseVersion[0])
    {
        fprintf(file_h, "OS Release: %s %s\n", OS_info.cRelease,
            OS_info.cReleaseVersion);
    }
    printf("Press enter to exit\n");
#else
    if (OS_info.dwMajorVersion > 0) /* Windows 10 */
    {
        fprintf(file_h, "OS Version: %d.%d.%s Build %s\n",
            OS_info.dwMajorVersion, OS_info.dwMinorVersion,
            OS_info.cBuild, OS_info.cBuild);
    }
    else if (OS_info.cCurrentVersion[0]) /* Windows 8 */
    {
        fprintf(file_h, "OS Version: %s.%s %s Build %s\n",
            OS_info.cCurrentVersion, OS_info.cBuild, OS_info.cCsdVersion,
            OS_info.cBuild);
    }
    printf("Press ESC to exit\n");
#endif
        fprintf(file_h, "\n");
        fflush(file_h);

        BZERO(debug);
        debug.dwCmd = DEBUG_SET_FILTER;
        debug.dwLevel = D_TRACE;
        debug.dwSection = (DWORD)S_ALL;
        WD_Debug(hWD, &debug);

        BZERO(debugDump);
#if defined(UNIX)
        fcntl(stdin_fileno, F_SETFL, fcntl(stdin_fileno, F_GETFL, 0) |
            O_NONBLOCK);
#endif
        for (;;)
        {
#if defined(UNIX)
            char buf[4];
            int nRead = read(stdin_fileno, buf, 4);

            if (nRead > 0)
                break;
#elif defined(WIN32)
            if (_kbhit() && _getch() == 27) break;
#endif
            for (;;)
            {
                WD_DebugDump(hWD, &debugDump);

                if (!debugDump.cBuffer[0])
                    break;

                fprintf(file_h, "%s", debugDump.cBuffer);
                fflush(file_h);
            }

            Sleep(100);
        }
    }
    else if (stricmp(argv[1], "on") == 0 || stricmp(argv[1], "dbg_on") == 0)
    {
        debug.dwCmd = DEBUG_SET_FILTER;
        debug.dwLevel = D_TRACE;
        debug.dwSection = (DWORD)S_ALL;

        if (argc > 2)
        {
            if (argc > 4)
            {
                fprintf(file_h, "Too many arguments\n");
                Usage();
                return EXIT_FAILURE;
            }

            if (!LevelToDword(argv[2], &debug.dwLevel))
            {
                fprintf(file_h, "Invalid level name (%s)\n", argv[2]);
                Usage();
                return EXIT_FAILURE;
            }

            if (argc == 4 && !SectionToDword(argv[3], &debug.dwSection))
            {
                fprintf(file_h, "Invalid section name (%s)\n", argv[3]);
                Usage();
                return EXIT_FAILURE;
            }
        }

        if (argc > 2 || stricmp(argv[1], "on") == 0)
            WD_Debug(hWD, &debug);
        Print_status(hWD);

        if (stricmp(argv[1], "dbg_on") == 0)
        {
            debug.dwCmd = KERNEL_DEBUGGER_ON;
            WD_Debug(hWD, &debug);
            fprintf(file_h, "WinDriver's debug messages are directed to kernel "
                "debugger.\n");
            #ifdef WIN32
                fprintf(file_h, "\nIf this is the first time you enabled the "
                    "kernel debugger, restart the PC to see the logs.\n");
            #endif
        }
    }
    else if (stricmp(argv[1], "off") == 0)
    {
        if (argc > 2)
        {
            fprintf(file_h, "Too many arguments\n");
            Usage();
            return EXIT_FAILURE;
        }

        debug.dwCmd = DEBUG_SET_FILTER;
        debug.dwLevel = D_OFF;
        debug.dwSection = 0;
        WD_Debug(hWD, &debug);
    }
    else if (stricmp(argv[1], "status") == 0)
    {
        if (argc > 2)
        {
            fprintf(file_h, "Too many arguments\n");
            Usage();
            return EXIT_FAILURE;
        }

        Print_version(&verBuf);
        Print_status(hWD);
    }
    else if (stricmp(argv[1], "dbg_off") == 0)
    {
        debug.dwCmd = KERNEL_DEBUGGER_OFF;
        WD_Debug(hWD, &debug);
        fprintf(file_h, "Stop redirecting WinDriver debug messages to a kernel "
            "debugger.\n");
    }
    else if (!stricmp(argv[1], "help"))
    {
        Print_version(&verBuf);
        fprintf(file_h, "\n");
        Usage();
        return EXIT_SUCCESS;
    }
    else if (stricmp(argv[1], "sect_info_on") == 0)
    {
        if (argc > 2)
        {
            fprintf(file_h, "Too many arguments\n");
            Usage();
            return EXIT_FAILURE;
        }

        debug.dwCmd = DEBUG_DUMP_SEC_ON;
        WD_Debug(hWD, &debug);
        fprintf(file_h, "Specify WinDriver debug sections (IO/MEM/DMA/etc).\n");
    }
    else if (stricmp(argv[1], "sect_info_off") == 0)
    {
        if (argc > 2)
        {
            fprintf(file_h, "Too many arguments\n");
            Usage();
            return EXIT_FAILURE;
        }

        debug.dwCmd = DEBUG_DUMP_SEC_OFF;
        WD_Debug(hWD, &debug);
        fprintf(file_h, "Stop specifying WinDriver debug sections.\n");
    }
    else if (stricmp(argv[1], "clock_on") == 0)
    {
        if (argc > 2)
        {
            fprintf(file_h, "Too many arguments\n");
            Usage();
            return EXIT_FAILURE;
        }

        debug.dwCmd = DEBUG_DUMP_CLOCK_ON;
        WD_Debug(hWD, &debug);
        fprintf(file_h, "Enable timestamp on WinDriver debug messages.\n");
    }
    else if (stricmp(argv[1], "clock_off") == 0)
    {
        if (argc > 2)
        {
            fprintf(file_h, "Too many arguments\n");
            Usage();
            return EXIT_FAILURE;
        }

        debug.dwCmd = DEBUG_DUMP_CLOCK_OFF;
        WD_Debug(hWD, &debug);
        fprintf(file_h, "Disable timestamp on WinDriver debug messages.\n");
    }
    else if (stricmp(argv[1], "clock_reset") == 0)
    {
        if (argc > 2)
        {
            fprintf(file_h, "Too many arguments\n");
            Usage();
            return EXIT_FAILURE;
        }

        debug.dwCmd = DEBUG_CLOCK_RESET;
        WD_Debug(hWD, &debug);
        fprintf(file_h, "Reset timestamp clock.\n");
    }
    else if (argc > 1)
    {
        fprintf(file_h, "Invalid option (%s)\n", argv[1]);
        Usage();
    }

    if (file_h != stdout)
        fclose(file_h);

    WD_Close(hWD);
    return EXIT_SUCCESS;
}

