/**
 * @file test_scroll_view.cc
 * @brief Unit tests for scroll_view convenience wrapper
 * @author Testing Infrastructure Team
 * @date 2025-10-29
 */

#include <doctest/doctest.h>

#include <../../include/onyxui/widgets/containers/scroll_view.hh>
#include <onyxui/widgets/label.hh>
#include <../../include/onyxui/widgets/containers/panel.hh>
#include <onyxui/layout/linear_layout.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_canvas_backend.hh"

using namespace onyxui;
using namespace onyxui::testing;

// Helper to create fixed-size panel for testing
template<UIBackend Backend>
std::unique_ptr<panel<Backend>> make_fixed_panel(int w, int h) {
    auto p = std::make_unique<panel<Backend>>();

    size_constraint width_constraint;
    width_constraint.policy = size_policy::fixed;
    width_constraint.preferred_size = w;
    width_constraint.min_size = w;
    width_constraint.max_size = w;
    p->set_width_constraint(width_constraint);

    size_constraint height_constraint;
    height_constraint.policy = size_policy::fixed;
    height_constraint.preferred_size = h;
    height_constraint.min_size = h;
    height_constraint.max_size = h;
    p->set_height_constraint(height_constraint);

    return p;
}

// =============================================================================
// 1. Construction Tests (4 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - Default construction") {
    scroll_view<test_canvas_backend> view;

    // Should have created internal components
    CHECK(view.content() != nullptr);
    CHECK(view.vertical_scrollbar() != nullptr);
    CHECK(view.horizontal_scrollbar() != nullptr);
    CHECK(view.controller() != nullptr);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - Internal grid created") {
    scroll_view<test_canvas_backend> view;

    // View should have exactly one child (the grid)
    CHECK(view.children().size() == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - Scrollable created automatically") {
    scroll_view<test_canvas_backend> view;

    auto* content = view.content();
    CHECK(content != nullptr);

    // Content should initially have no children
    CHECK(content->children().size() == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - Scroll controller connected") {
    scroll_view<test_canvas_backend> view;

    // Controller should exist
    auto* controller = view.controller();
    CHECK(controller != nullptr);

    // Add content and verify sync works
    view.add_child(make_fixed_panel<test_canvas_backend>(100, 200));

    [[maybe_unused]] auto size = view.measure(100, 100);
    view.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    // Scroll via view
    view.scroll_to(0, 50);

    // Verify scrollbar synced
    auto const info = view.vertical_scrollbar()->get_scroll_info();
    CHECK(point_utils::get_y(info.scroll_offset) == 50);
}

// =============================================================================
// 2. Forwarding Methods Tests (10 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - add_child forwards to scrollable") {
    scroll_view<test_canvas_backend> view;

    auto child = make_fixed_panel<test_canvas_backend>(50, 50);
    view.add_child(std::move(child));

    CHECK(view.content()->children().size() == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - emplace_child forwards to scrollable") {
    scroll_view<test_canvas_backend> view;

    auto* label_ptr = view.emplace_child<label>("Test");

    CHECK(label_ptr != nullptr);
    CHECK(view.content()->children().size() == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - remove_child forwards to scrollable") {
    scroll_view<test_canvas_backend> view;

    auto child = make_fixed_panel<test_canvas_backend>(50, 50);
    auto* child_ptr = child.get();
    view.add_child(std::move(child));

    auto removed = view.remove_child(child_ptr);

    CHECK(removed != nullptr);
    CHECK(view.content()->children().size() == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - clear_children forwards to scrollable") {
    scroll_view<test_canvas_backend> view;

    view.add_child(make_fixed_panel<test_canvas_backend>(50, 50));
    view.add_child(make_fixed_panel<test_canvas_backend>(50, 50));

    CHECK(view.content()->children().size() == 2);

    view.clear_children();

    CHECK(view.content()->children().size() == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - set_layout_strategy forwards to scrollable") {
    scroll_view<test_canvas_backend> view;

    auto layout = std::make_unique<linear_layout<test_canvas_backend>>(direction::vertical);
    view.set_layout_strategy(std::move(layout));

    // Verify layout was set by checking it exists
    CHECK(view.content()->has_layout_strategy());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - scroll_to forwards to scrollable") {
    scroll_view<test_canvas_backend> view;

    view.add_child(make_fixed_panel<test_canvas_backend>(100, 200));
    [[maybe_unused]] auto size = view.measure(100, 100);
    view.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    view.scroll_to(0, 50);

    auto const offset = view.content()->get_scroll_offset();
    CHECK(point_utils::get_y(offset) == 50);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - scroll_by forwards to scrollable") {
    scroll_view<test_canvas_backend> view;

    view.add_child(make_fixed_panel<test_canvas_backend>(100, 200));
    [[maybe_unused]] auto size = view.measure(100, 100);
    view.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    view.scroll_by(0, 25);
    view.scroll_by(0, 25);

    auto const offset = view.content()->get_scroll_offset();
    CHECK(point_utils::get_y(offset) == 50);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - scroll_into_view forwards to scrollable") {
    scroll_view<test_canvas_backend> view;

    auto child = make_fixed_panel<test_canvas_backend>(80, 80);
    auto* child_ptr = child.get();
    view.add_child(std::move(child));

    [[maybe_unused]] auto size = view.measure(100, 100);
    view.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    // Child fits, should remain at 0,0
    view.scroll_into_view(child_ptr);

    auto const offset = view.content()->get_scroll_offset();
    CHECK(point_utils::get_x(offset) == 0);
    CHECK(point_utils::get_y(offset) == 0);
}

// =============================================================================
// 3. Configuration Tests (8 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - set_scrollbar_policy both axes") {
    scroll_view<test_canvas_backend> view;

    view.set_scrollbar_policy(scrollbar_visibility::always);

    auto policy = view.content()->get_scrollbar_visibility_policy();
    CHECK(policy.horizontal == scrollbar_visibility::always);
    CHECK(policy.vertical == scrollbar_visibility::always);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - set_scrollbar_policy independent axes") {
    scroll_view<test_canvas_backend> view;

    view.set_scrollbar_policy(scrollbar_visibility::always, scrollbar_visibility::hidden);

    auto policy = view.content()->get_scrollbar_visibility_policy();
    CHECK(policy.horizontal == scrollbar_visibility::always);
    CHECK(policy.vertical == scrollbar_visibility::hidden);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - set_horizontal_scroll_enabled true") {
    scroll_view<test_canvas_backend> view;

    view.set_horizontal_scroll_enabled(true);

    // Policy should allow horizontal scrolling
    auto policy = view.content()->get_scrollbar_visibility_policy();
    CHECK(policy.horizontal != scrollbar_visibility::hidden);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - set_horizontal_scroll_enabled false") {
    scroll_view<test_canvas_backend> view;

    view.set_horizontal_scroll_enabled(false);

    // Policy should disable horizontal scrolling
    auto policy = view.content()->get_scrollbar_visibility_policy();
    CHECK(policy.horizontal == scrollbar_visibility::hidden);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - set_vertical_scroll_enabled true") {
    scroll_view<test_canvas_backend> view;

    view.set_vertical_scroll_enabled(true);

    // Policy should allow vertical scrolling
    auto policy = view.content()->get_scrollbar_visibility_policy();
    CHECK(policy.vertical != scrollbar_visibility::hidden);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - set_vertical_scroll_enabled false") {
    scroll_view<test_canvas_backend> view;

    view.set_vertical_scroll_enabled(false);

    // Policy should disable vertical scrolling
    auto policy = view.content()->get_scrollbar_visibility_policy();
    CHECK(policy.vertical == scrollbar_visibility::hidden);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - Configuration persists across measure/arrange") {
    scroll_view<test_canvas_backend> view;

    view.set_scrollbar_policy(scrollbar_visibility::always);
    view.add_child(make_fixed_panel<test_canvas_backend>(50, 50));

    [[maybe_unused]] auto size = view.measure(100, 100);
    view.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    // Policy should still be set
    auto policy = view.content()->get_scrollbar_visibility_policy();
    CHECK(policy.horizontal == scrollbar_visibility::always);
    CHECK(policy.vertical == scrollbar_visibility::always);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - Multiple configuration changes") {
    scroll_view<test_canvas_backend> view;

    view.set_scrollbar_policy(scrollbar_visibility::always);
    view.set_horizontal_scroll_enabled(false);

    // Horizontal should be hidden, vertical should be always
    auto policy = view.content()->get_scrollbar_visibility_policy();
    CHECK(policy.horizontal == scrollbar_visibility::hidden);
    CHECK(policy.vertical == scrollbar_visibility::always);
}

// =============================================================================
// 4. Access to Internals Tests (4 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - content() returns valid pointer") {
    scroll_view<test_canvas_backend> view;

    auto* content = view.content();

    CHECK(content != nullptr);
    // Verify it's actually a scrollable by checking we can add children
    content->add_child(make_fixed_panel<test_canvas_backend>(50, 50));
    CHECK(content->children().size() == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - vertical_scrollbar() returns valid pointer") {
    scroll_view<test_canvas_backend> view;

    auto* vscrollbar = view.vertical_scrollbar();

    CHECK(vscrollbar != nullptr);
    // Verify it's vertical
    CHECK(vscrollbar->get_orientation() == orientation::vertical);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - horizontal_scrollbar() returns valid pointer") {
    scroll_view<test_canvas_backend> view;

    auto* hscrollbar = view.horizontal_scrollbar();

    CHECK(hscrollbar != nullptr);
    // Verify it's horizontal
    CHECK(hscrollbar->get_orientation() == orientation::horizontal);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - controller() returns valid pointer") {
    scroll_view<test_canvas_backend> view;

    auto* controller = view.controller();

    CHECK(controller != nullptr);
}

// =============================================================================
// 5. Integration Tests (2 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - Complete workflow: add children, scroll, drag thumb") {
    scroll_view<test_canvas_backend> view;

    // Add content
    view.add_child(make_fixed_panel<test_canvas_backend>(100, 200));
    view.add_child(make_fixed_panel<test_canvas_backend>(100, 200));

    // Measure and arrange
    [[maybe_unused]] auto size = view.measure(100, 100);
    view.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    // Scroll via view
    view.scroll_to(0, 50);

    // Verify scrollable updated
    CHECK(point_utils::get_y(view.content()->get_scroll_offset()) == 50);

    // Verify scrollbar updated
    auto const scrollbar_info = view.vertical_scrollbar()->get_scroll_info();
    CHECK(point_utils::get_y(scrollbar_info.scroll_offset) == 50);

    // Simulate scrollbar drag
    view.vertical_scrollbar()->scroll_requested.emit(100);

    // Verify scrollable updated from scrollbar
    CHECK(point_utils::get_y(view.content()->get_scroll_offset()) == 100);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_view - All components accessible") {
    scroll_view<test_canvas_backend> view;

    // Verify all internal components are accessible
    CHECK(view.content() != nullptr);
    CHECK(view.vertical_scrollbar() != nullptr);
    CHECK(view.horizontal_scrollbar() != nullptr);
    CHECK(view.controller() != nullptr);

    // Verify they have the correct types/orientations
    CHECK(view.vertical_scrollbar()->get_orientation() == orientation::vertical);
    CHECK(view.horizontal_scrollbar()->get_orientation() == orientation::horizontal);
}
