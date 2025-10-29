/**
 * @file visual_test_helpers.hh
 * @brief Visual test harness for validating widget rendering
 * @author Testing Infrastructure Team
 * @date 2025-10-28
 *
 * @details
 * Provides a test harness that renders widgets to test_canvas and validates
 * visual output char-by-char. This allows tests to verify rendering correctness
 * beyond just layout calculations.
 */

#pragma once

#include "test_canvas.hh"
#include "test_canvas_backend.hh"
#include <onyxui/element.hh>
#include <onyxui/ui_context.hh>
#include <onyxui/ui_handle.hh>
#include <memory>
#include <string>
#include <fstream>

namespace onyxui::testing {

    /**
     * @class visual_test_harness
     * @brief Capture rendered output to test_canvas for visual validation
     *
     * @details
     * This class provides a convenient way to:
     * 1. Render widgets to a test_canvas
     * 2. Validate visual output with assertions
     * 3. Debug rendering issues with canvas dumps
     *
     * @example
     * @code
     * visual_test_harness<test_canvas_backend> harness(80, 25);
     * auto button = std::make_unique<button<test_canvas_backend>>("Click");
     * harness.render(button.get());
     * harness.expect_char_at(0, 0, '+');  // Top-left corner
     * @endcode
     */
    template<UIBackend Backend>
    class visual_test_harness {
    public:
        using element_type = ui_element<Backend>;
        using rect_type = typename Backend::rect_type;
        using canvas_type = test_canvas;

        /**
         * @brief Construct visual test harness with specified canvas dimensions
         * @param width Canvas width in characters
         * @param height Canvas height in characters
         */
        explicit visual_test_harness(int width, int height)
            : m_width(width)
            , m_height(height)
            , m_canvas(std::make_shared<test_canvas>(width, height))
        {
        }

        /**
         * @brief Render widget to canvas
         * @param widget Widget to render
         *
         * @details
         * This method:
         * 1. Creates a scoped UI context
         * 2. Measures the widget
         * 3. Arranges the widget to fill the canvas
         * 4. Renders the widget to the canvas via canvas_renderer
         */
        void render(element_type* widget) {
            if (!widget) return;

            // Create UI context for rendering
            scoped_ui_context<Backend> ctx;

            // Measure and arrange widget to fill canvas
            widget->measure(m_width, m_height);
            rect_type bounds{0, 0, m_width, m_height};
            widget->arrange(bounds);

            // Render widget to canvas
            typename Backend::renderer_type renderer(m_canvas);
            std::vector<rect_type> dirty_regions;  // Empty = render everything
            widget->render(renderer, dirty_regions);
        }

        /**
         * @brief Assert that a specific character is at a given position
         * @param x X coordinate
         * @param y Y coordinate
         * @param expected Expected character
         *
         * @details
         * Uses doctest CHECK macro to validate the character at (x, y).
         * If the check fails, the test will fail with a clear error message.
         */
        void expect_char_at(int x, int y, char expected) const {
            if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
                CHECK_MESSAGE(false, "Position (" << x << ", " << y << ") out of bounds");
                return;
            }

            char actual = m_canvas->get_char(x, y);
            CHECK_MESSAGE(actual == expected,
                         "Character at (" << x << ", " << y << "): "
                         << "expected '" << expected << "', got '" << actual << "'");
        }

        /**
         * @brief Assert that a rectangle is filled with a specific character
         * @param r Rectangle bounds
         * @param fill_char Expected fill character
         *
         * @details
         * Validates that every position within the rectangle contains the
         * specified fill character.
         */
        void expect_rect_filled(const rect_type& r, char fill_char) const {
            int const x = rect_utils::get_x(r);
            int const y = rect_utils::get_y(r);
            int const w = rect_utils::get_width(r);
            int const h = rect_utils::get_height(r);

            for (int dy = 0; dy < h; ++dy) {
                for (int dx = 0; dx < w; ++dx) {
                    expect_char_at(x + dx, y + dy, fill_char);
                }
            }
        }

        /**
         * @brief Assert that a rectangle has a border with specified characters
         * @param r Rectangle bounds
         * @param corner Corner character
         * @param horiz Horizontal edge character
         * @param vert Vertical edge character
         *
         * @details
         * Validates that the rectangle has a border drawn with box-drawing
         * characters. This is useful for testing panel borders, scrollbar tracks, etc.
         */
        void expect_border(const rect_type& r, char corner, char horiz, char vert) const {
            int const x = rect_utils::get_x(r);
            int const y = rect_utils::get_y(r);
            int const w = rect_utils::get_width(r);
            int const h = rect_utils::get_height(r);

            if (w < 2 || h < 2) return;  // Too small for border

            // Corners
            expect_char_at(x, y, corner);
            expect_char_at(x + w - 1, y, corner);
            expect_char_at(x, y + h - 1, corner);
            expect_char_at(x + w - 1, y + h - 1, corner);

            // Horizontal edges
            for (int dx = 1; dx < w - 1; ++dx) {
                expect_char_at(x + dx, y, horiz);
                expect_char_at(x + dx, y + h - 1, horiz);
            }

            // Vertical edges
            for (int dy = 1; dy < h - 1; ++dy) {
                expect_char_at(x, y + dy, vert);
                expect_char_at(x + w - 1, y + dy, vert);
            }
        }

        /**
         * @brief Assert that a region contains only spaces
         * @param r Rectangle bounds
         *
         * @details
         * Validates that the region is empty (filled with spaces).
         * Useful for testing clipping, transparency, and empty areas.
         */
        void expect_empty_region(const rect_type& r) const {
            expect_rect_filled(r, ' ');
        }

        /**
         * @brief Dump canvas contents as a string
         * @return Multi-line string representation of canvas
         *
         * @details
         * Returns a human-readable representation of the canvas with:
         * - Row numbers on the left
         * - Visible characters
         * - Border around canvas
         *
         * Useful for debugging test failures.
         *
         * @example
         * @code
         * std::cout << harness.dump_canvas() << std::endl;
         * @endcode
         */
        [[nodiscard]] std::string dump_canvas() const {
            std::string result;
            result.reserve(static_cast<std::string::size_type>((m_width + 10) * (m_height + 2)));

            // Top border
            result += "    +";
            result.append(static_cast<std::string::size_type>(m_width), '-');
            result += "+\n";

            // Canvas rows with line numbers
            for (int y = 0; y < m_height; ++y) {
                result += std::to_string(y);
                if (y < 10) result += "   |";
                else if (y < 100) result += "  |";
                else result += " |";

                for (int x = 0; x < m_width; ++x) {
                    result += m_canvas->get_char(x, y);
                }
                result += "|\n";
            }

            // Bottom border
            result += "    +";
            result.append(static_cast<std::string::size_type>(m_width), '-');
            result += "+\n";

            return result;
        }

        /**
         * @brief Save canvas snapshot to file
         * @param filename File path to save to
         *
         * @details
         * Saves the canvas dump to a text file for later inspection.
         * Useful for debugging complex rendering issues.
         *
         * @note File is created/overwritten in the current directory.
         */
        void save_snapshot(const std::string& filename) const {
            std::ofstream file(filename);
            if (file.is_open()) {
                file << dump_canvas();
                file.close();
            }
        }

        /**
         * @brief Get canvas dimensions
         */
        [[nodiscard]] int width() const noexcept { return m_width; }
        [[nodiscard]] int height() const noexcept { return m_height; }

        /**
         * @brief Access underlying canvas (for advanced use)
         */
        [[nodiscard]] std::shared_ptr<test_canvas> canvas() const noexcept { return m_canvas; }

    private:
        int m_width;
        int m_height;
        std::shared_ptr<test_canvas> m_canvas;
    };

} // namespace onyxui::testing
