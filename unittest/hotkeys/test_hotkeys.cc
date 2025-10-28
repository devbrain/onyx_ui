/**
 * @file test_hotkeys.cc
 * @brief Comprehensive tests for Phase 1 of hotkeys and mnemonics implementation
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Tests cover:
 * - key_sequence types and operations
 * - Backend event traits (to_ascii, to_f_key)
 * - Action keyboard shortcut support
 */

#include <doctest/doctest.h>
#include <memory>
#include <onyxui/hotkeys/key_sequence.hh>
#include <onyxui/concepts/event_like.hh>
#include <onyxui/widgets/action.hh>
#include <optional>
#include "utils/test_backend.hh"

using namespace onyxui;

// ======================================================================
// Test Suite: key_modifier Bitwise Operations
// ======================================================================

TEST_SUITE("key_modifier") {
    TEST_CASE("Bitwise OR combines modifiers") {
        auto result = key_modifier::ctrl | key_modifier::alt;
        CHECK((result & key_modifier::ctrl) != key_modifier::none);
        CHECK((result & key_modifier::alt) != key_modifier::none);
        CHECK((result & key_modifier::shift) == key_modifier::none);
    }

    TEST_CASE("Bitwise AND tests modifiers") {
        auto mods = key_modifier::ctrl | key_modifier::shift;
        CHECK((mods & key_modifier::ctrl) != key_modifier::none);
        CHECK((mods & key_modifier::shift) != key_modifier::none);
        CHECK((mods & key_modifier::alt) == key_modifier::none);
    }

    TEST_CASE("Bitwise XOR toggles modifiers") {
        auto mods = key_modifier::ctrl;
        mods = mods ^ key_modifier::alt;
        CHECK((mods & key_modifier::ctrl) != key_modifier::none);
        CHECK((mods & key_modifier::alt) != key_modifier::none);
    }

    TEST_CASE("Bitwise NOT inverts modifiers") {
        auto mods = ~key_modifier::none;
        CHECK((mods & key_modifier::ctrl) != key_modifier::none);
        CHECK((mods & key_modifier::alt) != key_modifier::none);
        CHECK((mods & key_modifier::shift) != key_modifier::none);
    }

    TEST_CASE("Compound assignment |=") {
        auto mods = key_modifier::ctrl;
        mods |= key_modifier::alt;
        CHECK((mods & key_modifier::ctrl) != key_modifier::none);
        CHECK((mods & key_modifier::alt) != key_modifier::none);
    }

    TEST_CASE("Compound assignment &=") {
        auto mods = key_modifier::ctrl | key_modifier::alt;
        mods &= key_modifier::ctrl;
        CHECK((mods & key_modifier::ctrl) != key_modifier::none);
        CHECK((mods & key_modifier::alt) == key_modifier::none);
    }

    TEST_CASE("Compound assignment ^=") {
        auto mods = key_modifier::ctrl;
        mods ^= key_modifier::ctrl;
        CHECK(mods == key_modifier::none);
    }

    TEST_CASE("Three-way combination") {
        auto mods = key_modifier::ctrl | key_modifier::alt | key_modifier::shift;
        CHECK((mods & key_modifier::ctrl) != key_modifier::none);
        CHECK((mods & key_modifier::alt) != key_modifier::none);
        CHECK((mods & key_modifier::shift) != key_modifier::none);
    }
}

// ======================================================================
// Test Suite: key_sequence Construction
// ======================================================================

TEST_SUITE("key_sequence::construction") {
    TEST_CASE("Default constructor creates empty sequence") {
        key_sequence const seq;
        CHECK(seq.empty());
        CHECK(static_cast<char>(seq.key) == '\0');
        CHECK(function_key_to_number(seq.key) == 0);
        CHECK(seq.modifiers == key_modifier::none);
    }

    TEST_CASE("ASCII key constructor normalizes to lowercase") {
        key_sequence const upper{'S', key_modifier::ctrl};
        key_sequence lower{'s', key_modifier::ctrl};
        CHECK(static_cast<char>(upper.key) == 's');
        CHECK(static_cast<char>(lower.key) == 's');
        CHECK(upper == lower);
    }

    TEST_CASE("ASCII key constructor with modifiers") {
        key_sequence const seq{'a', key_modifier::ctrl | key_modifier::shift};
        CHECK(static_cast<char>(seq.key) == 'a');
        CHECK(function_key_to_number(seq.key) == 0);
        CHECK((seq.modifiers & key_modifier::ctrl) != key_modifier::none);
        CHECK((seq.modifiers & key_modifier::shift) != key_modifier::none);
        CHECK(!seq.empty());
    }

    TEST_CASE("ASCII key constructor default modifiers") {
        key_sequence const seq{'x'};
        CHECK(static_cast<char>(seq.key) == 'x');
        CHECK(seq.modifiers == key_modifier::none);
        CHECK(!seq.empty());
    }

    TEST_CASE("F-key constructor") {
        key_sequence const seq{key_code::f9, key_modifier::alt};
        CHECK(function_key_to_number(seq.key) == 9);
        CHECK((seq.modifiers & key_modifier::alt) != key_modifier::none);
        CHECK(!seq.empty());
    }

    TEST_CASE("F-key constructor default modifiers") {
        key_sequence const seq{key_code::f1};
        CHECK(function_key_to_number(seq.key) == 1);
        CHECK(seq.modifiers == key_modifier::none);
        CHECK(!seq.empty());
    }

    TEST_CASE("Various ASCII characters are normalized") {
        CHECK(static_cast<char>(key_sequence{'A'}.key) == 'a');
        CHECK(static_cast<char>(key_sequence{'Z'}.key) == 'z');
        CHECK(static_cast<char>(key_sequence{'M'}.key) == 'm');
        CHECK(static_cast<char>(key_sequence{'0'}.key) == '0');  // Digits unchanged
        CHECK(static_cast<char>(key_sequence{'9'}.key) == '9');
        CHECK(static_cast<char>(key_sequence{'-'}.key) == '-');  // Punctuation unchanged
    }
}

// ======================================================================
// Test Suite: key_sequence Queries
// ======================================================================

TEST_SUITE("key_sequence::queries") {
    TEST_CASE("empty() returns true for default-constructed sequence") {
        key_sequence const seq;
        CHECK(seq.empty());
    }

    TEST_CASE("empty() returns false for ASCII key") {
        key_sequence const seq{'a'};
        CHECK(!seq.empty());
    }

    TEST_CASE("empty() returns false for F-key") {
        key_sequence const seq{1};
        CHECK(!seq.empty());
    }

    TEST_CASE("is_f_key() detects F-keys") {
        CHECK(key_sequence{key_code::f1}.is_f_key());
        CHECK(key_sequence{key_code::f12}.is_f_key());
        CHECK(!key_sequence{'a'}.is_f_key());
        CHECK(!key_sequence{}.is_f_key());
    }

    TEST_CASE("is_ascii_key() detects ASCII keys") {
        CHECK(key_sequence{'a'}.is_ascii_key());
        CHECK(key_sequence{'z'}.is_ascii_key());
        CHECK(key_sequence{'0'}.is_ascii_key());
        CHECK(!key_sequence{key_code::f1}.is_ascii_key());
        CHECK(!key_sequence{}.is_ascii_key());
    }

    TEST_CASE("has_ctrl() detects Ctrl modifier") {
        CHECK(key_sequence{'s', key_modifier::ctrl}.has_ctrl());
        CHECK(!key_sequence{'s', key_modifier::alt}.has_ctrl());
        CHECK(!key_sequence{'s'}.has_ctrl());
    }

    TEST_CASE("has_alt() detects Alt modifier") {
        CHECK(key_sequence{'q', key_modifier::alt}.has_alt());
        CHECK(!key_sequence{'q', key_modifier::ctrl}.has_alt());
        CHECK(!key_sequence{'q'}.has_alt());
    }

    TEST_CASE("has_shift() detects Shift modifier") {
        CHECK(key_sequence{'s', key_modifier::shift}.has_shift());
        CHECK(!key_sequence{'s', key_modifier::ctrl}.has_shift());
        CHECK(!key_sequence{'s'}.has_shift());
    }

    TEST_CASE("Multiple modifiers detected correctly") {
        key_sequence const seq{'s', key_modifier::ctrl | key_modifier::shift};
        CHECK(seq.has_ctrl());
        CHECK(seq.has_shift());
        CHECK(!seq.has_alt());
    }
}

// ======================================================================
// Test Suite: key_sequence Comparison
// ======================================================================

TEST_SUITE("key_sequence::comparison") {
    TEST_CASE("Equality comparison") {
        key_sequence const seq1{'s', key_modifier::ctrl};
        key_sequence seq2{'s', key_modifier::ctrl};
        key_sequence seq3{'s', key_modifier::alt};
        key_sequence seq4{'a', key_modifier::ctrl};

        CHECK(seq1 == seq2);
        CHECK(seq1 != seq3);  // Different modifier
        CHECK(seq1 != seq4);  // Different key
    }

    TEST_CASE("Case-insensitive equality") {
        key_sequence const upper{'S', key_modifier::ctrl};
        key_sequence lower{'s', key_modifier::ctrl};
        CHECK(upper == lower);
    }

    TEST_CASE("F-key equality") {
        key_sequence const f1a{1, key_modifier::none};
        key_sequence f1b{1, key_modifier::none};
        key_sequence f2{2, key_modifier::none};

        CHECK(f1a == f1b);
        CHECK(f1a != f2);
    }

    TEST_CASE("Ordering for std::map") {
        key_sequence const a{'a', key_modifier::ctrl};
        key_sequence b{'b', key_modifier::ctrl};
        key_sequence c{'a', key_modifier::alt};

        CHECK(a < b);  // Different keys
        CHECK(a != c); // Different modifiers
    }

    TEST_CASE("Empty sequences are equal") {
        key_sequence const empty1;
        key_sequence empty2;
        CHECK(empty1 == empty2);
    }
}

// ======================================================================
// Test Suite: key_sequence Helper Functions
// ======================================================================

TEST_SUITE("key_sequence::helpers") {
    TEST_CASE("make_ctrl_key creates Ctrl+key") {
        auto seq = make_ctrl_key('s');
        CHECK(static_cast<char>(seq.key) == 's');
        CHECK(seq.has_ctrl());
        CHECK(!seq.has_alt());
        CHECK(!seq.has_shift());
    }

    TEST_CASE("make_alt_key creates Alt+key") {
        auto seq = make_alt_key('q');
        CHECK(static_cast<char>(seq.key) == 'q');
        CHECK(!seq.has_ctrl());
        CHECK(seq.has_alt());
        CHECK(!seq.has_shift());
    }

    TEST_CASE("make_shift_key creates Shift+key") {
        auto seq = make_shift_key('t');
        CHECK(static_cast<char>(seq.key) == 't');
        CHECK(!seq.has_ctrl());
        CHECK(!seq.has_alt());
        CHECK(seq.has_shift());
    }

    TEST_CASE("make_f_key creates F-key") {
        auto seq = make_f_key(9);
        CHECK(function_key_to_number(seq.key) == 9);
        CHECK(seq.is_f_key());
        CHECK(seq.modifiers == key_modifier::none);
    }

    TEST_CASE("Helper functions normalize to lowercase") {
        CHECK(static_cast<char>(make_ctrl_key('S').key) == 's');
        CHECK(static_cast<char>(make_alt_key('Q').key) == 'q');
        CHECK(static_cast<char>(make_shift_key('T').key) == 't');
    }
}

// ======================================================================
// Test Suite: Backend Event Traits - to_ascii()
// ======================================================================

TEST_SUITE("event_traits::to_ascii") {
    using traits = event_traits<test_backend::test_keyboard_event>;

    TEST_CASE("Lowercase letters pass through") {
        for (char c = 'a'; c <= 'z'; ++c) {
            test_backend::test_keyboard_event event;
            event.key_code = c;
            CHECK(traits::to_ascii(event) == c);
        }
    }

    TEST_CASE("Uppercase letters convert to lowercase") {
        for (char c = 'A'; c <= 'Z'; ++c) {
            test_backend::test_keyboard_event event;
            event.key_code = c;
            char expected = static_cast<char>(c - 'A' + 'a');
            CHECK(traits::to_ascii(event) == expected);
        }
    }

    TEST_CASE("Digits pass through") {
        for (char c = '0'; c <= '9'; ++c) {
            test_backend::test_keyboard_event event;
            event.key_code = c;
            CHECK(traits::to_ascii(event) == c);
        }
    }

    TEST_CASE("Common punctuation passes through") {
        const char punctuation[] = " !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
        for (char c : punctuation) {
            if (c == '\0') break;
            test_backend::test_keyboard_event event;
            event.key_code = c;
            CHECK(traits::to_ascii(event) == c);
        }
    }

    TEST_CASE("Non-hotkey keys return null character") {
        test_backend::test_keyboard_event event;

        // Control characters ARE valid hotkeys (for semantic actions)
        event.key_code = traits::KEY_TAB;
        CHECK(traits::to_ascii(event) == '\t');  // Tab is a valid hotkey

        event.key_code = traits::KEY_ENTER;
        CHECK(traits::to_ascii(event) == '\n');  // Enter is a valid hotkey

        event.key_code = traits::KEY_ESCAPE;
        CHECK(traits::to_ascii(event) == 27);  // Escape is a valid hotkey

        // Arrow keys handled by to_special_key(), not to_ascii()
        event.key_code = 2000;  // KEY_UP
        CHECK(traits::to_ascii(event) == '\0');

        event.key_code = 9999;  // Arbitrary high value
        CHECK(traits::to_ascii(event) == '\0');
    }
}

// ======================================================================
// Test Suite: Backend Event Traits - to_f_key()
// ======================================================================

TEST_SUITE("event_traits::to_f_key") {
    using traits = event_traits<test_backend::test_keyboard_event>;

    TEST_CASE("F1-F12 keys convert correctly") {
        for (int i = 1; i <= 12; ++i) {
            test_backend::test_keyboard_event event;
            event.key_code = traits::KEY_F1 + (i - 1);
            CHECK(traits::to_f_key(event) == i);
        }
    }

    TEST_CASE("Non-F-keys return zero") {
        test_backend::test_keyboard_event event;

        event.key_code = 'a';
        CHECK(traits::to_f_key(event) == 0);

        event.key_code = '0';
        CHECK(traits::to_f_key(event) == 0);

        event.key_code = traits::KEY_ENTER;
        CHECK(traits::to_f_key(event) == 0);
    }

    TEST_CASE("Values outside F1-F12 range return zero") {
        test_backend::test_keyboard_event event;

        event.key_code = traits::KEY_F1 - 1;
        CHECK(traits::to_f_key(event) == 0);

        event.key_code = traits::KEY_F12 + 1;
        CHECK(traits::to_f_key(event) == 0);
    }
}

// ======================================================================
// Test Suite: Backend Event Traits - Modifier Keys
// ======================================================================

TEST_SUITE("event_traits::modifiers") {
    using traits = event_traits<test_backend::test_keyboard_event>;

    TEST_CASE("shift_pressed detects Shift") {
        test_backend::test_keyboard_event event;
        event.shift = true;
        CHECK(traits::shift_pressed(event));

        event.shift = false;
        CHECK(!traits::shift_pressed(event));
    }

    TEST_CASE("ctrl_pressed detects Ctrl") {
        test_backend::test_keyboard_event event;
        event.ctrl = true;
        CHECK(traits::ctrl_pressed(event));

        event.ctrl = false;
        CHECK(!traits::ctrl_pressed(event));
    }

    TEST_CASE("alt_pressed detects Alt") {
        test_backend::test_keyboard_event event;
        event.alt = true;
        CHECK(traits::alt_pressed(event));

        event.alt = false;
        CHECK(!traits::alt_pressed(event));
    }

    TEST_CASE("Multiple modifiers") {
        test_backend::test_keyboard_event event;
        event.ctrl = true;
        event.shift = true;
        event.alt = false;

        CHECK(traits::ctrl_pressed(event));
        CHECK(traits::shift_pressed(event));
        CHECK(!traits::alt_pressed(event));
    }
}

// ======================================================================
// Test Suite: Backend Event Traits - Key Recognition
// ======================================================================

TEST_SUITE("event_traits::key_recognition") {
    using traits = event_traits<test_backend::test_keyboard_event>;

    TEST_CASE("is_tab_key detects Tab") {
        test_backend::test_keyboard_event event;
        event.key_code = traits::KEY_TAB;
        CHECK(traits::is_tab_key(event));

        event.key_code = 'a';
        CHECK(!traits::is_tab_key(event));
    }

    TEST_CASE("is_enter_key detects Enter") {
        test_backend::test_keyboard_event event;
        event.key_code = traits::KEY_ENTER;
        CHECK(traits::is_enter_key(event));

        event.key_code = 'b';
        CHECK(!traits::is_enter_key(event));
    }

    TEST_CASE("is_space_key detects Space") {
        test_backend::test_keyboard_event event;
        event.key_code = traits::KEY_SPACE;
        CHECK(traits::is_space_key(event));

        event.key_code = 'c';
        CHECK(!traits::is_space_key(event));
    }

    TEST_CASE("is_escape_key detects Escape") {
        test_backend::test_keyboard_event event;
        event.key_code = traits::KEY_ESCAPE;
        CHECK(traits::is_escape_key(event));

        event.key_code = 'd';
        CHECK(!traits::is_escape_key(event));
    }

    TEST_CASE("is_key_press detects key press") {
        test_backend::test_keyboard_event event;
        event.pressed = true;
        CHECK(traits::is_key_press(event));

        event.pressed = false;
        CHECK(!traits::is_key_press(event));
    }
}

// ======================================================================
// Test Suite: Action Shortcuts - Basic Operations
// ======================================================================

TEST_SUITE("action::shortcuts::basic") {
    TEST_CASE("Action has no shortcut by default") {
        auto act = std::make_shared<action<test_backend>>();
        CHECK(!act->has_shortcut());
        CHECK(!act->shortcut().has_value());
    }

    TEST_CASE("set_shortcut with key_sequence") {
        auto act = std::make_shared<action<test_backend>>();
        key_sequence seq{'s', key_modifier::ctrl};

        act->set_shortcut(seq);

        REQUIRE(act->has_shortcut());
        CHECK((*act->shortcut()) == seq);
    }

    TEST_CASE("set_shortcut with char and modifiers") {
        auto act = std::make_shared<action<test_backend>>();

        act->set_shortcut('q', key_modifier::alt);

        REQUIRE(act->has_shortcut());
        auto shortcut = (*act->shortcut());
        CHECK(static_cast<char>(shortcut.key) == 'q');
        CHECK(shortcut.has_alt());
    }

    TEST_CASE("set_shortcut with char only (no modifiers)") {
        auto act = std::make_shared<action<test_backend>>();

        act->set_shortcut('h');

        REQUIRE(act->has_shortcut());
        auto shortcut = (*act->shortcut());
        CHECK(static_cast<char>(shortcut.key) == 'h');
        CHECK(shortcut.modifiers == key_modifier::none);
    }

    TEST_CASE("set_shortcut_f with F-key") {
        auto act = std::make_shared<action<test_backend>>();

        act->set_shortcut_f(9);

        REQUIRE(act->has_shortcut());
        auto shortcut = (*act->shortcut());
        CHECK(function_key_to_number(shortcut.key) == 9);
        CHECK(shortcut.is_f_key());
    }

    TEST_CASE("set_shortcut_f with F-key and modifiers") {
        auto act = std::make_shared<action<test_backend>>();

        act->set_shortcut_f(4, key_modifier::alt);

        REQUIRE(act->has_shortcut());
        auto shortcut = (*act->shortcut());
        CHECK(function_key_to_number(shortcut.key) == 4);
        CHECK(shortcut.has_alt());
    }

    TEST_CASE("clear_shortcut removes shortcut") {
        auto act = std::make_shared<action<test_backend>>();
        act->set_shortcut('s', key_modifier::ctrl);

        REQUIRE(act->has_shortcut());

        act->clear_shortcut();

        CHECK(!act->has_shortcut());
        CHECK(!act->shortcut().has_value());
    }

    TEST_CASE("Overwriting shortcut replaces old one") {
        auto act = std::make_shared<action<test_backend>>();
        act->set_shortcut('s', key_modifier::ctrl);

        act->set_shortcut('q', key_modifier::alt);

        REQUIRE(act->has_shortcut());
        auto shortcut = (*act->shortcut());
        CHECK(static_cast<char>(shortcut.key) == 'q');
        CHECK(shortcut.has_alt());
        CHECK(!shortcut.has_ctrl());
    }
}

// ======================================================================
// Test Suite: Action Shortcuts - Signal Emission
// ======================================================================

TEST_SUITE("action::shortcuts::signals") {
    TEST_CASE("shortcut_changed emitted when setting shortcut") {
        auto act = std::make_shared<action<test_backend>>();

        bool signal_emitted = false;
        std::optional<key_sequence> received_shortcut;

        act->shortcut_changed.connect([&](const std::optional<key_sequence>& shortcut) {
            signal_emitted = true;
            received_shortcut = shortcut;
        });

        act->set_shortcut('s', key_modifier::ctrl);

        CHECK(signal_emitted);
        REQUIRE(received_shortcut.has_value());
        CHECK(static_cast<char>(received_shortcut.value().key) == 's');
        CHECK(received_shortcut.value().has_ctrl());
    }

    TEST_CASE("shortcut_changed emitted when clearing shortcut") {
        auto act = std::make_shared<action<test_backend>>();
        act->set_shortcut('s', key_modifier::ctrl);

        bool signal_emitted = false;
        std::optional<key_sequence> received_shortcut;
        received_shortcut = key_sequence{'x'};  // Set to non-empty initially

        act->shortcut_changed.connect([&](const std::optional<key_sequence>& shortcut) {
            signal_emitted = true;
            received_shortcut = shortcut;
        });

        act->clear_shortcut();

        CHECK(signal_emitted);
        CHECK(!received_shortcut.has_value());
    }

    TEST_CASE("shortcut_changed emitted when overwriting shortcut") {
        auto act = std::make_shared<action<test_backend>>();
        act->set_shortcut('s', key_modifier::ctrl);

        int signal_count = 0;
        act->shortcut_changed.connect([&](const std::optional<key_sequence>&) {
            signal_count++;
        });

        act->set_shortcut('q', key_modifier::alt);

        CHECK(signal_count == 1);
    }

    TEST_CASE("Multiple listeners receive shortcut_changed") {
        auto act = std::make_shared<action<test_backend>>();

        int listener1_count = 0;
        int listener2_count = 0;

        act->shortcut_changed.connect([&](const std::optional<key_sequence>&) {
            listener1_count++;
        });

        act->shortcut_changed.connect([&](const std::optional<key_sequence>&) {
            listener2_count++;
        });

        act->set_shortcut('s', key_modifier::ctrl);

        CHECK(listener1_count == 1);
        CHECK(listener2_count == 1);
    }
}

// ======================================================================
// Test Suite: Action Shortcuts - Real-World Scenarios
// ======================================================================

TEST_SUITE("action::shortcuts::scenarios") {
    TEST_CASE("File menu shortcuts") {
        auto save_action = std::make_shared<action<test_backend>>();
        save_action->set_text("Save");
        save_action->set_shortcut('s', key_modifier::ctrl);

        auto save_as_action = std::make_shared<action<test_backend>>();
        save_as_action->set_text("Save As...");
        save_as_action->set_shortcut('s', key_modifier::ctrl | key_modifier::shift);

        auto quit_action = std::make_shared<action<test_backend>>();
        quit_action->set_text("Quit");
        quit_action->set_shortcut('q', key_modifier::alt);

        REQUIRE(save_action->has_shortcut());
        REQUIRE(save_as_action->has_shortcut());
        REQUIRE(quit_action->has_shortcut());

        CHECK(static_cast<char>((*save_action->shortcut()).key) == 's');
        CHECK((*save_as_action->shortcut()).has_shift());
        CHECK((*quit_action->shortcut()).has_alt());
    }

    TEST_CASE("Function key shortcuts (Norton Commander style)") {
        auto help_action = std::make_shared<action<test_backend>>();
        help_action->set_text("Help");
        help_action->set_shortcut_f(1);  // F1

        auto rename_action = std::make_shared<action<test_backend>>();
        rename_action->set_text("Rename");
        rename_action->set_shortcut_f(6);  // F6

        auto menu_action = std::make_shared<action<test_backend>>();
        menu_action->set_text("Menu");
        menu_action->set_shortcut_f(9);  // F9

        CHECK(function_key_to_number((*help_action->shortcut()).key) == 1);
        CHECK(function_key_to_number((*rename_action->shortcut()).key) == 6);
        CHECK(function_key_to_number((*menu_action->shortcut()).key) == 9);
    }

    TEST_CASE("Shortcuts can be dynamically changed") {
        auto act = std::make_shared<action<test_backend>>();

        // Initial shortcut
        act->set_shortcut('n', key_modifier::ctrl);
        CHECK(static_cast<char>((*act->shortcut()).key) == 'n');

        // User changes shortcut preference
        act->set_shortcut('m', key_modifier::ctrl);
        CHECK(static_cast<char>((*act->shortcut()).key) == 'm');

        // User disables shortcut
        act->clear_shortcut();
        CHECK(!act->has_shortcut());
    }

    TEST_CASE("Action with both text and shortcut") {
        auto copy_action = std::make_shared<action<test_backend>>();
        copy_action->set_text("Copy");
        copy_action->set_shortcut('c', key_modifier::ctrl);

        CHECK(copy_action->text() == "Copy");
        REQUIRE(copy_action->has_shortcut());
        CHECK(static_cast<char>((*copy_action->shortcut()).key) == 'c');
        CHECK((*copy_action->shortcut()).has_ctrl());
    }

    TEST_CASE("Disabled action retains shortcut") {
        auto act = std::make_shared<action<test_backend>>();
        act->set_shortcut('p', key_modifier::ctrl);
        act->set_enabled(false);

        CHECK(!act->is_enabled());
        CHECK(act->has_shortcut());
        CHECK(static_cast<char>((*act->shortcut()).key) == 'p');
    }
}

// ======================================================================
// Test Suite: HotkeyCapable Concept Validation
// ======================================================================

TEST_SUITE("concepts::hotkey_capable") {
    TEST_CASE("test_keyboard_event satisfies HotkeyCapable concept") {
        // This is a compile-time check - if it compiles, the concept is satisfied
        static_assert(HotkeyCapable<test_backend::test_keyboard_event>,
                      "test_keyboard_event should satisfy HotkeyCapable concept");

        // Runtime verification
        test_backend::test_keyboard_event event;
        event.key_code = 's';
        event.pressed = true;
        event.ctrl = true;

        using traits = event_traits<test_backend::test_keyboard_event>;
        CHECK(traits::to_ascii(event) == 's');
        CHECK(traits::to_f_key(event) == 0);
        CHECK(traits::is_key_press(event));
        CHECK(traits::ctrl_pressed(event));
    }

    TEST_CASE("test_keyboard_event satisfies KeyboardEvent concept") {
        static_assert(KeyboardEvent<test_backend::test_keyboard_event>,
                      "test_keyboard_event should satisfy KeyboardEvent concept");
    }

    TEST_CASE("test_keyboard_event satisfies ModifierState concept") {
        static_assert(ModifierState<test_backend::test_keyboard_event>,
                      "test_keyboard_event should satisfy ModifierState concept");
    }
}
