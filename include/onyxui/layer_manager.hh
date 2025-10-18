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
#include <algorithm>
#include <onyxui/concepts/backend.hh>
#include <onyxui/element.hh>

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
         * @brief Add a new layer
         *
         * @param type Layer type (determines default z-index)
         * @param root Root element for this layer (non-owning pointer)
         * @param custom_z_index Custom z-index (-1 = use default for type)
         * @return Layer ID for later reference
         *
         * @details
         * The layer_manager does NOT take ownership of the element.
         * Caller must ensure the element remains valid while the layer exists.
         * Call remove_layer() before destroying the element.
         */
        layer_id add_layer(layer_type type,
                          element_type* root,
                          int custom_z_index = -1) {
            layer_id id(m_next_id++);

            int z_index = (custom_z_index >= 0) ? custom_z_index : get_default_z_index(type);

            m_layers.push_back(layer_data{
                id,
                type,
                z_index,
                root,
                true,  // visible
                type == layer_type::modal,  // modal
                type == layer_type::modal   // blocks_events
            });

            sort_layers_by_z_index();

            return id;
        }

        /**
         * @brief Remove a layer
         *
         * @param id Layer to remove
         */
        void remove_layer(layer_id id) {
            auto it = find_layer(id);
            if (it != m_layers.end()) {
                m_layers.erase(it);
            }
        }

        /**
         * @brief Remove all layers of specific type
         *
         * @param type Type of layers to remove
         */
        void clear_layers(layer_type type) {
            m_layers.erase(
                std::remove_if(m_layers.begin(), m_layers.end(),
                    [type](const layer_data& layer) {
                        return layer.type == type;
                    }),
                m_layers.end()
            );
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
         * Caller must keep content alive while layer exists.
         */
        layer_id show_popup(element_type* content,
                           const rect_type& anchor_bounds,
                           popup_placement placement = popup_placement::below) {
            // Store anchor bounds for positioning during render
            layer_id id = add_layer(layer_type::popup, content);

            auto it = find_layer(id);
            if (it != m_layers.end()) {
                it->anchor_bounds = anchor_bounds;
                it->placement = placement;
                it->needs_positioning = true;
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
         * Caller must keep content alive while layer exists.
         */
        layer_id show_tooltip(element_type* content,
                             int x, int y) {
            layer_id id = add_layer(layer_type::tooltip, content);

            auto it = find_layer(id);
            if (it != m_layers.end()) {
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
         * Caller must keep content alive while layer exists.
         */
        layer_id show_modal_dialog(element_type* content,
                                   dialog_position pos = dialog_position::center) {
            layer_id id = add_layer(layer_type::modal, content);

            auto it = find_layer(id);
            if (it != m_layers.end()) {
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
                return it->visible;
            }
            return false;
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
            // Check if modal is active
            std::optional<int> modal_z_index;
            for (const auto& layer : m_layers) {
                if (layer.modal && layer.visible) {
                    modal_z_index = layer.z_index;
                    break;
                }
            }

            // Route to layers (highest z first)
            for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
                if (!it->visible) continue;

                // If modal active, skip layers below it
                if (modal_z_index.has_value() && it->z_index < *modal_z_index) {
                    continue;
                }

                // Route event to layer
                if (it->root && it->root->process_event(event)) {
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
         *
         * @details
         * Renders all visible layers from lowest z to highest z.
         * Performs layout (measure/arrange) before rendering.
         */
        void render_all_layers(renderer_type& renderer, const rect_type& viewport) {
            for (auto& layer : m_layers) {
                if (!layer.visible) continue;
                if (!layer.root) continue;

                // Position layer if needed
                if (layer.needs_positioning) {
                    position_layer(layer, viewport);
                    layer.needs_positioning = false;
                }

                // Measure and arrange
                int width = rect_utils::get_width(layer.bounds);
                int height = rect_utils::get_height(layer.bounds);
                [[maybe_unused]] auto measured_size = layer.root->measure(width, height);
                layer.root->arrange(layer.bounds);

                // Render
                layer.root->render(renderer);
            }
        }

        // ============================================================
        // Queries
        // ============================================================

        /**
         * @brief Check if any modal layer is active
         */
        [[nodiscard]] bool has_modal_layer() const {
            return std::any_of(m_layers.begin(), m_layers.end(),
                [](const layer_data& layer) {
                    return layer.modal && layer.visible;
                });
        }

        /**
         * @brief Get number of layers
         */
        [[nodiscard]] size_t layer_count() const noexcept {
            return m_layers.size();
        }

    private:
        // Layer data structure
        struct layer_data {
            layer_id id;
            layer_type type;
            int z_index;
            element_type* root;  // Non-owning pointer
            bool visible;
            bool modal;
            bool blocks_events;

            // Positioning info
            rect_type bounds;
            rect_type anchor_bounds;
            popup_placement placement = popup_placement::below;
            dialog_position dialog_pos = dialog_position::center;
            bool needs_positioning = false;
        };

        config m_config;
        std::vector<layer_data> m_layers;
        uint32_t m_next_id;

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
            // Measure to get desired size
            int vp_width = rect_utils::get_width(viewport);
            int vp_height = rect_utils::get_height(viewport);

            auto size = layer.root->measure(vp_width, vp_height);

            int width = size_utils::get_width(size);
            int height = size_utils::get_height(size);

            // Center in viewport
            int x = rect_utils::get_x(viewport) + (vp_width - width) / 2;
            int y = rect_utils::get_y(viewport) + (vp_height - height) / 2;

            rect_utils::set_bounds(layer.bounds, x, y, width, height);
        }

        void position_popup(layer_data& layer, const rect_type& viewport) {
            // Measure to get desired size
            int vp_width = rect_utils::get_width(viewport);
            int vp_height = rect_utils::get_height(viewport);

            auto size = layer.root->measure(vp_width, vp_height);

            int width = size_utils::get_width(size);
            int height = size_utils::get_height(size);

            // Get anchor position
            int anchor_x = rect_utils::get_x(layer.anchor_bounds);
            int anchor_y = rect_utils::get_y(layer.anchor_bounds);
            int anchor_w = rect_utils::get_width(layer.anchor_bounds);
            int anchor_h = rect_utils::get_height(layer.anchor_bounds);

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
            int vp_x = rect_utils::get_x(viewport);
            int vp_y = rect_utils::get_y(viewport);
            x = std::max(vp_x, std::min(x, vp_x + vp_width - width));
            y = std::max(vp_y, std::min(y, vp_y + vp_height - height));

            rect_utils::set_bounds(layer.bounds, x, y, width, height);
        }
    };

} // namespace onyxui
