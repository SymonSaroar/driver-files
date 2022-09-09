C# USB Sample
==============

This file contains:
1. An overview of the contents of the csharp.net\usb_sample\ directory.
2. List of files.
3. Instructions for using the sample application.
4. Instructions for building the USB library.
5. Instructions for building the USB sample code.


1. Overview
   =========
   This code sample is provided AS-IS and as a guiding sample only.

   The csharp.net\usb_sample\ directory contains:
   - A C# .NET USB library - usb_lib_dotnet.dll.
   - A graphical (GUI) C# .NET USB sample diagnostics application -
     csharp_usb_sample.exe.
   - The source code of the C# USB library and diagnostics application.

   The code was written by Jungo Connectivity using WinDriver's USB API.

   General Overview
   -----------------
   The library and sample application support handling of multiple USB devices
   and simultaneous execution of several tasks (on different devices or on the
   same device) - such as reading from one pipe while writing to the other,
   etc.

   Both the library and the sample code use the WinDriver .NET API DLL -
   wdapi_dotnet<version>.dll - which provides the required .NET interface for
   the WinDriver USB API and enables the use of the C WinDriver API DLL -
   wdapi<version>.dll (found under the WinDriver\redist\ directory).
   A copy of wdapi_dotnet<version>.dll is provided with the sample (see below),
   as well as under the WinDriver\lib\<CPU>\<.NET Version>\ directory
   (e.g. WinDriver\lib\x86\v1.1.4322 - for Windows x86 32-bit, .NET v1.1.4322).
   The source code of wdapi_dotnet<version>.dll is found in the
   WinDriver\src\wdapi.net\ directory.

   The usb_lib_dotnet Library
   ---------------------------
   The library provides an interface between the WinDriver .NET API DLL
   (wdapi_dotnet<version>.dll - see above) and .NET USB applications, such as
   the csharp_usb_sample application.

   The library's core are the UsbDevice and UsbDeviceManager classes, which
   provide an object oriented implementation of a USB device object and a USB
   driver object.

   The csharp_usb_sample Application
   ----------------------------------
   The GUI sample, which uses the usb_lib_dotnet.dll library, reflects the
   insertion and removal of USB devices registered to work with WinDriver (via
   an *.inf file) and displays the configuration and resources (pipes)
   information for each connected device.
   The application also enables changing the active alternate setting for each
   connected device.

   You can easily switch between the connected devices from the GUI and test
   the communication with the device. You can issue requests on the control
   pipe, write and read (listen) from/to the pipes, or reset a pipe.


2. Files
   ======
   This section describes the sub-directories and files provided under the
   csharp.net\usb_sample\ directory.

   - readme.txt:
         Describes the contents of the csharp.net\usb_sample\ directory.

   - lib\ sub-directory:
     -------------------
     This directory contains the source code of the usb_lib_dotnet.dll library,
     as well as project and solution files for building the library:

     For all Windows platforms:
     --------------------------
     - UsbDevice.cs:
           C# source file.
           Implements a USB device class and a USB pipe class.

     - UsbDeviceManager.cs:
           C# source file.
           Implements the library's driver module.


   - diag\ sub-directory:
     --------------------
     This directory contains the csharp_usb_sample.exe sample application, as
     well as its source code and project and solution files for building the
     code:

     For all Windows platforms:
     --------------------------
     - UsbSample.cs:
           C# source file.
           The sample's main form. Implements the sample's graphical user
           interface (GUI) and related code.

     - FormChangeSettings.cs:
           C# source file.
           Input form for changing a device's active alternate setting.

     - FormTransfers.cs:
           C# source file.
           Input form for performing pipe transfers.

     - DeviceTabPage.cs:
           C# source file.
           Defines a new USB device tab page class (DeviceTabPage), which
           inherits from the graphical .NET System.Windows.Forms.TabPage class.

     - csharp_usb_sample_msdev_2010AnyCPU.csproj:
           Microsoft .NET 2010 C# project file.

     - csharp_usb_sample_msdev_2010AnyCPU.sln:
           Microsoft .NET 2010 C# solution file.

     - diag\Release\<.NET Version>\ sub-directory:

       - csharp_usb_sample.exe:
             A pre-compiled sample executable.

       - usb_lib_dotnet.dll:
             A copy of the C# .NET USB library.

       - wdapi_dotnet<version>.dll:
            A copy of the WinDriver .NET API DLL.

     - Additional files required for building the sample code (resources,
       assembly, etc.)

     For x86 32-bit platforms:
     -------------------------
     - csharp_usb_sample_msdev_2003.csproj:
           Microsoft .NET 2003 C# project file.

     - csharp_usb_sample_msdev_2003.sln:
           Microsoft .NET 2003 C# solution file.



3. Using the sample application
   =============================
   To use the sample csharp_usb_sample.exe application, follow these steps:

   1) Install the .NET Framework.
   2) Build the csharp_usb_sample.exe application by following the instructions
      in section #4 of this file, or use the pre-built version of the
      application from the diag\Release\ sub-directory
      (see section #2 above.)
   3) Install an INF file for any USB device that you wish to view and control
      from the application, which registers the device(s) to work with
      WinDriver. You can use the DriverWizard (Start | WinDriver | DriverWizard)
      to create and install the required INF file(s).
   4) Run the csharp_usb_sample.exe executable and use it to test the
      communication with your USB device(s).
      NOTE: In order to run the executable the wdapi_dotnet<version>.dll and
      usb_lib_dotnet.dll files must be found in the same directory as
      csharp_usb_sample.exe.


4. Building the USB library code
   ==============================
   1) Install the .NET Framework and Visual Studio (2012 or higher).
   2) Open the relevant library solution file (usb_lib_dotnet.csproj).
   3) Build the solution (CTRL+SHIFT+B OR from the Build | Build Solution menu).


5. Building the sample code
   =========================
   1) Install the .NET Framework and Visual Studio (2012 or higher).
   2) Open the relevant sample solution file (csharp_usb_sample.csproj).
   3) Build the solution (CTRL+SHIFT+B OR from the Build | Build Solution menu).

