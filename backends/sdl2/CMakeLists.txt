# SDL2 Backend CMake Configuration

# Find SDL2 and SDL2_ttf packages
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

# Create SDL2 backend library using the helper function
add_backend_library(
    NAME sdl2
    SOURCES
        src/sdl2_renderer.cc
        src/sdl2_backend.cc
    PUBLIC_HEADERS
        include/onyxui/sdl2/colors.hh
        include/onyxui/sdl2/geometry.hh
        include/onyxui/sdl2/sdl2_backend.hh
        include/onyxui/sdl2/sdl2_events.hh
        include/onyxui/sdl2/sdl2_renderer.hh
    PRIVATE_SOURCES
        src/sdl2_themes.hh
    DEPENDENCIES
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
)

# Demo application
add_executable(sdl2_demo
        demo.cc
)

target_link_libraries(sdl2_demo PRIVATE onyxui_sdl2_backend)
