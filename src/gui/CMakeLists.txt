cmake_minimum_required(VERSION 3.1)
project(qpx CXX)

#For Qt
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

#Locate packages
find_package(Qt5Widgets REQUIRED)
find_package(OpenGL REQUIRED)

#ROOT
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT REQUIRED COMPONENTS Hist)
include(${ROOT_USE_FILE})


#add_definitions(-DQCUSTOMPLOT_USE_OPENGL)

file(GLOB_RECURSE gui_SOURCES *.cpp *.h *.ui)
file(GLOB_RECURSE gui_HEADERS *.h *.hpp ${PROJECT_BINARY_DIR}/*.h)
foreach (_headerFile ${gui_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND gui_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES gui_INCLUDE_DIRS)


include_directories(
  ${ROOT_INCLUDE_DIR}
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
  ${engine_LIBRARIES}
  ${consumers_LIBRARIES}
  ${producers_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${ROOT_LIBRARIES}
)

qt5_use_modules(qpx Widgets PrintSupport)

if(UNIX)
  install(TARGETS qpx DESTINATION bin)
endif()

