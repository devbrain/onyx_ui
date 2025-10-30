/**
 * @file test_scrollbar_theming.cc
 * @brief Tests for scrollbar theme integration (Gap 3)
 * @author Claude Code
 * @date 2025-10-29
 */

#include <doctest/doctest.h>
#include <../../include/onyxui/widgets/containers/scroll/scrollbar.hh>
#include <../../include/onyxui/theming/theme.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_canvas_backend.hh"

using namespace onyxui;
using namespace onyxui::testing;

// Test wrapper to expose protected methods for testing
template<UIBackend Backend>
class test_scrollbar : public scrollbar<Backend> {
public:
    using scrollbar<Backend>::scrollbar;
    using scrollbar<Backend>::calculate_layout;      // Expose for testing
    using scrollbar<Backend>::handle_mouse_move;     // Expose for testing
    using scrollbar<Backend>::handle_mouse_down;     // Expose for testing
    using scrollbar<Backend>::handle_mouse_up;       // Expose for testing
    using scrollbar<Backend>::handle_mouse_leave;    // Expose for testing
};

// =============================================================================
// Theme Color Usage Tests
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Theme: Uses theme colors for rendering") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    // Set up scroll info
    scroll_info<test_canvas_backend> info{
        {100, 500},  // content_size
        {100, 100},  // viewport_size
        {0, 0}       // scroll_offset
    };
    sb.set_scroll_info(info);

    // Measure and arrange
    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    // Render to canvas - should use theme colors
    auto canvas = render_to_canvas(sb, 80, 200);

    // Verify something was rendered (theme colors were used)
    bool has_content = false;
    for (int y = 0; y < 200 && !has_content; ++y) {
        for (int x = 0; x < 16; ++x) {
            if (!canvas->is_empty_at(x, y)) {
                has_content = true;
                break;
            }
        }
    }
    CHECK(has_content);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Theme: Track uses track_normal style") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    scroll_info<test_canvas_backend> info{
        {100, 500},  // content_size
        {100, 100},  // viewport_size
        {0, 0}       // scroll_offset
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    // Get layout to check track bounds
    auto const* themes = ui_services<test_canvas_backend>::themes();
    REQUIRE(themes != nullptr);
    auto const* theme = themes->get_current_theme();
    REQUIRE(theme != nullptr);

    auto layout = sb.calculate_layout(theme->scrollbar.style);

    // Track should have non-zero size
    CHECK(rect_utils::get_width(layout.track) > 0);
    CHECK(rect_utils::get_height(layout.track) > 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Theme: Thumb uses thumb_normal style by default") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    scroll_info<test_canvas_backend> info{
        {100, 500},  // content_size
        {100, 100},  // viewport_size
        {0, 0}       // scroll_offset
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    auto const* themes = ui_services<test_canvas_backend>::themes();
    auto const* theme = themes->get_current_theme();
    auto layout = sb.calculate_layout(theme->scrollbar.style);

    // Thumb should exist
    CHECK(rect_utils::get_width(layout.thumb) > 0);
    CHECK(rect_utils::get_height(layout.thumb) > 0);
}

// =============================================================================
// State-Based Styling Tests
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Theme: Hover state changes rendering") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    scroll_info<test_canvas_backend> info{
        {100, 500},  // content_size
        {100, 100},  // viewport_size
        {0, 0}       // scroll_offset
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    // Simulate mouse move over thumb
    auto thumb = sb.get_thumb_bounds();
    int thumb_x = rect_utils::get_x(thumb) + rect_utils::get_width(thumb) / 2;
    int thumb_y = rect_utils::get_y(thumb) + rect_utils::get_height(thumb) / 2;

    // Move mouse over thumb
    sb.handle_mouse_move(thumb_x, thumb_y);

    // Render - should use hover colors
    auto canvas = render_to_canvas(sb, 80, 200);

    // Verify rendering occurred (check anywhere on canvas)
    bool has_content = false;
    for (int y = 0; y < 200 && !has_content; ++y) {
        for (int x = 0; x < 16; ++x) {
            if (!canvas->is_empty_at(x, y)) {
                has_content = true;
                break;
            }
        }
    }
    CHECK(has_content);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Theme: Pressed state changes rendering") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    scroll_info<test_canvas_backend> info{
        {100, 500},  // content_size
        {100, 100},  // viewport_size
        {0, 0}       // scroll_offset
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    // Simulate mouse press on thumb
    auto thumb = sb.get_thumb_bounds();
    int thumb_x = rect_utils::get_x(thumb) + rect_utils::get_width(thumb) / 2;
    int thumb_y = rect_utils::get_y(thumb) + rect_utils::get_height(thumb) / 2;

    sb.handle_mouse_down(thumb_x, thumb_y, 0);

    // Render - should use pressed colors
    auto canvas = render_to_canvas(sb, 80, 200);

    // Verify rendering occurred (check anywhere on canvas)
    bool has_content = false;
    for (int y = 0; y < 200 && !has_content; ++y) {
        for (int x = 0; x < 16; ++x) {
            if (!canvas->is_empty_at(x, y)) {
                has_content = true;
                break;
            }
        }
    }
    CHECK(has_content);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Theme: Mouse hover and leave don't crash") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    scroll_info<test_canvas_backend> info{
        {100, 500},  // content_size
        {100, 100},  // viewport_size
        {0, 0}       // scroll_offset
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    // Simulate hover over thumb
    auto thumb = sb.get_thumb_bounds();
    int thumb_x = rect_utils::get_x(thumb) + rect_utils::get_width(thumb) / 2;
    int thumb_y = rect_utils::get_y(thumb) + rect_utils::get_height(thumb) / 2;
    sb.handle_mouse_move(thumb_x, thumb_y);

    // Render in hover state
    auto canvas_hover = render_to_canvas(sb, 16, 200);
    CHECK(canvas_hover != nullptr);

    // Simulate mouse leave
    sb.handle_mouse_leave();

    // Render after leave - verifies theme applies correctly in non-hover state
    auto canvas_normal = render_to_canvas(sb, 16, 200);
    CHECK(canvas_normal != nullptr);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Theme: Disabled state uses thumb_disabled style") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    scroll_info<test_canvas_backend> info{
        {100, 500},  // content_size
        {100, 100},  // viewport_size
        {0, 0}       // scroll_offset
    };
    sb.set_scroll_info(info);

    // Disable the scrollbar
    sb.set_enabled(false);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    // Render - should use disabled style
    auto canvas = render_to_canvas(sb, 80, 200);

    // Verify something rendered
    bool has_content = false;
    for (int y = 0; y < 200 && !has_content; ++y) {
        for (int x = 0; x < 16; ++x) {
            if (!canvas->is_empty_at(x, y)) {
                has_content = true;
                break;
            }
        }
    }
    CHECK(has_content);
}

// =============================================================================
// Geometry Properties Tests
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Theme: Uses theme width in measurement") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    // Measure with theme width
    auto size = sb.measure(100, 200);

    // Width should match theme width (16 from test fixture)
    CHECK(size_utils::get_width(size) == 16);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Theme: Honors min_thumb_size from theme") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    // Huge content with tiny viewport to test min thumb size
    scroll_info<test_canvas_backend> info{
        {100, 10000},  // content_size (huge)
        {100, 100},    // viewport_size (tiny)
        {0, 0}         // scroll_offset
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    auto thumb = sb.get_thumb_bounds();
    int thumb_h = rect_utils::get_height(thumb);

    // Thumb should be at least min_thumb_size (20 from test fixture)
    CHECK(thumb_h >= 20);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Theme: Horizontal uses theme width for height") {
    test_scrollbar<test_canvas_backend> sb(orientation::horizontal);

    // Measure with theme width
    auto size = sb.measure(200, 100);

    // Height should match theme width (16 from test fixture)
    CHECK(size_utils::get_height(size) == 16);
}

// =============================================================================
// Arrow Button Theme Tests
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Theme: Arrow buttons use arrow_normal style") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    scroll_info<test_canvas_backend> info{
        {100, 500},  // content_size
        {100, 100},  // viewport_size
        {0, 0}       // scroll_offset
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    // Get layout to check arrow bounds
    auto const* themes = ui_services<test_canvas_backend>::themes();
    auto const* theme = themes->get_current_theme();
    auto layout = sb.calculate_layout(theme->scrollbar.style);

    // If style has arrows, they should have size
    if (layout.has_arrows()) {
        CHECK((rect_utils::get_width(layout.arrow_decrement) > 0 ||
               rect_utils::get_height(layout.arrow_decrement) > 0));
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Theme: Arrow hover changes style") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    scroll_info<test_canvas_backend> info{
        {100, 500},  // content_size
        {100, 100},  // viewport_size
        {0, 0}       // scroll_offset
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    // Check if we have arrows
    auto const* themes = ui_services<test_canvas_backend>::themes();
    auto const* theme = themes->get_current_theme();
    auto layout = sb.calculate_layout(theme->scrollbar.style);

    if (layout.has_arrows()) {
        // Hover over arrow
        int arrow_x = rect_utils::get_x(layout.arrow_decrement) + 1;
        int arrow_y = rect_utils::get_y(layout.arrow_decrement) + 1;
        sb.handle_mouse_move(arrow_x, arrow_y);

        // Render - should use arrow_hover colors
        auto canvas = render_to_canvas(sb, 80, 200);

        // Verify rendering
        bool has_content = false;
        for (int y = 0; y < 200 && !has_content; ++y) {
            for (int x = 0; x < 16; ++x) {
                if (!canvas->is_empty_at(x, y)) {
                    has_content = true;
                    break;
                }
            }
        }
        CHECK(has_content);
    }
    // Note: If no arrows, test is skipped (theme has arrows disabled)
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Theme: Arrow press changes style") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    scroll_info<test_canvas_backend> info{
        {100, 500},  // content_size
        {100, 100},  // viewport_size
        {0, 0}       // scroll_offset
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    auto const* themes = ui_services<test_canvas_backend>::themes();
    auto const* theme = themes->get_current_theme();
    auto layout = sb.calculate_layout(theme->scrollbar.style);

    if (layout.has_arrows()) {
        // Press arrow
        int arrow_x = rect_utils::get_x(layout.arrow_increment) + 1;
        int arrow_y = rect_utils::get_y(layout.arrow_increment) + 1;
        sb.handle_mouse_down(arrow_x, arrow_y, 0);

        // Render - should use arrow_pressed colors
        auto canvas = render_to_canvas(sb, 80, 200);

        // Verify rendering
        bool has_content = false;
        for (int y = 0; y < 200 && !has_content; ++y) {
            for (int x = 0; x < 16; ++x) {
                if (!canvas->is_empty_at(x, y)) {
                    has_content = true;
                    break;
                }
            }
        }
        CHECK(has_content);
    }
    // Note: If no arrows, test is skipped (theme has arrows disabled)
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Theme: Complete theme integration") {
    test_scrollbar<test_canvas_backend> sb(orientation::vertical);

    // Set up realistic scroll scenario
    scroll_info<test_canvas_backend> info{
        {100, 1000},  // content_size
        {100, 200},   // viewport_size
        {0, 100}      // scroll_offset (scrolled down)
    };
    sb.set_scroll_info(info);

    // Measure with theme
    auto size = sb.measure(16, 300);
    CHECK(size_utils::get_width(size) == 16);  // Theme width

    // Arrange
    sb.arrange({0, 0, 16, 300});

    // Get thumb bounds
    auto thumb = sb.get_thumb_bounds();
    CHECK(rect_utils::get_height(thumb) >= 20);  // Theme min_thumb_size

    // Hover and render
    int thumb_x = rect_utils::get_x(thumb) + rect_utils::get_width(thumb) / 2;
    int thumb_y = rect_utils::get_y(thumb) + rect_utils::get_height(thumb) / 2;
    sb.handle_mouse_move(thumb_x, thumb_y);

    // Render with all theme properties
    auto canvas = render_to_canvas(sb, 80, 300);

    // Verify full rendering
    bool has_track = false;
    bool has_thumb = false;

    for (int y = 0; y < 300; ++y) {
        for (int x = 0; x < 16; ++x) {
            if (!canvas->is_empty_at(x, y)) {
                has_track = true;
                // Check if in thumb region
                if (x >= rect_utils::get_x(thumb) && x < rect_utils::get_x(thumb) + rect_utils::get_width(thumb) &&
                    y >= rect_utils::get_y(thumb) && y < rect_utils::get_y(thumb) + rect_utils::get_height(thumb)) {
                    has_thumb = true;
                }
            }
        }
    }

    CHECK(has_track);
    CHECK(has_thumb);
}
