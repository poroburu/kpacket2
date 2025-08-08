cmake_minimum_required(VERSION 3.22)

project(kpacket_examples CXX)

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

# FTXUI monitor (optional)
option(BUILD_FTXUI_MONITOR "Build FTXUI-based packet monitor" ON)
if (BUILD_FTXUI_MONITOR)
    find_package(ftxui CONFIG QUIET)
    if (ftxui_FOUND)
        add_executable(packet_monitor_ftxui packet_monitor_ftxui.cpp)
        target_link_libraries(packet_monitor_ftxui PRIVATE cppzmq nlohmann_json ftxui::screen ftxui::dom ftxui::component)
        target_compile_features(packet_monitor_ftxui PRIVATE cxx_std_17)
        set_target_properties(packet_monitor_ftxui PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../bin/examples"
        )
        target_compile_definitions(packet_monitor_ftxui PRIVATE -DKPACKET_PROTOCOL_V1)
    endif()
endif()