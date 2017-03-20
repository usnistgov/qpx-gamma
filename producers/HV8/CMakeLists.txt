cmake_minimum_required(VERSION 3.1)
project(producer_hv8 CXX)

file(GLOB_RECURSE hv8_SOURCES *.cpp)
file(GLOB_RECURSE hv8_HEADERS *.h)
foreach (_headerFile ${hv8_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND hv8_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES hv8_INCLUDE_DIRS)

include_directories(
  ${engine_INCLUDE_DIRS}
  ${hv8_INCLUDE_DIRS}
)

add_library(
  ${PROJECT_NAME}
  ${hv8_SOURCES}
)

target_link_libraries(
  ${PROJECT_NAME}
)

# Required on Unix OS family to be able to be linked into shared libraries.
set_target_properties(${PROJECT_NAME}
                      PROPERTIES POSITION_INDEPENDENT_CODE ON
                      )

# Expose public includes (including Boost transitively) to other
# subprojects through cache variable.
set(${PROJECT_NAME}_INCLUDE_DIRS
    ${hv8_INCLUDE_DIRS}
CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
