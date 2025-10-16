/**
 * @file absolute_panel.hh
 * @brief Absolute positioning layout widget with explicit coordinates
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Convenience widget that positions children at explicit (x, y) coordinates.
 * This is a pre-configured widget with absolute_layout.
 */

#pragma once

#include <onyxui/widgets/widget.hh>
#include <onyxui/layout/absolute_layout.hh>

namespace onyxui {
    /**
     * @class absolute_panel
     * @brief Container that positions children at explicit coordinates
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * absolute_panel is a convenience widget that automatically uses absolute_layout
     * to position children at explicit pixel coordinates, with optional size overrides.
     *
     * ## Features
     *
     * - Pixel-perfect positioning with explicit (x, y) coordinates
     * - Optional size overrides for each child
     * - Free-form overlapping support
     * - Coordinates relative to panel's content area
     * - Default positioning at origin (0, 0)
     *
     * ## When to Use
     *
     * - **Drag-and-drop interfaces** - User-positioned elements
     * - **Tooltips** - Positioned near cursor or target
     * - **Custom dialogs** - Precise control over button placement
     * - **Node editors** - Graph-like layouts with connections
     * - **Prototyping** - Quick mockups with exact positioning
     *
     * ## Comparison with Other Widgets
     *
     * - vs `anchor_panel`: absolute uses exact pixels, anchor uses relative positioning
     * - vs `grid`: absolute allows overlapping, grid enforces cell structure
     * - vs `hbox`/`vbox`: absolute has no automatic flow, linear layouts stack children
     *
     * @example Tooltip Positioning
     * @code
     * auto overlay = std::make_unique<absolute_panel<Backend>>();
     *
     * // Position tooltip near mouse cursor
     * auto tooltip = std::make_unique<label<Backend>>("Click to continue");
     * overlay->set_position(tooltip.get(), mouse_x + 10, mouse_y - 30);
     * overlay->add_child(std::move(tooltip));
     * @endcode
     *
     * @example Custom Dialog
     * @code
     * auto dialog = std::make_unique<absolute_panel<Backend>>();
     *
     * // Title at top with fixed size
     * auto title = std::make_unique<label<Backend>>("Confirm Action");
     * dialog->set_position(title.get(), 20, 10, 260, 30);
     * dialog->add_child(std::move(title));
     *
     * // Message in middle
     * auto message = std::make_unique<label<Backend>>("Are you sure?");
     * dialog->set_position(message.get(), 20, 50, 260, 80);
     * dialog->add_child(std::move(message));
     *
     * // Buttons at bottom
     * auto ok = std::make_unique<button<Backend>>("OK");
     * dialog->set_position(ok.get(), 50, 150, 80, 30);
     * dialog->add_child(std::move(ok));
     *
     * auto cancel = std::make_unique<button<Backend>>("Cancel");
     * dialog->set_position(cancel.get(), 170, 150, 80, 30);
     * dialog->add_child(std::move(cancel));
     * @endcode
     *
     * @example Drag and Drop
     * @code
     * auto canvas = std::make_unique<absolute_panel<Backend>>();
     *
     * // Position draggable items
     * for (auto& item : items) {
     *     canvas->set_position(item.widget.get(), item.x, item.y);
     *     canvas->add_child(std::move(item.widget));
     * }
     *
     * // Update position during drag
     * void on_drag(Widget* item, int new_x, int new_y) {
     *     canvas->set_position(item, new_x, new_y);
     *     canvas->invalidate_arrange();
     * }
     * @endcode
     */
    template<UIBackend Backend>
    class absolute_panel : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using element_type = ui_element<Backend>;

        /**
         * @brief Construct an absolute panel
         * @param parent Parent element (nullptr for none)
         */
        explicit absolute_panel(ui_element<Backend>* parent = nullptr)
            : base(parent) {
            auto layout = std::make_unique<absolute_layout<Backend>>();

            // Store pointer before moving
            m_absolute_layout = layout.get();
            this->set_layout_strategy(std::move(layout));
            this->set_focusable(false);  // Container, not focusable
        }

        /**
         * @brief Destructor
         */
        ~absolute_panel() override = default;

        // Rule of Five
        absolute_panel(const absolute_panel&) = delete;
        absolute_panel& operator=(const absolute_panel&) = delete;

        /**
         * @brief Move constructor
         * @note Updates raw layout pointer to maintain validity
         */
        absolute_panel(absolute_panel&& other) noexcept
            : base(std::move(other))
            , m_absolute_layout(std::exchange(other.m_absolute_layout, nullptr)) {}

        /**
         * @brief Move assignment operator
         * @note Updates raw layout pointer to maintain validity
         */
        absolute_panel& operator=(absolute_panel&& other) noexcept {
            if (this != &other) {
                base::operator=(std::move(other));
                m_absolute_layout = std::exchange(other.m_absolute_layout, nullptr);
            }
            return *this;
        }

        /**
         * @brief Set the position and optionally size for a child element
         *
         * @param child Pointer to the child element to position
         * @param x X coordinate in pixels relative to panel's content area
         * @param y Y coordinate in pixels relative to panel's content area
         * @param width Width override in pixels (-1 to use measured width)
         * @param height Height override in pixels (-1 to use measured height)
         *
         * @details
         * Configures the exact position and optional size for a child element.
         * Coordinates are relative to the panel's content area origin.
         *
         * ## Coordinate System
         *
         * - Origin (0, 0) is at top-left of panel's content area
         * - X increases rightward, Y increases downward
         * - Negative coordinates position children outside panel
         *
         * ## Size Overrides
         *
         * - `-1` (default): Use child's measured size
         * - `0`: Effectively hides the element (zero size)
         * - Positive value: Override with fixed size
         *
         * @note
         * - This can be called before or after adding the child
         * - Null children are silently ignored
         * - Previous position settings are replaced
         * - Size overrides bypass child's size constraints
         * - Negative coordinates are allowed but may cause clipping
         *
         * @warning Size overrides ignore the child's min/max constraints.
         *          Use with caution to avoid breaking child layouts.
         *
         * @example
         * @code
         * auto panel = std::make_unique<absolute_panel<Backend>>();
         * auto btn = std::make_unique<button<Backend>>("Click");
         * auto* btn_ptr = btn.get();
         *
         * // Position at (50, 100) with auto-size
         * panel->set_position(btn_ptr, 50, 100);
         * panel->add_child(std::move(btn));
         *
         * // Position at (200, 100) with fixed size 120x40
         * auto btn2 = std::make_unique<button<Backend>>("Fixed");
         * panel->set_position(btn2.get(), 200, 100, 120, 40);
         * panel->add_child(std::move(btn2));
         * @endcode
         */
        void set_position(element_type* child, int x, int y,
                          int width = -1, int height = -1) {
            if (!m_absolute_layout) return;
            m_absolute_layout->set_position(child, x, y, width, height);
        }

    protected:
        void do_apply_theme([[maybe_unused]] const typename base::theme_type& theme) override {
            // Absolute panel is a layout container, no theme application needed
        }

    private:
        absolute_layout<Backend>* m_absolute_layout = nullptr;  ///< Non-owning pointer to layout
    };
}
