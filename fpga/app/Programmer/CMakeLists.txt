cmake_minimum_required(VERSION 3.9)
project(Programmer C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)

add_executable(Programmer main.c KernelInterface.c jb_jtag.c jrunner.c)