cmake_minimum_required(VERSION 3.0)

project(pci_scan C)
include(../../../include/wd.cmake)
include_directories(
    ../shared
    ../../../include
    )

set(SRCS pci_scan.c)
add_executable(pci_scan ${SRCS} ${SAMPLE_SHARED_SRCS})
target_link_libraries(pci_scan ${WDAPI_LIB})
set_target_properties(pci_scan PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ARCH}/")
