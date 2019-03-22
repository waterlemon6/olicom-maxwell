cmake_minimum_required(VERSION 3.9)
project(Map C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)

add_executable(Map main.c main.h)