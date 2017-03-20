cmake_minimum_required(VERSION 3.1)
project(consumers CXX)

file(GLOB_RECURSE consumers_SOURCES *.cpp)
file(GLOB_RECURSE consumers_HEADERS *.h)
foreach (_headerFile ${consumers_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND consumers_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES consumers_INCLUDE_DIRS)

include_directories(
  ${engine_INCLUDE_DIRS}
  ${consumers_INCLUDE_DIRS}
)

add_library(
  ${PROJECT_NAME}
  ${consumers_SOURCES}
)

target_link_libraries(
  ${PROJECT_NAME}
)

set(${PROJECT_NAME}_LIBRARIES
    -Wl,--whole-archive ${PROJECT_NAME} -Wl,--no-whole-archive
    PARENT_SCOPE)
