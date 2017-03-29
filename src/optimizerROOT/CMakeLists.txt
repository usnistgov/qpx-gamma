cmake_minimum_required(VERSION 3.1)
project(optimizerROOT CXX)

#ROOT
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT REQUIRED COMPONENTS Hist)
include(${ROOT_USE_FILE})


file(GLOB_RECURSE optimizerROOT_SOURCES *.cpp)
file(GLOB_RECURSE optimizerROOT_HEADERS *.h)
foreach (_headerFile ${optimizerROOT_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND optimizerROOT_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES optimizerROOT_INCLUDE_DIRS)

include_directories(
  ${engine_INCLUDE_DIRS}
  ${ROOT_INCLUDE_DIR}
  ${optimizerROOT_INCLUDE_DIRS}
)

add_library(
  ${PROJECT_NAME}
  ${optimizerROOT_SOURCES}
)

target_link_libraries(
  ${PROJECT_NAME}
  ${ROOT_LIBRARIES}
)

set(${PROJECT_NAME}_LIBRARIES
    -Wl,--whole-archive ${PROJECT_NAME} -Wl,--no-whole-archive
    PARENT_SCOPE)
