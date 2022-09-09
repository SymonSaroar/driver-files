cmake_minimum_required(VERSION 3.0)

project(i8253 C)
include(../../../include/wd.cmake)
include_directories(
    ../../..
    ../../../include
    )

set(SRCS
    i8253.c
    )
add_executable(i8253 ${SRCS})
target_link_libraries(i8253 ${WDAPI_LIB})
set_target_properties(i8253 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ARCH}/")

