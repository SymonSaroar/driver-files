cmake_minimum_required(VERSION 3.0)

project(p6466_diag C)
include(../../../../include/wd.cmake)
include_directories(
    ../../shared
    ../../../../include
    )

set(SRCS
    p6466_diag.c lib_6466/plx_lib_6466.c
    ../diag_lib/plx_diag_lib.c ../lib/plx_lib.c
    lib_6466/plx_regs_6466.h ../diag_lib/plx_diag_lib.h
    lib_6466/plx_lib_6466.h ../lib/plx_lib.h
    )

add_executable(p6466_diag ${SRCS} ${SAMPLE_SHARED_SRCS})
target_link_libraries(p6466_diag ${WDAPI_LIB})
if(${ARCH} STREQUAL "LINUX")
    target_link_libraries(p6466_diag m)
endif()
set_target_properties(p6466_diag PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ARCH}/")
