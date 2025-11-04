/**
 * @file test_scroll_view_keyboard.cc
 * @brief Visual tests for text_view and scroll_view display
 * @author OnyxUI Framework
 * @date 2025-11-03
 *
 * @details
 * This file tests that scroll_view correctly displays content at the top,
 * not just the last child. The fix was to use a panel container with
 * pre-measurement and pre-arrangement before adding to scroll_view.
 *
 * TODO: Add keyboard event tests once text_view properly forwards events
 * to its internal scroll_view.
 */

#include "../utils/test_helpers.hh"
#include <onyxui/widgets/text_view.hh>

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Multi-line content display") {
    // Verify text_view displays multiple lines correctly (not just the last line)
    auto view = std::make_unique<text_view<test_canvas_backend>>();

    // Create multi-line content
    std::string content;
    for (int i = 1; i <= 10; ++i) {
        content += "[LINE " + std::to_string(i) + "]\n";
    }
    view->set_text(content);

    // Render - should show first lines, NOT just the last line
    auto canvas = render_to_canvas(*view, 40, 6);

    // Verify first lines are visible
    bool found_line1 = false;
    bool found_line10 = false;
    for (int row = 0; row < canvas->height(); ++row) {
        std::string line = canvas->get_row(row);
        if (line.find("[LINE 1]") != std::string::npos) found_line1 = true;
        if (line.find("[LINE 10]") != std::string::npos) found_line10 = true;
    }

    CHECK(found_line1);        // Should see first line
    CHECK_FALSE(found_line10);  // Should NOT see last line (it's scrolled off)
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Initial scroll position (diagnose LOG 15 issue)") {
    // Reproduce the exact scenario from widgets_demo
    auto text_view_widget = std::make_unique<text_view<test_canvas_backend>>();

    // Generate demo text content (same as in demo.hh)
    std::string demo_text =
        "Welcome to OnyxUI Text View Demo!\n"
        "\n"
        "This widget demonstrates:\n"
        "  * Multi-line text display\n"
        "  * Automatic scrolling\n"
        "\n";

    // Add simulated log entries (same as demo.hh)
    for (int i = 1; i <= 15; ++i) {
        demo_text += "[LOG " + std::to_string(i) + "] Entry at timestamp " +
                    std::to_string(1000 + i * 100) + " ms\n";
    }

    text_view_widget->set_text(demo_text);

    // Render initial view
    auto canvas = render_to_canvas(*text_view_widget, 40, 8);

    // Check what's visible
    bool found_welcome = false;
    bool found_log1 = false;
    bool found_log15 = false;

    for (int row = 0; row < canvas->height(); ++row) {
        std::string line = canvas->get_row(row);
        if (line.find("Welcome") != std::string::npos) found_welcome = true;
        if (line.find("[LOG 1]") != std::string::npos) found_log1 = true;
        if (line.find("[LOG 15]") != std::string::npos) found_log15 = true;
    }

    // Initial view should show beginning, not end!
    CHECK((found_welcome || found_log1));
    CHECK_FALSE(found_log15);  // Should NOT see LOG 15 initially
}
