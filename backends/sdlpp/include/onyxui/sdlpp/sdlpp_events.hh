/**
 * @file sdlpp_events.hh
 * @brief Event type aliases for SDL++ backend
 *
 * lib_sdlpp provides comprehensive event handling via sdlpp::event
 * and type-safe event variants.
 */

#pragma once

#include <sdlpp/events/events.hh>

namespace onyxui::sdlpp {

// SDL3 event types via lib_sdlpp
using sdl_event = ::sdlpp::event;

// Specific event types for UIBackend concept compliance
using sdl_keyboard_event = ::sdlpp::event;
using sdl_mouse_button_event = ::sdlpp::event;
using sdl_mouse_motion_event = ::sdlpp::event;
using sdl_mouse_wheel_event = ::sdlpp::event;
using sdl_text_input_event = ::sdlpp::event;
using sdl_window_event = ::sdlpp::event;

} // namespace onyxui::sdlpp
