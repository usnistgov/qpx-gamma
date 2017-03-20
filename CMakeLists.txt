cmake_minimum_required(VERSION 3.1)
project(qpx_gamma CXX)

set (CMAKE_CXX_STANDARD 11)
add_definitions(-DBOOST_LOG_DYN_LINK)

add_subdirectory(engine)
add_subdirectory(consumers)
add_subdirectory(producers)
add_subdirectory(cmd)
add_subdirectory(gui)
