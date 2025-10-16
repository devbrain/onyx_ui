cmake_minimum_required(VERSION 3.15)
project(onyx_ui LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ONYX_UI_BUILD_TEST "Build unittests for onyx_ui" ON)
option(ONYX_UI_ENABLE_CLANG_TIDY "Enable clang-tidy checks" OFF)
option(ONYX_UI_ENABLE_CPPCHECK "Enable cppcheck static analysis" OFF)

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
# First check if failsafe target already exists (from parent project)
if(TARGET failsafe)
    message(STATUS "Found failsafe target from parent project")
    if(NOT TARGET thirdparty::failsafe)
        add_library(thirdparty::failsafe ALIAS failsafe)
    endif()
elseif(TARGET thirdparty::failsafe)
    message(STATUS "Found thirdparty::failsafe target from parent project")
else()
    # Try to find failsafe locally
    find_package(failsafe QUIET)

    if(NOT failsafe_FOUND)
        message(STATUS "failsafe not found locally, fetching from GitHub...")

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
        add_library(thirdparty::failsafe ALIAS failsafe)
    else()
        message(STATUS "Found failsafe locally")
        if(NOT TARGET thirdparty::failsafe)
            add_library(thirdparty::failsafe ALIAS failsafe::failsafe)
        endif()
    endif()
endif()

# ===========================================================================

add_library(onyx_ui INTERFACE
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
)

target_include_directories(onyx_ui INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(onyx_ui INTERFACE thirdparty::failsafe)

if (ONYX_UI_BUILD_TEST)
    add_subdirectory(unittest)
endif()


add_subdirectory(backends/conio)