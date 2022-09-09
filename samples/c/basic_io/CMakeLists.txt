cmake_minimum_required(VERSION 3.0)

project(basic_io C)
include(../../../include/wd.cmake)
include_directories(
    ../../..
    ../../../include
    )

set(SRCS basic_io.c)
add_executable(basic_io ${SRCS})
target_link_libraries(basic_io ${WDAPI_LIB})
set_target_properties(basic_io PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ARCH}/")
