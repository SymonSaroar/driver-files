cmake_minimum_required(VERSION 3.0)

project(wddebug C)
include(../../../include/wd.cmake)
include_directories(
    ../../../include
    )

# samples/wddebug
set(SRCS
    wddebug.c
    )
add_executable(wddebug ../../../src/wdapi/windrvr.c ${SRCS})
if(APPLE)
    target_link_libraries(wddebug "-framework IOKit")
endif()
set_target_properties(wddebug PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${ARCH}/")
