//
// stateful_widget.hh - Helper base class for interactive widgets with state
//
// Created: 2025-10-23 (Theme System Refactoring v2.0)
//

#pragma once

#include <iostream>
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
         * Uses `get_effective_state()` which considers both mouse interaction
         * state AND keyboard focus, so focused widgets show hover state.
         *
         * @example
         * @code
         * auto bg = get_state_background(theme->button);
         * @endcode
         */
        template<typename WidgetTheme>
        [[nodiscard]] color_type get_state_background(const WidgetTheme& widget_theme) const noexcept {
            switch (get_effective_state()) {
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
         * Uses `get_effective_state()` which considers both mouse interaction
         * state AND keyboard focus, so focused widgets show hover state.
         *
         * @example
         * @code
         * auto fg = get_state_foreground(theme->button);
         * @endcode
         */
        template<typename WidgetTheme>
        [[nodiscard]] color_type get_state_foreground(const WidgetTheme& widget_theme) const noexcept {
            switch (get_effective_state()) {
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
         * @brief Get mnemonic foreground color for current state
         *
         * @tparam WidgetTheme Widget-specific theme structure type
         * @param widget_theme The widget theme (e.g., theme.button)
         * @return Mnemonic foreground color appropriate for current state
         *
         * @details
         * Expects the widget theme to have visual_state bundles with mnemonic_foreground:
         * - normal.mnemonic_foreground, hover.mnemonic_foreground, etc.
         *
         * Uses `get_effective_state()` which considers both mouse interaction
         * state AND keyboard focus, so focused widgets show hover state.
         *
         * @example
         * @code
         * auto mnemonic_fg = get_state_mnemonic_foreground(theme->button);
         * @endcode
         */
        template<typename WidgetTheme>
        [[nodiscard]] color_type get_state_mnemonic_foreground(const WidgetTheme& widget_theme) const noexcept {
            switch (get_effective_state()) {
                case interaction_state::disabled:
                    return widget_theme.disabled.mnemonic_foreground;
                case interaction_state::pressed:
                    return widget_theme.pressed.mnemonic_foreground;
                case interaction_state::hover:
                    return widget_theme.hover.mnemonic_foreground;
                case interaction_state::normal:
                default:
                    return widget_theme.normal.mnemonic_foreground;
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
         * Uses `get_effective_state()` which considers both mouse interaction
         * state AND keyboard focus, so focused widgets show hover state.
         *
         * @example
         * @code
         * auto font = get_state_font(theme->button);
         * @endcode
         */
        template<typename WidgetTheme>
        [[nodiscard]] font_type get_state_font(const WidgetTheme& widget_theme) const noexcept {
            switch (get_effective_state()) {
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
         * @brief Handle mouse events and manage interaction state
         * @param mouse Mouse event containing action, position, button, and modifiers
         * @return true if event was handled
         *
         * @details
         * Unified mouse event handler that automatically manages interaction state:
         * - **Hover changes**: Sets hover state when entering, normal when leaving
         * - **Mouse press**: Sets pressed state if enabled
         * - **Mouse release**: Returns to hover (if still hovering) or normal
         * - **Pressed drag-out**: Maintains pressed state until release
         *
         * The interaction state provides visual feedback for user actions.
         */
        bool handle_mouse(const mouse_event& mouse) override {
            if (!this->is_enabled()) {
                return base::handle_mouse(mouse);
            }

            // Track hover state changes for enter/leave detection
            bool const was_hovered = base::is_hovered();

            // Let base class update hover/pressed state
            bool handled = base::handle_mouse(mouse);

            // Detect hover state changes after base class processing
            bool const now_hovered = base::is_hovered();

            // Update interaction state based on hover changes
            if (now_hovered && !was_hovered) {
                // Mouse entered
                set_interaction_state(interaction_state::hover);
            } else if (!now_hovered && was_hovered) {
                // Mouse left - only return to normal if not currently pressed
                // If pressed, we'll transition on release instead
                if (m_state != interaction_state::pressed) {
                    set_interaction_state(interaction_state::normal);
                }
            }

            // Update interaction state based on mouse actions
            switch (mouse.act) {
                case mouse_event::action::press:
                    // Mouse button pressed - set pressed state
                    set_interaction_state(interaction_state::pressed);
                    break;

                case mouse_event::action::release:
                    // Mouse button released - return to hover (if hovering) or normal
                    set_interaction_state(now_hovered ? interaction_state::hover : interaction_state::normal);
                    break;

                case mouse_event::action::move:
                case mouse_event::action::wheel_up:
                case mouse_event::action::wheel_down:
                    // Move and wheel events handled by hover tracking above
                    break;
            }

            return handled;
        }

        // ===================================================================
        // Effective State (considers both interaction state AND focus)
        // ===================================================================

        /**
         * @brief Get effective interaction state considering focus
         * @return Effective state for rendering
         *
         * @details
         * Returns the interaction state that should be used for rendering,
         * taking into account both the mouse interaction state AND keyboard focus.
         *
         * **Priority:**
         * 1. Disabled state (highest priority)
         * 2. Pressed state
         * 3. Hover state OR focused state (keyboard focus shows as hover)
         * 4. Normal state (default)
         *
         * This allows keyboard focus (Tab navigation) to show the same visual
         * feedback as mouse hover, providing clear indication of which widget
         * is active.
         */
        [[nodiscard]] interaction_state get_effective_state() const noexcept {
            // Disabled always takes priority
            if (m_state == interaction_state::disabled) {
                return interaction_state::disabled;
            }

            // Pressed state takes second priority
            if (m_state == interaction_state::pressed) {
                return interaction_state::pressed;
            }

            // Keyboard focus OR mouse hover both show as hover state
            if (this->has_focus() || m_state == interaction_state::hover) {
                return interaction_state::hover;
            }

            // Default to normal
            return interaction_state::normal;
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
