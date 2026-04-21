/**
 * @file font_source_provider.hh
 * @brief Hook point for applications to resolve named fonts without
 *        going through the filesystem.
 *
 * Usage:
 *   - sdlpp_renderer's font_cache_manager consults the currently-
 *     registered provider whenever it builds a cache entry for a
 *     sdlpp_renderer::font.
 *   - If the provider returns a non-null bitmap_font, the cache uses
 *     it directly (no file I/O). Lifetime of the returned pointer is
 *     the provider's concern — it must outlive all active caches.
 *   - If the provider returns nullptr, the cache falls through to the
 *     path-based TTF loader (unchanged default behaviour).
 *
 * Default provider: only knows `font_set == "bios"` and returns
 * `&onyx_font::bios_font_8x16()` for that. Install your own provider
 * via set_font_source_provider() to hook into an asset registry.
 */

#pragma once

#include <onyxui/sdlpp/sdlpp_renderer.hh>
#include <onyx_font/bitmap_font.hh>
#include <memory>

namespace onyxui::sdlpp {

struct font_source_provider {
    virtual ~font_source_provider() = default;

    /**
     * @brief Resolve a font spec to a bitmap_font, or defer.
     *
     * @param f Font spec (path / font_set / font_name / size / style flags).
     * @return Pointer to a bitmap_font owned by the provider, or nullptr
     *         to defer to the default path-based TTF loader. The returned
     *         pointer must remain valid until the renderer shuts down.
     */
    [[nodiscard]] virtual const onyx_font::bitmap_font* find_bitmap(
        const sdlpp_renderer::font& f) = 0;
};

/**
 * @brief The default provider: knows only the built-in "bios" font set.
 *
 * For `font_set == "bios"` it returns &onyx_font::bios_font_8x16().
 * Everything else returns nullptr (deferred to path-based loader).
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
