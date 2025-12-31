# Basic demo (original)
add_executable(demo
        demo.cc
        demo.hh
)

target_link_libraries(demo PRIVATE onyxui_conio_backend)

# ============================================================================
# Comprehensive widgets demo - builds for both backends from same sources
# ============================================================================

# Conio (terminal) version
add_executable(conio_widgets_demo
        widgets_demo/main.cc
        widgets_demo/widgets_demo.hh
)
target_link_libraries(conio_widgets_demo PRIVATE onyxui_conio_backend)

# SDL++ (graphical) version
add_executable(sdlpp_widgets_demo
        widgets_demo/main.cc
        widgets_demo/widgets_demo.hh
)
target_compile_definitions(sdlpp_widgets_demo PRIVATE USE_SDLPP_BACKEND)
target_link_libraries(sdlpp_widgets_demo PRIVATE onyxui_sdlpp_backend)

# Keep old target name as alias to conio version for compatibility
add_executable(widgets_demo
        widgets_demo/main.cc
        widgets_demo/widgets_demo.hh
)
target_link_libraries(widgets_demo PRIVATE onyxui_conio_backend)

