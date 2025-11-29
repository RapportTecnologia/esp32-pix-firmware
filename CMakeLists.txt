# ESP-IDF 5.5.0 Project CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Set the target chip (ESP32)
set(SDKCONFIG_DEFAULTS "sdkconfig.defaults")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp-pix)
