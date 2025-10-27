//
// Created by igor on 16/10/2025.
//

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"

#include <memory>
#include "../utils/test_helpers.hh"
#include <onyxui/widgets/button.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/widgets/vbox.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/widgets/hbox.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/widgets/label.hh>
#include "../utils/test_helpers.hh"
#include <utility>
#include "../utils/test_helpers.hh"
#include "../utils/test_backend.hh"
#include "../utils/test_helpers.hh"
#include "../utils/rule_of_five_tests.hh"
#include "../utils/test_helpers.hh"
#include "onyxui/concepts/rect_like.hh"
#include "../utils/test_helpers.hh"
#include "onyxui/layout_strategy.hh"
#include "../utils/test_helpers.hh"
#include "../utils/test_canvas_backend.hh"
using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "VBox - Vertical layout widget") {
    SUBCASE("Construction") {
        vbox<test_canvas_backend> const box(10);  // 10px spacing

        CHECK_FALSE(box.is_focusable());
    }

    SUBCASE("Add children") {
        vbox<test_canvas_backend> box;

        auto lbl1 = std::make_unique<label<test_canvas_backend>>("Name:");
        auto lbl2 = std::make_unique<label<test_canvas_backend>>("Email:");

        box.add_child(std::move(lbl1));
        box.add_child(std::move(lbl2));

        CHECK(box.children().size() == 2);
    }

    SUBCASE("Complex nested layout") {
        vbox<test_canvas_backend> main_layout;

        // Add title
        auto title = std::make_unique<label<test_canvas_backend>>("Dialog Title");
        main_layout.add_child(std::move(title));

        // Add button row (hbox inside vbox)
        auto button_row = std::make_unique<hbox<test_canvas_backend>>(5);
        button_row->add_child(std::make_unique<button<test_canvas_backend>>("OK"));
        button_row->add_child(std::make_unique<button<test_canvas_backend>>("Cancel"));
        main_layout.add_child(std::move(button_row));

        CHECK(main_layout.children().size() == 2);

        // Verify measure/arrange don't crash
        CHECK_NOTHROW((void)main_layout.measure(200, 200));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 200, 200);
        CHECK_NOTHROW(main_layout.arrange(bounds));
    }

    SUBCASE("Child alignment configuration") {
        vbox<test_canvas_backend> box(8, horizontal_alignment::center, vertical_alignment::center);

        CHECK(box.spacing() == 8);
        CHECK(box.child_h_align() == horizontal_alignment::center);
        CHECK(box.child_v_align() == vertical_alignment::center);

        // Change alignments
        box.set_child_h_align(horizontal_alignment::left);
        box.set_child_v_align(vertical_alignment::bottom);

        CHECK(box.child_h_align() == horizontal_alignment::left);
        CHECK(box.child_v_align() == vertical_alignment::bottom);
    }

    // Rule of Five tests - using generic framework
    onyxui::testing::test_rule_of_five<vbox<test_canvas_backend>>(
        [](auto& box) { box.set_spacing(25); },
        [](const auto& box) { return box.spacing() == 25; }
    );
}