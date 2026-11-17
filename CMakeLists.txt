cmake_minimum_required(VERSION 3.10)
project(opcua_factory_server)

set(CMAKE_C_STANDARD 99)

find_package(open62541 REQUIRED)

# This must be the ONLY add_executable line in the file
add_executable(factory_server main.c factory_sensors.c)

target_link_libraries(factory_server open62541::open62541)
