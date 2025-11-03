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

            // Create 2x2 grid layout (2 columns, 2 rows)
            // Grid will use content-based sizing from its children
            auto grid_widget = std::make_unique<grid<Backend>>(2, 2);

            // Store raw pointers BEFORE moving
            m_content_ptr = content.get();
            m_vscrollbar_ptr = vscrollbar.get();
            m_hscrollbar_ptr = hscrollbar.get();
            auto* grid_ptr = grid_widget.get();

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
            auto layout = std::make_unique<linear_layout<Backend>>(direction::vertical);
            this->set_layout_strategy(std::move(layout));

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
         * @brief Override measure to properly size based on grid child
         */
        size_type do_measure(int available_width, int available_height) override {
            // Measure the grid (our only child)
            if (m_grid_ptr) {
                return m_grid_ptr->measure(available_width, available_height);
            }
            return base::do_measure(available_width, available_height);
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
