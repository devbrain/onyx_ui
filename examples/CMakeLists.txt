# Basic demo (original)
add_executable(demo
        demo.cc
        demo.hh
)

target_link_libraries(demo PRIVATE onyxui_conio_backend)

# Comprehensive widgets demo (new)
add_executable(widgets_demo
        widgets_demo/main.cc
        widgets_demo/widgets_demo.hh
)

target_link_libraries(widgets_demo PRIVATE onyxui_conio_backend)

