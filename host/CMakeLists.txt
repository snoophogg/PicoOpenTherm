cmake_minimum_required(VERSION 3.13)
project(host_simulator C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Host-native simulator target (built with system toolchain)
add_executable(host_simulator
    ${CMAKE_CURRENT_LIST_DIR}/../src/host_simulator.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/simulated_opentherm.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/opentherm_protocol.cpp
)

target_include_directories(host_simulator PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../src
    ${CMAKE_CURRENT_LIST_DIR}/../src/generated
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MOSQUITTO REQUIRED libmosquitto)
target_include_directories(host_simulator PRIVATE ${MOSQUITTO_INCLUDE_DIRS})
target_link_libraries(host_simulator ${MOSQUITTO_LIBRARIES} pthread)
