/**
 * @file test_helpers.hh
 * @brief Testing utilities and helper classes for layout tests
 */

#pragma once

#include <doctest/doctest.h>
#include <../../include/onyxui/core/element.hh>
#include <../../include/onyxui/services/ui_services.hh>
#include <../../include/onyxui/services/ui_context.hh>
#include "test_backend.hh"
#include <vector>
#include <iostream>

using namespace onyxui;

// ============================================================================
// Test Fixture for UI Context (RAII pattern for theme initialization)
// ============================================================================

/**
 * @brief Test fixture that automatically sets up ui_context with theme
 *
 * @details
 * Use this fixture in any TEST_CASE that needs to measure or render widgets.
 * It ensures that a valid theme is available for measurement, preventing
 * "widget::get_content_size() called without theme" errors.
 *
 * @example
 * TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "My test") {
 *     // Theme is already set up - widgets can be measured
 *     button<test_backend> btn("Test");
 *     auto size = btn.measure(100, 100);  // Works!
 * }
 */
template<UIBackend Backend>
struct ui_context_fixture {
    ui_context_fixture()
        : ctx(make_terminal_metrics<Backend>())  // Tests use 1:1 mapping for simplicity
    {
        // Setup theme with minimal required properties
        ui_theme<Backend> theme;
        theme.name = "TestTheme";
        theme.window_bg = typename Backend::color_type{50, 50, 50};
        theme.text_fg = typename Backend::color_type{255, 255, 255};
        theme.border_color = typename Backend::color_type{100, 100, 100};

        // Initialize scrollbar theme properties for visual tests
        using box_style_type = typename Backend::renderer_type::box_style;
        box_style_type scrollbar_box{};
        scrollbar_box.draw_border = true;  // Enable border drawing for visual tests

        theme.scrollbar.width = 16;              // Default scrollbar width
        theme.scrollbar.min_thumb_size = 20;     // Minimum thumb size
        theme.scrollbar.arrow_size = 1;          // Arrow button size (1px for text UI)
        theme.scrollbar.min_render_size = 8;     // Minimum size to prevent corruption
        theme.scrollbar.line_increment = 20;     // Scroll amount per arrow click
        theme.scrollbar.track_normal.box_style = scrollbar_box;
        theme.scrollbar.thumb_normal.box_style = scrollbar_box;
        theme.scrollbar.thumb_hover.box_style = scrollbar_box;
        theme.scrollbar.thumb_pressed.box_style = scrollbar_box;
        theme.scrollbar.thumb_disabled.box_style = scrollbar_box;
        theme.scrollbar.arrow_normal.box_style = scrollbar_box;
        theme.scrollbar.arrow_hover.box_style = scrollbar_box;
        theme.scrollbar.arrow_pressed.box_style = scrollbar_box;

        // Register and activate theme
        ctx.themes().register_theme(std::move(theme));
        ctx.themes().set_current_theme("TestTheme");
    }

    ~ui_context_fixture() = default;

    // Make context accessible to tests if needed
    scoped_ui_context<Backend> ctx;
};

// Use the test_backend for all tests
using TestBackend = test_backend;
using TestRect = TestBackend::rect_type;
using TestSize = TestBackend::size_type;
using TestPoint = TestBackend::point_type;

// Test element that exposes protected members for testing
class TestElement : public ui_element <TestBackend> {
    public:
        explicit TestElement(TestElement* parent = nullptr)
            : ui_element <TestBackend>(parent) {
        }

        // Get child at index for testing
        TestElement* child_at(size_t index) {
            const auto& ch = children();
            if (index < ch.size()) {
                return static_cast <TestElement*>(ch[index].get());
            }
            return nullptr;
        }

        size_t child_count() const { return children().size(); }

        // Helper to add test children
        void add_test_child(std::unique_ptr <TestElement> child) {
            add_child(std::move(child));
        }

        // Expose bounds for testing (convert logical to physical)
        TestRect test_bounds() const {
            const auto& lb = bounds();
            TestRect r;
            rect_utils::set_bounds(r, lb.x.to_int(), lb.y.to_int(), lb.width.to_int(), lb.height.to_int());
            return r;
        }
};

// Custom element with content size
class ContentElement : public TestElement {
    public:
        ContentElement(int w, int h)
            : TestElement(nullptr), content_width(w), content_height(h) {
        }

    protected:
        logical_size get_content_size() const override {
            return logical_size{logical_unit(static_cast<double>(content_width)),
                              logical_unit(static_cast<double>(content_height))};
        }

    private:
        int content_width, content_height;
};

// Test fixture helper
class LayoutTestFixture {
    public:
        void assert_bounds(TestElement* element, int x, int y, int w, int h) {
            const auto& bounds = element->test_bounds();
            CHECK(bounds.x == x);
            CHECK(bounds.y == y);
            CHECK(bounds.w == w);
            CHECK(bounds.h == h);
        }

        void assert_size(const TestSize& size, int w, int h) {
            CHECK(size.w == w);
            CHECK(size.h == h);
        }
};

// ============================================================================
// Visual Testing Helpers (NEW - for canvas-based validation)
// ============================================================================

#include "test_canvas.hh"
#include "test_canvas_backend.hh"
#include <sstream>
#include <iomanip>

namespace onyxui::testing {

    /**
     * @brief Render element to canvas and return canvas for inspection
     *
     * @details
     * This is the main entry point for visual testing:
     * 1. Creates a canvas of specified size
     * 2. Creates a renderer for that canvas
     * 3. Measures and arranges the element
     * 4. Renders the element to the canvas
     * 5. Returns the canvas for visual inspection
     */
    inline std::shared_ptr<test_canvas> render_to_canvas(
        ui_element<test_canvas_backend>& element,
        int width,
        int height)
    {
        auto canvas = std::make_shared<test_canvas>(width, height);
        canvas_renderer renderer(canvas);

        // Measure and arrange
        [[maybe_unused]] auto measured_size = element.measure(
            logical_unit(static_cast<double>(width)),
            logical_unit(static_cast<double>(height)));
        logical_rect bounds{
            logical_unit(0.0),
            logical_unit(0.0),
            logical_unit(static_cast<double>(width)),
            logical_unit(static_cast<double>(height))
        };
        element.arrange(bounds);

        // Render with current theme (if available)
        auto* themes_registry = ui_services<test_canvas_backend>::themes();
        if (!themes_registry) {
            throw std::runtime_error(
                "render_to_canvas() requires an active UI context on the stack. "
                "Ensure your test creates a scoped_ui_context<Backend> before calling render_to_canvas()."
            );
        }

        auto* theme_ptr = themes_registry->get_current_theme();
        if (!theme_ptr) {
            throw std::runtime_error("No current theme set!");
        }

        // Get metrics (tests use terminal metrics - 1:1 mapping)
        auto metrics = make_terminal_metrics<test_canvas_backend>();
        element.render(renderer, theme_ptr, metrics);

        return canvas;
    }

    /**
     * @brief Print canvas for debugging (with coordinates)
     */
    inline std::string debug_canvas(const test_canvas& canvas) {
        std::ostringstream oss;

        // Header with X coordinates
        oss << "   ";
        for (int x = 0; x < canvas.width(); ++x) {
            oss << (x % 10);
        }
        oss << '\n';

        // Rows with Y coordinates
        for (int y = 0; y < canvas.height(); ++y) {
            oss << std::setw(2) << y << ' ';
            oss << canvas.get_row(y) << '\n';
        }

        return oss.str();
    }

    // ============================================================================
    // Helper function for Phase 2: Strong coordinate types
    // ============================================================================

    /**
     * @brief Helper to create relative_rect from coordinates
     * @details Wraps braced initializer lists for arrange() calls in tests
     */
    template<UIBackend Backend>
    inline geometry::relative_rect<Backend> make_relative_rect(int x, int y, int w, int h) {
        typename Backend::rect_type r;
        rect_utils::set_bounds(r, x, y, w, h);
        return geometry::relative_rect<Backend>{r};
    }

} // namespace onyxui::testing
