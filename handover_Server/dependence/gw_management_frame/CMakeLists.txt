cmake_minimum_required(VERSION 3.5)

option(PUBLIC "This is a default option for PUBLIC" ON)
message(STATUS "build public platform switch: "${PUBLIC})
project(Mylibrary)


if(${PUBLIC} STREQUAL "ON")
    message(STATUS "start to build ARM platform.")
	# cross complie
	SET(CMAKE_SYSTEM_NAME Linux)
	SET(CMAKE_SYSTEM_PROCESSOR arm) 
	SET(TOOLCHAIN_DIR "/home/gyl/petalinux/tools/linux-i386/gcc-arm-linux-gnueabi")
	SET(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_DIR})
	SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
	SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

	SET(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/bin/arm-linux-gnueabihf-gcc-6.2.1)
	SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/bin/arm-linux-gnueabihf-g++)
	# cross complie
	link_directories("${PROJECT_SOURCE_DIR}/../lib_arm")
	# zlog
	link_directories("/home/gyl/liqingSpace/code/GW_arm_server/temp/lib/")
	include_directories("/home/gyl/liqingSpace/code/GW_arm_server/temp/include/")
elseif(${PUBLIC} STREQUAL "OFF")
    message(STATUS "start to build PC platform.")
	link_directories("${PROJECT_SOURCE_DIR}/../lib")
endif(${PUBLIC} STREQUAL "ON")

SET(CMAKE_BUILD_TYPE "Debug")  
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")  
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
SET(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/..")


SET(LIB_NAME gw_frame)
SET(CMAKE_VERBOSE_MAKEFILE ON)    
SET(ROOT_PATH ".")  

SET(ALL_LIBRARY_SRCS gw_frame.c)

add_library(${LIB_NAME} SHARED ${ALL_LIBRARY_SRCS})

include_directories("${PROJECT_SOURCE_DIR}/../include/")

target_link_libraries(${LIB_NAME} -lzlog)
target_link_libraries(${LIB_NAME} gw_utility)


if(${PUBLIC} STREQUAL "ON")
    message(STATUS "start to install ARM platform library.")
	install(TARGETS ${LIB_NAME} LIBRARY DESTINATION lib_arm/)
elseif(${PUBLIC} STREQUAL "OFF")
    message(STATUS "start to build PC platform library.")
	install(TARGETS ${LIB_NAME} LIBRARY DESTINATION lib/)
endif(${PUBLIC} STREQUAL "ON")

install(FILES ${PROJECT_SOURCE_DIR}/gw_frame.h DESTINATION include/)


