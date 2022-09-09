cmake_minimum_required(VERSION 3.0)
project(UsbDiag)

include(../../../../../include/wd.cmake)
find_package(Java REQUIRED)

if (WIN32)
	set(WDAPI_CLASSPATH "\".\;${WD_BASEDIR}/lib/wdapi_java${WD_VERSION}.jar\"")
	set(WDAPI_LIB_SUBDIR ${PLAT_DIR})
else()
	set(WDAPI_CLASSPATH ".:${WD_BASEDIR}/lib/wdapi_java${WD_VERSION}.jar")
endif()

add_custom_target(UsbDiag ALL
    COMMAND ${Java_JAVAC_EXECUTABLE} -cp ${WDAPI_CLASSPATH} com/jungo/UsbDiag/UsbDiag.java
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/../../../
)

if(NOT TARGET run)
	add_custom_target(run
        COMMAND ${Java_JAVA_EXECUTABLE} -cp ${WDAPI_CLASSPATH} 
			-Djava.library.path=${WD_BASEDIR}/lib/${WDAPI_LIB_SUBDIR} com/jungo/UsbDiag/UsbDiag
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/../../../
    )
add_dependencies(run UsbDiag)
endif()

