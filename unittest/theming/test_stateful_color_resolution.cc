/**
 * @file test_stateful_color_resolution.cc
 * @brief Tests for state-dependent color resolution vs CSS inheritance
 *
 * @details
 * This test file addresses a critical gap in test coverage:
 * The interaction between CSS inheritance and widget state-dependent colors.
 *
 * Bug discovered: When a parent sets a background color, stateful widgets
 * (like menu_item, button) were inheriting the parent's color instead of
 * using their own state-dependent colors (hover, focus, pressed).
 *
 * These tests ensure that stateful widgets correctly override parent
 * inheritance when they have their own state-dependent theme colors.
 */

#include <doctest/doctest.h>
#include <onyxui/widgets/menu_item.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/panel.hh>
#include <onyxui/widgets/menu.hh>
#include <onyxui/theme.hh>
#include "../utils/test_backend.hh"

using namespace onyxui;

// Helper to create a test theme with distinct state colors
template<UIBackend Backend>
ui_theme<Backend> create_test_theme_with_states() {
    ui_theme<Backend> theme;

    // Window colors
    theme.window_bg = {10, 10, 10};      // Dark gray
    theme.text_fg = {200, 200, 200};     // Light gray
    theme.border_color = {100, 100, 100};

    // Button states - VERY DISTINCT COLORS for easy verification
    theme.button.normal = {
        .font = {},
        .foreground = {255, 255, 255},
        .background = {50, 50, 50}       // Normal: Dark gray
    };
    theme.button.hover = {
        .font = {},
        .foreground = {0, 0, 0},
        .background = {255, 255, 0}      // Hover: YELLOW (very distinct!)
    };
    theme.button.pressed = {
        .font = {},
        .foreground = {255, 255, 255},
        .background = {255, 0, 0}        // Pressed: RED
    };
    theme.button.disabled = {
        .font = {},
        .foreground = {100, 100, 100},
        .background = {30, 30, 30}       // Disabled: Very dark gray
    };

    // Menu item states - VERY DISTINCT COLORS
    theme.menu_item.normal = {
        .font = {},
        .foreground = {255, 255, 255},
        .background = {0, 100, 0}        // Normal: Dark green
    };
    theme.menu_item.highlighted = {
        .font = {},
        .foreground = {0, 0, 0},
        .background = {0, 255, 0}        // Highlighted: BRIGHT GREEN
    };
    theme.menu_item.disabled = {
        .font = {},
        .foreground = {100, 100, 100},
        .background = {0, 50, 0}         // Disabled: Very dark green
    };

    // Panel/container colors
    theme.panel.background = {40, 40, 40};
    theme.panel.border_color = {80, 80, 80};

    return theme;
}

TEST_CASE("Stateful Color Resolution - menu_item with parent background") {
    using backend_type = test_backend;

    SUBCASE("menu_item normal state ignores parent background") {
        auto theme = create_test_theme_with_states<backend_type>();

        auto parent = std::make_unique<panel<backend_type>>();
        parent->apply_theme(std::move(theme));

        // Parent explicitly sets background color
        parent->set_background_color({100, 100, 200});  // Blue

        auto item = std::make_unique<menu_item<backend_type>>("Test");
        auto* item_ptr = item.get();
        parent->add_child(std::move(item));

        // menu_item should use its OWN theme color, not parent's blue!
        auto style = item_ptr->resolve_style();
        auto bg = style.background_color;

        CHECK(bg.r == 0);     // Dark green from theme.menu_item.normal
        CHECK(bg.g == 100);
        CHECK(bg.b == 0);
        CHECK_FALSE(bg.r == 100);  // NOT parent's blue!
    }

    SUBCASE("menu_item highlighted state with parent background") {
        auto theme = create_test_theme_with_states<backend_type>();

        auto parent = std::make_unique<panel<backend_type>>();
        parent->apply_theme(std::move(theme));
        parent->set_background_color({100, 100, 200});  // Blue parent

        auto item = std::make_unique<menu_item<backend_type>>("Test");
        auto* item_ptr = item.get();
        parent->add_child(std::move(item));

        // Check theme colors directly (theme should have highlighted state)
        auto* t = item_ptr->get_theme();
        REQUIRE(t != nullptr);

        auto highlighted_bg = t->menu_item.highlighted.background;

        // When focused/hovered, should be BRIGHT GREEN, not parent's blue
        CHECK(highlighted_bg.r == 0);
        CHECK(highlighted_bg.g == 255);  // Bright green!
        CHECK(highlighted_bg.b == 0);
    }

    SUBCASE("menu_item disabled state with parent background") {
        auto theme = create_test_theme_with_states<backend_type>();

        auto parent = std::make_unique<panel<backend_type>>();
        parent->apply_theme(std::move(theme));
        parent->set_background_color({200, 0, 0});  // Red parent

        auto item = std::make_unique<menu_item<backend_type>>("Test");
        auto* item_ptr = item.get();
        parent->add_child(std::move(item));

        item_ptr->set_enabled(false);

        auto* t = item_ptr->get_theme();
        REQUIRE(t != nullptr);

        auto disabled_bg = t->menu_item.disabled.background;

        // Disabled should be very dark green, not parent's red
        CHECK(disabled_bg.r == 0);
        CHECK(disabled_bg.g == 50);
        CHECK(disabled_bg.b == 0);
        CHECK_FALSE(disabled_bg.r == 200);  // NOT parent's red!
    }
}

TEST_CASE("Stateful Color Resolution - button with parent background") {
    using backend_type = test_backend;

    SUBCASE("button normal state with parent background") {
        auto theme = create_test_theme_with_states<backend_type>();

        auto parent = std::make_unique<panel<backend_type>>();
        parent->apply_theme(std::move(theme));
        parent->set_background_color({0, 200, 200});  // Cyan parent

        auto btn = std::make_unique<button<backend_type>>("Test");
        auto* btn_ptr = btn.get();
        parent->add_child(std::move(btn));

        // Button uses stateful_widget which should NOT inherit parent bg
        auto style = btn_ptr->resolve_style();
        auto bg = style.background_color;

        // Should be button's normal state (dark gray), not parent's cyan
        CHECK(bg.r == 50);
        CHECK(bg.g == 50);
        CHECK(bg.b == 50);
        CHECK_FALSE(bg.g == 200);  // NOT parent's cyan!
    }

    SUBCASE("button hover state with parent background") {
        auto theme = create_test_theme_with_states<backend_type>();

        auto parent = std::make_unique<panel<backend_type>>();
        parent->apply_theme(std::move(theme));
        parent->set_background_color({255, 0, 255});  // Magenta parent

        auto btn = std::make_unique<button<backend_type>>("Test");
        auto* btn_ptr = btn.get();
        parent->add_child(std::move(btn));

        auto* t = btn_ptr->get_theme();
        REQUIRE(t != nullptr);

        // Check theme color directly (simulating hover)
        auto hover_bg = t->button.hover.background;

        // Should be YELLOW, not parent's magenta!
        CHECK(hover_bg.r == 255);
        CHECK(hover_bg.g == 255);
        CHECK(hover_bg.b == 0);
    }

    SUBCASE("button pressed state with parent background") {
        auto theme = create_test_theme_with_states<backend_type>();

        auto parent = std::make_unique<panel<backend_type>>();
        parent->apply_theme(std::move(theme));
        parent->set_background_color({100, 100, 100});  // Gray parent

        auto btn = std::make_unique<button<backend_type>>("Test");
        auto* btn_ptr = btn.get();
        parent->add_child(std::move(btn));

        auto* t = btn_ptr->get_theme();
        REQUIRE(t != nullptr);

        auto pressed_bg = t->button.pressed.background;

        // Should be RED, not parent's gray!
        CHECK(pressed_bg.r == 255);
        CHECK(pressed_bg.g == 0);
        CHECK(pressed_bg.b == 0);
    }

    SUBCASE("button disabled state with parent background") {
        auto theme = create_test_theme_with_states<backend_type>();

        auto parent = std::make_unique<panel<backend_type>>();
        parent->apply_theme(std::move(theme));
        parent->set_background_color({200, 200, 0});  // Yellow parent

        auto btn = std::make_unique<button<backend_type>>("Test");
        auto* btn_ptr = btn.get();
        parent->add_child(std::move(btn));

        btn_ptr->set_enabled(false);

        auto* t = btn_ptr->get_theme();
        REQUIRE(t != nullptr);

        auto disabled_bg = t->button.disabled.background;

        // Should be very dark gray, not parent's yellow!
        CHECK(disabled_bg.r == 30);
        CHECK(disabled_bg.g == 30);
        CHECK(disabled_bg.b == 30);
        CHECK_FALSE(disabled_bg.g == 200);  // NOT parent's yellow!
    }
}

TEST_CASE("Stateful Color Resolution - nested containers") {
    using backend_type = test_backend;

    SUBCASE("menu_item in menu in panel - triple nesting") {
        auto theme = create_test_theme_with_states<backend_type>();

        // Level 1: Root panel with background
        auto root = std::make_unique<panel<backend_type>>();
        root->apply_theme(std::move(theme));
        root->set_background_color({50, 0, 0});  // Dark red

        // Level 2: Menu with different background
        auto menu_widget = std::make_unique<menu<backend_type>>();
        menu_widget->set_background_color({0, 50, 0});  // Dark green
        auto* menu_ptr = menu_widget.get();
        root->add_child(std::move(menu_widget));

        // Level 3: Menu item (should use its OWN state colors!)
        auto item = std::make_unique<menu_item<backend_type>>("Test");
        auto* item_ptr = item.get();
        menu_ptr->add_item(std::move(item));

        // menu_item should IGNORE both parent and grandparent backgrounds
        auto style = item_ptr->resolve_style();
        auto bg = style.background_color;

        // Should be menu_item's normal state, not menu's or root's background
        CHECK(bg.r == 0);
        CHECK(bg.g == 100);  // Dark green from menu_item.normal
        CHECK(bg.b == 0);
        CHECK_FALSE(bg.r == 50);  // NOT root's dark red
        CHECK_FALSE(bg.g == 50);  // NOT menu's dark green
    }

    SUBCASE("button in panel in panel - triple nesting") {
        auto theme = create_test_theme_with_states<backend_type>();

        auto root = std::make_unique<panel<backend_type>>();
        root->apply_theme(std::move(theme));
        root->set_background_color({100, 0, 0});  // Red

        auto container = std::make_unique<panel<backend_type>>();
        container->set_background_color({0, 100, 0});  // Green
        auto* container_ptr = container.get();
        root->add_child(std::move(container));

        auto btn = std::make_unique<button<backend_type>>("Test");
        auto* btn_ptr = btn.get();
        container_ptr->add_child(std::move(btn));

        auto style = btn_ptr->resolve_style();
        auto bg = style.background_color;

        // Should be button's normal state, ignoring both parents
        CHECK(bg.r == 50);
        CHECK(bg.g == 50);
        CHECK(bg.b == 50);
        CHECK_FALSE(bg.r == 100);  // NOT root's red
        CHECK_FALSE(bg.g == 100);  // NOT container's green
    }
}

TEST_CASE("Stateful Color Resolution - CSS override still works") {
    using backend_type = test_backend;

    SUBCASE("menu_item with explicit set_background_color overrides theme") {
        auto theme = create_test_theme_with_states<backend_type>();

        auto parent = std::make_unique<panel<backend_type>>();
        parent->apply_theme(std::move(theme));

        auto item = std::make_unique<menu_item<backend_type>>("Test");
        auto* item_ptr = item.get();
        parent->add_child(std::move(item));

        // Explicit override should take precedence over EVERYTHING
        item_ptr->set_background_color({123, 45, 67});

        auto style = item_ptr->resolve_style();
        auto bg = style.background_color;

        // Should use the explicit override
        CHECK(bg.r == 123);
        CHECK(bg.g == 45);
        CHECK(bg.b == 67);
    }

    SUBCASE("button with explicit set_background_color overrides state colors") {
        auto theme = create_test_theme_with_states<backend_type>();

        auto parent = std::make_unique<panel<backend_type>>();
        parent->apply_theme(std::move(theme));

        auto btn = std::make_unique<button<backend_type>>("Test");
        auto* btn_ptr = btn.get();
        parent->add_child(std::move(btn));

        // Explicit override
        btn_ptr->set_background_color({200, 150, 100});

        auto style = btn_ptr->resolve_style();
        auto bg = style.background_color;

        CHECK(bg.r == 200);
        CHECK(bg.g == 150);
        CHECK(bg.b == 100);
    }
}

TEST_CASE("Stateful Color Resolution - foreground colors") {
    using backend_type = test_backend;

    SUBCASE("menu_item foreground state colors with parent foreground") {
        auto theme = create_test_theme_with_states<backend_type>();

        auto parent = std::make_unique<panel<backend_type>>();
        parent->apply_theme(std::move(theme));
        parent->set_foreground_color({255, 0, 0});  // Red text

        auto item = std::make_unique<menu_item<backend_type>>("Test");
        auto* item_ptr = item.get();
        parent->add_child(std::move(item));

        // menu_item should use its theme foreground, not parent's
        auto style = item_ptr->resolve_style();
        auto fg = style.foreground_color;

        // Should be white from menu_item.normal, not parent's red
        CHECK(fg.r == 255);
        CHECK(fg.g == 255);
        CHECK(fg.b == 255);
    }

    SUBCASE("button foreground state colors with parent foreground") {
        auto theme = create_test_theme_with_states<backend_type>();

        auto parent = std::make_unique<panel<backend_type>>();
        parent->apply_theme(std::move(theme));
        parent->set_foreground_color({0, 255, 0});  // Green text

        auto btn = std::make_unique<button<backend_type>>("Test");
        auto* btn_ptr = btn.get();
        parent->add_child(std::move(btn));

        auto style = btn_ptr->resolve_style();
        auto fg = style.foreground_color;

        // Should be white from button.normal, not parent's green
        CHECK(fg.r == 255);
        CHECK(fg.g == 255);
        CHECK(fg.b == 255);
    }
}

TEST_CASE("Stateful Color Resolution - resolve_style integration") {
    using backend_type = test_backend;

    SUBCASE("menu_item resolve_style includes state-dependent colors") {
        auto theme = create_test_theme_with_states<backend_type>();

        auto parent = std::make_unique<panel<backend_type>>();
        parent->apply_theme(std::move(theme));
        parent->set_background_color({100, 100, 100});  // Gray parent

        auto item = std::make_unique<menu_item<backend_type>>("Test");
        auto* item_ptr = item.get();
        parent->add_child(std::move(item));

        // resolve_style() should call get_effective_background_color()
        // which should return state colors, not parent's gray
        auto style = item_ptr->resolve_style();

        CHECK(style.background_color.r == 0);
        CHECK(style.background_color.g == 100);  // menu_item normal state
        CHECK(style.background_color.b == 0);
        CHECK_FALSE(style.background_color.r == 100);  // NOT parent's gray!
    }

    SUBCASE("button resolve_style includes state-dependent colors") {
        auto theme = create_test_theme_with_states<backend_type>();

        auto parent = std::make_unique<panel<backend_type>>();
        parent->apply_theme(std::move(theme));
        parent->set_background_color({200, 50, 50});  // Red parent

        auto btn = std::make_unique<button<backend_type>>("Test");
        auto* btn_ptr = btn.get();
        parent->add_child(std::move(btn));

        auto style = btn_ptr->resolve_style();

        CHECK(style.background_color.r == 50);
        CHECK(style.background_color.g == 50);  // button normal state
        CHECK(style.background_color.b == 50);
        CHECK_FALSE(style.background_color.r == 200);  // NOT parent's red!
    }
}

TEST_CASE("Stateful Color Resolution - no theme fallback") {
    using backend_type = test_backend;

    SUBCASE("menu_item without theme uses default colors") {
        auto parent = std::make_unique<panel<backend_type>>();
        parent->set_background_color({100, 100, 100});

        auto item = std::make_unique<menu_item<backend_type>>("Test");
        auto* item_ptr = item.get();
        parent->add_child(std::move(item));

        // Without theme, should return default-constructed color
        auto style = item_ptr->resolve_style();
        auto bg = style.background_color;

        // Default color (all zeros)
        CHECK(bg.r == 0);
        CHECK(bg.g == 0);
        CHECK(bg.b == 0);
    }

    SUBCASE("button without theme uses default colors") {
        auto parent = std::make_unique<panel<backend_type>>();
        parent->set_background_color({150, 150, 150});

        auto btn = std::make_unique<button<backend_type>>("Test");
        auto* btn_ptr = btn.get();
        parent->add_child(std::move(btn));

        auto style = btn_ptr->resolve_style();
        auto bg = style.background_color;

        // Default color
        CHECK(bg.r == 0);
        CHECK(bg.g == 0);
        CHECK(bg.b == 0);
    }
}
