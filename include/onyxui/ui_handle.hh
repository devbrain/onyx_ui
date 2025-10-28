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
         * 2. Renders background via ui_services
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

            // Get dirty regions from previous frame
            auto dirty_regions = m_root->get_and_clear_dirty_regions();

            // Add dirty regions from removed layers (fixes menu switching artifacts)
            if (auto* layers = ui_services<Backend>::layers()) {
                const auto& removed_regions = layers->get_removed_layer_dirty_regions();
                dirty_regions.insert(dirty_regions.end(), removed_regions.begin(), removed_regions.end());
                layers->clear_removed_layer_dirty_regions();
            }

            // Render background BEFORE widgets (from ui_services)
            if (auto* bg = ui_services<Backend>::background()) {
                bg->render(m_renderer, bounds, dirty_regions);
            }

            // Two-pass layout for base UI
            [[maybe_unused]] auto measured_size = m_root->measure(rect_utils::get_width(bounds),
                          rect_utils::get_height(bounds));
            m_root->arrange(bounds);

            // Get global theme ONCE at rendering entry point (REQUIRED - theme cannot be null)
            auto* themes = ui_services<Backend>::themes();
            if (!themes) {
                throw std::runtime_error("Theme service not initialized! Ensure ui_context is created before rendering.");
            }
            auto* theme_ptr = themes->get_current_theme();
            if (!theme_ptr) {
                throw std::runtime_error("No current theme set! Ensure themes are registered before rendering.");
            }
            const auto& theme = *theme_ptr;

            // Render base UI to back buffer with dirty rectangle optimization
            // Only renders widgets that intersect with dirty regions
            // Theme is passed down through the widget tree by reference
            m_root->render(m_renderer, theme);

            // Render all overlay layers from current context (pass theme for popup menus, dialogs)
            if (auto* layers = ui_services<Backend>::layers()) {
                layers->render_all_layers(m_renderer, bounds, theme);
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

            // Add dirty regions from removed layers (fixes menu switching artifacts)
            if (auto* layers = ui_services<Backend>::layers()) {
                const auto& removed_regions = layers->get_removed_layer_dirty_regions();
                dirty_regions.insert(dirty_regions.end(), removed_regions.begin(), removed_regions.end());
                layers->clear_removed_layer_dirty_regions();
            }

            // Render background BEFORE widgets (from ui_services)
            if (auto* bg = ui_services<Backend>::background()) {
                bg->render(m_renderer, bounds, dirty_regions);
            }

            // Two-pass layout for base UI
            [[maybe_unused]] auto measured_size = m_root->measure(rect_utils::get_width(bounds),
                          rect_utils::get_height(bounds));
            m_root->arrange(bounds);

            // Get global theme ONCE at rendering entry point (REQUIRED - theme cannot be null)
            auto* themes = ui_services<Backend>::themes();
            if (!themes) {
                throw std::runtime_error("Theme service not initialized! Ensure ui_context is created before rendering.");
            }
            auto* theme_ptr = themes->get_current_theme();
            if (!theme_ptr) {
                throw std::runtime_error("No current theme set! Ensure themes are registered before rendering.");
            }
            const auto& theme = *theme_ptr;

            // Render base UI to back buffer with dirty rectangle optimization
            // Only renders widgets that intersect with dirty regions
            // Theme is passed down through the widget tree by reference
            m_root->render(m_renderer, theme);

            // Render all overlay layers from current context (pass theme for popup menus, dialogs)
            if (auto* layers = ui_services<Backend>::layers()) {
                layers->render_all_layers(m_renderer, bounds, theme);
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
                // Runtime check: is this actually a key press event?
                if (event_traits<event_type>::is_key_press(event)) {
                    // 1. Handle Tab navigation (input_manager traverses tree automatically)
                    if (auto* input = ui_services<Backend>::input()) {
                        if (input->handle_tab_navigation_in_tree(event, m_root.get())) {
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
                    if (auto* input = ui_services<Backend>::input()) {
                        auto* focused_target = input->get_focused();
                        focused = static_cast<widget_type*>(focused_target);
                    }

                    // 4. Forward keyboard event to focused widget
                    if (focused) {
                        return focused->process_event(event);
                    }
                }
            }

            // ============================================================
            // Mouse Event Handling (Unified with Input Manager)
            // ============================================================
            if constexpr (MousePositionEvent<event_type>) {
                auto* input = ui_services<Backend>::input();
                if (!input) return false;  // No input manager available

                int mouse_x = event_traits<event_type>::mouse_x(event);
                int mouse_y = event_traits<event_type>::mouse_y(event);

                // Check if this is a button press/release event
                bool is_button_event = false;
                bool is_press = false;
                if constexpr (MouseButtonEvent<event_type>) {
                    is_button_event = true;
                    is_press = event_traits<event_type>::is_button_press(event);
                }

                // Determine target widget:
                // - If mouse is captured, route to captured widget
                // - Otherwise, hit test to find widget under mouse
                auto* captured_target = input->get_captured();
                auto* captured = static_cast<widget_type*>(captured_target);
                widget_type* target_widget = captured ? captured : m_root->hit_test(mouse_x, mouse_y);

                // Handle hover changes (only when NOT captured)
                if (!captured) {
                    input->set_hover(target_widget);
                }

                // Handle mouse capture
                if (is_button_event) {
                    if (is_press) {
                        // WORKAROUND: Some backends (like termbox2) don't send mouse release events.
                        // If we're pressing on a different widget than what's captured,
                        // implicitly release the old capture first by sending it a mouse up event.
                        auto* currently_captured = input->get_captured();
                        if (currently_captured && currently_captured != target_widget) {
                            // Send mouse up to previously captured widget to clean up its state
                            static_cast<widget_type*>(currently_captured)->handle_mouse_up(mouse_x, mouse_y, 1);
                            input->release_capture();
                        }

                        // Mouse button down - capture mouse to this widget
                        input->set_capture(target_widget);
                        // Note: set_capture also sets focus if widget is focusable
                    } else {
                        // Mouse button up - release capture
                        // Note: This branch may never execute on backends like termbox2 that don't
                        // send release events, but we keep it for backends that do.
                        input->release_capture();

                        // After releasing capture, update hover state based on current mouse position
                        // This ensures is_hovered() is correct when handle_mouse_up() is called
                        widget_type* hover_target = m_root->hit_test(mouse_x, mouse_y);
                        input->set_hover(hover_target);
                    }
                }

                // Route event to target widget (either captured or under mouse)
                if (target_widget) {
                    // Route event through process_event to get proper click generation
                    return target_widget->process_event(event);
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

        // NOTE: input() and layers() accessors removed
        // Use ui_services<Backend>::input() and ui_services<Backend>::layers() instead

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