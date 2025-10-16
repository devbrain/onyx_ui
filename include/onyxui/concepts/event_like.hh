/**
 * @file events.hh
 * @brief Event traits and concepts for backend-agnostic event handling
 * @author igor
 * @date 10/10/2025
 *
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
     *
     * @details
     * Basic keyboard event support - provides key code and press state.
     * For hotkey support, also implement HotkeyCapable concept.
     */
    template<typename T>
    concept KeyboardEvent = requires(const T& e) {
        { event_traits<T>::key_code(e) } -> std::convertible_to<int>;
        { event_traits<T>::is_key_press(e) } -> std::convertible_to<bool>;
        { event_traits<T>::is_repeat(e) } -> std::convertible_to<bool>;
    };

    /**
     * @concept HotkeyCapable
     * @brief Concept for keyboard events that support hotkey conversion
     *
     * @details
     * Extends KeyboardEvent with methods to convert backend-specific key codes
     * to backend-agnostic ASCII characters or F-key numbers.
     *
     * **Backend Implementation Requirements:**
     * - `to_ascii()`: Convert key to lowercase ASCII ('a'-'z', '0'-'9', punctuation)
     *   - Return '\0' for non-ASCII keys (arrows, Home, End, etc.)
     *   - Always return lowercase ('A' → 'a', 'S' → 's')
     * - `to_f_key()`: Convert key to F-key number (1-12 for F1-F12)
     *   - Return 0 for non-F-keys
     *
     * **Why This Design:**
     * This simple mapping allows backend-agnostic hotkeys using only the common
     * denominator of keys across all platforms (DOS, Windows, Linux, terminals).
     *
     * @example Backend Implementation
     * ```cpp
     * // SDL backend example
     * template<>
     * struct event_traits<SDL_KeyboardEvent> {
     *     static char to_ascii(const SDL_KeyboardEvent& e) {
     *         SDL_Keycode key = e.keysym.sym;
     *         if (key >= SDLK_a && key <= SDLK_z) return static_cast<char>(key);
     *         if (key >= SDLK_0 && key <= SDLK_9) return static_cast<char>(key);
     *         // ... handle punctuation ...
     *         return '\0';  // Not an ASCII key
     *     }
     *
     *     static int to_f_key(const SDL_KeyboardEvent& e) {
     *         SDL_Keycode key = e.keysym.sym;
     *         if (key >= SDLK_F1 && key <= SDLK_F12) {
     *             return (key - SDLK_F1) + 1;
     *         }
     *         return 0;  // Not an F-key
     *     }
     * };
     * ```
     */
    template<typename T>
    concept HotkeyCapable = KeyboardEvent<T> && requires(const T& e) {
        { event_traits<T>::to_ascii(e) } -> std::convertible_to<char>;
        { event_traits<T>::to_f_key(e) } -> std::convertible_to<int>;
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
    // Event Type Detection
    // ======================================================================

    /**
     * @brief Type trait to check if a type is a supported event
     */
    template<typename T>
    struct is_event : std::false_type {};

    template<typename T>
    inline constexpr bool is_event_v = is_event<T>::value;

    /**
     * @concept Event
     * @brief Concept for any supported event type
     */
    template<typename T>
    concept Event = is_event_v<T>;
}