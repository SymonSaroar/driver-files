cmake_minimum_required(VERSION 3.0)

project(int_io C)
include(../../../include/wd.cmake)
include_directories(
    ../../..
    ../../../include
    )

set(SRCS int_io.c)
add_executable(int_io ${SRCS})
target_link_libraries(int_io ${WDAPI_LIB})
set_target_properties(int_io PROPERTIES RUNTIME_OUTPUT_DIRECTORY
    "${ARCH}/")
