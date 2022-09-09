cmake_minimum_required(VERSION 3.0)

project(kp_bmd)

include(../../../../../include/wd.cmake)

if (NOT "${KERNEL_SRC}" STREQUAL "")
   set(WITH_KERNEL_SRC "--with-kernel-source=${KERNEL_SRC}")
endif()

add_custom_target(kp_bmd_module ALL
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMAND bash -c "sudo ./configure ${WITH_KERNEL_SRC}"
    COMMAND bash -c "sudo WD_BASEDIR=${WD_BASEDIR} make install")
