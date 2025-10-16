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
        conio_app.hh
        conio_app.cc)

# Link with termbox2 header library
target_link_libraries(conio PRIVATE termbox2_lib onyx_ui)