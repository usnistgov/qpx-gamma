cmake_minimum_required(VERSION 3.1)
project(producer_parser_evt CXX)

file(GLOB_RECURSE parser_evt_SOURCES *.cpp)
file(GLOB_RECURSE parser_evt_HEADERS *.h)
foreach (_headerFile ${parser_evt_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND parser_evt_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES parser_evt_INCLUDE_DIRS)

include_directories(
  ${engine_INCLUDE_DIRS}
  ${producer_vme_INCLUDE_DIRS}
  ${parser_evt_INCLUDE_DIRS}
)

add_library(
  ${PROJECT_NAME}
  ${parser_evt_SOURCES}
  ${vme_SOURCES}
)

target_link_libraries(
  ${PROJECT_NAME}
  ${vme_SOURCES}
)

# Required on Unix OS family to be able to be linked into shared libraries.
set_target_properties(${PROJECT_NAME}
                      PROPERTIES POSITION_INDEPENDENT_CODE ON
                      )

# Expose public includes (including Boost transitively) to other
# subprojects through cache variable.
set(${PROJECT_NAME}_INCLUDE_DIRS
    ${parser_evt_INCLUDE_DIRS}
CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
