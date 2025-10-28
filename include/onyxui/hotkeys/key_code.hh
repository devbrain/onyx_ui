/**
 * @file key_code.hh
 * @brief Universal keyboard key code enumeration
 * @author igor
 * @date 28/10/2025
 *
 * @details
 * Provides a unified enumeration for all keyboard keys, eliminating
 * magic numbers and providing type safety for key codes.
 *
 * ## Design Philosophy
 *
 * **Single Enum for All Keys:**
 * - ASCII characters (0-127): Use their actual ASCII values
 * - Special keys (256+): Named enum values outside ASCII range
 * - Function keys (300+): Sequential numbering for F1-F12
 * - Navigation keys (320+): Home, End, Page Up/Down, etc.
 *
 * **No Magic Numbers:**
 * Instead of `-1` for Up arrow or `10` for F10, use:
 * - `key_code::arrow_up`
 * - `key_code::f10`
 *
 * **Type Safety:**
 * The enum class prevents accidental integer conversions and makes
 * code self-documenting.
 *
 * ## Usage Examples
 *
 * ### With key_sequence
 * ```cpp
 * key_sequence save{key_code::s, key_modifier::ctrl};  // Ctrl+S
 * key_sequence menu{key_code::f10};                     // F10
 * key_sequence nav{key_code::arrow_down};               // Down arrow
 * ```
 *
 * ### ASCII Convenience
 * ```cpp
 * key_sequence{'s', key_modifier::ctrl};  // Still works! Converted to key_code
 * ```
 *
 * ### Comparison
 * ```cpp
 * if (pressed_key == key_code::enter) {
 *     // Handle Enter key
 * }
 * ```
 */

#pragma once

#include <cstdint>

namespace onyxui {

    /**
     * @enum key_code
     * @brief Universal key code for all keyboard keys
     *
     * @details
     * Values are organized in ranges:
     * - 0-127: ASCII characters (use lowercase for letters)
     * - 256-299: Arrow and special navigation keys
     * - 300-319: Function keys (F1-F12)
     * - 320-399: Extended navigation (Home, End, Page Up/Down, etc.)
     * - 400+: System keys (Windows, Menu, etc.)
     *
     * ## ASCII Range (0-127)
     * ASCII characters use their actual values:
     * - 'a'-'z' = 97-122 (lowercase)
     * - '0'-'9' = 48-57
     * - '\n' = 10 (Enter)
     * - '\t' = 9 (Tab)
     * - 27 = Escape
     *
     * Letters are normalized to lowercase in key_sequence constructor.
     *
     * ## Special Keys (256+)
     * Non-ASCII keys start at 256 to avoid conflicts with extended ASCII.
     */
    enum class key_code : int {
        // =====================================================================
        // Special value
        // =====================================================================
        none = 0,  ///< No key / uninitialized

        // =====================================================================
        // ASCII control characters (for convenience)
        // =====================================================================
        tab = '\t',        ///< Tab (ASCII 9)
        enter = '\n',      ///< Enter/Return (ASCII 10)
        escape = 27,       ///< Escape (ASCII 27)
        backspace = '\b',  ///< Backspace (ASCII 8)
        space = ' ',       ///< Space (ASCII 32)

        // =====================================================================
        // ASCII printable characters (examples, not exhaustive)
        // =====================================================================
        // Use actual char values: 'a', 'b', '0', '1', etc.
        // Letters a-z: 97-122 (lowercase)
        // Digits 0-9: 48-57
        // Symbols: '!', '@', '#', etc. (various ASCII codes)

        // Named aliases for common keys
        a = 'a', b = 'b', c = 'c', d = 'd', e = 'e', f = 'f', g = 'g', h = 'h',
        i = 'i', j = 'j', k = 'k', l = 'l', m = 'm', n = 'n', o = 'o', p = 'p',
        q = 'q', r = 'r', s = 's', t = 't', u = 'u', v = 'v', w = 'w', x = 'x',
        y = 'y', z = 'z',

        // =====================================================================
        // Arrow keys (256-259)
        // =====================================================================
        arrow_up = 256,     ///< Up arrow
        arrow_down = 257,   ///< Down arrow
        arrow_left = 258,   ///< Left arrow
        arrow_right = 259,  ///< Right arrow

        // =====================================================================
        // Function keys (300-311)
        // =====================================================================
        f1 = 300,   ///< F1 function key
        f2 = 301,   ///< F2 function key
        f3 = 302,   ///< F3 function key
        f4 = 303,   ///< F4 function key
        f5 = 304,   ///< F5 function key
        f6 = 305,   ///< F6 function key
        f7 = 306,   ///< F7 function key
        f8 = 307,   ///< F8 function key
        f9 = 308,   ///< F9 function key
        f10 = 309,  ///< F10 function key
        f11 = 310,  ///< F11 function key
        f12 = 311,  ///< F12 function key

        // =====================================================================
        // Extended navigation keys (320-329)
        // =====================================================================
        home = 320,       ///< Home key
        end = 321,        ///< End key
        page_up = 322,    ///< Page Up key
        page_down = 323,  ///< Page Down key
        insert = 324,     ///< Insert key
        delete_key = 325, ///< Delete key

        // =====================================================================
        // System keys (400+)
        // =====================================================================
        windows_key = 400,  ///< Windows key (Super/Meta on Linux)
        menu_key = 401,     ///< Menu/Application key
    };

    /**
     * @brief Check if key code is in ASCII range (printable + control)
     * @param code Key code to check
     * @return True if code is in range 0-127
     */
    [[nodiscard]] constexpr bool is_ascii(key_code code) noexcept {
        return static_cast<int>(code) >= 0 && static_cast<int>(code) <= 127;
    }

    /**
     * @brief Check if key code is a function key (F1-F12)
     * @param code Key code to check
     * @return True if code is F1-F12
     */
    [[nodiscard]] constexpr bool is_function_key(key_code code) noexcept {
        int const val = static_cast<int>(code);
        return val >= 300 && val <= 311;
    }

    /**
     * @brief Check if key code is an arrow key
     * @param code Key code to check
     * @return True if code is an arrow key
     */
    [[nodiscard]] constexpr bool is_arrow_key(key_code code) noexcept {
        int const val = static_cast<int>(code);
        return val >= 256 && val <= 259;
    }

    /**
     * @brief Check if key code is a navigation key (Home, End, Page Up/Down, etc.)
     * @param code Key code to check
     * @return True if code is a navigation key
     */
    [[nodiscard]] constexpr bool is_navigation_key(key_code code) noexcept {
        int const val = static_cast<int>(code);
        return val >= 320 && val <= 329;
    }

    /**
     * @brief Convert function key number (1-12) to key_code
     * @param f_num Function key number (1 for F1, 2 for F2, etc.)
     * @return Corresponding key_code, or key_code::none if out of range
     */
    [[nodiscard]] constexpr key_code function_key_from_number(int f_num) noexcept {
        if (f_num < 1 || f_num > 12) {
            return key_code::none;
        }
        return static_cast<key_code>(299 + f_num);  // F1=300, F2=301, etc.
    }

    /**
     * @brief Convert key_code to function key number (1-12)
     * @param code Key code to convert
     * @return Function key number (1-12), or 0 if not a function key
     */
    [[nodiscard]] constexpr int function_key_to_number(key_code code) noexcept {
        if (!is_function_key(code)) {
            return 0;
        }
        return static_cast<int>(code) - 299;  // F1=300 → 1, F2=301 → 2, etc.
    }

} // namespace onyxui
