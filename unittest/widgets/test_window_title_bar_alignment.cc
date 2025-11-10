/**
 * @file test_window_title_bar_alignment.cc
 * @brief Tests for window title bar title alignment (left/center/right)
 */

#include <doctest/doctest.h>
#include <utils/test_helpers.hh>
#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/window/window_title_bar.hh>
#include <onyxui/services/ui_services.hh>
#include <iostream>

using namespace onyxui;
using Backend = testing::test_canvas_backend;

TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "window_title_bar - Title alignment") {

    SUBCASE("Left-aligned title (default)") {
        // Left alignment is the default, so just use current theme
        typename window<Backend>::window_flags flags;
        flags.has_menu_button = true;
        flags.has_close_button = true;

        auto win = std::make_unique<window<Backend>>("Title", flags);
        win->measure(30, 10);
        win->arrange({0, 0, 30, 10});

        // Render
        auto canvas = render_to_canvas(*win, 30, 10);
        std::string output = canvas->render_ascii();

        MESSAGE("Left-aligned output:");
        MESSAGE(output);

        // Extract title bar line
        std::istringstream iss(output);
        std::string line;
        std::getline(iss, line); // Skip top border
        std::getline(iss, line); // Title bar

        MESSAGE("Title bar: '" << line << "'");

        // Title should be near the left (after menu icon)
        // Format: |=Title####################X|
        CHECK(line.find("=Title") != std::string::npos);
    }

    SUBCASE("Center-aligned title") {
        // Create custom theme with center alignment
        auto* theme_registry = ui_services<Backend>::themes();
        REQUIRE(theme_registry != nullptr);

        // Create a copy of current theme and modify it
        auto custom_theme = *theme_registry->get_current_theme();
        custom_theme.name = "TestCenter";
        custom_theme.window.title_alignment = horizontal_alignment::center;

        // Register temporary theme
        theme_registry->register_theme(custom_theme);
        theme_registry->set_current_theme("TestCenter");

        // Create window
        typename window<Backend>::window_flags flags;
        flags.has_menu_button = false;
        flags.has_close_button = true;

        auto win = std::make_unique<window<Backend>>("Title", flags);
        win->measure(30, 10);
        win->arrange({0, 0, 30, 10});

        // Render
        auto canvas = render_to_canvas(*win, 30, 10);
        std::string output = canvas->render_ascii();

        MESSAGE("Center-aligned output:");
        MESSAGE(output);

        // Extract title bar line
        std::istringstream iss(output);
        std::string line;
        std::getline(iss, line); // Skip top border
        std::getline(iss, line); // Title bar

        MESSAGE("Title bar: '" << line << "'");

        // Title should be roughly centered
        // Find "Title" position
        size_t title_pos = line.find("Title");
        REQUIRE(title_pos != std::string::npos);

        // Center should be around position 12-14 (width 28 / 2 = 14, title length 5)
        MESSAGE("Title position: " << title_pos);
        CHECK(title_pos >= 10);  // Not at left edge
        CHECK(title_pos <= 16);  // Roughly centered
    }

    SUBCASE("Right-aligned title") {
        // Create custom theme with right alignment
        auto* theme_registry = ui_services<Backend>::themes();
        REQUIRE(theme_registry != nullptr);

        auto custom_theme = *theme_registry->get_current_theme();
        custom_theme.name = "TestRight";
        custom_theme.window.title_alignment = horizontal_alignment::right;

        theme_registry->register_theme(custom_theme);
        theme_registry->set_current_theme("TestRight");

        // Create window
        typename window<Backend>::window_flags flags;
        flags.has_menu_button = false;
        flags.has_close_button = true;

        auto win = std::make_unique<window<Backend>>("Title", flags);
        win->measure(30, 10);
        win->arrange({0, 0, 30, 10});

        // Render
        auto canvas = render_to_canvas(*win, 30, 10);
        std::string output = canvas->render_ascii();

        MESSAGE("Right-aligned output:");
        MESSAGE(output);

        // Extract title bar line
        std::istringstream iss(output);
        std::string line;
        std::getline(iss, line); // Skip top border
        std::getline(iss, line); // Title bar

        MESSAGE("Title bar: '" << line << "'");

        // Title should be near the right (before control icons)
        // Format: |####################Title_OX| (Title followed by minimize, maximize, close)
        size_t title_pos = line.find("Title");
        REQUIRE(title_pos != std::string::npos);
        MESSAGE("Title position: " << title_pos);
        CHECK(title_pos >= 18);  // Should be far right (not left or center)
    }

    SUBCASE("Alignment with long title") {
        auto* theme_registry = ui_services<Backend>::themes();

        // Test left alignment (default)
        auto win1 = std::make_unique<window<Backend>>("Very Long Window Title Text",
            typename window<Backend>::window_flags{});
        win1->measure(40, 10);
        win1->arrange({0, 0, 40, 10});
        auto canvas1 = render_to_canvas(*win1, 40, 10);
        MESSAGE("Long title, left-aligned:");
        MESSAGE(canvas1->render_ascii());

        // Test center alignment
        auto custom_theme = *theme_registry->get_current_theme();
        custom_theme.name = "TestLongCenter";
        custom_theme.window.title_alignment = horizontal_alignment::center;
        theme_registry->register_theme(custom_theme);
        theme_registry->set_current_theme("TestLongCenter");

        auto win2 = std::make_unique<window<Backend>>("Very Long Window Title Text",
            typename window<Backend>::window_flags{});
        win2->measure(40, 10);
        win2->arrange({0, 0, 40, 10});
        auto canvas2 = render_to_canvas(*win2, 40, 10);
        MESSAGE("Long title, center-aligned:");
        MESSAGE(canvas2->render_ascii());

        // Just verify they render without errors
        CHECK(canvas1->render_ascii().length() > 0);
        CHECK(canvas2->render_ascii().length() > 0);
    }
}
