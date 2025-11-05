//
// Created by igor on 14/10/2025.
//

#pragma once

#include <onyxui/concepts/rect_like.hh>
#include <onyxui/concepts/size_like.hh>
#include <string_view>
#include <vector>

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
     * - **Screenshot capability**: take_screenshot (save rendered content to stream)
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
     * ## take_screenshot() Rationale
     *
     * Screenshot capability allows capturing rendered content:
     * - **Testing**: Automated visual regression testing
     * - **Documentation**: Generate screenshots for manuals/tutorials
     * - **Debugging**: Save current UI state for analysis
     * - **User features**: In-app screenshot saving functionality
     * - **Recording**: Frame capture for video/GIF generation
     *
     * Output format is backend-specific:
     * - **TUI backends**: Text format (ANSI escape codes, plain text, or HTML)
     * - **GUI backends**: Image formats (PNG, BMP, etc.)
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
     *
     * @example Screenshot usage
     * @code
     * // Render UI
     * root->render(renderer);
     * renderer.present();
     *
     * // Save screenshot
     * std::ofstream file("screenshot.txt");
     * renderer.take_screenshot(file);
     * @endcode
     */
    template<typename T, typename R>
    concept RenderLike = RectLike<R> && requires(T renderer,
                                                  const R& rect,
                                                  const typename T::box_style& box,
                                                  const typename T::line_style& line,
                                                  const typename T::font& f,
                                                  const typename T::icon_style& i,
                                                  const typename T::background_style& bg,
                                                  const typename T::color_type& color,
                                                  const std::vector<R>& regions,
                                                  std::ostream& stream)
    {
        typename T::box_style;
        typename T::line_style;
        typename T::font;
        typename T::icon_style;
        typename T::background_style;  // Backend-specific background attributes
        typename T::color_type;  // Color type for stateless drawing
        typename T::size_type;  // Required for measure_text return type

        // Drawing methods (stateless - pass colors directly)
        { renderer.draw_box(rect, box, color, color) } -> std::same_as<void>;
        { renderer.draw_text(rect, "text", f, color, color) } -> std::same_as<void>;
        { renderer.draw_icon(rect, i, color, color) } -> std::same_as<void>;

        // Line drawing methods (stateless - pass colors directly)
        { renderer.draw_horizontal_line(rect, line, color, color) } -> std::same_as<void>;
        { renderer.draw_vertical_line(rect, line, color, color) } -> std::same_as<void>;

        // Background drawing methods (optimized for viewport clearing)
        { renderer.draw_background(rect, bg) } -> std::same_as<void>;
        { renderer.draw_background(rect, bg, regions) } -> std::same_as<void>;

        // Clearing method (for dirty rectangle support - stateless)
        { renderer.clear_region(rect, color) } -> std::same_as<void>;

        // Shadow drawing (for popup elements like menus, dialogs, tooltips)
        // Draws shadow margins to the right and bottom of widget_bounds
        { renderer.draw_shadow(rect, 1, 1) } -> std::same_as<void>;

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

        // Screenshot capability
        { renderer.take_screenshot(stream) } -> std::same_as<void>;

        // Static measurement methods (no instance needed)
        { T::measure_text(std::string_view{}, f) } -> SizeLike;
        { T::get_icon_size(i) } -> SizeLike;
    };
}
