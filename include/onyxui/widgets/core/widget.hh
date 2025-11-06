/**
 * @file widget.hh
 * @brief Base widget class with integrated signal/slot support
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Provides the base widget class that combines ui_element layout capabilities
 * with signal/slot event handling. All concrete widgets inherit from this class.
 *
 * ## Architecture
 *
 * widget extends ui_element by adding:
 * - Common signals for user interaction (clicked, focused, etc.)
 * - Virtual rendering method for backends
 * - Enable/disable state management
 * - Signal emission in event handlers
 *
 * ## Usage Pattern
 *
 * @code
 * template<UIBackend Backend>
 * class my_widget : public widget<Backend> {
 * protected:
 *     void do_render(render_context<Backend>& ctx) const override {
 *         // Custom rendering - same code handles BOTH measurement and rendering
 *         // Widgets are completely unaware of context type (visitor pattern)
 *         ctx.draw_text(m_text, position, font, color);
 *         ctx.draw_rect(bounds, box_style);
 *     }
 * };
 * @endcode
 */

#pragma once

#include <onyxui/core/element.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/layout/linear_layout.hh>
#include <onyxui/core/rendering/measure_context.hh>
#include <onyxui/services/ui_services.hh>
#include <onyxui/core/raii/scoped_layer.hh>
#include <onyxui/services/layer_manager.hh>
#include <memory>
#include <stdexcept>

namespace onyxui {
    // Forward declarations
    template<UIBackend Backend>
    class action;

    template<UIBackend Backend>
    class menu;

    template<UIBackend Backend>
    class label;

    template<UIBackend Backend>
    class scoped_tooltip;
}

// Include after forward declaration to avoid circular dependency issues
#include <onyxui/actions/action.hh>

namespace onyxui {
    /**
     * @class widget
     * @brief Base class for all interactive UI widgets
     *
     * @tparam Backend The UI backend type satisfying UIBackend concept
     *
     * @details
     * widget is the base class for all concrete widgets in the framework.
     * It combines layout management from ui_element with event handling
     * through signals, providing a complete foundation for interactive UI.
     *
     * ## Common Signals
     *
     * All widgets provide these signals:
     * - clicked: Emitted on mouse button release (full click)
     * - double_clicked: Emitted on double-click
     * - mouse_entered: Emitted when mouse enters widget bounds
     * - mouse_exited: Emitted when mouse leaves widget bounds
     * - mouse_moved: Emitted with x, y coordinates
     * - focus_gained: Emitted when widget receives focus
     * - focus_lost: Emitted when widget loses focus
     * - enabled_changed: Emitted when enabled state changes
     * - visible_changed: Emitted when visibility changes
     *
     * ## Event Handling
     *
     * Override event_target methods to handle low-level events, or
     * connect to signals for high-level reactive programming.
     *
     * @example Basic Widget Usage
     * @code
     * auto button = std::make_unique<widget<Backend>>(nullptr);
     * button->clicked.connect([]() {
     *     std::cout << "Button clicked!" << std::endl;
     * });
     * @endcode
     */
    template<UIBackend Backend>
    class widget : public ui_element<Backend> {
    public:
        using base = ui_element<Backend>;
        using renderer_type = typename Backend::renderer_type;
        using point_type = typename Backend::point_type;
        using event_type = typename Backend::event_type;

        // Common signals
        signal<> clicked;                       ///< Emitted on complete click (press + release)
        signal<> double_clicked;                ///< Emitted on double-click
        signal<> mouse_entered;                 ///< Emitted when mouse enters widget
        signal<> mouse_exited;                  ///< Emitted when mouse leaves widget
        signal<int, int> mouse_moved;           ///< Emitted with x, y when mouse moves
        signal<> focus_gained;                  ///< Emitted when focus is gained
        signal<> focus_lost;                    ///< Emitted when focus is lost
        signal<bool> enabled_changed;           ///< Emitted when enabled state changes
        signal<bool> visible_changed;           ///< Emitted when visibility changes

        /**
         * @brief Construct a widget
         * @param parent Pointer to parent element (nullptr for root)
         */
        explicit widget(ui_element<Backend>* parent = nullptr)
            : base(parent) {}

        /**
         * @brief Virtual destructor
         */
        ~widget() override = default;

        // Disable copy, allow move
        widget(const widget&) = delete;
        widget& operator=(const widget&) = delete;
        widget(widget&&) noexcept = default;
        widget& operator=(widget&&) noexcept = default;

        /**
         * @brief Set enabled state with signal emission
         * @param enabled True to enable, false to disable
         *
         * @details
         * Disabled widgets don't respond to user interaction and are
         * typically rendered with a dimmed appearance. This overrides
         * event_target::set_enabled() to also emit a signal.
         */
        void set_enabled(bool enabled) {
            bool const was_enabled = this->base::is_enabled();
            this->base::set_enabled(enabled);
            if (was_enabled != enabled) {
                enabled_changed.emit(enabled);
                this->invalidate_arrange();  // May need visual update
            }
        }

        // Note: is_enabled() is inherited from event_target<Backend>

        /**
         * @brief Set visibility with signal emission
         * @param visible True to show, false to hide
         *
         * @details
         * This overrides ui_element::set_visible() to also emit a signal.
         */
        void set_visible(bool visible) {
            bool const was_visible = this->is_visible();
            base::set_visible(visible);
            if (was_visible != visible) {
                visible_changed.emit(visible);
            }
        }

        // Note: is_hovered() and is_pressed() are inherited from event_target<Backend>

        /**
         * @brief Set vertical box layout strategy
         * @param spacing Spacing between children in pixels
         *
         * @details
         * Convenience method that sets a vertical linear layout strategy.
         * Children will be stacked vertically with the specified spacing.
         *
         * @example
         * @code
         * auto panel = std::make_unique<panel<Backend>>();
         * panel->set_vbox_layout(5);  // 5px spacing between children
         * @endcode
         */
        void set_vbox_layout(int spacing = 0) {
            this->set_layout_strategy(
                std::make_unique<linear_layout<Backend>>(direction::vertical, spacing)
            );
        }

        /**
         * @brief Set horizontal box layout strategy
         * @param spacing Spacing between children in pixels
         *
         * @details
         * Convenience method that sets a horizontal linear layout strategy.
         * Children will be laid out horizontally with the specified spacing.
         *
         * @example
         * @code
         * auto toolbar = std::make_unique<panel<Backend>>();
         * toolbar->set_hbox_layout(2);  // 2px spacing between buttons
         * @endcode
         */
        void set_hbox_layout(int spacing = 0) {
            this->set_layout_strategy(
                std::make_unique<linear_layout<Backend>>(direction::horizontal, spacing)
            );
        }

        /**
         * @brief Associate an action with this widget
         *
         * @param action_ptr Shared pointer to the action to associate
         *
         * @details
         * Associates an action with this widget, establishing bidirectional
         * synchronization. The widget will:
         * - Sync its enabled state with the action
         * - Derived classes can customize behavior via on_action_changed()
         *
         * When the widget is destroyed, all connections are automatically
         * cleaned up via scoped_connection.
         *
         * **Ownership:** Widget holds a weak_ptr, so it doesn't keep the
         * action alive. Action should be owned by the application.
         *
         * @example
         * @code
         * auto save_action = std::make_shared<action<Backend>>();
         * save_action->set_text("Save");
         *
         * // Multiple widgets can share the same action
         * toolbar_button->set_action(save_action);
         * menu_item->set_action(save_action);
         *
         * // Disabling action disables all connected widgets
         * save_action->set_enabled(false);
         * @endcode
         *
         * @see on_action_changed For customizing action integration
         */
        void set_action(std::shared_ptr<action<Backend>> action_ptr) {
            m_action = action_ptr;
            on_action_changed(action_ptr);
        }

        /**
         * @brief Get the associated action
         *
         * @return Shared pointer to the action, or nullptr if no action or action destroyed
         *
         * @details
         * Returns the currently associated action. May return nullptr if:
         * - No action has been set
         * - The action was destroyed (weak_ptr expired)
         *
         * @example
         * @code
         * if (auto action = button->get_action()) {
         *     std::cout << "Action: " << action->text() << std::endl;
         * }
         * @endcode
         */
        [[nodiscard]] std::shared_ptr<action<Backend>> get_action() const noexcept {
            return m_action.lock();
        }

        // ================================================================
        // Layer Management Convenience Methods
        // ================================================================


        /**
         * @brief Show a tooltip with custom content
         * @param content Tooltip content (caller manages lifetime)
         * @return Layer ID, or invalid if no layer manager available
         *
         * @details
         * Shows a custom widget as a tooltip. Caller must ensure content
         * remains valid while the tooltip is visible.
         */
        layer_id show_tooltip(ui_element<Backend>* content) {
            auto* layers = ui_services<Backend>::layers();
            if (!layers) return layer_id::invalid();

            auto bounds = this->bounds();
            int x = rect_utils::get_x(bounds);
            int y = rect_utils::get_y(bounds) + rect_utils::get_height(bounds);

            return layers->show_tooltip(content, x, y);
        }

        /**
         * @brief Show a popup relative to this widget
         * @param content Popup content (caller manages lifetime)
         * @param placement Where to position the popup
         * @return Layer ID, or invalid if no layer manager available
         *
         * @example
         * @code
         * void show_dropdown() {
         *     this->show_popup(m_dropdown_menu.get(), popup_placement::below);
         * }
         * @endcode
         */
        layer_id show_popup(ui_element<Backend>* content,
                           popup_placement placement = popup_placement::below,
                           std::function<void()> outside_click_callback = nullptr) {
            auto* layers = ui_services<Backend>::layers();
            if (!layers) return layer_id::invalid();

            return layers->show_popup(content, this->bounds(), placement, std::move(outside_click_callback));
        }

        /**
         * @brief Show a context menu at widget position
         * @param menu_content Menu to show (caller manages lifetime)
         * @return Layer ID, or invalid if no layer manager available
         *
         * @example
         * @code
         * void on_mouse_down(const event_type& e) override {
         *     if (is_right_click(e)) {
         *         this->show_context_menu(m_context_menu.get());
         *     }
         * }
         * @endcode
         */
        layer_id show_context_menu(menu<Backend>* menu_content) {
            auto* layers = ui_services<Backend>::layers();
            if (!layers) return layer_id::invalid();

            // Phase 1.3: Pass callback to emit menu's closing signal on outside click
            return layers->show_popup(menu_content, this->bounds(), popup_placement::auto_best,
                [menu_content]() {
                    if (menu_content) {
                        menu_content->closing.emit();
                    }
                });
        }

        /**
         * @brief Show a modal dialog
         * @param content Dialog content (caller manages lifetime)
         * @return Layer ID, or invalid if no layer manager available
         */
        layer_id show_modal_dialog(ui_element<Backend>* content) {
            auto* layers = ui_services<Backend>::layers();
            if (!layers) return layer_id::invalid();

            return layers->show_modal_dialog(content, dialog_position::center);
        }

        // ================================================================
        // SCOPED VERSIONS (RAII cleanup)
        // ================================================================

        /**
         * @brief Show text tooltip with automatic cleanup
         * @param text Tooltip text
         * @return scoped_tooltip that owns both content and layer
         *
         * @details
         * Creates a label with the given text, shows it as a tooltip below
         * this widget, and returns a scoped_tooltip that owns both the label
         * and manages the layer lifetime.
         *
         * @note Requires scoped_tooltip.hh to be included in implementation file
         *
         * @example
         * @code
         * class help_button : public button<Backend> {
         *     scoped_tooltip<Backend> m_tooltip;
         *
         *     void on_mouse_enter() override {
         *         m_tooltip = this->show_tooltip_scoped("Click for help");
         *     }
         * };
         * @endcode
         */
        [[nodiscard]] scoped_tooltip<Backend> show_tooltip_scoped(const std::string& text);

        /**
         * @brief Show tooltip with custom content and automatic cleanup
         */
        [[nodiscard]] scoped_layer<Backend> show_tooltip_scoped(ui_element<Backend>* content) {
            return scoped_layer<Backend>(ui_services<Backend>::layers(), show_tooltip(content));
        }

        /**
         * @brief Show popup with automatic cleanup
         */
        [[nodiscard]] scoped_layer<Backend> show_popup_scoped(
            ui_element<Backend>* content,
            popup_placement placement = popup_placement::below) {

            return scoped_layer<Backend>(ui_services<Backend>::layers(), show_popup(content, placement));
        }

        /**
         * @brief Show context menu with automatic cleanup
         */
        [[nodiscard]] scoped_layer<Backend> show_context_menu_scoped(menu<Backend>* menu_content) {
            return scoped_layer<Backend>(ui_services<Backend>::layers(), show_context_menu(menu_content));
        }

        /**
         * @brief Show modal dialog with automatic cleanup
         */
        [[nodiscard]] scoped_layer<Backend> show_modal_dialog_scoped(ui_element<Backend>* content) {
            return scoped_layer<Backend>(ui_services<Backend>::layers(), show_modal_dialog(content));
        }

    protected:
        /**
         * @brief Called when action is set or changed (virtual customization point)
         *
         * @param action_ptr The new action (may be nullptr)
         *
         * @details
         * Virtual hook that derived classes can override to customize how
         * they integrate with actions.
         *
         * **Default behavior:**
         * - Syncs widget enabled state with action
         * - Sets up bidirectional connection for enabled state
         *
         * **Derived class examples:**
         * - button: Trigger action on click, sync text
         * - checkbox: Bidirectional checked state sync
         * - label: Sync text display
         *
         * **Connection Lifetime:**
         * Connections are stored in m_action_connections and automatically
         * cleaned up when the widget is destroyed or action is changed.
         *
         * @example Button override
         * @code
         * void on_action_changed(std::shared_ptr<action<Backend>> action_ptr) override {
         *     // Call base to sync enabled state
         *     widget<Backend>::on_action_changed(action_ptr);
         *
         *     if (action_ptr) {
         *         // Sync button text
         *         set_text(action_ptr->text());
         *
         *         // Trigger action on button click
         *         m_action_connections.emplace_back(
         *             this->clicked,
         *             [weak_action = std::weak_ptr(action_ptr)]() {
         *                 if (auto action = weak_action.lock()) {
         *                     action->trigger();
         *                 }
         *             }
         *         );
         *     }
         * }
         * @endcode
         */
        virtual void on_action_changed(std::shared_ptr<action<Backend>> action_ptr) {
            // Clear previous connections
            m_action_connections.clear();

            if (action_ptr) {
                // Sync initial enabled state
                this->set_enabled(action_ptr->is_enabled());

                // Connect action enabled changes to widget
                m_action_connections.emplace_back(
                    action_ptr->enabled_changed,
                    [this](bool enabled) {
                        this->set_enabled(enabled);
                    }
                );
            }
        }

        std::weak_ptr<action<Backend>> m_action;                     ///< Associated action (weak reference)
        std::vector<scoped_connection> m_action_connections;         ///< Auto-cleanup connections


        /**
         * @brief Automatic content size calculation via measure_context
         *
         * @details
         * Default implementation that automatically calculates size by calling
         * do_render() with a measure_context. This eliminates the need for widgets
         * to implement get_content_size() separately.
         *
         * **How it works:**
         * 1. Gets current theme and resolves style (same as rendering does)
         * 2. Creates a measure_context with resolved style
         * 3. Calls do_render(ctx) - same rendering code used for drawing
         * 4. Returns the measured size from the context
         *
         * **Benefits:**
         * - Single source of truth (do_render handles both measurement and rendering)
         * - Measurement uses same resolved style as rendering (prevents inconsistencies)
         * - Impossible for measurement and rendering to get out of sync
         * - ~50-70% less code in widget implementations
         *
         * **Override if needed:**
         * Widgets can still override this if they need special measurement logic
         * that differs from rendering (rare).
         */
        [[nodiscard]] typename base::size_type get_content_size() const override {
            // Get current theme from ui_services (same as rendering does)
            auto* theme = ui_services<Backend>::themes()
                ? ui_services<Backend>::themes()->get_current_theme()
                : nullptr;

            // Theme is required for measurement - if missing, it's a programming error
            if (!theme) {
                throw std::runtime_error(
                    "widget::get_content_size() called without theme - "
                    "ui_context not initialized or no theme set"
                );
            }

            // Create default parent style from theme
            // This mimics what ui_element::render() does for top-level rendering
            resolved_style<Backend> parent_style = resolved_style<Backend>::from_theme(*theme);

            // Resolve my style by merging parent_style with my overrides (same as rendering)
            auto style = this->resolve_style(theme, parent_style);

            // Create measure_context with resolved style (ensures measurement = rendering)
            // The theme is already set via the resolved_style, accessible via ctx.theme()
            measure_context<Backend> ctx(style);

            // "Render" for measurement (same code path as actual rendering)
            this->do_render(ctx);

            return ctx.get_size();
        }

        /**
         * @brief Handle mouse events and emit signals
         */
        bool handle_mouse(const mouse_event& mouse) override {
            // Track previous states for change detection
            bool const was_hovered = this->is_hovered();
            bool const was_pressed = this->is_pressed();

            // Let base class update hover/pressed state and generate click
            bool handled = base::handle_mouse(mouse);

            // Detect state changes and emit signals
            bool const now_hovered = this->is_hovered();
            bool const now_pressed = this->is_pressed();

            if (now_hovered != was_hovered) {
                this->mark_dirty();  // Visual state changed
                if (now_hovered) {
                    mouse_entered.emit();
                } else {
                    mouse_exited.emit();
                }
            }

            if (now_pressed != was_pressed) {
                this->mark_dirty();  // Visual state changed
            }

            // Emit click signal when release happens inside (base class returns true)
            if (mouse.act == mouse_event::action::release && handled) {
                clicked.emit();
            }

            // Emit mouse_moved for move events
            if (mouse.act == mouse_event::action::move && now_hovered) {
                mouse_moved.emit(mouse.x, mouse.y);
            }

            return handled;
        }

        /**
         * @brief Handle keyboard events and emit clicked signal for Enter/Space
         */
        bool handle_keyboard(const keyboard_event& kbd) override {
            // Track previous pressed state
            bool const was_pressed = this->is_pressed();

            // Let base class handle the keyboard event (Enter/Space set/clear pressed state)
            bool handled = base::handle_keyboard(kbd);

            // Detect pressed state change
            bool const now_pressed = this->is_pressed();
            if (now_pressed != was_pressed) {
                this->mark_dirty();  // Visual state changed
            }

            // Emit click signal when Enter/Space is released
            if (handled && !kbd.pressed && this->accepts_keys_as_click()) {
                if (kbd.key == key_code::enter || kbd.key == key_code::space) {
                    clicked.emit();
                }
            }

            return handled;
        }

        /**
         * @brief Handle focus state changes
         */
        void on_focus_changed(bool gained) override {
            // Mark dirty for visual state change
            this->mark_dirty();

            // Emit appropriate signal
            if (gained) {
                focus_gained.emit();
            } else {
                focus_lost.emit();
            }
        }
    };
}
