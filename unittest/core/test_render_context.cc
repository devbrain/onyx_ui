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
#include <onyxui/concepts/size_like.hh>
#include <onyxui/concepts/point_like.hh>
#include <onyxui/concepts/rect_like.hh>
#include "../utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

TEST_CASE("measure_context - Basic functionality") {
    SUBCASE("Initial state is zero size") {
        measure_context<Backend> ctx;
        auto size = ctx.get_size();
        CHECK(size_utils::get_width(size) == 0);
        CHECK(size_utils::get_height(size) == 0);
    }

    SUBCASE("is_measuring returns true") {
        measure_context<Backend> ctx;
        CHECK(ctx.is_measuring() == true);
        CHECK(ctx.is_rendering() == false);
    }

    SUBCASE("renderer() returns nullptr") {
        measure_context<Backend> ctx;
        CHECK(ctx.renderer() == nullptr);
    }
}

TEST_CASE("measure_context - Text measurement") {
    SUBCASE("Single text at origin") {
        measure_context<Backend> ctx;

        Backend::point_type pos{0, 0};
        Backend::renderer_type::font font;
        Backend::color_type color;

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
        measure_context<Backend> ctx;

        Backend::point_type pos{10, 5};
        Backend::renderer_type::font font;
        Backend::color_type color;

        // Draw text "Hi" (2 chars) at (10, 5)
        ctx.draw_text("Hi", pos, font, color);

        // Should track to position + size = (10+2, 5+1) = (12, 6)
        auto measured = ctx.get_size();
        CHECK(size_utils::get_width(measured) == 12);
        CHECK(size_utils::get_height(measured) == 6);
    }
}

TEST_CASE("measure_context - Rectangle measurement") {
    SUBCASE("Single rectangle at origin") {
        measure_context<Backend> ctx;

        Backend::rect_type rect;
        rect_utils::set_bounds(rect, 0, 0, 10, 5);
        Backend::renderer_type::box_style style{};

        ctx.draw_rect(rect, style);

        // Should track rectangle bounds
        auto measured = ctx.get_size();
        CHECK(size_utils::get_width(measured) == 10);
        CHECK(size_utils::get_height(measured) == 5);
    }

    SUBCASE("Rectangle at offset position") {
        measure_context<Backend> ctx;

        Backend::rect_type rect;
        rect_utils::set_bounds(rect, 5, 3, 8, 4);  // (x, y, w, h)
        Backend::renderer_type::box_style style{};

        ctx.draw_rect(rect, style);

        // Should track to position + size = (5+8, 3+4) = (13, 7)
        auto measured = ctx.get_size();
        CHECK(size_utils::get_width(measured) == 13);
        CHECK(size_utils::get_height(measured) == 7);
    }
}

TEST_CASE("measure_context - Reset functionality") {
    SUBCASE("Reset clears tracked size") {
        measure_context<Backend> ctx;

        // Measure some text
        Backend::point_type pos{0, 0};
        Backend::renderer_type::font font;
        Backend::color_type color;
        ctx.draw_text("Hello", pos, font, color);

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
    SUBCASE("is_rendering returns true") {
        Backend::renderer_type renderer;
        draw_context<Backend> ctx(renderer);

        CHECK(ctx.is_measuring() == false);
        CHECK(ctx.is_rendering() == true);
    }

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

        Backend::point_type pos{10, 5};
        Backend::renderer_type::font font;
        Backend::color_type color;

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
        Backend::renderer_type::box_style style{};

        // Should not throw
        CHECK_NOTHROW(ctx.draw_rect(rect, style));
    }
}

TEST_CASE("Polymorphic usage") {
    SUBCASE("Can use render_context* to point to measure_context") {
        auto ctx = std::make_unique<measure_context<Backend>>();
        render_context<Backend>* base = ctx.get();

        CHECK(base->is_measuring() == true);
        CHECK(base->is_rendering() == false);
        CHECK(base->renderer() == nullptr);
    }

    SUBCASE("Can use render_context* to point to draw_context") {
        Backend::renderer_type renderer;
        auto ctx = std::make_unique<draw_context<Backend>>(renderer);
        render_context<Backend>* base = ctx.get();

        CHECK(base->is_measuring() == false);
        CHECK(base->is_rendering() == true);
        CHECK(base->renderer() != nullptr);
    }
}

TEST_CASE("Context lifetimes") {
    SUBCASE("measure_context is moveable") {
        measure_context<Backend> ctx1;

        Backend::point_type pos{0, 0};
        Backend::renderer_type::font font;
        Backend::color_type color;
        ctx1.draw_text("Hello", pos, font, color);

        // Move
        measure_context<Backend> ctx2 = std::move(ctx1);

        // Moved context retains state
        auto measured = ctx2.get_size();
        CHECK(size_utils::get_width(measured) == 5);
    }

    SUBCASE("draw_context is moveable") {
        Backend::renderer_type renderer;
        draw_context<Backend> ctx1(renderer);

        // Move
        draw_context<Backend> ctx2 = std::move(ctx1);

        // Moved context works
        CHECK(ctx2.is_rendering() == true);
    }
}
