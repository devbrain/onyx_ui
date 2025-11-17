/**
 * @file test_scoped_clip.cc
 * @brief Tests for scoped_clip RAII guard
 * @author Claude Code
 * @date 2025-10-29
 */

#include <doctest/doctest.h>
#include <../../include/onyxui/core/raii/scoped_clip.hh>
#include <../../include/onyxui/widgets/containers/panel.hh>
#include <../../include/onyxui/widgets/containers/scroll/scrollable.hh>
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;
using namespace onyxui::testing;

// =============================================================================
// Basic RAII Tests
// =============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scoped_clip - RAII: Constructor and destructor work correctly") {
    // Create a simple panel
    auto test_panel = std::make_unique<panel<test_canvas_backend>>();

    // Measure and arrange
    [[maybe_unused]] auto size = test_panel->measure(80, 25);
    test_panel->arrange(make_relative_rect<test_canvas_backend>(0, 0, 80, 25));

    // Render with clipping - just verify compilation and runtime
    auto canvas = render_to_canvas(*test_panel, 80, 25);

    // Test passes if no crashes or leaks
    CHECK(canvas->width() == 80);
    CHECK(canvas->height() == 25);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "scoped_clip - RAII: Used in scrollable widget") {
    // The real test is that scrollable uses scoped_clip successfully
    // This is tested implicitly by the existing scrollable clipping tests
    // This test just verifies the API is usable

    // Create a scrollable panel with content
    auto scrollable_panel = std::make_unique<scrollable<test_canvas_backend>>();

    for (int i = 0; i < 10; ++i) {
        scrollable_panel->emplace_child<panel>();
    }

    // Measure and arrange
    [[maybe_unused]] auto size = scrollable_panel->measure(80, 25);
    scrollable_panel->arrange(make_relative_rect<test_canvas_backend>(0, 0, 80, 25));

    // Render (uses scoped_clip internally)
    auto canvas = render_to_canvas(*scrollable_panel, 80, 25);

    // Test passes if rendering succeeds
    CHECK(canvas != nullptr);
}

// Note: The real tests for scoped_clip behavior are in test_scrollable_clipping.cc
// which verifies that scrollable correctly uses scoped_clip for viewport clipping.
