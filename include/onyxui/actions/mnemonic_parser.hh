/**
 * @file mnemonic_parser.hh
 * @brief Parser for mnemonic syntax ("&" prefix for keyboard shortcuts)
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Provides parsing for classic mnemonic syntax used in UI frameworks like Qt,
 * Windows Forms, and classic DOS applications (QBasic, Norton Commander).
 *
 * ## Mnemonic Syntax
 *
 * **Single Ampersand (&):**
 * The character following "&" becomes the mnemonic (keyboard shortcut).
 * It is typically rendered with visual emphasis (underline, bold, color).
 *
 * Examples:
 * - "&File" → "File" with 'F' emphasized, mnemonic = 'f'
 * - "E&xit" → "Exit" with 'x' emphasized, mnemonic = 'x'
 * - "&Save" → "Save" with 'S' emphasized, mnemonic = 's'
 *
 * **Double Ampersand (&&):**
 * Escape sequence for literal ampersand character.
 *
 * Examples:
 * - "Save && Exit" → "Save & Exit" (literal &), no mnemonic
 * - "A&&B" → "A&B" (literal &), no mnemonic
 *
 * **Multiple Mnemonics:**
 * Only the FIRST "&" is treated as a mnemonic marker.
 * Subsequent "&" characters are treated as literals or escape sequences.
 *
 * Examples:
 * - "&File & Edit" → "File & Edit", mnemonic = 'f'
 * - "&A & &B" → "A & B", mnemonic = 'a' (first & only)
 *
 * **Case Normalization:**
 * Mnemonic characters are normalized to lowercase for consistency.
 * This matches key_sequence behavior (case-insensitive shortcuts).
 *
 * ## Design Philosophy
 *
 * **Why This Syntax?**
 * - Industry standard (Qt, Windows, classic DOS)
 * - Easy to type and read in source code
 * - No special editor required
 * - Translators familiar with the convention
 *
 * **Classic Examples:**
 * - QBasic: "&File" menu → press Alt+F
 * - Norton Commander: "&File" → press Alt+F
 * - Windows: "&Open" → press Alt+O
 *
 * ## Usage Examples
 *
 * ### Basic Parsing
 * ```cpp
 * using Backend = test_backend;
 * Font normal = theme.normal_font();
 * Font mnemonic = theme.mnemonic_font();
 *
 * auto result = parse_mnemonic<Backend>("&Save", normal, mnemonic);
 *
 * // result.text = [{"S", mnemonic}, {"ave", normal}]
 * // result.mnemonic_char = 's'
 * ```
 *
 * ### Escape Sequences
 * ```cpp
 * auto result = parse_mnemonic<Backend>("Save && Exit", normal, mnemonic);
 *
 * // result.text = [{"Save & Exit", normal}]
 * // result.mnemonic_char = '\0'
 * ```
 *
 * ### Menu Items
 * ```cpp
 * // File menu
 * menu.add_item(parse_mnemonic<Backend>("&File", normal, mnemonic));  // Alt+F
 * menu.add_item(parse_mnemonic<Backend>("&Edit", normal, mnemonic));  // Alt+E
 * menu.add_item(parse_mnemonic<Backend>("&View", normal, mnemonic));  // Alt+V
 *
 * // File submenu
 * file_menu.add_item(parse_mnemonic<Backend>("&New", normal, mnemonic));    // Alt+N
 * file_menu.add_item(parse_mnemonic<Backend>("&Open", normal, mnemonic));   // Alt+O
 * file_menu.add_item(parse_mnemonic<Backend>("&Save", normal, mnemonic));   // Alt+S
 * file_menu.add_item(parse_mnemonic<Backend>("E&xit", normal, mnemonic));   // Alt+X
 * ```
 *
 * ## Integration with Actions
 *
 * Mnemonics are separate from keyboard shortcuts:
 * - **Mnemonic:** Alt+key for menu navigation (local to menu/dialog)
 * - **Shortcut:** Global keyboard shortcut (e.g., Ctrl+S for Save)
 *
 * ```cpp
 * auto save_action = std::make_shared<action<Backend>>();
 *
 * // Mnemonic for menu navigation
 * auto mnemonic_text = parse_mnemonic<Backend>("&Save", normal, mnemonic);
 * save_action->set_text("Save");  // Plain text
 *
 * // Global shortcut
 * save_action->set_shortcut('s', key_modifier::ctrl);  // Ctrl+S
 *
 * // Menu displays: "Save    Ctrl+S" with 'S' underlined for Alt+S
 * ```
 *
 * @see styled_text.hh For text rendering structures
 * @see action.hh For keyboard shortcuts (different from mnemonics)
 */

#pragma once

#include <onyxui/widgets/styled_text.hh>
#include <onyxui/concepts/backend.hh>
#include <string_view>
#include <cctype>

namespace onyxui {

    /**
     * @struct mnemonic_info
     * @brief Result of parsing mnemonic syntax
     *
     * @details
     * Contains the parsed styled_text for rendering and the extracted
     * mnemonic character for keyboard handling.
     *
     * **Fields:**
     * - `text`: Styled text with mnemonic character emphasized
     * - `mnemonic_char`: The mnemonic key (lowercase ASCII), or '\0' if none
     *
     * **Lifetime:**
     * - Owns styled_text (safe to store/pass around)
     * - mnemonic_char is a plain char (trivially copyable)
     *
     * @tparam Backend The backend traits type satisfying UIBackend concept
     *
     * @see parse_mnemonic For creating mnemonic_info from text
     */
    template<UIBackend Backend>
    struct mnemonic_info {
        styled_text<Backend> text;  ///< Styled text for rendering
        char mnemonic_char;         ///< Mnemonic key ('\0' if none)

        /**
         * @brief Default constructor
         *
         * @details
         * Creates mnemonic_info with empty text and no mnemonic.
         */
        mnemonic_info()
            : text()
            , mnemonic_char('\0') {}

        /**
         * @brief Construct with text and mnemonic
         *
         * @param t Styled text
         * @param c Mnemonic character (should be lowercase)
         */
        mnemonic_info(styled_text<Backend> t, char c)
            : text(std::move(t))
            , mnemonic_char(c) {}
    };

    /**
     * @brief Parse mnemonic syntax and create styled text
     *
     * @param input Input text with optional "&" mnemonic markers
     * @param normal_font Font for regular text
     * @param mnemonic_font Font for emphasized mnemonic character
     * @return mnemonic_info containing styled text and mnemonic character
     *
     * @details
     * Parses text with mnemonic syntax:
     * - "&X" → X becomes mnemonic (rendered with mnemonic_font)
     * - "&&" → Literal "&" (escape sequence)
     * - Multiple "&" → Only first is mnemonic, rest are literal
     *
     * **Algorithm:**
     * 1. Scan for '&' characters
     * 2. First '&' followed by non-'&' → that char is mnemonic
     * 3. "&&" → replace with single '&'
     * 4. Build styled_text with appropriate fonts per segment
     * 5. Normalize mnemonic to lowercase
     *
     * **Edge Cases:**
     * - Empty string → empty text, no mnemonic
     * - "&" at end → treated as literal '&', no mnemonic
     * - No "&" → plain text, no mnemonic
     * - "&&" only → literal "&", no mnemonic
     *
     * **Exception Safety:**
     * - May throw std::bad_alloc if string allocation fails
     * - Strong guarantee: no state change on exception
     *
     * **Complexity:** O(n) where n is input length
     *
     * @example
     * @code
     * using Backend = test_backend;
     * Font normal = theme.normal_font();
     * Font mnemonic = theme.mnemonic_font();
     *
     * // Basic mnemonic
     * auto r1 = parse_mnemonic<Backend>("&Save", normal, mnemonic);
     * // r1.text = [{"S", mnemonic}, {"ave", normal}]
     * // r1.mnemonic_char = 's'
     *
     * // Mnemonic in middle
     * auto r2 = parse_mnemonic<Backend>("E&xit", normal, mnemonic);
     * // r2.text = [{"E", normal}, {"x", mnemonic}, {"it", normal}]
     * // r2.mnemonic_char = 'x'
     *
     * // Escape sequence
     * auto r3 = parse_mnemonic<Backend>("Save && Exit", normal, mnemonic);
     * // r3.text = [{"Save & Exit", normal}]
     * // r3.mnemonic_char = '\0'
     *
     * // No mnemonic
     * auto r4 = parse_mnemonic<Backend>("Plain Text", normal, mnemonic);
     * // r4.text = [{"Plain Text", normal}]
     * // r4.mnemonic_char = '\0'
     * @endcode
     *
     * @tparam Backend The backend traits type satisfying UIBackend concept
     */
    template<UIBackend Backend>
    mnemonic_info<Backend> parse_mnemonic(
        std::string_view input,
        typename Backend::renderer_type::font normal_font,
        typename Backend::renderer_type::font mnemonic_font
    ) {
        mnemonic_info<Backend> result;

        if (input.empty()) {
            return result;
        }

        std::string current_segment;
        bool found_mnemonic = false;

        for (std::size_t i = 0; i < input.length(); ++i) {
            char const c = input[i];

            if (c == '&') {
                // Check if this is an escape sequence "&&"
                if (i + 1 < input.length() && input[i + 1] == '&') {
                    // Escape sequence: "&&" → literal "&"
                    current_segment += '&';
                    ++i;  // Skip the second '&'
                } else if (!found_mnemonic && i + 1 < input.length()) {
                    // First mnemonic: "&X" → X is the mnemonic
                    char const mnemonic = input[i + 1];

                    // Flush current segment with normal font
                    if (!current_segment.empty()) {
                        result.text.push_back(text_segment<Backend>{
                            std::move(current_segment),
                            normal_font,
                            false  // Not a mnemonic
                        });
                        current_segment.clear();
                    }

                    // Add mnemonic character with mnemonic font
                    result.text.push_back(text_segment<Backend>{
                        std::string(1, mnemonic),
                        mnemonic_font,
                        true  // This IS a mnemonic
                    });

                    // Store mnemonic (normalized to lowercase)
                    result.mnemonic_char = static_cast<char>(
                        std::tolower(static_cast<unsigned char>(mnemonic))
                    );

                    found_mnemonic = true;
                    ++i;  // Skip the mnemonic character
                } else {
                    // Trailing '&' or subsequent '&' after mnemonic found
                    // Treat as literal
                    current_segment += '&';
                }
            } else {
                // Regular character
                current_segment += c;
            }
        }

        // Flush remaining segment
        if (!current_segment.empty()) {
            result.text.push_back(text_segment<Backend>{
                std::move(current_segment),
                normal_font,
                false  // Not a mnemonic
            });
        }

        // If no segments were added (shouldn't happen, but be safe)
        if (result.text.empty()) {
            result.text.push_back(text_segment<Backend>{
                std::string(input),
                normal_font,
                false  // Not a mnemonic
            });
        }

        return result;
    }

    /**
     * @brief Check if a string contains mnemonic syntax
     *
     * @param text Text to check
     * @return True if text contains "&" that isn't part of "&&"
     *
     * @details
     * Quick check to see if parse_mnemonic() would find a mnemonic.
     * Useful for optimization or validation.
     *
     * **Complexity:** O(n) where n is text length
     *
     * **Exception Safety:** No-throw guarantee (noexcept)
     *
     * @example
     * @code
     * CHECK(has_mnemonic("&Save") == true);
     * CHECK(has_mnemonic("Save && Exit") == false);  // Only escape
     * CHECK(has_mnemonic("Plain Text") == false);
     * CHECK(has_mnemonic("A&B&C") == true);  // Has mnemonic (first &)
     * @endcode
     */
    [[nodiscard]] inline bool has_mnemonic(std::string_view text) noexcept {
        for (std::size_t i = 0; i < text.length(); ++i) {
            if (text[i] == '&') {
                // Check if this is NOT an escape sequence
                if (i + 1 < text.length() && text[i + 1] != '&') {
                    return true;
                }
                // Skip escape sequence
                if (i + 1 < text.length() && text[i + 1] == '&') {
                    ++i;
                }
            }
        }
        return false;
    }

    /**
     * @brief Remove all mnemonic markers from text
     *
     * @param text Text with mnemonic syntax
     * @return Plain text with "&" markers removed
     *
     * @details
     * Strips mnemonic syntax to get plain text:
     * - "&X" → "X" (first occurrence only)
     * - "&&" → "&"
     * - Subsequent '&' after first mnemonic → kept as literal
     *
     * Useful for:
     * - Clipboard operations (copy plain text)
     * - Text searching
     * - Logging/debugging
     * - Accessibility (screen readers)
     *
     * **Complexity:** O(n) where n is text length
     *
     * **Exception Safety:**
     * - May throw std::bad_alloc if string allocation fails
     * - Strong guarantee: no state change on exception
     *
     * @example
     * @code
     * CHECK(strip_mnemonic("&Save") == "Save");
     * CHECK(strip_mnemonic("E&xit") == "Exit");
     * CHECK(strip_mnemonic("Save && Exit") == "Save & Exit");
     * CHECK(strip_mnemonic("&File &Edit") == "File &Edit");  // Only first & is mnemonic
     * CHECK(strip_mnemonic("Plain Text") == "Plain Text");
     * @endcode
     */
    [[nodiscard]] inline std::string strip_mnemonic(std::string_view text) {
        std::string result;
        result.reserve(text.length());

        bool found_mnemonic = false;

        for (std::size_t i = 0; i < text.length(); ++i) {
            char const c = text[i];

            if (c == '&') {
                // Check for escape sequence "&&"
                if (i + 1 < text.length() && text[i + 1] == '&') {
                    // Escape: "&&" → "&"
                    result += '&';
                    ++i;
                } else if (!found_mnemonic && i + 1 < text.length()) {
                    // First mnemonic: "&X" → skip the '&'
                    found_mnemonic = true;
                    // Don't add the '&', just continue to add the next character
                } else {
                    // Subsequent '&' after mnemonic found, or trailing '&'
                    result += '&';
                }
            } else {
                result += c;
            }
        }

        return result;
    }

} // namespace onyxui
