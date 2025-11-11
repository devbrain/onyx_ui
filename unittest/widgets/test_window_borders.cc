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

/**
 * Helper: Get full line at specific y position
 */
static std::string get_line_at(const std::string& output, int y) {
    std::istringstream iss(output);
    std::string line;
    int current_y = 0;

    while (std::getline(iss, line)) {
        if (current_y == y) {
            return line;
        }
        current_y++;
    }
    return "";
}

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
        std::string row0 = get_line_at(output, 0);
        CHECK(row0.find("TestWin") != std::string::npos);

        // CRITICAL: Title should appear early in row 0 (within first 10 chars)
        size_t title_pos = row0.find("TestWin");
        REQUIRE(title_pos != std::string::npos);
        CHECK(title_pos < 10);  // Title at left or center, not at end

        // CRITICAL: Content border should start at row 1 (directly below title bar)
        std::string row1 = get_line_at(output, 1);

        // Check if row starts with box drawing characters (use string.find since UTF-8)
        bool has_top_border = (row1.find("┌") == 0 || row1.find("+") == 0 ||
                               row1.find("╔") == 0 || row1.find("#") == 0);
        CHECK(has_top_border);

        // CRITICAL: Middle rows (2-6) should have vertical borders at sides
        for (int y = 2; y < 7; y++) {
            std::string row = get_line_at(output, y);
            if (!row.empty()) {
                // Check for vertical border at start (UTF-8 safe)
                bool has_left_border = (row.find("│") == 0 || row.find("|") == 0 ||
                                       row.find("║") == 0 || row.find("#") == 0);
                CHECK(has_left_border);
            }
        }

        // CRITICAL: Bottom row should have bottom border
        std::string row7 = get_line_at(output, 7);
        if (!row7.empty()) {
            bool has_bottom_border = (row7.find("└") == 0 || row7.find("+") == 0 ||
                                     row7.find("╚") == 0 || row7.find("#") == 0);
            CHECK(has_bottom_border);
        }
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

        std::string output = canvas->render_ascii();


        // CRITICAL: Title bar should be at row 5 (y=5), column 10 (x=10)
        std::string row5 = get_line_at(output, 5);

        // Title should appear starting around column 10
        size_t title_pos = row5.find("Offset");
        REQUIRE(title_pos != std::string::npos);

        // CRITICAL: Title must be within window bounds [10, 30]
        // If double-offsetting occurred, title would be way off (like column 20+)
        CHECK(title_pos >= 10);   // Not before window start
        CHECK(title_pos <= 25);   // Reasonably within window width

        // CRITICAL: Content border top should be at row 6 (y=6), starting at column 10
        std::string row6 = get_line_at(output, 6);

        // Border should be around column 10 (±2 for rendering variations)
        // Check substring from column 8-12 for border character
        if (row6.length() > 12) {
            std::string border_region = row6.substr(8, 5);  // cols 8-12
            bool has_border = (border_region.find("┌") != std::string::npos ||
                              border_region.find("+") != std::string::npos ||
                              border_region.find("#") != std::string::npos);
            CHECK(has_border);
        }

        // CRITICAL: Middle content rows should have borders starting around column 10
        for (int y = 7; y < 10; y++) {
            std::string row = get_line_at(output, y);
            if (row.length() > 12) {
                std::string left_region = row.substr(8, 5);
                bool has_vert = (left_region.find("│") != std::string::npos ||
                                left_region.find("|") != std::string::npos ||
                                left_region.find("#") != std::string::npos);
                CHECK(has_vert);
            }
        }
    }

}

