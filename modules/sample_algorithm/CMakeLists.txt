add_library(sample_algorithm MODULE
    sample_algorithm.cpp
)

target_include_directories(sample_algorithm
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(sample_algorithm
    PRIVATE
        pthread
)

set_target_properties(sample_algorithm PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/modules
)
