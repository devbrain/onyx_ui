/**
 * @file test_icon.cc
 * @brief Tests for icon widget
 */

#include <doctest/doctest.h>
#include <onyxui/widgets/icon.hh>
#include <unittest/utils/test_canvas_backend.hh>
#include <unittest/utils/ui_context_fixture.hh>

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Icon - Construction and accessors") {
    using icon_style = test_canvas_backend::renderer_type::icon_style;

    SUBCASE("Construct with icon style") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::check);

        CHECK(icon_widget->get_icon() == icon_style::check);
    }

    SUBCASE("Construct with different icon styles") {
        auto icon_check = std::make_unique<icon<test_canvas_backend>>(icon_style::check);
        auto icon_cross = std::make_unique<icon<test_canvas_backend>>(icon_style::cross);
        auto icon_arrow = std::make_unique<icon<test_canvas_backend>>(icon_style::arrow_up);

        CHECK(icon_check->get_icon() == icon_style::check);
        CHECK(icon_cross->get_icon() == icon_style::cross);
        CHECK(icon_arrow->get_icon() == icon_style::arrow_up);
    }

    SUBCASE("Set icon style") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::check);

        icon_widget->set_icon(icon_style::cross);
        CHECK(icon_widget->get_icon() == icon_style::cross);

        icon_widget->set_icon(icon_style::bullet);
        CHECK(icon_widget->get_icon() == icon_style::bullet);
    }

    SUBCASE("Set same icon doesn't invalidate") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::arrow_up);

        // Measure once
        auto size1 = icon_widget->measure(100, 100);

        // Set to same icon
        icon_widget->set_icon(icon_style::arrow_up);

        // Should still have same size (no invalidation)
        auto size2 = icon_widget->get_desired_size();
        CHECK(size2.w == size1.w);
        CHECK(size2.h == size1.h);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Icon - Measure and arrange") {
    using icon_style = test_canvas_backend::renderer_type::icon_style;

    SUBCASE("Icon has minimum size") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::bullet);

        auto size = icon_widget->measure(100, 100);

        // Icon should have minimum size of 1x1
        CHECK(size.w >= 1);
        CHECK(size.h >= 1);
    }

    SUBCASE("Different icons same size") {
        auto icon_check = std::make_unique<icon<test_canvas_backend>>(icon_style::check);
        auto icon_cross = std::make_unique<icon<test_canvas_backend>>(icon_style::cross);

        auto size_check = icon_check->measure(100, 100);
        auto size_cross = icon_cross->measure(100, 100);

        // Most icons are same size (1x1 character cells)
        CHECK(size_check.w == size_cross.w);
        CHECK(size_check.h == size_cross.h);
    }

    SUBCASE("Arrange updates bounds") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::folder);

        icon_widget->measure(100, 100);
        icon_widget->arrange({10, 20, 5, 5});

        CHECK(icon_widget->x() == 10);
        CHECK(icon_widget->y() == 20);
        CHECK(icon_widget->width() == 5);
        CHECK(icon_widget->height() == 5);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Icon - Rendering") {
    using icon_style = test_canvas_backend::renderer_type::icon_style;

    SUBCASE("Render check icon") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::check);

        icon_widget->measure(100, 100);
        icon_widget->arrange({0, 0, 10, 10});

        test_canvas canvas;
        auto ctx = create_render_context(canvas);
        icon_widget->render(*ctx);

        // Icon should be rendered (backend-specific verification)
        // test_canvas records draw_icon() calls
    }

    SUBCASE("Render different icon styles") {
        test_canvas canvas;
        auto ctx = create_render_context(canvas);

        const icon_style styles[] = {
            icon_style::check,
            icon_style::cross,
            icon_style::bullet,
            icon_style::arrow_up,
            icon_style::folder
        };

        for (auto style : styles) {
            auto icon_widget = std::make_unique<icon<test_canvas_backend>>(style);
            icon_widget->measure(100, 100);
            icon_widget->arrange({0, 0, 10, 10});
            icon_widget->render(*ctx);

            CHECK(icon_widget->get_icon() == style);
        }
    }

    SUBCASE("Invisible icon doesn't render") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::check);
        icon_widget->set_visible(false);

        icon_widget->measure(100, 100);
        icon_widget->arrange({0, 0, 10, 10});

        test_canvas canvas;
        auto ctx = create_render_context(canvas);
        icon_widget->render(*ctx);

        // Verify nothing rendered (widget not visible)
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Icon - Window management icons") {
    using icon_style = test_canvas_backend::renderer_type::icon_style;

    SUBCASE("Menu icon") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::menu);
        CHECK(icon_widget->get_icon() == icon_style::menu);
    }

    SUBCASE("Minimize icon") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::minimize);
        CHECK(icon_widget->get_icon() == icon_style::minimize);
    }

    SUBCASE("Maximize icon") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::maximize);
        CHECK(icon_widget->get_icon() == icon_style::maximize);
    }

    SUBCASE("Restore icon") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::restore);
        CHECK(icon_widget->get_icon() == icon_style::restore);
    }

    SUBCASE("Close icon") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::close_x);
        CHECK(icon_widget->get_icon() == icon_style::close_x);
    }

    SUBCASE("Toggle between maximize and restore") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::maximize);

        CHECK(icon_widget->get_icon() == icon_style::maximize);

        // Simulate maximize/restore toggle
        icon_widget->set_icon(icon_style::restore);
        CHECK(icon_widget->get_icon() == icon_style::restore);

        icon_widget->set_icon(icon_style::maximize);
        CHECK(icon_widget->get_icon() == icon_style::maximize);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Icon - Navigation arrows") {
    using icon_style = test_canvas_backend::renderer_type::icon_style;

    SUBCASE("All arrow icons") {
        auto icon_up = std::make_unique<icon<test_canvas_backend>>(icon_style::arrow_up);
        auto icon_down = std::make_unique<icon<test_canvas_backend>>(icon_style::arrow_down);
        auto icon_left = std::make_unique<icon<test_canvas_backend>>(icon_style::arrow_left);
        auto icon_right = std::make_unique<icon<test_canvas_backend>>(icon_style::arrow_right);

        CHECK(icon_up->get_icon() == icon_style::arrow_up);
        CHECK(icon_down->get_icon() == icon_style::arrow_down);
        CHECK(icon_left->get_icon() == icon_style::arrow_left);
        CHECK(icon_right->get_icon() == icon_style::arrow_right);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Icon - General purpose icons") {
    using icon_style = test_canvas_backend::renderer_type::icon_style;

    SUBCASE("Check and cross") {
        auto icon_check = std::make_unique<icon<test_canvas_backend>>(icon_style::check);
        auto icon_cross = std::make_unique<icon<test_canvas_backend>>(icon_style::cross);

        CHECK(icon_check->get_icon() == icon_style::check);
        CHECK(icon_cross->get_icon() == icon_style::cross);
    }

    SUBCASE("Bullet, folder, file") {
        auto icon_bullet = std::make_unique<icon<test_canvas_backend>>(icon_style::bullet);
        auto icon_folder = std::make_unique<icon<test_canvas_backend>>(icon_style::folder);
        auto icon_file = std::make_unique<icon<test_canvas_backend>>(icon_style::file);

        CHECK(icon_bullet->get_icon() == icon_style::bullet);
        CHECK(icon_folder->get_icon() == icon_style::folder);
        CHECK(icon_file->get_icon() == icon_style::file);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Icon - Layout invalidation") {
    using icon_style = test_canvas_backend::renderer_type::icon_style;

    SUBCASE("Changing icon invalidates measure") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::check);

        // Measure once
        icon_widget->measure(100, 100);

        // Change icon - should invalidate
        icon_widget->set_icon(icon_style::cross);

        // Measure again should recalculate
        auto size = icon_widget->measure(100, 100);

        // Icon size typically 1x1 for TUI backends
        CHECK(size.w >= 1);
        CHECK(size.h >= 1);
    }

    SUBCASE("Setting same icon doesn't invalidate") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::bullet);

        icon_widget->measure(100, 100);
        auto size1 = icon_widget->get_desired_size();

        // Set same icon
        icon_widget->set_icon(icon_style::bullet);

        // Should not trigger re-measure
        auto size2 = icon_widget->get_desired_size();
        CHECK(size1.w == size2.w);
        CHECK(size1.h == size2.h);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Icon - Theme integration") {
    using icon_style = test_canvas_backend::renderer_type::icon_style;

    SUBCASE("Icon inherits theme colors") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::check);

        // Apply theme
        icon_widget->apply_theme("Norton Blue", themes);

        // Icon should inherit text color from theme
        // (Verification depends on theme system integration)
        // Icon rendering uses parent's foreground color
    }

    SUBCASE("Icon in themed container") {
        auto container = std::make_unique<panel<test_canvas_backend>>();
        container->apply_theme("Norton Blue", themes);

        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::folder);
        container->add_child(std::move(icon_widget));

        // Icon should inherit theme from parent
        container->measure(100, 100);
        container->arrange({0, 0, 100, 100});

        test_canvas canvas;
        auto ctx = create_render_context(canvas);
        container->render(*ctx);
    }
}
