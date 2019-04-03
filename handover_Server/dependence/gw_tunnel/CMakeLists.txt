cmake_minimum_required(VERSION 3.5)

project(Mylibrary)

message(STATUS "start to build PC platform.")

SET(CMAKE_BUILD_TYPE "Debug")  
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")  
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
SET(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/..")


SET(LIB_NAME gw_tunnel)
SET(CMAKE_VERBOSE_MAKEFILE ON)    
SET(ROOT_PATH ".")  

SET(ALL_LIBRARY_SRCS gw_tunnel.c)

add_library(${LIB_NAME} SHARED ${ALL_LIBRARY_SRCS})

message(STATUS "start to build PC platform library.")
install(TARGETS ${LIB_NAME} LIBRARY DESTINATION lib/)

install(FILES ${PROJECT_SOURCE_DIR}/gw_tunnel.h ${PROJECT_SOURCE_DIR}/If_tunnel.h DESTINATION include/)

