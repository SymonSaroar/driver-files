cmake_minimum_required(VERSION 3.0)

project(qsys_diag C)
include(../../../../include/wd.cmake)
include_directories(
    ../../shared
    ../../../../include
    )

set(SRCS
    qsys_diag.c
    qsys_lib.c
    qsys_lib.h
    )

add_executable(qsys_diag ${SRCS} ${SAMPLE_SHARED_SRCS})
target_link_libraries(qsys_diag ${WDAPI_LIB})
set_target_properties(qsys_diag PROPERTIES RUNTIME_OUTPUT_DIRECTORY
    "${ARCH}/")
add_compile_definitions(HAS_INTS)
