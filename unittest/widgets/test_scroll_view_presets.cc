/**
 * @file test_scroll_view_presets.cc
 * @brief Unit tests for scroll_view preset factory functions
 * @author Testing Infrastructure Team
 * @date 2025-10-29
 */

#include <doctest/doctest.h>

#include <../../include/onyxui/widgets/containers/scroll/scroll_view_presets.hh>
#include <../../include/onyxui/widgets/containers/panel.hh>
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
// 1. modern_scroll_view Tests (4 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "modern_scroll_view - Uses auto_hide policy") {
    auto view = modern_scroll_view<test_canvas_backend>();

    auto policy = view->content()->get_scrollbar_visibility_policy();
    CHECK(policy.horizontal == scrollbar_visibility::auto_hide);
    CHECK(policy.vertical == scrollbar_visibility::auto_hide);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "modern_scroll_view - Both scrollbars enabled") {
    auto view = modern_scroll_view<test_canvas_backend>();

    auto policy = view->content()->get_scrollbar_visibility_policy();
    CHECK(policy.horizontal != scrollbar_visibility::hidden);
    CHECK(policy.vertical != scrollbar_visibility::hidden);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "modern_scroll_view - Can add children") {
    auto view = modern_scroll_view<test_canvas_backend>();

    view->add_child(make_fixed_panel<test_canvas_backend>(100, 200));

    CHECK(view->content()->children().size() == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "modern_scroll_view - Scrolling works") {
    auto view = modern_scroll_view<test_canvas_backend>();

    view->add_child(make_fixed_panel<test_canvas_backend>(100, 200));

    [[maybe_unused]] auto size = view->measure(100, 100);
    view->arrange({0, 0, 100, 100});

    view->scroll_to(0, 50);

    auto const offset = view->content()->get_scroll_offset();
    CHECK(point_utils::get_y(offset) == 50);
}

// =============================================================================
// 2. classic_scroll_view Tests (4 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "classic_scroll_view - Uses always visible policy") {
    auto view = classic_scroll_view<test_canvas_backend>();

    auto policy = view->content()->get_scrollbar_visibility_policy();
    CHECK(policy.horizontal == scrollbar_visibility::always);
    CHECK(policy.vertical == scrollbar_visibility::always);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "classic_scroll_view - Both scrollbars enabled") {
    auto view = classic_scroll_view<test_canvas_backend>();

    auto policy = view->content()->get_scrollbar_visibility_policy();
    CHECK(policy.horizontal != scrollbar_visibility::hidden);
    CHECK(policy.vertical != scrollbar_visibility::hidden);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "classic_scroll_view - Can add children") {
    auto view = classic_scroll_view<test_canvas_backend>();

    view->add_child(make_fixed_panel<test_canvas_backend>(100, 200));

    CHECK(view->content()->children().size() == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "classic_scroll_view - Scrolling works") {
    auto view = classic_scroll_view<test_canvas_backend>();

    view->add_child(make_fixed_panel<test_canvas_backend>(100, 200));

    [[maybe_unused]] auto size = view->measure(100, 100);
    view->arrange({0, 0, 100, 100});

    view->scroll_to(0, 50);

    auto const offset = view->content()->get_scroll_offset();
    CHECK(point_utils::get_y(offset) == 50);
}

// =============================================================================
// 3. compact_scroll_view Tests (4 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "compact_scroll_view - Uses auto_hide policy") {
    auto view = compact_scroll_view<test_canvas_backend>();

    auto policy = view->content()->get_scrollbar_visibility_policy();
    CHECK(policy.horizontal == scrollbar_visibility::auto_hide);
    CHECK(policy.vertical == scrollbar_visibility::auto_hide);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "compact_scroll_view - Both scrollbars enabled") {
    auto view = compact_scroll_view<test_canvas_backend>();

    auto policy = view->content()->get_scrollbar_visibility_policy();
    CHECK(policy.horizontal != scrollbar_visibility::hidden);
    CHECK(policy.vertical != scrollbar_visibility::hidden);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "compact_scroll_view - Can add children") {
    auto view = compact_scroll_view<test_canvas_backend>();

    view->add_child(make_fixed_panel<test_canvas_backend>(100, 200));

    CHECK(view->content()->children().size() == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "compact_scroll_view - Scrolling works") {
    auto view = compact_scroll_view<test_canvas_backend>();

    view->add_child(make_fixed_panel<test_canvas_backend>(100, 200));

    [[maybe_unused]] auto size = view->measure(100, 100);
    view->arrange({0, 0, 100, 100});

    view->scroll_to(0, 50);

    auto const offset = view->content()->get_scroll_offset();
    CHECK(point_utils::get_y(offset) == 50);
}

// =============================================================================
// 4. vertical_only_scroll_view Tests (4 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "vertical_only_scroll_view - Horizontal scroll disabled") {
    auto view = vertical_only_scroll_view<test_canvas_backend>();

    auto policy = view->content()->get_scrollbar_visibility_policy();
    CHECK(policy.horizontal == scrollbar_visibility::hidden);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "vertical_only_scroll_view - Vertical scroll enabled") {
    auto view = vertical_only_scroll_view<test_canvas_backend>();

    auto policy = view->content()->get_scrollbar_visibility_policy();
    CHECK(policy.vertical != scrollbar_visibility::hidden);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "vertical_only_scroll_view - Can add children") {
    auto view = vertical_only_scroll_view<test_canvas_backend>();

    view->add_child(make_fixed_panel<test_canvas_backend>(100, 200));

    CHECK(view->content()->children().size() == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "vertical_only_scroll_view - Vertical scrolling works") {
    auto view = vertical_only_scroll_view<test_canvas_backend>();

    view->add_child(make_fixed_panel<test_canvas_backend>(100, 200));

    [[maybe_unused]] auto size = view->measure(100, 100);
    view->arrange({0, 0, 100, 100});

    view->scroll_to(0, 50);

    auto const offset = view->content()->get_scroll_offset();
    CHECK(point_utils::get_y(offset) == 50);
}
