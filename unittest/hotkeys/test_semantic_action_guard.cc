/**
 * @file test_semantic_action_guard.cc
 * @brief Unit tests for semantic_action_guard RAII class
 * @author Assistant
 * @date 2025-10-28
 *
 * @details
 * Tests the RAII guard for semantic action registration:
 * - Automatic registration on construction
 * - Automatic unregistration on destruction
 * - Move semantics
 * - Exception safety
 * - Vector storage (typical use case)
 */

#include <doctest/doctest.h>
#include <onyxui/hotkeys/semantic_action_guard.hh>
#include <onyxui/hotkeys/hotkey_manager.hh>
#include <onyxui/hotkeys/hotkey_action.hh>
#include <onyxui/ui_context.hh>
#include "utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

// ======================================================================
// Test Suite: semantic_action_guard - Basic Functionality
// ======================================================================

TEST_SUITE("semantic_action_guard") {

    TEST_CASE("Default constructor creates invalid guard") {
        semantic_action_guard<Backend> guard;

        CHECK_FALSE(guard.is_valid());
    }

    TEST_CASE("Constructor with params creates valid guard and registers action") {
        scoped_ui_context<Backend> ctx;
        auto& hotkeys = ctx.hotkeys();

        bool handler_called = false;

        {
            semantic_action_guard<Backend> guard(
                &hotkeys,
                hotkey_action::menu_down,
                [&handler_called]() { handler_called = true; }
            );

            CHECK(guard.is_valid());
            CHECK(guard.action() == hotkey_action::menu_down);

            // Handler should be registered
            CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_down));
        }

        // Guard destroyed - handler should be unregistered
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_down));
    }

    TEST_CASE("Destructor automatically unregisters action") {
        scoped_ui_context<Backend> ctx;
        auto& hotkeys = ctx.hotkeys();

        bool handler_called = false;

        {
            semantic_action_guard<Backend> guard(
                &hotkeys,
                hotkey_action::menu_up,
                [&handler_called]() { handler_called = true; }
            );

            // Handler registered
            CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_up));
        }

        // Guard destroyed - handler unregistered
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_up));
    }

    TEST_CASE("Null manager creates invalid but safe guard") {
        semantic_action_guard<Backend> guard(
            nullptr,  // nullptr manager
            hotkey_action::menu_down,
            []() {}
        );

        CHECK_FALSE(guard.is_valid());
        // Should not crash on destruction
    }

    TEST_CASE("action() returns the guarded action") {
        scoped_ui_context<Backend> ctx;
        auto& hotkeys = ctx.hotkeys();

        semantic_action_guard<Backend> guard(
            &hotkeys,
            hotkey_action::menu_select,
            []() {}
        );

        CHECK(guard.action() == hotkey_action::menu_select);
    }

    TEST_CASE("release() prevents unregistration on destruction") {
        scoped_ui_context<Backend> ctx;
        auto& hotkeys = ctx.hotkeys();

        {
            semantic_action_guard<Backend> guard(
                &hotkeys,
                hotkey_action::menu_cancel,
                []() {}
            );

            CHECK(guard.is_valid());
            CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_cancel));

            // Release ownership
            guard.release();
            CHECK_FALSE(guard.is_valid());
        }

        // Guard destroyed but action should still be registered (leaked)
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_cancel));

        // Manual cleanup
        hotkeys.unregister_semantic_action(hotkey_action::menu_cancel);
    }
}

// ======================================================================
// Test Suite: semantic_action_guard - Move Semantics
// ======================================================================

TEST_SUITE("semantic_action_guard::move_semantics") {

    TEST_CASE("Move constructor transfers ownership") {
        scoped_ui_context<Backend> ctx;
        auto& hotkeys = ctx.hotkeys();

        semantic_action_guard<Backend> guard1(
            &hotkeys,
            hotkey_action::focus_next,
            []() {}
        );

        CHECK(guard1.is_valid());
        CHECK(hotkeys.has_semantic_handler(hotkey_action::focus_next));

        // Move construct
        semantic_action_guard<Backend> guard2(std::move(guard1));

        CHECK_FALSE(guard1.is_valid());  // guard1 invalidated
        CHECK(guard2.is_valid());         // guard2 owns registration
        CHECK(hotkeys.has_semantic_handler(hotkey_action::focus_next));

        // Destroy guard1 (should not unregister)
        guard1.~semantic_action_guard();
        new (&guard1) semantic_action_guard<Backend>();  // Placement new for safety

        CHECK(hotkeys.has_semantic_handler(hotkey_action::focus_next));

        // Destroy guard2 (should unregister)
        guard2.~semantic_action_guard();
        new (&guard2) semantic_action_guard<Backend>();

        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::focus_next));
    }

    TEST_CASE("Move assignment transfers ownership") {
        scoped_ui_context<Backend> ctx;
        auto& hotkeys = ctx.hotkeys();

        semantic_action_guard<Backend> guard1(
            &hotkeys,
            hotkey_action::focus_previous,
            []() {}
        );

        semantic_action_guard<Backend> guard2;

        CHECK(guard1.is_valid());
        CHECK_FALSE(guard2.is_valid());

        // Move assign
        guard2 = std::move(guard1);

        CHECK_FALSE(guard1.is_valid());  // guard1 invalidated
        CHECK(guard2.is_valid());         // guard2 owns registration
        CHECK(hotkeys.has_semantic_handler(hotkey_action::focus_previous));
    }

    TEST_CASE("Move assignment unregisters previous action") {
        scoped_ui_context<Backend> ctx;
        auto& hotkeys = ctx.hotkeys();

        semantic_action_guard<Backend> guard1(
            &hotkeys,
            hotkey_action::menu_left,
            []() {}
        );

        semantic_action_guard<Backend> guard2(
            &hotkeys,
            hotkey_action::menu_right,
            []() {}
        );

        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_left));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_right));

        // Move assign - guard2's previous action should be unregistered
        guard2 = std::move(guard1);

        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_left));   // Still registered (moved to guard2)
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_right));  // Unregistered (guard2's old action)
    }

    TEST_CASE("Self-move assignment is safe") {
        scoped_ui_context<Backend> ctx;
        auto& hotkeys = ctx.hotkeys();

        semantic_action_guard<Backend> guard(
            &hotkeys,
            hotkey_action::activate_menu_bar,
            []() {}
        );

        CHECK(guard.is_valid());

        // Self-move (should be no-op)
        guard = std::move(guard);

        CHECK(guard.is_valid());
        CHECK(hotkeys.has_semantic_handler(hotkey_action::activate_menu_bar));
    }
}

// ======================================================================
// Test Suite: semantic_action_guard - Vector Storage (Typical Use Case)
// ======================================================================

TEST_SUITE("semantic_action_guard::vector_storage") {

    TEST_CASE("Can store guards in vector") {
        scoped_ui_context<Backend> ctx;
        auto& hotkeys = ctx.hotkeys();

        std::vector<semantic_action_guard<Backend>> guards;

        // Add guards
        guards.emplace_back(&hotkeys, hotkey_action::menu_down, []() {});
        guards.emplace_back(&hotkeys, hotkey_action::menu_up, []() {});
        guards.emplace_back(&hotkeys, hotkey_action::menu_select, []() {});

        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_down));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_up));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_select));

        // Clear vector - all guards destroyed, all actions unregistered
        guards.clear();

        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_down));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_up));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_select));
    }

    TEST_CASE("Vector reallocation preserves guards") {
        scoped_ui_context<Backend> ctx;
        auto& hotkeys = ctx.hotkeys();

        std::vector<semantic_action_guard<Backend>> guards;
        guards.reserve(2);  // Small capacity to force reallocation

        guards.emplace_back(&hotkeys, hotkey_action::menu_down, []() {});
        guards.emplace_back(&hotkeys, hotkey_action::menu_up, []() {});

        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_down));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_up));

        // Force reallocation
        guards.emplace_back(&hotkeys, hotkey_action::menu_select, []() {});

        // All actions should still be registered after reallocation
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_down));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_up));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_select));
    }

    TEST_CASE("Replacing vector contents unregisters old guards") {
        scoped_ui_context<Backend> ctx;
        auto& hotkeys = ctx.hotkeys();

        std::vector<semantic_action_guard<Backend>> guards;

        // First set of guards
        guards.emplace_back(&hotkeys, hotkey_action::menu_down, []() {});
        guards.emplace_back(&hotkeys, hotkey_action::menu_up, []() {});

        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_down));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_up));

        // Clear and add new guards
        guards.clear();

        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_down));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_up));

        guards.emplace_back(&hotkeys, hotkey_action::menu_left, []() {});
        guards.emplace_back(&hotkeys, hotkey_action::menu_right, []() {});

        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_left));
        CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_right));
    }
}

// ======================================================================
// Test Suite: semantic_action_guard - Exception Safety
// ======================================================================

TEST_SUITE("semantic_action_guard::exception_safety") {

    TEST_CASE("Guard unregisters even if exception thrown") {
        scoped_ui_context<Backend> ctx;
        auto& hotkeys = ctx.hotkeys();

        try {
            semantic_action_guard<Backend> guard(
                &hotkeys,
                hotkey_action::menu_cancel,
                []() {}
            );

            CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_cancel));

            // Simulate exception
            throw std::runtime_error("Simulated error");
        } catch (const std::exception&) {
            // Exception caught
        }

        // Guard should have unregistered despite exception
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_cancel));
    }

    TEST_CASE("Vector of guards is exception-safe") {
        scoped_ui_context<Backend> ctx;
        auto& hotkeys = ctx.hotkeys();

        try {
            std::vector<semantic_action_guard<Backend>> guards;

            guards.emplace_back(&hotkeys, hotkey_action::menu_down, []() {});
            guards.emplace_back(&hotkeys, hotkey_action::menu_up, []() {});

            CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_down));
            CHECK(hotkeys.has_semantic_handler(hotkey_action::menu_up));

            // Simulate exception during vector usage
            throw std::runtime_error("Simulated error");
        } catch (const std::exception&) {
            // Exception caught
        }

        // All guards should have unregistered
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_down));
        CHECK_FALSE(hotkeys.has_semantic_handler(hotkey_action::menu_up));
    }
}
