cmake_minimum_required(VERSION 3.0)
project(wdapi_netcore)

include(../../include/wd.cmake)

set(SRCS
    pci_regs_netcore.cs
    utils_netcore.cs
    wdc_lib_netcore.cs
    wdu_lib_netcore.cs
    windrvr_netcore.cs
    )

 add_custom_target(wdapi_netcore${WD_VERSION} ALL
     COMMAND dotnet build -property:Configuration=${CMAKE_BUILD_TYPE}
     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
     SOURCES ${SRCS}
     COMMENT "Building WinDriver-API with .NET CORE 5"
)
