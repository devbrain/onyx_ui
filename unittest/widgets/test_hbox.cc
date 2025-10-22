//
// Created by igor on 16/10/2025.
//

#include <doctest/doctest.h>

#include <memory>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/hbox.hh>
#include <utility>
#include "../utils/test_backend.hh"
#include "../utils/rule_of_five_tests.hh"
#include "onyxui/layout_strategy.hh"


using namespace onyxui;

TEST_CASE("HBox - Horizontal layout widget") {
    SUBCASE("Construction") {
        hbox<test_backend> const box(10);  // 10px spacing

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
        hbox<test_backend> box(5);

        // Should not crash
        box.set_spacing(10);
        CHECK_NOTHROW((void)box.measure(100, 100));
    }

    SUBCASE("Child alignment configuration") {
        hbox<test_backend> box(5, horizontal_alignment::center, vertical_alignment::center);

        CHECK(box.spacing() == 5);
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
        [](auto& box) { box.set_spacing(20); },
        [](const auto& box) { return box.spacing() == 20; }
    );
}