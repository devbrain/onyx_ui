# ============================================================================
# Comprehensive widgets demo - builds for both backends from same sources
# ============================================================================

# Conio (terminal) version
add_executable(conio_widgets_demo
        widgets_demo/main.cc
        widgets_demo/widgets_demo.hh
)
target_compile_definitions(conio_widgets_demo PRIVATE
        ONYXUI_USE_CONIO_BACKEND
)
target_link_libraries(conio_widgets_demo PRIVATE onyxui_conio_backend)

# SDL++ (graphical) version
add_executable(sdlpp_widgets_demo
        widgets_demo/main.cc
        widgets_demo/widgets_demo.hh
)
target_compile_definitions(sdlpp_widgets_demo PRIVATE
        ONYXUI_USE_SDLPP_BACKEND
)
target_link_libraries(sdlpp_widgets_demo PRIVATE onyxui_sdlpp_backend)

# ============================================================================
# Simple-shell Hello-World — the one-include, no-template-parameter
# standalone story (WAR-56). Gated on sdlpp being available because the
# bundle header <onyxui/for/sdlpp.hh> pulls in sdlpp-specific pieces.
# ============================================================================

if(TARGET onyxui_sdlpp_backend)
    add_executable(simple_hello simple_hello/main.cc)
    target_link_libraries(simple_hello PRIVATE onyxui_sdlpp_backend)
endif()


