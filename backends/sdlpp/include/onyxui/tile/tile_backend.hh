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
 * Usage:
 * @code
 * // Set up theme at startup
 * tile::set_theme(my_theme);
 *
 * // Run application - all standard widgets work, plus tile widgets
 * return sdlpp_tile_backend::run_app<MyGameUI>("My Game", 1280, 720);
 * @endcode
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
 * @brief Set the current tile renderer (called by run_app)
 * @param renderer Tile renderer instance
 *
 * This is called internally by run_app() and should not be called directly.
 */
void set_renderer(tile_renderer* renderer);

/**
 * @brief Get the current tile renderer
 * @return Pointer to current renderer, or nullptr if not in run_app
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
 * - run_app(), init(), shutdown()
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
     * Called automatically by scoped_ui_context on first context creation.
     */
    static void register_themes(
        onyxui::theme_registry<sdlpp_tile_backend>& registry);

    // ========================================================================
    // Standalone Application Mode (inherited but re-declared for tile backend)
    // ========================================================================

    /**
     * @brief Run a complete standalone application
     * @tparam Widget Widget class template (must accept Backend parameter)
     * @param title Window title
     * @param width Initial window width
     * @param height Initial window height
     * @param setup Optional callback to configure widget after creation
     * @return Exit code (0 for success)
     */
    template<template<typename> class Widget>
    static int run_app(
        const char* title = "Tile UI",
        int width = 800,
        int height = 600,
        std::function<void(Widget<sdlpp_tile_backend>&)> setup = nullptr);

    /**
     * @brief Run standalone application with a concrete widget type
     * @tparam Widget Concrete widget class (not a template)
     */
    template<typename Widget>
        requires (!requires { typename Widget::template rebind<sdlpp_tile_backend>; })
    static int run_app(
        const char* title = "Tile UI",
        int width = 800,
        int height = 600,
        std::function<void(Widget&)> setup = nullptr);
};

} // namespace onyxui::tile

// Include template implementation
#include <onyxui/tile/tile_backend_impl.hh>
