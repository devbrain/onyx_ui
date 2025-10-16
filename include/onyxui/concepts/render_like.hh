//
// Created by igor on 14/10/2025.
//

#pragma once

#include <onyxui/concepts/rect_like.hh>
#include <onyxui/concepts/size_like.hh>
#include <string_view>

namespace onyxui {
    /**
     * @concept RenderLike
     * @brief Concept for renderer types that can draw UI elements
     *
     * @tparam T The renderer type
     * @tparam R The rectangle type (must satisfy RectLike)
     *
     * @details
     * A renderer must provide:
     * - **Type definitions**: box_style, font, icon_style, size_type
     * - **Drawing methods**: draw_box, draw_text, draw_icon
     * - **Text measurement**: measure_text (returns size_type with width/height)
     * - **Clipping methods**: push_clip, pop_clip, get_clip_rect
     *
     * ## measure_text() Rationale
     *
     * Text measurement is essential for:
     * - **Label auto-sizing**: Determining natural text dimensions
     * - **Button sizing**: Fitting text with padding
     * - **Multi-segment text**: Positioning underlined mnemonics (e.g., "F_ile")
     * - **Text wrapping**: Detecting when text exceeds available width
     * - **Alignment**: Centering/right-aligning requires knowing actual width
     * - **Ellipsis truncation**: "Long text..." needs measurement to know cut point
     *
     * Using `text.length()` is incorrect because it:
     * - Assumes fixed-width font (not true for proportional fonts)
     * - Counts bytes, not visual characters (wrong for UTF-8: "→" = 3 bytes, 1 char)
     * - Ignores kerning, ligatures, and font metrics
     *
     * @example Proper text measurement
     * @code
     * auto size = renderer.measure_text("Hello", font);
     * rect text_bounds{x, y, size.width, size.height};
     * renderer.draw_text(text_bounds, "Hello", font);
     * x += size.width;  // Correct advance
     * @endcode
     */
    template<typename T, typename R>
    concept RenderLike = RectLike<R> && requires(T renderer,
                                                  const R& rect,
                                                  const typename T::box_style& box,
                                                  const typename T::font& f,
                                                  const typename T::icon_style& i)
    {
        typename T::box_style;
        typename T::font;
        typename T::icon_style;
        typename T::size_type;  // Required for measure_text return type

        // Drawing methods
        { renderer.draw_box(rect, box) } -> std::same_as<void>;
        { renderer.draw_text(rect, "text", f) } -> std::same_as<void>;
        { renderer.draw_icon(rect, i) } -> std::same_as<void>;

        // Clipping methods
        { renderer.push_clip(rect) } -> std::same_as<void>;
        { renderer.pop_clip() } -> std::same_as<void>;
        { renderer.get_clip_rect() } -> std::same_as<R>;
    };
}
