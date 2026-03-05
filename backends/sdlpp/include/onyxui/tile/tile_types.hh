/**
 * @file tile_types.hh
 * @brief Core types for tile-based UI rendering
 *
 * This file defines the fundamental structures for tile/sprite-based UI:
 * - Slice types (h_slice, v_slice, nine_slice) for scalable elements
 * - tile_atlas for texture atlas management
 * - bitmap_font for pre-rendered text
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace onyxui::tile {

// ============================================================================
// Ownership Documentation Type Aliases
// ============================================================================
// These type aliases make ownership semantics explicit in the API.
// They are purely documentary - they don't change runtime behavior.

/**
 * @brief Non-owning pointer type alias for documentation purposes
 *
 * Use this to clearly indicate that a pointer is NOT owned by the containing
 * struct/class. The pointed-to object must remain valid for the lifetime
 * of the struct containing this pointer.
 *
 * @tparam T The pointed-to type
 *
 * @example
 * @code
 * struct my_struct {
 *     non_owning<tile_atlas> atlas;  // Clearly shows we don't own this
 * };
 * @endcode
 */
template<typename T>
using non_owning = T*;

/**
 * @brief Optional non-owning pointer type alias
 *
 * Use when the pointer may be null and that's an expected/valid state.
 * The containing code should check for null before use.
 */
template<typename T>
using optional_non_owning = T*;

// ============================================================================
// Basic Geometry Types
// ============================================================================

/**
 * @struct tile_rect
 * @brief Simple rectangle for tile rendering
 */
struct tile_rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    [[nodiscard]] constexpr bool operator==(const tile_rect& other) const noexcept = default;
};

/**
 * @struct tile_def
 * @brief Definition of a single tile in an atlas
 */
struct tile_def {
    int x = 0;          ///< X position in atlas
    int y = 0;          ///< Y position in atlas
    int width = 16;     ///< Tile width
    int height = 16;    ///< Tile height
};

// ============================================================================
// Slice Types
// ============================================================================

/**
 * @struct h_slice
 * @brief Horizontal 3-part slice for scalable horizontal elements
 *
 * Used for progress bars, horizontal scrollbar tracks, tabs, etc.
 *
 * ```
 * ┌───┬─────────────┬───┐
 * │ L │   center    │ R │
 * └───┴─────────────┴───┘
 * ```
 */
struct v_slice;  // Forward declaration

struct h_slice {
    int left{-1};       ///< Left cap tile ID (-1 = none)
    int center{-1};     ///< Middle tile ID (stretched or tiled)
    int right{-1};      ///< Right cap tile ID (-1 = none)
    int margin{0};      ///< Cap width in pixels

    /// Check if this slice is valid (has at least a center tile)
    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return center >= 0;
    }

    /// Convert to vertical slice (left->top, right->bottom)
    [[nodiscard]] constexpr v_slice to_v_slice() const noexcept;
};

/**
 * @struct v_slice
 * @brief Vertical 3-part slice for scalable vertical elements
 *
 * Used for vertical scrollbar thumbs, vertical progress bars, etc.
 *
 * ```
 * ┌───┐
 * │ T │
 * ├───┤
 * │ C │  <- center (stretched or tiled)
 * ├───┤
 * │ B │
 * └───┘
 * ```
 */
struct v_slice {
    int top{-1};        ///< Top cap tile ID (-1 = none)
    int center{-1};     ///< Middle tile ID (stretched or tiled)
    int bottom{-1};     ///< Bottom cap tile ID (-1 = none)
    int margin{0};      ///< Cap height in pixels

    /// Check if this slice is valid (has at least a center tile)
    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return center >= 0;
    }
};

// Implementation of h_slice::to_v_slice() (after v_slice is defined)
inline constexpr v_slice h_slice::to_v_slice() const noexcept {
    return v_slice{left, center, right, margin};
}

/**
 * @struct nine_slice
 * @brief 9-part slice for fully scalable rectangular elements
 *
 * Used for panels, buttons, windows, text inputs, etc.
 *
 * ```
 * ┌───┬─────────┬───┐
 * │ 0 │    1    │ 2 │
 * ├───┼─────────┼───┤
 * │ 3 │    4    │ 5 │
 * ├───┼─────────┼───┤
 * │ 6 │    7    │ 8 │
 * └───┴─────────┴───┘
 * ```
 *
 * Corners (0,2,6,8) stay fixed size.
 * Edges (1,3,5,7) stretch in one direction.
 * Center (4) stretches in both directions.
 */
struct nine_slice {
    // Top row
    int top_left{-1};       ///< Corner tile ID
    int top{-1};            ///< Top edge tile ID
    int top_right{-1};      ///< Corner tile ID

    // Middle row
    int left{-1};           ///< Left edge tile ID
    int center{-1};         ///< Center tile ID (can be -1 for transparent)
    int right{-1};          ///< Right edge tile ID

    // Bottom row
    int bottom_left{-1};    ///< Corner tile ID
    int bottom{-1};         ///< Bottom edge tile ID
    int bottom_right{-1};   ///< Corner tile ID

    // Margins
    int margin_h{0};        ///< Left/right margin in pixels
    int margin_v{0};        ///< Top/bottom margin in pixels

    /// Check if this slice is valid (has corner tiles)
    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return top_left >= 0 && top_right >= 0 &&
               bottom_left >= 0 && bottom_right >= 0;
    }

    /// Check if center should be rendered
    [[nodiscard]] constexpr bool has_center() const noexcept {
        return center >= 0;
    }
};

// ============================================================================
// Tile Atlas
// ============================================================================

/**
 * @enum texture_type
 * @brief Identifies the type of texture stored in tile_atlas
 *
 * This tag enables runtime validation to prevent UB from mismatched casts.
 * Each backend sets the appropriate type when creating the atlas.
 *
 * @warning Always set tex_type when storing a texture pointer!
 * Failure to do so will cause validation failures in debug builds
 * and potential silent UB in release builds.
 */
enum class texture_type : std::uint8_t {
    unknown = 0,    ///< Type not specified (legacy/testing) - triggers warnings
    sdlpp = 1,      ///< sdlpp::texture* from lib_sdlpp
    sdl_raw = 2,    ///< Raw SDL_Texture* from SDL3
};

/**
 * @brief Get human-readable name for texture type
 * @param type The texture type
 * @return String name of the type
 */
[[nodiscard]] inline constexpr const char* texture_type_name(texture_type type) noexcept {
    switch (type) {
        case texture_type::unknown: return "unknown";
        case texture_type::sdlpp: return "sdlpp";
        case texture_type::sdl_raw: return "sdl_raw";
        default: return "invalid";
    }
}

/**
 * @brief Validate texture type matches expected type
 * @param actual The actual texture type stored in atlas
 * @param expected The expected texture type for the backend
 * @return true if types match (or actual is unknown for legacy compatibility)
 *
 * @note In release builds with NDEBUG defined, this always returns true for
 * maximum performance. Type safety is enforced via assertions in debug builds.
 *
 * @code
 * // Typical usage in renderer:
 * if (!validate_texture_type(atlas->tex_type, texture_type::sdlpp)) {
 *     // Log error and return early
 *     return;
 * }
 * auto* sdlpp_tex = static_cast<sdlpp::texture*>(atlas->texture);
 * @endcode
 */
[[nodiscard]] inline constexpr bool validate_texture_type(
    texture_type actual,
    texture_type expected) noexcept
{
#ifdef NDEBUG
    // In release builds, trust the programmer (max performance)
    (void)actual;
    (void)expected;
    return true;
#else
    // In debug builds, validate strictly
    // Allow unknown for backwards compatibility, but prefer explicit types
    return actual == expected || actual == texture_type::unknown;
#endif
}

/**
 * @struct tile_atlas
 * @brief Texture atlas containing UI tiles organized in a grid
 *
 * The atlas is a texture divided into equal-sized tiles arranged in rows.
 * Tiles are indexed by ID starting from 0 (top-left), incrementing
 * left-to-right, top-to-bottom.
 *
 * Example layout (4 columns):
 * ```
 * ┌───┬───┬───┬───┐
 * │ 0 │ 1 │ 2 │ 3 │
 * ├───┼───┼───┼───┤
 * │ 4 │ 5 │ 6 │ 7 │
 * ├───┼───┼───┼───┤
 * │ 8 │ 9 │10 │11 │
 * └───┴───┴───┴───┘
 * ```
 *
 * @warning OWNERSHIP: The texture pointer is NOT owned by this struct.
 * The caller is responsible for:
 * 1. Creating the texture before using the atlas
 * 2. Keeping the texture valid for the lifetime of the atlas
 * 3. Destroying the texture after the atlas is no longer used
 *
 * @note Always call is_valid() before using source_x/source_y/column/row
 * methods to avoid undefined behavior from division by zero.
 */
struct tile_atlas {
    /**
     * @brief Backend-specific texture handle
     *
     * @warning NOT OWNED: This is a non-owning pointer. The caller must:
     * - Create and manage the texture's lifetime externally
     * - Ensure the texture remains valid while the atlas is in use
     * - Destroy the texture after the atlas is no longer needed
     *
     * @note TYPE DEPENDS ON BACKEND: The actual type stored here depends
     * on which backend you're using:
     * - **sdlpp_tile_backend**: Use `sdlpp::texture*` (NOT raw SDL_Texture*)
     * - **Raw SDL backend**: Use `SDL_Texture*`
     *
     * Set tex_type appropriately to enable runtime type validation.
     * The tile_renderer will validate tex_type before casting.
     *
     * Example usage with sdlpp backend:
     * @code
     * // Create texture using sdlpp
     * auto tex_result = renderer.create_texture_from_surface(surface);
     * sdlpp::texture tex = std::move(*tex_result);
     *
     * // Store pointer to sdlpp::texture with type tag
     * tile_atlas atlas;
     * atlas.texture = &tex;
     * atlas.tex_type = texture_type::sdlpp;
     * atlas.tile_width = atlas.tile_height = 16;
     * atlas.columns = 16;
     * @endcode
     *
     * Example with raw SDL (if using raw SDL backend):
     * @code
     * SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
     * tile_atlas atlas;
     * atlas.texture = tex;
     * atlas.tex_type = texture_type::sdl_raw;
     * @endcode
     */
    void* texture{nullptr};

    /**
     * @brief Type tag for runtime validation
     *
     * Set this when storing the texture pointer to enable type checking
     * before casting. Renderers should validate this matches their expected type.
     */
    texture_type tex_type{texture_type::unknown};
    int tile_width{16};         ///< Width of each tile in pixels (must be > 0)
    int tile_height{16};        ///< Height of each tile in pixels (must be > 0)
    int columns{16};            ///< Number of tiles per row in atlas (must be > 0)

    /**
     * @brief Calculate source X coordinate for a tile ID
     * @param tile_id The tile index (0-based)
     * @return X pixel coordinate in the atlas texture, or 0 if columns <= 0
     *
     * @pre is_valid() should be true for meaningful results
     */
    [[nodiscard]] constexpr int source_x(int tile_id) const noexcept {
        if (columns <= 0) return 0;  // Prevent division by zero
        return (tile_id % columns) * tile_width;
    }

    /**
     * @brief Calculate source Y coordinate for a tile ID
     * @param tile_id The tile index (0-based)
     * @return Y pixel coordinate in the atlas texture, or 0 if columns <= 0
     *
     * @pre is_valid() should be true for meaningful results
     */
    [[nodiscard]] constexpr int source_y(int tile_id) const noexcept {
        if (columns <= 0) return 0;  // Prevent division by zero
        return (tile_id / columns) * tile_height;
    }

    /**
     * @brief Get the column index for a tile ID
     * @param tile_id The tile index (0-based)
     * @return Column number (0-based), or 0 if columns <= 0
     *
     * @pre is_valid() should be true for meaningful results
     */
    [[nodiscard]] constexpr int column(int tile_id) const noexcept {
        if (columns <= 0) return 0;  // Prevent division by zero
        return tile_id % columns;
    }

    /**
     * @brief Get the row index for a tile ID
     * @param tile_id The tile index (0-based)
     * @return Row number (0-based), or 0 if columns <= 0
     *
     * @pre is_valid() should be true for meaningful results
     */
    [[nodiscard]] constexpr int row(int tile_id) const noexcept {
        if (columns <= 0) return 0;  // Prevent division by zero
        return tile_id / columns;
    }

    /**
     * @brief Check if the atlas is valid for rendering
     * @return true if atlas has valid dimensions (all > 0)
     *
     * @note A valid atlas may still have a null texture if not yet loaded.
     *       For full rendering validity, also check texture != nullptr.
     *
     * Always check this before using source_x/source_y/column/row to ensure
     * safe operation without division by zero.
     */
    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return tile_width > 0 && tile_height > 0 && columns > 0;
    }

    /**
     * @brief Check if the atlas is fully ready for rendering
     * @return true if atlas has valid dimensions AND a non-null texture
     */
    [[nodiscard]] constexpr bool is_ready() const noexcept {
        return is_valid() && texture != nullptr;
    }
};

// ============================================================================
// Bitmap Font
// ============================================================================

/**
 * @struct bitmap_font
 * @brief Pre-rendered font using tiles from an atlas
 *
 * Bitmap fonts have fixed-width glyphs stored sequentially in the atlas.
 * Colors are baked into the font tiles - no runtime tinting.
 *
 * For multiple text colors, create multiple bitmap_font instances
 * pointing to different pre-colored glyph sets in the atlas.
 *
 * ASCII character mapping:
 * - first_char is typically 32 (space)
 * - Characters map to tiles: tile_id = first_tile_id + (char - first_char)
 *
 * @warning OWNERSHIP: The atlas pointer is NOT owned by this struct.
 * The atlas must remain valid for the lifetime of this bitmap_font.
 *
 * Example usage:
 * @code
 * bitmap_font font{&my_atlas, 8, 8, 32, 96, 0};
 *
 * // Safe way to render text:
 * for (char ch : text) {
 *     int tile = font.glyph_id(ch);
 *     if (tile >= 0) {  // ALWAYS check for -1!
 *         renderer.draw_tile(tile, x, y);
 *     }
 *     x += font.glyph_width;
 * }
 *
 * // Or use has_glyph() first:
 * if (font.has_glyph(ch)) {
 *     renderer.draw_tile(font.glyph_id(ch), x, y);
 * }
 * @endcode
 */
struct bitmap_font {
    /**
     * @brief Font atlas (can share with UI atlas)
     *
     * @warning NOT OWNED: This is a non-owning pointer. The atlas must
     * remain valid for the lifetime of this bitmap_font instance.
     *
     * @see non_owning for ownership semantics
     */
    optional_non_owning<tile_atlas> atlas{nullptr};
    int glyph_width{8};             ///< Fixed glyph width in pixels (must be > 0)
    int glyph_height{8};            ///< Glyph height in pixels (must be > 0)
    int first_char{32};             ///< First ASCII character (typically 32 = space)
    int char_count{96};             ///< Number of characters in font (typically 96: space to ~)
    int first_tile_id{0};           ///< Tile ID of first character in atlas

    /**
     * @brief Get the tile ID for a character
     * @param ch ASCII character
     * @return Tile ID in the atlas, or -1 if character not in font
     *
     * @warning ALWAYS check the return value! A return of -1 means the
     * character is not in this font and should not be rendered.
     *
     * @code
     * int tile = font.glyph_id(ch);
     * if (tile >= 0) {  // Character is valid
     *     renderer.draw_tile(tile, x, y);
     * } else {
     *     // Character not in font - skip or use fallback
     * }
     * @endcode
     */
    [[nodiscard]] constexpr int glyph_id(char ch) const noexcept {
        int index = static_cast<unsigned char>(ch) - first_char;
        if (index < 0 || index >= char_count) {
            return -1;  // Character not in font
        }
        return first_tile_id + index;
    }

    /**
     * @brief Safely get the tile ID for a character with fallback
     * @param ch ASCII character
     * @param fallback Tile ID to return if character not in font
     * @return Tile ID in the atlas, or fallback if character not in font
     *
     * This is a safer alternative to glyph_id() that never returns -1.
     * Useful when you have a designated "missing character" glyph.
     *
     * @code
     * const int MISSING_GLYPH = 127;  // Last character often used as placeholder
     * int tile = font.glyph_id_or(ch, MISSING_GLYPH);
     * renderer.draw_tile(tile, x, y);  // Always safe
     * @endcode
     */
    [[nodiscard]] constexpr int glyph_id_or(char ch, int fallback) const noexcept {
        int id = glyph_id(ch);
        return id >= 0 ? id : fallback;
    }

    /**
     * @brief Check if a character is in this font
     * @param ch ASCII character
     * @return true if character has a glyph
     *
     * Use this to check before calling glyph_id() if you want to handle
     * missing characters differently.
     */
    [[nodiscard]] constexpr bool has_glyph(char ch) const noexcept {
        int index = static_cast<unsigned char>(ch) - first_char;
        return index >= 0 && index < char_count;
    }

    /**
     * @brief Calculate text width in pixels (single line)
     * @param text Text string (should not contain newlines)
     * @return Width in pixels
     *
     * @note For multiline text, use text_size() instead.
     * @note For very long strings (>2 billion chars), this may truncate.
     *       In practice, UI text is never this long.
     */
    [[nodiscard]] constexpr int text_width(std::string_view text) const noexcept {
        // Prevent overflow for extremely long strings (theoretical safety)
        constexpr std::size_t MAX_SAFE_LEN = static_cast<std::size_t>(INT32_MAX);
        std::size_t len = text.length() < MAX_SAFE_LEN ? text.length() : MAX_SAFE_LEN;
        return static_cast<int>(len) * glyph_width;
    }

    /**
     * @brief Calculate text size with multiline support
     * @param text Text string (may contain \\n for line breaks)
     * @param line_spacing Extra pixels between lines (default 0)
     * @return Size {max_line_width, total_height}
     *
     * Handles newlines by:
     * - Width = maximum width of any line
     * - Height = (line_count * glyph_height) + ((line_count - 1) * line_spacing)
     */
    [[nodiscard]] constexpr std::pair<int, int> text_size(
        std::string_view text,
        int line_spacing = 0) const noexcept
    {
        if (text.empty()) {
            return {0, glyph_height};  // Empty text still has height
        }

        int max_width = 0;
        int current_line_width = 0;
        int line_count = 1;

        for (char ch : text) {
            if (ch == '\n') {
                max_width = current_line_width > max_width ? current_line_width : max_width;
                current_line_width = 0;
                ++line_count;
            } else {
                current_line_width += glyph_width;
            }
        }
        // Don't forget the last line
        max_width = current_line_width > max_width ? current_line_width : max_width;

        int total_height = line_count * glyph_height;
        if (line_count > 1) {
            total_height += (line_count - 1) * line_spacing;
        }

        return {max_width, total_height};
    }

    /**
     * @brief Check if the font is valid for rendering
     * @return true if font has valid configuration
     *
     * Checks that:
     * - Atlas pointer is not null
     * - Atlas is valid (dimensions > 0)
     * - Glyph dimensions are positive
     * - At least one character is defined
     */
    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return atlas != nullptr && atlas->is_valid() &&
               glyph_width > 0 && glyph_height > 0 && char_count > 0;
    }
};

// ============================================================================
// Text Measurement Cache
// ============================================================================

/**
 * @class text_measurement_cache
 * @brief LRU cache for text size measurements
 *
 * For UIs that display the same text repeatedly (labels, buttons),
 * caching text measurements can significantly reduce CPU usage during
 * layout passes.
 *
 * @example
 * @code
 * text_measurement_cache cache(64);  // Cache up to 64 measurements
 *
 * // In your widget's measure function:
 * auto cached = cache.get(m_text, m_font.glyph_width);
 * if (cached) {
 *     return *cached;
 * }
 * auto size = m_font.text_size(m_text);
 * cache.put(m_text, m_font.glyph_width, size);
 * return size;
 * @endcode
 */
class text_measurement_cache {
public:
    /// Cache entry with LRU tracking
    struct entry {
        std::pair<int, int> size;  ///< {width, height}
        uint64_t last_access;      ///< LRU timestamp
    };

    /**
     * @brief Create cache with specified capacity
     * @param max_entries Maximum number of entries to cache
     */
    explicit text_measurement_cache(std::size_t max_entries = 64)
        : m_max_entries(max_entries)
    {}

    /**
     * @brief Look up cached measurement
     * @param text The text string
     * @param glyph_width Font glyph width (used as part of key)
     * @return Cached size if found, std::nullopt otherwise
     */
    [[nodiscard]] std::optional<std::pair<int, int>> get(
        std::string_view text,
        int glyph_width) noexcept
    {
        auto key = make_key(text, glyph_width);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            it->second.last_access = ++m_access_counter;
            return it->second.size;
        }
        return std::nullopt;
    }

    /**
     * @brief Store measurement in cache
     * @param text The text string
     * @param glyph_width Font glyph width (used as part of key)
     * @param size The measured size {width, height}
     */
    void put(std::string_view text, int glyph_width, std::pair<int, int> size) {
        auto key = make_key(text, glyph_width);

        // Evict LRU entry if at capacity
        if (m_cache.size() >= m_max_entries) {
            evict_lru();
        }

        m_cache[key] = {size, ++m_access_counter};
    }

    /**
     * @brief Clear all cached entries
     */
    void clear() noexcept {
        m_cache.clear();
    }

    /**
     * @brief Get current cache size
     */
    [[nodiscard]] std::size_t size() const noexcept {
        return m_cache.size();
    }

    /**
     * @brief Invalidate entries for specific text
     * @param text Text to invalidate (all glyph widths)
     *
     * Use this when text content changes and cached measurements are stale.
     */
    void invalidate(std::string_view text) {
        // Erase all entries with matching text (any glyph width)
        // This is O(n) but invalidation is rare
        for (auto it = m_cache.begin(); it != m_cache.end(); ) {
            // Key format: "width:text" - check if suffix matches
            if (it->first.size() > text.size() &&
                it->first.substr(it->first.size() - text.size()) == text) {
                it = m_cache.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    using cache_map = std::unordered_map<std::string, entry>;

    [[nodiscard]] static std::string make_key(std::string_view text, int glyph_width) {
        // Simple key: "width:text"
        std::string key;
        key.reserve(12 + text.size());
        key += std::to_string(glyph_width);
        key += ':';
        key += text;
        return key;
    }

    void evict_lru() {
        if (m_cache.empty()) return;

        auto lru_it = m_cache.begin();
        uint64_t min_access = lru_it->second.last_access;

        for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
            if (it->second.last_access < min_access) {
                min_access = it->second.last_access;
                lru_it = it;
            }
        }

        m_cache.erase(lru_it);
    }

    cache_map m_cache;
    std::size_t m_max_entries;
    uint64_t m_access_counter = 0;
};

} // namespace onyxui::tile
