//
// Created by igor on 16/10/2025.
//
#include <doctest/doctest.h>

#include <onyxui/widgets/vbox.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/panel.hh>
#include "../utils/test_backend.hh"
#include "../utils/warnings.hh"
using namespace onyxui;

TEST_CASE("Panel - Container widget") {
    SUBCASE("Construction") {
        panel<test_backend> p;

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

    SUBCASE("Rule of Five - Copy operations deleted") {
        static_assert(!std::is_copy_constructible_v<panel<test_backend>>,
                      "panel should not be copy constructible");
        static_assert(!std::is_copy_assignable_v<panel<test_backend>>,
                      "panel should not be copy assignable");
    }

    SUBCASE("Rule of Five - Move constructor") {
        panel<test_backend> p1;
        p1.set_has_border(true);

        // Move construct
        panel<test_backend> p2(std::move(p1));

        CHECK(p2.has_border());
        CHECK_NOTHROW(p1.set_has_border(false));
    }

    SUBCASE("Rule of Five - Move assignment") {
        panel<test_backend> p1;
        panel<test_backend> p2;

        p1.set_has_border(true);

        // Move assign
        p2 = std::move(p1);

        CHECK(p2.has_border());
        CHECK_NOTHROW(p1.set_has_border(false));
    }

    SUBCASE("Rule of Five - Move semantics with containers") {
        std::vector<panel<test_backend>> panels;

        panels.push_back(panel<test_backend>());
        panels.emplace_back();

        CHECK(panels.size() == 2);
    }

    SUBCASE("Rule of Five - Self-assignment safety") {
        panel<test_backend> p;
SUPPRESS_SELF_MOVE_BEGIN
        p = std::move(p);
SUPPRESS_SELF_MOVE_END
        CHECK_NOTHROW(p.set_has_border(true));
    }
}