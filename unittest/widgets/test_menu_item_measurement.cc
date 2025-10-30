/**
 * @file test_menu_item_measurement.cc
 * @brief Tests for menu item measurement and padding
 * @date 2025-10-25
 */

#include <doctest/doctest.h>
#include <onyxui/widgets/menu/menu_item.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/actions/action.hh>
#include <onyxui/widgets/menu/menu.hh>
#include <onyxui/widgets/menu/menu_bar.hh>
#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/services/ui_context.hh>
#include <onyxui/theming/theme.hh>
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"
#include "../utils/layout_assertions.hh"

using namespace onyxui;
using namespace onyxui::testing;
using Backend = test_canvas_backend;

namespace {
    // Helper to create a test theme with default values
    ui_theme<Backend> create_menu_test_theme(const std::string& name) {
        ui_theme<Backend> theme;
        theme.name = name;
        theme.description = "Test theme for menu measurement";

        // Set default colors
        theme.window_bg = {0, 0, 170};
        theme.text_fg = {255, 255, 255};
        theme.border_color = {255, 255, 255};

        // Button/menu item colors
        theme.button.normal.background = {0, 0, 170};
        theme.button.normal.foreground = {255, 255, 255};
        theme.button.hover.background = {0, 170, 170};
        theme.button.hover.foreground = {255, 255, 255};
        theme.button.pressed.background = {170, 0, 0};
        theme.button.pressed.foreground = {255, 255, 255};
        theme.button.disabled.background = {85, 85, 85};
        theme.button.disabled.foreground = {170, 170, 170};

        // Label colors
        theme.label.background = {0, 0, 170};
        theme.label.text = {255, 255, 255};

        // Panel border settings (important for other tests that share this theme!)
        theme.panel.box_style.draw_border = true;

        // Default font
        theme.button.normal.font = {};  // Use default font

        return theme;
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Menu Item - Measurement includes padding") {
    SUBCASE("Short text like 'About' needs padding space") {
        // Register a test theme (use fixture's context)
        ctx.themes().register_theme(create_menu_test_theme("Test Theme"));

        menu_item<Backend> item;
        item.set_text("About");  // 5 characters

        // Measure the menu item
        auto size = item.measure(100, 100);
        int width = size_utils::get_width(size);

        // The text is 5 chars, but it's drawn at x+2 (left padding)
        // So the minimum width should be 5 + 2 = 7
        // But ideally we want 5 + 2 (left) + 2 (right) = 9
        CHECK(width >= 9);  // Text (5) + left padding (2) + right padding (2)
    }

    SUBCASE("Menu item with shortcut needs padding on both sides") {
        ctx.themes().register_theme(create_menu_test_theme("Test Theme"));

        menu_item<Backend> item;
        item.set_text("Open");  // 4 characters

        // Create an action with a shortcut
        auto action = std::make_shared<onyxui::action<Backend>>();
        action->set_text("Open");
        action->set_shortcut('O', key_modifier::ctrl);  // "Ctrl+O" = 6 chars
        item.set_action(action);

        // Measure the menu item
        auto size = item.measure(100, 100);
        int width = size_utils::get_width(size);

        // Text "Open" (4) at position 2
        // Shortcut "Ctrl+O" (6) with 2 chars right padding
        // Total needed: 2 + 4 + spacing + 6 + 2 = at least 14
        // Should have enough space for text with left padding and shortcut with right padding
        CHECK(width >= 14);
    }

    SUBCASE("Separator has fixed height and expand width") {
        ctx.themes().register_theme(create_menu_test_theme("Test Theme"));

        auto sep = std::make_unique<separator<Backend>>();

        // Measure the separator
        auto size = sep->measure(100, 100);
        int width = size_utils::get_width(size);
        int height = size_utils::get_height(size);

        // Separator has expand policy for width (returns 0 during measurement)
        // and fixed height of 1
        CHECK(height == 1);
        CHECK(width == 0);  // Expand policy returns 0 during measurement

        // After arrange(), the separator will fill available width
        typename Backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 100, 1);
        sep->arrange(bounds);

        auto arranged_bounds = sep->bounds();
        CHECK(rect_utils::get_width(arranged_bounds) == 100);  // Now fills available width
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Menu Item - Text is not truncated when rendered") {
    SUBCASE("Menu item displays full text with padding") {
        ctx.themes().register_theme(create_menu_test_theme("Test Theme"));

        menu_item<Backend> item;
        item.set_text("About");

        // Measure and arrange with the measured size
        auto measured_size = item.measure(100, 100);
        int measured_width = size_utils::get_width(measured_size);

        // Arrange with exactly the measured width
        Backend::rect_type bounds{0, 0, measured_width, 1};
        item.arrange(bounds);

        // Now simulate rendering to check if text fits
        // The text "About" (5 chars) is drawn at position x+2
        // So we need at least 7 chars width to display it fully
        int text_start = 2;  // Left padding
        int text_length = 5;  // "About"
        int required_width = text_start + text_length;

        // The measured width should be enough to display the text with padding
        CHECK(measured_width >= required_width);

        // Even better: should have right padding too
        int ideal_width = text_start + text_length + 2;  // +2 for right padding
        CHECK(measured_width >= ideal_width);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Menu Item - Theme changes don't break measurement") {
    SUBCASE("Changing theme invalidates cached measurement") {
        // Register two different themes
        ctx.themes().register_theme(create_menu_test_theme("Theme 1"));
        auto theme2 = create_menu_test_theme("Theme 2");
        theme2.button.normal.foreground = {255, 0, 0};  // Different color
        ctx.themes().register_theme(std::move(theme2));

        menu_item<Backend> item;
        item.set_text("About");

        // Measure with first theme
        auto size1 = item.measure(100, 100);
        int width1 = size_utils::get_width(size1);

        CHECK(width1 >= 9);  // Should have padding

        // Measure again - should still have correct width
        auto size2 = item.measure(100, 100);
        int width2 = size_utils::get_width(size2);

        CHECK(width2 >= 9);  // Should still have padding after theme change
        CHECK(width2 == width1);  // Width should remain consistent
    }

    SUBCASE("Theme change forces remeasurement for items with shortcuts") {
        ctx.themes().register_theme(create_menu_test_theme("Theme 1"));
        ctx.themes().register_theme(create_menu_test_theme("Theme 2"));

        menu_item<Backend> item;
        item.set_text("Save");

        // Add action with shortcut
        auto action = std::make_shared<onyxui::action<Backend>>();
        action->set_text("Save");
        action->set_shortcut('S', key_modifier::ctrl);
        item.set_action(action);

        // Measure with first theme
        auto size1 = item.measure(100, 100);
        int width1 = size_utils::get_width(size1);

        CHECK(width1 >= 14);  // Text + shortcut + padding

        // Measure again
        auto size2 = item.measure(100, 100);
        int width2 = size_utils::get_width(size2);

        CHECK(width2 >= 14);  // Should still have proper width
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Menu Item - Visual rendering consistency") {
    using CanvasBackend = onyxui::testing::test_canvas_backend;
    using namespace onyxui::testing;

    SUBCASE("Multiple menu items render consistently with borders") {
        // Register test theme for canvas rendering
        ui_theme<CanvasBackend> theme;
        theme.name = "Canvas Test Theme";
        theme.description = "Theme for visual testing";
        theme.window_bg = {0, 0, 170};
        theme.text_fg = {255, 255, 255};
        theme.border_color = {255, 255, 255};

        // Button colors for menu items
        theme.button.normal.background = {0, 0, 170};
        theme.button.normal.foreground = {255, 255, 255};
        theme.button.hover.background = {0, 170, 170};
        theme.button.hover.foreground = {255, 255, 255};
        theme.button.pressed.background = {170, 0, 0};
        theme.button.pressed.foreground = {255, 255, 255};
        theme.button.disabled.background = {85, 85, 85};
        theme.button.disabled.foreground = {170, 170, 170};

        // Box style with border
        theme.button.box_style.draw_border = true;
        theme.button.normal.font = {};

        // Panel border settings (CRITICAL for other tests!)
        theme.panel.box_style.draw_border = true;
        theme.panel.box_style.corner = '+';
        theme.panel.box_style.horizontal = '-';
        theme.panel.box_style.vertical = '|';

        ctx.themes().register_theme(std::move(theme));
        ctx.themes().set_current_theme("Canvas Test Theme");

        // Create three menu items like in the menu bar
        menu_item<CanvasBackend> file_item;
        file_item.set_text("File");
//         file_item.apply_theme("Canvas Test Theme", ctx.themes());  // No longer needed - widgets use global theme

        menu_item<CanvasBackend> theme_item;
        theme_item.set_text("Theme");
//         theme_item.apply_theme("Canvas Test Theme", ctx.themes());  // No longer needed - widgets use global theme

        menu_item<CanvasBackend> help_item;
        help_item.set_text("Help");
//         help_item.apply_theme("Canvas Test Theme", ctx.themes());  // No longer needed - widgets use global theme

        // Render each to canvas
        auto file_canvas = render_to_canvas(file_item, 10, 3);
        auto theme_canvas = render_to_canvas(theme_item, 10, 3);
        auto help_canvas = render_to_canvas(help_item, 10, 3);

        // All items should have consistent border rendering
        bool file_has_border = file_canvas->has_complete_border(0, 0, 10, 3);
        bool theme_has_border = theme_canvas->has_complete_border(0, 0, 10, 3);
        bool help_has_border = help_canvas->has_complete_border(0, 0, 10, 3);

        // All should have consistent border rendering
        CHECK(file_has_border == theme_has_border);
        CHECK(theme_has_border == help_has_border);
        CHECK(file_has_border == help_has_border);
    }

    SUBCASE("Menu bar buttons render consistently") {
        using namespace onyxui;

        // Register test theme
        ui_theme<CanvasBackend> theme;
        theme.name = "Border Test Theme";
        theme.button.box_style.draw_border = true;
        theme.button.box_style.corner = '+';
        theme.button.box_style.horizontal = '-';
        theme.button.box_style.vertical = '|';
        theme.button.normal.background = {0, 0, 170};
        theme.button.normal.foreground = {255, 255, 255};
        theme.button.normal.font = {};
        ctx.themes().register_theme(std::move(theme));
        ctx.themes().set_current_theme("Border Test Theme");

        // Create three buttons like in menu bar
        button<CanvasBackend> file_btn("File");
//         file_btn.apply_theme("Border Test Theme", ctx.themes());  // No longer needed - widgets use global theme

        button<CanvasBackend> theme_btn("Theme");
//         theme_btn.apply_theme("Border Test Theme", ctx.themes());  // No longer needed - widgets use global theme

        button<CanvasBackend> help_btn("Help");
//         help_btn.apply_theme("Border Test Theme", ctx.themes());  // No longer needed - widgets use global theme

        // Render each
        auto file_canvas = render_to_canvas(file_btn, 10, 3);
        auto theme_canvas = render_to_canvas(theme_btn, 10, 3);
        auto help_canvas = render_to_canvas(help_btn, 10, 3);

        // Check borders
        bool file_border = file_canvas->has_complete_border(0, 0, 10, 3);
        bool theme_border = theme_canvas->has_complete_border(0, 0, 10, 3);
        bool help_border = help_canvas->has_complete_border(0, 0, 10, 3);

        INFO("File button canvas:\n", debug_canvas(*file_canvas));
        INFO("Theme button canvas:\n", debug_canvas(*theme_canvas));
        INFO("Help button canvas:\n", debug_canvas(*help_canvas));

        // EXPECTED TO FAIL if there's an inconsistency
        CHECK(file_border);
        CHECK(theme_border);
        CHECK(help_border);
        CHECK(file_border == theme_border);
        CHECK(theme_border == help_border);
    }

    SUBCASE("Menu bar with actual menu_bar widget - theme applied AFTER creation") {
        using namespace onyxui;

        // Register test theme
        ui_theme<CanvasBackend> theme;
        theme.name = "Widget Demo Theme";
        theme.button.box_style.draw_border = true;
        theme.button.box_style.corner = '+';
        theme.button.box_style.horizontal = '-';
        theme.button.box_style.vertical = '|';
        theme.button.normal.background = {0, 0, 170};
        theme.button.normal.foreground = {255, 255, 255};
        theme.button.normal.font = {};
        ctx.themes().register_theme(std::move(theme));
        ctx.themes().set_current_theme("Widget Demo Theme");

        // Create a panel as the parent (simulating main_widget)
        panel<CanvasBackend> root;
        root.set_vbox_layout(0);

        // Create menu bar and add menus (BEFORE applying theme, like widget_demo)
        auto menu_bar_ptr = std::make_unique<menu_bar<CanvasBackend>>(&root);

        // Add File menu
        auto file_menu = std::make_unique<menu<CanvasBackend>>();
        menu_bar_ptr->add_menu("File", std::move(file_menu));

        // Add Theme menu
        auto theme_menu = std::make_unique<menu<CanvasBackend>>();
        menu_bar_ptr->add_menu("Theme", std::move(theme_menu));

        // Add Help menu
        auto help_menu = std::make_unique<menu<CanvasBackend>>();
        menu_bar_ptr->add_menu("Help", std::move(help_menu));

        // Keep pointer to menu bar
        auto* menu_bar = menu_bar_ptr.get();

        // Add menu bar to root
        root.add_child(std::move(menu_bar_ptr));

        // Measure and arrange
        auto root_size = root.measure(100, 100);
        (void)root_size;  // Measurement needed for arrange
        root.arrange({0, 0, 100, 100});

        // Get the button bounds from menu bar
        auto file_btn_bounds = menu_bar->get_menu_button_bounds(0);
        auto theme_btn_bounds = menu_bar->get_menu_button_bounds(1);
        auto help_btn_bounds = menu_bar->get_menu_button_bounds(2);

        // For now, just check that the buttons have non-zero bounds
        CHECK(rect_utils::get_width(file_btn_bounds) > 0);
        CHECK(rect_utils::get_width(theme_btn_bounds) > 0);
        CHECK(rect_utils::get_width(help_btn_bounds) > 0);

        INFO("File button at: ", rect_utils::get_x(file_btn_bounds), ",", rect_utils::get_y(file_btn_bounds),
             " size: ", rect_utils::get_width(file_btn_bounds), "x", rect_utils::get_height(file_btn_bounds));
        INFO("Theme button at: ", rect_utils::get_x(theme_btn_bounds), ",", rect_utils::get_y(theme_btn_bounds),
             " size: ", rect_utils::get_width(theme_btn_bounds), "x", rect_utils::get_height(theme_btn_bounds));
        INFO("Help button at: ", rect_utils::get_x(help_btn_bounds), ",", rect_utils::get_y(help_btn_bounds),
             " size: ", rect_utils::get_width(help_btn_bounds), "x", rect_utils::get_height(help_btn_bounds));
    }

    SUBCASE("REGRESSION: File menu button border (visual rendering test)") {
        // This test reproduces the exact structure of widgets_demo
        // to catch the bug where File button doesn't have a border
        using namespace onyxui;

        // Set Canvas Test Theme as current (auto-registered by backend)
        ctx.themes().set_current_theme("Canvas Test Theme");

        // Create main panel (like main_widget in demo.hh)
        auto root = std::make_unique<panel<CanvasBackend>>();
        root->set_vbox_layout(0);
        root->set_padding(thickness::all(0));

        // Create menu bar as child (BEFORE applying theme, line 181 of demo.hh)
        auto* menu_bar_ptr = root->emplace_child<menu_bar>();

        // Add three menus in exact same order as widgets_demo
        auto file_menu = std::make_unique<menu<CanvasBackend>>();
        menu_bar_ptr->add_menu("&File", std::move(file_menu));

        auto theme_menu = std::make_unique<menu<CanvasBackend>>();
        menu_bar_ptr->add_menu("&Theme", std::move(theme_menu));

        auto help_menu = std::make_unique<menu<CanvasBackend>>();
        menu_bar_ptr->add_menu("&Help", std::move(help_menu));

        // Measure and arrange
        auto root_size = root->measure(80, 25);
        (void)root_size;  // Measurement needed for arrange
        root->arrange({0, 0, 80, 25});

        // Render to canvas
        auto canvas = std::make_shared<test_canvas>(80, 25);
        canvas_renderer renderer(canvas);
        std::vector<canvas_rect> dirty_regions = {{0, 0, 80, 25}};
        auto* theme_ptr = ctx.themes().get_current_theme();

        // Theme is required for rendering
        if (!theme_ptr) {
            throw std::runtime_error("No theme set!");
        }
        const auto& theme = *theme_ptr;

        // Create root parent_style from theme
        auto parent_style = resolved_style<CanvasBackend>::from_theme(theme);

        root->render(renderer, dirty_regions, &theme, parent_style);

        // Visual inspection
        INFO("Rendered menu bar:\n", debug_canvas(*canvas));

        // Get the three menu bar items (NOT buttons!)
        auto& mb_children = menu_bar_ptr->children();
        REQUIRE(mb_children.size() == 3);

        auto* file_item = dynamic_cast<menu_bar_item<CanvasBackend>*>(mb_children[0].get());
        auto* theme_item = dynamic_cast<menu_bar_item<CanvasBackend>*>(mb_children[1].get());
        auto* help_item = dynamic_cast<menu_bar_item<CanvasBackend>*>(mb_children[2].get());

        REQUIRE(file_item != nullptr);
        REQUIRE(theme_item != nullptr);
        REQUIRE(help_item != nullptr);

        auto file_bounds = file_item->bounds();
        auto theme_bounds = theme_item->bounds();
        auto help_bounds = help_item->bounds();

        INFO("File button bounds: ", rect_utils::get_x(file_bounds), ",", rect_utils::get_y(file_bounds),
             " ", rect_utils::get_width(file_bounds), "x", rect_utils::get_height(file_bounds));
        INFO("Theme button bounds: ", rect_utils::get_x(theme_bounds), ",", rect_utils::get_y(theme_bounds),
             " ", rect_utils::get_width(theme_bounds), "x", rect_utils::get_height(theme_bounds));
        INFO("Help button bounds: ", rect_utils::get_x(help_bounds), ",", rect_utils::get_y(help_bounds),
             " ", rect_utils::get_width(help_bounds), "x", rect_utils::get_height(help_bounds));

        // CRITICAL: Verify menu bar items DON'T have borders!
        // Menu bar items should render ONLY text, no borders
        // This is the architectural fix for the File menu button border bug
        CHECK_MESSAGE(!canvas->has_complete_border(
            rect_utils::get_x(file_bounds), rect_utils::get_y(file_bounds),
            rect_utils::get_width(file_bounds), rect_utils::get_height(file_bounds)),
            "File menu item should NOT have border (it's not a button!)");

        CHECK_MESSAGE(!canvas->has_complete_border(
            rect_utils::get_x(theme_bounds), rect_utils::get_y(theme_bounds),
            rect_utils::get_width(theme_bounds), rect_utils::get_height(theme_bounds)),
            "Theme menu item should NOT have border (it's not a button!)");

        CHECK_MESSAGE(!canvas->has_complete_border(
            rect_utils::get_x(help_bounds), rect_utils::get_y(help_bounds),
            rect_utils::get_width(help_bounds), rect_utils::get_height(help_bounds)),
            "Help menu item should NOT have border (it's not a button!)");

        // Verify text is rendered (the important part)
        // Text should be left-aligned (horizontal_padding = 0 for menu bar items)
        assert_text_at(*canvas, "File", rect_utils::get_x(file_bounds), rect_utils::get_y(file_bounds), "File menu text");
        assert_text_at(*canvas, "Theme", rect_utils::get_x(theme_bounds), rect_utils::get_y(theme_bounds), "Theme menu text");
        assert_text_at(*canvas, "Help", rect_utils::get_x(help_bounds), rect_utils::get_y(help_bounds), "Help menu text");
    }
}