/**
 * @file anchor_panel.hh
 * @brief Anchor-based layout widget for positioning children at predefined points
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Convenience widget that positions children at anchor points (corners, edges, center)
 * within the parent area. This is a pre-configured widget with anchor_layout.
 */

#pragma once

#include <onyxui/widgets/widget.hh>
#include <onyxui/layout/anchor_layout.hh>

namespace onyxui {
    /**
     * @class anchor_panel
     * @brief Container that positions children at anchor points
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * anchor_panel is a convenience widget that automatically uses anchor_layout
     * to position children at predefined anchor points (corners, edges, center).
     *
     * ## Features
     *
     * - 9 predefined anchor points (corners, edge centers, absolute center)
     * - Pixel offsets for fine-tuning positions
     * - Children maintain relative positions when parent resizes
     * - Children can overlap (useful for layered UI elements)
     * - Default top-left positioning for unanchored children
     *
     * ## Anchor Points
     *
     * - `top_left`, `top_center`, `top_right`
     * - `center_left`, `center`, `center_right`
     * - `bottom_left`, `bottom_center`, `bottom_right`
     *
     * ## Common Use Cases
     *
     * - Title bars with edge-aligned buttons and centered text
     * - Floating action buttons in corners
     * - Status indicators at screen edges
     * - Centered modal dialogs
     * - Game HUD elements
     *
     * @example Title Bar
     * @code
     * auto title_bar = std::make_unique<anchor_panel<Backend>>();
     *
     * // Menu button at left edge
     * auto menu = std::make_unique<button<Backend>>("Menu");
     * title_bar->set_anchor(menu.get(), anchor_point::center_left, 10, 0);
     * title_bar->add_child(std::move(menu));
     *
     * // Title text in center
     * auto title = std::make_unique<label<Backend>>("My App");
     * title_bar->set_anchor(title.get(), anchor_point::center);
     * title_bar->add_child(std::move(title));
     *
     * // Close button at right edge with margin
     * auto close = std::make_unique<button<Backend>>("X");
     * title_bar->set_anchor(close.get(), anchor_point::center_right, -10, 0);
     * title_bar->add_child(std::move(close));
     * @endcode
     *
     * @example Floating Action Button
     * @code
     * auto screen = std::make_unique<anchor_panel<Backend>>();
     *
     * // FAB in bottom-right corner with margins
     * auto fab = std::make_unique<button<Backend>>("+");
     * screen->set_anchor(fab.get(), anchor_point::bottom_right, -16, -16);
     * screen->add_child(std::move(fab));
     * @endcode
     *
     * @example Centered Dialog
     * @code
     * auto overlay = std::make_unique<anchor_panel<Backend>>();
     *
     * // Dialog centered on screen
     * auto dialog = std::make_unique<panel<Backend>>();
     * overlay->set_anchor(dialog.get(), anchor_point::center);
     * overlay->add_child(std::move(dialog));
     * @endcode
     */
    template<UIBackend Backend>
    class anchor_panel : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using element_type = ui_element<Backend>;

        /**
         * @brief Construct an anchor panel
         * @param parent Parent element (nullptr for none)
         */
        explicit anchor_panel(ui_element<Backend>* parent = nullptr)
            : base(parent) {
            auto layout = std::make_unique<anchor_layout<Backend>>();

            // Store pointer before moving
            m_anchor_layout = layout.get();
            this->set_layout_strategy(std::move(layout));
            this->set_focusable(false);  // Container, not focusable
        }

        /**
         * @brief Destructor
         */
        ~anchor_panel() override = default;

        // Rule of Five
        anchor_panel(const anchor_panel&) = delete;
        anchor_panel& operator=(const anchor_panel&) = delete;

        /**
         * @brief Move constructor
         * @note Updates raw layout pointer to maintain validity
         */
        anchor_panel(anchor_panel&& other) noexcept
            : base(std::move(static_cast<base&>(other)))
            , m_anchor_layout(std::exchange(other.m_anchor_layout, nullptr)) {}

        /**
         * @brief Move assignment operator
         * @note Updates raw layout pointer to maintain validity
         */
        anchor_panel& operator=(anchor_panel&& other) noexcept {
            if (this != &other) {
                base::operator=(std::move(static_cast<base&>(other)));
                m_anchor_layout = std::exchange(other.m_anchor_layout, nullptr);
            }
            return *this;
        }

        /**
         * @brief Set the anchor point and offset for a child element
         *
         * @param child Pointer to the child element to anchor
         * @param point Which anchor point to use for positioning
         * @param offset_x Horizontal offset in pixels (positive = right, negative = left)
         * @param offset_y Vertical offset in pixels (positive = down, negative = up)
         *
         * @details
         * Configures how a child element should be positioned relative to the panel.
         * The anchor point determines the base position, and offsets provide fine-tuning.
         *
         * ## Offset Guidelines
         *
         * - Positive offsets move right/down from anchor point
         * - Negative offsets move left/up from anchor point
         * - Use negative offsets with edge anchors to keep children visible
         *   (e.g., `bottom_right` with offset `-10, -10` for 10px margins)
         *
         * @note
         * - This can be called before or after adding the child
         * - Null children are silently ignored
         * - Previous anchor settings are replaced
         * - Offsets can position children outside panel bounds
         *
         * @example
         * @code
         * auto panel = std::make_unique<anchor_panel<Backend>>();
         * auto btn = std::make_unique<button<Backend>>("Click");
         * auto* btn_ptr = btn.get();
         *
         * // Position button at bottom-right with 10px margins
         * panel->set_anchor(btn_ptr, anchor_point::bottom_right, -10, -10);
         * panel->add_child(std::move(btn));
         * @endcode
         */
        void set_anchor(element_type* child, anchor_point point,
                        int offset_x = 0, int offset_y = 0) {
            if (!m_anchor_layout) return;
            m_anchor_layout->set_anchor(child, point, offset_x, offset_y);
        }

    protected:
        void do_apply_theme([[maybe_unused]] const typename base::theme_type& theme) override {
            // Anchor panel is a layout container - children inherit via CSS-style inheritance
        }

    private:
        anchor_layout<Backend>* m_anchor_layout = nullptr;  ///< Non-owning pointer to layout
    };
}
