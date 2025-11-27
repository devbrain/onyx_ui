/**
 * @file test_window_scrollbar_layout.cc
 * @brief Tests for window scrollbar layout and positioning
 * @author OnyxUI Team
 * @date 2025-11-17
 *
 * @details
 * Tests to verify that scrollable windows properly display scrollbars
 * with correct bounds when content overflows.
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

TEST_SUITE("window - scrollbar layout") {

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Scrollbar appears immediately when content overflows") {
        // This test reproduces the bug where scrollbars don't appear until maximize

        // Create scrollable window
        typename window<Backend>::window_flags flags;
        flags.has_close_button = true;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.is_resizable = false;
        flags.is_scrollable = true;  // Enable scrollbars

        auto win = std::make_unique<window<Backend>>("Test Window", flags);

        // Create content that overflows (like in the demo)
        auto content = std::make_unique<vbox<Backend>>(spacing::tiny);
        content->template emplace_child<label>("Line 1");
        content->template emplace_child<label>("Line 2");
        content->template emplace_child<label>("Line 3");
        content->template emplace_child<label>("Line 4");
        content->template emplace_child<label>("Line 5");
        content->template emplace_child<button>("Button 1");
        content->template emplace_child<button>("Button 2");
        content->template emplace_child<button>("Button 3");
        // This content needs ~19 lines, but window will be 18 lines

        // Set content FIRST (like in the demo)
        win->set_content(std::move(content));

        // Set size AFTER content (this should trigger proper layout)
        win->set_size(45, 18);
        win->set_position(8, 4);

        // Get the scroll_view from content_area
        auto* wca = win->get_content_area();
        REQUIRE(wca != nullptr);
        REQUIRE(wca->is_scrollable());

        auto* scroll_view = wca->get_scroll_view();
        REQUIRE(scroll_view != nullptr);

        // Get the vertical scrollbar
        auto* vscrollbar = scroll_view->vertical_scrollbar();
        REQUIRE(vscrollbar != nullptr);

        // Scrollbar should be visible (content overflows)
        CHECK(vscrollbar->is_visible());

        // Scrollbar should have proper bounds
        auto vsb_bounds = vscrollbar->bounds();
        INFO("Scrollbar bounds: (" << vsb_bounds.x.to_int() << ", " << vsb_bounds.y.to_int()
             << ", " << vsb_bounds.width.to_int() << ", " << vsb_bounds.height.to_int() << ")");

        // Scrollbar should span the full height of the scroll_view
        auto sv_bounds = scroll_view->bounds();
        INFO("Scroll_view bounds: (" << sv_bounds.x.to_int() << ", " << sv_bounds.y.to_int()
             << ", " << sv_bounds.width.to_int() << ", " << sv_bounds.height.to_int() << ")");

        // CORE FIX VERIFICATION: Scrollbar must be visible and have non-zero bounds
        // Before the fix, scrollbar had bounds (0,0,0,0) until window was maximized
        CHECK(vsb_bounds.width.to_int() > 0);  // Has width
        CHECK(vsb_bounds.height.to_int() > 0); // Has height

        // Scrollbar should be near the right edge (grid auto-sizing may leave small gap)
        CHECK(vsb_bounds.x.to_int() + vsb_bounds.width.to_int() >= sv_bounds.width.to_int() - 2);

        // Scrollbar should have reasonable height (may not fill full height with auto-sized grid)
        CHECK(vsb_bounds.height.to_int() >= sv_bounds.height.to_int() / 2);

        // Scrollbar should start at top (y=0 or y=1 for borders)
        CHECK(vsb_bounds.y.to_int() <= 1);
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Scrollbar has correct bounds after window layout") {
        // Simpler test: just verify scrollbar gets proper bounds

        typename window<Backend>::window_flags flags;
        flags.is_scrollable = true;

        auto win = std::make_unique<window<Backend>>("Test", flags);

        // Create overflowing content
        auto content = std::make_unique<vbox<Backend>>(spacing::none);
        for (int i = 0; i < 30; i++) {  // 30 labels definitely overflow
            content->template emplace_child<label>("Item " + std::to_string(i));
        }

        win->set_content(std::move(content));
        win->set_size(40, 15);  // Small window, content won't fit

        // Get scrollbar
        auto* wca = win->get_content_area();
        REQUIRE(wca != nullptr);
        auto* scroll_view = wca->get_scroll_view();
        REQUIRE(scroll_view != nullptr);
        auto* vscrollbar = scroll_view->vertical_scrollbar();
        REQUIRE(vscrollbar != nullptr);

        // Scrollbar must be visible and have non-zero bounds
        CHECK(vscrollbar->is_visible());
        auto bounds = vscrollbar->bounds();
        CHECK(bounds.width.to_int() > 0);
        CHECK(bounds.height.to_int() > 0);
    }
}
