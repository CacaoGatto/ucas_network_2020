cmake_minimum_required (VERSION 3.5.2)

project (prj03)

add_executable(server server.c)
add_executable(client client.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
