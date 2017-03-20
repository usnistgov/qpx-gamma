cmake_minimum_required(VERSION 3.1)
project(producer_parser_raw CXX)

file(GLOB_RECURSE parser_raw_SOURCES *.cpp)
file(GLOB_RECURSE parser_raw_HEADERS *.h)
foreach (_headerFile ${parser_raw_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND parser_raw_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES parser_raw_INCLUDE_DIRS)

include_directories(
  ${engine_INCLUDE_DIRS}
  ${parser_raw_INCLUDE_DIRS}
)

add_library(
  ${PROJECT_NAME}
  ${parser_raw_SOURCES}
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
    ${parser_raw_INCLUDE_DIRS}
CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
