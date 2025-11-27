// Copyright (c) 2025 OnyxUI Framework
// SPDX-License-Identifier: MIT

#include <doctest/doctest.h>

#include <onyxui/widgets/input/checkbox.hh>
#include "../utils/test_backend.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;

using Backend = test_backend;

// Test wrapper that exposes protected methods
template<UIBackend BackendT>
class test_checkbox : public checkbox<BackendT> {
public:
    using checkbox<BackendT>::checkbox;
    using checkbox<BackendT>::handle_event;
    using checkbox<BackendT>::handle_semantic_action;
};

// ===== Construction Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - Default construction") {
    test_checkbox<Backend> cb;

    REQUIRE(cb.text() == "");
    REQUIRE_FALSE(cb.is_checked());
    REQUIRE(cb.get_tri_state() == tri_state::unchecked);
    REQUIRE_FALSE(cb.is_tri_state_enabled());
    REQUIRE(cb.mnemonic() == '\0');
    REQUIRE(cb.is_focusable());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - Construction with text") {
    test_checkbox<Backend> cb("Enable notifications");

    REQUIRE(cb.text() == "Enable notifications");
    REQUIRE_FALSE(cb.is_checked());
    REQUIRE(cb.get_tri_state() == tri_state::unchecked);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - Construction with text and initial state") {
    test_checkbox<Backend> cb_checked("Option 1", true);
    test_checkbox<Backend> cb_unchecked("Option 2", false);

    REQUIRE(cb_checked.is_checked());
    REQUIRE(cb_checked.get_tri_state() == tri_state::checked);

    REQUIRE_FALSE(cb_unchecked.is_checked());
    REQUIRE(cb_unchecked.get_tri_state() == tri_state::unchecked);
}

// ===== State Management Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - set_checked(true) sets state to checked") {
    test_checkbox<Backend> cb;

    cb.set_checked(true);

    REQUIRE(cb.is_checked());
    REQUIRE(cb.get_tri_state() == tri_state::checked);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - set_checked(false) sets state to unchecked") {
    test_checkbox<Backend> cb("Test", true);

    cb.set_checked(false);

    REQUIRE_FALSE(cb.is_checked());
    REQUIRE(cb.get_tri_state() == tri_state::unchecked);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - set_tri_state() sets any tri-state value") {
    test_checkbox<Backend> cb;
    cb.set_tri_state_enabled(true);

    cb.set_tri_state(tri_state::checked);
    REQUIRE(cb.get_tri_state() == tri_state::checked);

    cb.set_tri_state(tri_state::indeterminate);
    REQUIRE(cb.get_tri_state() == tri_state::indeterminate);

    cb.set_tri_state(tri_state::unchecked);
    REQUIRE(cb.get_tri_state() == tri_state::unchecked);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - Cannot set indeterminate when tri-state disabled") {
    test_checkbox<Backend> cb;
    cb.set_tri_state_enabled(false);  // Ensure disabled

    cb.set_tri_state(tri_state::indeterminate);

    // Should remain unchecked (indeterminate rejected)
    REQUIRE(cb.get_tri_state() == tri_state::unchecked);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - tri_state_enabled flag") {
    test_checkbox<Backend> cb;

    REQUIRE_FALSE(cb.is_tri_state_enabled());

    cb.set_tri_state_enabled(true);
    REQUIRE(cb.is_tri_state_enabled());

    cb.set_tri_state_enabled(false);
    REQUIRE_FALSE(cb.is_tri_state_enabled());
}

// ===== Signal Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - toggled signal emitted on set_checked(true)") {
    test_checkbox<Backend> cb;

    bool signal_emitted = false;
    bool signal_value = false;

    cb.toggled.connect([&](bool checked) {
        signal_emitted = true;
        signal_value = checked;
    });

    cb.set_checked(true);

    REQUIRE(signal_emitted);
    REQUIRE(signal_value == true);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - toggled signal emitted on set_checked(false)") {
    test_checkbox<Backend> cb("Test", true);

    bool signal_emitted = false;
    bool signal_value = true;

    cb.toggled.connect([&](bool checked) {
        signal_emitted = true;
        signal_value = checked;
    });

    cb.set_checked(false);

    REQUIRE(signal_emitted);
    REQUIRE(signal_value == false);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - state_changed signal emitted on all state changes") {
    test_checkbox<Backend> cb;
    cb.set_tri_state_enabled(true);

    int signal_count = 0;
    tri_state last_state = tri_state::unchecked;

    cb.state_changed.connect([&](tri_state state) {
        signal_count++;
        last_state = state;
    });

    cb.set_checked(true);
    REQUIRE(signal_count == 1);
    REQUIRE(last_state == tri_state::checked);

    cb.set_tri_state(tri_state::indeterminate);
    REQUIRE(signal_count == 2);
    REQUIRE(last_state == tri_state::indeterminate);

    cb.set_tri_state(tri_state::unchecked);
    REQUIRE(signal_count == 3);
    REQUIRE(last_state == tri_state::unchecked);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - toggled NOT emitted when entering indeterminate state") {
    test_checkbox<Backend> cb("Test", true);
    cb.set_tri_state_enabled(true);

    bool toggled_emitted = false;
    bool state_changed_emitted = false;

    cb.toggled.connect([&](bool) {
        toggled_emitted = true;
    });

    cb.state_changed.connect([&](tri_state) {
        state_changed_emitted = true;
    });

    cb.set_tri_state(tri_state::indeterminate);

    // state_changed should emit, but toggled should NOT
    REQUIRE(state_changed_emitted);
    REQUIRE_FALSE(toggled_emitted);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - Signals not emitted when state doesn't change") {
    test_checkbox<Backend> cb("Test", true);

    int toggled_count = 0;
    int state_changed_count = 0;

    cb.toggled.connect([&](bool) { toggled_count++; });
    cb.state_changed.connect([&](tri_state) { state_changed_count++; });

    cb.set_checked(true);  // Already checked

    REQUIRE(toggled_count == 0);
    REQUIRE(state_changed_count == 0);
}

// ===== Event Handling Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - Mouse click toggles unchecked to checked") {
    test_checkbox<Backend> cb;

    mouse_event mouse_evt{.x = 0, .y = 0, .btn = mouse_event::button::left,
                          .act = mouse_event::action::press, .modifiers = {}};

    const bool handled = cb.handle_event(ui_event{mouse_evt}, event_phase::target);

    REQUIRE(handled);
    REQUIRE(cb.is_checked());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - Mouse click toggles checked to unchecked") {
    test_checkbox<Backend> cb("Test", true);

    mouse_event mouse_evt{.x = 0, .y = 0, .btn = mouse_event::button::left,
                          .act = mouse_event::action::press, .modifiers = {}};

    const bool handled = cb.handle_event(ui_event{mouse_evt}, event_phase::target);

    REQUIRE(handled);
    REQUIRE_FALSE(cb.is_checked());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - Space key toggles state") {
    test_checkbox<Backend> cb;

    const bool handled = cb.handle_semantic_action(hotkey_action::activate_widget);

    REQUIRE(handled);
    REQUIRE(cb.is_checked());

    // Toggle again
    const bool handled2 = cb.handle_semantic_action(hotkey_action::activate_widget);

    REQUIRE(handled2);
    REQUIRE_FALSE(cb.is_checked());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - Events ignored when disabled") {
    test_checkbox<Backend> cb;
    cb.set_enabled(false);

    // Try mouse click
    mouse_event mouse_evt{.x = 0, .y = 0, .btn = mouse_event::button::left,
                          .act = mouse_event::action::press, .modifiers = {}};
    cb.handle_event(ui_event{mouse_evt}, event_phase::target);

    // State should remain unchanged
    REQUIRE_FALSE(cb.is_checked());

    // Try Space key
    const bool key_handled = cb.handle_semantic_action(hotkey_action::activate_widget);

    REQUIRE_FALSE(key_handled);
    REQUIRE_FALSE(cb.is_checked());
}

// ===== Tri-State Toggle Behavior Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - Indeterminate → Checked on user toggle") {
    test_checkbox<Backend> cb;
    cb.set_tri_state_enabled(true);
    cb.set_tri_state(tri_state::indeterminate);

    // Simulate user Space key press
    cb.handle_semantic_action(hotkey_action::activate_widget);

    // Should exit indeterminate to checked
    REQUIRE(cb.get_tri_state() == tri_state::checked);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - Normal toggle skips indeterminate state") {
    test_checkbox<Backend> cb;
    cb.set_tri_state_enabled(true);

    // Unchecked → Checked (skip indeterminate)
    cb.handle_semantic_action(hotkey_action::activate_widget);
    REQUIRE(cb.get_tri_state() == tri_state::checked);

    // Checked → Unchecked (skip indeterminate)
    cb.handle_semantic_action(hotkey_action::activate_widget);
    REQUIRE(cb.get_tri_state() == tri_state::unchecked);
}

// ===== Text and Mnemonic Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - set_text() updates label") {
    test_checkbox<Backend> cb;

    cb.set_text("New label");

    REQUIRE(cb.text() == "New label");
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - set_mnemonic() stores mnemonic character") {
    test_checkbox<Backend> cb;

    cb.set_mnemonic('e');

    REQUIRE(cb.mnemonic() == 'e');
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - Mnemonic can be cleared") {
    test_checkbox<Backend> cb;
    cb.set_mnemonic('e');

    cb.set_mnemonic('\0');

    REQUIRE(cb.mnemonic() == '\0');
}

// ===== Layout Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - Measure returns correct size for empty text") {
    test_checkbox<Backend> cb;

    const auto size = cb.measure(100_lu, 100_lu);

    // Minimum size: icon (3 chars), no spacing when no text
    REQUIRE(size.width.to_int() == 3);
    REQUIRE(size.height.to_int() == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - Measure returns correct size with text") {
    test_checkbox<Backend> cb("Enable notifications");

    const auto size = cb.measure(100_lu, 100_lu);

    // Size: icon (3) + spacing (1) + text width
    REQUIRE(size.width.to_int() > 4);  // More than just the icon
    REQUIRE(size.height.to_int() == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - Height is always 1 line") {
    test_checkbox<Backend> cb1;
    test_checkbox<Backend> cb2("Short");
    test_checkbox<Backend> cb3("Very long label text that spans many characters");

    const auto size1 = cb1.measure(100_lu, 100_lu);
    const auto size2 = cb2.measure(100_lu, 100_lu);
    const auto size3 = cb3.measure(100_lu, 100_lu);

    REQUIRE(size1.height.to_int() == 1);
    REQUIRE(size2.height.to_int() == 1);
    REQUIRE(size3.height.to_int() == 1);
}

// ===== Integration Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - Multiple independent checkboxes") {
    test_checkbox<Backend> cb1("Option 1");
    test_checkbox<Backend> cb2("Option 2");
    test_checkbox<Backend> cb3("Option 3");

    // Check different combinations
    cb1.set_checked(true);
    cb3.set_checked(true);

    REQUIRE(cb1.is_checked());
    REQUIRE_FALSE(cb2.is_checked());
    REQUIRE(cb3.is_checked());

    // Checkboxes are independent (not mutually exclusive)
    cb2.set_checked(true);

    REQUIRE(cb1.is_checked());
    REQUIRE(cb2.is_checked());
    REQUIRE(cb3.is_checked());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - Tri-state select-all scenario") {
    test_checkbox<Backend> select_all("Select All");
    select_all.set_tri_state_enabled(true);

    // Scenario: 0 items selected → unchecked
    select_all.set_tri_state(tri_state::unchecked);
    REQUIRE(select_all.get_tri_state() == tri_state::unchecked);

    // User clicks "Select All" → all items selected
    select_all.handle_semantic_action(hotkey_action::activate_widget);
    REQUIRE(select_all.is_checked());

    // One item unchecked programmatically → indeterminate
    select_all.set_tri_state(tri_state::indeterminate);
    REQUIRE(select_all.get_tri_state() == tri_state::indeterminate);

    // User clicks again → checked (select all again)
    select_all.handle_semantic_action(hotkey_action::activate_widget);
    REQUIRE(select_all.is_checked());

    // User clicks again → unchecked (deselect all)
    select_all.handle_semantic_action(hotkey_action::activate_widget);
    REQUIRE_FALSE(select_all.is_checked());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "checkbox - Focus management") {
    test_checkbox<Backend> cb("Test");

    REQUIRE(cb.is_focusable());

    // Note: Full focus testing requires focus_manager integration
}
