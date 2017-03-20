cmake_minimum_required(VERSION 3.1)
project(cpx CXX)

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
  ${engine_LIBRARIES}
  ${consumers_LIBRARIES}
  ${producers_LIBRARIES}
)
