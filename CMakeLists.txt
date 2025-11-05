cmake_minimum_required(VERSION 3.13)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Compiladores y plataforma
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m0plus)
set(CMAKE_ASM_COMPILER /usr/bin/arm-none-eabi-gcc)
set(CMAKE_C_COMPILER /usr/bin/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/arm-none-eabi-g++)

project(serv_http_esp8266 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(serv_http_esp8266
    main.cpp
    lib/Esp8266HttpServer.cpp
)

target_include_directories(serv_http_esp8266 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

target_link_libraries(serv_http_esp8266
    pico_stdlib
    hardware_uart
    hardware_gpio
)

pico_enable_stdio_usb(serv_http_esp8266 1)
pico_enable_stdio_uart(serv_http_esp8266 0)

target_compile_options(serv_http_esp8266 PRIVATE -O2 -Wall -Wextra)
target_link_options(serv_http_esp8266 PRIVATE -Wl,--gc-sections)

pico_add_extra_outputs(serv_http_esp8266)