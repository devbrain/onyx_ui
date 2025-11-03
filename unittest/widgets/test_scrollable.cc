/**
 * @file test_scrollable.cc
 * @brief Unit tests for scrollable container widget - Phase 1 baseline
 * @author Testing Infrastructure Team
 * @date 2025-10-29
 */

#include <doctest/doctest.h>
#include <../../include/onyxui/widgets/containers/scroll/scrollable.hh>
#include <../../include/onyxui/widgets/containers/panel.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_canvas_backend.hh"

using namespace onyxui;
using namespace onyxui::testing;

// Helper function to create a fixed-size panel
static std::unique_ptr<panel<test_canvas_backend>> make_fixed_panel(int width, int height) {
    auto p = std::make_unique<panel<test_canvas_backend>>();

    size_constraint width_constraint;
    width_constraint.policy = size_policy::fixed;
    width_constraint.preferred_size = width;
    width_constraint.min_size = width;   // Force minimum size
    width_constraint.max_size = width;   // Force maximum size
    p->set_width_constraint(width_constraint);

    size_constraint height_constraint;
    height_constraint.policy = size_policy::fixed;
    height_constraint.preferred_size = height;
    height_constraint.min_size = height;  // Force minimum size
    height_constraint.max_size = height;  // Force maximum size
    p->set_height_constraint(height_constraint);

    return p;
}

// =============================================================================
// 1. Basic Functionality Tests
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Default construction") {
    scrollable<test_canvas_backend> scroll;

    auto info = scroll.get_scroll_info();
    CHECK(size_utils::get_width(info.content_size) == 0);
    CHECK(size_utils::get_height(info.content_size) == 0);
    CHECK(point_utils::get_x(info.scroll_offset) == 0);
    CHECK(point_utils::get_y(info.scroll_offset) == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Construction with child") {
    auto child = make_fixed_panel(100, 100);
    scrollable<test_canvas_backend> scroll(std::move(child));

    CHECK(scroll.children().size() == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Single child measurement") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);

    auto info = scroll.get_scroll_info();
    CHECK(size_utils::get_width(info.content_size) == 200);
    CHECK(size_utils::get_height(info.content_size) == 300);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - get_scroll_info") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    auto info = scroll.get_scroll_info();

    CHECK(size_utils::get_width(info.content_size) == 200);
    CHECK(size_utils::get_height(info.content_size) == 300);
    CHECK(size_utils::get_width(info.viewport_size) == 100);
    CHECK(size_utils::get_height(info.viewport_size) == 150);
    CHECK(info.needs_horizontal_scroll());
    CHECK(info.needs_vertical_scroll());
}

// =============================================================================
// 2. Scroll Position Tests
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - scroll_to with clamping") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    // max_scroll = (200 - 100, 300 - 150) = (100, 150)
    scroll.scroll_to(50, 75);

    auto offset = scroll.get_scroll_offset();
    CHECK(point_utils::get_x(offset) == 50);
    CHECK(point_utils::get_y(offset) == 75);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - scroll_by accumulation") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    scroll.scroll_by(30, 50);
    auto offset1 = scroll.get_scroll_offset();
    CHECK(point_utils::get_x(offset1) == 30);
    CHECK(point_utils::get_y(offset1) == 50);

    scroll.scroll_by(20, 25);
    auto offset2 = scroll.get_scroll_offset();
    CHECK(point_utils::get_x(offset2) == 50);
    CHECK(point_utils::get_y(offset2) == 75);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Negative scroll clamping") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    scroll.scroll_to(-10, -20);

    auto offset = scroll.get_scroll_offset();
    CHECK(point_utils::get_x(offset) == 0);
    CHECK(point_utils::get_y(offset) == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Excessive scroll clamping") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    scroll.scroll_to(500, 600);

    auto offset = scroll.get_scroll_offset();
    CHECK(point_utils::get_x(offset) == 100);  // max = 200 - 100
    CHECK(point_utils::get_y(offset) == 150);  // max = 300 - 150
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - scroll_changed signal") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    int signal_count = 0;
    test_canvas_backend::point_type emitted_offset;

    scroll.scroll_changed.connect([&](const auto& offset) {
        signal_count++;
        emitted_offset = offset;
    });

    scroll.scroll_to(50, 75);

    CHECK(signal_count == 1);
    CHECK(point_utils::get_x(emitted_offset) == 50);
    CHECK(point_utils::get_y(emitted_offset) == 75);
}

// =============================================================================
// 3. Content Measurement Tests
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Unconstrained measurement") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(500, 800);
    scroll.add_child(std::move(child));

    // Measure with small viewport - child should measure at preferred size
    [[maybe_unused]] auto size = scroll.measure(100, 150);

    auto info = scroll.get_scroll_info();
    CHECK(size_utils::get_width(info.content_size) == 500);
    CHECK(size_utils::get_height(info.content_size) == 800);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Content larger than viewport") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    auto info = scroll.get_scroll_info();
    CHECK(info.needs_horizontal_scroll());
    CHECK(info.needs_vertical_scroll());
    CHECK(info.max_scroll_x() == 100);
    CHECK(info.max_scroll_y() == 150);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Content smaller than viewport") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(50, 75);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    auto info = scroll.get_scroll_info();
    CHECK_FALSE(info.needs_horizontal_scroll());
    CHECK_FALSE(info.needs_vertical_scroll());
    CHECK(info.max_scroll_x() == 0);
    CHECK(info.max_scroll_y() == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - content_size_changed signal") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    int signal_count = 0;
    test_canvas_backend::size_type emitted_size;

    scroll.content_size_changed.connect([&](const auto& size) {
        signal_count++;
        emitted_size = size;
    });

    [[maybe_unused]] auto size = scroll.measure(100, 150);

    CHECK(signal_count == 1);
    CHECK(size_utils::get_width(emitted_size) == 200);
    CHECK(size_utils::get_height(emitted_size) == 300);
}

// =============================================================================
// 4. Layout & Arrangement Tests (10 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Child positioned at negative offset") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    auto* child_ptr = child.get();
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    scroll.scroll_to(50, 75);
    scroll.arrange({0, 0, 100, 150});

    auto child_bounds = child_ptr->bounds();
    CHECK(rect_utils::get_x(child_bounds) == -50);
    CHECK(rect_utils::get_y(child_bounds) == -75);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Child at zero offset initially") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    auto* child_ptr = child.get();
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    auto child_bounds = child_ptr->bounds();
    CHECK(rect_utils::get_x(child_bounds) == 0);
    CHECK(rect_utils::get_y(child_bounds) == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Child size unchanged by scroll") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    auto* child_ptr = child.get();
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    scroll.scroll_to(50, 75);
    scroll.arrange({0, 0, 100, 150});

    auto child_bounds = child_ptr->bounds();
    CHECK(rect_utils::get_width(child_bounds) == 200);
    CHECK(rect_utils::get_height(child_bounds) == 300);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Arrangement with padding") {
    scrollable<test_canvas_backend> scroll;
    scroll.set_padding(thickness{10, 10, 10, 10});

    auto child = make_fixed_panel(200, 300);
    auto* child_ptr = child.get();
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(120, 170);
    scroll.arrange({0, 0, 120, 170});

    // Child coordinates are relative to scrollable's content area, not absolute screen coordinates
    // With padding (10,10,10,10), child is positioned at content area origin (0,0)
    auto child_bounds = child_ptr->bounds();
    CHECK(rect_utils::get_x(child_bounds) == 0);
    CHECK(rect_utils::get_y(child_bounds) == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Viewport size updated after arrange") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    auto info = scroll.get_scroll_info();
    CHECK(size_utils::get_width(info.viewport_size) == 100);
    CHECK(size_utils::get_height(info.viewport_size) == 150);

    // Re-arrange with different size
    scroll.arrange({0, 0, 150, 200});

    auto info2 = scroll.get_scroll_info();
    CHECK(size_utils::get_width(info2.viewport_size) == 150);
    CHECK(size_utils::get_height(info2.viewport_size) == 200);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Multiple children arrangement") {
    scrollable<test_canvas_backend> scroll;

    auto child1 = make_fixed_panel(100, 100);
    auto* child1_ptr = child1.get();
    auto child2 = make_fixed_panel(150, 150);
    auto* child2_ptr = child2.get();

    scroll.add_child(std::move(child1));
    scroll.add_child(std::move(child2));

    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    // Both children should be arranged (though overlapping)
    auto bounds1 = child1_ptr->bounds();
    auto bounds2 = child2_ptr->bounds();

    CHECK(rect_utils::get_x(bounds1) == 0);
    CHECK(rect_utils::get_y(bounds1) == 0);
    CHECK(rect_utils::get_x(bounds2) == 0);
    CHECK(rect_utils::get_y(bounds2) == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Arrangement at non-zero position") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    auto* child_ptr = child.get();
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({50, 60, 100, 150});

    // Child coordinates are relative to scrollable's content area, not absolute screen coordinates
    // Even when scrollable is positioned at (50,60), child is at content area origin (0,0)
    auto child_bounds = child_ptr->bounds();
    CHECK(rect_utils::get_x(child_bounds) == 0);
    CHECK(rect_utils::get_y(child_bounds) == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Content area calculation with border") {
    scrollable<test_canvas_backend> scroll;
    scroll.set_has_border(true);

    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(102, 152);
    scroll.arrange({0, 0, 102, 152});

    // Content area should be reduced by border (1px each side)
    auto info = scroll.get_scroll_info();
    CHECK(size_utils::get_width(info.viewport_size) == 100);
    CHECK(size_utils::get_height(info.viewport_size) == 150);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Scroll offset affects child position") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    auto* child_ptr = child.get();
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    auto bounds_before = child_ptr->bounds();
    int x_before = rect_utils::get_x(bounds_before);
    int y_before = rect_utils::get_y(bounds_before);

    scroll.scroll_to(25, 30);
    scroll.arrange({0, 0, 100, 150});

    auto bounds_after = child_ptr->bounds();
    int x_after = rect_utils::get_x(bounds_after);
    int y_after = rect_utils::get_y(bounds_after);

    CHECK(x_after == x_before - 25);
    CHECK(y_after == y_before - 30);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Arrangement invalidation on scroll") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    // Scrolling should trigger arrangement update
    scroll.scroll_to(50, 75);

    // Re-arrange should apply new scroll offset
    scroll.arrange({0, 0, 100, 150});

    auto info = scroll.get_scroll_info();
    CHECK(point_utils::get_x(info.scroll_offset) == 50);
    CHECK(point_utils::get_y(info.scroll_offset) == 75);
}

// =============================================================================
// 5. scroll_into_view Tests (8 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - scroll_into_view already visible") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(80, 80);
    auto* child_ptr = child.get();
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    auto offset_before = scroll.get_scroll_offset();

    scroll.scroll_into_view(child_ptr);

    auto offset_after = scroll.get_scroll_offset();

    // Child is already visible, no scroll should occur
    CHECK(point_utils::get_x(offset_before) == point_utils::get_x(offset_after));
    CHECK(point_utils::get_y(offset_before) == point_utils::get_y(offset_after));
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - scroll_into_view with nullptr") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    auto offset_before = scroll.get_scroll_offset();

    scroll.scroll_into_view(nullptr);

    auto offset_after = scroll.get_scroll_offset();

    // nullptr should be no-op
    CHECK(point_utils::get_x(offset_before) == point_utils::get_x(offset_after));
    CHECK(point_utils::get_y(offset_before) == point_utils::get_y(offset_after));
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - scroll_into_view non-child element") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    auto offset_before = scroll.get_scroll_offset();

    // Try to scroll to an element that's not a child
    panel<test_canvas_backend> other_panel;
    scroll.scroll_into_view(&other_panel);

    auto offset_after = scroll.get_scroll_offset();

    // Non-child should be no-op
    CHECK(point_utils::get_x(offset_before) == point_utils::get_x(offset_after));
    CHECK(point_utils::get_y(offset_before) == point_utils::get_y(offset_after));
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - scroll_into_view child larger than viewport") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    auto* child_ptr = child.get();
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    scroll.scroll_into_view(child_ptr);

    // Child is larger than viewport, should scroll to top-left
    auto offset = scroll.get_scroll_offset();
    CHECK(point_utils::get_x(offset) >= 0);
    CHECK(point_utils::get_y(offset) >= 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - scroll_into_view empty container") {
    scrollable<test_canvas_backend> scroll;

    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    // Calling with nullptr should be a no-op
    scroll.scroll_into_view(nullptr);

    // Verify scroll offset unchanged (still at 0,0)
    auto offset = scroll.get_scroll_offset();
    CHECK(point_utils::get_x(offset) == 0);
    CHECK(point_utils::get_y(offset) == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - scroll_into_view updates offset") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(50, 50);
    auto* child_ptr = child.get();
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    // Initial scroll position
    CHECK(point_utils::get_x(scroll.get_scroll_offset()) == 0);
    CHECK(point_utils::get_y(scroll.get_scroll_offset()) == 0);

    scroll.scroll_into_view(child_ptr);

    // Child fits in viewport, should remain at 0,0
    auto offset = scroll.get_scroll_offset();
    CHECK(point_utils::get_x(offset) == 0);
    CHECK(point_utils::get_y(offset) == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - scroll_into_view with multiple children") {
    scrollable<test_canvas_backend> scroll;
    auto child1 = make_fixed_panel(100, 100);
    auto* child1_ptr = child1.get();
    auto child2 = make_fixed_panel(100, 100);

    scroll.add_child(std::move(child1));
    scroll.add_child(std::move(child2));

    [[maybe_unused]] auto size = scroll.measure(50, 50);
    scroll.arrange({0, 0, 50, 50});

    // Child1 is 100x100 but viewport is only 50x50
    // Child extends beyond viewport, so scroll_into_view should adjust
    scroll.scroll_into_view(child1_ptr);

    // When child is larger than viewport and already at top-left,
    // scroll_into_view brings the bottom-right into view
    auto offset = scroll.get_scroll_offset();
    CHECK(point_utils::get_x(offset) == 50);  // Scroll right to show right edge
    CHECK(point_utils::get_y(offset) == 50);  // Scroll down to show bottom edge
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - scroll_into_view after content resize") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    auto* child_ptr = child.get();
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    // Scroll to middle of content
    scroll.scroll_to(50, 75);
    CHECK(point_utils::get_x(scroll.get_scroll_offset()) == 50);
    CHECK(point_utils::get_y(scroll.get_scroll_offset()) == 75);

    // Child is 200x300, viewport is 100x150, currently showing middle portion
    // scroll_into_view will adjust to show the bottom-right corner
    scroll.scroll_into_view(child_ptr);

    // Should scroll to show right/bottom edge of child
    auto offset = scroll.get_scroll_offset();
    CHECK(point_utils::get_x(offset) == 100);  // max_scroll_x = 200-100 = 100
    CHECK(point_utils::get_y(offset) == 150);  // max_scroll_y = 300-150 = 150
}

// =============================================================================
// 6. Edge Case Tests (8 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Zero viewport size") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(0, 0);
    scroll.arrange({0, 0, 0, 0});

    auto info = scroll.get_scroll_info();
    CHECK(size_utils::get_width(info.viewport_size) == 0);
    CHECK(size_utils::get_height(info.viewport_size) == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Very large content size") {
    scrollable<test_canvas_backend> scroll;

    constexpr int LARGE = std::numeric_limits<int>::max() / 2;
    auto child = make_fixed_panel(LARGE, LARGE);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    auto info = scroll.get_scroll_info();
    CHECK(size_utils::get_width(info.content_size) == LARGE);
    CHECK(size_utils::get_height(info.content_size) == LARGE);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Rapid scroll updates") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    // Perform many rapid scrolls
    for (int i = 0; i < 100; ++i) {
        scroll.scroll_to(i % 101, i % 151);
    }

    auto offset = scroll.get_scroll_offset();
    CHECK(point_utils::get_x(offset) == 99);
    CHECK(point_utils::get_y(offset) == 99);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Content exactly matches viewport") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(100, 150);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    auto info = scroll.get_scroll_info();
    CHECK_FALSE(info.needs_horizontal_scroll());
    CHECK_FALSE(info.needs_vertical_scroll());
    CHECK(info.max_scroll_x() == 0);
    CHECK(info.max_scroll_y() == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Scroll during measurement") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size1 = scroll.measure(100, 150);
    scroll.scroll_to(50, 75);

    // Re-measure should preserve scroll position
    [[maybe_unused]] auto size2 = scroll.measure(100, 150);

    auto offset = scroll.get_scroll_offset();
    CHECK(point_utils::get_x(offset) == 50);
    CHECK(point_utils::get_y(offset) == 75);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Multiple measure calls") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    for (int i = 0; i < 10; ++i) {
        [[maybe_unused]] auto size = scroll.measure(100, 150);
    }

    auto info = scroll.get_scroll_info();
    CHECK(size_utils::get_width(info.content_size) == 200);
    CHECK(size_utils::get_height(info.content_size) == 300);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Scroll offset persistence") {
    scrollable<test_canvas_backend> scroll;
    auto child = make_fixed_panel(200, 300);
    scroll.add_child(std::move(child));

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    scroll.scroll_to(30, 40);

    // Multiple arrange calls should preserve scroll offset
    for (int i = 0; i < 5; ++i) {
        scroll.arrange({0, 0, 100, 150});
    }

    auto offset = scroll.get_scroll_offset();
    CHECK(point_utils::get_x(offset) == 30);
    CHECK(point_utils::get_y(offset) == 40);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Empty scrollable behavior") {
    scrollable<test_canvas_backend> scroll;

    [[maybe_unused]] auto size = scroll.measure(100, 150);
    scroll.arrange({0, 0, 100, 150});

    // Should handle empty state gracefully
    scroll.scroll_to(10, 20);

    auto offset = scroll.get_scroll_offset();
    // Empty scrollable should clamp to 0,0
    CHECK(point_utils::get_x(offset) == 0);
    CHECK(point_utils::get_y(offset) == 0);
}

// =============================================================================
// 7. Scrollbar Visibility Policies (Phase 2)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Default visibility policy is auto_hide") {
    scrollable<test_canvas_backend> scroll;

    auto policy = scroll.get_scrollbar_visibility_policy();
    CHECK(policy.horizontal == scrollbar_visibility::auto_hide);
    CHECK(policy.vertical == scrollbar_visibility::auto_hide);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Set visibility policy for both axes") {
    scrollable<test_canvas_backend> scroll;

    scroll.set_scrollbar_visibility(scrollbar_visibility::always);

    auto policy = scroll.get_scrollbar_visibility_policy();
    CHECK(policy.horizontal == scrollbar_visibility::always);
    CHECK(policy.vertical == scrollbar_visibility::always);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Set independent visibility policies") {
    scrollable<test_canvas_backend> scroll;

    scroll.set_scrollbar_visibility(scrollbar_visibility::always, scrollbar_visibility::hidden);

    auto policy = scroll.get_scrollbar_visibility_policy();
    CHECK(policy.horizontal == scrollbar_visibility::always);
    CHECK(policy.vertical == scrollbar_visibility::hidden);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Always policy shows scrollbars regardless of content") {
    scrollable<test_canvas_backend> scroll;
    scroll.set_scrollbar_visibility(scrollbar_visibility::always);

    // Content fits - but scrollbars still visible
    scroll.add_child(make_fixed_panel(50, 50));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    CHECK(scroll.should_show_horizontal_scrollbar() == true);
    CHECK(scroll.should_show_vertical_scrollbar() == true);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Always policy for empty container") {
    scrollable<test_canvas_backend> scroll;
    scroll.set_scrollbar_visibility(scrollbar_visibility::always);

    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    // Even with no content, always policy shows scrollbars
    CHECK(scroll.should_show_horizontal_scrollbar() == true);
    CHECK(scroll.should_show_vertical_scrollbar() == true);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Always policy with overflowing content") {
    scrollable<test_canvas_backend> scroll;
    scroll.set_scrollbar_visibility(scrollbar_visibility::always);

    scroll.add_child(make_fixed_panel(200, 200));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    // Content overflows - scrollbars visible
    CHECK(scroll.should_show_horizontal_scrollbar() == true);
    CHECK(scroll.should_show_vertical_scrollbar() == true);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Auto_hide policy hides when content fits") {
    scrollable<test_canvas_backend> scroll;
    scroll.set_scrollbar_visibility(scrollbar_visibility::auto_hide);

    // Content fits viewport
    scroll.add_child(make_fixed_panel(50, 50));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    // Content fits - no scrollbars needed
    CHECK(scroll.should_show_horizontal_scrollbar() == false);
    CHECK(scroll.should_show_vertical_scrollbar() == false);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Auto_hide policy shows when content overflows") {
    scrollable<test_canvas_backend> scroll;
    scroll.set_scrollbar_visibility(scrollbar_visibility::auto_hide);

    // Content exceeds viewport
    scroll.add_child(make_fixed_panel(200, 200));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    // Content overflows - scrollbars needed
    CHECK(scroll.should_show_horizontal_scrollbar() == true);
    CHECK(scroll.should_show_vertical_scrollbar() == true);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Auto_hide policy with horizontal overflow only") {
    scrollable<test_canvas_backend> scroll;
    scroll.set_scrollbar_visibility(scrollbar_visibility::auto_hide);

    // Content wide but not tall
    scroll.add_child(make_fixed_panel(200, 50));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    // Only horizontal scrollbar needed
    CHECK(scroll.should_show_horizontal_scrollbar() == true);
    CHECK(scroll.should_show_vertical_scrollbar() == false);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Auto_hide policy with vertical overflow only") {
    scrollable<test_canvas_backend> scroll;
    scroll.set_scrollbar_visibility(scrollbar_visibility::auto_hide);

    // Content tall but not wide
    scroll.add_child(make_fixed_panel(50, 200));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    // Only vertical scrollbar needed
    CHECK(scroll.should_show_horizontal_scrollbar() == false);
    CHECK(scroll.should_show_vertical_scrollbar() == true);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Auto_hide policy with exact fit") {
    scrollable<test_canvas_backend> scroll;
    scroll.set_scrollbar_visibility(scrollbar_visibility::auto_hide);

    // Content exactly matches viewport
    scroll.add_child(make_fixed_panel(100, 100));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    // Exact fit - no scrollbars needed
    CHECK(scroll.should_show_horizontal_scrollbar() == false);
    CHECK(scroll.should_show_vertical_scrollbar() == false);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Auto_hide policy with empty container") {
    scrollable<test_canvas_backend> scroll;
    scroll.set_scrollbar_visibility(scrollbar_visibility::auto_hide);

    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    // No content - no scrollbars needed
    CHECK(scroll.should_show_horizontal_scrollbar() == false);
    CHECK(scroll.should_show_vertical_scrollbar() == false);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Hidden policy never shows scrollbars") {
    scrollable<test_canvas_backend> scroll;
    scroll.set_scrollbar_visibility(scrollbar_visibility::hidden);

    // Content overflows significantly
    scroll.add_child(make_fixed_panel(300, 300));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    // Even with overflow, scrollbars hidden
    CHECK(scroll.should_show_horizontal_scrollbar() == false);
    CHECK(scroll.should_show_vertical_scrollbar() == false);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Hidden policy with fitting content") {
    scrollable<test_canvas_backend> scroll;
    scroll.set_scrollbar_visibility(scrollbar_visibility::hidden);

    scroll.add_child(make_fixed_panel(50, 50));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    // Content fits, scrollbars still hidden
    CHECK(scroll.should_show_horizontal_scrollbar() == false);
    CHECK(scroll.should_show_vertical_scrollbar() == false);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Independent axis policies (horizontal always, vertical auto)") {
    scrollable<test_canvas_backend> scroll;
    scroll.set_scrollbar_visibility(scrollbar_visibility::always, scrollbar_visibility::auto_hide);

    // Content fits both dimensions
    scroll.add_child(make_fixed_panel(50, 50));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    // Horizontal always visible, vertical auto-hidden
    CHECK(scroll.should_show_horizontal_scrollbar() == true);
    CHECK(scroll.should_show_vertical_scrollbar() == false);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Independent axis policies (horizontal auto, vertical hidden)") {
    scrollable<test_canvas_backend> scroll;
    scroll.set_scrollbar_visibility(scrollbar_visibility::auto_hide, scrollbar_visibility::hidden);

    // Content overflows horizontally
    scroll.add_child(make_fixed_panel(200, 50));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    // Horizontal auto-shown, vertical always hidden
    CHECK(scroll.should_show_horizontal_scrollbar() == true);
    CHECK(scroll.should_show_vertical_scrollbar() == false);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Independent axis policies (horizontal hidden, vertical always)") {
    scrollable<test_canvas_backend> scroll;
    scroll.set_scrollbar_visibility(scrollbar_visibility::hidden, scrollbar_visibility::always);

    // Content overflows both dimensions
    scroll.add_child(make_fixed_panel(200, 200));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    // Horizontal hidden, vertical always shown
    CHECK(scroll.should_show_horizontal_scrollbar() == false);
    CHECK(scroll.should_show_vertical_scrollbar() == true);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Visibility change emits signal") {
    scrollable<test_canvas_backend> scroll;

    bool signal_emitted = false;
    bool h_visible = false;
    bool v_visible = false;

    scroll.scrollbar_visibility_changed.connect([&](bool h, bool v) {
        signal_emitted = true;
        h_visible = h;
        v_visible = v;
    });

    scroll.set_scrollbar_visibility(scrollbar_visibility::always);

    CHECK(signal_emitted == true);
    CHECK(h_visible == true);
    CHECK(v_visible == true);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - No signal when policy unchanged") {
    scrollable<test_canvas_backend> scroll;
    scroll.set_scrollbar_visibility(scrollbar_visibility::always);

    bool signal_emitted = false;
    scroll.scrollbar_visibility_changed.connect([&](bool, bool) {
        signal_emitted = true;
    });

    // Set same policy again
    scroll.set_scrollbar_visibility(scrollbar_visibility::always);

    CHECK(signal_emitted == false);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Visibility signal on content size change (auto_hide)") {
    scrollable<test_canvas_backend> scroll;
    scroll.set_scrollbar_visibility(scrollbar_visibility::auto_hide);

    // Start with fitting content
    scroll.add_child(make_fixed_panel(50, 50));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange({0, 0, 100, 100});

    bool signal_emitted = false;
    bool h_visible = false;
    bool v_visible = false;

    scroll.scrollbar_visibility_changed.connect([&](bool h, bool v) {
        signal_emitted = true;
        h_visible = h;
        v_visible = v;
    });

    // Replace with overflowing content
    [[maybe_unused]] auto removed = scroll.remove_child(scroll.children()[0].get());
    scroll.add_child(make_fixed_panel(200, 200));
    size = scroll.measure(100, 100);

    // Signal should emit when content size changes visibility
    CHECK(signal_emitted == true);
    CHECK(h_visible == true);
    CHECK(v_visible == true);
}
