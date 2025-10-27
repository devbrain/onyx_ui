//
// Created by claude on 16/10/2025.
//
// Tests for convenient factory functions

#include <doctest/doctest.h>

#include <memory>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/panel.hh>
#include <onyxui/widgets/hbox.hh>
#include <onyxui/widgets/vbox.hh>
#include <onyxui/widgets/spacer.hh>
#include <onyxui/widgets/spring.hh>
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"
#include "onyxui/concepts/size_like.hh"
#include "onyxui/concepts/rect_like.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Factory Functions - Spacers") {
    SUBCASE("create_hgap - Creates horizontal gap") {
        auto gap = create_hgap<test_canvas_backend>(20);

        REQUIRE(gap != nullptr);
        CHECK(gap->width() == 20);
        CHECK(gap->height() == 0);

        auto size = gap->measure(1000, 1000);
        CHECK(size_utils::get_width(size) == 20);
        CHECK(size_utils::get_height(size) == 0);
    }

    SUBCASE("create_vgap - Creates vertical gap") {
        auto gap = create_vgap<test_canvas_backend>(30);

        REQUIRE(gap != nullptr);
        CHECK(gap->width() == 0);
        CHECK(gap->height() == 30);

        auto size = gap->measure(1000, 1000);
        CHECK(size_utils::get_width(size) == 0);
        CHECK(size_utils::get_height(size) == 30);
    }

    SUBCASE("create_gap - Creates square gap") {
        auto gap = create_gap<test_canvas_backend>(50);

        REQUIRE(gap != nullptr);
        CHECK(gap->width() == 50);
        CHECK(gap->height() == 50);

        auto size = gap->measure(1000, 1000);
        CHECK(size_utils::get_width(size) == 50);
        CHECK(size_utils::get_height(size) == 50);
    }

    SUBCASE("create_hgap - Usage in hbox") {
        hbox<test_canvas_backend> toolbar;

        toolbar.add_child(std::make_unique<button<test_canvas_backend>>("New"));
        toolbar.add_child(create_hgap<test_canvas_backend>(20));
        toolbar.add_child(std::make_unique<button<test_canvas_backend>>("Open"));

        CHECK(toolbar.children().size() == 3);
        CHECK_NOTHROW((void)toolbar.measure(400, 50));
    }

    SUBCASE("create_vgap - Usage in vbox") {
        vbox<test_canvas_backend> menu;

        menu.add_child(std::make_unique<label<test_canvas_backend>>("Title"));
        menu.add_child(create_vgap<test_canvas_backend>(30));
        menu.add_child(std::make_unique<label<test_canvas_backend>>("Content"));

        CHECK(menu.children().size() == 3);
        CHECK_NOTHROW((void)menu.measure(200, 400));
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Factory Functions - Springs") {
    SUBCASE("create_hspring - Creates horizontal spring") {
        auto spr = create_hspring<test_canvas_backend>();

        REQUIRE(spr != nullptr);
        CHECK(spr->is_horizontal());
        CHECK(spr->weight() == 1.0F);
    }

    SUBCASE("create_hspring - With custom weight") {
        auto spr = create_hspring<test_canvas_backend>(2.5F);

        REQUIRE(spr != nullptr);
        CHECK(spr->is_horizontal());
        CHECK(spr->weight() == 2.5F);
    }

    SUBCASE("create_vspring - Creates vertical spring") {
        auto spr = create_vspring<test_canvas_backend>();

        REQUIRE(spr != nullptr);
        CHECK_FALSE(spr->is_horizontal());
        CHECK(spr->weight() == 1.0F);
    }

    SUBCASE("create_vspring - With custom weight") {
        auto spr = create_vspring<test_canvas_backend>(3.0F);

        REQUIRE(spr != nullptr);
        CHECK_FALSE(spr->is_horizontal());
        CHECK(spr->weight() == 3.0F);
    }

    SUBCASE("create_hspring - Push buttons to edges") {
        hbox<test_canvas_backend> toolbar;

        toolbar.add_child(std::make_unique<button<test_canvas_backend>>("File"));
        toolbar.add_child(create_hspring<test_canvas_backend>());
        toolbar.add_child(std::make_unique<button<test_canvas_backend>>("Help"));

        CHECK(toolbar.children().size() == 3);
        CHECK_NOTHROW((void)toolbar.measure(400, 50));
    }

    SUBCASE("create_hspring - Center content") {
        hbox<test_canvas_backend> container;

        container.add_child(create_hspring<test_canvas_backend>());
        container.add_child(std::make_unique<label<test_canvas_backend>>("Centered"));
        container.add_child(create_hspring<test_canvas_backend>());

        CHECK(container.children().size() == 3);
        CHECK_NOTHROW((void)container.measure(600, 100));
    }

    SUBCASE("create_vspring - Vertical spacing") {
        vbox<test_canvas_backend> layout;

        layout.add_child(std::make_unique<label<test_canvas_backend>>("Header"));
        layout.add_child(create_vspring<test_canvas_backend>());
        layout.add_child(std::make_unique<label<test_canvas_backend>>("Footer"));

        CHECK(layout.children().size() == 3);
        CHECK_NOTHROW((void)layout.measure(300, 600));
    }

    SUBCASE("Weighted springs - 2:1 horizontal ratio") {
        hbox<test_canvas_backend> box;

        box.add_child(create_hspring<test_canvas_backend>(2.0F));
        box.add_child(std::make_unique<panel<test_canvas_backend>>());
        box.add_child(create_hspring<test_canvas_backend>(1.0F));

        CHECK(box.children().size() == 3);
        CHECK_NOTHROW((void)box.measure(400, 100));
    }

    SUBCASE("Weighted springs - 3:1 vertical ratio") {
        vbox<test_canvas_backend> box;

        box.add_child(std::make_unique<panel<test_canvas_backend>>());
        box.add_child(create_vspring<test_canvas_backend>(3.0F));
        box.add_child(std::make_unique<panel<test_canvas_backend>>());
        box.add_child(create_vspring<test_canvas_backend>(1.0F));
        box.add_child(std::make_unique<panel<test_canvas_backend>>());

        CHECK(box.children().size() == 5);
        CHECK_NOTHROW((void)box.measure(300, 600));
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Factory Functions - Mixed Usage") {
    SUBCASE("Toolbar with gaps and springs") {
        hbox<test_canvas_backend> toolbar;

        // Left group
        toolbar.add_child(std::make_unique<button<test_canvas_backend>>("New"));
        toolbar.add_child(create_hgap<test_canvas_backend>(5));
        toolbar.add_child(std::make_unique<button<test_canvas_backend>>("Open"));

        // Expanding space
        toolbar.add_child(create_hspring<test_canvas_backend>());

        // Right group
        toolbar.add_child(std::make_unique<button<test_canvas_backend>>("Settings"));
        toolbar.add_child(create_hgap<test_canvas_backend>(5));
        toolbar.add_child(std::make_unique<button<test_canvas_backend>>("Help"));

        CHECK(toolbar.children().size() == 7);
        CHECK_NOTHROW((void)toolbar.measure(800, 50));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 800, 50);
        CHECK_NOTHROW(toolbar.arrange(bounds));
    }

    SUBCASE("Form with vertical gaps and springs") {
        vbox<test_canvas_backend> form;

        // Title
        form.add_child(std::make_unique<label<test_canvas_backend>>("Registration Form"));
        form.add_child(create_vgap<test_canvas_backend>(20));

        // Form fields
        form.add_child(std::make_unique<label<test_canvas_backend>>("Name:"));
        form.add_child(std::make_unique<panel<test_canvas_backend>>());  // Input
        form.add_child(create_vgap<test_canvas_backend>(10));

        form.add_child(std::make_unique<label<test_canvas_backend>>("Email:"));
        form.add_child(std::make_unique<panel<test_canvas_backend>>());  // Input

        // Flexible space
        form.add_child(create_vspring<test_canvas_backend>());

        // Buttons
        form.add_child(std::make_unique<button<test_canvas_backend>>("Submit"));

        CHECK(form.children().size() == 9);
        CHECK_NOTHROW((void)form.measure(400, 600));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 400, 600);
        CHECK_NOTHROW(form.arrange(bounds));
    }
}
