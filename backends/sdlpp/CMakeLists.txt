# =============================================================================
# SDL++ Backend CMake Configuration
# =============================================================================

# Fetch lib_sdlpp via neutrino-cmake (also transitively fetches SDL3,
# onyx_font, and onyx_image when SDLPP_WITH_FONT/SDLPP_WITH_IMAGE are ON —
# both default ON). We also call the onyx_font/onyx_image fetch helpers
# explicitly so reviewers see the graph in one place instead of having to
# trace transitive lib_sdlpp options.
include(${NEUTRINO_CMAKE_DIR}/deps/sdlpp.cmake)
neutrino_fetch_sdlpp()

include(${NEUTRINO_CMAKE_DIR}/deps/onyx_font.cmake)
neutrino_fetch_onyx_font()

include(${NEUTRINO_CMAKE_DIR}/deps/onyx_image.cmake)
neutrino_fetch_onyx_image()

# Create SDL++ backend library using the helper function
add_backend_library(
    NAME sdlpp
    SOURCES
        src/sdlpp_renderer.cc
        src/sdlpp_backend.cc
        src/xpm_parser.cc
        src/simple/app_window.cc
        src/simple/run.cc
    PUBLIC_HEADERS
        include/onyxui/sdlpp/colors.hh
        include/onyxui/sdlpp/geometry.hh
        include/onyxui/sdlpp/sdlpp_backend.hh
        include/onyxui/sdlpp/sdlpp_events.hh
        include/onyxui/sdlpp/sdlpp_renderer.hh
        include/onyxui/sdlpp/xpm_parser.hh
    DEPENDENCIES
        neutrino::sdlpp
        neutrino::onyx_font
        neutrino::onyx_image
)

# Demo applications
if(NEUTRINO_ONYX_UI_BUILD_EXAMPLES)
    add_executable(sdlpp_demo demo.cc)
    target_link_libraries(sdlpp_demo PRIVATE onyxui_sdlpp_backend)
endif()