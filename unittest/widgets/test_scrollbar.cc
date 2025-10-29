/**
 * @file test_scrollbar.cc
 * @brief Unit tests for visual scrollbar widget
 * @author Testing Infrastructure Team
 * @date 2025-10-29
 */

#include <doctest/doctest.h>
#include <onyxui/widgets/scrollbar.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_canvas_backend.hh"

using namespace onyxui;
using namespace onyxui::testing;

// =============================================================================
// 1. Construction and Basic Properties
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Default construction") {
    scrollbar<test_canvas_backend> sb;

    CHECK(sb.get_orientation() == orientation::vertical);

    auto info = sb.get_scroll_info();
    CHECK(size_utils::get_width(info.content_size) == 0);
    CHECK(size_utils::get_height(info.content_size) == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Construction with orientation") {
    scrollbar<test_canvas_backend> sb_h(orientation::horizontal);
    scrollbar<test_canvas_backend> sb_v(orientation::vertical);

    CHECK(sb_h.get_orientation() == orientation::horizontal);
    CHECK(sb_v.get_orientation() == orientation::vertical);
}

// =============================================================================
// 2. Scroll Info Management
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Set scroll info") {
    scrollbar<test_canvas_backend> sb;

    scroll_info<test_canvas_backend> info{
        {200, 300},  // content_size
        {100, 150},  // viewport_size
        {10, 20}     // scroll_offset
    };

    sb.set_scroll_info(info);

    auto retrieved = sb.get_scroll_info();
    CHECK(retrieved == info);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Scroll info with no scrolling needed") {
    scrollbar<test_canvas_backend> sb;

    scroll_info<test_canvas_backend> info{
        {50, 50},    // content fits
        {100, 100},  // viewport
        {0, 0}       // no scroll
    };

    sb.set_scroll_info(info);

    CHECK(sb.get_scroll_info() == info);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Scroll info updates") {
    scrollbar<test_canvas_backend> sb;

    scroll_info<test_canvas_backend> info1{{100, 100}, {50, 50}, {0, 0}};
    scroll_info<test_canvas_backend> info2{{200, 200}, {50, 50}, {25, 30}};

    sb.set_scroll_info(info1);
    CHECK(sb.get_scroll_info() == info1);

    sb.set_scroll_info(info2);
    CHECK(sb.get_scroll_info() == info2);
}

// =============================================================================
// 3. Orientation
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Change orientation") {
    scrollbar<test_canvas_backend> sb(orientation::horizontal);
    CHECK(sb.get_orientation() == orientation::horizontal);

    sb.set_orientation(orientation::vertical);
    CHECK(sb.get_orientation() == orientation::vertical);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Orientation affects measurement") {
    scrollbar<test_canvas_backend> sb_h(orientation::horizontal);
    scrollbar<test_canvas_backend> sb_v(orientation::vertical);

    auto size_h = sb_h.measure(200, 200);
    auto size_v = sb_v.measure(200, 200);

    // Horizontal: height is fixed (thickness)
    CHECK(size_utils::get_height(size_h) == 16);  // Fixed thickness

    // Vertical: width is fixed (thickness)
    CHECK(size_utils::get_width(size_v) == 16);  // Fixed thickness
}

// =============================================================================
// 4. Measurement
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Horizontal measurement") {
    scrollbar<test_canvas_backend> sb(orientation::horizontal);

    auto size = sb.measure(300, 100);

    // Horizontal scrollbar has fixed height
    CHECK(size_utils::get_height(size) == 16);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Vertical measurement") {
    scrollbar<test_canvas_backend> sb(orientation::vertical);

    auto size = sb.measure(100, 300);

    // Vertical scrollbar has fixed width
    CHECK(size_utils::get_width(size) == 16);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Measurement with zero constraints") {
    scrollbar<test_canvas_backend> sb;

    auto size = sb.measure(0, 0);

    // Should still return valid size
    CHECK(size_utils::get_width(size) > 0);
    CHECK(size_utils::get_height(size) > 0);
}

// =============================================================================
// 5. Basic Arrangement Tests
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Thumb is zero-sized without scroll info") {
    scrollbar<test_canvas_backend> sb;

    [[maybe_unused]] auto size = sb.measure(100, 200);
    sb.arrange({0, 0, 16, 200});

    auto thumb = sb.get_thumb_bounds();
    // No scroll info = no content = zero thumb
    CHECK(rect_utils::get_width(thumb) == 0);
    CHECK(rect_utils::get_height(thumb) == 0);
}

// =============================================================================
// 6. Thumb Calculation - Vertical Scrollbar
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Vertical thumb at scroll position 0") {
    scrollbar<test_canvas_backend> sb(orientation::vertical);

    scroll_info<test_canvas_backend> info{
        {100, 400},  // content (4x viewport height)
        {100, 200},  // viewport
        {0, 0}       // scroll at top
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    auto thumb = sb.get_thumb_bounds();
    // Thumb size = (viewport_h / content_h) * track_h = (200/400) * 200 = 100
    CHECK(rect_utils::get_y(thumb) == 0);        // At top of track
    CHECK(rect_utils::get_height(thumb) == 100); // 50% of track height
    CHECK(rect_utils::get_x(thumb) == 0);
    CHECK(rect_utils::get_width(thumb) == 16);   // Scrollbar width
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Vertical thumb at middle scroll position") {
    scrollbar<test_canvas_backend> sb(orientation::vertical);

    scroll_info<test_canvas_backend> info{
        {100, 400},  // content (4x viewport height)
        {100, 200},  // viewport
        {0, 100}     // scroll halfway (max_scroll = 200, so 100 = 50%)
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    auto thumb = sb.get_thumb_bounds();
    // Thumb size = 100 (50% of track)
    // Max thumb position = 200 - 100 = 100
    // At 50% scroll, thumb at 50% of available space = 50
    CHECK(rect_utils::get_y(thumb) == 50);       // Middle of track
    CHECK(rect_utils::get_height(thumb) == 100); // Still 50% of track
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Vertical thumb at max scroll position") {
    scrollbar<test_canvas_backend> sb(orientation::vertical);

    scroll_info<test_canvas_backend> info{
        {100, 400},  // content (4x viewport height)
        {100, 200},  // viewport
        {0, 200}     // scroll at bottom (max_scroll = 200)
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    auto thumb = sb.get_thumb_bounds();
    // At max scroll, thumb at bottom of track
    CHECK(rect_utils::get_y(thumb) == 100);      // Bottom position (200 - 100)
    CHECK(rect_utils::get_height(thumb) == 100); // Still 50% of track
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Vertical thumb with content that fits") {
    scrollbar<test_canvas_backend> sb(orientation::vertical);

    scroll_info<test_canvas_backend> info{
        {100, 100},  // content fits in viewport
        {100, 200},  // viewport
        {0, 0}       // no scroll needed
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    auto thumb = sb.get_thumb_bounds();
    // Content fits - no scrolling needed, thumb should be zero height
    CHECK(rect_utils::get_height(thumb) == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Vertical thumb with minimum size enforcement") {
    scrollbar<test_canvas_backend> sb(orientation::vertical);

    // Huge content (100x viewport) - would make tiny thumb
    scroll_info<test_canvas_backend> info{
        {100, 20000},  // content (100x viewport height)
        {100, 200},    // viewport
        {0, 0}         // scroll at top
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    auto thumb = sb.get_thumb_bounds();
    // Calculated thumb size = (200/20000) * 200 = 2 pixels
    // But minimum thumb size is 20 pixels
    CHECK(rect_utils::get_height(thumb) >= 20);  // Minimum enforced
    CHECK(rect_utils::get_y(thumb) == 0);        // At top
}

// =============================================================================
// 7. Thumb Calculation - Horizontal Scrollbar
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Horizontal thumb at scroll position 0") {
    scrollbar<test_canvas_backend> sb(orientation::horizontal);

    scroll_info<test_canvas_backend> info{
        {400, 100},  // content (4x viewport width)
        {200, 100},  // viewport
        {0, 0}       // scroll at left
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(200, 16);
    sb.arrange({0, 0, 200, 16});

    auto thumb = sb.get_thumb_bounds();
    // Thumb size = (viewport_w / content_w) * track_w = (200/400) * 200 = 100
    CHECK(rect_utils::get_x(thumb) == 0);        // At left of track
    CHECK(rect_utils::get_width(thumb) == 100);  // 50% of track width
    CHECK(rect_utils::get_y(thumb) == 0);
    CHECK(rect_utils::get_height(thumb) == 16);  // Scrollbar height
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Horizontal thumb at middle scroll position") {
    scrollbar<test_canvas_backend> sb(orientation::horizontal);

    scroll_info<test_canvas_backend> info{
        {400, 100},  // content (4x viewport width)
        {200, 100},  // viewport
        {100, 0}     // scroll halfway (max_scroll = 200, so 100 = 50%)
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(200, 16);
    sb.arrange({0, 0, 200, 16});

    auto thumb = sb.get_thumb_bounds();
    // At 50% scroll, thumb at 50% of available space = 50
    CHECK(rect_utils::get_x(thumb) == 50);       // Middle of track
    CHECK(rect_utils::get_width(thumb) == 100);  // Still 50% of track
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Horizontal thumb at max scroll position") {
    scrollbar<test_canvas_backend> sb(orientation::horizontal);

    scroll_info<test_canvas_backend> info{
        {400, 100},  // content (4x viewport width)
        {200, 100},  // viewport
        {200, 0}     // scroll at right (max_scroll = 200)
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(200, 16);
    sb.arrange({0, 0, 200, 16});

    auto thumb = sb.get_thumb_bounds();
    // At max scroll, thumb at right of track
    CHECK(rect_utils::get_x(thumb) == 100);      // Right position (200 - 100)
    CHECK(rect_utils::get_width(thumb) == 100);  // Still 50% of track
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Horizontal thumb with content that fits") {
    scrollbar<test_canvas_backend> sb(orientation::horizontal);

    scroll_info<test_canvas_backend> info{
        {100, 100},  // content fits in viewport
        {200, 100},  // viewport
        {0, 0}       // no scroll needed
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(200, 16);
    sb.arrange({0, 0, 200, 16});

    auto thumb = sb.get_thumb_bounds();
    // Content fits - no scrolling needed, thumb should be zero width
    CHECK(rect_utils::get_width(thumb) == 0);
}

// =============================================================================
// 8. Edge Cases
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Zero content size") {
    scrollbar<test_canvas_backend> sb;

    scroll_info<test_canvas_backend> info{
        {0, 0},      // zero content
        {100, 200},  // viewport
        {0, 0}
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    auto thumb = sb.get_thumb_bounds();
    // Zero content = zero thumb
    CHECK(rect_utils::get_width(thumb) == 0);
    CHECK(rect_utils::get_height(thumb) == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Zero viewport size") {
    scrollbar<test_canvas_backend> sb;

    scroll_info<test_canvas_backend> info{
        {100, 200},  // content
        {0, 0},      // zero viewport
        {0, 0}
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    auto thumb = sb.get_thumb_bounds();
    // Zero viewport = can't calculate meaningful thumb
    CHECK(rect_utils::get_width(thumb) == 0);
    CHECK(rect_utils::get_height(thumb) == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Zero track length") {
    scrollbar<test_canvas_backend> sb;

    scroll_info<test_canvas_backend> info{
        {100, 400},
        {100, 200},
        {0, 0}
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 0);
    sb.arrange({0, 0, 16, 0});

    auto thumb = sb.get_thumb_bounds();
    // Zero track = zero thumb
    CHECK(rect_utils::get_height(thumb) == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Very large content") {
    scrollbar<test_canvas_backend> sb;

    scroll_info<test_canvas_backend> info{
        {100, 100000},  // Extremely large content
        {100, 200},
        {0, 0}
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    auto thumb = sb.get_thumb_bounds();
    // Very large content should still enforce minimum thumb size
    CHECK(rect_utils::get_height(thumb) >= 20);
    // Thumb should be at top when scroll is at 0
    CHECK(rect_utils::get_y(thumb) == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Scroll offset beyond max") {
    scrollbar<test_canvas_backend> sb;

    // Note: scrollable widget clamps scroll offset, but scrollbar should handle it
    scroll_info<test_canvas_backend> info{
        {100, 400},
        {100, 200},
        {0, 500}     // Scroll beyond max (max would be 200)
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    auto thumb = sb.get_thumb_bounds();
    // Thumb should still be calculated (clamped to track bounds)
    // At way-beyond-max scroll, thumb should be at bottom
    int thumb_y = rect_utils::get_y(thumb);
    int thumb_h = rect_utils::get_height(thumb);
    CHECK(thumb_y + thumb_h <= 200);  // Thumb stays within track
}

// =============================================================================
// 9. Scroll Info State Changes
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Content size changes") {
    scrollbar<test_canvas_backend> sb;

    scroll_info<test_canvas_backend> info1{{100, 200}, {100, 150}, {0, 0}};
    scroll_info<test_canvas_backend> info2{{100, 400}, {100, 150}, {0, 0}};

    sb.set_scroll_info(info1);
    [[maybe_unused]] auto size = sb.measure(16, 150);
    sb.arrange({0, 0, 16, 150});

    auto thumb1 = sb.get_thumb_bounds();
    int height1 = rect_utils::get_height(thumb1);

    // Change content size (affects thumb size)
    sb.set_scroll_info(info2);
    sb.arrange({0, 0, 16, 150});

    auto thumb2 = sb.get_thumb_bounds();
    int height2 = rect_utils::get_height(thumb2);

    // More content = smaller thumb
    CHECK(height2 < height1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Viewport size changes") {
    scrollbar<test_canvas_backend> sb;

    scroll_info<test_canvas_backend> info1{{100, 400}, {100, 100}, {0, 0}};
    scroll_info<test_canvas_backend> info2{{100, 400}, {100, 200}, {0, 0}};

    sb.set_scroll_info(info1);
    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    auto thumb1 = sb.get_thumb_bounds();
    int height1 = rect_utils::get_height(thumb1);

    // Change viewport size (affects thumb size)
    sb.set_scroll_info(info2);
    sb.arrange({0, 0, 16, 200});

    auto thumb2 = sb.get_thumb_bounds();
    int height2 = rect_utils::get_height(thumb2);

    // Larger viewport = larger thumb
    CHECK(height2 > height1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Scroll position changes") {
    scrollbar<test_canvas_backend> sb;

    scroll_info<test_canvas_backend> info1{{100, 400}, {100, 200}, {0, 0}};
    scroll_info<test_canvas_backend> info2{{100, 400}, {100, 200}, {0, 100}};
    scroll_info<test_canvas_backend> info3{{100, 400}, {100, 200}, {0, 200}};

    sb.set_scroll_info(info1);
    [[maybe_unused]] auto size = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    auto thumb1 = sb.get_thumb_bounds();
    CHECK(rect_utils::get_y(thumb1) == 0);  // At top

    // Scroll to middle
    sb.set_scroll_info(info2);
    sb.arrange({0, 0, 16, 200});

    auto thumb2 = sb.get_thumb_bounds();
    CHECK(rect_utils::get_y(thumb2) == 50);  // At middle

    // Scroll to bottom
    sb.set_scroll_info(info3);
    sb.arrange({0, 0, 16, 200});

    auto thumb3 = sb.get_thumb_bounds();
    CHECK(rect_utils::get_y(thumb3) == 100);  // At bottom
}

// =============================================================================
// 10. Orientation Switching
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Switch orientation with active scroll info") {
    scrollbar<test_canvas_backend> sb(orientation::vertical);

    scroll_info<test_canvas_backend> info{
        {400, 400},  // Square content
        {200, 200},  // Square viewport
        {50, 100}    // Some scroll offset
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size_v = sb.measure(16, 200);
    sb.arrange({0, 0, 16, 200});

    auto thumb_v = sb.get_thumb_bounds();
    // Vertical: uses Y offset (100), thumb positioned proportionally
    CHECK(rect_utils::get_y(thumb_v) == 50);  // 50% scroll = 50% position

    // Switch to horizontal
    sb.set_orientation(orientation::horizontal);

    [[maybe_unused]] auto size_h = sb.measure(200, 16);
    sb.arrange({0, 0, 200, 16});

    auto thumb_h = sb.get_thumb_bounds();
    // Horizontal: uses X offset (50), thumb positioned proportionally
    CHECK(rect_utils::get_x(thumb_h) == 25);  // 25% scroll = 25% position
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Multiple orientation switches") {
    scrollbar<test_canvas_backend> sb;

    scroll_info<test_canvas_backend> info{
        {400, 400},
        {200, 200},
        {100, 100}
    };
    sb.set_scroll_info(info);

    for (int i = 0; i < 3; ++i) {
        sb.set_orientation(orientation::horizontal);
        [[maybe_unused]] auto size_h = sb.measure(200, 16);
        sb.arrange({0, 0, 200, 16});

        auto thumb_h = sb.get_thumb_bounds();
        CHECK(rect_utils::get_x(thumb_h) == 50);  // Consistent horizontal positioning

        sb.set_orientation(orientation::vertical);
        [[maybe_unused]] auto size_v = sb.measure(16, 200);
        sb.arrange({0, 0, 16, 200});

        auto thumb_v = sb.get_thumb_bounds();
        CHECK(rect_utils::get_y(thumb_v) == 50);  // Consistent vertical positioning
    }

    // Should end in vertical orientation
    CHECK(sb.get_orientation() == orientation::vertical);
}
