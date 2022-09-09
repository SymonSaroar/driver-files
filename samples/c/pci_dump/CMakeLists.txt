cmake_minimum_required(VERSION 3.0)

project(pci_dump C)
include(../../../include/wd.cmake)
include_directories(
    ../../../include
    ../shared
    )

set(SRCS
    pci_dump.c)
add_executable(pci_dump ${SRCS} ${SAMPLE_SHARED_SRCS})
target_link_libraries(pci_dump ${WDAPI_LIB})
set_target_properties(pci_dump PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ARCH}")
