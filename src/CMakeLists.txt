cmake_minimum_required(VERSION 3.1)
project(qpx_gamma CXX)

set (CMAKE_CXX_STANDARD 11)
add_definitions(-DBOOST_LOG_DYN_LINK)

add_subdirectory(config)

add_subdirectory(engine)
add_subdirectory(consumers)
add_subdirectory(producers)

if (QPX_FITTER_ROOT)
  add_subdirectory(optimizerROOT)
  set(optimizer_LIBRARIES ${optimizerROOT_LIBRARIES})
endif()

if (QPX_FITTER_CERES)
  add_subdirectory(optimizer_ceres)
  set(optimizer_LIBRARIES ${optimizer_ceres_LIBRARIES})
endif()

if (QPX_CMD)
  add_subdirectory(cmd)
endif()

if (QPX_GUI)
  add_subdirectory(gui)
endif()
