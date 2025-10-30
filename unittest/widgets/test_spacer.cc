//
// Created by igor on 16/10/2025.
//

#include <doctest/doctest.h>

#include <memory>
#include <../../include/onyxui/widgets/layout/spacer.hh>
#include <onyxui/widgets/button.hh>
#include <../../include/onyxui/widgets/containers/hbox.hh>
#include <../../include/onyxui/widgets/containers/vbox.hh>
#include <utility>
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/rule_of_five_tests.hh"
#include "../utils/test_helpers.hh"
#include "onyxui/concepts/size_like.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Spacer - Fixed-size spacing widget") {
    SUBCASE("Construction with zero size") {
        spacer<test_canvas_backend> const s;

        CHECK(s.width() == 0);
        CHECK(s.height() == 0);
        CHECK_FALSE(s.is_focusable());
    }

    SUBCASE("Construction with horizontal spacing") {
        spacer<test_canvas_backend> s(20, 0);

        CHECK(s.width() == 20);
        CHECK(s.height() == 0);

        auto size = s.measure(100, 100);
        CHECK(size_utils::get_width(size) == 20);
    }

    SUBCASE("Construction with vertical spacing") {
        spacer<test_canvas_backend> s(0, 30);

        CHECK(s.width() == 0);
        CHECK(s.height() == 30);

        auto size = s.measure(100, 100);
        CHECK(size_utils::get_height(size) == 30);
    }

    SUBCASE("Construction with both dimensions") {
        spacer<test_canvas_backend> s(50, 40);

        CHECK(s.width() == 50);
        CHECK(s.height() == 40);

        auto size = s.measure(200, 200);
        CHECK(size_utils::get_width(size) == 50);
        CHECK(size_utils::get_height(size) == 40);
    }

    SUBCASE("Set width") {
        spacer<test_canvas_backend> s;

        s.set_width(25);
        CHECK(s.width() == 25);

        auto size = s.measure(100, 100);
        CHECK(size_utils::get_width(size) == 25);
    }

    SUBCASE("Set height") {
        spacer<test_canvas_backend> s;

        s.set_height(35);
        CHECK(s.height() == 35);

        auto size = s.measure(100, 100);
        CHECK(size_utils::get_height(size) == 35);
    }

    SUBCASE("Change dimensions invalidates layout") {
        spacer<test_canvas_backend> s(10, 10);

        s.set_width(20);
        CHECK(s.width() == 20);

        s.set_height(30);
        CHECK(s.height() == 30);

        auto size = s.measure(100, 100);
        CHECK(size_utils::get_width(size) == 20);
        CHECK(size_utils::get_height(size) == 30);
    }

    SUBCASE("Spacer in horizontal layout") {
        hbox<test_canvas_backend> box;

        auto btn1 = std::make_unique<button<test_canvas_backend>>("Left");
        auto gap = std::make_unique<spacer<test_canvas_backend>>(20, 0);
        auto btn2 = std::make_unique<button<test_canvas_backend>>("Right");

        box.add_child(std::move(btn1));
        box.add_child(std::move(gap));
        box.add_child(std::move(btn2));

        CHECK(box.children().size() == 3);
        CHECK_NOTHROW((void)box.measure(300, 50));
    }

    SUBCASE("Spacer in vertical layout") {
        vbox<test_canvas_backend> box;

        auto btn1 = std::make_unique<button<test_canvas_backend>>("Top");
        auto gap = std::make_unique<spacer<test_canvas_backend>>(0, 30);
        auto btn2 = std::make_unique<button<test_canvas_backend>>("Bottom");

        box.add_child(std::move(btn1));
        box.add_child(std::move(gap));
        box.add_child(std::move(btn2));

        CHECK(box.children().size() == 3);
        CHECK_NOTHROW((void)box.measure(300, 200));
    }

    SUBCASE("Multiple spacers with different sizes") {
        hbox<test_canvas_backend> box;

        box.add_child(std::make_unique<button<test_canvas_backend>>("A"));
        box.add_child(std::make_unique<spacer<test_canvas_backend>>(10, 0));
        box.add_child(std::make_unique<button<test_canvas_backend>>("B"));
        box.add_child(std::make_unique<spacer<test_canvas_backend>>(20, 0));
        box.add_child(std::make_unique<button<test_canvas_backend>>("C"));

        CHECK(box.children().size() == 5);
        CHECK_NOTHROW((void)box.measure(400, 50));
    }

    SUBCASE("Spacer is not focusable") {
        spacer<test_canvas_backend> const s(10, 10);

        CHECK_FALSE(s.is_focusable());
    }

    // Rule of Five tests - using generic framework for sized widgets
    onyxui::testing::test_rule_of_five_sized_widget<spacer<test_canvas_backend>>(40, 30);
}


