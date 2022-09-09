cmake_minimum_required(VERSION 3.0)

project(p9080_diag C)
include(../../../../include/wd.cmake)
include_directories(
    ../../shared
    ../../../../include
    )

set(SRCS
    p9080_diag.c
    )

add_executable(p9080_diag ${SRCS} ../diag_lib/plx_diag_lib.c
    ../lib/plx_lib.c ${SAMPLE_SHARED_SRCS})
target_link_libraries(p9080_diag ${WDAPI_LIB})
if(${ARCH} STREQUAL "LINUX")
    target_link_libraries(p9080_diag m)
endif()
set_target_properties(p9080_diag PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ARCH}/")
