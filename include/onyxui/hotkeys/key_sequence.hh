/**
 * @file key_sequence.hh
 * @brief Backend-agnostic key sequence representation for hotkeys
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Provides a simple, backend-agnostic representation of keyboard shortcuts
 * using ASCII characters, F-keys, and modifier keys (Ctrl, Alt, Shift).
 *
 * ## Design Philosophy
 *
 * This system intentionally supports only the **common denominator** of keys
 * that exist across all platforms (DOS, Windows, Linux, terminal, GUI):
 * - ASCII keys: a-z, 0-9, and common punctuation
 * - Function keys: F1-F12
 * - Modifiers: Ctrl, Alt, Shift
 *
 * This matches classic UI patterns from:
 * - **QBasic**: Alt for menu bar
 * - **Norton Commander**: F9 for menu
 * - **Borland C**: Alt+Q to quit
 * - **Emacs/Vim**: Ctrl-based shortcuts
 *
 * ## Key Representation
 *
 * Keys are always normalized to lowercase internally:
 * - User presses 'S' or 's' → stored as 's'
 * - Shift is explicit via modifier flag
 * - Case-insensitive matching by default
 *
 * ## Usage Examples
 *
 * ### Basic Hotkeys
 * ```cpp
 * // Ctrl+S (Save)
 * key_sequence save_key{'s', key_modifier::ctrl};
 *
 * // Alt+Q (Quit - Borland C style)
 * key_sequence quit_key{'q', key_modifier::alt};
 *
 * // F9 (Menu - Norton Commander style)
 * key_sequence menu_key{9};  // F9
 * ```
 *
 * ### Modifier Combinations
 * ```cpp
 * // Ctrl+Shift+S (Save As)
 * key_sequence save_as{
 *     's',
 *     key_modifier::ctrl | key_modifier::shift
 * };
 *
 * // Alt+F4
 * key_sequence close_key{4, key_modifier::alt};
 * ```
 *
 * ### Comparison and Storage
 * ```cpp
 * std::map<key_sequence, std::shared_ptr<action>> hotkeys;
 * hotkeys[key_sequence{'s', key_modifier::ctrl}] = save_action;
 *
 * if (hotkeys.contains(pressed_key)) {
 *     hotkeys[pressed_key]->trigger();
 * }
 * ```
 *
 * ## Thread Safety
 * key_sequence is a simple POD type - thread-safe for read-only access.
 * Concurrent modification requires external synchronization.
 *
 * @see hotkey_manager For hotkey registration and dispatch
 * @see action::set_shortcut For associating shortcuts with actions
 */

#pragma once

#include <cstdint>
#include <compare>
#include <cctype>
#include <string>
#include <map>
#include <stdexcept>

namespace onyxui {

    /**
     * @enum key_modifier
     * @brief Keyboard modifier keys (Ctrl, Alt, Shift)
     *
     * @details
     * Modifiers are bitwise flags that can be combined:
     * - `ctrl | alt` = Ctrl+Alt
     * - `ctrl | shift` = Ctrl+Shift
     * - `ctrl | alt | shift` = Ctrl+Alt+Shift
     *
     * Use bitwise OR to combine, bitwise AND to test:
     * ```cpp
     * key_modifier mods = key_modifier::ctrl | key_modifier::shift;
     * if ((mods & key_modifier::ctrl) != key_modifier::none) {
     *     // Ctrl is pressed
     * }
     * ```
     */
    enum class key_modifier : uint8_t {
        none  = 0,         ///< No modifiers
        ctrl  = 1 << 0,    ///< Ctrl key (0x01)
        alt   = 1 << 1,    ///< Alt key (0x02)
        shift = 1 << 2     ///< Shift key (0x04)
    };

    /**
     * @brief Bitwise OR for combining modifiers
     * @relates key_modifier
     */
    constexpr key_modifier operator|(key_modifier lhs, key_modifier rhs) noexcept {
        return static_cast<key_modifier>(
            static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs)
        );
    }

    /**
     * @brief Bitwise AND for testing modifiers
     * @relates key_modifier
     */
    constexpr key_modifier operator&(key_modifier lhs, key_modifier rhs) noexcept {
        return static_cast<key_modifier>(
            static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs)
        );
    }

    /**
     * @brief Bitwise XOR for modifiers
     * @relates key_modifier
     */
    constexpr key_modifier operator^(key_modifier lhs, key_modifier rhs) noexcept {
        return static_cast<key_modifier>(
            static_cast<uint8_t>(lhs) ^ static_cast<uint8_t>(rhs)
        );
    }

    /**
     * @brief Bitwise NOT for modifiers
     * @relates key_modifier
     */
    constexpr key_modifier operator~(key_modifier mod) noexcept {
        return static_cast<key_modifier>(~static_cast<uint8_t>(mod));
    }

    /**
     * @brief Compound bitwise OR assignment
     * @relates key_modifier
     */
    constexpr key_modifier& operator|=(key_modifier& lhs, key_modifier rhs) noexcept {
        lhs = lhs | rhs;
        return lhs;
    }

    /**
     * @brief Compound bitwise AND assignment
     * @relates key_modifier
     */
    constexpr key_modifier& operator&=(key_modifier& lhs, key_modifier rhs) noexcept {
        lhs = lhs & rhs;
        return lhs;
    }

    /**
     * @brief Compound bitwise XOR assignment
     * @relates key_modifier
     */
    constexpr key_modifier& operator^=(key_modifier& lhs, key_modifier rhs) noexcept {
        lhs = lhs ^ rhs;
        return lhs;
    }

    /**
     * @struct key_sequence
     * @brief Represents a keyboard shortcut (key + modifiers)
     *
     * @details
     * A key_sequence uniquely identifies a keyboard shortcut by storing:
     * - Either an ASCII character (a-z, 0-9, etc.) OR an F-key number (1-12)
     * - Optional modifier keys (Ctrl, Alt, Shift)
     *
     * ## Key Normalization
     * - ASCII keys are normalized to lowercase ('A' → 'a')
     * - This makes shortcuts case-insensitive by default
     * - Use Shift modifier explicitly if case matters
     *
     * ## Constructors
     * ```cpp
     * key_sequence{'s', key_modifier::ctrl};      // Ctrl+S
     * key_sequence{'s'};                          // Just 's' (no modifiers)
     * key_sequence{9, key_modifier::none};        // F9
     * key_sequence{9};                            // F9 (implicit none)
     * ```
     *
     * ## Empty Sequence
     * Default-constructed key_sequence is empty (no key, no modifiers).
     * Use for "no hotkey assigned" state.
     *
     * ## Comparison
     * Supports full ordering via spaceship operator:
     * - Can be used in std::map, std::set
     * - Ordered first by key, then by f_key, then by modifiers
     *
     * @note Only one of `key` or `f_key` should be set (non-zero)
     */
    struct key_sequence {
        char key = '\0';                              ///< ASCII key (lowercase), or '\0' for F-key
        int f_key = 0;                                ///< F1-F12 (1-12), or 0 for ASCII key
        key_modifier modifiers = key_modifier::none; ///< Modifier keys (Ctrl, Alt, Shift)

        /**
         * @brief Default constructor - empty sequence
         */
        constexpr key_sequence() = default;

        /**
         * @brief Construct from ASCII key and modifiers
         *
         * @param k ASCII character (will be normalized to lowercase)
         * @param mods Modifier keys (default: none)
         *
         * @details
         * The key is automatically converted to lowercase for case-insensitive
         * matching. If you need case sensitivity, use the Shift modifier.
         *
         * @example
         * ```cpp
         * key_sequence save{'S', key_modifier::ctrl};  // Stored as 's'
         * key_sequence shift_s{'s', key_modifier::shift};  // Explicit uppercase
         * ```
         */
        constexpr key_sequence(char k, key_modifier mods = key_modifier::none) noexcept
            : key(static_cast<char>(std::tolower(static_cast<unsigned char>(k))))
            , f_key(0)
            , modifiers(mods) {}

        /**
         * @brief Construct from F-key number and modifiers
         *
         * @param f F-key number (1-12 for F1-F12)
         * @param mods Modifier keys (default: none)
         *
         * @details
         * Creates a function key shortcut. F-key numbers should be in range 1-12.
         * Values outside this range are allowed but may not be supported by all
         * backends.
         *
         * @example
         * ```cpp
         * key_sequence menu{9};                    // F9
         * key_sequence help{1};                    // F1
         * key_sequence alt_f4{4, key_modifier::alt};  // Alt+F4
         * ```
         */
        constexpr key_sequence(int f, key_modifier mods = key_modifier::none) noexcept
            : key('\0')
            , f_key(f)
            , modifiers(mods) {}

        /**
         * @brief Check if sequence is empty (no key assigned)
         *
         * @return True if no key or f_key is set
         *
         * @note noexcept - guaranteed not to throw
         */
        [[nodiscard]] constexpr bool empty() const noexcept {
            return key == '\0' && f_key == 0;
        }

        /**
         * @brief Check if this is an F-key sequence
         *
         * @return True if f_key is set (non-zero)
         *
         * @note noexcept - guaranteed not to throw
         */
        [[nodiscard]] constexpr bool is_f_key() const noexcept {
            return f_key != 0;
        }

        /**
         * @brief Check if this is an ASCII key sequence
         *
         * @return True if key is set (non-null)
         *
         * @note noexcept - guaranteed not to throw
         */
        [[nodiscard]] constexpr bool is_ascii_key() const noexcept {
            return key != '\0';
        }

        /**
         * @brief Check if Ctrl modifier is active
         *
         * @return True if Ctrl is in modifiers
         *
         * @note noexcept - guaranteed not to throw
         */
        [[nodiscard]] constexpr bool has_ctrl() const noexcept {
            return (modifiers & key_modifier::ctrl) != key_modifier::none;
        }

        /**
         * @brief Check if Alt modifier is active
         *
         * @return True if Alt is in modifiers
         *
         * @note noexcept - guaranteed not to throw
         */
        [[nodiscard]] constexpr bool has_alt() const noexcept {
            return (modifiers & key_modifier::alt) != key_modifier::none;
        }

        /**
         * @brief Check if Shift modifier is active
         *
         * @return True if Shift is in modifiers
         *
         * @note noexcept - guaranteed not to throw
         */
        [[nodiscard]] constexpr bool has_shift() const noexcept {
            return (modifiers & key_modifier::shift) != key_modifier::none;
        }

        /**
         * @brief Three-way comparison operator
         *
         * @details
         * Enables full ordering for use in std::map, std::set, etc.
         * Ordering: key < f_key < modifiers
         *
         * @note noexcept - guaranteed not to throw
         */
        constexpr auto operator<=>(const key_sequence&) const noexcept = default;

        /**
         * @brief Equality comparison
         *
         * @note noexcept - guaranteed not to throw
         */
        constexpr bool operator==(const key_sequence&) const noexcept = default;
    };

    /**
     * @brief Create a key sequence for Ctrl+key
     *
     * @param k ASCII character
     * @return key_sequence with Ctrl modifier
     *
     * @details
     * Convenience function for common Ctrl shortcuts.
     *
     * @example
     * ```cpp
     * auto save = make_ctrl_key('s');  // Ctrl+S
     * auto copy = make_ctrl_key('c');  // Ctrl+C
     * ```
     */
    constexpr key_sequence make_ctrl_key(char k) noexcept {
        return key_sequence{k, key_modifier::ctrl};
    }

    /**
     * @brief Create a key sequence for Alt+key
     *
     * @param k ASCII character
     * @return key_sequence with Alt modifier
     *
     * @details
     * Convenience function for common Alt shortcuts (mnemonics).
     *
     * @example
     * ```cpp
     * auto file_menu = make_alt_key('f');  // Alt+F
     * auto quit = make_alt_key('q');       // Alt+Q (Borland C style)
     * ```
     */
    constexpr key_sequence make_alt_key(char k) noexcept {
        return key_sequence{k, key_modifier::alt};
    }

    /**
     * @brief Create a key sequence for Shift+key
     *
     * @param k ASCII character
     * @return key_sequence with Shift modifier
     *
     * @details
     * Convenience function for Shift shortcuts.
     *
     * @example
     * ```cpp
     * auto save_as = make_shift_key('s');  // Shift+S
     * ```
     */
    constexpr key_sequence make_shift_key(char k) noexcept {
        return key_sequence{k, key_modifier::shift};
    }

    /**
     * @brief Create a key sequence for F-key
     *
     * @param f_num F-key number (1-12)
     * @return key_sequence for F-key
     *
     * @details
     * Convenience function for function keys.
     *
     * @example
     * ```cpp
     * auto help = make_f_key(1);   // F1
     * auto menu = make_f_key(9);   // F9 (Norton Commander)
     * ```
     */
    constexpr key_sequence make_f_key(int f_num) noexcept {
        return key_sequence{f_num, key_modifier::none};
    }

    // ================================================================
    // String Parsing and Formatting
    // ================================================================

    namespace detail {
        // Special key name to code mapping
        inline const std::map<std::string, int> special_keys = {
            {"Up", -1}, {"Down", -2}, {"Left", -3}, {"Right", -4},
            {"Enter", '\n'}, {"Escape", 27}, {"Tab", '\t'},
            {"Space", ' '}, {"Backspace", '\b'},
        };
    }

    /**
     * @brief Parse key sequence from string
     * @param str String like "Ctrl+S", "F10", "Down"
     * @return Parsed key sequence
     * @throws std::runtime_error if format is invalid
     *
     * @details
     * Supported formats:
     * - Modifiers: "Ctrl+", "Alt+", "Shift+"
     * - F-keys: "F1" through "F12"
     * - Special keys: "Up", "Down", "Left", "Right", "Enter", "Escape", "Tab"
     * - Single characters: "a", "s", "A" (case insensitive)
     *
     * @example
     * @code
     * auto key = parse_key_sequence("Ctrl+S");    // Ctrl+S
     * auto key = parse_key_sequence("F10");        // F10
     * auto key = parse_key_sequence("Down");       // Down arrow
     * auto key = parse_key_sequence("Shift+Tab");  // Shift+Tab
     * @endcode
     */
    inline key_sequence parse_key_sequence(const std::string& str) {
        key_modifier mods = key_modifier::none;
        std::string remaining = str;

        // Parse modifiers
        auto parse_modifier = [&](const std::string& prefix, key_modifier mod) {
            if (remaining.starts_with(prefix)) {
                mods = mods | mod;
                remaining = remaining.substr(prefix.length());
                return true;
            }
            return false;
        };

        while (parse_modifier("Ctrl+", key_modifier::ctrl) ||
               parse_modifier("Alt+", key_modifier::alt) ||
               parse_modifier("Shift+", key_modifier::shift)) {}

        if (remaining.empty()) {
            throw std::runtime_error("Empty key sequence");
        }

        // F-keys: "F1" through "F12"
        if (remaining[0] == 'F' && remaining.length() >= 2) {
            try {
                int f_num = std::stoi(remaining.substr(1));
                if (f_num < 1 || f_num > 12) {
                    throw std::runtime_error("F-key out of range (1-12): " + remaining);
                }
                return key_sequence{f_num, mods};
            } catch (const std::exception&) {
                throw std::runtime_error("Invalid F-key format: " + remaining);
            }
        }

        // Special keys
        auto it = detail::special_keys.find(remaining);
        if (it != detail::special_keys.end()) {
            return key_sequence{static_cast<char>(it->second), mods};
        }

        // Single character
        if (remaining.length() == 1) {
            char ch = static_cast<char>(std::tolower(static_cast<unsigned char>(remaining[0])));
            return key_sequence{ch, mods};
        }

        throw std::runtime_error("Invalid key sequence: " + str);
    }

    /**
     * @brief Format key sequence as string
     * @param seq Key sequence to format
     * @return String like "Ctrl+S" or "F10"
     *
     * @example
     * @code
     * key_sequence seq{'s', key_modifier::ctrl};
     * std::string str = format_key_sequence(seq);  // "Ctrl+S"
     * @endcode
     */
    inline std::string format_key_sequence(const key_sequence& seq) {
        std::string result;

        // Add modifiers
        if ((seq.modifiers & key_modifier::ctrl) != key_modifier::none) {
            result += "Ctrl+";
        }
        if ((seq.modifiers & key_modifier::alt) != key_modifier::none) {
            result += "Alt+";
        }
        if ((seq.modifiers & key_modifier::shift) != key_modifier::none) {
            result += "Shift+";
        }

        // Add key
        if (seq.f_key != 0) {
            result += "F" + std::to_string(seq.f_key);
        } else {
            // Check if special key
            for (const auto& [name, code] : detail::special_keys) {
                if (static_cast<int>(seq.key) == code) {
                    result += name;
                    return result;
                }
            }
            // Regular character
            result += static_cast<char>(std::toupper(static_cast<unsigned char>(seq.key)));
        }

        return result;
    }

} // namespace onyxui
