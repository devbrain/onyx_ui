//
// SDL2 Backend Definition
//

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/sdl2/geometry.hh>
#include <onyxui/sdl2/colors.hh>
#include <onyxui/sdl2/sdl2_renderer.hh>
#include <onyxui/sdl2/sdl2_events.hh>
#include <onyxui/theme_registry.hh>

#include <SDL2/SDL.h>

namespace onyxui::sdl2 {
    // ======================================================================
    // Backend Definition
    // ======================================================================

    /**
     * @struct sdl2_backend
     * @brief SDL2 backend for pixel-based GUI rendering
     *
     * This backend provides all required types for the UIBackend concept:
     * - Geometric types: rect, size, point
     * - Event types: SDL2 events
     * - Renderer: sdl2_renderer using hardware-accelerated rendering
     *
     * @example Usage
     * @code
     * using element = ui_element<sdl2_backend>;
     * auto root = std::make_unique<element>();
     *
     * SDL_Window* window = SDL_CreateWindow(...);
     * sdl2_renderer renderer(window, 800, 600);
     *
     * root->measure(800, 600);
     * root->arrange(sdl2_backend::rect{0, 0, 800, 600});
     * root->render(&renderer);
     * renderer.present();
     * @endcode
     */
    struct sdl2_backend {
        // Required geometric types
        using rect_type = rect;
        using size_type = size;
        using point_type = point;

        // Required event types (all use SDL_Event)
        using event_type = SDL_Event;
        using keyboard_event_type = SDL_Event;
        using mouse_button_event_type = SDL_Event;
        using mouse_motion_event_type = SDL_Event;
        using mouse_wheel_event_type = SDL_Event;
        using text_input_event_type = SDL_Event;
        using window_event_type = SDL_Event;

        // Required rendering types
        using color_type = color;
        using renderer_type = sdl2_renderer;

        // Backend identification
        static constexpr const char* name() { return "SDL2"; }

        /**
         * @brief Register backend-provided themes
         * @param registry Theme registry to populate
         *
         * @details
         * Registers all built-in SDL2 themes. The first theme registered
         * is considered the default theme.
         *
         * This method is called automatically by ui_context on first
         * context creation, so manual registration is not required.
         *
         * Registered themes (in order):
         * 1. "Windows 3.11" (default) - Classic Windows 3.x look
         * 2. (More themes can be added later)
         */
        static void register_themes(onyxui::theme_registry<sdl2_backend>& registry);
    };

} // namespace onyxui::sdl2


// Static assertion to verify backend compliance
static_assert(onyxui::UIBackend<onyxui::sdl2::sdl2_backend>, "sdl2_backend must satisfy UIBackend concept");
