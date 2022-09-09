cmake_minimum_required(VERSION 3.0)

project(p9054_diag C)
include(../../../../include/wd.cmake)
include_directories(
    ../../shared
    ../../../../include
    )

set(SRCS
    p9054_diag.c
    )

add_executable(p9054_diag ${SRCS} ../diag_lib/plx_diag_lib.c
    ../lib/plx_lib.c ${SAMPLE_SHARED_SRCS})
target_compile_definitions(p9054_diag PRIVATE P9054_LITE_BOARD)
target_link_libraries (p9054_diag ${WDAPI_LIB})
if(${ARCH} STREQUAL "LINUX")
    target_link_libraries(p9054_diag m)
endif()
set_target_properties(p9054_diag PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ARCH}/")

