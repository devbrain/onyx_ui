# Use doctest from ext/ directory (consistent with fkyaml, utf8cpp pattern)
# The doctest target is provided by the main CMakeLists.txt via ext/doctest
if(NOT TARGET doctest::doctest)
    message(STATUS "doctest not available, using ext/doctest")
    add_subdirectory(${CMAKE_SOURCE_DIR}/ext/doctest ${CMAKE_BINARY_DIR}/ext/doctest)
endif()

if(TARGET doctest::doctest)
    message(STATUS "doctest available for testing")
endif()

# Note: test_support library is defined in main CMakeLists.txt (before backends)
# This ensures backend test DLLs can depend on it

add_executable(ui_unittest
        core/test_element.cc
        core/test_signal_slot.cc
        core/test_rule_of_five.cc
        core/test_dirty_tracking.cc
        core/test_exception_safety.cc
        core/test_layer_manager.cc
        core/test_layer_manager_lifetime.cc
        core/test_layer_manager_events.cc
        core/test_scoped_layer.cc
        core/test_render_context.cc
        core/test_content_area.cc
        events/test_events.cc
        events/test_event_target.cc
        focus/test_focus_manager.cc
        layout/test_absolute.cc
        layout/test_anchor.cc
        layout/test_composition.cc
        layout/test_grid.cc
        layout/test_linear.cc
        layout/test_size_constraint.cc
        main.cc
        utils/test_backend.hh
        utils/test_helpers.hh
        utils/test_canvas.hh
        utils/test_canvas_backend.hh
        utils/layout_assertions.hh
        utils/test_safe_math.cc
        widgets/test_widgets.cc
        widgets/widgets.hh
        widgets/test_hbox.cc
        widgets/test_vbox.cc
        widgets/test_grid.cc
        widgets/test_anchor_panel.cc
        widgets/test_absolute_panel.cc
        widgets/test_button.cc
        widgets/test_label.cc
        widgets/test_panel.cc
        widgets/test_spacer.cc
        utils/warnings.hh
        widgets/test_spring.cc
        widgets/test_layout_correctness.cc
        widgets/test_factory_functions.cc
        widgets/test_actions.cc
        widgets/test_mnemonics.cc
        widgets/test_menus.cc
        widgets/test_theme_switching.cc
        widgets/test_status_bar.cc
        widgets/test_group_box.cc
        widgets/test_panel_layout.cc
        widgets/test_group_box_layout.cc
        widgets/test_layout_edge_cases.cc
        widgets/test_complex_layouts.cc
        widgets/test_theme_layout_integration.cc
        hotkeys/test_hotkeys.cc
        hotkeys/test_hotkey_manager.cc
        # YAML infrastructure tests (backend-agnostic, use test::test_types)
        reflection/test_fkyaml_basic.cc
        reflection/test_color_yaml.cc
        reflection/test_enum_reflection.cc
        reflection/test_nested_structs.cc
        reflection/test_font_reflection.cc
        # Integration tests moved to backends/conio/unittest/ (conio_unittest.so)
)
target_compile_options(ui_unittest PRIVATE ${ONYX_UI_WARNING_FLAGS})

# No backend-specific includes! All core and infrastructure tests are backend-agnostic
target_include_directories(ui_unittest PRIVATE
    ${CMAKE_BINARY_DIR}/include  # For generated export headers
)

target_link_libraries(ui_unittest PRIVATE
    onyxui
    doctest::doctest
    thirdparty::utf8cpp  # Required by test_backend.hh for measure_text()
    test_support         # Test utilities and generated config
)

# Link with backend integration test DLLs
# Tests are automatically discovered from DLLs via doctest's DLL mechanism
if(ONYX_UI_BUILD_BACKEND_CONIO AND TARGET conio_unittest)
    target_link_libraries(ui_unittest PRIVATE conio_unittest)
    target_compile_definitions(ui_unittest PRIVATE ONYX_UI_BUILD_BACKEND_CONIO=1)
    message(STATUS "ui_unittest: Linked with conio_unittest DLL (doctest auto-discovery)")
endif()