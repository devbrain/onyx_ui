/**
 * @file sdlpp_renderer.hh
 * @brief SDL++ renderer implementing RenderLike concept
 *
 * Uses lib_sdlpp's built-in:
 * - sdlpp::renderer for hardware-accelerated 2D rendering
 * - sdlpp::font::font for text measurement and rendering
 * - sdlpp::image for icon/texture loading
 */

#pragma once

#include <cstdint>
#include <string_view>
#include <memory>
#include <vector>
#include <filesystem>
#include <ostream>

#include <onyxui/sdlpp/geometry.hh>
#include <onyxui/sdlpp/colors.hh>

// Forward declarations
namespace sdlpp {
    class renderer;
    class texture;
}
namespace onyx_image {
    class memory_surface;
}

namespace onyxui::sdlpp {

/**
 * @class sdlpp_renderer
 * @brief Hardware-accelerated 2D renderer using lib_sdlpp
 *
 * Implements the RenderLike concept for graphical UI rendering.
 * Uses lib_sdlpp's built-in font and image support.
 */
class sdlpp_renderer {
public:
    // =================================================================
    // Required Types for RenderLike Concept
    // =================================================================

    enum class border_style_type : std::uint8_t {
        none,
        flat,
        raised,
        sunken,
        etched,
        thick_raised,
        thick_sunken
    };

    struct box_style {
        border_style_type style = border_style_type::none;
        bool is_solid = true;

        constexpr box_style() noexcept = default;
        constexpr explicit box_style(border_style_type s, bool solid = true) noexcept
            : style(s), is_solid(solid) {}
        constexpr bool operator==(const box_style&) const noexcept = default;
    };

    struct line_style {
        border_style_type style = border_style_type::flat;
        int thickness = 1;

        constexpr line_style() noexcept = default;
        constexpr explicit line_style(border_style_type s, int t = 1) noexcept
            : style(s), thickness(t) {}
        constexpr bool operator==(const line_style&) const noexcept = default;
    };

    /**
     * @struct font
     * @brief Font specification for text rendering
     *
     * lib_sdlpp's font system supports:
     * - TTF fonts (TrueType)
     * - Windows FON fonts
     * - BGI vector fonts
     * - Raw BIOS font dumps
     */
    struct font {
        std::filesystem::path path;     ///< TTF file path (used when no provider resolves the spec)
        std::string font_set;           ///< Named set key for font_source_provider (e.g. "bios")
        std::string font_name;           ///< Sub-name within the set
        float size_px = 16.0f;          ///< Size in pixels
        bool bold = false;
        bool italic = false;
        bool underline = false;
        bool strikethrough = false;

        [[nodiscard]] std::size_t hash() const noexcept;
    };

    enum class icon_style : std::uint8_t {
        none,
        check, cross, bullet, folder, file,
        arrow_up, arrow_down, arrow_left, arrow_right,
        menu, minimize, maximize, restore, close_x,
        cursor_insert, cursor_overwrite,
        checkbox_unchecked, checkbox_checked, checkbox_indeterminate,
        radio_unchecked, radio_checked,
        progress_filled, progress_empty,
        slider_filled, slider_empty, slider_thumb
    };

    struct background_style {
        color bg_color;
        bool use_gradient = false;
        color gradient_end;

        constexpr background_style() noexcept = default;
        constexpr explicit background_style(const color& c) noexcept
            : bg_color(c) {}
        constexpr bool operator==(const background_style&) const noexcept = default;
    };

    using size_type = size;
    using color_type = color;

    // =================================================================
    // Construction / Destruction
    // =================================================================

    /**
     * @brief Construct renderer from lib_sdlpp renderer
     * @param sdl_renderer Reference to sdlpp::renderer
     * @param assets_path Path to assets directory (win311/)
     */
    explicit sdlpp_renderer(::sdlpp::renderer& sdl_renderer,
                            const std::filesystem::path& assets_path = {});

    virtual ~sdlpp_renderer();

    sdlpp_renderer(const sdlpp_renderer&) = delete;
    sdlpp_renderer& operator=(const sdlpp_renderer&) = delete;
    sdlpp_renderer(sdlpp_renderer&&) noexcept;
    sdlpp_renderer& operator=(sdlpp_renderer&&) noexcept;

    // =================================================================
    // Drawing Methods (RenderLike Concept)
    // =================================================================

    void draw_box(const rect& r, const box_style& style,
                  const color& fg, const color& bg);

    virtual void draw_text(const rect& r, std::string_view text,
                           const font& f, const color& fg, const color& bg);

    void draw_icon(const rect& r, icon_style style,
                   const color& fg, const color& bg);

    void draw_horizontal_line(const rect& r, const line_style& style,
                              const color& fg, const color& bg);

    void draw_vertical_line(const rect& r, const line_style& style,
                            const color& fg, const color& bg);

    void draw_background(const rect& viewport, const background_style& style);

    void draw_background(const rect& viewport, const background_style& style,
                         const std::vector<rect>& dirty_regions);

    void clear_region(const rect& r, const color& bg);

    void draw_shadow(const rect& widget_bounds, int offset_x, int offset_y);

    // =================================================================
    // Image / Texture (onyx_image-based)
    // =================================================================

    /**
     * @brief Blit an onyx_image::memory_surface stretched to `dst`.
     *
     * Cache semantics (per-renderer):
     *   - First call uploads the pixel data as an SDL texture created
     *     against this renderer. Subsequent calls with the same surface
     *     address reuse that texture.
     *   - Cache lives on this renderer instance; destroying the renderer
     *     (or calling `invalidate_image` / `clear_image_cache()`)
     *     releases the GPU textures.
     *
     * **Stable-identity contract** (caller's responsibility):
     *   - The caller must keep the surface alive as long as they want
     *     the cache hit, and must NOT mutate its pixels after the first
     *     draw. If either condition breaks, call `invalidate_image` to
     *     force re-upload on the next draw.
     *   - Do not reuse the same surface address for different content —
     *     the cache key is the pointer, so a destroyed-then-recreated
     *     surface at the same address will display the old texture.
     *
     * Supported pixel formats: rgba8888 (alpha-blended), rgb888
     * (opaque), indexed8 (palette expanded inline).
     *
     * The signature takes `memory_surface` explicitly rather than the
     * abstract `surface` base so the contract is honest: only surfaces
     * that expose a CPU pixel buffer are uploadable.
     */
    void draw_image(const rect& dst, const onyx_image::memory_surface& img);

    /**
     * @brief Drop the cached GPU texture for `img` (if any).
     *
     * Call before mutating the surface's pixels or before destroying it
     * to avoid the "stale content via reused address" pitfall.
     */
    void invalidate_image(const onyx_image::memory_surface& img);

    /** @brief Drop every cached image texture on this renderer. */
    void clear_image_cache();

    /**
     * @brief Blit a caller-owned SDL texture stretched to `dst`.
     *
     * No caching — the caller is responsible for the texture's
     * lifetime (e.g. a pre-rendered game-thumbnail atlas). Use this
     * over `draw_image` when you want explicit control over upload
     * timing, mutation, and lifetime.
     */
    void draw_texture(const rect& dst, ::sdlpp::texture& tex);

    // =================================================================
    // Clipping Methods
    // =================================================================

    void push_clip(const rect& r);
    void pop_clip();
    [[nodiscard]] rect get_clip_rect() const;

    // =================================================================
    // Viewport and Presentation
    // =================================================================

    [[nodiscard]] rect get_viewport() const;
    void present();
    void on_resize();
    void take_screenshot(std::ostream& sink) const;

    // =================================================================
    // Static Measurement Methods
    // =================================================================

    /**
     * @brief Measure text dimensions using lib_sdlpp's font system
     * @param text UTF-8 text to measure
     * @param f Font specification
     * @return Size in pixels
     */
    [[nodiscard]] static size measure_text(std::string_view text, const font& f);

    [[nodiscard]] static size get_icon_size(icon_style icon) noexcept;

    [[nodiscard]] static constexpr int get_border_thickness(
        const box_style& style) noexcept
    {
        switch (style.style) {
            case border_style_type::none: return 0;
            case border_style_type::flat: return 1;
            case border_style_type::raised:
            case border_style_type::sunken:
            case border_style_type::etched: return 2;
            case border_style_type::thick_raised:
            case border_style_type::thick_sunken: return 3;
        }
        return 0;
    }

    /**
     * @brief Release all cached fonts before SDL shutdown
     *
     * Must be called before SDL init object goes out of scope to prevent
     * SIGSEGV from fonts being destroyed after FreeType is shut down.
     */
    static void shutdown();

private:
    struct impl;
    std::unique_ptr<impl> m_pimpl;
};

} // namespace onyxui::sdlpp
