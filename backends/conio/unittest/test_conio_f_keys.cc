/**
 * @file test_conio_f_keys.cc
 * @brief Conio backend F-key ordering and event conversion tests
 * @author igor
 * @date 2025-10-27
 *
 * @details
 * Tests conio backend-specific assumptions about event conversion, particularly
 * F-key ordering which is reversed in termbox2 (F1=65535, F12=65524).
 */

#include <doctest/doctest.h>
#include <onyxui/conio/conio_events.hh>
#include <onyxui/conio/termbox2_wrappers.hh>
#include <onyxui/hotkeys/key_sequence.hh>

using namespace onyxui;

TEST_SUITE("Conio Backend Event Traits") {
    TEST_CASE("F-key ordering assumptions") {
        // Verify termbox2 F-key constants are in reverse order
        CHECK(TB_KEY_F1 > TB_KEY_F2);
        CHECK(TB_KEY_F2 > TB_KEY_F3);
        CHECK(TB_KEY_F10 > TB_KEY_F11);
        CHECK(TB_KEY_F11 > TB_KEY_F12);

        // Verify spacing between F-keys
        CHECK(TB_KEY_F1 - TB_KEY_F2 == 1);
        CHECK(TB_KEY_F2 - TB_KEY_F3 == 1);
        CHECK(TB_KEY_F1 - TB_KEY_F12 == 11);
    }

    TEST_CASE("F-key conversion") {
        using traits = event_traits<tb_event>;

        SUBCASE("F1 conversion") {
            tb_event ev{};
            ev.type = TB_EVENT_KEY;
            ev.key = TB_KEY_F1;

            CHECK(traits::to_f_key(ev) == 1);
            CHECK(traits::to_ascii(ev) == '\0');
            CHECK(traits::to_special_key(ev) == 0);
        }

        SUBCASE("F10 conversion") {
            tb_event ev{};
            ev.type = TB_EVENT_KEY;
            ev.key = TB_KEY_F10;

            CHECK(traits::to_f_key(ev) == 10);
            CHECK(traits::to_ascii(ev) == '\0');
            CHECK(traits::to_special_key(ev) == 0);
        }

        SUBCASE("F12 conversion") {
            tb_event ev{};
            ev.type = TB_EVENT_KEY;
            ev.key = TB_KEY_F12;

            CHECK(traits::to_f_key(ev) == 12);
            CHECK(traits::to_ascii(ev) == '\0');
            CHECK(traits::to_special_key(ev) == 0);
        }

        SUBCASE("Non-F-key returns 0") {
            tb_event ev{};
            ev.type = TB_EVENT_KEY;
            ev.key = TB_KEY_ENTER;

            CHECK(traits::to_f_key(ev) == 0);
        }
    }

    TEST_CASE("Special key conversion") {
        using traits = event_traits<tb_event>;

        SUBCASE("Arrow keys") {
            tb_event ev{};
            ev.type = TB_EVENT_KEY;

            ev.key = TB_KEY_ARROW_UP;
            CHECK(traits::to_special_key(ev) == -1);

            ev.key = TB_KEY_ARROW_DOWN;
            CHECK(traits::to_special_key(ev) == -2);

            ev.key = TB_KEY_ARROW_LEFT;
            CHECK(traits::to_special_key(ev) == -3);

            ev.key = TB_KEY_ARROW_RIGHT;
            CHECK(traits::to_special_key(ev) == -4);
        }

        SUBCASE("Control characters") {
            tb_event ev{};
            ev.type = TB_EVENT_KEY;

            ev.key = TB_KEY_ESC;
            CHECK(traits::to_ascii(ev) == 27);

            ev.key = TB_KEY_ENTER;
            CHECK(traits::to_ascii(ev) == '\n');

            ev.key = TB_KEY_TAB;
            CHECK(traits::to_ascii(ev) == '\t');
        }
    }

    TEST_CASE("Modifier detection") {
        using traits = event_traits<tb_event>;

        SUBCASE("Single modifiers") {
            tb_event ev{};
            ev.type = TB_EVENT_KEY;

            ev.mod = TB_MOD_CTRL;
            CHECK(traits::ctrl_pressed(ev));
            CHECK_FALSE(traits::alt_pressed(ev));
            CHECK_FALSE(traits::shift_pressed(ev));

            ev.mod = TB_MOD_ALT;
            CHECK_FALSE(traits::ctrl_pressed(ev));
            CHECK(traits::alt_pressed(ev));
            CHECK_FALSE(traits::shift_pressed(ev));

            ev.mod = TB_MOD_SHIFT;
            CHECK_FALSE(traits::ctrl_pressed(ev));
            CHECK_FALSE(traits::alt_pressed(ev));
            CHECK(traits::shift_pressed(ev));
        }

        SUBCASE("Combined modifiers") {
            tb_event ev{};
            ev.type = TB_EVENT_KEY;

            ev.mod = TB_MOD_CTRL | TB_MOD_ALT;
            CHECK(traits::ctrl_pressed(ev));
            CHECK(traits::alt_pressed(ev));
            CHECK_FALSE(traits::shift_pressed(ev));

            ev.mod = TB_MOD_CTRL | TB_MOD_SHIFT;
            CHECK(traits::ctrl_pressed(ev));
            CHECK_FALSE(traits::alt_pressed(ev));
            CHECK(traits::shift_pressed(ev));

            ev.mod = TB_MOD_CTRL | TB_MOD_ALT | TB_MOD_SHIFT;
            CHECK(traits::ctrl_pressed(ev));
            CHECK(traits::alt_pressed(ev));
            CHECK(traits::shift_pressed(ev));
        }
    }

    TEST_CASE("Static assertions compile") {
        // The static_asserts in to_f_key() verify our assumptions at compile time
        // If this test compiles and runs, the assertions pass
        CHECK(true);
    }
}