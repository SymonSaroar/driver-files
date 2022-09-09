PCI_DIAG

This file contains:
1. An overview of the pci_diag sample.
2. Testing the sample.
3. Instructions for building the pci_diag application.
4. Instructions for building the kp_pci Kernel PlugIn driver.
5. Instructions for installing the kp_pci Kernel PlugIn driver.
6. Guidelines for converting pci_diag from a console-mode (CUI)
   application to a graphical (GUI) application.


1. Overview
   =========

   pci_diag and kp_pci - General Overview
   ---------------------------------------
   The pci_diag/ directory contains the C source code of a diagnostics
   WinDriver PCI library, as well as user-mode console application (pci_diag)
   and a kernel-mode Kernel PlugIn driver (kp_pci), which use the sample
   library API. For a list of the files provided in this directory,
   refer to the pci_diag/files.txt file.

   The sample code demonstrates how to use WinDriver's WDC library to
   communicate with a PCI device, including:
   -- Scanning the PCI bus to locate a specific device and retrieve its
      resources information
   -- Reading/writing from/to a specific address or register
   -- Reading/writing from/to the PCI configuration space
   -- Handling the interrupts of a PCI device
   -- Registering to receive notifications for Plug and Play and power
      management events for the device

   The Kernel PlugIn
   ------------------
   The WinDriver Kernel PlugIn feature enables optimal performance I/O and
   interrupt handling, by executing these directly from the kernel mode.
   The Kernel PlugIn is written in C and uses the same WinDriver C APIs
   (WDC_XXX/ WD_XXX) in the kernel mode, as are supported in the user mode.
   This enables user-mode WinDriver code to be easily ported to the kernel
   mode, thereby saving the context-switch time and enabling the developer to
   create a high performance driver.

   Using the kp_pci Kernel PlugIn driver
   --------------------------------------
   The user-mode application (pci_diag) attempts to open a handle to the
   selected device using the kp_pci Kernel PlugIn driver. If successful,
   pci_diag will use the Kernel PlugIn driver to perform performance-critical
   operations, such as interrupt handling or handling of Plug and Play and
   power management events, directly in the kernel. pci_diag also demonstrates
   how to pass data between the user-mode application and Kernel PlugIn driver.
   If the application fails to open a handle to kp_pci (for example, if you did
   not install this driver), it will attempt to open a device handle without a
   Kernel PlugIn. If successful, pci_diag application will perform all
   communication with the device, including interrupt handling and handling of
   Plug and Play and power management events, from the user mode.

   This structure enables you to first test the communication with the device
   solely from the user-mode, and then easily port functionality from the user
   mode to the kernel by simply installing the kp_pci Kernel PlugIn driver.

   Interrupt Notes
   ----------------
   1. The sample Kernel PlugIn interrupt handler clears the interrupt at HIGH
      IRQL and performs deferred processing and notifies the user-mode once for
      every 5 interrupts.

   2. As documented in the code, the commands for acknowledging the interrupt
      are hardware-specific. Therefore, to use this sample to handle the
      interrupts on your device, you must first modify the code to implement
      the correct commands for acknowledging the interrupts.

      When using kp_pci to handle the interrupts, change the implementation of
      KP_PCI_IntAtIrql() (in kp_pci.c) in order to correctly acknowledge the
      interrupt on your device.

      When handling interrupts without a Kernel PlugIn driver, change the code
      in PCI_IntEnable() (in pci_lib.c) and set up the correct transfer
      commands for clearing the interrupt, as indicated by the comments in this
      function.


2. Testing the sample
   ===================

   1) If you wish to test the kp_pci Kernel PlugIn driver, build the driver
      for your target operating system, as explained in section #3 below.
      On Windows you can also use the pre-compiled kp_driver.sys driver from
      the pci_diag/kp_pci/WINNT.i386 (for Windows x86 32-bit) directory,
      or pci_diag/kp_pci/WINNT.x86_64 (for Windows x86_64 64-bit) directory,
      which were built with the Windows Driver Kit (WDK).

   2) If you wish to test the kp_pci Kernel PlugIn driver, install the driver
      you have built in step #1, or the pre-compiled Windows kp_pci.sys driver,
      by following the steps in section #5 of this file.

   3) Build the user-mode pci_diag application (e.g. pci_diag.exe for Windows),
      as explained in section #3 of this file, or use the pre-compiled
      pci_diag application (see files.txt).

   4) Run the sample pci_diag application and test the communication with
      the device, either using a Kernel PlugIn driver (if you installed kp_pci)
      or entirely from the user-mode.

      On Win32 platforms:
          Run:
          WIN32\pci_diag.exe

      On LINUX:
          Run:
          LINUX/pci_diag


3. Building the pci_diag application
   ==================================
   To compile and build the pci_diag application you need an appropriate C/C++
   compiler for your development platform.

   - On Windows: If you are using one of the supported Visual Studio
     IDEs (see files.txt): open the pci_diag project/solution
     file from the relevant compiler sub-directory and simply build the project.

   - On Linux: If you are using GNU make, simply build the sample
      using the makefile from the LINUX/ sub-directory.

   - If you are using a different C/C++ compiler or make utility:
     - Create a new console mode project/makefile for your compiler.
     - Add the following files to the project/makefile:
       WinDriver/samples/c/pci_diag/pci_diag.c
       WinDriver/samples/c/pci_diag/pci_lib.c
       WinDriver/samples/c/shared/wdc_diag_lib.c
       WinDriver/samples/c/shared/wds_diag_lib.c
       WinDriver/samples/c/shared/diag_lib.c
     - Link your project with WinDriver/lib/wdapi<version>.lib (Windows)
       or WinDriver/lib/libwdapi<version>.so (Linux) OR
       add the following files (which are exported in the wdapi<version> DLL /
       shared object) to your project/makefile:
       WinDriver/src/wdapi/utils.c
       WinDriver/src/wdapi/windrvr_int_thread.c
       WinDriver/src/wdapi/windrvr_events.c
       WinDriver/src/wdapi/status_strings.c
       WinDriver/src/wdapi/wdc_general.c
       WinDriver/src/wdapi/wdc_cfg.c
       WinDriver/src/wdapi/wdc_mem_io.c
       WinDriver/src/wdapi/wdc_ints.c
       WinDriver/src/wdapi/wds_ipc.c
       WinDriver/src/wdapi/wdc_events.c
       WinDriver/src/wdapi/wdc_err.c
       WinDriver/src/wdapi/wdc_dma.c
     - Build your project.


4. Building the kp_pci Kernel PlugIn driver
   =========================================
   To compile and build the kp_pci Kernel PlugIn driver you need an appropriate
   C/C++ compiler for your development platform.

   On Windows:
   -----------
   To compile the kp_pci Kernel PlugIn project on Windows you need the
   Windows Driver Kit (WDK) for your target OS.

   1) Install the Windows Driver Kit (WDK) for your target OS.

   2) Open one of the pci_diag Visual Studio solution files
      (see files.txt) with Visual Studio, verify that the kp_pci project is set
      as the active project, select your preferred active build configuration
      (e.g. x64 - Release), and then simply build the project.
      If the build was successful, you should find a kp_pci.sys driver for
      your target OS under the kp_pci/<os>.<platform> sub-directory
      (e.g. kp_pci/WINNT.i386/kp_pci.sys).


   On Linux:
   ---------

   1) Change directory to the kp_pci/ directory:
      cd kp_pci

   2) Run the configure script to create a Kernel PlugIn makefile based on
      your specific running kernel:
      ./configure

      Note: You can also run the configure script based on another kernel
            source you have installed, by adding the flag
            --with-kernel-source=<path> to the configure script, where <path>
            is the full path to the target kernel source directory, e.g.
            /usr/src/linux.

   3) Build the Kernel PlugIn driver using the makefile that you created:
      make

      This command creates a new LINUX.xxx/ directory (where xxx depends on
      the Linux kernel), which contains the created kp_pci.o/.ko driver.


5. Installing the kp_pci Kernel PlugIn driver
   ==========================================

   On Windows:
   -----------
   1) Copy the kp_pci.sys driver file for your target OS to the
      %windir%\system32\drivers directory.

   2) Install the driver from the command-line using the
      WinDriver\util\wdreg.exe utility:
          wdreg.exe -name KP_PCI install


   On Linux:
   ---------

   1) Change directory to the kp_pci/ directory:
      cd kp_pci

   2) Run the following command to install the kp_pci.o/.ko driver:
      make install


6. Converting pci_diag to a GUI application
   =========================================
   pci_diag was written as a console-mode application (rather than a GUI
   application) that uses standard input and standard output.
   This was done in order to simplify the source code.
   You can change the sample into a GUI application by replacing all calls to
   <stdio.h> functions such as printf(), sprintf(), scanf(), etc. with relevant
   GUI functions and replacing the main() function in pci_diag.c with a GUI
   equivalent.

