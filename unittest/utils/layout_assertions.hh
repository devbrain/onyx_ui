/**
 * @file layout_assertions.hh
 * @brief Custom assertions for layout validation
 * @author Testing Infrastructure Team
 * @date 2025-10-21
 */

#pragma once

#include "test_canvas.hh"
#include <onyxui/element.hh>
#include <onyxui/concepts/rect_like.hh>
#include <doctest/doctest.h>
#include <string>

namespace onyxui::testing {

    /**
     * @brief Assert child is positioned inside parent with expected inset
     */
    template<typename Backend>
    void assert_child_inset(
        const ui_element<Backend>& parent,
        const ui_element<Backend>& child,
        int expected_inset_x,
        int expected_inset_y,
        const std::string& message = "")
    {
        auto parent_bounds = parent.bounds();
        auto child_bounds = child.bounds();

        int actual_inset_x = rect_utils::get_x(child_bounds) - rect_utils::get_x(parent_bounds);
        int actual_inset_y = rect_utils::get_y(child_bounds) - rect_utils::get_y(parent_bounds);

        INFO("Parent bounds: (", rect_utils::get_x(parent_bounds), ",", rect_utils::get_y(parent_bounds),
             ") ", rect_utils::get_width(parent_bounds), "x", rect_utils::get_height(parent_bounds));
        INFO("Child bounds: (", rect_utils::get_x(child_bounds), ",", rect_utils::get_y(child_bounds),
             ") ", rect_utils::get_width(child_bounds), "x", rect_utils::get_height(child_bounds));
        if (!message.empty()) {
            INFO(message);
        }

        CHECK(actual_inset_x == expected_inset_x);
        CHECK(actual_inset_y == expected_inset_y);
    }

    /**
     * @brief Assert canvas has border at expected location
     */
    inline void assert_border_at_rect(
        const test_canvas& canvas,
        int x, int y, int w, int h,
        const std::string& message = "")
    {
        INFO("Checking border at rect: (", x, ",", y, ") ", w, "x", h);
        if (!message.empty()) {
            INFO(message);
        }
        INFO("Canvas:\n", canvas.render_ascii());

        CHECK(canvas.has_complete_border(x, y, w, h));
    }

    /**
     * @brief Assert text appears at expected position
     */
    inline void assert_text_at(
        const test_canvas& canvas,
        const std::string& text,
        int expected_x,
        int expected_y,
        const std::string& message = "")
    {
        auto pos = canvas.find_text(text);

        INFO("Looking for text: '", text, "'");
        INFO("Expected position: (", expected_x, ",", expected_y, ")");
        if (!message.empty()) {
            INFO(message);
        }
        INFO("Canvas:\n", canvas.render_ascii());

        REQUIRE(pos.has_value());
        CHECK(pos->first == expected_x);
        CHECK(pos->second == expected_y);
    }

} // namespace onyxui::testing
