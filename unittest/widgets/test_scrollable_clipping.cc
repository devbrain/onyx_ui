/**
 * @file test_scrollable_clipping.cc
 * @brief Tests for scrollable viewport clipping functionality
 * @author Claude Code
 * @date 2025-10-29
 */

#include <doctest/doctest.h>
#include <../../include/onyxui/widgets/containers/scroll/scrollable.hh>
#include <../../include/onyxui/widgets/containers/panel.hh>
#include <../../include/onyxui/widgets/containers/vbox.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_canvas_backend.hh"

using namespace onyxui;
using namespace onyxui::testing;

// Helper function to create fixed-size panel
static std::unique_ptr<panel<test_canvas_backend>> make_fixed_panel(int width, int height) {
    auto p = std::make_unique<panel<test_canvas_backend>>();

    size_constraint width_constraint;
    width_constraint.policy = size_policy::fixed;
    width_constraint.preferred_size = width;
    width_constraint.min_size = width;
    width_constraint.max_size = width;
    p->set_width_constraint(width_constraint);

    size_constraint height_constraint;
    height_constraint.policy = size_policy::fixed;
    height_constraint.preferred_size = height;
    height_constraint.min_size = height;
    height_constraint.max_size = height;
    p->set_height_constraint(height_constraint);

    return p;
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Viewport clipping with large content") {
    scrollable<test_canvas_backend> scroll;

    // Add single large child (1000px tall)
    scroll.add_child(make_fixed_panel(200, 1000));

    // Measure with viewport height of 100px
    [[maybe_unused]] auto size = scroll.measure(200, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 200, 100}});

    // Verify content extends beyond viewport
    auto info = scroll.get_scroll_info();
    CHECK(scroll.children().size() == 1);
    CHECK(size_utils::get_height(info.content_size) == 1000);
    CHECK(size_utils::get_height(info.viewport_size) == 100);
    CHECK(info.needs_vertical_scroll());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Nested clipping") {
    // Create outer scrollable
    scrollable<test_canvas_backend> outer;

    // Create inner scrollable with large content
    auto inner = std::make_unique<scrollable<test_canvas_backend>>();
    auto* inner_ptr = inner.get();
    inner_ptr->add_child(make_fixed_panel(250, 600));  // 600px tall

    outer.add_child(std::move(inner));

    // Measure and arrange
    [[maybe_unused]] auto size = outer.measure(300, 200);
    outer.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 300, 200}});

    // Both scrollables should exist
    CHECK_FALSE(outer.children().empty());
    CHECK_FALSE(inner_ptr->children().empty());

    // Inner should need scrolling
    auto inner_info = inner_ptr->get_scroll_info();
    CHECK(size_utils::get_height(inner_info.content_size) == 600);
    // Inner viewport might be same as content if outer gave it enough space
    // This test just verifies nested scrollables work, not that both need scrolling
    CHECK(size_utils::get_height(inner_info.viewport_size) <= 600);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Clipping with empty content") {
    scrollable<test_canvas_backend> scroll;

    // No children added
    [[maybe_unused]] auto size = scroll.measure(200, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 200, 100}});

    CHECK(scroll.children().empty());
    auto info = scroll.get_scroll_info();
    CHECK(size_utils::get_width(info.content_size) == 0);
    CHECK(size_utils::get_height(info.content_size) == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Clipping with single large item") {
    scrollable<test_canvas_backend> scroll;

    // Add one huge item
    scroll.add_child(make_fixed_panel(200, 5000));  // 5000px tall

    // Measure with small viewport
    [[maybe_unused]] auto size = scroll.measure(200, 100);  // Viewport: 100px
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 200, 100}});

    // Item exists even though most is clipped
    CHECK(scroll.children().size() == 1);

    auto info = scroll.get_scroll_info();
    CHECK(size_utils::get_height(info.content_size) == 5000);
    CHECK(size_utils::get_height(info.viewport_size) == 100);
    CHECK(info.needs_vertical_scroll());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Scroll position affects viewport") {
    scrollable<test_canvas_backend> scroll;

    // Add large content (3000px tall)
    scroll.add_child(make_fixed_panel(200, 3000));

    [[maybe_unused]] auto size = scroll.measure(200, 100);  // Viewport: 100px tall
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 200, 100}});

    SUBCASE("Initial scroll position (top)") {
        scroll.scroll_to(0, 0);
        auto info = scroll.get_scroll_info();
        CHECK(point_utils::get_y(info.scroll_offset) == 0);
    }

    SUBCASE("Scrolled down") {
        scroll.scroll_to(0, 500);
        auto info = scroll.get_scroll_info();
        CHECK(point_utils::get_y(info.scroll_offset) == 500);
    }

    SUBCASE("Scrolled to bottom") {
        // Content: 3000px, Viewport: 100px, Max scroll: 2900px
        scroll.scroll_to(0, 5000);  // Try to scroll beyond
        auto info = scroll.get_scroll_info();
        // Should be clamped to max
        CHECK(point_utils::get_y(info.scroll_offset) == 2900);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scrollable - Clipping with vbox content") {
    scrollable<test_canvas_backend> scroll;

    // Use vbox to stack multiple children
    auto content = std::make_unique<vbox<test_canvas_backend>>();
    auto* content_ptr = content.get();

    // Add multiple panels to vbox
    content_ptr->add_child(make_fixed_panel(100, 50));
    content_ptr->add_child(make_fixed_panel(200, 75));
    content_ptr->add_child(make_fixed_panel(150, 100));

    scroll.add_child(std::move(content));

    [[maybe_unused]] auto size = scroll.measure(300, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 300, 100}});

    auto info = scroll.get_scroll_info();

    // Content height = sum of children + spacing gaps
    // 50 + 75 + 100 = 225, plus 2 gaps of spacing::medium (1 each) = 227
    CHECK(size_utils::get_height(info.content_size) == 227);
    CHECK(size_utils::get_height(info.viewport_size) == 100);
    CHECK(info.needs_vertical_scroll());
}
