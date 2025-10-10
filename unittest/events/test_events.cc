/**
 * @file test_events.cc
 * @brief Comprehensive tests for event traits and concepts
 */

#include <doctest/doctest.h>
#include <onyxui/events.hh>
#include <utils/test_backend.hh>

using namespace onyxui;

TEST_SUITE("Events") {
    // ======================================================================
    // Event Concept Tests
    // ======================================================================

    TEST_CASE("MousePositionEvent concept") {
        SUBCASE("test_mouse_event satisfies MousePositionEvent") {
            CHECK(MousePositionEvent<test_backend::test_mouse_event>);
        }

        SUBCASE("test_keyboard_event does not satisfy MousePositionEvent") {
            CHECK_FALSE(MousePositionEvent<test_backend::test_keyboard_event>);
        }
    }

    TEST_CASE("MouseButtonEvent concept") {
        SUBCASE("test_mouse_event satisfies MouseButtonEvent") {
            CHECK(MouseButtonEvent<test_backend::test_mouse_event>);
        }

        SUBCASE("requires both position and button info") {
            // test_mouse_event has x, y, button, and pressed
            CHECK(MousePositionEvent<test_backend::test_mouse_event>);
            CHECK(MouseButtonEvent<test_backend::test_mouse_event>);
        }
    }

    TEST_CASE("KeyboardEvent concept") {
        SUBCASE("test_keyboard_event satisfies KeyboardEvent") {
            CHECK(KeyboardEvent<test_backend::test_keyboard_event>);
        }

        SUBCASE("test_mouse_event does not satisfy KeyboardEvent") {
            CHECK_FALSE(KeyboardEvent<test_backend::test_mouse_event>);
        }
    }

    TEST_CASE("ModifierState concept") {
        SUBCASE("test_keyboard_event satisfies ModifierState") {
            CHECK(ModifierState<test_backend::test_keyboard_event>);
        }

        SUBCASE("test_mouse_event does not satisfy ModifierState by default") {
            // Our simple test backend doesn't include modifiers in mouse events
            CHECK_FALSE(ModifierState<test_backend::test_mouse_event>);
        }
    }

    // ======================================================================
    // Keyboard Event Traits Tests
    // ======================================================================

    TEST_CASE("Keyboard event traits - basic accessors") {
        using traits = event_traits<test_backend::test_keyboard_event>;

        SUBCASE("key_code accessor") {
            test_backend::test_keyboard_event evt;
            evt.key_code = 42;
            CHECK(traits::key_code(evt) == 42);
        }

        SUBCASE("is_key_press accessor") {
            test_backend::test_keyboard_event evt;
            evt.pressed = true;
            CHECK(traits::is_key_press(evt) == true);

            evt.pressed = false;
            CHECK(traits::is_key_press(evt) == false);
        }

        SUBCASE("is_repeat always returns false for test backend") {
            test_backend::test_keyboard_event evt;
            CHECK(traits::is_repeat(evt) == false);
        }
    }

    TEST_CASE("Keyboard event traits - modifier keys") {
        using traits = event_traits<test_backend::test_keyboard_event>;

        SUBCASE("shift_pressed") {
            test_backend::test_keyboard_event evt;
            evt.shift = false;
            CHECK(traits::shift_pressed(evt) == false);

            evt.shift = true;
            CHECK(traits::shift_pressed(evt) == true);
        }

        SUBCASE("ctrl_pressed") {
            test_backend::test_keyboard_event evt;
            evt.ctrl = false;
            CHECK(traits::ctrl_pressed(evt) == false);

            evt.ctrl = true;
            CHECK(traits::ctrl_pressed(evt) == true);
        }

        SUBCASE("alt_pressed") {
            test_backend::test_keyboard_event evt;
            evt.alt = false;
            CHECK(traits::alt_pressed(evt) == false);

            evt.alt = true;
            CHECK(traits::alt_pressed(evt) == true);
        }

        SUBCASE("multiple modifiers") {
            test_backend::test_keyboard_event evt;
            evt.shift = true;
            evt.ctrl = true;
            evt.alt = false;

            CHECK(traits::shift_pressed(evt) == true);
            CHECK(traits::ctrl_pressed(evt) == true);
            CHECK(traits::alt_pressed(evt) == false);
        }
    }

    TEST_CASE("Keyboard event traits - key identification") {
        using traits = event_traits<test_backend::test_keyboard_event>;

        SUBCASE("is_tab_key") {
            test_backend::test_keyboard_event evt;
            evt.key_code = '\t';
            CHECK(traits::is_tab_key(evt) == true);

            evt.key_code = 'A';
            CHECK(traits::is_tab_key(evt) == false);
        }

        SUBCASE("is_enter_key") {
            test_backend::test_keyboard_event evt;
            evt.key_code = '\n';
            CHECK(traits::is_enter_key(evt) == true);

            evt.key_code = '\r';
            CHECK(traits::is_enter_key(evt) == false);
        }

        SUBCASE("is_space_key") {
            test_backend::test_keyboard_event evt;
            evt.key_code = ' ';
            CHECK(traits::is_space_key(evt) == true);

            evt.key_code = '\t';
            CHECK(traits::is_space_key(evt) == false);
        }

        SUBCASE("is_escape_key") {
            test_backend::test_keyboard_event evt;
            evt.key_code = 27;  // ESC key code
            CHECK(traits::is_escape_key(evt) == true);

            evt.key_code = 'E';
            CHECK(traits::is_escape_key(evt) == false);
        }
    }

    TEST_CASE("Keyboard event traits - named constants") {
        using traits = event_traits<test_backend::test_keyboard_event>;

        SUBCASE("KEY_TAB constant") {
            CHECK(traits::KEY_TAB == '\t');
        }

        SUBCASE("KEY_ENTER constant") {
            CHECK(traits::KEY_ENTER == '\n');
        }

        SUBCASE("KEY_SPACE constant") {
            CHECK(traits::KEY_SPACE == ' ');
        }

        SUBCASE("KEY_ESCAPE constant") {
            CHECK(traits::KEY_ESCAPE == 27);
        }
    }

    // ======================================================================
    // Mouse Event Traits Tests
    // ======================================================================

    TEST_CASE("Mouse event traits - position") {
        using traits = event_traits<test_backend::test_mouse_event>;

        SUBCASE("mouse_x accessor") {
            test_backend::test_mouse_event evt;
            evt.x = 100;
            CHECK(traits::mouse_x(evt) == 100);
        }

        SUBCASE("mouse_y accessor") {
            test_backend::test_mouse_event evt;
            evt.y = 200;
            CHECK(traits::mouse_y(evt) == 200);
        }

        SUBCASE("position at origin") {
            test_backend::test_mouse_event evt;
            evt.x = 0;
            evt.y = 0;
            CHECK(traits::mouse_x(evt) == 0);
            CHECK(traits::mouse_y(evt) == 0);
        }

        SUBCASE("negative positions") {
            test_backend::test_mouse_event evt;
            evt.x = -10;
            evt.y = -20;
            CHECK(traits::mouse_x(evt) == -10);
            CHECK(traits::mouse_y(evt) == -20);
        }
    }

    TEST_CASE("Mouse event traits - button state") {
        using traits = event_traits<test_backend::test_mouse_event>;

        SUBCASE("mouse_button accessor") {
            test_backend::test_mouse_event evt;
            evt.button = 1;  // Left button
            CHECK(traits::mouse_button(evt) == 1);

            evt.button = 2;  // Middle button
            CHECK(traits::mouse_button(evt) == 2);

            evt.button = 3;  // Right button
            CHECK(traits::mouse_button(evt) == 3);
        }

        SUBCASE("is_button_press for pressed") {
            test_backend::test_mouse_event evt;
            evt.pressed = true;
            CHECK(traits::is_button_press(evt) == true);
        }

        SUBCASE("is_button_press for released") {
            test_backend::test_mouse_event evt;
            evt.pressed = false;
            CHECK(traits::is_button_press(evt) == false);
        }
    }

    TEST_CASE("Mouse event traits - combined scenarios") {
        using traits = event_traits<test_backend::test_mouse_event>;

        SUBCASE("left click at specific position") {
            test_backend::test_mouse_event evt;
            evt.x = 50;
            evt.y = 75;
            evt.button = 1;
            evt.pressed = true;

            CHECK(traits::mouse_x(evt) == 50);
            CHECK(traits::mouse_y(evt) == 75);
            CHECK(traits::mouse_button(evt) == 1);
            CHECK(traits::is_button_press(evt) == true);
        }

        SUBCASE("right button release") {
            test_backend::test_mouse_event evt;
            evt.x = 100;
            evt.y = 100;
            evt.button = 3;
            evt.pressed = false;

            CHECK(traits::mouse_button(evt) == 3);
            CHECK(traits::is_button_press(evt) == false);
        }
    }

    // ======================================================================
    // Event Trait Consistency Tests
    // ======================================================================

    TEST_CASE("Event traits are noexcept") {
        using kb_traits = event_traits<test_backend::test_keyboard_event>;
        using mouse_traits = event_traits<test_backend::test_mouse_event>;

        SUBCASE("keyboard trait methods are noexcept") {
            test_backend::test_keyboard_event evt;
            CHECK(noexcept(kb_traits::key_code(evt)));
            CHECK(noexcept(kb_traits::is_key_press(evt)));
            CHECK(noexcept(kb_traits::shift_pressed(evt)));
            CHECK(noexcept(kb_traits::is_tab_key(evt)));
        }

        SUBCASE("mouse trait methods are noexcept") {
            test_backend::test_mouse_event evt;
            CHECK(noexcept(mouse_traits::mouse_x(evt)));
            CHECK(noexcept(mouse_traits::mouse_y(evt)));
            CHECK(noexcept(mouse_traits::mouse_button(evt)));
            CHECK(noexcept(mouse_traits::is_button_press(evt)));
        }
    }

    // ======================================================================
    // Real-world Event Scenarios
    // ======================================================================

    TEST_CASE("Real-world keyboard scenarios") {
        using traits = event_traits<test_backend::test_keyboard_event>;

        SUBCASE("Ctrl+S shortcut") {
            test_backend::test_keyboard_event evt;
            evt.key_code = 'S';
            evt.pressed = true;
            evt.ctrl = true;
            evt.shift = false;
            evt.alt = false;

            CHECK(traits::is_key_press(evt));
            CHECK(traits::ctrl_pressed(evt));
            CHECK_FALSE(traits::shift_pressed(evt));
        }

        SUBCASE("Shift+Tab navigation") {
            test_backend::test_keyboard_event evt;
            evt.key_code = '\t';
            evt.pressed = true;
            evt.shift = true;
            evt.ctrl = false;
            evt.alt = false;

            CHECK(traits::is_tab_key(evt));
            CHECK(traits::shift_pressed(evt));
        }

        SUBCASE("Alt+F4 close window") {
            test_backend::test_keyboard_event evt;
            evt.key_code = 'F';  // Simplified F4 as 'F'
            evt.pressed = true;
            evt.alt = true;
            evt.ctrl = false;
            evt.shift = false;

            CHECK(traits::is_key_press(evt));
            CHECK(traits::alt_pressed(evt));
        }

        SUBCASE("Escape key to cancel") {
            test_backend::test_keyboard_event evt;
            evt.key_code = 27;
            evt.pressed = true;
            evt.shift = false;
            evt.ctrl = false;
            evt.alt = false;

            CHECK(traits::is_escape_key(evt));
            CHECK(traits::is_key_press(evt));
        }
    }

    TEST_CASE("Real-world mouse scenarios") {
        using traits = event_traits<test_backend::test_mouse_event>;

        SUBCASE("Double-click detection") {
            // First click
            test_backend::test_mouse_event click1;
            click1.x = 100;
            click1.y = 100;
            click1.button = 1;
            click1.pressed = true;

            // Second click at same position
            test_backend::test_mouse_event click2;
            click2.x = 100;
            click2.y = 100;
            click2.button = 1;
            click2.pressed = true;

            CHECK(traits::mouse_x(click1) == traits::mouse_x(click2));
            CHECK(traits::mouse_y(click1) == traits::mouse_y(click2));
            CHECK(traits::mouse_button(click1) == traits::mouse_button(click2));
        }

        SUBCASE("Context menu (right click)") {
            test_backend::test_mouse_event evt;
            evt.x = 150;
            evt.y = 200;
            evt.button = 3;  // Right button
            evt.pressed = true;

            CHECK(traits::mouse_button(evt) == 3);
            CHECK(traits::is_button_press(evt));
        }

        SUBCASE("Drag operation") {
            // Mouse down
            test_backend::test_mouse_event down;
            down.x = 50;
            down.y = 50;
            down.button = 1;
            down.pressed = true;

            // Mouse moved
            test_backend::test_mouse_event move;
            move.x = 100;
            move.y = 75;

            // Mouse up
            test_backend::test_mouse_event up;
            up.x = 100;
            up.y = 75;
            up.button = 1;
            up.pressed = false;

            CHECK(traits::mouse_x(down) == 50);
            CHECK(traits::mouse_x(move) == 100);
            CHECK(traits::mouse_x(up) == 100);
            CHECK(traits::is_button_press(down));
            CHECK_FALSE(traits::is_button_press(up));
        }
    }

    // ======================================================================
    // Edge Cases and Boundary Tests
    // ======================================================================

    TEST_CASE("Edge cases for keyboard events") {
        using traits = event_traits<test_backend::test_keyboard_event>;

        SUBCASE("zero key code") {
            test_backend::test_keyboard_event evt;
            evt.key_code = 0;
            CHECK(traits::key_code(evt) == 0);
        }

        SUBCASE("all modifiers pressed") {
            test_backend::test_keyboard_event evt;
            evt.shift = true;
            evt.ctrl = true;
            evt.alt = true;

            CHECK(traits::shift_pressed(evt));
            CHECK(traits::ctrl_pressed(evt));
            CHECK(traits::alt_pressed(evt));
        }

        SUBCASE("no modifiers pressed") {
            test_backend::test_keyboard_event evt;
            evt.shift = false;
            evt.ctrl = false;
            evt.alt = false;

            CHECK_FALSE(traits::shift_pressed(evt));
            CHECK_FALSE(traits::ctrl_pressed(evt));
            CHECK_FALSE(traits::alt_pressed(evt));
        }
    }

    TEST_CASE("Edge cases for mouse events") {
        using traits = event_traits<test_backend::test_mouse_event>;

        SUBCASE("maximum coordinate values") {
            test_backend::test_mouse_event evt;
            evt.x = 9999;
            evt.y = 9999;

            CHECK(traits::mouse_x(evt) == 9999);
            CHECK(traits::mouse_y(evt) == 9999);
        }

        SUBCASE("button 0 (no button)") {
            test_backend::test_mouse_event evt;
            evt.button = 0;
            CHECK(traits::mouse_button(evt) == 0);
        }

        SUBCASE("large button numbers") {
            test_backend::test_mouse_event evt;
            evt.button = 10;  // Extended mouse buttons
            CHECK(traits::mouse_button(evt) == 10);
        }
    }

    // ======================================================================
    // Type Safety Tests
    // ======================================================================

    TEST_CASE("Event traits return correct types") {
        using kb_traits = event_traits<test_backend::test_keyboard_event>;
        using mouse_traits = event_traits<test_backend::test_mouse_event>;

        SUBCASE("keyboard traits return int for key_code") {
            test_backend::test_keyboard_event evt;
            auto code = kb_traits::key_code(evt);
            CHECK(std::is_same_v<decltype(code), int>);
        }

        SUBCASE("keyboard traits return bool for modifiers") {
            test_backend::test_keyboard_event evt;
            auto shift = kb_traits::shift_pressed(evt);
            CHECK(std::is_same_v<decltype(shift), bool>);
        }

        SUBCASE("mouse traits return int for coordinates") {
            test_backend::test_mouse_event evt;
            auto x = mouse_traits::mouse_x(evt);
            auto y = mouse_traits::mouse_y(evt);
            CHECK(std::is_same_v<decltype(x), int>);
            CHECK(std::is_same_v<decltype(y), int>);
        }

        SUBCASE("mouse traits return bool for button press") {
            test_backend::test_mouse_event evt;
            auto pressed = mouse_traits::is_button_press(evt);
            CHECK(std::is_same_v<decltype(pressed), bool>);
        }
    }
}