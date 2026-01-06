/**
 * @file test_backend_metrics.cc
 * @brief Comprehensive tests for backend_metrics (logical→physical conversion)
 * @author OnyxUI Framework
 * @date 2025-11-26
 */

#include <doctest/doctest.h>
#include <../../include/onyxui/core/backend_metrics.hh>
#include <../../include/onyxui/core/physical_conversions.hh>
#include <../../unittest/utils/test_canvas_backend.hh>

using namespace onyxui;
using namespace onyxui::testing;

// ============================================================================
// Snapping Tests
// ============================================================================

TEST_CASE("backend_metrics - Snap logical X to physical") {
    backend_metrics<test_canvas_backend> metrics;
    metrics.logical_to_physical_x = 1.0;  // 1:1 mapping
    metrics.dpi_scale = 1.0;

    SUBCASE("Floor snapping") {
        CHECK(metrics.snap_to_physical_x(10.0_lu, snap_mode::floor) == physical_x(10));
        CHECK(metrics.snap_to_physical_x(10.7_lu, snap_mode::floor) == physical_x(10));
        CHECK(metrics.snap_to_physical_x(10.3_lu, snap_mode::floor) == physical_x(10));
    }

    SUBCASE("Ceil snapping") {
        CHECK(metrics.snap_to_physical_x(10.0_lu, snap_mode::ceil) == physical_x(10));
        CHECK(metrics.snap_to_physical_x(10.3_lu, snap_mode::ceil) == physical_x(11));
        CHECK(metrics.snap_to_physical_x(10.7_lu, snap_mode::ceil) == physical_x(11));
    }

    SUBCASE("Round snapping") {
        CHECK(metrics.snap_to_physical_x(10.0_lu, snap_mode::round) == physical_x(10));
        CHECK(metrics.snap_to_physical_x(10.3_lu, snap_mode::round) == physical_x(10));
        CHECK(metrics.snap_to_physical_x(10.7_lu, snap_mode::round) == physical_x(11));
        CHECK(metrics.snap_to_physical_x(10.5_lu, snap_mode::round) == physical_x(11));  // Round half up
    }

    SUBCASE("Truncate (towards zero)") {
        CHECK(metrics.snap_to_physical_x(10.0_lu, snap_mode::truncate) == physical_x(10));
        CHECK(metrics.snap_to_physical_x(10.7_lu, snap_mode::truncate) == physical_x(10));
    }
}

TEST_CASE("backend_metrics - Snap logical Y to physical") {
    backend_metrics<test_canvas_backend> metrics;
    metrics.logical_to_physical_y = 1.0;  // 1:1 mapping
    metrics.dpi_scale = 1.0;

    SUBCASE("Floor snapping") {
        CHECK(metrics.snap_to_physical_y(20.7_lu, snap_mode::floor) == physical_y(20));
    }

    SUBCASE("Ceil snapping") {
        CHECK(metrics.snap_to_physical_y(20.3_lu, snap_mode::ceil) == physical_y(21));
    }

    SUBCASE("Round snapping") {
        CHECK(metrics.snap_to_physical_y(20.5_lu, snap_mode::round) == physical_y(21));
    }
}

// ============================================================================
// Conversion Tests (Terminal Backend 1:1)
// ============================================================================

TEST_CASE("backend_metrics - Terminal conversion (1:1 mapping)") {
    auto metrics = make_terminal_metrics<test_canvas_backend>();

    SUBCASE("Verify terminal metrics") {
        CHECK(metrics.logical_to_physical_x == 1.0);
        CHECK(metrics.logical_to_physical_y == 1.0);
        CHECK(metrics.dpi_scale == 1.0);
    }

    SUBCASE("Snap size") {
        logical_size size(80.0_lu, 25.0_lu);
        auto physical = metrics.snap_size(size);
        CHECK(physical.width.value == 80);
        CHECK(physical.height.value == 25);
    }

    SUBCASE("Snap point") {
        logical_point point(10.5_lu, 20.7_lu);
        auto physical = metrics.snap_point(point);
        CHECK(physical.x.value == 10);  // Floor
        CHECK(physical.y.value == 20);  // Floor
    }

    SUBCASE("Snap rect (edge-based)") {
        logical_rect rect(10.0_lu, 20.0_lu, 30.0_lu, 40.0_lu);
        auto physical = metrics.snap_rect(rect);

        CHECK(physical.x.value == 10);
        CHECK(physical.y.value == 20);
        CHECK(physical.width.value == 30);
        CHECK(physical.height.value == 40);
    }

    SUBCASE("Snap rect with fractional coordinates") {
        // Position 10.5, 20.7 - should floor to 10, 20
        // Size 30.0, 40.0
        // Right edge: 10.5 + 30.0 = 40.5 → ceil to 41
        // Bottom edge: 20.7 + 40.0 = 60.7 → ceil to 61
        // Width from edges: 41 - 10 = 31
        // Height from edges: 61 - 20 = 41
        logical_rect rect(10.5_lu, 20.7_lu, 30.0_lu, 40.0_lu);
        auto physical = metrics.snap_rect(rect);

        CHECK(physical.x.value == 10);   // Floor(10.5)
        CHECK(physical.y.value == 20);   // Floor(20.7)
        CHECK(physical.width.value == 31);   // Ceil(40.5) - Floor(10.5) = 41 - 10
        CHECK(physical.height.value == 41);   // Ceil(60.7) - Floor(20.7) = 61 - 20
    }
}

// ============================================================================
// Conversion Tests (GUI Backend 8:1)
// ============================================================================

TEST_CASE("backend_metrics - GUI conversion (8 pixels per logical unit)") {
    auto metrics = make_gui_metrics<test_canvas_backend>();

    SUBCASE("Verify GUI metrics") {
        CHECK(metrics.logical_to_physical_x == 8.0);
        CHECK(metrics.logical_to_physical_y == 8.0);
        CHECK(metrics.dpi_scale == 1.0);
    }

    SUBCASE("Snap size") {
        logical_size size(10.0_lu, 5.0_lu);
        auto physical = metrics.snap_size(size);
        CHECK(physical.width.value == 80);  // 10 * 8
        CHECK(physical.height.value == 40);  // 5 * 8
    }

    SUBCASE("Snap point") {
        logical_point point(10.5_lu, 5.25_lu);
        auto physical = metrics.snap_point(point);
        CHECK(physical.x.value == 84);  // Floor(10.5 * 8) = Floor(84) = 84
        CHECK(physical.y.value == 42);  // Floor(5.25 * 8) = Floor(42) = 42
    }

    SUBCASE("Snap rect") {
        logical_rect rect(10.0_lu, 5.0_lu, 20.0_lu, 10.0_lu);
        auto physical = metrics.snap_rect(rect);

        CHECK(physical.x.value == 80);   // 10 * 8
        CHECK(physical.y.value == 40);   // 5 * 8
        CHECK(physical.width.value == 160);  // 20 * 8
        CHECK(physical.height.value == 80);   // 10 * 8
    }

    SUBCASE("Snap rect with fractional coordinates") {
        // Position 10.5, 5.25 in logical units
        // Size 20.0, 10.0 in logical units
        // Left: Floor(10.5 * 8) = Floor(84) = 84
        // Top: Floor(5.25 * 8) = Floor(42) = 42
        // Right: Ceil((10.5 + 20.0) * 8) = Ceil(244) = 244
        // Bottom: Ceil((5.25 + 10.0) * 8) = Ceil(122) = 122
        // Width: 244 - 84 = 160
        // Height: 122 - 42 = 80
        logical_rect rect(10.5_lu, 5.25_lu, 20.0_lu, 10.0_lu);
        auto physical = metrics.snap_rect(rect);

        CHECK(physical.x.value == 84);
        CHECK(physical.y.value == 42);
        CHECK(physical.width.value == 160);
        CHECK(physical.height.value == 80);
    }
}

// ============================================================================
// DPI Scaling Tests
// ============================================================================

TEST_CASE("backend_metrics - DPI scaling") {
    SUBCASE("2x DPI scale") {
        auto metrics = make_gui_metrics<test_canvas_backend>(2.0);
        CHECK(metrics.dpi_scale == 2.0);

        logical_size size(10.0_lu, 5.0_lu);
        auto physical = metrics.snap_size(size);
        CHECK(physical.width.value == 160);  // 10 * 8 * 2
        CHECK(physical.height.value == 80);   // 5 * 8 * 2
    }

    SUBCASE("1.5x DPI scale") {
        auto metrics = make_gui_metrics<test_canvas_backend>(1.5);
        CHECK(metrics.dpi_scale == 1.5);

        logical_size size(10.0_lu, 10.0_lu);
        auto physical = metrics.snap_size(size);
        CHECK(physical.width.value == 120);  // 10 * 8 * 1.5 = 120
        CHECK(physical.height.value == 120);
    }

    SUBCASE("3x DPI scale (high-DPI)") {
        auto metrics = make_gui_metrics<test_canvas_backend>(3.0);

        logical_point point(10.0_lu, 10.0_lu);
        auto physical = metrics.snap_point(point);
        CHECK(physical.x.value == 240);  // 10 * 8 * 3
        CHECK(physical.y.value == 240);
    }
}

// ============================================================================
// Physical→Logical Conversion Tests
// ============================================================================

TEST_CASE("backend_metrics - Physical to logical conversion") {
    SUBCASE("Terminal backend (1:1)") {
        auto metrics = make_terminal_metrics<test_canvas_backend>();

        CHECK(metrics.physical_to_logical_x(physical_x(80)).value == 80.0);
        CHECK(metrics.physical_to_logical_y(physical_y(25)).value == 25.0);

        auto size = metrics.physical_to_logical_size(canvas_size(80, 25));
        CHECK(size.width.value == 80.0);
        CHECK(size.height.value == 25.0);
    }

    SUBCASE("GUI backend (8:1)") {
        auto metrics = make_gui_metrics<test_canvas_backend>();

        CHECK(metrics.physical_to_logical_x(physical_x(800)).value == 100.0);  // 800 / 8
        CHECK(metrics.physical_to_logical_y(physical_y(600)).value == 75.0);   // 600 / 8

        auto point = metrics.physical_to_logical_point(canvas_point(400, 300));
        CHECK(point.x.value == 50.0);  // 400 / 8
        CHECK(point.y.value == 37.5);  // 300 / 8

        auto rect = metrics.physical_to_logical_rect(canvas_rect(80, 40, 160, 80));
        CHECK(rect.x.value == 10.0);      // 80 / 8
        CHECK(rect.y.value == 5.0);       // 40 / 8
        CHECK(rect.width.value == 20.0);  // 160 / 8
        CHECK(rect.height.value == 10.0); // 80 / 8
    }

    SUBCASE("Round-trip conversion") {
        auto metrics = make_gui_metrics<test_canvas_backend>();

        // Logical → Physical → Logical (should preserve integer logical values)
        logical_size original(10.0_lu, 5.0_lu);
        auto physical = metrics.snap_size(original);
        // physical_size uses physical_x/physical_y, convert to backend size for round-trip
        auto backend_size = to_backend_size<test_canvas_backend>(physical);
        auto round_trip = metrics.physical_to_logical_size(backend_size);

        CHECK(round_trip.width.value == original.width.value);
        CHECK(round_trip.height.value == original.height.value);
    }
}

// ============================================================================
// Edge-Based Snapping Tests (Gap Prevention)
// ============================================================================

TEST_CASE("backend_metrics - Edge-based snapping prevents gaps") {
    auto metrics = make_gui_metrics<test_canvas_backend>();

    SUBCASE("Two adjacent rects should share edge") {
        // First rect: [0, 0, 10, 10]
        logical_rect rect1(0.0_lu, 0.0_lu, 10.0_lu, 10.0_lu);
        auto phys1 = metrics.snap_rect(rect1);

        // Second rect: [10, 0, 10, 10] (starts where first ends)
        logical_rect rect2(10.0_lu, 0.0_lu, 10.0_lu, 10.0_lu);
        auto phys2 = metrics.snap_rect(rect2);

        // Right edge of rect1 should equal left edge of rect2 (no gap!)
        CHECK(phys1.x + phys1.width == phys2.x);
    }

    SUBCASE("Fractional thirds should tile without gaps") {
        // Three rects of width 33.333... logical units each
        logical_unit third = 100.0_lu / 3;

        logical_rect rect1(0.0_lu, 0.0_lu, third, 10.0_lu);
        logical_rect rect2(third, 0.0_lu, third, 10.0_lu);
        logical_rect rect3(third * 2.0, 0.0_lu, third, 10.0_lu);

        auto phys1 = metrics.snap_rect(rect1);
        auto phys2 = metrics.snap_rect(rect2);
        auto phys3 = metrics.snap_rect(rect3);

        // Edge-based snapping causes small overlaps (not gaps!)
        // rect1: [0, 267), rect2: [266, 534), rect3: [533, 800)
        // This is expected: overlaps are better than gaps for visual quality
        CHECK(phys1.x.value == 0);
        CHECK(phys1.width.value == 267);  // Ceil(33.333*8) = 267
        CHECK(phys2.x.value == 266);  // Floor(33.333*8) = 266 (1px overlap with rect1)
        CHECK(phys2.width.value == 268);  // 534 - 266
        CHECK(phys3.x.value == 533);  // Floor(66.666*8) = 533 (1px overlap with rect2)
        CHECK(phys3.width.value == 267);  // 800 - 533

        // Last rect should end exactly at total width (no gap at right edge)
        CHECK((phys3.x + phys3.width).value == 800);
    }
}

// ============================================================================
// Practical Usage Tests
// ============================================================================

TEST_CASE("backend_metrics - Practical usage scenarios") {
    SUBCASE("Centering a window on terminal (80×25)") {
        auto metrics = make_terminal_metrics<test_canvas_backend>();

        logical_size screen_size(80.0_lu, 25.0_lu);
        logical_size window_size(60.0_lu, 20.0_lu);

        auto x = (screen_size.width - window_size.width) / 2;
        auto y = (screen_size.height - window_size.height) / 2;

        logical_rect window(x, y, window_size.width, window_size.height);
        auto physical = metrics.snap_rect(window);

        CHECK(physical.x.value == 10);  // (80 - 60) / 2 = 10
        CHECK(physical.y.value == 2);   // Floor((25 - 20) / 2) = Floor(2.5) = 2
    }

    SUBCASE("Centering a window on GUI (800×600 pixels)") {
        auto metrics = make_gui_metrics<test_canvas_backend>();

        // Screen size: 800×600 pixels = 100×75 logical units
        logical_size screen_size(100.0_lu, 75.0_lu);
        logical_size window_size(60.0_lu, 40.0_lu);

        auto x = (screen_size.width - window_size.width) / 2;
        auto y = (screen_size.height - window_size.height) / 2;

        logical_rect window(x, y, window_size.width, window_size.height);
        auto physical = metrics.snap_rect(window);

        CHECK(physical.x.value == 160);  // (100 - 60) / 2 * 8 = 20 * 8 = 160
        CHECK(physical.y.value == 140);  // (75 - 40) / 2 * 8 = 17.5 * 8 = 140
    }

    SUBCASE("Sub-pixel animation on GUI") {
        auto metrics = make_gui_metrics<test_canvas_backend>();

        // Animate from 10.0 to 10.5 logical units
        logical_unit start_x = 10.0_lu;
        logical_unit delta = 0.1_lu;

        for (int frame = 0; frame <= 5; ++frame) {
            logical_unit x = start_x + delta * static_cast<double>(frame);
            physical_x phys_x = metrics.snap_to_physical_x(x, snap_mode::floor);

            // Physical position should change smoothly (every frame or every few frames)
            // At 8 pixels/lu, 0.1 lu = 0.8 pixels, so position changes every ~2 frames
            if (frame == 0) CHECK(phys_x.value == 80);  // 10.0 * 8 = 80
            if (frame == 5) CHECK(phys_x.value == 84);  // Floor(10.5 * 8) = Floor(84) = 84
        }
    }
}
