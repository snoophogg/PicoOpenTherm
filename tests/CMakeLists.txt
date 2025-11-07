cmake_minimum_required(VERSION 3.13)

# Build tests for host platform
set(PICO_PLATFORM host)

# Set the path to the Pico SDK
set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/../pico-sdk)

# Include the Pico SDK import
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(PicoOpenThermTests C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Test 1: OpenTherm Protocol Tests
add_executable(test_opentherm_protocol
    test_opentherm_protocol.cpp
    ../src/opentherm_protocol.cpp
)

target_include_directories(test_opentherm_protocol PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../src
)

# Link minimal required libraries for host build  
target_link_libraries(test_opentherm_protocol
    pico_stdlib
)

# Test 2: MQTT Topics Tests
add_executable(test_mqtt_topics
    test_mqtt_topics.cpp
)

target_include_directories(test_mqtt_topics PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../src
)

# Link minimal required libraries for host build
target_link_libraries(test_mqtt_topics
    pico_stdlib
)

# Test 3: Simulator Tests
add_executable(test_simulator
    test_simulator.cpp
    ../src/simulated_opentherm.cpp
)

target_include_directories(test_simulator PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../src
)

# Link minimal required libraries for host build
target_link_libraries(test_simulator
    pico_stdlib
)

# Enable testing
enable_testing()

# Add tests to CTest
add_test(NAME OpenThermProtocol COMMAND test_opentherm_protocol)
add_test(NAME MQTTTopics COMMAND test_mqtt_topics)
add_test(NAME Simulator COMMAND test_simulator)
