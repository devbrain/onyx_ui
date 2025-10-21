add_executable(widgets_demo
        demo.cc
        demo.hh
)

target_link_libraries(widgets_demo PRIVATE onyxui_conio_backend)
