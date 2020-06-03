CMAKE_MINIMUM_REQUIRED(VERSION 3.5.1)
PROJECT(prj07)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR})
LINK_DIRECTORIES(${CMAKE_SOURCE_DIR})
#LINK_LIBRARIES(libipstack.a)
ADD_EXECUTABLE(mospfd ip.c main.c mospf_database.c mospf_daemon.c mospf_proto.c packet.c)
TARGET_LINK_LIBRARIES(mospfd libipstack.a pthread)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g")
