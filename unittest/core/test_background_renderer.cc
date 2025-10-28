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
        struct background_style {
            test_backend::color bg_color;
            char fill_char = ' ';

            bool operator==(const background_style&) const = default;
        };

        // Track calls
        std::vector<std::pair<test_backend::rect, background_style>> draw_background_calls;

        void draw_box(const test_backend::rect&, const box_style&) {}
        void draw_text(const test_backend::rect&, std::string_view, const font&) {}
        void draw_icon(const test_backend::rect&, const icon_style&) {}

        void draw_background(const test_backend::rect& r, const background_style& style) {
            draw_background_calls.push_back({r, style});
        }

        void draw_background(const test_backend::rect& r, const background_style& style,
                            const std::vector<test_backend::rect>&) {
            // For testing, just record the viewport call
            draw_background_calls.push_back({r, style});
        }

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
    };

    /**
     * @brief Test backend specialization for mocking
     */
    struct mock_test_backend {
        using rect_type = test_backend::rect;
        using size_type = test_backend::size;
        using point_type = test_backend::point;
        using color_type = test_backend::color;
        using event_type = test_backend::test_event;
        using keyboard_event_type = test_backend::test_keyboard_event;
        using mouse_button_event_type = test_backend::test_mouse_event;
        using mouse_motion_event_type = test_backend::test_mouse_event;
        using mouse_wheel_event_type = test_backend::test_mouse_event;
        using text_input_event_type = test_backend::test_event;
        using window_event_type = test_backend::test_window_event;
        using renderer_type = mock_renderer;
        using texture_type = void*;
        using font_type = void*;

        [[nodiscard]] static std::optional<onyxui::ui_event> create_event([[maybe_unused]] const event_type& native) noexcept {
            return std::nullopt;
        }

        static void register_themes([[maybe_unused]] theme_registry<mock_test_backend>& registry) {}
    };

    static_assert(UIBackend<mock_test_backend>, "mock_test_backend must satisfy UIBackend");
}

// =============================================================================
// Construction Tests
// =============================================================================

TEST_CASE("Background Renderer - Construction") {
    SUBCASE("Default constructor creates transparent background") {
        background_renderer<mock_test_backend> bg;
        CHECK_FALSE(bg.has_style());
    }

    SUBCASE("Constructor with color creates opaque background") {
        test_backend::color blue{0, 0, 170};
        background_renderer<mock_test_backend> bg(blue);
        CHECK(bg.has_style());

        auto style_opt = bg.get_style();
        REQUIRE(style_opt.has_value());
        CHECK(style_opt->bg_color.r == 0);
        CHECK(style_opt->bg_color.g == 0);
        CHECK(style_opt->bg_color.b == 170);
    }
}

// =============================================================================
// Style Management Tests
// =============================================================================

TEST_CASE("Background Renderer - Style management") {
    SUBCASE("Default is transparent (no style)") {
        background_renderer<mock_test_backend> bg;
        CHECK_FALSE(bg.has_style());
    }

    SUBCASE("set_color() enables rendering") {
        background_renderer<mock_test_backend> bg;
        test_backend::color red{255, 0, 0};
        bg.set_color(red);

        CHECK(bg.has_style());
        auto style_opt = bg.get_style();
        REQUIRE(style_opt.has_value());
        CHECK(style_opt->bg_color.r == 255);
        CHECK(style_opt->bg_color.g == 0);
        CHECK(style_opt->bg_color.b == 0);
    }

    SUBCASE("clear_style() disables rendering") {
        background_renderer<mock_test_backend> bg;
        bg.set_color({100, 100, 100});
        CHECK(bg.has_style());

        bg.clear_style();
        CHECK_FALSE(bg.has_style());
    }

    SUBCASE("Can toggle between transparent and opaque") {
        background_renderer<mock_test_backend> bg;

        bg.set_color({255, 0, 0});
        CHECK(bg.has_style());

        bg.clear_style();
        CHECK_FALSE(bg.has_style());

        bg.set_color({0, 255, 0});
        CHECK(bg.has_style());
    }

    SUBCASE("Multiple color changes are tracked") {
        background_renderer<mock_test_backend> bg;
        test_backend::color c1{255, 0, 0};
        test_backend::color c2{0, 255, 0};

        bg.set_color(c1);
        auto style1 = bg.get_style();
        REQUIRE(style1.has_value());
        CHECK(style1->bg_color == c1);

        bg.set_color(c2);
        auto style2 = bg.get_style();
        REQUIRE(style2.has_value());
        CHECK(style2->bg_color == c2);
    }
}

// =============================================================================
// Rendering Tests - With Style (Opaque)
// =============================================================================

TEST_CASE("Background Renderer - Opaque rendering") {
    SUBCASE("Renders with style when empty dirty regions fills entire viewport") {
        background_renderer<mock_test_backend> bg;
        test_backend::color blue{0, 0, 170};
        bg.set_color(blue);

        mock_renderer renderer;
        test_backend::rect viewport{0, 0, 80, 25};
        std::vector<test_backend::rect> dirty_regions;  // Empty

        bg.render(renderer, viewport, dirty_regions);

        // Should call draw_background once
        REQUIRE(renderer.draw_background_calls.size() == 1);
        CHECK(renderer.draw_background_calls[0].first == viewport);
        CHECK(renderer.draw_background_calls[0].second.bg_color == blue);
    }

    SUBCASE("Renders with dirty regions") {
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

        // Should call draw_background once (implementation may optimize)
        REQUIRE(renderer.draw_background_calls.size() >= 1);
        CHECK(renderer.draw_background_calls[0].second.bg_color == red);
    }

    SUBCASE("Renders with multiple dirty regions") {
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

        // Should call draw_background (implementation handles regions)
        CHECK_FALSE(renderer.draw_background_calls.empty());
    }
}

// =============================================================================
// Rendering Tests - Transparent (No Style)
// =============================================================================

TEST_CASE("Background Renderer - Transparent rendering") {
    SUBCASE("Transparent renders nothing") {
        background_renderer<mock_test_backend> bg;
        // Default is transparent

        mock_renderer renderer;
        test_backend::rect viewport{0, 0, 80, 25};
        std::vector<test_backend::rect> dirty_regions;

        bg.render(renderer, viewport, dirty_regions);

        // Should not call any renderer methods
        CHECK(renderer.draw_background_calls.empty());
    }

    SUBCASE("Transparent with dirty regions still renders nothing") {
        background_renderer<mock_test_backend> bg;
        bg.clear_style();  // Explicit transparent

        mock_renderer renderer;
        test_backend::rect viewport{0, 0, 80, 25};
        std::vector<test_backend::rect> dirty_regions{{10, 10, 20, 20}};

        bg.render(renderer, viewport, dirty_regions);

        // Should not call any renderer methods
        CHECK(renderer.draw_background_calls.empty());
    }

    SUBCASE("Setting color then clearing style makes it transparent") {
        background_renderer<mock_test_backend> bg;
        bg.set_color({255, 0, 0});
        bg.clear_style();

        mock_renderer renderer;
        test_backend::rect viewport{0, 0, 80, 25};
        std::vector<test_backend::rect> dirty_regions;

        bg.render(renderer, viewport, dirty_regions);

        CHECK(renderer.draw_background_calls.empty());
    }
}

// =============================================================================
// Edge Cases and State Tests
// =============================================================================

TEST_CASE("Background Renderer - Edge cases") {
    SUBCASE("Empty viewport with empty dirty regions") {
        background_renderer<mock_test_backend> bg;
        bg.set_color({255, 255, 255});

        mock_renderer renderer;
        test_backend::rect viewport{0, 0, 0, 0};  // Empty viewport
        std::vector<test_backend::rect> dirty_regions;

        bg.render(renderer, viewport, dirty_regions);

        // Should still call renderer (renderer handles empty rect)
        CHECK(renderer.draw_background_calls.size() == 1);
    }

    SUBCASE("Negative viewport coordinates") {
        background_renderer<mock_test_backend> bg;
        bg.set_color({255, 255, 255});

        mock_renderer renderer;
        test_backend::rect viewport{-10, -10, 100, 50};
        std::vector<test_backend::rect> dirty_regions;

        bg.render(renderer, viewport, dirty_regions);

        // Should pass through to renderer (renderer handles clipping)
        REQUIRE(renderer.draw_background_calls.size() == 1);
        CHECK(renderer.draw_background_calls[0].first == viewport);
    }

    SUBCASE("Large viewport") {
        background_renderer<mock_test_backend> bg;
        bg.set_color({200, 200, 200});

        mock_renderer renderer;
        test_backend::rect viewport{0, 0, 10000, 10000};
        std::vector<test_backend::rect> dirty_regions;

        bg.render(renderer, viewport, dirty_regions);

        // Should pass through (renderer handles large rectangles)
        REQUIRE(renderer.draw_background_calls.size() == 1);
        CHECK(renderer.draw_background_calls[0].first == viewport);
    }
}

// =============================================================================
// Copy and Move Semantics
// =============================================================================

TEST_CASE("Background Renderer - Copy and move semantics") {
    SUBCASE("Copy constructor copies style") {
        background_renderer<mock_test_backend> bg1;
        bg1.set_color({100, 100, 100});

        background_renderer<mock_test_backend> bg2 = bg1;

        CHECK(bg2.has_style());
        auto style = bg2.get_style();
        REQUIRE(style.has_value());
        CHECK(style->bg_color.r == 100);
        CHECK(style->bg_color.g == 100);
        CHECK(style->bg_color.b == 100);
    }

    SUBCASE("Copy assignment copies style") {
        background_renderer<mock_test_backend> bg1;
        bg1.set_color({150, 150, 150});

        background_renderer<mock_test_backend> bg2;
        bg2 = bg1;

        CHECK(bg2.has_style());
        auto style = bg2.get_style();
        REQUIRE(style.has_value());
        CHECK(style->bg_color.r == 150);
    }

    SUBCASE("Move constructor transfers style") {
        background_renderer<mock_test_backend> bg1;
        bg1.set_color({200, 200, 200});

        background_renderer<mock_test_backend> bg2 = std::move(bg1);

        CHECK(bg2.has_style());
        auto style = bg2.get_style();
        REQUIRE(style.has_value());
        CHECK(style->bg_color.r == 200);
    }

    SUBCASE("Move assignment transfers style") {
        background_renderer<mock_test_backend> bg1;
        bg1.set_color({250, 250, 250});

        background_renderer<mock_test_backend> bg2;
        bg2 = std::move(bg1);

        CHECK(bg2.has_style());
        auto style = bg2.get_style();
        REQUIRE(style.has_value());
        CHECK(style->bg_color.r == 250);
    }

    SUBCASE("Copying transparent background") {
        background_renderer<mock_test_backend> bg1;  // Transparent

        background_renderer<mock_test_backend> bg2 = bg1;

        CHECK_FALSE(bg2.has_style());
    }
}

// =============================================================================
// ui_context Integration Tests
// =============================================================================

TEST_CASE("Background Renderer - ui_context integration") {
    SUBCASE("ui_services provides background_renderer instance") {
        scoped_ui_context<mock_test_backend> ctx;

        auto* bg = ui_services<mock_test_backend>::background();
        REQUIRE(bg != nullptr);

        // Default is transparent
        CHECK_FALSE(bg->has_style());
    }

    SUBCASE("Can configure background through ui_services") {
        scoped_ui_context<mock_test_backend> ctx;

        auto* bg = ui_services<mock_test_backend>::background();
        REQUIRE(bg != nullptr);

        test_backend::color teal{0, 128, 128};
        bg->set_color(teal);

        CHECK(bg->has_style());
        auto style = bg->get_style();
        REQUIRE(style.has_value());
        CHECK(style->bg_color == teal);
    }

    SUBCASE("Multiple contexts have independent backgrounds") {
        scoped_ui_context<mock_test_backend> ctx1;
        auto* bg1 = ui_services<mock_test_backend>::background();
        bg1->set_color({255, 0, 0});

        {
            scoped_ui_context<mock_test_backend> ctx2;
            auto* bg2 = ui_services<mock_test_backend>::background();

            // New context should have independent background
            CHECK_FALSE(bg2->has_style());

            bg2->set_color({0, 255, 0});
            CHECK(bg2->has_style());
        }

        // Original context should still have red background
        auto style1 = bg1->get_style();
        REQUIRE(style1.has_value());
        CHECK(style1->bg_color.r == 255);
        CHECK(style1->bg_color.g == 0);
        CHECK(style1->bg_color.b == 0);
    }
}
