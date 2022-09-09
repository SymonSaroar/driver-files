if(NOT WIN32 AND NOT APPLE AND
   ("${USER_BITS}" STREQUAL "" OR "${PLATFORM}" STREQUAL ""))
        execute_process(
            COMMAND uname -m
            COMMAND tr -d '\n'
            OUTPUT_VARIABLE UNAMEM
            )

        if ("${UNAMEM}" STREQUAL "x86")
            set(USER_BITS 32)
            set(PLATFORM "x86")
        elseif ("${UNAMEM}" STREQUAL "armv7l")
            set(USER_BITS 32)
            set(PLATFORM "ARM")
        elseif ("${UNAMEM}" STREQUAL "arm64" OR
                "${UNAMEM}" STREQUAL "aarch64")
            set(USER_BITS 64)
            set(PLATFORM "ARM64")
        else() # x86_64 is the default
            set(USER_BITS 64)
            set(PLATFORM "x86_64")
        endif()
endif()

file(RELATIVE_PATH PROJECT_DIR ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

# VERSION DEFINES

file(STRINGS ${CMAKE_CURRENT_LIST_DIR}/wd_ver.h wd_ver)
string(REGEX MATCH "WD_MAJOR_VER ([0-9]*)" _ ${wd_ver})
set(WD_MAJOR_VER ${CMAKE_MATCH_1})
string(REGEX MATCH "WD_MINOR_VER ([0-9]*)" _ ${wd_ver})
set(WD_MINOR_VER ${CMAKE_MATCH_1})
string(REGEX MATCH "WD_SUB_MINOR_VER ([0-9]*)" _ ${wd_ver})
set(WD_SUB_MINOR_VER ${CMAKE_MATCH_1})
string(CONCAT WD_VERSION "${WD_MAJOR_VER}${WD_MINOR_VER}${WD_SUB_MINOR_VER}")
string(CONCAT WD_VERSION_DOTS "${WD_MAJOR_VER}.${WD_MINOR_VER}.${WD_SUB_MINOR_VER}")

# Sets WD_BASEDIR to the parent folder of this file. DO NOT change this
set(WD_BASEDIR ${CMAKE_CURRENT_LIST_DIR}/..)
# Populate driver name
set(DRIVER_NAME windrvr${WD_VERSION})
set(SERVICE_NAME WinDriver${WD_VERSION})
set(WDHWID *WINDRVR${WD_VERSION})

# SHARED DEPENDENCIES
find_package(Threads)

set (SAMPLE_SHARED_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/../samples/c/shared/wdc_diag_lib.c
    ${CMAKE_CURRENT_LIST_DIR}/../samples/c/shared/diag_lib.c
    ${CMAKE_CURRENT_LIST_DIR}/../samples/c/shared/pci_menus_common.c)

# ENVIRONMENT SPECIFIC DEFINES
# 64 application or 32 on 64 application
if ("${USER_BITS}" STREQUAL "64" OR DEFINED USER_32_ON_64)
    set(PLAT_DIR "amd64")
else()
    set(PLAT_DIR "x86")
    #add_definitions("-fno-pie")
endif()
message (STATUS "Compiling for ${USER_BITS}-bit (${PROJECT_DIR})")

if (UNIX)
    add_definitions("-DUNIX")

    add_definitions(-O2)
    add_definitions("-Wno-unused-result -Wno-write-strings ")
    if (NOT APPLE)
        message (STATUS "Compiling for LINUX")
        set(ARCH LINUX)
        set(OS LN)
        if ("${PLATFORM}" STREQUAL "ARM")
            set(KERNEL_BITS 32)
        elseif("${PLATFORM}" STREQUAL "ARM64")
            set(KERNEL_BITS 64)
        elseif("${PLATFORM}" STREQUAL "i386")
            set(KERNEL_BITS 32)
        else()
            set(KERNEL_BITS 64)
        endif()

        set(KERNEL_FLAGS "-D__KERNEL__ -DMODULE -x c -nostdinc
        -Wall -Werror -Wcast-align -Wstrict-prototypes -Wno-trigraphs
        -Wno-sign-compare
        -iwithprefix include -pipe
        -fno-strict-aliasing -fno-common -fno-omit-frame-pointer
        -fno-reorder-blocks -fno-asynchronous-unwind-tables
        -fno-pie -fno-stack-protector")

        if ("${PLATFORM}" STREQUAL "x86_64")
            set(KERNEL_FLAGS "${KERNEL_FLAGS} -march=k8 -mno-red-zone
                -mcmodel=kernel -funit-at-a-time")
        elseif("${PLATFORM}" STREQUAL "i386")
            set(KERNEL_FLAGS "${KERNEL_FLAGS} -mpreferred-stack-boundary=2
                -mregparm=3")
        endif()

        if ("${PLATFORM}" STREQUAL "x86_64")
            #set(ARCH_KERNEL_LFLAG -melf_x86_64)
        else()
            set(ARCH_LFLAG -fno-pie)
        endif()

        set (CMAKE_SHARED_LINKER_FLAGS "${ARCH_LFLAG}")
        if ("${USER_BITS}" STREQUAL "32" AND "${KERNEL_BITS}" STREQUAL "64")
            set(WDAPI_LIB "wdapi${WD_VERSION}_32")
        else()
            set(WDAPI_LIB "wdapi${WD_VERSION}")
        endif()
    elseif (APPLE)
        message("Compiling for APPLE (${CMAKE_HOST_SYSTEM_PROCESSOR})")
        add_definitions("-DAPPLE")
        set(ARCH APPLE)
        set(USER_BITS 64)
        set(KERNEL_BITS 64)
        set(OS "MAC")

        if ("${CMAKE_HOST_SYSTEM_PROCESSOR}" STREQUAL "arm64")
            set(CMAKE_OSX_ARCHITECTURES "arm64"
                CACHE STRING "Build architectures for Mac OS X" FORCE)
	    include_directories(/System/Volumes/Data/Library/Developer/CommandLineTools/SDKs/MacOSX11.3.sdk/System/Library/Frameworks/DriverKit.framework/Versions/A/Headers/)
            set(PLATFORM "ARM64")
            add_definitions("-DTARGET_OS_OSX -DARM64")
        else()
            set(PLATFORM "x86_64")
            set(CMAKE_OSX_SYSROOT /Users/Shared/MacOSX10.14.sdk/)
include_directories(/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.14.sdk/System/Library/Frameworks/IOKit.framework/Headers)
            add_definitions("-Dx86")
        endif()

        find_library(WDAPI_LIB wdapi${WD_VERSION} HINTS /usr/local/lib)
        if (${WDAPI_LIB} MATCHES -NOTFOUND)
            set(WDAPI_LIB "wdapi${WD_VERSION}")
        endif()
endif()

    # CLEANUP
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${ARCH}/*")
elseif (WIN32)
    message (STATUS "Compiling for WINDOWS")

    if ("${CMAKE_GENERATOR}" STREQUAL "MinGW Makefiles")
        set(PLATFORM "x86_64")
        message ("Compiling with MinGW")
    else()
        add_definitions(/W3 /wd26498 /wd26812 /std:c++14 /MP)
    endif()
    
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DWINNT)
    set(WDAPI_LIB wdapi${WD_VERSION})
    if ("${USER_BITS}" STREQUAL "64" OR DEFINED USER_32_ON_64)
	    set(KERNEL_BITS 64)
	else()
		set(KERNEL_BITS 32)
	endif()

    set(OS "")
    set(ARCH WIN32)

    if ("${USER_BITS}" STREQUAL "32" AND "${KERNEL_BITS}" STREQUAL "64")
        set(WDAPI_LIB
            "${WD_BASEDIR}/lib/${PLAT_DIR}/x86/wdapi${WD_VERSION}_32.lib")
    else()
	    set(WDAPI_LIB "${WD_BASEDIR}/lib/${PLAT_DIR}/wdapi${WD_VERSION}.lib")
    endif()

    if(${KERNEL_BITS} STREQUAL 64)
        set(WIN_PLATFORM_DIR "WINNT.x86_64")
    else()
        set(WIN_PLATFORM_DIR "WINNT.i386")
    endif()
endif()

message(STATUS "${KERNEL_BITS} Bit Kernel Detected")
if ("${PLATFORM}" STREQUAL "ARM")
    message("ARM compilation")
elseif("${PLATFORM}" STREQUAL "ARM64")
    add_definitions(-DKERNEL_64BIT)
    message("ARM64 compilation")
elseif("${PLATFORM}" STREQUAL "x86_64")
    add_definitions(-DKERNEL_64BIT -Dx86_64)
    message("x86_64 compilation")
else()
    add_definitions(-Dx86 -Di386)
	# 32 on 64
	if (DEFINED USER_32_ON_64)
		 add_definitions(-DKERNEL_64BIT)
	endif()
    message("x86 compilation")
endif()

add_definitions(-D${ARCH} -DWD_DRIVER_NAME_CHANGE)
