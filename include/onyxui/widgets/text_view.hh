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
            : base(parent)
        {
            // Make text_view itself focusable for keyboard scrolling
            base::set_focusable(true);

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
                    int viewport_height = rect_utils::get_height(viewport_bounds);
                    m_scroll_view->scroll_by(0, -viewport_height);
                    return true;
                }

                case hotkey_action::scroll_page_down: {
                    auto viewport_bounds = m_scroll_view->bounds();
                    int viewport_height = rect_utils::get_height(viewport_bounds);
                    m_scroll_view->scroll_by(0, viewport_height);
                    return true;
                }

                case hotkey_action::scroll_home:
                    m_scroll_view->scroll_to(0, 0);
                    return true;

                case hotkey_action::scroll_end:
                    m_scroll_view->scroll_to(0, 999999);
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
            // Only intercept in CAPTURE phase (before children)
            if (phase == event_phase::capture) {
                // Check if this is a mouse button press
                if (auto* mouse_evt = std::get_if<mouse_event>(&event)) {
                    if (mouse_evt->act == mouse_event::action::press) {
                        // Request focus for text_view when any mouse button is pressed inside it
                        auto* input = ui_services<Backend>::input();
                        if (input && base::is_focusable()) {
                            input->set_focus(this);
                        }
                        // Return false to allow event to continue to children
                        // (label might want to handle clicks for selection, etc.)
                        return false;
                    }
                }
            }

            // Pass all other phases to base class
            return base::handle_event(event, phase);
        }

        /**
         * @brief Handle focus gained - triggers redraw for border color change
         * @return true if handled
         */
        bool handle_focus_gained() override {
            // Invalidate visual to trigger redraw with new border color
            this->invalidate_visual();
            return base::handle_focus_gained();
        }

        /**
         * @brief Handle focus lost - triggers redraw for border color change
         * @return true if handled
         */
        bool handle_focus_lost() override {
            // Invalidate visual to trigger redraw with normal border color
            this->invalidate_visual();
            return base::handle_focus_lost();
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

    private:
        /**
         * @brief Create scroll view with content
         */
        void create_scroll_view() {
            // Clear any existing children
            base::clear_children();

            // Create scroll view with always-visible scrollbars
            auto scroll_view_ptr = classic_scroll_view<Backend>();

            // Explicitly ensure scrollbars are always visible
            scroll_view_ptr->set_scrollbar_policy(scrollbar_visibility::always);

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
