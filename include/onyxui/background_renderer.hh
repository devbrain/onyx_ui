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

#include <cstdint>
#include <vector>
#include <onyxui/concepts/backend.hh>
#include <onyxui/concepts/rect_like.hh>

namespace onyxui {

    /**
     * @enum background_mode
     * @brief How the UI background should be rendered
     */
    enum class background_mode : uint8_t {
        solid,       ///< Solid color fill (default)
        transparent, ///< No background - for game overlays, HUDs
        pattern      ///< Pattern/texture fill (future extension)
    };

    /**
     * @class background_renderer
     * @brief Manages background rendering for the UI
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * The background_renderer is responsible for filling the viewport background
     * before widgets are rendered. It supports multiple modes:
     *
     * **Solid Mode (default):**
     * - Fills viewport with a solid color
     * - Optimized with dirty regions (only fills changed areas)
     * - Suitable for traditional desktop applications
     *
     * **Transparent Mode:**
     * - No background rendering
     * - UI widgets overlay game/3D rendering
     * - Suitable for game HUDs, in-game menus
     *
     * **Pattern Mode (future):**
     * - Fills with repeating pattern or texture
     * - Can be tiled, stretched, or centered
     * - Suitable for themed applications
     *
     * ## Usage
     *
     * @code
     * // Access from ui_services
     * auto* bg = ui_services<Backend>::background();
     *
     * // Configure
     * bg->set_mode(background_mode::solid);
     * bg->set_color({0, 0, 170});  // Blue background
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
        using box_style_type = typename renderer_type::box_style;

        /**
         * @brief Default constructor
         *
         * @details
         * Initializes with solid mode and default-constructed color (usually black).
         */
        background_renderer() = default;

        /**
         * @brief Construct with specific color
         * @param color Background color (for solid mode)
         */
        explicit background_renderer(const color_type& color)
            : m_mode(background_mode::solid)
            , m_background_color(color) {}

        /**
         * @brief Copy constructor
         *
         * @details
         * Creates a copy with the same mode and color.
         * Since members are trivially copyable (enum + color), compiler-generated is optimal.
         */
        background_renderer(const background_renderer&) = default;

        /**
         * @brief Move constructor
         *
         * @details
         * Moves state from another background_renderer.
         * Since members are trivially copyable, this is equivalent to copy.
         */
        background_renderer(background_renderer&&) noexcept = default;

        /**
         * @brief Copy assignment operator
         *
         * @details
         * Assigns mode and color from another background_renderer.
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
         * @brief Set background rendering mode
         * @param mode The rendering mode (solid, transparent, pattern)
         *
         * @details
         * **Frame-Based Behavior:**
         * - Changes take effect on the **next frame** (next `ui_handle::display()` call)
         * - No UI invalidation is triggered
         * - Application controls when to redraw
         *
         * This is consistent with the event-driven architecture where the application
         * decides when to render frames.
         */
        void set_mode(background_mode mode) noexcept {
            m_mode = mode;
        }

        /**
         * @brief Set background color (for solid mode)
         * @param color The background color
         *
         * @details
         * **Frame-Based Behavior:**
         * - Color change takes effect on the **next frame**
         * - No immediate redraw is triggered
         * - Application controls rendering via `ui_handle::display()`
         *
         * @example Changing background color
         * @code
         * auto* bg = ui_services<Backend>::background();
         * bg->set_color({255, 0, 0});  // Red
         * // ... color change pending ...
         * ui.display();  // NOW the red background is rendered
         * @endcode
         */
        void set_color(const color_type& color) noexcept {
            m_background_color = color;
        }

        /**
         * @brief Get current background mode
         */
        [[nodiscard]] background_mode get_mode() const noexcept {
            return m_mode;
        }

        /**
         * @brief Get current background color
         */
        [[nodiscard]] const color_type& get_color() const noexcept {
            return m_background_color;
        }

        /**
         * @brief Render the background
         * @param renderer The renderer instance
         * @param viewport The viewport bounds to fill
         * @param dirty_regions Dirty regions from previous frame
         *
         * @details
         * Renders background according to current mode:
         *
         * **Solid Mode:**
         * - If dirty_regions is empty: Fills entire viewport
         * - If dirty_regions provided: Fills only dirty regions (optimization)
         *
         * **Transparent Mode:**
         * - Does nothing (no background fill)
         *
         * **Pattern Mode:**
         * - Future: Renders pattern/texture
         */
        void render(renderer_type& renderer,
                   const rect_type& viewport,
                   const std::vector<rect_type>& dirty_regions) {
            switch (m_mode) {
                case background_mode::solid:
                    render_solid(renderer, viewport, dirty_regions);
                    break;

                case background_mode::transparent:
                    // Do nothing - no background
                    break;

                case background_mode::pattern:
                    // Future: render_pattern(renderer, viewport, dirty_regions);
                    // For now, fall back to solid
                    render_solid(renderer, viewport, dirty_regions);
                    break;
            }
        }

    private:
        /**
         * @brief Render solid color background
         */
        void render_solid(renderer_type& renderer,
                         const rect_type& viewport,
                         const std::vector<rect_type>& dirty_regions) {
            // Set renderer colors
            renderer.set_foreground(m_background_color);
            renderer.set_background(m_background_color);

            // Create fill style (no border, solid fill)
            box_style_type fill_style{};

            if (dirty_regions.empty()) {
                // No dirty regions: fill entire viewport
                renderer.draw_box(viewport, fill_style);
            } else {
                // Fill only dirty regions for optimization
                for (const auto& region : dirty_regions) {
                    renderer.draw_box(region, fill_style);
                }
            }
        }

        background_mode m_mode = background_mode::solid;
        color_type m_background_color{};  ///< Background color for solid mode
    };

} // namespace onyxui
