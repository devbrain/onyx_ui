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
     * - **Type definitions**: box_style, line_style, font, icon_style, size_type
     * - **Drawing methods**: draw_box, draw_text, draw_icon, draw_horizontal_line, draw_vertical_line
     * - **Static measurement methods**: measure_text, get_icon_size (no instance needed)
     * - **Clipping methods**: push_clip, pop_clip, get_clip_rect
     * - **Viewport management**: get_viewport (returns drawable area bounds)
     * - **Presentation**: present (display rendered content)
     * - **Resize handling**: on_resize (update buffers on window resize)
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
     * ## get_icon_size() Rationale
     *
     * Icon size measurement allows widgets to layout icons correctly:
     * - **Menu items**: Icon + text + shortcut alignment
     * - **Buttons**: Icon + text combinations
     * - **Toolbar items**: Icon-based buttons
     * - **Status indicators**: Fixed-size icon placement
     *
     * Icon size is backend-specific:
     * - **TUI**: Typically 1x1 characters (single emoji/glyph)
     * - **GUI**: Typically 16x16, 24x24, or 32x32 pixels
     * - **Theme-dependent**: May vary based on current theme/DPI
     *
     * ## get_viewport() Rationale
     *
     * The viewport defines the drawable area for the renderer:
     * - **Auto-layout**: ui_handle::display() needs viewport to measure/arrange UI
     * - **Fullscreen rendering**: Viewport = entire window/screen
     * - **Sub-region rendering**: Viewport = portion of screen (e.g., HUD area)
     * - **Multi-renderer**: Different viewports for different UI panels
     *
     * @example Proper text measurement
     * @code
     * auto size = Renderer::measure_text("Hello", font);  // Static method
     * rect text_bounds{x, y, size.width, size.height};
     * renderer.draw_text(text_bounds, "Hello", font);
     * x += size.width;  // Correct advance
     * @endcode
     *
     * @example Icon size query
     * @code
     * auto icon_size = Renderer::get_icon_size(icon_style);  // Static method
     * rect icon_bounds{x, y, icon_size.width, icon_size.height};
     * renderer.draw_icon(icon_bounds, icon_style);
     * @endcode
     *
     * @example Viewport usage
     * @code
     * auto viewport = renderer.get_viewport();
     * root->measure(viewport.w, viewport.h);
     * root->arrange(viewport);
     * root->render(renderer);
     * @endcode
     */
    template<typename T, typename R>
    concept RenderLike = RectLike<R> && requires(T renderer,
                                                  const R& rect,
                                                  const typename T::box_style& box,
                                                  const typename T::line_style& line,
                                                  const typename T::font& f,
                                                  const typename T::icon_style& i)
    {
        typename T::box_style;
        typename T::line_style;
        typename T::font;
        typename T::icon_style;
        typename T::size_type;  // Required for measure_text return type

        // Drawing methods
        { renderer.draw_box(rect, box) } -> std::same_as<void>;
        { renderer.draw_text(rect, "text", f) } -> std::same_as<void>;
        { renderer.draw_icon(rect, i) } -> std::same_as<void>;

        // Line drawing methods
        { renderer.draw_horizontal_line(rect, line) } -> std::same_as<void>;
        { renderer.draw_vertical_line(rect, line) } -> std::same_as<void>;

        // Clearing method (for dirty rectangle support)
        { renderer.clear_region(rect) } -> std::same_as<void>;

        // Clipping methods
        { renderer.push_clip(rect) } -> std::same_as<void>;
        { renderer.pop_clip() } -> std::same_as<void>;
        { renderer.get_clip_rect() } -> std::same_as<R>;

        // Viewport management
        { renderer.get_viewport() } -> std::same_as<R>;

        // Presentation method
        { renderer.present() } -> std::same_as<void>;

        // Resize handling
        { renderer.on_resize() } -> std::same_as<void>;

        // Static measurement methods (no instance needed)
        { T::measure_text(std::string_view{}, f) } -> SizeLike;
        { T::get_icon_size(i) } -> SizeLike;
    };
}
