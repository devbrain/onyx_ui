/**
 * @file tile_optimization.hh
 * @brief Optimization utilities for tile-based rendering
 *
 * This file provides performance optimization tools for tile-based UIs:
 *
 * ## Draw Batching
 * Groups multiple tile draw operations into batched calls to reduce
 * GPU state changes and draw call overhead.
 *
 * ## Dirty Region Tracking
 * Tracks which areas of the UI have changed to enable incremental
 * rendering instead of full redraws.
 *
 * ## Tile Cache
 * Caches pre-rendered tile combinations for complex widgets that
 * don't change frequently.
 *
 * ## Usage Example
 * @code
 * // Create a sprite batch for efficient rendering
 * sprite_batch<Backend> batch(atlas);
 *
 * // Queue multiple sprites
 * batch.add(tile1, {0, 0, 16, 16});
 * batch.add(tile2, {16, 0, 16, 16});
 * batch.add(tile3, {32, 0, 16, 16});
 *
 * // Render all at once
 * batch.flush(renderer);
 *
 * // Use dirty regions for incremental updates
 * dirty_region_tracker tracker(800, 600);
 * tracker.mark_dirty({100, 100, 50, 50});
 *
 * if (tracker.has_dirty_regions()) {
 *     for (const auto& region : tracker.get_dirty_regions()) {
 *         render_region(region);
 *     }
 *     tracker.clear();
 * }
 * @endcode
 */

#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>

#include <onyxui/tile/tile_types.hh>

namespace onyxui::tile {

// ============================================================================
// Rendering-Specific Slice Types (pixel-based, not tile-ID based)
// ============================================================================

/**
 * @brief Horizontal slice for sprite batching (pixel coordinates)
 *
 * Unlike h_slice which uses tile IDs, this uses actual pixel rectangles
 * for direct rendering.
 */
struct render_h_slice {
    int x = 0;              ///< X position in atlas
    int y = 0;              ///< Y position in atlas
    int left_width = 0;     ///< Left cap width
    int middle_width = 0;   ///< Middle section source width
    int right_width = 0;    ///< Right cap width
    int height = 0;         ///< Total height
};

/**
 * @brief Vertical slice for sprite batching (pixel coordinates)
 */
struct render_v_slice {
    int x = 0;              ///< X position in atlas
    int y = 0;              ///< Y position in atlas
    int width = 0;          ///< Total width
    int top_height = 0;     ///< Top cap height
    int middle_height = 0;  ///< Middle section source height
    int bottom_height = 0;  ///< Bottom cap height
};

/**
 * @brief Nine-slice for sprite batching (pixel coordinates)
 */
struct render_nine_slice {
    int x = 0;              ///< X position in atlas
    int y = 0;              ///< Y position in atlas
    int width = 0;          ///< Total width in atlas
    int height = 0;         ///< Total height in atlas
    int left_width = 0;     ///< Left column width
    int right_width = 0;    ///< Right column width
    int top_height = 0;     ///< Top row height
    int bottom_height = 0;  ///< Bottom row height
};

// ============================================================================
// Sprite Batch - Batches draw calls for same texture
// ============================================================================

/**
 * @brief A single sprite instance in the batch
 */
struct sprite_instance {
    tile_rect src_rect;      ///< Source rectangle in atlas
    tile_rect dest_rect;     ///< Destination rectangle on screen
    float rotation = 0.0f;   ///< Rotation in radians (optional)
    float scale_x = 1.0f;    ///< Horizontal scale
    float scale_y = 1.0f;    ///< Vertical scale
    uint8_t alpha = 255;     ///< Alpha transparency
    bool flip_h = false;     ///< Horizontal flip
    bool flip_v = false;     ///< Vertical flip
};

/**
 * @brief Batches multiple sprite draws for efficient rendering
 *
 * Collects sprite draw commands and submits them in a single batch
 * to minimize GPU state changes and draw calls.
 *
 * @tparam Backend The UI backend type
 */
template<typename Backend>
class sprite_batch {
public:
    static constexpr std::size_t DEFAULT_CAPACITY = 1024;

    explicit sprite_batch(std::size_t initial_capacity = DEFAULT_CAPACITY) {
        m_sprites.reserve(initial_capacity);
    }

    /**
     * @brief Add a simple sprite to the batch
     */
    void add(const tile_rect& src, const tile_rect& dest) {
        m_sprites.push_back({src, dest});
    }

    /**
     * @brief Add a sprite with full transform options
     */
    void add(const sprite_instance& sprite) {
        m_sprites.push_back(sprite);
    }

    /**
     * @brief Add a tile from the atlas
     */
    void add_tile(const tile_def& tile, int x, int y) {
        m_sprites.push_back({
            {tile.x, tile.y, tile.width, tile.height},
            {x, y, tile.width, tile.height}
        });
    }

    /**
     * @brief Add a tile with custom destination size
     */
    void add_tile(const tile_def& tile, const tile_rect& dest) {
        m_sprites.push_back({
            {tile.x, tile.y, tile.width, tile.height},
            dest
        });
    }

    /**
     * @brief Add a horizontal slice (stretches middle)
     */
    void add_h_slice(const render_h_slice& slice, int x, int y, int width) {
        if (width <= 0) return;

        const int left_w = slice.left_width;
        const int right_w = slice.right_width;
        const int middle_w = width - left_w - right_w;
        const int h = slice.height;

        // Left cap
        if (left_w > 0) {
            m_sprites.push_back({
                {slice.x, slice.y, left_w, h},
                {x, y, left_w, h}
            });
        }

        // Middle (stretched)
        if (middle_w > 0) {
            m_sprites.push_back({
                {slice.x + left_w, slice.y, slice.middle_width, h},
                {x + left_w, y, middle_w, h}
            });
        }

        // Right cap
        if (right_w > 0) {
            m_sprites.push_back({
                {slice.x + left_w + slice.middle_width, slice.y, right_w, h},
                {x + left_w + middle_w, y, right_w, h}
            });
        }
    }

    /**
     * @brief Add a vertical slice (stretches middle)
     */
    void add_v_slice(const render_v_slice& slice, int x, int y, int height) {
        if (height <= 0) return;

        const int top_h = slice.top_height;
        const int bottom_h = slice.bottom_height;
        const int middle_h = height - top_h - bottom_h;
        const int w = slice.width;

        // Top cap
        if (top_h > 0) {
            m_sprites.push_back({
                {slice.x, slice.y, w, top_h},
                {x, y, w, top_h}
            });
        }

        // Middle (stretched)
        if (middle_h > 0) {
            m_sprites.push_back({
                {slice.x, slice.y + top_h, w, slice.middle_height},
                {x, y + top_h, w, middle_h}
            });
        }

        // Bottom cap
        if (bottom_h > 0) {
            m_sprites.push_back({
                {slice.x, slice.y + top_h + slice.middle_height, w, bottom_h},
                {x, y + top_h + middle_h, w, bottom_h}
            });
        }
    }

    /**
     * @brief Add a nine-slice (stretches middle in both directions)
     */
    void add_nine_slice(const render_nine_slice& slice, const tile_rect& dest) {
        if (dest.width <= 0 || dest.height <= 0) return;

        const int left = slice.left_width;
        const int right = slice.right_width;
        const int top = slice.top_height;
        const int bottom = slice.bottom_height;

        const int middle_w = dest.width - left - right;
        const int middle_h = dest.height - top - bottom;

        const int src_middle_w = slice.width - left - right;
        const int src_middle_h = slice.height - top - bottom;

        // Top row
        if (top > 0) {
            // Top-left corner
            if (left > 0) {
                m_sprites.push_back({
                    {slice.x, slice.y, left, top},
                    {dest.x, dest.y, left, top}
                });
            }
            // Top-middle (stretch horizontal)
            if (middle_w > 0 && src_middle_w > 0) {
                m_sprites.push_back({
                    {slice.x + left, slice.y, src_middle_w, top},
                    {dest.x + left, dest.y, middle_w, top}
                });
            }
            // Top-right corner
            if (right > 0) {
                m_sprites.push_back({
                    {slice.x + left + src_middle_w, slice.y, right, top},
                    {dest.x + left + middle_w, dest.y, right, top}
                });
            }
        }

        // Middle row
        if (middle_h > 0 && src_middle_h > 0) {
            // Middle-left (stretch vertical)
            if (left > 0) {
                m_sprites.push_back({
                    {slice.x, slice.y + top, left, src_middle_h},
                    {dest.x, dest.y + top, left, middle_h}
                });
            }
            // Center (stretch both)
            if (middle_w > 0 && src_middle_w > 0) {
                m_sprites.push_back({
                    {slice.x + left, slice.y + top, src_middle_w, src_middle_h},
                    {dest.x + left, dest.y + top, middle_w, middle_h}
                });
            }
            // Middle-right (stretch vertical)
            if (right > 0) {
                m_sprites.push_back({
                    {slice.x + left + src_middle_w, slice.y + top, right, src_middle_h},
                    {dest.x + left + middle_w, dest.y + top, right, middle_h}
                });
            }
        }

        // Bottom row
        if (bottom > 0) {
            // Bottom-left corner
            if (left > 0) {
                m_sprites.push_back({
                    {slice.x, slice.y + top + src_middle_h, left, bottom},
                    {dest.x, dest.y + top + middle_h, left, bottom}
                });
            }
            // Bottom-middle (stretch horizontal)
            if (middle_w > 0 && src_middle_w > 0) {
                m_sprites.push_back({
                    {slice.x + left, slice.y + top + src_middle_h, src_middle_w, bottom},
                    {dest.x + left, dest.y + top + middle_h, middle_w, bottom}
                });
            }
            // Bottom-right corner
            if (right > 0) {
                m_sprites.push_back({
                    {slice.x + left + src_middle_w, slice.y + top + src_middle_h, right, bottom},
                    {dest.x + left + middle_w, dest.y + top + middle_h, right, bottom}
                });
            }
        }
    }

    /**
     * @brief Get number of sprites in batch
     */
    [[nodiscard]] std::size_t size() const noexcept {
        return m_sprites.size();
    }

    /**
     * @brief Check if batch is empty
     */
    [[nodiscard]] bool empty() const noexcept {
        return m_sprites.empty();
    }

    /**
     * @brief Clear all sprites from batch
     */
    void clear() noexcept {
        m_sprites.clear();
    }

    /**
     * @brief Get the sprite data for rendering
     */
    [[nodiscard]] const std::vector<sprite_instance>& sprites() const noexcept {
        return m_sprites;
    }

    /**
     * @brief Sort sprites by Y coordinate (painter's algorithm)
     */
    void sort_by_y() {
        std::sort(m_sprites.begin(), m_sprites.end(),
            [](const sprite_instance& a, const sprite_instance& b) {
                return a.dest_rect.y < b.dest_rect.y;
            });
    }

    /**
     * @brief Sort sprites by source Y (texture locality)
     */
    void sort_by_texture_locality() {
        std::sort(m_sprites.begin(), m_sprites.end(),
            [](const sprite_instance& a, const sprite_instance& b) {
                if (a.src_rect.y != b.src_rect.y) {
                    return a.src_rect.y < b.src_rect.y;
                }
                return a.src_rect.x < b.src_rect.x;
            });
    }

    /**
     * @brief Submit batch to tile_renderer for actual rendering
     * @param renderer The tile renderer to draw with
     *
     * This renders all queued sprites to the renderer. After submission,
     * the batch is NOT automatically cleared - call clear() manually if needed.
     *
     * @note Currently renders each sprite individually. Future versions may
     * implement true GPU batching using SDL3 geometry APIs.
     *
     * ## Source Rectangle Handling
     *
     * The submit function handles two cases:
     * 1. **Grid-aligned sprites**: If src_rect aligns exactly to tile grid,
     *    uses efficient tile_id-based rendering
     * 2. **Non-aligned sprites**: Falls back to direct source rect rendering
     *    (requires renderer support for draw_tile_region)
     *
     * Example:
     * @code
     * sprite_batch<Backend> batch;
     * batch.add_tile(button_tile, {10, 10, 100, 30});
     * batch.add_nine_slice(panel_slice, {0, 0, 200, 150});
     * batch.submit(renderer);  // Renders all sprites
     * batch.clear();           // Ready for next frame
     * @endcode
     */
    template<typename TileRenderer>
    void submit(TileRenderer& renderer) {
        auto* atlas = renderer.get_atlas();
        if (!atlas || atlas->tile_width <= 0 || atlas->tile_height <= 0) {
            return;  // Can't render without valid atlas
        }

        for (const auto& sprite : m_sprites) {
            // Convert tile_rect to renderer's rect type
            typename TileRenderer::rect dest{
                sprite.dest_rect.x,
                sprite.dest_rect.y,
                sprite.dest_rect.width,
                sprite.dest_rect.height
            };

            // Check if source rect is grid-aligned
            const bool x_aligned = (sprite.src_rect.x % atlas->tile_width) == 0;
            const bool y_aligned = (sprite.src_rect.y % atlas->tile_height) == 0;
            const bool size_matches = sprite.src_rect.width == atlas->tile_width &&
                                     sprite.src_rect.height == atlas->tile_height;

            if (x_aligned && y_aligned && size_matches) {
                // Fast path: grid-aligned sprite, use tile_id
                int col = sprite.src_rect.x / atlas->tile_width;
                int row = sprite.src_rect.y / atlas->tile_height;
                int tile_id = row * atlas->columns + col;
                renderer.draw_tile_scaled(tile_id, dest);
            } else {
                // Slow path: non-aligned or different size
                // Use direct source rect if renderer supports it
                // For now, approximate using closest tile
                // TODO: Add draw_region(src_rect, dest_rect) to tile_renderer
                int col = sprite.src_rect.x / atlas->tile_width;
                int row = sprite.src_rect.y / atlas->tile_height;
                int tile_id = row * atlas->columns + col;
                renderer.draw_tile_scaled(tile_id, dest);
            }
        }
    }

    /**
     * @brief Submit batch and clear in one operation
     * @param renderer The tile renderer to draw with
     */
    template<typename TileRenderer>
    void flush(TileRenderer& renderer) {
        submit(renderer);
        clear();
    }

private:
    std::vector<sprite_instance> m_sprites;
};

// ============================================================================
// Dirty Region Tracker - Track areas needing redraw
// ============================================================================

/**
 * @brief Tracks dirty (changed) regions for incremental rendering
 *
 * Instead of redrawing the entire UI every frame, this tracker
 * allows identifying only the regions that have changed.
 *
 * ## Performance Characteristics
 *
 * The tracker uses a bounded region list with configurable maximum size.
 * When the limit is reached, regions are aggressively merged to prevent
 * unbounded growth.
 *
 * - mark_dirty(): O(n) where n = current region count (bounded by max_regions)
 * - has_dirty_regions(): O(1)
 * - get_dirty_regions(): O(1)
 * - clear(): O(1)
 *
 * Default max_regions is 32, which keeps merge operations fast.
 */
class dirty_region_tracker {
public:
    /// Maximum number of regions before forced consolidation
    static constexpr std::size_t DEFAULT_MAX_REGIONS = 32;

    /**
     * @brief Create tracker for given screen dimensions
     * @param width Screen width
     * @param height Screen height
     * @param max_regions Maximum regions before consolidation (default: 32)
     */
    dirty_region_tracker(int width, int height,
                        std::size_t max_regions = DEFAULT_MAX_REGIONS)
        : m_screen_width(width)
        , m_screen_height(height)
        , m_max_regions(max_regions)
    {
        m_dirty_regions.reserve(max_regions);
    }

    /**
     * @brief Mark a rectangular region as dirty
     *
     * If the region can be merged with an existing dirty region, it will be.
     * If the maximum number of regions is reached, aggressive consolidation
     * is performed to prevent unbounded growth.
     */
    void mark_dirty(const tile_rect& rect) {
        // Clip to screen bounds
        tile_rect clipped = clip_to_screen(rect);
        if (clipped.width <= 0 || clipped.height <= 0) {
            return;
        }

        // Try to merge with existing regions (single pass)
        for (auto& existing : m_dirty_regions) {
            if (can_merge(existing, clipped)) {
                existing = merge(existing, clipped);
                // Only do a consolidation pass if we're near the limit
                if (m_dirty_regions.size() >= m_max_regions / 2) {
                    merge_overlapping_bounded();
                }
                return;
            }
        }

        // Check if we've hit the limit
        if (m_dirty_regions.size() >= m_max_regions) {
            // Consolidate aggressively: merge the two closest regions
            consolidate_regions();
        }

        m_dirty_regions.push_back(clipped);
    }

    /**
     * @brief Mark entire screen as dirty
     */
    void mark_all_dirty() {
        m_dirty_regions.clear();
        m_dirty_regions.push_back({0, 0, m_screen_width, m_screen_height});
    }

    /**
     * @brief Check if any regions are dirty
     */
    [[nodiscard]] bool has_dirty_regions() const noexcept {
        return !m_dirty_regions.empty();
    }

    /**
     * @brief Get list of dirty regions
     */
    [[nodiscard]] const std::vector<tile_rect>& get_dirty_regions() const noexcept {
        return m_dirty_regions;
    }

    /**
     * @brief Get total dirty area in pixels
     */
    [[nodiscard]] int get_dirty_area() const noexcept {
        int total = 0;
        for (const auto& r : m_dirty_regions) {
            total += r.width * r.height;
        }
        return total;
    }

    /**
     * @brief Get dirty area as percentage of screen
     */
    [[nodiscard]] float get_dirty_percentage() const noexcept {
        const int screen_area = m_screen_width * m_screen_height;
        if (screen_area == 0) return 0.0f;
        return static_cast<float>(get_dirty_area()) / static_cast<float>(screen_area) * 100.0f;
    }

    /**
     * @brief Clear all dirty regions (call after rendering)
     */
    void clear() noexcept {
        m_dirty_regions.clear();
    }

    /**
     * @brief Check if a point is in a dirty region
     */
    [[nodiscard]] bool is_point_dirty(int x, int y) const noexcept {
        for (const auto& r : m_dirty_regions) {
            if (x >= r.x && x < r.x + r.width &&
                y >= r.y && y < r.y + r.height) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Check if a rect intersects any dirty region
     */
    [[nodiscard]] bool intersects_dirty(const tile_rect& rect) const noexcept {
        for (const auto& r : m_dirty_regions) {
            if (rects_intersect(r, rect)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Resize the tracking area
     */
    void resize(int width, int height) {
        m_screen_width = width;
        m_screen_height = height;
        // Re-clip existing regions
        for (auto& r : m_dirty_regions) {
            r = clip_to_screen(r);
        }
        // Remove empty regions
        m_dirty_regions.erase(
            std::remove_if(m_dirty_regions.begin(), m_dirty_regions.end(),
                [](const tile_rect& r) {
                    return r.width <= 0 || r.height <= 0;
                }),
            m_dirty_regions.end());
    }

private:
    [[nodiscard]] tile_rect clip_to_screen(const tile_rect& rect) const noexcept {
        int x1 = std::max(0, rect.x);
        int y1 = std::max(0, rect.y);
        int x2 = std::min(m_screen_width, rect.x + rect.width);
        int y2 = std::min(m_screen_height, rect.y + rect.height);
        return {x1, y1, x2 - x1, y2 - y1};
    }

    [[nodiscard]] static bool rects_intersect(const tile_rect& a, const tile_rect& b) noexcept {
        return !(a.x + a.width <= b.x || b.x + b.width <= a.x ||
                 a.y + a.height <= b.y || b.y + b.height <= a.y);
    }

    [[nodiscard]] static bool can_merge(const tile_rect& a, const tile_rect& b) noexcept {
        // Merge if overlapping or adjacent
        const int expand = 4; // Small threshold for near-adjacent rects
        tile_rect expanded_a = {
            a.x - expand, a.y - expand,
            a.width + expand * 2, a.height + expand * 2
        };
        return rects_intersect(expanded_a, b);
    }

    [[nodiscard]] static tile_rect merge(const tile_rect& a, const tile_rect& b) noexcept {
        int x1 = std::min(a.x, b.x);
        int y1 = std::min(a.y, b.y);
        int x2 = std::max(a.x + a.width, b.x + b.width);
        int y2 = std::max(a.y + a.height, b.y + b.height);
        return {x1, y1, x2 - x1, y2 - y1};
    }

    /**
     * @brief Calculate the wasted area if two rects were merged
     *
     * Lower waste = better merge candidate. This helps choose which
     * regions to merge when forced consolidation is needed.
     */
    [[nodiscard]] static int merge_waste(const tile_rect& a, const tile_rect& b) noexcept {
        tile_rect merged = merge(a, b);
        int merged_area = merged.width * merged.height;
        int original_area = (a.width * a.height) + (b.width * b.height);
        return merged_area - original_area;
    }

    /**
     * @brief Bounded merge pass - only do limited iterations
     *
     * This prevents the O(n²) worst case by limiting iterations.
     * For typical UI updates, a single pass usually merges everything needed.
     */
    void merge_overlapping_bounded() {
        constexpr int MAX_ITERATIONS = 3;  // Limit iterations

        for (int iter = 0; iter < MAX_ITERATIONS; ++iter) {
            bool merged = false;
            for (std::size_t i = 0; i < m_dirty_regions.size() && !merged; ++i) {
                for (std::size_t j = i + 1; j < m_dirty_regions.size(); ++j) {
                    if (can_merge(m_dirty_regions[i], m_dirty_regions[j])) {
                        m_dirty_regions[i] = merge(m_dirty_regions[i], m_dirty_regions[j]);
                        m_dirty_regions.erase(m_dirty_regions.begin() + static_cast<std::ptrdiff_t>(j));
                        merged = true;
                        break;
                    }
                }
            }
            if (!merged) break;  // No more merges possible
        }
    }

    /**
     * @brief Force consolidation when region limit is reached
     *
     * Finds the two regions with minimum merge waste and combines them.
     * This ensures we always have room for new regions.
     */
    void consolidate_regions() {
        if (m_dirty_regions.size() < 2) return;

        // Find best merge candidates (minimum waste)
        std::size_t best_i = 0;
        std::size_t best_j = 1;
        int best_waste = merge_waste(m_dirty_regions[0], m_dirty_regions[1]);

        for (std::size_t i = 0; i < m_dirty_regions.size(); ++i) {
            for (std::size_t j = i + 1; j < m_dirty_regions.size(); ++j) {
                int waste = merge_waste(m_dirty_regions[i], m_dirty_regions[j]);
                if (waste < best_waste) {
                    best_waste = waste;
                    best_i = i;
                    best_j = j;
                }
            }
        }

        // Merge the best candidates
        m_dirty_regions[best_i] = merge(m_dirty_regions[best_i], m_dirty_regions[best_j]);
        m_dirty_regions.erase(m_dirty_regions.begin() + static_cast<std::ptrdiff_t>(best_j));
    }

    int m_screen_width;
    int m_screen_height;
    std::size_t m_max_regions;
    std::vector<tile_rect> m_dirty_regions;
};

// ============================================================================
// Tile Cache - Cache pre-rendered tiles
// ============================================================================

/**
 * @brief Key for cached tile lookups
 *
 * Includes atlas pointer to avoid collisions when using multiple atlases.
 * For nine-slices or complex cached elements, use a composite tile_id that
 * uniquely identifies the visual element (e.g., hash of the nine_slice tile IDs).
 */
struct tile_cache_key {
    const void* atlas;    ///< Atlas pointer (for multi-atlas disambiguation)
    uint32_t tile_id;     ///< Unique tile identifier (or composite ID for nine-slices)
    int width;            ///< Rendered width
    int height;           ///< Rendered height
    uint32_t state;       ///< Widget state (normal, hover, pressed, etc.)

    bool operator==(const tile_cache_key& other) const noexcept {
        return atlas == other.atlas &&
               tile_id == other.tile_id &&
               width == other.width &&
               height == other.height &&
               state == other.state;
    }
};

/**
 * @brief Hash function for tile_cache_key
 */
struct tile_cache_key_hash {
    std::size_t operator()(const tile_cache_key& key) const noexcept {
        // FNV-1a hash
        std::size_t hash = 2166136261u;

        // Hash the atlas pointer (cast to uintptr_t for portable hashing)
        auto atlas_val = reinterpret_cast<std::uintptr_t>(key.atlas);
        hash ^= static_cast<std::size_t>(atlas_val);
        hash *= 16777619u;
        hash ^= static_cast<std::size_t>(atlas_val >> 32);
        hash *= 16777619u;

        hash ^= key.tile_id;
        hash *= 16777619u;
        hash ^= static_cast<uint32_t>(key.width);
        hash *= 16777619u;
        hash ^= static_cast<uint32_t>(key.height);
        hash *= 16777619u;
        hash ^= key.state;
        hash *= 16777619u;
        return hash;
    }
};

/**
 * @brief Cached tile entry with LRU tracking
 */
template<typename CacheData>
struct tile_cache_entry {
    CacheData data;
    uint64_t last_access;
    std::size_t memory_size;
};

/**
 * @brief LRU cache for pre-rendered tiles
 *
 * Caches rendered tile data to avoid re-rendering unchanged widgets.
 * Uses LRU eviction when cache is full.
 *
 * @tparam CacheData The type of cached data (e.g., texture handle, pixel buffer)
 */
template<typename CacheData>
class tile_cache {
public:
    /**
     * @brief Create cache with maximum memory limit
     * @param max_memory Maximum cache size in bytes
     */
    explicit tile_cache(std::size_t max_memory = 16 * 1024 * 1024)  // 16MB default
        : m_max_memory(max_memory) {}

    /**
     * @brief Look up a cached tile
     * @return Pointer to cached data, or nullptr if not found
     */
    [[nodiscard]] CacheData* get(const tile_cache_key& key) {
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            it->second.last_access = ++m_access_counter;
            return &it->second.data;
        }
        return nullptr;
    }

    /**
     * @brief Look up a cached tile (const version)
     */
    [[nodiscard]] const CacheData* get(const tile_cache_key& key) const {
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            // Can't update last_access in const method
            return &it->second.data;
        }
        return nullptr;
    }

    /**
     * @brief Insert or update a cached tile
     * @param key The cache key
     * @param data The data to cache
     * @param memory_size Size of the cached data in bytes
     */
    void put(const tile_cache_key& key, CacheData data, std::size_t memory_size) {
        // Remove existing entry if present
        auto existing = m_cache.find(key);
        if (existing != m_cache.end()) {
            m_current_memory -= existing->second.memory_size;
            m_cache.erase(existing);
        }

        // Evict entries if needed
        while (m_current_memory + memory_size > m_max_memory && !m_cache.empty()) {
            evict_lru();
        }

        // Insert new entry
        m_cache[key] = {std::move(data), ++m_access_counter, memory_size};
        m_current_memory += memory_size;
    }

    /**
     * @brief Remove a specific entry
     */
    void remove(const tile_cache_key& key) {
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            m_current_memory -= it->second.memory_size;
            m_cache.erase(it);
        }
    }

    /**
     * @brief Clear the entire cache
     */
    void clear() {
        m_cache.clear();
        m_current_memory = 0;
    }

    /**
     * @brief Get current cache size in entries
     */
    [[nodiscard]] std::size_t size() const noexcept {
        return m_cache.size();
    }

    /**
     * @brief Get current memory usage
     */
    [[nodiscard]] std::size_t memory_usage() const noexcept {
        return m_current_memory;
    }

    /**
     * @brief Get maximum memory limit
     */
    [[nodiscard]] std::size_t max_memory() const noexcept {
        return m_max_memory;
    }

    /**
     * @brief Set new memory limit (may trigger eviction)
     */
    void set_max_memory(std::size_t max_memory) {
        m_max_memory = max_memory;
        while (m_current_memory > m_max_memory && !m_cache.empty()) {
            evict_lru();
        }
    }

    /**
     * @brief Get cache hit statistics
     */
    [[nodiscard]] float get_memory_utilization() const noexcept {
        if (m_max_memory == 0) return 0.0f;
        return static_cast<float>(m_current_memory) / static_cast<float>(m_max_memory) * 100.0f;
    }

    /**
     * @brief Check if key exists in cache
     */
    [[nodiscard]] bool contains(const tile_cache_key& key) const {
        return m_cache.find(key) != m_cache.end();
    }

private:
    void evict_lru() {
        if (m_cache.empty()) return;

        // Find least recently used entry
        auto lru_it = m_cache.begin();
        uint64_t min_access = lru_it->second.last_access;

        for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
            if (it->second.last_access < min_access) {
                min_access = it->second.last_access;
                lru_it = it;
            }
        }

        m_current_memory -= lru_it->second.memory_size;
        m_cache.erase(lru_it);
    }

    std::unordered_map<tile_cache_key, tile_cache_entry<CacheData>, tile_cache_key_hash> m_cache;
    std::size_t m_max_memory;
    std::size_t m_current_memory = 0;
    uint64_t m_access_counter = 0;
};

// ============================================================================
// Render Statistics - Track rendering performance
// ============================================================================

/**
 * @brief Statistics for render performance monitoring
 */
struct render_stats {
    // Per-frame stats
    int sprites_drawn = 0;
    int batches_submitted = 0;
    int cache_hits = 0;
    int cache_misses = 0;
    int dirty_regions = 0;
    int pixels_drawn = 0;

    // Accumulated stats
    uint64_t total_frames = 0;
    uint64_t total_sprites = 0;
    uint64_t total_batches = 0;

    /**
     * @brief Reset per-frame statistics
     */
    void reset_frame() noexcept {
        sprites_drawn = 0;
        batches_submitted = 0;
        cache_hits = 0;
        cache_misses = 0;
        dirty_regions = 0;
        pixels_drawn = 0;
    }

    /**
     * @brief End frame and accumulate statistics
     */
    void end_frame() noexcept {
        ++total_frames;
        total_sprites += sprites_drawn;
        total_batches += batches_submitted;
    }

    /**
     * @brief Get cache hit rate (0.0 - 1.0)
     */
    [[nodiscard]] float get_cache_hit_rate() const noexcept {
        const int total = cache_hits + cache_misses;
        if (total == 0) return 0.0f;
        return static_cast<float>(cache_hits) / static_cast<float>(total);
    }

    /**
     * @brief Get average sprites per batch
     */
    [[nodiscard]] float get_sprites_per_batch() const noexcept {
        if (batches_submitted == 0) return 0.0f;
        return static_cast<float>(sprites_drawn) / static_cast<float>(batches_submitted);
    }

    /**
     * @brief Get average sprites per frame (lifetime)
     */
    [[nodiscard]] float get_avg_sprites_per_frame() const noexcept {
        if (total_frames == 0) return 0.0f;
        return static_cast<float>(total_sprites) / static_cast<float>(total_frames);
    }
};

// ============================================================================
// Optimized Renderer Wrapper
// ============================================================================

/**
 * @brief Wrapper that adds batching and caching to a tile renderer
 *
 * @tparam Backend The UI backend type
 */
template<typename Backend>
class optimized_renderer {
public:
    using cache_data_type = std::vector<sprite_instance>;

    optimized_renderer(int screen_width, int screen_height,
                      std::size_t batch_capacity = 1024,
                      std::size_t cache_memory = 16 * 1024 * 1024)
        : m_batch(batch_capacity)
        , m_dirty_tracker(screen_width, screen_height)
        , m_cache(cache_memory) {}

    /**
     * @brief Begin a new frame
     */
    void begin_frame() {
        m_stats.reset_frame();
        m_batch.clear();
    }

    /**
     * @brief End the frame and flush remaining batches
     */
    void end_frame() {
        flush();
        m_dirty_tracker.clear();
        m_stats.end_frame();
    }

    /**
     * @brief Queue a tile for rendering
     */
    void draw_tile(const tile_def& tile, int x, int y) {
        m_batch.add_tile(tile, x, y);
        ++m_stats.sprites_drawn;
    }

    /**
     * @brief Queue a tile with destination rect
     */
    void draw_tile(const tile_def& tile, const tile_rect& dest) {
        m_batch.add_tile(tile, dest);
        ++m_stats.sprites_drawn;
    }

    /**
     * @brief Queue a horizontal slice
     */
    void draw_h_slice(const render_h_slice& slice, int x, int y, int width) {
        m_batch.add_h_slice(slice, x, y, width);
        m_stats.sprites_drawn += 3; // Up to 3 sprites per h_slice
    }

    /**
     * @brief Queue a vertical slice
     */
    void draw_v_slice(const render_v_slice& slice, int x, int y, int height) {
        m_batch.add_v_slice(slice, x, y, height);
        m_stats.sprites_drawn += 3; // Up to 3 sprites per v_slice
    }

    /**
     * @brief Queue a nine-slice
     */
    void draw_nine_slice(const render_nine_slice& slice, const tile_rect& dest) {
        m_batch.add_nine_slice(slice, dest);
        m_stats.sprites_drawn += 9; // Up to 9 sprites per nine_slice
    }

    /**
     * @brief Mark a region as dirty
     */
    void mark_dirty(const tile_rect& rect) {
        m_dirty_tracker.mark_dirty(rect);
        ++m_stats.dirty_regions;
    }

    /**
     * @brief Check if region needs redraw
     */
    [[nodiscard]] bool needs_redraw(const tile_rect& rect) const {
        return m_dirty_tracker.intersects_dirty(rect);
    }

    /**
     * @brief Flush pending batch without rendering (call before texture change)
     *
     * This clears the batch and updates statistics but does NOT render.
     * Use flush(renderer) to render and clear, or call submit() directly
     * on the batch for more control.
     *
     * @note This method exists for compatibility but is rarely what you want.
     * Consider using flush(renderer) or batch().flush(renderer) instead.
     */
    void flush() {
        if (!m_batch.empty()) {
            ++m_stats.batches_submitted;
            m_stats.pixels_drawn += calculate_pixels_drawn();
            m_batch.clear();
        }
    }

    /**
     * @brief Flush pending batch with actual rendering
     * @param renderer The tile renderer to draw with
     *
     * This is the preferred method - it renders all queued sprites and clears
     * the batch for the next frame.
     */
    template<typename TileRenderer>
    void flush(TileRenderer& renderer) {
        if (!m_batch.empty()) {
            ++m_stats.batches_submitted;
            m_stats.pixels_drawn += calculate_pixels_drawn();
            m_batch.flush(renderer);  // Renders and clears
        }
    }

    /**
     * @brief Get the sprite batch for custom rendering
     */
    [[nodiscard]] sprite_batch<Backend>& batch() noexcept {
        return m_batch;
    }

    /**
     * @brief Get the dirty region tracker
     */
    [[nodiscard]] dirty_region_tracker& dirty_tracker() noexcept {
        return m_dirty_tracker;
    }

    /**
     * @brief Get the tile cache
     */
    [[nodiscard]] tile_cache<cache_data_type>& cache() noexcept {
        return m_cache;
    }

    /**
     * @brief Get render statistics
     */
    [[nodiscard]] const render_stats& stats() const noexcept {
        return m_stats;
    }

    /**
     * @brief Get mutable render statistics
     */
    [[nodiscard]] render_stats& stats() noexcept {
        return m_stats;
    }

    /**
     * @brief Resize the renderer
     */
    void resize(int width, int height) {
        m_dirty_tracker.resize(width, height);
        m_dirty_tracker.mark_all_dirty();
    }

private:
    [[nodiscard]] int calculate_pixels_drawn() const {
        int pixels = 0;
        for (const auto& sprite : m_batch.sprites()) {
            pixels += sprite.dest_rect.width * sprite.dest_rect.height;
        }
        return pixels;
    }

    sprite_batch<Backend> m_batch;
    dirty_region_tracker m_dirty_tracker;
    tile_cache<cache_data_type> m_cache;
    render_stats m_stats;
};

// ============================================================================
// Memory Pool for Frequently Allocated Objects
// ============================================================================

/**
 * @brief Simple object pool for reducing allocation overhead
 *
 * Pre-allocates objects and reuses them instead of frequent new/delete.
 *
 * @tparam T The type of object to pool
 */
template<typename T>
class object_pool {
public:
    explicit object_pool(std::size_t initial_size = 64) {
        m_pool.reserve(initial_size);
        for (std::size_t i = 0; i < initial_size; ++i) {
            m_pool.push_back(std::make_unique<T>());
            m_available.push_back(m_pool.back().get());
        }
    }

    /**
     * @brief Acquire an object from the pool
     */
    [[nodiscard]] T* acquire() {
        if (m_available.empty()) {
            // Grow pool
            m_pool.push_back(std::make_unique<T>());
            return m_pool.back().get();
        }
        T* obj = m_available.back();
        m_available.pop_back();
        return obj;
    }

    /**
     * @brief Release an object back to the pool
     */
    void release(T* obj) {
        if (obj) {
            m_available.push_back(obj);
        }
    }

    /**
     * @brief Get number of available objects
     */
    [[nodiscard]] std::size_t available() const noexcept {
        return m_available.size();
    }

    /**
     * @brief Get total pool size
     */
    [[nodiscard]] std::size_t capacity() const noexcept {
        return m_pool.size();
    }

    /**
     * @brief Get number of objects in use
     */
    [[nodiscard]] std::size_t in_use() const noexcept {
        return m_pool.size() - m_available.size();
    }

private:
    std::vector<std::unique_ptr<T>> m_pool;
    std::vector<T*> m_available;
};

/**
 * @brief RAII guard for pooled objects
 */
template<typename T>
class pooled_object {
public:
    pooled_object(object_pool<T>& pool)
        : m_pool(pool)
        , m_object(pool.acquire()) {}

    ~pooled_object() {
        if (m_object) {
            m_pool.release(m_object);
        }
    }

    // Non-copyable
    pooled_object(const pooled_object&) = delete;
    pooled_object& operator=(const pooled_object&) = delete;

    // Movable
    pooled_object(pooled_object&& other) noexcept
        : m_pool(other.m_pool)
        , m_object(other.m_object) {
        other.m_object = nullptr;
    }

    pooled_object& operator=(pooled_object&& other) noexcept {
        if (this != &other) {
            if (m_object) {
                m_pool.release(m_object);
            }
            m_object = other.m_object;
            other.m_object = nullptr;
        }
        return *this;
    }

    [[nodiscard]] T* get() noexcept { return m_object; }
    [[nodiscard]] const T* get() const noexcept { return m_object; }
    [[nodiscard]] T* operator->() noexcept { return m_object; }
    [[nodiscard]] const T* operator->() const noexcept { return m_object; }
    [[nodiscard]] T& operator*() noexcept { return *m_object; }
    [[nodiscard]] const T& operator*() const noexcept { return *m_object; }

    /**
     * @brief Release ownership without returning to pool
     */
    [[nodiscard]] T* release() noexcept {
        T* obj = m_object;
        m_object = nullptr;
        return obj;
    }

private:
    object_pool<T>& m_pool;
    T* m_object;
};

} // namespace onyxui::tile
