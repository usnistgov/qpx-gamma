cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)
project(producers CXX)

if (QPX_SIMULATOR2D)
  add_subdirectory(simulator2d)
  LIST(APPEND prod_LIBRARIES producer_simulator2d)
endif()

if (QPX_PARSER_RAW)
  add_subdirectory(parser_raw)
  LIST(APPEND prod_LIBRARIES producer_parser_raw)
endif()

if (QPX_VME)
  add_subdirectory(VME)
  LIST(APPEND prod_LIBRARIES producer_vme)
endif()

if (QPX_PARSER_EVT)
  add_subdirectory(parser_evt)
  LIST(APPEND prod_LIBRARIES producer_parser_evt)
endif()

if (QPX_HV8)
  add_subdirectory(HV8)
  LIST(APPEND prod_LIBRARIES producer_hv8)
endif()

if (QPX_PIXIE4)
  add_subdirectory(pixie4)
  LIST(APPEND prod_LIBRARIES producer_pixie4)
endif()

set(${PROJECT_NAME}_LIBRARIES
    -Wl,--whole-archive
    ${prod_LIBRARIES}
    -Wl,--no-whole-archive
    PARENT_SCOPE)
