/**
 * @file sdlpp_backend.hh
 * @brief SDL++ backend implementation using lib_sdlpp for SDL3
 *
 * This backend supports two usage modes:
 *
 * ## Standalone App Mode
 * Use run_app() for a complete application with event loop:
 * @code
 * int main() {
 *     return sdlpp_backend::run_app<MyWidget>("My App", 800, 600);
 * }
 * @endcode
 *
 * ## Game Engine Integration Mode
 * Use init/shutdown/process_event for embedding in existing apps:
 * @code
 * sdlpp_backend::init(my_sdl_renderer);
 * while (running) {
 *     while (auto ev = poll_sdl_event()) {
 *         if (auto ui_ev = sdlpp_backend::process_event(*ev)) {
 *             ui.handle_event(*ui_ev);
 *         }
 *     }
 *     ui.display();
 * }
 * sdlpp_backend::shutdown();
 * @endcode
 */

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/sdlpp/geometry.hh>
#include <onyxui/sdlpp/colors.hh>
#include <onyxui/sdlpp/sdlpp_renderer.hh>
#include <onyxui/sdlpp/sdlpp_events.hh>
#include <onyxui/events/ui_event.hh>
#include <onyxui/theming/theme_registry.hh>
#include <onyxui/services/ui_context.hh>
#include <onyxui/ui_handle.hh>
#include <optional>
#include <memory>
#include <functional>

// Forward declarations for SDL types
namespace sdlpp {
    class renderer;
    class event;
}

namespace onyxui::sdlpp {

/**
 * @struct sdlpp_backend
 * @brief SDL++ backend for graphical UIs using SDL3 via lib_sdlpp
 *
 * This backend provides all required types for the UIBackend concept:
 * - Geometric types: rect, size, point (pixel-based)
 * - Event types: SDL3 events via lib_sdlpp
 * - Renderer: Hardware-accelerated 2D rendering
 *
 * Additionally provides static methods for platform integration:
 * - init/shutdown for game engine embedding
 * - process_event for event conversion
 * - run_app for standalone applications
 */
struct sdlpp_backend {
    // ========================================================================
    // Required Type Definitions (UIBackend concept)
    // ========================================================================

    // Required geometric types (pixel-based)
    using rect_type = rect;
    using size_type = size;
    using point_type = point;

    // Required event types (SDL3 events)
    using event_type = sdl_event;
    using keyboard_event_type = sdl_keyboard_event;
    using mouse_button_event_type = sdl_mouse_button_event;
    using mouse_motion_event_type = sdl_mouse_motion_event;
    using mouse_wheel_event_type = sdl_mouse_wheel_event;
    using text_input_event_type = sdl_text_input_event;
    using window_event_type = sdl_window_event;

    // Required rendering types
    using color_type = color;
    using renderer_type = sdlpp_renderer;

    // Backend identification
    [[nodiscard]] static constexpr const char* name() noexcept { return "SDL++"; }

    // GUI backends have full mouse tracking
    static constexpr bool has_mouse_tracking = true;

    // ========================================================================
    // Event Conversion
    // ========================================================================

    /**
     * @brief Convert SDL3 event to framework-level ui_event
     * @param native SDL event from event queue
     * @return Framework event, or nullopt if unknown/unhandled event type
     */
    [[nodiscard]] static std::optional<onyxui::ui_event> create_event(
        const sdl_event& native) noexcept;

    // ========================================================================
    // Theme Registration
    // ========================================================================

    /**
     * @brief Register backend-provided themes
     * @param registry Theme registry to populate
     *
     * Registered themes:
     * 1. "Windows 3.11" (default) - Classic Windows 3.11 style
     */
    static void register_themes(
        onyxui::theme_registry<sdlpp_backend>& registry);

    // ========================================================================
    // Platform Integration (Game Engine Mode)
    // ========================================================================

    /**
     * @brief Initialize backend with external SDL renderer
     * @param renderer SDL renderer from game engine
     * @return true on success, false on failure
     *
     * Call this before using OnyxUI in a game engine context.
     * The game engine owns the SDL context and renderer.
     */
    static bool init(::sdlpp::renderer& renderer);

    /**
     * @brief Shutdown backend and release resources
     *
     * Must be called before SDL shuts down to prevent crashes.
     * Clears font cache and releases GPU resources.
     */
    static void shutdown();

    /**
     * @brief Process SDL event and convert to ui_event
     * @param event Native SDL event
     * @return ui_event if event is relevant to UI, nullopt otherwise
     *
     * Use this in game engine integration to convert SDL events.
     * Returns nullopt for events OnyxUI doesn't handle (e.g., audio events).
     */
    [[nodiscard]] static std::optional<ui_event> process_event(
        const ::sdlpp::event& event);

    /**
     * @brief Check if quit was requested (window close, etc.)
     * @return true if application should exit
     */
    [[nodiscard]] static bool should_quit() noexcept;

    /**
     * @brief Clear the quit flag
     */
    static void clear_quit_flag() noexcept;

    // ========================================================================
    // Standalone Application Mode
    // ========================================================================

    /**
     * @brief Run a complete standalone application
     * @tparam Widget Widget class template (must accept Backend parameter)
     * @param title Window title
     * @param width Initial window width
     * @param height Initial window height
     * @param setup Optional callback to configure widget after creation
     * @return Exit code (0 for success)
     *
     * Creates SDL window/renderer, runs event loop, handles cleanup.
     * The Widget class should have a should_quit() method or use
     * sdlpp_backend::should_quit() for exit detection.
     *
     * @code
     * template<typename Backend>
     * class MyApp : public main_window<Backend> {
     *     // ... widget implementation
     * };
     *
     * int main() {
     *     return sdlpp_backend::run_app<MyApp>("My App", 800, 600);
     * }
     * @endcode
     */
    template<template<typename> class Widget>
    static int run_app(
        const char* title = "OnyxUI",
        int width = 800,
        int height = 600,
        std::function<void(Widget<sdlpp_backend>&)> setup = nullptr);

    /**
     * @brief Run standalone application with a concrete widget type
     * @tparam Widget Concrete widget class (not a template)
     * @param title Window title
     * @param width Initial window width
     * @param height Initial window height
     * @param setup Optional callback to configure widget after creation
     * @return Exit code (0 for success)
     *
     * This overload is for widgets that are concrete classes (not templates).
     * Useful when Backend type is determined at compile-time via macros.
     *
     * @code
     * // In backend_config.hh:
     * using Backend = onyxui::sdlpp::sdlpp_backend;
     *
     * // Concrete widget class:
     * class MyApp : public main_window<Backend> { ... };
     *
     * int main() {
     *     return Backend::run_app<MyApp>("My App", 800, 600);
     * }
     * @endcode
     */
    template<typename Widget>
        requires (!requires { typename Widget::template rebind<sdlpp_backend>; })
    static int run_app(
        const char* title = "OnyxUI",
        int width = 800,
        int height = 600,
        std::function<void(Widget&)> setup = nullptr);
};

} // namespace onyxui::sdlpp

// Static assertion to verify backend compliance
static_assert(onyxui::UIBackend<onyxui::sdlpp::sdlpp_backend>,
              "sdlpp_backend must satisfy UIBackend concept");

// Include template implementation
#include <onyxui/sdlpp/sdlpp_backend_impl.hh>
