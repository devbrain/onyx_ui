/**
 * @file test_hotkey_scheme.cc
 * @brief Tests for hotkey scheme structures and string parsing
 * @author Claude Code
 * @date 2025-10-26
 */

#include <doctest/doctest.h>
#include <onyxui/hotkeys/hotkey_scheme.hh>
#include <onyxui/hotkeys/hotkey_scheme_registry.hh>
#include <onyxui/hotkeys/builtin_hotkey_schemes.hh>
#include <onyxui/hotkeys/key_sequence.hh>
#include <vector>

using namespace onyxui;

TEST_SUITE("Hotkey Scheme") {

    TEST_CASE("key_sequence - String parsing") {
        SUBCASE("Single character") {
            auto seq = parse_key_sequence("s");
            CHECK(static_cast<char>(seq.key) == 's');
            CHECK(function_key_to_number(seq.key) == 0);
            CHECK(seq.modifiers == key_modifier::none);
        }

        SUBCASE("Ctrl+character") {
            auto seq = parse_key_sequence("Ctrl+S");
            CHECK(static_cast<char>(seq.key) == 's');  // Normalized to lowercase
            CHECK((seq.modifiers & key_modifier::ctrl) != key_modifier::none);
        }

        SUBCASE("Alt+character") {
            auto seq = parse_key_sequence("Alt+Q");
            CHECK(static_cast<char>(seq.key) == 'q');
            CHECK((seq.modifiers & key_modifier::alt) != key_modifier::none);
        }

        SUBCASE("Shift+character") {
            auto seq = parse_key_sequence("Shift+A");
            CHECK(static_cast<char>(seq.key) == 'a');
            CHECK((seq.modifiers & key_modifier::shift) != key_modifier::none);
        }

        SUBCASE("Multiple modifiers") {
            auto seq = parse_key_sequence("Ctrl+Shift+S");
            CHECK(static_cast<char>(seq.key) == 's');
            CHECK((seq.modifiers & key_modifier::ctrl) != key_modifier::none);
            CHECK((seq.modifiers & key_modifier::shift) != key_modifier::none);
        }

        SUBCASE("F-keys") {
            auto f1 = parse_key_sequence("F1");
            CHECK(function_key_to_number(f1.key) == 1);
            CHECK(is_function_key(f1.key));

            auto f10 = parse_key_sequence("F10");
            CHECK(function_key_to_number(f10.key) == 10);

            auto f12 = parse_key_sequence("F12");
            CHECK(function_key_to_number(f12.key) == 12);
        }

        SUBCASE("F-keys with modifiers") {
            auto seq = parse_key_sequence("Alt+F4");
            CHECK(function_key_to_number(seq.key) == 4);
            CHECK((seq.modifiers & key_modifier::alt) != key_modifier::none);
        }

        SUBCASE("Special keys") {
            auto down = parse_key_sequence("Down");
            CHECK(down.key == key_code::arrow_down);  // Down arrow code

            auto up = parse_key_sequence("Up");
            CHECK(up.key == key_code::arrow_up);  // Up arrow code

            auto left = parse_key_sequence("Left");
            CHECK(left.key == key_code::arrow_left);

            auto right = parse_key_sequence("Right");
            CHECK(right.key == key_code::arrow_right);

            auto enter = parse_key_sequence("Enter");
            CHECK(static_cast<char>(enter.key) == '\n');

            auto esc = parse_key_sequence("Escape");
            CHECK(static_cast<int>(esc.key) == 27);

            auto tab = parse_key_sequence("Tab");
            CHECK(static_cast<char>(tab.key) == '\t');
        }

        SUBCASE("Special keys with modifiers") {
            auto seq = parse_key_sequence("Shift+Tab");
            CHECK(static_cast<char>(seq.key) == '\t');
            CHECK((seq.modifiers & key_modifier::shift) != key_modifier::none);
        }

        SUBCASE("Invalid sequences throw") {
            CHECK_THROWS_AS(parse_key_sequence(""), std::runtime_error);
            CHECK_THROWS_AS(parse_key_sequence("Ctrl+"), std::runtime_error);
            CHECK_THROWS_AS(parse_key_sequence("F0"), std::runtime_error);
            CHECK_THROWS_AS(parse_key_sequence("F13"), std::runtime_error);
            CHECK_THROWS_AS(parse_key_sequence("F99"), std::runtime_error);
            CHECK_THROWS_AS(parse_key_sequence("InvalidKey"), std::runtime_error);
        }
    }

    TEST_CASE("key_sequence - String formatting") {
        SUBCASE("Format single character") {
            key_sequence seq{'a'};
            CHECK(format_key_sequence(seq) == "A");
        }

        SUBCASE("Format Ctrl+S") {
            key_sequence seq{'s', key_modifier::ctrl};
            CHECK(format_key_sequence(seq) == "Ctrl+S");
        }

        SUBCASE("Format Alt+Q") {
            key_sequence seq{'q', key_modifier::alt};
            CHECK(format_key_sequence(seq) == "Alt+Q");
        }

        SUBCASE("Format Shift+A") {
            key_sequence seq{'a', key_modifier::shift};
            CHECK(format_key_sequence(seq) == "Shift+A");
        }

        SUBCASE("Format multiple modifiers") {
            key_sequence seq{'s', key_modifier::ctrl | key_modifier::shift};
            CHECK(format_key_sequence(seq) == "Ctrl+Shift+S");
        }

        SUBCASE("Format F-keys") {
            key_sequence f1{key_code::f1};
            CHECK(format_key_sequence(f1) == "F1");

            key_sequence f10{key_code::f10};
            CHECK(format_key_sequence(f10) == "F10");

            key_sequence f12{key_code::f12};
            CHECK(format_key_sequence(f12) == "F12");
        }

        SUBCASE("Format F-key with modifier") {
            key_sequence seq{key_code::f4, key_modifier::alt};
            CHECK(format_key_sequence(seq) == "Alt+F4");
        }

        SUBCASE("Format special keys") {
            key_sequence tab{'\t'};
            CHECK(format_key_sequence(tab) == "Tab");

            key_sequence enter{'\n'};
            CHECK(format_key_sequence(enter) == "Enter");

            key_sequence esc{static_cast<char>(27)};
            CHECK(format_key_sequence(esc) == "Escape");
        }

        SUBCASE("Format Shift+Tab") {
            key_sequence seq{'\t', key_modifier::shift};
            CHECK(format_key_sequence(seq) == "Shift+Tab");
        }
    }

    TEST_CASE("key_sequence - Round-trip parsing") {
        std::vector<std::string> test_cases = {
            "Ctrl+S", "Alt+F4", "Shift+Tab", "F10", "Down", "Enter", "Escape",
            "F1", "F12", "Ctrl+Shift+A", "Up", "Left", "Right", "Tab"
        };

        for (const auto& original : test_cases) {
            auto parsed = parse_key_sequence(original);
            auto formatted = format_key_sequence(parsed);
            INFO("Round-trip: ", original, " -> ", formatted);
            CHECK(formatted == original);
        }
    }

    TEST_CASE("hotkey_scheme - Basic operations") {
        hotkey_scheme scheme;
        scheme.name = "Test Scheme";
        scheme.description = "Test description";

        SUBCASE("Empty scheme has no bindings") {
            CHECK_FALSE(scheme.has_binding(hotkey_action::activate_menu_bar));
            CHECK_FALSE(scheme.get_binding(hotkey_action::menu_down).has_value());
            CHECK(scheme.binding_count() == 0);
        }

        SUBCASE("Set and get binding") {
            auto f10 = parse_key_sequence("F10");
            scheme.set_binding(hotkey_action::activate_menu_bar, f10);

            CHECK(scheme.has_binding(hotkey_action::activate_menu_bar));
            CHECK(scheme.binding_count() == 1);

            auto retrieved = scheme.get_binding(hotkey_action::activate_menu_bar);
            REQUIRE(retrieved.has_value());
            CHECK(*retrieved == f10);
        }

        SUBCASE("Find action by key") {
            scheme.set_binding(hotkey_action::menu_down, parse_key_sequence("Down"));
            scheme.set_binding(hotkey_action::menu_up, parse_key_sequence("Up"));

            auto down_action = scheme.find_action_for_key(parse_key_sequence("Down"));
            REQUIRE(down_action.has_value());
            CHECK(*down_action == hotkey_action::menu_down);

            auto up_action = scheme.find_action_for_key(parse_key_sequence("Up"));
            REQUIRE(up_action.has_value());
            CHECK(*up_action == hotkey_action::menu_up);

            // Non-existent key
            auto none = scheme.find_action_for_key(parse_key_sequence("F5"));
            CHECK_FALSE(none.has_value());
        }

        SUBCASE("Remove binding") {
            scheme.set_binding(hotkey_action::menu_down, parse_key_sequence("Down"));
            REQUIRE(scheme.has_binding(hotkey_action::menu_down));
            REQUIRE(scheme.binding_count() == 1);

            scheme.remove_binding(hotkey_action::menu_down);
            CHECK_FALSE(scheme.has_binding(hotkey_action::menu_down));
            CHECK(scheme.binding_count() == 0);
        }

        SUBCASE("Remove non-existent binding (safe)") {
            scheme.remove_binding(hotkey_action::menu_cancel);  // Should not crash
            CHECK_FALSE(scheme.has_binding(hotkey_action::menu_cancel));
        }

        SUBCASE("Multiple bindings") {
            scheme.set_binding(hotkey_action::activate_menu_bar, parse_key_sequence("F10"));
            scheme.set_binding(hotkey_action::menu_down, parse_key_sequence("Down"));
            scheme.set_binding(hotkey_action::menu_up, parse_key_sequence("Up"));
            scheme.set_binding(hotkey_action::menu_select, parse_key_sequence("Enter"));
            scheme.set_binding(hotkey_action::menu_cancel, parse_key_sequence("Escape"));

            CHECK(scheme.binding_count() == 5);
        }

        SUBCASE("Overwrite existing binding") {
            scheme.set_binding(hotkey_action::activate_menu_bar, parse_key_sequence("F10"));

            auto retrieved1 = scheme.get_binding(hotkey_action::activate_menu_bar);
            REQUIRE(retrieved1.has_value());
            CHECK(function_key_to_number(retrieved1->key) == 10);

            // Overwrite with F9
            scheme.set_binding(hotkey_action::activate_menu_bar, parse_key_sequence("F9"));

            auto retrieved2 = scheme.get_binding(hotkey_action::activate_menu_bar);
            REQUIRE(retrieved2.has_value());
            CHECK(function_key_to_number(retrieved2->key) == 9);

            CHECK(scheme.binding_count() == 1);  // Still only one binding
        }

        SUBCASE("Clear all bindings") {
            scheme.set_binding(hotkey_action::activate_menu_bar, parse_key_sequence("F10"));
            scheme.set_binding(hotkey_action::menu_down, parse_key_sequence("Down"));
            REQUIRE(scheme.binding_count() == 2);

            scheme.clear();
            CHECK(scheme.binding_count() == 0);
            CHECK_FALSE(scheme.has_binding(hotkey_action::activate_menu_bar));
            CHECK_FALSE(scheme.has_binding(hotkey_action::menu_down));
        }
    }

    TEST_CASE("hotkey_scheme - Query non-existent binding") {
        hotkey_scheme scheme;
        scheme.name = "Minimal";

        // Query action with no binding (should return nullopt, NOT error)
        auto binding = scheme.get_binding(hotkey_action::activate_menu_bar);
        CHECK_FALSE(binding.has_value());

        // This is valid - widgets should fall back to mouse
        INFO("Missing binding is NOT an error - mouse fallback is expected");
    }

    TEST_CASE("hotkey_scheme_registry - Basic operations") {
        hotkey_scheme_registry registry;

        SUBCASE("Empty registry") {
            CHECK(registry.scheme_count() == 0);
            CHECK_FALSE(registry.has_scheme("Windows"));
            CHECK(registry.get_scheme("Windows") == nullptr);
            CHECK(registry.get_current_scheme() == nullptr);
            CHECK_FALSE(registry.get_current_scheme_name().has_value());
        }

        SUBCASE("Register single scheme") {
            hotkey_scheme scheme;
            scheme.name = "Test Scheme";
            scheme.description = "Test description";

            registry.register_scheme(std::move(scheme));

            CHECK(registry.scheme_count() == 1);
            CHECK(registry.has_scheme("Test Scheme"));

            auto* retrieved = registry.get_scheme("Test Scheme");
            REQUIRE(retrieved != nullptr);
            CHECK(retrieved->name == "Test Scheme");
            CHECK(retrieved->description == "Test description");
        }

        SUBCASE("First registered is default") {
            hotkey_scheme scheme1;
            scheme1.name = "First";
            registry.register_scheme(std::move(scheme1));

            hotkey_scheme scheme2;
            scheme2.name = "Second";
            registry.register_scheme(std::move(scheme2));

            CHECK(registry.scheme_count() == 2);

            // First registered should be current
            auto* current = registry.get_current_scheme();
            REQUIRE(current != nullptr);
            CHECK(current->name == "First");

            auto current_name = registry.get_current_scheme_name();
            REQUIRE(current_name.has_value());
            CHECK(*current_name == "First");
        }

        SUBCASE("Set current scheme") {
            hotkey_scheme scheme1;
            scheme1.name = "Windows";
            registry.register_scheme(std::move(scheme1));

            hotkey_scheme scheme2;
            scheme2.name = "Norton Commander";
            registry.register_scheme(std::move(scheme2));

            // Switch to second scheme
            CHECK(registry.set_current_scheme("Norton Commander"));

            auto* current = registry.get_current_scheme();
            REQUIRE(current != nullptr);
            CHECK(current->name == "Norton Commander");

            // Try to set non-existent scheme
            CHECK_FALSE(registry.set_current_scheme("Invalid"));

            // Current should remain unchanged
            current = registry.get_current_scheme();
            REQUIRE(current != nullptr);
            CHECK(current->name == "Norton Commander");
        }

        SUBCASE("Get scheme names") {
            hotkey_scheme scheme1;
            scheme1.name = "Windows";
            registry.register_scheme(std::move(scheme1));

            hotkey_scheme scheme2;
            scheme2.name = "Norton Commander";
            registry.register_scheme(std::move(scheme2));

            auto names = registry.get_scheme_names();
            CHECK(names.size() == 2);

            // Names should include both schemes (order not guaranteed)
            bool has_windows = false;
            bool has_norton = false;
            for (const auto& name : names) {
                if (name == "Windows") has_windows = true;
                if (name == "Norton Commander") has_norton = true;
            }
            CHECK(has_windows);
            CHECK(has_norton);
        }

        SUBCASE("Remove scheme") {
            hotkey_scheme scheme;
            scheme.name = "Test";
            registry.register_scheme(std::move(scheme));

            REQUIRE(registry.has_scheme("Test"));
            CHECK(registry.remove_scheme("Test"));
            CHECK_FALSE(registry.has_scheme("Test"));
            CHECK(registry.scheme_count() == 0);

            // Remove non-existent scheme
            CHECK_FALSE(registry.remove_scheme("Invalid"));
        }

        SUBCASE("Remove current scheme") {
            hotkey_scheme scheme1;
            scheme1.name = "First";
            registry.register_scheme(std::move(scheme1));

            hotkey_scheme scheme2;
            scheme2.name = "Second";
            registry.register_scheme(std::move(scheme2));

            // Current is "First"
            REQUIRE(registry.get_current_scheme()->name == "First");

            // Remove current scheme
            CHECK(registry.remove_scheme("First"));
            CHECK(registry.get_current_scheme() == nullptr);
            CHECK_FALSE(registry.get_current_scheme_name().has_value());
        }

        SUBCASE("Clear registry") {
            hotkey_scheme scheme1;
            scheme1.name = "First";
            registry.register_scheme(std::move(scheme1));

            hotkey_scheme scheme2;
            scheme2.name = "Second";
            registry.register_scheme(std::move(scheme2));

            REQUIRE(registry.scheme_count() == 2);

            registry.clear();
            CHECK(registry.scheme_count() == 0);
            CHECK(registry.get_current_scheme() == nullptr);
        }

        SUBCASE("Replace existing scheme") {
            hotkey_scheme scheme1;
            scheme1.name = "Test";
            scheme1.description = "First version";
            scheme1.set_binding(hotkey_action::activate_menu_bar, parse_key_sequence("F10"));
            registry.register_scheme(std::move(scheme1));

            // Replace with same name
            hotkey_scheme scheme2;
            scheme2.name = "Test";
            scheme2.description = "Second version";
            scheme2.set_binding(hotkey_action::activate_menu_bar, parse_key_sequence("F9"));
            registry.register_scheme(std::move(scheme2));

            // Should still have only one scheme
            CHECK(registry.scheme_count() == 1);

            // Should have updated version
            auto* retrieved = registry.get_scheme("Test");
            REQUIRE(retrieved != nullptr);
            CHECK(retrieved->description == "Second version");

            auto binding = retrieved->get_binding(hotkey_action::activate_menu_bar);
            REQUIRE(binding.has_value());
            CHECK(function_key_to_number(binding->key) == 9);
        }
    }

    TEST_CASE("builtin_hotkey_schemes - Windows scheme") {
        auto scheme = builtin_hotkey_schemes::windows();

        CHECK(scheme.name == "Windows");
        CHECK(scheme.description == "Standard Windows keyboard shortcuts (F10 for menu)");

        // Check F10 for menu activation
        auto menu_key = scheme.get_binding(hotkey_action::activate_menu_bar);
        REQUIRE(menu_key.has_value());
        CHECK(function_key_to_number(menu_key->key) == 10);

        // Check all expected bindings
        CHECK(scheme.has_binding(hotkey_action::activate_menu_bar));
        CHECK(scheme.has_binding(hotkey_action::menu_up));
        CHECK(scheme.has_binding(hotkey_action::menu_down));
        CHECK(scheme.has_binding(hotkey_action::menu_left));
        CHECK(scheme.has_binding(hotkey_action::menu_right));
        CHECK(scheme.has_binding(hotkey_action::menu_select));
        CHECK(scheme.has_binding(hotkey_action::menu_cancel));
        CHECK(scheme.has_binding(hotkey_action::focus_next));
        CHECK(scheme.has_binding(hotkey_action::focus_previous));
        CHECK(scheme.has_binding(hotkey_action::activate_focused));

        // 10 original bindings + 6 scrolling actions (scroll_up, scroll_down, scroll_page_up, scroll_page_down, scroll_home, scroll_end)
        CHECK(scheme.binding_count() == 16);
    }

    TEST_CASE("builtin_hotkey_schemes - Norton Commander scheme") {
        auto scheme = builtin_hotkey_schemes::norton_commander();

        CHECK(scheme.name == "Norton Commander");
        CHECK(scheme.description == "Classic DOS Norton Commander shortcuts (F9 for menu)");

        // Check F9 for menu activation (main difference from Windows)
        auto menu_key = scheme.get_binding(hotkey_action::activate_menu_bar);
        REQUIRE(menu_key.has_value());
        CHECK(function_key_to_number(menu_key->key) == 9);

        // Should have same number of bindings as Windows scheme
        // 10 original bindings + 6 scrolling actions
        CHECK(scheme.binding_count() == 16);
    }
}
