cmake_minimum_required(VERSION 3.22)

project(kpacket2_examples CXX)

# Find required packages
find_package(cppzmq CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Packet Monitor Example
add_executable(packet_monitor packet_monitor.cpp)
target_link_libraries(packet_monitor PRIVATE cppzmq nlohmann_json)
target_compile_features(packet_monitor PRIVATE cxx_std_17)

# Set output directory
set_target_properties(packet_monitor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../bin/examples"
)