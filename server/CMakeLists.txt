cmake_minimum_required(VERSION 2.8)
project(myserver)

include_directories(../shared/)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -std=c++17 -pthread")

set(SRC_LIST sources/main.cpp sources/server.cpp sources/daemon.cpp sources/myservice.cpp sources/srvapi.cpp sources/expression.cpp ../shared/logger.cpp sources/threadpool.cpp sources/mysettings.cpp ../shared/cfgparser.cpp ../shared/socket.cpp ../shared/packsock.cpp ../shared/csnet_api.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../bin)

add_executable(${PROJECT_NAME} ${SRC_LIST})

#target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
