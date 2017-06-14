cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(cpx CXX)

# Boost
#SET(Boost_USE_STATIC_LIBS ON)
add_definitions(-DBOOST_LOG_DYN_LINK)
find_package(Boost COMPONENTS
  system filesystem thread timer date_time
  log log_setup regex REQUIRED)

file(GLOB_RECURSE ${PROJECT_NAME}_SOURCES *.cpp)
file(GLOB_RECURSE ${PROJECT_NAME}_HEADERS *.h)
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
)

target_link_libraries(
  ${PROJECT_NAME}
  ${engine_LIBRARIES}
  ${consumers_LIBRARIES}
  ${producers_LIBRARIES}
  ${Boost_LIBRARIES}
)

if(UNIX)
  install(TARGETS cpx DESTINATION bin)
endif()
