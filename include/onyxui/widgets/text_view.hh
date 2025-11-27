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

#include <onyxui/widgets/core/widget_container.hh>
#include <onyxui/widgets/containers/scroll/scroll_view_presets.hh>
#include <onyxui/widgets/containers/scroll/scrollbar_visibility.hh>
#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/layout/linear_layout.hh>
#include <onyxui/concepts/rect_like.hh>
#include <onyxui/hotkeys/hotkey_action.hh>
#include <onyxui/events/event_phase.hh>
#include <string>
#include <vector>
#include <sstream>
#include <exception>
#include <iostream>
#include <limits>

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
     * Supports optional border rendering via set_has_border().
     */
    template<UIBackend Backend>
    class text_view : public widget_container<Backend> {
        using base = widget_container<Backend>;
        using element_type = ui_element<Backend>;
        using size_type = typename Backend::size_type;
        using rect_type = typename Backend::rect_type;
        using scroll_view_type = scroll_view<Backend>;

    public:
        /**
         * @brief Construct empty text view
         * @param parent Optional parent element
         */
        explicit text_view(element_type* parent = nullptr)
            : base(
                std::make_unique<linear_layout<Backend>>(
                    direction::vertical,
                    0,  // No spacing - single child (scroll_view)
                    horizontal_alignment::stretch,
                    vertical_alignment::stretch
                ),
                parent
              )
        {
            // Make text_view itself focusable for keyboard scrolling
            base::set_focusable(true);

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

        /**
         * @brief Set whether to draw a border around the text view
         * @param has_border True to draw border, false for borderless
         *
         * @details
         * Enables or disables border rendering around the text view.
         * When enabled, the border takes up 1 character on each side,
         * reducing the available content area.
         */
        void set_has_border(bool has_border) {
            if (this->m_has_border != has_border) {
                this->m_has_border = has_border;
                // Border affects content area, so invalidate layout
                this->invalidate_measure();
            }
        }

        /**
         * @brief Check if border is enabled
         * @return True if border is drawn, false otherwise
         */
        [[nodiscard]] bool has_border() const noexcept {
            return this->m_has_border;
        }

        /**
         * @brief Set preferred height based on number of visible lines
         * @param lines Number of text lines to display (minimum 1)
         *
         * @details
         * Sets a semantic size constraint where height = lines in logical units.
         * This is a convenience method that makes text_view sizing more intuitive
         * and backend-agnostic.
         *
         * @example
         * @code
         * auto log_view = std::make_unique<text_view<Backend>>();
         * log_view->set_visible_lines(20);  // Show 20 lines of text
         * @endcode
         */
        void set_visible_lines(int lines) {
            if (lines < 1) lines = 1;

            size_constraint height_constraint;
            height_constraint.policy = size_policy::content;
            height_constraint.preferred_size = logical_unit(static_cast<double>(lines));  // For now, 1 line = 1 unit (TUI-friendly)
            this->set_height_constraint(height_constraint);
        }

        /**
         * @brief Set preferred width based on number of visible characters
         * @param chars Number of characters to display horizontally (minimum 1)
         *
         * @details
         * Sets a semantic size constraint where width = chars in logical units.
         * This is a convenience method that makes text_view sizing more intuitive
         * and backend-agnostic.
         *
         * @example
         * @code
         * auto editor = std::make_unique<text_view<Backend>>();
         * editor->set_visible_chars(80);  // Classic 80-column width
         * @endcode
         */
        void set_visible_chars(int chars) {
            if (chars < 1) chars = 1;

            size_constraint width_constraint;
            width_constraint.policy = size_policy::content;
            width_constraint.preferred_size = chars;  // For now, 1 char = 1 unit (TUI-friendly)
            this->set_width_constraint(width_constraint);
        }

        /**
         * @brief Handle semantic actions for scrolling
         * @param action The semantic action to handle
         * @return true if action was handled
         *
         * @details
         * Public override to handle keyboard scrolling actions.
         * Dispatches to internal scroll_view for actual scrolling.
         */
        bool handle_semantic_action(hotkey_action action) override {
            if (!m_scroll_view) {
                return base::handle_semantic_action(action);
            }

            switch (action) {
                case hotkey_action::scroll_up:
                    m_scroll_view->scroll_by(0, -1);
                    return true;

                case hotkey_action::scroll_down:
                    m_scroll_view->scroll_by(0, 1);
                    return true;

                case hotkey_action::scroll_page_up: {
                    auto viewport_bounds = m_scroll_view->bounds();
                    int viewport_height = viewport_bounds.height.to_int();
                    m_scroll_view->scroll_by(0, -viewport_height);
                    return true;
                }

                case hotkey_action::scroll_page_down: {
                    auto viewport_bounds = m_scroll_view->bounds();
                    int viewport_height = viewport_bounds.height.to_int();
                    m_scroll_view->scroll_by(0, viewport_height);
                    return true;
                }

                case hotkey_action::scroll_home:
                    m_scroll_view->scroll_to(0, 0);
                    return true;

                case hotkey_action::scroll_end:
                    m_scroll_view->scroll_to(0, std::numeric_limits<int>::max());
                    return true;

                default:
                    return base::handle_semantic_action(action);
            }
        }

        // Bring base class handle_event into scope (avoid hiding it)
        using base::handle_event;

    protected:
        /**
         * @brief Override handle_event to intercept mouse clicks in capture phase
         * @param event The event to handle
         * @param phase The event routing phase (capture/target/bubble)
         * @return true to consume event, false to continue propagation
         *
         * @details
         * Uses capture phase to intercept mouse clicks BEFORE they reach child labels.
         * This solves the focus problem: clicking anywhere inside text_view gives it focus,
         * not just on empty areas.
         *
         * ## Why Capture Phase?
         * Without capture phase, hit-test returns the deepest child (label), and text_view
         * never sees the event. With capture phase, text_view intercepts BEFORE children,
         * can request focus, then lets the event continue to children normally.
         *
         * ## Event Flow Example:
         * 1. User clicks on label inside text_view
         * 2. CAPTURE: text_view gets event first, requests focus, returns false
         * 3. TARGET: label gets event (might not handle it)
         * 4. BUBBLE: event bubbles back up
         *
         * This is why we need three-phase routing!
         */
        bool handle_event(const ui_event& event, event_phase phase) override {
            // Check if this is a mouse event
            if (auto* mouse_evt = std::get_if<mouse_event>(&event)) {
                // Handle mouse button press (for focus)
                if (mouse_evt->act == mouse_event::action::press) {
                    // In CAPTURE phase, request focus before children handle the event
                    if (phase == event_phase::capture) {
                        auto* input = ui_services<Backend>::input();
                        if (input && base::is_focusable()) {
                            input->set_focus(this);
                        }
                    }
                    // Return false in ALL phases to allow event to continue to children
                    // (label might want to handle clicks for selection, etc.)
                    return false;
                }

                // Handle mouse wheel (for scrolling)
                if (mouse_evt->act == mouse_event::action::wheel_up) {
                    // Trigger scroll_up semantic action (scroll up 3 lines)
                    return this->handle_semantic_action(hotkey_action::scroll_up);
                }
                if (mouse_evt->act == mouse_event::action::wheel_down) {
                    // Trigger scroll_down semantic action (scroll down 3 lines)
                    return this->handle_semantic_action(hotkey_action::scroll_down);
                }
            }

            // Pass all other events to base class
            return base::handle_event(event, phase);
        }

        /**
         * @brief Handle focus state change - triggers redraw for border color change
         * @param gained True if focus gained, false if lost
         *
         * @details
         * Invalidates visual to trigger redraw with focused/unfocused border color.
         */
        void on_focus_changed(bool gained) override {
            (void)gained;  // Parameter not used, just need to redraw
            // Invalidate visual to trigger redraw with new border color
            this->invalidate_visual();
            base::on_focus_changed(gained);
        }

        /**
         * @brief Override style resolution to use yellow border when focused
         * @param theme Global theme pointer
         * @param parent_style Parent's resolved style
         * @return Resolved style with yellow border color if focused
         *
         * @details
         * Norton Utilities 8 style focus indicator - yellow foreground when focused.
         * We change foreground_color (not just border_color) because draw_rect()
         * uses foreground_color for the border.
         */
        [[nodiscard]] resolved_style<Backend> resolve_style(
            const ui_theme<Backend>* theme,
            const resolved_style<Backend>& parent_style
        ) const override {
            // Get base resolved style
            auto style = base::resolve_style(theme, parent_style);

            // FOCUS INDICATOR: Change foreground to yellow when focused (NU8 style)
            // draw_rect() uses foreground_color for borders, so we change that
            if (this->has_focus()) {
                // Norton Utilities 8 yellow: RGB(255, 255, 0)
                style.foreground_color = typename Backend::color_type{255, 255, 0};
                style.border_color = typename Backend::color_type{255, 255, 0};
            }

            return style;
        }

    protected:
        /**
         * @brief Override measure to ensure minimum size for always-visible scrollbars
         *
         * @details
         * text_view uses classic_scroll_view with always-visible scrollbars.
         * When has_border() is true, we need: border (2px) + scroll_view min (8px) = 10px minimum.
         * This override ensures text_view requests adequate space even when constrained.
         */
        logical_size do_measure(logical_unit available_width, logical_unit available_height) override {
            // Let base class measure (handles border + child)
            auto measured = base::do_measure(available_width, available_height);

            // CRITICAL FIX: Enforce minimum size for always-visible scrollbars
            // Get scrollbar dimensions from theme
            auto const* themes = ui_services<Backend>::themes();
            auto const* theme = themes ? themes->get_current_theme() : nullptr;
            int const min_scrollbar_length = theme ? theme->scrollbar.min_render_size : ui_constants::DEFAULT_SCROLLBAR_MIN_RENDER_SIZE;
            int const scrollbar_thickness = theme ? theme->scrollbar.width : ui_constants::DEFAULT_SCROLLBAR_THICKNESS;

            const int border_size = this->has_border() ? 2 : 0;

            // Minimum height: border + minimum scrollbar length (horizontal scrollbar height)
            const int min_height = border_size + min_scrollbar_length;

            // Minimum width: border + some content + vertical scrollbar width
            // Need at least min_scrollbar_length for content + scrollbar_thickness for vertical scrollbar
            const int min_width = border_size + min_scrollbar_length + scrollbar_thickness;

            if (measured.height.to_int() < min_height) {
                measured.height = logical_unit(static_cast<double>(min_height));
            }

            if (measured.width.to_int() < min_width) {
                measured.width = logical_unit(static_cast<double>(min_width));
            }

            return measured;
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

            // Only show vertical scrollbar (horizontal is hidden since text wraps)
            scroll_view_ptr->set_scrollbar_policy(
                scrollbar_visibility::hidden,  // Horizontal: hidden
                scrollbar_visibility::always   // Vertical: always visible
            );

            // Create content container
            auto content_container_ptr = std::make_unique<panel<Backend>>();

            // Set linear layout for vertical stacking
            content_container_ptr->set_layout_strategy(
                std::make_unique<linear_layout<Backend>>(
                    direction::vertical,
                    0,  // No spacing between lines
                    horizontal_alignment::left,
                    vertical_alignment::top));

            content_container_ptr->set_padding(logical_thickness(1_lu));  // Small padding

            // Add all labels to content before adding to scroll view
            for (const auto& line : m_lines) {
                auto label_widget = std::make_unique<label<Backend>>(line);
                content_container_ptr->add_child(std::move(label_widget));
            }

            // Add content to scroll view WITHOUT pre-arrange
            // The scrollable will arrange children when needed
            scroll_view_ptr->add_child(std::move(content_container_ptr));

            // Set alignment and size policy on scroll view
            scroll_view_ptr->set_horizontal_align(horizontal_alignment::stretch);
            scroll_view_ptr->set_vertical_align(vertical_alignment::stretch);

            // CRITICAL FIX: Use fill_parent policy so scroll_view fills text_view's height
            // vertical_alignment::stretch doesn't work in vertical layouts!
            onyxui::size_constraint height_policy;
            height_policy.policy = onyxui::size_policy::fill_parent;
            scroll_view_ptr->set_height_constraint(height_policy);

            // NOTE: Don't trigger measurement or scroll_to here
            // The scrollable will be measured and arranged during the normal layout flow
            // Scroll position defaults to {0,0} which is correct for showing the top

            // Store pointer to scroll_view before moving
            m_scroll_view = scroll_view_ptr.get();

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

        // =====================================================================
        // Member Variables
        // =====================================================================

        std::vector<std::string> m_lines;
        scroll_view_type* m_scroll_view = nullptr;  ///< Non-owning pointer to internal scroll_view
    };

} // namespace onyxui
