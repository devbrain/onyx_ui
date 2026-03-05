/**
 * @file tile_renderer.cc
 * @brief Implementation of tile-based renderer extending sdlpp_renderer
 */

#include <onyxui/tile/tile_renderer.hh>
#include <onyxui/tile/tile_backend.hh>  // For get_renderer()

#include <sdlpp/video/renderer.hh>
#include <sdlpp/video/texture.hh>

#include <vector>
#include <algorithm>
#include <cassert>
#include <cstdio>

// ============================================================================
// Debug Logging for Tile Rendering Issues
// ============================================================================

#ifdef ONYXUI_TILE_DEBUG
    #define TILE_DEBUG_LOG(fmt, ...) \
        std::fprintf(stderr, "[tile_renderer] " fmt "\n", ##__VA_ARGS__)
#else
    #define TILE_DEBUG_LOG(fmt, ...) ((void)0)
#endif

#ifndef NDEBUG
    #define TILE_WARN(fmt, ...) \
        std::fprintf(stderr, "[tile_renderer WARNING] " fmt "\n", ##__VA_ARGS__)
#else
    #define TILE_WARN(fmt, ...) ((void)0)
#endif

namespace onyxui::tile {

// ============================================================================
// Tile-specific Implementation (separate from inherited sdlpp_renderer::impl)
// ============================================================================

struct tile_renderer::tile_impl {
    ::sdlpp::renderer* sdl_renderer{nullptr};
    tile_atlas* atlas{nullptr};
    ::sdlpp::texture* texture{nullptr};
    const bitmap_font* default_bitmap_font{nullptr};  // For standard widget text rendering

    explicit tile_impl(::sdlpp::renderer& renderer)
        : sdl_renderer(&renderer)
    {}

    // Helper to convert tile_rect to SDL rect
    [[nodiscard]] SDL_FRect to_sdl_frect(const tile_rect& r) const noexcept {
        return SDL_FRect{
            static_cast<float>(r.x),
            static_cast<float>(r.y),
            static_cast<float>(r.width),
            static_cast<float>(r.height)
        };
    }

    // Helper to get source rect for a tile
    [[nodiscard]] SDL_FRect get_source_rect(int tile_id) const noexcept {
        if (!atlas || tile_id < 0) {
            return SDL_FRect{0, 0, 0, 0};
        }
        return SDL_FRect{
            static_cast<float>(atlas->source_x(tile_id)),
            static_cast<float>(atlas->source_y(tile_id)),
            static_cast<float>(atlas->tile_width),
            static_cast<float>(atlas->tile_height)
        };
    }

    // Draw a tile to a destination rect
    void render_tile(int tile_id, const tile_rect& dest) {
        // Validation
        if (!atlas) {
            TILE_WARN("render_tile called without atlas");
            return;
        }

        if (!texture) {
            TILE_WARN("render_tile called without texture");
            return;
        }

        if (tile_id < 0) {
            TILE_DEBUG_LOG("Skipping invalid tile_id=%d", tile_id);
            return;
        }

        if (!validate_texture_type(atlas->tex_type, texture_type::sdlpp)) {
            TILE_WARN("Texture type mismatch");
            return;
        }

        if (atlas->texture != texture) {
            TILE_WARN("atlas->texture does not match renderer texture");
            return;
        }

        // Render
        SDL_FRect src = get_source_rect(tile_id);
        SDL_FRect dst = to_sdl_frect(dest);
        SDL_RenderTexture(sdl_renderer->get(), texture->get(), &src, &dst);
    }
};

// ============================================================================
// Construction / Destruction
// ============================================================================

tile_renderer::tile_renderer(::sdlpp::renderer& sdl_renderer,
                             const std::filesystem::path& assets_path)
    : base(sdl_renderer, assets_path)  // Initialize base class
    , m_tile_pimpl(std::make_unique<tile_impl>(sdl_renderer))
{}

tile_renderer::~tile_renderer() = default;

tile_renderer::tile_renderer(tile_renderer&& other) noexcept
    : base(std::move(other))
    , m_tile_pimpl(std::move(other.m_tile_pimpl))
{}

tile_renderer& tile_renderer::operator=(tile_renderer&& other) noexcept {
    if (this != &other) {
        base::operator=(std::move(other));
        m_tile_pimpl = std::move(other.m_tile_pimpl);
    }
    return *this;
}

// ============================================================================
// Configuration
// ============================================================================

void tile_renderer::set_atlas(tile_atlas* atlas) noexcept {
    m_tile_pimpl->atlas = atlas;
}

tile_atlas* tile_renderer::get_atlas() const noexcept {
    return m_tile_pimpl->atlas;
}

void tile_renderer::set_texture(::sdlpp::texture* texture) noexcept {
    m_tile_pimpl->texture = texture;
}

// ============================================================================
// Single Tile Drawing
// ============================================================================

void tile_renderer::draw_tile(int tile_id, tile_point pos) {
    if (!m_tile_pimpl->atlas || tile_id < 0) {
        return;
    }

    tile_rect dest{
        pos.x,
        pos.y,
        m_tile_pimpl->atlas->tile_width,
        m_tile_pimpl->atlas->tile_height
    };

    m_tile_pimpl->render_tile(tile_id, dest);
}

void tile_renderer::draw_tile_scaled(int tile_id, tile_rect bounds) {
    m_tile_pimpl->render_tile(tile_id, bounds);
}

// ============================================================================
// Slice Drawing
// ============================================================================

void tile_renderer::draw_h_slice(const h_slice& slice, tile_rect bounds) {
    if (!slice.is_valid() || !m_tile_pimpl->atlas) {
        return;
    }

    const int margin = slice.margin;

    // Left cap
    if (slice.left >= 0 && margin > 0) {
        m_tile_pimpl->render_tile(slice.left, {bounds.x, bounds.y, margin, bounds.height});
    }

    // Right cap
    if (slice.right >= 0 && margin > 0) {
        tile_rect right_dest{bounds.x + bounds.width - margin, bounds.y, margin, bounds.height};
        m_tile_pimpl->render_tile(slice.right, right_dest);
    }

    // Center (stretched)
    if (slice.center >= 0) {
        int center_x = bounds.x + margin;
        int center_width = bounds.width - 2 * margin;
        if (center_width > 0) {
            tile_rect center_dest{center_x, bounds.y, center_width, bounds.height};
            m_tile_pimpl->render_tile(slice.center, center_dest);
        }
    }
}

void tile_renderer::draw_v_slice(const v_slice& slice, tile_rect bounds) {
    if (!slice.is_valid() || !m_tile_pimpl->atlas) {
        return;
    }

    const int margin = slice.margin;

    // Top cap
    if (slice.top >= 0 && margin > 0) {
        m_tile_pimpl->render_tile(slice.top, {bounds.x, bounds.y, bounds.width, margin});
    }

    // Bottom cap
    if (slice.bottom >= 0 && margin > 0) {
        m_tile_pimpl->render_tile(slice.bottom, {bounds.x, bounds.y + bounds.height - margin, bounds.width, margin});
    }

    // Center (stretched)
    if (slice.center >= 0) {
        int center_y = bounds.y + margin;
        int center_height = bounds.height - 2 * margin;
        if (center_height > 0) {
            m_tile_pimpl->render_tile(slice.center, {bounds.x, center_y, bounds.width, center_height});
        }
    }
}

void tile_renderer::draw_nine_slice(const nine_slice& slice, tile_rect bounds) {
    if (!slice.is_valid() || !m_tile_pimpl->atlas) {
        return;
    }

    const int mh = slice.margin_h;
    const int mv = slice.margin_v;

    const int left = bounds.x;
    const int right = bounds.x + bounds.width - mh;
    const int top = bounds.y;
    const int bottom = bounds.y + bounds.height - mv;

    const int center_x = left + mh;
    const int center_y = top + mv;
    const int center_w = bounds.width - 2 * mh;
    const int center_h = bounds.height - 2 * mv;

    if (center_w < 0 || center_h < 0) {
        return;
    }

    // Top row
    if (mv > 0) {
        m_tile_pimpl->render_tile(slice.top_left, {left, top, mh, mv});
        if (center_w > 0 && slice.top >= 0) {
            m_tile_pimpl->render_tile(slice.top, {center_x, top, center_w, mv});
        }
        m_tile_pimpl->render_tile(slice.top_right, {right, top, mh, mv});
    }

    // Middle row
    if (center_h > 0) {
        if (mh > 0 && slice.left >= 0) {
            m_tile_pimpl->render_tile(slice.left, {left, center_y, mh, center_h});
        }
        if (center_w > 0 && slice.has_center()) {
            m_tile_pimpl->render_tile(slice.center, {center_x, center_y, center_w, center_h});
        }
        if (mh > 0 && slice.right >= 0) {
            m_tile_pimpl->render_tile(slice.right, {right, center_y, mh, center_h});
        }
    }

    // Bottom row
    if (mv > 0) {
        m_tile_pimpl->render_tile(slice.bottom_left, {left, bottom, mh, mv});
        if (center_w > 0 && slice.bottom >= 0) {
            m_tile_pimpl->render_tile(slice.bottom, {center_x, bottom, center_w, mv});
        }
        m_tile_pimpl->render_tile(slice.bottom_right, {right, bottom, mh, mv});
    }
}

// ============================================================================
// Bitmap Text Drawing
// ============================================================================

void tile_renderer::draw_bitmap_text(std::string_view text, tile_point pos, const bitmap_font& font) {
    if (text.empty() || !font.atlas) {
        return;
    }

    if (!font.atlas->texture) {
        return;
    }

    // Save current state
    tile_atlas* saved_atlas = m_tile_pimpl->atlas;
    ::sdlpp::texture* saved_texture = m_tile_pimpl->texture;

    // Use the font's atlas and texture
    m_tile_pimpl->atlas = font.atlas;
    m_tile_pimpl->texture = static_cast<::sdlpp::texture*>(font.atlas->texture);

    int x = pos.x;
    int y = pos.y;
    const int glyph_w = font.glyph_width;
    const int glyph_h = font.glyph_height;

    for (char ch : text) {
        if (ch == '\n') {
            x = pos.x;
            y += glyph_h;
            continue;
        }
        int tile_id = font.glyph_id(ch);
        if (tile_id >= 0) {
            tile_rect dest{x, y, glyph_w, glyph_h};
            m_tile_pimpl->render_tile(tile_id, dest);
        }
        x += glyph_w;
    }

    // Restore original state
    m_tile_pimpl->atlas = saved_atlas;
    m_tile_pimpl->texture = saved_texture;
}

void tile_renderer::draw_bitmap_text(std::string_view text, tile_point pos, const bitmap_font& font,
                                     const onyxui::sdlpp::color& color) {
    if (text.empty() || !font.atlas) {
        return;
    }

    if (!font.atlas->texture) {
        return;
    }

    // Save current state
    tile_atlas* saved_atlas = m_tile_pimpl->atlas;
    ::sdlpp::texture* saved_texture = m_tile_pimpl->texture;

    // Use the font's atlas and texture
    m_tile_pimpl->atlas = font.atlas;
    m_tile_pimpl->texture = static_cast<::sdlpp::texture*>(font.atlas->texture);

    // Apply color modulation to the texture
    // This multiplies each pixel's color by (r/255, g/255, b/255)
    // For white source glyphs (255,255,255), this produces the exact target color
    m_tile_pimpl->texture->set_color_mod(::sdlpp::color{color.r, color.g, color.b, color.a});
    m_tile_pimpl->texture->set_alpha_mod(color.a);

    int x = pos.x;
    int y = pos.y;
    const int glyph_w = font.glyph_width;
    const int glyph_h = font.glyph_height;

    for (char ch : text) {
        if (ch == '\n') {
            x = pos.x;
            y += glyph_h;
            continue;
        }
        int tile_id = font.glyph_id(ch);
        if (tile_id >= 0) {
            tile_rect dest{x, y, glyph_w, glyph_h};
            m_tile_pimpl->render_tile(tile_id, dest);
        }
        x += glyph_w;
    }

    // Reset color modulation to default (white = no modulation)
    m_tile_pimpl->texture->set_color_mod(::sdlpp::color{255, 255, 255, 255});
    m_tile_pimpl->texture->set_alpha_mod(255);

    // Restore original state
    m_tile_pimpl->atlas = saved_atlas;
    m_tile_pimpl->texture = saved_texture;
}

void tile_renderer::draw_bitmap_text_centered(std::string_view text, tile_rect bounds, const bitmap_font& font) {
    if (text.empty()) {
        return;
    }

    auto [text_width, text_height] = font.text_size(text);

    int x = bounds.x + (bounds.width - text_width) / 2;
    int y = bounds.y + (bounds.height - text_height) / 2;

    draw_bitmap_text(text, {x, y}, font);
}

void tile_renderer::draw_bitmap_text_centered(std::string_view text, tile_rect bounds, const bitmap_font& font,
                                              const onyxui::sdlpp::color& color) {
    if (text.empty()) {
        return;
    }

    auto [text_width, text_height] = font.text_size(text);

    int x = bounds.x + (bounds.width - text_width) / 2;
    int y = bounds.y + (bounds.height - text_height) / 2;

    draw_bitmap_text(text, {x, y}, font, color);
}

// ============================================================================
// Tile-specific Fill
// ============================================================================

void tile_renderer::fill_rect(tile_rect r, onyxui::sdlpp::color c) {
    SDL_FRect sdl_rect = m_tile_pimpl->to_sdl_frect(r);
    SDL_SetRenderDrawColor(
        m_tile_pimpl->sdl_renderer->get(),
        c.r, c.g, c.b, c.a
    );
    SDL_RenderFillRect(m_tile_pimpl->sdl_renderer->get(), &sdl_rect);
}

// ============================================================================
// Bitmap Font Integration (override base class text rendering)
// ============================================================================

void tile_renderer::set_default_bitmap_font(const bitmap_font* font) noexcept {
    m_tile_pimpl->default_bitmap_font = font;
}

const bitmap_font* tile_renderer::get_default_bitmap_font() const noexcept {
    return m_tile_pimpl->default_bitmap_font;
}

void tile_renderer::draw_text(const onyxui::sdlpp::rect& r, std::string_view text,
                              const font& f, const onyxui::sdlpp::color& fg,
                              const onyxui::sdlpp::color& bg) {
    // If a default bitmap font is set, use it for rendering
    if (m_tile_pimpl->default_bitmap_font && m_tile_pimpl->default_bitmap_font->is_valid()) {
        const bitmap_font& bf = *m_tile_pimpl->default_bitmap_font;

        // Draw background if not transparent
        if (bg.a > 0) {
            fill_rect({r.x, r.y, r.w, r.h}, bg);
        }

        // Calculate text dimensions for centering
        auto [text_w, text_h] = bf.text_size(text);

        // Use rect position directly (rect already sized to text)
        int text_x = r.x;
        int text_y = r.y;

        // Draw text using bitmap font with color modulation from theme
        draw_bitmap_text(text, {text_x, text_y}, bf, fg);
        return;
    }

    // Fall back to base class TTF rendering
    base::draw_text(r, text, f, fg, bg);
}

onyxui::sdlpp::size tile_renderer::measure_text_with_bitmap(
    std::string_view text, const font& f) const {
    // If a default bitmap font is set, use it for measurement
    if (m_tile_pimpl->default_bitmap_font && m_tile_pimpl->default_bitmap_font->is_valid()) {
        auto [w, h] = m_tile_pimpl->default_bitmap_font->text_size(text);
        return onyxui::sdlpp::size{w, h};
    }

    // Fall back to base class TTF measurement
    return base::measure_text(text, f);
}

// ============================================================================
// Static Text Measurement (override base class for bitmap font support)
// ============================================================================

onyxui::sdlpp::size tile_renderer::measure_text(
    std::string_view text, const font& f) {
    // Check if a global tile_renderer is set and has a bitmap font
    tile_renderer* renderer = get_renderer();
    if (renderer) {
        const bitmap_font* bf = renderer->get_default_bitmap_font();
        if (bf && bf->is_valid()) {
            auto [w, h] = bf->text_size(text);
            return onyxui::sdlpp::size{w, h};
        }
    }

    // Fall back to base class TTF measurement
    return base::measure_text(text, f);
}

} // namespace onyxui::tile
