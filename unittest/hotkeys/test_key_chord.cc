/**
 * @file test_key_chord.cc
 * @brief Tests for multi-key sequences and modifier-only activation
 * @author igor
 * @date 2025-10-27
 */

#include <doctest/doctest.h>
#include <onyxui/hotkeys/key_chord.hh>
#include <onyxui/hotkeys/hotkey_manager.hh>
#include "../utils/test_backend.hh"
#include <chrono>
#include <thread>

using namespace onyxui;

TEST_SUITE("Key Chords") {
    TEST_CASE("Basic chord construction") {
        SUBCASE("Single-key chord") {
            key_sequence seq{'s', key_modifier::ctrl};
            key_chord chord(seq);

            CHECK(chord.type == chord_type::single_key);
            CHECK(chord.length() == 1);
            CHECK_FALSE(chord.is_modifier_only());
            CHECK_FALSE(chord.is_multi_sequence());
        }

        SUBCASE("Modifier-only chord") {
            key_chord chord(key_modifier::alt);

            CHECK(chord.type == chord_type::modifier_only);
            CHECK(chord.is_modifier_only());
            CHECK_FALSE(chord.is_multi_sequence());
            CHECK(chord.length() == 1);
        }

        SUBCASE("Multi-key sequence") {
            key_chord chord({
                {'x', key_modifier::ctrl},
                {'c', key_modifier::ctrl}
            });

            CHECK(chord.type == chord_type::multi_sequence);
            CHECK(chord.is_multi_sequence());
            CHECK_FALSE(chord.is_modifier_only());
            CHECK(chord.length() == 2);
        }
    }

    TEST_CASE("Helper functions") {
        SUBCASE("make_emacs_chord") {
            auto chord = make_emacs_chord({
                {'x', key_modifier::ctrl},
                {'s', key_modifier::ctrl}
            });

            CHECK(chord.type == chord_type::multi_sequence);
            CHECK(chord.length() == 2);
            CHECK(chord.timeout_ms == std::chrono::milliseconds{1000});
        }

        SUBCASE("make_vim_chord") {
            auto chord = make_vim_chord("dd");

            CHECK(chord.type == chord_type::modal_sequence);
            CHECK(chord.length() == 2);
            CHECK(static_cast<char>(chord.sequence[0].key) == 'd');
            CHECK(static_cast<char>(chord.sequence[1].key) == 'd');
            CHECK(chord.timeout_ms == std::chrono::milliseconds{500});
        }

        SUBCASE("make_modifier_chord") {
            auto chord = make_modifier_chord(key_modifier::alt);

            CHECK(chord.type == chord_type::modifier_only);
            CHECK(chord.is_modifier_only());
        }
    }

    TEST_CASE("Chord matcher") {
        chord_matcher matcher;

        SUBCASE("Single-key match") {
            key_chord chord(key_sequence{'s', key_modifier::ctrl});
            key_sequence seq{'s', key_modifier::ctrl};

            CHECK(matcher.process_key(seq, chord));
            CHECK_FALSE(matcher.has_partial_match());
        }

        SUBCASE("Multi-key sequence - complete match") {
            key_chord chord({
                {'x', key_modifier::ctrl},
                {'c', key_modifier::ctrl}
            });

            // First key - partial match
            key_sequence seq1{'x', key_modifier::ctrl};
            CHECK_FALSE(matcher.process_key(seq1, chord));
            CHECK(matcher.has_partial_match());
            CHECK(matcher.get_position() == 1);

            // Second key - complete match
            key_sequence seq2{'c', key_modifier::ctrl};
            CHECK(matcher.process_key(seq2, chord));
            CHECK_FALSE(matcher.has_partial_match());
        }

        SUBCASE("Multi-key sequence - mismatch") {
            key_chord chord({
                {'x', key_modifier::ctrl},
                {'c', key_modifier::ctrl}
            });

            // First key matches
            key_sequence seq1{'x', key_modifier::ctrl};
            CHECK_FALSE(matcher.process_key(seq1, chord));
            CHECK(matcher.has_partial_match());

            // Second key doesn't match - resets
            key_sequence seq2{'s', key_modifier::ctrl};
            CHECK_FALSE(matcher.process_key(seq2, chord));
            CHECK_FALSE(matcher.has_partial_match());
            CHECK(matcher.get_position() == 0);
        }

        SUBCASE("Reset clears state") {
            key_chord chord({
                {'x', key_modifier::ctrl},
                {'c', key_modifier::ctrl}
            });

            // Start partial match
            key_sequence seq{'x', key_modifier::ctrl};
            CHECK_FALSE(matcher.process_key(seq, chord));
            CHECK(matcher.has_partial_match());

            // Reset
            matcher.reset();
            CHECK_FALSE(matcher.has_partial_match());
            CHECK(matcher.get_position() == 0);
        }
    }

    TEST_CASE("Hotkey manager - chord support") {
        using Backend = test_backend;
        hotkey_manager<Backend> manager;

        SUBCASE("Register and trigger Emacs-style chord") {
            bool triggered = false;

            // Register Ctrl+X, Ctrl+C chord
            manager.register_chord(
                make_emacs_chord({
                    {'x', key_modifier::ctrl},
                    {'c', key_modifier::ctrl}
                }),
                [&triggered]() { triggered = true; }
            );

            // Process first key
            key_sequence seq1{'x', key_modifier::ctrl};
            CHECK(manager.process_chord(seq1));  // Returns true for partial match
            CHECK_FALSE(triggered);

            // Process second key
            key_sequence seq2{'c', key_modifier::ctrl};
            CHECK(manager.process_chord(seq2));  // Returns true for complete match
            CHECK(triggered);
        }

        SUBCASE("Register and trigger modifier-only activation") {
            bool menu_activated = false;

            // Register Alt key alone to activate menu (QBasic-style)
            manager.register_modifier_activation(
                key_modifier::alt,
                [&menu_activated]() { menu_activated = true; }
            );

            // Create Alt-only event
            test_backend::test_keyboard_event ev;
            ev.pressed = true;
            ev.key_code = 0;  // No character key
            ev.alt = true;
            ev.ctrl = false;
            ev.shift = false;

            CHECK(manager.handle_modifier_event(ev));
            CHECK(menu_activated);
        }

        SUBCASE("Modifier with character doesn't trigger modifier-only") {
            bool menu_activated = false;

            manager.register_modifier_activation(
                key_modifier::alt,
                [&menu_activated]() { menu_activated = true; }
            );

            // Alt+F (not just Alt)
            test_backend::test_keyboard_event ev;
            ev.pressed = true;
            ev.key_code = 'f';
            ev.alt = true;

            CHECK_FALSE(manager.handle_modifier_event(ev));
            CHECK_FALSE(menu_activated);
        }
    }

    TEST_CASE("Integration - priority order") {
        using Backend = test_backend;
        hotkey_manager<Backend> manager;

        int last_triggered = 0;

        // Register modifier-only (highest priority)
        manager.register_modifier_activation(
            key_modifier::alt,
            [&last_triggered]() { last_triggered = 1; }
        );

        // Register chord (second priority)
        manager.register_chord(
            make_emacs_chord({
                {'x', key_modifier::ctrl},
                {'c', key_modifier::ctrl}
            }),
            [&last_triggered]() { last_triggered = 2; }
        );

        // Register regular hotkey (lower priority)
        auto act = std::make_shared<onyxui::action<Backend>>();
        act->set_shortcut('s', key_modifier::ctrl);
        act->triggered.connect([&last_triggered]() { last_triggered = 3; });
        manager.register_action(act);

        SUBCASE("Modifier-only takes precedence") {
            test_backend::test_keyboard_event ev;
            ev.pressed = true;
            ev.key_code = 0;
            ev.alt = true;

            manager.handle_key_event(ev);
            CHECK(last_triggered == 1);
        }

        SUBCASE("Chord takes precedence over regular hotkey") {
            test_backend::test_keyboard_event ev1;
            ev1.pressed = true;
            ev1.key_code = 'x';
            ev1.ctrl = true;

            test_backend::test_keyboard_event ev2;
            ev2.pressed = true;
            ev2.key_code = 'c';
            ev2.ctrl = true;

            manager.handle_key_event(ev1);
            manager.handle_key_event(ev2);
            CHECK(last_triggered == 2);
        }

        SUBCASE("Regular hotkey works when others don't match") {
            test_backend::test_keyboard_event ev;
            ev.pressed = true;
            ev.key_code = 's';
            ev.ctrl = true;

            manager.handle_key_event(ev);
            CHECK(last_triggered == 3);
        }
    }
}