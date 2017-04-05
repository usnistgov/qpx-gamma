cmake_minimum_required(VERSION 3.1)
project(optimizer_ceres CXX)

#Ceres
#list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(Ceres REQUIRED)
include_directories(${CERES_INCLUDE_DIRS})

file(GLOB_RECURSE optimizer_ceres_SOURCES *.cpp)
file(GLOB_RECURSE optimizer_ceres_HEADERS *.h)
foreach (_headerFile ${optimizer_ceres_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND optimizer_ceres_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES optimizer_ceres_INCLUDE_DIRS)

include_directories(
  ${engine_INCLUDE_DIRS}
  ${CERES_INCLUDE_DIRS}
  ${optimizer_ceres_INCLUDE_DIRS}
)

add_library(
  ${PROJECT_NAME}
  ${optimizer_ceres_SOURCES}
)

target_link_libraries(
  ${PROJECT_NAME}
  ${CERES_LIBRARIES}
)

set(${PROJECT_NAME}_LIBRARIES
    -Wl,--whole-archive ${PROJECT_NAME} -Wl,--no-whole-archive
    PARENT_SCOPE)
