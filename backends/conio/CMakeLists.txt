include(FetchContent)

# Fetch termbox2 (single-file header library)
FetchContent_Declare(
        termbox2
        GIT_REPOSITORY https://github.com/termbox/termbox2.git
        GIT_TAG master
)

FetchContent_MakeAvailable(termbox2)

# Create interface library for termbox2
add_library(termbox2_lib INTERFACE)
target_include_directories(termbox2_lib INTERFACE ${termbox2_SOURCE_DIR})

# Create conio backend library using the helper function
add_backend_library(
    NAME conio
    SOURCES
        src/conio_renderer.cc
        src/vram.cc
        src/conio_backend.cc
    PUBLIC_HEADERS
        include/onyxui/conio/colors.hh
        include/onyxui/conio/conio_backend.hh
        include/onyxui/conio/conio_events.hh
        include/onyxui/conio/conio_renderer.hh
        include/onyxui/conio/geometry.hh
        include/onyxui/conio/termbox2_wrappers.hh
    PRIVATE_SOURCES
        src/conio_themes.hh
        src/dos_chars.h
        src/vram.hh
    DEPENDENCIES
        termbox2_lib
    PRIVATE_DEPENDENCIES
        utf8cpp::utf8cpp  # Required for UTF-8 text measurement in renderer
)


# Conio backend integration tests (built as shared library for doctest DLL)
if(NEUTRINO_ONYX_UI_BUILD_TESTS)
    add_subdirectory(unittest)
endif()
