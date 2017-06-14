cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(qpx CXX)

# Boost
#SET(Boost_USE_STATIC_LIBS ON)
add_definitions(-DBOOST_LOG_DYN_LINK)
find_package(Boost COMPONENTS
  system filesystem thread timer date_time
  log log_setup regex REQUIRED)

#For Qt
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt5Widgets REQUIRED)

#OpenGL for plots
find_package(OpenGL REQUIRED)
add_definitions(-DQCUSTOMPLOT_USE_OPENGL)

file(GLOB ${PROJECT_NAME}_RESOURCES resources/*.qrc)
file(GLOB ${PROJECT_NAME}_SOURCES *.cpp *.ui
  analysis/*.cpp analysis/*.ui
  daq/*.cpp daq/*.ui
  fitter/*.cpp fitter/*.ui
  widgets/*.cpp widgets/*.ui
  widgets/qtcolorpicker/*.cpp)
file(GLOB ${PROJECT_NAME}_HEADERS *.h *.hpp ${PROJECT_BINARY_DIR}/*.h
  analysis/*.h daq/*.h fitter/*.h widgets/*.h
  widgets/qtcolorpicker/*.h)
dirs_of(${PROJECT_NAME}_INCLUDE_DIRS "${${PROJECT_NAME}_HEADERS}")

file(GLOB_RECURSE QPLOT_SOURCES qplot/src/qplot/*.cpp)
file(GLOB_RECURSE QPLOT_HEADERS qplot/src/qplot/*.h)
dirs_of(QPLOT_INCLUDE_DIRS "${QPLOT_HEADERS}")

add_executable(
  ${PROJECT_NAME}
  ${${PROJECT_NAME}_SOURCES}
  ${${PROJECT_NAME}_HEADERS}
  ${QPLOT_SOURCES}
  ${QPLOT_HEADERS}
)

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS}
  PRIVATE ${engine_INCLUDE_DIRS}
  PRIVATE ${PROJECT_BINARY_DIR}
  PRIVATE ${QPLOT_INCLUDE_DIRS}
  PRIVATE ${OPENGL_INCLUDE_DIR}
)

target_link_libraries(
  ${PROJECT_NAME}
  ${engine_LIBRARIES}
  ${consumers_LIBRARIES}
  ${producers_LIBRARIES}
  ${optimizer_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${Boost_LIBRARIES}
)

qt5_use_modules(qpx Widgets PrintSupport)

if(UNIX)
  install(TARGETS qpx DESTINATION bin)
endif()

