cmake_minimum_required(VERSION 3.1)
project(producers CXX)

add_subdirectory(simulator2d)
add_subdirectory(parser_raw)
add_subdirectory(VME)
add_subdirectory(parser_evt)
add_subdirectory(HV8)
add_subdirectory(pixie4)

set(${PROJECT_NAME}_LIBRARIES
    -Wl,--whole-archive
    producer_simulator2d
    producer_parser_raw
    producer_vme
    producer_parser_evt
    producer_hv8
    producer_pixie4
    -Wl,--no-whole-archive
    PARENT_SCOPE)
