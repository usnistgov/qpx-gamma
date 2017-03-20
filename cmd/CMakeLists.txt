cmake_minimum_required(VERSION 3.1)
project(cpx CXX)

#set (CMAKE_CXX_STANDARD 11)
#add_definitions(-DBOOST_LOG_DYN_LINK)

file(GLOB_RECURSE cmd_SOURCES *.cpp)
file(GLOB_RECURSE cmd_HEADERS *.h)
foreach (_headerFile ${cmd_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND cmd_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES cmd_INCLUDE_DIRS)

include_directories(
  ${engine_INCLUDE_DIRS}
  ${cmd_INCLUDE_DIRS}
)

add_executable(
  ${PROJECT_NAME}
  ${cmd_SOURCES}
)

target_link_libraries(
  ${PROJECT_NAME}
  engine
  consumers
  producer_simulator2d
  producer_parser_raw
  producer_vme
  producer_parser_evt
  producer_hv8
  producer_pixie4
)
