include(FetchContent)

# Fetch termbox2 (single-file header library)
FetchContent_Declare(
    termbox2
    GIT_REPOSITORY https://github.com/termbox/termbox2.git
    GIT_TAG        master
)

FetchContent_MakeAvailable(termbox2)

# Create interface library for termbox2
add_library(termbox2_lib INTERFACE)
target_include_directories(termbox2_lib INTERFACE ${termbox2_SOURCE_DIR})

add_executable(conio main.cc
        dos_chars.h
        vram.cc
        vram.hh
        colors.hh
        rect.hh
        drawing_utils.cc
        drawing_utils.hh
        conio_backend.hh
        conio_renderer.cc
       )

# Link with termbox2 header library and utf8cpp (for text measurement)
target_link_libraries(conio PRIVATE
    termbox2_lib
    onyx_ui
    thirdparty::utf8cpp  # Required for UTF-8 text measurement in renderer
)

# Optional: DOS theme showcase demo
option(BUILD_DOS_THEME_SHOWCASE "Build the DOS theme showcase demo" ON)

if(BUILD_DOS_THEME_SHOWCASE)
    add_executable(dos_theme_showcase
        dos_theme_showcase.cc
        dos_theme.hh
        vram.cc
        vram.hh
        colors.hh
        rect.hh
        dos_chars.h
        conio_backend.hh
        conio_renderer.cc
    )

    target_link_libraries(dos_theme_showcase PRIVATE
        termbox2_lib
        onyx_ui
        thirdparty::utf8cpp
    )
endif()

# Optional: Text attributes demo
option(BUILD_TEXT_ATTRIBUTES_DEMO "Build the text attributes demo" OFF)

if(BUILD_TEXT_ATTRIBUTES_DEMO)
    add_executable(text_attributes_demo
        text_attributes_demo.cc
        vram.cc
        vram.hh
        colors.hh
        rect.hh
    )

    target_link_libraries(text_attributes_demo PRIVATE
        termbox2_lib
    )
endif()

# Event tester
add_executable(test_tb_events
    test_tb_events.cc
)

target_link_libraries(test_tb_events PRIVATE
    termbox2_lib
)