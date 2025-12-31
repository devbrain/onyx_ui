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
    PUBLIC_HEADERS
        include/onyxui/sdlpp/colors.hh
        include/onyxui/sdlpp/geometry.hh
        include/onyxui/sdlpp/sdlpp_backend.hh
        include/onyxui/sdlpp/sdlpp_events.hh
        include/onyxui/sdlpp/sdlpp_renderer.hh
    DEPENDENCIES
        neutrino::sdlpp
)

# Demo application
if(NEUTRINO_ONYX_UI_BUILD_EXAMPLES)
    add_executable(sdlpp_demo demo.cc)
    target_link_libraries(sdlpp_demo PRIVATE onyxui_sdlpp_backend)
endif()
