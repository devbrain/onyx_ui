# Generate test configuration file with absolute paths to test data
configure_file(
        test_config.hh.in
        ${CMAKE_BINARY_DIR}/include/test_config.hh
        @ONLY
)

# Test support interface library
add_library(test_support INTERFACE)

target_include_directories(test_support INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/unittest  # For test_types.hh, test_types_yaml.hh
        ${CMAKE_BINARY_DIR}/include           # For generated test_config.hh
)

target_sources(test_support INTERFACE
        test_types.hh
        test_types_yaml.hh
        ${CMAKE_BINARY_DIR}/include/test_config.hh
)

message(STATUS "Test support library configured (test_types, test_config)")