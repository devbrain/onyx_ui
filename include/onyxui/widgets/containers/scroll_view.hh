/**
 * @file scroll_view.hh
 * @brief All-in-one scrollable view with automatic scrollbar management
 * @author OnyxUI Framework
 * @date 2025-10-29
 */

#pragma once

#include <onyxui/widgets/containers/scroll/scrollable.hh>
#include <onyxui/widgets/containers/scroll/scrollbar.hh>
#include <onyxui/widgets/containers/scroll/scroll_controller.hh>
#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/containers/grid.hh>
#include <onyxui/layout/linear_layout.hh>
#include <onyxui/events/ui_event.hh>
#include <onyxui/hotkeys/key_code.hh>
#include <memory>

namespace onyxui {

    /**
     * @class scroll_view
     * @brief Convenience wrapper combining scrollable + scrollbars + controller
     *
     * @details
     * The scroll_view provides a complete scrolling solution with minimal setup.
     * It automatically creates and manages:
     * - Scrollable content area
     * - Vertical and horizontal scrollbars
     * - Scroll controller for bidirectional sync
     * - 2x2 grid layout for positioning
     *
     * Layout:
     * ```
     * ┌─────────────────┬──┐
     * │                 │▲ │  ← Vertical scrollbar
     * │   Content       │█ │
     * │   (scrollable)  │▼ │
     * ├─────────────────┼──┤
     * │◄──█──►          │  │  ← Horizontal scrollbar
     * └─────────────────┴──┘
     *                    └─ Corner filler
     * ```
     *
     * Usage:
     * ```cpp
     * auto view = std::make_unique<scroll_view<Backend>>();
     *
     * // Add content
     * view->add_child(std::make_unique<label>("Item 1"));
     * view->add_child(std::make_unique<label>("Item 2"));
     *
     * // Configure scrollbars
     * view->set_scrollbar_policy(scrollbar_visibility::auto_hide);
     * view->set_scrollbar_style(scrollbar_style::minimal);
     * ```
     *
     * @tparam Backend UIBackend implementation
     */
    template<UIBackend Backend>
    class scroll_view : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using scrollable_type = scrollable<Backend>;
        using scrollbar_type = scrollbar<Backend>;
        using controller_type = scroll_controller<Backend>;
        using element_type = ui_element<Backend>;
        using layout_strategy_type = layout_strategy<Backend>;
        using size_type = typename Backend::size_type;

        /**
         * @brief Construct scroll_view with default configuration
         */
        scroll_view()
        {
            // Create components
            std::unique_ptr<scrollable_type> content = std::make_unique<scrollable_type>();
            std::unique_ptr<scrollbar_type> vscrollbar = std::make_unique<scrollbar_type>(orientation::vertical);
            std::unique_ptr<scrollbar_type> hscrollbar = std::make_unique<scrollbar_type>(orientation::horizontal);
            std::unique_ptr<panel<Backend>> corner = std::make_unique<panel<Backend>>();

            // Store raw pointers BEFORE moving
            m_content_ptr = content.get();
            m_vscrollbar_ptr = vscrollbar.get();
            m_hscrollbar_ptr = hscrollbar.get();

            // Set alignment to position scrollbars at widget edges
            vscrollbar->set_horizontal_align(horizontal_alignment::right);  // Vertical scrollbar at right edge
            hscrollbar->set_vertical_align(vertical_alignment::bottom);      // Horizontal scrollbar at bottom edge

            // Set default vertical layout for scrollable content
            // This ensures children are arranged vertically by default
            m_content_ptr->set_layout_strategy(std::make_unique<linear_layout<Backend>>(direction::vertical));

            // Create 2x2 grid layout (2 columns, 2 rows)
            // Grid will use content-based sizing from its children
            auto grid_widget = std::make_unique<grid<Backend>>(2, 2);
            auto* grid_ptr = grid_widget.get();

            // CRITICAL: Set scrollable to stretch to fill its cell
            // This ensures scrollable expands to use all available space in the grid
            content->set_horizontal_align(horizontal_alignment::stretch);
            content->set_vertical_align(vertical_alignment::stretch);

            // Add components to grid (grid will auto-assign to 2x2 cells)
            // Auto-assignment goes left-to-right, top-to-bottom
            grid_widget->add_child(std::move(content));      // Cell (0,0)
            grid_widget->add_child(std::move(vscrollbar));   // Cell (0,1)
            grid_widget->add_child(std::move(hscrollbar));   // Cell (1,0)
            grid_widget->add_child(std::move(corner));       // Cell (1,1)

            // Create controller
            m_controller = std::make_unique<controller_type>(
                m_content_ptr,
                m_vscrollbar_ptr,
                m_hscrollbar_ptr
            );
            m_controller_ptr = m_controller.get();

            // Set layout strategy BEFORE adding children
            // Use vertical linear layout with single child (the grid)
            // IMPORTANT: Use base::set_layout_strategy to set on scroll_view itself,
            // NOT this->set_layout_strategy which forwards to scrollable!
            auto layout = std::make_unique<linear_layout<Backend>>(direction::vertical);
            base::set_layout_strategy(std::move(layout));

            // Add grid as our only child (use base class add_child, not our forwarding method)
            base::add_child(std::move(grid_widget));
            m_grid_ptr = grid_ptr;

            // Make scroll_view focusable by default so it can receive keyboard/mouse events for scrolling
            this->set_focusable(true);
        }

        // =====================================================================
        // Forwarding Methods - Content Management
        // =====================================================================

        /**
         * @brief Add child to scrollable content area
         * @param child Child widget to add
         */
        void add_child(std::unique_ptr<element_type> child) {
            m_content_ptr->add_child(std::move(child));
        }

        /**
         * @brief Emplace child directly in scrollable content
         * @tparam WidgetTemplate Widget template (e.g., label, button)
         * @tparam Args Constructor argument types
         * @param args Constructor arguments
         * @return Pointer to created child
         */
        template<template<typename> class WidgetTemplate, typename... Args>
        auto* emplace_child(Args&&... args) {
            return m_content_ptr->template emplace_child<WidgetTemplate>(std::forward<Args>(args)...);
        }

        /**
         * @brief Remove child from scrollable content
         * @param child Pointer to child to remove
         * @return Unique pointer to removed child
         */
        std::unique_ptr<element_type> remove_child(element_type* child) {
            return m_content_ptr->remove_child(child);
        }

        /**
         * @brief Clear all children from scrollable content
         */
        void clear_children() {
            m_content_ptr->clear_children();
        }

        /**
         * @brief Set layout strategy for scrollable content
         * @param strategy Layout strategy (takes ownership)
         */
        void set_layout_strategy(std::unique_ptr<layout_strategy_type> strategy) {
            m_content_ptr->set_layout_strategy(std::move(strategy));
        }

        // =====================================================================
        // Forwarding Methods - Scrolling
        // =====================================================================

        /**
         * @brief Scroll to absolute position
         * @param x Horizontal scroll position
         * @param y Vertical scroll position
         */
        void scroll_to(int x, int y) {
            m_content_ptr->scroll_to(x, y);
        }

        /**
         * @brief Scroll by relative delta
         * @param dx Horizontal delta
         * @param dy Vertical delta
         */
        void scroll_by(int dx, int dy) {
            m_content_ptr->scroll_by(dx, dy);
        }

        /**
         * @brief Scroll child widget into view
         * @param child Pointer to child widget
         */
        void scroll_into_view(const element_type* child) {
            m_content_ptr->scroll_into_view(child);
        }

        // =====================================================================
        // Configuration Methods
        // =====================================================================

        /**
         * @brief Set visibility policy for both scrollbars
         * @param policy Visibility policy (always, auto_hide, hidden)
         */
        void set_scrollbar_policy(scrollbar_visibility policy) {
            m_content_ptr->set_scrollbar_visibility(policy);
            m_controller_ptr->refresh();  // Update visibility
        }

        /**
         * @brief Set independent policies for each scrollbar
         * @param horizontal Horizontal scrollbar policy
         * @param vertical Vertical scrollbar policy
         */
        void set_scrollbar_policy(scrollbar_visibility horizontal, scrollbar_visibility vertical) {
            m_content_ptr->set_scrollbar_visibility(horizontal, vertical);
            m_controller_ptr->refresh();
        }

        // Note: Scrollbar visual style (minimal/classic/compact) is controlled
        // by the theme system, not directly on scrollbar widgets

        /**
         * @brief Enable or disable horizontal scrolling
         * @param enabled True to enable, false to disable
         */
        void set_horizontal_scroll_enabled(bool enabled) {
            m_content_ptr->set_scrollbar_visibility(
                enabled ? scrollbar_visibility::auto_hide : scrollbar_visibility::hidden,
                m_content_ptr->get_scrollbar_visibility_policy().vertical
            );
            m_controller_ptr->refresh();
        }

        /**
         * @brief Enable or disable vertical scrolling
         * @param enabled True to enable, false to disable
         */
        void set_vertical_scroll_enabled(bool enabled) {
            m_content_ptr->set_scrollbar_visibility(
                m_content_ptr->get_scrollbar_visibility_policy().horizontal,
                enabled ? scrollbar_visibility::auto_hide : scrollbar_visibility::hidden
            );
            m_controller_ptr->refresh();
        }

        // =====================================================================
        // Access to Internals (for advanced usage)
        // =====================================================================

        /**
         * @brief Get scrollable content area
         * @return Pointer to scrollable widget
         */
        [[nodiscard]] scrollable_type* content() noexcept {
            return m_content_ptr;
        }

        /**
         * @brief Get vertical scrollbar
         * @return Pointer to vertical scrollbar
         */
        [[nodiscard]] scrollbar_type* vertical_scrollbar() noexcept {
            return m_vscrollbar_ptr;
        }

        /**
         * @brief Get horizontal scrollbar
         * @return Pointer to horizontal scrollbar
         */
        [[nodiscard]] scrollbar_type* horizontal_scrollbar() noexcept {
            return m_hscrollbar_ptr;
        }

        /**
         * @brief Get scroll controller
         * @return Pointer to controller
         */
        [[nodiscard]] controller_type* controller() noexcept {
            return m_controller_ptr;
        }

    protected:
        /**
         * @brief Override measure to ensure minimum size for always-visible scrollbars
         *
         * @details
         * When scrollbar_visibility is "always", scrollbars need minimum 8 pixels to render
         * without corruption. This method ensures scroll_view requests adequate space.
         */
        size_type do_measure(int available_width, int available_height) override {
            // Measure the grid (our only child)
            size_type measured_size;
            if (m_grid_ptr) {
                measured_size = m_grid_ptr->measure(available_width, available_height);
            } else {
                measured_size = base::do_measure(available_width, available_height);
            }

            // CRITICAL FIX: Enforce minimum size for always-visible scrollbars
            // Get minimum size from theme to prevent rendering corruption
            auto policy = m_content_ptr->get_scrollbar_visibility_policy();

            auto const* themes = ui_services<Backend>::themes();
            auto const* theme = themes ? themes->get_current_theme() : nullptr;
            int const min_size = theme ? theme->scrollbar.min_render_size : ui_constants::DEFAULT_SCROLLBAR_MIN_RENDER_SIZE;

            // Add minimum height for horizontal scrollbar if always visible
            if (policy.horizontal == scrollbar_visibility::always) {
                if (measured_size.h < min_size) {
                    measured_size.h = min_size;
                }
            }

            // Add minimum width for vertical scrollbar if always visible
            if (policy.vertical == scrollbar_visibility::always) {
                if (measured_size.w < min_size) {
                    measured_size.w = min_size;
                }
            }

            return measured_size;
        }

        /**
         * @brief Handle keyboard events for scrolling
         * @param kbd Keyboard event
         * @return True if event was handled
         *
         * @details
         * Implements standard keyboard scrolling:
         * - Arrow Up/Down: Scroll by 1 line
         * - Page Up/Down: Scroll by viewport height
         * - Home: Jump to top
         * - End: Jump to bottom
         */
        bool handle_keyboard(const keyboard_event& kbd) override {
            // Only handle key press (pressed = true), not release
            if (!kbd.pressed) {
                return false;
            }

            // Arrow keys - scroll by line
            if (kbd.key == key_code::arrow_up) {
                m_content_ptr->scroll_by(0, -1);
                return true;
            }
            if (kbd.key == key_code::arrow_down) {
                m_content_ptr->scroll_by(0, 1);
                return true;
            }
            if (kbd.key == key_code::arrow_left) {
                m_content_ptr->scroll_by(-1, 0);
                return true;
            }
            if (kbd.key == key_code::arrow_right) {
                m_content_ptr->scroll_by(1, 0);
                return true;
            }

            // Page Up/Down - scroll by viewport height
            if (kbd.key == key_code::page_up) {
                // Use scrollable bounds to get viewport height
                auto viewport_bounds = m_content_ptr->bounds();
                int viewport_height = rect_utils::get_height(viewport_bounds);
                m_content_ptr->scroll_by(0, -viewport_height);
                return true;
            }
            if (kbd.key == key_code::page_down) {
                // Use scrollable bounds to get viewport height
                auto viewport_bounds = m_content_ptr->bounds();
                int viewport_height = rect_utils::get_height(viewport_bounds);
                m_content_ptr->scroll_by(0, viewport_height);
                return true;
            }

            // Home/End - jump to top/bottom
            if (kbd.key == key_code::home) {
                m_content_ptr->scroll_to(0, 0);
                return true;
            }
            if (kbd.key == key_code::end) {
                // Scroll to large number to ensure we reach the end
                // scrollable will clamp this to max scroll position
                m_content_ptr->scroll_to(0, 9999);
                return true;
            }

            // Not handled - let parent handle it
            return base::handle_keyboard(kbd);
        }

    private:
        // Controller (we own this)
        std::unique_ptr<controller_type> m_controller;   ///< Scroll controller

        // Raw pointers to components (owned by grid, which is owned by our parent widget class)
        grid<Backend>* m_grid_ptr = nullptr;             ///< 2x2 grid layout
        scrollable_type* m_content_ptr = nullptr;        ///< Scrollable content area
        scrollbar_type* m_vscrollbar_ptr = nullptr;      ///< Vertical scrollbar
        scrollbar_type* m_hscrollbar_ptr = nullptr;      ///< Horizontal scrollbar
        controller_type* m_controller_ptr = nullptr;     ///< Controller (same as m_controller.get())
    };

} // namespace onyxui
