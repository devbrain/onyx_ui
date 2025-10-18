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
 *     void do_render(renderer_type& renderer) override {
 *         // Custom rendering
 *     }
 * };
 * @endcode
 */

#pragma once

#include <onyxui/element.hh>
#include <onyxui/signal.hh>
#include <onyxui/layout/linear_layout.hh>
#include <onyxui/hotkeys/hotkey_manager.hh>
#include <memory>

namespace onyxui {
    // Forward declaration for pointer usage
    template<UIBackend Backend>
    class action;
}

// Include after forward declaration to avoid circular dependency issues
#include <onyxui/widgets/action.hh>

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
            bool was_enabled = this->base::is_enabled();
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
            bool was_visible = this->is_visible();
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
         * @brief Get the hotkey manager for this widget
         * @return Reference to the widget's hotkey manager
         *
         * @details
         * Every widget has its own hotkey manager that can register
         * keyboard shortcuts and handle key events. This allows widgets
         * to define their own local keyboard shortcuts.
         *
         * @example
         * @code
         * auto panel = std::make_unique<panel<Backend>>();
         *
         * // Register a hotkey
         * auto save_action = std::make_shared<action<Backend>>();
         * save_action->set_shortcut('s');
         * save_action->triggered.connect([]() { save_document(); });
         * panel->hotkeys().register_action(save_action);
         *
         * // Handle key events
         * panel->hotkeys().handle_key_event(event, focused_widget);
         * @endcode
         */
        [[nodiscard]] hotkey_manager<Backend>& hotkeys() noexcept {
            return m_hotkey_manager;
        }

        /**
         * @brief Get the hotkey manager for this widget (const)
         * @return Const reference to the widget's hotkey manager
         */
        [[nodiscard]] const hotkey_manager<Backend>& hotkeys() const noexcept {
            return m_hotkey_manager;
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
        hotkey_manager<Backend> m_hotkey_manager;                    ///< Widget's hotkey manager

    protected:
        /**
         * @brief Apply theme to this widget (must be implemented by derived classes)
         */
        void do_apply_theme([[maybe_unused]] const typename base::theme_type& theme) override {
            // Base widget has no visual representation to theme
            // Derived classes should override this
        }

        /**
         * @brief Handle mouse enter event (called by event_target)
         */
        bool handle_mouse_enter() override {
            mouse_entered.emit();
            return base::handle_mouse_enter();
        }

        /**
         * @brief Handle mouse leave event (called by event_target)
         */
        bool handle_mouse_leave() override {
            mouse_exited.emit();
            return base::handle_mouse_leave();
        }

        /**
         * @brief Handle click event (called by event_target)
         */
        bool handle_click([[maybe_unused]] int x, [[maybe_unused]] int y) override {
            clicked.emit();
            return true;  // Consumed
        }

        /**
         * @brief Handle mouse move event
         */
        bool handle_mouse_move(int x, int y) override {
            mouse_moved.emit(x, y);
            return base::handle_mouse_move(x, y);
        }

        /**
         * @brief Handle focus gained event (called by event_target)
         */
        bool handle_focus_gained() override {
            focus_gained.emit();
            return base::handle_focus_gained();
        }

        /**
         * @brief Handle focus lost event (called by event_target)
         */
        bool handle_focus_lost() override {
            focus_lost.emit();
            return base::handle_focus_lost();
        }
    };
}
