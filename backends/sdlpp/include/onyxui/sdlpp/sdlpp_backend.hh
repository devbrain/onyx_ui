/**
 * @file sdlpp_backend.hh
 * @brief SDL++ backend implementation using lib_sdlpp for SDL3
 *
 * Two supported usage modes:
 *
 * ## Standalone tools — simple shell
 * Include `<onyxui/for/sdlpp.hh>` and use `onyxui::simple::app_window`
 * + `onyxui::simple::run()` for a FLTK-grade entry point.
 *
 * ## Game engine embedding — `ui_host<B>`
 * Hold an `onyxui::ui_host<onyxui::sdlpp::sdlpp_backend>` in your
 * engine class and drive `render()` / `handle_event()` from your
 * existing event pump and renderer. `sdlpp_backend::init()` /
 * `shutdown()` / `process_event()` hook the backend-level
 * one-shots (global renderer registration, native→framework event
 * translation); the UI lifecycle lives on the host.
 */

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/sdlpp/geometry.hh>
#include <onyxui/sdlpp/colors.hh>
#include <onyxui/sdlpp/sdlpp_renderer.hh>
#include <onyxui/sdlpp/sdlpp_events.hh>
#include <onyxui/events/ui_event.hh>
#include <onyxui/theming/theme_registry.hh>
#include <onyxui/ui_host.hh>
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
 *
 * Standalone-tool consumers should use the simple shell
 * (`<onyxui/for/sdlpp.hh>`) rather than reaching into this class.
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

};

} // namespace onyxui::sdlpp

// Static assertion to verify backend compliance
static_assert(onyxui::UIBackend<onyxui::sdlpp::sdlpp_backend>,
              "sdlpp_backend must satisfy UIBackend concept");
