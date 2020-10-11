cmake_minimum_required(VERSION 2.8)
project(myclient)

include_directories(../shared/)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -pthread -std=c++17")

set(SRC_LIST sources/main.cpp sources/mysettings.cpp sources/clnapi.cpp sources/clnapi.cpp ../shared/cfgparser.cpp ../shared/socket.cpp ../shared/packsock.cpp ../shared/csnet_api.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../bin)

add_executable(${PROJECT_NAME} ${SRC_LIST})

#target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
