/**
 * @file layer_manager.hh
 * @brief Layer management system for UI overlays, popups, and z-ordering
 * @author igor
 * @date 17/10/2025
 *
 * @details
 * The layer manager provides multi-layer rendering and event routing for UI overlays.
 * It enables popups, dropdowns, modal dialogs, tooltips, and other UI elements that
 * need to appear on top of the base UI with proper z-ordering and event handling.
 *
 * ## Key Features
 *
 * - **Z-Ordered Layers**: Multiple layers with automatic sorting by z-index
 * - **Event Routing**: Events route to top layers first, with modal blocking
 * - **Popup Positioning**: Smart positioning relative to anchor elements
 * - **Modal Dialogs**: Block underlying UI with semi-transparent overlay
 * - **Tooltips**: Non-blocking overlays with auto-hide
 * - **Animations**: Fade in/out for smooth transitions (future)
 *
 * ## Architecture
 *
 * ```
 * layer_manager
 *   ├── Layer Stack (sorted by z-index)
 *   │   ├── Debug Layer (z=1000)
 *   │   ├── Notification Layer (z=500)
 *   │   ├── Modal Dialog Layer (z=400)
 *   │   ├── Dialog Layer (z=300)
 *   │   ├── Popup Layer (z=200)
 *   │   └── Tooltip Layer (z=100)
 *   │
 *   ├── Event Routing (top to bottom)
 *   └── Rendering (bottom to top)
 * ```
 *
 * ## Usage Example
 *
 * ```cpp
 * // Create and keep ownership of menu
 * auto menu_content = std::make_unique<menu<Backend>>();
 * // ... populate menu
 *
 * // Show dropdown menu (layer_manager doesn't take ownership)
 * layer_id menu_id = layer_mgr.show_popup(
 *     menu_content.get(),
 *     anchor_button_bounds,
 *     popup_placement::below
 * );
 *
 * // Show modal dialog (caller keeps widget alive)
 * auto dialog_content = std::make_unique<panel<Backend>>();
 * layer_id dialog_id = layer_mgr.show_modal_dialog(
 *     dialog_content.get(),
 *     dialog_position::center
 * );
 *
 * // Hide layer (widget still exists)
 * layer_mgr.remove_layer(menu_id);
 *
 * // Widget can be shown again later
 * menu_id = layer_mgr.show_popup(menu_content.get(), ...);
 * ```
 */

#pragma once

#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <algorithm>
#include <iostream>
#include <onyxui/concepts/backend.hh>
#include <onyxui/concepts/event_like.hh>  // For event_traits
#include <onyxui/events/ui_event.hh>      // For ui_event, mouse_event, keyboard_event
#include <onyxui/events/event_phase.hh>   // For event_phase
#include <onyxui/events/hit_test_path.hh> // For hit_test_path
#include <onyxui/events/event_router.hh>  // For route_event()
#include <onyxui/theming/theme.hh>

namespace onyxui {

    // ======================================================================
    // Layer ID Type
    // ======================================================================

    /**
     * @struct layer_id
     * @brief Strongly-typed identifier for layers
     *
     * @details
     * Provides type-safe layer identification. Invalid ID (value=0) represents
     * "no layer" and is used as sentinel value.
     */
    struct layer_id {
        uint32_t value;

        constexpr layer_id() noexcept : value(0) {}
        explicit constexpr layer_id(uint32_t v) noexcept : value(v) {}

        bool operator==(const layer_id& other) const noexcept = default;
        bool operator!=(const layer_id& other) const noexcept = default;

        [[nodiscard]] bool is_valid() const noexcept { return value != 0; }

        static constexpr layer_id invalid() noexcept { return layer_id(0); }
    };

    // ======================================================================
    // Forward Declarations
    // ======================================================================

    template<UIBackend Backend>
    class scoped_layer;

    // ======================================================================
    // Layer Types and Configuration
    // ======================================================================

    /**
     * @enum layer_type
     * @brief Type of UI layer with associated z-index
     */
    enum class layer_type : uint8_t {
        base,         ///< Main UI (z-index: 0)
        tooltip,      ///< Tooltips (z-index: 100)
        popup,        ///< Dropdowns, context menus (z-index: 200)
        dialog,       ///< Non-modal dialogs (z-index: 300)
        modal,        ///< Modal dialogs (z-index: 400)
        notification, ///< Toast/snackbar (z-index: 500)
        debug         ///< Debug overlay (z-index: 1000)
    };

    /**
     * @enum popup_placement
     * @brief Popup positioning relative to anchor element
     */
    enum class popup_placement {
        below,        ///< Below anchor, left-aligned
        above,        ///< Above anchor, left-aligned
        left,         ///< Left of anchor, top-aligned
        right,        ///< Right of anchor, top-aligned
        below_right,  ///< Below anchor, right-aligned
        above_right,  ///< Above anchor, right-aligned
        auto_best     ///< Choose best position to fit viewport
    };

    /**
     * @enum dialog_position
     * @brief Dialog positioning within viewport
     */
    enum class dialog_position {
        center,       ///< Center of viewport
        top,          ///< Top center
        bottom,       ///< Bottom center
        custom        ///< Use provided position
    };

    /**
     * @brief Get default z-index for layer type
     */
    constexpr int get_default_z_index(layer_type type) noexcept {
        switch (type) {
            case layer_type::base:         return 0;
            case layer_type::tooltip:      return 100;
            case layer_type::popup:        return 200;
            case layer_type::dialog:       return 300;
            case layer_type::modal:        return 400;
            case layer_type::notification: return 500;
            case layer_type::debug:        return 1000;
        }
        return 0;
    }

    // ======================================================================
    // Layer Manager
    // ======================================================================

    /**
     * @class layer_manager
     * @brief Manages UI layers with z-ordering and event routing
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * The layer manager coordinates multiple UI layers, handling rendering order,
     * event routing, and modal blocking. It provides high-level APIs for showing
     * popups, dialogs, and tooltips.
     *
     * ## Responsibilities
     *
     * - **Layer Stack Management**: Maintains sorted list of layers by z-index
     * - **Event Routing**: Routes events to top layers first, respects modal blocking
     * - **Rendering Coordination**: Renders layers in correct order (bottom to top)
     * - **Popup Positioning**: Calculates optimal popup positions with viewport constraints
     * - **Modal Blocking**: Prevents interaction with underlying UI when modal active
     *
     * ## Thread Safety
     *
     * Not thread-safe. All operations must occur on UI thread.
     */
    template<UIBackend Backend>
    class layer_manager {
    public:
        using element_type = ui_element<Backend>;
        using rect_type = typename Backend::rect_type;
        using event_type = typename Backend::event_type;
        using renderer_type = typename Backend::renderer_type;
        using theme_type = ui_theme<Backend>;

        // Configuration
        struct config {
            bool enable_animations = false;  // TODO: Animation support
            bool debug_show_layers = false;
        };

        /**
         * @brief Construct layer manager with configuration
         */
        explicit layer_manager(config cfg = {})
            : m_config(cfg)
            , m_next_id(1) {
        }

        /**
         * @brief Destructor
         */
        ~layer_manager() = default;

        // Disable copy/move
        layer_manager(const layer_manager&) = delete;
        layer_manager& operator=(const layer_manager&) = delete;
        layer_manager(layer_manager&&) noexcept = delete;
        layer_manager& operator=(layer_manager&&) noexcept = delete;

        // ============================================================
        // Layer Management
        // ============================================================

        /**
         * @brief Add a new layer with lifetime tracking (RECOMMENDED)
         *
         * @param type Layer type (determines default z-index)
         * @param root Root element for this layer (shared_ptr for lifetime tracking)
         * @param custom_z_index Custom z-index (-1 = use default for type)
         * @return Layer ID for later reference
         *
         * @details
         * **Phase 1.2**: This is the SAFE API that tracks element lifetime.
         * The layer_manager tracks the element via weak_ptr and automatically
         * handles cleanup if the element is destroyed.
         *
         * **Usage**:
         * @code
         * auto menu = std::make_shared<menu<Backend>>();
         * layer_id id = mgr.add_layer(layer_type::popup, menu);
         * // Safe: Layer manager tracks lifetime via weak_ptr
         * @endcode
         */
        layer_id add_layer(layer_type type,
                          std::shared_ptr<element_type> root,
                          int custom_z_index = -1) {
            layer_id id(m_next_id++);

            int const z_index = (custom_z_index >= 0) ? custom_z_index : get_default_z_index(type);

            m_layers.push_back(layer_data{
                id,
                type,
                z_index,
                std::weak_ptr<element_type>(root),  // Store as weak_ptr for tracking
                nullptr,  // owner: nullptr for add_layer (caller owns the element)
                true,  // visible
                type == layer_type::modal,  // modal
                type == layer_type::modal,  // blocks_events
                {},  // bounds
                {},  // anchor_bounds
                popup_placement::below,  // placement
                dialog_position::center,  // dialog_pos
                false,  // needs_positioning
                nullptr  // outside_click_callback (Phase 1.3)
            });

            sort_layers_by_z_index();
            m_layers_changed = true;  // Signal that layers were modified

            return id;
        }

        /**
         * @brief Create a layer with RAII automatic cleanup (Phase 1.4)
         *
         * @param type Layer type (determines default z-index)
         * @param root Root element for this layer (shared_ptr for lifetime safety)
         * @param custom_z_index Custom z-index (-1 = use default for type)
         * @return RAII handle that auto-removes layer on destruction
         *
         * @details
         * Returns a scoped_layer handle that automatically removes the layer
         * when it goes out of scope. This provides exception-safe layer management.
         *
         * **Usage**:
         * @code
         * auto menu = std::make_shared<menu<Backend>>();
         * {
         *     auto layer = mgr.add_scoped_layer(layer_type::popup, menu);
         *     // Use layer...
         * }  // Layer automatically removed here
         * @endcode
         *
         * **Note**: Defined in scoped_layer.hh (after scoped_layer class definition)
         */
        scoped_layer<Backend> add_scoped_layer(layer_type type,
                                               std::shared_ptr<element_type> root,
                                               int custom_z_index = -1);

        /**
         * @brief Remove a layer
         *
         * @param id Layer to remove
         *
         * @details
         * Tracks the removed layer's bounds as a dirty region so the area
         * gets redrawn on the next frame (fixes menu switching artifacts).
         */
        void remove_layer(layer_id id) {
            auto it = find_layer(id);
            if (it != m_layers.end()) {
                // Track removed layer bounds for dirty region marking
                // Try to get bounds from the widget itself (more accurate if manually positioned)
                rect_type bounds_to_track = it->bounds;
                it->with_root([&](element_type* root) {
                    bounds_to_track = root->bounds();
                });
                m_removed_layer_dirty_regions.push_back(bounds_to_track);
                m_layers.erase(it);
                m_layers_changed = true;  // Signal that layers were modified
            }
        }

        /**
         * @brief Remove all layers of specific type
         *
         * @param type Type of layers to remove
         */
        void clear_layers(layer_type type) {
            auto old_size = m_layers.size();
            m_layers.erase(
                std::remove_if(m_layers.begin(), m_layers.end(),
                    [type](const layer_data& layer) {
                        return layer.type == type;
                    }),
                m_layers.end()
            );
            if (m_layers.size() != old_size) {
                m_layers_changed = true;  // Signal that layers were modified
            }
        }

        /**
         * @brief Bring layer to front (highest z-order within its type)
         *
         * @param id Layer to bring to front
         *
         * @details
         * Moves the specified layer to render last among layers of the same type,
         * making it appear on top. The z-index remains unchanged, only the render
         * order within the same z-index is adjusted.
         *
         * Does nothing if layer ID is invalid or layer not found.
         */
        void bring_to_front(layer_id id) {
            auto it = find_layer(id);
            if (it == m_layers.end()) return;  // Layer not found

            // Already at end (front)?
            if (it == m_layers.end() - 1) return;

            // Move to end (renders last = on top)
            layer_data layer = std::move(*it);
            m_layers.erase(it);
            m_layers.push_back(std::move(layer));

            m_layers_changed = true;  // Signal that layers were modified
        }

        /**
         * @brief Remove all layers except base UI
         */
        void clear_all_layers() {
            m_layers.clear();
        }

        // ============================================================
        // Popup Helpers
        // ============================================================

        /**
         * @brief Show popup positioned relative to anchor
         *
         * @param content Popup content (non-owning pointer)
         * @param anchor_bounds Bounds of anchor element
         * @param placement Desired placement
         * @return Layer ID
         *
         * @details
         * Creates a non-owning reference to the content. Caller MUST keep
         * content alive while layer exists and remove layer before destroying content.
         */
        layer_id show_popup(element_type* content,
                           const rect_type& anchor_bounds,
                           popup_placement placement = popup_placement::below,
                           std::function<void()> outside_click_callback = nullptr) {
            // Create non-owning shared_ptr (caller owns the memory)
            auto non_owning = std::shared_ptr<element_type>(content, [](element_type*) {});
            layer_id id = add_layer(layer_type::popup, non_owning);

            auto it = find_layer(id);
            if (it != m_layers.end()) {
                it->owner = non_owning;  // Store to keep weak_ptr valid
                it->anchor_bounds = anchor_bounds;
                it->placement = placement;
                it->needs_positioning = true;
                it->outside_click_callback = std::move(outside_click_callback);  // Phase 1.3
            }

            return id;
        }

        /**
         * @brief Show tooltip at specific position
         *
         * @param content Tooltip content (non-owning pointer)
         * @param x X position
         * @param y Y position
         * @return Layer ID
         *
         * @details
         * Creates a non-owning reference to the content. Caller MUST keep
         * content alive while layer exists and remove layer before destroying content.
         */
        layer_id show_tooltip(element_type* content,
                             int x, int y) {
            // Create non-owning shared_ptr (caller owns the memory)
            auto non_owning = std::shared_ptr<element_type>(content, [](element_type*) {});
            layer_id id = add_layer(layer_type::tooltip, non_owning);

            auto it = find_layer(id);
            if (it != m_layers.end()) {
                it->owner = non_owning;  // Store to keep weak_ptr valid
                rect_type pos;
                rect_utils::set_bounds(pos, x, y, 0, 0);
                it->anchor_bounds = pos;
                it->placement = popup_placement::below;
                it->needs_positioning = true;
            }

            return id;
        }

        /**
         * @brief Show modal dialog
         *
         * @param content Dialog content (non-owning pointer)
         * @param pos Dialog position
         * @return Layer ID
         *
         * @details
         * Creates a non-owning reference to the content. Caller MUST keep
         * content alive while layer exists and remove layer before destroying content.
         */
        layer_id show_modal_dialog(element_type* content,
                                   dialog_position pos = dialog_position::center) {
            // Create non-owning shared_ptr (caller owns the memory)
            auto non_owning = std::shared_ptr<element_type>(content, [](element_type*) {});
            layer_id id = add_layer(layer_type::modal, non_owning);

            auto it = find_layer(id);
            if (it != m_layers.end()) {
                it->owner = non_owning;  // Store to keep weak_ptr valid
                it->dialog_pos = pos;
                it->needs_positioning = true;
            }

            return id;
        }

        // ============================================================
        // Layer Visibility
        // ============================================================

        /**
         * @brief Show layer (make visible)
         */
        void show_layer(layer_id id) {
            if (auto it = find_layer(id); it != m_layers.end()) {
                it->visible = true;
            }
        }

        /**
         * @brief Hide layer (make invisible but don't remove)
         */
        void hide_layer(layer_id id) {
            if (auto it = find_layer(id); it != m_layers.end()) {
                it->visible = false;
            }
        }

        /**
         * @brief Check if layer is visible
         */
        [[nodiscard]] bool is_layer_visible(layer_id id) const {
            if (auto it = find_layer_const(id); it != m_layers.end()) {
                // Phase 1.2: Return false for expired layers
                return it->is_valid() && it->visible;
            }
            return false;
        }

        /**
         * @brief Request automatic positioning for a layer
         * @details Marks layer as needing positioning so it will be auto-positioned
         *          on next render using position_layer()
         */
        void request_layer_positioning(layer_id id) {
            if (auto it = find_layer(id); it != m_layers.end()) {
                it->needs_positioning = true;
            }
        }

        /**
         * @brief Set layer bounds manually (for windows with explicit position/size)
         */
        void set_layer_bounds(layer_id id, const rect_type& bounds) {
            if (auto it = find_layer(id); it != m_layers.end()) {
                it->bounds = bounds;
                it->needs_positioning = false;  // Manual bounds, no auto-positioning
            }
        }

        // ============================================================
        // Event Routing
        // ============================================================

        /**
         * @brief Route event through layer stack
         *
         * @param event Event to route
         * @return true if event was handled by a layer
         *
         * @details
         * Routes events from top layer to bottom. If a modal is active,
         * only routes to modal and layers above it.
         */
        bool route_event(const event_type& event) {
            // Phase 1.2: Clean up expired layers first
            cleanup_expired_layers();

            // Convert native event to ui_event using unified event API
            std::optional<ui_event> ui_evt = Backend::create_event(event);
            if (!ui_evt) {
                return false;  // Unknown/unsupported event type
            }

            // Check if modal is active
            std::optional<int> modal_z_index;
            for (const auto& layer : m_layers) {
                if (layer.modal && layer.visible && layer.is_valid()) {
                    modal_z_index = layer.z_index;
                    break;
                }
            }

            // Phase 1.3: Generic click-outside detection
            // We need to defer the callback to avoid modifying m_layers while iterating
            std::function<void()> const deferred_callback;

            // Check if this is a mouse click event
            if constexpr (requires { event_traits<event_type>::is_button_press(event); }) {
                if (event_traits<event_type>::is_button_press(event)) {
                    // Check if we have any popup layers
                    for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
                        if (it->type == layer_type::popup && it->visible && it->is_valid()) {
                            // Get click position
                            int click_x = event_traits<event_type>::mouse_x(event);
                            int click_y = event_traits<event_type>::mouse_y(event);

                            // Check if click is outside popup bounds
                            if (!rect_utils::contains(it->bounds, click_x, click_y)) {
                                // Click outside popup - trigger callback but DON'T consume event
                                // This allows:
                                // 1. Menu switching: Click Theme while File is open → File closes, Theme opens
                                // 2. Normal clicks: Click anywhere → menu closes, click continues to target
                                if (it->outside_click_callback) {
                                    it->outside_click_callback();
                                    // Don't set deferred_callback - event continues to underlying widget
                                }
                            }
                            // Only check the topmost popup (whether inside or outside)
                            break;
                        }
                    }

                    // Now invoke the deferred callback (after iteration is complete)
                    if (deferred_callback) {
                        deferred_callback();
                        return true;  // We handled the click by triggering callback
                    }
                }
            }

            // Route to layers (highest z first)
            for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
                if (!it->visible || !it->is_valid()) {
                    continue;
                }

                // If modal active, skip layers below it
                if (modal_z_index.has_value() && it->z_index < *modal_z_index) {
                    continue;
                }

                // Route event to layer using unified event API
                bool handled = false;
                it->with_root([&](element_type* root_ptr) {
                    // For mouse events, use hit-testing with three-phase routing
                    if (auto* mouse = std::get_if<mouse_event>(&ui_evt.value())) {
                        int const x = mouse->x;
                        int const y = mouse->y;

                        // Build hit test path from root to target
                        hit_test_path<Backend> path;
                        element_type* target = root_ptr->hit_test(x, y, path);

                        if (target) {
                            if (!path.empty()) {
                                // Route event through three phases (capture/target/bubble)
                                handled = ::onyxui::route_event(ui_evt.value(), path);
                            } else {
                                // Path empty (simple widget tree) - deliver directly to target
                                handled = target->handle_event(ui_evt.value(), event_phase::target);
                            }
                        }
                    } else {
                        // Non-mouse events (keyboard, resize) - deliver to root directly
                        handled = root_ptr->handle_event(ui_evt.value(), event_phase::target);
                    }
                });

                if (handled) {
                    return true;  // Event handled
                }

                // Modal blocks events even if not handled
                if (it->modal && it->blocks_events) {
                    return true;  // Block event from going lower
                }
            }

            return false;  // Event not handled by any layer
        }

        // ============================================================
        // Rendering
        // ============================================================

        /**
         * @brief Render all layers
         *
         * @param renderer Renderer to use
         * @param viewport Viewport bounds
         * @param theme Theme pointer for styling (required, pass nullptr if no theme)
         *
         * @details
         * Renders all visible layers from lowest z to highest z.
         * Performs layout (measure/arrange) before rendering.
         * **Phase 1.2**: Automatically skips expired layers.
         * Theme must be explicitly passed to ensure popup menus/dialogs use correct styling.
         */
        void render_all_layers(renderer_type& renderer, const rect_type& viewport, const theme_type* theme) {
            // Phase 1.2: Clean up expired layers first
            cleanup_expired_layers();

            std::cerr << "[DEBUG] render_all_layers: total layers=" << m_layers.size() << "\n";
            for (auto& layer : m_layers) {
                std::cerr << "[DEBUG] Layer id=" << layer.id.value << ", visible=" << layer.visible
                          << ", is_valid=" << layer.is_valid() << "\n";
                if (!layer.visible || !layer.is_valid()) {
                    std::cerr << "[DEBUG] Skipping layer id=" << layer.id.value << "\n";
                    continue;
                }

                // Position layer if needed
                if (layer.needs_positioning) {
                    position_layer(layer, viewport);
                    layer.needs_positioning = false;
                }

                // Measure, arrange, and render - Phase 1.2: Use safe access
                layer.with_root([&](element_type* root_ptr) {
                    std::cerr << "[DEBUG] Rendering layer id=" << layer.id.value << ", bounds=("
                              << layer.bounds.x << "," << layer.bounds.y << ","
                              << layer.bounds.w << "," << layer.bounds.h << ")\n";
                    // Measure and arrange
                    int const width = rect_utils::get_width(layer.bounds);
                    int const height = rect_utils::get_height(layer.bounds);
                    [[maybe_unused]] auto measured_size = root_ptr->measure(width, height);

                    // Arrange with absolute coordinates (layer.bounds contains screen position)
                    root_ptr->arrange(layer.bounds);

                    // Render with theme (for proper styling in menus, dialogs, etc.)
                    root_ptr->render(renderer, theme);
                    std::cerr << "[DEBUG] Finished rendering layer id=" << layer.id.value << "\n";
                });
            }
        }

        // ============================================================
        // Queries
        // ============================================================

        /**
         * @brief Check if any modal layer is active
         *
         * @details
         * **Phase 1.2**: Only counts valid (non-expired) modal layers.
         */
        [[nodiscard]] bool has_modal_layer() const {
            return std::any_of(m_layers.begin(), m_layers.end(),
                [](const layer_data& layer) {
                    return layer.modal && layer.visible && layer.is_valid();
                });
        }

        /**
         * @brief Get number of layers
         */
        [[nodiscard]] size_t layer_count() const noexcept {
            return m_layers.size();
        }

        /**
         * @brief Test helper: Get topmost visible layer ID
         * @return Layer ID of topmost layer, or invalid if no layers
         */
        [[nodiscard]] layer_id get_topmost_layer() const {
            for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
                if (it->visible && it->is_valid()) {
                    return it->id;
                }
            }
            return layer_id::invalid();
        }

        /**
         * @brief Test helper: Trigger outside-click callback for a layer
         * @param id Layer to trigger callback for
         * @return true if callback was triggered, false if no callback or invalid ID
         *
         * @details
         * This is a test utility to verify outside-click behavior without
         * needing to simulate actual mouse clicks with coordinates.
         */
        bool trigger_outside_click(layer_id id) {
            auto it = find_layer(id);
            if (it != m_layers.end() && it->outside_click_callback) {
                it->outside_click_callback();
                return true;
            }
            return false;
        }

        /**
         * @brief Get dirty regions from removed layers
         * @return Vector of rectangles that were occupied by removed layers
         *
         * @details
         * Returns the bounds of all layers removed since the last call to
         * clear_removed_layer_dirty_regions(). These regions should be marked
         * as dirty to ensure proper redrawing.
         *
         * **Use case:** Menu switching - when menu A is replaced by menu B,
         * menu A's area needs to be redrawn to clear artifacts.
         */
        [[nodiscard]] const std::vector<rect_type>& get_removed_layer_dirty_regions() const noexcept {
            return m_removed_layer_dirty_regions;
        }

        /**
         * @brief Clear the list of removed layer dirty regions
         *
         * @details
         * Should be called after the dirty regions have been processed/marked.
         * Typically called by ui_handle::display() after incorporating the
         * regions into the dirty region tracking system.
         */
        void clear_removed_layer_dirty_regions() noexcept {
            m_removed_layer_dirty_regions.clear();
        }

        /**
         * @brief Check if layers were added or removed since last check
         * @return true if layers changed, false otherwise
         *
         * @details
         * **Use case with blocking event loops (like conio_poll_event):**
         *
         * When using a blocking event loop, layer changes during event handling
         * won't be visible until the next frame. To fix this, check this flag
         * after handling events and call display() immediately if true:
         *
         * @code
         * while (!quit) {
         *     ui.display();
         *     ui.present();
         *
         *     tb_event ev;
         *     conio_poll_event(&ev);  // Blocks until event
         *     ui.handle_event(ev);     // May open/close layers
         *
         *     // Fix: Redraw immediately if layers changed during event handling
         *     auto* layers = ui_services<Backend>::layers();
         *     if (layers && layers->layers_changed()) {
         *         layers->clear_layers_changed_flag();
         *         ui.display();  // Immediate redraw
         *         ui.present();
         *     }
         * }
         * @endcode
         */
        [[nodiscard]] bool layers_changed() const noexcept {
            return m_layers_changed;
        }

        /**
         * @brief Clear the layers changed flag
         *
         * @details
         * Should be called after checking layers_changed() and handling the change.
         */
        void clear_layers_changed_flag() noexcept {
            m_layers_changed = false;
        }

    private:
        // Layer data structure
        struct layer_data {
            layer_id id;
            layer_type type = layer_type::base;
            int z_index = 0;
            std::weak_ptr<element_type> root;  // Safe lifetime tracking (Phase 1.2)
            std::shared_ptr<element_type> owner;  // Keeps root alive (may be non-owning for show_popup/etc)
            bool visible = true;
            bool modal = false;
            bool blocks_events = true;

            // Positioning info
            rect_type bounds;
            rect_type anchor_bounds;
            popup_placement placement = popup_placement::below;
            dialog_position dialog_pos = dialog_position::center;
            bool needs_positioning = false;

            // Click-outside callback (Phase 1.3)
            std::function<void()> outside_click_callback;

            /**
             * @brief Check if the layer's element is still valid
             * @return true if element exists, false if destroyed
             *
             * @details
             * Checks if the weak_ptr to the element is still valid.
             * Returns false if the element has been destroyed.
             */
            [[nodiscard]] bool is_valid() const noexcept {
                return !root.expired();
            }

            /**
             * @brief Safely access the root element
             * @param fn Function to call with the locked pointer
             * @return true if element was valid and function was called
             *
             * @details
             * Locks the weak_ptr, calls the function if valid, automatically releases.
             * Thread-safe access pattern.
             *
             * @code
             * layer.with_root([](element_type* elem) {
             *     elem->process_event(event);
             * });
             * @endcode
             */
            template<typename Fn>
            bool with_root(Fn&& fn) {
                if (auto locked = root.lock()) {
                    std::forward<Fn>(fn)(locked.get());
                    return true;
                }
                return false;
            }

            /**
             * @brief Safely access the root element (const version)
             */
            template<typename Fn>
            bool with_root(Fn&& fn) const {
                if (auto locked = root.lock()) {
                    std::forward<Fn>(fn)(locked.get());
                    return true;
                }
                return false;
            }
        };

        config m_config;
        std::vector<layer_data> m_layers;
        uint32_t m_next_id;
        std::vector<rect_type> m_removed_layer_dirty_regions;  ///< Bounds of removed layers (for dirty region tracking)
        bool m_layers_changed = false;  ///< Flag indicating layers were added/removed since last check

        // Helper methods
        auto find_layer(layer_id id) {
            return std::find_if(m_layers.begin(), m_layers.end(),
                [id](const layer_data& layer) {
                    return layer.id == id;
                });
        }

        auto find_layer_const(layer_id id) const {
            return std::find_if(m_layers.begin(), m_layers.end(),
                [id](const layer_data& layer) {
                    return layer.id == id;
                });
        }

        void sort_layers_by_z_index() {
            std::sort(m_layers.begin(), m_layers.end(),
                [](const layer_data& a, const layer_data& b) {
                    return a.z_index < b.z_index;
                });
        }

        /**
         * @brief Remove layers whose elements have been destroyed
         *
         * @details
         * **Phase 1.2**: Automatic cleanup of expired layers.
         * Scans the layer list and removes any layers whose weak_ptr has expired.
         * Called automatically at strategic points (event routing, rendering).
         *
         * **Performance**: O(n) where n = number of layers. Typically very fast
         * since most layers remain valid.
         */
        void cleanup_expired_layers() {
            m_layers.erase(
                std::remove_if(m_layers.begin(), m_layers.end(),
                    [](const layer_data& layer) {
                        return !layer.is_valid();
                    }),
                m_layers.end()
            );
        }

        void position_layer(layer_data& layer, const rect_type& viewport) {
            if (layer.type == layer_type::modal) {
                position_dialog(layer, viewport);
            } else if (layer.type == layer_type::popup || layer.type == layer_type::tooltip) {
                position_popup(layer, viewport);
            } else {
                // Default: center in viewport
                position_dialog(layer, viewport);
            }
        }

        void position_dialog(layer_data& layer, const rect_type& viewport) {
            // Phase 1.2: Use safe access for positioning
            layer.with_root([&](element_type* root_ptr) {
                // Measure to get desired size
                int const vp_width = rect_utils::get_width(viewport);
                int const vp_height = rect_utils::get_height(viewport);

                auto size = root_ptr->measure(vp_width, vp_height);

                int const width = size_utils::get_width(size);
                int const height = size_utils::get_height(size);

                // Center in viewport
                int const x = rect_utils::get_x(viewport) + ((vp_width - width) / 2);
                int const y = rect_utils::get_y(viewport) + ((vp_height - height) / 2);

                rect_utils::set_bounds(layer.bounds, x, y, width, height);
            });
        }

        void position_popup(layer_data& layer, const rect_type& viewport) {
            // Phase 1.2: Use safe access for positioning
            layer.with_root([&](element_type* root_ptr) {
                // Measure to get desired size
                int const vp_width = rect_utils::get_width(viewport);
                int const vp_height = rect_utils::get_height(viewport);

                auto size = root_ptr->measure(vp_width, vp_height);

                int const width = size_utils::get_width(size);
                int const height = size_utils::get_height(size);

                // Get anchor position
                int const anchor_x = rect_utils::get_x(layer.anchor_bounds);
                int const anchor_y = rect_utils::get_y(layer.anchor_bounds);
                int const anchor_w = rect_utils::get_width(layer.anchor_bounds);
                int const anchor_h = rect_utils::get_height(layer.anchor_bounds);

                // Calculate position based on placement
                int x = anchor_x;
                int y = anchor_y;

                switch (layer.placement) {
                    case popup_placement::below:
                        y = anchor_y + anchor_h;
                        break;

                    case popup_placement::above:
                        y = anchor_y - height;
                        break;

                    case popup_placement::left:
                        x = anchor_x - width;
                        break;

                    case popup_placement::right:
                        x = anchor_x + anchor_w;
                        break;

                    case popup_placement::below_right:
                        x = anchor_x + anchor_w - width;
                        y = anchor_y + anchor_h;
                        break;

                    case popup_placement::above_right:
                        x = anchor_x + anchor_w - width;
                        y = anchor_y - height;
                        break;

                    case popup_placement::auto_best:
                        // Try below first, then above, then right, then left
                        if (anchor_y + anchor_h + height <= rect_utils::get_y(viewport) + vp_height) {
                            y = anchor_y + anchor_h;  // Below fits
                        } else if (anchor_y - height >= rect_utils::get_y(viewport)) {
                            y = anchor_y - height;  // Above fits
                        } else if (anchor_x + anchor_w + width <= rect_utils::get_x(viewport) + vp_width) {
                            x = anchor_x + anchor_w;  // Right fits
                            y = anchor_y;
                        } else {
                            x = anchor_x - width;  // Left
                            y = anchor_y;
                        }
                        break;
                }

                // Clamp to viewport
                int const vp_x = rect_utils::get_x(viewport);
                int const vp_y = rect_utils::get_y(viewport);
                x = std::max(vp_x, std::min(x, vp_x + vp_width - width));
                y = std::max(vp_y, std::min(y, vp_y + vp_height - height));

                rect_utils::set_bounds(layer.bounds, x, y, width, height);
            });
        }
    };

} // namespace onyxui
