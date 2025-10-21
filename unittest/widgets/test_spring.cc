//
// Created by igor on 16/10/2025.
//
#include <doctest/doctest.h>

#include <onyxui/widgets/spring.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/hbox.hh>
#include <onyxui/widgets/vbox.hh>
#include "../utils/test_backend.hh"
#include "../utils/warnings.hh"
#include "../utils/rule_of_five_tests.hh"

using namespace onyxui;

TEST_CASE("Spring - Flexible expanding spacing widget") {
    SUBCASE("Construction with default weight") {
        spring<test_backend> s;

        CHECK(s.weight() == 1.0f);
        CHECK(s.is_horizontal());
        CHECK_FALSE(s.is_focusable());
    }

    SUBCASE("Construction with custom weight") {
        spring<test_backend> s(2.5f);

        CHECK(s.weight() == 2.5f);
    }

    SUBCASE("Construction with vertical orientation") {
        spring<test_backend> s(1.0f, false);

        CHECK_FALSE(s.is_horizontal());
    }

    SUBCASE("Set weight") {
        spring<test_backend> s;

        s.set_weight(3.0f);
        CHECK(s.weight() == 3.0f);
    }

    SUBCASE("Set min/max size") {
        spring<test_backend> s;

        s.set_min_size(10);
        s.set_max_size(100);

        CHECK(s.min_size() == 10);
        CHECK(s.max_size() == 100);
    }

    SUBCASE("Spring with min size constraint") {
        spring<test_backend> s;
        s.set_min_size(50);

        auto size = s.measure(100, 100);
        int width = size_utils::get_width(size);

        // Should respect min size
        CHECK(width >= 50);
    }

    SUBCASE("Push buttons to edges in toolbar") {
        hbox<test_backend> toolbar;

        auto left_btn = std::make_unique<button<test_backend>>("File");
        auto expanding_space = std::make_unique<spring<test_backend>>();
        auto right_btn = std::make_unique<button<test_backend>>("Help");

        toolbar.add_child(std::move(left_btn));
        toolbar.add_child(std::move(expanding_space));
        toolbar.add_child(std::move(right_btn));

        CHECK(toolbar.children().size() == 3);
        CHECK_NOTHROW((void)toolbar.measure(400, 50));
    }

    SUBCASE("Center content with springs on both sides") {
        hbox<test_backend> centered;

        auto left_spring = std::make_unique<spring<test_backend>>();
        auto content = std::make_unique<button<test_backend>>("Centered");
        auto right_spring = std::make_unique<spring<test_backend>>();

        centered.add_child(std::move(left_spring));
        centered.add_child(std::move(content));
        centered.add_child(std::move(right_spring));

        CHECK(centered.children().size() == 3);
        CHECK_NOTHROW((void)centered.measure(400, 50));
    }

    SUBCASE("Weighted springs (proportional distribution)") {
        vbox<test_backend> layout;

        auto header = std::make_unique<button<test_backend>>("Header");
        auto spring1 = std::make_unique<spring<test_backend>>(3.0f, false); // 3x weight vertical
        auto content = std::make_unique<button<test_backend>>("Content");
        auto spring2 = std::make_unique<spring<test_backend>>(1.0f, false); // 1x weight vertical
        auto footer = std::make_unique<button<test_backend>>("Footer");

        layout.add_child(std::move(header));
        layout.add_child(std::move(spring1));
        layout.add_child(std::move(content));
        layout.add_child(std::move(spring2));
        layout.add_child(std::move(footer));

        CHECK(layout.children().size() == 5);
        CHECK_NOTHROW((void)layout.measure(300, 400));
    }

    SUBCASE("Multiple springs with different weights") {
        hbox<test_backend> box;

        box.add_child(std::make_unique<spring<test_backend>>(2.0f));
        box.add_child(std::make_unique<button<test_backend>>("Button"));
        box.add_child(std::make_unique<spring<test_backend>>(1.0f));

        CHECK(box.children().size() == 3);
        CHECK_NOTHROW((void)box.measure(400, 50));
    }

    SUBCASE("Spring is not focusable") {
        spring<test_backend> s;

        CHECK_FALSE(s.is_focusable());
    }

    // Rule of Five tests - using generic framework
    onyxui::testing::test_rule_of_five<spring<test_backend>>(
        [](auto& s) { s.set_weight(2.5f); s.set_min_size(20); },
        [](const auto& s) { return s.weight() == 2.5f && s.min_size() == 20; }
    );
}