/**
 * @file test_scroll_view_keyboard.cc
 * @brief Tests for scroll_view display and keyboard handling
 * @author OnyxUI Framework
 * @date 2025-11-03
 *
 * @details
 * This file tests:
 * 1. scroll_view correctly displays content at the top (not last child)
 * 2. Direct keyboard event handling via handle_event()
 *
 * Note: Semantic action tests are in test_text_view_keyboard.cc
 */

#include "../utils/test_helpers.hh"
#include <onyxui/widgets/text_view.hh>
#include <onyxui/widgets/containers/scroll_view.hh>
#include <onyxui/widgets/containers/scroll/scroll_view_presets.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/layout/linear_layout.hh>
#include <onyxui/concepts/point_like.hh>

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
    text_view_widget->set_has_border(true);  // Enable border

    // Render initial view (larger size to show more content)
    auto canvas = render_to_canvas(*text_view_widget, 80, 15);

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

// ============================================================================
// Direct keyboard event handling tests for scroll_view
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - Direct keyboard handling") {
    using Backend = test_canvas_backend;

    // Create a scroll_view
    auto sv = modern_scroll_view<Backend>();

    // Add content (layout calculation varies by backend - we focus on event routing)
    for (int i = 1; i <= 10; ++i) {
        sv->emplace_child<label>("Line " + std::to_string(i));
    }

    // Measure and arrange
    (void)sv->measure(40_lu, 10_lu);
    sv->arrange(logical_rect{0_lu, 0_lu, 40_lu, 10_lu});

    auto* scrollable = sv->content();
    REQUIRE(scrollable != nullptr);

    // Helper to send keyboard event through public handle_event API
    auto send_key = [&sv](key_code key, bool pressed = true) -> bool {
        keyboard_event kbd{key, key_modifier::none, pressed};
        return sv->handle_event(kbd, event_phase::target);
    };

    SUBCASE("Arrow keys are handled") {
        // Arrow keys should always return handled=true (scroll_view handles them)
        // Whether actual scrolling occurs depends on content size vs viewport
        CHECK(send_key(key_code::arrow_down));
        CHECK(send_key(key_code::arrow_up));
        CHECK(send_key(key_code::arrow_left));
        CHECK(send_key(key_code::arrow_right));
    }

    SUBCASE("Page keys are handled") {
        CHECK(send_key(key_code::page_down));
        CHECK(send_key(key_code::page_up));
    }

    SUBCASE("Home and End are handled") {
        CHECK(send_key(key_code::home));
        CHECK(send_key(key_code::end));
    }

    SUBCASE("Key release events are not handled") {
        CHECK_FALSE(send_key(key_code::arrow_down, false));
        CHECK_FALSE(send_key(key_code::page_down, false));
        CHECK_FALSE(send_key(key_code::home, false));
    }

    SUBCASE("Unhandled keys fall through to base") {
        // Keys not handled by scroll_view should return false
        CHECK_FALSE(send_key(key_code::escape));
        CHECK_FALSE(send_key(key_code::tab));
    }
}
