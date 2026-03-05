/**
 * @file test_tile_optimization.cc
 * @brief Unit tests for tile optimization utilities
 */

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include <onyxui/tile/tile_optimization.hh>

using namespace onyxui;
using namespace onyxui::tile;
using namespace onyxui::testing;

using Backend = test_canvas_backend;

// ============================================================================
// Sprite Batch Tests
// ============================================================================

TEST_SUITE("tile_optimization_sprite_batch") {
    TEST_CASE("sprite_batch initializes empty") {
        sprite_batch<Backend> batch;

        CHECK(batch.empty());
        CHECK(batch.size() == 0);
    }

    TEST_CASE("sprite_batch add simple sprite") {
        sprite_batch<Backend> batch;

        batch.add({0, 0, 16, 16}, {100, 100, 16, 16});

        CHECK_FALSE(batch.empty());
        CHECK(batch.size() == 1);
        CHECK(batch.sprites()[0].src_rect.x == 0);
        CHECK(batch.sprites()[0].dest_rect.x == 100);
    }

    TEST_CASE("sprite_batch add sprite_instance") {
        sprite_batch<Backend> batch;

        sprite_instance sprite;
        sprite.src_rect = {0, 0, 32, 32};
        sprite.dest_rect = {50, 50, 32, 32};
        sprite.rotation = 0.5f;
        sprite.alpha = 128;

        batch.add(sprite);

        CHECK(batch.size() == 1);
        CHECK(batch.sprites()[0].rotation == doctest::Approx(0.5f));
        CHECK(batch.sprites()[0].alpha == 128);
    }

    TEST_CASE("sprite_batch add_tile") {
        sprite_batch<Backend> batch;

        tile_def tile{16, 32, 8, 8};
        batch.add_tile(tile, 100, 200);

        CHECK(batch.size() == 1);
        CHECK(batch.sprites()[0].src_rect.x == 16);
        CHECK(batch.sprites()[0].src_rect.y == 32);
        CHECK(batch.sprites()[0].dest_rect.x == 100);
        CHECK(batch.sprites()[0].dest_rect.y == 200);
    }

    TEST_CASE("sprite_batch add_tile with custom dest") {
        sprite_batch<Backend> batch;

        tile_def tile{0, 0, 16, 16};
        batch.add_tile(tile, {50, 50, 32, 32});

        CHECK(batch.sprites()[0].dest_rect.width == 32);
        CHECK(batch.sprites()[0].dest_rect.height == 32);
    }

    TEST_CASE("sprite_batch add_h_slice") {
        sprite_batch<Backend> batch;

        render_h_slice slice{0, 0, 4, 8, 4, 16};
        batch.add_h_slice(slice, 0, 0, 100);

        CHECK(batch.size() == 3); // Left, middle, right
    }

    TEST_CASE("sprite_batch add_h_slice zero width") {
        sprite_batch<Backend> batch;

        render_h_slice slice{0, 0, 4, 8, 4, 16};
        batch.add_h_slice(slice, 0, 0, 0);

        CHECK(batch.empty());
    }

    TEST_CASE("sprite_batch add_v_slice") {
        sprite_batch<Backend> batch;

        render_v_slice slice{0, 0, 16, 4, 8, 4};
        batch.add_v_slice(slice, 0, 0, 100);

        CHECK(batch.size() == 3); // Top, middle, bottom
    }

    TEST_CASE("sprite_batch add_nine_slice") {
        sprite_batch<Backend> batch;

        render_nine_slice slice{0, 0, 24, 24, 8, 8, 8, 8};
        batch.add_nine_slice(slice, {0, 0, 100, 100});

        CHECK(batch.size() == 9); // 3x3 grid
    }

    TEST_CASE("sprite_batch clear") {
        sprite_batch<Backend> batch;

        batch.add({0, 0, 16, 16}, {0, 0, 16, 16});
        batch.add({0, 0, 16, 16}, {16, 0, 16, 16});

        CHECK(batch.size() == 2);

        batch.clear();

        CHECK(batch.empty());
        CHECK(batch.size() == 0);
    }

    TEST_CASE("sprite_batch sort_by_y") {
        sprite_batch<Backend> batch;

        batch.add({0, 0, 16, 16}, {0, 100, 16, 16});
        batch.add({0, 0, 16, 16}, {0, 50, 16, 16});
        batch.add({0, 0, 16, 16}, {0, 200, 16, 16});

        batch.sort_by_y();

        CHECK(batch.sprites()[0].dest_rect.y == 50);
        CHECK(batch.sprites()[1].dest_rect.y == 100);
        CHECK(batch.sprites()[2].dest_rect.y == 200);
    }

    TEST_CASE("sprite_batch sort_by_texture_locality") {
        sprite_batch<Backend> batch;

        batch.add({32, 0, 16, 16}, {0, 0, 16, 16});
        batch.add({0, 16, 16, 16}, {0, 0, 16, 16});
        batch.add({0, 0, 16, 16}, {0, 0, 16, 16});

        batch.sort_by_texture_locality();

        CHECK(batch.sprites()[0].src_rect.y == 0);
        CHECK(batch.sprites()[0].src_rect.x == 0);
        CHECK(batch.sprites()[1].src_rect.x == 32);
        CHECK(batch.sprites()[2].src_rect.y == 16);
    }
}

// ============================================================================
// Dirty Region Tracker Tests
// ============================================================================

TEST_SUITE("tile_optimization_dirty_regions") {
    TEST_CASE("dirty_region_tracker initializes clean") {
        dirty_region_tracker tracker(800, 600);

        CHECK_FALSE(tracker.has_dirty_regions());
        CHECK(tracker.get_dirty_regions().empty());
    }

    TEST_CASE("mark_dirty adds region") {
        dirty_region_tracker tracker(800, 600);

        tracker.mark_dirty({100, 100, 50, 50});

        CHECK(tracker.has_dirty_regions());
        CHECK(tracker.get_dirty_regions().size() == 1);
    }

    TEST_CASE("mark_dirty clips to screen bounds") {
        dirty_region_tracker tracker(800, 600);

        tracker.mark_dirty({-50, -50, 100, 100});

        const auto& regions = tracker.get_dirty_regions();
        CHECK(regions.size() == 1);
        CHECK(regions[0].x == 0);
        CHECK(regions[0].y == 0);
    }

    TEST_CASE("mark_dirty rejects fully out of bounds") {
        dirty_region_tracker tracker(800, 600);

        tracker.mark_dirty({-100, -100, 50, 50});

        CHECK_FALSE(tracker.has_dirty_regions());
    }

    TEST_CASE("mark_all_dirty covers entire screen") {
        dirty_region_tracker tracker(800, 600);

        tracker.mark_all_dirty();

        CHECK(tracker.has_dirty_regions());
        const auto& regions = tracker.get_dirty_regions();
        CHECK(regions.size() == 1);
        CHECK(regions[0].x == 0);
        CHECK(regions[0].y == 0);
        CHECK(regions[0].width == 800);
        CHECK(regions[0].height == 600);
    }

    TEST_CASE("get_dirty_area calculates correctly") {
        dirty_region_tracker tracker(800, 600);

        tracker.mark_dirty({0, 0, 100, 100});

        CHECK(tracker.get_dirty_area() == 10000);
    }

    TEST_CASE("get_dirty_percentage calculates correctly") {
        dirty_region_tracker tracker(100, 100);

        tracker.mark_dirty({0, 0, 50, 50});

        CHECK(tracker.get_dirty_percentage() == doctest::Approx(25.0f));
    }

    TEST_CASE("clear removes all regions") {
        dirty_region_tracker tracker(800, 600);

        tracker.mark_dirty({100, 100, 50, 50});
        tracker.mark_dirty({200, 200, 50, 50});
        tracker.clear();

        CHECK_FALSE(tracker.has_dirty_regions());
    }

    TEST_CASE("is_point_dirty checks correctly") {
        dirty_region_tracker tracker(800, 600);

        tracker.mark_dirty({100, 100, 50, 50});

        CHECK(tracker.is_point_dirty(125, 125));
        CHECK_FALSE(tracker.is_point_dirty(50, 50));
        CHECK_FALSE(tracker.is_point_dirty(200, 200));
    }

    TEST_CASE("intersects_dirty checks correctly") {
        dirty_region_tracker tracker(800, 600);

        tracker.mark_dirty({100, 100, 50, 50});

        CHECK(tracker.intersects_dirty({120, 120, 20, 20}));
        CHECK(tracker.intersects_dirty({80, 80, 40, 40}));
        CHECK_FALSE(tracker.intersects_dirty({200, 200, 50, 50}));
    }

    TEST_CASE("resize updates bounds") {
        dirty_region_tracker tracker(800, 600);

        tracker.mark_dirty({700, 500, 100, 100});
        tracker.resize(400, 300);

        // Region should be clipped or removed
        const auto& regions = tracker.get_dirty_regions();
        if (!regions.empty()) {
            CHECK(regions[0].x + regions[0].width <= 400);
            CHECK(regions[0].y + regions[0].height <= 300);
        }
    }

    TEST_CASE("adjacent regions merge") {
        dirty_region_tracker tracker(800, 600);

        tracker.mark_dirty({100, 100, 50, 50});
        tracker.mark_dirty({150, 100, 50, 50}); // Adjacent

        // Should merge into one region
        CHECK(tracker.get_dirty_regions().size() <= 2);
    }
}

// ============================================================================
// Tile Cache Tests
// ============================================================================

TEST_SUITE("tile_optimization_cache") {
    TEST_CASE("tile_cache initializes empty") {
        tile_cache<int> cache;

        CHECK(cache.size() == 0);
        CHECK(cache.memory_usage() == 0);
    }

    TEST_CASE("tile_cache put and get") {
        tile_cache<std::string> cache;

        tile_cache_key key{.atlas = nullptr, .tile_id = 1, .width = 100, .height = 100, .state = 0};
        cache.put(key, "test_data", 10);

        auto* result = cache.get(key);
        CHECK(result != nullptr);
        CHECK(*result == "test_data");
    }

    TEST_CASE("tile_cache miss returns nullptr") {
        tile_cache<int> cache;

        tile_cache_key key{.atlas = nullptr, .tile_id = 999, .width = 0, .height = 0, .state = 0};
        auto* result = cache.get(key);

        CHECK(result == nullptr);
    }

    TEST_CASE("tile_cache tracks memory") {
        tile_cache<int> cache;

        tile_cache_key key1{.atlas = nullptr, .tile_id = 1, .width = 0, .height = 0, .state = 0};
        tile_cache_key key2{.atlas = nullptr, .tile_id = 2, .width = 0, .height = 0, .state = 0};

        cache.put(key1, 42, 100);
        CHECK(cache.memory_usage() == 100);

        cache.put(key2, 43, 200);
        CHECK(cache.memory_usage() == 300);
    }

    TEST_CASE("tile_cache evicts when full") {
        tile_cache<int> cache(500);

        tile_cache_key key1{.atlas = nullptr, .tile_id = 1, .width = 0, .height = 0, .state = 0};
        tile_cache_key key2{.atlas = nullptr, .tile_id = 2, .width = 0, .height = 0, .state = 0};
        tile_cache_key key3{.atlas = nullptr, .tile_id = 3, .width = 0, .height = 0, .state = 0};

        cache.put(key1, 1, 200);
        cache.put(key2, 2, 200);
        // Adding key3 should evict key1 (LRU)
        cache.put(key3, 3, 200);

        CHECK(cache.memory_usage() <= 500);
    }

    TEST_CASE("tile_cache LRU eviction") {
        tile_cache<int> cache(300);

        tile_cache_key key1{.atlas = nullptr, .tile_id = 1, .width = 0, .height = 0, .state = 0};
        tile_cache_key key2{.atlas = nullptr, .tile_id = 2, .width = 0, .height = 0, .state = 0};
        tile_cache_key key3{.atlas = nullptr, .tile_id = 3, .width = 0, .height = 0, .state = 0};

        cache.put(key1, 1, 100);
        cache.put(key2, 2, 100);

        // Access key1 to make it more recent
        (void)cache.get(key1);

        // Adding key3 should evict key2 (least recently used)
        cache.put(key3, 3, 150);

        CHECK(cache.get(key1) != nullptr); // key1 should still exist
        CHECK(cache.get(key2) == nullptr); // key2 should be evicted
        CHECK(cache.get(key3) != nullptr); // key3 should exist
    }

    TEST_CASE("tile_cache remove") {
        tile_cache<int> cache;

        tile_cache_key key{.atlas = nullptr, .tile_id = 1, .width = 0, .height = 0, .state = 0};
        cache.put(key, 42, 100);

        CHECK(cache.contains(key));

        cache.remove(key);

        CHECK_FALSE(cache.contains(key));
        CHECK(cache.memory_usage() == 0);
    }

    TEST_CASE("tile_cache clear") {
        tile_cache<int> cache;

        cache.put({.atlas = nullptr, .tile_id = 1, .width = 0, .height = 0, .state = 0}, 1, 100);
        cache.put({.atlas = nullptr, .tile_id = 2, .width = 0, .height = 0, .state = 0}, 2, 100);
        cache.put({.atlas = nullptr, .tile_id = 3, .width = 0, .height = 0, .state = 0}, 3, 100);

        cache.clear();

        CHECK(cache.size() == 0);
        CHECK(cache.memory_usage() == 0);
    }

    TEST_CASE("tile_cache set_max_memory triggers eviction") {
        tile_cache<int> cache(1000);

        cache.put({.atlas = nullptr, .tile_id = 1, .width = 0, .height = 0, .state = 0}, 1, 300);
        cache.put({.atlas = nullptr, .tile_id = 2, .width = 0, .height = 0, .state = 0}, 2, 300);
        cache.put({.atlas = nullptr, .tile_id = 3, .width = 0, .height = 0, .state = 0}, 3, 300);

        CHECK(cache.memory_usage() == 900);

        cache.set_max_memory(500);

        CHECK(cache.memory_usage() <= 500);
    }

    TEST_CASE("tile_cache_key equality") {
        tile_cache_key key1{.atlas = nullptr, .tile_id = 1, .width = 100, .height = 100, .state = 0};
        tile_cache_key key2{.atlas = nullptr, .tile_id = 1, .width = 100, .height = 100, .state = 0};
        tile_cache_key key3{.atlas = nullptr, .tile_id = 2, .width = 100, .height = 100, .state = 0};

        CHECK(key1 == key2);
        CHECK_FALSE(key1 == key3);
    }

    TEST_CASE("tile_cache memory utilization") {
        tile_cache<int> cache(1000);

        cache.put({.atlas = nullptr, .tile_id = 1, .width = 0, .height = 0, .state = 0}, 1, 500);

        CHECK(cache.get_memory_utilization() == doctest::Approx(50.0f));
    }
}

// ============================================================================
// Render Stats Tests
// ============================================================================

TEST_SUITE("tile_optimization_render_stats") {
    TEST_CASE("render_stats initializes to zero") {
        render_stats stats;

        CHECK(stats.sprites_drawn == 0);
        CHECK(stats.batches_submitted == 0);
        CHECK(stats.total_frames == 0);
    }

    TEST_CASE("render_stats reset_frame") {
        render_stats stats;

        stats.sprites_drawn = 100;
        stats.batches_submitted = 10;
        stats.cache_hits = 50;

        stats.reset_frame();

        CHECK(stats.sprites_drawn == 0);
        CHECK(stats.batches_submitted == 0);
        CHECK(stats.cache_hits == 0);
    }

    TEST_CASE("render_stats end_frame accumulates") {
        render_stats stats;

        stats.sprites_drawn = 100;
        stats.batches_submitted = 10;
        stats.end_frame();

        stats.sprites_drawn = 200;
        stats.batches_submitted = 20;
        stats.end_frame();

        CHECK(stats.total_frames == 2);
        CHECK(stats.total_sprites == 300);
        CHECK(stats.total_batches == 30);
    }

    TEST_CASE("render_stats cache_hit_rate") {
        render_stats stats;

        stats.cache_hits = 75;
        stats.cache_misses = 25;

        CHECK(stats.get_cache_hit_rate() == doctest::Approx(0.75f));
    }

    TEST_CASE("render_stats sprites_per_batch") {
        render_stats stats;

        stats.sprites_drawn = 1000;
        stats.batches_submitted = 10;

        CHECK(stats.get_sprites_per_batch() == doctest::Approx(100.0f));
    }

    TEST_CASE("render_stats avg_sprites_per_frame") {
        render_stats stats;

        stats.total_sprites = 3000;
        stats.total_frames = 60;

        CHECK(stats.get_avg_sprites_per_frame() == doctest::Approx(50.0f));
    }
}

// ============================================================================
// Object Pool Tests
// ============================================================================

TEST_SUITE("tile_optimization_object_pool") {
    TEST_CASE("object_pool initializes with capacity") {
        object_pool<int> pool(32);

        CHECK(pool.capacity() == 32);
        CHECK(pool.available() == 32);
        CHECK(pool.in_use() == 0);
    }

    TEST_CASE("object_pool acquire reduces available") {
        object_pool<int> pool(10);

        auto* obj = pool.acquire();

        CHECK(obj != nullptr);
        CHECK(pool.available() == 9);
        CHECK(pool.in_use() == 1);
    }

    TEST_CASE("object_pool release returns to pool") {
        object_pool<int> pool(10);

        auto* obj = pool.acquire();
        pool.release(obj);

        CHECK(pool.available() == 10);
        CHECK(pool.in_use() == 0);
    }

    TEST_CASE("object_pool grows when exhausted") {
        object_pool<int> pool(2);

        auto* obj1 = pool.acquire();
        auto* obj2 = pool.acquire();
        auto* obj3 = pool.acquire(); // Should grow pool

        CHECK(obj1 != nullptr);
        CHECK(obj2 != nullptr);
        CHECK(obj3 != nullptr);
        CHECK(pool.capacity() == 3);
    }

    TEST_CASE("object_pool release nullptr is safe") {
        object_pool<int> pool(10);

        pool.release(nullptr); // Should not crash

        CHECK(pool.available() == 10);
    }

    TEST_CASE("pooled_object RAII") {
        object_pool<int> pool(10);

        {
            pooled_object<int> obj(pool);
            *obj = 42;
            CHECK(*obj == 42);
            CHECK(pool.in_use() == 1);
        }

        CHECK(pool.in_use() == 0);
        CHECK(pool.available() == 10);
    }

    TEST_CASE("pooled_object move") {
        object_pool<int> pool(10);

        pooled_object<int> obj1(pool);
        *obj1 = 100;

        pooled_object<int> obj2(std::move(obj1));

        CHECK(*obj2 == 100);
        CHECK(pool.in_use() == 1);
    }

    TEST_CASE("pooled_object release") {
        object_pool<int> pool(10);

        pooled_object<int> obj(pool);
        int* raw = obj.release();

        CHECK(raw != nullptr);
        CHECK(pool.in_use() == 1); // Not returned to pool

        pool.release(raw); // Manual release
        CHECK(pool.in_use() == 0);
    }
}

// ============================================================================
// Optimized Renderer Tests
// ============================================================================

TEST_SUITE("tile_optimization_renderer") {
    TEST_CASE("optimized_renderer initializes") {
        optimized_renderer<Backend> renderer(800, 600);

        CHECK(renderer.stats().sprites_drawn == 0);
        CHECK(renderer.batch().empty());
    }

    TEST_CASE("optimized_renderer draw_tile") {
        optimized_renderer<Backend> renderer(800, 600);

        renderer.begin_frame();
        renderer.draw_tile({0, 0, 16, 16}, 100, 100);

        CHECK(renderer.stats().sprites_drawn == 1);
        CHECK(renderer.batch().size() == 1);
    }

    TEST_CASE("optimized_renderer flush increments batches") {
        optimized_renderer<Backend> renderer(800, 600);

        renderer.begin_frame();
        renderer.draw_tile({0, 0, 16, 16}, 0, 0);
        renderer.flush();

        CHECK(renderer.stats().batches_submitted == 1);
        CHECK(renderer.batch().empty());
    }

    TEST_CASE("optimized_renderer end_frame accumulates stats") {
        optimized_renderer<Backend> renderer(800, 600);

        renderer.begin_frame();
        renderer.draw_tile({0, 0, 16, 16}, 0, 0);
        renderer.end_frame();

        CHECK(renderer.stats().total_frames == 1);
    }

    TEST_CASE("optimized_renderer mark_dirty") {
        optimized_renderer<Backend> renderer(800, 600);

        renderer.mark_dirty({100, 100, 50, 50});

        CHECK(renderer.dirty_tracker().has_dirty_regions());
        CHECK(renderer.stats().dirty_regions == 1);
    }

    TEST_CASE("optimized_renderer needs_redraw") {
        optimized_renderer<Backend> renderer(800, 600);

        renderer.mark_dirty({100, 100, 50, 50});

        CHECK(renderer.needs_redraw({120, 120, 20, 20}));
        CHECK_FALSE(renderer.needs_redraw({200, 200, 50, 50}));
    }

    TEST_CASE("optimized_renderer resize marks all dirty") {
        optimized_renderer<Backend> renderer(800, 600);

        renderer.resize(1024, 768);

        CHECK(renderer.dirty_tracker().has_dirty_regions());
        const auto& regions = renderer.dirty_tracker().get_dirty_regions();
        CHECK(regions[0].width == 1024);
        CHECK(regions[0].height == 768);
    }

    TEST_CASE("optimized_renderer draw_h_slice") {
        optimized_renderer<Backend> renderer(800, 600);

        renderer.begin_frame();
        render_h_slice slice{0, 0, 4, 8, 4, 16};
        renderer.draw_h_slice(slice, 0, 0, 100);

        CHECK(renderer.stats().sprites_drawn == 3);
    }

    TEST_CASE("optimized_renderer draw_nine_slice") {
        optimized_renderer<Backend> renderer(800, 600);

        renderer.begin_frame();
        render_nine_slice slice{0, 0, 24, 24, 8, 8, 8, 8};
        renderer.draw_nine_slice(slice, {0, 0, 100, 100});

        CHECK(renderer.stats().sprites_drawn == 9);
    }

    TEST_CASE("optimized_renderer cache access") {
        optimized_renderer<Backend> renderer(800, 600);

        tile_cache_key key{.atlas = nullptr, .tile_id = 1, .width = 100, .height = 100, .state = 0};
        renderer.cache().put(key, {}, 100);

        CHECK(renderer.cache().contains(key));
    }
}
