cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)
project(engine CXX)

# Build time in UTC ISO 8601
FILE (WRITE ${CMAKE_BINARY_DIR}/compiletime.cmake "STRING(TIMESTAMP TIMEZ UTC)\n")
FILE (APPEND ${CMAKE_BINARY_DIR}/compiletime.cmake "FILE(WRITE compiletime.h \"#ifndef TIMESTAMP_H\\n\")\n")
FILE (APPEND ${CMAKE_BINARY_DIR}/compiletime.cmake "FILE(APPEND compiletime.h \"#define TIMESTAMP_H\\n\\n\")\n")
FILE (APPEND ${CMAKE_BINARY_DIR}/compiletime.cmake "FILE(APPEND compiletime.h \"#define _TIMEZ_ \\\"\${TIMEZ}\\\"\\n\\n\")\n")
FILE (APPEND ${CMAKE_BINARY_DIR}/compiletime.cmake "FILE(APPEND compiletime.h \"#endif // TIMESTAMP_H\\n\")\n")
ADD_CUSTOM_TARGET (
    compiletime
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/compiletime.cmake
    ADD_DEPENDENCIES ${CMAKE_BINARY_DIR}/compiletime.cmake)

# Git commit SHA1
execute_process(COMMAND
    git rev-parse HEAD
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_SHA1
    OUTPUT_STRIP_TRAILING_WHITESPACE)

add_definitions(-DGIT_VERSION="${GIT_SHA1}")
add_definitions(-DCMAKE_SYSTEM="${CMAKE_SYSTEM}")
add_definitions(-DCMAKE_SYSTEM_PROCESSOR="${CMAKE_SYSTEM_PROCESSOR}")

# Boost
SET(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS
  system filesystem thread timer date_time
  log log_setup regex REQUIRED)
#link_directories( ${BOOST_LIBRARY_DIRS} )

FIND_PACKAGE ( Threads REQUIRED )

file(GLOB_RECURSE ${PROJECT_NAME}_SOURCES *.cpp)
file(GLOB_RECURSE ${PROJECT_NAME}_HEADERS *.h *.hpp)
dirs_of(${PROJECT_NAME}_INCLUDE_DIRS "${${PROJECT_NAME}_HEADERS}")

add_library(
  ${PROJECT_NAME}
  ${${PROJECT_NAME}_SOURCES}
  ${${PROJECT_NAME}_HEADERS}
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS}
  PUBLIC ${PROJECT_BINARY_DIR}
  PUBLIC ${h5cc_INCLUDE_DIRS}
  PUBLIC ${json_INCLUDE_DIRS}
  PUBLIC ${Boost_INCLUDE_DIRS}
)

target_link_libraries(
  ${PROJECT_NAME}
  ${h5cc_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

ADD_DEPENDENCIES(${PROJECT_NAME} compiletime)

message(STATUS "H5CC LIBS ===== " ${h5cc_INCLUDE_DIRS})
message(STATUS "json LIBS ===== " ${json_INCLUDE_DIRS})

# Expose public includes
set(${PROJECT_NAME}_INCLUDE_DIRS
    ${${PROJECT_NAME}_INCLUDE_DIRS}
    ${h5cc_INCLUDE_DIRS}
    ${json_INCLUDE_DIRS}
CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

set(${PROJECT_NAME}_LIBRARIES
    -Wl,--whole-archive ${PROJECT_NAME} -Wl,--no-whole-archive
    PARENT_SCOPE)
