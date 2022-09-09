cmake_minimum_required(VERSION 3.0)

project(lscdma_diag C)
include(../../../include/wd.cmake)
include_directories(
    ../shared
    ../../../include
    )

set(SRCS
    lscdma_diag.c
    lscdma_lib.c
    lscdma_lib.h
    )
add_executable(lscdma_diag ${SRCS} ${SAMPLE_SHARED_SRCS})
target_link_libraries(lscdma_diag ${WDAPI_LIB})
set_target_properties(lscdma_diag PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ARCH}/")
