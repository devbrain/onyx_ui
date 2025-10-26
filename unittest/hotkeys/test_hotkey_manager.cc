/**
 * @file test_hotkey_manager.cc
 * @brief Comprehensive tests for Phase 3 - Hotkey System
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Tests cover:
 * - Hotkey registration and lookup
 * - Event capture and dispatching
 * - Focus-aware scoping
 * - Conflict detection
 * - Cleanup of expired registrations
 */

#include <doctest/doctest.h>
#include <memory>
#include <onyxui/hotkeys/hotkey_manager.hh>
#include <onyxui/hotkeys/hotkey_scheme_registry.hh>
#include <onyxui/hotkeys/builtin_hotkey_schemes.hh>
#include <onyxui/widgets/action.hh>
#include <onyxui/widgets/panel.hh>
#include "onyxui/hotkeys/key_sequence.hh"
#include <utility>
#include "utils/test_backend.hh"

using namespace onyxui;

// ======================================================================
// Test Suite: hotkey_manager - Basic Registration
// ======================================================================

TEST_SUITE("hotkey_manager::registration") {
    using Backend = test_backend;

    TEST_CASE("Register action with shortcut") {
        hotkey_manager<Backend> manager;

        auto save_action = std::make_shared<action<Backend>>();
        save_action->set_shortcut('s', key_modifier::ctrl);

        CHECK(manager.register_action(save_action));
    }

    TEST_CASE("Cannot register action without shortcut") {
        hotkey_manager<Backend> manager;

        auto act = std::make_shared<action<Backend>>();
        // No shortcut set

        CHECK(!manager.register_action(act));
    }

    TEST_CASE("is_registered detects registered hotkeys") {
        hotkey_manager<Backend> manager;

        auto act = std::make_shared<action<Backend>>();
        act->set_shortcut('s', key_modifier::ctrl);

        manager.register_action(act);

        key_sequence const seq{'s', key_modifier::ctrl};
        CHECK(manager.is_registered(seq));
    }

    TEST_CASE("is_registered returns false for unregistered hotkeys") {
        hotkey_manager<Backend> const manager;

        key_sequence const seq{'x', key_modifier::ctrl};
        CHECK(!manager.is_registered(seq));
    }

    TEST_CASE("Unregister removes hotkey") {
        hotkey_manager<Backend> manager;

        auto act = std::make_shared<action<Backend>>();
        act->set_shortcut('s', key_modifier::ctrl);

        manager.register_action(act);
        manager.unregister_action(act);

        key_sequence const seq{'s', key_modifier::ctrl};
        CHECK(!manager.is_registered(seq));
    }

    TEST_CASE("Unregister non-existent action is safe") {
        hotkey_manager<Backend> manager;

        auto act = std::make_shared<action<Backend>>();
        act->set_shortcut('s', key_modifier::ctrl);

        // Never registered
        manager.unregister_action(act);  // Should not crash
    }
}

// ======================================================================
// Test Suite: hotkey_manager - Event Handling
// ======================================================================

TEST_SUITE("hotkey_manager::event_handling") {
    using Backend = test_backend;

    TEST_CASE("Keyboard event triggers action") {
        hotkey_manager<Backend> manager;

        auto act = std::make_shared<action<Backend>>();
        act->set_shortcut('s', key_modifier::ctrl);

        bool triggered = false;
        act->triggered.connect([&]() { triggered = true; });

        manager.register_action(act);

        // Create keyboard event
        test_backend::test_keyboard_event event;
        event.key_code = 's';
        event.pressed = true;
        event.ctrl = true;

        CHECK(manager.handle_key_event(event));
        CHECK(triggered);
    }

    TEST_CASE("Event without matching hotkey returns false") {
        hotkey_manager<Backend> manager;

        auto act = std::make_shared<action<Backend>>();
        act->set_shortcut('s', key_modifier::ctrl);
        manager.register_action(act);

        test_backend::test_keyboard_event event;
        event.key_code = 'x';  // Different key
        event.pressed = true;
        event.ctrl = true;

        CHECK(!manager.handle_key_event(event));
    }

    TEST_CASE("Key release does not trigger hotkey") {
        hotkey_manager<Backend> manager;

        auto act = std::make_shared<action<Backend>>();
        act->set_shortcut('s', key_modifier::ctrl);

        bool triggered = false;
        act->triggered.connect([&]() { triggered = true; });

        manager.register_action(act);

        test_backend::test_keyboard_event event;
        event.key_code = 's';
        event.pressed = false;  // Release, not press
        event.ctrl = true;

        CHECK(!manager.handle_key_event(event));
        CHECK(!triggered);
    }

    TEST_CASE("Modifier keys must match exactly") {
        hotkey_manager<Backend> manager;

        auto act = std::make_shared<action<Backend>>();
        act->set_shortcut('s', key_modifier::ctrl);
        manager.register_action(act);

        // Ctrl+Shift+S should not match Ctrl+S
        test_backend::test_keyboard_event event;
        event.key_code = 's';
        event.pressed = true;
        event.ctrl = true;
        event.shift = true;  // Extra modifier

        CHECK(!manager.handle_key_event(event));
    }

    TEST_CASE("F-key hotkeys work") {
        hotkey_manager<Backend> manager;

        auto act = std::make_shared<action<Backend>>();
        act->set_shortcut_f(9);  // F9

        bool triggered = false;
        act->triggered.connect([&]() { triggered = true; });

        manager.register_action(act);

        test_backend::test_keyboard_event event;
        event.key_code = event_traits<test_backend::test_keyboard_event>::KEY_F9;
        event.pressed = true;

        CHECK(manager.handle_key_event(event));
        CHECK(triggered);
    }

    TEST_CASE("Disabled action does not trigger") {
        hotkey_manager<Backend> manager;

        auto act = std::make_shared<action<Backend>>();
        act->set_shortcut('s', key_modifier::ctrl);
        act->set_enabled(false);  // Disabled

        bool triggered = false;
        act->triggered.connect([&]() { triggered = true; });

        manager.register_action(act);

        test_backend::test_keyboard_event event;
        event.key_code = 's';
        event.pressed = true;
        event.ctrl = true;

        // Event is "handled" but action doesn't trigger
        CHECK(!manager.handle_key_event(event));
        CHECK(!triggered);
    }
}

// ======================================================================
// Test Suite: hotkey_manager - Scoped Hotkeys
// ======================================================================

TEST_SUITE("hotkey_manager::scoping") {
    using Backend = test_backend;

    TEST_CASE("Element-scoped hotkey requires focus") {
        hotkey_manager<Backend> manager;

        auto root = std::make_unique<panel<Backend>>();

        auto act = std::make_shared<action<Backend>>();
        act->set_shortcut('b', key_modifier::ctrl);

        bool triggered = false;
        act->triggered.connect([&]() { triggered = true; });

        // Register scoped to root
        manager.register_action(act, root.get());

        test_backend::test_keyboard_event event;
        event.key_code = 'b';
        event.pressed = true;
        event.ctrl = true;

        // Without focus info, scoped hotkey doesn't trigger
        CHECK(!manager.handle_key_event(event));
        CHECK(!triggered);
    }

    TEST_CASE("Element-scoped hotkey triggers with focus") {
        hotkey_manager<Backend> manager;

        auto root = std::make_unique<panel<Backend>>();

        auto act = std::make_shared<action<Backend>>();
        act->set_shortcut('b', key_modifier::ctrl);

        bool triggered = false;
        act->triggered.connect([&]() { triggered = true; });

        manager.register_action(act, root.get());

        test_backend::test_keyboard_event event;
        event.key_code = 'b';
        event.pressed = true;
        event.ctrl = true;

        // With focus info, scoped hotkey triggers
        CHECK(manager.handle_key_event(event, root.get()));
        CHECK(triggered);
    }

    TEST_CASE("Global hotkeys work without focus") {
        hotkey_manager<Backend> manager;

        auto act = std::make_shared<action<Backend>>();
        act->set_shortcut('s', key_modifier::ctrl);

        bool triggered = false;
        act->triggered.connect([&]() { triggered = true; });

        manager.register_action(act);  // Global

        test_backend::test_keyboard_event event;
        event.key_code = 's';
        event.pressed = true;
        event.ctrl = true;

        // No focus info needed for global
        CHECK(manager.handle_key_event(event));
        CHECK(triggered);
    }

    TEST_CASE("Scoped hotkeys check parent elements") {
        hotkey_manager<Backend> manager;

        auto root = std::make_unique<panel<Backend>>(nullptr);
        auto child = std::make_unique<panel<Backend>>(root.get());
        panel<Backend>* child_ptr = child.get();
        root->add_child(std::move(child));

        auto act = std::make_shared<action<Backend>>();
        act->set_shortcut('p', key_modifier::ctrl);

        bool triggered = false;
        act->triggered.connect([&]() { triggered = true; });

        // Register scoped to root
        manager.register_action(act, root.get());

        test_backend::test_keyboard_event event;
        event.key_code = 'p';
        event.pressed = true;
        event.ctrl = true;

        // Focused on child, but hotkey is on parent
        CHECK(manager.handle_key_event(event, child_ptr));
        CHECK(triggered);
    }

    TEST_CASE("Element-scoped hotkeys have priority over global") {
        hotkey_manager<Backend> manager;

        auto root = std::make_unique<panel<Backend>>(nullptr);

        // Global action
        auto global_action = std::make_shared<action<Backend>>();
        global_action->set_shortcut('s', key_modifier::ctrl);
        bool global_triggered = false;
        global_action->triggered.connect([&]() { global_triggered = true; });
        manager.register_action(global_action);

        // Scoped action (same hotkey)
        auto scoped_action = std::make_shared<action<Backend>>();
        scoped_action->set_shortcut('s', key_modifier::ctrl);
        bool scoped_triggered = false;
        scoped_action->triggered.connect([&]() { scoped_triggered = true; });
        manager.register_action(scoped_action, root.get());

        test_backend::test_keyboard_event event;
        event.key_code = 's';
        event.pressed = true;
        event.ctrl = true;

        // With focus, scoped takes priority
        CHECK(manager.handle_key_event(event, root.get()));
        CHECK(scoped_triggered);
        CHECK(!global_triggered);
    }
}

// ======================================================================
// Test Suite: hotkey_manager - Conflict Detection
// ======================================================================

TEST_SUITE("hotkey_manager::conflicts") {
    using Backend = test_backend;

    TEST_CASE("Conflict policy 'allow' permits duplicates") {
        hotkey_manager<Backend> manager;
        manager.set_conflict_policy(hotkey_manager<Backend>::conflict_policy::allow);

        auto action1 = std::make_shared<action<Backend>>();
        action1->set_shortcut('s', key_modifier::ctrl);

        auto action2 = std::make_shared<action<Backend>>();
        action2->set_shortcut('s', key_modifier::ctrl);

        CHECK(manager.register_action(action1));
        CHECK(manager.register_action(action2));  // Allowed
    }

    TEST_CASE("Conflict policy 'error' rejects duplicates") {
        hotkey_manager<Backend> manager;
        manager.set_conflict_policy(hotkey_manager<Backend>::conflict_policy::error);

        auto action1 = std::make_shared<action<Backend>>();
        action1->set_shortcut('s', key_modifier::ctrl);

        auto action2 = std::make_shared<action<Backend>>();
        action2->set_shortcut('s', key_modifier::ctrl);

        CHECK(manager.register_action(action1));
        CHECK(!manager.register_action(action2));  // Rejected
    }

    TEST_CASE("Conflict policy 'warn' accepts but warns") {
        hotkey_manager<Backend> manager;
        manager.set_conflict_policy(hotkey_manager<Backend>::conflict_policy::warn);

        auto action1 = std::make_shared<action<Backend>>();
        action1->set_shortcut('s', key_modifier::ctrl);

        auto action2 = std::make_shared<action<Backend>>();
        action2->set_shortcut('s', key_modifier::ctrl);

        CHECK(manager.register_action(action1));
        CHECK(manager.register_action(action2));  // Accepted (with warning)
    }

    TEST_CASE("Different scopes don't conflict") {
        hotkey_manager<Backend> manager;
        manager.set_conflict_policy(hotkey_manager<Backend>::conflict_policy::error);

        auto root = std::make_unique<panel<Backend>>(nullptr);

        auto global_action = std::make_shared<action<Backend>>();
        global_action->set_shortcut('s', key_modifier::ctrl);

        auto scoped_action = std::make_shared<action<Backend>>();
        scoped_action->set_shortcut('s', key_modifier::ctrl);

        CHECK(manager.register_action(global_action));  // Global
        CHECK(manager.register_action(scoped_action, root.get()));  // Scoped - no conflict
    }

    TEST_CASE("get_conflict_policy returns current policy") {
        hotkey_manager<Backend> manager;

        manager.set_conflict_policy(hotkey_manager<Backend>::conflict_policy::error);
        CHECK(manager.get_conflict_policy() == hotkey_manager<Backend>::conflict_policy::error);

        manager.set_conflict_policy(hotkey_manager<Backend>::conflict_policy::allow);
        CHECK(manager.get_conflict_policy() == hotkey_manager<Backend>::conflict_policy::allow);
    }
}

// ======================================================================
// Test Suite: hotkey_manager - Cleanup
// ======================================================================

TEST_SUITE("hotkey_manager::cleanup") {
    using Backend = test_backend;

    TEST_CASE("Expired registrations are cleaned up") {
        hotkey_manager<Backend> manager;

        {
            auto act = std::make_shared<action<Backend>>();
            act->set_shortcut('s', key_modifier::ctrl);
            manager.register_action(act);

            key_sequence const seq{'s', key_modifier::ctrl};
            CHECK(manager.is_registered(seq));
        } // act goes out of scope

        // Cleanup removes expired registrations
        manager.cleanup();

        key_sequence const seq{'s', key_modifier::ctrl};
        CHECK(!manager.is_registered(seq));
    }

    TEST_CASE("Cleanup doesn't affect valid registrations") {
        hotkey_manager<Backend> manager;

        auto act = std::make_shared<action<Backend>>();
        act->set_shortcut('s', key_modifier::ctrl);
        manager.register_action(act);

        manager.cleanup();

        key_sequence const seq{'s', key_modifier::ctrl};
        CHECK(manager.is_registered(seq));  // Still there
    }

    TEST_CASE("Event handling triggers automatic cleanup") {
        hotkey_manager<Backend> manager;

        {
            auto act = std::make_shared<action<Backend>>();
            act->set_shortcut('s', key_modifier::ctrl);
            manager.register_action(act);
        } // act expires

        // Handle event (triggers cleanup)
        test_backend::test_keyboard_event event;
        event.key_code = 's';
        event.pressed = true;
        event.ctrl = true;

        CHECK(!manager.handle_key_event(event));  // No longer registered

        // Verify cleanup happened
        key_sequence const seq{'s', key_modifier::ctrl};
        CHECK(!manager.is_registered(seq));
    }
}

// ======================================================================
// Test Suite: hotkey_manager - Real-World Scenarios
// ======================================================================

TEST_SUITE("hotkey_manager::scenarios") {
    using Backend = test_backend;

    TEST_CASE("Application-wide shortcuts") {
        hotkey_manager<Backend> manager;

        auto save_action = std::make_shared<action<Backend>>();
        save_action->set_shortcut('s', key_modifier::ctrl);

        auto quit_action = std::make_shared<action<Backend>>();
        quit_action->set_shortcut('q', key_modifier::ctrl);

        auto help_action = std::make_shared<action<Backend>>();
        help_action->set_shortcut_f(1);  // F1

        manager.register_action(save_action);
        manager.register_action(quit_action);
        manager.register_action(help_action);

        // Verify all registered
        CHECK(manager.is_registered(key_sequence{'s', key_modifier::ctrl}));
        CHECK(manager.is_registered(key_sequence{'q', key_modifier::ctrl}));
        CHECK(manager.is_registered(key_sequence{1, key_modifier::none}));
    }

    TEST_CASE("Text editor with formatting shortcuts") {
        hotkey_manager<Backend> manager;

        auto editor = std::make_unique<panel<Backend>>(nullptr);

        auto bold_action = std::make_shared<action<Backend>>();
        bold_action->set_shortcut('b', key_modifier::ctrl);

        auto italic_action = std::make_shared<action<Backend>>();
        italic_action->set_shortcut('i', key_modifier::ctrl);

        auto underline_action = std::make_shared<action<Backend>>();
        underline_action->set_shortcut('u', key_modifier::ctrl);

        // Register all scoped to editor
        manager.register_action(bold_action, editor.get());
        manager.register_action(italic_action, editor.get());
        manager.register_action(underline_action, editor.get());

        // Test Ctrl+B with focus
        bool bold_triggered = false;
        bold_action->triggered.connect([&]() { bold_triggered = true; });

        test_backend::test_keyboard_event event;
        event.key_code = 'b';
        event.pressed = true;
        event.ctrl = true;

        CHECK(manager.handle_key_event(event, editor.get()));
        CHECK(bold_triggered);
    }

    TEST_CASE("Menu accelerators (Alt+key)") {
        hotkey_manager<Backend> manager;

        auto file_action = std::make_shared<action<Backend>>();
        file_action->set_shortcut('f', key_modifier::alt);

        auto edit_action = std::make_shared<action<Backend>>();
        edit_action->set_shortcut('e', key_modifier::alt);

        manager.register_action(file_action);
        manager.register_action(edit_action);

        // Test Alt+F
        bool file_triggered = false;
        file_action->triggered.connect([&]() { file_triggered = true; });

        test_backend::test_keyboard_event event;
        event.key_code = 'f';
        event.pressed = true;
        event.alt = true;

        CHECK(manager.handle_key_event(event));
        CHECK(file_triggered);
    }

    TEST_CASE("Dynamic registration and unregistration") {
        hotkey_manager<Backend> manager;

        auto act = std::make_shared<action<Backend>>();
        act->set_shortcut('x', key_modifier::ctrl);

        // Register
        CHECK(manager.register_action(act));
        CHECK(manager.is_registered(key_sequence{'x', key_modifier::ctrl}));

        // Unregister
        manager.unregister_action(act);
        CHECK(!manager.is_registered(key_sequence{'x', key_modifier::ctrl}));

        // Re-register
        CHECK(manager.register_action(act));
        CHECK(manager.is_registered(key_sequence{'x', key_modifier::ctrl}));
    }
}

// ======================================================================
// Test Suite: hotkey_manager - Semantic Actions (Phase 3)
// ======================================================================

TEST_SUITE("hotkey_manager::semantic_actions") {
    using Backend = test_backend;

    TEST_CASE("Semantic action registration") {
        hotkey_scheme_registry registry;
        registry.register_scheme(builtin_hotkey_schemes::windows());

        hotkey_manager<Backend> manager(&registry);

        bool handler_called = false;
        manager.register_semantic_action(
            hotkey_action::activate_menu_bar,
            [&]() { handler_called = true; }
        );

        CHECK(manager.has_semantic_handler(hotkey_action::activate_menu_bar));
        CHECK_FALSE(manager.has_semantic_handler(hotkey_action::menu_down));
    }

    TEST_CASE("Semantic action triggering - F10 (Windows scheme)") {
        hotkey_scheme_registry registry;
        registry.register_scheme(builtin_hotkey_schemes::windows());

        hotkey_manager<Backend> manager(&registry);

        bool menu_activated = false;
        manager.register_semantic_action(
            hotkey_action::activate_menu_bar,
            [&]() { menu_activated = true; }
        );

        // Simulate F10 press (Windows scheme)
        test_backend::test_keyboard_event event;
        event.key_code = event_traits<test_backend::test_keyboard_event>::KEY_F10;
        event.pressed = true;

        CHECK(manager.handle_key_event(event));
        CHECK(menu_activated);
    }

    TEST_CASE("Semantic action triggering - F9 (Norton Commander scheme)") {
        hotkey_scheme_registry registry;
        registry.register_scheme(builtin_hotkey_schemes::norton_commander());

        hotkey_manager<Backend> manager(&registry);

        bool menu_activated = false;
        manager.register_semantic_action(
            hotkey_action::activate_menu_bar,
            [&]() { menu_activated = true; }
        );

        // Simulate F9 press (Norton Commander scheme)
        test_backend::test_keyboard_event event;
        event.key_code = event_traits<test_backend::test_keyboard_event>::KEY_F9;
        event.pressed = true;

        CHECK(manager.handle_key_event(event));
        CHECK(menu_activated);
    }

    TEST_CASE("Scheme switching changes active keys") {
        hotkey_scheme_registry registry;
        registry.register_scheme(builtin_hotkey_schemes::windows());
        registry.register_scheme(builtin_hotkey_schemes::norton_commander());

        hotkey_manager<Backend> manager(&registry);

        int menu_activation_count = 0;
        manager.register_semantic_action(
            hotkey_action::activate_menu_bar,
            [&]() { menu_activation_count++; }
        );

        // Start with Windows scheme (F10)
        registry.set_current_scheme("Windows");

        test_backend::test_keyboard_event f10_event;
        f10_event.key_code = event_traits<test_backend::test_keyboard_event>::KEY_F10;
        f10_event.pressed = true;

        CHECK(manager.handle_key_event(f10_event));
        CHECK(menu_activation_count == 1);

        // Switch to Norton Commander scheme (F9)
        registry.set_current_scheme("Norton Commander");

        // F10 should no longer work
        menu_activation_count = 0;
        CHECK_FALSE(manager.handle_key_event(f10_event));
        CHECK(menu_activation_count == 0);

        // F9 should work now
        test_backend::test_keyboard_event f9_event;
        f9_event.key_code = event_traits<test_backend::test_keyboard_event>::KEY_F9;
        f9_event.pressed = true;

        CHECK(manager.handle_key_event(f9_event));
        CHECK(menu_activation_count == 1);
    }

    TEST_CASE("Priority: Semantic actions before application actions") {
        hotkey_scheme_registry registry;
        registry.register_scheme(builtin_hotkey_schemes::windows());

        hotkey_manager<Backend> manager(&registry);

        bool semantic_called = false;
        bool application_called = false;

        // Register semantic action for F10
        manager.register_semantic_action(
            hotkey_action::activate_menu_bar,
            [&]() { semantic_called = true; }
        );

        // Register application action for F10 (should be lower priority)
        auto app_action = std::make_shared<action<Backend>>();
        app_action->set_shortcut_f(10);
        app_action->triggered.connect([&]() { application_called = true; });
        manager.register_action(app_action);

        // Simulate F10 press
        test_backend::test_keyboard_event event;
        event.key_code = event_traits<test_backend::test_keyboard_event>::KEY_F10;
        event.pressed = true;

        // Semantic action should take priority
        CHECK(manager.handle_key_event(event));
        CHECK(semantic_called);
        CHECK_FALSE(application_called);  // Application action should NOT be called
    }

    TEST_CASE("Graceful fallback: No handler registered") {
        hotkey_scheme_registry registry;
        registry.register_scheme(builtin_hotkey_schemes::windows());

        hotkey_manager<Backend> manager(&registry);

        // Don't register any handler for activate_menu_bar

        // Simulate F10 press
        test_backend::test_keyboard_event event;
        event.key_code = event_traits<test_backend::test_keyboard_event>::KEY_F10;
        event.pressed = true;

        // Should return false (no handler), allowing fallback to application actions
        CHECK_FALSE(manager.handle_key_event(event));
    }

    TEST_CASE("Graceful fallback: No scheme registry") {
        hotkey_manager<Backend> manager(nullptr);  // No scheme registry

        // Register semantic action (should have no effect without registry)
        bool handler_called = false;
        manager.register_semantic_action(
            hotkey_action::activate_menu_bar,
            [&]() { handler_called = true; }
        );

        // Simulate F10 press
        test_backend::test_keyboard_event event;
        event.key_code = event_traits<test_backend::test_keyboard_event>::KEY_F10;
        event.pressed = true;

        // Should return false (no scheme registry)
        CHECK_FALSE(manager.handle_key_event(event));
        CHECK_FALSE(handler_called);
    }

    TEST_CASE("Unregister semantic action") {
        hotkey_scheme_registry registry;
        registry.register_scheme(builtin_hotkey_schemes::windows());

        hotkey_manager<Backend> manager(&registry);

        bool handler_called = false;
        manager.register_semantic_action(
            hotkey_action::activate_menu_bar,
            [&]() { handler_called = true; }
        );

        REQUIRE(manager.has_semantic_handler(hotkey_action::activate_menu_bar));

        // Unregister
        manager.unregister_semantic_action(hotkey_action::activate_menu_bar);
        CHECK_FALSE(manager.has_semantic_handler(hotkey_action::activate_menu_bar));

        // Handler should not be called after unregistering
        test_backend::test_keyboard_event event;
        event.key_code = event_traits<test_backend::test_keyboard_event>::KEY_F10;
        event.pressed = true;

        CHECK_FALSE(manager.handle_key_event(event));
        CHECK_FALSE(handler_called);
    }

    TEST_CASE("Set scheme registry after construction") {
        hotkey_manager<Backend> manager;  // No registry initially

        CHECK(manager.get_scheme_registry() == nullptr);

        hotkey_scheme_registry registry;
        registry.register_scheme(builtin_hotkey_schemes::windows());

        manager.set_scheme_registry(&registry);
        CHECK(manager.get_scheme_registry() == &registry);

        // Should now work
        bool menu_activated = false;
        manager.register_semantic_action(
            hotkey_action::activate_menu_bar,
            [&]() { menu_activated = true; }
        );

        test_backend::test_keyboard_event event;
        event.key_code = event_traits<test_backend::test_keyboard_event>::KEY_F10;
        event.pressed = true;

        CHECK(manager.handle_key_event(event));
        CHECK(menu_activated);
    }
}
