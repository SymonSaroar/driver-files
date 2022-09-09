/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/*
 * File - windrvr.c
 *
 * WinDriver driver name
 */

#if defined(__KERNEL__)
    #include "kpstdlib.h"
#else
    #include <stdio.h>
#endif
#include <windrvr.h>

#if defined(WIN32)
    #define snprintf _snprintf
/* Defines and includes related to SecureBoot checking */
    #define REGBUFSIZE 32
    #define SECBOOT_REG_PATH "SYSTEM\\CurrentControlSet\\Control" \
                "\\SecureBoot\\State"
    #define SECBOOT_REG_NAME "UEFISecureBootEnabled"
#else
    #include <sys/utsname.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
const char* DLLCALLCONV WD_DriverName(const char *sName)
{
    static const char *sDriverName = WD_DEFAULT_DRIVER_NAME;
    static char sTmpName[WD_MAX_DRIVER_NAME_LENGTH];

    if (!sName)
        return sDriverName;

    BZERO(sTmpName);
    snprintf(sTmpName, sizeof(sTmpName), "%s%s", WD_DRIVER_NAME_PREFIX,
        sName);

    /* Driver name can only be set once */
    if (strcmp(sDriverName, WD_DEFAULT_DRIVER_NAME) &&
        strcmp(sDriverName, sTmpName))
    {
        return NULL;
    }

    sDriverName = sTmpName;

    return sDriverName;
}

#if !defined(__KERNEL__)

#ifdef WIN32
static DWORD OSGetInfo(WD_OS_INFO *OS_info)
{
    HKEY key;
    DWORD dwStatus = WD_STATUS_SUCCESS;
    DWORD bufSize;
    DWORD dwBuildNumber = 0;
     
    /* Get registry key */
    dwStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE,
        &key);
    if (dwStatus != WD_STATUS_SUCCESS)
        goto Exit;

    /* Get product name string */
    bufSize = REGKEY_BUFSIZE;
    dwStatus = RegQueryValueEx(key, "ProductName", NULL, NULL,
        (PBYTE)&OS_info->cProdName, &bufSize);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        strncpy(OS_info->cProdName, OS_CAN_NOT_DETECT_TEXT,
            sizeof(OS_CAN_NOT_DETECT_TEXT));
    }

    /* Get installation type string */
    bufSize = REGKEY_BUFSIZE;
    dwStatus = RegQueryValueEx(key, "InstallationType",
        NULL, NULL, (PBYTE)&OS_info->cInstallationType, &bufSize);
    if (dwStatus != WD_STATUS_SUCCESS)
    {
        strncpy(OS_info->cInstallationType, INSTALLATION_TYPE_NOT_DETECT_TEXT,
             sizeof(INSTALLATION_TYPE_NOT_DETECT_TEXT));
    }

    /* Get CSD version string */
    bufSize = REGKEY_BUFSIZE;
    if (RegQueryValueEx(key, "CSDVersion", NULL, NULL,
        (PBYTE)&OS_info->cCsdVersion, &bufSize))
    {
        OS_info->cCsdVersion[0] = '\0';
    }

    /* Get current build string */
    bufSize = REGKEY_BUFSIZE;
    if (RegQueryValueEx(key, "CurrentBuild", NULL, NULL,
        (PBYTE)&OS_info->cBuild, &bufSize))
    {
        OS_info->cBuild[0] = '\0';
    }

    dwBuildNumber = strtol(OS_info->cBuild, NULL, 10);
    if (dwBuildNumber >= 21996 && !strstr(OS_info->cProdName, "11"))
    {
        strncat(OS_info->cProdName,
            " [WinDriver has detected that your OS version is Windows 11]",
            REGKEY_BUFSIZE);
    }

    /* Get current version string */
    bufSize = REGKEY_BUFSIZE;
    if (RegQueryValueEx(key, "CurrentVersion", NULL, NULL,
		(PBYTE)&OS_info->cCurrentVersion, &bufSize))
		{
			OS_info->cCurrentVersion[0] = '\0';
		}

    /* Get current major version DWORD */
    bufSize = sizeof(DWORD);
	if (RegQueryValueEx(key, "CurrentMajorVersionNumber",
	    NULL, NULL, (PBYTE)&OS_info->dwMajorVersion, &bufSize))
	{
		OS_info->dwMajorVersion = 0;
		goto Exit;
	}

    /* Get current minor version DWORD */
    bufSize = sizeof(DWORD);
	if (RegQueryValueEx(key, "CurrentMinorVersionNumber", NULL, NULL,
		(PBYTE)&OS_info->dwMinorVersion, &bufSize))
		{
			OS_info->dwMinorVersion = 0;
		}
Exit:
    RegCloseKey(key);
    return dwStatus;
}
#endif

WD_OS_INFO DLLCALLCONV get_os_type(void)
{
    static WD_OS_INFO OS_info;
    static BOOL OS_set = FALSE;

    if (OS_set)
        goto Exit;
    else
        BZERO(OS_info);

#if defined(WIN32)
    #if !defined (ARM)
        if (OSGetInfo(&OS_info))
            OS_set = FALSE;
        else
            OS_set = TRUE;
        goto Exit;
    #else
        strncpy(OS_info.cProdName, "Windows 10 IoT ARM",
            sizeof("Windows 10 IoT ARM"));
    #endif
#elif defined(UNIX)
    struct utsname unameInfo;
    BZERO(unameInfo);

    if (uname(&unameInfo) < 0)
    {
        strncpy(OS_info.cProdName, "Unix", sizeof(OS_info.cProdName));
        OS_set = FALSE;
    }
    else
    {
        strncpy(OS_info.cProdName, unameInfo.sysname,
            MIN(sizeof(unameInfo.sysname), REGKEY_BUFSIZE - 1));
        strncpy(OS_info.cRelease, unameInfo.release,
            MIN(sizeof(unameInfo.release), REGKEY_BUFSIZE - 1));
        strncpy(OS_info.cReleaseVersion, unameInfo.version,
            MIN(sizeof(unameInfo.version), REGKEY_BUFSIZE - 1));
        strncpy(OS_info.cInstallationType, INSTALLATION_TYPE_NOT_DETECT_TEXT,
            sizeof(OS_info.cInstallationType));

        OS_set = TRUE;
    }
#else
    strncpy(OS_info.cProdName, OS_CAN_NOT_DETECT_TEXT,
        sizeof(OS_CAN_NOT_DETECT_TEXT));
    strncpy(OS_info.cInstallationType, INSTALLATION_TYPE_NOT_DETECT_TEXT,
        sizeof(INSTALLATION_TYPE_NOT_DETECT_TEXT));
#endif

Exit:
    return OS_info;
}

/*  If Secure boot is supported it can either be enabled (dwVal = 1)
    or disabled (dwVal = 0). If it's not supported the SECBOOT_REG_PATH
    is not a valid path, hence RegOpenKeyEx fails */
DWORD DLLCALLCONV check_secureBoot_enabled(void)
{
#if defined(WIN32) && !defined (ARM)
    HKEY key;
    DWORD dwDataType;
    DWORD dwStatus;
    DWORD dwSize = REGBUFSIZE;
    DWORD dwVal;

    dwStatus = (DWORD)RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        TEXT(SECBOOT_REG_PATH), 0,KEY_QUERY_VALUE, &key);

    if (dwStatus == ERROR_SUCCESS)
    {
        dwStatus = (DWORD)RegQueryValueEx(key, TEXT(SECBOOT_REG_NAME), NULL,
            &dwDataType, (PBYTE)&dwVal, &dwSize);

        if (dwVal != 1)
            dwStatus = WD_WINDRIVER_STATUS_ERROR;
    }

    RegCloseKey(key);
    return dwStatus;
#else
    return WD_NOT_IMPLEMENTED;
#endif
}

#if defined(APPLE)
#include <mach/kern_return.h>
DWORD WD_FUNCTION_LOCAL(int wFuncNum, HANDLE h,
    PVOID pParam, DWORD dwSize, BOOL fWait)
{
    kern_return_t rc;
    io_connect_t data_port = (io_connect_t)(uintptr_t)h;
    UINT64 scalar[1];
    UINT64 status[1];
    uint32_t out_status = 1;
    size_t outSize = dwSize;
    DWORD wd_rc = WD_INVALID_PARAMETER;

    UNUSED_VAR(fWait);

    if (data_port == (io_connect_t)(uintptr_t)INVALID_HANDLE_VALUE)
        return WD_STATUS_INVALID_WD_HANDLE;

    scalar[0] = (UINT64)wFuncNum;
    rc = IOConnectCallMethod(data_port,
        kWinDriverMethodSyncIoctl,
        scalar, /* Scalars input */
        1, /* Number of scalar input array */
        pParam, /* Input structure */
        (size_t)dwSize, /* Size of input structure */
        status, /* Pointer to size scalar output array */
        &out_status, /* Number of scalar output array */
        pParam, /* Output structure */
        &outSize /* Pointer to size of output structure */
        );
    if (!rc)
        wd_rc = (DWORD)status[0];

    return wd_rc;
}

HANDLE WD_OpenLocal()
{
    kern_return_t rc;
    mach_port_t master_port;
    io_service_t service_obj = IO_OBJECT_NULL;
    io_iterator_t iterator = IO_OBJECT_NULL;
    CFDictionaryRef class_to_match;
    io_object_t data_port = IO_OBJECT_NULL;
    uint32_t app_type;

    /* Get the mach port used to initiate communication with IOKit. */
    rc = IOMasterPort(MACH_PORT_NULL, &master_port);
    if (rc != KERN_SUCCESS)
    {
        printf("%s: IOMasterPort failed, rc 0x%x\n", __FUNCTION__, rc);
        goto Exit;
    }

    class_to_match = IOServiceMatching("WinDriver");
    if (class_to_match == NULL)
    {
        printf("%s: IOServiceMatching failed\n", __FUNCTION__);
        goto Exit;
    }

    /* Creates an io_iterator_t of all instances of our drivers class
     * that exist in the IORegistry. */
    rc = IOServiceGetMatchingServices(master_port, class_to_match,
        &iterator);
    if (rc != KERN_SUCCESS)
    {
        printf("%s: IOServiceGetMatchingServices failed, rc 0x%x\n",
            __FUNCTION__, rc);
        goto Exit;
    }

    /* Find service object and release the iterator */
    service_obj = IOIteratorNext(iterator);
    if (!service_obj)
    {
        printf("%s: IOIteratorNext failed, unable to find WinDriver kernel"
            " extension\n", __FUNCTION__);
        goto Exit;
    }

    app_type = MAC_UC_MAGIC_NUM | MAC_UC_64BIT_APP;

    /* Obtain a handle to found service object */
    rc = IOServiceOpen(service_obj, mach_task_self(), app_type, &data_port);
    if (data_port == IO_OBJECT_NULL)
    {
        printf("%s: Failed to retrieve data port, rc 0x%x\n",
            __FUNCTION__, rc);
    }
Exit:
    if (service_obj != IO_OBJECT_NULL)
        IOObjectRelease(service_obj);
    if (iterator != IO_OBJECT_NULL)
        IOObjectRelease(iterator);

    return (HANDLE)(UINT64)data_port;
}

void WD_CloseLocal(HANDLE h)
{
    io_object_t data_port = (io_object_t)(uintptr_t)h;
    IOServiceClose(data_port);
}
#endif


#ifdef __cplusplus
}
#endif
#endif /* !defined(__KERNEL__) */

