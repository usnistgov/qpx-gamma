cmake_minimum_required(VERSION 3.1)
project(engine CXX)

#root
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

if (QPX_USE_HDF5)
  set(HDF5_USE_SHARED_LIBRARIES ON)
  find_package(HDF5 COMPONENTS CXX REQUIRED)
  add_definitions(-DH5_ENABLED)

  file(GLOB_RECURSE h5_SOURCES ../h5/*.cpp)
  file(GLOB_RECURSE h5_TEMPLATES ../h5/*.tpp)
  file(GLOB_RECURSE h5_HEADERS ../h5/*.h)
  foreach (_headerFile ${h5_HEADERS})
      get_filename_component(_dir ${_headerFile} PATH)
      list (APPEND h5_INCLUDE_DIRS ${_dir})
  endforeach()
  list(REMOVE_DUPLICATES h5_INCLUDE_DIRS)
endif()

find_package(Boost COMPONENTS system filesystem thread timer date_time 
             program_options log log_setup regex REQUIRED)

find_package(ROOT REQUIRED COMPONENTS RIO Net)

#ROOT needs this
include(${ROOT_USE_FILE})

link_directories( ${BOOST_LIBRARY_DIRS} )

execute_process(COMMAND
  git rev-parse HEAD
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE GIT_SHA1
  OUTPUT_STRIP_TRAILING_WHITESPACE)

add_definitions(-DGIT_VERSION="${GIT_SHA1}")

file(GLOB_RECURSE engine_SOURCES *.cpp)
file(GLOB_RECURSE engine_HEADERS *.h *.hpp)
foreach (_headerFile ${engine_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND engine_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES engine_INCLUDE_DIRS)

include_directories(
  ${engine_INCLUDE_DIRS}
  ${h5_INCLUDE_DIRS}
  ${HDF5_INCLUDE_DIRS}
  ${ROOT_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
)

add_library(
  ${PROJECT_NAME}
  ${engine_SOURCES}
  ${h5_SOURCES}
  ${h5_TEMPLATES}
)

target_link_libraries(
  ${PROJECT_NAME}
  ${HDF5_LIBRARIES}
  ${Boost_LIBRARIES}
  ${ROOT_LIBRARIES}
)

# Expose public includes
set(${PROJECT_NAME}_INCLUDE_DIRS
	  ${engine_INCLUDE_DIRS}
    ${h5_INCLUDE_DIRS}
    ${HDF5_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${ROOT_INCLUDE_DIR}
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

set(${PROJECT_NAME}_LIBRARIES
    -Wl,--whole-archive ${PROJECT_NAME} -Wl,--no-whole-archive
    PARENT_SCOPE)
