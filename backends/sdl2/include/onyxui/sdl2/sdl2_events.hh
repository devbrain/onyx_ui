//
// SDL2 Backend Event Types
//

#pragma once

#include <SDL2/SDL.h>

namespace onyxui::sdl2 {
    // SDL2 backend uses SDL_Event for all event types
    // The event_target system will handle event dispatching based on event.type

    // Note: SDL_Event is a union type that contains different event structures
    // based on the event.type field:
    // - SDL_KEYDOWN/SDL_KEYUP -> event.key
    // - SDL_MOUSEBUTTONDOWN/SDL_MOUSEBUTTONUP -> event.button
    // - SDL_MOUSEMOTION -> event.motion
    // - SDL_MOUSEWHEEL -> event.wheel
    // - SDL_TEXTINPUT -> event.text
    // - SDL_WINDOWEVENT -> event.window
}
