/**
 * @file events.hh
 * @brief Event traits and concepts for backend-agnostic event handling
 * @author igor
 * @date 10/10/2025
 *
 * @details
 * This file provides traits and concepts to work with native backend events
 * (SDL_Event, GLFWwindow callbacks, etc.) without creating an abstraction layer.
 *
 * The goal is to:
 * - Work directly with native events (no conversion overhead)
 * - Use concepts to ensure type safety at compile time
 * - Use traits to extract common data from different event types
 * - Avoid reinventing the wheel - use native event structures
 */

#pragma once

#include <concepts>
#include <type_traits>
#include <string_view>
#include <cstdint>

// Forward declarations for common backends
#ifdef ONYXUI_SDL_BACKEND
struct SDL_Event;
struct SDL_MouseMotionEvent;
struct SDL_MouseButtonEvent;
struct SDL_KeyboardEvent;
#endif

#ifdef ONYXUI_GLFW_BACKEND
struct GLFWwindow;
#endif

namespace onyxui {

    // ======================================================================
    // Forward Declarations
    // ======================================================================

    /**
     * @struct event_traits
     * @brief Forward declaration of traits to extract data from backend-specific events
     */
    template<typename Event>
    struct event_traits;

    // ======================================================================
    // Event Type Concepts
    // ======================================================================

    /**
     * @concept MousePositionEvent
     * @brief Concept for events that contain mouse position
     */
    template<typename T>
    concept MousePositionEvent = requires(const T& e) {
        { event_traits<T>::mouse_x(e) } -> std::convertible_to<int>;
        { event_traits<T>::mouse_y(e) } -> std::convertible_to<int>;
    };

    /**
     * @concept MouseButtonEvent
     * @brief Concept for events that contain mouse button info
     */
    template<typename T>
    concept MouseButtonEvent = MousePositionEvent<T> && requires(const T& e) {
        { event_traits<T>::mouse_button(e) } -> std::convertible_to<int>;
        { event_traits<T>::is_button_press(e) } -> std::convertible_to<bool>;
    };

    /**
     * @concept MouseWheelEvent
     * @brief Concept for mouse wheel/scroll events
     */
    template<typename T>
    concept MouseWheelEvent = requires(const T& e) {
        { event_traits<T>::wheel_delta_x(e) } -> std::convertible_to<float>;
        { event_traits<T>::wheel_delta_y(e) } -> std::convertible_to<float>;
    };

    /**
     * @concept KeyboardEvent
     * @brief Concept for keyboard events
     */
    template<typename T>
    concept KeyboardEvent = requires(const T& e) {
        { event_traits<T>::key_code(e) } -> std::convertible_to<int>;
        { event_traits<T>::is_key_press(e) } -> std::convertible_to<bool>;
        { event_traits<T>::is_repeat(e) } -> std::convertible_to<bool>;
    };

    /**
     * @concept TextInputEvent
     * @brief Concept for text input events
     */
    template<typename T>
    concept TextInputEvent = requires(const T& e) {
        { event_traits<T>::text(e) } -> std::convertible_to<std::string_view>;
    };

    /**
     * @concept ModifierState
     * @brief Concept for events that have modifier key state
     */
    template<typename T>
    concept ModifierState = requires(const T& e) {
        { event_traits<T>::shift_pressed(e) } -> std::convertible_to<bool>;
        { event_traits<T>::ctrl_pressed(e) } -> std::convertible_to<bool>;
        { event_traits<T>::alt_pressed(e) } -> std::convertible_to<bool>;
    };

    /**
     * @concept WindowEvent
     * @brief Concept for window events (resize, close, etc.)
     */
    template<typename T>
    concept WindowEvent = requires(const T& e) {
        { event_traits<T>::window_width(e) } -> std::convertible_to<int>;
        { event_traits<T>::window_height(e) } -> std::convertible_to<int>;
    };

    /**
     * @concept TimestampedEvent
     * @brief Concept for events with timestamps
     */
    template<typename T>
    concept TimestampedEvent = requires(const T& e) {
        { event_traits<T>::timestamp(e) } -> std::convertible_to<uint32_t>;
    };

    // ======================================================================
    // Event Traits (Primary Template - Specialze for each backend)
    // ======================================================================

    /**
     * @struct event_traits
     * @brief Traits to extract common data from backend-specific events
     *
     * Specialize this for each backend to provide unified access to event data
     * without creating wrapper types.
     */
    template<typename Event>
    struct event_traits {
        // No default implementation - must specialize
    };

    // ======================================================================
    // SDL Event Traits
    // ======================================================================

#ifdef ONYXUI_SDL_BACKEND
    #include <SDL2/SDL.h>

    // Traits for SDL_MouseMotionEvent
    template<>
    struct event_traits<SDL_MouseMotionEvent> {
        [[nodiscard]] static int mouse_x(const SDL_MouseMotionEvent& e) noexcept { return e.x; }
        [[nodiscard]] static int mouse_y(const SDL_MouseMotionEvent& e) noexcept { return e.y; }
        [[nodiscard]] static int delta_x(const SDL_MouseMotionEvent& e) noexcept { return e.xrel; }
        [[nodiscard]] static int delta_y(const SDL_MouseMotionEvent& e) noexcept { return e.yrel; }
        [[nodiscard]] static uint32_t timestamp(const SDL_MouseMotionEvent& e) noexcept { return e.timestamp; }

        // Modifier state from event state field
        [[nodiscard]] static bool shift_pressed(const SDL_MouseMotionEvent& e) noexcept {
            // SDL_MouseMotionEvent doesn't carry mod state, fall back to global
            // This is a limitation of SDL's API
            return SDL_GetModState() & KMOD_SHIFT;
        }
        [[nodiscard]] static bool ctrl_pressed(const SDL_MouseMotionEvent& e) noexcept {
            return SDL_GetModState() & KMOD_CTRL;
        }
        [[nodiscard]] static bool alt_pressed(const SDL_MouseMotionEvent& e) noexcept {
            return SDL_GetModState() & KMOD_ALT;
        }
    };

    // Traits for SDL_MouseButtonEvent
    template<>
    struct event_traits<SDL_MouseButtonEvent> {
        [[nodiscard]] static int mouse_x(const SDL_MouseButtonEvent& e) noexcept { return e.x; }
        [[nodiscard]] static int mouse_y(const SDL_MouseButtonEvent& e) noexcept { return e.y; }
        [[nodiscard]] static int mouse_button(const SDL_MouseButtonEvent& e) noexcept { return e.button; }
        [[nodiscard]] static bool is_button_press(const SDL_MouseButtonEvent& e) noexcept {
            return e.type == SDL_MOUSEBUTTONDOWN;
        }
        [[nodiscard]] static int click_count(const SDL_MouseButtonEvent& e) noexcept { return e.clicks; }
        [[nodiscard]] static uint32_t timestamp(const SDL_MouseButtonEvent& e) noexcept { return e.timestamp; }

        // SDL_MouseButtonEvent also doesn't have mod state in the event
        // Must use global state (SDL limitation)
        [[nodiscard]] static bool shift_pressed(const SDL_MouseButtonEvent&) noexcept {
            return SDL_GetModState() & KMOD_SHIFT;
        }
        [[nodiscard]] static bool ctrl_pressed(const SDL_MouseButtonEvent&) noexcept {
            return SDL_GetModState() & KMOD_CTRL;
        }
        [[nodiscard]] static bool alt_pressed(const SDL_MouseButtonEvent&) noexcept {
            return SDL_GetModState() & KMOD_ALT;
        }
    };

    // Traits for SDL_MouseWheelEvent
    template<>
    struct event_traits<SDL_MouseWheelEvent> {
        static float wheel_delta_x(const SDL_MouseWheelEvent& e) {
            return static_cast<float>(e.x);
        }
        static float wheel_delta_y(const SDL_MouseWheelEvent& e) {
            return static_cast<float>(e.y) * (e.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1);
        }
        static uint32_t timestamp(const SDL_MouseWheelEvent& e) { return e.timestamp; }
    };

    // Traits for SDL_KeyboardEvent
    template<>
    struct event_traits<SDL_KeyboardEvent> {
        [[nodiscard]] static int key_code(const SDL_KeyboardEvent& e) noexcept { return e.keysym.sym; }
        [[nodiscard]] static int scan_code(const SDL_KeyboardEvent& e) noexcept { return e.keysym.scancode; }
        [[nodiscard]] static bool is_key_press(const SDL_KeyboardEvent& e) noexcept {
            return e.type == SDL_KEYDOWN;
        }
        [[nodiscard]] static bool is_repeat(const SDL_KeyboardEvent& e) noexcept { return e.repeat != 0; }
        [[nodiscard]] static uint32_t timestamp(const SDL_KeyboardEvent& e) noexcept { return e.timestamp; }

        // Read modifiers from the event payload (keysym.mod)
        [[nodiscard]] static bool shift_pressed(const SDL_KeyboardEvent& e) noexcept {
            return (e.keysym.mod & KMOD_SHIFT) != 0;
        }
        [[nodiscard]] static bool ctrl_pressed(const SDL_KeyboardEvent& e) noexcept {
            return (e.keysym.mod & KMOD_CTRL) != 0;
        }
        [[nodiscard]] static bool alt_pressed(const SDL_KeyboardEvent& e) noexcept {
            return (e.keysym.mod & KMOD_ALT) != 0;
        }

        // Key identification methods
        [[nodiscard]] static bool is_tab_key(const SDL_KeyboardEvent& e) noexcept {
            return e.keysym.sym == SDLK_TAB;
        }
        [[nodiscard]] static bool is_enter_key(const SDL_KeyboardEvent& e) noexcept {
            return e.keysym.sym == SDLK_RETURN || e.keysym.sym == SDLK_KP_ENTER;
        }
        [[nodiscard]] static bool is_space_key(const SDL_KeyboardEvent& e) noexcept {
            return e.keysym.sym == SDLK_SPACE;
        }
        [[nodiscard]] static bool is_escape_key(const SDL_KeyboardEvent& e) noexcept {
            return e.keysym.sym == SDLK_ESCAPE;
        }
    };

    // Traits for SDL_TextInputEvent
    template<>
    struct event_traits<SDL_TextInputEvent> {
        static std::string_view text(const SDL_TextInputEvent& e) {
            return std::string_view(e.text);
        }
        static uint32_t timestamp(const SDL_TextInputEvent& e) { return e.timestamp; }
    };

    // Traits for SDL_WindowEvent
    template<>
    struct event_traits<SDL_WindowEvent> {
        static int window_width(const SDL_WindowEvent& e) {
            return (e.event == SDL_WINDOWEVENT_RESIZED) ? e.data1 : 0;
        }
        static int window_height(const SDL_WindowEvent& e) {
            return (e.event == SDL_WINDOWEVENT_RESIZED) ? e.data2 : 0;
        }
        static bool is_close(const SDL_WindowEvent& e) {
            return e.event == SDL_WINDOWEVENT_CLOSE;
        }
        static uint32_t timestamp(const SDL_WindowEvent& e) { return e.timestamp; }
    };

    // Generic traits for SDL_Event (dispatches to specific event type)
    template<>
    struct event_traits<SDL_Event> {
        static bool is_mouse_motion(const SDL_Event& e) {
            return e.type == SDL_MOUSEMOTION;
        }
        static bool is_mouse_button(const SDL_Event& e) {
            return e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP;
        }
        static bool is_mouse_wheel(const SDL_Event& e) {
            return e.type == SDL_MOUSEWHEEL;
        }
        static bool is_keyboard(const SDL_Event& e) {
            return e.type == SDL_KEYDOWN || e.type == SDL_KEYUP;
        }
        static bool is_text_input(const SDL_Event& e) {
            return e.type == SDL_TEXTINPUT;
        }
        static bool is_window(const SDL_Event& e) {
            return e.type == SDL_WINDOWEVENT;
        }
        static bool is_quit(const SDL_Event& e) {
            return e.type == SDL_QUIT;
        }

        static uint32_t timestamp(const SDL_Event& e) {
            return e.common.timestamp;
        }
    };
#endif

    // ======================================================================
    // GLFW Event Traits (Example)
    // ======================================================================

#ifdef ONYXUI_GLFW_BACKEND
    // GLFW uses callbacks, so we need a small wrapper to hold callback data
    struct glfw_mouse_event {
        GLFWwindow* window;
        double x, y;
        int button;
        int action;
        int mods;
    };

    struct glfw_key_event {
        GLFWwindow* window;
        int key;
        int scancode;
        int action;
        int mods;
    };

    template<>
    struct event_traits<glfw_mouse_event> {
        static int mouse_x(const glfw_mouse_event& e) {
            return static_cast<int>(e.x);
        }
        static int mouse_y(const glfw_mouse_event& e) {
            return static_cast<int>(e.y);
        }
        static int mouse_button(const glfw_mouse_event& e) { return e.button; }
        static bool is_button_press(const glfw_mouse_event& e) {
            return e.action == GLFW_PRESS;
        }

        static bool shift_pressed(const glfw_mouse_event& e) {
            return e.mods & GLFW_MOD_SHIFT;
        }
        static bool ctrl_pressed(const glfw_mouse_event& e) {
            return e.mods & GLFW_MOD_CONTROL;
        }
        static bool alt_pressed(const glfw_mouse_event& e) {
            return e.mods & GLFW_MOD_ALT;
        }
    };

    template<>
    struct event_traits<glfw_key_event> {
        static int key_code(const glfw_key_event& e) { return e.key; }
        static int scan_code(const glfw_key_event& e) { return e.scancode; }
        static bool is_key_press(const glfw_key_event& e) {
            return e.action == GLFW_PRESS;
        }
        static bool is_repeat(const glfw_key_event& e) {
            return e.action == GLFW_REPEAT;
        }

        static bool shift_pressed(const glfw_key_event& e) {
            return e.mods & GLFW_MOD_SHIFT;
        }
        static bool ctrl_pressed(const glfw_key_event& e) {
            return e.mods & GLFW_MOD_CONTROL;
        }
        static bool alt_pressed(const glfw_key_event& e) {
            return e.mods & GLFW_MOD_ALT;
        }

        // Key identification methods
        static bool is_tab_key(const glfw_key_event& e) {
            return e.key == GLFW_KEY_TAB;
        }
        static bool is_enter_key(const glfw_key_event& e) {
            return e.key == GLFW_KEY_ENTER || e.key == GLFW_KEY_KP_ENTER;
        }
        static bool is_space_key(const glfw_key_event& e) {
            return e.key == GLFW_KEY_SPACE;
        }
        static bool is_escape_key(const glfw_key_event& e) {
            return e.key == GLFW_KEY_ESCAPE;
        }
    };
#endif


    // ======================================================================
    // Event Type Detection
    // ======================================================================

    /**
     * @brief Type trait to check if a type is a supported event
     */
    template<typename T>
    struct is_event : std::false_type {};

#ifdef ONYXUI_SDL_BACKEND
    template<> struct is_event<SDL_Event> : std::true_type {};
    template<> struct is_event<SDL_MouseMotionEvent> : std::true_type {};
    template<> struct is_event<SDL_MouseButtonEvent> : std::true_type {};
    template<> struct is_event<SDL_MouseWheelEvent> : std::true_type {};
    template<> struct is_event<SDL_KeyboardEvent> : std::true_type {};
    template<> struct is_event<SDL_TextInputEvent> : std::true_type {};
    template<> struct is_event<SDL_WindowEvent> : std::true_type {};
#endif

#ifdef ONYXUI_GLFW_BACKEND
    template<> struct is_event<glfw_mouse_event> : std::true_type {};
    template<> struct is_event<glfw_key_event> : std::true_type {};
#endif

    template<typename T>
    inline constexpr bool is_event_v = is_event<T>::value;

    /**
     * @concept Event
     * @brief Concept for any supported event type
     */
    template<typename T>
    concept Event = is_event_v<T>;
}