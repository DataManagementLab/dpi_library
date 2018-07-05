cmake_minimum_required(VERSION 2.8)

project(dpi)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# Compiler Options
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLINUX -DDEBUG -std=c++11 -Wall -g  -Wextra ")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLINUX -DDEBUG -std=c++11 -Wall -O3  -Wextra ")

# the compiler flags for compiling C++ sources 
MESSAGE( STATUS "CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS} )

add_subdirectory(src)

file(COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/src/conf/DPI.conf
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin/conf/
)
