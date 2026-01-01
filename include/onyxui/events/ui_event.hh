/**
 * @file ui_event.hh
 * @brief Framework-level event structures (backend-agnostic)
 * @author igor
 * @date 28/10/2025
 *
 * @defgroup events Event System
 * @brief Unified event handling with std::variant
 *
 * ## Overview
 *
 * The event system uses a single `ui_event` variant to represent all event types.
 * Backends convert their native events to framework-level structures, normalizing
 * platform-specific quirks during conversion.
 *
 * ## Design Philosophy
 *
 * - **Single conversion**: Backend converts once (not on every query)
 * - **Type safety**: std::variant ensures all cases handled at compile time
 * - **Zero overhead**: No allocations, minimal copying
 * - **Self-documenting**: Event structures are explicit and readable
 *
 * ## Usage
 *
 * @code
 * // Backend converts native event
 * std::optional<ui_event> evt = Backend::create_event(native);
 * if (!evt) return;  // Unknown event
 *
 * // Dispatch using std::get_if
 * if (auto* kbd = std::get_if<keyboard_event>(&evt.value())) {
 *     handle_keyboard(*kbd);
 * }
 * @endcode
 */

#pragma once

#include <variant>
#include <cstdint>
#include <optional>
#include <onyxui/hotkeys/key_code.hh>
#include <onyxui/hotkeys/key_sequence.hh>
#include <onyxui/core/geometry.hh>

namespace onyxui {

    /**
     * @brief Framework-level keyboard event (backend-agnostic)
     * @ingroup events
     *
     * @details
     * Simplified keyboard event using unified key_code enum. Backends convert their
     * native events directly to key_code, eliminating the need for a separate conversion layer.
     *
     * ## Backend Responsibilities
     *
     * Backends must:
     * - Map native key codes to key_code enum values
     * - Normalize alphabetic characters to lowercase (use shift modifier for uppercase)
     * - Remove platform quirks (e.g., Terminal Enter=Ctrl+M → modifiers.ctrl=false)
     * - Set correct key_code for all key types (ASCII, function keys, arrows, navigation)
     *
     * ## Key Code Categories
     *
     * - **ASCII characters**: key_code::a through key_code::z, digits, punctuation
     * - **Control keys**: key_code::enter, key_code::tab, key_code::escape, key_code::space
     * - **Function keys**: key_code::f1 through key_code::f12
     * - **Arrow keys**: key_code::arrow_up/down/left/right
     * - **Navigation keys**: key_code::home, key_code::end, key_code::page_up, etc.
     * - **System keys**: key_code::windows_key, key_code::menu_key
     *
     * @example Plain Enter Key
     * @code
     * keyboard_event {
     *     .key = key_code::enter,
     *     .modifiers = key_modifier::none,
     *     .pressed = true
     * }
     * @endcode
     *
     * @example Ctrl+S Hotkey
     * @code
     * keyboard_event {
     *     .key = key_code::s,  // Lowercase normalized
     *     .modifiers = key_modifier::ctrl,
     *     .pressed = true
     * }
     * @endcode
     *
     * @example F10 Function Key
     * @code
     * keyboard_event {
     *     .key = key_code::f10,
     *     .modifiers = key_modifier::none,
     *     .pressed = true
     * }
     * @endcode
     *
     * @example Arrow Down Key
     * @code
     * keyboard_event {
     *     .key = key_code::arrow_down,
     *     .modifiers = key_modifier::none,
     *     .pressed = true
     * }
     * @endcode
     *
     * @example Convert to key_sequence (trivial)
     * @code
     * keyboard_event evt = get_event();
     * key_sequence seq{evt.key, evt.modifiers};  // Direct construction!
     * @endcode
     */
    struct keyboard_event {
        key_code key = key_code::none;              ///< Unified key code (no magic numbers!)
        key_modifier modifiers = key_modifier::none; ///< Keyboard modifiers (normalized by backend)
        bool pressed = true;                         ///< true = key down, false = key up
    };

    /**
     * @brief Framework-level mouse event (backend-agnostic)
     * @ingroup events
     *
     * @details
     * Mouse events are coordinate-based with optional button and action information.
     * Coordinates are relative to the window/viewport origin (top-left = 0,0).
     *
     * ## Button Mapping
     *
     * - **left**: Primary button (typically left mouse button)
     * - **right**: Secondary button (typically right mouse button, context menu)
     * - **middle**: Tertiary button (typically middle mouse button or wheel click)
     * - **none**: No button (used for move events, wheel events)
     *
     * ## Action Types
     *
     * - **press**: Button pressed down
     * - **release**: Button released (may not identify which button on some platforms)
     * - **move**: Mouse moved (no button state change)
     * - **wheel_up**: Mouse wheel scrolled up
     * - **wheel_down**: Mouse wheel scrolled down
     *
     * @example Left Button Click
     * @code
     * // User clicks left mouse button at (10, 20) logical units
     * mouse_event {
     *     .x = 10.0_lu,
     *     .y = 20.0_lu,
     *     .btn = button::left,
     *     .act = action::press,
     *     .modifiers = { .ctrl = false, .alt = false, .shift = false }
     * }
     * @endcode
     *
     * @example Mouse Wheel Scroll
     * @code
     * // User scrolls wheel up at (50, 60) logical units while holding Ctrl
     * mouse_event {
     *     .x = 50.0_lu,
     *     .y = 60.0_lu,
     *     .btn = button::none,
     *     .act = action::wheel_up,
     *     .modifiers = { .ctrl = true, .alt = false, .shift = false }
     * }
     * @endcode
     *
     * @example Mouse Move
     * @code
     * // Mouse moved to (100, 150) logical units without any button pressed
     * mouse_event {
     *     .x = 100.0_lu,
     *     .y = 150.0_lu,
     *     .btn = button::none,
     *     .act = action::move,
     *     .modifiers = { .ctrl = false, .alt = false, .shift = false }
     * }
     * @endcode
     */
    struct mouse_event {
        /**
         * @brief Mouse button identifier
         */
        enum class button : uint8_t {
            none,    ///< No button (used for move, wheel events)
            left,    ///< Primary button (left)
            right,   ///< Secondary button (right, context menu)
            middle   ///< Tertiary button (middle, wheel click)
        };

        /**
         * @brief Mouse action type
         */
        enum class action : uint8_t {
            press,      ///< Button pressed down
            release,    ///< Button released (may not identify which button)
            move,       ///< Mouse moved (no button change)
            wheel_up,   ///< Wheel scrolled up
            wheel_down  ///< Wheel scrolled down
        };

        logical_unit x;  ///< Mouse X coordinate in logical units (relative to viewport, top-left = 0)
        logical_unit y;  ///< Mouse Y coordinate in logical units (relative to viewport, top-left = 0)
        button btn;     ///< Button involved (none for move/wheel)
        action act;     ///< Action type

        /**
         * @brief Keyboard modifiers during mouse event
         *
         * @details
         * Indicates which modifier keys were held during the mouse action.
         * Useful for Ctrl+Click, Shift+Click, etc.
         */
        struct {
            bool ctrl : 1;   ///< Ctrl key held during mouse action
            bool alt : 1;    ///< Alt key held during mouse action
            bool shift : 1;  ///< Shift key held during mouse action
        } modifiers;
    };

    /**
     * @brief Framework-level resize event (backend-agnostic)
     * @ingroup events
     *
     * @details
     * Resize events indicate the window/viewport has changed dimensions.
     * Framework will automatically trigger layout reflow (measure/arrange).
     *
     * Units depend on backend:
     * - **GUI backends**: pixels
     * - **Terminal backends**: character cells
     *
     * @example Window Resized
     * @code
     * // Window resized to 1024x768 pixels
     * resize_event {
     *     .width = 1024,
     *     .height = 768
     * }
     * @endcode
     *
     * @example Terminal Resized
     * @code
     * // Terminal resized to 80x24 character cells
     * resize_event {
     *     .width = 80,
     *     .height = 24
     * }
     * @endcode
     */
    struct resize_event {
        int width;   ///< New window/viewport width (pixels or cells)
        int height;  ///< New window/viewport height (pixels or cells)
    };

    /**
     * @brief Unified UI event (variant of all event types)
     * @ingroup events
     *
     * @details
     * Uses std::variant to represent any of the three event types.
     * No redundant type enum - variant tracks the active alternative.
     *
     * ## Usage Patterns
     *
     * **Pattern 1: std::get_if (simplest, recommended)**
     * @code
     * std::optional<ui_event> evt = Backend::create_event(native);
     * if (!evt) return;
     *
     * if (auto* kbd = std::get_if<keyboard_event>(&evt.value())) {
     *     handle_keyboard(*kbd);
     * } else if (auto* mouse = std::get_if<mouse_event>(&evt.value())) {
     *     handle_mouse(*mouse);
     * } else if (auto* resize = std::get_if<resize_event>(&evt.value())) {
     *     handle_resize(*resize);
     * }
     * @endcode
     *
     * **Pattern 2: std::visit with if constexpr (elegant)**
     * @code
     * std::optional<ui_event> evt = Backend::create_event(native);
     * if (!evt) return;
     *
     * std::visit([](auto&& e) {
     *     using T = std::decay_t<decltype(e)>;
     *     if constexpr (std::is_same_v<T, keyboard_event>) {
     *         handle_keyboard(e);
     *     } else if constexpr (std::is_same_v<T, mouse_event>) {
     *         handle_mouse(e);
     *     } else if constexpr (std::is_same_v<T, resize_event>) {
     *         handle_resize(e);
     *     }
     * }, evt.value());
     * @endcode
     *
     * **Pattern 3: std::visit with overloaded (most elegant, C++17)**
     * @code
     * // Helper for overload pattern
     * template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
     * template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
     *
     * std::optional<ui_event> evt = Backend::create_event(native);
     * if (!evt) return;
     *
     * std::visit(overloaded{
     *     [](const keyboard_event& kbd) { handle_keyboard(kbd); },
     *     [](const mouse_event& mouse) { handle_mouse(mouse); },
     *     [](const resize_event& resize) { handle_resize(resize); }
     * }, evt.value());
     * @endcode
     *
     * **Pattern 4: std::holds_alternative (type checking)**
     * @code
     * std::optional<ui_event> evt = Backend::create_event(native);
     * if (!evt) return;
     *
     * if (std::holds_alternative<keyboard_event>(evt.value())) {
     *     auto& kbd = std::get<keyboard_event>(evt.value());
     *     handle_keyboard(kbd);
     * }
     * @endcode
     *
     * @see keyboard_event
     * @see mouse_event
     * @see resize_event
     */
    using ui_event = std::variant<keyboard_event, mouse_event, resize_event>;

} // namespace onyxui
