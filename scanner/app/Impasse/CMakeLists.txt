cmake_minimum_required(VERSION 3.9)
project(Impasse)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_CXX_FLAGS -O3)

link_libraries(pthread)

add_executable(Impasse main.cpp Module.cpp PrinterDevice.cpp)