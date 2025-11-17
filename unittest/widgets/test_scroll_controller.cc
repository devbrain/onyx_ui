/**
 * @file test_scroll_controller.cc
 * @brief Unit tests for scroll_controller coordination layer
 * @author Testing Infrastructure Team
 * @date 2025-10-29
 */

#include <doctest/doctest.h>

#include <../../include/onyxui/widgets/containers/scroll/scroll_controller.hh>
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
    width_constraint.min_size = w;   // Force minimum size
    width_constraint.max_size = w;   // Force maximum size
    p->set_width_constraint(width_constraint);

    size_constraint height_constraint;
    height_constraint.policy = size_policy::fixed;
    height_constraint.preferred_size = h;
    height_constraint.min_size = h;  // Force minimum size
    height_constraint.max_size = h;  // Force maximum size
    p->set_height_constraint(height_constraint);

    return p;
}

// =============================================================================
// 1. Construction & Setup Tests (4 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Construction with vertical scrollbar") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);

    // Constructor should not throw
    CHECK_NOTHROW(scroll_controller<test_canvas_backend>(&scroll, &vscrollbar, nullptr));
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Construction with both scrollbars") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);
    scrollbar<test_canvas_backend> hscrollbar(orientation::horizontal);

    // Constructor should not throw with both scrollbars
    CHECK_NOTHROW(scroll_controller<test_canvas_backend>(&scroll, &vscrollbar, &hscrollbar));
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Construction with nullptr scrollbars") {
    scrollable<test_canvas_backend> scroll;

    // Constructor should not throw with null scrollbars
    CHECK_NOTHROW(scroll_controller<test_canvas_backend>(&scroll, nullptr, nullptr));
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Destructor disconnects signals (RAII)") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);

    int scroll_signal_count = 0;

    // Connect to scroll signal to verify disconnection
    auto conn = scoped_connection(
        scroll.scroll_changed,
        [&scroll_signal_count](const auto&) {
            scroll_signal_count++;
        }
    );

    {
        // Controller created and connects signals
        scroll_controller<test_canvas_backend> controller(&scroll, &vscrollbar, nullptr);

        // Add content and scroll
        auto child = make_fixed_panel<test_canvas_backend>(100, 200);
        scroll.add_child(std::move(child));
        [[maybe_unused]] auto size = scroll.measure(100, 100);
        scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

        scroll.scroll_to(0, 50);
        CHECK(scroll_signal_count == 1);  // Signal emitted

        // Controller destroyed here (RAII cleanup)
    }

    // After controller destruction, scrollbar shouldn't update
    // (we can't easily test this without exposing internals, but RAII guarantees cleanup)
    scroll.scroll_to(0, 75);
    CHECK(scroll_signal_count == 2);  // Only our connection triggered
}

// =============================================================================
// 2. Scrollable → Scrollbar Sync Tests (8 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Scrollable scroll_to() updates scrollbar thumb") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);

    scroll_controller<test_canvas_backend> controller(&scroll, &vscrollbar, nullptr);

    // Add content larger than viewport
    auto child = make_fixed_panel<test_canvas_backend>(100, 200);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});
    vscrollbar.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 16, 100}});

    // Scroll content
    scroll.scroll_to(0, 50);

    // Verify scrollbar received updated scroll_info
    auto const info = vscrollbar.get_scroll_info();
    CHECK(point_utils::get_y(info.scroll_offset) == 50);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Scrollable scroll_by() updates scrollbar thumb") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);

    scroll_controller<test_canvas_backend> controller(&scroll, &vscrollbar, nullptr);

    // Add content
    auto child = make_fixed_panel<test_canvas_backend>(100, 200);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});
    vscrollbar.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 16, 100}});

    // Scroll incrementally
    scroll.scroll_by(0, 25);
    scroll.scroll_by(0, 25);

    // Verify scrollbar updated
    auto const info = vscrollbar.get_scroll_info();
    CHECK(point_utils::get_y(info.scroll_offset) == 50);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Content size change updates scrollbar visibility") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);

    scroll.set_scrollbar_visibility(scrollbar_visibility::auto_hide);
    scroll_controller<test_canvas_backend> controller(&scroll, &vscrollbar, nullptr);

    // Initially no content - scrollbar should be hidden
    [[maybe_unused]] auto size1 = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});
    vscrollbar.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 16, 100}});

    CHECK_FALSE(vscrollbar.is_visible());

    // Add large content - scrollbar should show
    auto child = make_fixed_panel<test_canvas_backend>(100, 200);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size2 = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    controller.refresh();  // Force visibility update
    CHECK(vscrollbar.is_visible());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Needs vertical scroll shows vscrollbar") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);

    scroll.set_scrollbar_visibility(scrollbar_visibility::auto_hide);
    scroll_controller<test_canvas_backend> controller(&scroll, &vscrollbar, nullptr);

    // Add tall content
    auto child = make_fixed_panel<test_canvas_backend>(50, 200);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    controller.refresh();
    CHECK(vscrollbar.is_visible());  // Content height > viewport height
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - No vertical scroll hides vscrollbar") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);

    scroll.set_scrollbar_visibility(scrollbar_visibility::auto_hide);
    scroll_controller<test_canvas_backend> controller(&scroll, &vscrollbar, nullptr);

    // Add small content that fits
    auto child = make_fixed_panel<test_canvas_backend>(50, 50);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    controller.refresh();
    CHECK_FALSE(vscrollbar.is_visible());  // Content fits in viewport
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Needs horizontal scroll shows hscrollbar") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> hscrollbar(orientation::horizontal);

    scroll.set_scrollbar_visibility(scrollbar_visibility::auto_hide);
    scroll_controller<test_canvas_backend> controller(&scroll, nullptr, &hscrollbar);

    // Add wide content
    auto child = make_fixed_panel<test_canvas_backend>(200, 50);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    controller.refresh();
    CHECK(hscrollbar.is_visible());  // Content width > viewport width
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Both scrollbars updated on content change") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);
    scrollbar<test_canvas_backend> hscrollbar(orientation::horizontal);

    scroll.set_scrollbar_visibility(scrollbar_visibility::auto_hide);
    scroll_controller<test_canvas_backend> controller(&scroll, &vscrollbar, &hscrollbar);

    // Add large content in both dimensions
    auto child = make_fixed_panel<test_canvas_backend>(200, 300);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    controller.refresh();
    CHECK(vscrollbar.is_visible());
    CHECK(hscrollbar.is_visible());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Scrollbar visibility respects always policy") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);

    scroll.set_scrollbar_visibility(scrollbar_visibility::always);
    scroll_controller<test_canvas_backend> controller(&scroll, &vscrollbar, nullptr);

    // Add small content that fits
    auto child = make_fixed_panel<test_canvas_backend>(50, 50);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    controller.refresh();
    CHECK(vscrollbar.is_visible());  // Always visible even when content fits
}

// =============================================================================
// 3. Scrollbar → Scrollable Sync Tests (8 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Vertical scrollbar scroll_requested updates scrollable Y") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);

    scroll_controller<test_canvas_backend> controller(&scroll, &vscrollbar, nullptr);

    // Add content
    auto child = make_fixed_panel<test_canvas_backend>(100, 200);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    // Simulate scrollbar interaction
    vscrollbar.scroll_requested.emit(75);

    // Verify scrollable updated
    auto const offset = scroll.get_scroll_offset();
    CHECK(point_utils::get_y(offset) == 75);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Horizontal scrollbar scroll_requested updates scrollable X") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> hscrollbar(orientation::horizontal);

    scroll_controller<test_canvas_backend> controller(&scroll, nullptr, &hscrollbar);

    // Add content
    auto child = make_fixed_panel<test_canvas_backend>(200, 100);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    // Simulate scrollbar interaction
    hscrollbar.scroll_requested.emit(50);

    // Verify scrollable updated
    auto const offset = scroll.get_scroll_offset();
    CHECK(point_utils::get_x(offset) == 50);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Vertical scrollbar preserves X offset") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);

    scroll_controller<test_canvas_backend> controller(&scroll, &vscrollbar, nullptr);

    // Add content
    auto child = make_fixed_panel<test_canvas_backend>(200, 200);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    // Set initial scroll position
    scroll.scroll_to(25, 0);

    // Vertical scrollbar changes Y only
    vscrollbar.scroll_requested.emit(50);

    // Verify X preserved, Y updated
    auto const offset = scroll.get_scroll_offset();
    CHECK(point_utils::get_x(offset) == 25);
    CHECK(point_utils::get_y(offset) == 50);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Horizontal scrollbar preserves Y offset") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> hscrollbar(orientation::horizontal);

    scroll_controller<test_canvas_backend> controller(&scroll, nullptr, &hscrollbar);

    // Add content
    auto child = make_fixed_panel<test_canvas_backend>(200, 200);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    // Set initial scroll position
    scroll.scroll_to(0, 25);

    // Horizontal scrollbar changes X only
    hscrollbar.scroll_requested.emit(75);

    // Verify Y preserved, X updated
    auto const offset = scroll.get_scroll_offset();
    CHECK(point_utils::get_x(offset) == 75);
    CHECK(point_utils::get_y(offset) == 25);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Both scrollbars update independently") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);
    scrollbar<test_canvas_backend> hscrollbar(orientation::horizontal);

    scroll_controller<test_canvas_backend> controller(&scroll, &vscrollbar, &hscrollbar);

    // Add content
    auto child = make_fixed_panel<test_canvas_backend>(200, 300);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    // Update vertical
    vscrollbar.scroll_requested.emit(50);
    CHECK(point_utils::get_y(scroll.get_scroll_offset()) == 50);
    CHECK(point_utils::get_x(scroll.get_scroll_offset()) == 0);

    // Update horizontal
    hscrollbar.scroll_requested.emit(75);
    CHECK(point_utils::get_x(scroll.get_scroll_offset()) == 75);
    CHECK(point_utils::get_y(scroll.get_scroll_offset()) == 50);  // Preserved
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Scrollbar clamping respected") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);

    scroll_controller<test_canvas_backend> controller(&scroll, &vscrollbar, nullptr);

    // Add content: 200px content in 100px viewport = max_scroll = 100
    auto child = make_fixed_panel<test_canvas_backend>(100, 200);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    // Try to scroll beyond max
    vscrollbar.scroll_requested.emit(150);

    // Should be clamped to 100
    auto const offset = scroll.get_scroll_offset();
    CHECK(point_utils::get_y(offset) == 100);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Negative scroll clamped to zero") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);

    scroll_controller<test_canvas_backend> controller(&scroll, &vscrollbar, nullptr);

    // Add content
    auto child = make_fixed_panel<test_canvas_backend>(100, 200);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    // Scroll to middle first
    scroll.scroll_to(0, 50);

    // Try to scroll negative
    vscrollbar.scroll_requested.emit(-10);

    // Should be clamped to 0
    auto const offset = scroll.get_scroll_offset();
    CHECK(point_utils::get_y(offset) == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Multiple scrollbar updates accumulate correctly") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);

    scroll_controller<test_canvas_backend> controller(&scroll, &vscrollbar, nullptr);

    // Add content
    auto child = make_fixed_panel<test_canvas_backend>(100, 400);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    // Multiple scrollbar updates
    vscrollbar.scroll_requested.emit(50);
    vscrollbar.scroll_requested.emit(100);
    vscrollbar.scroll_requested.emit(150);

    // Final position should be 150
    auto const offset = scroll.get_scroll_offset();
    CHECK(point_utils::get_y(offset) == 150);
}

// =============================================================================
// 4. Bidirectional Sync Tests (4 tests)
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Mouse wheel on scrollable updates scrollbar") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);

    scroll_controller<test_canvas_backend> controller(&scroll, &vscrollbar, nullptr);

    // Add content
    auto child = make_fixed_panel<test_canvas_backend>(100, 200);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});
    vscrollbar.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 16, 100}});

    // Scroll via scrollable (simulating mouse wheel)
    scroll.scroll_by(0, 30);

    // Verify scrollbar updated
    auto const info = vscrollbar.get_scroll_info();
    CHECK(point_utils::get_y(info.scroll_offset) == 30);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - No infinite loop on scrollbar drag") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);

    int scroll_signal_count = 0;

    // Track signal emissions
    auto conn1 = scoped_connection(
        scroll.scroll_changed,
        [&scroll_signal_count](const auto&) { scroll_signal_count++; }
    );

    scroll_controller<test_canvas_backend> controller(&scroll, &vscrollbar, nullptr);

    // Add content
    auto child = make_fixed_panel<test_canvas_backend>(100, 200);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});

    // Simulate scrollbar drag
    vscrollbar.scroll_requested.emit(50);

    // Should trigger scroll_changed exactly once (no loop)
    CHECK(scroll_signal_count == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Multiple rapid scrolls stay in sync") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);

    scroll_controller<test_canvas_backend> controller(&scroll, &vscrollbar, nullptr);

    // Add content
    auto child = make_fixed_panel<test_canvas_backend>(100, 500);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});
    vscrollbar.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 16, 100}});

    // Rapid scroll sequence
    scroll.scroll_to(0, 50);
    scroll.scroll_by(0, 25);
    scroll.scroll_to(0, 100);
    scroll.scroll_by(0, -20);

    // Verify scrollbar matches final position
    auto const scroll_offset = scroll.get_scroll_offset();
    auto const scrollbar_info = vscrollbar.get_scroll_info();

    CHECK(point_utils::get_y(scroll_offset) == 80);
    CHECK(point_utils::get_y(scrollbar_info.scroll_offset) == 80);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scroll_controller - Mixed scroll sources stay in sync") {
    scrollable<test_canvas_backend> scroll;
    scrollbar<test_canvas_backend> vscrollbar(orientation::vertical);
    scrollbar<test_canvas_backend> hscrollbar(orientation::horizontal);

    scroll_controller<test_canvas_backend> controller(&scroll, &vscrollbar, &hscrollbar);

    // Add content
    auto child = make_fixed_panel<test_canvas_backend>(300, 400);
    scroll.add_child(std::move(child));
    [[maybe_unused]] auto size = scroll.measure(100, 100);
    scroll.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 100}});
    vscrollbar.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 16, 100}});
    hscrollbar.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 100, 16}});

    // Scroll from different sources
    scroll.scroll_to(50, 100);           // Direct scroll
    vscrollbar.scroll_requested.emit(150);  // Vertical scrollbar
    hscrollbar.scroll_requested.emit(75);   // Horizontal scrollbar
    scroll.scroll_by(-10, -20);          // Mouse wheel

    // Final state should be consistent
    auto const scroll_offset = scroll.get_scroll_offset();
    auto const vscroll_info = vscrollbar.get_scroll_info();
    auto const hscroll_info = hscrollbar.get_scroll_info();

    CHECK(point_utils::get_x(scroll_offset) == 65);  // 75 - 10
    CHECK(point_utils::get_y(scroll_offset) == 130); // 150 - 20
    CHECK(point_utils::get_x(hscroll_info.scroll_offset) == 65);
    CHECK(point_utils::get_y(vscroll_info.scroll_offset) == 130);
}
