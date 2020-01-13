cmake_minimum_required(VERSION 3.9)
project(Fork)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")

link_libraries(pthread)
link_libraries(/home/waterlemon/jpeg/libjpeg-turbo-2.0.1/a64-64bit/a64/libjpeg.a)
link_libraries(/home/waterlemon/jpeg/libjpeg-turbo-2.0.1/a64-64bit/a64/libturbojpeg.a)

aux_source_directory(./module DIR_MODULE)
include_directories(./module)

aux_source_directory(./scanner DIR_SCANNER)
include_directories(./scanner)

aux_source_directory(./scanner_ext DIR_SCANNER_EXT)
include_directories(./scanner_ext)

include_directories(.)
add_executable(Fork main.cpp main.h Switch.cpp Switch.h
        ${DIR_MODULE}
        ${DIR_SCANNER}
        ${DIR_SCANNER_EXT})
