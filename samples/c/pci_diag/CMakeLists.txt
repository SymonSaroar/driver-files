cmake_minimum_required(VERSION 3.0)

# Project's name
project(pci_diag C)
include(../../../include/wd.cmake)
include_directories(
    ../../../include
    ../shared
    )

# samples/pci_diag
set(SRCS
    pci_diag.c
    pci_lib.c
    pci_lib.h)

add_definitions(-DHAS_INTS)
add_executable(pci_diag ${SRCS} ../shared/wds_diag_lib.c
    ${SAMPLE_SHARED_SRCS})
target_link_libraries(pci_diag ${WDAPI_LIB})
set_target_properties(pci_diag PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ARCH}/")
