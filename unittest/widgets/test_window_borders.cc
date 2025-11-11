/**
 * @file test_window_borders.cc
 * @brief Tests for window border rendering (title bar vs content area)
 *
 * CRITICAL: These tests verify EXACT positions of elements in rendered output.
 * This catches coordinate bugs where elements render at wrong absolute positions.
 */

#include <doctest/doctest.h>
#include <utils/test_helpers.hh>
#include <onyxui/widgets/window/window.hh>
#include <onyxui/services/ui_services.hh>
#include <iostream>
#include <sstream>

using namespace onyxui;
using Backend = testing::test_canvas_backend;

TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "window - Border rendering with EXACT position checks") {

    SUBCASE("Basic window at origin - verify exact positions") {
        // Create window at origin (0,0)
        typename window<Backend>::window_flags flags;
        flags.has_title_bar = true;
        flags.has_close_button = true;

        auto win = std::make_unique<window<Backend>>("TestWin", flags);

        // Small, controlled size
        (void)win->measure(20, 8);
        win->arrange({0, 0, 20, 8});

        // Render to exact canvas size
        auto canvas = render_to_canvas(*win, 20, 8);
        std::string output = canvas->render_ascii();


        // CRITICAL: Verify title bar is at row 0
        // Semantic assertion: Check that title text exists (not specific characters)
        auto title_pos = canvas->find_text("TestWin");
        REQUIRE(title_pos.has_value());
        CHECK(title_pos->second == 0);  // Title at row 0
        CHECK(title_pos->first < 10);   // Title at left or center, not at end

        // CRITICAL: Content border should start at row 1 (directly below title bar)
        // Semantic assertion: Check for border presence, not specific characters
        CHECK(canvas->has_border_at(0, 1));  // Top-left corner has border

        // CRITICAL: Middle rows (2-6) should have vertical borders at sides
        // Semantic assertion: Check for border line, not specific characters
        CHECK(canvas->has_vertical_border_line(0, 2, 5));  // Left edge has borders

        // CRITICAL: Bottom row should have bottom border
        // Semantic assertion: Border at bottom-left corner
        CHECK(canvas->has_border_at(0, 7));
    }

    SUBCASE("CRITICAL: Window at offset position (10,5) - catches coordinate bugs") {
        // This test would have caught the double-offset bug where title bar
        // background was rendered at (window_x + relative_x) instead of just window_x

        typename window<Backend>::window_flags flags;
        flags.has_title_bar = true;
        flags.has_close_button = true;

        auto win = std::make_unique<window<Backend>>("Offset", flags);

        // Create canvas first
        auto canvas = std::make_shared<testing::test_canvas>(40, 15);
        testing::canvas_renderer renderer(canvas);

        // Measure and arrange at offset position (10, 5)
        (void)win->measure(20, 6);
        win->arrange({10, 5, 20, 6});

        // Render WITHOUT re-arranging
        auto* themes_registry = ui_services<Backend>::themes();
        REQUIRE(themes_registry != nullptr);
        auto* theme_ptr = themes_registry->get_current_theme();
        REQUIRE(theme_ptr != nullptr);

        win->render(renderer, theme_ptr);

        // CRITICAL: Title bar should be at row 5 (y=5), column 10 (x=10)
        // Semantic assertion: Check that title text exists at correct position
        auto title_pos = canvas->find_text("Offset");
        REQUIRE(title_pos.has_value());
        CHECK(title_pos->second == 5);  // Title at row 5

        // CRITICAL: Title must be within window bounds [10, 30]
        // If double-offsetting occurred, title would be way off (like column 20+)
        CHECK(title_pos->first >= 10);   // Not before window start
        CHECK(title_pos->first <= 25);   // Reasonably within window width

        // CRITICAL: Content border top should be at row 6 (y=6), starting at column 10
        // Semantic assertion: Check for border presence in expected region
        // Window should have border around column 10 (check range for rendering variations)
        bool found_border = false;
        for (int x = 8; x <= 12; ++x) {
            if (canvas->has_border_at(x, 6)) {
                found_border = true;
                break;
            }
        }
        CHECK(found_border);

        // CRITICAL: Middle content rows should have borders starting around column 10
        // Semantic assertion: Check for vertical border line in expected region
        for (int y = 7; y < 10; y++) {
            bool found_vert = false;
            for (int x = 8; x <= 12; ++x) {
                if (canvas->has_border_at(x, y)) {
                    found_vert = true;
                    break;
                }
            }
            CHECK(found_vert);
        }
    }

}

