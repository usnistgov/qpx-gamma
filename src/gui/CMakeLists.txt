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

add_definitions(-DQCUSTOMPLOT_USE_OPENGL)

file(GLOB_RECURSE ${PROJECT_NAME}_SOURCES *.cpp *.qrc *.ui)
file(GLOB_RECURSE ${PROJECT_NAME}_HEADERS *.h *.hpp ${PROJECT_BINARY_DIR}/*.h)
dirs_of(${PROJECT_NAME}_INCLUDE_DIRS "${${PROJECT_NAME}_HEADERS}")

add_executable(
  ${PROJECT_NAME}
  ${${PROJECT_NAME}_SOURCES}
  ${${PROJECT_NAME}_HEADERS}
)

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS}
  PRIVATE ${engine_INCLUDE_DIRS}
  PRIVATE ${PROJECT_BINARY_DIR}
  PRIVATE ${OPENGL_INCLUDE_DIR}
)

target_link_libraries(
  ${PROJECT_NAME}
  ${engine_LIBRARIES}
  ${consumers_LIBRARIES}
  ${producers_LIBRARIES}
  ${optimizer_LIBRARIES}
  ${OPENGL_LIBRARIES}
)

qt5_use_modules(qpx Widgets PrintSupport)

if(UNIX)
  install(TARGETS qpx DESTINATION bin)
endif()

