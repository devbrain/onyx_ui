/**
 * @file test_window_content_layout.cc
 * @brief Tests for window content layout bug
 * @author OnyxUI Team
 * @date 2025-11-17
 *
 * @details
 * Tests to verify that window content is properly visible and laid out
 * when set_content() is called before set_size().
 */

#include <doctest/doctest.h>
#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/button.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_canvas_backend.hh"

using namespace onyxui;
using namespace onyxui::testing;
using Backend = test_canvas_backend;

TEST_SUITE("window - content layout") {

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Content is visible after set_content then set_size") {
        // This test reproduces the bug where content isn't visible until maximize

        // Create window
        typename window<Backend>::window_flags flags;
        flags.has_close_button = true;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_menu_button = true;
        flags.is_resizable = false;
        flags.is_movable = true;

        auto win = std::make_unique<window<Backend>>("Test Window", flags);

        // Create content with multiple children (like in the demo)
        auto content = std::make_unique<vbox<Backend>>(spacing::tiny);

        auto* label1 = content->template emplace_child<label>("First Label");
        auto* label2 = content->template emplace_child<label>("Second Label");
        auto* button1 = content->template emplace_child<button>("Button 1");

        // Set content FIRST (like in the working demo)
        win->set_content(std::move(content));

        // Set size AFTER content (like the fixed demo)
        win->set_size(45, 18);
        win->set_position(8, 4);

        // Now check that content is properly measured and arranged
        auto* content_widget = win->get_content();
        REQUIRE(content_widget != nullptr);

        // Content should have non-zero bounds
        auto content_bounds = content_widget->bounds();
        INFO("Content bounds: (" << content_bounds.x.to_int() << ", " << content_bounds.y.to_int()
             << ", " << content_bounds.width.to_int() << ", " << content_bounds.height.to_int() << ")");

        CHECK(content_bounds.width.to_int() > 0);
        CHECK(content_bounds.height.to_int() > 0);

        // Children should have non-zero bounds
        auto label1_bounds = label1->bounds();
        INFO("Label1 bounds: (" << label1_bounds.x.to_int() << ", " << label1_bounds.y.to_int()
             << ", " << label1_bounds.width.to_int() << ", " << label1_bounds.height.to_int() << ")");

        CHECK(label1_bounds.width.to_int() > 0);
        CHECK(label1_bounds.height.to_int() > 0);

        auto label2_bounds = label2->bounds();
        CHECK(label2_bounds.width.to_int() > 0);
        CHECK(label2_bounds.height.to_int() > 0);

        auto button1_bounds = button1->bounds();
        CHECK(button1_bounds.width.to_int() > 0);
        CHECK(button1_bounds.height.to_int() > 0);

        // Children should be positioned sequentially (vbox layout)
        // label2.y should be greater than label1.y (below it)
        CHECK(label2_bounds.y.to_int() > label1_bounds.y.to_int());

        // button1.y should be greater than label2.y (below it)
        CHECK(button1_bounds.y.to_int() > label2_bounds.y.to_int());
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Content WITHOUT stretch alignment should still be visible") {
        // This test specifically checks that content is visible even without
        // manually setting stretch alignment (the hack we added)

        typename window<Backend>::window_flags flags;
        flags.has_close_button = true;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_menu_button = true;

        auto win = std::make_unique<window<Backend>>("Test Window", flags);

        // Create content WITHOUT setting stretch alignment
        auto content = std::make_unique<vbox<Backend>>(spacing::tiny);
        // DO NOT call: content->set_horizontal_align(horizontal_alignment::stretch);
        // DO NOT call: content->set_vertical_align(vertical_alignment::stretch);

        content->template emplace_child<label>("Test Label");
        auto* btn = content->template emplace_child<button>("Test Button");

        win->set_content(std::move(content));
        win->set_size(40, 15);

        // Button should have non-zero bounds
        auto btn_bounds = btn->bounds();
        INFO("Button bounds: (" << btn_bounds.x.to_int() << ", " << btn_bounds.y.to_int()
             << ", " << btn_bounds.width.to_int() << ", " << btn_bounds.height.to_int() << ")");

        CHECK(btn_bounds.width.to_int() > 0);
        CHECK(btn_bounds.height.to_int() > 0);
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Reverse order: set_size BEFORE set_content (the bug case)") {
        // This test demonstrates the ORIGINAL BUG where set_size() is called
        // BEFORE set_content(), causing content to not be visible

        typename window<Backend>::window_flags flags;
        flags.has_close_button = true;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;

        auto win = std::make_unique<window<Backend>>("Test Window", flags);

        // BUG: Set size BEFORE content (like the broken demo did)
        win->set_size(45, 18);
        win->set_position(8, 4);

        // Create content AFTER sizing
        auto content = std::make_unique<vbox<Backend>>(spacing::tiny);
        auto* label1 = content->template emplace_child<label>("First Label");
        auto* button1 = content->template emplace_child<button>("Button 1");

        win->set_content(std::move(content));

        // Even though we set content AFTER sizing, it should still work!
        // The set_content() should trigger re-measure/re-arrange

        auto* content_widget = win->get_content();
        REQUIRE(content_widget != nullptr);

        auto content_bounds = content_widget->bounds();
        INFO("Content bounds: (" << content_bounds.x.to_int() << ", " << content_bounds.y.to_int()
             << ", " << content_bounds.width.to_int() << ", " << content_bounds.height.to_int() << ")");

        CHECK(content_bounds.width.to_int() > 0);
        CHECK(content_bounds.height.to_int() > 0);

        auto label1_bounds = label1->bounds();
        CHECK(label1_bounds.width.to_int() > 0);
        CHECK(label1_bounds.height.to_int() > 0);

        auto button1_bounds = button1->bounds();
        CHECK(button1_bounds.width.to_int() > 0);
        CHECK(button1_bounds.height.to_int() > 0);
    }
}
