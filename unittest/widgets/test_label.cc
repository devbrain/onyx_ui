//
// Created by igor on 16/10/2025.
//

#include <doctest/doctest.h>

#include <onyxui/widgets/label.hh>
#include "../utils/test_backend.hh"
#include "../utils/test_helpers.hh"
#include "../utils/rule_of_five_tests.hh"
#include "onyxui/concepts/size_like.hh"
using namespace onyxui;

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "Label - Text display widget") {
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

        auto size = lbl.measure(100_lu, 100_lu);
        int const width = size.width.to_int();

        CHECK(width >= 4);  // At least as wide as text length
    }

    // Rule of Five tests - using generic framework
    onyxui::testing::test_rule_of_five_text_widget<label<test_backend>>("Original Text");
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "Label - word-wrap mode") {
    using lbl_t = label<test_backend>;

    // test_backend's measure_text returns {glyph_count, 1}, so widths
    // line up with character counts and each line is exactly 1 unit
    // tall. That makes the wrap math easy to assert against.

    SUBCASE("default wrap mode is none") {
        lbl_t lbl("alpha beta gamma");
        CHECK(lbl.get_wrap_mode() == lbl_t::wrap_mode::none);

        // Without wrapping the label asks for its full natural width
        // even when given a narrow available width.
        auto size = lbl.measure(10_lu, 100_lu);
        CHECK(size.width.to_int()  == 16);  // "alpha beta gamma".length()
        CHECK(size.height.to_int() == 1);
    }

    SUBCASE("word wrap fits within available width") {
        lbl_t lbl("alpha beta gamma");
        lbl.set_wrap_mode(lbl_t::wrap_mode::word);

        // 10 chars fits "alpha beta" (10 chars) exactly; "gamma" goes
        // on its own line. Two lines × 1 unit each = height 2; widest
        // line = 10.
        auto size = lbl.measure(10_lu, 100_lu);
        CHECK(size.width.to_int()  == 10);
        CHECK(size.height.to_int() == 2);
    }

    SUBCASE("word wrap honours embedded \\n as hard break") {
        lbl_t lbl("alpha\nbeta gamma delta");
        lbl.set_wrap_mode(lbl_t::wrap_mode::word);

        // First paragraph is just "alpha" (5).
        // Second paragraph wraps inside 10 chars: "beta gamma" (10),
        // "delta" (5).  3 lines total, widest 10.
        auto size = lbl.measure(10_lu, 100_lu);
        CHECK(size.width.to_int()  == 10);
        CHECK(size.height.to_int() == 3);
    }

    SUBCASE("a single word wider than the limit stays on its own line") {
        // "supercalifragilistic" is 20 chars; available_width = 5.
        // Wrap policy keeps the word whole rather than mid-breaking.
        lbl_t lbl("supercalifragilistic");
        lbl.set_wrap_mode(lbl_t::wrap_mode::word);

        auto size = lbl.measure(5_lu, 100_lu);
        CHECK(size.width.to_int()  == 20);
        CHECK(size.height.to_int() == 1);
    }

    SUBCASE("changing text invalidates the wrap cache") {
        lbl_t lbl("alpha beta gamma");
        lbl.set_wrap_mode(lbl_t::wrap_mode::word);
        (void)lbl.measure(10_lu, 100_lu);

        lbl.set_text("aaa bbb");  // shorter — fits on one line at width 10
        auto size = lbl.measure(10_lu, 100_lu);
        CHECK(size.width.to_int()  == 7);
        CHECK(size.height.to_int() == 1);
    }
}