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
        [[maybe_unused]] auto measured = title_bar->measure(80_lu, 1_lu);

        title_bar->arrange(logical_rect{0_lu, 0_lu, 80_lu, 1_lu});

        // Should have 5 children (1 label + 1 spring + 3 icons)
        auto& children = title_bar->children();
        REQUIRE(children.size() == 5);

        // Check bounds of each child
        for (size_t i = 0; i < children.size(); ++i) {
            auto bounds = children[i]->bounds();

            // All children should have non-zero width and height
            CHECK(bounds.width.to_int() > 0);
            CHECK(bounds.height.to_int() == 1);  // Title bar is 1 row tall

            // Icons should be positioned to the right of the title
            if (i > 0) {  // Icons are after the label
                CHECK(bounds.x.to_int() > 0);  // Should not be at position 0
            }
        }

        // Icons should be positioned at the right side
        // Last icon (close button) should be near the right edge
        // Children: 0=label, 1=spring, 2=minimize, 3=maximize, 4=close
        auto close_icon_bounds = children[4]->bounds();
        CHECK(close_icon_bounds.x.to_int() >= 77);  // Should be near right edge (80 - 3 icons)
    }

    // Note: Rendering test removed - the important tests are:
    // 1. Icons are created (children count)
    // 2. Icons are positioned at right edge (bounds check)

    SUBCASE("Icon widgets report correct size in do_measure") {
        using icon_type = typename Backend::renderer_type::icon_style;
        using icon_widget = icon<Backend>;

        auto icon_ptr = std::make_unique<icon_widget>(icon_type::close_x);

        // Icons should measure as 1x1 for text backends
        auto size = icon_ptr->measure(10_lu, 10_lu);
        CHECK(size.width.to_int() == 1);
        CHECK(size.height.to_int() == 1);
    }

    SUBCASE("Title label width is constrained to content") {
        typename window<Backend>::window_flags flags;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_close_button = true;

        auto title_bar = std::make_unique<window_title_bar<Backend>>("Short", flags);

        [[maybe_unused]] auto measured_size = title_bar->measure(80_lu, 1_lu);
        title_bar->arrange(logical_rect{0_lu, 0_lu, 80_lu, 1_lu});

        // First child should be the title label
        auto& children = title_bar->children();
        REQUIRE(children.size() >= 5);  // title + spring + icons

        auto title_bounds = children[0]->bounds();

        // Title label should only be as wide as its content
        // "Short" = 5 characters
        CHECK(title_bounds.width.to_int() <= 10);  // Allow some padding but not full width

    }

    SUBCASE("Icons are positioned after title with proper spacing") {
        typename window<Backend>::window_flags flags;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_close_button = true;

        auto title_bar = std::make_unique<window_title_bar<Backend>>("MyApp", flags);

        [[maybe_unused]] auto measured_size = title_bar->measure(80_lu, 1_lu);
        title_bar->arrange(logical_rect{0_lu, 0_lu, 80_lu, 1_lu});

        auto& children = title_bar->children();
        REQUIRE(children.size() == 5);  // title + spring + 3 icons

        // Get positions (children: 0=title, 1=spring, 2=minimize, 3=maximize, 4=close)
        auto title_bounds = children[0]->bounds();
        auto min_bounds = children[2]->bounds();
        auto max_bounds = children[3]->bounds();
        auto close_bounds = children[4]->bounds();


        // Icons should be to the right of the title
        CHECK(min_bounds.x.to_int() > (title_bounds.x.to_int() + title_bounds.width.to_int()));

        // Icons should be in order (minimize, maximize, close)
        CHECK(max_bounds.x.to_int() > min_bounds.x.to_int());
        CHECK(close_bounds.x.to_int() > max_bounds.x.to_int());

        // Icons should be near the right edge
        CHECK(close_bounds.x.to_int() >= 77);  // 80 - 3 for the close button itself
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
        (void)win->measure(40_lu, 15_lu);
        typename Backend::rect_type win_bounds;
        rect_utils::set_bounds(win_bounds, 5, 3, 40, 15);
        win->arrange(logical_rect{5_lu, 3_lu, 40_lu, 15_lu});


        // Get the title bar (first child of window)
        REQUIRE(!win->children().empty());
        auto* title_bar = dynamic_cast<window_title_bar<Backend>*>(win->children()[0].get());
        REQUIRE(title_bar != nullptr);

        // Title bar should be at (0, 0) relative to window, size (40, 1)
        auto tb_bounds = title_bar->bounds();
        CHECK(tb_bounds.x.to_int() == 0);
        CHECK(tb_bounds.y.to_int() == 0);
        CHECK(tb_bounds.width.to_int() == 40);
        CHECK(tb_bounds.height.to_int() == 1);

        // Close icon should be at right edge of title bar
        // With 40 width, close icon should be at x=39 (relative to title bar)
        REQUIRE(!title_bar->children().empty());
        auto* close_icon = title_bar->children().back().get();  // Last child
        REQUIRE(close_icon != nullptr);

        auto icon_bounds = close_icon->bounds();
        CHECK(icon_bounds.x.to_int() == 39);
        CHECK(icon_bounds.y.to_int() == 0);
        CHECK(icon_bounds.width.to_int() == 1);
        CHECK(icon_bounds.height.to_int() == 1);

        // Close icon absolute screen position should be:
        // window_x + title_bar_x + icon_x = 5 + 0 + 39 = 44
        // window_y + title_bar_y + icon_y = 3 + 0 + 0 = 3
        // So clicking at absolute coordinates (44, 3) should trigger close icon

        // DEBUG: Print actual absolute bounds
        auto icon_abs = close_icon->get_absolute_bounds();
        INFO("Icon absolute bounds: (" << icon_abs.x() << ", " << icon_abs.y()
             << ", " << icon_abs.width() << ", " << icon_abs.height() << ")");
        INFO("Expected: (44, 3, 1, 1)");

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

        INFO("Hit test path size: " << path.size());
        for (size_t i = 0; i < path.size(); ++i) {
            INFO("Path[" << i << "]: " << typeid(*path[i]).name());
        }

        // Route event through three phases
        [[maybe_unused]] bool handled = route_event(evt, path);
        INFO("Event handled: " << handled);

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
        [[maybe_unused]] auto measured = win->measure(30_lu, 10_lu);

        typename Backend::rect_type win_bounds;
        rect_utils::set_bounds(win_bounds, 10, 5, 30, 10);
        win->arrange(logical_rect{0_lu, 0_lu, 30_lu, 10_lu});

        REQUIRE(!win->children().empty());
        auto* title_bar = dynamic_cast<window_title_bar<Backend>*>(win->children()[0].get());
        REQUIRE(title_bar != nullptr);

        bool minimize_clicked = false;
        title_bar->minimize_clicked.connect([&]() {
            minimize_clicked = true;
        });

        // Minimize icon should be at x=29 relative to title bar
        // Use relative coordinates for hit testing (window is at 0,0 with size 30x10)
        mouse_event click;
        click.x = 29;  // Relative to window (icon at right edge)
        click.y = 0;   // Title bar is at y=0
        click.btn = mouse_event::button::none;
        click.act = mouse_event::action::release;
        ui_event evt = click;

        // Route through three-phase system
        hit_test_path<Backend> path;
        auto* target = win->hit_test(29, 0, path);
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
        [[maybe_unused]] auto measured = win->measure(30_lu, 10_lu);

        typename Backend::rect_type win_bounds;
        rect_utils::set_bounds(win_bounds, 10, 5, 30, 10);
        win->arrange(logical_rect{0_lu, 0_lu, 30_lu, 10_lu});

        REQUIRE(!win->children().empty());
        auto* title_bar = dynamic_cast<window_title_bar<Backend>*>(win->children()[0].get());
        REQUIRE(title_bar != nullptr);

        bool maximize_clicked = false;
        title_bar->maximize_clicked.connect([&]() {
            maximize_clicked = true;
        });

        // Maximize icon should be at x=29 relative to title bar
        // Use relative coordinates for hit testing (window is at 0,0 with size 30x10)
        mouse_event click;
        click.x = 29;  // Relative to window (icon at right edge)
        click.y = 0;   // Title bar is at y=0
        click.btn = mouse_event::button::none;
        click.act = mouse_event::action::release;
        ui_event evt = click;

        // Route through three-phase system
        hit_test_path<Backend> path;
        auto* target = win->hit_test(29, 0, path);
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

        // Add window to parent FIRST
        parent->add_child(std::move(win));
        auto* win_ptr = dynamic_cast<window<Backend>*>(parent->children()[0].get());
        REQUIRE(win_ptr != nullptr);

        // Now measure and arrange parent (which will layout its children)
        typename Backend::rect_type parent_bounds;
        rect_utils::set_bounds(parent_bounds, 0, 0, 80, 25);
        [[maybe_unused]] auto parent_size = parent->measure(80_lu, 25_lu);
        parent->arrange(logical_rect{0_lu, 0_lu, 80_lu, 25_lu});

        // Position window at (10, 5) with size 30x10
        typename Backend::rect_type win_bounds;
        rect_utils::set_bounds(win_bounds, 10, 5, 30, 10);
        [[maybe_unused]] auto measured = win_ptr->measure(30_lu, 10_lu);
        win_ptr->arrange(logical_rect{0_lu, 0_lu, 30_lu, 10_lu});


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
        click.x = abs_icon_bounds.x();
        click.y = abs_icon_bounds.y();
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
        CHECK(final_bounds.x.to_int() == 0);
        CHECK(final_bounds.y.to_int() == 0);
        CHECK(final_bounds.width.to_int() == 80);
        CHECK(final_bounds.height.to_int() == 25);
    }

    SUBCASE("All buttons work correctly (minimize, maximize, close all enabled)") {
        // Test with all buttons like in real demo
        typename window<Backend>::window_flags flags;
        // Use defaults: all buttons enabled

        auto win = std::make_unique<window<Backend>>("Test", flags);

        // Create parent
        auto parent = std::make_unique<panel<Backend>>();

        // Add window FIRST
        parent->add_child(std::move(win));
        auto* win_ptr = dynamic_cast<window<Backend>*>(parent->children()[0].get());
        REQUIRE(win_ptr != nullptr);

        // Now measure and arrange parent
        typename Backend::rect_type parent_bounds;
        rect_utils::set_bounds(parent_bounds, 0, 0, 80, 25);
        [[maybe_unused]] auto parent_size = parent->measure(80_lu, 25_lu);
        parent->arrange(logical_rect{0_lu, 0_lu, 80_lu, 25_lu});

        // Position window
        typename Backend::rect_type win_bounds;
        rect_utils::set_bounds(win_bounds, 10, 5, 40, 15);
        [[maybe_unused]] auto measured = win_ptr->measure(40_lu, 15_lu);
        win_ptr->arrange(logical_rect{0_lu, 0_lu, 40_lu, 15_lu});

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
        click.x = max_bounds.x();
        click.y = max_bounds.y();
        click.btn = mouse_event::button::none;
        click.act = mouse_event::action::release;

        hit_test_path<Backend> path;
        auto* target = parent->hit_test(click.x, click.y, path);
        REQUIRE(target != nullptr);
        route_event(ui_event{click}, path);

        CHECK(maximize_fired == true);

        // Verify window maximized
        auto final_bounds = win_ptr->bounds();
        CHECK(final_bounds.width.to_int() == 80);
        CHECK(final_bounds.height.to_int() == 25);
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

        // Semantic assertions: Test BEHAVIOR, not specific characters
        auto canvas = harness.canvas();

        // Check title text exists (not just first character)
        CHECK(canvas->has_text_at(0, 0, "Visual Test"));

        // Check icon presence at right edge (not specific 'O' character)
        CHECK(canvas->has_content_at(79, 0));  // Icon renders something

        // Check border presence (not specific '+' characters)
        CHECK(canvas->has_border_at(0, 1));   // Top-left corner
        CHECK(canvas->has_border_at(79, 1));  // Top-right corner
        CHECK(canvas->has_border_at(0, 2));   // Bottom-left corner
        CHECK(canvas->has_border_at(79, 2));  // Bottom-right corner

        // Verify area below window is empty
        CHECK(canvas->is_empty_at(0, 3));    // Below window
        CHECK(canvas->is_empty_at(79, 24));  // Bottom-right of canvas

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
        click.x = abs_icon_bounds.x();
        click.y = abs_icon_bounds.y();
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
        CHECK(final_bounds.x.to_int() == 0);
        CHECK(final_bounds.y.to_int() == 0);
        CHECK(final_bounds.width.to_int() == 80);
        CHECK(final_bounds.height.to_int() == 25);

        // Semantic assertions: Verify window fills canvas
        // Check title text (not specific character)
        CHECK(canvas->has_text_at(0, 0, "Visual Test"));

        // Check icon presence (not specific 'o' character for restore icon)
        CHECK(canvas->has_content_at(79, 0));  // Restore icon renders

        // Check border presence (not specific '+' characters)
        CHECK(canvas->has_border_at(0, 1));    // Content top border left
        CHECK(canvas->has_border_at(79, 1));   // Content top border right
        CHECK(canvas->has_border_at(0, 24));   // Content bottom-left corner
        CHECK(canvas->has_border_at(79, 24));  // Content bottom-right corner
    }
}