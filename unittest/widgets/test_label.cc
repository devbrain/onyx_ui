//
// Created by igor on 16/10/2025.
//

#include <doctest/doctest.h>

#include <onyxui/widgets/label.hh>
#include "../utils/test_backend.hh"
#include "../utils/rule_of_five_tests.hh"
#include "onyxui/concepts/size_like.hh"
using namespace onyxui;

TEST_CASE("Label - Text display widget") {
    SUBCASE("Construction with text") {
        label<test_backend> const lbl("Hello World");

        CHECK(lbl.text() == "Hello World");
        CHECK_FALSE(lbl.is_focusable());  // Labels aren't focusable
    }

    SUBCASE("Set text") {
        label<test_backend> lbl;

        lbl.set_text("Initial");
        CHECK(lbl.text() == "Initial");

        lbl.set_text("Updated");
        CHECK(lbl.text() == "Updated");
    }

    SUBCASE("Content size based on text") {
        label<test_backend> lbl("Test");

        auto size = lbl.measure(100, 100);
        int const width = size_utils::get_width(size);

        CHECK(width >= 4);  // At least as wide as text length
    }

    // Rule of Five tests - using generic framework
    onyxui::testing::test_rule_of_five_text_widget<label<test_backend>>("Original Text");
}