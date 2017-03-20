cmake_minimum_required(VERSION 3.1)
project(producer_pixie4 CXX)

if(UNIX)
  add_definitions(-DXIA_LINUX)
  add_definitions(-DPLX_LINUX)
  set(CMAKE_CXX_FLAGS "-fpermissive -w")
endif()

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    MESSAGE( "64 bits compiler detected" )
    SET( EX_PLATFORM 64 )
    SET( EX_PLATFORM_NAME "x64" )
else( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    MESSAGE( "32 bits compiler detected" )
    SET( EX_PLATFORM 32 )
    SET( EX_PLATFORM_NAME "x86" )
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )


IF( EX_PLATFORM EQUAL 64 )
link_directories( ${PROJECT_SOURCE_DIR}/PLX/Library/64bit )
SET (PLX_LIB_DIRECTORY ${PROJECT_SOURCE_DIR}/PLX/Library/64bit)
ELSE( EX_PLATFORM EQUAL 64 )
link_directories( ${PROJECT_SOURCE_DIR}/PLX/Library/32bit )
SET (PLX_LIB_DIRECTORY ${PROJECT_SOURCE_DIR}/PLX/Library/32bit)
ENDIF( EX_PLATFORM EQUAL 64 )


file(GLOB_RECURSE pixie4_SOURCES *.cpp XIA/*.cpp)
file(GLOB_RECURSE pixie4_HEADERS *.h)
foreach (_headerFile ${pixie4_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND pixie4_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES pixie4_INCLUDE_DIRS)

include_directories(
  ${engine_INCLUDE_DIRS}
  ${pixie4_INCLUDE_DIRS}
)

add_library(
  ${PROJECT_NAME}
  ${pixie4_SOURCES}
)

target_link_libraries(
  ${PROJECT_NAME}
  ${PLX_LIB_DIRECTORY}/PlxApi.a
)

# Required on Unix OS family to be able to be linked into shared libraries.
set_target_properties(${PROJECT_NAME}
                      PROPERTIES POSITION_INDEPENDENT_CODE ON
                      )

# Expose public includes (including Boost transitively) to other
# subprojects through cache variable.
set(${PROJECT_NAME}_INCLUDE_DIRS
    ${pixie4_INCLUDE_DIRS}
CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
