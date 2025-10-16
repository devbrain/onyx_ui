//
// Created by igor on 16/10/2025.
//

#include <doctest/doctest.h>

#include <onyxui/widgets/spacer.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/hbox.hh>
#include <onyxui/widgets/vbox.hh>
#include "../utils/test_backend.hh"
#include "../utils/warnings.hh"

using namespace onyxui;

TEST_CASE("Spacer - Fixed-size spacing widget") {
    SUBCASE("Construction with zero size") {
        spacer<test_backend> s;

        CHECK(s.width() == 0);
        CHECK(s.height() == 0);
        CHECK_FALSE(s.is_focusable());
    }

    SUBCASE("Construction with horizontal spacing") {
        spacer<test_backend> s(20, 0);

        CHECK(s.width() == 20);
        CHECK(s.height() == 0);

        auto size = s.measure(100, 100);
        CHECK(size_utils::get_width(size) == 20);
    }

    SUBCASE("Construction with vertical spacing") {
        spacer<test_backend> s(0, 30);

        CHECK(s.width() == 0);
        CHECK(s.height() == 30);

        auto size = s.measure(100, 100);
        CHECK(size_utils::get_height(size) == 30);
    }

    SUBCASE("Construction with both dimensions") {
        spacer<test_backend> s(50, 40);

        CHECK(s.width() == 50);
        CHECK(s.height() == 40);

        auto size = s.measure(200, 200);
        CHECK(size_utils::get_width(size) == 50);
        CHECK(size_utils::get_height(size) == 40);
    }

    SUBCASE("Set width") {
        spacer<test_backend> s;

        s.set_width(25);
        CHECK(s.width() == 25);

        auto size = s.measure(100, 100);
        CHECK(size_utils::get_width(size) == 25);
    }

    SUBCASE("Set height") {
        spacer<test_backend> s;

        s.set_height(35);
        CHECK(s.height() == 35);

        auto size = s.measure(100, 100);
        CHECK(size_utils::get_height(size) == 35);
    }

    SUBCASE("Change dimensions invalidates layout") {
        spacer<test_backend> s(10, 10);

        s.set_width(20);
        CHECK(s.width() == 20);

        s.set_height(30);
        CHECK(s.height() == 30);

        auto size = s.measure(100, 100);
        CHECK(size_utils::get_width(size) == 20);
        CHECK(size_utils::get_height(size) == 30);
    }

    SUBCASE("Spacer in horizontal layout") {
        hbox<test_backend> box;

        auto btn1 = std::make_unique<button<test_backend>>("Left");
        auto gap = std::make_unique<spacer<test_backend>>(20, 0);
        auto btn2 = std::make_unique<button<test_backend>>("Right");

        box.add_child(std::move(btn1));
        box.add_child(std::move(gap));
        box.add_child(std::move(btn2));

        CHECK(box.children().size() == 3);
        CHECK_NOTHROW((void)box.measure(300, 50));
    }

    SUBCASE("Spacer in vertical layout") {
        vbox<test_backend> box;

        auto btn1 = std::make_unique<button<test_backend>>("Top");
        auto gap = std::make_unique<spacer<test_backend>>(0, 30);
        auto btn2 = std::make_unique<button<test_backend>>("Bottom");

        box.add_child(std::move(btn1));
        box.add_child(std::move(gap));
        box.add_child(std::move(btn2));

        CHECK(box.children().size() == 3);
        CHECK_NOTHROW((void)box.measure(300, 200));
    }

    SUBCASE("Multiple spacers with different sizes") {
        hbox<test_backend> box;

        box.add_child(std::make_unique<button<test_backend>>("A"));
        box.add_child(std::make_unique<spacer<test_backend>>(10, 0));
        box.add_child(std::make_unique<button<test_backend>>("B"));
        box.add_child(std::make_unique<spacer<test_backend>>(20, 0));
        box.add_child(std::make_unique<button<test_backend>>("C"));

        CHECK(box.children().size() == 5);
        CHECK_NOTHROW((void)box.measure(400, 50));
    }

    SUBCASE("Spacer is not focusable") {
        spacer<test_backend> s(10, 10);

        CHECK_FALSE(s.is_focusable());
    }

    SUBCASE("Rule of Five - Copy operations deleted") {
        static_assert(!std::is_copy_constructible_v<spacer<test_backend>>,
                      "spacer should not be copy constructible");
        static_assert(!std::is_copy_assignable_v<spacer<test_backend>>,
                      "spacer should not be copy assignable");
    }

    SUBCASE("Rule of Five - Move constructor") {
        spacer<test_backend> s1(40, 30);

        spacer<test_backend> s2(std::move(s1));

        CHECK(s2.width() == 40);
        CHECK(s2.height() == 30);
        CHECK_NOTHROW(s1.set_width(10));
    }

    SUBCASE("Rule of Five - Move assignment") {
        spacer<test_backend> s1(25, 15);
        spacer<test_backend> s2(50, 60);

        s2 = std::move(s1);

        CHECK(s2.width() == 25);
        CHECK(s2.height() == 15);
        CHECK_NOTHROW(s1.set_height(20));
    }

    SUBCASE("Rule of Five - Move semantics with containers") {
        std::vector<spacer<test_backend>> spacers;

        spacers.push_back(spacer<test_backend>(10, 10));
        spacers.emplace_back(20, 20);

        CHECK(spacers.size() == 2);
        CHECK(spacers[0].width() == 10);
        CHECK(spacers[1].width() == 20);
    }

    SUBCASE("Rule of Five - Self-assignment safety") {
        spacer<test_backend> s(30, 40);
SUPPRESS_SELF_MOVE_BEGIN
        s = std::move(s);
SUPPRESS_SELF_MOVE_END
        CHECK_NOTHROW(s.set_width(35));
    }
}


