//
// Created by igor on 16/10/2025.
//

#include <doctest/doctest.h>

#include <memory>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <../../include/onyxui/widgets/containers/hbox.hh>
#include <../../include/onyxui/widgets/containers/vbox.hh>
#include <utility>
#include "../utils/test_backend.hh"
#include "../utils/rule_of_five_tests.hh"
#include "../../include/onyxui/layout/layout_strategy.hh"


using namespace onyxui;

TEST_CASE("HBox - Horizontal layout widget") {
    SUBCASE("Construction") {
        hbox<test_backend> const box(spacing::large);

        CHECK_FALSE(box.is_focusable());
    }

    SUBCASE("Add children") {
        hbox<test_backend> box;

        auto btn1 = std::make_unique<button<test_backend>>("OK");
        auto btn2 = std::make_unique<button<test_backend>>("Cancel");

        box.add_child(std::move(btn1));
        box.add_child(std::move(btn2));

        CHECK(box.children().size() == 2);
    }

    SUBCASE("Change spacing") {
        hbox<test_backend> box(spacing::small);

        // Should not crash
        box.set_spacing(spacing::large);
        CHECK_NOTHROW((void)box.measure(100_lu, 100_lu));
    }

    SUBCASE("Child alignment configuration") {
        hbox<test_backend> box(spacing::small, horizontal_alignment::center, vertical_alignment::center);

        CHECK(box.get_spacing() == spacing::small);
        CHECK(box.child_h_align() == horizontal_alignment::center);
        CHECK(box.child_v_align() == vertical_alignment::center);

        // Change alignments
        box.set_child_h_align(horizontal_alignment::right);
        box.set_child_v_align(vertical_alignment::bottom);

        CHECK(box.child_h_align() == horizontal_alignment::right);
        CHECK(box.child_v_align() == vertical_alignment::bottom);
    }


    // Rule of Five tests - using generic framework
    onyxui::testing::test_rule_of_five<hbox<test_backend>>(
        [](auto& box) { box.set_spacing(spacing::xlarge); },
        [](const auto& box) { return box.get_spacing() == spacing::xlarge; }
    );
}

#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui::testing;
using hbox_fixture = ui_context_fixture<test_canvas_backend>;

TEST_CASE_FIXTURE(hbox_fixture,
                  "HBox - fixed sibling stays inside content area when expand child wants more") {
    // Reproduces the warlords scenario_info_dialog bug: hbox of
    //   [vbox(text labels, expand), fixed-width child]
    // arranges the fixed child past the right edge of the available
    // area when the text labels naturally measure wider than the
    // hbox's allocated width.

    hbox<test_canvas_backend> h(spacing::medium);

    auto text_col = std::make_unique<vbox<test_canvas_backend>>(spacing::small);
    text_col->set_width_constraint({size_policy::expand});
    // 250 chars with wrap_mode::word — same shape as the warlords
    // scenario_info_dialog: a wrapped description label naturally
    // wider than the hbox content area minus the fixed-width image.
    auto desc = std::make_unique<label<test_canvas_backend>>(
        std::string(250, 'X'));
    desc->set_wrap_mode(label<test_canvas_backend>::wrap_mode::word);
    text_col->add_child(std::move(desc));

    auto image = std::make_unique<button<test_canvas_backend>>("IMG");
    size_constraint img_w;
    img_w.policy = size_policy::fixed;
    img_w.preferred_size = logical_unit(200.0);
    image->set_width_constraint(img_w);
    auto* image_ptr = image.get();

    h.add_child(std::move(text_col));
    h.add_child(std::move(image));

    h.measure(400_lu, 100_lu);
    h.arrange(logical_rect{0_lu, 0_lu, 400_lu, 100_lu});

    const auto img_bounds = image_ptr->bounds();
    const int img_x      = img_bounds.x.to_int();
    const int img_w_int  = img_bounds.width.to_int();
    const int img_right  = img_x + img_w_int;

    MESSAGE("image bounds x=" << img_x
                              << " w=" << img_w_int
                              << " right=" << img_right);
    CHECK_MESSAGE(img_right <= 400,
                  "fixed image extends past content area's right edge ("
                  << img_right << " > 400)");
    CHECK_MESSAGE(img_w_int > 0,
                  "fixed image got zero width");
}