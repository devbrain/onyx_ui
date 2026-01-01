/**
 * @file test_scrollbar_arrows.cc
 * @brief Tests for scrollbar arrow button interaction (Gap 2.4)
 * @author Claude Code
 * @date 2025-10-29
 */

#include <doctest/doctest.h>
#include <../../include/onyxui/widgets/containers/scroll/scrollbar.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_canvas_backend.hh"

using namespace onyxui;
using namespace onyxui::testing;

// Test wrapper to simulate mouse events for testing
template<UIBackend Backend>
class test_scrollbar : public scrollbar<Backend> {
public:
    using scrollbar<Backend>::scrollbar;

    // Helper for simulating clicks at specific positions
    // Returns true if click was handled
    bool simulate_click_at(int x, int y) {
        // Simulate press
        mouse_event press{.x = logical_unit(static_cast<double>(x)), .y = logical_unit(static_cast<double>(y)), .btn = mouse_event::button::left, .act = mouse_event::action::press, .modifiers = {}};
        this->handle_mouse(press);

        // Simulate release (generates click)
        mouse_event release{.x = logical_unit(static_cast<double>(x)), .y = logical_unit(static_cast<double>(y)), .btn = mouse_event::button::left, .act = mouse_event::action::release, .modifiers = {}};
        return this->handle_mouse(release);
    }
};

// =============================================================================
// Arrow Button Click Tests - Vertical Scrollbar
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Arrow: Decrement arrow emits scroll_requested") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    // Set up scroll info
    scroll_info<test_canvas_backend> info{
        {100, 500},  // content_size
        {100, 100},  // viewport_size
        {0, 0}       // scroll_offset
    };
    sb.set_scroll_info(info);

    // Measure and arrange
    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    // Connect to scroll_requested signal
    int emitted_value = 0;
    bool signal_emitted = false;
    sb.scroll_requested.connect([&](int value) {
        emitted_value = value;
        signal_emitted = true;
    });

    // Click at top (decrement arrow position)
    // For classic style with arrow_size=1: arrow is at top (y=0 to y=1)
    bool handled = sb.simulate_click_at(8, 0);

    SUBCASE("Signal was emitted") {
        CHECK(signal_emitted);
    }

    SUBCASE("Emitted negative value (scroll up)") {
        CHECK(emitted_value < 0);
    }

    SUBCASE("Click was handled") {
        CHECK(handled);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Arrow: Increment arrow emits scroll_requested") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    // Set up scroll info
    scroll_info<test_canvas_backend> info{
        {100, 500},  // content_size
        {100, 100},  // viewport_size
        {0, 0}       // scroll_offset
    };
    sb.set_scroll_info(info);

    // Measure and arrange
    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    // Connect to scroll_requested signal
    int emitted_value = 0;
    bool signal_emitted = false;
    sb.scroll_requested.connect([&](int value) {
        emitted_value = value;
        signal_emitted = true;
    });

    // Click at bottom (increment arrow position)
    // For 1px arrow: arrow is at bottom (y=199 for 200px scrollbar)
    bool handled = sb.simulate_click_at(8, 199);

    SUBCASE("Signal was emitted") {
        CHECK(signal_emitted);
    }

    SUBCASE("Emitted positive value (scroll down)") {
        CHECK(emitted_value > 0);
    }

    SUBCASE("Click was handled") {
        CHECK(handled);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Arrow: Click on track does not emit signal") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    // Set up scroll info
    scroll_info<test_canvas_backend> info{
        {100, 500},  // content_size
        {100, 100},  // viewport_size
        {0, 0}       // scroll_offset
    };
    sb.set_scroll_info(info);

    // Measure and arrange
    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    // Connect to scroll_requested signal
    bool signal_emitted = false;
    sb.scroll_requested.connect([&](int /*value*/) {
        signal_emitted = true;
    });

    // Click in middle of track (not on arrows or thumb)
    bool handled = sb.simulate_click_at(8, 100);

    SUBCASE("Signal was not emitted") {
        CHECK_FALSE(signal_emitted);
    }

    SUBCASE("Click was handled (inside bounds) but no action taken") {
        // With unified event API: click inside bounds returns true (handled)
        // even if no specific action (like arrow scroll) was triggered
        CHECK(handled);
    }
}

// =============================================================================
// Arrow Button Click Tests - Horizontal Scrollbar
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Arrow: Horizontal decrement arrow") {
    test_scrollbar<test_canvas_backend> sb(orientation::horizontal);

    // Set up scroll info
    scroll_info<test_canvas_backend> info{
        {500, 100},  // content_size
        {100, 100},  // viewport_size
        {0, 0}       // scroll_offset
    };
    sb.set_scroll_info(info);

    // Measure and arrange
    [[maybe_unused]] auto size = sb.measure(200_lu, 16_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 200_lu, 16_lu});

    // Connect to scroll_requested signal
    int emitted_value = 0;
    bool signal_emitted = false;
    sb.scroll_requested.connect([&](int value) {
        emitted_value = value;
        signal_emitted = true;
    });

    // Click at left (decrement arrow position)
    // For 1px arrow: arrow is at left (x=0)
    bool handled = sb.simulate_click_at(0, 8);

    CHECK(signal_emitted);
    CHECK(emitted_value < 0);  // Negative = scroll left
    CHECK(handled);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Arrow: Horizontal increment arrow") {
    test_scrollbar<test_canvas_backend> sb(orientation::horizontal);

    // Set up scroll info
    scroll_info<test_canvas_backend> info{
        {500, 100},  // content_size
        {100, 100},  // viewport_size
        {0, 0}       // scroll_offset
    };
    sb.set_scroll_info(info);

    // Measure and arrange
    [[maybe_unused]] auto size = sb.measure(200_lu, 16_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 200_lu, 16_lu});

    // Connect to scroll_requested signal
    int emitted_value = 0;
    bool signal_emitted = false;
    sb.scroll_requested.connect([&](int value) {
        emitted_value = value;
        signal_emitted = true;
    });

    // Click at right (increment arrow position)
    // For 1px arrow: arrow is at right (x=199 for 200px scrollbar)
    bool handled = sb.simulate_click_at(199, 8);

    CHECK(signal_emitted);
    CHECK(emitted_value > 0);  // Positive = scroll right
    CHECK(handled);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Arrow: Click outside bounds does nothing") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    // Set up scroll info
    scroll_info<test_canvas_backend> info{
        {100, 500},  // content_size
        {100, 100},  // viewport_size
        {0, 0}       // scroll_offset
    };
    sb.set_scroll_info(info);

    // Measure and arrange
    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    // Connect to scroll_requested signal
    bool signal_emitted = false;
    sb.scroll_requested.connect([&](int /*value*/) {
        signal_emitted = true;
    });

    // Click outside scrollbar bounds
    bool handled = sb.simulate_click_at(-10, 100);

    CHECK_FALSE(signal_emitted);
    CHECK_FALSE(handled);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Arrow: Multiple clicks emit multiple signals") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    // Set up scroll info
    scroll_info<test_canvas_backend> info{
        {100, 500},  // content_size
        {100, 100},  // viewport_size
        {0, 0}       // scroll_offset
    };
    sb.set_scroll_info(info);

    // Measure and arrange
    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    // Connect to scroll_requested signal
    int emit_count = 0;
    sb.scroll_requested.connect([&](int /*value*/) {
        ++emit_count;
    });

    // Click decrement arrow multiple times
    sb.simulate_click_at(8, 0);
    sb.simulate_click_at(8, 0);
    sb.simulate_click_at(8, 0);

    CHECK(emit_count == 3);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Arrow: Different arrows emit different values") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    // Set up scroll info
    scroll_info<test_canvas_backend> info{
        {100, 500},  // content_size
        {100, 100},  // viewport_size
        {0, 0}       // scroll_offset
    };
    sb.set_scroll_info(info);

    // Measure and arrange
    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    // Connect to scroll_requested signal
    int last_emitted = 0;
    sb.scroll_requested.connect([&](int value) {
        last_emitted = value;
    });

    // Click decrement arrow
    sb.simulate_click_at(8, 0);
    int decrement_value = last_emitted;

    // Click increment arrow
    sb.simulate_click_at(8, 199);
    int increment_value = last_emitted;

    // Values should have opposite signs
    CHECK(decrement_value < 0);
    CHECK(increment_value > 0);
    CHECK(decrement_value == -increment_value);  // Should be equal magnitude
}
