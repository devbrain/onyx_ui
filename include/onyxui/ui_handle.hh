/**
 * @file ui_handle.hh
 * @brief Backend-agnostic UI handle for display and event management
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Provides a simple, backend-agnostic handle for managing UI display,
 * event handling, and presentation. This allows the same UI code to
 * work with any backend without changing the application structure.
 *
 * ## Design Philosophy
 *
 * The ui_handle class provides three essential methods:
 * - display(): Measure, arrange, and render the UI
 * - handle_event(): Process input events
 * - present(): Present the rendered frame
 *
 * This design allows the UI library to integrate cleanly with existing
 * applications rather than taking over the event loop.
 *
 * ## Usage Pattern
 *
 * @code
 * // Create UI
 * auto ui = create_my_ui<conio_backend>();
 *
 * // Main loop (controlled by application)
 * while (!quit) {
 *     ui.display();
 *
 *     if (auto event = poll_event()) {
 *         ui.handle_event(event);
 *     }
 *
 *     ui.present();
 * }
 * @endcode
 */

#pragma once

#include <memory>
#include <iostream>
#include <onyxui/concepts/backend.hh>
#include <onyxui/concepts/rect_like.hh>
#include <onyxui/concepts/event_like.hh>
#include <onyxui/element.hh>
#include <onyxui/focus_manager.hh>
#include <onyxui/layer_manager.hh>
#include <onyxui/ui_services.hh>
#include <onyxui/widgets/widget.hh>

namespace onyxui {

    /**
     * @class ui_handle
     * @brief Backend-agnostic handle for UI display and event management
     *
     * @tparam Backend The UI backend type satisfying UIBackend concept
     *
     * @details
     * ui_handle provides a simple interface for managing a UI tree.
     * It encapsulates the root widget, renderer, and provides methods
     * for display, event handling, and presentation.
     *
     * ## Lifecycle
     *
     * 1. Create ui_handle with root widget
     * 2. Call display() to measure, arrange, and render
     * 3. Call handle_event() for each input event
     * 4. Call present() to show the frame
     *
     * ## Integration with Applications
     *
     * ui_handle is designed to integrate with existing applications:
     * - Game engines can render UI on top of game graphics
     * - Terminal apps can create rich TUIs
     * - Desktop apps can embed UI panels
     *
     * @example Game Integration
     * @code
     * class Game {
     *     ui_handle<sdl_backend> menu_ui;
     *     ui_handle<sdl_backend> hud_ui;
     *
     *     void render() {
     *         render_world();
     *         hud_ui.display();
     *         if (menu_open) menu_ui.display();
     *         SDL_RenderPresent(renderer);
     *     }
     * };
     * @endcode
     */
    template<UIBackend Backend>
    class ui_handle {
    public:
        using widget_type = ui_element<Backend>;
        using renderer_type = typename Backend::renderer_type;
        using rect_type = typename Backend::rect_type;
        using event_type = typename Backend::event_type;

    private:
        std::unique_ptr<widget_type> m_root;
        renderer_type m_renderer;
        widget_type* m_hovered_widget = nullptr;  ///< Widget currently under mouse

    public:
        /**
         * @brief Construct a UI handle with root widget
         * @param root The root widget (takes ownership)
         *
         * @details
         * The ui_handle takes ownership of the root widget and
         * creates its own renderer instance. Services (layer manager,
         * focus manager) are accessed from the current ui_context
         * via ui_services.
         */
        explicit ui_handle(std::unique_ptr<widget_type> root)
            : m_root(std::move(root))
            , m_renderer() {
        }

        /**
         * @brief Construct a UI handle with root widget and custom renderer
         * @param root The root widget (takes ownership)
         * @param renderer The renderer to use
         *
         * @details
         * This constructor allows passing a pre-configured renderer,
         * useful when the renderer needs special initialization.
         * Services (layer manager, focus manager) are accessed from
         * the current ui_context via ui_services.
         */
        ui_handle(std::unique_ptr<widget_type> root, renderer_type renderer)
            : m_root(std::move(root))
            , m_renderer(std::move(renderer)) {
        }

        /**
         * @brief Destructor
         *
         * @details
         * Destroys the UI handle and releases the root widget.
         * Services remain in the ui_context managed by scoped_ui_context.
         */
        ~ui_handle() = default;

        /**
         * @brief Display the UI (measure, arrange, render)
         *
         * @details
         * Performs the complete layout and rendering pipeline:
         * 1. Gets viewport bounds from renderer
         * 2. Clears any dirty regions from previous frame
         * 3. Measures the UI tree
         * 4. Arranges widgets within bounds
         * 5. Renders to the back buffer
         *
         * Does not present the frame - call present() for that.
         */
        void display() {
            if (!m_root) return;

            // Get viewport from renderer (renderer knows its size)
            auto bounds = m_renderer.get_viewport();

            // DEBUG: Check what bounds renderer is returning
            int bounds_x = rect_utils::get_x(bounds);
            int bounds_y = rect_utils::get_y(bounds);
            int bounds_w = rect_utils::get_width(bounds);
            int bounds_h = rect_utils::get_height(bounds);
            auto y_unsigned = static_cast<unsigned int>(bounds_y);
            if (bounds_y < 0 || bounds_y > 10000 || y_unsigned > 10000) {
                std::cerr << "UI_HANDLE::display(): Renderer returned bad viewport!"
                          << " x=" << bounds_x << " y=" << bounds_y
                          << " (unsigned: " << y_unsigned << ")"
                          << " w=" << bounds_w << " h=" << bounds_h << std::endl;
                std::cerr.flush();
            }

            // Get dirty regions from previous frame
            auto dirty_regions = m_root->get_and_clear_dirty_regions();

            // Clear dirty regions before rendering
            for (const auto& region : dirty_regions) {
                m_renderer.clear_region(region);
            }

            // Two-pass layout for base UI
            [[maybe_unused]] auto measured_size = m_root->measure(rect_utils::get_width(bounds),
                          rect_utils::get_height(bounds));
            m_root->arrange(bounds);

            // Render base UI to back buffer with dirty rectangle optimization
            // Only renders widgets that intersect with dirty regions
            m_root->render(m_renderer, dirty_regions);

            // Render all overlay layers from current context
            if (auto* layers = ui_services<Backend>::layers()) {
                layers->render_all_layers(m_renderer, bounds);
            }
        }

        /**
         * @brief Display the UI within specific bounds
         * @param bounds The bounds to render within
         *
         * @details
         * Same as display() but uses provided bounds instead of
         * asking the renderer for viewport. Useful for rendering
         * UI in a subsection of the screen.
         */
        void display(const rect_type& bounds) {
            if (!m_root) return;

            // Get dirty regions from previous frame
            auto dirty_regions = m_root->get_and_clear_dirty_regions();

            // Clear dirty regions before rendering
            for (const auto& region : dirty_regions) {
                m_renderer.clear_region(region);
            }

            // Two-pass layout for base UI
            [[maybe_unused]] auto measured_size = m_root->measure(rect_utils::get_width(bounds),
                          rect_utils::get_height(bounds));
            m_root->arrange(bounds);

            // Render base UI to back buffer with dirty rectangle optimization
            // Only renders widgets that intersect with dirty regions
            m_root->render(m_renderer, dirty_regions);

            // Render all overlay layers from current context
            if (auto* layers = ui_services<Backend>::layers()) {
                layers->render_all_layers(m_renderer, bounds);
            }
        }

        /**
         * @brief Present the rendered frame
         *
         * @details
         * Presents the back buffer to the screen. This should be
         * called after display() to make the rendered UI visible.
         *
         * The actual presentation is handled by the renderer,
         * which knows how to present for its specific backend.
         */
        void present() {
            m_renderer.present();
        }

        /**
         * @brief Handle an input event
         * @param event The event to handle
         * @return true if the event was consumed, false otherwise
         *
         * @details
         * Processes an input event through the UI tree. Events are
         * dispatched based on focus, mouse position, and widget state.
         *
         * **Keyboard Event Routing:**
         * 1. Tab navigation - handled by hierarchical_focus_manager
         * 2. Global hotkeys - checked on root widget (works regardless of focus)
         * 3. Focused widget - keyboard events routed to focused widget
         *
         * **Mouse Event Routing:**
         * 1. Hit testing - finds deepest widget under mouse cursor
         * 2. Enter/Leave - tracks hovered widget, sends enter/leave events
         * 3. Button events - routes down/up to widget under mouse
         * 4. Focus on click - clicking a focusable widget sets focus
         * 5. Move events - routes move to widget under mouse
         *
         * The hierarchical_focus_manager automatically traverses the
         * widget tree to find focusable widgets for Tab navigation.
         *
         * Hit testing uses reverse z-order traversal (top widgets first)
         * and respects visibility (hidden widgets are not hit).
         *
         * Returns true if the UI handled the event, allowing the
         * application to skip its own event processing.
         *
         * @example
         * @code
         * while (running) {
         *     ui.display(bounds);
         *     ui.present();
         *
         *     if (auto event = poll_event()) {
         *         if (!ui.handle_event(event)) {
         *             // UI didn't handle it, process in game
         *             game.handle_event(event);
         *         }
         *     }
         * }
         * @endcode
         */
        bool handle_event(const event_type& event) {
            if (!m_root) return false;

            // ============================================================
            // Layer Event Routing - FIRST PRIORITY!
            // ============================================================
            // Route event through layers first (top to bottom).
            // If a layer handles the event or a modal blocks it, don't route to base UI.
            if (auto* layers = ui_services<Backend>::layers()) {
                if (layers->route_event(event)) {
                    return true;  // Layer handled the event
                }
            }

            // ============================================================
            // Window Event Handling (Resize) - CHECK FIRST!
            // ============================================================
            // Important: Check resize BEFORE other event types because union-style
            // event structures (like tb_event) may satisfy multiple concepts at
            // compile-time, but only represent one event type at runtime.
            if constexpr (WindowEvent<event_type>) {
                // Runtime check: is this actually a resize event?
                if (event_traits<event_type>::is_resize_event(event)) {
                    // Notify renderer to resize its buffers
                    m_renderer.on_resize();

                    // Get new window dimensions
                    int new_width = event_traits<event_type>::window_width(event);
                    int new_height = event_traits<event_type>::window_height(event);

                    // Remeasure and rearrange UI for new dimensions
                    rect_type new_bounds;
                    rect_utils::set_bounds(new_bounds, 0, 0, new_width, new_height);

                    [[maybe_unused]] auto measured_size = m_root->measure(new_width, new_height);
                    m_root->arrange(new_bounds);

                    return true;  // Resize handled
                }
            }

            // ============================================================
            // Keyboard Event Handling
            // ============================================================
            if constexpr (KeyboardEvent<event_type>) {
                // 1. Handle Tab navigation (focus_manager traverses tree automatically)
                if (auto* focus = ui_services<Backend>::focus()) {
                    if (focus->handle_tab_navigation_in_tree(event, m_root.get())) {
                        return true;  // Tab was handled, focus changed
                    }
                }

                // 2. Try global hotkeys from service locator
                // Global hotkeys work regardless of focus
                if (auto* hotkeys = ui_services<Backend>::hotkeys()) {
                    if (hotkeys->handle_key_event(event, nullptr)) {
                        return true;  // Global hotkey handled
                    }
                }

                // 3. Get focused widget for forwarding
                widget_type* focused = nullptr;
                if (auto* focus = ui_services<Backend>::focus()) {
                    auto* focused_target = focus->get_focused();
                    focused = static_cast<widget_type*>(focused_target);
                }

                // 4. Forward keyboard event to focused widget
                if (focused) {
                    return focused->process_event(event);
                }
            }

            // ============================================================
            // Mouse Event Handling
            // ============================================================
            if constexpr (MousePositionEvent<event_type>) {
                int mouse_x = event_traits<event_type>::mouse_x(event);
                int mouse_y = event_traits<event_type>::mouse_y(event);

                // Perform hit testing to find widget under mouse
                widget_type* hit_widget = m_root->hit_test(mouse_x, mouse_y);

                // Handle mouse enter/leave when hovered widget changes
                if (hit_widget != m_hovered_widget) {
                    // Mouse left previous widget
                    if (m_hovered_widget) {
                        m_hovered_widget->handle_mouse_leave();
                    }

                    // Mouse entered new widget
                    m_hovered_widget = hit_widget;
                    if (m_hovered_widget) {
                        m_hovered_widget->handle_mouse_enter();
                    }
                }

                // Route event to widget under mouse
                if (hit_widget) {
                    // Check if this is a button event
                    if constexpr (MouseButtonEvent<event_type>) {
                        if (bool is_press = event_traits<event_type>::is_button_press(event)) {
                            // Mouse button down - set focus to clicked widget
                            if (hit_widget->is_focusable() && hit_widget->is_enabled()) {
                                if (auto* focus = ui_services<Backend>::focus()) {
                                    focus->set_focus(hit_widget);
                                }
                            }
                        }
                    }

                    // Route event through process_event to get proper click generation
                    return hit_widget->process_event(event);
                }
            }

            return false;  // Event not handled
        }

        /**
         * @brief Get the root widget
         * @return Pointer to root widget (or nullptr)
         */
        [[nodiscard]] widget_type* root() noexcept {
            return m_root.get();
        }

        /**
         * @brief Get the root widget (const)
         * @return Const pointer to root widget (or nullptr)
         */
        [[nodiscard]] const widget_type* root() const noexcept {
            return m_root.get();
        }

        /**
         * @brief Get the renderer
         * @return Reference to the renderer
         */
        [[nodiscard]] renderer_type& renderer() noexcept {
            return m_renderer;
        }

        /**
         * @brief Get the renderer (const)
         * @return Const reference to the renderer
         */
        [[nodiscard]] const renderer_type& renderer() const noexcept {
            return m_renderer;
        }

        /**
         * @brief Set a new root widget
         * @param new_root The new root widget (takes ownership)
         */
        void set_root(std::unique_ptr<widget_type> new_root) {
            m_root = std::move(new_root);
        }

        /**
         * @brief Check if UI handle has a root widget
         * @return true if has root, false otherwise
         */
        [[nodiscard]] bool has_root() const noexcept {
            return m_root != nullptr;
        }

        // NOTE: focus() and layers() accessors removed
        // Use ui_services<Backend>::focus() and ui_services<Backend>::layers() instead

        /**
         * @brief Mark a region as dirty (needs redrawing)
         * @param region The region that needs redrawing
         *
         * @details
         * Widgets should call this when their visual state changes.
         * The region will be collected by the root element and
         * cleared before the next render.
         */
        void mark_dirty(const rect_type& region) {
            if (m_root) {
                m_root->mark_dirty_region(region);
            }
        }

        /**
         * @brief Mark the entire viewport as dirty
         *
         * @details
         * Use this when the entire UI needs to be redrawn,
         * such as after a major state change or theme switch.
         */
        void mark_all_dirty() {
            auto viewport = m_renderer.get_viewport();
            if (m_root) {
                m_root->mark_dirty_region(viewport);
            }
        }
    };

} // namespace onyxui