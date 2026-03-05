# =============================================================================
# SDL++ Backend CMake Configuration
# =============================================================================

# Fetch lib_sdlpp via neutrino-cmake
# (automatically fetches SDL3 and includes font/image support)
include(${NEUTRINO_CMAKE_DIR}/deps/sdlpp.cmake)
neutrino_fetch_sdlpp()

# Create SDL++ backend library using the helper function
add_backend_library(
    NAME sdlpp
    SOURCES
        src/sdlpp_renderer.cc
        src/sdlpp_backend.cc
        src/xpm_parser.cc
        src/tile_renderer.cc
        src/tile_backend.cc
        src/procedural_atlas.cc
    PUBLIC_HEADERS
        include/onyxui/sdlpp/colors.hh
        include/onyxui/sdlpp/geometry.hh
        include/onyxui/sdlpp/sdlpp_backend.hh
        include/onyxui/sdlpp/sdlpp_events.hh
        include/onyxui/sdlpp/sdlpp_renderer.hh
        include/onyxui/sdlpp/xpm_parser.hh
        include/onyxui/tile/tile_backend.hh
        include/onyxui/tile/tile_backend_impl.hh
        include/onyxui/tile/procedural_atlas.hh
    DEPENDENCIES
        neutrino::sdlpp
)

# Demo applications
if(NEUTRINO_ONYX_UI_BUILD_EXAMPLES)
    add_executable(sdlpp_demo demo.cc)
    target_link_libraries(sdlpp_demo PRIVATE onyxui_sdlpp_backend)


endif()

# Tile backend demo - verifies run_app template instantiation
add_executable(tile_demo tile_demo.cc)
target_link_libraries(tile_demo PRIVATE onyxui_sdlpp_backend)

# Strategy UI demo - implements design.txt layout
add_executable(strategy_ui_demo strategy_ui_demo.cc)
target_link_libraries(strategy_ui_demo PRIVATE onyxui_sdlpp_backend)