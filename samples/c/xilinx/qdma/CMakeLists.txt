cmake_minimum_required(VERSION 3.0)

project(qdma_diag C)
include(../../../../include/wd.cmake)
include_directories(
    ../../shared
    ../../../../include
    )

set (SRCS
    qdma_diag.c
    qdma_lib.c
    qdma_lib.h
    qdma_internal.h)

add_executable(qdma_diag ${SRCS} ${SAMPLE_SHARED_SRCS})
target_link_libraries(qdma_diag ${WDAPI_LIB})
set_target_properties(qdma_diag PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ARCH}/")

