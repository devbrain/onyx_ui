/**
 * @file test_helpers.hh
 * @brief Testing utilities and helper classes for layout tests
 */

#pragma once

#include <doctest/doctest.h>
#include <onyxui/element.hh>
#include <onyxui/ui_services.hh>
#include "test_backend.hh"
#include <vector>
#include <iostream>

using namespace onyxui;

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

        // Expose bounds for testing
        const TestRect& test_bounds() const { return bounds(); }
};

// Custom element with content size
class ContentElement : public TestElement {
    public:
        ContentElement(int w, int h)
            : TestElement(nullptr), content_width(w), content_height(h) {
        }

    protected:
        TestSize get_content_size() const override {
            return TestSize{content_width, content_height};
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
        [[maybe_unused]] auto measured_size = element.measure(width, height);
        canvas_rect bounds{0, 0, width, height};
        element.arrange(bounds);

        // Render with current theme (if available)
        auto* themes_registry = ui_services<test_canvas_backend>::themes();
        if (!themes_registry) {
            throw std::runtime_error(
                "render_to_canvas() requires an active UI context on the stack. "
                "Ensure your test creates a scoped_ui_context<Backend> before calling render_to_canvas()."
            );
        }

        auto* theme = themes_registry->get_current_theme();
        element.render(renderer, theme);

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

} // namespace onyxui::testing
