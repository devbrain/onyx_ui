//
// Style Thread Safety Tests
// Tests concurrent style resolution and theme registry access
// Phase 4, Section 7: Thread Safety
//

#include <doctest/doctest.h>
#include <onyxui/ui_context.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/panel.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/theme.hh>
#include <onyxui/resolved_style.hh>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include "../utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

namespace {
    // Helper to create a test theme
    ui_theme<Backend> create_thread_safety_theme(const std::string& name) {
        ui_theme<Backend> theme;
        theme.name = name;
        theme.window_bg = {50, 50, 50};
        theme.text_fg = {200, 200, 200};
        theme.border_color = {100, 100, 100};

        theme.button.fg_normal = {255, 255, 255};
        theme.button.bg_normal = {0, 120, 215};
        theme.label.text = {200, 200, 200};
        theme.label.background = {50, 50, 50};
        theme.panel.background = {60, 60, 60};
        theme.panel.border_color = {100, 100, 100};

        return theme;
    }
}

TEST_SUITE("Style Thread Safety") {

// ============================================================================
// Concurrent Style Resolution
// ============================================================================

TEST_CASE("Thread Safety - Concurrent style resolution (10 threads)") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_thread_safety_theme("Test Theme"));

    // Create 10 widgets
    std::vector<std::unique_ptr<button<Backend>>> widgets;
    for (int i = 0; i < 10; ++i) {
        auto btn = std::make_unique<button<Backend>>("Button");
        btn->apply_theme("Test Theme", ctx.themes());
        widgets.push_back(std::move(btn));
    }

    // Resolve styles concurrently from 10 threads
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (size_t i = 0; i < 10; ++i) {
        threads.emplace_back([&, i]() {
            // Each thread resolves style 100 times
            for (int j = 0; j < 100; ++j) {
                auto style = widgets[i]->resolve_style();
                if (style.background_color.g == 120) {  // button.bg_normal = {0, 120, 215}
                    ++success_count;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // All resolutions should succeed
    CHECK(success_count == 1000);  // 10 threads * 100 iterations
}

TEST_CASE("Thread Safety - Concurrent resolution of shared widget") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_thread_safety_theme("Shared"));

    auto widget = std::make_unique<button<Backend>>("Shared");
    widget->apply_theme("Shared", ctx.themes());

    // Multiple threads resolve style of same widget
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < 20; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 50; ++j) {
                auto style = widget->resolve_style();
                if (style.foreground_color.r == 255) {  // button.fg_normal = {255, 255, 255}
                    ++success_count;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // All resolutions should succeed
    CHECK(success_count == 1000);  // 20 threads * 50 iterations
}

TEST_CASE("Thread Safety - Concurrent deep hierarchy resolution") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_thread_safety_theme("Deep"));

    // Build 10-level deep hierarchy
    auto root = std::make_unique<panel<Backend>>();
    root->apply_theme("Deep", ctx.themes());
    root->set_background_color({100, 100, 100});

    panel<Backend>* current = root.get();
    for (int i = 0; i < 9; ++i) {
        current = current->template emplace_child<panel>();
    }

    auto* deep_btn = current->template emplace_child<button>("Deep");

    // Concurrent resolution from multiple threads
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 100; ++j) {
                auto style = deep_btn->resolve_style();
                if (style.background_color.r == 100) {
                    ++success_count;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    CHECK(success_count == 1000);
}

// ============================================================================
// Style Resolution During Theme Registry Updates
// ============================================================================

TEST_CASE("Thread Safety - Resolution during theme registration") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_thread_safety_theme("Initial"));

    auto widget = std::make_unique<button<Backend>>("Test");
    widget->apply_theme("Initial", ctx.themes());

    std::atomic<bool> keep_running{true};
    std::atomic<int> resolution_count{0};

    // Thread 1: Continuously resolve style
    std::thread resolver([&]() {
        while (keep_running) {
            [[maybe_unused]] auto style = widget->resolve_style();
            ++resolution_count;
            std::this_thread::yield();
        }
    });

    // Thread 2: Register new themes
    std::thread registrar([&]() {
        for (int i = 0; i < 50; ++i) {
            auto theme = create_thread_safety_theme("Theme_" + std::to_string(i));
            ctx.themes().register_theme(std::move(theme));
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        keep_running = false;
    });

    resolver.join();
    registrar.join();

    // Should complete without deadlock or crash
    CHECK(resolution_count > 0);
}

TEST_CASE("Thread Safety - Concurrent theme registration and lookup") {
    scoped_ui_context<Backend> ctx;

    std::atomic<int> register_count{0};
    std::atomic<int> lookup_count{0};

    // Thread group 1: Register themes
    std::vector<std::thread> registrars;
    for (int t = 0; t < 5; ++t) {
        registrars.emplace_back([&, t]() {
            for (int i = 0; i < 20; ++i) {
                auto theme = create_thread_safety_theme(
                    "Theme_" + std::to_string(t) + "_" + std::to_string(i)
                );
                ctx.themes().register_theme(std::move(theme));
                ++register_count;
                std::this_thread::yield();
            }
        });
    }

    // Thread group 2: Look up themes
    std::vector<std::thread> lookers;
    for (int t = 0; t < 5; ++t) {
        lookers.emplace_back([&]() {
            for (int i = 0; i < 100; ++i) {
                auto* theme = ctx.themes().get_theme("Theme_0_0");
                if (theme || !theme) {  // Either result is valid
                    ++lookup_count;
                }
                std::this_thread::yield();
            }
        });
    }

    for (auto& thread : registrars) {
        thread.join();
    }
    for (auto& thread : lookers) {
        thread.join();
    }

    // All operations should complete
    CHECK(register_count == 100);  // 5 threads * 20 registrations
    CHECK(lookup_count == 500);    // 5 threads * 100 lookups
}

TEST_CASE("Thread Safety - Apply theme during concurrent registration") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_thread_safety_theme("Base"));

    std::vector<std::unique_ptr<button<Backend>>> widgets;
    for (int i = 0; i < 10; ++i) {
        widgets.push_back(std::make_unique<button<Backend>>("Widget"));
    }

    std::atomic<int> apply_count{0};
    std::atomic<int> register_count{0};

    // Thread group 1: Apply themes
    std::vector<std::thread> appliers;
    for (size_t i = 0; i < 10; ++i) {
        appliers.emplace_back([&, i]() {
            for (int j = 0; j < 10; ++j) {
                bool success = widgets[i]->apply_theme("Base", ctx.themes());
                if (success) {
                    ++apply_count;
                }
                std::this_thread::yield();
            }
        });
    }

    // Thread group 2: Register new themes
    std::vector<std::thread> registrars;
    for (int t = 0; t < 3; ++t) {
        registrars.emplace_back([&, t]() {
            for (int i = 0; i < 10; ++i) {
                auto theme = create_thread_safety_theme(
                    "New_" + std::to_string(t) + "_" + std::to_string(i)
                );
                ctx.themes().register_theme(std::move(theme));
                ++register_count;
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }

    for (auto& thread : appliers) {
        thread.join();
    }
    for (auto& thread : registrars) {
        thread.join();
    }

    // All operations should complete without deadlock
    CHECK(apply_count == 100);     // 10 threads * 10 applies
    CHECK(register_count == 30);   // 3 threads * 10 registrations
}

// ============================================================================
// Render Context Creation Race Conditions
// ============================================================================

TEST_CASE("Thread Safety - Concurrent render context creation") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_thread_safety_theme("Render"));

    auto widget = std::make_unique<button<Backend>>("Test");
    widget->apply_theme("Render", ctx.themes());

    std::atomic<int> context_count{0};

    // Multiple threads create render contexts concurrently
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 50; ++j) {
                // Simulate render context creation (style resolution)
                auto style = widget->resolve_style();

                // Create measure context with style
                measure_context<Backend> mctx(style);
                ++context_count;

                std::this_thread::yield();
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    CHECK(context_count == 500);  // 10 threads * 50 contexts
}

// ============================================================================
// Reader-Writer Lock Contention
// ============================================================================

TEST_CASE("Thread Safety - Heavy read contention") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_thread_safety_theme("Read Heavy"));

    std::atomic<int> read_count{0};

    // 20 reader threads
    std::vector<std::thread> readers;
    for (int i = 0; i < 20; ++i) {
        readers.emplace_back([&]() {
            for (int j = 0; j < 500; ++j) {
                auto* theme = ctx.themes().get_theme("Read Heavy");
                if (theme) {
                    ++read_count;
                }
                std::this_thread::yield();
            }
        });
    }

    for (auto& thread : readers) {
        thread.join();
    }

    // All reads should succeed
    CHECK(read_count == 10000);  // 20 threads * 500 reads
}

TEST_CASE("Thread Safety - Mixed read/write contention") {
    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_thread_safety_theme("Initial"));

    std::atomic<int> read_count{0};
    std::atomic<int> write_count{0};

    // 10 reader threads
    std::vector<std::thread> readers;
    for (int i = 0; i < 10; ++i) {
        readers.emplace_back([&]() {
            for (int j = 0; j < 100; ++j) {
                auto* theme = ctx.themes().get_theme("Initial");
                if (theme) {
                    ++read_count;
                }
                std::this_thread::yield();
            }
        });
    }

    // 3 writer threads
    std::vector<std::thread> writers;
    for (int i = 0; i < 3; ++i) {
        writers.emplace_back([&, i]() {
            for (int j = 0; j < 10; ++j) {
                auto theme = create_thread_safety_theme(
                    "Write_" + std::to_string(i) + "_" + std::to_string(j)
                );
                ctx.themes().register_theme(std::move(theme));
                ++write_count;
                std::this_thread::sleep_for(std::chrono::microseconds(500));
            }
        });
    }

    for (auto& thread : readers) {
        thread.join();
    }
    for (auto& thread : writers) {
        thread.join();
    }

    // All operations should complete
    CHECK(read_count == 1000);  // 10 threads * 100 reads
    CHECK(write_count == 30);   // 3 threads * 10 writes
}

// ============================================================================
// Data Race Detection (for ThreadSanitizer)
// ============================================================================

TEST_CASE("Thread Safety - No data races in style resolution") {
    // This test primarily validates with ThreadSanitizer
    // Run with: cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" ...

    scoped_ui_context<Backend> ctx;
    ctx.themes().register_theme(create_thread_safety_theme("TSan Test"));

    auto widget = std::make_unique<button<Backend>>("Test");
    widget->apply_theme("TSan Test", ctx.themes());

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 100; ++j) {
                auto style = widget->resolve_style();
                // Access all fields to detect races
                [[maybe_unused]] auto bg = style.background_color;
                [[maybe_unused]] auto fg = style.foreground_color;
                [[maybe_unused]] auto border = style.border_color;
                [[maybe_unused]] auto opacity = style.opacity;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // If ThreadSanitizer detects races, test will fail
    CHECK(true);
}

TEST_CASE("Thread Safety - No data races in theme registry") {
    // Validates theme registry thread safety with ThreadSanitizer

    scoped_ui_context<Backend> ctx;

    std::vector<std::thread> threads;

    // Mix of readers and writers
    for (int i = 0; i < 20; ++i) {
        if (i % 3 == 0) {
            // Writer
            threads.emplace_back([&, i]() {
                for (int j = 0; j < 10; ++j) {
                    auto theme = create_thread_safety_theme("T" + std::to_string(i * 10 + j));
                    ctx.themes().register_theme(std::move(theme));
                }
            });
        } else {
            // Reader
            threads.emplace_back([&]() {
                for (int j = 0; j < 50; ++j) {
                    auto* theme = ctx.themes().get_theme("T0");
                    [[maybe_unused]] auto valid = (theme != nullptr);
                }
            });
        }
    }

    for (auto& thread : threads) {
        thread.join();
    }

    CHECK(true);
}

} // TEST_SUITE
