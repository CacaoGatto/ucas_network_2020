cmake_minimum_required (VERSION 3.5.1)

project (prj14)

include_directories(./include)

add_executable(tcp_stack arp.c arpcache.c icmp.c ip.c main.c packet.c rtable.c rtable_internal.c tcp.c tcp_apps.c tcp_in.c tcp_out.c tcp_sock.c tcp_timer.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g")

TARGET_LINK_LIBRARIES(tcp_stack pthread)
