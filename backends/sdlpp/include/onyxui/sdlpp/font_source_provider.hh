/**
 * @file font_source_provider.hh
 * @brief Hook point for applications to resolve named fonts.
 *
 * sdlpp_renderer's font_cache_manager consults the currently-registered
 * provider whenever it builds a cache entry for a sdlpp_renderer::font.
 * The provider returns a fully-formed `onyx_font::font_source` (which
 * owns whatever's underneath — bitmap reference, vector reference, or
 * TTF bytes + freetype rasterizer). The renderer never sees the
 * underlying font type; it only consumes the font_source through the
 * onyx_font rasterization API.
 *
 * If the provider returns `std::nullopt`, the renderer treats the font
 * spec as unresolvable and skips text rendering for that draw. There is
 * no built-in file-TTF fallback inside the renderer any more — that
 * policy belongs in the provider. See `onyx_font::load_ttf_file` (when
 * NEUTRINO_ONYX_FONT_LOADER_TTF is enabled) for a ready-made helper to
 * compose into custom providers.
 *
 * Default provider: only knows `font_set == "bios"` and returns a
 * font_source wrapping `onyx_font::bios_font_8x16()` for that. Install
 * your own provider via set_font_source_provider() to hook into an
 * asset registry.
 */

#pragma once

#include <onyxui/sdlpp/sdlpp_renderer.hh>
#include <onyx_font/text/font_source.hh>
#include <memory>
#include <optional>

namespace onyxui::sdlpp {

struct font_source_provider {
    virtual ~font_source_provider() = default;

    /**
     * @brief Resolve a font spec to a font_source.
     *
     * @param f Font spec (path / font_set / font_name / size / style flags).
     * @return A font_source for `f`, or `std::nullopt` if this provider
     *         cannot resolve the spec. The returned font_source is owned
     *         by the caller (the renderer's font cache).
     */
    [[nodiscard]] virtual std::optional<onyx_font::font_source> resolve(
        const sdlpp_renderer::font& f) = 0;
};

/**
 * @brief The default provider: knows only the built-in "bios" font set.
 *
 * For `font_set == "bios"` it returns a font_source wrapping
 * `onyx_font::bios_font_8x16()`. Everything else returns nullopt.
 */
[[nodiscard]] std::unique_ptr<font_source_provider> make_default_font_provider();

/**
 * @brief Install a font source provider for sdlpp_renderer.
 *
 * Thread-unsafe; call once during backend initialization.
 * Passing nullptr re-installs the default provider.
 */
void set_font_source_provider(std::unique_ptr<font_source_provider> provider);

/**
 * @brief Access the currently registered provider (never nullptr once the
 *        backend has been initialised — a default is installed if none
 *        has been set).
 */
[[nodiscard]] font_source_provider& current_font_source_provider();

} // namespace onyxui::sdlpp
