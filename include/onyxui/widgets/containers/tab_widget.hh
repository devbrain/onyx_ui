/**
 * @file tab_widget.hh
 * @brief Tab widget - Multi-page container with tabbed navigation
 *
 * @details
 * A tab widget provides multiple pages with clickable tabs for navigation.
 * Only one page is visible at a time, with tabs displayed at the top (default)
 * or other positions.
 *
 * ## Visual Appearance (top tabs)
 *
 * ```
 * ┌───────┬───────┬───────┐
 * │ Tab 1 │ Tab 2 │ Tab 3 │
 * ├───────┴───────┴───────┴────────┐
 * │                                │
 * │   Content of current tab       │
 * │                                │
 * └────────────────────────────────┘
 * ```
 *
 * ## Features
 *
 * - **Tab Management**: Add, insert, remove tabs dynamically
 * - **Tab Selection**: Set current tab by index, next/previous navigation
 * - **Close Buttons**: Optional per-tab close buttons (signal-based)
 * - **Tab Positioning**: Top, bottom, left, right tab bar placement
 * - **Keyboard Navigation**: Ctrl+Tab, Ctrl+Shift+Tab, Ctrl+W
 * - **Signals**: current_changed, tab_close_requested
 *
 * ## Usage Example
 *
 * ```cpp
 * auto tabs = std::make_unique<tab_widget<Backend>>();
 *
 * auto page1 = std::make_unique<label<Backend>>("Page 1 Content");
 * auto page2 = std::make_unique<label<Backend>>("Page 2 Content");
 *
 * tabs->add_tab(std::move(page1), "First");
 * tabs->add_tab(std::move(page2), "Second");
 *
 * tabs->current_changed.connect([](int idx) {
 *     std::cout << "Switched to tab " << idx << "\n";
 * });
 * ```
 */

#pragma once

#include "onyxui/core/rendering/render_context.hh"
#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/events/ui_event.hh>
#include <onyxui/concepts/point_like.hh>
#include <onyxui/concepts/size_like.hh>
#include <string>
#include <vector>
#include <memory>

namespace onyxui {

    /**
     * @enum tab_position
     * @brief Position of the tab bar relative to content
     */
    enum class tab_position {
        top,    ///< Tabs above content (default)
        bottom, ///< Tabs below content
        left,   ///< Tabs to left of content
        right   ///< Tabs to right of content
    };

    /**
     * @class tab_widget
     * @brief Multi-page container with tabbed navigation
     *
     * @tparam Backend The UI backend type
     */
    template<UIBackend Backend>
    class tab_widget : public panel<Backend> {
    public:
        using base = panel<Backend>;
        using size_type = typename Backend::size_type;
        using rect_type = typename Backend::rect_type;

        // ===================================================================
        // Construction
        // ===================================================================

        /**
         * @brief Construct an empty tab widget
         * @param parent Parent element
         */
        explicit tab_widget(ui_element<Backend>* parent = nullptr)
            : base(parent)
        {
            // Tab widgets have a border by default
            this->set_has_border(true);
        }

        ~tab_widget() override = default;

        // Rule of Five
        tab_widget(const tab_widget&) = delete;
        tab_widget& operator=(const tab_widget&) = delete;
        tab_widget(tab_widget&&) noexcept = default;
        tab_widget& operator=(tab_widget&&) noexcept = default;

        // ===================================================================
        // Tab Management
        // ===================================================================

        /**
         * @brief Add a new tab at the end
         * @param widget Widget to show when tab is active
         * @param label Tab button text
         * @return Index of the new tab
         */
        int add_tab(std::unique_ptr<ui_element<Backend>> widget, const std::string& label) {
            return insert_tab(static_cast<int>(m_tabs.size()), std::move(widget), label);
        }

        /**
         * @brief Insert a tab at a specific position
         * @param index Position to insert (0 = beginning)
         * @param widget Widget to show when tab is active
         * @param label Tab button text
         * @return Actual index where tab was inserted
         */
        int insert_tab(int index, std::unique_ptr<ui_element<Backend>> widget, const std::string& label) {
            if (index < 0) index = 0;
            if (index > static_cast<int>(m_tabs.size())) {
                index = static_cast<int>(m_tabs.size());
            }

            tab_info info;
            info.label = label;
            info.widget = std::move(widget);
            info.closeable = true;

            // Add widget as child
            if (info.widget) {
                this->add_child(std::unique_ptr<ui_element<Backend>>(info.widget.release()));
                info.widget = nullptr;  // Ownership transferred
            }

            m_tabs.insert(m_tabs.begin() + index, std::move(info));

            // If this is the first tab, make it current
            if (m_current_index < 0) {
                m_current_index = 0;
            } else if (m_current_index >= index) {
                // Adjust current index if we inserted before it
                m_current_index++;
            }

            // Always update visibility to hide non-current tabs
            update_visibility();
            this->invalidate_measure();
            return index;
        }

        /**
         * @brief Remove a tab by index
         * @param index Tab index to remove
         */
        void remove_tab(int index) {
            if (index < 0 || index >= static_cast<int>(m_tabs.size())) {
                return;
            }

            // Remove the child widget
            auto& children = this->children();
            if (index < static_cast<int>(children.size())) {
                (void)this->remove_child(children[static_cast<std::size_t>(index)].get());
            }

            m_tabs.erase(m_tabs.begin() + index);

            // Adjust current index
            if (m_tabs.empty()) {
                m_current_index = -1;
            } else if (m_current_index >= static_cast<int>(m_tabs.size())) {
                set_current_index(static_cast<int>(m_tabs.size()) - 1);
            } else if (m_current_index > index) {
                m_current_index--;
            }

            this->invalidate_measure();
        }

        /**
         * @brief Remove all tabs
         */
        void clear() {
            m_tabs.clear();
            this->clear_children();
            m_current_index = -1;
            this->invalidate_measure();
        }

        /**
         * @brief Get number of tabs
         * @return Tab count
         */
        [[nodiscard]] int count() const noexcept {
            return static_cast<int>(m_tabs.size());
        }

        // ===================================================================
        // Tab Selection
        // ===================================================================

        /**
         * @brief Set the current (visible) tab
         * @param index Tab index to make current
         */
        void set_current_index(int index) {
            if (index < 0 || index >= static_cast<int>(m_tabs.size())) {
                return;
            }
            if (m_current_index == index) {
                return;
            }

            m_current_index = index;
            ensure_tab_visible(index);  // Auto-scroll to make selected tab visible
            update_visibility();
            current_changed.emit(m_current_index);
            this->invalidate_arrange();
        }

        /**
         * @brief Get the current tab index
         * @return Current tab index, or -1 if no tabs
         */
        [[nodiscard]] int current_index() const noexcept {
            return m_current_index;
        }

        /**
         * @brief Get the current tab's widget
         * @return Widget pointer, or nullptr if no tabs
         */
        [[nodiscard]] ui_element<Backend>* current_widget() const {
            if (m_current_index < 0 || m_current_index >= static_cast<int>(this->children().size())) {
                return nullptr;
            }
            return this->children()[static_cast<std::size_t>(m_current_index)].get();
        }

        /**
         * @brief Switch to next tab (wraps around)
         */
        void next_tab() {
            if (m_tabs.empty()) return;
            int next = (m_current_index + 1) % static_cast<int>(m_tabs.size());
            set_current_index(next);
        }

        /**
         * @brief Switch to previous tab (wraps around)
         */
        void previous_tab() {
            if (m_tabs.empty()) return;
            int prev = m_current_index - 1;
            if (prev < 0) prev = static_cast<int>(m_tabs.size()) - 1;
            set_current_index(prev);
        }

        // ===================================================================
        // Tab Properties
        // ===================================================================

        /**
         * @brief Set tab label text
         * @param index Tab index
         * @param text New label text
         */
        void set_tab_text(int index, const std::string& text) {
            if (index >= 0 && index < static_cast<int>(m_tabs.size())) {
                m_tabs[static_cast<std::size_t>(index)].label = text;
                this->invalidate_arrange();
            }
        }

        /**
         * @brief Get tab label text
         * @param index Tab index
         * @return Label text, or empty string if invalid index
         */
        [[nodiscard]] std::string tab_text(int index) const {
            if (index >= 0 && index < static_cast<int>(m_tabs.size())) {
                return m_tabs[static_cast<std::size_t>(index)].label;
            }
            return "";
        }

        /**
         * @brief Get widget at tab index
         * @param index Tab index
         * @return Widget pointer, or nullptr if invalid index
         */
        [[nodiscard]] ui_element<Backend>* widget(int index) const {
            if (index >= 0 && index < static_cast<int>(this->children().size())) {
                return this->children()[static_cast<std::size_t>(index)].get();
            }
            return nullptr;
        }

        // ===================================================================
        // Tab Bar Configuration
        // ===================================================================

        /**
         * @brief Set tab bar position
         * @param position Where to place the tab bar
         */
        void set_tab_position(tab_position position) {
            if (m_tab_position != position) {
                m_tab_position = position;
                this->invalidate_measure();
            }
        }

        /**
         * @brief Get tab bar position
         * @return Current tab bar position
         */
        [[nodiscard]] tab_position get_tab_position() const noexcept {
            return m_tab_position;
        }

        // ===================================================================
        // Close Buttons
        // ===================================================================

        /**
         * @brief Enable/disable close buttons on all tabs
         * @param closable True to show close buttons
         */
        void set_tabs_closable(bool closable) {
            m_tabs_closable = closable;
            this->invalidate_arrange();
        }

        /**
         * @brief Check if tabs have close buttons
         * @return True if close buttons are shown
         */
        [[nodiscard]] bool tabs_closable() const noexcept {
            return m_tabs_closable;
        }

        /**
         * @brief Set whether a specific tab can be closed
         * @param index Tab index
         * @param closeable True if tab can be closed
         */
        void set_tab_closeable(int index, bool closeable) {
            if (index >= 0 && index < static_cast<int>(m_tabs.size())) {
                m_tabs[static_cast<std::size_t>(index)].closeable = closeable;
            }
        }

        /**
         * @brief Check if a specific tab can be closed
         * @param index Tab index
         * @return True if tab can be closed
         */
        [[nodiscard]] bool is_tab_closeable(int index) const {
            if (index >= 0 && index < static_cast<int>(m_tabs.size())) {
                return m_tabs[static_cast<std::size_t>(index)].closeable;
            }
            return false;
        }

        // ===================================================================
        // Tab Overflow Scrolling
        // ===================================================================

        /**
         * @brief Scroll tabs left (show previous tabs)
         */
        void scroll_left() {
            if (m_scroll_offset > 0) {
                m_scroll_offset--;
                this->invalidate_arrange();
            }
        }

        /**
         * @brief Scroll tabs right (show next tabs)
         */
        void scroll_right() {
            if (can_scroll_right()) {
                m_scroll_offset++;
                this->invalidate_arrange();
            }
        }

        /**
         * @brief Check if can scroll left
         * @return True if there are hidden tabs to the left
         */
        [[nodiscard]] bool can_scroll_left() const noexcept {
            return m_scroll_offset > 0;
        }

        /**
         * @brief Check if can scroll right
         * @return True if there are hidden tabs to the right
         */
        [[nodiscard]] bool can_scroll_right() const noexcept {
            return m_has_overflow && m_scroll_offset < static_cast<int>(m_tabs.size()) - 1;
        }

        /**
         * @brief Get the scroll offset (first visible tab index)
         * @return Scroll offset
         */
        [[nodiscard]] int scroll_offset() const noexcept {
            return m_scroll_offset;
        }

        /**
         * @brief Check if tabs are overflowing
         * @return True if there are more tabs than can fit
         */
        [[nodiscard]] bool has_overflow() const noexcept {
            return m_has_overflow;
        }

        /**
         * @brief Get the tab bar height based on theme
         * @return Tab bar height in rows/pixels
         */
        [[nodiscard]] int get_tab_bar_height() const {
            // Tab bar is 1 row for text + vertical padding
            // For TUI, this is typically 1 row
            return 1;
        }

        // ===================================================================
        // Signals
        // ===================================================================

        /**
         * @brief Emitted when current tab changes
         * @param index New current tab index
         */
        signal<int> current_changed;

        /**
         * @brief Emitted when close button clicked
         * @param index Tab index to close
         * @note Does not automatically remove tab - application must call remove_tab()
         */
        signal<int> tab_close_requested;

    protected:
        /**
         * @brief Render the tab widget
         */
        void do_render(render_context<Backend>& ctx) const override {
            auto* theme = ctx.theme();
            if (!theme || m_tabs.empty()) {
                base::do_render(ctx);
                return;
            }

            const auto& tab_style = theme->tab_widget;
            const auto& pos = ctx.position();
            int x = point_utils::get_x(pos);
            int y = point_utils::get_y(pos);

            // Helper to truncate label if it exceeds max length
            auto truncate_label = [](const std::string& label, int max_chars) -> std::string {
                if (max_chars <= 0 || static_cast<int>(label.length()) <= max_chars) {
                    return label;
                }
                if (max_chars <= 2) {
                    return label.substr(0, static_cast<std::size_t>(max_chars));
                }
                return label.substr(0, static_cast<std::size_t>(max_chars - 2)) + "..";
            };

            // Calculate available width for tabs
            auto content_bounds = this->bounds();
            int total_width = rect_utils::get_width(content_bounds);
            int content_height = rect_utils::get_height(content_bounds);

            // Calculate total width needed for all tabs
            int total_tabs_width = 0;
            int const max_label_len = tab_style.min_tab_width - 2;

            // Measure close icon width from backend
            auto close_icon_size = Backend::renderer_type::get_icon_size(tab_style.close_button_icon);
            m_close_icon_width = size_utils::get_width(close_icon_size);

            for (const auto& tab : m_tabs) {
                std::string display_label = truncate_label(tab.label, max_label_len);
                int tab_width = 2 + static_cast<int>(display_label.length());  // " label "
                if (m_tabs_closable && tab.closeable) {
                    tab_width += m_close_icon_width;
                }
                total_tabs_width += tab_width + tab_style.tab_spacing;
            }

            // Determine if we need scroll arrows
            int const arrow_width = tab_style.scroll_arrow_width;
            m_has_overflow = (total_tabs_width > total_width);

            // Draw tab bar based on position
            auto draw_tabs_horizontal = [&](int tab_y) {
                // Store relative Y for hit testing
                m_tab_bar_y = tab_y - rect_utils::get_y(content_bounds);
                int tab_x = x;

                // Draw left scroll arrow if overflow
                if (m_has_overflow) {
                    typename Backend::point_type arrow_pos{tab_x, tab_y};
                    ctx.draw_icon(tab_style.scroll_left_icon, arrow_pos);
                    m_left_arrow_end = tab_x - x + arrow_width;  // Relative
                    tab_x += arrow_width;
                } else {
                    m_left_arrow_end = 0;
                }

                int tabs_end_x = x + (m_has_overflow ? total_width - arrow_width : total_width);

                // Draw visible tabs starting from scroll_offset
                for (auto i = static_cast<std::size_t>(m_scroll_offset); i < m_tabs.size(); ++i) {
                    auto& tab = m_tabs[i];
                    bool is_active = (static_cast<int>(i) == m_current_index);
                    bool is_hovered = (static_cast<int>(i) == m_hovered_tab_index && !is_active);

                    const auto& fg = is_active ? tab_style.tab_active_text
                                   : is_hovered ? tab_style.tab_hover_text
                                   : tab_style.tab_normal_text;
                    const auto& font = is_active ? tab_style.tab_active_font : tab_style.tab_font;

                    // Truncate very long labels
                    std::string display_label = truncate_label(tab.label, max_label_len);
                    std::string text = " " + display_label + " ";

                    int close_icon_x = -1;
                    if (m_tabs_closable && tab.closeable) {
                        close_icon_x = tab_x + static_cast<int>(text.length());  // Absolute X for icon
                        tab.close_x = close_icon_x - x;  // Relative for hit testing
                    } else {
                        tab.close_x = -1;
                    }

                    // Check if this tab would overflow
                    int tab_width = static_cast<int>(text.length()) + (close_icon_x >= 0 ? m_close_icon_width : 0);
                    if (tab_x + tab_width > tabs_end_x) {
                        // Mark remaining tabs as not visible
                        tab.x_start = -1;
                        tab.x_end = -1;
                        continue;
                    }

                    tab.x_start = tab_x - x;  // Store relative to widget

                    // Draw tab label text
                    typename Backend::point_type tab_pos{tab_x, tab_y};
                    auto text_size = ctx.draw_text(text, tab_pos, font, fg);
                    int text_end_x = tab_x + size_utils::get_width(text_size);

                    // Draw close button icon if enabled
                    if (close_icon_x >= 0) {
                        typename Backend::point_type icon_pos{close_icon_x, tab_y};
                        ctx.draw_icon(tab_style.close_button_icon, icon_pos);
                        tab_x = close_icon_x + m_close_icon_width + tab_style.tab_spacing;
                    } else {
                        tab_x = text_end_x + tab_style.tab_spacing;
                    }
                    tab.x_end = tab_x - tab_style.tab_spacing - x;  // Store relative
                }

                // Mark tabs before scroll_offset as not visible
                for (int i = 0; i < m_scroll_offset && i < static_cast<int>(m_tabs.size()); ++i) {
                    m_tabs[static_cast<std::size_t>(i)].x_start = -1;
                    m_tabs[static_cast<std::size_t>(i)].x_end = -1;
                }

                // Draw right scroll arrow if overflow
                if (m_has_overflow) {
                    // Check if there are more tabs to the right
                    bool more_right = false;
                    for (auto i = static_cast<std::size_t>(m_scroll_offset); i < m_tabs.size(); ++i) {
                        if (m_tabs[i].x_start < 0) {
                            more_right = true;
                            break;
                        }
                    }
                    int arrow_abs_x = x + total_width - arrow_width;
                    m_right_arrow_start = total_width - arrow_width;  // Relative
                    typename Backend::point_type arrow_pos{arrow_abs_x, tab_y};
                    ctx.draw_icon(tab_style.scroll_right_icon, arrow_pos);
                } else {
                    m_right_arrow_start = total_width;  // Off-screen
                }
            };

            // Render current tab's content via base class FIRST
            base::do_render(ctx);

            // Then draw tab bar ON TOP (TUI overwrites mode)
            switch (m_tab_position) {
                case tab_position::top:
                    draw_tabs_horizontal(y);
                    break;
                case tab_position::bottom:
                    draw_tabs_horizontal(y + content_height - 1);
                    break;
                case tab_position::left:
                case tab_position::right:
                    // Vertical tabs - use same horizontal layout for simplicity
                    draw_tabs_horizontal(y);
                    break;
            }
        }

        /**
         * @brief Handle keyboard and mouse events
         */
        bool handle_event(const ui_event& event, event_phase phase) override {
            // Handle mouse events on tabs in CAPTURE phase (before children)
            if (auto* mouse_evt = std::get_if<mouse_event>(&event)) {
                // Convert mouse coords to relative (widget-local)
                auto content_bounds = this->bounds();
                int bounds_x = rect_utils::get_x(content_bounds);
                int bounds_y = rect_utils::get_y(content_bounds);
                int mouse_x = mouse_evt->x - bounds_x;  // Make relative
                int mouse_y = mouse_evt->y - bounds_y;  // Make relative

                // Use stored tab bar Y from rendering (m_tab_bar_y is set during do_render)
                bool in_tab_bar = (mouse_y == m_tab_bar_y);

                // Handle mouse move for hover state
                if (mouse_evt->act == mouse_event::action::move) {
                    int new_hover = -1;
                    if (in_tab_bar) {
                        for (std::size_t i = 0; i < m_tabs.size(); ++i) {
                            const auto& tab = m_tabs[i];
                            if (mouse_x >= tab.x_start && mouse_x < tab.x_end) {
                                new_hover = static_cast<int>(i);
                                break;
                            }
                        }
                    }
                    if (new_hover != m_hovered_tab_index) {
                        m_hovered_tab_index = new_hover;
                        this->mark_dirty();
                    }
                    return false;  // Don't consume move events
                }

                // Handle mouse press for clicks
                if (mouse_evt->act == mouse_event::action::press && in_tab_bar) {
                    // Check if left scroll arrow was clicked
                    if (m_has_overflow && mouse_x < m_left_arrow_end) {
                        const_cast<tab_widget*>(this)->scroll_left();
                        return true;
                    }

                    // Check if right scroll arrow was clicked
                    if (m_has_overflow && mouse_x >= m_right_arrow_start) {
                        const_cast<tab_widget*>(this)->scroll_right();
                        return true;
                    }

                    // Check tabs
                    for (std::size_t i = 0; i < m_tabs.size(); ++i) {
                        const auto& tab = m_tabs[i];
                        // Skip invisible tabs
                        if (tab.x_start < 0) continue;

                        if (mouse_x >= tab.x_start && mouse_x < tab.x_end) {
                            // Check if close button was clicked
                            if (tab.close_x >= 0 && mouse_x >= tab.close_x &&
                                mouse_x < tab.close_x + m_close_icon_width) {
                                tab_close_requested.emit(static_cast<int>(i));
                                return true;
                            }
                            // Otherwise, select the tab
                            set_current_index(static_cast<int>(i));
                            return true;
                        }
                    }
                }
            }

            // Handle keyboard navigation (only in target phase)
            if (phase == event_phase::target) {
              if (auto* kbd = std::get_if<keyboard_event>(&event)) {
                if (kbd->pressed) {
                    // Ctrl+Tab: Next tab
                    if (kbd->key == key_code::tab &&
                        (kbd->modifiers & key_modifier::ctrl) != key_modifier::none) {

                        if ((kbd->modifiers & key_modifier::shift) != key_modifier::none) {
                            previous_tab();
                        } else {
                            next_tab();
                        }
                        return true;
                    }

                    // Ctrl+W: Close current tab
                    if (kbd->key == key_code::w &&
                        (kbd->modifiers & key_modifier::ctrl) != key_modifier::none &&
                        m_tabs_closable) {

                        if (m_current_index >= 0 &&
                            m_current_index < static_cast<int>(m_tabs.size()) &&
                            m_tabs[static_cast<std::size_t>(m_current_index)].closeable) {
                            tab_close_requested.emit(m_current_index);
                        }
                        return true;
                    }

                    // Alt+Number: Jump to tab (Alt+1 through Alt+9)
                    if ((kbd->modifiers & key_modifier::alt) != key_modifier::none) {
                        int key_val = static_cast<int>(kbd->key);
                        if (key_val >= '1' && key_val <= '9') {
                            int tab_index = key_val - '1';  // Alt+1 = tab 0, Alt+2 = tab 1, etc.
                            if (tab_index < static_cast<int>(m_tabs.size())) {
                                set_current_index(tab_index);
                                return true;
                            }
                        }
                    }
                }
              }
            }

            return base::handle_event(event, phase);
        }

    private:
        /**
         * @brief Tab information
         */
        struct tab_info {
            std::string label;
            std::unique_ptr<ui_element<Backend>> widget;  // Temporarily holds widget before adding to children
            bool closeable = true;
            mutable int x_start = 0;   // Tab start position (for hit testing)
            mutable int x_end = 0;     // Tab end position (for hit testing)
            mutable int close_x = -1;  // Close button X position (-1 = no close button)
        };

        std::vector<tab_info> m_tabs;
        int m_current_index = -1;
        int m_hovered_tab_index = -1;  // Tab currently hovered (-1 = none)
        tab_position m_tab_position = tab_position::top;
        bool m_tabs_closable = false;

        // Scroll state for overflow
        int m_scroll_offset = 0;       // First visible tab index
        mutable bool m_has_overflow = false;  // True if tabs overflow available width
        mutable int m_left_arrow_end = 0;     // X position where left arrow ends
        mutable int m_tab_bar_y = 0;          // Y position of tab bar (relative)
        mutable int m_right_arrow_start = 0;  // X position where right arrow starts
        mutable int m_close_icon_width = 0;   // Close button icon width (measured from backend)

        /**
         * @brief Ensure a tab is visible by adjusting scroll offset
         * @param index Tab index to make visible
         *
         * @details
         * Auto-scrolls the tab bar to ensure the specified tab is visible:
         * - If tab is before scroll offset (hidden to the left), scroll left to show it
         * - If tab is after scroll offset, ensure it's in the visible range
         */
        void ensure_tab_visible(int index) {
            if (index < 0 || index >= static_cast<int>(m_tabs.size())) {
                return;
            }

            // If tab is scrolled out of view to the left, scroll left to show it
            if (index < m_scroll_offset) {
                m_scroll_offset = index;
                return;
            }

            // If tab is potentially scrolled out of view to the right, scroll right
            // Note: We can't know for certain without rendering, but we can make a reasonable guess
            // based on typical tab count. This will be refined during the next render pass.
            if (m_has_overflow && index > m_scroll_offset) {
                // Heuristic: If the selected tab is far from scroll offset, move offset closer
                // This will be corrected by rendering if needed
                int const estimated_visible_tabs = 3;  // Conservative estimate
                if (index >= m_scroll_offset + estimated_visible_tabs) {
                    m_scroll_offset = index - estimated_visible_tabs + 1;
                    if (m_scroll_offset < 0) {
                        m_scroll_offset = 0;
                    }
                }
            }
        }

        /**
         * @brief Update visibility of child widgets
         */
        void update_visibility() {
            auto& children = this->children();
            for (std::size_t i = 0; i < children.size(); ++i) {
                bool visible = (static_cast<int>(i) == m_current_index);
                children[i]->set_visible(visible);
            }
        }
    };

} // namespace onyxui
