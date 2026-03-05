/**
 * @file tile_renderer.hh
 * @brief Tile-based renderer extending sdlpp_renderer
 *
 * This renderer extends sdlpp_renderer with tile rendering capabilities:
 * - draw_tile: Single tile at position
 * - draw_h_slice: Horizontal 3-part scalable element
 * - draw_v_slice: Vertical 3-part scalable element
 * - draw_nine_slice: 9-part fully scalable element
 * - draw_bitmap_text: Pre-rendered bitmap font text
 *
 * Inherits all vector drawing capabilities from sdlpp_renderer:
 * - draw_box, draw_text (TTF), draw_icon, draw_line, etc.
 * - Clipping, viewport management
 * - TTF font support
 */

#pragma once

#include <onyxui/sdlpp/sdlpp_renderer.hh>
#include <onyxui/tile/tile_types.hh>
#include <onyxui/tile/tile_theme.hh>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string_view>
#include <vector>

// Forward declaration - user provides SDL renderer
namespace sdlpp {
    class renderer;
    class texture;
}

namespace onyxui::tile {

/**
 * @class tile_renderer
 * @brief Renderer for tile/sprite-based UI elements, extending sdlpp_renderer
 *
 * This class extends sdlpp_renderer with tile drawing primitives that render
 * tiles from a texture atlas. It's designed for pixel-art game UIs where
 * visual elements come from pre-made sprites.
 *
 * **Inherited capabilities (from sdlpp_renderer):**
 * - TTF font rendering (draw_text with TTF fonts)
 * - Vector drawing (boxes, lines, icons)
 * - Clipping stack
 * - Viewport management
 *
 * **Tile-specific capabilities:**
 * - Tile/nine-slice rendering from atlas
 * - Bitmap font text rendering
 *
 * Usage:
 * @code
 * tile_renderer renderer(sdl_renderer);
 * renderer.set_atlas(&my_atlas);
 *
 * // Draw single tile
 * renderer.draw_tile(42, {10, 20});
 *
 * // Draw nine-slice panel
 * renderer.draw_nine_slice(panel_slice, {0, 0, 200, 100});
 *
 * // Draw bitmap text
 * renderer.draw_bitmap_text("Hello!", {10, 10}, my_font);
 *
 * // Use inherited TTF font rendering
 * renderer.draw_text(rect{0,0,100,20}, "Vector text", ttf_font, fg, bg);
 * @endcode
 */
class tile_renderer : public onyxui::sdlpp::sdlpp_renderer {
public:
    using base = onyxui::sdlpp::sdlpp_renderer;

    // =========================================================================
    // Tile-specific types (for bitmap fonts and tile icons)
    // =========================================================================

    /**
     * @brief Font type for tile backend - wraps bitmap_font pointer
     *
     * Standard widgets use renderer_type::font for text operations.
     * For tile backend, this wraps a pointer to the bitmap_font to use.
     *
     * Note: This is separate from the inherited sdlpp_renderer::font (TTF fonts).
     * Use tile_font for bitmap fonts, inherited font for TTF.
     *
     * Example:
     * @code
     * tile_renderer::tile_font my_font{&theme.font_normal};
     * auto text_size = tile_renderer::measure_bitmap_text("Hello", my_font);
     * @endcode
     */
    struct tile_font {
        const bitmap_font* bitmap{nullptr};  ///< Pointer to bitmap font (not owned)

        tile_font() = default;
        explicit tile_font(const bitmap_font* f) : bitmap(f) {}
        explicit tile_font(const bitmap_font& f) : bitmap(&f) {}
    };

    /**
     * @brief Icon style for tile backend - uses tile ID
     *
     * For tile backend, icons are tiles from the atlas.
     * This is separate from the inherited icon_style enum.
     */
    struct tile_icon_style {
        int tile_id{-1};       ///< Tile ID for the icon (-1 = none)
        int width{16};         ///< Icon width in pixels
        int height{16};        ///< Icon height in pixels

        tile_icon_style() = default;
        tile_icon_style(int id, int w = 16, int h = 16) : tile_id(id), width(w), height(h) {}
    };

    // =========================================================================
    // Construction
    // =========================================================================

    /**
     * @brief Construct tile renderer with SDL renderer
     * @param sdl_renderer SDL renderer for drawing operations
     * @param assets_path Optional assets path for TTF fonts (inherited)
     */
    explicit tile_renderer(::sdlpp::renderer& sdl_renderer,
                          const std::filesystem::path& assets_path = {});

    ~tile_renderer();

    tile_renderer(const tile_renderer&) = delete;
    tile_renderer& operator=(const tile_renderer&) = delete;
    tile_renderer(tile_renderer&&) noexcept;
    tile_renderer& operator=(tile_renderer&&) noexcept;

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * @brief Set the tile atlas to use for drawing
     * @param atlas Pointer to tile atlas (user-owned, must outlive renderer)
     */
    void set_atlas(tile_atlas* atlas) noexcept;

    /**
     * @brief Get the current atlas
     * @return Pointer to current atlas, or nullptr if not set
     */
    [[nodiscard]] tile_atlas* get_atlas() const noexcept;

    /**
     * @brief Set the SDL texture for the atlas
     * @param texture SDL texture containing atlas image
     *
     * This must be called after set_atlas() to provide the actual texture.
     * The texture must match the atlas dimensions.
     */
    void set_texture(::sdlpp::texture* texture) noexcept;

    // =========================================================================
    // Tile Drawing Primitives
    // =========================================================================

    // Use int-based point/rect for tile operations (simpler than inherited types)
    struct tile_point {
        int x{0};
        int y{0};
    };

    struct tile_rect {
        int x{0};
        int y{0};
        int width{0};
        int height{0};
    };

    /**
     * @brief Draw a single tile at the specified position
     * @param tile_id Tile index in the atlas
     * @param pos Position to draw (top-left corner)
     *
     * The tile is drawn at its natural size (atlas tile_width x tile_height).
     * If tile_id is -1, nothing is drawn.
     */
    void draw_tile(int tile_id, tile_point pos);

    /**
     * @brief Draw a single tile scaled to fit bounds
     * @param tile_id Tile index in the atlas
     * @param bounds Destination rectangle
     *
     * The tile is stretched/scaled to fill the bounds.
     * If tile_id is -1, nothing is drawn.
     */
    void draw_tile_scaled(int tile_id, tile_rect bounds);

    /**
     * @brief Draw a horizontal 3-part slice
     * @param slice Slice configuration (left, center, right tiles)
     * @param bounds Destination rectangle
     */
    void draw_h_slice(const h_slice& slice, tile_rect bounds);

    /**
     * @brief Draw a vertical 3-part slice
     * @param slice Slice configuration (top, center, bottom tiles)
     * @param bounds Destination rectangle
     */
    void draw_v_slice(const v_slice& slice, tile_rect bounds);

    /**
     * @brief Draw a nine-slice scalable element
     * @param slice Nine-slice configuration
     * @param bounds Destination rectangle
     */
    void draw_nine_slice(const nine_slice& slice, tile_rect bounds);

    // =========================================================================
    // Bitmap Text Drawing
    // =========================================================================

    /**
     * @brief Draw text using a bitmap font
     * @param text Text string to draw
     * @param pos Position of first character (top-left)
     * @param font Bitmap font configuration
     *
     * Note: This version uses the font's baked-in colors. For themed text,
     * use the overload with a color parameter.
     */
    void draw_bitmap_text(std::string_view text, tile_point pos, const bitmap_font& font);

    /**
     * @brief Draw text using a bitmap font with color modulation
     * @param text Text string to draw
     * @param pos Position of first character (top-left)
     * @param font Bitmap font configuration
     * @param color Color to modulate the text (applied via SDL_SetTextureColorMod)
     *
     * The color is applied as a multiplicative modulation to the source texture.
     * For best results, use a white (255, 255, 255) source font in the atlas,
     * which allows any target color to be achieved.
     */
    void draw_bitmap_text(std::string_view text, tile_point pos, const bitmap_font& font,
                          const onyxui::sdlpp::color& color);

    /**
     * @brief Draw text centered within bounds
     * @param text Text string to draw
     * @param bounds Rectangle to center within
     * @param font Bitmap font configuration
     */
    void draw_bitmap_text_centered(std::string_view text, tile_rect bounds, const bitmap_font& font);

    /**
     * @brief Draw text centered within bounds with color modulation
     * @param text Text string to draw
     * @param bounds Rectangle to center within
     * @param font Bitmap font configuration
     * @param color Color to modulate the text
     */
    void draw_bitmap_text_centered(std::string_view text, tile_rect bounds, const bitmap_font& font,
                                   const onyxui::sdlpp::color& color);

    // =========================================================================
    // Static Measurement Methods (override base class for bitmap font support)
    // =========================================================================

    /**
     * @brief Measure text using global bitmap font if available, TTF otherwise
     * @param text UTF-8 text to measure
     * @param f Font specification (used for TTF fallback)
     * @return Size in pixels
     *
     * This static method checks if a global tile_renderer is set (via set_renderer)
     * and if it has a default bitmap font. If so, uses bitmap font measurement.
     * Otherwise falls back to base class TTF measurement.
     *
     * This allows standard widgets to get correct text measurements when using
     * the tile backend with bitmap fonts.
     */
    [[nodiscard]] static onyxui::sdlpp::size measure_text(
        std::string_view text, const font& f);

    /**
     * @brief Measure text using bitmap font
     * @param text Text to measure
     * @param f Font containing bitmap_font pointer
     * @return Size of rendered text, or {0,0} if font has no bitmap
     */
    [[nodiscard]] static onyxui::sdlpp::size measure_bitmap_text(
        std::string_view text, const tile_font& f)
    {
        if (!f.bitmap || !f.bitmap->is_valid()) {
            return onyxui::sdlpp::size{0, 0};
        }
        auto [w, h] = f.bitmap->text_size(text);
        return onyxui::sdlpp::size{w, h};
    }

    /**
     * @brief Get tile icon size
     * @param style Tile icon style containing dimensions
     * @return Icon size
     */
    [[nodiscard]] static onyxui::sdlpp::size get_tile_icon_size(const tile_icon_style& style) {
        return onyxui::sdlpp::size{style.width, style.height};
    }

    // =========================================================================
    // Tile-specific fill methods
    // =========================================================================

    /**
     * @brief Fill a rectangle with a solid color
     * @param r Rectangle to fill
     * @param c Color to fill with
     */
    void fill_rect(tile_rect r, onyxui::sdlpp::color c);

    // =========================================================================
    // Bitmap Font Integration (override base class text rendering)
    // =========================================================================

    /**
     * @brief Set the default bitmap font for standard widget text rendering
     * @param font Pointer to bitmap font (nullptr to use TTF fallback)
     *
     * When set, draw_text() will use this bitmap font instead of TTF fonts.
     * This allows standard OnyxUI widgets to render with bitmap fonts.
     */
    void set_default_bitmap_font(const bitmap_font* font) noexcept;

    /**
     * @brief Get the default bitmap font
     * @return Pointer to default bitmap font, or nullptr if using TTF
     */
    [[nodiscard]] const bitmap_font* get_default_bitmap_font() const noexcept;

    /**
     * @brief Draw text using bitmap font if available, TTF otherwise
     * @param r Bounding rectangle
     * @param text UTF-8 text to draw
     * @param f Font specification (used for TTF fallback)
     * @param fg Foreground color
     * @param bg Background color
     *
     * If a default bitmap font is set, renders using that.
     * Otherwise falls back to base class TTF rendering.
     */
    void draw_text(const onyxui::sdlpp::rect& r, std::string_view text,
                   const font& f, const onyxui::sdlpp::color& fg,
                   const onyxui::sdlpp::color& bg) override;

    /**
     * @brief Measure text using bitmap font if available, TTF otherwise
     * @param text UTF-8 text to measure
     * @param f Font specification (used for TTF fallback)
     * @return Size in pixels
     */
    [[nodiscard]] onyxui::sdlpp::size measure_text_with_bitmap(
        std::string_view text, const font& f) const;

private:
    struct tile_impl;
    std::unique_ptr<tile_impl> m_tile_pimpl;
};

} // namespace onyxui::tile
