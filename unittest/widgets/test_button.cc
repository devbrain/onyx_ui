//
// Created by igor on 16/10/2025.
//
#include <doctest/doctest.h>

#include "../utils/test_backend.hh"
#include "../utils/rule_of_five_tests.hh"
#include "../utils/test_helpers.hh"
#include "onyxui/widgets/button.hh"
#include "onyxui/widgets/containers/panel.hh"
#include "onyxui/services/ui_services.hh"
#include "widgets.hh"
using namespace onyxui;




TEST_CASE("Button - Clickable widget") {
    SUBCASE("Construction with text") {
        button<test_backend> const btn("Click Me");

        CHECK(btn.text() == "Click Me");
        CHECK(btn.is_focusable());  // Buttons are focusable
    }

    SUBCASE("Button click") {
        test_button<test_backend> btn("Test");
        int click_count = 0;

        btn.clicked.connect([&]() { click_count++; });

        // Simulate click
        btn.simulate_click();

        CHECK(click_count == 1);
    }

    SUBCASE("Set button text") {
        button<test_backend> btn;

        btn.set_text("Save");
        CHECK(btn.text() == "Save");

        btn.set_text("Cancel");
        CHECK(btn.text() == "Cancel");
    }

    // Rule of Five tests - using generic framework
    onyxui::testing::test_rule_of_five_text_widget<button<test_backend>>("Modified");
}

TEST_CASE("Button - Keyboard activation") {
    using traits = event_traits<test_backend::test_keyboard_event>;

    // Helper to simulate keyboard event (key down + key up)
    auto simulate_key = [](auto& widget, key_code key) -> bool {
        // Send key down
        keyboard_event down{};
        down.key = key;
        down.modifiers = key_modifier::none;
        down.pressed = true;
        widget.handle_event(ui_event{down}, event_phase::target);

        // Send key up (this triggers the click)
        keyboard_event up{};
        up.key = key;
        up.modifiers = key_modifier::none;
        up.pressed = false;
        return widget.handle_event(ui_event{up}, event_phase::target);
    };

    // Helper class to access protected focus methods
    class test_button_with_focus : public button<test_backend> {
    public:
        using button<test_backend>::button;

        // Public wrapper to expose protected set_focus for testing
        void give_focus() {
            this->set_focus(true);
        }
    };

    SUBCASE("Enter key triggers click when focused") {
        test_button_with_focus btn("Test");
        int click_count = 0;

        btn.clicked.connect([&]() { click_count++; });

        // Give button focus
        btn.give_focus();
        CHECK(btn.has_focus());

        // Simulate Enter key press
        bool handled = simulate_key(btn, key_code::enter);

        // Verify the click was triggered
        CHECK(handled);
        CHECK(click_count == 1);
    }

    SUBCASE("Space key triggers click when focused") {
        test_button_with_focus btn("Test");
        int click_count = 0;

        btn.clicked.connect([&]() { click_count++; });

        // Give button focus
        btn.give_focus();
        CHECK(btn.has_focus());

        // Simulate Space key press
        bool handled = simulate_key(btn, key_code::space);

        // Verify the click was triggered
        CHECK(handled);
        CHECK(click_count == 1);
    }

    SUBCASE("Keyboard does NOT trigger click when not focused") {
        button<test_backend> btn("Test");
        int click_count = 0;

        btn.clicked.connect([&]() { click_count++; });

        // Button is NOT focused
        CHECK_FALSE(btn.has_focus());

        // Simulate Enter key press
        bool handled = simulate_key(btn, key_code::enter);

        // Should NOT trigger click (not handled when unfocused)
        CHECK_FALSE(handled);
        CHECK(click_count == 0);
    }

    SUBCASE("Tab navigation and Enter activation - Full workflow") {
        // Create UI context with input manager
        ui_context_fixture<test_backend> fixture;

        // Create a container with two buttons
        auto root = std::make_unique<panel<test_backend>>();
        root->set_vbox_layout(spacing::tiny);

        auto* btn1 = root->template emplace_child<button>(std::string("Button 1"));
        auto* btn2 = root->template emplace_child<button>(std::string("Button 2"));

        int btn1_clicks = 0;
        int btn2_clicks = 0;

        btn1->clicked.connect([&]() { btn1_clicks++; });
        btn2->clicked.connect([&]() { btn2_clicks++; });

        // Layout the UI
        [[maybe_unused]] auto measured_size = root->measure(80, 25);
        typename test_backend::rect r{0, 0, 80, 25};
        root->arrange(geometry::relative_rect<test_backend>{r});

        // Get input manager
        auto* input = ui_services<test_backend>::input();
        REQUIRE(input != nullptr);

        // Initially no button has focus
        CHECK_FALSE(btn1->has_focus());
        CHECK_FALSE(btn2->has_focus());

        // Simulate Tab key to focus first button
        test_backend::test_keyboard_event tab_event;
        tab_event.pressed = true;
        tab_event.key_code = traits::KEY_TAB;
        tab_event.shift = false;

        bool tab1_handled = input->handle_tab_navigation_in_tree(tab_event, root.get());
        CHECK(tab1_handled);
        CHECK(btn1->has_focus());  // First button should be focused
        CHECK_FALSE(btn2->has_focus());

        // Simulate Tab key again to focus second button
        bool tab2_handled = input->handle_tab_navigation_in_tree(tab_event, root.get());
        CHECK(tab2_handled);
        CHECK_FALSE(btn1->has_focus());  // First button lost focus
        CHECK(btn2->has_focus());  // Second button should be focused

        // Simulate Enter key to activate second button
        bool enter_handled = simulate_key(*btn2, key_code::enter);
        CHECK(enter_handled);

        // Verify ONLY the second button was clicked
        CHECK(btn1_clicks == 0);
        CHECK(btn2_clicks == 1);

        // Simulate Space key to activate second button again
        bool space_handled = simulate_key(*btn2, key_code::space);
        CHECK(space_handled);

        // Verify second button was clicked again
        CHECK(btn1_clicks == 0);
        CHECK(btn2_clicks == 2);
    }
}