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

add_library(onyxui_conio_backend STATIC
        include/onyxui/conio/colors.hh
        include/onyxui/conio/conio_backend.hh
        include/onyxui/conio/conio_events.hh
        include/onyxui/conio/geometry.hh
        src/conio_themes.hh
        src/conio_renderer.cc
        src/dos_chars.h
        src/vram.cc
        src/vram.hh
        src/conio_backend.cc
)

target_link_libraries(onyxui_conio_backend
        PUBLIC
        termbox2_lib
        onyxui
        PRIVATE
        thirdparty::utf8cpp  # Required for UTF-8 text measurement in renderer
)
target_include_directories(onyxui_conio_backend
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)


add_executable(dos_theme_showcase
        dos_theme_showcase.cc
        dos_theme.hh
)

target_link_libraries(dos_theme_showcase PRIVATE onyxui_conio_backend)


