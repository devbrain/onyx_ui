/**
 * @file tile_core.hh
 * @brief Core tile types and global accessor functions
 *
 * This header provides the minimal types needed by tile widgets:
 * - Global theme access (get_theme, has_theme)
 * - Global renderer access (get_renderer)
 *
 * Unlike tile_backend.hh, this header does not depend on sdlpp types,
 * making it suitable for use with any backend.
 *
 * @warning LIFETIME REQUIREMENTS:
 * The theme passed to set_theme() must remain valid for the entire
 * duration of the application. Typically, create the theme as a
 * static object or as a member of your application class. Destroying
 * the theme while tile widgets exist will result in undefined behavior.
 *
 * ## Thread Safety
 *
 * @warning **NOT THREAD-SAFE**: The tile backend uses global state that is
 * NOT thread-safe. All tile UI operations MUST be performed from a single
 * thread (typically the main/render thread).
 *
 * **DO NOT:**
 * - Call set_theme/get_theme from multiple threads concurrently
 * - Call set_renderer/get_renderer from multiple threads concurrently
 * - Create/destroy tile widgets from background threads
 * - Share tile widgets between threads
 *
 * **For multi-threaded applications:**
 * - Marshal all UI operations to the main thread
 * - Use message queues or event systems to communicate with UI thread
 * - Initialize theme before spawning worker threads
 *
 * This design is intentional for game UIs where single-threaded rendering
 * is the standard pattern and lock-free access provides best performance.
 */

#pragma once

#include <onyxui/tile/tile_types.hh>
#include <onyxui/tile/tile_theme.hh>
#include <cassert>

// Forward declarations
namespace onyxui::tile {
    class tile_renderer;
}

namespace onyxui::tile {

// ============================================================================
// Global Theme Management
// ============================================================================

/**
 * @brief Set the global tile theme
 * @param theme Theme to use for all tile widgets
 *
 * @warning LIFETIME: The theme reference is stored internally. The caller
 * MUST ensure the theme object remains valid for the lifetime of all tile
 * widgets. Typically this means the theme should be:
 * - A static/global object, OR
 * - A member of your Application class that outlives all widgets, OR
 * - Allocated with std::make_unique and held for the application lifetime
 *
 * @pre theme must be a valid tile_theme
 * @post has_theme() returns true
 * @post get_theme() returns reference to the provided theme
 *
 * Must be called before creating any tile widgets.
 *
 * Example:
 * @code
 * // Good: Static theme lives for entire application
 * static tile_theme g_theme = create_my_theme();
 * set_theme(g_theme);
 *
 * // Good: Application-owned theme
 * class MyApp {
 *     tile_theme m_theme;
 * public:
 *     void init() {
 *         set_theme(m_theme);  // m_theme outlives all widgets
 *     }
 * };
 *
 * // BAD: Local theme goes out of scope!
 * void bad_example() {
 *     tile_theme local_theme;
 *     set_theme(local_theme);  // DANGER: local_theme destroyed at end of function!
 * }
 * @endcode
 */
void set_theme(const tile_theme& theme);

/**
 * @brief Get the current global tile theme
 * @return Reference to current theme
 * @throws std::runtime_error if no theme has been set (has_theme() == false)
 *
 * @pre has_theme() must be true
 *
 * @note In debug builds, this function asserts that a theme has been set.
 *       In release builds, calling without a theme throws std::runtime_error.
 */
[[nodiscard]] const tile_theme& get_theme();

/**
 * @brief Check if a theme has been set
 * @return true if set_theme() has been called with a valid theme
 *
 * Always check this before calling get_theme() if there's any doubt
 * whether set_theme() has been called.
 */
[[nodiscard]] bool has_theme() noexcept;

// ============================================================================
// Global Renderer Management
// ============================================================================

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

} // namespace onyxui::tile
