/**
 * @file text_view.hh
 * @brief Multi-line text display widget with automatic scrolling
 * @author OnyxUI Framework
 * @date 2025-11-01
 *
 * @details
 * The text_view widget provides a scrollable multi-line text display.
 * Perfect for logs, file viewers, help text, and read-only documents.
 *
 * ## Features
 * - Automatic line splitting on newlines
 * - Built-in scrolling with keyboard navigation
 * - Configurable scrollbar visibility
 * - Theme integration for text colors
 * - Efficient rendering (only visible lines)
 *
 * ## Usage Example
 *
 * ```cpp
 * auto text_view = std::make_unique<text_view<Backend>>();
 *
 * // Set multi-line content
 * text_view->set_text(
 *     "Line 1\n"
 *     "Line 2\n"
 *     "Line 3"
 * );
 *
 * // Or append for logs
 * text_view->append_line("New log entry");
 *
 * // Scroll to bottom (useful for logs)
 * text_view->scroll_to_bottom();
 * ```
 */

#pragma once

#include <onyxui/widgets/core/widget.hh>
#include <onyxui/widgets/containers/scroll/scroll_view_presets.hh>
#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/layout/linear_layout.hh>
#include <string>
#include <vector>
#include <sstream>

namespace onyxui {

    /**
     * @class text_view
     * @brief Scrollable multi-line text display widget
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Provides a read-only text display with automatic scrolling.
     * Uses scroll_view internally for efficient viewport-based rendering.
     */
    template<UIBackend Backend>
    class text_view : public widget<Backend> {
        using base = widget<Backend>;
        using element_type = ui_element<Backend>;
        using size_type = typename Backend::size_type;
        using rect_type = typename Backend::rect_type;

    public:
        /**
         * @brief Construct empty text view
         * @param parent Optional parent element
         */
        explicit text_view(element_type* parent = nullptr)
            : base(parent)
        {
            // Set layout strategy to measure single child (scroll_view)
            base::set_layout_strategy(
                std::make_unique<linear_layout<Backend>>(
                    direction::vertical, 0,
                    horizontal_alignment::stretch,
                    vertical_alignment::stretch));

            // Create scroll view with always-visible scrollbars
            auto scroll_view_ptr = classic_scroll_view<Backend>();

            // Create content container
            auto content_container_ptr = std::make_unique<panel<Backend>>();

            // Set linear layout for vertical stacking
            content_container_ptr->set_layout_strategy(
                std::make_unique<linear_layout<Backend>>(
                    direction::vertical,
                    0,  // No spacing between lines
                    horizontal_alignment::left,
                    vertical_alignment::top));

            content_container_ptr->set_padding(thickness::all(1));  // Small padding

            // Store raw pointer before moving ownership
            m_content_panel = content_container_ptr.get();

            // Add content to scroll view
            scroll_view_ptr->add_child(std::move(content_container_ptr));

            // Set alignment on scroll view
            scroll_view_ptr->set_horizontal_align(horizontal_alignment::stretch);
            scroll_view_ptr->set_vertical_align(vertical_alignment::stretch);

            // Add scroll view as child of this widget
            base::add_child(std::move(scroll_view_ptr));
        }

        /**
         * @brief Set text content (splits on newlines)
         * @param text Multi-line text (use \n for line breaks)
         */
        void set_text(const std::string& text) {
            // Split text into lines
            std::vector<std::string> lines;
            std::istringstream stream(text);
            std::string line;

            while (std::getline(stream, line)) {
                lines.push_back(line);
            }

            set_lines(std::move(lines));
        }

        /**
         * @brief Set pre-split lines
         * @param lines Vector of text lines
         */
        void set_lines(std::vector<std::string> lines) {
            m_lines = std::move(lines);
            rebuild_content();
        }

        /**
         * @brief Append a single line
         * @param line Text line to append
         */
        void append_line(const std::string& line) {
            m_lines.push_back(line);

            // Create and add label for new line
            auto label_widget = std::make_unique<label<Backend>>(line);
            label_widget->set_horizontal_align(horizontal_alignment::left);
            m_content_panel->add_child(std::move(label_widget));

            // Invalidate layout
            base::invalidate_measure();
        }

        /**
         * @brief Append text (splits on newlines)
         * @param text Text to append
         */
        void append_text(const std::string& text) {
            std::istringstream stream(text);
            std::string line;

            while (std::getline(stream, line)) {
                append_line(line);
            }
        }

        /**
         * @brief Clear all content
         */
        void clear() {
            m_lines.clear();
            m_content_panel->clear_children();
            base::invalidate_measure();
        }

        /**
         * @brief Get number of lines
         * @return Line count
         */
        [[nodiscard]] size_t line_count() const noexcept {
            return m_lines.size();
        }

        /**
         * @brief Get line by index
         * @param index Line index (0-based)
         * @return Line text (or empty if out of range)
         */
        [[nodiscard]] std::string get_line(size_t index) const {
            if (index < m_lines.size()) {
                return m_lines[index];
            }
            return "";
        }

        /**
         * @brief Get all lines
         * @return Reference to lines vector
         */
        [[nodiscard]] const std::vector<std::string>& lines() const noexcept {
            return m_lines;
        }

    private:
        /**
         * @brief Rebuild all content labels
         */
        void rebuild_content() {
            if (!m_content_panel) {
                return;
            }

            // Clear existing labels
            m_content_panel->clear_children();

            // Create label for each line
            for (const auto& line : m_lines) {
                auto label_widget = std::make_unique<label<Backend>>(line);
                m_content_panel->add_child(std::move(label_widget));
            }

            // Force complete re-measurement of the hierarchy
            // Invalidate from bottom up
            m_content_panel->invalidate_measure();

            // Find and invalidate the scrollable parent
            if (m_content_panel->parent()) {
                m_content_panel->parent()->invalidate_measure();
            }

            // Invalidate our own layout
            base::invalidate_measure();
        }

        std::vector<std::string> m_lines;
        widget<Backend>* m_content_panel = nullptr;  // Non-owning pointer to content container (vbox) inside scroll_view
    };

} // namespace onyxui
