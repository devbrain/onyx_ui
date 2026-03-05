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
 *   │   ├── Popup Layer (z=200) - Dropdowns, context menus
 *   │   ├── Window Layer (z=150) - Regular windows
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
 *
 * ## Design Decisions
 *
 * ### Click-Outside Callbacks
 * Click-outside callbacks (passed to show_popup) ARE allowed to call remove_layer()
 * or any other layer-modifying method. The implementation defers callback invocation
 * until after iteration completes, making this safe. This is the expected pattern
 * for dismissible popups:
 * ```cpp
 * layer_mgr.show_popup(popup, anchor, placement, [&]() {
 *     layer_mgr.remove_layer(popup_id);  // Safe - callback is deferred
 * });
 * ```
 *
 * ### Layer Cleanup
 * Layers should be explicitly removed via remove_layer() when no longer needed.
 * This ensures:
 * - Dirty regions are accurately tracked for repaint
 * - Outside-click callbacks can be invoked if applicable
 * - Predictable lifecycle management
 *
 * Automatic cleanup via cleanup_expired_layers() (when weak_ptr expires) is a
 * **safety net**, not the primary mechanism. It handles cases where the owning
 * widget is destroyed without explicitly removing its layer. However:
 * - Dirty region bounds may be stale if layer was repositioned
 * - No callbacks are invoked
 * - Cleanup happens lazily (during next event/render)
 *
 * **Best Practice**: Always use remove_layer() or scoped_layer RAII wrapper.
 */

#pragma once

#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <algorithm>
#include <onyxui/concepts/backend.hh>
#include <onyxui/concepts/event_like.hh>  // For event_traits
#include <onyxui/core/signal.hh>          // For scoped_connection
#include <onyxui/events/ui_event.hh>      // For ui_event, mouse_event, keyboard_event
#include <onyxui/events/event_phase.hh>   // For event_phase
#include <onyxui/events/hit_test_path.hh> // For hit_test_path
#include <onyxui/events/event_router.hh>  // For route_event()
#include <onyxui/geometry/coordinates.hh>
#include <onyxui/theming/theme.hh>
#include <onyxui/core/backend_metrics.hh>

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

    template<UIBackend Backend>
    class input_manager;

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
        window,       ///< Regular windows (z-index: 150)
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
            case layer_type::window:       return 150;
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

        /**
         * @brief Set input manager for capture handling
         * @param input Pointer to input_manager (can be nullptr)
         */
        void set_input_manager(input_manager<Backend>* input) noexcept {
            m_input_manager = input;
        }

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
         * **Safe Lifetime Tracking**: This API tracks element lifetime using weak_ptr.
         * The layer_manager automatically handles cleanup if the element is destroyed,
         * preventing dangling references and use-after-free bugs.
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
                nullptr  // outside_click_callback (optional click-outside handler)
            });

            sort_layers_by_z_index();
            m_layers_changed = true;  // Signal that layers were modified

            return id;
        }

        /**
         * @brief Create a layer with RAII automatic cleanup
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
                m_removed_layer_dirty_regions.push_back(expand_for_shadow(it->bounds));

                // CRITICAL: Clear hover state if hovered element is in this layer
                // Check if m_layer_hovered is a descendant of this layer's root
                clear_hover_if_in_layer(*it);

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

            // First pass: clear hover state and track dirty regions for layers being removed
            for (const auto& layer : m_layers) {
                if (layer.type == type) {
                    // Track bounds for dirty region
                    m_removed_layer_dirty_regions.push_back(expand_for_shadow(layer.bounds));

                    // Clear hover if it was in this layer
                    clear_hover_if_in_layer(layer);
                }
            }

            // Second pass: remove the layers
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
         * @brief Bring layer to front (highest z-order within its z-index band)
         *
         * @param id Layer to bring to front
         *
         * @details
         * Moves the specified layer to render last among layers of the SAME z-index,
         * making it appear on top within its band. This preserves the invariant that
         * modals always render above popups, which always render above windows, etc.
         *
         * The z-index remains unchanged. Only the render order within the same
         * z-index band is adjusted.
         *
         * Does nothing if layer ID is invalid or layer not found.
         *
         * **Why band-limited?** Moving a window to the absolute end would place it
         * above modals/popups, violating the z-index ordering contract. This
         * implementation respects z-index boundaries.
         */
        void bring_to_front(layer_id id) {
            auto it = find_layer(id);
            if (it == m_layers.end()) return;  // Layer not found

            const int target_z = it->z_index;

            // Find the range of layers with the same z-index
            // Layers are sorted by z-index, so same-z layers are contiguous
            auto band_begin = std::find_if(m_layers.begin(), m_layers.end(),
                [target_z](const layer_data& l) { return l.z_index == target_z; });
            auto band_end = std::find_if(band_begin, m_layers.end(),
                [target_z](const layer_data& l) { return l.z_index != target_z; });

            // Already at end of its band (front within z-index)?
            if (it == band_end - 1) return;

            // Move to end of its z-index band (renders last within band = on top)
            layer_data layer = std::move(*it);
            m_layers.erase(it);

            // Insert at the end of the z-index band
            // After erase, band_end may be invalidated, so recalculate
            auto insert_pos = std::find_if(m_layers.begin(), m_layers.end(),
                [target_z](const layer_data& l) { return l.z_index > target_z; });
            m_layers.insert(insert_pos, std::move(layer));

            m_layers_changed = true;  // Signal that layers were modified
        }

        /**
         * @brief Remove all layers except base UI
         */
        void clear_all_layers() {
            // Track all layer bounds for dirty region marking
            for (const auto& layer : m_layers) {
                m_removed_layer_dirty_regions.push_back(expand_for_shadow(layer.bounds));
            }

            // Clear hover on element before clearing pointer
            // Safe to call: destroying signal clears m_layer_hovered when element dies
            if (m_layer_hovered) {
                m_layer_hovered->reset_hover_and_press_state();
                set_layer_hover(nullptr, layer_id{});
            }

            if (!m_layers.empty()) {
                m_layers_changed = true;
            }
            m_layers.clear();
        }

        // ============================================================
        // Popup Helpers
        // ============================================================

        /**
         * @brief Show popup positioned relative to anchor
         *
         * @param content Popup content (non-owning pointer)
         * @param anchor_bounds Bounds of anchor element (in logical coordinates)
         * @param placement Desired placement (default: below)
         * @param outside_click_callback Optional callback invoked when clicking outside popup
         *        (safe to call remove_layer() from this callback - invocation is deferred)
         * @param preferred_size Optional explicit size for the popup (in logical units).
         *        If provided, position_popup() uses this instead of measuring content
         *        with viewport height, which can cause incorrect sizing for widgets
         *        with expand policies.
         * @return Layer ID
         *
         * @details
         * Creates a non-owning reference to the content. Caller MUST keep
         * content alive while layer exists and remove layer before destroying content.
         * All positioning is done in logical coordinate space.
         */
        layer_id show_popup(element_type* content,
                           const logical_rect& anchor_bounds,
                           popup_placement placement = popup_placement::below,
                           std::function<void()> outside_click_callback = nullptr,
                           std::optional<logical_size> preferred_size = std::nullopt) {
            // Create non-owning shared_ptr (caller owns the memory)
            auto non_owning = std::shared_ptr<element_type>(content, [](element_type*) {});
            layer_id id = add_layer(layer_type::popup, non_owning);

            auto it = find_layer(id);
            if (it != m_layers.end()) {
                it->owner = non_owning;  // Store to keep weak_ptr valid
                it->anchor_bounds = anchor_bounds;
                it->placement = placement;
                it->needs_positioning = true;
                it->outside_click_callback = std::move(outside_click_callback);  // Optional click-outside handler

                // Set preferred size if provided (prevents re-measuring with viewport height)
                if (preferred_size) {
                    it->bounds.width = preferred_size->width;
                    it->bounds.height = preferred_size->height;
                }

                // Connect to element's destroying signal for automatic cleanup
                // This handles the case where caller destroys element without removing layer
                it->destroying_conn = scoped_connection(content->destroying,
                    [this, id](element_type*) {
                        remove_layer(id);
                    });
            }

            return id;
        }

        /**
         * @brief Show tooltip at specific position
         *
         * @param content Tooltip content (non-owning pointer)
         * @param x X position (logical units)
         * @param y Y position (logical units)
         * @return Layer ID
         *
         * @details
         * Creates a non-owning reference to the content. Caller MUST keep
         * content alive while layer exists and remove layer before destroying content.
         * Position is in logical coordinate space.
         */
        layer_id show_tooltip(element_type* content,
                             logical_unit x, logical_unit y) {
            // Create non-owning shared_ptr (caller owns the memory)
            auto non_owning = std::shared_ptr<element_type>(content, [](element_type*) {});
            layer_id id = add_layer(layer_type::tooltip, non_owning);

            auto it = find_layer(id);
            if (it != m_layers.end()) {
                it->owner = non_owning;  // Store to keep weak_ptr valid
                it->anchor_bounds = logical_rect{x, y, 0.0_lu, 0.0_lu};
                it->placement = popup_placement::below;
                it->needs_positioning = true;

                // Connect to element's destroying signal for automatic cleanup
                it->destroying_conn = scoped_connection(content->destroying,
                    [this, id](element_type*) {
                        remove_layer(id);
                    });
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

                // Connect to element's destroying signal for automatic cleanup
                it->destroying_conn = scoped_connection(content->destroying,
                    [this, id](element_type*) {
                        remove_layer(id);
                    });
            }

            return id;
        }

        // ============================================================
        // Layer Visibility
        // ============================================================

        /**
         * @brief Show layer (make visible)
         *
         * @details
         * Marks layers_changed so the renderer knows to redraw.
         * The layer's bounds will be rendered on the next frame.
         */
        void show_layer(layer_id id) {
            if (auto it = find_layer(id); it != m_layers.end()) {
                if (!it->visible) {
                    it->visible = true;
                    m_layers_changed = true;  // Trigger redraw
                }
            }
        }

        /**
         * @brief Hide layer (make invisible but don't remove)
         *
         * @details
         * Marks the layer's bounds as dirty so the area gets repainted.
         * Without this, stale pixels would remain on screen.
         */
        void hide_layer(layer_id id) {
            if (auto it = find_layer(id); it != m_layers.end()) {
                if (it->visible) {
                    it->visible = false;
                    // Track the hidden layer's bounds for dirty region marking
                    // so the area gets repainted (clears the layer's pixels)
                    m_removed_layer_dirty_regions.push_back(expand_for_shadow(it->bounds));
                    m_layers_changed = true;  // Trigger redraw

                    // Clear hover if it was in this layer
                    clear_hover_if_in_layer(*it);
                }
            }
        }

        /**
         * @brief Check if layer is visible
         */
        [[nodiscard]] bool is_layer_visible(layer_id id) const {
            if (auto it = find_layer_const(id); it != m_layers.end()) {
                // Return false for expired layers (safe lifetime tracking)
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
         * @param id Layer ID
         * @param bounds Layer bounds in logical coordinates
         */
        void set_layer_bounds(layer_id id, const logical_rect& bounds) {
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
         * @param event UI event to route (already converted from native)
         * @return true if event was handled by a layer
         *
         * @details
         * Routes events from top layer to bottom. If a modal is active,
         * only routes to modal and layers above it.
         *
         * ARCHITECTURAL FIX: Now accepts ui_event directly instead of native event.
         * This prevents double conversion and ensures consistent event handling.
         */
        bool route_event(const ui_event& event) {
            // Clean up expired layers first (automatic lifetime management)
            cleanup_expired_layers();

            // ================================================================
            // CRITICAL FIX: Ensure layers are positioned/arranged before routing
            // ================================================================
            // Layers added between render() and route_event() won't have bounds set.
            // Without this, hit testing fails because children have default (0,0,0,0) bounds.
            // We must position and arrange layers with needs_positioning=true before routing.
            ensure_layers_positioned();

            // No conversion needed - we already have ui_event!
            const ui_event& ui_evt = event;

            // Check if modal is active
            std::optional<int> modal_z_index;
            for (const auto& layer : m_layers) {
                if (layer.modal && layer.visible && layer.is_valid()) {
                    modal_z_index = layer.z_index;
                    break;
                }
            }

            // Generic click-outside detection for dismissible layers
            // We MUST defer the callback to avoid modifying m_layers while iterating
            // (the callback typically calls remove_layer which would invalidate iterators)
            std::function<void()> deferred_callback;

            // Check if this is a mouse click event using ui_event
            if (const auto* mouse = std::get_if<mouse_event>(&ui_evt)) {
                if (mouse->act == mouse_event::action::press) {
                    // Check if we have any popup layers
                    for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
                        if (it->type == layer_type::popup && it->visible && it->is_valid()) {
                            // Check if click is outside popup bounds (both in logical coordinates)
                            bool const inside = mouse->x >= it->bounds.x &&
                                               mouse->x < (it->bounds.x + it->bounds.width) &&
                                               mouse->y >= it->bounds.y &&
                                               mouse->y < (it->bounds.y + it->bounds.height);

                            if (!inside) {
                                // Click outside popup - capture callback to invoke AFTER iteration
                                // This allows:
                                // 1. Menu switching: Click Theme while File is open → File closes, Theme opens
                                // 2. Normal clicks: Click anywhere → menu closes, click continues to target
                                if (it->outside_click_callback) {
                                    deferred_callback = it->outside_click_callback;
                                }
                            }
                            // Only check the topmost popup (whether inside or outside)
                            break;
                        }
                    }
                }
            }

            // Now invoke the deferred callback (AFTER iteration is complete)
            // This is safe because we're no longer iterating m_layers
            if (deferred_callback) {
                deferred_callback();
                // Don't return - let event continue to underlying widget for menu switching
            }

            // ================================================================
            // CRITICAL: Check for captured widget (with missing release handling)
            // ================================================================
            // When a widget has captured the mouse (e.g., during window dragging),
            // all mouse events should go directly to that widget regardless of
            // hit-testing or layer hierarchy.
            if (auto* mouse_evt = std::get_if<mouse_event>(&ui_evt)) {
                if (m_input_manager) {
                    auto* captured = m_input_manager->get_captured();
                    if (captured) {
                        if (mouse_evt->act == mouse_event::action::press) {
                            // Hit-test to find target widget for capture transfer check
                            element_type* top_target = nullptr;
                            for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
                                if (!it->visible || !it->is_valid()) continue;
                                if (modal_z_index.has_value() && it->z_index < *modal_z_index) continue;

                                it->with_root([&](element_type* root_ptr) {
                                    if (top_target) return;
                                    hit_test_path<Backend> path;
                                    top_target = root_ptr->hit_test_logical(
                                        mouse_evt->x, mouse_evt->y, path);
                                });

                                if (top_target) break;
                                if (it->modal && it->blocks_events) break;
                            }

                            // Use consolidated capture transfer logic from input_manager
                            // This handles the termbox2 "no release events" workaround
                            bool released = m_input_manager->handle_capture_transfer_on_press(
                                top_target,
                                [&](auto* old_capture) {
                                    // Send synthetic release to previously captured widget
                                    mouse_event release{
                                        .x = mouse_evt->x,
                                        .y = mouse_evt->y,
                                        .btn = mouse_event::button::left,
                                        .act = mouse_event::action::release,
                                        .modifiers = {}
                                    };
                                    old_capture->handle_event(ui_event{release}, event_phase::target);
                                }
                            );

                            // If capture wasn't transferred, deliver to captured widget
                            if (!released) {
                                return captured->handle_event(ui_evt, event_phase::target);
                            }
                            // Otherwise, fall through to normal routing
                        } else {
                            // Non-press events go directly to captured widget
                            bool handled = captured->handle_event(ui_evt, event_phase::target);
                            if (mouse_evt->act == mouse_event::action::release) {
                                m_input_manager->release_capture();
                            }
                            return handled;
                        }
                    }
                }
            }

            // ================================================================
            // ITERATOR SAFETY: Snapshot layer IDs before routing
            // ================================================================
            // Event handlers may call remove_layer() or other mutations during
            // routing. To prevent iterator invalidation, we take a snapshot of
            // layer IDs (highest z first) and look up each layer by ID.
            // If a layer was removed during routing, find_layer() returns end()
            // and we safely skip it.
            std::vector<layer_id> layer_snapshot;
            layer_snapshot.reserve(m_layers.size());
            for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
                layer_snapshot.push_back(it->id);
            }

            // Route to layers using snapshot (highest z first)
            for (layer_id lid : layer_snapshot) {
                // Look up layer by ID - may have been removed during routing
                auto it = find_layer(lid);
                if (it == m_layers.end()) {
                    continue;  // Layer was removed during event handling
                }

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
                    if (auto* mouse = std::get_if<mouse_event>(&ui_evt)) {
                        // Use hit_test_logical to preserve full precision of logical coordinates
                        hit_test_path<Backend> path;
                        element_type* target = root_ptr->hit_test_logical(mouse->x, mouse->y, path);

                        // CRITICAL: Clear hover on previously hovered element within layers
                        // This fixes submenu items staying highlighted when mouse moves away.
                        // The old element won't receive a mouse event (not in hit test path),
                        // so we must explicitly clear its state here.
                        if (m_layer_hovered != target) {
                            // Safe to call: destroying signal clears m_layer_hovered when element dies
                            if (m_layer_hovered) {
                                m_layer_hovered->reset_hover_and_press_state();
                            }
                            set_layer_hover(target, lid);
                        }

                        if (target) {
                            if (!path.empty()) {
                                // Route event through three phases (capture/target/bubble)
                                handled = ::onyxui::route_event(ui_evt, path);
                            } else {
                                // Path empty (simple widget tree) - deliver directly to target
                                handled = target->handle_event(ui_evt, event_phase::target);
                            }
                        }
                    } else {
                        // Non-mouse events (keyboard, resize) - deliver to root directly
                        handled = root_ptr->handle_event(ui_evt, event_phase::target);
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

            // Mouse moved out of all layers - clear layer hover
            if (std::holds_alternative<mouse_event>(ui_evt) && m_layer_hovered) {
                // Safe to call: destroying signal clears m_layer_hovered when element dies
                m_layer_hovered->reset_hover_and_press_state();
                set_layer_hover(nullptr, layer_id{});
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
         * Automatically skips expired layers (safe lifetime tracking).
         * Theme must be explicitly passed to ensure popup menus/dialogs use correct styling.
         */
        void render_all_layers(renderer_type& renderer, const rect_type& viewport, const theme_type* theme,
                               const backend_metrics<Backend>& metrics) {
            // Save viewport and metrics for event routing positioning
            m_viewport = viewport;
            m_metrics = metrics;  // Copy metrics for later use in ensure_layers_positioned()

            // Convert physical viewport to logical once at boundary (DPI-aware)
            const logical_rect logical_viewport = metrics.physical_to_logical_rect(viewport);

            // Clean up expired layers first (automatic lifetime management)
            cleanup_expired_layers();

            for (auto& layer : m_layers) {
                if (!layer.visible || !layer.is_valid()) {
                    continue;
                }

                // Position layer if needed (entirely in logical space)
                if (layer.needs_positioning) {
                    position_layer(layer, logical_viewport);
                    layer.needs_positioning = false;
                }

                // Measure, arrange, and render using safe lifetime access
                layer.with_root([&](element_type* root_ptr) {
                    // Layer bounds are already in logical coordinates
                    const logical_rect& logical_bounds = layer.bounds;

                    // Measure with logical size
                    (void)root_ptr->measure(logical_bounds.width, logical_bounds.height);

                    // Arrange with logical coordinates
                    root_ptr->arrange(logical_bounds);

                    // Render with theme and metrics (converts to physical during rendering)
                    root_ptr->render(renderer, theme, metrics);
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
         * Only counts valid (non-expired) modal layers via safe lifetime tracking.
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
         * @brief Get last rendered viewport bounds
         * @return Viewport rectangle (screen dimensions)
         *
         * @details
         * Returns the viewport bounds from the last render_all_layers() call.
         * This provides the actual screen/workspace dimensions for windows
         * that need to maximize without a parent.
         *
         * Default value is {0, 0, 0, 0} until first render.
         */
        [[nodiscard]] rect_type get_viewport() const noexcept {
            return m_viewport;
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
        /**
         * @brief Get bounds of removed layers for dirty region tracking
         * @return Vector of logical bounds (caller converts to physical using metrics)
         */
        [[nodiscard]] const std::vector<logical_rect>& get_removed_layer_dirty_regions() const noexcept {
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
            std::weak_ptr<element_type> root;  // Safe lifetime tracking via weak_ptr
            std::shared_ptr<element_type> owner;  // Keeps root alive (may be non-owning for show_popup/etc)
            bool visible = true;
            bool modal = false;
            bool blocks_events = true;

            // Positioning info (all in logical coordinates)
            logical_rect bounds;
            logical_rect anchor_bounds;
            popup_placement placement = popup_placement::below;
            dialog_position dialog_pos = dialog_position::center;
            bool needs_positioning = false;

            // Click-outside callback for dismissible layers
            std::function<void()> outside_click_callback;

            // Connection to element's destroying signal for automatic cleanup
            // Used by show_popup/show_tooltip/show_modal_dialog which take raw pointers
            scoped_connection destroying_conn;

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
        std::vector<logical_rect> m_removed_layer_dirty_regions;  ///< Bounds of removed layers (logical, caller converts to physical)
        bool m_layers_changed = false;  ///< Flag indicating layers were added/removed since last check
        rect_type m_viewport{};  ///< Last rendered viewport bounds (screen dimensions)
        std::optional<backend_metrics<Backend>> m_metrics;  ///< Last used metrics (for event routing positioning)
        element_type* m_layer_hovered = nullptr;  ///< Currently hovered element within layers (for clearing stale hover)
        layer_id m_hovered_layer_id = layer_id{};  ///< ID of the layer containing m_layer_hovered (for layer expiry checks)
        scoped_connection m_hovered_conn;  ///< Connection to hovered element's destroying signal
        input_manager<Backend>* m_input_manager = nullptr;  ///< Input manager for capture handling

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
            // Use stable_sort to preserve insertion order within the same z-index
            // This ensures deterministic ordering: layers added first render first
            // within their z-band, and bring_to_front() effects are preserved
            std::stable_sort(m_layers.begin(), m_layers.end(),
                [](const layer_data& a, const layer_data& b) {
                    return a.z_index < b.z_index;
                });
        }

        /**
         * @brief Expand bounds for shadow/outline dirty region tracking
         * @param bounds Original layer bounds
         * @return Expanded bounds covering potential shadow/outline area
         *
         * @details
         * Expands bounds symmetrically on ALL sides by SHADOW_MARGIN to ensure
         * dirty region tracking covers shadows, outlines, focus rings, etc.
         * Also clips to non-negative coordinates to prevent out-of-bounds issues
         * with renderers that don't handle negative coordinates gracefully.
         */
        [[nodiscard]] static logical_rect expand_for_shadow(const logical_rect& bounds) noexcept {
            // Conservative estimate for shadow/outline offset
            // Shadow defaults in theme.hh: offset_x=1, offset_y=1
            // We use 2 as a conservative estimate that covers typical themes.
            constexpr double SHADOW_MARGIN = 2.0;

            logical_rect expanded;
            // Expand on all sides, but clamp x/y to 0 (non-negative)
            expanded.x = std::max(bounds.x - logical_unit(SHADOW_MARGIN), 0.0_lu);
            expanded.y = std::max(bounds.y - logical_unit(SHADOW_MARGIN), 0.0_lu);

            // Calculate how much we actually subtracted (may be less if clamped)
            double const actual_left_expansion = (bounds.x - expanded.x).value;
            double const actual_top_expansion = (bounds.y - expanded.y).value;

            // Width/height expand by margin on both sides, adjusted for any clamping
            expanded.width = bounds.width + logical_unit(actual_left_expansion + SHADOW_MARGIN);
            expanded.height = bounds.height + logical_unit(actual_top_expansion + SHADOW_MARGIN);

            return expanded;
        }

        /**
         * @brief Set the layer-hovered element and connect to its destroying signal
         * @param element New hovered element (may be nullptr)
         * @param lid Layer ID containing the element
         *
         * @details
         * Connects to the element's destroying signal so we're notified when it's
         * destroyed. The signal handler clears m_layer_hovered WITHOUT calling
         * any methods on it (the element is being destroyed).
         */
        void set_layer_hover(element_type* element, layer_id lid) {
            m_layer_hovered = element;
            m_hovered_layer_id = lid;

            if (element) {
                m_hovered_conn = scoped_connection(element->destroying, [this](element_type*) {
                    // Element is being destroyed - clear pointer WITHOUT calling methods
                    m_layer_hovered = nullptr;
                    m_hovered_layer_id = layer_id{};
                });
            } else {
                m_hovered_conn = scoped_connection{};  // Disconnect
            }
        }

        /**
         * @brief Clear hover state if the hovered element belongs to a layer
         * @param layer Layer to check against
         *
         * @details
         * Uses m_hovered_layer_id to efficiently check if the hover belongs to
         * this layer. Safe to call: destroying signal clears m_layer_hovered
         * when element dies.
         */
        void clear_hover_if_in_layer(const layer_data& layer) {
            if (!m_layer_hovered || layer.id != m_hovered_layer_id) {
                return;
            }

            // Safe to call: destroying signal clears m_layer_hovered when element dies
            m_layer_hovered->reset_hover_and_press_state();
            set_layer_hover(nullptr, layer_id{});
        }

        /**
         * @brief Remove layers whose elements have been destroyed
         *
         * @details
         * Automatic cleanup of expired layers via weak_ptr tracking.
         * Scans the layer list and removes any layers whose weak_ptr has expired.
         * Called automatically at strategic points (event routing, rendering).
         *
         * **Safety**: The destroying signal on ui_element automatically clears
         * m_layer_hovered when the hovered element dies. No need to check for
         * dangling pointers here - the signal handles it.
         *
         * Marks dirty regions for proper redraw.
         *
         * **Performance**: O(n) where n = number of layers. Typically very fast
         * since most layers remain valid.
         */
        void cleanup_expired_layers() {
            auto const old_size = m_layers.size();

            // First pass: collect bounds of expired layers for dirty tracking
            for (const auto& layer : m_layers) {
                if (!layer.is_valid()) {
                    // Track bounds for dirty region
                    m_removed_layer_dirty_regions.push_back(expand_for_shadow(layer.bounds));
                }
            }

            // Second pass: remove expired layers
            m_layers.erase(
                std::remove_if(m_layers.begin(), m_layers.end(),
                    [](const layer_data& layer) {
                        return !layer.is_valid();
                    }),
                m_layers.end()
            );

            // Note: m_layer_hovered is automatically cleared by the destroying signal
            // when the hovered element dies, so no manual hover cleanup needed here

            if (m_layers.size() != old_size) {
                m_layers_changed = true;
            }
        }

        /**
         * @brief Ensure all layers that need positioning are positioned and arranged
         *
         * @details
         * This is called before event routing to ensure layers added between
         * render() and route_event() have valid bounds for hit testing.
         *
         * Without this, modal dialogs that open and immediately receive clicks
         * will fail hit testing because their children haven't been arranged yet.
         *
         * Uses the stored viewport from the last render_all_layers() call.
         * If no render has happened yet (viewport is empty), skips positioning.
         */
        void ensure_layers_positioned() {
            // Need viewport and metrics from previous render
            // If no render has happened, we can't position layers properly
            if (rect_utils::get_width(m_viewport) <= 0 ||
                rect_utils::get_height(m_viewport) <= 0 ||
                !m_metrics.has_value()) {
                return;  // No valid viewport or metrics yet
            }

            // Convert physical viewport to logical
            const logical_rect logical_viewport = m_metrics->physical_to_logical_rect(m_viewport);

            // Position and arrange any layers that need it
            for (auto& layer : m_layers) {
                if (layer.needs_positioning && layer.visible && layer.is_valid()) {
                    // Position the layer (calculates layer.bounds)
                    position_layer(layer, logical_viewport);
                    layer.needs_positioning = false;

                    // Arrange the layer so children have valid bounds for hit testing
                    layer.with_root([&](element_type* root_ptr) {
                        // Measure first
                        (void)root_ptr->measure(layer.bounds.width, layer.bounds.height);
                        // Then arrange
                        root_ptr->arrange(layer.bounds);
                    });
                }
            }
        }

        /**
         * @brief Position a layer within the logical viewport
         * @param layer Layer to position
         * @param viewport Logical viewport bounds (already converted from physical)
         */
        void position_layer(layer_data& layer, const logical_rect& viewport) {
            if (layer.type == layer_type::modal) {
                position_dialog(layer, viewport);
            } else if (layer.type == layer_type::popup || layer.type == layer_type::tooltip) {
                position_popup(layer, viewport);
            } else {
                // Default: center in viewport
                position_dialog(layer, viewport);
            }
        }

        /**
         * @brief Center a dialog in the logical viewport
         */
        void position_dialog(layer_data& layer, const logical_rect& viewport) {
            // Use safe weak_ptr access for positioning
            layer.with_root([&](element_type* root_ptr) {
                // Measure to get desired size in logical units
                auto size = root_ptr->measure(viewport.width, viewport.height);

                // Center in viewport (all in logical coordinates)
                logical_unit const x = viewport.x + (viewport.width - size.width) / 2.0;
                logical_unit const y = viewport.y + (viewport.height - size.height) / 2.0;

                layer.bounds = logical_rect{x, y, size.width, size.height};
            });
        }

        /**
         * @brief Position a popup relative to its anchor in logical space
         */
        void position_popup(layer_data& layer, const logical_rect& viewport) {
            // Use safe weak_ptr access for positioning
            layer.with_root([&](element_type* root_ptr) {
                // Get anchor dimensions (already in logical coordinates)
                logical_unit const anchor_x = layer.anchor_bounds.x;
                logical_unit const anchor_y = layer.anchor_bounds.y;
                logical_unit const anchor_w = layer.anchor_bounds.width;
                logical_unit const anchor_h = layer.anchor_bounds.height;

                // Use the pre-arranged bounds from when show_popup was called
                // This preserves the intended popup size set by the caller
                // (e.g., combo_box_view sets a specific height based on item count)
                logical_size size{layer.bounds.width, layer.bounds.height};

                // If bounds weren't set, fall back to measuring
                // CRITICAL: For tooltips (anchor_w <= 0), use viewport width to avoid
                // measuring with 0 which could collapse the tooltip's width.
                // Dropdowns use anchor_w as their minimum width.
                if (size.width.value <= 0 || size.height.value <= 0) {
                    logical_unit measure_width = (anchor_w.value > 0) ? anchor_w : viewport.width;
                    size = root_ptr->measure(measure_width, viewport.height);
                }

                // Calculate position based on placement
                logical_unit x = anchor_x;
                logical_unit y = anchor_y;

                switch (layer.placement) {
                    case popup_placement::below:
                        y = anchor_y + anchor_h;
                        break;

                    case popup_placement::above:
                        y = anchor_y - size.height;
                        break;

                    case popup_placement::left:
                        x = anchor_x - size.width;
                        break;

                    case popup_placement::right:
                        x = anchor_x + anchor_w;
                        break;

                    case popup_placement::below_right:
                        x = anchor_x + anchor_w - size.width;
                        y = anchor_y + anchor_h;
                        break;

                    case popup_placement::above_right:
                        x = anchor_x + anchor_w - size.width;
                        y = anchor_y - size.height;
                        break;

                    case popup_placement::auto_best:
                        // Try below first, then above, then right, then left
                        if (anchor_y + anchor_h + size.height <= viewport.y + viewport.height) {
                            y = anchor_y + anchor_h;  // Below fits
                        } else if (anchor_y - size.height >= viewport.y) {
                            y = anchor_y - size.height;  // Above fits
                        } else if (anchor_x + anchor_w + size.width <= viewport.x + viewport.width) {
                            x = anchor_x + anchor_w;  // Right fits
                            y = anchor_y;
                        } else {
                            x = anchor_x - size.width;  // Left
                            y = anchor_y;
                        }
                        break;
                }

                // Clamp to viewport
                // For oversized popups (larger than viewport), pin to viewport origin
                // and let the popup handle overflow via scrolling internally.
                // Order matters: check max FIRST, then clamp to min to ensure non-negative
                logical_unit const x_max = viewport.x + viewport.width - size.width;
                logical_unit const y_max = viewport.y + viewport.height - size.height;

                if (x > x_max) x = x_max;  // Don't extend past right edge
                if (x < viewport.x) x = viewport.x;  // But never go negative (handles oversize case)

                if (y > y_max) y = y_max;  // Don't extend past bottom edge
                if (y < viewport.y) y = viewport.y;  // But never go negative (handles oversize case)

                layer.bounds = logical_rect{x, y, size.width, size.height};
            });
        }
    };

} // namespace onyxui
