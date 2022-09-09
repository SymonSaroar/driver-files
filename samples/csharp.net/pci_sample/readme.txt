C# PCI Sample
==============

This file contains:
1. An overview of the contents of the csharp.net\pci_sample\ directory.
2. List of pci_sample/ files.
3. List of kp_pci/ Kernel PlugIn files.
4. Instructions for using the sample application.
5. Instructions for building the PCI library.
6. Instructions for building the PCI sample code.
7. Instructions for building the kp_pci Kernel PlugIn driver.
8. Instructions for installing the kp_pci Kernel PlugIn driver.


1. Overview
   =========
   This code sample is provided AS-IS and as a guiding sample only.

   The csharp.net\pci_sample\ directory contains:
   - A C# .NET PCI library - pci_lib.dll.
   - A graphical (GUI) C# .NET PCI sample diagnostics application -
     pci_sample.exe.
   - The source code of the C# PCI library and diagnostics application.

   The code was written by Jungo Connectivity using WinDriver's PCI API.

   General Overview
   -----------------
   The library and sample application support handling of multiple PCI devices
   and simultaneous execution of several tasks on different devices.
   It demonstrates how to use WinDriver's .NET WDC library to
   communicate with PCI devices, including:
   -- Scanning the PCI bus to locate a specific device and retrieve its
      resources information
   -- Reading/writing from/to a specific address or register
   -- Reading/writing from/to the PCI configuration space
   -- Handling the interrupts of a PCI device
   -- Registering to receive notifications for Plug and Play and power
      management events for the device

   Both the library and the sample code use the WinDriver .NET API DLL -
   wdapi_dotnet<version>.dll - which provides the required .NET interface for
   the WinDriver PCI API and enables the use of the C WinDriver API DLL -
   wdapi<version>.dll (found under the WinDriver\redist\ directory).
   A copy of wdapi_dotnet<version>.dll is provided with the sample (see below),
   as well as under the WinDriver\lib\<CPU>\<.NET Version>\ directory
   (e.g. WinDriver\lib\x86\v1.1.4322 - for Windows x86 32-bit, .NET v1.1.4322).
   The source code of wdapi_dotnet<version>.dll is found in the
   WinDriver\src\wdapi.net\ directory.

   In addition to the pci_lib library and the pci_sample application, the
   .NET pci_sample solution also includes a Kernel PlugIn project - kp_pci -
   which demonstrates how WinDriver's Kernel PlugIn feature can be combined
   with our .NET library.
   The kp_pci project included in the sample's solution is the same project
   used by the C pci_diag sample, and its source code can be found under the
   WinDriver\samples\pci_diag\kp_pci\ directory.

   The pci_lib Library
   ---------------------------
   The library provides an interface between the WinDriver .NET API DLL
   (wdapi_dotnet<version>.dll - see above) and .NET PCI applications, such as
   the pci_sample application.

   The library's core are the PCI_Device and PCI_DeviceList classes, which
   provide an object oriented implementation of a PCI device object and a PCI
   driver object.

   The pci_sample Application
   ---------------------------
   The GUI sample, which uses the pci_lib.dll library, displays the resources
   information for each connected device, and enables communication with the
   device through several GUI forms.

   The sample enables you to select whether to communicate with the device(s)
   entirely from the user mode, or use the kp_pci Kernel PlugIn driver to
   handle performance critical operations in kernel mode, as explained below.

   The kp_pci Kernel PlugIn Driver
   ---------------------------------
   The kp_pci project was implemented using WinDriver's Kernel PlugIn feature.
   This feature enables optimal performance I/O and interrupt handling, by
   executing these directly from the kernel mode.
   The Kernel PlugIn project is built into a *.sys kernel-mode driver, which is
   driven from a user-mode WinDriver application.

   The Kernel PlugIn is written in C and uses the same WinDriver C APIs
   (WDC_XXX/WD_XXX) in the kernel mode, as are supported in the user mode.
   This enables user-mode WinDriver C code to be easily ported to the kernel
   mode, thereby saving the context-switch time and enabling the developer to
   create a high performance driver.

   Using the kp_pci Kernel PlugIn Driver
   --------------------------------------
   After opening a handle to the device, the .NET user-mode application
   (pci_sample) attempts to open a handle to the kp_pci Kernel PlugIn driver.
   If successful, pci_sample can use the Kernel PlugIn driver to perform
   performance-critical operations, such as interrupt handling.
   pci_sample also demonstrates how to pass data between the .NET user-mode
   application and Kernel PlugIn driver.
   If the application fails to open a handle to kp_pci (for example, if you did
   not install this driver), it will continue to perform all communication with
   the device, including interrupt handling and handling of Plug and Play and
   power management events, from the user mode.

   This structure enables you to first test the communication with the device
   solely from the user mode, and then easily port functionality from the user
   mode to the kernel by simply installing the kp_pci Kernel PlugIn driver.

   Interrupt Notes
   ----------------
   1. The sample Kernel PlugIn interrupt handler clears each interrupt at HIGH
      IRQL, and performs deferred processing and notifies the user-mode once
      for every 5 interrupts.

   2. As documented in the code, the commands for acknowledging the interrupt
      are hardware-specific. Therefore, to use this sample to handle the
      interrupts on your device, you must first modify the code to implement
      the correct commands for acknowledging the interrupts.

      When using kp_pci to handle the interrupts, change the implementation of
      KP_PCI_IntAtIrql() (in kp_pci.c) in order to correctly acknowledge the
      interrupt on your device.

      When handling interrupts without a Kernel PlugIn driver, change the code
      in EnableInterrupts() and CreateIntTransCmds() (in PCI_Device.cs) and set
      up the correct transfer commands for clearing the interrupt, as indicated
      by the comments in these functions.


2. pci_sample/ Files
   ==================
   This section describes the sub-directories and files provided under the
   csharp.net\pci_sample\ directory.

   - readme.txt:
         Describes the contents of the csharp.net\pci_sample\ directory.

   - Visual Studio solution files for building the pci_lib library, kp_pci
     Kernel PlugIn driver and pci_sample application:

     For x86 32-bit platforms:
     -------------------------
     - x86\ subdirectory
       - msdev_20xx\ subdirectory
         - pci_sample.sln
           Visual Studio 20xx C# solution file.

     For AMD64 64-bit platforms:
     -------------------------
     - amd64\ subdirectory
       - msdev_20xx\ subdirectory
       - pci_sample.sln
          Visual Studio 20xx C# solution file.

   - lib\ sub-directory:
     -------------------
     This directory contains the source code of the pci_lib.dll library,
     as well as project files for building the library:

     For all Windows platforms:
     --------------------------
     - PCI_Device.cs:
           C# source file.
           Implements a PCI device class.

     - PCI_DeviceList.cs:
           C# source file.
           Implements the library's driver module.

     - PCI_Regs.cs:
           C# source file.
           Implements a register struct and an array of configuration
           space registers.

     - log.cs:
           C# source file.
           Implements a log class for the library's error and trace messages.

   - diag\ sub-directory:
     --------------------
     This directory contains the pci_sample.exe sample application, as
     well as its source code and project files for building the code:

     For all Windows platforms:
     --------------------------
     - PCI_Sample.cs:
           C# source file.
           The sample's main form. Implements the sample's graphical user
           interface (GUI) and related code.

     - AddrSpaceTransferForm.cs:
           C# source file.
           Implements reading and writing from/to the device's BARS.

     - CfgTransfersForm.cs:
           C# source file.
           Implements reading and writing from/to the device's PCI
           configuration space (using offsets).

     - Registers.cs:
           C# source file.
           Implements reading and writing from/to the device's PCI
           configuration space registers.

     For x86 32-bit platforms:
     -------------------------
     - x86\ subdirectory
       - msdev_20xx\ subdirectory
         - pci_sample.csproj:
           Visual Studio 20xx C# project file.

     For AMD64 64-bit platforms:
     -------------------------
     - amd64\ subdirectory
      - msdev_20xx\ subdirectory
      - pci_sample.csproj:
           Visual Studio 20xx C# project file.

     - diag\Release\ sub-directory:

       - pci_sample.exe:
             A pre-compiled sample executable.

       - pci_lib.dll:
             A copy of the C# .NET PCI library.

       - wdapi_dotnet<version>.dll:
            A copy of the WinDriver .NET API DLL.

     - Additional files required for building the sample code (resources,
       assembly, etc.)


3. kp_pci Files
   =============
   This section describes the kp_pci Kernel PlugIn sub-directories and files
   provided under the WinDriver\samples\pci_diag\ directory.

   - pci_lib.c:
         Implementation of a library for accessing PCI devices using
         WinDriver's C WDC API. The library's API are used by the Kernel
         PlugIn driver (kp_pci).

   - pci_lib.h:
         Header file for the pci_lib library.

   - kp_pci\ sub-directory:
     -----------------------

     For all Windows platforms:
     --------------------------

     - kp_pci.c:
           Implementation of a sample WinDriver PCI Kernel PlugIn driver, which
           demonstrates how to communicate with PCI devices from the kernel
           using WinDriver's C API and the WDC library.

      - makefile, kp_pci.mak, kp_pci.rc, sources:
            Make, resources and sources files required for building the Kernel
            PlugIn project.

      For x86 32-bit platforms:
      -------------------------
      - x86/ sub-directory:
        -------------------
        Windows x86 32-bit project files for building the kp_pci.sys Kernel
        PlugIn driver:
        - x86/ sub-directory:
          - msdev_20xx/ sub-directory:
              - kp_pci.vcxproj
                    Visual Studio 20xx project file

      - WINNT.i386/ sub_directory:
        --------------------------
        - kp_pci.sys:
              A pre-compiled 32-bit version of the KP_PCI Kernel PlugIn
              driver, which was built with the Windows Driver Kit (WDK).

      For AMD64 64-bit platforms:
      ---------------------------
      - amd64/ sub-directory:
        ---------------------
        Windows x64 64-bit project file for building the kp_pci.sys Kernel
        PlugIn driver:

        - amd64/
          - msdev_20xx/ sub-directory:
              - kp_pci.vcxproj
                    Visual Studio 20xx project file

      - WINNT.x86_64/ sub_directory:
        ----------------------------
        - kp_pci.sys:
              A pre-compiled 64-bit version of the KP_PCI Kernel PlugIn
              driver, which was built with the Windows Driver Kit (WDK).


4. Using the sample application
   =============================
   To use the sample pci_sample.exe application, follow these steps:

   1) Install the .NET Framework.
   2) If you wish to test the kp_pci Kernel PlugIn driver, build the driver
      for your target operating system, as explained in section #7 below.
      You can also use the pre-compiled kp_driver.sys driver from
      the pci_diag/kp_pci/WINNT.i386 directory (for Windows x86 32-bit), or
      pci_diag/kp_pci/WINNT.x86_64 directory (for Windows x86_64 64-bit), which
      were built with the Windows Driver Kit (WDK).
   3) If you wish to test the kp_pci Kernel PlugIn driver, install the driver
      you have built in step #2, or the pre-compiled Windows kp_pci.sys driver,
      by following the steps in section #8 of this file.
   4) Build the pci_lib.dll and the pci_sample.exe application by following the
      instructions in sections #5 and #6 of this file, or use the pre-built
      versions of the dll and the application from the
      diag\Release\ sub-directory (see section #2 above).
   5) Install an INF file for any PCI device that you wish to view and control
      from the application, which registers the device(s) to work with
      WinDriver. You can use the DriverWizard (Start | WinDriver | DriverWizard)
      to create and install the required INF file(s).
   6) Run the pci_sample.exe executable and use it to test the communication
      with your PCI device(s).
      NOTE: In order to run the executable the wdapi_dotnet<version>.dll and
      pci_lib.dll files must be found in the same directory as pci_sample.exe.


5. Building the PCI library code (*)
   ==============================
   1) Install the .NET Framework and Visual Studio (2012 or higher).
   2) Open the relevant solution file (pci_sample.sln).
   3) Set pci_lib as the active project and build it by right-clicking on it
      and then choosing Build.


6. Building the sample code (*)
   =========================
   1) Install the .NET Framework and Visual Studio (2012 or higher).
   2) Open the relevant solution file (pci_sample.sln).
   3) Set pci_sample as the active project and build it by right-clicking on it
      and then choosing Build.


7. Building the kp_pci Kernel PlugIn driver (*)
   =========================================
   To compile and build the kp_pci Kernel PlugIn driver you need an appropriate
   C/C++ compiler for your development platform.

   To compile the kp_pci Kernel PlugIn project on Windows you need the
   Windows Driver Kit (WDK) for your target OS.

   1) Install the WDK for your target OS.

   2) Open the relevant pci_sample solution file (see section #2),
      verify that the kp_pci project is set as the active project, select your
      preferred active build configuration (e.g. x64 - Release), and then
      simply build the project.
      If the build was successful, you should find a kp_pci.sys driver for
      your target OS under the samples/pci_diag/kp_pci/<target object dir>.<CPU>
      sub-directory (e.g. kp_pci/WINNT.x86_64/kp_pci.sys).


(*) Note: You can build all three projects at once (pci_lib, pci_sample and
    kp_pci) by simply building the solution (CTRL+SHIFT+B OR from the Build |
    Build Solution menu).


8. Installing the kp_pci Kernel PlugIn driver
   ===========================================
   1) Copy the kp_pci.sys driver file for your target OS to the
      %windir%\system32\drivers directory.

   2) Install the driver from the command-line using the
      WinDriver\util\wdreg.exe utility:
          wdreg.exe -name KP_PCI install

