/**
 * @file test_background_renderer.cc
 * @brief Comprehensive unit tests for background_renderer
 * @author Testing Infrastructure Team
 * @date 2025-10-25
 */

#include <doctest/doctest.h>
#include <onyxui/background_renderer.hh>
#include <onyxui/ui_context.hh>
#include <onyxui/ui_services.hh>
#include "../utils/test_backend.hh"
#include <vector>

using namespace onyxui;

namespace {
    /**
     * @brief Mock renderer that tracks method calls for verification
     */
    struct mock_renderer {
        using size_type = test_backend::size;

        struct box_style {
            bool draw_border = false;
            bool operator==(const box_style&) const = default;
        };
        struct line_style {
            char line_char = '-';
            bool operator==(const line_style&) const = default;
        };
        struct icon_style {
            bool operator==(const icon_style&) const = default;
        };
        struct font {
            bool operator==(const font&) const = default;
        };

        // Track calls
        std::vector<test_backend::color> foreground_calls;
        std::vector<test_backend::color> background_calls;
        std::vector<test_backend::rect> draw_box_calls;

        void set_foreground(const test_backend::color& c) {
            foreground_calls.push_back(c);
        }

        void set_background(const test_backend::color& c) {
            background_calls.push_back(c);
        }

        void draw_box(const test_backend::rect& r, const box_style&) {
            draw_box_calls.push_back(r);
        }

        void draw_text(const test_backend::rect&, std::string_view, const font&) {}
        void draw_icon(const test_backend::rect&, const icon_style&) {}
        void clear_region(const test_backend::rect&) {}
        void draw_horizontal_line(const test_backend::rect&, const line_style&) {}
        void draw_vertical_line(const test_backend::rect&, const line_style&) {}

        static size_type measure_text(std::string_view, const font&) {
            return {0, 0};
        }

        static size_type get_icon_size(const icon_style&) {
            return {1, 1};
        }

        static int get_border_thickness(const box_style&) {
            return 0;
        }

        test_backend::rect get_viewport() const {
            return {0, 0, 80, 25};
        }

        void push_clip(const test_backend::rect&) {}
        void pop_clip() {}
        test_backend::rect get_clip_rect() const { return {0, 0, 80, 25}; }
        void present() {}
        void on_resize() {}

        // Reset tracking
        void reset() {
            foreground_calls.clear();
            background_calls.clear();
            draw_box_calls.clear();
        }
    };

    /**
     * @brief Test backend using mock_renderer
     */
    struct mock_test_backend {
        using rect_type = test_backend::rect;
        using size_type = test_backend::size;
        using point_type = test_backend::point;
        using color_type = test_backend::color;
        using event_type = test_backend::test_keyboard_event;
        using keyboard_event_type = test_backend::test_keyboard_event;
        using mouse_button_event_type = test_backend::test_mouse_event;
        using mouse_motion_event_type = test_backend::test_mouse_event;
        using mouse_wheel_event_type = test_backend::test_mouse_event;
        using renderer_type = mock_renderer;

        [[nodiscard]] static std::optional<onyxui::ui_event> create_event([[maybe_unused]] const test_backend::test_keyboard_event&) noexcept {
            return std::nullopt;
        }

        static void register_themes(theme_registry<mock_test_backend>&) {}
    };
}

// =============================================================================
// Mode Switching Tests
// =============================================================================

TEST_CASE("Background Renderer - Mode switching") {
    SUBCASE("Default mode is solid") {
        background_renderer<mock_test_backend> bg;
        CHECK(bg.get_mode() == background_mode::solid);
    }

    SUBCASE("Can switch to transparent mode") {
        background_renderer<mock_test_backend> bg;
        bg.set_mode(background_mode::transparent);
        CHECK(bg.get_mode() == background_mode::transparent);
    }

    SUBCASE("Can switch to pattern mode") {
        background_renderer<mock_test_backend> bg;
        bg.set_mode(background_mode::pattern);
        CHECK(bg.get_mode() == background_mode::pattern);
    }

    SUBCASE("Can switch back to solid mode") {
        background_renderer<mock_test_backend> bg;
        bg.set_mode(background_mode::transparent);
        bg.set_mode(background_mode::solid);
        CHECK(bg.get_mode() == background_mode::solid);
    }
}

// =============================================================================
// Color Management Tests
// =============================================================================

TEST_CASE("Background Renderer - Color management") {
    SUBCASE("Default color is default-constructed") {
        background_renderer<mock_test_backend> bg;
        auto color = bg.get_color();
        CHECK(color.r == 0);
        CHECK(color.g == 0);
        CHECK(color.b == 0);
    }

    SUBCASE("Constructor with color sets initial color") {
        test_backend::color blue{0, 0, 170};
        background_renderer<mock_test_backend> bg(blue);
        auto color = bg.get_color();
        CHECK(color.r == 0);
        CHECK(color.g == 0);
        CHECK(color.b == 170);
    }

    SUBCASE("Can change color via setter") {
        background_renderer<mock_test_backend> bg;
        test_backend::color red{255, 0, 0};
        bg.set_color(red);
        auto color = bg.get_color();
        CHECK(color.r == 255);
        CHECK(color.g == 0);
        CHECK(color.b == 0);
    }

    SUBCASE("Multiple color changes are tracked") {
        background_renderer<mock_test_backend> bg;
        test_backend::color c1{255, 0, 0};
        test_backend::color c2{0, 255, 0};
        bg.set_color(c1);
        CHECK(bg.get_color() == c1);
        bg.set_color(c2);
        CHECK(bg.get_color() == c2);
    }
}

// =============================================================================
// Rendering Tests - Solid Mode
// =============================================================================

TEST_CASE("Background Renderer - Solid mode rendering") {
    SUBCASE("Solid mode with empty dirty regions fills entire viewport") {
        background_renderer<mock_test_backend> bg;
        test_backend::color blue{0, 0, 170};
        bg.set_color(blue);
        bg.set_mode(background_mode::solid);

        mock_renderer renderer;
        test_backend::rect viewport{0, 0, 80, 25};
        std::vector<test_backend::rect> dirty_regions;  // Empty

        bg.render(renderer, viewport, dirty_regions);

        // Should set colors
        REQUIRE(renderer.foreground_calls.size() == 1);
        REQUIRE(renderer.background_calls.size() == 1);
        CHECK(renderer.foreground_calls[0] == blue);
        CHECK(renderer.background_calls[0] == blue);

        // Should draw entire viewport
        REQUIRE(renderer.draw_box_calls.size() == 1);
        CHECK(renderer.draw_box_calls[0] == viewport);
    }

    SUBCASE("Solid mode with dirty regions fills only those regions") {
        background_renderer<mock_test_backend> bg;
        test_backend::color red{255, 0, 0};
        bg.set_color(red);

        mock_renderer renderer;
        test_backend::rect viewport{0, 0, 80, 25};
        std::vector<test_backend::rect> dirty_regions{
            {10, 10, 20, 10},
            {40, 5, 15, 8}
        };

        bg.render(renderer, viewport, dirty_regions);

        // Should set colors
        CHECK(renderer.foreground_calls.size() == 1);
        CHECK(renderer.background_calls.size() == 1);

        // Should draw exactly the dirty regions
        REQUIRE(renderer.draw_box_calls.size() == 2);
        CHECK(renderer.draw_box_calls[0] == dirty_regions[0]);
        CHECK(renderer.draw_box_calls[1] == dirty_regions[1]);
    }

    SUBCASE("Solid mode with multiple dirty regions") {
        background_renderer<mock_test_backend> bg;
        bg.set_color({128, 128, 128});

        mock_renderer renderer;
        test_backend::rect viewport{0, 0, 100, 50};
        std::vector<test_backend::rect> dirty_regions{
            {0, 0, 10, 10},
            {20, 20, 15, 15},
            {50, 10, 30, 20},
            {90, 45, 10, 5}
        };

        bg.render(renderer, viewport, dirty_regions);

        // Should draw all dirty regions
        REQUIRE(renderer.draw_box_calls.size() == 4);
        for (size_t i = 0; i < dirty_regions.size(); ++i) {
            CHECK(renderer.draw_box_calls[i] == dirty_regions[i]);
        }
    }
}

// =============================================================================
// Rendering Tests - Transparent Mode
// =============================================================================

TEST_CASE("Background Renderer - Transparent mode rendering") {
    SUBCASE("Transparent mode renders nothing") {
        background_renderer<mock_test_backend> bg;
        bg.set_mode(background_mode::transparent);
        bg.set_color({255, 255, 255});  // Color should be ignored

        mock_renderer renderer;
        test_backend::rect viewport{0, 0, 80, 25};
        std::vector<test_backend::rect> dirty_regions;

        bg.render(renderer, viewport, dirty_regions);

        // Should not call any renderer methods
        CHECK(renderer.foreground_calls.empty());
        CHECK(renderer.background_calls.empty());
        CHECK(renderer.draw_box_calls.empty());
    }

    SUBCASE("Transparent mode with dirty regions still renders nothing") {
        background_renderer<mock_test_backend> bg;
        bg.set_mode(background_mode::transparent);

        mock_renderer renderer;
        test_backend::rect viewport{0, 0, 80, 25};
        std::vector<test_backend::rect> dirty_regions{{10, 10, 20, 20}};

        bg.render(renderer, viewport, dirty_regions);

        // Should not call any renderer methods
        CHECK(renderer.foreground_calls.empty());
        CHECK(renderer.background_calls.empty());
        CHECK(renderer.draw_box_calls.empty());
    }
}

// =============================================================================
// Rendering Tests - Pattern Mode (Future)
// =============================================================================

TEST_CASE("Background Renderer - Pattern mode rendering") {
    SUBCASE("Pattern mode falls back to solid mode") {
        background_renderer<mock_test_backend> bg;
        test_backend::color green{0, 255, 0};
        bg.set_color(green);
        bg.set_mode(background_mode::pattern);

        mock_renderer renderer;
        test_backend::rect viewport{0, 0, 80, 25};
        std::vector<test_backend::rect> dirty_regions;

        bg.render(renderer, viewport, dirty_regions);

        // Should behave like solid mode (fallback)
        CHECK(renderer.foreground_calls.size() == 1);
        CHECK(renderer.background_calls.size() == 1);
        REQUIRE(renderer.draw_box_calls.size() == 1);
        CHECK(renderer.draw_box_calls[0] == viewport);
    }
}

// =============================================================================
// Edge Cases and State Tests
// =============================================================================

TEST_CASE("Background Renderer - Edge cases") {
    SUBCASE("Empty viewport with empty dirty regions") {
        background_renderer<mock_test_backend> bg;
        bg.set_mode(background_mode::solid);

        mock_renderer renderer;
        test_backend::rect viewport{0, 0, 0, 0};  // Empty viewport
        std::vector<test_backend::rect> dirty_regions;

        bg.render(renderer, viewport, dirty_regions);

        // Should still call renderer methods (renderer handles empty rect)
        CHECK(renderer.foreground_calls.size() == 1);
        CHECK(renderer.background_calls.size() == 1);
        CHECK(renderer.draw_box_calls.size() == 1);
    }

    SUBCASE("Negative viewport coordinates") {
        background_renderer<mock_test_backend> bg;
        bg.set_mode(background_mode::solid);

        mock_renderer renderer;
        test_backend::rect viewport{-10, -10, 100, 50};
        std::vector<test_backend::rect> dirty_regions;

        bg.render(renderer, viewport, dirty_regions);

        // Should pass through to renderer (renderer handles clipping)
        REQUIRE(renderer.draw_box_calls.size() == 1);
        CHECK(renderer.draw_box_calls[0] == viewport);
    }

    SUBCASE("Single dirty region should work") {
        background_renderer<mock_test_backend> bg;
        bg.set_mode(background_mode::solid);

        mock_renderer renderer;
        test_backend::rect const viewport{0, 0, 80, 25};
        std::vector<test_backend::rect> dirty_regions{{5, 5, 10, 10}};

        bg.render(renderer, viewport, dirty_regions);

        REQUIRE(renderer.draw_box_calls.size() == 1);
        CHECK(renderer.draw_box_calls[0] == dirty_regions[0]);
    }
}

// =============================================================================
// Integration with ui_context
// =============================================================================

TEST_CASE("Background Renderer - ui_context integration") {
    SUBCASE("ui_context provides background_renderer instance") {
        scoped_ui_context<test_backend> ctx;

        auto* bg = ui_services<test_backend>::background();
        REQUIRE(bg != nullptr);
    }

    SUBCASE("Can configure background via ui_services") {
        scoped_ui_context<test_backend> ctx;

        auto* bg = ui_services<test_backend>::background();
        REQUIRE(bg != nullptr);

        bg->set_mode(background_mode::transparent);
        bg->set_color({255, 0, 0});

        CHECK(bg->get_mode() == background_mode::transparent);
        auto color = bg->get_color();
        CHECK(color.r == 255);
        CHECK(color.g == 0);
        CHECK(color.b == 0);
    }

    SUBCASE("Multiple contexts have independent background renderers") {
        test_backend::color color1{255, 0, 0};
        test_backend::color color2{0, 255, 0};

        {
            scoped_ui_context<test_backend> ctx1;
            auto* bg1 = ui_services<test_backend>::background();
            bg1->set_color(color1);
            bg1->set_mode(background_mode::solid);

            {
                scoped_ui_context<test_backend> ctx2;
                auto* bg2 = ui_services<test_backend>::background();
                bg2->set_color(color2);
                bg2->set_mode(background_mode::transparent);

                // ctx2 is active
                CHECK(bg2->get_color() == color2);
                CHECK(bg2->get_mode() == background_mode::transparent);
            }

            // ctx1 is active again
            auto* bg1_again = ui_services<test_backend>::background();
            CHECK(bg1_again->get_color() == color1);
            CHECK(bg1_again->get_mode() == background_mode::solid);
        }
    }

    SUBCASE("background() returns nullptr when no context") {
        // No context active
        auto* bg = ui_services<test_backend>::background();
        CHECK(bg == nullptr);
    }
}

// =============================================================================
// Copy and Move Semantics
// =============================================================================

TEST_CASE("Background Renderer - Copy and move semantics") {
    SUBCASE("Copy constructor preserves state") {
        test_backend::color red{255, 0, 0};
        background_renderer<mock_test_backend> bg1;
        bg1.set_mode(background_mode::transparent);
        bg1.set_color(red);

        background_renderer<mock_test_backend> bg2 = bg1;  // Copy

        CHECK(bg2.get_mode() == background_mode::transparent);
        CHECK(bg2.get_color() == red);
    }

    SUBCASE("Copy assignment preserves state") {
        test_backend::color blue{0, 0, 255};
        background_renderer<mock_test_backend> bg1;
        bg1.set_mode(background_mode::pattern);
        bg1.set_color(blue);

        background_renderer<mock_test_backend> bg2;
        bg2 = bg1;  // Copy assignment

        CHECK(bg2.get_mode() == background_mode::pattern);
        CHECK(bg2.get_color() == blue);
    }

    SUBCASE("Move constructor transfers state") {
        test_backend::color green{0, 255, 0};
        background_renderer<mock_test_backend> bg1;
        bg1.set_mode(background_mode::solid);
        bg1.set_color(green);

        background_renderer<mock_test_backend> bg2 = std::move(bg1);  // Move

        CHECK(bg2.get_mode() == background_mode::solid);
        CHECK(bg2.get_color() == green);
    }

    SUBCASE("Move assignment transfers state") {
        test_backend::color yellow{255, 255, 0};
        background_renderer<mock_test_backend> bg1;
        bg1.set_mode(background_mode::transparent);
        bg1.set_color(yellow);

        background_renderer<mock_test_backend> bg2;
        bg2 = std::move(bg1);  // Move assignment

        CHECK(bg2.get_mode() == background_mode::transparent);
        CHECK(bg2.get_color() == yellow);
    }
}
