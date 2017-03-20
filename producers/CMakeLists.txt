cmake_minimum_required(VERSION 3.1)
project(producers CXX)

add_subdirectory(simulator2d)
add_subdirectory(parser_raw)
add_subdirectory(VME)
add_subdirectory(parser_evt)
add_subdirectory(HV8)
add_subdirectory(pixie4)

# Expose public includes (including Boost transitively) to other
# subprojects through cache variable.
set(${PROJECT_NAME}_INCLUDE_DIRS
    ${producer_simulator2d_INCLUDE_DIRS}
    ${producer_parser_raw_INCLUDE_DIRS}
    ${producer_vme_INCLUDE_DIRS}
    ${producer_parser_evt_INCLUDE_DIRS}
    ${producer_hv8_INCLUDE_DIRS}
    ${producer_pixie4_INCLUDE_DIRS}
CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
