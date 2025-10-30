//
// stateful_widget.hh - Helper base class for interactive widgets with state
//
// Created: 2025-10-23 (Theme System Refactoring v2.0)
//

#pragma once

#include <onyxui/widgets/core/widget.hh>

namespace onyxui {

    /**
     * @class stateful_widget
     * @brief Base class for interactive widgets with visual states
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Provides state management for widgets that change appearance based on
     * user interaction (hover, press, disabled, etc.).
     *
     * ## Supported States
     *
     * - **normal**: Default resting state
     * - **hover**: Mouse cursor is over the widget
     * - **pressed**: Widget is being clicked/activated
     * - **disabled**: Widget is not interactive
     *
     * ## Usage Pattern
     *
     * @code
     * template<UIBackend Backend>
     * class button : public stateful_widget<Backend> {
     *     void do_render(render_context<Backend>& ctx) const override {
     *         // Get state-appropriate colors from theme via ctx.theme()
     *         auto* theme = ctx.theme();
     *         if (theme) {
     *             auto bg = this->get_state_background(theme->button);
     *             auto fg = this->get_state_foreground(theme->button);
     *             // Use bg/fg for rendering...
     *         }
     *     }
     *
     *     void on_mouse_enter() override {
     *         this->set_interaction_state(interaction_state::hover);
     *     }
     * };
     * @endcode
     *
     * ## Automatic Invalidation
     *
     * When the state changes, `set_interaction_state()` automatically calls
     * `invalidate_arrange()` to trigger a redraw.
     *
     * ## Theme Integration
     *
     * The `get_state_background()` and `get_state_foreground()` helpers
     * extract the appropriate colors from widget-specific theme structures
     * that follow the pattern:
     * - `bg_normal`, `fg_normal`
     * - `bg_hover`, `fg_hover`
     * - `bg_pressed`, `fg_pressed`
     * - `bg_disabled`, `fg_disabled`
     */
    template<UIBackend Backend>
    class stateful_widget : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using color_type = typename Backend::color_type;
        using font_type = typename Backend::renderer_type::font;

        /**
         * @enum interaction_state
         * @brief Visual state of an interactive widget
         */
        enum class interaction_state : uint8_t {
            normal,    ///< Default resting state
            hover,     ///< Mouse cursor over widget
            pressed,   ///< Widget being clicked/activated
            disabled   ///< Widget not interactive
        };

    protected:
        /**
         * @brief Protected constructor
         * @param parent Pointer to parent element (nullptr for root)
         */
        explicit stateful_widget(ui_element<Backend>* parent = nullptr)
            : base(parent) {}

        /**
         * @brief Get the current interaction state
         * @return Current state
         */
        [[nodiscard]] interaction_state get_interaction_state() const noexcept {
            return m_state;
        }

        /**
         * @brief Set the interaction state
         * @param state New state
         *
         * @details
         * Automatically calls invalidate_arrange() if the state changes,
         * triggering a redraw with the new state's visual properties.
         */
        void set_interaction_state(interaction_state state) {
            if (m_state != state) {
                m_state = state;
                this->invalidate_arrange();  // Trigger redraw
            }
        }

        // ===================================================================
        // State-Based Color Helpers
        // ===================================================================

        /**
         * @brief Get background color for current state
         *
         * @tparam WidgetTheme Widget-specific theme structure type
         * @param widget_theme The widget theme (e.g., theme.button)
         * @return Background color appropriate for current state
         *
         * @details
         * Expects the widget theme to have these properties:
         * - bg_normal, bg_hover, bg_pressed, bg_disabled
         *
         * @example
         * @code
         * auto bg = get_state_background(theme->button);
         * @endcode
         */
        template<typename WidgetTheme>
        [[nodiscard]] color_type get_state_background(const WidgetTheme& widget_theme) const noexcept {
            switch (m_state) {
                case interaction_state::disabled:
                    return widget_theme.disabled.background;
                case interaction_state::pressed:
                    return widget_theme.pressed.background;
                case interaction_state::hover:
                    return widget_theme.hover.background;
                case interaction_state::normal:
                default:
                    return widget_theme.normal.background;
            }
        }

        /**
         * @brief Get foreground color for current state
         *
         * @tparam WidgetTheme Widget-specific theme structure type
         * @param widget_theme The widget theme (e.g., theme.button)
         * @return Foreground color appropriate for current state
         *
         * @details
         * Expects the widget theme to have these properties:
         * - fg_normal, fg_hover, fg_pressed, fg_disabled
         *
         * @example
         * @code
         * auto fg = get_state_foreground(theme->button);
         * @endcode
         */
        template<typename WidgetTheme>
        [[nodiscard]] color_type get_state_foreground(const WidgetTheme& widget_theme) const noexcept {
            switch (m_state) {
                case interaction_state::disabled:
                    return widget_theme.disabled.foreground;
                case interaction_state::pressed:
                    return widget_theme.pressed.foreground;
                case interaction_state::hover:
                    return widget_theme.hover.foreground;
                case interaction_state::normal:
                default:
                    return widget_theme.normal.foreground;
            }
        }

        /**
         * @brief Get font for current state
         *
         * @tparam WidgetTheme Widget-specific theme structure type
         * @param widget_theme The widget theme (e.g., theme.button)
         * @return Font appropriate for current state
         *
         * @details
         * Expects the widget theme to have visual_state bundles:
         * - normal, hover, pressed, disabled (each with .font property)
         *
         * @example
         * @code
         * auto font = get_state_font(theme->button);
         * @endcode
         */
        template<typename WidgetTheme>
        [[nodiscard]] font_type get_state_font(const WidgetTheme& widget_theme) const noexcept {
            switch (m_state) {
                case interaction_state::disabled:
                    return widget_theme.disabled.font;
                case interaction_state::pressed:
                    return widget_theme.pressed.font;
                case interaction_state::hover:
                    return widget_theme.hover.font;
                case interaction_state::normal:
                default:
                    return widget_theme.normal.font;
            }
        }

        // ===================================================================
        // Event Handlers (Auto-sync m_state with mouse events)
        // ===================================================================

        /**
         * @brief Handle mouse enter event (tracked by event_target)
         * @details Automatically sets state to hover if enabled
         */
        bool handle_mouse_enter() override {
            if (this->is_enabled()) {
                set_interaction_state(interaction_state::hover);
            }
            return base::handle_mouse_enter();
        }

        /**
         * @brief Handle mouse leave event (tracked by event_target)
         * @details Returns to normal state ONLY if not currently in pressed state
         *
         * If mouse button is still held down when leaving (visual state = pressed),
         * maintain that state. This prevents visual state desync during drag operations.
         */
        bool handle_mouse_leave() override {
            if (this->is_enabled()) {
                // Only return to normal if we're not visually pressed
                // If pressed, we'll transition on mouse_up instead
                if (m_state != interaction_state::pressed) {
                    set_interaction_state(interaction_state::normal);
                }
            }
            return base::handle_mouse_leave();
        }

        /**
         * @brief Handle mouse down event (OLD API - backward compatibility)
         * @details Automatically sets state to pressed if enabled
         */
        bool handle_mouse_down(int x, int y, int button) override {
            if (this->is_enabled()) {
                set_interaction_state(interaction_state::pressed);
            }
            return base::handle_mouse_down(x, y, button);
        }

        /**
         * @brief Handle mouse up event (OLD API - backward compatibility)
         * @details Automatically sets state back to hover or normal
         */
        bool handle_mouse_up(int x, int y, int button) override {
            if (this->is_enabled()) {
                // Return to hover if still hovering, otherwise normal
                set_interaction_state(base::is_hovered() ? interaction_state::hover : interaction_state::normal);
            }
            return base::handle_mouse_up(x, y, button);
        }

        /**
         * @brief Handle mouse events (NEW Phase 4 API)
         * @param mouse Mouse event containing action, position, button, and modifiers
         * @return true if event was handled
         *
         * @details
         * Unified mouse event handler that automatically manages interaction state:
         * - **Mouse press**: Sets pressed state if enabled
         * - **Mouse release**: Returns to hover (if still hovering) or normal
         * - **Mouse move**: Handled by base class
         * - **Mouse wheel**: Handled by base class
         *
         * Note: Mouse enter/leave are tracked separately by event_target's internal
         * is_inside() checking and call handle_mouse_enter/leave directly.
         *
         * This provides a cleaner API than handle_mouse_down/up (which are kept for
         * backward compatibility with existing code and tests).
         */
        bool handle_mouse(const mouse_event& mouse) override {
            if (!this->is_enabled()) {
                return base::handle_mouse(mouse);
            }

            // Dispatch based on mouse action
            switch (mouse.act) {
                case mouse_event::action::press:
                    // Mouse button pressed - set pressed state
                    set_interaction_state(interaction_state::pressed);
                    break;

                case mouse_event::action::release:
                    // Mouse button released - return to hover (if hovering) or normal
                    set_interaction_state(base::is_hovered() ? interaction_state::hover : interaction_state::normal);
                    break;

                case mouse_event::action::move:
                case mouse_event::action::wheel_up:
                case mouse_event::action::wheel_down:
                    // Move and wheel events don't affect state - pass to base
                    break;
            }

            // Chain to base class for standard event processing
            return base::handle_mouse(mouse);
        }

        // ===================================================================
        // Convenience State Queries
        // ===================================================================

        /**
         * @brief Check if widget is in normal state
         */
        [[nodiscard]] bool is_normal() const noexcept {
            return m_state == interaction_state::normal;
        }

        /**
         * @brief Check if widget is disabled
         */
        [[nodiscard]] bool is_disabled() const noexcept {
            return m_state == interaction_state::disabled;
        }

        /**
         * @brief Enable the widget (set to normal state)
         */
        void enable() {
            this->set_enabled(true);  // Enable event processing
            set_interaction_state(interaction_state::normal);
        }

        /**
         * @brief Disable the widget
         */
        void disable() {
            this->set_enabled(false);  // Disable event processing
            set_interaction_state(interaction_state::disabled);
        }

    private:
        interaction_state m_state = interaction_state::normal;
    };

} // namespace onyxui
