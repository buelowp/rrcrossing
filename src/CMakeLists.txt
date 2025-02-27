cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(semaphore)

pico_sdk_init()

add_executable(${PROJECT_NAME} main.cpp ws2812.cpp Servo.cpp statemachine.cpp)

# Add pico_stdlib library, add more if used
target_link_libraries(${PROJECT_NAME} pico_stdlib pico_multicore hardware_pio hardware_pwm)

# enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Need to generate UF2 file for upload to RP2040
pico_add_extra_outputs(${PROJECT_NAME})
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)
