cmake_minimum_required(VERSION 3.0)

project(pci_lib_netcore)
include(../../../../../include/wd.cmake)

set(SRCS
    ../AssemblyInfo.cs
    ../PCI_Device.cs
    ../PCI_DeviceList.cs
    ../dma.cs
    ../log.cs
    ../PCI_Regs.cs)


add_custom_target(pci_lib_netcore ALL
	COMMAND dotnet build -property:Configuration=${CMAKE_BUILD_TYPE}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	SOURCES ${SRCS}
	COMMENT "Building Pci-Lib with .NET CORE 5"
)
