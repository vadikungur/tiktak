cmake_minimum_required(VERSION 3.16)
project(tiktak_controller LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(tiktak_core
    src/devices/DeviceConfig.cpp
    src/devices/DeviceRegistry.cpp
    src/network/EthernetTransport.cpp
    src/services/AcquisitionService.cpp
    src/services/ActuationService.cpp
    src/algorithms/AlgorithmEngine.cpp
    src/ui/ConsoleUI.cpp
)

target_include_directories(tiktak_core
    PUBLIC
        include
)

target_link_libraries(tiktak_core
    PUBLIC
        pthread
        dl
)

add_executable(tiktak
    src/main.cpp
)

target_link_libraries(tiktak
    PRIVATE
        tiktak_core
)

add_subdirectory(modules/sample_algorithm)
