//
// Created by igor on 16/10/2025.
//
#include <doctest/doctest.h>

#include <memory>
#include <onyxui/widgets/spring.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/hbox.hh>
#include <onyxui/widgets/vbox.hh>
#include <utility>
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/rule_of_five_tests.hh"
#include "../utils/test_helpers.hh"
#include "onyxui/concepts/size_like.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Spring - Flexible expanding spacing widget") {
    SUBCASE("Construction with default weight") {
        spring<test_canvas_backend> const s;

        CHECK(s.weight() == 1.0F);
        CHECK(s.is_horizontal());
        CHECK_FALSE(s.is_focusable());
    }

    SUBCASE("Construction with custom weight") {
        spring<test_canvas_backend> const s(2.5F);

        CHECK(s.weight() == 2.5F);
    }

    SUBCASE("Construction with vertical orientation") {
        spring<test_canvas_backend> const s(1.0F, false);

        CHECK_FALSE(s.is_horizontal());
    }

    SUBCASE("Set weight") {
        spring<test_canvas_backend> s;

        s.set_weight(3.0F);
        CHECK(s.weight() == 3.0F);
    }

    SUBCASE("Set min/max size") {
        spring<test_canvas_backend> s;

        s.set_min_size(10);
        s.set_max_size(100);

        CHECK(s.min_size() == 10);
        CHECK(s.max_size() == 100);
    }

    SUBCASE("Spring with min size constraint") {
        spring<test_canvas_backend> s;
        s.set_min_size(50);

        auto size = s.measure(100, 100);
        int const width = size_utils::get_width(size);

        // Should respect min size
        CHECK(width >= 50);
    }

    SUBCASE("Push buttons to edges in toolbar") {
        hbox<test_canvas_backend> toolbar;

        auto left_btn = std::make_unique<button<test_canvas_backend>>("File");
        auto expanding_space = std::make_unique<spring<test_canvas_backend>>();
        auto right_btn = std::make_unique<button<test_canvas_backend>>("Help");

        toolbar.add_child(std::move(left_btn));
        toolbar.add_child(std::move(expanding_space));
        toolbar.add_child(std::move(right_btn));

        CHECK(toolbar.children().size() == 3);
        CHECK_NOTHROW((void)toolbar.measure(400, 50));
    }

    SUBCASE("Center content with springs on both sides") {
        hbox<test_canvas_backend> centered;

        auto left_spring = std::make_unique<spring<test_canvas_backend>>();
        auto content = std::make_unique<button<test_canvas_backend>>("Centered");
        auto right_spring = std::make_unique<spring<test_canvas_backend>>();

        centered.add_child(std::move(left_spring));
        centered.add_child(std::move(content));
        centered.add_child(std::move(right_spring));

        CHECK(centered.children().size() == 3);
        CHECK_NOTHROW((void)centered.measure(400, 50));
    }

    SUBCASE("Weighted springs (proportional distribution)") {
        vbox<test_canvas_backend> layout;

        auto header = std::make_unique<button<test_canvas_backend>>("Header");
        auto spring1 = std::make_unique<spring<test_canvas_backend>>(3.0F, false); // 3x weight vertical
        auto content = std::make_unique<button<test_canvas_backend>>("Content");
        auto spring2 = std::make_unique<spring<test_canvas_backend>>(1.0F, false); // 1x weight vertical
        auto footer = std::make_unique<button<test_canvas_backend>>("Footer");

        layout.add_child(std::move(header));
        layout.add_child(std::move(spring1));
        layout.add_child(std::move(content));
        layout.add_child(std::move(spring2));
        layout.add_child(std::move(footer));

        CHECK(layout.children().size() == 5);
        CHECK_NOTHROW((void)layout.measure(300, 400));
    }

    SUBCASE("Multiple springs with different weights") {
        hbox<test_canvas_backend> box;

        box.add_child(std::make_unique<spring<test_canvas_backend>>(2.0F));
        box.add_child(std::make_unique<button<test_canvas_backend>>("Button"));
        box.add_child(std::make_unique<spring<test_canvas_backend>>(1.0F));

        CHECK(box.children().size() == 3);
        CHECK_NOTHROW((void)box.measure(400, 50));
    }

    SUBCASE("Spring is not focusable") {
        spring<test_canvas_backend> const s;

        CHECK_FALSE(s.is_focusable());
    }

    // Rule of Five tests - using generic framework
    onyxui::testing::test_rule_of_five<spring<test_canvas_backend>>(
        [](auto& s) { s.set_weight(2.5F); s.set_min_size(20); },
        [](const auto& s) { return s.weight() == 2.5F && s.min_size() == 20; }
    );
}