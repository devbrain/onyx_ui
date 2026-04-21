#include <onyxui/sdlpp/sdlpp_renderer.hh>
#include <onyxui/sdlpp/sdlpp_backend.hh>
#include <onyxui/sdlpp/win311_icons.hh>

#include <sdlpp/video/renderer.hh>
#include <sdlpp/video/texture.hh>
#include <sdlpp/video/surface.hh>
#include <sdlpp/image/image.hh>
#include <onyx_font/font_factory.hh>
#include <onyx_font/ttf_font.hh>
#include <onyx_font/text/font_source.hh>
#include <onyx_font/text/glyph_cache.hh>
#include <onyx_font/text/atlas_surface.hh>
#include <onyx_font/text/text_style.hh>
#include <onyx_font/text/utf8.hh>
#include <onyx_image/codecs/xpm.hpp>
#include <onyx_image/surface.hpp>
#include <onyx_image/types.hpp>
#include <onyxui/services/ui_services.hh>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

namespace onyxui::sdlpp {

namespace {
    [[nodiscard]] float current_dpi_scale() noexcept {
        auto const* metrics = onyxui::ui_services<sdlpp_backend>::metrics();
        if (!metrics || metrics->dpi_scale <= 0.0) {
            return 1.0f;
        }
        return static_cast<float>(metrics->dpi_scale);
    }

    [[nodiscard]] sdlpp_renderer::font dpi_scaled(sdlpp_renderer::font f) noexcept {
        f.size_px *= current_dpi_scale();
        return f;
    }
} // namespace

// =================================================================
// Font Cache — onyx_font::glyph_cache with lazy SDL atlas upload
//
// Uses onyx_font directly rather than going through lib_sdlpp's
// sdlpp::font::font_cache (which was itself a thin wrapper around
// onyx_font::glyph_cache). Removing that hop lets us:
//   - control atlas upload timing (lazy, dirty-tracked)
//   - keep the same API point (measure_text + draw_text) while
//     dropping an indirection
//   - align with how warlords already uses onyx_font directly
// =================================================================

class font_cache_manager {
public:
    static font_cache_manager& instance() {
        static font_cache_manager mgr;
        return mgr;
    }

    struct font_entry {
        std::vector<std::uint8_t> font_data;  ///< keeps ttf_font's span valid
        std::unique_ptr<onyx_font::ttf_font> ttf;
        std::unique_ptr<onyx_font::glyph_cache<onyx_font::memory_atlas>> cache;
        std::vector<::sdlpp::texture> atlas_textures;
        std::vector<bool> atlas_dirty;
    };

    /**
     * @brief Clear all cached fonts and glyph caches
     *
     * Must be called before SDL shutdown so GPU textures are released
     * before the SDL renderer goes away.
     */
    void clear() {
        m_entries.clear();
    }

    /**
     * @brief Get or create a font_entry for this font spec.
     * @param f Font specification (path, size, style flags)
     * @return Pointer to entry, or nullptr if font load failed.
     */
    font_entry* get_or_create_entry(const sdlpp_renderer::font& f) {
        std::size_t const key = f.hash();
        if (auto it = m_entries.find(key); it != m_entries.end()) {
            return &it->second;
        }

        std::filesystem::path font_path = f.path;
        if (font_path.empty()) {
            font_path = find_fallback_font();
            if (font_path.empty()) {
                return nullptr;
            }
        }

        font_entry entry;
        entry.font_data = read_file(font_path);
        if (entry.font_data.empty()) {
            return nullptr;
        }

        try {
            entry.ttf = std::make_unique<onyx_font::ttf_font>(
                std::span<const std::uint8_t>(entry.font_data.data(),
                                              entry.font_data.size()));
        } catch (...) {
            return nullptr;
        }

        onyx_font::text_style style = onyx_font::text_style::normal;
        if (f.bold)          style = style | onyx_font::text_style::bold;
        if (f.italic)        style = style | onyx_font::text_style::italic;
        if (f.underline)     style = style | onyx_font::text_style::underline;
        if (f.strikethrough) style = style | onyx_font::text_style::strikethrough;

        onyx_font::glyph_cache_config config;
        config.atlas_size = 512;
        config.pre_cache_ascii = true;

        entry.cache = std::make_unique<
            onyx_font::glyph_cache<onyx_font::memory_atlas>>(
                onyx_font::font_source::from_ttf(*entry.ttf),
                f.size_px,
                config);
        entry.cache->set_style(style);
        entry.atlas_dirty.resize(
            static_cast<std::size_t>(entry.cache->atlas_count()), true);

        auto [inserted_it, _] = m_entries.emplace(key, std::move(entry));
        return &inserted_it->second;
    }

    /**
     * @brief Sync onyx_font's CPU atlases to SDL textures. Only re-uploads
     *        pages that were marked dirty since the last call.
     */
    void ensure_textures(::sdlpp::renderer& renderer, font_entry& entry) {
        auto const count = static_cast<std::size_t>(entry.cache->atlas_count());
        entry.atlas_textures.resize(count);
        entry.atlas_dirty.resize(count, true);

        for (std::size_t i = 0; i < count; ++i) {
            if (!entry.atlas_dirty[i] && entry.atlas_textures[i]) continue;

            const auto& atlas = entry.cache->atlas(static_cast<int>(i));
            int const w = atlas.width();
            int const h = atlas.height();

            if (!entry.atlas_textures[i]) {
                auto tex_result = ::sdlpp::texture::create(
                    renderer,
                    ::sdlpp::pixel_format_enum::ABGR8888,
                    ::sdlpp::texture_access::streaming,
                    w, h);
                if (!tex_result) continue;
                entry.atlas_textures[i] = std::move(*tex_result);
                entry.atlas_textures[i].set_blend_mode(::sdlpp::blend_mode::blend);
                entry.atlas_textures[i].set_scale_mode(::sdlpp::scale_mode::linear);
            }

            // onyx_font atlases store 1 byte/pixel alpha; we expand to
            // RGBA (white + alpha) so SDL color-modulation gives us the
            // final glyph color at draw time.
            auto lock_result = entry.atlas_textures[i].lock<::sdlpp::rect_i>();
            if (!lock_result) continue;
            auto [pixels, pitch] = *lock_result;

            const std::uint8_t* alpha_data = atlas.data();
            auto* rgba = static_cast<std::uint8_t*>(pixels);

            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    std::uint8_t a = alpha_data[y * w + x];
                    int const dst = y * pitch + x * 4;
                    rgba[dst + 0] = 255;  // R
                    rgba[dst + 1] = 255;  // G
                    rgba[dst + 2] = 255;  // B
                    rgba[dst + 3] = a;    // A
                }
            }

            entry.atlas_textures[i].unlock();
            entry.atlas_dirty[i] = false;
        }
    }

    /**
     * @brief Measure text via onyx_font's rasterizer.
     */
    size measure_text_cached(const sdlpp_renderer::font& f,
                             std::string_view text) {
        if (text.empty()) {
            return {0, static_cast<int>(f.size_px)};
        }

        auto* entry = get_or_create_entry(f);
        if (!entry) {
            return {static_cast<int>(text.length() * f.size_px * 0.6f),
                    static_cast<int>(f.size_px)};
        }

        auto const extents = entry->cache->measure(text);
        return {static_cast<int>(std::ceil(extents.width)),
                static_cast<int>(std::ceil(extents.height))};
    }

private:
    font_cache_manager() = default;

    static std::vector<std::uint8_t> read_file(const std::filesystem::path& p) {
        std::ifstream in(p, std::ios::binary | std::ios::ate);
        if (!in) return {};
        auto const sz = in.tellg();
        if (sz <= 0) return {};
        in.seekg(0);
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(sz));
        in.read(reinterpret_cast<char*>(bytes.data()), sz);
        if (!in) return {};
        return bytes;
    }

    static std::filesystem::path find_fallback_font() {
        static const std::vector<std::filesystem::path> fallback_paths = {
            // Linux
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            // macOS
            "/System/Library/Fonts/Helvetica.ttc",
            "/System/Library/Fonts/SFNSText.ttf",
            "/Library/Fonts/Arial.ttf",
            // Windows (via Wine/WSL)
            "/usr/share/fonts/truetype/msttcorefonts/arial.ttf",
            "C:/Windows/Fonts/arial.ttf",
        };

        for (const auto& path : fallback_paths) {
            if (std::filesystem::exists(path)) {
                return path;
            }
        }
        return {};
    }

    std::unordered_map<std::size_t, font_entry> m_entries;
};

// =================================================================
// XPM Icon Texture Cache
// =================================================================

/**
 * @brief Cache for XPM-based icon textures
 *
 * Loads Windows 3.11 XPM icons on first use and caches them as SDL textures.
 */
class xpm_icon_cache {
public:
    /**
     * @brief Icon types that have XPM versions
     */
    enum class xpm_icon_type {
        // Window controls
        close_active,
        close_inactive,
        minimize_active,
        minimize_inactive,
        maximize_active,
        maximize_inactive,
        restore_active,
        restore_inactive,
        menu_active,
        menu_inactive,
        // Checkboxes
        checkbox_unchecked,
        checkbox_checked,
        checkbox_indeterminate,
        checkbox_unchecked_disabled,
        checkbox_checked_disabled,
        // Radio buttons
        radio_unchecked,
        radio_checked,
        radio_unchecked_disabled,
        radio_checked_disabled,
        // Scrollbar arrows
        arrow_up,
        arrow_down,
        arrow_left,
        arrow_right,
        arrow_up_disabled,
        arrow_down_disabled,
        arrow_left_disabled,
        arrow_right_disabled
    };

    /**
     * @brief Get or create texture for an XPM icon
     * @param renderer SDL renderer for texture creation
     * @param type Icon type to get
     * @return Pointer to cached texture, or nullptr on failure
     */
    ::sdlpp::texture* get_icon(::sdlpp::renderer& renderer, xpm_icon_type type) {
        auto it = m_textures.find(type);
        if (it != m_textures.end()) {
            return &it->second;
        }

        // Get XPM data for this icon type
        const char* const* xpm_data = get_xpm_data(type);
        if (!xpm_data) {
            return nullptr;
        }

        // Wrap the embedded char* const[] icon data as an XPM source blob
        // and decode through onyx_image. The decoder expects a full source
        // file (XPM marker, C array, quoted strings); we synthesize exactly
        // that by re-quoting the pre-parsed lines.
        std::string xpm_source = synthesize_xpm_source(xpm_data);
        if (xpm_source.empty()) {
            return nullptr;
        }

        onyx_image::memory_surface surf;
        auto const* bytes = reinterpret_cast<const std::uint8_t*>(xpm_source.data());
        auto decode = onyx_image::xpm_decoder::decode(
            std::span<const std::uint8_t>(bytes, xpm_source.size()), surf);
        if (!decode.success) {
            return nullptr;
        }

        // onyx_image writes RGBA8888 bytes (R, G, B, A low→high). On little-
        // endian systems read as uint32, that pattern is 0xAABBGGRR — which
        // SDL names ABGR8888. Same pixel layout the old parser produced.
        int const pitch = surf.width() * 4;
        auto surf_result = ::sdlpp::surface::create_from_pixels(
            const_cast<std::uint8_t*>(surf.pixels().data()),
            surf.width(), surf.height(), pitch,
            ::sdlpp::pixel_format_enum::ABGR8888
        );

        if (!surf_result) {
            return nullptr;
        }

        // Create texture from surface
        auto tex_result = ::sdlpp::texture::create(renderer, *surf_result);
        if (!tex_result) {
            return nullptr;
        }

        // Enable alpha blending for transparency
        tex_result->set_blend_mode(::sdlpp::blend_mode::blend);

        // Cache and return
        auto [inserted_it, success] = m_textures.emplace(type, std::move(*tex_result));
        return success ? &inserted_it->second : nullptr;
    }

    void clear() {
        m_textures.clear();
    }

private:
    std::unordered_map<xpm_icon_type, ::sdlpp::texture> m_textures;

    // Rebuild an XPM source string from an embedded char* const[] icon:
    //   "/* XPM */\nstatic char *img[] = { \"<line0>\", \"<line1>\", ... };\n"
    //
    // The data has no null terminator, so we read the header (line 0) to
    // compute the expected total line count: 1 + num_colors + height.
    [[nodiscard]] static std::string synthesize_xpm_source(const char* const* data) {
        if (!data || !data[0]) {
            return {};
        }
        int width = 0, height = 0, num_colors = 0, cpp = 0;
        if (std::sscanf(data[0], "%d %d %d %d", &width, &height, &num_colors, &cpp) != 4) {
            return {};
        }
        if (width <= 0 || height <= 0 || num_colors <= 0 || cpp <= 0) {
            return {};
        }
        std::size_t const total_lines =
            1 + static_cast<std::size_t>(num_colors) + static_cast<std::size_t>(height);

        std::string out;
        out.reserve(total_lines * 32);
        out += "/* XPM */\nstatic char * img[] = {\n";
        for (std::size_t i = 0; i < total_lines; ++i) {
            if (!data[i]) {
                return {};
            }
            out += '\"';
            out += data[i];
            out += "\",\n";
        }
        out += "};\n";
        return out;
    }

    static const char* const* get_xpm_data(xpm_icon_type type) {
        switch (type) {
            // Window controls
            case xpm_icon_type::close_active:      return win311_icons::close_active;
            case xpm_icon_type::close_inactive:    return win311_icons::close_inactive;
            case xpm_icon_type::minimize_active:   return win311_icons::minimize_active;
            case xpm_icon_type::minimize_inactive: return win311_icons::minimize_inactive;
            case xpm_icon_type::maximize_active:   return win311_icons::maximize_active;
            case xpm_icon_type::maximize_inactive: return win311_icons::maximize_inactive;
            case xpm_icon_type::restore_active:    return win311_icons::restore_active;
            case xpm_icon_type::restore_inactive:  return win311_icons::restore_inactive;
            case xpm_icon_type::menu_active:       return win311_icons::menu_active;
            case xpm_icon_type::menu_inactive:     return win311_icons::menu_inactive;
            // Checkboxes
            case xpm_icon_type::checkbox_unchecked:          return win311_icons::checkbox_unchecked;
            case xpm_icon_type::checkbox_checked:            return win311_icons::checkbox_checked;
            case xpm_icon_type::checkbox_indeterminate:      return win311_icons::checkbox_indeterminate;
            case xpm_icon_type::checkbox_unchecked_disabled: return win311_icons::checkbox_unchecked_disabled;
            case xpm_icon_type::checkbox_checked_disabled:   return win311_icons::checkbox_checked_disabled;
            // Radio buttons
            case xpm_icon_type::radio_unchecked:          return win311_icons::radio_unchecked;
            case xpm_icon_type::radio_checked:            return win311_icons::radio_checked;
            case xpm_icon_type::radio_unchecked_disabled: return win311_icons::radio_unchecked_disabled;
            case xpm_icon_type::radio_checked_disabled:   return win311_icons::radio_checked_disabled;
            // Scrollbar arrows
            case xpm_icon_type::arrow_up:             return win311_icons::arrow_up;
            case xpm_icon_type::arrow_down:           return win311_icons::arrow_down;
            case xpm_icon_type::arrow_left:           return win311_icons::arrow_left;
            case xpm_icon_type::arrow_right:          return win311_icons::arrow_right;
            case xpm_icon_type::arrow_up_disabled:    return win311_icons::arrow_up_disabled;
            case xpm_icon_type::arrow_down_disabled:  return win311_icons::arrow_down_disabled;
            case xpm_icon_type::arrow_left_disabled:  return win311_icons::arrow_left_disabled;
            case xpm_icon_type::arrow_right_disabled: return win311_icons::arrow_right_disabled;
        }
        return nullptr;
    }
};

// Global XPM icon cache (per-process)
static xpm_icon_cache& get_xpm_cache() {
    static xpm_icon_cache cache;
    return cache;
}

// =================================================================
// Renderer Implementation
// =================================================================

struct sdlpp_renderer::impl {
    ::sdlpp::renderer* renderer = nullptr;
    std::stack<rect> clip_stack;
    rect viewport{0, 0, 800, 600};
    std::filesystem::path assets_path;

    explicit impl(::sdlpp::renderer& r, const std::filesystem::path& assets)
        : renderer(&r), assets_path(assets)
    {
        // Get viewport from renderer's output size
        if (auto size_result = r.get_output_size<::sdlpp::size_i>()) {
            viewport = rect{0, 0, size_result->width, size_result->height};
        }
    }
};

sdlpp_renderer::sdlpp_renderer(::sdlpp::renderer& sdl_renderer,
                               const std::filesystem::path& assets_path)
    : m_pimpl(std::make_unique<impl>(sdl_renderer, assets_path))
{
}

sdlpp_renderer::~sdlpp_renderer() = default;
sdlpp_renderer::sdlpp_renderer(sdlpp_renderer&&) noexcept = default;
sdlpp_renderer& sdlpp_renderer::operator=(sdlpp_renderer&&) noexcept = default;

void sdlpp_renderer::draw_box(const rect& r, const box_style& style,
                               const color& fg, const color& bg)
{
    auto& rend = *m_pimpl->renderer;

    // Fill background
    if (style.is_solid) {
        rend.set_draw_color(::sdlpp::color{bg.r, bg.g, bg.b, bg.a});
        rend.fill_rect(::sdlpp::rect_i{r.x, r.y, r.w, r.h});
    }

    // Draw border
    switch (style.style) {
        case border_style_type::none:
            break;

        case border_style_type::flat:
            rend.set_draw_color(::sdlpp::color{fg.r, fg.g, fg.b, fg.a});
            rend.draw_rect(::sdlpp::rect_i{r.x, r.y, r.w, r.h});
            break;

        case border_style_type::raised:
            // Light edge (top-left)
            rend.set_draw_color(::sdlpp::color{255, 255, 255, 255});
            rend.draw_line(::sdlpp::point_i{r.x, r.y},
                          ::sdlpp::point_i{r.x + r.w - 1, r.y});
            rend.draw_line(::sdlpp::point_i{r.x, r.y},
                          ::sdlpp::point_i{r.x, r.y + r.h - 1});
            // Dark edge (bottom-right)
            rend.set_draw_color(::sdlpp::color{128, 128, 128, 255});
            rend.draw_line(::sdlpp::point_i{r.x + r.w - 1, r.y},
                          ::sdlpp::point_i{r.x + r.w - 1, r.y + r.h - 1});
            rend.draw_line(::sdlpp::point_i{r.x, r.y + r.h - 1},
                          ::sdlpp::point_i{r.x + r.w - 1, r.y + r.h - 1});
            break;

        case border_style_type::sunken:
            // Dark edge (top-left)
            rend.set_draw_color(::sdlpp::color{128, 128, 128, 255});
            rend.draw_line(::sdlpp::point_i{r.x, r.y},
                          ::sdlpp::point_i{r.x + r.w - 1, r.y});
            rend.draw_line(::sdlpp::point_i{r.x, r.y},
                          ::sdlpp::point_i{r.x, r.y + r.h - 1});
            // Light edge (bottom-right)
            rend.set_draw_color(::sdlpp::color{255, 255, 255, 255});
            rend.draw_line(::sdlpp::point_i{r.x + r.w - 1, r.y},
                          ::sdlpp::point_i{r.x + r.w - 1, r.y + r.h - 1});
            rend.draw_line(::sdlpp::point_i{r.x, r.y + r.h - 1},
                          ::sdlpp::point_i{r.x + r.w - 1, r.y + r.h - 1});
            break;

        default:
            break;
    }
}

void sdlpp_renderer::draw_text(const rect& r, std::string_view text,
                                const font& f, const color& fg, const color& bg)
{
    (void)bg;  // text renders glyph alpha — bg is the caller's problem
    if (text.empty()) {
        return;
    }

    const font scaled_font = dpi_scaled(f);

    auto& mgr = font_cache_manager::instance();
    auto* entry = mgr.get_or_create_entry(scaled_font);
    if (!entry) {
        return;
    }

    auto& rend = *m_pimpl->renderer;
    mgr.ensure_textures(rend, *entry);

    auto& cache = *entry->cache;
    auto const metrics = cache.rasterizer().get_metrics();

    float pen_x = static_cast<float>(r.x);
    float const pen_y = static_cast<float>(r.y);

    for (char32_t cp : onyx_font::utf8_view(text)) {
        const auto& g = cache.get(cp);

        if (g.atlas_index < 0 ||
            g.atlas_index >= static_cast<int>(entry->atlas_textures.size())) {
            pen_x += g.advance_x;
            continue;
        }

        auto& tex = entry->atlas_textures[static_cast<std::size_t>(g.atlas_index)];
        if (!tex) {
            pen_x += g.advance_x;
            continue;
        }

        tex.set_color_mod(::sdlpp::color{fg.r, fg.g, fg.b, fg.a});
        tex.set_alpha_mod(fg.a);

        float const dst_x = pen_x + g.bearing_x;
        float const dst_y = pen_y + (metrics.ascent - g.bearing_y);

        std::optional<::sdlpp::rect_f> src_opt{
            ::sdlpp::rect_f(static_cast<float>(g.rect.x),
                          static_cast<float>(g.rect.y),
                          static_cast<float>(g.rect.w),
                          static_cast<float>(g.rect.h))};
        std::optional<::sdlpp::rect_f> dst_opt{
            ::sdlpp::rect_f(dst_x, dst_y,
                          static_cast<float>(g.rect.w),
                          static_cast<float>(g.rect.h))};

        rend.copy(tex, src_opt, dst_opt);
        pen_x += g.advance_x;
    }
}

void sdlpp_renderer::draw_icon(const rect& r, icon_style style,
                                const color& fg, const color& bg)
{
    auto& rend = *m_pimpl->renderer;
    const auto sdl_fg = ::sdlpp::color{fg.r, fg.g, fg.b, fg.a};
    const auto sdl_bg = ::sdlpp::color{bg.r, bg.g, bg.b, bg.a};

    // Try to use XPM texture for window control icons
    // These are the authentic Windows 3.11 button graphics
    auto try_render_xpm = [&](xpm_icon_cache::xpm_icon_type active_type,
                               xpm_icon_cache::xpm_icon_type inactive_type) -> bool {
        // Determine if active based on background brightness
        // Active buttons have gray background, inactive have different appearance
        // For now, always use active icons (the title bar styling handles visual state)
        auto& cache = get_xpm_cache();
        auto* texture = cache.get_icon(rend, active_type);
        if (!texture) {
            return false;  // Fall back to procedural drawing
        }

        // Render the XPM texture scaled to fit the target rect
        ::sdlpp::rect_i dst{r.x, r.y, r.w, r.h};
        std::optional<::sdlpp::rect_i> src_opt = std::nullopt;
        std::optional<::sdlpp::rect_i> dst_opt = dst;
        rend.copy(*texture, src_opt, dst_opt);
        return true;
    };

    // Helper to render a single XPM icon type
    auto try_render_single_xpm = [&](xpm_icon_cache::xpm_icon_type type) -> bool {
        auto& cache = get_xpm_cache();
        auto* texture = cache.get_icon(rend, type);
        if (!texture) {
            return false;
        }
        ::sdlpp::rect_i dst{r.x, r.y, r.w, r.h};
        std::optional<::sdlpp::rect_i> src_opt = std::nullopt;
        std::optional<::sdlpp::rect_i> dst_opt = dst;
        rend.copy(*texture, src_opt, dst_opt);
        return true;
    };

    // Check for icons that have XPM versions
    switch (style) {
        // Window controls
        case icon_style::close_x:
            if (try_render_xpm(xpm_icon_cache::xpm_icon_type::close_active,
                               xpm_icon_cache::xpm_icon_type::close_inactive)) {
                return;
            }
            break;
        case icon_style::minimize:
            if (try_render_xpm(xpm_icon_cache::xpm_icon_type::minimize_active,
                               xpm_icon_cache::xpm_icon_type::minimize_inactive)) {
                return;
            }
            break;
        case icon_style::maximize:
            if (try_render_xpm(xpm_icon_cache::xpm_icon_type::maximize_active,
                               xpm_icon_cache::xpm_icon_type::maximize_inactive)) {
                return;
            }
            break;
        case icon_style::restore:
            if (try_render_xpm(xpm_icon_cache::xpm_icon_type::restore_active,
                               xpm_icon_cache::xpm_icon_type::restore_inactive)) {
                return;
            }
            break;
        case icon_style::menu:
            if (try_render_xpm(xpm_icon_cache::xpm_icon_type::menu_active,
                               xpm_icon_cache::xpm_icon_type::menu_inactive)) {
                return;
            }
            break;
        // Checkboxes
        case icon_style::checkbox_unchecked:
            if (try_render_single_xpm(xpm_icon_cache::xpm_icon_type::checkbox_unchecked)) {
                return;
            }
            break;
        case icon_style::checkbox_checked:
            if (try_render_single_xpm(xpm_icon_cache::xpm_icon_type::checkbox_checked)) {
                return;
            }
            break;
        case icon_style::checkbox_indeterminate:
            if (try_render_single_xpm(xpm_icon_cache::xpm_icon_type::checkbox_indeterminate)) {
                return;
            }
            break;
        // Radio buttons
        case icon_style::radio_unchecked:
            if (try_render_single_xpm(xpm_icon_cache::xpm_icon_type::radio_unchecked)) {
                return;
            }
            break;
        case icon_style::radio_checked:
            if (try_render_single_xpm(xpm_icon_cache::xpm_icon_type::radio_checked)) {
                return;
            }
            break;
        // Scrollbar arrows
        case icon_style::arrow_up:
            if (try_render_single_xpm(xpm_icon_cache::xpm_icon_type::arrow_up)) {
                return;
            }
            break;
        case icon_style::arrow_down:
            if (try_render_single_xpm(xpm_icon_cache::xpm_icon_type::arrow_down)) {
                return;
            }
            break;
        case icon_style::arrow_left:
            if (try_render_single_xpm(xpm_icon_cache::xpm_icon_type::arrow_left)) {
                return;
            }
            break;
        case icon_style::arrow_right:
            if (try_render_single_xpm(xpm_icon_cache::xpm_icon_type::arrow_right)) {
                return;
            }
            break;
        default:
            break;  // Use procedural drawing for other icons
    }

    // Helper to draw filled triangle (for arrows)
    auto fill_triangle = [&rend](int x1, int y1, int x2, int y2, int x3, int y3) {
        // Simple scanline fill for small triangles
        int min_y = std::min({y1, y2, y3});
        int max_y = std::max({y1, y2, y3});
        for (int y = min_y; y <= max_y; ++y) {
            int min_x = 10000, max_x = -10000;
            // Find x intersections with triangle edges
            auto intersect = [&min_x, &max_x, y](int ax, int ay, int bx, int by) {
                if ((ay <= y && by > y) || (by <= y && ay > y)) {
                    int x = ax + (y - ay) * (bx - ax) / (by - ay);
                    min_x = std::min(min_x, x);
                    max_x = std::max(max_x, x);
                }
            };
            intersect(x1, y1, x2, y2);
            intersect(x2, y2, x3, y3);
            intersect(x3, y3, x1, y1);
            if (min_x <= max_x) {
                rend.draw_line(::sdlpp::point_i(min_x, y), ::sdlpp::point_i(max_x, y));
            }
        }
    };

    switch (style) {
        case icon_style::none:
            break;

        // ═══════════════════════════════════════════════════════════════
        // Checkmark (✓)
        // ═══════════════════════════════════════════════════════════════
        case icon_style::check: {
            rend.set_draw_color(sdl_fg);
            int cx = r.x + r.w / 4;
            int cy = r.y + r.h / 2;
            // Draw checkmark with 2px thickness for visibility
            for (int t = 0; t < 2; ++t) {
                rend.draw_line(::sdlpp::point_i{cx, cy + t},
                              ::sdlpp::point_i{cx + r.w / 4, cy + r.h / 4 + t});
                rend.draw_line(::sdlpp::point_i{cx + r.w / 4, cy + r.h / 4 + t},
                              ::sdlpp::point_i{cx + r.w / 2, cy - r.h / 4 + t});
            }
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Cross / Close X (×)
        // ═══════════════════════════════════════════════════════════════
        case icon_style::cross:
        case icon_style::close_x: {
            rend.set_draw_color(sdl_fg);
            int m = std::max(2, r.w / 4);
            for (int t = 0; t < 2; ++t) {
                rend.draw_line(::sdlpp::point_i{r.x + m + t, r.y + m},
                              ::sdlpp::point_i{r.x + r.w - m + t, r.y + r.h - m});
                rend.draw_line(::sdlpp::point_i{r.x + r.w - m + t, r.y + m},
                              ::sdlpp::point_i{r.x + m + t, r.y + r.h - m});
            }
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Bullet (•)
        // ═══════════════════════════════════════════════════════════════
        case icon_style::bullet: {
            rend.set_draw_color(sdl_fg);
            int cx = r.x + r.w / 2;
            int cy = r.y + r.h / 2;
            int radius = std::min(r.w, r.h) / 4;
            // Draw filled circle (simple approximation)
            for (int dy = -radius; dy <= radius; ++dy) {
                for (int dx = -radius; dx <= radius; ++dx) {
                    if (dx*dx + dy*dy <= radius*radius) {
                        rend.draw_point(::sdlpp::point_i{cx + dx, cy + dy});
                    }
                }
            }
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Folder / Submenu indicator (▶)
        // ═══════════════════════════════════════════════════════════════
        case icon_style::folder: {
            rend.set_draw_color(sdl_fg);
            int m = std::max(2, r.w / 4);
            // Right-pointing triangle
            fill_triangle(r.x + m, r.y + m,
                         r.x + m, r.y + r.h - m,
                         r.x + r.w - m, r.y + r.h / 2);
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // File (■)
        // ═══════════════════════════════════════════════════════════════
        case icon_style::file: {
            rend.set_draw_color(sdl_fg);
            int m = std::max(2, r.w / 4);
            ::sdlpp::rect_i rect{r.x + m, r.y + m, r.w - 2*m, r.h - 2*m};
            rend.fill_rect(rect);
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Arrow Up (▲)
        // ═══════════════════════════════════════════════════════════════
        case icon_style::arrow_up: {
            rend.set_draw_color(sdl_fg);
            int m = std::max(2, r.w / 4);
            fill_triangle(r.x + r.w / 2, r.y + m,
                         r.x + m, r.y + r.h - m,
                         r.x + r.w - m, r.y + r.h - m);
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Arrow Down (▼)
        // ═══════════════════════════════════════════════════════════════
        case icon_style::arrow_down: {
            rend.set_draw_color(sdl_fg);
            int m = std::max(2, r.w / 4);
            fill_triangle(r.x + m, r.y + m,
                         r.x + r.w - m, r.y + m,
                         r.x + r.w / 2, r.y + r.h - m);
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Arrow Left (◄)
        // ═══════════════════════════════════════════════════════════════
        case icon_style::arrow_left: {
            rend.set_draw_color(sdl_fg);
            int m = std::max(2, r.w / 4);
            fill_triangle(r.x + m, r.y + r.h / 2,
                         r.x + r.w - m, r.y + m,
                         r.x + r.w - m, r.y + r.h - m);
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Arrow Right (►)
        // ═══════════════════════════════════════════════════════════════
        case icon_style::arrow_right: {
            rend.set_draw_color(sdl_fg);
            int m = std::max(2, r.w / 4);
            fill_triangle(r.x + m, r.y + m,
                         r.x + m, r.y + r.h - m,
                         r.x + r.w - m, r.y + r.h / 2);
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Menu (≡)
        // ═══════════════════════════════════════════════════════════════
        case icon_style::menu: {
            rend.set_draw_color(sdl_fg);
            int m = std::max(2, r.w / 5);
            int line_h = (r.h - 2*m) / 4;
            for (int i = 0; i < 3; ++i) {
                int y = r.y + m + i * (line_h + 1);
                rend.draw_line(::sdlpp::point_i{r.x + m, y},
                              ::sdlpp::point_i{r.x + r.w - m, y});
                rend.draw_line(::sdlpp::point_i{r.x + m, y + 1},
                              ::sdlpp::point_i{r.x + r.w - m, y + 1});
            }
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Minimize - Windows 3.11 style: DOWN arrow/chevron
        // ═══════════════════════════════════════════════════════════════
        case icon_style::minimize: {
            rend.set_draw_color(sdl_fg);
            int cx = r.x + r.w / 2;
            int cy = r.y + r.h / 2;
            int size = std::min(r.w, r.h) / 3;
            // Draw down-pointing chevron (Win3.11 minimize style)
            // Widest row at top, narrowing to point at bottom
            for (int row = 0; row <= size; ++row) {
                int half_width = size - row;
                rend.draw_line(::sdlpp::point_i{cx - half_width, cy - size/2 + row},
                              ::sdlpp::point_i{cx + half_width, cy - size/2 + row});
            }
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Maximize - Windows 3.11 style: UP arrow/chevron
        // ═══════════════════════════════════════════════════════════════
        case icon_style::maximize: {
            rend.set_draw_color(sdl_fg);
            int cx = r.x + r.w / 2;
            int cy = r.y + r.h / 2;
            int size = std::min(r.w, r.h) / 3;
            // Draw up-pointing chevron (Win3.11 maximize style)
            // Point at top, widening to base at bottom
            for (int row = 0; row <= size; ++row) {
                int half_width = row;
                rend.draw_line(::sdlpp::point_i{cx - half_width, cy - size/2 + row},
                              ::sdlpp::point_i{cx + half_width, cy - size/2 + row});
            }
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Restore - Windows 3.11 style: UP + DOWN arrows
        // ═══════════════════════════════════════════════════════════════
        case icon_style::restore: {
            rend.set_draw_color(sdl_fg);
            int cx = r.x + r.w / 2;
            int cy = r.y + r.h / 2;
            int size = std::min(r.w, r.h) / 4;
            int gap = 1;
            // Draw up arrow (top half)
            for (int row = 0; row <= size; ++row) {
                int half_width = row;
                rend.draw_line(::sdlpp::point_i{cx - half_width, cy - gap - size + row},
                              ::sdlpp::point_i{cx + half_width, cy - gap - size + row});
            }
            // Draw down arrow (bottom half)
            for (int row = 0; row <= size; ++row) {
                int half_width = size - row;
                rend.draw_line(::sdlpp::point_i{cx - half_width, cy + gap + row},
                              ::sdlpp::point_i{cx + half_width, cy + gap + row});
            }
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Cursor Insert (│)
        // ═══════════════════════════════════════════════════════════════
        case icon_style::cursor_insert: {
            rend.set_draw_color(sdl_fg);
            int x = r.x + r.w / 2;
            rend.draw_line(::sdlpp::point_i{x, r.y}, ::sdlpp::point_i{x, r.y + r.h});
            rend.draw_line(::sdlpp::point_i{x + 1, r.y}, ::sdlpp::point_i{x + 1, r.y + r.h});
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Cursor Overwrite (█)
        // ═══════════════════════════════════════════════════════════════
        case icon_style::cursor_overwrite: {
            rend.set_draw_color(sdl_fg);
            ::sdlpp::rect_i rect{r.x, r.y, r.w, r.h};
            rend.fill_rect(rect);
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Checkbox Unchecked [ ]
        // ═══════════════════════════════════════════════════════════════
        case icon_style::checkbox_unchecked: {
            // Windows 3.11 style: 2-level sunken border
            // Outer: dark gray (#7d7d7d) top-left, white bottom-right
            // Inner: black top-left, button face bottom-right
            constexpr ::sdlpp::color dark_gray{125, 125, 125, 255};  // #7d7d7d
            constexpr ::sdlpp::color black{0, 0, 0, 255};
            constexpr ::sdlpp::color white{255, 255, 255, 255};
            constexpr ::sdlpp::color button_face{192, 192, 192, 255};  // #C0C0C0

            // Outer bevel - dark gray top/left
            rend.set_draw_color(dark_gray);
            rend.draw_line(::sdlpp::point_i{r.x, r.y}, ::sdlpp::point_i{r.x + r.w - 1, r.y});
            rend.draw_line(::sdlpp::point_i{r.x, r.y}, ::sdlpp::point_i{r.x, r.y + r.h - 1});
            // Outer bevel - white bottom/right
            rend.set_draw_color(white);
            rend.draw_line(::sdlpp::point_i{r.x + r.w - 1, r.y}, ::sdlpp::point_i{r.x + r.w - 1, r.y + r.h - 1});
            rend.draw_line(::sdlpp::point_i{r.x, r.y + r.h - 1}, ::sdlpp::point_i{r.x + r.w - 1, r.y + r.h - 1});
            // Inner bevel - black top/left
            rend.set_draw_color(black);
            rend.draw_line(::sdlpp::point_i{r.x + 1, r.y + 1}, ::sdlpp::point_i{r.x + r.w - 2, r.y + 1});
            rend.draw_line(::sdlpp::point_i{r.x + 1, r.y + 1}, ::sdlpp::point_i{r.x + 1, r.y + r.h - 2});
            // Inner bevel - button face bottom/right
            rend.set_draw_color(button_face);
            rend.draw_line(::sdlpp::point_i{r.x + r.w - 2, r.y + 1}, ::sdlpp::point_i{r.x + r.w - 2, r.y + r.h - 2});
            rend.draw_line(::sdlpp::point_i{r.x + 1, r.y + r.h - 2}, ::sdlpp::point_i{r.x + r.w - 2, r.y + r.h - 2});
            // Fill interior with white
            rend.set_draw_color(sdl_bg);
            ::sdlpp::rect_i inner{r.x + 2, r.y + 2, r.w - 4, r.h - 4};
            rend.fill_rect(inner);
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Checkbox Checked [X]
        // ═══════════════════════════════════════════════════════════════
        case icon_style::checkbox_checked: {
            // Draw box first
            draw_icon(r, icon_style::checkbox_unchecked, fg, bg);
            // Draw checkmark - Windows 3.11 style thick checkmark
            rend.set_draw_color(sdl_fg);
            int m = 3;
            // Draw checkmark with multiple passes for thickness
            for (int t = 0; t < 2; ++t) {
                // Left part of checkmark (going down)
                rend.draw_line(::sdlpp::point_i{r.x + m + t, r.y + r.h/2},
                              ::sdlpp::point_i{r.x + r.w/3 + t, r.y + r.h - m - 1});
                // Right part of checkmark (going up)
                rend.draw_line(::sdlpp::point_i{r.x + r.w/3 + t, r.y + r.h - m - 1},
                              ::sdlpp::point_i{r.x + r.w - m + t, r.y + m});
            }
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Checkbox Indeterminate [-]
        // ═══════════════════════════════════════════════════════════════
        case icon_style::checkbox_indeterminate: {
            draw_icon(r, icon_style::checkbox_unchecked, fg, bg);
            rend.set_draw_color(sdl_fg);
            int m = 3;
            int y = r.y + r.h / 2;
            rend.draw_line(::sdlpp::point_i{r.x + m, y}, ::sdlpp::point_i{r.x + r.w - m, y});
            rend.draw_line(::sdlpp::point_i{r.x + m, y + 1}, ::sdlpp::point_i{r.x + r.w - m, y + 1});
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Radio Unchecked ( )
        // ═══════════════════════════════════════════════════════════════
        case icon_style::radio_unchecked: {
            // Windows 3.11 style: circular 3D sunken border
            constexpr ::sdlpp::color dark_gray{125, 125, 125, 255};  // #7d7d7d
            constexpr ::sdlpp::color black{0, 0, 0, 255};
            constexpr ::sdlpp::color white{255, 255, 255, 255};
            constexpr ::sdlpp::color button_face{192, 192, 192, 255};

            int cx = r.x + r.w / 2;
            int cy = r.y + r.h / 2;
            int radius = std::min(r.w, r.h) / 2 - 1;

            // Draw filled white circle for interior
            rend.set_draw_color(sdl_bg);
            for (int dy = -radius + 2; dy <= radius - 2; ++dy) {
                for (int dx = -radius + 2; dx <= radius - 2; ++dx) {
                    if (dx*dx + dy*dy <= (radius - 2) * (radius - 2)) {
                        rend.draw_point(::sdlpp::point_i{cx + dx, cy + dy});
                    }
                }
            }

            // Draw 3D bevel effect on circle
            // Top-left arc: dark gray outer, black inner
            // Bottom-right arc: white outer, button face inner
            for (int angle = 0; angle < 360; angle += 5) {
                double rad = angle * 3.14159 / 180.0;
                int x_outer = cx + static_cast<int>(radius * std::cos(rad));
                int y_outer = cy + static_cast<int>(radius * std::sin(rad));
                int x_inner = cx + static_cast<int>((radius - 1) * std::cos(rad));
                int y_inner = cy + static_cast<int>((radius - 1) * std::sin(rad));

                // Determine if we're in top-left or bottom-right half
                if (angle >= 225 || angle < 45) {  // Right side - white/button_face
                    rend.set_draw_color(white);
                    rend.draw_point(::sdlpp::point_i{x_outer, y_outer});
                    rend.set_draw_color(button_face);
                    rend.draw_point(::sdlpp::point_i{x_inner, y_inner});
                } else if (angle >= 45 && angle < 225) {  // Left side - dark_gray/black
                    rend.set_draw_color(dark_gray);
                    rend.draw_point(::sdlpp::point_i{x_outer, y_outer});
                    rend.set_draw_color(black);
                    rend.draw_point(::sdlpp::point_i{x_inner, y_inner});
                }
            }
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Radio Checked (*)
        // ═══════════════════════════════════════════════════════════════
        case icon_style::radio_checked: {
            draw_icon(r, icon_style::radio_unchecked, fg, bg);
            // Draw filled black center dot - Windows 3.11 style
            constexpr ::sdlpp::color black{0, 0, 0, 255};
            rend.set_draw_color(black);
            int cx = r.x + r.w / 2;
            int cy = r.y + r.h / 2;
            int radius = std::min(r.w, r.h) / 5;  // Slightly smaller dot
            for (int dy = -radius; dy <= radius; ++dy) {
                for (int dx = -radius; dx <= radius; ++dx) {
                    if (dx*dx + dy*dy <= radius*radius) {
                        rend.draw_point(::sdlpp::point_i{cx + dx, cy + dy});
                    }
                }
            }
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Progress Filled (▓)
        // ═══════════════════════════════════════════════════════════════
        case icon_style::progress_filled: {
            rend.set_draw_color(sdl_fg);
            ::sdlpp::rect_i rect{r.x, r.y, r.w, r.h};
            rend.fill_rect(rect);
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Progress Empty (░)
        // ═══════════════════════════════════════════════════════════════
        case icon_style::progress_empty: {
            // Dithered pattern
            rend.set_draw_color(sdl_fg);
            for (int y = r.y; y < r.y + r.h; ++y) {
                for (int x = r.x; x < r.x + r.w; ++x) {
                    if ((x + y) % 2 == 0) {
                        rend.draw_point(::sdlpp::point_i{x, y});
                    }
                }
            }
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Slider Filled (═)
        // ═══════════════════════════════════════════════════════════════
        case icon_style::slider_filled: {
            rend.set_draw_color(sdl_fg);
            int y = r.y + r.h / 2;
            for (int t = -1; t <= 1; ++t) {
                rend.draw_line(::sdlpp::point_i{r.x, y + t}, ::sdlpp::point_i{r.x + r.w, y + t});
            }
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Slider Empty (─)
        // ═══════════════════════════════════════════════════════════════
        case icon_style::slider_empty: {
            rend.set_draw_color(sdl_fg);
            int y = r.y + r.h / 2;
            rend.draw_line(::sdlpp::point_i{r.x, y}, ::sdlpp::point_i{r.x + r.w, y});
            break;
        }

        // ═══════════════════════════════════════════════════════════════
        // Slider Thumb (◆)
        // ═══════════════════════════════════════════════════════════════
        case icon_style::slider_thumb: {
            rend.set_draw_color(sdl_fg);
            int cx = r.x + r.w / 2;
            int cy = r.y + r.h / 2;
            int size = std::min(r.w, r.h) / 2 - 1;
            // Draw diamond shape
            fill_triangle(cx, r.y + 1, r.x + 1, cy, cx, r.y + r.h - 1);
            fill_triangle(cx, r.y + 1, r.x + r.w - 1, cy, cx, r.y + r.h - 1);
            break;
        }
    }
}

void sdlpp_renderer::draw_horizontal_line(const rect& r, const line_style& style,
                                           const color& fg, const color& bg)
{
    auto& rend = *m_pimpl->renderer;
    rend.set_draw_color(::sdlpp::color{fg.r, fg.g, fg.b, fg.a});

    int y = r.y + r.h / 2;
    for (int i = 0; i < style.thickness; ++i) {
        rend.draw_line(::sdlpp::point_i{r.x, y + i},
                      ::sdlpp::point_i{r.x + r.w, y + i});
    }
}

void sdlpp_renderer::draw_vertical_line(const rect& r, const line_style& style,
                                         const color& fg, const color& bg)
{
    auto& rend = *m_pimpl->renderer;
    rend.set_draw_color(::sdlpp::color{fg.r, fg.g, fg.b, fg.a});

    int x = r.x + r.w / 2;
    for (int i = 0; i < style.thickness; ++i) {
        rend.draw_line(::sdlpp::point_i{x + i, r.y},
                      ::sdlpp::point_i{x + i, r.y + r.h});
    }
}

void sdlpp_renderer::draw_background(const rect& viewport,
                                      const background_style& style)
{
    auto& rend = *m_pimpl->renderer;
    rend.set_draw_color(::sdlpp::color{
        style.bg_color.r, style.bg_color.g,
        style.bg_color.b, style.bg_color.a});
    rend.fill_rect(::sdlpp::rect_i{viewport.x, viewport.y, viewport.w, viewport.h});
}

void sdlpp_renderer::draw_background(const rect& viewport,
                                      const background_style& style,
                                      const std::vector<rect>& dirty_regions)
{
    if (dirty_regions.empty()) {
        draw_background(viewport, style);
        return;
    }

    auto& rend = *m_pimpl->renderer;
    rend.set_draw_color(::sdlpp::color{
        style.bg_color.r, style.bg_color.g,
        style.bg_color.b, style.bg_color.a});

    for (const auto& r : dirty_regions) {
        rend.fill_rect(::sdlpp::rect_i{r.x, r.y, r.w, r.h});
    }
}

void sdlpp_renderer::clear_region(const rect& r, const color& bg)
{
    auto& rend = *m_pimpl->renderer;
    rend.set_draw_color(::sdlpp::color{bg.r, bg.g, bg.b, bg.a});
    rend.fill_rect(::sdlpp::rect_i{r.x, r.y, r.w, r.h});
}

void sdlpp_renderer::draw_shadow(const rect& wb, int offset_x, int offset_y)
{
    auto& rend = *m_pimpl->renderer;
    rend.set_draw_blend_mode(::sdlpp::blend_mode::blend);
    rend.set_draw_color(::sdlpp::color{0, 0, 0, 64});

    // Right shadow
    rend.fill_rect(::sdlpp::rect_i{
        wb.x + wb.w, wb.y + offset_y, offset_x, wb.h});
    // Bottom shadow
    rend.fill_rect(::sdlpp::rect_i{
        wb.x + offset_x, wb.y + wb.h, wb.w, offset_y});
}

void sdlpp_renderer::push_clip(const rect& r)
{
    // Calculate effective clip by intersecting with current clip
    rect effective_clip = r;

    if (!m_pimpl->clip_stack.empty()) {
        const auto& current = m_pimpl->clip_stack.top();

        // Intersect the new clip rect with the current one
        int x1 = std::max(r.x, current.x);
        int y1 = std::max(r.y, current.y);
        int x2 = std::min(r.x + r.w, current.x + current.w);
        int y2 = std::min(r.y + r.h, current.y + current.h);

        // Ensure non-negative dimensions
        effective_clip.x = x1;
        effective_clip.y = y1;
        effective_clip.w = std::max(0, x2 - x1);
        effective_clip.h = std::max(0, y2 - y1);
    }

    m_pimpl->clip_stack.push(effective_clip);
    m_pimpl->renderer->set_clip_rect(
        std::optional<::sdlpp::rect_i>{::sdlpp::rect_i(
            effective_clip.x, effective_clip.y,
            effective_clip.w, effective_clip.h)});
}

void sdlpp_renderer::pop_clip()
{
    if (!m_pimpl->clip_stack.empty()) {
        m_pimpl->clip_stack.pop();
    }

    if (m_pimpl->clip_stack.empty()) {
        m_pimpl->renderer->set_clip_rect(std::optional<::sdlpp::rect_i>{});
    } else {
        const auto& r = m_pimpl->clip_stack.top();
        m_pimpl->renderer->set_clip_rect(
            std::optional<::sdlpp::rect_i>{::sdlpp::rect_i(r.x, r.y, r.w, r.h)});
    }
}

rect sdlpp_renderer::get_clip_rect() const
{
    if (m_pimpl->clip_stack.empty()) {
        return m_pimpl->viewport;
    }
    return m_pimpl->clip_stack.top();
}

rect sdlpp_renderer::get_viewport() const
{
    // Always query SDL for current output size to ensure accuracy
    // This handles cases where the window was resized without an explicit resize event
    if (auto size_result = m_pimpl->renderer->get_output_size<::sdlpp::size_i>()) {
        // Update cache if different (viewport is just a cache that should reflect SDL state)
        if (size_result->width != m_pimpl->viewport.w ||
            size_result->height != m_pimpl->viewport.h) {
            const_cast<impl*>(m_pimpl.get())->viewport = rect{0, 0, size_result->width, size_result->height};
        }
        return rect{0, 0, size_result->width, size_result->height};
    }
    return m_pimpl->viewport;
}

void sdlpp_renderer::present()
{
    m_pimpl->renderer->present();
}

void sdlpp_renderer::on_resize()
{
    // Update viewport from renderer's current output size
    if (auto size_result = m_pimpl->renderer->get_output_size<::sdlpp::size_i>()) {
        m_pimpl->viewport = rect{0, 0, size_result->width, size_result->height};
    }
}

void sdlpp_renderer::take_screenshot(std::ostream& sink) const
{
    sink << "SDL++ Renderer Screenshot\n";
    sink << "Viewport: " << m_pimpl->viewport.w << "x"
         << m_pimpl->viewport.h << "\n";
}

// =================================================================
// Static Measurement Methods
// =================================================================

size sdlpp_renderer::measure_text(std::string_view text, const font& f)
{
    // Use cached glyph metrics when available for faster measurement
    return font_cache_manager::instance().measure_text_cached(dpi_scaled(f), text);
}

size sdlpp_renderer::get_icon_size(icon_style icon) noexcept
{
    switch (icon) {
        case icon_style::checkbox_unchecked:
        case icon_style::checkbox_checked:
        case icon_style::checkbox_indeterminate:
            return {13, 13};  // Windows 3.11 standard checkbox size

        // All other icons use 16x16 to match XPM artwork
        default:
            return {16, 16};
    }
}

std::size_t sdlpp_renderer::font::hash() const noexcept
{
    std::size_t h = std::hash<std::string>{}(path.string());
    h ^= std::hash<float>{}(size_px) << 1;
    h ^= std::hash<bool>{}(bold) << 2;
    h ^= std::hash<bool>{}(italic) << 3;
    h ^= std::hash<bool>{}(underline) << 4;
    return h;
}

void sdlpp_renderer::shutdown()
{
    font_cache_manager::instance().clear();
}

} // namespace onyxui::sdlpp
