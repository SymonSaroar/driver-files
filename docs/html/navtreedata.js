/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "Jungo WinDriver", "index.html", [
    [ "Introduction", "index.html", null ],
    [ "Chapter 1: Overview", "ch1_overview.html", [
      [ "1.1. Introduction", "ch1_overview.html#ch1_1_introduction", null ],
      [ "1.2. Main Features", "ch1_overview.html#ch1_2_main_features", null ],
      [ "1.3. Architecture", "ch1_overview.html#ch1_3_architecture", null ],
      [ "1.4. Supported Platforms", "ch1_overview.html#ch1_4_supported_platforms", null ],
      [ "1.5. Linux ARM", "ch1_overview.html#ch1_5_linux_arm", null ],
      [ "1.6. Operating Systems Support Overview", "ch1_overview.html#ch1_6_operating_systems_support_overview", null ],
      [ "1.7. Limitations of Different Evaluation Versions", "ch1_overview.html#ch1_7_limitations_of_different_evaluation_versions", [
        [ "1.7.1. Limitations on MacOS", "ch1_overview.html#ch1_7_1_limitations_on_macos", null ]
      ] ],
      [ "1.8. What WinDriver includes", "ch1_overview.html#ch1_8_what_windriver_includes", null ]
    ] ],
    [ "Chapter 2: Understanding Device Drivers", "ch2_understanding_device_drivers.html", [
      [ "2.1. Device Driver Overview", "ch2_understanding_device_drivers.html#ch2_1_device_driver_overview", null ],
      [ "2.2. Classification of Drivers According to Functionality", "ch2_understanding_device_drivers.html#ch2_2_classification_of_drivers_according_to_functionality", [
        [ "2.2.1. Monolithic Drivers", "ch2_understanding_device_drivers.html#ch2_2_1_monolithic_drivers", null ],
        [ "2.2.2. Layered Drivers", "ch2_understanding_device_drivers.html#ch2_2_2_layered_drivers", null ],
        [ "2.2.3. Miniport Drivers", "ch2_understanding_device_drivers.html#ch2_2_3_miniport_drivers", null ]
      ] ],
      [ "2.3. Classification of Drivers According to Operating Systems", "ch2_understanding_device_drivers.html#ch2_3_classification_of_drivers_according_to_operating_systems", [
        [ "2.3.1. WDM Drivers", "ch2_understanding_device_drivers.html#ch2_3_1_wdm_drivers", null ],
        [ "2.3.2. WDF Drivers", "ch2_understanding_device_drivers.html#ch2_3_2_wdf_drivers", null ],
        [ "2.3.3. Unix Device Drivers", "ch2_understanding_device_drivers.html#ch2_3_3_unix_device_drivers", null ],
        [ "2.3.4. Linux Device Drivers", "ch2_understanding_device_drivers.html#ch2_3_4_linux_device_drivers", null ]
      ] ],
      [ "2.4. The Entry Point of the Driver", "ch2_understanding_device_drivers.html#ch2_4_the_entry_point_of_the_driver", null ],
      [ "2.5. Associating the Hardware with the Driver", "ch2_understanding_device_drivers.html#ch2_5_associating_the_hardware_with_the_driver", null ],
      [ "2.6. Communicating with Drivers", "ch2_understanding_device_drivers.html#ch2_6_communicating_with_drivers", null ]
    ] ],
    [ "Chapter 3: Installing WinDriver", "ch3_installing_windriver.html", [
      [ "3.1. System Requirements", "ch3_installing_windriver.html#ch3_1_system_requirements", [
        [ "3.1.1. Windows System Requirements", "ch3_installing_windriver.html#ch3_1_1_windows_system_requirements", null ],
        [ "3.1.2. Linux System Requirements", "ch3_installing_windriver.html#ch3_1_2_linux_system_requirements", null ],
        [ "3.1.3. MacOS System Requirements", "ch3_installing_windriver.html#ch3_1_3_macos_system_requirements", null ]
      ] ],
      [ "3.2. WinDriver Installation Process", "ch3_installing_windriver.html#ch3_2_windriver_installation_process", [
        [ "3.2.1. Windows WinDriver Installation Instructions", "ch3_installing_windriver.html#ch3_2_1_windows_windriver_installation_instructions", null ],
        [ "3.2.2. Linux WinDriver Installation Instructions", "ch3_installing_windriver.html#ch3_2_2_linux_windriver_installation_instructions", [
          [ "3.2.2.1. Preparing the System for Installation", "ch3_installing_windriver.html#ch3_2_2_1_preparing_the_system_for_installation_linux", null ],
          [ "3.2.2.2. Installing WinDriver on x86/x86_64 systems", "ch3_installing_windriver.html#ch3_2_2_2_installing_windriver_on_x86-x86_64_systems", null ],
          [ "3.2.2.3. Installing WinDriver on ARM/ARM64 systems", "ch3_installing_windriver.html#ch3_2_2_3_installing_windriver_on_arm-arm64_systems", [
            [ "3.2.2.3.1. Cross compiling the WinDriver kernel module for Linux ARM/ARM64 systems", "ch3_installing_windriver.html#ch3_2_2_3_1_cross_compiling_the_windriver_kernel_module_for_linux_arm-arm64_systems", null ]
          ] ],
          [ "3.2.2.4. Restricting Hardware Access on Linux", "ch3_installing_windriver.html#ch3_2_2_4_restricting_hardware_access_on_linux", null ]
        ] ],
        [ "3.2.3. MacOS WinDriver Installation Instructions", "ch3_installing_windriver.html#ch3_2_3_macos_windriver_installation_instructions", [
          [ "3.2.3.1 Preparing the System for Installation", "ch3_installing_windriver.html#ch3_2_3_1_preparing_the_system_for_installation_macos", null ],
          [ "3.2.3.2 Installation on MacOS x86_64", "ch3_installing_windriver.html#ch3_2_3_2_installation_on_macos_x86_64", null ],
          [ "3.2.3.3 Installation on MacOS ARM64 (M1)", "ch3_installing_windriver.html#ch3_2_3_3_installation_on_macos_arm64_m1", null ]
        ] ]
      ] ],
      [ "3.3. Upgrading Your Installation", "ch3_installing_windriver.html#ch3_3_upgrading_your_installation", null ],
      [ "3.4. Checking Your Installation", "ch3_installing_windriver.html#ch3_4_checking_your_installation", [
        [ "3.4.1. Installation Check", "ch3_installing_windriver.html#ch3_4_1_installation_check", null ]
      ] ],
      [ "3.5. Uninstalling WinDriver", "ch3_installing_windriver.html#ch3_5_uninstalling_windriver", [
        [ "3.5.1. Windows WinDriver Uninstall Instructions", "ch3_installing_windriver.html#ch3_5_1_windows_windriver_uninstall_instructions", null ],
        [ "3.5.2. Linux WinDriver Uninstall Instructions", "ch3_installing_windriver.html#ch3_5_2_linux_windriver_uninstall_instructions", null ],
        [ "3.5.3. MacOS WinDriver Uninstall Instructions", "ch3_installing_windriver.html#ch3_5_3_macos_windriver_uninstall_instructions", null ]
      ] ]
    ] ],
    [ "Chapter 4: PCI Express Overview", "ch4_pci_express_overview.html", [
      [ "4.1. Overview", "ch4_pci_express_overview.html#ch4_1_overview_pci_express", null ],
      [ "4.2. WinDriver for PCI Express", "ch4_pci_express_overview.html#ch4_2_windriver_for_pci_express", null ],
      [ "4.3. The pci_dump and pci_scan Utilities", "ch4_pci_express_overview.html#ch4_3_the_pci_dump_and_pci_scan_utilities", [
        [ "4.3.1. Dump PCI Configuration Space into a file", "ch4_pci_express_overview.html#ch4_3_1_dump_pci_configuration_space_into_a_file", null ]
      ] ],
      [ "4.4. FAQ", "ch4_pci_express_overview.html#ch4_4_faq_pci_express_overview", [
        [ "4.4.1. Enabling legacy PCI configuration space read/write for identifying PCI devices on Windows", "ch4_pci_express_overview.html#ch4_4_1_enabling_legacy_pci_configuration_space_read-write_for_identifying_pci_devices_on_windows", null ],
        [ "4.3.2. How do I access the memory on my PCI card using WinDriver?", "ch4_pci_express_overview.html#ch4_3_2_how_do_i_access_the_memory_on_my_pci_card_using_windriver", null ],
        [ "4.3.3. How do I use WinDriver for PCI Express over Thunderbolt?", "ch4_pci_express_overview.html#ch4_3_3_how_do_i_use_windriver_for_pci_express_over_thunderbolt", null ]
      ] ]
    ] ],
    [ "Chapter 5: USB Overview", "ch5_usb_overview.html", [
      [ "5.1. Introduction to USB", "ch5_usb_overview.html#ch5_1_introduction_to_usb", null ],
      [ "5.2. WinDriver USB Benefits", "ch5_usb_overview.html#ch5_2_windriver_usb_benefits", null ],
      [ "5.3. USB Components", "ch5_usb_overview.html#ch5_3_usb_components", null ],
      [ "5.4. Data Flow in USB Devices", "ch5_usb_overview.html#ch5_4_data_flow_in_usb_devices", null ],
      [ "5.5. USB Data Exchange", "ch5_usb_overview.html#ch5_5_usb_data_exchange", null ],
      [ "5.6. USB Data Transfer Types", "ch5_usb_overview.html#ch5_6_usb_data_transfer_types", [
        [ "5.6.1. Control Transfer", "ch5_usb_overview.html#ch5_6_1_control_transfer", null ],
        [ "5.6.2. Isochronous Transfer", "ch5_usb_overview.html#ch5_6_2_isochronous_transfer", null ],
        [ "5.6.3. Interrupt Transfer", "ch5_usb_overview.html#ch5_6_3_interrupt_transfer", null ],
        [ "5.6.4. Bulk Transfer", "ch5_usb_overview.html#ch5_6_4_bulk_transfer", null ]
      ] ],
      [ "5.7. USB Configuration", "ch5_usb_overview.html#ch5_7_usb_configuration", null ],
      [ "5.8. WinDriver USB", "ch5_usb_overview.html#ch5_8_windriver_usb", null ],
      [ "5.9. WinDriver USB Architecture", "ch5_usb_overview.html#ch5_9_windriver_usb_architecture", null ],
      [ "5.10. WinDriver USB (WDU) Library Overview", "ch5_usb_overview.html#ch5_10_windriver_usb_wdu_library_overview", [
        [ "5.10.1. Calling Sequence for WinDriver USB", "ch5_usb_overview.html#ch5_10_1_calling_sequence_for_windriver_usb", null ],
        [ "5.10.2. Upgrading from the WD_xxx USB API to the WDU_xxx API", "ch5_usb_overview.html#ch5_10_2_upgrading_from_the_wd_xxx_usb_api_to_the_wdu_xxx_api", null ]
      ] ],
      [ "5.11. FAQ", "ch5_usb_overview.html#ch5_11_faq_usb_overview", [
        [ "5.11.1 How do I reset my USB device using WinDriver?", "ch5_usb_overview.html#ch5_11_1_how_do_i_reset_my_usb_device_using_windriver", null ]
      ] ]
    ] ],
    [ "Chapter 6: Using DriverWizard", "ch6_using_driverwizard.html", [
      [ "6.1. An Overview", "ch6_using_driverwizard.html#ch6_1_an_overview", null ],
      [ "6.2. DriverWizard Walkthrough", "ch6_using_driverwizard.html#ch6_2_driverwizard_walkthrough", [
        [ "6.2.1. Attach your hardware to the computer", "ch6_using_driverwizard.html#ch6_2_1_attach_your_hardware_to_the_computer", null ],
        [ "6.2.2. Run DriverWizard and select your device", "ch6_using_driverwizard.html#ch6_2_2_run_driverwizard_and_select_your_device", null ],
        [ "6.2.3. Generate and install an INF file for your device (Windows)", "ch6_using_driverwizard.html#ch6_2_3_generate_and_install_an_inf_file_for_your_device_windows", null ],
        [ "6.2.4. Uninstall the INF file of your device (Windows)", "ch6_using_driverwizard.html#ch6_2_4_uninstall_the_inf_file_of_your_device_windows", null ],
        [ "6.2.5. Select the desired alternate setting (USB)", "ch6_using_driverwizard.html#ch6_2_5_select_the_desired_alternate_setting_usb", null ],
        [ "6.2.6. Diagnose your device (PCI)", "ch6_using_driverwizard.html#ch6_2_6_diagnose_your_device_pci", null ],
        [ "6.2.7. Diagnose your device (USB)", "ch6_using_driverwizard.html#ch6_2_7_diagnose_your_device_usb", null ],
        [ "6.2.8. Import Register Information from CSV file", "ch6_using_driverwizard.html#ch6_2_8_import_register_information_from_csv_file", null ],
        [ "6.2.9. Samples And Automatic Code Generation", "ch6_using_driverwizard.html#ch6_2_9_samples_and_automatic_code_generation", [
          [ "6.2.9.1. Samples Or Code Generation", "ch6_using_driverwizard.html#ch6_2_9_1_samples_or_code_generation", null ],
          [ "6.2.9.2. The Generated PCI/ISA and USB C Code", "ch6_using_driverwizard.html#ch6_2_9_2_the_generated_pci-isa_and_usb_c_code", null ],
          [ "6.2.9.3. The Generated PCI/ISA Python Code", "ch6_using_driverwizard.html#ch6_2_9_3_the_generated_pci-isa_python_code", null ],
          [ "6.2.9.4. The Generated USB Python Code", "ch6_using_driverwizard.html#ch6_2_9_4_the_generated_usb_python_code", null ],
          [ "6.2.9.5. The Generated PCI/ISA Java Code", "ch6_using_driverwizard.html#ch6_2_9_5_the_generated_pci-isa_java_code", null ],
          [ "6.2.9.6. The Generated USB Java Code", "ch6_using_driverwizard.html#ch6_2_9_6_the_generated_usb_java_code", null ],
          [ "6.2.9.7. The Generated PCI/ISA and USB .NET Code", "ch6_using_driverwizard.html#ch6_2_9_7_the_generated_pci-isa_and_usb_net_code", null ]
        ] ]
      ] ],
      [ "6.3. Compiling the Generated Code", "ch6_using_driverwizard.html#ch6_3_compiling_the_generated_code", [
        [ "6.3.1. C/C#/VB Windows Compilation", "ch6_using_driverwizard.html#ch6_3_1_c-c-sharp-vb_windows_compilation", [
          [ "6.3.1.1. C Compilation with a Linux Makefile", "ch6_using_driverwizard.html#ch6_3_1_1_c_compilation_with_a_linux_makefile", null ],
          [ "6.3.1.2. C Compilation with CMake", "ch6_using_driverwizard.html#ch6_3_1_2_c_compilation_with_cmake", null ],
          [ "6.3.1.2.1 Generating a WinDriver C project for Xcode (MacOS)", "ch6_using_driverwizard.html#ch6_3_1_2_1_generating_a_windriver_c_project_for_xcode_macos", null ],
          [ "6.3.1.2.2 Generating a WinDriver C project for MinGW (Windows)", "ch6_using_driverwizard.html#ch6_3_1_2_2_generating_a_windriver_c_project_for_mingw_windows", null ]
        ] ],
        [ "6.3.2. Java Compilation", "ch6_using_driverwizard.html#ch6_3_2_java_compilation", [
          [ "6.3.2.1. Opening the DriverWizard generated Java code with Eclipse IDE", "ch6_using_driverwizard.html#ch6_3_2_1_opening_the_driverwizard_generated_java_code_with_eclipse_ide", null ],
          [ "6.3.2.2. Compiling and running the DriverWizard generated Java code from the command line", "ch6_using_driverwizard.html#ch6_3_2_2_compiling_and_running_the_driverwizard_generated_java_code_from_the_command_line", null ]
        ] ]
      ] ],
      [ "6.4. FAQ", "ch6_using_driverwizard.html#ch6_4_faq_using_driver_wizard", [
        [ "6.4.1. If a variable requires a pointer to be assigned to it, as in pBuffer = &dwVal, how do I do it in C# dotNET/Java/Python?", "ch6_using_driverwizard.html#ch6_4_1_if_a_variable_requires_a_pointer_to_be_assigned_to_it_as_in_pbuffer_dwval_how_do_i_do_it_in_c-sharp_dotnet-java-python", null ]
      ] ]
    ] ],
    [ "Chapter 7: Developing a Driver", "ch7_developing_a_driver.html", [
      [ "7.1. Using DriverWizard to Build a Device Driver", "ch7_developing_a_driver.html#ch7_1_using_driverwizard_to_build_a_device_driver", null ],
      [ "7.2. Using a code sample to Build a Device Driver", "ch7_developing_a_driver.html#ch7_2_using_a_code_sample_to_build_a_device_driver", null ],
      [ "7.3. Writing the Device Driver Without DriverWizard", "ch7_developing_a_driver.html#ch7_3_writing_the_device_driver_without_driverwizard", [
        [ "7.3.1. Include the Required WinDriver Files", "ch7_developing_a_driver.html#ch7_3_1_include_the_required_windriver_files", null ],
        [ "7.3.2. Write Your Code (PCI/ISA)", "ch7_developing_a_driver.html#ch7_3_2_write_your_code_pci-isa", null ],
        [ "7.3.3. Write Your Code (USB)", "ch7_developing_a_driver.html#ch7_3_3_write_your_code_usb", null ],
        [ "7.3.4. Configure and Build Your Code", "ch7_developing_a_driver.html#ch7_3_4_configure_and_build_your_code", null ]
      ] ],
      [ "7.4. Upgrading a Driver", "ch7_developing_a_driver.html#ch7_4_upgrading_a_driver", [
        [ "7.4.1. Regenerating code and gradually merging", "ch7_developing_a_driver.html#ch7_4_1_regenerating_code_and_gradually_merging", null ],
        [ "7.4.2. Checklist for Driver Code upgrade", "ch7_developing_a_driver.html#ch7_4_2_checklist_for_driver_code_upgrade", [
          [ "7.4.2.1 Register Your New License", "ch7_developing_a_driver.html#ch7_4_2_1_register_your_new_license", null ],
          [ "7.4.2.2 Conform to API Updates", "ch7_developing_a_driver.html#ch7_4_2_2_conform_to_api_updates", null ],
          [ "7.4.2.3 64-bit OS upgrade (Windows and Linux)", "ch7_developing_a_driver.html#ch7_4_2_3-bit_os_upgrade_windows_and_linux", null ],
          [ "7.4.2.4 Rename your driver (Windows and Linux)", "ch7_developing_a_driver.html#ch7_4_2_4_rename_your_driver_windows_and_linux", null ],
          [ "7.4.2.5 Ensure that your code uses the correct driver module", "ch7_developing_a_driver.html#ch7_4_2_5_ensure_that_your_code_uses_the_correct_driver_module", null ],
          [ "7.4.2.6 Rebuild your updated driver", "ch7_developing_a_driver.html#ch7_4_2_6_rebuild_your_updated_driver", null ],
          [ "7.4.2.7 Upgrade Your Device INF File (Windows)", "ch7_developing_a_driver.html#ch7_4_2_7_upgrade_your_device_inf_file_windows", null ],
          [ "7.4.2.8 Digitally Sign Your Driver Files (Windows)", "ch7_developing_a_driver.html#ch7_4_2_8_digitally_sign_your_driver_files_windows", null ],
          [ "7.4.2.9 Upgrade Your Driver Distribution/Installation Package", "ch7_developing_a_driver.html#ch7_4_2_9_upgrade_your_driver_distribution-installation_package", null ],
          [ "7.4.2.10 Check for changes in variable and struct sizes", "ch7_developing_a_driver.html#ch7_4_2_10_check_for_changes_in_variable_and_struct_sizes", null ]
        ] ]
      ] ],
      [ "7.5. 32-Bit Applications on 64-Bit Windows and Linux Platforms", "ch7_developing_a_driver.html#ch7_5-bit_applications_on_64-bit_windows_and_linux_platforms", [
        [ "7.5.1. Developing a 32-Bit Application for Both 32-Bit and 64-Bit Platforms", "ch7_developing_a_driver.html#ch7_5_1_developing_a_32-bit_application_for_both_32-bit_and_64-bit_platforms", null ],
        [ "7.5.2 64-Bit and 32-Bit Data Types", "ch7_developing_a_driver.html#ch7_5_2-bit_and_32-bit_data_types", null ]
      ] ],
      [ "7.6. WinDriver .NET APIs in PowerShell", "ch7_developing_a_driver.html#ch7_6_windriver_dotnet_apis_in_powershell", null ],
      [ "7.7. WinDriver Server API", "ch7_developing_a_driver.html#ch7_7_windriver_server_api", null ],
      [ "7.8. FAQ", "ch7_developing_a_driver.html#ch7_8_faq_developing_a_driver", [
        [ "7.8.1. Using WinDriver to build a GUI Application", "ch7_developing_a_driver.html#ch7_8_1_using_windriver_to_build_a_gui_application", null ],
        [ "7.8.2. Can WinDriver handle multiple devices, of different or similar types, at the same time?", "ch7_developing_a_driver.html#ch7_8_2_can_windriver_handle_multiple_devices_of_different_or_similar_types_at_the_same_time", null ],
        [ "7.8.3. Can I run two different device drivers, both developed with WinDriver, on the same machine?", "ch7_developing_a_driver.html#ch7_8_3_can_i_run_two_different_device_drivers_both_developed_with_windriver_on_the_same_machine", null ],
        [ "7.8.4. Can WinDriver group I/O and memory transfers?", "ch7_developing_a_driver.html#ch7_8_4_can_windriver_group_i-o_and_memory_transfers", null ],
        [ "7.8.5. I need to define more than 20 \"hardware items\" (I/O, memory, and interrupts) for my ISA card. Therefore, I increased the value of WD_CARD_ITEMS in the windrvr.h header file (due to the definition of the Item member of the WD_CARD structure as an array of WD_CARD_ITEMS WD_ITEMS structures). But now WD_CardRegister() will not work. Why?", "ch7_developing_a_driver.html#ch7_8_5_i_need_to_define_more_than_20_hardware_items_i-o_memory_and_interrupts_for_my_isa_card_therefore_i_increased_the_value_of_wd_card_items_in_the_windrvr_h_header_file_due_to_the_definition_of_the_item_member_of_the_wd_card_structure_as_an_array_of_wd_card_items_wd_items_structures_but_now_wd_cardregister_will_not_work_why", null ],
        [ "7.8.6. I have a WinDriver based application running on a certain operating system, how do I port my code to a different operating system?", "ch7_developing_a_driver.html#ch7_8_6_i_have_a_windriver_based_application_running_on_a_certain_operating_system_how_do_i_port_my_code_to_a_different_operating_system", null ]
      ] ]
    ] ],
    [ "Chapter 8: Debugging Drivers", "ch8_debugging_drivers.html", [
      [ "8.1. User-Mode Debugging", "ch8_debugging_drivers.html#ch8_1_user-mode_debugging", null ],
      [ "8.2. Debug Monitor", "ch8_debugging_drivers.html#ch8_2_debug_monitor", [
        [ "8.2.1. The wddebug_gui Utility", "ch8_debugging_drivers.html#ch8_2_1_the_wddebug_gui_utility", [
          [ "8.2.1.1. Search in wddebug_gui", "ch8_debugging_drivers.html#ch8_2_1_1_search_in_wddebug_gui", null ],
          [ "8.2.1.2. Opening Windows kernel crash dump with wddebug_gui", "ch8_debugging_drivers.html#ch8_2_1_2_opening_windows_kernel_crash_dump_with_wddebug_gui", null ],
          [ "8.2.1.3. Running wddebug_gui for a Renamed Driver", "ch8_debugging_drivers.html#ch8_2_1_3_running_wddebug_gui_for_a_renamed_driver", null ]
        ] ],
        [ "8.2.2. The wddebug Utility", "ch8_debugging_drivers.html#ch8_2_2_the_wddebug_utility", [
          [ "8.2.2.1. Console-Mode wddebug Execution", "ch8_debugging_drivers.html#ch8_2_2_1_console-mode_wddebug_execution", null ],
          [ "8.2.2.2. Debugging on a test machine", "ch8_debugging_drivers.html#ch8_2_2_2_debugging_on_a_test_machine", null ]
        ] ]
      ] ],
      [ "8.3. FAQ", "ch8_debugging_drivers.html#ch8_3_faq_debugging_drivers", [
        [ "8.3.1. Should I use wddebug_gui or wddebug?", "ch8_debugging_drivers.html#ch8_3_1_should_i_use_wddebug_gui_or_wddebug", null ],
        [ "8.3.2. Can I debug WinDriver-based code easily using MS Visual Studio (Visual C++)?", "ch8_debugging_drivers.html#ch8_3_2_can_i_debug_windriver-based_code_easily_using_ms_visual_studio_visual_cpp", null ],
        [ "8.3.3. How would you recommend debugging a WinDriver-based application?", "ch8_debugging_drivers.html#ch8_3_3_how_would_you_recommend_debugging_a_windriver-based_application", null ]
      ] ]
    ] ],
    [ "Chapter 9: Enhanced Support for Specific Chipsets", "ch9_enhanced_support_for_specific_chipsets.html", [
      [ "9.1. Enhanced Support for Specific Chipsets Overview", "ch9_enhanced_support_for_specific_chipsets.html#ch9_1_enhanced_support_for_specific_chipsets_overview", null ],
      [ "9.2. Developing a Driver Using the Enhanced Chipset Support", "ch9_enhanced_support_for_specific_chipsets.html#ch9_2_developing_a_driver_using_the_enhanced_chipset_support", null ],
      [ "9.3. The XDMA sample code", "ch9_enhanced_support_for_specific_chipsets.html#ch9_3_the_xdma_sample_code", [
        [ "9.3.1. Performing Direct Memory Access (DMA) tests", "ch9_enhanced_support_for_specific_chipsets.html#ch9_3_1_performing_direct_memory_access_dma_tests", null ],
        [ "9.3.2. The XDMA GUI utility", "ch9_enhanced_support_for_specific_chipsets.html#ch9_3_2_the_xdma_gui_utility", null ],
        [ "9.3.3. XDMA code generation in DriverWizard", "ch9_enhanced_support_for_specific_chipsets.html#ch9_3_3_xdma_code_generation_in_driverwizard", null ]
      ] ],
      [ "9.4. The QDMA sample code", "ch9_enhanced_support_for_specific_chipsets.html#ch9_4_the_qdma_sample_code", [
        [ "9.4.1. Performing Direct Memory Access (DMA) transaction", "ch9_enhanced_support_for_specific_chipsets.html#ch9_4_1_performing_direct_memory_access_dma_transaction", null ],
        [ "9.4.2. Changing between physical functions", "ch9_enhanced_support_for_specific_chipsets.html#ch9_4_2_changing_between_physical_functions", null ],
        [ "9.4.3. Requests info", "ch9_enhanced_support_for_specific_chipsets.html#ch9_4_3_requests_info", null ],
        [ "9.4.4. QDMA code generation in DriverWizard", "ch9_enhanced_support_for_specific_chipsets.html#ch9_4_4_qdma_code_generation_in_driverwizard", null ]
      ] ],
      [ "9.5. The Avalon-MM sample code", "ch9_enhanced_support_for_specific_chipsets.html#ch9_5_the_avalon-mm_sample_code", [
        [ "9.5.1. Avalon-MM code generation in DriverWizard", "ch9_enhanced_support_for_specific_chipsets.html#ch9_5_1_avalon-mm_code_generation_in_driverwizard", null ]
      ] ]
    ] ],
    [ "Chapter 10: PCI Advanced Features", "ch10_pci_advanced_features.html", [
      [ "10.1. Handling Interrupts", "ch10_pci_advanced_features.html#ch10_1_handling_interrupts", [
        [ "10.1.1. Interrupt Handling — Overview", "ch10_pci_advanced_features.html#ch10_1_1_interrupt_handling_overview", null ],
        [ "10.1.2. WinDriver Interrupt Handling Sequence", "ch10_pci_advanced_features.html#ch10_1_2_windriver_interrupt_handling_sequence", null ],
        [ "10.1.3. Registering IRQs for Non-Plug-and-Play Hardware", "ch10_pci_advanced_features.html#ch10_1_3_registering_irqs_for_non-plug-and-play_hardware", null ],
        [ "10.1.4. Determining the Interrupt Types Supported by the Hardware", "ch10_pci_advanced_features.html#ch10_1_4_determining_the_interrupt_types_supported_by_the_hardware", null ],
        [ "10.1.5. Determining the Interrupt Type Enabled for a PCI Card", "ch10_pci_advanced_features.html#ch10_1_5_determining_the_interrupt_type_enabled_for_a_pci_card", null ],
        [ "10.1.6. Setting Up Kernel-Mode Interrupt Transfer Commands", "ch10_pci_advanced_features.html#ch10_1_6_setting_up_kernel-mode_interrupt_transfer_commands", [
          [ "10.1.6.1. Interrupt Mask Commands", "ch10_pci_advanced_features.html#ch10_1_6_1_interrupt_mask_commands", null ],
          [ "10.1.6.2. Sample WinDriver Transfer Commands Code", "ch10_pci_advanced_features.html#ch10_1_6_2_sample_windriver_transfer_commands_code", null ]
        ] ],
        [ "10.1.7. WinDriver MSI/MSI-X Interrupt Handling", "ch10_pci_advanced_features.html#ch10_1_7_windriver_msi-msi-x_interrupt_handling", [
          [ "10.1.7.1. Windows MSI/MSI-X Device INF Files", "ch10_pci_advanced_features.html#ch10_1_7_1_windows_msi-msi-x_device_inf_files", null ]
        ] ],
        [ "10.1.8. Sample User-Mode WinDriver Interrupt Handling Code", "ch10_pci_advanced_features.html#ch10_1_8_sample_user-mode_windriver_interrupt_handling_code", null ]
      ] ],
      [ "10.2. Reserving and locking physical memory on Windows and Linux", "ch10_pci_advanced_features.html#ch10_2_reserving_and_locking_physical_memory_on_windows_and_linux", null ],
      [ "10.3. Buffer sharing between multiple processes", "ch10_pci_advanced_features.html#ch10_3_buffer_sharing_between_multiple_processes", null ],
      [ "10.4. Single Root I/O Virtualization (SR-IOV)", "ch10_pci_advanced_features.html#ch10_4_single_root_i-o_virtualization_sr-iov", [
        [ "10.4.1. Introduction", "ch10_pci_advanced_features.html#ch10_4_1_introduction_single_root_i_o_virtualization_sr-iov", null ],
        [ "10.4.2. Using SR-IOV with WinDriver", "ch10_pci_advanced_features.html#ch10_4_2_using_sr-iov_with_windriver", null ]
      ] ],
      [ "10.5. Using WinDriver's IPC APIs", "ch10_pci_advanced_features.html#ch10_5_using_windrivers_ipc_apis", [
        [ "10.5.1 IPC Overview", "ch10_pci_advanced_features.html#ch10_5_1_ipc_overview", null ],
        [ "10.5.2. Shared Interrupts via IPC", "ch10_pci_advanced_features.html#ch10_5_2_shared_interrupts_via_ipc", [
          [ "10.5.2.1. Enabling Shared Interrupts:", "ch10_pci_advanced_features.html#ch10_5_2_1_enabling_shared_interrupts", null ],
          [ "10.5.2.2. Disabling Shared Interrupts:", "ch10_pci_advanced_features.html#ch10_5_2_2_disabling_shared_interrupts", null ]
        ] ]
      ] ],
      [ "10.6. FAQ", "ch10_pci_advanced_features.html#ch10_6_faq_pci_advanced_features", [
        [ "10.6.1. What is the significance of marking a resource as 'shared' with WinDriver, and how can I verify the 'shared' status of a specific resource on my card?", "ch10_pci_advanced_features.html#ch10_6_1_what_is_the_significance_of_marking_a_resource_as_shared_with_windriver_and_how_can_i_verify_the_shared_status_of_a_specific_resource_on_my_card", null ],
        [ "10.6.2. Can I access the same device, simultaneously, from several WinDriver applications?", "ch10_pci_advanced_features.html#ch10_6_2_can_i_access_the_same_device_simultaneously_from_several_windriver_applications", null ],
        [ "10.6.3. How can I read the value of the PCI interrupt status register from my WinDriver ISR, in order to determine, for example, which card generated the interrupt when the IRQ is shared between several devices?", "ch10_pci_advanced_features.html#ch10_6_3_how_can_i_read_the_value_of_the_pci_interrupt_status_register_from_my_windriver_isr_in_order_to_determine_for_example_which_card_generated_the_interrupt_when_the_irq_is_shared_between_several_devices", null ],
        [ "10.6.4. I need to be able to count the number of interrupts occurring and possibly call a routine every time an interrupt occurs. Is this possible with WinDriver?", "ch10_pci_advanced_features.html#ch10_6_4_i_need_to_be_able_to_count_the_number_of_interrupts_occurring_and_possibly_call_a_routine_every_time_an_interrupt_occurs_is_this_possible_with_windriver", null ],
        [ "10.6.5. Does WinDriver poll the interrupt (Busy Wait)?", "ch10_pci_advanced_features.html#ch10_6_5_does_windriver_poll_the_interrupt_busy_wait", null ],
        [ "10.6.6. Can I write to disk files during an interrupt routine?", "ch10_pci_advanced_features.html#ch10_6_6_can_i_write_to_disk_files_during_an_interrupt_routine", null ]
      ] ]
    ] ],
    [ "Chapter 11: Improving PCI Performance", "ch11_improving_pci_performance.html", [
      [ "11.1. Improving PCI Performance Overview", "ch11_improving_pci_performance.html#ch11_1_improving_pci_performance_overview", [
        [ "11.1.1. Performance Improvement Checklist", "ch11_improving_pci_performance.html#ch11_1_1_performance_improvement_checklist", null ],
        [ "11.1.2 PCI Transfers Overview", "ch11_improving_pci_performance.html#ch11_1_2_pci_transfers_overview", null ],
        [ "11.1.3. Improving the Performance of a User-Mode Driver", "ch11_improving_pci_performance.html#ch11_1_3_improving_the_performance_of_a_user-mode_driver", [
          [ "11.1.3.1. Using Direct Access to Memory-Mapped Regions", "ch11_improving_pci_performance.html#ch11_1_3_1_using_direct_access_to_memory-mapped_regions", null ],
          [ "11.1.3.2. Block Transfers and Grouping Multiple Transfers (Burst Transfer)", "ch11_improving_pci_performance.html#ch11_1_3_2_block_transfers_and_grouping_multiple_transfers_burst_transfer", null ],
          [ "11.1.3.3. Performing 64-Bit Data Transfers", "ch11_improving_pci_performance.html#ch11_1_3_3_performing_64-bit_data_transfers", null ]
        ] ]
      ] ],
      [ "11.2. Performing Direct Memory Access (DMA)", "ch11_improving_pci_performance.html#ch11_2_performing_direct_memory_access_dma", [
        [ "11.2.1. Implementing Scatter/Gather DMA", "ch11_improving_pci_performance.html#ch11_2_1_implementing_scatter-gather_dma", [
          [ "11.2.1.1. C Example", "ch11_improving_pci_performance.html#ch11_2_1_1_c_example_implementing_scatter-gather_dma", null ],
          [ "11.2.1.2. C# Example", "ch11_improving_pci_performance.html#ch11_2_1_2_c-sharp_example_implementing_scatter-gather_dma", null ],
          [ "11.2.1.3. What Should You Implement?", "ch11_improving_pci_performance.html#ch11_2_1_3_what_should_you_implement_implementing_scatter_gather_dma", null ]
        ] ],
        [ "11.2.2. Implementing Contiguous-Buffer DMA", "ch11_improving_pci_performance.html#ch11_2_2_implementing_contiguous-buffer_dma", [
          [ "11.2.2.1. C Example", "ch11_improving_pci_performance.html#ch11_2_2_1_c_example_implementing_contiguous-buffer_dma", null ],
          [ "11.2.2.2. C# Example", "ch11_improving_pci_performance.html#ch11_2_2_2_c-sharp_example_implementing_contiguous-buffer_dma", null ],
          [ "11.2.2.3. What Should You Implement?", "ch11_improving_pci_performance.html#ch11_2_2_3_what_should_you_implement_implementing_contiguous-buffer_dma", null ],
          [ "11.2.2.4. Preallocating Contiguous DMA Buffers on Windows", "ch11_improving_pci_performance.html#ch11_2_2_4_preallocating_contiguous_dma_buffers_on_windows", null ]
        ] ]
      ] ],
      [ "11.3. Performing Direct Memory Access (DMA) transactions", "ch11_improving_pci_performance.html#ch11_3_performing_direct_memory_access_dma_transactions", [
        [ "11.3.1. Implementing Scatter/Gather DMA transactions", "ch11_improving_pci_performance.html#ch11_3_1_implementing_scatter-gather_dma_transactions", [
          [ "11.3.1.1. C Example", "ch11_improving_pci_performance.html#ch11_3_1_1_c_example_implementing_scatter-gather_dma_transactions", null ],
          [ "11.3.1.2. C# Example", "ch11_improving_pci_performance.html#ch11_3_1_2_c-sharp_example_implementing_scatter-gather_dma_transactions", null ],
          [ "11.3.1.3. What Should You Implement?", "ch11_improving_pci_performance.html#ch11_3_1_3_what_should_you_implement_implementing_scatter-gather_dma_transactions", null ]
        ] ],
        [ "11.3.2. Implementing Contiguous-Buffer DMA transactions", "ch11_improving_pci_performance.html#ch11_3_2_implementing_contiguous-buffer_dma_transactions", [
          [ "11.3.2.1. C Example", "ch11_improving_pci_performance.html#ch11_3_2_1_c_example_implementing_contiguous-buffer_dma_transactions", null ],
          [ "11.3.2.2. C# Example", "ch11_improving_pci_performance.html#ch11_3_2_2_c-sharp_example_implementing_contiguous-buffer_dma_transactions", null ],
          [ "11.3.2.3 What Should You Implement?", "ch11_improving_pci_performance.html#ch11_3_2_3_what_should_you_implement_implementing_contiguous-buffer_dma_transactions", null ]
        ] ]
      ] ],
      [ "11.4. DMA between PCI devices and NVIDIA GPUs with GPUDirect (Linux only)", "ch11_improving_pci_performance.html#ch11_4_dma_between_pci_devices_and_nvidia_gpus_with_gpudirect_linux_only", [
        [ "11.4.1. What is GPUDirect for RDMA?", "ch11_improving_pci_performance.html#ch11_4_1_what_is_gpudirect_for_rdma", null ],
        [ "11.4.2. System Requirements", "ch11_improving_pci_performance.html#ch11_4_2_system_requirements_dma_between_pci_devices_and_nvidia_gpus_with_gpudirect", null ],
        [ "11.4.3. Software Prerequisites", "ch11_improving_pci_performance.html#ch11_4_3_software_prerequisites", null ],
        [ "11.4.4. WinDriver installation", "ch11_improving_pci_performance.html#ch11_4_4_windriver_installation", null ],
        [ "11.4.5. Moving DMA from CPU to GPU", "ch11_improving_pci_performance.html#ch11_4_5_moving_dma_from_cpu_to_gpu", null ],
        [ "11.4.6. Modify Compilation", "ch11_improving_pci_performance.html#ch11_4_6_modify_compilation", null ],
        [ "11.4.7. Modify your code", "ch11_improving_pci_performance.html#ch11_4_7_modify_your_code", null ],
        [ "11.4.8. CMake Example", "ch11_improving_pci_performance.html#ch11_4_8_cmake_example", null ]
      ] ],
      [ "11.5. FAQ", "ch11_improving_pci_performance.html#ch11_5_faq_improving_pci_performance", [
        [ "11.5.1. How do I perform system or slave DMA using WinDriver?", "ch11_improving_pci_performance.html#ch11_5_1_how_do_i_perform_system_or_slave_dma_using_windriver", null ],
        [ "11.5.2. I have locked a memory buffer for DMA on Windows. Now, when I access this memory directly, using the user-mode pointer, it seems to be 5 times slower than accessing a “regular” memory buffer, allocated with malloc(). Why?", "ch11_improving_pci_performance.html#ch11_5_2_i_have_locked_a_memory_buffer_for_dma_on_windows_now_when_i_access_this_memory_directly_using_the_user-mode_pointer_it_seems_to_be_5_times_slower_than_accessing_a_regular_memory_buffer_allocated_with_malloc_why", null ],
        [ "11.5.3. My attempt to allocate and lock a 1GB DMA buffer with WinDriver on Windows fails. Is this a limitation of the operating system?", "ch11_improving_pci_performance.html#ch11_5_3_my_attempt_to_allocate_and_lock_a_1gb_dma_buffer_with_windriver_on_windows_fails_is_this_a_limitation_of_the_operating_system", null ],
        [ "11.5.4. How do I perform PCI DMA Writes from system memory to my card, using WinDriver?", "ch11_improving_pci_performance.html#ch11_5_4_how_do_i_perform_pci_dma_writes_from_system_memory_to_my_card_using_windriver", null ],
        [ "11.5.5. How do I perform Direct Block transfers from one PCI card to another?", "ch11_improving_pci_performance.html#ch11_5_5_how_do_i_perform_direct_block_transfers_from_one_pci_card_to_another", null ]
      ] ]
    ] ],
    [ "Chapter 12: Understanding the Kernel PlugIn", "ch12_understanding_the_kernel_plugin.html", [
      [ "12.1. Background", "ch12_understanding_the_kernel_plugin.html#ch12_1_background", null ],
      [ "12.2. Expected Performance", "ch12_understanding_the_kernel_plugin.html#ch12_2_expected_performance", null ],
      [ "12.3. Overview of the Development Process", "ch12_understanding_the_kernel_plugin.html#ch12_3_overview_of_the_development_process", null ],
      [ "12.4. The Kernel PlugIn Architecture", "ch12_understanding_the_kernel_plugin.html#ch12_4_the_kernel_plugin_architecture", [
        [ "12.4.1. Architecture Overview", "ch12_understanding_the_kernel_plugin.html#ch12_4_1_architecture_overview", null ],
        [ "12.4.2. WinDriver's Kernel and Kernel PlugIn Interaction", "ch12_understanding_the_kernel_plugin.html#ch12_4_2_windrivers_kernel_and_kernel_plugin_interaction", null ],
        [ "12.4.3. Kernel PlugIn Components", "ch12_understanding_the_kernel_plugin.html#ch12_4_3_kernel_plugin_components", null ],
        [ "12.4.4. Kernel PlugIn Event Sequence", "ch12_understanding_the_kernel_plugin.html#ch12_4_4_kernel_plugin_event_sequence", [
          [ "12.4.4.1. Opening a Handle from the User Mode to a Kernel PlugIn Driver", "ch12_understanding_the_kernel_plugin.html#ch12_4_4_1_opening_a_handle_from_the_user_mode_to_a_kernel_plugin_driver", null ],
          [ "12.4.4.2. Handling User-Mode Requests from the Kernel PlugIn", "ch12_understanding_the_kernel_plugin.html#ch12_4_4_2_handling_user-mode_requests_from_the_kernel_plugin", null ],
          [ "12.4.4.3. Interrupt Handling — Enable/Disable and High Interrupt Request Level Processing", "ch12_understanding_the_kernel_plugin.html#ch12_4_4_3_interrupt_handling_—_enable-disable_and_high_interrupt_request_level_processing", null ],
          [ "12.4.4.4. Interrupt Handling — Deferred Procedure Calls Event/Callback Notes", "ch12_understanding_the_kernel_plugin.html#ch12_4_4_4_interrupt_handling_—_deferred_procedure_calls_event-callback_notes", null ],
          [ "12.4.4.5. Plug-and-Play and Power Management Events", "ch12_understanding_the_kernel_plugin.html#ch12_4_4_5_plug-and-play_and_power_management_events", null ]
        ] ]
      ] ],
      [ "12.5. How Does Kernel PlugIn Work?", "ch12_understanding_the_kernel_plugin.html#ch12_5_how_does_kernel_plugin_work", [
        [ "12.5.1. Minimal Requirements for Creating a Kernel PlugIn Driver", "ch12_understanding_the_kernel_plugin.html#ch12_5_1_minimal_requirements_for_creating_a_kernel_plugin_driver", null ],
        [ "12.5.2. Kernel PlugIn Implementation", "ch12_understanding_the_kernel_plugin.html#ch12_5_2_kernel_plugin_implementation", [
          [ "12.5.2.1. Before You Begin", "ch12_understanding_the_kernel_plugin.html#ch12_5_2_1_before_you_begin", null ],
          [ "12.5.2.2. Write Your KP_Init Function", "ch12_understanding_the_kernel_plugin.html#ch12_5_2_2_write_your_kp_init_function", null ],
          [ "12.5.2.3. Write Your KP_Open Function(s)", "ch12_understanding_the_kernel_plugin.html#ch12_5_2_3_write_your_kp_open_functions", null ],
          [ "12.5.2.4. Write the Remaining PlugIn Callbacks", "ch12_understanding_the_kernel_plugin.html#ch12_5_2_4_write_the_remaining_plugin_callbacks", null ]
        ] ],
        [ "12.5.3. Sample/Generated Kernel PlugIn Driver Code Overview", "ch12_understanding_the_kernel_plugin.html#ch12_5_3_sample-generated_kernel_plugin_driver_code_overview", null ],
        [ "12.5.4. Kernel PlugIn Sample/Generated Code Directory Structure", "ch12_understanding_the_kernel_plugin.html#ch12_5_4_kernel_plugin_sample-generated_code_directory_structure", [
          [ "12.5.4.1. pci_diag and kp_pci Sample Directories", "ch12_understanding_the_kernel_plugin.html#ch12_5_4_1_pci_diag_and_kp_pci_sample_directories", null ],
          [ "12.5.4.2. Xilinx BMD Kernel PlugIn Directory Structure", "ch12_understanding_the_kernel_plugin.html#ch12_5_4_2_xilinx_bmd_kernel_plugin_directory_structure", null ],
          [ "12.5.4.3. The Generated DriverWizard Kernel PlugIn Directory", "ch12_understanding_the_kernel_plugin.html#ch12_5_4_3_the_generated_driverwizard_kernel_plugin_directory", null ]
        ] ],
        [ "12.5.5. Handling Interrupts in the Kernel PlugIn", "ch12_understanding_the_kernel_plugin.html#ch12_5_5_handling_interrupts_in_the_kernel_plugin", [
          [ "12.5.5.1. Interrupt Handling in the User Mode (Without the Kernel PlugIn)", "ch12_understanding_the_kernel_plugin.html#ch12_5_5_1_interrupt_handling_in_the_user_mode_without_the_kernel_plugin", null ],
          [ "12.5.5.2. Interrupt Handling in the Kernel (Using the Kernel PlugIn)", "ch12_understanding_the_kernel_plugin.html#ch12_5_5_2_interrupt_handling_in_the_kernel_using_the_kernel_plugin", null ]
        ] ],
        [ "12.5.6. Message Passing", "ch12_understanding_the_kernel_plugin.html#ch12_5_6_message_passing", null ]
      ] ],
      [ "12.6. FAQ", "ch12_understanding_the_kernel_plugin.html#ch12_6_faq_understanding_the_kernel_plugin", [
        [ "12.6.1. Why does my WD_KernelPlugInOpen() call fail?", "ch12_understanding_the_kernel_plugin.html#ch12_6_1_why_does_my_wd_kernelpluginopen_call_fail", null ],
        [ "12.6.2. When handling my interrupts entirely in the Kernel PlugIn, can I erase the interrupt handler in the user mode?", "ch12_understanding_the_kernel_plugin.html#ch12_6_2_when_handling_my_interrupts_entirely_in_the_kernel_plugin_can_i_erase_the_interrupt_handler_in_the_user_mode", null ],
        [ "12.6.3. How can I print debug statements from the Kernel PlugIn that I can view using a kernel debugger, such as WinDbg?", "ch12_understanding_the_kernel_plugin.html#ch12_6_3_how_can_i_print_debug_statements_from_the_kernel_plugin_that_i_can_view_using_a_kernel_debugger_such_as_windbg", null ],
        [ "12.6.4. My PC hangs while closing my application. The code fails in WD_IntDisable(). Why is this happening? I am using the Kernel PlugIn to handle interrupts.", "ch12_understanding_the_kernel_plugin.html#ch12_6_4_my_pc_hangs_while_closing_my_application_the_code_fails_in_wd_intdisable_why_is_this_happening_i_am_using_the_kernel_plugin_to_handle_interrupts", null ]
      ] ]
    ] ],
    [ "Chapter 13: Creating a Kernel PlugIn Driver", "ch13_creating_a_kernel_plugin_driver.html", [
      [ "13.1. Determine Whether a Kernel PlugIn is Needed", "ch13_creating_a_kernel_plugin_driver.html#ch13_1_determine_whether_a_kernel_plugin_is_needed", null ],
      [ "13.2. What programming languages can be used with a Kernel PlugIn?", "ch13_creating_a_kernel_plugin_driver.html#ch13_2_what_programming_languages_can_be_used_with_a_kernel_plugin", null ],
      [ "13.3. Prepare the User-Mode Source Code", "ch13_creating_a_kernel_plugin_driver.html#ch13_3_prepare_the_user-mode_source_code", null ],
      [ "13.4. Create a New Kernel PlugIn Project", "ch13_creating_a_kernel_plugin_driver.html#ch13_4_create_a_new_kernel_plugin_project", null ],
      [ "13.5. Open a Handle to the Kernel PlugIn", "ch13_creating_a_kernel_plugin_driver.html#ch13_5_open_a_handle_to_the_kernel_plugin", null ],
      [ "13.6. Set Interrupt Handling in the Kernel PlugIn", "ch13_creating_a_kernel_plugin_driver.html#ch13_6_set_interrupt_handling_in_the_kernel_plugin", null ],
      [ "13.7. Set I/O Handling in the Kernel PlugIn", "ch13_creating_a_kernel_plugin_driver.html#ch13_7_set_i-o_handling_in_the_kernel_plugin", null ],
      [ "13.8. Compile Your Kernel PlugIn Driver", "ch13_creating_a_kernel_plugin_driver.html#ch13_8_compile_your_kernel_plugin_driver", [
        [ "13.8.1. Windows Kernel PlugIn Driver Compilation", "ch13_creating_a_kernel_plugin_driver.html#ch13_8_1_windows_kernel_plugin_driver_compilation", null ],
        [ "13.8.2. Linux Kernel PlugIn Driver Compilation", "ch13_creating_a_kernel_plugin_driver.html#ch13_8_2_linux_kernel_plugin_driver_compilation", null ],
        [ "13.8.3. Porting a Kernel PlugIn project developed prior to version 10.3.0, to support working with a 32-bit user-mode application and a 64-bit Kernel PlugIn driver", "ch13_creating_a_kernel_plugin_driver.html#ch13_8_3_porting_a_kernel_plugin_project_developed_prior_to_version_10_3_0_to_support_working_with_a_32-bit_user-mode_application_and_a_64-bit_kernel_plugin_driver", null ]
      ] ],
      [ "13.9. Install Your Kernel PlugIn Driver", "ch13_creating_a_kernel_plugin_driver.html#ch13_9_install_your_kernel_plugin_driver", [
        [ "13.9.1. Windows Kernel PlugIn Driver Installation", "ch13_creating_a_kernel_plugin_driver.html#ch13_9_1_windows_kernel_plugin_driver_installation", null ],
        [ "13.9.2. Linux Kernel PlugIn Driver Installation", "ch13_creating_a_kernel_plugin_driver.html#ch13_9_2_linux_kernel_plugin_driver_installation", null ]
      ] ],
      [ "13.10. FAQ", "ch13_creating_a_kernel_plugin_driver.html#ch13_10_faq_creating_a_kernel_plugin_driver", [
        [ "13.10.1. I would like to execute in the kernel some pieces of code written in languages other than C/C++ (Python/Java/C#/Visual Basic.NET), using the Kernel PlugIn. Is it possible?", "ch13_creating_a_kernel_plugin_driver.html#ch13_10_1_i_would_like_to_execute_in_the_kernel_some_pieces_of_code_written_in_languages_other_than_c-cpp_python-java-c-sharp-visual_basic_dotnet_using_the_kernel_plugin_is_it_possible", null ],
        [ "13.10.2. How do I allocate locked memory in the kernel that can be used inside the interrupt handler?", "ch13_creating_a_kernel_plugin_driver.html#ch13_10_2_how_do_i_allocate_locked_memory_in_the_kernel_that_can_be_used_inside_the_interrupt_handler", null ],
        [ "13.10.3. How do I handle shared PCI interrupts in the Kernel PlugIn?", "ch13_creating_a_kernel_plugin_driver.html#ch13_10_3_how_do_i_handle_shared_pci_interrupts_in_the_kernel_plugin", null ],
        [ "13.10.4. What is pIntContext in the Kernel PlugIn interrupt functions?", "ch13_creating_a_kernel_plugin_driver.html#ch13_10_4_what_is_pintcontext_in_the_kernel_plugin_interrupt_functions", null ],
        [ "13.10.5. I need to call WD_Transfer() in the Kernel PlugIn. From where do I get hWD to pass to these functions?", "ch13_creating_a_kernel_plugin_driver.html#ch13_10_5_i_need_to_call_wd_transfer_in_the_kernel_plugin_from_where_do_i_get_hwd_to_pass_to_these_functions", null ],
        [ "13.10.6. A restriction in KP_IntAtIrql is to use only non-pageable memory. What does this mean?", "ch13_creating_a_kernel_plugin_driver.html#ch13_10_6_a_restriction_in_kp_intatirql_is_to_use_only_non-pageable_memory_what_does_this_mean", null ],
        [ "13.10.7. How do I call WD_Transfer() in the Kernel PlugIn interrupt handler?", "ch13_creating_a_kernel_plugin_driver.html#ch13_10_7_how_do_i_call_wd_transfer_in_the_kernel_plugin_interrupt_handler", null ],
        [ "13.10.8. How do I share a memory buffer between Kernel PlugIn and user-mode projects for DMA or other purposes?", "ch13_creating_a_kernel_plugin_driver.html#ch13_10_8_how_do_i_share_a_memory_buffer_between_kernel_plugin_and_user-mode_projects_for_dma_or_other_purposes", null ],
        [ "13.10.9. If I write a new function in my SYS Kernel PlugIn driver, must it also be declared with __cdecl?", "ch13_creating_a_kernel_plugin_driver.html#ch13_10_9_if_i_write_a_new_function_in_my_sys_kernel_plugin_driver_must_it_also_be_declared_with___cdecl", null ]
      ] ]
    ] ],
    [ "Chapter 14: USB Advanced Features", "ch14_usb_advanced_features.html", [
      [ "14.1. USB Control Transfers", "ch14_usb_advanced_features.html#ch14_1_usb_control_transfers", null ],
      [ "14.2. USB Control Transfers Overview", "ch14_usb_advanced_features.html#ch14_2_usb_control_transfers_overview", [
        [ "14.2.1. Control Data Exchange", "ch14_usb_advanced_features.html#ch14_2_1_control_data_exchange", null ],
        [ "14.2.2. More About the Control Transfer", "ch14_usb_advanced_features.html#ch14_2_2_more_about_the_control_transfer", null ],
        [ "14.2.3. The Setup Packet", "ch14_usb_advanced_features.html#ch14_2_3_the_setup_packet", null ],
        [ "14.2.4. USB Setup Packet Format", "ch14_usb_advanced_features.html#ch14_2_4_usb_setup_packet_format", null ],
        [ "14.2.5. Standard Device Request Codes", "ch14_usb_advanced_features.html#ch14_2_5_standard_device_request_codes", null ],
        [ "14.2.6. Setup Packet Example", "ch14_usb_advanced_features.html#ch14_2_6_setup_packet_example", null ]
      ] ],
      [ "14.3. Performing Control Transfers with WinDriver", "ch14_usb_advanced_features.html#ch14_3_performing_control_transfers_with_windriver", [
        [ "14.3.1. Control Transfers with DriverWizard", "ch14_usb_advanced_features.html#ch14_3_1_control_transfers_with_driverwizard", null ],
        [ "14.3.2. Control Transfers with WinDriver API", "ch14_usb_advanced_features.html#ch14_3_2_control_transfers_with_windriver_api", null ]
      ] ],
      [ "14.4. Functional USB Data Transfers", "ch14_usb_advanced_features.html#ch14_4_functional_usb_data_transfers", [
        [ "14.4.1. Functional USB Data Transfers Overview", "ch14_usb_advanced_features.html#ch14_4_1_functional_usb_data_transfers_overview", null ],
        [ "14.4.2. Single-Blocking Transfers", "ch14_usb_advanced_features.html#ch14_4_2_single-blocking_transfers", [
          [ "14.4.2.1. Performing Single-Blocking Transfers with WinDriver", "ch14_usb_advanced_features.html#ch14_4_2_1_performing_single-blocking_transfers_with_windriver", null ]
        ] ],
        [ "14.4.3.Streaming Data Transfers {#ch14_4_3.streaming_data_transfers}", "ch14_usb_advanced_features.html#autotoc_md0", [
          [ "14.4.3.1. Performing Streaming with WinDriver", "ch14_usb_advanced_features.html#ch14_4_3_1_performing_streaming_with_windriver", null ]
        ] ]
      ] ],
      [ "14.5. FAQ", "ch14_usb_advanced_features.html#ch14_5_faq_usb_advanced_features", [
        [ "14.5.1. Buffer Overrun Error: WDU_Transfer() sometimes returns the 0xC000000C error code. What does this error code mean? How do I solve this problem?", "ch14_usb_advanced_features.html#ch14_5_1_buffer_overrun_error_wdu_transfer_sometimes_returns_the_0xc000000c_error_code_what_does_this_error_code_mean_how_do_i_solve_this_problem", null ],
        [ "14.5.2. How do I extract the string descriptors contained in the Device and Configuration descriptor tables?", "ch14_usb_advanced_features.html#ch14_5_2_how_do_i_extract_the_string_descriptors_contained_in_the_device_and_configuration_descriptor_tables", null ],
        [ "14.5.3. How do I detect that a USB device has been plugged in or disconnected?", "ch14_usb_advanced_features.html#ch14_5_3_how_do_i_detect_that_a_usb_device_has_been_plugged_in_or_disconnected", null ],
        [ "14.5.4. How do I setup the transfer buffer to send a null data packet through the control pipe?", "ch14_usb_advanced_features.html#ch14_5_4_how_do_i_setup_the_transfer_buffer_to_send_a_null_data_packet_through_the_control_pipe", null ],
        [ "14.5.5. Can I write a driver for a USB hub or a USB Host Controller card using WinDriver?", "ch14_usb_advanced_features.html#ch14_5_5_can_i_write_a_driver_for_a_usb_hub_or_a_usb_host_controller_card_using_windriver", null ],
        [ "14.5.6. Does WinDriver USB support isochronous streaming mode?", "ch14_usb_advanced_features.html#ch14_5_6_does_windriver_usb_support_isochronous_streaming_mode", null ]
      ] ]
    ] ],
    [ "Chapter 15: Distributing Your Driver", "ch15_distributing_your_driver.html", [
      [ "15.1. Getting a Valid WinDriver License", "ch15_distributing_your_driver.html#ch15_1_getting_a_valid_windriver_license", null ],
      [ "15.2. Windows Driver Distribution", "ch15_distributing_your_driver.html#ch15_2_windows_driver_distribution", [
        [ "15.2.1. Preparing the Distribution Package", "ch15_distributing_your_driver.html#ch15_2_1_preparing_the_distribution_package_windows", null ],
        [ "15.2.2. Installing Your Driver on the Target Computer", "ch15_distributing_your_driver.html#ch15_2_2_installing_your_driver_on_the_target_computer", null ],
        [ "15.2.3. Installing Your Kernel PlugIn on the Target Computer", "ch15_distributing_your_driver.html#ch15_2_3_installing_your_kernel_plugin_on_the_target_computer", null ],
        [ "15.2.4. Redistribute Your WinDriver-based package as a self-extracting EXE", "ch15_distributing_your_driver.html#ch15_2_4_redistribute_your_windriver-based_package_as_a_self-extracting_exe", [
          [ "15.2.4.1. The Installer", "ch15_distributing_your_driver.html#ch15_2_4_1_the_installer_windows", null ],
          [ "15.2.4.2. Requirements", "ch15_distributing_your_driver.html#ch15_2_4_2_requirements_windows", null ],
          [ "15.2.4.3. Instructions", "ch15_distributing_your_driver.html#ch15_2_4_3_instructions_windows", null ]
        ] ]
      ] ],
      [ "15.3. Linux Driver Distribution", "ch15_distributing_your_driver.html#ch15_3_linux_driver_distribution", [
        [ "15.3.1. Preparing the Distribution Package", "ch15_distributing_your_driver.html#ch15_3_1_preparing_the_distribution_package_linux", [
          [ "15.3.1.1. Kernel Module Components", "ch15_distributing_your_driver.html#ch15_3_1_1_kernel_module_components", null ],
          [ "15.3.1.2. User-Mode Hardware-Control Application or Shared Object", "ch15_distributing_your_driver.html#ch15_3_1_2_user-mode_hardware-control_application_or_shared_object", null ]
        ] ],
        [ "15.3.2. Building and Installing the WinDriver Driver Module on the Target", "ch15_distributing_your_driver.html#ch15_3_2_building_and_installing_the_windriver_driver_module_on_the_target", null ],
        [ "15.3.3. Building and Installing Your Kernel PlugIn Driver on the Target", "ch15_distributing_your_driver.html#ch15_3_3_building_and_installing_your_kernel_plugin_driver_on_the_target", null ],
        [ "15.3.4. Installing the User-Mode Hardware-Control Application or Shared Object", "ch15_distributing_your_driver.html#ch15_3_4_installing_the_user-mode_hardware-control_application_or_shared_object", null ],
        [ "15.3.5. Redistribute Your WinDriver-based package as a self-extracting SH (STGZ)", "ch15_distributing_your_driver.html#ch15_3_5_redistribute_your_windriver-based_package_as_a_self-extracting_sh_stgz", [
          [ "15.3.5.1. The Installer", "ch15_distributing_your_driver.html#ch15_3_5_1_the_installer_linux", null ],
          [ "15.3.5.2. Requirements", "ch15_distributing_your_driver.html#ch15_3_5_2_requirements_linux", null ],
          [ "15.3.5.3. Instructions", "ch15_distributing_your_driver.html#ch15_3_5_3_instructions_linux", null ]
        ] ]
      ] ]
    ] ],
    [ "Chapter 16: Dynamically Loading Your Driver", "ch16_dynamically_loading_your_driver.html", [
      [ "16.1. Why Do You Need a Dynamically Loadable Driver?", "ch16_dynamically_loading_your_driver.html#ch16_1_why_do_you_need_a_dynamically_loadable_driver", null ],
      [ "16.2. Windows Dynamic Driver Loading", "ch16_dynamically_loading_your_driver.html#ch16_2_windows_dynamic_driver_loading", [
        [ "16.2.1. The wdreg Utility", "ch16_dynamically_loading_your_driver.html#ch16_2_1_the_wdreg_utility", [
          [ "16.2.1.1. WDM Drivers", "ch16_dynamically_loading_your_driver.html#ch16_2_1_1_wdm_drivers_the_wdreg_utility", null ],
          [ "16.2.1.2. Non-WDM Drivers", "ch16_dynamically_loading_your_driver.html#ch16_2_1_2_non-wdm_drivers", null ]
        ] ],
        [ "16.2.2. Dynamically Loading/Unloading windrvr1511.sys INF Files", "ch16_dynamically_loading_your_driver.html#ch16_2_2_dynamically_loading-unloading_windrvr_sys_inf_files", null ],
        [ "16.2.3. Dynamically Loading/Unloading Your Kernel PlugIn Driver", "ch16_dynamically_loading_your_driver.html#ch16_2_3_dynamically_loading-unloading_your_kernel_plugin_driver_windows", null ]
      ] ],
      [ "16.3. The wdreg_frontend utility", "ch16_dynamically_loading_your_driver.html#ch16_3_the_wdreg_frontend_utility", null ],
      [ "16.4. Linux Dynamic Driver Loading", "ch16_dynamically_loading_your_driver.html#ch16_4_linux_dynamic_driver_loading", [
        [ "16.4.1. Dynamically Loading/Unloading Your Kernel PlugIn Driver", "ch16_dynamically_loading_your_driver.html#ch16_4_1_dynamically_loading-unloading_your_kernel_plugin_driver_linux", null ]
      ] ]
    ] ],
    [ "Chapter 17: Driver Installation — Advanced Issues", "ch17_driver_installation_advanced_issues.html", [
      [ "17.1. Windows INF Files", "ch17_driver_installation_advanced_issues.html#ch17_1_windows_inf_files", [
        [ "17.1.1. Why Should I Create an INF File?", "ch17_driver_installation_advanced_issues.html#ch17_1_1_why_should_i_create_an_inf_file", null ],
        [ "17.1.2. How Do I Install an INF File When No Driver Exists?", "ch17_driver_installation_advanced_issues.html#ch17_1_2_how_do_i_install_an_inf_file_when_no_driver_exists", null ],
        [ "17.1.3. How Do I Replace an Existing Driver Using the INF File?", "ch17_driver_installation_advanced_issues.html#ch17_1_3_how_do_i_replace_an_existing_driver_using_the_inf_file", null ]
      ] ],
      [ "17.2. Renaming the WinDriver Kernel Driver", "ch17_driver_installation_advanced_issues.html#ch17_2_renaming_the_windriver_kernel_driver", [
        [ "17.2.1. Windows Driver Renaming", "ch17_driver_installation_advanced_issues.html#ch17_2_1_windows_driver_renaming", [
          [ "17.2.1.1 Rebuilding with Custom Version Information (Windows)", "ch17_driver_installation_advanced_issues.html#ch17_2_1_1_rebuilding_with_custom_version_information_windows", null ]
        ] ],
        [ "17.2.2. Linux Driver Renaming", "ch17_driver_installation_advanced_issues.html#ch17_2_2_linux_driver_renaming", null ]
      ] ],
      [ "17.3. Windows Digital Driver Signing and Certification", "ch17_driver_installation_advanced_issues.html#ch17_3_windows_digital_driver_signing_and_certification", [
        [ "17.3.1. Windows Digital Driver Signing and Certification Overview", "ch17_driver_installation_advanced_issues.html#ch17_3_1_windows_digital_driver_signing_and_certification_overview", [
          [ "17.3.1.1. Authenticode Driver Signature", "ch17_driver_installation_advanced_issues.html#ch17_3_1_1_authenticode_driver_signature", null ],
          [ "17.3.1.2. Windows Certification Program", "ch17_driver_installation_advanced_issues.html#ch17_3_1_2_windows_certification_program", null ]
        ] ],
        [ "17.3.2. Driver Signing and Certification of WinDriver-Based Drivers", "ch17_driver_installation_advanced_issues.html#ch17_3_2_driver_signing_and_certification_of_windriver-based_drivers", [
          [ "17.3.2.1. HCK/HLK Test Notes", "ch17_driver_installation_advanced_issues.html#ch17_3_2_1_hck-hlk_test_notes", null ]
        ] ],
        [ "17.3.3. Secure Boot and Driver Development", "ch17_driver_installation_advanced_issues.html#ch17_3_3_secure_boot_and_driver_development", [
          [ "17.3.3.1. Through System Information", "ch17_driver_installation_advanced_issues.html#ch17_3_3_1_through_system_information", null ],
          [ "17.3.3.2. Through PowerShell", "ch17_driver_installation_advanced_issues.html#ch17_3_3_2_through_powershell", null ],
          [ "17.3.3.3. Through WinDriver User-Mode API", "ch17_driver_installation_advanced_issues.html#ch17_3_3_3_through_windriver_user-mode_api", null ],
          [ "17.3.3.4. Disabling Secure Boot", "ch17_driver_installation_advanced_issues.html#ch17_3_3_4_disabling_secure_boot", null ]
        ] ],
        [ "17.3.4. Temporary disabling digital signature enforcement in Windows 10", "ch17_driver_installation_advanced_issues.html#ch17_3_4_temporary_disabling_digital_signature_enforcement_in_windows_10", null ]
      ] ]
    ] ],
    [ "Data Structures", "annotated.html", [
      [ "Data Structures", "annotated.html", "annotated_dup" ],
      [ "Data Structure Index", "classes.html", null ],
      [ "Data Fields", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", null ],
        [ "Variables", "functions_vars.html", "functions_vars" ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "Globals", "globals.html", [
        [ "All", "globals.html", "globals_dup" ],
        [ "Functions", "globals_func.html", null ],
        [ "Typedefs", "globals_type.html", null ],
        [ "Enumerations", "globals_enum.html", null ],
        [ "Enumerator", "globals_eval.html", "globals_eval" ],
        [ "Macros", "globals_defs.html", "globals_defs" ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"annotated.html",
"ch1_overview.html#ch1_4_supported_platforms",
"functions_vars_h.html",
"pci__regs_8h_a56ac6caf3d8634eed814dc58e8b5a025.html#a56ac6caf3d8634eed814dc58e8b5a025",
"pci__regs_8h_af099ee48d83d3d16bda02aa1d8c14f5b.html#af099ee48d83d3d16bda02aa1d8c14f5b",
"structWD__DMA__PAGE_a3b430f83710faf4328d7040733f1445e.html#a3b430f83710faf4328d7040733f1445e",
"wd__ver_8h_ae81d591c2caf11bef195a4a296927d1c.html#ae81d591c2caf11bef195a4a296927d1c",
"windrvr_8h_a0dd4286b8b46d5f0414a071e08f99b34.html#a0dd4286b8b46d5f0414a071e08f99b34",
"windrvr_8h_a5a8ad1609207945d35bbd4624b55cb97.html#a5a8ad1609207945d35bbd4624b55cb97acaf5960a1fd5c34b8692efca57d706bb",
"windrvr__usb_8h_a4aeef81faa1a87f8e1aeb019671ef73d.html#a4aeef81faa1a87f8e1aeb019671ef73daea7abd19ac068ad6aa0eccf376dfca6d"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';