cmake_minimum_required(VERSION 3.0)

project(wdapi C)
include(../../include/wd.cmake)
include_directories(
    ../..
    ../../include
    )

set(wdapi_SRCS
    status_strings.c
    windrvr_int_thread.c
    wdc_err.c
    wdc_err.h
    wdc_general.c
    wdc_cfg.c
    wdc_mem_io.c
    wdc_ints.c
    wds_ipc.c
    wdc_events.c
    windrvr.c
    windrvr_events.c
    utils.c
    wds_kerbuf.c
    wdc_sriov.c
    wdc_dma.c
    wd_log.c
    pci_strings.c
)

if (WIN32)
    add_library(wdapi${WD_VERSION} SHARED
        ${wdapi_SRCS}
        wdu_lib.c
        wdapi.def
        wdapi.rc)

    target_link_libraries(wdapi${WD_VERSION} ${CMAKE_THREAD_LIBS_INIT})
    set_target_properties(wdapi${WD_VERSION} PROPERTIES
        COMPILE_FLAGS "-DWD_DRIVER_NAME_CHANGE"
        OUTPUT_NAME wdapi${WD_VERSION}
        )
elseif (${ARCH} STREQUAL LINUX)
    add_library(wdapi${WD_VERSION} SHARED ${wdapi_SRCS} wdu_lib.c)
    target_link_libraries(wdapi${WD_VERSION} ${CMAKE_THREAD_LIBS_INIT})
    set_target_properties(wdapi${WD_VERSION} PROPERTIES
        COMPILE_FLAGS "-DWD_DRIVER_NAME_CHANGE"
        LIBRARY_OUTPUT_DIRECTORY "${ARCH}/")
endif()

