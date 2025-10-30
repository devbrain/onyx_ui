/**
 * @file background_renderer.hh
 * @brief Background rendering component for UI framework
 * @author igor
 * @date 25/10/2025
 *
 * @details
 * The background_renderer encapsulates all background rendering logic,
 * supporting multiple rendering modes:
 * - Solid color fill
 * - Transparent (for game overlays)
 * - Pattern/texture fill (future)
 *
 * This is a global UI concern managed by ui_context, not a per-widget property.
 */

#pragma once

#include <optional>
#include <vector>
#include <onyxui/concepts/backend.hh>
#include <onyxui/concepts/rect_like.hh>

namespace onyxui {

    // Forward declaration for theme change handler
    template<UIBackend Backend>
    struct ui_theme;

    /**
     * @class background_renderer
     * @brief Manages background rendering for the UI
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * The background_renderer is responsible for filling the viewport background
     * before widgets are rendered. Rendering is governed exclusively by the
     * background_style:
     *
     * **With background_style (has_style() == true):**
     * - Renders background using the style (color, fill character, etc.)
     * - Optimized with dirty regions (only fills changed areas)
     * - Suitable for traditional desktop applications
     * - Backend-specific style allows patterns, gradients, textures
     *
     * **Without background_style (has_style() == false):**
     * - No background rendering (transparent)
     * - UI widgets overlay game/3D rendering
     * - Suitable for game HUDs, in-game menus
     *
     * ## Usage
     *
     * @code
     * // Access from ui_services
     * auto* bg = ui_services<Backend>::background();
     *
     * // Set background (enables rendering)
     * bg->set_style(background_style_type{{0, 0, 170}});
     *
     * // Or use convenience method
     * bg->set_color({0, 0, 170});  // Blue background
     *
     * // Clear background (transparent mode)
     * bg->clear_style();
     *
     * // Render (called by ui_handle)
     * bg->render(renderer, viewport_bounds, dirty_regions);
     * @endcode
     *
     * ## Thread Safety
     *
     * Not thread-safe. All operations must occur on UI thread.
     */
    template<UIBackend Backend>
    class background_renderer {
    public:
        using color_type = typename Backend::color_type;
        using rect_type = typename Backend::rect_type;
        using renderer_type = typename Backend::renderer_type;
        using background_style_type = typename renderer_type::background_style;

        /**
         * @brief Default constructor
         *
         * @details
         * Initializes with no background style (transparent mode).
         * Call set_color() or set_style() to enable background rendering.
         */
        background_renderer() = default;

        /**
         * @brief Construct with specific color
         * @param color Background color
         *
         * @details
         * Constructs background_style from color and enables background rendering.
         */
        explicit background_renderer(const color_type& color)
            : m_background_style(background_style_type{color}) {}

        /**
         * @brief Copy constructor
         *
         * @details
         * Creates a copy with the same background style (if any).
         */
        background_renderer(const background_renderer&) = default;

        /**
         * @brief Move constructor
         *
         * @details
         * Moves state from another background_renderer.
         */
        background_renderer(background_renderer&&) noexcept = default;

        /**
         * @brief Copy assignment operator
         *
         * @details
         * Assigns background style from another background_renderer.
         */
        background_renderer& operator=(const background_renderer&) = default;

        /**
         * @brief Move assignment operator
         *
         * @details
         * Moves state from another background_renderer.
         */
        background_renderer& operator=(background_renderer&&) noexcept = default;

        /**
         * @brief Destructor
         *
         * @details
         * Trivial destructor (no resources to release).
         */
        ~background_renderer() = default;

        /**
         * @brief Set background style
         * @param style The background style to use
         *
         * @details
         * Sets the background style and enables background rendering.
         *
         * **Frame-Based Behavior:**
         * - Changes take effect on the **next frame** (next `ui_handle::display()` call)
         * - No UI invalidation is triggered
         * - Application controls when to redraw
         */
        void set_style(const background_style_type& style) noexcept {
            m_background_style = style;
        }

        /**
         * @brief Set background color (convenience method)
         * @param color The background color
         *
         * @details
         * Constructs a background_style from the color and enables background rendering.
         *
         * **Frame-Based Behavior:**
         * - Color change takes effect on the **next frame**
         * - No immediate redraw is triggered
         * - Application controls rendering via `ui_handle::display()`
         *
         * @example Changing background color
         * @code
         * auto* bg = ui_services<Backend>::background();
         * bg->set_color({255, 0, 0});  // Red background
         * // ... color change pending ...
         * ui.display();  // NOW the red background is rendered
         * @endcode
         */
        void set_color(const color_type& color) noexcept {
            m_background_style = background_style_type{color};
        }

        /**
         * @brief Clear background style (transparent mode)
         *
         * @details
         * Removes the background style, disabling background rendering.
         * UI widgets will overlay whatever was previously rendered (game/3D content).
         */
        void clear_style() noexcept {
            m_background_style.reset();
        }

        /**
         * @brief Check if background style is set
         * @return true if background will be rendered, false if transparent
         */
        [[nodiscard]] bool has_style() const noexcept {
            return m_background_style.has_value();
        }

        /**
         * @brief Get current background style
         * @return Optional containing the style if set, nullopt if transparent
         */
        [[nodiscard]] const std::optional<background_style_type>& get_style() const noexcept {
            return m_background_style;
        }

        /**
         * @brief Handle theme change notification
         * @param theme Pointer to new theme (nullptr if theme cleared)
         *
         * @details
         * Updates background color to match the theme's window_bg color.
         * If theme is nullptr, clears the background style (transparent mode).
         *
         * **Automatic Synchronization:**
         * This method is designed to be connected to theme_registry::theme_changed signal
         * in ui_context initialization. This ensures the background always matches
         * the current theme without manual intervention.
         *
         * @example Connect in ui_context
         * @code
         * // In ui_context constructor:
         * auto* themes = ui_services<Backend>::themes();
         * auto* bg = ui_services<Backend>::background();
         *
         * themes->theme_changed.connect([bg](const ui_theme<Backend>* theme) {
         *     bg->on_theme_changed(theme);
         * });
         * @endcode
         */
        void on_theme_changed(const ui_theme<Backend>* theme) noexcept {
            if (theme) {
                set_color(theme->window_bg);
            } else {
                clear_style();
            }
        }

        /**
         * @brief Render the background
         * @param renderer The renderer instance
         * @param viewport The viewport bounds to fill
         * @param dirty_regions Dirty regions from previous frame
         *
         * @details
         * Rendering is governed exclusively by the background_style:
         *
         * **With background_style:**
         * - If dirty_regions is empty: Fills entire viewport with style
         * - If dirty_regions provided: Fills only dirty regions (optimization)
         * - Backend determines how to render (solid, pattern, gradient, etc.)
         *
         * **Without background_style:**
         * - Does nothing (transparent mode)
         * - UI widgets overlay whatever was previously rendered
         */
        void render(renderer_type& renderer,
                   const rect_type& viewport,
                   const std::vector<rect_type>& dirty_regions) {
            // Rendering governed exclusively by background_style presence
            if (!m_background_style.has_value()) {
                return;  // Transparent mode - no background
            }

            // Render with the style (backend determines interpretation)
            if (dirty_regions.empty()) {
                // No dirty regions: fill entire viewport
                renderer.draw_background(viewport, *m_background_style);
            } else {
                // Fill only dirty regions for optimization
                renderer.draw_background(viewport, *m_background_style, dirty_regions);
            }
        }

    private:
        std::optional<background_style_type> m_background_style;  ///< Background style (nullopt = transparent)
    };

} // namespace onyxui
