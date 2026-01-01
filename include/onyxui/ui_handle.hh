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
#include <onyxui/concepts/backend.hh>
#include <onyxui/concepts/rect_like.hh>
#include <onyxui/core/element.hh>
#include <onyxui/events/event_router.hh>
#include <onyxui/events/hit_test_path.hh>
#include <onyxui/hotkeys/hotkey_action.hh>
#include <onyxui/services/focus_manager.hh>
#include <onyxui/services/ui_services.hh>
#include <onyxui/widgets/core/widget.hh>

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
            display_impl(bounds);
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
            display_impl(bounds);
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
        bool handle_event(const event_type& native_event) {
            if (!m_root) return false;

            // ============================================================
            // Event Conversion
            // ============================================================
            // Convert backend-specific event to unified ui_event
            auto ui_evt_opt = Backend::create_event(native_event);
            if (!ui_evt_opt) {
                // Backend couldn't convert event (unsupported/malformed)
                return false;
            }
            const ui_event& ui_evt = *ui_evt_opt;

            // ============================================================
            // Cache Service References (Performance Optimization)
            // ============================================================
            // Cache service pointers once at function entry to avoid repeated
            // service locator lookups throughout event handling
            auto* layers = ui_services<Backend>::layers();
            auto* input = ui_services<Backend>::input();
            auto* hotkeys = ui_services<Backend>::hotkeys();

            // ============================================================
            // Layer Event Routing - FIRST PRIORITY!
            // ============================================================
            // Route event through layers first (top to bottom).
            // If a layer handles the event or a modal blocks it, don't route to root widget.
            // ARCHITECTURAL FIX: Pass ui_event instead of native_event to prevent double conversion
            if (layers) {
                if (layers->route_event(ui_evt)) {
                    return true;  // Layer handled the event
                }
            }

            // ============================================================
            // Window Event Handling (Resize) - CHECK FIRST!
            // ============================================================
            // Use variant dispatch to handle resize events
            if (auto* resize_evt = std::get_if<resize_event>(&ui_evt)) {
                // Notify renderer to resize its buffers
                m_renderer.on_resize();

                // Get new window dimensions from typed event (physical pixels)
                const int new_width = resize_evt->width;
                const int new_height = resize_evt->height;

                // Convert physical dimensions to logical using metrics (DPI-aware)
                const auto* metrics = ui_services<Backend>::metrics();
                if (!metrics) {
                    // Fallback: assume 1:1 scaling if metrics unavailable
                    [[maybe_unused]] auto measured_size = m_root->measure(
                        logical_unit(static_cast<double>(new_width)),
                        logical_unit(static_cast<double>(new_height)));
                    m_root->arrange(logical_rect{0.0_lu, 0.0_lu,
                        logical_unit(static_cast<double>(new_width)),
                        logical_unit(static_cast<double>(new_height))});
                } else {
                    // Proper DPI-aware conversion
                    const logical_unit logical_width = metrics->physical_to_logical_x(new_width);
                    const logical_unit logical_height = metrics->physical_to_logical_y(new_height);

                    [[maybe_unused]] auto measured_size = m_root->measure(logical_width, logical_height);
                    m_root->arrange(logical_rect{0.0_lu, 0.0_lu, logical_width, logical_height});
                }

                return true;  // Resize handled
            }

            // ============================================================
            // Keyboard Event Handling
            // ============================================================
            // Use variant dispatch to handle keyboard events
            if (auto* kbd_evt = std::get_if<keyboard_event>(&ui_evt)) {
                // 1. Handle Tab navigation (input_manager traverses tree automatically)
                if (input) {
                    if (input->handle_tab_navigation_in_tree(*kbd_evt, m_root.get())) {
                        return true;  // Tab was handled, focus changed
                    }
                }

                // 2. Get focused widget for hotkey dispatch
                widget_type* focused = nullptr;
                if (input) {
                    auto* focused_target = input->get_focused();
                    focused = static_cast<widget_type*>(focused_target);
                }

                // 3. Try global hotkeys from service locator
                // Use ui_event API for hotkey matching
                // IMPORTANT: Pass focused element so semantic actions can be dispatched to it
                if (hotkeys) {
                    if (hotkeys->handle_ui_event(ui_evt, focused)) {
                        return true;  // Global hotkey handled
                    }
                }

                // 3.5. Handle activate_focused semantic action for widgets that accept keys as clicks
                if (focused && focused->accepts_keys_as_click()) {
                    if (hotkeys) {
                        if (hotkeys->matches_action(*kbd_evt, hotkey_action::activate_focused)) {
                            // With unified event API, widgets that accept keys as clicks
                            // handle Enter/Space through handle_keyboard() which emits clicked signal.
                            // The keyboard event has already been routed to the focused widget above,
                            // so no additional action needed here.
                            return true;  // Activation handled by widget's keyboard handler
                        }
                    }
                }

                // 3.6. Try semantic actions (scrolling, navigation) on focused widget
                if (focused) {
                    if (hotkeys) {
                        // Check for scroll actions
                        if (hotkeys->matches_action(*kbd_evt, hotkey_action::scroll_up)) {
                            return focused->handle_semantic_action(hotkey_action::scroll_up);
                        }
                        if (hotkeys->matches_action(*kbd_evt, hotkey_action::scroll_down)) {
                            return focused->handle_semantic_action(hotkey_action::scroll_down);
                        }
                        if (hotkeys->matches_action(*kbd_evt, hotkey_action::scroll_page_up)) {
                            return focused->handle_semantic_action(hotkey_action::scroll_page_up);
                        }
                        if (hotkeys->matches_action(*kbd_evt, hotkey_action::scroll_page_down)) {
                            return focused->handle_semantic_action(hotkey_action::scroll_page_down);
                        }
                        if (hotkeys->matches_action(*kbd_evt, hotkey_action::scroll_home)) {
                            return focused->handle_semantic_action(hotkey_action::scroll_home);
                        }
                        if (hotkeys->matches_action(*kbd_evt, hotkey_action::scroll_end)) {
                            return focused->handle_semantic_action(hotkey_action::scroll_end);
                        }
                    }
                }

                // 4. Forward keyboard event to focused widget
                // Route event to target phase for focused widget handling
                if (focused) {
                    return focused->handle_event(ui_evt, event_phase::target);
                }
            }

            // ============================================================
            // Mouse Event Handling (Unified with Input Manager)
            // ============================================================
            // Use variant dispatch to handle mouse events
            if (auto* mouse_evt = std::get_if<mouse_event>(&ui_evt)) {
                if (!input) return false;  // No input manager available

                // Mouse coordinates are now in logical units (normalized at backend level)
                // This preserves fractional precision for accurate hit testing

                // Check if this is a button press/release event
                bool is_button_event = (mouse_evt->act == mouse_event::action::press ||
                                       mouse_evt->act == mouse_event::action::release);
                bool is_press = (mouse_evt->act == mouse_event::action::press);

                // Close popups (menus) when scrolling - standard UI behavior
                bool is_wheel = (mouse_evt->act == mouse_event::action::wheel_up ||
                                mouse_evt->act == mouse_event::action::wheel_down);
                if (is_wheel) {
                    if (auto* layers = ui_services<Backend>::layers()) {
                        layers->clear_layers(layer_type::popup);
                    }
                }

                // Determine target widget:
                // - If mouse is captured, route to captured widget (direct, no path)
                // - Otherwise, hit test with path recording for three-phase routing
                auto* captured_target = input->get_captured();
                auto* captured = static_cast<widget_type*>(captured_target);

                widget_type* target_widget = nullptr;
                hit_test_path<Backend> hit_path;

                if (captured) {
                    // Captured widget gets event directly (no three-phase routing)
                    target_widget = captured;
                } else {
                    // Hit test with logical coordinates for precision
                    target_widget = m_root->hit_test_logical(mouse_evt->x, mouse_evt->y, hit_path);
                }

                // Handle hover changes (only when NOT captured)
                if (!captured) {
                    input->set_hover(target_widget);
                }

                // Handle mouse capture
                if (is_button_event) {
                    if (is_press) {
                        // WORKAROUND (PERMANENT): Some backends (like termbox2) don't send mouse release events
                        // due to terminal limitations. This is expected behavior, not a bug.
                        // If we're pressing on a different widget than what's captured,
                        // implicitly release the old capture first by sending it a synthetic mouse release event.
                        // This ensures widgets can clean up their pressed state properly.
                        auto* currently_captured = input->get_captured();
                        if (currently_captured && currently_captured != target_widget) {
                            // Send mouse release to previously captured widget to clean up its state
                            // With unified event API, construct proper mouse_event with action::release
                            // Note: mouse_event uses physical coordinates from original event
                            mouse_event release{
                                .x = mouse_evt->x,
                                .y = mouse_evt->y,
                                .btn = mouse_event::button::left,
                                .act = mouse_event::action::release,
                                .modifiers = {}
                            };
                            static_cast<widget_type*>(currently_captured)->handle_event(
                                ui_event{release}, event_phase::target
                            );
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

                        // After releasing capture, update hover state using existing hit-test result
                        // (Performance: reuse hit_path from above instead of doing a second hit-test)
                        input->set_hover(target_widget);
                    }
                }

                // Route event to target widget
                if (target_widget) {
                    if (captured) {
                        // Captured widget: direct routing to TARGET phase only
                        return target_widget->handle_event(ui_evt, event_phase::target);
                    } else {
                        // Non-captured: three-phase routing (NEW!)
                        // This enables text_view to intercept clicks in capture phase
                        return route_event(ui_evt, hit_path);
                    }
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

    private:
        /**
         * @brief Internal implementation for display methods
         * @param bounds The bounds to render within
         *
         * @details
         * Common implementation shared by both display() and display(bounds).
         * Performs the complete layout and rendering pipeline.
         */
        void display_impl(const rect_type& bounds) {
            // Get metrics for coordinate conversion (required)
            const auto* metrics = ui_services<Backend>::metrics();
            if (!metrics) {
                throw std::runtime_error("Backend metrics not available! Ensure ui_context is created before rendering.");
            }

            // Get dirty regions from previous frame
            auto dirty_regions = m_root->get_and_clear_dirty_regions();

            // Add dirty regions from removed layers (fixes menu switching artifacts)
            // Layer manager stores logical coordinates; convert to physical using metrics (DPI-aware)
            if (auto* layers = ui_services<Backend>::layers()) {
                const auto& removed_regions = layers->get_removed_layer_dirty_regions();
                for (const auto& logical_region : removed_regions) {
                    // Use snap_rect for proper DPI-aware conversion
                    rect_type physical_region = metrics->snap_rect(logical_region);
                    dirty_regions.push_back(physical_region);
                }
                layers->clear_removed_layer_dirty_regions();
            }

            // Render background BEFORE widgets (from ui_services)
            if (auto* bg = ui_services<Backend>::background()) {
                bg->render(m_renderer, bounds, dirty_regions);
            }

            // Convert physical viewport to logical coordinates
            const logical_size logical_viewport = metrics->physical_to_logical_size(
                typename Backend::size_type{
                    rect_utils::get_width(bounds),
                    rect_utils::get_height(bounds)
                }
            );


            // Two-pass layout in logical coordinates
            [[maybe_unused]] auto measured_size = m_root->measure(
                logical_viewport.width,
                logical_viewport.height);

            m_root->arrange(logical_rect{
                logical_unit(0.0),
                logical_unit(0.0),
                logical_viewport.width,
                logical_viewport.height});

            // Get global theme ONCE at rendering entry point (REQUIRED - theme cannot be null)
            auto* themes = ui_services<Backend>::themes();
            if (!themes) {
                throw std::runtime_error("Theme service not initialized! Ensure ui_context is created before rendering.");
            }
            auto* theme_ptr = themes->get_current_theme();
            if (!theme_ptr) {
                throw std::runtime_error("No current theme set! Ensure themes are registered before rendering.");
            }

            // Render root widget to back buffer with dirty rectangle optimization
            // Only renders widgets that intersect with dirty regions
            // Theme is passed down through the widget tree by pointer
            // Metrics are used for logical → physical coordinate conversion
            m_root->render(m_renderer, theme_ptr, *metrics);

            // Render all overlay layers from current context (pass theme for popup menus, dialogs)
            if (auto* layers = ui_services<Backend>::layers()) {
                layers->render_all_layers(m_renderer, bounds, theme_ptr, *metrics);
            }
        }
    };

} // namespace onyxui