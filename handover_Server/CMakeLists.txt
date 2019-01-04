cmake_minimum_required(VERSION 3.5)

SET(CMAKE_BUILD_TYPE "Debug")  
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")  
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
#add_definitions(-std=c++11)

include_directories("./dependence/include")
link_directories("./dependence/lib")

project(handoverServer)

SET(LIB_NAME mylib)
SET(CMAKE_VERBOSE_MAKEFILE ON)    
SET(ROOT_PATH ".")

find_package(Protobuf REQUIRED)

file(GLOB_RECURSE ALL_LIBRARY_HDRS "include/*.h")
file(GLOB_RECURSE ALL_LIBRARY_SRCS "src/*.cpp")


add_library(${LIB_NAME} STATIC ${ALL_LIBRARY_HDRS} ${ALL_LIBRARY_SRCS})

target_include_directories(${LIB_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/dependence/include/>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(${LIB_NAME} -lglog)
target_link_libraries(${LIB_NAME} -ldl)
target_link_libraries(${LIB_NAME} cjson)

file(GLOB_RECURSE ALL_EXECUTABLES "*_main.cpp")
#message(STATUS "ALL_EXECUTABLES : ${ALL_EXECUTABLES}")

add_executable(${PROJECT_NAME} ${ALL_EXECUTABLES})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/dependence/include/>
)


TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_NAME})
target_link_libraries(${PROJECT_NAME} -levent)
target_link_libraries(${PROJECT_NAME} -pthread)
target_link_libraries(${PROJECT_NAME} -lglog)
target_link_libraries(${PROJECT_NAME} cjson)




