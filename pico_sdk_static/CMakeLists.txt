cmake_minimum_required(VERSION 3.13...3.27)

include(pico_sdk_import.cmake)

project(pico_sdk_static)

pico_sdk_init()

add_library(pico_sdk_static STATIC)
target_link_libraries(pico_sdk_static pico_stdlib)
