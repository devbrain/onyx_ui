//
// Created by igor on 16/10/2025.
//

#include <doctest/doctest.h>

#include <memory>
#include <onyxui/widgets/button.hh>
#include <../../include/onyxui/widgets/containers/hbox.hh>
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
        CHECK_NOTHROW((void)box.measure(100, 100));
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