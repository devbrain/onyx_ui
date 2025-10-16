//
// Created by igor on 16/10/2025.
//
#include <doctest/doctest.h>


#include <onyxui/widgets/label.hh>

#include "../utils/test_backend.hh"
#include "../utils/warnings.hh"
#include "widgets.hh"
using namespace onyxui;




TEST_CASE("Button - Clickable widget") {
    SUBCASE("Construction with text") {
        button<test_backend> btn("Click Me");

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

    SUBCASE("Rule of Five - Copy operations deleted") {
        // Verify copy constructor is deleted
        static_assert(!std::is_copy_constructible_v<button<test_backend>>,
                      "button should not be copy constructible");

        // Verify copy assignment is deleted
        static_assert(!std::is_copy_assignable_v<button<test_backend>>,
                      "button should not be copy assignable");
    }

    SUBCASE("Rule of Five - Move constructor") {
        button<test_backend> btn1("Original");
        btn1.set_text("Modified");

        // Move construct
        button<test_backend> btn2(std::move(btn1));

        // btn2 should have the data
        CHECK(btn2.text() == "Modified");

        // btn1 should be in valid (but unspecified) state
        // We can still call methods on it without crashing
        CHECK_NOTHROW(btn1.set_text("New Text"));
    }

    SUBCASE("Rule of Five - Move assignment") {
        button<test_backend> btn1("Button 1");
        button<test_backend> btn2("Button 2");

        btn1.set_text("Updated 1");

        // Move assign
        btn2 = std::move(btn1);

        // btn2 should have btn1's data
        CHECK(btn2.text() == "Updated 1");

        // btn1 should be in valid state
        CHECK_NOTHROW(btn1.set_text("Reassigned"));
    }

    SUBCASE("Rule of Five - Move semantics with containers") {
        std::vector<button<test_backend>> buttons;

        // Should be able to move into vector
        buttons.push_back(button<test_backend>("Button 1"));
        buttons.push_back(button<test_backend>("Button 2"));
        buttons.emplace_back("Button 3");

        CHECK(buttons.size() == 3);
        CHECK(buttons[0].text() == "Button 1");
        CHECK(buttons[1].text() == "Button 2");
        CHECK(buttons[2].text() == "Button 3");

        // Vector reallocation should work (requires move)
        for (int i = 0; i < 10; i++) {
            buttons.emplace_back("Button " + std::to_string(i + 4));
        }

        CHECK(buttons.size() == 13);
    }

    SUBCASE("Rule of Five - Self-assignment safety") {
        button<test_backend> btn("Test");

        // Self-move-assignment should be safe
SUPPRESS_SELF_MOVE_BEGIN
        btn = std::move(btn);
SUPPRESS_SELF_MOVE_END

        // Should still be valid
        CHECK_NOTHROW(btn.set_text("After self-move"));
    }
}