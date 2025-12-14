cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(PicoFET)

pico_sdk_init()

add_executable(PicoFET src/cmd.c src/jtaglib.c src/pico.c src/usb_descriptors.c)
target_compile_options(PicoFET PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(PicoFET tinyusb_device_unmarked)
target_link_libraries(PicoFET pico_stdlib)
target_link_libraries(PicoFET pico_status_led)
target_link_libraries(PicoFET pico_unique_id)
target_include_directories(PicoFET PRIVATE ${PROJECT_SOURCE_DIR}/src)

#pico_enable_stdio_uart(PicoFET 0)
#pico_enable_stdio_usb(PicoFET 1)
