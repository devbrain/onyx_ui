/**
 * @file tile_backend.hh
 * @brief Tile-based backend extending sdlpp_backend
 *
 * This backend extends sdlpp_backend with tile rendering capabilities.
 * It uses tile_renderer (which extends sdlpp_renderer) for drawing.
 *
 * **Inherited from sdlpp_backend:**
 * - All geometric types (rect, size, point, color)
 * - All event types (keyboard, mouse, window)
 * - TTF font support
 * - Vector drawing capabilities
 *
 * **Added by tile backend:**
 * - Tile/nine-slice rendering
 * - Bitmap font support
 * - Tile atlas management
 *
 * Usage: the tile backend does NOT have a simple-shell bundle
 * header yet — `<onyxui/for/sdlpp.hh>` is wired to `sdlpp_backend`,
 * not `sdlpp_tile_backend`, and the simple shell's `app_window`
 * owns an `sdlpp_renderer` rather than a `tile_renderer`. Until a
 * tile-specific bundle lands, tile-backed applications embed via
 * `ui_host<sdlpp_tile_backend>` and drive their own render/event
 * loop (see `backends/sdlpp/strategy_ui_demo.cc` for a worked
 * example and the full tile_renderer setup sequence).
 */

#pragma once

// Extend the SDL++ backend
#include <onyxui/sdlpp/sdlpp_backend.hh>

// Tile-specific types
#include <onyxui/tile/tile_types.hh>
#include <onyxui/tile/tile_theme.hh>
#include <onyxui/tile/tile_renderer.hh>

#include <functional>
#include <memory>
#include <optional>

// Forward declarations
namespace sdlpp {
    class renderer;
    class texture;
}

namespace onyxui::tile {

// ============================================================================
// Global Theme Management
// ============================================================================

/**
 * @brief Set the global tile theme
 * @param theme Theme to use for all tile widgets
 *
 * Must be called before creating any tile widgets.
 * The theme must remain valid for the lifetime of the application.
 */
void set_theme(const tile_theme& theme);

/**
 * @brief Get the current global tile theme
 * @return Reference to current theme
 * @throws std::runtime_error if no theme has been set
 */
[[nodiscard]] const tile_theme& get_theme();

/**
 * @brief Check if a theme has been set
 * @return true if set_theme() has been called
 */
[[nodiscard]] bool has_theme() noexcept;

/**
 * @brief Set the current tile renderer (set during tile-backend setup)
 * @param renderer Tile renderer instance
 *
 * This is set by the tile-backend setup path and should not be called directly.
 */
void set_renderer(tile_renderer* renderer);

/**
 * @brief Get the current tile renderer
 * @return Pointer to current renderer, or nullptr if no tile renderer is currently active
 *
 * Tile widgets use this to access the renderer for drawing.
 */
[[nodiscard]] tile_renderer* get_renderer() noexcept;

// ============================================================================
// Tile Backend (extends SDL++ Backend)
// ============================================================================

/**
 * @struct sdlpp_tile_backend
 * @brief Backend for tile/sprite-based UI rendering, extending sdlpp_backend
 *
 * This backend extends sdlpp_backend with tile rendering capabilities.
 * All standard widgets work unchanged (using inherited TTF/vector rendering),
 * while tile-specific widgets can use tile_renderer's tile/nine-slice methods.
 *
 * **Inherited from sdlpp_backend:**
 * - rect_type, size_type, point_type, color_type
 * - All event types (keyboard, mouse, window)
 * - init(), shutdown() (see docs/ONYXUI_SIMPLE_SHELL_DESIGN.md for standalone-tool usage)
 * - TTF font support
 *
 * **Overridden:**
 * - renderer_type = tile_renderer (extends sdlpp_renderer)
 * - register_themes() - adds tile-specific theme setup
 *
 * **Added:**
 * - Global tile theme management (set_theme/get_theme)
 * - Access to tile renderer for tile widgets
 */
struct sdlpp_tile_backend : public onyxui::sdlpp::sdlpp_backend {
    // ========================================================================
    // Override renderer type to use tile_renderer
    // ========================================================================

    using renderer_type = tile_renderer;

    // ========================================================================
    // Backend identification
    // ========================================================================

    [[nodiscard]] static constexpr const char* name() noexcept { return "SDL++ Tile"; }

    // ========================================================================
    // Event Conversion (uses tile backend metrics for coordinate conversion)
    // ========================================================================

    /**
     * @brief Convert SDL3 event to framework-level ui_event
     * @param native SDL event from event queue
     * @return Framework event, or nullopt if unknown/unhandled event type
     *
     * This override uses tile backend metrics for physical-to-logical
     * coordinate conversion, which may differ from base sdlpp_backend.
     */
    [[nodiscard]] static std::optional<onyxui::ui_event> create_event(
        const onyxui::sdlpp::sdl_event& native) noexcept;

    // ========================================================================
    // Theme Registration (override to add tile-specific themes)
    // ========================================================================

    /**
     * @brief Register default themes for tile backend
     * @param registry Theme registry to register themes with
     *
     * @details
     * Registers a theme suitable for tile-based rendering with bitmap fonts.
     * Standard widgets use this theme, while tile widgets use the global
     * tile_theme (via get_theme()).
     *
     * Called automatically by ui_host on first context creation.
     */
    static void register_themes(
        onyxui::theme_registry<sdlpp_tile_backend>& registry);

};

} // namespace onyxui::tile
