cmake_minimum_required(VERSION 3.10)
project(opcua_factory_server C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(ENABLE_COVERAGE "Enable gcov code coverage" OFF)

find_package(open62541 REQUIRED)

add_executable(factory_server
    main.c
    config.c
    hierarchy.c
    variables.c
    logic.c
)
target_compile_options(factory_server PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(factory_server open62541::open62541)

enable_testing()
add_executable(test_week1
    tests/test_week1.c
    config.c
    hierarchy.c
    variables.c
    logic.c
)
target_include_directories(test_week1 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(test_week1 PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(test_week1 open62541::open62541)

if(ENABLE_COVERAGE)
    target_compile_options(test_week1 PRIVATE --coverage -O0 -g)
    target_link_options(test_week1 PRIVATE --coverage)
endif()

add_test(NAME week1_unit_tests COMMAND test_week1)
