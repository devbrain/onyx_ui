// Copyright (c) 2025 OnyxUI Framework
// SPDX-License-Identifier: MIT

#include <doctest/doctest.h>

#include <onyxui/widgets/input/radio_button.hh>
#include <onyxui/widgets/input/button_group.hh>
#include "../utils/test_backend.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;

using Backend = test_backend;

// Test wrapper that exposes protected methods
template<UIBackend BackendT>
class test_radio_button : public radio_button<BackendT> {
public:
    using radio_button<BackendT>::radio_button;
    using radio_button<BackendT>::handle_event;
    using radio_button<BackendT>::handle_semantic_action;
};

// ===== Construction Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - Default construction") {
    test_radio_button<Backend> rb;

    REQUIRE(rb.text() == "");
    REQUIRE_FALSE(rb.is_checked());
    REQUIRE(rb.group() == nullptr);
    REQUIRE(rb.mnemonic() == '\0');
    REQUIRE(rb.is_focusable());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - Construction with text") {
    test_radio_button<Backend> rb("Option 1");

    REQUIRE(rb.text() == "Option 1");
    REQUIRE_FALSE(rb.is_checked());
    REQUIRE(rb.group() == nullptr);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - Construction with text and initial state") {
    test_radio_button<Backend> rb_checked("Option 1", true);
    test_radio_button<Backend> rb_unchecked("Option 2", false);

    REQUIRE(rb_checked.is_checked());
    REQUIRE_FALSE(rb_unchecked.is_checked());
}

// ===== State Management Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - set_checked(true) sets state to checked") {
    test_radio_button<Backend> rb;

    rb.set_checked(true);

    REQUIRE(rb.is_checked());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - set_checked(false) sets state to unchecked") {
    test_radio_button<Backend> rb("Test", true);

    rb.set_checked(false);

    REQUIRE_FALSE(rb.is_checked());
}

// ===== Signal Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - toggled signal emitted on set_checked(true)") {
    test_radio_button<Backend> rb;

    bool signal_emitted = false;
    bool signal_value = false;

    rb.toggled.connect([&](bool checked) {
        signal_emitted = true;
        signal_value = checked;
    });

    rb.set_checked(true);

    REQUIRE(signal_emitted);
    REQUIRE(signal_value == true);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - toggled signal emitted on set_checked(false)") {
    test_radio_button<Backend> rb("Test", true);

    bool signal_emitted = false;
    bool signal_value = true;

    rb.toggled.connect([&](bool checked) {
        signal_emitted = true;
        signal_value = checked;
    });

    rb.set_checked(false);

    REQUIRE(signal_emitted);
    REQUIRE(signal_value == false);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - Signals not emitted when state doesn't change") {
    test_radio_button<Backend> rb("Test", true);

    int toggled_count = 0;

    rb.toggled.connect([&](bool) { toggled_count++; });

    rb.set_checked(true);  // Already checked

    REQUIRE(toggled_count == 0);
}

// ===== Event Handling Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - Mouse click checks unchecked button") {
    test_radio_button<Backend> rb;

    mouse_event mouse_evt{.x = 0, .y = 0, .btn = mouse_event::button::left,
                          .act = mouse_event::action::press, .modifiers = {}};

    const bool handled = rb.handle_event(ui_event{mouse_evt}, event_phase::target);

    REQUIRE(handled);
    REQUIRE(rb.is_checked());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - Mouse click on checked button does nothing") {
    test_radio_button<Backend> rb("Test", true);

    mouse_event mouse_evt{.x = 0, .y = 0, .btn = mouse_event::button::left,
                          .act = mouse_event::action::press, .modifiers = {}};

    // Click on already checked button - should do nothing
    rb.handle_event(ui_event{mouse_evt}, event_phase::target);

    REQUIRE(rb.is_checked());  // Still checked
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - Space key checks unchecked button") {
    test_radio_button<Backend> rb;

    const bool handled = rb.handle_semantic_action(hotkey_action::activate_widget);

    REQUIRE(handled);
    REQUIRE(rb.is_checked());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - Space key on checked button does nothing") {
    test_radio_button<Backend> rb("Test", true);

    const bool handled = rb.handle_semantic_action(hotkey_action::activate_widget);

    REQUIRE_FALSE(handled);  // Event not handled (no state change)
    REQUIRE(rb.is_checked());  // Still checked
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - Events ignored when disabled") {
    test_radio_button<Backend> rb;
    rb.set_enabled(false);

    // Try mouse click
    mouse_event mouse_evt{.x = 0, .y = 0, .btn = mouse_event::button::left,
                          .act = mouse_event::action::press, .modifiers = {}};
    rb.handle_event(ui_event{mouse_evt}, event_phase::target);

    // State should remain unchanged
    REQUIRE_FALSE(rb.is_checked());

    // Try Space key
    const bool key_handled = rb.handle_semantic_action(hotkey_action::activate_widget);

    REQUIRE_FALSE(key_handled);
    REQUIRE_FALSE(rb.is_checked());
}

// ===== Text and Mnemonic Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - set_text() updates label") {
    test_radio_button<Backend> rb;

    rb.set_text("New label");

    REQUIRE(rb.text() == "New label");
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - set_mnemonic() stores mnemonic character") {
    test_radio_button<Backend> rb;

    rb.set_mnemonic('s');

    REQUIRE(rb.mnemonic() == 's');
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - Mnemonic can be cleared") {
    test_radio_button<Backend> rb;
    rb.set_mnemonic('s');

    rb.set_mnemonic('\0');

    REQUIRE(rb.mnemonic() == '\0');
}

// ===== Group Management Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - set_group() updates group pointer") {
    test_radio_button<Backend> rb;
    button_group<Backend> group;

    rb.set_group(&group);

    REQUIRE(rb.group() == &group);

    rb.set_group(nullptr);

    REQUIRE(rb.group() == nullptr);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - Checking button notifies group") {
    test_radio_button<Backend> rb1("Option 1");
    test_radio_button<Backend> rb2("Option 2", true);  // Initially checked

    button_group<Backend> group;
    group.add_button(&rb1, 0);
    group.add_button(&rb2, 1);

    REQUIRE(rb2.is_checked());
    REQUIRE_FALSE(rb1.is_checked());

    // Check rb1 - should uncheck rb2 via group
    rb1.set_checked(true);

    REQUIRE(rb1.is_checked());
    REQUIRE_FALSE(rb2.is_checked());  // Unchecked by group
}

// ===== Layout Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - Measure returns correct size for empty text") {
    test_radio_button<Backend> rb;

    const auto size = rb.measure(100, 100);

    // Minimum size: icon (3 chars), no spacing when no text
    REQUIRE(size_utils::get_width(size) == 3);
    REQUIRE(size_utils::get_height(size) == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - Measure returns correct size with text") {
    test_radio_button<Backend> rb("Option 1");

    const auto size = rb.measure(100, 100);

    // Size: icon (3) + spacing (1) + text width
    REQUIRE(size_utils::get_width(size) > 4);  // More than just the icon
    REQUIRE(size_utils::get_height(size) == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - Height is always 1 line") {
    test_radio_button<Backend> rb1;
    test_radio_button<Backend> rb2("Short");
    test_radio_button<Backend> rb3("Very long label text that spans many characters");

    const auto size1 = rb1.measure(100, 100);
    const auto size2 = rb2.measure(100, 100);
    const auto size3 = rb3.measure(100, 100);

    REQUIRE(size_utils::get_height(size1) == 1);
    REQUIRE(size_utils::get_height(size2) == 1);
    REQUIRE(size_utils::get_height(size3) == 1);
}

// ===== Integration Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - Multiple independent buttons without group") {
    test_radio_button<Backend> rb1("Option 1");
    test_radio_button<Backend> rb2("Option 2");
    test_radio_button<Backend> rb3("Option 3");

    // Without group, they're independent (not recommended, but possible)
    rb1.set_checked(true);
    rb2.set_checked(true);

    REQUIRE(rb1.is_checked());
    REQUIRE(rb2.is_checked());  // Both checked (independent)
    REQUIRE_FALSE(rb3.is_checked());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "radio_button - Focus management") {
    test_radio_button<Backend> rb("Test");

    REQUIRE(rb.is_focusable());

    // Note: Full focus testing requires focus_manager integration
}
