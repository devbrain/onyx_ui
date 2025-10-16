//
// Created by igor on 16/10/2025.
//

#include <doctest/doctest.h>

#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/hbox.hh>
#include "../utils/test_backend.hh"
#include "../utils/warnings.hh"


using namespace onyxui;

TEST_CASE("HBox - Horizontal layout widget") {
    SUBCASE("Construction") {
        hbox<test_backend> box(10);  // 10px spacing

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

    SUBCASE("Rule of Five - Copy operations deleted") {
        static_assert(!std::is_copy_constructible_v<hbox<test_backend>>,
                      "hbox should not be copy constructible");
        static_assert(!std::is_copy_assignable_v<hbox<test_backend>>,
                      "hbox should not be copy assignable");
    }

    SUBCASE("Rule of Five - Move constructor") {
        hbox<test_backend> box1(15);
        box1.set_spacing(20);

        // Move construct
        hbox<test_backend> box2(std::move(box1));

        CHECK(box2.spacing() == 20);
        CHECK_NOTHROW(box1.set_spacing(5));
    }

    SUBCASE("Rule of Five - Move assignment") {
        hbox<test_backend> box1(10);
        hbox<test_backend> box2(20);

        // Move assign
        box2 = std::move(box1);

        CHECK(box2.spacing() == 10);
        CHECK_NOTHROW(box1.set_spacing(30));
    }

    SUBCASE("Rule of Five - Move semantics with containers") {
        std::vector<hbox<test_backend>> boxes;

        boxes.emplace_back(5);
        boxes.emplace_back(10);

        CHECK(boxes.size() == 2);
        CHECK(boxes[0].spacing() == 5);
        CHECK(boxes[1].spacing() == 10);
    }

    SUBCASE("Rule of Five - Self-assignment safety") {
        hbox<test_backend> box(10);
SUPPRESS_SELF_MOVE_BEGIN
        box = std::move(box);
SUPPRESS_SELF_MOVE_END
        CHECK_NOTHROW(box.set_spacing(20));
    }
}