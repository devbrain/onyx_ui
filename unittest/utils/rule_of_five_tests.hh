#pragma once
/**
 * @file rule_of_five_tests.hh
 * @brief Generic test framework for C++ Rule of Five compliance
 * @author Code Review Refactoring
 * @date October 21, 2025
 *
 * @details
 * Provides generic test templates to eliminate duplication of Rule of Five
 * testing across widget test files. Instead of 50-70 lines of boilerplate
 * per widget, use a single line:
 *
 * @code
 * test_rule_of_five_text_widget<button<test_backend>>();
 * @endcode
 */

#include <doctest/doctest.h>
#include <vector>
#include <string>
#include <type_traits>
#include "warnings.hh"

namespace onyxui::testing {

/**
 * @brief Generic Rule of Five test generator
 *
 * Tests that a type properly implements move semantics and disables copy operations.
 * Covers all five special member functions:
 * - Copy constructor (deleted)
 * - Copy assignment (deleted)
 * - Move constructor (implemented)
 * - Move assignment (implemented)
 * - Destructor (implicit)
 *
 * @tparam Widget The widget type to test (must be move-constructible/assignable)
 * @tparam SetupFunc Callable: void(Widget&) - Sets widget to a testable state
 * @tparam VerifyFunc Callable: bool(const Widget&) - Verifies widget is in expected state
 *
 * @param setup Function to configure widget to a specific state
 * @param verify Function to check if widget is in expected state after move
 * @param widget_name Descriptive name for error messages (default: "widget")
 *
 * @example Basic usage with lambda functions:
 * @code
 * test_rule_of_five<button<test_backend>>(
 *     [](auto& btn) { btn.set_text("Test"); btn.set_enabled(false); },
 *     [](const auto& btn) { return btn.text() == "Test" && !btn.is_enabled(); }
 * );
 * @endcode
 *
 * @example Using helper for text-based widgets:
 * @code
 * test_rule_of_five_text_widget<label<test_backend>>("Custom Text");
 * @endcode
 */
template<typename Widget, typename SetupFunc, typename VerifyFunc>
void test_rule_of_five(SetupFunc setup, VerifyFunc verify,
                       const std::string& widget_name = "widget") {

    SUBCASE("Rule of Five - Copy operations deleted") {
        static_assert(!std::is_copy_constructible_v<Widget>,
                      "widget should not be copy constructible");
        static_assert(!std::is_copy_assignable_v<Widget>,
                      "widget should not be copy assignable");

        INFO("Widget type: " << widget_name);
        CHECK(true);  // Static assertions passed
    }

    SUBCASE("Rule of Five - Move constructor") {
        Widget w1;
        setup(w1);

        // Move construct
        Widget w2(std::move(w1));

        // Verify moved-to object has expected state
        INFO("Widget type: " << widget_name);
        CHECK(verify(w2));

        // Verify moved-from object is in valid (but unspecified) state
        // Should be safe to assign to or destroy
        CHECK_NOTHROW(setup(w1));
    }

    SUBCASE("Rule of Five - Move assignment") {
        Widget w1, w2;
        setup(w1);

        // Move assign
        w2 = std::move(w1);

        // Verify moved-to object has expected state
        INFO("Widget type: " << widget_name);
        CHECK(verify(w2));

        // Verify moved-from object is valid
        CHECK_NOTHROW(setup(w1));
    }

    SUBCASE("Rule of Five - Move semantics with containers") {
        std::vector<Widget> widgets;

        // Test push_back (move into vector)
        Widget w;
        setup(w);
        widgets.push_back(std::move(w));

        INFO("Widget type: " << widget_name);
        CHECK(widgets.size() == 1);
        CHECK(verify(widgets[0]));

        // Test emplace_back (construct in-place)
        widgets.emplace_back();
        CHECK(widgets.size() == 2);

        // Verify vector operations don't invalidate moved objects
        auto moved_out = std::move(widgets[0]);
        CHECK(verify(moved_out));
    }

    SUBCASE("Rule of Five - Self-assignment safety") {
        Widget w;
        setup(w);

        INFO("Widget type: " << widget_name);
        INFO("Self-move is intentional for this test");

        // Self-move assignment should not crash
        // Behavior is implementation-defined but must be safe
SUPPRESS_SELF_MOVE_BEGIN
        w = std::move(w);
SUPPRESS_SELF_MOVE_END

        // Object should still be valid (even if state is unspecified)
        CHECK_NOTHROW(setup(w));
    }
}

/**
 * @brief Simplified Rule of Five test for widgets with text property
 *
 * Convenience wrapper for widgets that have set_text() and text() methods.
 * Automatically tests move semantics by setting and verifying text content.
 *
 * @tparam Widget Widget type with text property (must have set_text() and text())
 * @param test_text Text to use for testing (default: "Test")
 *
 * @example
 * @code
 * test_rule_of_five_text_widget<button<test_backend>>();
 * test_rule_of_five_text_widget<label<test_backend>>("Custom Label");
 * @endcode
 */
template<typename Widget>
void test_rule_of_five_text_widget(const std::string& test_text = "Test") {
    test_rule_of_five<Widget>(
        [&](Widget& w) {
            w.set_text(test_text);
        },
        [&](const Widget& w) {
            return w.text() == test_text;
        },
        std::string(typeid(Widget).name())
    );
}

/**
 * @brief Rule of Five test for layout containers
 *
 * Tests widgets that primarily contain children (panels, boxes, grids).
 * Verifies child count and layout state after move operations.
 *
 * @tparam Container Container widget type (must have add_child() and children())
 * @tparam ChildFactory Callable that returns std::unique_ptr to child widget
 * @param expected_children Number of children to add
 * @param make_child Factory function to create child widgets
 *
 * @example
 * @code
 * test_rule_of_five_container<vbox<test_backend>>(
 *     3,
 *     []() { return std::make_unique<label<test_backend>>("Child"); }
 * );
 * @endcode
 */
template<typename Container, typename ChildFactory>
void test_rule_of_five_container(int expected_children, ChildFactory make_child) {
    test_rule_of_five<Container>(
        [&](Container& c) {
            for (int i = 0; i < expected_children; ++i) {
                c.add_child(make_child());
            }
        },
        [&](const Container& c) {
            return static_cast<int>(c.children().size()) == expected_children;
        }
    );
}

/**
 * @brief Rule of Five test for sized widgets (spacer, spring)
 *
 * Tests widgets with explicit width/height properties.
 * Verifies dimensions are preserved across move operations.
 *
 * @tparam Widget Widget type with width/height (must have set_width(), set_height(),
 *                width(), and height() methods)
 * @param w Expected width
 * @param h Expected height
 *
 * @example
 * @code
 * test_rule_of_five_sized_widget<spacer<test_backend>>(50, 30);
 * test_rule_of_five_sized_widget<spring<test_backend>>(100, 20);
 * @endcode
 */
template<typename Widget>
void test_rule_of_five_sized_widget(int w, int h) {
    test_rule_of_five<Widget>(
        [&](Widget& widget) {
            widget.set_width(w);
            widget.set_height(h);
        },
        [&](const Widget& widget) {
            return widget.width() == w && widget.height() == h;
        }
    );
}

} // namespace onyxui::testing
