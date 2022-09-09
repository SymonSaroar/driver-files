cmake_minimum_required(VERSION 3.2)

project(usb_diag C)
include(../../../include/wd.cmake)
include_directories(
    ../shared
    ../../../include
    )

set(SRCS
    usb_diag.c
    ../shared/usb_diag_lib.c
    )
add_executable(usb_diag ${SRCS} ${SAMPLE_SHARED_SRCS})
target_compile_definitions(usb_diag PRIVATE USB_DIAG_SAMPLE)
target_link_libraries(usb_diag ${WDAPI_LIB})
set_target_properties(usb_diag PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ARCH}/")
