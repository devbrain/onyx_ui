//
// Created by igor on 16/10/2025.
//
#include <doctest/doctest.h>

#include <memory>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/panel.hh>
#include <utility>
#include "../utils/test_backend.hh"
#include "../utils/rule_of_five_tests.hh"
#include "onyxui/layout/linear_layout.hh"
#include "onyxui/layout_strategy.hh"
using namespace onyxui;

TEST_CASE("Panel - Container widget") {
    SUBCASE("Construction") {
        panel<test_backend> const p;

        CHECK_FALSE(p.has_border());
        CHECK_FALSE(p.is_focusable());  // Panels aren't focusable
    }

    SUBCASE("Border setting") {
        panel<test_backend> p;

        p.set_has_border(true);
        CHECK(p.has_border());

        p.set_has_border(false);
        CHECK_FALSE(p.has_border());
    }

    SUBCASE("Panel with children") {
        panel<test_backend> p;
        p.set_layout_strategy(
            std::make_unique<linear_layout<test_backend>>(direction::vertical, 5));

        auto child1 = std::make_unique<label<test_backend>>("Item 1");
        auto child2 = std::make_unique<label<test_backend>>("Item 2");

        p.add_child(std::move(child1));
        p.add_child(std::move(child2));

        CHECK(p.children().size() == 2);
    }

    // Rule of Five tests - using generic framework
    onyxui::testing::test_rule_of_five<panel<test_backend>>(
        [](auto& p) { p.set_has_border(true); },
        [](const auto& p) { return p.has_border(); }
    );
}