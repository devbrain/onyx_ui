/**
 * @file test_scrollbar_styles.cc
 * @brief Visual tests for scrollbar style rendering and layout (Gap 2)
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

// Helper to create a scrollbar with scroll info
static void setup_scrollbar_with_content(
    scrollbar<test_canvas_backend>& sb,
    orientation orient,
    int content_size,
    int viewport_size,
    int scroll_offset
) {
    sb.set_orientation(orient);

    scroll_info<test_canvas_backend> info;
    if (orient == orientation::vertical) {
        info.content_size = {100, content_size};
        info.viewport_size = {100, viewport_size};
        info.scroll_offset = {0, scroll_offset};
    } else {
        info.content_size = {content_size, 100};
        info.viewport_size = {viewport_size, 100};
        info.scroll_offset = {scroll_offset, 0};
    }

    sb.set_scroll_info(info);
}

// =============================================================================
// Visual Rendering Tests - Vertical Scrollbar
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Visual: Vertical scrollbar renders") {
    scrollbar<test_canvas_backend> sb(orientation::vertical);
    setup_scrollbar_with_content(sb, orientation::vertical, 500, 100, 0);

    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    // Render to canvas
    auto canvas = render_to_canvas(sb, 80, 200);

    SUBCASE("Canvas has content") {
        // The scrollbar should render something (track/thumb)
        bool has_content = false;
        for (int y = 0; y < 200; ++y) {
            for (int x = 0; x < 16; ++x) {
                if (!canvas->is_empty_at(x, y)) {
                    has_content = true;
                    break;
                }
            }
            if (has_content) break;
        }
        CHECK(has_content);
    }

    SUBCASE("Thumb is visible") {
        // Get thumb bounds and verify it's visible on canvas
        auto thumb = sb.get_thumb_bounds();
        int thumb_x = rect_utils::get_x(thumb);
        int thumb_y = rect_utils::get_y(thumb);
        int thumb_w = rect_utils::get_width(thumb);
        int thumb_h = rect_utils::get_height(thumb);

        // Thumb should exist
        CHECK(thumb_h > 0);
        CHECK(thumb_w > 0);

        // Verify thumb area has content
        bool thumb_rendered = false;
        for (int y = thumb_y; y < thumb_y + thumb_h && y < 200; ++y) {
            for (int x = thumb_x; x < thumb_x + thumb_w && x < 16; ++x) {
                if (!canvas->is_empty_at(x, y)) {
                    thumb_rendered = true;
                    break;
                }
            }
            if (thumb_rendered) break;
        }
        CHECK(thumb_rendered);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Visual: Thumb position changes with scroll") {
    scrollbar<test_canvas_backend> sb(orientation::vertical);

    SUBCASE("Scroll at top") {
        setup_scrollbar_with_content(sb, orientation::vertical, 400, 100, 0);
        [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
        sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

        auto thumb = sb.get_thumb_bounds();
        int thumb_y_top = rect_utils::get_y(thumb);

        // Thumb should be near top
        CHECK(thumb_y_top < 50);
    }

    SUBCASE("Scroll at middle") {
        setup_scrollbar_with_content(sb, orientation::vertical, 400, 100, 150);
        [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
        sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

        auto thumb = sb.get_thumb_bounds();
        int thumb_y_mid = rect_utils::get_y(thumb);

        // Thumb should be in middle region
        CHECK(thumb_y_mid > 50);
        CHECK(thumb_y_mid < 150);
    }

    SUBCASE("Scroll at bottom") {
        setup_scrollbar_with_content(sb, orientation::vertical, 400, 100, 300);
        [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
        sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

        auto thumb = sb.get_thumb_bounds();
        int thumb_y_bot = rect_utils::get_y(thumb);

        // Thumb should be near bottom
        CHECK(thumb_y_bot > 100);
    }
}

// =============================================================================
// Visual Rendering Tests - Horizontal Scrollbar
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Visual: Horizontal scrollbar renders") {
    scrollbar<test_canvas_backend> sb(orientation::horizontal);
    setup_scrollbar_with_content(sb, orientation::horizontal, 500, 100, 0);

    [[maybe_unused]] auto size = sb.measure(200_lu, 16_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 200_lu, 16_lu});

    // Render to canvas
    auto canvas = render_to_canvas(sb, 200, 80);

    SUBCASE("Canvas has content") {
        bool has_content = false;
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 200; ++x) {
                if (!canvas->is_empty_at(x, y)) {
                    has_content = true;
                    break;
                }
            }
            if (has_content) break;
        }
        CHECK(has_content);
    }

    SUBCASE("Thumb is visible") {
        auto thumb = sb.get_thumb_bounds();
        int thumb_w = rect_utils::get_width(thumb);
        CHECK(thumb_w > 0);
    }
}

// =============================================================================
// Style-Specific Visual Tests
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Visual: Thumb size reflects viewport ratio") {
    scrollbar<test_canvas_backend> sb(orientation::vertical);

    SUBCASE("Small viewport (25%) = small thumb") {
        setup_scrollbar_with_content(sb, orientation::vertical, 400, 100, 0);
        [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
        sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

        auto thumb = sb.get_thumb_bounds();
        int thumb_h = rect_utils::get_height(thumb);

        // Viewport is 25% of content, thumb should be ~25% of track
        // Track ≈ 200px, so thumb ≈ 50px (viewport:content = 100:400 = 1:4)
        CHECK(thumb_h >= 20);  // Min thumb size
        CHECK(thumb_h <= 75);  // Should be roughly 1/4 of track
    }

    SUBCASE("Large viewport (75%) = large thumb") {
        setup_scrollbar_with_content(sb, orientation::vertical, 200, 150, 0);
        [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
        sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

        auto thumb = sb.get_thumb_bounds();
        int thumb_h = rect_utils::get_height(thumb);

        // Viewport is 75% of content, thumb should be ~75% of track
        CHECK(thumb_h >= 100);  // Should be substantial portion
        CHECK(thumb_h <= 200);  // Max = track size
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Visual: No thumb when content fits") {
    scrollbar<test_canvas_backend> sb(orientation::vertical);
    // Content fits in viewport (no scrolling needed)
    setup_scrollbar_with_content(sb, orientation::vertical, 100, 200, 0);

    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    auto thumb = sb.get_thumb_bounds();

    // Thumb should be zero-sized
    CHECK(rect_utils::get_height(thumb) == 0);
    CHECK(rect_utils::get_width(thumb) == 0);
}

// =============================================================================
// Edge Cases - Visual Verification
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Visual: Very small scrollbar") {
    scrollbar<test_canvas_backend> sb(orientation::vertical);
    setup_scrollbar_with_content(sb, orientation::vertical, 500, 100, 0);

    // Very small height
    [[maybe_unused]] auto size = sb.measure(16_lu, 40_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 40_lu});

    auto canvas = render_to_canvas(sb, 80, 40);

    SUBCASE("Renders without crash") {
        // Just verify rendering completed
        CHECK(canvas->width() == 80);
        CHECK(canvas->height() == 40);
    }

    SUBCASE("Thumb respects minimum size") {
        auto thumb = sb.get_thumb_bounds();
        int thumb_h = rect_utils::get_height(thumb);
        // Should honor minimum thumb size even in small scrollbar
        CHECK(thumb_h >= 20);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Visual: Extreme scroll ratios") {
    scrollbar<test_canvas_backend> sb(orientation::vertical);

    SUBCASE("Huge content (1:100 ratio)") {
        setup_scrollbar_with_content(sb, orientation::vertical, 10000, 100, 0);
        [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
        sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

        auto thumb = sb.get_thumb_bounds();
        int thumb_h = rect_utils::get_height(thumb);

        // Minimum thumb size should be enforced
        CHECK(thumb_h >= 20);
        CHECK(thumb_h <= 200);
    }

    SUBCASE("Nearly equal content (95% fit)") {
        setup_scrollbar_with_content(sb, orientation::vertical, 210, 200, 0);
        [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
        sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

        auto thumb = sb.get_thumb_bounds();
        int thumb_h = rect_utils::get_height(thumb);

        // Thumb should be almost full track size
        CHECK(thumb_h >= 180);
        CHECK(thumb_h <= 200);
    }
}

// =============================================================================
// Orientation Change - Visual Verification
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Visual: Orientation affects visual layout") {
    SUBCASE("Vertical scrollbar is tall and thin") {
        scrollbar<test_canvas_backend> sb(orientation::vertical);
        setup_scrollbar_with_content(sb, orientation::vertical, 500, 100, 0);

        [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
        sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

        auto thumb = sb.get_thumb_bounds();
        CHECK(rect_utils::get_height(thumb) > rect_utils::get_width(thumb));
    }

    SUBCASE("Horizontal scrollbar is wide and short") {
        scrollbar<test_canvas_backend> sb(orientation::horizontal);
        setup_scrollbar_with_content(sb, orientation::horizontal, 500, 100, 0);

        [[maybe_unused]] auto size = sb.measure(200_lu, 16_lu);
        sb.arrange(logical_rect{0_lu, 0_lu, 200_lu, 16_lu});

        auto thumb = sb.get_thumb_bounds();
        CHECK(rect_utils::get_width(thumb) > rect_utils::get_height(thumb));
    }
}
