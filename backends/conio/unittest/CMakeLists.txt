# Conio Backend Integration Tests
# Built as a shared library and linked into ui_unittest
# Uses doctest DLL mechanism for automatic test discovery

add_library(conio_unittest SHARED
        # DLL entry point (forces static test registration)
        dll_entry.cc
        # Conio integration tests - validate YAML system with real backend
        test_theme_loader.cc
        test_example_themes.cc
        test_directory_loading.cc
        test_coverage_analysis.cc
        test_json_schema.cc
        test_complete_theme.cc
        test_widget_style_reflection.cc
        test_theme_builder_compat.cc
        conio_themes_original.hh
)

# Enable doctest DLL export mechanism
# This makes doctest export test registration symbols from the DLL
target_compile_definitions(conio_unittest PRIVATE
        DOCTEST_CONFIG_IMPLEMENTATION_IN_DLL
)

# Include directories
target_include_directories(conio_unittest PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/backends/conio/include
)

# Link dependencies
target_link_libraries(conio_unittest PRIVATE
        onyxui
        onyxui_conio_backend
        doctest::doctest
        test_support  # Test utilities (test_types, test_config)
)

# Apply warning flags
target_compile_options(conio_unittest PRIVATE ${ONYX_UI_WARNING_FLAGS})

# Disable logging in tests (override inherited DEBUG level)
target_compile_options(conio_unittest PRIVATE
        -ULOGGER_MIN_LEVEL                      # Undefine inherited value
        -DLOGGER_MIN_LEVEL=LOGGER_LEVEL_ERROR   # Set to ERROR only
)

# Install to bin directory for easy loading
install(TARGETS conio_unittest
        LIBRARY DESTINATION bin
        RUNTIME DESTINATION bin
)
