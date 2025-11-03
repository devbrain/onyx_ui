/**
 * @file test_scroll_view_keyboard.cc
 * @brief Visual tests for scroll_view keyboard event handling
 * @author OnyxUI Framework
 * @date 2025-11-03
 */

#include "../utils/test_helpers.hh"
#include <onyxui/widgets/containers/scroll_view.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/text_view.hh>
#include <onyxui/events/ui_event.hh>
#include <onyxui/hotkeys/key_code.hh>

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - Keyboard scrolling with visual verification") {
    // Create scroll_view with multiple lines of content
    auto view = std::make_unique<scroll_view<test_canvas_backend>>();

    // Set vertical layout (like working integration test)
    view->set_layout_strategy(std::make_unique<linear_layout<test_canvas_backend>>(direction::vertical));

    // Add 10 labeled lines using emplace_child (like working tests)
    for (int i = 1; i <= 10; ++i) {
        view->emplace_child<label>("[LINE " + std::to_string(i) + "]");
    }

    // Initial render - should show first lines
    auto canvas = render_to_canvas(*view, 40, 6);

    MESSAGE("=== Initial View (should show LINE 1) ===");
    for (int row = 0; row < canvas->height(); ++row) {
        MESSAGE("Row " << row << ": " << canvas->get_row(row));
    }

    // Verify LINE 1 is visible initially
    bool found_line1 = false;
    for (int row = 0; row < canvas->height(); ++row) {
        std::string line = canvas->get_row(row);
        if (line.find("[LINE 1]") != std::string::npos) {
            found_line1 = true;
            break;
        }
    }
    CHECK(found_line1);

    // Simulate Arrow Down key press (scroll down by 1 line)
    keyboard_event key_down{};
    key_down.key = key_code::arrow_down;
    key_down.pressed = true;

    bool handled = view->handle_event(key_down);
    CHECK(handled);  // Should handle arrow down

    // Re-render after scroll
    canvas = render_to_canvas(*view, 40, 6);

    MESSAGE("=== After Arrow Down (scrolled down 1 line) ===");
    for (int row = 0; row < canvas->height(); ++row) {
        MESSAGE("Row " << row << ": " << canvas->get_row(row));
    }

    // Simulate multiple Arrow Down presses to scroll further
    for (int i = 0; i < 5; ++i) {
        view->handle_event(key_down);
    }

    // Re-render after multiple scrolls
    canvas = render_to_canvas(*view, 40, 6);

    MESSAGE("=== After 5 more Arrow Downs (should show later lines) ===");
    for (int row = 0; row < canvas->height(); ++row) {
        MESSAGE("Row " << row << ": " << canvas->get_row(row));
    }

    // Verify we've scrolled - later lines should be visible
    bool found_later_line = false;
    for (int row = 0; row < canvas->height(); ++row) {
        std::string line = canvas->get_row(row);
        // Should see LINE 6, 7, 8, etc.
        if (line.find("[LINE 6]") != std::string::npos ||
            line.find("[LINE 7]") != std::string::npos ||
            line.find("[LINE 8]") != std::string::npos) {
            found_later_line = true;
            break;
        }
    }
    CHECK(found_later_line);

    // Simulate Arrow Up key press (scroll up)
    keyboard_event key_up{};
    key_up.key = key_code::arrow_up;
    key_up.pressed = true;

    for (int i = 0; i < 6; ++i) {
        view->handle_event(key_up);
    }

    // Re-render after scrolling up
    canvas = render_to_canvas(*view, 40, 6);

    MESSAGE("=== After 6 Arrow Ups (scrolled back up) ===");
    for (int row = 0; row < canvas->height(); ++row) {
        MESSAGE("Row " << row << ": " << canvas->get_row(row));
    }

    // Should see early lines again
    bool found_early_line = false;
    for (int row = 0; row < canvas->height(); ++row) {
        std::string line = canvas->get_row(row);
        if (line.find("[LINE 1]") != std::string::npos ||
            line.find("[LINE 2]") != std::string::npos) {
            found_early_line = true;
            break;
        }
    }
    CHECK(found_early_line);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - Page Up/Down keyboard navigation") {
    auto view = std::make_unique<scroll_view<test_canvas_backend>>();

    // Add 20 lines using emplace_child
    for (int i = 1; i <= 20; ++i) {
        view->emplace_child<label>("Line " + std::to_string(i));
    }

    // Initial render
    auto canvas = render_to_canvas(*view, 40, 8);
    MESSAGE("=== Initial View ===");
    for (int row = 0; row < canvas->height(); ++row) {
        MESSAGE("Row " << row << ": " << canvas->get_row(row));
    }

    // Simulate Page Down (should scroll by viewport height ~8 lines)
    keyboard_event page_down{};
    page_down.key = key_code::page_down;
    page_down.pressed = true;

    bool handled = view->handle_event(page_down);
    CHECK(handled);

    canvas = render_to_canvas(*view, 40, 8);
    MESSAGE("=== After Page Down ===");
    for (int row = 0; row < canvas->height(); ++row) {
        MESSAGE("Row " << row << ": " << canvas->get_row(row));
    }

    // Verify we scrolled down significantly
    bool found_mid_lines = false;
    for (int row = 0; row < canvas->height(); ++row) {
        std::string line = canvas->get_row(row);
        if (line.find("Line 8") != std::string::npos ||
            line.find("Line 9") != std::string::npos ||
            line.find("Line 10") != std::string::npos) {
            found_mid_lines = true;
            break;
        }
    }
    CHECK(found_mid_lines);

    // Page Up to go back
    keyboard_event page_up{};
    page_up.key = key_code::page_up;
    page_up.pressed = true;

    view->handle_event(page_up);

    canvas = render_to_canvas(*view, 40, 8);
    MESSAGE("=== After Page Up ===");
    for (int row = 0; row < canvas->height(); ++row) {
        MESSAGE("Row " << row << ": " << canvas->get_row(row));
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - Home/End keyboard navigation") {
    auto view = std::make_unique<scroll_view<test_canvas_backend>>();

    // Add 15 lines using emplace_child
    for (int i = 1; i <= 15; ++i) {
        view->emplace_child<label>("[LOG " + std::to_string(i) + "]");
    }

    // Scroll to end first
    keyboard_event end_key{};
    end_key.key = key_code::end;
    end_key.pressed = true;

    bool handled = view->handle_event(end_key);
    CHECK(handled);

    auto canvas = render_to_canvas(*view, 40, 6);
    MESSAGE("=== After End key (should show last lines) ===");
    for (int row = 0; row < canvas->height(); ++row) {
        MESSAGE("Row " << row << ": " << canvas->get_row(row));
    }

    // Verify we see the last lines
    bool found_log15 = false;
    for (int row = 0; row < canvas->height(); ++row) {
        std::string line = canvas->get_row(row);
        if (line.find("[LOG 15]") != std::string::npos) {
            found_log15 = true;
            MESSAGE("Found [LOG 15] at row " << row << ": " << line);
            break;
        }
    }
    CHECK(found_log15);

    // Home to go back to top
    keyboard_event home_key{};
    home_key.key = key_code::home;
    home_key.pressed = true;

    view->handle_event(home_key);

    canvas = render_to_canvas(*view, 40, 6);
    MESSAGE("=== After Home key (should show first lines) ===");
    for (int row = 0; row < canvas->height(); ++row) {
        MESSAGE("Row " << row << ": " << canvas->get_row(row));
    }

    // Verify we see the first lines
    bool found_log1 = false;
    for (int row = 0; row < canvas->height(); ++row) {
        std::string line = canvas->get_row(row);
        if (line.find("[LOG 1]") != std::string::npos) {
            found_log1 = true;
            MESSAGE("Found [LOG 1] at row " << row << ": " << line);
            break;
        }
    }
    CHECK(found_log1);
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

    MESSAGE("=== text_view Initial Render (should show first lines, NOT just LOG 15!) ===");
    for (int row = 0; row < canvas->height(); ++row) {
        MESSAGE("Row " << row << ": " << canvas->get_row(row));
    }

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

    MESSAGE("Found Welcome: " << found_welcome);
    MESSAGE("Found [LOG 1]: " << found_log1);
    MESSAGE("Found [LOG 15]: " << found_log15);

    // Initial view should show beginning, not end!
    CHECK((found_welcome || found_log1));
    CHECK_FALSE(found_log15);  // Should NOT see LOG 15 initially
}
