# ============================================================================
# Comprehensive widgets demo — simple-shell sdlpp port (WAR-57).
#
# The conio variant was removed when widgets_demo was ported onto
# <onyxui/for/sdlpp.hh>. Conio parity returns under WAR-59 once the
# conio simple-shell impls (app_window/run/dialogs) land.
# ============================================================================

if(TARGET onyxui_sdlpp_backend)
    add_executable(sdlpp_widgets_demo
            widgets_demo/main.cc
            widgets_demo/widgets_demo.hh
    )
    target_link_libraries(sdlpp_widgets_demo PRIVATE onyxui_sdlpp_backend)
endif()

# ============================================================================
# Simple-shell Hello-World — the one-include, no-template-parameter
# standalone story (WAR-56). Gated on sdlpp being available because the
# bundle header <onyxui/for/sdlpp.hh> pulls in sdlpp-specific pieces.
# ============================================================================

if(TARGET onyxui_sdlpp_backend)
    add_executable(simple_hello simple_hello/main.cc)
    target_link_libraries(simple_hello PRIVATE onyxui_sdlpp_backend)
endif()

# Conio parity (WAR-59) — same main.cc, bundle header swap via
# ONYXUI_EXAMPLE_USE_CONIO. Proves the simple-shell API is genuinely
# backend-agnostic at the source level.
if(TARGET onyxui_conio_backend)
    add_executable(simple_hello_conio simple_hello/main.cc)
    target_compile_definitions(simple_hello_conio PRIVATE ONYXUI_EXAMPLE_USE_CONIO=1)
    target_link_libraries(simple_hello_conio PRIVATE onyxui_conio_backend)
endif()
