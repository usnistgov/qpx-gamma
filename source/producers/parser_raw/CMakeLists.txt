cmake_minimum_required(VERSION 3.1)
project(producer_parser_raw CXX)

file(GLOB_RECURSE parser_raw_SOURCES *.cpp)
file(GLOB_RECURSE parser_raw_HEADERS *.h)
foreach (_headerFile ${parser_raw_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND parser_raw_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES parser_raw_INCLUDE_DIRS)

include_directories(
  ${engine_INCLUDE_DIRS}
  ${parser_raw_INCLUDE_DIRS}
)

add_library(
  ${PROJECT_NAME}
  ${parser_raw_SOURCES}
)

target_link_libraries(
  ${PROJECT_NAME}
)
