cmake_minimum_required (VERSION 3.5.1)

project (prj04)

include_directories(./include)

add_executable(hub main.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g")
