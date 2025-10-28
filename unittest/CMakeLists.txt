add_subdirectory(test_support)

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
        core/test_background_renderer.cc
        events/test_events.cc
        events/test_event_target.cc
        events/test_ui_event.cc
        events/test_event_target_ui_event.cc
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
        widgets/test_button_mouse_tracking.cc
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
        widgets/test_submenu_infrastructure.cc
        widgets/test_menu_system.cc
        widgets/test_menu_item_measurement.cc
        widgets/test_menu_border_layout.cc
        widgets/test_menu_hotkey_integration.cc
        widgets/test_theme_switching.cc
        widgets/test_status_bar.cc
        widgets/test_group_box.cc
        widgets/test_panel_layout.cc
        widgets/test_group_box_layout.cc
        widgets/test_layout_edge_cases.cc
        widgets/test_complex_layouts.cc
        widgets/test_theme_layout_integration.cc
        widgets/test_stateful_widget.cc
        widgets/test_separator.cc
        hotkeys/test_hotkeys.cc
        hotkeys/test_hotkey_manager.cc
        hotkeys/test_hotkey_ui_event.cc
        hotkeys/test_hotkey_scheme.cc
        hotkeys/test_key_chord.cc
        hotkeys/test_semantic_action_guard.cc
        # YAML infrastructure tests (backend-agnostic, use test::test_types)
        reflection/test_fkyaml_basic.cc
        reflection/test_color_yaml.cc
        reflection/test_enum_reflection.cc
        reflection/test_nested_structs.cc
        reflection/test_font_reflection.cc
        # Layer management tests
        layer/test_layer_dirty_regions.cc
        # Integration tests moved to backends/conio/unittest/ (conio_unittest.so)
)
target_compile_options(ui_unittest PRIVATE ${ONYX_UI_WARNING_FLAGS})
target_include_directories(ui_unittest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# Disable logging in unit tests (override inherited DEBUG level)
target_compile_options(ui_unittest PRIVATE
    -ULOGGER_MIN_LEVEL                      # Undefine inherited value
    -DLOGGER_MIN_LEVEL=LOGGER_LEVEL_ERROR   # Set to ERROR only
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

