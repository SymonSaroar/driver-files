cmake_minimum_required(VERSION 3.0)

project(cmos C)
include(../../../include/wd.cmake)
include_directories(
    ../../..
    ../../../include
    )

set(SRCS
    cmos.c cmos_lib.c cmos_lib.h
    )
add_executable(cmos ${SRCS})
target_link_libraries(cmos ${WDAPI_LIB})
set_target_properties(cmos PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ARCH}/")
