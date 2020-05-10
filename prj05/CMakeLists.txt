cmake_minimum_required (VERSION 3.5.1)

project (prj05)

include_directories(./include)

add_executable(switch main.c mac.c packet.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -lpthread")
