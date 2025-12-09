cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(PicoFET)

pico_sdk_init()

add_executable(PicoFET src/cmd.c src/jtaglib.c src/pico.c)
target_link_libraries(PicoFET pico_stdlib)
target_link_libraries(PicoFET pico_status_led)

pico_enable_stdio_uart(PicoFET 0)
pico_enable_stdio_usb(PicoFET 1)
