/**
 * @file test_canvas.hh
 * @brief Visual testing framework - Virtual rendering surface for layout validation
 * @author Testing Infrastructure Team
 * @date 2025-10-21
 *
 * @details
 * Provides a text-mode canvas similar to VRAM in the conio backend.
 * Allows tests to verify not just coordinates, but actual visual output.
 *
 * This solves the problem where tests verify coordinates but miss rendering bugs.
 * Now we can check: "Is the border actually drawn?" and "Is text at the right position?"
 */

#pragma once

#include <vector>
#include <string>
#include <optional>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <iostream>

namespace onyxui::testing {

    /**
     * @struct canvas_cell
     * @brief Single character cell with attributes
     */
    struct canvas_cell {
        char ch;           ///< Character (space = empty)
        uint8_t fg_color;    ///< Foreground color (0-15)
        uint8_t bg_color;    ///< Background color (0-15)
        uint8_t attrs;       ///< Text attributes (bold, underline, etc.)

        // Explicit default constructor
        canvas_cell() : ch(' '), fg_color(7), bg_color(0), attrs(0) {}

        bool is_empty() const { return ch == ' ' && bg_color == 0; }

        bool is_border() const {
            // Only check ASCII box characters since ch is a single-byte char.
            // UTF-8 box-drawing chars (├ ┤ ┌ ┐ etc.) require 2-3 bytes and cannot
            // be stored in a char. If UTF-8 support is needed, change ch to std::string.
            return ch == '+' || ch == '-' || ch == '|';
        }
    };

    /**
     * @class test_canvas
     * @brief Virtual text-mode rendering surface for tests
     *
     * @details
     * Acts like VRAM but for testing. Provides:
     * - Character grid with attributes
     * - Border detection
     * - Text pattern matching
     * - ASCII rendering for debugging
     *
     * @example
     * @code
     * test_canvas canvas(80, 25);
     * canvas.put(0, 0, '+');
     * canvas.put(1, 0, '-');
     *
     * CHECK(canvas.has_border_at(0, 0));
     * auto text_pos = canvas.find_text("Hello");
     * INFO(canvas.render_ascii());  // Visual debugging
     * @endcode
     */
    class test_canvas {
    public:
        /**
         * @brief Construct canvas with dimensions
         */
        explicit test_canvas(int width, int height)
            : m_width(width)
            , m_height(height)
            , m_cells(static_cast<size_t>(width) * static_cast<size_t>(height))
        {
        }

        /**
         * @brief Get canvas dimensions
         */
        [[nodiscard]] int width() const { return m_width; }
        [[nodiscard]] int height() const { return m_height; }

        /**
         * @brief Put character at position
         */
        void put(int x, int y, char ch, uint8_t fg = 7, uint8_t bg = 0, uint8_t attrs = 0) {
            if (x < 0 || x >= m_width || y < 0 || y >= m_height) return;

            auto& cell = m_cells[static_cast<size_t>(y) * static_cast<size_t>(m_width) + static_cast<size_t>(x)];
            cell.ch = ch;
            cell.fg_color = fg;
            cell.bg_color = bg;
            cell.attrs = attrs;
        }

        /**
         * @brief Get cell at position
         */
        [[nodiscard]] const canvas_cell& get(int x, int y) const {
            static const canvas_cell empty;
            if (x < 0 || x >= m_width || y < 0 || y >= m_height) return empty;
            return m_cells[static_cast<size_t>(y) * static_cast<size_t>(m_width) + static_cast<size_t>(x)];
        }

        /**
         * @brief Get character at position
         */
        [[nodiscard]] char get_char(int x, int y) const {
            return get(x, y).ch;
        }

        /**
         * @brief Check if position has border character
         */
        [[nodiscard]] bool has_border_at(int x, int y) const {
            return get(x, y).is_border();
        }

        /**
         * @brief Check if position is empty
         */
        [[nodiscard]] bool is_empty_at(int x, int y) const {
            return get(x, y).is_empty();
        }

        /**
         * @brief Find text on canvas (returns first occurrence)
         */
        [[nodiscard]] std::optional<std::pair<int, int>> find_text(const std::string& text) const {
            for (int y = 0; y < m_height; ++y) {
                for (int x = 0; x <= m_width - static_cast<int>(text.length()); ++x) {
                    bool match = true;
                    for (size_t i = 0; i < text.length(); ++i) {
                        if (get_char(x + static_cast<int>(i), y) != text[i]) {
                            match = false;
                            break;
                        }
                    }
                    if (match) return std::make_pair(x, y);
                }
            }
            return std::nullopt;
        }

        /**
         * @brief Get text at position (reads up to length or newline)
         */
        [[nodiscard]] std::string get_text(int x, int y, int max_length) const {
            std::string result;
            for (int i = 0; i < max_length && x + i < m_width; ++i) {
                char ch = get_char(x + i, y);
                if (ch == '\0' || ch == '\n') break;
                result += ch;
            }
            return result;
        }

        /**
         * @brief Count border characters in rectangle
         */
        [[nodiscard]] int count_borders_in_rect(int x, int y, int w, int h) const {
            int count = 0;
            for (int dy = 0; dy < h; ++dy) {
                for (int dx = 0; dx < w; ++dx) {
                    if (has_border_at(x + dx, y + dy)) count++;
                }
            }
            return count;
        }

        /**
         * @brief Check if rectangle has complete border
         */
        [[nodiscard]] bool has_complete_border(int x, int y, int w, int h) const {
            if (w < 2 || h < 2) return false;

            // Check corners
            if (!has_border_at(x, y)) return false;           // Top-left
            if (!has_border_at(x + w - 1, y)) return false;   // Top-right
            if (!has_border_at(x, y + h - 1)) return false;   // Bottom-left
            if (!has_border_at(x + w - 1, y + h - 1)) return false; // Bottom-right

            // Check edges
            for (int i = 1; i < w - 1; ++i) {
                if (!has_border_at(x + i, y)) return false;       // Top edge
                if (!has_border_at(x + i, y + h - 1)) return false; // Bottom edge
            }
            for (int i = 1; i < h - 1; ++i) {
                if (!has_border_at(x, y + i)) return false;       // Left edge
                if (!has_border_at(x + w - 1, y + i)) return false; // Right edge
            }

            return true;
        }

        /**
         * @brief Render canvas as ASCII art (for debugging)
         */
        [[nodiscard]] std::string render_ascii() const {
            std::ostringstream oss;
            for (int y = 0; y < m_height; ++y) {
                for (int x = 0; x < m_width; ++x) {
                    oss << get_char(x, y);
                }
                oss << '\n';
            }
            return oss.str();
        }

        /**
         * @brief Clear entire canvas
         */
        void clear() {
            for (auto& cell : m_cells) {
                cell = canvas_cell{};
            }
        }

        /**
         * @brief Get row as string (for debugging)
         */
        [[nodiscard]] std::string get_row(int y) const {
            std::string result;
            if (y >= 0 && y < m_height) {
                for (int x = 0; x < m_width; ++x) {
                    result += get_char(x, y);
                }
            }
            return result;
        }

        /**
         * @brief Semantic assertion: Check if text exists at exact position
         * @param x Starting x coordinate
         * @param y Y coordinate (row)
         * @param text Expected text string
         * @return true if text matches exactly at position
         *
         * @details
         * This is a SEMANTIC assertion - tests BEHAVIOR not implementation.
         * Example: CHECK(canvas.has_text_at(0, 0, "Window Title"));
         */
        [[nodiscard]] bool has_text_at(int x, int y, const std::string& text) const {
            if (x < 0 || y < 0 || y >= m_height) return false;
            if (x + static_cast<int>(text.length()) > m_width) return false;

            for (size_t i = 0; i < text.length(); ++i) {
                if (get_char(x + static_cast<int>(i), y) != text[i]) {
                    return false;
                }
            }
            return true;
        }

        /**
         * @brief Semantic assertion: Check if there's non-empty content at position
         * @details Use this instead of hardcoding expected characters
         */
        [[nodiscard]] bool has_content_at(int x, int y) const {
            return !is_empty_at(x, y);
        }

        /**
         * @brief Semantic assertion: Check if horizontal line of borders exists
         * @param x Starting x coordinate
         * @param y Y coordinate
         * @param length Length of line to check
         * @return true if all positions in line have border characters
         */
        [[nodiscard]] bool has_horizontal_border_line(int x, int y, int length) const {
            for (int i = 0; i < length; ++i) {
                if (!has_border_at(x + i, y)) return false;
            }
            return true;
        }

        /**
         * @brief Semantic assertion: Check if vertical line of borders exists
         * @param x X coordinate
         * @param y Starting y coordinate
         * @param length Length of line to check
         * @return true if all positions in line have border characters
         */
        [[nodiscard]] bool has_vertical_border_line(int x, int y, int length) const {
            for (int i = 0; i < length; ++i) {
                if (!has_border_at(x, y + i)) return false;
            }
            return true;
        }

        /**
         * @brief Semantic assertion: Check if rectangle has ANY border elements
         * @details Less strict than has_complete_border - just checks if borders exist
         */
        [[nodiscard]] bool has_some_border_in_rect(int x, int y, int w, int h) const {
            return count_borders_in_rect(x, y, w, h) > 0;
        }

    private:
        int m_width;
        int m_height;
        std::vector<canvas_cell> m_cells;
    };

} // namespace onyxui::testing
