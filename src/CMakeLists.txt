cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)
project(qpx_gamma CXX)

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

#===========================================================
# Internal compiler options
#===========================================================
# C++ standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD LESS 11)
  set(CMAKE_CXX_STANDARD 11)
endif()

#=========================================================
# Function for deducing include dirs
# retname - name of parent variable to fill
# file_list - list of dirs, pass to fn as "${list_var}"
#=========================================================
function(dirs_of retname file_list)
  set(dlist "")
  foreach (_file ${file_list})
      get_filename_component(_dir ${_file} PATH)
      list (APPEND dlist ${_dir})
  endforeach()
  list(REMOVE_DUPLICATES dlist)
  set (${retname} ${dlist} PARENT_SCOPE)
endfunction(dirs_of)

# deprecate
add_definitions(-DBOOST_LOG_DYN_LINK)

add_subdirectory(config)

# json++
list (APPEND json_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/external/json/src)

# HDF5 if enabled
if (QPX_USE_HDF5)
  add_subdirectory(h5)
  add_definitions(-DH5_ENABLED)
endif()

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
