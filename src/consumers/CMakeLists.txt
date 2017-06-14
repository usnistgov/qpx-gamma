cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)
project(consumers CXX)

file(GLOB ${PROJECT_NAME}_SOURCES *.cpp xylib/xylib/*.cpp)
file(GLOB ${PROJECT_NAME}_HEADERS *.h xylib/xylib/*.h)
dirs_of(${PROJECT_NAME}_INCLUDE_DIRS "${${PROJECT_NAME}_HEADERS}")

add_library(
  ${PROJECT_NAME}
  ${consumers_SOURCES}
)

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE ${engine_INCLUDE_DIRS}
  PRIVATE ${consumers_INCLUDE_DIRS}
  PRIVATE ${h5cc_INCLUDE_DIRS}
)

target_link_libraries(
  ${PROJECT_NAME}
  ${h5cc_LIBRARIES}
)

set(${PROJECT_NAME}_LIBRARIES
    -Wl,--whole-archive ${PROJECT_NAME} -Wl,--no-whole-archive
    PARENT_SCOPE)
