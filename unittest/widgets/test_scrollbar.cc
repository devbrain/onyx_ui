/**
 * @file test_scrollbar.cc
 * @brief Unit tests for visual scrollbar widget
 * @author Testing Infrastructure Team
 * @date 2025-10-29
 */

#include <doctest/doctest.h>
#include <../../include/onyxui/widgets/containers/scroll/scrollbar.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/visual_test_helpers.hh"

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

    auto size_h = sb_h.measure(200_lu, 200_lu);
    auto size_v = sb_v.measure(200_lu, 200_lu);

    // Horizontal: height is fixed (thickness)
    CHECK(size_h.height.to_int() == 16);  // Fixed thickness

    // Vertical: width is fixed (thickness)
    CHECK(size_v.width.to_int() == 16);  // Fixed thickness
}

// =============================================================================
// 4. Measurement
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Horizontal measurement") {
    scrollbar<test_canvas_backend> sb(orientation::horizontal);

    auto size = sb.measure(300_lu, 100_lu);

    // Horizontal scrollbar has fixed height
    CHECK(size.height.to_int() == 16);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Vertical measurement") {
    scrollbar<test_canvas_backend> sb(orientation::vertical);

    auto size = sb.measure(100_lu, 300_lu);

    // Vertical scrollbar has fixed width
    CHECK(size.width.to_int() == 16);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Measurement with zero constraints") {
    scrollbar<test_canvas_backend> sb;

    auto size = sb.measure(0_lu, 0_lu);

    // Should still return valid size
    CHECK(size.width.to_int() > 0);
    CHECK(size.height.to_int() > 0);
}

// =============================================================================
// 5. Basic Arrangement Tests
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Thumb is zero-sized without scroll info") {
    scrollbar<test_canvas_backend> sb;

    [[maybe_unused]] auto size = sb.measure(100_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    auto thumb = sb.get_thumb_bounds();
    // No scroll info = no content = zero thumb
    CHECK(rect_utils::get_width(thumb) == 0);
    CHECK(rect_utils::get_height(thumb) == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollbar - Child widgets are created and visible") {
    scrollbar<test_canvas_backend> sb(orientation::vertical);

    scroll_info<test_canvas_backend> info{
        {1, 200},   // content (tall)
        {1, 100},   // viewport
        {0, 0}      // scroll at top
    };
    sb.set_scroll_info(info);

    [[maybe_unused]] auto size = sb.measure(1_lu, 100_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 1_lu, 100_lu});

    // Verify thumb bounds are calculated (child widget positioning)
    auto thumb = sb.get_thumb_bounds();
    CHECK(rect_utils::get_height(thumb) > 0);  // Thumb should have height
    CHECK(rect_utils::get_width(thumb) > 0);   // Thumb should have width

    // This test verifies the fix for child widget visibility
    // Before fix: child widgets were created but invisible, so nothing rendered
    // After fix: child widgets are explicitly set visible in constructor
    // The fact that thumb bounds are non-zero confirms children are properly set up
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>,
                  "scrollbar - Thumb visible immediately after set_scroll_info") {
    // This test verifies the fix for: "thumb is visible only after i scroll"
    // Bug: Thumb had zero bounds until next layout cycle after set_scroll_info()
    // Fix: set_scroll_info() now immediately updates child widget bounds

    scrollbar<test_canvas_backend> sb(orientation::vertical);

    // Initial measure and arrange with default scroll_info (all zeros)
    [[maybe_unused]] auto size = sb.measure(1_lu, 100_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 1_lu, 100_lu});

    // Thumb should be zero-sized initially (no scrolling needed)
    auto thumb_before = sb.get_thumb_bounds();
    CHECK(rect_utils::get_height(thumb_before) == 0);

    // Simulate what happens during first frame: scrollable widget measures content
    // and calls set_scroll_info() with valid values
    scroll_info<test_canvas_backend> info{
        {1, 200},   // content is tall (200)
        {1, 100},   // viewport is smaller (100) - scrolling needed!
        {0, 0}      // scroll at top
    };
    sb.set_scroll_info(info);

    // CRITICAL: Thumb should now have non-zero bounds IMMEDIATELY
    // Without the fix, thumb would still be zero until next arrange() call
    auto thumb_after = sb.get_thumb_bounds();
    CHECK(rect_utils::get_height(thumb_after) > 0);  // Should be ~50 (viewport/content ratio)
    CHECK(rect_utils::get_width(thumb_after) > 0);   // Should be 1 (scrollbar width)
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>,
                  "scrollbar - Thumb visible when set_scroll_info called BEFORE arrange") {
    // This reproduces the ACTUAL demo bug: scroll_view calls set_scroll_info()
    // during its measure phase, which happens BEFORE scrollbar is arranged.
    // Bug: Thumb invisible because set_scroll_info happened before arrange
    // Fix: do_arrange must use current scroll_info, not stale default

    scrollbar<test_canvas_backend> sb(orientation::vertical);

    // CRITICAL: Set scroll_info BEFORE measure/arrange (real demo flow!)
    scroll_info<test_canvas_backend> info{
        {1, 200},   // content is tall (200)
        {1, 100},   // viewport is smaller (100) - scrolling needed!
        {0, 0}      // scroll at top
    };
    sb.set_scroll_info(info);

    // Now measure and arrange (this is when scrollbar gets its bounds)
    [[maybe_unused]] auto size = sb.measure(1_lu, 100_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 1_lu, 100_lu});

    // BUG: Thumb should have non-zero bounds after arrange!
    // Without fix: thumb is zero-sized because arrange uses stale scroll_info
    auto thumb_after = sb.get_thumb_bounds();
    CHECK(rect_utils::get_height(thumb_after) > 0);  // Should be ~50 (viewport/content ratio)
    CHECK(rect_utils::get_width(thumb_after) > 0);   // Should be 1 (scrollbar width)
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>,
                  "scrollbar - BUG: Thumb not visible initially") {
    // This reproduces the demo bug: thumb is not visible until you scroll
    // Root cause: ???

    scrollbar<test_canvas_backend> sb(orientation::vertical);

    // Set scroll_info BEFORE arrange (real demo flow!)
    scroll_info<test_canvas_backend> info{
        {1, 200},   // content is tall (200)
        {1, 100},   // viewport is smaller (100) - scrolling needed!
        {0, 0}       // scroll at top
    };
    sb.set_scroll_info(info);

    // Measure and arrange with WIDTH=1 (like real scrollbar)
    [[maybe_unused]] auto size = sb.measure(1_lu, 100_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 1_lu, 100_lu});

    // Check thumb bounds - SHOULD BE NON-ZERO
    auto thumb_bounds = sb.get_thumb_bounds();

    INFO("Scrollbar bounds: " << sb.bounds().width.to_int() << "x" << sb.bounds().height.to_int());
    INFO("Content: " << size_utils::get_width(info.content_size) << "x" << size_utils::get_height(info.content_size));
    INFO("Viewport: " << size_utils::get_width(info.viewport_size) << "x" << size_utils::get_height(info.viewport_size));
    INFO("Thumb bounds: " << rect_utils::get_width(thumb_bounds) << "x" << rect_utils::get_height(thumb_bounds));

    // With 200 content and 100 viewport, thumb should be ~50 tall
    CHECK(rect_utils::get_height(thumb_bounds) > 0);

    // Now test RENDERING - does thumb actually draw?
    visual_test_harness<test_canvas_backend> harness(1, 100);
    typename test_canvas_backend::renderer_type renderer(harness.canvas());
    auto const* themes = ui_services<test_canvas_backend>::themes();
    auto const* theme = themes ? themes->get_current_theme() : nullptr;
    REQUIRE(theme != nullptr);

    sb.render(renderer, theme);

    // Count non-space characters (thumb should render as '#')
    int non_space_count = 0;
    for (int y = 0; y < 100; ++y) {
        char c = harness.canvas()->get_char(0, y);
        if (c != ' ') {
            non_space_count++;
        }
    }

    INFO("Canvas dump:\n" << harness.dump_canvas());
    INFO("Non-space characters: " << non_space_count);

    // We should have SOME rendering (thumb + arrows)
    CHECK(non_space_count > 0);
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

    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    auto thumb = sb.get_thumb_bounds();
    // Thumb size = (viewport_h / content_h) * track_h = (200/400) * 200 = 100
    CHECK(thumb.y == 0);        // At top of track
    CHECK(rect_utils::get_height(thumb) == 100); // 50% of track height
    CHECK(thumb.x == 0);
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

    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    auto thumb = sb.get_thumb_bounds();
    // Thumb size = 100 (50% of track)
    // Max thumb position = 200 - 100 = 100
    // At 50% scroll, thumb at 50% of available space = 50
    CHECK(thumb.y == 50);       // Middle of track
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

    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    auto thumb = sb.get_thumb_bounds();
    // At max scroll, thumb at bottom of track
    CHECK(thumb.y == 100);      // Bottom position (200 - 100)
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

    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

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

    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    auto thumb = sb.get_thumb_bounds();
    // Calculated thumb size = (200/20000) * 200 = 2 pixels
    // But minimum thumb size is 20 pixels
    CHECK(rect_utils::get_height(thumb) >= 20);  // Minimum enforced
    CHECK(thumb.y == 0);        // At top
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

    [[maybe_unused]] auto size = sb.measure(200_lu, 16_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 200_lu, 16_lu});

    auto thumb = sb.get_thumb_bounds();
    // Thumb size = (viewport_w / content_w) * track_w = (200/400) * 200 = 100
    CHECK(thumb.x == 0);        // At left of track
    CHECK(rect_utils::get_width(thumb) == 100);  // 50% of track width
    CHECK(thumb.y == 0);
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

    [[maybe_unused]] auto size = sb.measure(200_lu, 16_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 200_lu, 16_lu});

    auto thumb = sb.get_thumb_bounds();
    // At 50% scroll, thumb at 50% of available space = 50
    CHECK(thumb.x == 50);       // Middle of track
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

    [[maybe_unused]] auto size = sb.measure(200_lu, 16_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 200_lu, 16_lu});

    auto thumb = sb.get_thumb_bounds();
    // At max scroll, thumb at right of track
    CHECK(thumb.x == 100);      // Right position (200 - 100)
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

    [[maybe_unused]] auto size = sb.measure(200_lu, 16_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 200_lu, 16_lu});

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

    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

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

    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

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

    [[maybe_unused]] auto size = sb.measure(16_lu, 0_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 0_lu});

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

    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    auto thumb = sb.get_thumb_bounds();
    // Very large content should still enforce minimum thumb size
    CHECK(rect_utils::get_height(thumb) >= 20);
    // Thumb should be at top when scroll is at 0
    CHECK(thumb.y == 0);
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

    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    auto thumb = sb.get_thumb_bounds();
    // Thumb should still be calculated (clamped to track bounds)
    // At way-beyond-max scroll, thumb should be at bottom
    int thumb_y = thumb.y;
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
    [[maybe_unused]] auto size = sb.measure(16_lu, 150_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 150_lu});

    auto thumb1 = sb.get_thumb_bounds();
    int height1 = rect_utils::get_height(thumb1);

    // Change content size (affects thumb size)
    sb.set_scroll_info(info2);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 150_lu});

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
    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    auto thumb1 = sb.get_thumb_bounds();
    int height1 = rect_utils::get_height(thumb1);

    // Change viewport size (affects thumb size)
    sb.set_scroll_info(info2);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

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
    [[maybe_unused]] auto size = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    auto thumb1 = sb.get_thumb_bounds();
    CHECK(thumb1.y == 0);  // At top

    // Scroll to middle
    sb.set_scroll_info(info2);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    auto thumb2 = sb.get_thumb_bounds();
    CHECK(thumb2.y == 50);  // At middle

    // Scroll to bottom
    sb.set_scroll_info(info3);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    auto thumb3 = sb.get_thumb_bounds();
    CHECK(thumb3.y == 100);  // At bottom
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

    [[maybe_unused]] auto size_v = sb.measure(16_lu, 200_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

    auto thumb_v = sb.get_thumb_bounds();
    // Vertical: uses Y offset (100), thumb positioned proportionally
    CHECK(thumb_v.y == 50);  // 50% scroll = 50% position

    // Switch to horizontal
    sb.set_orientation(orientation::horizontal);

    [[maybe_unused]] auto size_h = sb.measure(200_lu, 16_lu);
    sb.arrange(logical_rect{0_lu, 0_lu, 200_lu, 16_lu});

    auto thumb_h = sb.get_thumb_bounds();
    // Horizontal: uses X offset (50), thumb positioned proportionally
    CHECK(thumb_h.x == 25);  // 25% scroll = 25% position
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
        [[maybe_unused]] auto size_h = sb.measure(200_lu, 16_lu);
        sb.arrange(logical_rect{0_lu, 0_lu, 200_lu, 16_lu});

        auto thumb_h = sb.get_thumb_bounds();
        CHECK(thumb_h.x == 50);  // Consistent horizontal positioning

        sb.set_orientation(orientation::vertical);
        [[maybe_unused]] auto size_v = sb.measure(16_lu, 200_lu);
        sb.arrange(logical_rect{0_lu, 0_lu, 16_lu, 200_lu});

        auto thumb_v = sb.get_thumb_bounds();
        CHECK(thumb_v.y == 50);  // Consistent vertical positioning
    }

    // Should end in vertical orientation
    CHECK(sb.get_orientation() == orientation::vertical);
}
