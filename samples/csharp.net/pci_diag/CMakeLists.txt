cmake_minimum_required(VERSION 3.0)

project(pci_diag_console_netcore)
include(${CMAKE_SOURCE_DIR}/include/wd.cmake)

if(APPLE)
    set(BUILD_FLAGS -r osx-x64 -o bin/${CMAKE_BUILD_TYPE}/net5.0)
endif()

set(SRCS
    pci_diag.cs
    ../shared/diag_lib.cs
    ../shared/wdc_diag_lib.cs
    ../shared/wds_diag_lib.cs
    ../shared/pci_lib.cs)

add_custom_target(pci_diag_console_netcore ALL
    COMMAND dotnet build -property:Configuration=${CMAKE_BUILD_TYPE} 
        ${BUILD_FLAGS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    SOURCES ${SRCS}
    COMMENT "Building PCI diag console with .NET CORE"
    )
