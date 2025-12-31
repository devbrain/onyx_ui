# =============================================================================
# OnyxUI - Modern C++20 Header-Only UI Framework
# =============================================================================

cmake_minimum_required(VERSION 3.20)

project(onyx_ui
    VERSION 1.0.0
    DESCRIPTION "Modern C++20 header-only UI framework with backend-agnostic design"
    LANGUAGES C CXX
)

# =============================================================================
# Neutrino CMake Integration
# =============================================================================

include(FetchContent)

if(NOT DEFINED NEUTRINO_CMAKE_DIR)
    FetchContent_Declare(
        neutrino_cmake
        GIT_REPOSITORY https://github.com/devbrain/neutrino-cmake.git
        GIT_TAG master
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(neutrino_cmake)
    set(NEUTRINO_CMAKE_DIR "${neutrino_cmake_SOURCE_DIR}/cmake")
endif()

list(APPEND CMAKE_MODULE_PATH "${NEUTRINO_CMAKE_DIR}")
include(NeutrinoInit)

# =============================================================================
# Options
# =============================================================================

# Standard neutrino options (BUILD_TESTS, BUILD_EXAMPLES, INSTALL, etc.)
neutrino_define_options(onyx_ui)

# Component-specific options
neutrino_is_top_level(_onyx_ui_is_top_level)

option(NEUTRINO_ONYX_UI_THREAD_SAFE
    "Enable thread-safe signal/slot system (adds mutex overhead)" ON)
option(NEUTRINO_ONYX_UI_ENABLE_YAML_THEMES
    "Enable YAML theme support (fkYAML + reflect-cpp)" ON)

# Backend build options
if(_onyx_ui_is_top_level)
    option(NEUTRINO_ONYX_UI_BUILD_BACKEND_CONIO "Build conio (terminal) backend" ON)
    option(NEUTRINO_ONYX_UI_BUILD_BACKEND_SDLPP "Build SDL++ (graphical) backend" ON)
else()
    option(NEUTRINO_ONYX_UI_BUILD_BACKEND_CONIO "Build conio (terminal) backend" OFF)
    option(NEUTRINO_ONYX_UI_BUILD_BACKEND_SDLPP "Build SDL++ (graphical) backend" OFF)
endif()

# =============================================================================
# C++ Standard
# =============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# Output Directories
# =============================================================================

set(BINARY_OUT_DIR ${CMAKE_BINARY_DIR}/bin)
set(LIB_OUT_DIR ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LIB_OUT_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BINARY_OUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${BINARY_OUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${LIB_OUT_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BINARY_OUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${BINARY_OUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${LIB_OUT_DIR})

# =============================================================================
# Project Root (for backends and other subdirectories)
# =============================================================================

set(ONYX_UI_PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Include backend library helper
include(cmake/AddBackend.cmake)

# =============================================================================
# Dependencies
# =============================================================================

# ---------------------------------------------------------------------------
# Failsafe (error handling library)
# ---------------------------------------------------------------------------
include(${NEUTRINO_CMAKE_DIR}/deps/failsafe.cmake)
neutrino_fetch_failsafe()

# ---------------------------------------------------------------------------
# UTF8-CPP (header-only library for Unicode handling)
# ---------------------------------------------------------------------------
include(${NEUTRINO_CMAKE_DIR}/deps/utf8cpp.cmake)
neutrino_fetch_utf8cpp()

# ---------------------------------------------------------------------------
# fkYAML (header-only YAML library) - Only if YAML themes enabled
# ---------------------------------------------------------------------------
if(NEUTRINO_ONYX_UI_ENABLE_YAML_THEMES)
    if(NOT TARGET fkYAML AND NOT TARGET neutrino::fkyaml)
        find_package(fkYAML QUIET CONFIG)

        if(NOT fkYAML_FOUND)
            message(STATUS "fkYAML not found - fetching from GitHub...")

            set(FK_YAML_BUILD_EXAMPLES OFF CACHE INTERNAL "")
            set(FK_YAML_BUILD_TESTS OFF CACHE INTERNAL "")

            FetchContent_Declare(
                fkYAML
                GIT_REPOSITORY https://github.com/fktn-k/fkYAML.git
                GIT_TAG v0.4.0
                GIT_SHALLOW TRUE
                GIT_PROGRESS TRUE
            )
            FetchContent_MakeAvailable(fkYAML)
        endif()
    endif()

    # Create neutrino:: alias
    if(TARGET fkYAML AND NOT TARGET neutrino::fkyaml)
        add_library(neutrino::fkyaml ALIAS fkYAML)
    elseif(TARGET fkYAML::fkYAML AND NOT TARGET neutrino::fkyaml)
        add_library(neutrino::fkyaml ALIAS fkYAML::fkYAML)
    endif()
endif()

# ---------------------------------------------------------------------------
# reflect-cpp (reflection library with JSON support) - Only if YAML themes enabled
# ---------------------------------------------------------------------------
if(NEUTRINO_ONYX_UI_ENABLE_YAML_THEMES)
    if(NOT TARGET reflectcpp AND NOT TARGET neutrino::reflectcpp)
        find_package(reflectcpp QUIET CONFIG)

        if(NOT reflectcpp_FOUND)
            message(STATUS "reflect-cpp not found - fetching from GitHub...")

            # Disable all optional format support (we only need JSON via bundled yyjson)
            set(REFLECTCPP_BUILD_TESTS OFF CACHE INTERNAL "")
            set(REFLECTCPP_BSON OFF CACHE INTERNAL "")
            set(REFLECTCPP_CBOR OFF CACHE INTERNAL "")
            set(REFLECTCPP_FLEXBUFFERS OFF CACHE INTERNAL "")
            set(REFLECTCPP_MSGPACK OFF CACHE INTERNAL "")
            set(REFLECTCPP_XML OFF CACHE INTERNAL "")
            set(REFLECTCPP_TOML OFF CACHE INTERNAL "")
            set(REFLECTCPP_YAML OFF CACHE INTERNAL "")
            set(REFLECTCPP_USE_VCPKG OFF CACHE INTERNAL "")

            FetchContent_Declare(
                reflectcpp
                GIT_REPOSITORY https://github.com/getml/reflect-cpp.git
                GIT_TAG v0.10.0
                GIT_SHALLOW TRUE
                GIT_PROGRESS TRUE
                GIT_SUBMODULES ""
            )
            FetchContent_MakeAvailable(reflectcpp)
        endif()
    endif()

    # Create neutrino:: alias and mark as SYSTEM
    if(TARGET reflectcpp AND NOT TARGET neutrino::reflectcpp)
        get_target_property(REFLECTCPP_INCLUDES reflectcpp INTERFACE_INCLUDE_DIRECTORIES)
        if(REFLECTCPP_INCLUDES)
            set_target_properties(reflectcpp PROPERTIES
                INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${REFLECTCPP_INCLUDES}")
        endif()
        add_library(neutrino::reflectcpp ALIAS reflectcpp)
    elseif(TARGET reflectcpp::reflectcpp AND NOT TARGET neutrino::reflectcpp)
        get_target_property(REFLECTCPP_INCLUDES reflectcpp::reflectcpp INTERFACE_INCLUDE_DIRECTORIES)
        if(REFLECTCPP_INCLUDES)
            set_target_properties(reflectcpp::reflectcpp PROPERTIES
                INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${REFLECTCPP_INCLUDES}")
        endif()
        add_library(neutrino::reflectcpp ALIAS reflectcpp::reflectcpp)
    endif()
endif()

# =============================================================================

add_library(onyxui INTERFACE
        include/onyxui/concepts/backend.hh
        include/onyxui/concepts/color_like.hh
        include/onyxui/concepts/common.hh
        include/onyxui/concepts/event_like.hh
        include/onyxui/concepts/point_like.hh
        include/onyxui/concepts/rect_like.hh
        include/onyxui/concepts/render_like.hh
        include/onyxui/concepts/size_like.hh
        include/onyxui/core/element.hh
        include/onyxui/core/element_fwd.hh
        include/onyxui/core/event_target.hh
        include/onyxui/core/rendering/draw_context.hh
        include/onyxui/core/rendering/measure_context.hh
        include/onyxui/core/rendering/render_context.hh
        include/onyxui/core/rendering/resolved_style.hh
        include/onyxui/core/raii/scoped_clip.hh
        include/onyxui/core/signal.hh
        include/onyxui/events/ui_event.hh
        include/onyxui/hotkeys/builtin_hotkey_schemes.hh
        include/onyxui/hotkeys/hotkey_action.hh
        include/onyxui/hotkeys/hotkey_manager.hh
        include/onyxui/hotkeys/hotkey_scheme.hh
        include/onyxui/hotkeys/hotkey_scheme_registry.hh
        include/onyxui/hotkeys/key_chord.hh
        include/onyxui/hotkeys/key_code.hh
        include/onyxui/hotkeys/key_sequence.hh
        include/onyxui/hotkeys/semantic_action_guard.hh
        include/onyxui/layout/absolute_layout.hh
        include/onyxui/layout/anchor_layout.hh
        include/onyxui/layout/grid_layout.hh
        include/onyxui/layout/layout_strategy.hh
        include/onyxui/layout/linear_layout.hh
        include/onyxui/core/raii/scoped_layer.hh
        include/onyxui/core/raii/scoped_tooltip.hh
        include/onyxui/services/background_renderer.hh
        include/onyxui/services/dirty_region_manager.hh
        include/onyxui/services/focus_manager.hh
        include/onyxui/services/input_manager.hh
        include/onyxui/services/layer_manager.hh
        include/onyxui/services/ui_context.hh
        include/onyxui/services/ui_services.hh
        include/onyxui/theming/theme.hh
        include/onyxui/theming/theme_loader.hh
        include/onyxui/theming/theme_registry.hh
        include/onyxui/theming/themeable.hh
        include/onyxui/ui_constants.hh
        include/onyxui/ui_handle.hh
        include/onyxui/utils/fkyaml_adapter.hh
        include/onyxui/utils/safe_math.hh
        include/onyxui/actions/action.hh
        include/onyxui/actions/action_group.hh
        include/onyxui/actions/mnemonic_parser.hh
        include/onyxui/widgets/button.hh
        include/onyxui/widgets/containers/absolute_panel.hh
        include/onyxui/widgets/containers/anchor_panel.hh
        include/onyxui/widgets/containers/grid.hh
        include/onyxui/widgets/containers/group_box.hh
        include/onyxui/widgets/containers/hbox.hh
        include/onyxui/widgets/containers/panel.hh
        include/onyxui/widgets/containers/scroll/scroll_controller.hh
        include/onyxui/widgets/containers/scroll/scroll_info.hh
        include/onyxui/widgets/containers/scroll/scroll_view_presets.hh
        include/onyxui/widgets/containers/scroll/scrollable.hh
        include/onyxui/widgets/containers/scroll/scrollbar.hh
        include/onyxui/widgets/containers/scroll/scrollbar_visibility.hh
        include/onyxui/widgets/containers/scroll_view.hh
        include/onyxui/widgets/containers/vbox.hh
        include/onyxui/widgets/core/stateful_widget.hh
        include/onyxui/widgets/core/widget.hh
        include/onyxui/widgets/core/widget_container.hh
        include/onyxui/widgets/core/widget_impl.hh
        include/onyxui/widgets/label.hh
        include/onyxui/widgets/menu/menu.hh
        include/onyxui/widgets/menu/menu_bar.hh
        include/onyxui/widgets/menu/menu_bar_item.hh
        include/onyxui/widgets/menu/menu_item.hh
        include/onyxui/widgets/menu/menu_system.hh
        include/onyxui/widgets/menu/separator.hh
        include/onyxui/widgets/layout/spacer.hh
        include/onyxui/widgets/layout/spring.hh
        include/onyxui/widgets/status_bar.hh
        include/onyxui/widgets/styled_text.hh

)

# Create neutrino:: alias
add_library(neutrino::onyxui ALIAS onyxui)

target_include_directories(onyxui INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(onyxui INTERFACE
    neutrino::failsafe
)

# Conditionally link YAML theme dependencies
if(NEUTRINO_ONYX_UI_ENABLE_YAML_THEMES)
    target_link_libraries(onyxui INTERFACE
        neutrino::fkyaml       # YAML parsing for theme support
        neutrino::reflectcpp   # Automatic C++ reflection for JSON serialization
    )
    target_compile_definitions(onyxui INTERFACE ONYXUI_ENABLE_YAML_THEMES=1)
    message(STATUS "YAML theme support enabled (fkYAML + reflect-cpp)")
else()
    message(STATUS "YAML theme support disabled")
endif()

# Thread safety configuration
if(NEUTRINO_ONYX_UI_THREAD_SAFE)
    target_compile_definitions(onyxui INTERFACE ONYXUI_THREAD_SAFE=1)
    message(STATUS "Thread-safe signal/slot system enabled (ONYXUI_THREAD_SAFE=1)")
else()
    message(STATUS "Thread-safe signal/slot system disabled (maximum performance, single-threaded only)")
endif()

# Failsafe logging configuration
# Debug builds: Enable verbose logging for development and debugging
# Release builds: Only log errors to minimize performance impact
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(onyxui INTERFACE FAILSAFE_LOG_LEVEL=FAILSAFE_LOG_DEBUG)
    message(STATUS "Failsafe logging: DEBUG level enabled")
else()
    target_compile_definitions(onyxui INTERFACE FAILSAFE_LOG_LEVEL=FAILSAFE_LOG_ERROR)
    message(STATUS "Failsafe logging: ERROR level only")
endif()

# =============================================================================
# Backends
# =============================================================================

if(NEUTRINO_ONYX_UI_BUILD_BACKEND_CONIO)
    add_subdirectory(backends/conio)
endif()

if(NEUTRINO_ONYX_UI_BUILD_BACKEND_SDLPP)
    add_subdirectory(backends/sdlpp)
endif()

# =============================================================================
# Tests
# =============================================================================

if(NEUTRINO_ONYX_UI_BUILD_TESTS)
    include(${NEUTRINO_CMAKE_DIR}/deps/doctest.cmake)
    neutrino_fetch_doctest()

    enable_testing()
    add_subdirectory(unittest)
endif()

# =============================================================================
# Examples
# =============================================================================

if(NEUTRINO_ONYX_UI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# =============================================================================
# Installation
# =============================================================================

if(NEUTRINO_ONYX_UI_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    # Install headers only (header-only library)
    install(DIRECTORY include/onyxui
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hh" PATTERN "*.h"
    )

    # Generate version file
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/onyxuiConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    # Generate config file
    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/onyxuiConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/onyxuiConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/onyxui
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/onyxuiConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/onyxuiConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/onyxui
    )
endif()

# =============================================================================
# Summary
# =============================================================================

neutrino_is_top_level(_onyx_ui_is_top_level)
if(_onyx_ui_is_top_level)
    neutrino_print_options(onyx_ui)
    message(STATUS "")
    message(STATUS "onyx_ui ${PROJECT_VERSION} Configuration:")
    message(STATUS "  C++ Standard:       20")
    message(STATUS "  Build tests:        ${NEUTRINO_ONYX_UI_BUILD_TESTS}")
    message(STATUS "  Build examples:     ${NEUTRINO_ONYX_UI_BUILD_EXAMPLES}")
    message(STATUS "  Thread-safe:        ${NEUTRINO_ONYX_UI_THREAD_SAFE}")
    message(STATUS "  YAML themes:        ${NEUTRINO_ONYX_UI_ENABLE_YAML_THEMES}")
    message(STATUS "  Backend conio:      ${NEUTRINO_ONYX_UI_BUILD_BACKEND_CONIO}")
    message(STATUS "  Backend SDL++:      ${NEUTRINO_ONYX_UI_BUILD_BACKEND_SDLPP}")
    message(STATUS "  Install:            ${NEUTRINO_ONYX_UI_INSTALL}")
    message(STATUS "")
endif()