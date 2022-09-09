cmake_minimum_required(VERSION 3.0)

# Project's name
project(bmd_diag C)
include(../../../../include/wd.cmake)
include_directories(
    ../../shared
    ../../../../include
    )

# xilinx/bmd_diag
set (SRCS
    bmd_diag.c
    bmd_lib.c
    bmd_lib.h)

add_executable(bmd_diag ${SRCS} ${SAMPLE_SHARED_SRCS})
target_link_libraries(bmd_diag ${WDAPI_LIB})
set_target_properties(bmd_diag PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ARCH}/")
add_compile_definitions(HAS_INTS)
