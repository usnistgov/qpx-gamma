cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)
project(h5cc CXX)

# HDF5
set(HDF5_USE_SHARED_LIBRARIES ON)
FIND_PACKAGE(HDF5 1.8.15 COMPONENTS CXX HL REQUIRED)
add_library(HDF5 INTERFACE IMPORTED)
set_property(TARGET HDF5 PROPERTY
    INTERFACE_INCLUDE_DIRECTORIES ${HDF5_INCLUDE_DIRS})
#link_directories ( ${HDF5_LIBRARY_DIRS} )

file(GLOB ${PROJECT_NAME}_sources *.cpp h5cc/source/h5cc/*.cpp)
file(GLOB ${PROJECT_NAME}_headers *.h *.tpp
  h5cc/source/h5cc/*.h
  h5cc/source/h5cc/*.tpp)
dirs_of(${PROJECT_NAME}_include_dirs "${${PROJECT_NAME}_headers}")

#message(STATUS "sources   " ${${PROJECT_NAME}_sources})
#message(STATUS "headers   " ${${PROJECT_NAME}_headers})
#message(STATUS "inc dir   " ${${PROJECT_NAME}_include_dirs})

add_library(
  ${PROJECT_NAME}
  ${${PROJECT_NAME}_sources}
  ${${PROJECT_NAME}_headers}
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${${PROJECT_NAME}_include_dirs}
  PUBLIC ${HDF5_INCLUDE_DIRS}
  PRIVATE ${json_INCLUDE_DIRS}
  )

target_link_libraries(
  ${PROJECT_NAME}
  ${HDF5_LIBRARIES}
)

set(${PROJECT_NAME}_LIBRARIES
    ${PROJECT_NAME}
    PARENT_SCOPE)
