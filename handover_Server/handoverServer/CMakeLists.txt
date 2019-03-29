cmake_minimum_required(VERSION 3.5)

SET(CMAKE_BUILD_TYPE "Debug")  
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")  
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
#add_definitions(-std=c++11)


include_directories("../dependence/include")
link_directories("../dependence/lib")
link_directories("/usr/local/lib")

project(handoverServer)

SET(CMAKE_VERBOSE_MAKEFILE ON)    
SET(ROOT_PATH ".")


file(GLOB_RECURSE ALL_LIBRARY_HDRS "include/*.h")
file(GLOB_RECURSE ALL_LIBRARY_SRCS "src/*.cpp")


file(GLOB_RECURSE ALL_EXECUTABLES "*_main.cpp")

add_executable(${PROJECT_NAME} ${ALL_EXECUTABLES} ${ALL_LIBRARY_SRCS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/dependence/include/>
)

target_link_libraries(${PROJECT_NAME} -levent)
target_link_libraries(${PROJECT_NAME} -pthread)
target_link_libraries(${PROJECT_NAME} -ldl)
target_link_libraries(${PROJECT_NAME} -lglog)
target_link_libraries(${PROJECT_NAME} cjson)
target_link_libraries(${PROJECT_NAME} gwtunnel)
target_link_libraries(${PROJECT_NAME} gw_utility)



