cmake_minimum_required(VERSION 3.15)
project(onyx_ui LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ONYX_UI_BUILD_TEST "Build unittests for onyx_ui" ON)
option(ONYX_UI_ENABLE_CLANG_TIDY "Enable clang-tidy checks" OFF)
option(ONYX_UI_ENABLE_CPPCHECK "Enable cppcheck static analysis" OFF)
option(ONYXUI_THREAD_SAFE "Enable thread-safe signal/slot system (adds mutex overhead)" ON)
option(ONYX_UI_ENABLE_YAML_THEMES "Enable YAML theme support (fkYAML + reflect-cpp)" ON)

# Backend build options
option(ONYX_UI_BUILD_BACKEND_CONIO "Build conio (terminal) backend" ON)
option(ONYX_UI_BUILD_BACKEND_SDL2 "Build SDL2 (graphical) backend" ON)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(ONYX_UI_WARNING_FLAGS
            -Wall           # All standard warnings
            -Wextra         # Extra warnings
            -Wpedantic      # Strict ISO C++ compliance
            -Wcast-align    # Warn about potential performance problem casts
            -Wcast-qual     # Warn about casts that remove qualifiers
            -Wconversion    # Warn about type conversions that may lose data
            -Wdouble-promotion  # Warn about float to double promotion
            -Wformat=2      # Additional format string warnings
            -Wnon-virtual-dtor  # Warn about non-virtual destructors
            -Wold-style-cast    # Warn about C-style casts
            -Woverloaded-virtual # Warn about overloaded virtual functions
            -Wshadow        # Warn about variable shadowing
            -Wsign-conversion   # Warn about sign conversions
            -Wundef         # Warn about undefined identifiers in #if
            -Wunused        # Warn about unused entities
            -Wzero-as-null-pointer-constant # Warn about using 0 as nullptr
            -Wstringop-overflow
    )

    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        list(APPEND ONYX_UI_WARNING_FLAGS
                -Wlogical-op    # Warn about logical operations being used where bitwise were probably wanted
                -Wuseless-cast  # Warn about useless casts
        )
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(ONYX_UI_WARNING_FLAGS
            /W4             # Highest warning level
            /permissive-    # Strict standard conformance
            /Zc:__cplusplus # Report correct __cplusplus value
    )
endif ()

# Treat warnings as errors in CI/Release builds (optional)
# list(APPEND ONYX_UI_WARNING_FLAGS -Werror)

# Sanitizer flags
if (ONYX_UI_ENABLE_SANITIZERS)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(ONYX_UI_SANITIZER_FLAGS
                -fsanitize=address
                -fsanitize=undefined
                -fno-omit-frame-pointer
        )
        list(APPEND ONYX_UI_WARNING_FLAGS ${ONYX_UI_SANITIZER_FLAGS})

        # Link flags for sanitizers
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fsanitize=undefined")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address -fsanitize=undefined")
    else ()
        message(WARNING "Sanitizers are only supported with GCC and Clang")
    endif ()
endif ()

# ===========================================================================
# Static Analysis / Linters
# ===========================================================================

# clang-tidy (comprehensive static analysis)
if (ONYX_UI_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy-18 clang-tidy-17 clang-tidy-16)
    if (CLANG_TIDY_EXE)
        message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
        set(CMAKE_CXX_CLANG_TIDY
            ${CLANG_TIDY_EXE};
            --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy;
        )
    else()
        message(WARNING "clang-tidy requested but not found")
    endif()
endif()

# cppcheck (additional static analysis)
if (ONYX_UI_ENABLE_CPPCHECK)
    find_program(CPPCHECK_EXE NAMES cppcheck)
    if (CPPCHECK_EXE)
        message(STATUS "Found cppcheck: ${CPPCHECK_EXE}")
        set(CMAKE_CXX_CPPCHECK
            ${CPPCHECK_EXE};
            --enable=warning,style,performance,portability;
            --std=c++20;
            --inline-suppr;
            --suppress=missingIncludeSystem;
            --suppress=unmatchedSuppression;
            --suppress=unusedFunction;
            --quiet;
            --error-exitcode=0;
        )
    else()
        message(WARNING "cppcheck requested but not found")
    endif()
endif()

# Output binary to predictable location.
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
# ===========================================================================
set(ONYX_UI_PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# Include dependency helper functions
include(cmake/FetchDependency.cmake)

# Include backend library helper
include(cmake/AddBackend.cmake)

# ===========================================================================
# Failsafe Dependency
# ===========================================================================
# Note: Failsafe is fetched via FetchContent (not ext/) due to OVERRIDE_FIND_PACKAGE

if(NOT TARGET failsafe AND NOT TARGET thirdparty::failsafe)
    find_package(failsafe QUIET)

    if(NOT failsafe_FOUND)
        message(STATUS "failsafe not found - fetching from GitHub...")

        # Disable failsafe tests and examples
        set(FAILSAFE_BUILD_TESTS OFF CACHE INTERNAL "")
        set(FAILSAFE_BUILD_EXAMPLES OFF CACHE INTERNAL "")

        fetchcontent_declare(
                FAILSAFE
                GIT_REPOSITORY "https://github.com/devbrain/failsafe.git"
                GIT_TAG master
                GIT_PROGRESS TRUE
                UPDATE_COMMAND ""
                OVERRIDE_FIND_PACKAGE
        )
        fetchcontent_makeavailable(FAILSAFE)
    endif()
endif()

# Create standardized alias
if(TARGET failsafe AND NOT TARGET thirdparty::failsafe)
    add_library(thirdparty::failsafe ALIAS failsafe)
elseif(TARGET failsafe::failsafe AND NOT TARGET thirdparty::failsafe)
    add_library(thirdparty::failsafe ALIAS failsafe::failsafe)
endif()

# ===========================================================================
# UTF8-CPP Dependency (header-only library)
# ===========================================================================
# Used by test backend for correct Unicode code point counting
# Note: Failsafe provides this transitively, but we manage it explicitly

fetch_dependency(
    NAME utf8cpp
    TARGET utf8cpp
    ALIAS thirdparty::utf8cpp
    ALTERNATIVE_TARGETS utf8cpp::utf8cpp
    FETCH_METHOD SUBDIR
    FETCH_SOURCE ext/utf8cpp
)

# ===========================================================================
# fkYAML Dependency (header-only YAML library)
# ===========================================================================
# Used for YAML theme support - allows loading/saving themes from YAML files
# Only fetched if ONYX_UI_ENABLE_YAML_THEMES is ON

if(ONYX_UI_ENABLE_YAML_THEMES)
    fetch_dependency(
        NAME fkYAML
        TARGET fkYAML
        ALIAS thirdparty::fkYAML
        ALTERNATIVE_TARGETS fkYAML::fkYAML
        FETCH_METHOD SUBDIR
        FETCH_SOURCE ext/fkyaml
    )
endif()

# ===========================================================================
# reflect-cpp Dependency (reflection library with JSON support)
# ===========================================================================
# Used for automatic C++ struct serialization/deserialization to/from JSON
# Provides JSON Schema generation for IDE autocomplete support
# Only fetched if ONYX_UI_ENABLE_YAML_THEMES is ON
#
# Dependencies brought by reflect-cpp:
# - yyjson.c (bundled, ~345KB) - Fast JSON parser/generator
# - Optional formats (all disabled): BSON, CBOR, FlexBuffers, MsgPack, TOML, XML, YAML
#
# Note: We use JSON support only (via bundled yyjson). YAML parsing uses fkYAML separately.

if(ONYX_UI_ENABLE_YAML_THEMES AND NOT TARGET reflectcpp AND NOT TARGET thirdparty::reflectcpp)
    find_package(reflectcpp QUIET)

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
        set(REFLECTCPP_YAML OFF CACHE INTERNAL "")  # We use fkYAML instead
        set(REFLECTCPP_USE_VCPKG OFF CACHE INTERNAL "")

        FetchContent_Declare(
            reflectcpp
            GIT_REPOSITORY "https://github.com/getml/reflect-cpp.git"
            GIT_TAG v0.10.0
            GIT_PROGRESS TRUE
            GIT_SUBMODULES ""  # Skip vcpkg submodule (we don't use vcpkg)
            UPDATE_COMMAND ""
            OVERRIDE_FIND_PACKAGE
        )
        FetchContent_MakeAvailable(reflectcpp)
    endif()
endif()

# Create standardized alias (only if YAML themes enabled)
if(ONYX_UI_ENABLE_YAML_THEMES)
    if(TARGET reflectcpp AND NOT TARGET thirdparty::reflectcpp)
        # Mark reflect-cpp headers as SYSTEM to suppress warnings from external code
        get_target_property(REFLECTCPP_INCLUDES reflectcpp INTERFACE_INCLUDE_DIRECTORIES)
        set_target_properties(reflectcpp PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${REFLECTCPP_INCLUDES}")
        # Now create the alias
        add_library(thirdparty::reflectcpp ALIAS reflectcpp)
    elseif(TARGET reflectcpp::reflectcpp AND NOT TARGET thirdparty::reflectcpp)
        # Mark reflect-cpp headers as SYSTEM to suppress warnings from external code
        get_target_property(REFLECTCPP_INCLUDES reflectcpp::reflectcpp INTERFACE_INCLUDE_DIRECTORIES)
        set_target_properties(reflectcpp::reflectcpp PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${REFLECTCPP_INCLUDES}")
        # Now create the alias
        add_library(thirdparty::reflectcpp ALIAS reflectcpp::reflectcpp)
    endif()
endif()

# ===========================================================================

add_library(onyxui INTERFACE
        include/onyxui/element.hh
        include/onyxui/utils/safe_math.hh
        include/onyxui/layout_strategy.hh
        include/onyxui/layout/linear_layout.hh
        include/onyxui/layout/grid_layout.hh
        include/onyxui/layout/anchor_layout.hh
        include/onyxui/layout/absolute_layout.hh
        include/onyxui/event_target.hh
        include/onyxui/signal.hh
        include/onyxui/concepts/event_like.hh
        include/onyxui/concepts/point_like.hh
        include/onyxui/concepts/size_like.hh
        include/onyxui/concepts/rect_like.hh
        include/onyxui/concepts/color_like.hh
        include/onyxui/concepts/backend.hh
        include/onyxui/theme.hh
        include/onyxui/themeable.hh
        include/onyxui/concepts/render_like.hh
        include/onyxui/widgets/widget.hh
        include/onyxui/widgets/label.hh
        include/onyxui/widgets/button.hh
        include/onyxui/widgets/panel.hh
        include/onyxui/widgets/hbox.hh
        include/onyxui/widgets/vbox.hh
        include/onyxui/widgets/spring.hh
        include/onyxui/widgets/status_bar.hh
        include/onyxui/widgets/group_box.hh
        examples/demo.cc
        examples/demo.hh
)

target_include_directories(onyxui INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(onyxui INTERFACE
    thirdparty::failsafe
)

# Conditionally link YAML theme dependencies
if(ONYX_UI_ENABLE_YAML_THEMES)
    target_link_libraries(onyxui INTERFACE
        thirdparty::fkYAML       # YAML parsing for theme support
        thirdparty::reflectcpp   # Automatic C++ reflection for JSON serialization
    )
    target_compile_definitions(onyxui INTERFACE ONYXUI_ENABLE_YAML_THEMES=1)
    message(STATUS "YAML theme support enabled (fkYAML + reflect-cpp)")
else()
    message(STATUS "YAML theme support disabled")
endif()

# Thread safety configuration
if(ONYXUI_THREAD_SAFE)
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

if (ONYX_UI_BUILD_TEST)
    add_subdirectory(unittest)
endif()

# Build backends based on configuration
if (ONYX_UI_BUILD_BACKEND_CONIO)
    add_subdirectory(backends/conio)
endif()

if (ONYX_UI_BUILD_BACKEND_SDL2)
    add_subdirectory(backends/sdl2)
endif()

add_subdirectory(examples)