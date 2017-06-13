cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)
project(producer_pixie4 C CXX)

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

file(GLOB_RECURSE ${PROJECT_NAME}_SOURCES *.cpp XIA/*.c)
file(GLOB_RECURSE ${PROJECT_NAME}_HEADERS *.h)
dirs_of(${PROJECT_NAME}_INCLUDE_DIRS "${${PROJECT_NAME}_HEADERS}")

set_source_files_properties(XIA/utilities.c PROPERTIES LANGUAGE CXX )
set_source_files_properties(XIA/Boot.c PROPERTIES LANGUAGE CXX )
set_source_files_properties(XIA/eeprom.c PROPERTIES LANGUAGE CXX )
set_source_files_properties(XIA/globals.c PROPERTIES LANGUAGE CXX )
set_source_files_properties(XIA/pixie_c.c PROPERTIES LANGUAGE CXX )
set_source_files_properties(XIA/ua_par_io.c PROPERTIES LANGUAGE CXX )

add_library(
  ${PROJECT_NAME}
  ${${PROJECT_NAME}_SOURCES}
  ${${PROJECT_NAME}_HEADERS}
)

include_directories(
  ${PROJECT_NAME}
  PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS}
  PRIVATE ${engine_INCLUDE_DIRS}
)

target_link_libraries(
  ${PROJECT_NAME}
#  ${engine_LIBRARIES}
  ${PLX_LIB_DIRECTORY}/PlxApi.a
  ${CMAKE_DL_LIBS}
)
