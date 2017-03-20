cmake_minimum_required(VERSION 3.1)
project(producer_simulator2d CXX)

file(GLOB_RECURSE simulator2d_SOURCES *.cpp)
file(GLOB_RECURSE simulator2d_HEADERS *.h)
foreach (_headerFile ${simulator2d_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND simulator2d_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES simulator2d_INCLUDE_DIRS)

include_directories(
  ${engine_INCLUDE_DIRS}
  ${simulator2d_INCLUDE_DIRS}
)

add_library(
  ${PROJECT_NAME}
  ${simulator2d_SOURCES}
)

target_link_libraries(
  ${PROJECT_NAME}
)
