//
// Created by igor on 16/10/2025.
//

#include <doctest/doctest.h>

#include <onyxui/widgets/vbox.hh>
#include <onyxui/widgets/label.hh>
#include "../utils/test_backend.hh"
#include "../utils/warnings.hh"
using namespace onyxui;

TEST_CASE("Label - Text display widget") {
    SUBCASE("Construction with text") {
        label<test_backend> lbl("Hello World");

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
        int width = size_utils::get_width(size);

        CHECK(width >= 4);  // At least as wide as text length
    }

    SUBCASE("Rule of Five - Copy operations deleted") {
        static_assert(!std::is_copy_constructible_v<label<test_backend>>,
                      "label should not be copy constructible");
        static_assert(!std::is_copy_assignable_v<label<test_backend>>,
                      "label should not be copy assignable");
    }

    SUBCASE("Rule of Five - Move constructor") {
        label<test_backend> lbl1("Original Text");

        // Move construct
        label<test_backend> lbl2(std::move(lbl1));

        CHECK(lbl2.text() == "Original Text");
        CHECK_NOTHROW(lbl1.set_text("New Text"));
    }

    SUBCASE("Rule of Five - Move assignment") {
        label<test_backend> lbl1("Label 1");
        label<test_backend> lbl2("Label 2");

        // Move assign
        lbl2 = std::move(lbl1);

        CHECK(lbl2.text() == "Label 1");
        CHECK_NOTHROW(lbl1.set_text("Reassigned"));
    }

    SUBCASE("Rule of Five - Move semantics with containers") {
        std::vector<label<test_backend>> labels;

        labels.push_back(label<test_backend>("Label 1"));
        labels.push_back(label<test_backend>("Label 2"));
        labels.emplace_back("Label 3");

        CHECK(labels.size() == 3);
        CHECK(labels[0].text() == "Label 1");
        CHECK(labels[1].text() == "Label 2");
        CHECK(labels[2].text() == "Label 3");
    }

    SUBCASE("Rule of Five - Self-assignment safety") {
        label<test_backend> lbl("Test");
SUPPRESS_SELF_MOVE_BEGIN
        lbl = std::move(lbl);
SUPPRESS_SELF_MOVE_END
        CHECK_NOTHROW(lbl.set_text("After self-move"));
    }
}