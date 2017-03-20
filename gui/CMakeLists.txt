cmake_minimum_required(VERSION 3.1)
project(qpx CXX)

# Set C++11 flag.
#set (CMAKE_CXX_STANDARD 11)

##root
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

#For Qt
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

#set(HDF5_USE_SHARED_LIBRARIES ON)

#Locate packages
#find_package(HDF5 COMPONENTS CXX REQUIRED)
#find_package(Boost COMPONENTS system filesystem thread timer date_time
#             program_options log log_setup regex REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(ROOT REQUIRED COMPONENTS RIO Net)
find_package(OpenGL REQUIRED)

#ROOT needs this
include(${ROOT_USE_FILE})

# Boost needs this
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_LOG_DYN_LINK")

#link_directories( ${BOOST_LIBRARY_DIRS} )

#add_definitions(-DQCUSTOMPLOT_USE_OPENGL)

#execute_process(COMMAND
#  git rev-parse HEAD
#  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
#  OUTPUT_VARIABLE GIT_SHA1
#  OUTPUT_STRIP_TRAILING_WHITESPACE)

#add_definitions("-DGIT_VERSION=\"${GIT_SHA1}\"")

file(GLOB_RECURSE gui_SOURCES *.cpp *.h *.ui)
file(GLOB_RECURSE gui_HEADERS *.h *.hpp ${PROJECT_BINARY_DIR}/*.h)
foreach (_headerFile ${gui_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND gui_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES gui_INCLUDE_DIRS)


include_directories(
  ${ROOT_INCLUDE_DIR} #necessary?
  ${engine_INCLUDE_DIRS}
  ${PROJECT_BINARY_DIR}
  ${gui_INCLUDE_DIRS}
  ${OPENGL_INCLUDE_DIR}
)

add_executable(
  ${PROJECT_NAME}
  ${gui_SOURCES}
  resources/qpxres.qrc
)

target_link_libraries(
  ${PROJECT_NAME}
  engine
  consumers
  producer_simulator2d
  producer_parser_raw
  producer_vme
  producer_parser_evt
  producer_hv8
  producer_pixie4
  ${OPENGL_LIBRARIES}
)

qt5_use_modules(qpx Widgets PrintSupport)
