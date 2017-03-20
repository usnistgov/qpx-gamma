cmake_minimum_required(VERSION 3.1)
project(consumers CXX)

#add_subdirectory(../engine)

file(GLOB_RECURSE consumers_SOURCES *.cpp)
file(GLOB_RECURSE consumers_HEADERS *.h)
foreach (_headerFile ${consumers_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND consumers_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES consumers_INCLUDE_DIRS)

include_directories(
  ${engine_INCLUDE_DIRS}
  ${consumers_INCLUDE_DIRS}
)

add_library(
  ${PROJECT_NAME}
  ${consumers_SOURCES}
)

target_link_libraries(
  ${PROJECT_NAME}
#  engine
)

# Required on Unix OS family to be able to be linked into shared libraries.
set_target_properties(${PROJECT_NAME}
                      PROPERTIES POSITION_INDEPENDENT_CODE ON
                      )

# Expose public includes (including Boost transitively) to other
# subprojects through cache variable.
set(${PROJECT_NAME}_INCLUDE_DIRS
#    ${engine_INCLUDE_DIRS}
    ${consumers_INCLUDE_DIRS}
CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
