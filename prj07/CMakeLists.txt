CMAKE_MINIMUM_REQUIRED(VERSION 3.5.1)
PROJECT(prj07)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR})
SET(LIBIP_SRCS arp.c arpcache.c icmp.c ip_base.c packet.c rtable.c rtable_internal.c)
ADD_LIBRARY(libipstack STATIC ${LIBIP_SRCS})

LINK_DIRECTORIES(${CMAKE_SOURCE_DIR})
#LINK_LIBRARIES(libipstack.a)
ADD_EXECUTABLE(router main.c ip.c)
TARGET_LINK_LIBRARIES(router libipstack.a pthread)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g")
