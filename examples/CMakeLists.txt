add_executable(widgets_demo
        demo.cc
        demo.hh
)

target_link_libraries(widgets_demo PRIVATE onyxui_conio_backend)

add_executable(test_hotkeys_simple
        test_hotkeys_simple.cc
)

target_link_libraries(test_hotkeys_simple PRIVATE onyxui_conio_backend)

add_executable(test_hotkey_debug
        test_hotkey_debug.cc
)

target_link_libraries(test_hotkey_debug PRIVATE onyxui_conio_backend)
