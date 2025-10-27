/**
 * @file test_render_context.cc
 * @brief Unit tests for render_context, draw_context, and measure_context
 * @date 19/10/2025
 */

#include <doctest/doctest.h>
#include <memory>
#include <onyxui/render_context.hh>
#include <onyxui/draw_context.hh>
#include <onyxui/measure_context.hh>
#include <onyxui/resolved_style.hh>
#include <onyxui/concepts/size_like.hh>
#include <onyxui/concepts/rect_like.hh>
#include <utility>
#include "../utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

// Helper function to create a default resolved_style for tests
inline resolved_style<Backend> make_default_style() {
    return resolved_style<Backend>{
        .background_color = Backend::color_type{},
        .foreground_color = Backend::color_type{},
        .border_color = Backend::color_type{},
        .box_style = Backend::renderer_type::box_style{},
        .font = Backend::renderer_type::font{},
        .opacity = 1.0f,
        .icon_style = std::optional<Backend::renderer_type::icon_style>{},
        .padding_horizontal = std::optional<int>{},
        .padding_vertical = std::optional<int>{},
        .mnemonic_font = std::optional<Backend::renderer_type::font>{}
    };
}

TEST_CASE("measure_context - Basic functionality") {
    SUBCASE("Initial state is zero size") {
        measure_context<Backend> const ctx(make_default_style());
        auto size = ctx.get_size();
        CHECK(size_utils::get_width(size) == 0);
        CHECK(size_utils::get_height(size) == 0);
    }

    SUBCASE("renderer() returns nullptr") {
        measure_context<Backend> ctx(make_default_style());
        CHECK(ctx.renderer() == nullptr);
    }
}

TEST_CASE("measure_context - Text measurement") {
    SUBCASE("Single text at origin") {
        measure_context<Backend> ctx(make_default_style());

        Backend::point_type const pos{0, 0};
        Backend::renderer_type::font const font;
        Backend::color_type const color;

        // Draw text "Hello" (5 chars, 1 line height)
        auto text_size = ctx.draw_text("Hello", pos, font, color);

        // Verify returned size
        CHECK(size_utils::get_width(text_size) == 5);
        CHECK(size_utils::get_height(text_size) == 1);

        // Verify tracked size
        auto measured = ctx.get_size();
        CHECK(size_utils::get_width(measured) == 5);
        CHECK(size_utils::get_height(measured) == 1);
    }

    SUBCASE("Text at offset position") {
        measure_context<Backend> ctx(make_default_style());

        Backend::point_type const pos{10, 5};
        Backend::renderer_type::font const font;
        Backend::color_type const color;

        // Draw text "Hi" (2 chars) at (10, 5)
        (void)ctx.draw_text("Hi", pos, font, color);

        // Bounding box: min={10,5}, max={12,6} → size = {2, 1}
        // Position is ignored - only size matters for measurement
        auto measured = ctx.get_size();
        CHECK(size_utils::get_width(measured) == 2);
        CHECK(size_utils::get_height(measured) == 1);
    }
}

TEST_CASE("measure_context - Rectangle measurement") {
    SUBCASE("Single rectangle at origin") {
        measure_context<Backend> ctx(make_default_style());

        Backend::rect_type rect;
        rect_utils::set_bounds(rect, 0, 0, 10, 5);
        Backend::renderer_type::box_style const style{};

        ctx.draw_rect(rect, style);

        // Should track rectangle bounds
        auto measured = ctx.get_size();
        CHECK(size_utils::get_width(measured) == 10);
        CHECK(size_utils::get_height(measured) == 5);
    }

    SUBCASE("Rectangle at offset position") {
        measure_context<Backend> ctx(make_default_style());

        Backend::rect_type rect;
        rect_utils::set_bounds(rect, 5, 3, 8, 4);  // (x, y, w, h)
        Backend::renderer_type::box_style const style{};

        ctx.draw_rect(rect, style);

        // Bounding box: min={5,3}, max={13,7} → size = {8, 4}
        // Position is ignored - only size matters for measurement
        auto measured = ctx.get_size();
        CHECK(size_utils::get_width(measured) == 8);
        CHECK(size_utils::get_height(measured) == 4);
    }
}

TEST_CASE("measure_context - Reset functionality") {
    SUBCASE("Reset clears tracked size") {
        measure_context<Backend> ctx(make_default_style());

        // Measure some text
        Backend::point_type const pos{0, 0};
        Backend::renderer_type::font const font;
        Backend::color_type const color;
        (void)ctx.draw_text("Hello", pos, font, color);

        // Verify size is tracked
        auto measured1 = ctx.get_size();
        CHECK(size_utils::get_width(measured1) == 5);

        // Reset
        ctx.reset();

        // Verify size is zero
        auto measured2 = ctx.get_size();
        CHECK(size_utils::get_width(measured2) == 0);
        CHECK(size_utils::get_height(measured2) == 0);
    }
}

TEST_CASE("draw_context - Basic functionality") {
    SUBCASE("renderer() returns non-null") {
        Backend::renderer_type renderer;
        draw_context<Backend> ctx(renderer);

        CHECK(ctx.renderer() != nullptr);
        CHECK(ctx.renderer() == &renderer);
    }
}

TEST_CASE("draw_context - Text rendering") {
    SUBCASE("draw_text returns correct size") {
        Backend::renderer_type renderer;
        draw_context<Backend> ctx(renderer);

        Backend::point_type const pos{10, 5};
        Backend::renderer_type::font const font;
        Backend::color_type const color;

        // Draw text
        auto text_size = ctx.draw_text("Hello", pos, font, color);

        // Should return measured text size
        CHECK(size_utils::get_width(text_size) == 5);
        CHECK(size_utils::get_height(text_size) == 1);
    }
}

TEST_CASE("draw_context - Rectangle rendering") {
    SUBCASE("draw_rect does not throw") {
        Backend::renderer_type renderer;
        draw_context<Backend> ctx(renderer);

        Backend::rect_type rect;
        rect_utils::set_bounds(rect, 0, 0, 10, 5);
        Backend::renderer_type::box_style const style{};

        // Should not throw
        CHECK_NOTHROW(ctx.draw_rect(rect, style));
    }
}

TEST_CASE("Polymorphic usage") {
    SUBCASE("Can use render_context* to point to measure_context") {
        auto ctx = std::make_unique<measure_context<Backend>>(make_default_style());
        render_context<Backend>* base = ctx.get();

        // measure_context has no renderer
        CHECK(base->renderer() == nullptr);
    }

    SUBCASE("Can use render_context* to point to draw_context") {
        Backend::renderer_type renderer;
        auto ctx = std::make_unique<draw_context<Backend>>(renderer);
        render_context<Backend>* base = ctx.get();

        // draw_context has valid renderer
        CHECK(base->renderer() != nullptr);
    }
}

TEST_CASE("Context lifetimes") {
    SUBCASE("measure_context is moveable") {
        measure_context<Backend> ctx1(make_default_style());

        Backend::point_type const pos{0, 0};
        Backend::renderer_type::font const font;
        Backend::color_type const color;
        (void)ctx1.draw_text("Hello", pos, font, color);

        // Move
        measure_context<Backend> const ctx2 = std::move(ctx1);

        // Moved context retains state
        auto measured = ctx2.get_size();
        CHECK(size_utils::get_width(measured) == 5);
    }

    SUBCASE("draw_context is moveable") {
        Backend::renderer_type renderer;
        draw_context<Backend> ctx1(renderer);

        // Move
        draw_context<Backend> const ctx2 = std::move(ctx1);

        // Moved context retains renderer
        CHECK(ctx2.renderer() != nullptr);
    }
}

// ============================================================================
// resolved_style Integration Tests
// ============================================================================

#include <onyxui/resolved_style.hh>
#include <onyxui/theme.hh>

TEST_CASE("render_context - Style accessor returns resolved_style") {
    SUBCASE("measure_context with style") {
        resolved_style<Backend> style{
            .background_color = Backend::color_type{0, 255, 0},
            .foreground_color = Backend::color_type{255, 0, 0},
            .border_color = Backend::color_type{0, 0, 0},
            .box_style = Backend::renderer_type::box_style{},
            .font = Backend::renderer_type::font{},
            .opacity = 1.0f,
            .icon_style = std::optional<Backend::renderer_type::icon_style>{},
            .padding_horizontal = std::optional<int>{},
            .padding_vertical = std::optional<int>{},
            .mnemonic_font = std::optional<Backend::renderer_type::font>{}
        };

        measure_context<Backend> ctx(style);

        CHECK(ctx.style().foreground_color.value.r == 255);
        CHECK(ctx.style().background_color.value.g == 255);
    }

    SUBCASE("draw_context with style") {
        Backend::renderer_type renderer;
        resolved_style<Backend> style{
            .background_color = Backend::color_type{200, 200, 200},
            .foreground_color = Backend::color_type{100, 100, 100},
            .border_color = Backend::color_type{0, 0, 0},
            .box_style = Backend::renderer_type::box_style{},
            .font = Backend::renderer_type::font{},
            .opacity = 1.0f,
            .icon_style = std::optional<Backend::renderer_type::icon_style>{},
            .padding_horizontal = std::optional<int>{},
            .padding_vertical = std::optional<int>{},
            .mnemonic_font = std::optional<Backend::renderer_type::font>{}
        };

        draw_context<Backend> ctx(renderer, style);

        CHECK(ctx.style().foreground_color.value.r == 100);
        CHECK(ctx.style().background_color.value.r == 200);
    }
}

TEST_CASE("draw_context - Constructor accepts resolved_style") {
    Backend::renderer_type renderer;
    resolved_style<Backend> style{
        .background_color = Backend::color_type{0, 0, 0},
        .foreground_color = Backend::color_type{100, 100, 100},
        .border_color = Backend::color_type{0, 0, 0},
        .box_style = Backend::renderer_type::box_style{},
        .font = Backend::renderer_type::font{},
        .opacity = 1.0f,
        .icon_style = std::optional<Backend::renderer_type::icon_style>{},
        .padding_horizontal = std::optional<int>{},
        .padding_vertical = std::optional<int>{},
        .mnemonic_font = std::optional<Backend::renderer_type::font>{}
    };

    draw_context<Backend> ctx(renderer, style);

    CHECK(ctx.style().foreground_color.value.r == 100);
    CHECK(ctx.renderer() != nullptr);
}

TEST_CASE("measure_context - Constructor accepts resolved_style") {
    resolved_style<Backend> style{
        .background_color = Backend::color_type{200, 200, 200},
        .foreground_color = Backend::color_type{50, 50, 50},
        .border_color = Backend::color_type{0, 0, 0},
        .box_style = Backend::renderer_type::box_style{},
        .font = Backend::renderer_type::font{},
        .opacity = 1.0f,
        .icon_style = std::optional<Backend::renderer_type::icon_style>{},
        .padding_horizontal = std::optional<int>{},
        .padding_vertical = std::optional<int>{},
        .mnemonic_font = std::optional<Backend::renderer_type::font>{}
    };

    measure_context<Backend> ctx(style);

    CHECK(ctx.style().foreground_color.value.r == 50);
    CHECK(ctx.style().background_color.value.r == 200);
    CHECK(ctx.renderer() == nullptr);
}

TEST_CASE("render_context - Style is passed to context") {
    resolved_style<Backend> style1{
        .background_color = Backend::color_type{50, 50, 50},
        .foreground_color = Backend::color_type{100, 100, 100},
        .border_color = Backend::color_type{0, 0, 0},
        .box_style = Backend::renderer_type::box_style{},
        .font = Backend::renderer_type::font{},
        .opacity = 1.0f,
        .icon_style = std::optional<Backend::renderer_type::icon_style>{},
        .padding_horizontal = std::optional<int>{},
        .padding_vertical = std::optional<int>{},
        .mnemonic_font = std::optional<Backend::renderer_type::font>{}
    };

    resolved_style<Backend> style2{
        .background_color = Backend::color_type{150, 150, 150},
        .foreground_color = Backend::color_type{200, 200, 200},
        .border_color = Backend::color_type{0, 0, 0},
        .box_style = Backend::renderer_type::box_style{},
        .font = Backend::renderer_type::font{},
        .opacity = 1.0f,
        .icon_style = std::optional<Backend::renderer_type::icon_style>{},
        .padding_horizontal = std::optional<int>{},
        .padding_vertical = std::optional<int>{},
        .mnemonic_font = std::optional<Backend::renderer_type::font>{}
    };

    measure_context<Backend> ctx1(style1);
    measure_context<Backend> ctx2(style2);

    // Each context should have its own style
    CHECK(ctx1.style().foreground_color.value.r == 100);
    CHECK(ctx2.style().foreground_color.value.r == 200);
    CHECK(ctx1.style().background_color.value.r == 50);
    CHECK(ctx2.style().background_color.value.r == 150);
}

TEST_CASE("render_context - Style default construction") {
    SUBCASE("measure_context with default-valued style") {
        resolved_style<Backend> style{
            .background_color = Backend::color_type{0, 0, 0},
            .foreground_color = Backend::color_type{0, 0, 0},
            .border_color = Backend::color_type{0, 0, 0},
            .box_style = Backend::renderer_type::box_style{},
            .font = Backend::renderer_type::font{},
            .opacity = 1.0f,
            .icon_style = std::optional<Backend::renderer_type::icon_style>{},
            .padding_horizontal = std::optional<int>{},
            .padding_vertical = std::optional<int>{},
            .mnemonic_font = std::optional<Backend::renderer_type::font>{}
        };
        measure_context<Backend> ctx(style);

        // Should have default values
        CHECK(ctx.style().foreground_color.value.r == 0);
        CHECK(ctx.style().foreground_color.value.g == 0);
        CHECK(ctx.style().foreground_color.value.b == 0);
    }

    SUBCASE("draw_context with default-valued style") {
        Backend::renderer_type renderer;
        resolved_style<Backend> style{
            .background_color = Backend::color_type{0, 0, 0},
            .foreground_color = Backend::color_type{0, 0, 0},
            .border_color = Backend::color_type{0, 0, 0},
            .box_style = Backend::renderer_type::box_style{},
            .font = Backend::renderer_type::font{},
            .opacity = 1.0f,
            .icon_style = std::optional<Backend::renderer_type::icon_style>{},
            .padding_horizontal = std::optional<int>{},
            .padding_vertical = std::optional<int>{},
            .mnemonic_font = std::optional<Backend::renderer_type::font>{}
        };
        draw_context<Backend> ctx(renderer, style);

        // Should have default values
        CHECK(ctx.style().background_color.value.r == 0);
        CHECK(ctx.style().background_color.value.g == 0);
        CHECK(ctx.style().background_color.value.b == 0);
    }
}

TEST_CASE("render_context - Style is copied on construction") {
    resolved_style<Backend> style{
        .background_color = Backend::color_type{0, 0, 0},
        .foreground_color = Backend::color_type{123, 45, 67},
        .border_color = Backend::color_type{0, 0, 0},
        .box_style = Backend::renderer_type::box_style{},
        .font = Backend::renderer_type::font{},
        .opacity = 1.0f,
        .icon_style = std::optional<Backend::renderer_type::icon_style>{},
        .padding_horizontal = std::optional<int>{},
        .padding_vertical = std::optional<int>{},
        .mnemonic_font = std::optional<Backend::renderer_type::font>{}
    };

    measure_context<Backend> ctx(style);

    // Create new style with different values
    resolved_style<Backend> modified_style{
        .background_color = Backend::color_type{0, 0, 0},
        .foreground_color = Backend::color_type{255, 255, 255},
        .border_color = Backend::color_type{0, 0, 0},
        .box_style = Backend::renderer_type::box_style{},
        .font = Backend::renderer_type::font{},
        .opacity = 1.0f,
        .icon_style = std::optional<Backend::renderer_type::icon_style>{},
        .padding_horizontal = std::optional<int>{},
        .padding_vertical = std::optional<int>{},
        .mnemonic_font = std::optional<Backend::renderer_type::font>{}
    };

    // Context should retain original values
    CHECK(ctx.style().foreground_color.value.r == 123);
    CHECK(ctx.style().foreground_color.value.g == 45);
    CHECK(ctx.style().foreground_color.value.b == 67);
}
