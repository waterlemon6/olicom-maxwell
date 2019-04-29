cmake_minimum_required(VERSION 3.9)
project(compress C)

set(CMAKE_C_STANDARD 11)

add_executable(compress main.c Compress.c)