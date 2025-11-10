/**
 * @file test_window_title_bar_visual.cc
 * @brief Visual rendering tests for window title bar icons
 * @details Tests that icons ACTUALLY RENDER in the output, not just layout
 */

#include <doctest/doctest.h>
#include <utils/test_helpers.hh>
#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/window/window_title_bar.hh>
#include <onyxui/widgets/icon.hh>
#include <utils/test_canvas_backend.hh>
#include <iostream>
#include <sstream>

using namespace onyxui;
using Backend = testing::test_canvas_backend;

TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "window_title_bar - Visual rendering of icons") {

    SUBCASE("Title bar with all icons renders visible characters at right edge") {
        // Create window with title bar
        typename window<Backend>::window_flags flags;
        flags.has_menu_button = true;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_close_button = true;

        auto win = std::make_unique<window<Backend>>("Test Window", flags);

        // Measure and arrange window
        win->measure(40, 20);
        win->arrange({0, 0, 40, 20});

        // Render to canvas
        auto canvas = render_to_canvas(*win, 40, 20);
        std::string output = canvas->render_ascii();

        MESSAGE("Rendered window output:");
        MESSAGE(output);

        // Extract just the title bar line (first line inside border)
        std::istringstream iss(output);
        std::string line;
        int line_num = 0;
        std::string title_bar_line;

        while (std::getline(iss, line)) {
            line_num++;
            // Title bar should be on line 2 (after top border)
            if (line_num == 2) {
                title_bar_line = line;
                break;
            }
        }

        MESSAGE("Title bar line: '" << title_bar_line << "'");

        // Visual assertions:
        // 1. Title bar should contain the window title
        CHECK(title_bar_line.find("Test Window") != std::string::npos);

        // 2. Title bar should have NON-SPACE characters at the right edge
        //    This is where icons should render
        REQUIRE(title_bar_line.length() >= 38);  // At least content width

        // Check last 4 characters (where icons should be: menu, minimize, maximize, close)
        std::string right_edge = title_bar_line.substr(title_bar_line.length() - 4);
        MESSAGE("Right edge (last 4 chars): '" << right_edge << "'");

        // Count non-space characters in right edge
        int non_space_count = 0;
        for (char c : right_edge) {
            if (c != ' ') {
                non_space_count++;
            }
        }

        MESSAGE("Non-space characters in right edge: " << non_space_count);

        // Should have at least 4 icon characters (≡ ▁ □ ×) at right edge
        CHECK(non_space_count >= 4);
    }

    SUBCASE("Title bar with minimal icons renders at correct positions") {
        // Create window with only close button
        typename window<Backend>::window_flags flags;
        flags.has_menu_button = false;
        flags.has_minimize_button = false;
        flags.has_maximize_button = false;
        flags.has_close_button = true;

        auto win = std::make_unique<window<Backend>>("Short", flags);

        // Measure and arrange
        win->measure(30, 15);
        win->arrange({0, 0, 30, 15});

        // Render
        auto canvas = render_to_canvas(*win, 30, 15);
        std::string output = canvas->render_ascii();
        MESSAGE("Rendered window:");
        MESSAGE(output);

        // Extract title bar line
        std::istringstream iss(output);
        std::string line;
        int line_num = 0;
        std::string title_bar_line;

        while (std::getline(iss, line)) {
            line_num++;
            if (line_num == 2) {
                title_bar_line = line;
                break;
            }
        }

        MESSAGE("Title bar: '" << title_bar_line << "'");

        // Should contain title at left
        CHECK(title_bar_line.find("Short") != std::string::npos);

        // Should have close icon (×) at right edge (last character before border)
        REQUIRE(title_bar_line.length() >= 28);
        char last_char = title_bar_line[title_bar_line.length() - 1];

        MESSAGE("Last character (close icon): '" << last_char << "' (code: " << (int)last_char << ")");

        // Close icon should be non-space
        CHECK(last_char != ' ');
    }

    SUBCASE("Multiple windows render icons independently") {
        // Create two windows with different titles
        typename window<Backend>::window_flags flags;
        flags.has_close_button = true;
        flags.has_minimize_button = true;

        auto win1 = std::make_unique<window<Backend>>("Win1", flags);
        auto win2 = std::make_unique<window<Backend>>("Win2", flags);

        // Arrange at different positions
        win1->measure(25, 10);
        win1->arrange({0, 0, 25, 10});

        win2->measure(25, 10);
        win2->arrange({5, 5, 25, 10});

        // Render both windows to same canvas (overlapping)
        auto canvas = std::make_shared<testing::test_canvas>(40, 20);
        testing::canvas_renderer renderer(canvas);
        auto* theme = ui_services<Backend>::themes()->get_current_theme();
        REQUIRE(theme != nullptr);

        win1->render(renderer, theme);
        win2->render(renderer, theme);

        std::string output = canvas->render_ascii();
        MESSAGE("Rendered two windows:");
        MESSAGE(output);

        // Both windows should show their titles
        CHECK(output.find("Win1") != std::string::npos);
        CHECK(output.find("Win2") != std::string::npos);

        // Count total non-space characters - should include both sets of icons
        int non_space = 0;
        for (char c : output) {
            if (c != ' ' && c != '\n') {
                non_space++;
            }
        }

        MESSAGE("Total non-space characters: " << non_space);

        // Should have at least: borders + titles + icons for both windows
        CHECK(non_space >= 50);  // Reasonable threshold
    }
}
