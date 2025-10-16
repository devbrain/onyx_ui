/**
 * @file styled_text.hh
 * @brief Multi-font text rendering support for mnemonics
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Provides structures for rendering text with multiple font styles,
 * primarily used for mnemonic rendering (underlined characters in menu items).
 *
 * ## Design Philosophy
 *
 * **Backend-Specific Fonts:**
 * Unlike key_sequence which is backend-agnostic, styled_text uses the
 * backend's native font types. This allows each backend to define what
 * "underlined font" or "bold font" means in its own terms.
 *
 * **Theme-Controlled Styling:**
 * Fonts come from the theme system. The theme provides:
 * - `normal_font` - Regular text
 * - `mnemonic_font` - Underlined/emphasized text for mnemonics
 *
 * **Zero-Copy Text Segments:**
 * Each segment stores text by value to ensure lifetime safety when
 * passing styled_text around.
 *
 * ## Usage Examples
 *
 * ### Creating styled_text from plain text
 * ```cpp
 * using Backend = test_backend;
 * using Font = Backend::font_type;
 *
 * Font normal_font = theme.normal_font();
 * styled_text<Backend> text = make_plain_text<Backend>("Hello World", normal_font);
 *
 * // Renders as: Hello World (all same font)
 * ```
 *
 * ### Creating styled_text with multiple segments
 * ```cpp
 * Font normal_font = theme.normal_font();
 * Font mnemonic_font = theme.mnemonic_font();
 *
 * styled_text<Backend> text;
 * text.push_back({"S", mnemonic_font});  // Underlined S
 * text.push_back({"ave", normal_font});  // Normal text
 *
 * // Renders as: S̲ave (S underlined)
 * ```
 *
 * ### Parsing mnemonic syntax
 * ```cpp
 * // See mnemonic_parser.hh for parse_mnemonic()
 * auto [text, mnemonic_char] = parse_mnemonic<Backend>(
 *     "&Save",
 *     normal_font,
 *     mnemonic_font
 * );
 * // text = [{"S", mnemonic_font}, {"ave", normal_font}]
 * // mnemonic_char = 's'
 * ```
 *
 * ## Rendering
 *
 * Widgets render styled_text by iterating segments and calling
 * `renderer.draw_text()` for each segment with its specific font.
 *
 * @see mnemonic_parser.hh For parsing "&" syntax
 * @see theme.hh For mnemonic_font configuration
 */

#pragma once

#include <onyxui/concepts/backend.hh>
#include <string>
#include <vector>

namespace onyxui {

    /**
     * @struct text_segment
     * @brief A segment of text with an associated font
     *
     * @details
     * Represents a contiguous piece of text that should be rendered
     * with a specific font. Multiple segments combine to form styled_text.
     *
     * **Lifetime:**
     * - Text is stored by value (owns the string data)
     * - Font is copied (assumed to be a lightweight handle or POD type)
     *
     * **Backend Requirements:**
     * - Backend must define renderer_type with font member type
     * - Font type should be copyable and lightweight
     *
     * @tparam Backend The backend traits type satisfying UIBackend concept
     *
     * @example
     * @code
     * using Backend = test_backend;
     * using Font = Backend::renderer_type::font;
     *
     * Font my_font = get_font();
     * text_segment<Backend> segment{"Hello", my_font};
     *
     * // Later, render it:
     * renderer.draw_text(rect, segment.text, segment.font);
     * @endcode
     */
    template<UIBackend Backend>
    struct text_segment {
        std::string text;                             ///< The text content
        typename Backend::renderer_type::font font;   ///< The font to render with

        /**
         * @brief Construct a text segment
         *
         * @param t Text content
         * @param f Font for rendering
         *
         * @details
         * Text is moved if passed as rvalue, copied if lvalue.
         * Font is copied (assumed lightweight).
         */
        text_segment(std::string t, typename Backend::renderer_type::font f)
            : text(std::move(t))
            , font(f) {}

        /**
         * @brief Default constructor
         *
         * @details
         * Creates an empty text segment with default-constructed font.
         */
        text_segment() = default;
    };

    /**
     * @typedef styled_text
     * @brief A sequence of text segments with different fonts
     *
     * @details
     * Represents text with multiple font styles. Each segment has its own
     * text content and font, allowing mixed styles (normal, underlined, bold).
     *
     * **Usage:**
     * - Build manually: `text.push_back({"Hello", font})`
     * - Parse from mnemonic syntax: `parse_mnemonic("&Save", ...)`
     * - Create from plain text: `make_plain_text("Hello", font)`
     *
     * **Rendering:**
     * Iterate segments and call `renderer.draw_text()` for each,
     * tracking X position to concatenate segments horizontally.
     *
     * @tparam Backend The backend traits type satisfying UIBackend concept
     *
     * @see text_segment For individual segment details
     * @see make_plain_text For creating single-segment styled text
     */
    template<UIBackend Backend>
    using styled_text = std::vector<text_segment<Backend>>;

    /**
     * @brief Create styled_text from plain text with single font
     *
     * @param text The text content
     * @param font The font to use for all text
     * @return styled_text with single segment
     *
     * @details
     * Convenience function for creating styled_text without font variation.
     * Useful for rendering plain labels without mnemonics.
     *
     * **Exception Safety:**
     * - May throw std::bad_alloc if string allocation fails
     * - Strong guarantee: no state change on exception
     *
     * @example
     * @code
     * using Backend = test_backend;
     * Font normal_font = theme.normal_font();
     *
     * auto text = make_plain_text<Backend>("Save", normal_font);
     * // text = [{"Save", normal_font}]
     * @endcode
     *
     * @tparam Backend The backend traits type satisfying UIBackend concept
     */
    template<UIBackend Backend>
    styled_text<Backend> make_plain_text(
        std::string text,
        typename Backend::renderer_type::font font
    ) {
        styled_text<Backend> result;
        result.push_back(text_segment<Backend>{std::move(text), font});
        return result;
    }

    /**
     * @brief Get the total character count in styled_text
     *
     * @param text The styled_text to measure
     * @return Total number of characters across all segments
     *
     * @details
     * Useful for calculating text width or validating text content.
     *
     * **Complexity:** O(n) where n is number of segments
     *
     * **Exception Safety:** No-throw guarantee (noexcept)
     *
     * @example
     * @code
     * styled_text<Backend> text;
     * text.push_back({"Hel", font1});
     * text.push_back({"lo", font2});
     * CHECK(text_length(text) == 5);  // "Hel" + "lo" = 5
     * @endcode
     */
    template<UIBackend Backend>
    [[nodiscard]] std::size_t text_length(const styled_text<Backend>& text) noexcept {
        std::size_t total = 0;
        for (const auto& segment : text) {
            total += segment.text.length();
        }
        return total;
    }

    /**
     * @brief Get the plain text content without styling
     *
     * @param text The styled_text to flatten
     * @return Concatenated text from all segments
     *
     * @details
     * Removes all font information and concatenates segment text.
     * Useful for text searching, clipboard operations, or debugging.
     *
     * **Complexity:** O(n) where n is total character count
     *
     * **Exception Safety:**
     * - May throw std::bad_alloc if string allocation fails
     * - Strong guarantee: no state change on exception
     *
     * @example
     * @code
     * styled_text<Backend> text;
     * text.push_back({"S", mnemonic_font});
     * text.push_back({"ave", normal_font});
     * CHECK(to_plain_text(text) == "Save");
     * @endcode
     */
    template<UIBackend Backend>
    [[nodiscard]] std::string to_plain_text(const styled_text<Backend>& text) {
        std::string result;
        result.reserve(text_length(text));
        for (const auto& segment : text) {
            result += segment.text;
        }
        return result;
    }

    /**
     * @brief Check if styled_text is empty (no segments or all empty)
     *
     * @param text The styled_text to check
     * @return True if no segments or all segments are empty strings
     *
     * @details
     * A styled_text is considered empty if it has no segments,
     * or if all segments contain empty strings.
     *
     * **Complexity:** O(n) where n is number of segments
     *
     * **Exception Safety:** No-throw guarantee (noexcept)
     *
     * @example
     * @code
     * styled_text<Backend> text1;
     * CHECK(is_empty(text1) == true);  // No segments
     *
     * styled_text<Backend> text2;
     * text2.push_back({"", font});
     * CHECK(is_empty(text2) == true);  // Empty segment
     *
     * styled_text<Backend> text3;
     * text3.push_back({"Hi", font});
     * CHECK(is_empty(text3) == false);  // Has content
     * @endcode
     */
    template<UIBackend Backend>
    [[nodiscard]] bool is_empty(const styled_text<Backend>& text) noexcept {
        if (text.empty()) {
            return true;
        }
        for (const auto& segment : text) {
            if (!segment.text.empty()) {
                return false;
            }
        }
        return true;
    }

} // namespace onyxui
