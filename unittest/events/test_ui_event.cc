/**
 * @file test_ui_event.cc
 * @brief Unit tests for ui_event structures (Phase 1)
 * @author igor
 * @date 28/10/2025
 *
 * @details
 * Tests for the framework-level event structures:
 * - keyboard_event: Construction, modifiers, key_code
 * - mouse_event: Construction, button/action enums
 * - resize_event: Construction, dimensions
 * - ui_event variant: Type safety, std::visit, std::get_if
 */

#include <doctest/doctest.h>
#include <onyxui/events/ui_event.hh>
#include <onyxui/hotkeys/key_code.hh>
#include <variant>

using namespace onyxui;

// ============================================================================
// keyboard_event Tests
// ============================================================================

TEST_CASE("keyboard_event - Character key construction") {
    keyboard_event kbd{};
    kbd.key = key_code::a;
    kbd.modifiers = key_modifier::none;
    kbd.pressed = true;

    CHECK(kbd.key == key_code::a);
    CHECK(kbd.modifiers == key_modifier::none);
    CHECK(kbd.pressed);
}

TEST_CASE("keyboard_event - Function key construction") {
    keyboard_event kbd{};
    kbd.key = key_code::f10;
    kbd.modifiers = key_modifier::none;
    kbd.pressed = true;

    CHECK(kbd.key == key_code::f10);
    CHECK(kbd.modifiers == key_modifier::none);
}

TEST_CASE("keyboard_event - Special key construction (arrow down)") {
    keyboard_event kbd{};
    kbd.key = key_code::arrow_down;
    kbd.modifiers = key_modifier::none;
    kbd.pressed = true;

    CHECK(kbd.key == key_code::arrow_down);
}

TEST_CASE("keyboard_event - Modifier flags") {
    SUBCASE("Ctrl modifier") {
        keyboard_event kbd{};
        kbd.key = key_code::s;
        kbd.modifiers = key_modifier::ctrl;

        CHECK((kbd.modifiers & key_modifier::ctrl) != key_modifier::none);
        CHECK((kbd.modifiers & key_modifier::alt) == key_modifier::none);
        CHECK((kbd.modifiers & key_modifier::shift) == key_modifier::none);
    }

    SUBCASE("Alt modifier") {
        keyboard_event kbd{};
        kbd.key = key_code::f;
        kbd.modifiers = key_modifier::alt;

        CHECK((kbd.modifiers & key_modifier::ctrl) == key_modifier::none);
        CHECK((kbd.modifiers & key_modifier::alt) != key_modifier::none);
        CHECK((kbd.modifiers & key_modifier::shift) == key_modifier::none);
    }

    SUBCASE("Shift modifier") {
        keyboard_event kbd{};
        kbd.key = key_code::a;  // Normalized to lowercase
        kbd.modifiers = key_modifier::shift;

        CHECK((kbd.modifiers & key_modifier::ctrl) == key_modifier::none);
        CHECK((kbd.modifiers & key_modifier::alt) == key_modifier::none);
        CHECK((kbd.modifiers & key_modifier::shift) != key_modifier::none);
    }

    SUBCASE("Multiple modifiers (Ctrl+Shift+S)") {
        keyboard_event kbd{};
        kbd.key = key_code::s;
        kbd.modifiers = key_modifier::ctrl | key_modifier::shift;

        CHECK((kbd.modifiers & key_modifier::ctrl) != key_modifier::none);
        CHECK((kbd.modifiers & key_modifier::alt) == key_modifier::none);
        CHECK((kbd.modifiers & key_modifier::shift) != key_modifier::none);
    }
}

TEST_CASE("keyboard_event - Control characters") {
    SUBCASE("Enter key") {
        keyboard_event kbd{};
        kbd.key = key_code::enter;
        kbd.modifiers = key_modifier::none;

        CHECK(kbd.key == key_code::enter);
        CHECK(kbd.modifiers == key_modifier::none);
    }

    SUBCASE("Tab key") {
        keyboard_event kbd{};
        kbd.key = key_code::tab;
        kbd.modifiers = key_modifier::none;

        CHECK(kbd.key == key_code::tab);
        CHECK(kbd.modifiers == key_modifier::none);
    }

    SUBCASE("Escape key") {
        keyboard_event kbd{};
        kbd.key = key_code::escape;

        CHECK(kbd.key == key_code::escape);
    }
}

TEST_CASE("keyboard_event - Key press/release") {
    SUBCASE("Key pressed") {
        keyboard_event kbd{};
        kbd.key = key_code::a;
        kbd.pressed = true;

        CHECK(kbd.pressed);
    }

    SUBCASE("Key released") {
        keyboard_event kbd{};
        kbd.key = key_code::a;
        kbd.pressed = false;

        CHECK_FALSE(kbd.pressed);
    }
}

// ============================================================================
// mouse_event Tests
// ============================================================================

TEST_CASE("mouse_event - Button press") {
    SUBCASE("Left button press") {
        mouse_event mouse{};
        mouse.x = 10;
        mouse.y = 20;
        mouse.btn = mouse_event::button::left;
        mouse.act = mouse_event::action::press;
        mouse.modifiers.ctrl = false;
        mouse.modifiers.alt = false;
        mouse.modifiers.shift = false;

        CHECK(mouse.x == 10);
        CHECK(mouse.y == 20);
        CHECK(mouse.btn == mouse_event::button::left);
        CHECK(mouse.act == mouse_event::action::press);
    }

    SUBCASE("Right button press") {
        mouse_event mouse{};
        mouse.x = 50;
        mouse.y = 60;
        mouse.btn = mouse_event::button::right;
        mouse.act = mouse_event::action::press;

        CHECK(mouse.btn == mouse_event::button::right);
        CHECK(mouse.act == mouse_event::action::press);
    }

    SUBCASE("Middle button press") {
        mouse_event mouse{};
        mouse.x = 100;
        mouse.y = 150;
        mouse.btn = mouse_event::button::middle;
        mouse.act = mouse_event::action::press;

        CHECK(mouse.btn == mouse_event::button::middle);
        CHECK(mouse.act == mouse_event::action::press);
    }
}

TEST_CASE("mouse_event - Button release") {
    mouse_event mouse{};
    mouse.x = 10;
    mouse.y = 20;
    mouse.btn = mouse_event::button::none;  // Unknown which button
    mouse.act = mouse_event::action::release;

    CHECK(mouse.btn == mouse_event::button::none);
    CHECK(mouse.act == mouse_event::action::release);
}

TEST_CASE("mouse_event - Mouse move") {
    mouse_event mouse{};
    mouse.x = 100;
    mouse.y = 150;
    mouse.btn = mouse_event::button::none;
    mouse.act = mouse_event::action::move;

    CHECK(mouse.x == 100);
    CHECK(mouse.y == 150);
    CHECK(mouse.btn == mouse_event::button::none);
    CHECK(mouse.act == mouse_event::action::move);
}

TEST_CASE("mouse_event - Mouse wheel") {
    SUBCASE("Wheel up") {
        mouse_event mouse{};
        mouse.x = 50;
        mouse.y = 60;
        mouse.btn = mouse_event::button::none;
        mouse.act = mouse_event::action::wheel_up;

        CHECK(mouse.act == mouse_event::action::wheel_up);
    }

    SUBCASE("Wheel down") {
        mouse_event mouse{};
        mouse.x = 50;
        mouse.y = 60;
        mouse.btn = mouse_event::button::none;
        mouse.act = mouse_event::action::wheel_down;

        CHECK(mouse.act == mouse_event::action::wheel_down);
    }
}

TEST_CASE("mouse_event - Modifiers during mouse action") {
    SUBCASE("Ctrl+Click") {
        mouse_event mouse{};
        mouse.x = 10;
        mouse.y = 20;
        mouse.btn = mouse_event::button::left;
        mouse.act = mouse_event::action::press;
        mouse.modifiers.ctrl = true;
        mouse.modifiers.alt = false;
        mouse.modifiers.shift = false;

        // Cast bit-fields to bool for doctest CHECK macro compatibility
        CHECK(static_cast<bool>(mouse.modifiers.ctrl));
        CHECK_FALSE(static_cast<bool>(mouse.modifiers.alt));
        CHECK_FALSE(static_cast<bool>(mouse.modifiers.shift));
    }

    SUBCASE("Shift+Click") {
        mouse_event mouse{};
        mouse.x = 10;
        mouse.y = 20;
        mouse.btn = mouse_event::button::left;
        mouse.act = mouse_event::action::press;
        mouse.modifiers.ctrl = false;
        mouse.modifiers.alt = false;
        mouse.modifiers.shift = true;

        // Cast bit-fields to bool for doctest CHECK macro compatibility
        CHECK_FALSE(static_cast<bool>(mouse.modifiers.ctrl));
        CHECK_FALSE(static_cast<bool>(mouse.modifiers.alt));
        CHECK(static_cast<bool>(mouse.modifiers.shift));
    }
}

// ============================================================================
// resize_event Tests
// ============================================================================

TEST_CASE("resize_event - Construction") {
    resize_event resize{};
    resize.width = 1024;
    resize.height = 768;

    CHECK(resize.width == 1024);
    CHECK(resize.height == 768);
}

TEST_CASE("resize_event - Terminal dimensions") {
    resize_event resize{};
    resize.width = 80;
    resize.height = 24;

    CHECK(resize.width == 80);
    CHECK(resize.height == 24);
}

// ============================================================================
// ui_event Variant Tests
// ============================================================================

TEST_CASE("ui_event - Variant holds keyboard_event") {
    keyboard_event kbd{};
    kbd.key = key_code::a;

    ui_event evt = kbd;

    CHECK(std::holds_alternative<keyboard_event>(evt));
    CHECK_FALSE(std::holds_alternative<mouse_event>(evt));
    CHECK_FALSE(std::holds_alternative<resize_event>(evt));
}

TEST_CASE("ui_event - Variant holds mouse_event") {
    mouse_event mouse{};
    mouse.x = 10;
    mouse.y = 20;
    mouse.btn = mouse_event::button::left;
    mouse.act = mouse_event::action::press;

    ui_event evt = mouse;

    CHECK_FALSE(std::holds_alternative<keyboard_event>(evt));
    CHECK(std::holds_alternative<mouse_event>(evt));
    CHECK_FALSE(std::holds_alternative<resize_event>(evt));
}

TEST_CASE("ui_event - Variant holds resize_event") {
    resize_event resize{};
    resize.width = 80;
    resize.height = 24;

    ui_event evt = resize;

    CHECK_FALSE(std::holds_alternative<keyboard_event>(evt));
    CHECK_FALSE(std::holds_alternative<mouse_event>(evt));
    CHECK(std::holds_alternative<resize_event>(evt));
}

TEST_CASE("ui_event - std::get_if access pattern") {
    SUBCASE("Get keyboard event") {
        keyboard_event kbd{};
        kbd.key = key_code::s;
        kbd.modifiers = key_modifier::ctrl;

        ui_event evt = kbd;

        auto* kbd_ptr = std::get_if<keyboard_event>(&evt);
        REQUIRE(kbd_ptr != nullptr);
        CHECK(kbd_ptr->key == key_code::s);
        CHECK((kbd_ptr->modifiers & key_modifier::ctrl) != key_modifier::none);
    }

    SUBCASE("Get mouse event") {
        mouse_event mouse{};
        mouse.x = 50;
        mouse.y = 60;
        mouse.btn = mouse_event::button::right;

        ui_event evt = mouse;

        auto* mouse_ptr = std::get_if<mouse_event>(&evt);
        REQUIRE(mouse_ptr != nullptr);
        CHECK(mouse_ptr->x == 50);
        CHECK(mouse_ptr->y == 60);
        CHECK(mouse_ptr->btn == mouse_event::button::right);
    }

    SUBCASE("Get resize event") {
        resize_event resize{};
        resize.width = 1920;
        resize.height = 1080;

        ui_event evt = resize;

        auto* resize_ptr = std::get_if<resize_event>(&evt);
        REQUIRE(resize_ptr != nullptr);
        CHECK(resize_ptr->width == 1920);
        CHECK(resize_ptr->height == 1080);
    }
}

TEST_CASE("ui_event - std::visit pattern") {
    SUBCASE("Visit keyboard event") {
        keyboard_event kbd{};
        kbd.key = key_code::a;

        ui_event evt = kbd;

        bool visited = false;
        std::visit([&visited](auto&& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, keyboard_event>) {
                visited = true;
                CHECK(e.key == key_code::a);
            }
        }, evt);

        CHECK(visited);
    }

    SUBCASE("Visit mouse event") {
        mouse_event mouse{};
        mouse.x = 100;
        mouse.y = 150;

        ui_event evt = mouse;

        bool visited = false;
        std::visit([&visited](auto&& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, mouse_event>) {
                visited = true;
                CHECK(e.x == 100);
                CHECK(e.y == 150);
            }
        }, evt);

        CHECK(visited);
    }

    SUBCASE("Visit resize event") {
        resize_event resize{};
        resize.width = 80;
        resize.height = 24;

        ui_event evt = resize;

        bool visited = false;
        std::visit([&visited](auto&& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, resize_event>) {
                visited = true;
                CHECK(e.width == 80);
                CHECK(e.height == 24);
            }
        }, evt);

        CHECK(visited);
    }
}

TEST_CASE("ui_event - Type safety (wrong type returns nullptr)") {
    keyboard_event kbd{};
    kbd.key = key_code::x;

    ui_event evt = kbd;

    // Try to get wrong type
    auto* mouse_ptr = std::get_if<mouse_event>(&evt);
    auto* resize_ptr = std::get_if<resize_event>(&evt);

    CHECK(mouse_ptr == nullptr);
    CHECK(resize_ptr == nullptr);

    // Correct type succeeds
    auto* kbd_ptr = std::get_if<keyboard_event>(&evt);
    CHECK(kbd_ptr != nullptr);
}
