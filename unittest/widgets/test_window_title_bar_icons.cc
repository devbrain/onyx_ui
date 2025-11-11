/**
 * @file test_window_title_bar_icons.cc
 * @brief Unit tests for window title bar icon rendering
 */

#include <doctest/doctest.h>
#include <utils/test_helpers.hh>
#include <utils/visual_test_helpers.hh>
#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/window/window_title_bar.hh>
#include <onyxui/widgets/icon.hh>
#include <onyxui/widgets/containers/panel.hh>
#include <utils/test_canvas_backend.hh>
#include <onyxui/core/rendering/draw_context.hh>
#include <iostream>

using namespace onyxui;
using Backend = testing::test_canvas_backend;

TEST_CASE("window_title_bar - Icons are created and rendered") {
    // Initialize UI context for theme support
    ui_context_fixture<Backend> fixture;

    SUBCASE("Title bar with all icons creates children correctly") {
        // Create title bar with all icons enabled
        typename window<Backend>::window_flags flags;
        flags.has_menu_button = true;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_close_button = true;

        auto title_bar = std::make_unique<window_title_bar<Backend>>("Test Window", flags);

        // Children: 1 label + 1 spring + 4 icons = 6
        CHECK(title_bar->children().size() == 6);
    }

    SUBCASE("Icons have proper bounds after layout") {
        typename window<Backend>::window_flags flags;
        flags.has_menu_button = false;  // Disable menu for simpler test
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_close_button = true;

        auto title_bar = std::make_unique<window_title_bar<Backend>>("Test", flags);

        // Measure and arrange
        [[maybe_unused]] auto measured = title_bar->measure(80, 1);

        title_bar->arrange({0, 0, 80, 1});

        // Should have 5 children (1 label + 1 spring + 3 icons)
        auto& children = title_bar->children();
        REQUIRE(children.size() == 5);

        // Check bounds of each child
        for (size_t i = 0; i < children.size(); ++i) {
            auto bounds = children[i]->bounds();

            // All children should have non-zero width and height
            CHECK(bounds.w > 0);
            CHECK(bounds.h == 1);  // Title bar is 1 row tall

            // Icons should be positioned to the right of the title
            if (i > 0) {  // Icons are after the label
                CHECK(bounds.x > 0);  // Should not be at position 0
            }
        }

        // Icons should be positioned at the right side
        // Last icon (close button) should be near the right edge
        // Children: 0=label, 1=spring, 2=minimize, 3=maximize, 4=close
        auto close_icon_bounds = children[4]->bounds();
        CHECK(close_icon_bounds.x >= 77);  // Should be near right edge (80 - 3 icons)
    }

    // Note: Rendering test removed - the important tests are:
    // 1. Icons are created (children count)
    // 2. Icons are positioned at right edge (bounds check)

    SUBCASE("Icon widgets report correct size in do_measure") {
        using icon_type = typename Backend::renderer_type::icon_style;
        using icon_widget = icon<Backend>;

        auto icon_ptr = std::make_unique<icon_widget>(icon_type::close_x);

        // Icons should measure as 1x1 for text backends
        auto size = icon_ptr->measure(10, 10);
        CHECK(size.w == 1);
        CHECK(size.h == 1);
    }

    SUBCASE("Title label width is constrained to content") {
        typename window<Backend>::window_flags flags;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_close_button = true;

        auto title_bar = std::make_unique<window_title_bar<Backend>>("Short", flags);

        [[maybe_unused]] auto measured_size = title_bar->measure(80, 1);
        title_bar->arrange({0, 0, 80, 1});

        // First child should be the title label
        auto& children = title_bar->children();
        REQUIRE(children.size() >= 5);  // title + spring + icons

        auto title_bounds = children[0]->bounds();

        // Title label should only be as wide as its content
        // "Short" = 5 characters
        CHECK(title_bounds.w <= 10);  // Allow some padding but not full width

    }

    SUBCASE("Icons are positioned after title with proper spacing") {
        typename window<Backend>::window_flags flags;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_close_button = true;

        auto title_bar = std::make_unique<window_title_bar<Backend>>("MyApp", flags);

        [[maybe_unused]] auto measured_size = title_bar->measure(80, 1);
        title_bar->arrange({0, 0, 80, 1});

        auto& children = title_bar->children();
        REQUIRE(children.size() == 5);  // title + spring + 3 icons

        // Get positions (children: 0=title, 1=spring, 2=minimize, 3=maximize, 4=close)
        auto title_bounds = children[0]->bounds();
        auto min_bounds = children[2]->bounds();
        auto max_bounds = children[3]->bounds();
        auto close_bounds = children[4]->bounds();


        // Icons should be to the right of the title
        CHECK(min_bounds.x > (title_bounds.x + title_bounds.w));

        // Icons should be in order (minimize, maximize, close)
        CHECK(max_bounds.x > min_bounds.x);
        CHECK(close_bounds.x > max_bounds.x);

        // Icons should be near the right edge
        CHECK(close_bounds.x >= 77);  // 80 - 3 for the close button itself
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "window_title_bar - Icon click detection with absolute coordinates") {

    SUBCASE("Close icon click at absolute screen coordinates") {
        // Create a window at position (5, 3)
        typename window<Backend>::window_flags flags;
        flags.has_close_button = true;
        flags.has_minimize_button = false;
        flags.has_maximize_button = false;
        flags.has_menu_button = false;

        auto win = std::make_unique<window<Backend>>("Test Window", flags);

        // Measure and arrange window at screen position (5, 3) with size 40x15
        (void)win->measure(40, 15);
        typename Backend::rect_type win_bounds;
        rect_utils::set_bounds(win_bounds, 5, 3, 40, 15);
        win->arrange(win_bounds);


        // Get the title bar (first child of window)
        REQUIRE(!win->children().empty());
        auto* title_bar = dynamic_cast<window_title_bar<Backend>*>(win->children()[0].get());
        REQUIRE(title_bar != nullptr);

        // Title bar should be at (0, 0) relative to window, size (40, 1)
        auto tb_bounds = title_bar->bounds();
        CHECK(tb_bounds.x == 0);
        CHECK(tb_bounds.y == 0);
        CHECK(tb_bounds.w == 40);
        CHECK(tb_bounds.h == 1);

        // Close icon should be at right edge of title bar
        // With 40 width, close icon should be at x=39 (relative to title bar)
        REQUIRE(!title_bar->children().empty());
        auto* close_icon = title_bar->children().back().get();  // Last child
        REQUIRE(close_icon != nullptr);

        auto icon_bounds = close_icon->bounds();
        CHECK(icon_bounds.x == 39);
        CHECK(icon_bounds.y == 0);
        CHECK(icon_bounds.w == 1);
        CHECK(icon_bounds.h == 1);

        // Close icon absolute screen position should be:
        // window_x + title_bar_x + icon_x = 5 + 0 + 39 = 44
        // window_y + title_bar_y + icon_y = 3 + 0 + 0 = 3
        // So clicking at absolute coordinates (44, 3) should trigger close icon

        // Create a mouse click event at absolute coordinates (44, 3)
        mouse_event click;
        click.x = 44;
        click.y = 3;
        click.btn = mouse_event::button::none;  // termbox2 sets btn=none on release
        click.act = mouse_event::action::release;
        ui_event evt = click;

        // Track if close_clicked signal was emitted
        bool close_clicked = false;
        title_bar->close_clicked.connect([&]() {
            close_clicked = true;
        });

        // Hit test from window root should find the close icon
        auto* target = win->hit_test(44, 3);
        CHECK(target != nullptr);

        // Route the event through the three-phase system
        // This simulates what layer_manager does
        hit_test_path<Backend> path;
        auto* hit_target = win->hit_test(44, 3, path);
        REQUIRE(hit_target != nullptr);

        // Route event through three phases
        [[maybe_unused]] bool handled = route_event(evt, path);

        // This should work: close icon click should be detected
        CHECK(close_clicked == true);
    }

    SUBCASE("Minimize icon click at absolute screen coordinates") {
        typename window<Backend>::window_flags flags;
        flags.has_close_button = false;
        flags.has_minimize_button = true;
        flags.has_maximize_button = false;
        flags.has_menu_button = false;

        auto win = std::make_unique<window<Backend>>("Test", flags);
        [[maybe_unused]] auto measured = win->measure(30, 10);

        typename Backend::rect_type win_bounds;
        rect_utils::set_bounds(win_bounds, 10, 5, 30, 10);
        win->arrange(win_bounds);

        REQUIRE(!win->children().empty());
        auto* title_bar = dynamic_cast<window_title_bar<Backend>*>(win->children()[0].get());
        REQUIRE(title_bar != nullptr);

        bool minimize_clicked = false;
        title_bar->minimize_clicked.connect([&]() {
            minimize_clicked = true;
        });

        // Minimize icon should be at x=29 relative to title bar
        // Absolute position: 10 + 0 + 29 = 39, y = 5
        mouse_event click;
        click.x = 39;
        click.y = 5;
        click.btn = mouse_event::button::none;
        click.act = mouse_event::action::release;
        ui_event evt = click;

        // Route through three-phase system
        hit_test_path<Backend> path;
        auto* target = win->hit_test(39, 5, path);
        REQUIRE(target != nullptr);

        [[maybe_unused]] bool handled = route_event(evt, path);

        CHECK(minimize_clicked == true);
    }

    SUBCASE("Maximize icon click at absolute screen coordinates") {
        typename window<Backend>::window_flags flags;
        flags.has_close_button = false;
        flags.has_minimize_button = false;
        flags.has_maximize_button = true;
        flags.has_menu_button = false;

        auto win = std::make_unique<window<Backend>>("Test", flags);
        [[maybe_unused]] auto measured = win->measure(30, 10);

        typename Backend::rect_type win_bounds;
        rect_utils::set_bounds(win_bounds, 10, 5, 30, 10);
        win->arrange(win_bounds);

        REQUIRE(!win->children().empty());
        auto* title_bar = dynamic_cast<window_title_bar<Backend>*>(win->children()[0].get());
        REQUIRE(title_bar != nullptr);

        bool maximize_clicked = false;
        title_bar->maximize_clicked.connect([&]() {
            maximize_clicked = true;
        });

        // Maximize icon should be at x=29 relative to title bar
        // Absolute position: 10 + 0 + 29 = 39, y = 5
        mouse_event click;
        click.x = 39;
        click.y = 5;
        click.btn = mouse_event::button::none;
        click.act = mouse_event::action::release;
        ui_event evt = click;

        // Route through three-phase system
        hit_test_path<Backend> path;
        auto* target = win->hit_test(39, 5, path);
        REQUIRE(target != nullptr);

        [[maybe_unused]] bool handled = route_event(evt, path);

        CHECK(maximize_clicked == true);
    }

    SUBCASE("Maximize button actually maximizes window") {
        typename window<Backend>::window_flags flags;
        flags.has_menu_button = false;
        flags.has_minimize_button = false;
        flags.has_maximize_button = true;
        flags.has_close_button = false;

        auto win = std::make_unique<window<Backend>>("Test", flags);

        // Create a parent container to maximize within
        auto parent = std::make_unique<panel<Backend>>();
        typename Backend::rect_type parent_bounds;
        rect_utils::set_bounds(parent_bounds, 0, 0, 80, 25);
        parent->arrange(parent_bounds);

        // Add window to parent
        parent->add_child(std::move(win));
        auto* win_ptr = dynamic_cast<window<Backend>*>(parent->children()[0].get());
        REQUIRE(win_ptr != nullptr);

        // Position window at (10, 5) with size 30x10
        typename Backend::rect_type win_bounds;
        rect_utils::set_bounds(win_bounds, 10, 5, 30, 10);
        [[maybe_unused]] auto measured = win_ptr->measure(30, 10);
        win_ptr->arrange(win_bounds);


        // Get title bar
        REQUIRE(!win_ptr->children().empty());
        auto* title_bar = dynamic_cast<window_title_bar<Backend>*>(win_ptr->children()[0].get());
        REQUIRE(title_bar != nullptr);

        // Connect to maximize signal to verify it fires
        bool maximize_signal_fired = false;
        title_bar->maximize_clicked.connect([&]() {
            maximize_signal_fired = true;
        });

        // Find maximize icon - need to get its actual position
        // Look through title bar children to find maximize icon
        ui_element<Backend>* maximize_icon = nullptr;
        for (auto& child : title_bar->children()) {
            auto* icon_ptr = dynamic_cast<icon<Backend>*>(child.get());
            if (icon_ptr) {
                maximize_icon = icon_ptr;
            }
        }
        REQUIRE(maximize_icon != nullptr);

        // Get absolute bounds of maximize icon
        auto abs_icon_bounds = maximize_icon->get_absolute_bounds();

        // Click on the icon
        mouse_event click;
        click.x = abs_icon_bounds.x;
        click.y = abs_icon_bounds.y;
        click.btn = mouse_event::button::none;
        click.act = mouse_event::action::release;
        ui_event evt = click;

        // Route through event system
        hit_test_path<Backend> path;
        auto* target = parent->hit_test(click.x, click.y, path);

        if (target) {
            route_event(evt, path);
        }

        // Check if signal was fired

        // Check if window was maximized
        auto final_bounds = win_ptr->bounds();

        // First verify signal fired
        CHECK(maximize_signal_fired == true);

        // Window should now fill parent (0, 0, 80, 25)
        CHECK(final_bounds.x == 0);
        CHECK(final_bounds.y == 0);
        CHECK(final_bounds.w == 80);
        CHECK(final_bounds.h == 25);
    }

    SUBCASE("All buttons work correctly (minimize, maximize, close all enabled)") {
        // Test with all buttons like in real demo
        typename window<Backend>::window_flags flags;
        // Use defaults: all buttons enabled

        auto win = std::make_unique<window<Backend>>("Test", flags);

        // Create parent
        auto parent = std::make_unique<panel<Backend>>();
        typename Backend::rect_type parent_bounds;
        rect_utils::set_bounds(parent_bounds, 0, 0, 80, 25);
        parent->arrange(parent_bounds);

        // Add window
        parent->add_child(std::move(win));
        auto* win_ptr = dynamic_cast<window<Backend>*>(parent->children()[0].get());
        REQUIRE(win_ptr != nullptr);

        // Position window
        typename Backend::rect_type win_bounds;
        rect_utils::set_bounds(win_bounds, 10, 5, 40, 15);
        [[maybe_unused]] auto measured = win_ptr->measure(40, 15);
        win_ptr->arrange(win_bounds);

        // Find all icons
        auto* title_bar = dynamic_cast<window_title_bar<Backend>*>(win_ptr->children()[0].get());
        REQUIRE(title_bar != nullptr);

        std::vector<ui_element<Backend>*> icons;
        for (auto& child : title_bar->children()) {
            auto* icon_ptr = dynamic_cast<icon<Backend>*>(child.get());
            if (icon_ptr) {
                icons.push_back(icon_ptr);
            }
        }

        REQUIRE(icons.size() == 3);  // minimize, maximize, close

        // Test maximize button (middle icon with default ordering)
        bool maximize_fired = false;
        title_bar->maximize_clicked.connect([&]() { maximize_fired = true; });

        auto* maximize_icon = icons[1];  // Middle icon should be maximize
        auto max_bounds = maximize_icon->get_absolute_bounds();

        mouse_event click;
        click.x = max_bounds.x;
        click.y = max_bounds.y;
        click.btn = mouse_event::button::none;
        click.act = mouse_event::action::release;

        hit_test_path<Backend> path;
        auto* target = parent->hit_test(click.x, click.y, path);
        REQUIRE(target != nullptr);
        route_event(ui_event{click}, path);

        CHECK(maximize_fired == true);

        // Verify window maximized
        auto final_bounds = win_ptr->bounds();
        CHECK(final_bounds.w == 80);
        CHECK(final_bounds.h == 25);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "window_title_bar - Visual test of maximize functionality") {
    SUBCASE("Maximize button visually changes window size") {
        // Create visual harness - canvas represents the screen/layer
        testing::visual_test_harness<Backend> harness(80, 25);

        // Create parent panel that fills the canvas (for window to have a parent to maximize within)
        auto parent = std::make_unique<panel<Backend>>();

        // Create window with maximize button as child of panel
        typename window<Backend>::window_flags flags;
        flags.has_menu_button = false;
        flags.has_minimize_button = false;
        flags.has_maximize_button = true;
        flags.has_close_button = false;

        auto* win = parent->template emplace_child<window>("Visual Test", flags);

        // Render using harness (this will measure, arrange, and render the whole tree)
        // Panel's default layout will position children at (0,0) and size them to fill
        harness.render(parent.get());

        // Window starts at (0, 0) filling width but only 3 rows tall
        // Row 0: Title bar
        // Row 1: Content top border
        // Row 2: Content bottom border
        harness.expect_char_at(0, 0, 'V');   // First char of "Visual Test" title
        harness.expect_char_at(79, 0, 'O');  // Maximize icon at right edge
        harness.expect_char_at(0, 1, '+');   // Top-left corner of content border
        harness.expect_char_at(79, 1, '+');  // Top-right corner
        harness.expect_char_at(0, 2, '+');   // Bottom-left corner
        harness.expect_char_at(79, 2, '+');  // Bottom-right corner

        // Verify area below window is empty
        harness.expect_char_at(0, 3, ' ');   // Below window
        harness.expect_char_at(79, 24, ' '); // Bottom-right of canvas

        // Now click maximize button
        auto* title_bar = dynamic_cast<window_title_bar<Backend>*>(win->children()[0].get());
        REQUIRE(title_bar != nullptr);

        // Find maximize icon
        ui_element<Backend>* maximize_icon = nullptr;
        for (auto& child : title_bar->children()) {
            auto* icon_ptr = dynamic_cast<icon<Backend>*>(child.get());
            if (icon_ptr) {
                maximize_icon = icon_ptr;
                break;
            }
        }
        REQUIRE(maximize_icon != nullptr);

        // Get absolute bounds of maximize icon
        auto abs_icon_bounds = maximize_icon->get_absolute_bounds();

        // Click maximize button
        mouse_event click;
        click.x = abs_icon_bounds.x;
        click.y = abs_icon_bounds.y;
        click.btn = mouse_event::button::none;
        click.act = mouse_event::action::release;

        hit_test_path<Backend> path;
        auto* target = win->hit_test(click.x, click.y, path);
        REQUIRE(target != nullptr);
        route_event(ui_event{click}, path);

        // Re-render after maximize click (manual render to avoid re-arranging)
        {
            typename Backend::renderer_type renderer(harness.canvas());
            auto const* themes = ui_services<Backend>::themes();
            auto const* theme = themes ? themes->get_current_theme() : nullptr;
            if (theme) {
                parent->render(renderer, theme);
            }
        }


        // Window has parent, so maximize should fill parent (80x25)
        // Window should now be maximized to fill the entire canvas
        auto final_bounds = win->bounds();
        CHECK(final_bounds.x == 0);
        CHECK(final_bounds.y == 0);
        CHECK(final_bounds.w == 80);
        CHECK(final_bounds.h == 25);

        // Verify window fills the canvas
        harness.expect_char_at(0, 0, 'V');    // First char of "Visual Test" title at top-left
        harness.expect_char_at(79, 0, 'o');   // Icon at top-right (restore icon after maximize)
        harness.expect_char_at(0, 1, '+');    // Content top border starts at row 1
        harness.expect_char_at(79, 1, '+');   // Content top border right edge
        harness.expect_char_at(0, 24, '+');   // Content bottom-left corner
        harness.expect_char_at(79, 24, '+');  // Content bottom-right corner
    }
}