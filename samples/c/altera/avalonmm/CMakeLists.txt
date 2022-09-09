cmake_minimum_required(VERSION 3.0)

project(avalonmm_diag C)
include(../../../../include/wd.cmake)
include_directories(
    ../../shared
    ../../../../include
    )

set (SRCS
    avalonmm_diag.c
    avalonmm_lib.c
    avalonmm_lib.h
    avalonmm_internal.h)

add_executable(avalonmm_diag ${SRCS} ${SAMPLE_SHARED_SRCS})
target_link_libraries(avalonmm_diag ${WDAPI_LIB})
set_target_properties(avalonmm_diag PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ARCH}/")
add_compile_definitions(HAS_INTS)
