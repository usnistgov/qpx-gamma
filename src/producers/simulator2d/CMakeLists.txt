cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)
project(producer_simulator2d CXX)

file(GLOB_RECURSE ${PROJECT_NAME}_SOURCES *.cpp)
file(GLOB_RECURSE ${PROJECT_NAME}_HEADERS *.h)
dirs_of(${PROJECT_NAME}_INCLUDE_DIRS "${${PROJECT_NAME}_HEADERS}")

add_library(
  ${PROJECT_NAME}
  ${${PROJECT_NAME}_SOURCES}
  ${${PROJECT_NAME}_HEADERS}
)

include_directories(
  ${PROJECT_NAME}
  PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS}
  PRIVATE ${engine_INCLUDE_DIRS}
)

target_link_libraries(
  ${PROJECT_NAME}
#  ${engine_LIBRARIES}
)
