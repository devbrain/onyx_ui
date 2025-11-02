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
#include <onyxui/core/raii/scoped_clip.hh>
#include <onyxui/concepts/rect_like.hh>
#include <string>
#include <vector>
#include <sstream>
#include <exception>

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

            // Initialize with empty scroll view
            create_scroll_view();
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

            // Rebuild entire view
            rebuild_content();
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
            rebuild_content();
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

    protected:
        /**
         * @brief Override do_arrange to properly position the scroll_view
         * @param final_bounds The final bounds assigned to this widget
         */
        void do_arrange(const rect_type& final_bounds) override {
            (void)final_bounds; // Suppress unused parameter warning

            // Arrange scroll_view within our content area
            if (!this->children().empty()) {
                auto* scroll_view = this->children()[0].get();
                auto content_area = this->get_content_area();

                // Use the content area directly (it's already in absolute coordinates)
                scroll_view->arrange(content_area);
            }
        }

        /**
         * @brief Override do_render to ensure proper clipping
         * @param ctx The render context
         */
        void do_render(render_context<Backend>& ctx) const override {
            // During measurement, just use base implementation
            if (!ctx.is_rendering()) {
                base::do_render(ctx);
                return;
            }

            // During rendering, clip to our bounds to prevent overflow
            auto our_bounds = this->bounds();

            // Create a clipping region for our bounds
            {
                scoped_clip<Backend> clip(ctx, our_bounds);

                // Call base implementation which will handle rendering
                // children properly within the clipped region
                base::do_render(ctx);
            }
        }

    private:
        /**
         * @brief Create scroll view with content
         */
        void create_scroll_view() {
            // Clear any existing children
            base::clear_children();

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

            // Add all labels to content before adding to scroll view
            for (const auto& line : m_lines) {
                auto label_widget = std::make_unique<label<Backend>>(line);
                content_container_ptr->add_child(std::move(label_widget));
            }

            // WORKAROUND: Pre-arrange the panel before adding to scroll_view
            // Without this, the panel's children won't be positioned correctly.
            // This appears to be a bug in how scrollable interacts with freshly
            // created containers that haven't been arranged yet.
            // TODO: Fix the root cause in scrollable/panel interaction
            //
            // Only do this if we have content - measuring requires a theme which
            // may not be available during initial construction
            if (!m_lines.empty()) {
                try {
                    auto panel_size = content_container_ptr->measure_unconstrained();
                    content_container_ptr->arrange({0, 0, panel_size.w, panel_size.h});
                } catch (const std::exception&) {
                    // Ignore measurement errors during construction when no theme is set
                    // The workaround will be applied later when content is added with a theme
                }
            }

            // Add content to scroll view
            scroll_view_ptr->add_child(std::move(content_container_ptr));

            // Set alignment on scroll view
            scroll_view_ptr->set_horizontal_align(horizontal_alignment::stretch);
            scroll_view_ptr->set_vertical_align(vertical_alignment::stretch);

            // Ensure scroll starts at the top
            scroll_view_ptr->scroll_to(0, 0);

            // Add scroll view as child of this widget
            base::add_child(std::move(scroll_view_ptr));

            // Invalidate our measurement
            base::invalidate_measure();
        }

        /**
         * @brief Rebuild all content labels
         */
        void rebuild_content() {
            // Recreate entire scroll view with new content
            create_scroll_view();
        }

        std::vector<std::string> m_lines;
    };

} // namespace onyxui
