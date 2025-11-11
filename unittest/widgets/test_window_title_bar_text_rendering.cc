/**
 * @file test_window_title_bar_text_rendering.cc
 * @brief Test to verify window title bar actually renders text
 * @author Claude Code
 * @date 2025-11-09
 *
 * @details
 * This test was created to diagnose a critical bug where windows measured to {0, 0}
 * and rendered nothing. The root cause was that window didn't set a layout_strategy
 * to arrange its title_bar and content_area children.
 *
 * Fix: Added vertical linear_layout in window constructor to stack children properly.
 */

#include <doctest/doctest.h>
#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/label.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_canvas_backend.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>,
                  "window_title_bar - Text rendering verification") {

    SUBCASE("Standalone label renders text correctly") {
        label<test_canvas_backend> lbl("Test Text");

        auto size = lbl.measure(80, 25);
        CHECK(size.w > 0);
        lbl.arrange({0, 0, 80, 25});

        auto canvas = render_to_canvas(lbl, 80, 25);
        std::string rendered = canvas->render_ascii();


        // Label should render its text
        CHECK(rendered.find("Test Text") != std::string::npos);
    }

    SUBCASE("window with title bar renders text") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_title_bar = true;
        flags.has_menu_button = false;
        flags.has_minimize_button = false;
        flags.has_maximize_button = false;
        flags.has_close_button = false;

        window<test_canvas_backend> win("My Title", flags);

        // Check visibility

        auto size = win.measure(80, 25);

        win.arrange({0, 0, 80, 25});

        // Check bounds
        auto bounds = win.bounds();

        auto canvas = render_to_canvas(win, 80, 25);
        std::string rendered = canvas->render_ascii();


        // Count non-whitespace characters to see if ANYTHING rendered
        int non_whitespace = 0;
        for (char c : rendered) {
            if (c != ' ' && c != '\n') non_whitespace++;
        }

        // Window should render content
        CHECK(non_whitespace > 0);

        // Window should render title text
        CHECK(rendered.find("My Title") != std::string::npos);
    }
}
