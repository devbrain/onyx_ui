//
// Created by igor on 16/10/2025.
//
#include <doctest/doctest.h>



#include "../utils/test_backend.hh"
#include "../utils/rule_of_five_tests.hh"
#include "onyxui/widgets/button.hh"
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