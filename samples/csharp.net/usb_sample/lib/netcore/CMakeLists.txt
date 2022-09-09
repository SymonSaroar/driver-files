cmake_minimum_required(VERSION 3.0)

project(usb_lib_netcore)
include(../../../../../include/wd.cmake)

set(SRCS
    ../AssemblyInfo.cs
    ../UsbDevice.cs
    ../UsbDeviceManager.cs)

    add_custom_target(usb_lib_netcore ALL
        COMMAND dotnet build -property:Configuration=${CMAKE_BUILD_TYPE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        SOURCES ${SRCS}
        COMMENT "Building Usb-Lib with .NET CORE 5"
      )
