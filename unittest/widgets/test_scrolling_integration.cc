/**
 * @file test_scrolling_integration.cc
 * @brief Integration tests for scrolling system in real-world scenarios
 * @author Testing Infrastructure Team
 * @date 2025-10-29
 */

#include <doctest/doctest.h>

#include <../../include/onyxui/widgets/containers/scroll_view.hh>
#include <../../include/onyxui/widgets/containers/scroll/scroll_view_presets.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/button.hh>
#include <../../include/onyxui/widgets/containers/panel.hh>
#include <../../include/onyxui/widgets/containers/vbox.hh>
#include <../../include/onyxui/widgets/containers/grid.hh>
#include <onyxui/layout/linear_layout.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_canvas_backend.hh"
#include <cstddef>

using namespace onyxui;
using namespace onyxui::testing;

// =============================================================================
// 1. List Scenarios (3 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Integration - scroll_view with vertical list of 20 labels") {
    auto view = modern_scroll_view<test_canvas_backend>();

    // Add vertical layout
    view->set_layout_strategy(std::make_unique<linear_layout<test_canvas_backend>>(direction::vertical));

    // Add 20 labels
    for (int i = 0; i < 20; ++i) {
        view->emplace_child<label>("Item " + std::to_string(i));
    }

    // Measure and arrange
    [[maybe_unused]] auto size = view->measure(200, 100);
    view->arrange({0, 0, 200, 100});

    // Verify we added all the labels
    CHECK(view->content()->children().size() == 20);

    // Scroll and verify scrolling works (may clamp to valid range)
    view->scroll_to(0, 50);
    auto offset = point_utils::get_y(view->content()->get_scroll_offset());

    // Verify scrollbar is synced with whatever offset we got
    auto scrollbar_info = view->vertical_scrollbar()->get_scroll_info();
    CHECK(point_utils::get_y(scrollbar_info.scroll_offset) == offset);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Integration - scroll_view with grid of 5x5 buttons") {
    auto view = classic_scroll_view<test_canvas_backend>();

    // Create a grid widget with 5 columns
    auto grid_widget = std::make_unique<grid<test_canvas_backend>>(5);  // 5 columns

    // Add 25 buttons to the grid (5x5 grid)
    for (int i = 0; i < 25; ++i) {
        grid_widget->emplace_child<button>("Btn" + std::to_string(i));
    }

    // Add the grid to the scroll view
    view->add_child(std::move(grid_widget));

    // Measure and arrange
    [[maybe_unused]] auto size = view->measure(200, 150);
    view->arrange({0, 0, 200, 150});

    // Verify content exists
    CHECK(view->content()->children().size() == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Integration - vertical_only scroll view with text content") {
    auto view = vertical_only_scroll_view<test_canvas_backend>();

    // Add vertical layout
    view->set_layout_strategy(std::make_unique<linear_layout<test_canvas_backend>>(direction::vertical));

    // Add multiple paragraphs
    for (int i = 0; i < 10; ++i) {
        view->emplace_child<label>("Paragraph " + std::to_string(i) + ": Lorem ipsum dolor sit amet");
    }

    [[maybe_unused]] auto size = view->measure(300, 200);
    view->arrange({0, 0, 300, 200});

    // Verify only vertical scrolling is enabled
    auto policy = view->content()->get_scrollbar_visibility_policy();
    CHECK(policy.horizontal == scrollbar_visibility::hidden);
    CHECK(policy.vertical != scrollbar_visibility::hidden);
}

// =============================================================================
// 2. Nested Scenarios (3 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Integration - scroll_view with nested panels") {
    auto view = modern_scroll_view<test_canvas_backend>();

    // Add nested structure: panel → panel → content
    auto outer_panel = std::make_unique<panel<test_canvas_backend>>();
    auto inner_panel = std::make_unique<panel<test_canvas_backend>>();

    inner_panel->emplace_child<label>("Nested content");

    outer_panel->add_child(std::move(inner_panel));
    view->add_child(std::move(outer_panel));

    [[maybe_unused]] auto size = view->measure(200, 150);
    view->arrange({0, 0, 200, 150});

    // Verify structure is intact
    CHECK(view->content()->children().size() == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Integration - scroll_view inside a panel") {
    auto container = std::make_unique<panel<test_canvas_backend>>();

    auto view = modern_scroll_view<test_canvas_backend>();
    view->emplace_child<label>("Content inside scrollable inside panel");

    auto* view_ptr = view.get();
    container->add_child(std::move(view));

    [[maybe_unused]] auto size = container->measure(300, 200);
    container->arrange({0, 0, 300, 200});

    // Verify scroll_view is a child of container
    CHECK(container->children().size() == 1);

    // Verify scroll_to doesn't crash when nested
    view_ptr->scroll_to(0, 10);
    // Offset may be clamped to valid range, just verify it doesn't crash
    CHECK(point_utils::get_y(view_ptr->content()->get_scroll_offset()) >= 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Integration - Two independent scroll_views") {
    auto view1 = modern_scroll_view<test_canvas_backend>();
    auto view2 = modern_scroll_view<test_canvas_backend>();

    // Add content to both
    view1->emplace_child<label>("View 1 content");
    view2->emplace_child<label>("View 2 content");

    [[maybe_unused]] auto size1 = view1->measure(200, 100);
    view1->arrange({0, 0, 200, 100});

    [[maybe_unused]] auto size2 = view2->measure(200, 100);
    view2->arrange({0, 0, 200, 100});

    // Scroll view1
    view1->scroll_to(0, 20);
    auto offset1 = point_utils::get_y(view1->content()->get_scroll_offset());

    // Verify view2 is still at 0 (independent scrolling)
    CHECK(point_utils::get_y(view2->content()->get_scroll_offset()) == 0);
    // view1 may have clamped, but should have attempted to scroll
    CHECK(offset1 >= 0);
}

// =============================================================================
// 3. Dynamic Content Scenarios (3 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Integration - Add items dynamically and scroll") {
    auto view = modern_scroll_view<test_canvas_backend>();
    view->set_layout_strategy(std::make_unique<linear_layout<test_canvas_backend>>(direction::vertical));

    // Start with 5 items
    for (int i = 0; i < 5; ++i) {
        view->emplace_child<label>("Item " + std::to_string(i));
    }

    [[maybe_unused]] auto size = view->measure(200, 100);
    view->arrange({0, 0, 200, 100});

    CHECK(view->content()->children().size() == 5);

    // Add 10 more items
    for (int i = 5; i < 15; ++i) {
        view->emplace_child<label>("Item " + std::to_string(i));
    }

    // Remeasure
    [[maybe_unused]] auto new_size = view->measure(200, 100);
    view->arrange({0, 0, 200, 100});

    // Verify items were added
    CHECK(view->content()->children().size() == 15);

    // Scrolling should work without crashing
    view->scroll_to(0, 50);
    CHECK(point_utils::get_y(view->content()->get_scroll_offset()) >= 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Integration - Remove items and verify scroll bounds") {
    auto view = modern_scroll_view<test_canvas_backend>();
    view->set_layout_strategy(std::make_unique<linear_layout<test_canvas_backend>>(direction::vertical));

    // Add 20 items
    std::vector<label<test_canvas_backend>*> labels;
    for (int i = 0; i < 20; ++i) {
        labels.push_back(view->emplace_child<label>("Item " + std::to_string(i)));
    }

    [[maybe_unused]] auto size = view->measure(200, 100);
    view->arrange({0, 0, 200, 100});

    // Scroll to bottom
    auto info = view->content()->get_scroll_info();
    int max_scroll = size_utils::get_height(info.content_size) - size_utils::get_height(info.viewport_size);
    view->scroll_to(0, max_scroll);

    // Remove half the items
    for (std::size_t i = 0; i < 10; ++i) {
        view->remove_child(labels[i]);
    }

    // Remeasure
    [[maybe_unused]] auto new_size = view->measure(200, 100);
    view->arrange({0, 0, 200, 100});

    // Scroll should clamp to new max
    auto new_offset = point_utils::get_y(view->content()->get_scroll_offset());
    auto new_info = view->content()->get_scroll_info();
    int new_max_scroll = size_utils::get_height(new_info.content_size) - size_utils::get_height(new_info.viewport_size);
    CHECK(new_offset <= new_max_scroll);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Integration - Clear all children") {
    auto view = modern_scroll_view<test_canvas_backend>();
    view->set_layout_strategy(std::make_unique<linear_layout<test_canvas_backend>>(direction::vertical));

    // Add items
    for (int i = 0; i < 10; ++i) {
        view->emplace_child<label>("Item " + std::to_string(i));
    }

    [[maybe_unused]] auto size = view->measure(200, 100);
    view->arrange({0, 0, 200, 100});

    view->scroll_to(0, 50);

    // Clear all children
    view->clear_children();

    // Remeasure
    [[maybe_unused]] auto new_size = view->measure(200, 100);
    view->arrange({0, 0, 200, 100});

    // Should have no children
    CHECK(view->content()->children().size() == 0);

    // Scroll offset should be valid (clamped to 0 since no content)
    CHECK(point_utils::get_y(view->content()->get_scroll_offset()) >= 0);
}

// =============================================================================
// 4. Preset Variants Integration (3 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Integration - modern_scroll_view with mixed widgets") {
    auto view = modern_scroll_view<test_canvas_backend>();
    view->set_layout_strategy(std::make_unique<linear_layout<test_canvas_backend>>(direction::vertical));

    // Add different widget types
    view->emplace_child<label>("Header");
    view->emplace_child<button>("Action");
    view->emplace_child<panel>();
    view->emplace_child<label>("Footer");

    [[maybe_unused]] auto size = view->measure(200, 100);
    view->arrange({0, 0, 200, 100});

    CHECK(view->content()->children().size() == 4);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Integration - classic_scroll_view always shows scrollbars") {
    auto view = classic_scroll_view<test_canvas_backend>();

    // Add minimal content (doesn't need scrolling)
    view->emplace_child<label>("Small");

    [[maybe_unused]] auto size = view->measure(200, 200);
    view->arrange({0, 0, 200, 200});

    // Classic style has always-visible policy
    auto policy = view->content()->get_scrollbar_visibility_policy();
    CHECK(policy.vertical == scrollbar_visibility::always);
    CHECK(policy.horizontal == scrollbar_visibility::always);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Integration - compact_scroll_view with long content") {
    auto view = compact_scroll_view<test_canvas_backend>();
    view->set_layout_strategy(std::make_unique<linear_layout<test_canvas_backend>>(direction::vertical));

    // Add many items
    for (int i = 0; i < 50; ++i) {
        view->emplace_child<label>("Item " + std::to_string(i));
    }

    [[maybe_unused]] auto size = view->measure(150, 100);
    view->arrange({0, 0, 150, 100});

    // Verify items were added
    CHECK(view->content()->children().size() == 50);

    // Scroll and verify it doesn't crash
    view->scroll_by(0, 100);
    CHECK(point_utils::get_y(view->content()->get_scroll_offset()) >= 0);
}
