/**
 * @file test_focus_manager.cc
 * @brief Comprehensive tests for focus_manager keyboard navigation
 */

#include <doctest/doctest.h>
#include <onyxui/focus_manager.hh>
#include <onyxui/event_target.hh>
#include <utils/test_backend.hh>
#include <memory>
#include <vector>

using namespace onyxui;

// Test event target that we can configure for testing
class TestTarget : public event_target<test_backend> {
public:
    TestTarget(int tab_idx = -1, bool enabled = true, bool visible = true, bool focusable = true)
        : m_visible(visible)
    {
        set_tab_index(tab_idx);
        set_enabled(enabled);
        set_focusable(focusable);
    }

    // Implement pure virtual is_inside() method
    bool is_inside(int x, int y) const override {
        // Simple test bounds: 0,0 to 100,100
        return x >= 0 && x < 100 && y >= 0 && y < 100;
    }

    // Optional is_visible() for focus_manager's compile-time check
    bool is_visible() const { return m_visible; }
    void set_visible(bool v) { m_visible = v; }

    // Override virtual handlers to track calls
    bool handle_focus_gained() override {
        focus_gain_count++;
        return event_target<test_backend>::handle_focus_gained();
    }

    bool handle_focus_lost() override {
        focus_loss_count++;
        return event_target<test_backend>::handle_focus_lost();
    }

    // Test inspection
    int focus_gain_count = 0;
    int focus_loss_count = 0;

private:
    bool m_visible;
};

TEST_SUITE("FocusManager") {
    // ======================================================================
    // Focus State Management Tests
    // ======================================================================

    TEST_CASE("Focus manager - set_focus basic behavior") {
        focus_manager<test_backend> manager;
        auto target = std::make_unique<TestTarget>();
        auto* target_ptr = target.get();

        SUBCASE("set_focus on enabled target") {
            manager.set_focus(target_ptr);
            CHECK(manager.get_focused() == target_ptr);
            CHECK(target_ptr->has_focus());
            CHECK(target_ptr->focus_gain_count == 1);
        }

        SUBCASE("set_focus on disabled target does nothing") {
            target->set_enabled(false);
            manager.set_focus(target_ptr);
            CHECK(manager.get_focused() == nullptr);
            CHECK_FALSE(target_ptr->has_focus());
            CHECK(target_ptr->focus_gain_count == 0);
        }

        SUBCASE("set_focus on invisible target succeeds") {
            // Note: focus_manager doesn't check is_visible() in set_focus()
            // It's up to the navigation functions to filter invisible targets
            target->set_visible(false);
            manager.set_focus(target_ptr);
            CHECK(manager.get_focused() == target_ptr);
            CHECK(target_ptr->has_focus());
            CHECK(target_ptr->focus_gain_count == 1);
        }

        SUBCASE("set_focus on non-focusable target does nothing") {
            target->set_focusable(false);
            manager.set_focus(target_ptr);
            CHECK(manager.get_focused() == nullptr);
            CHECK_FALSE(target_ptr->has_focus());
            CHECK(target_ptr->focus_gain_count == 0);
        }

        SUBCASE("set_focus with nullptr clears focus") {
            manager.set_focus(target_ptr);
            CHECK(manager.get_focused() == target_ptr);

            manager.set_focus(nullptr);
            CHECK(manager.get_focused() == nullptr);
            CHECK_FALSE(target_ptr->has_focus());
            CHECK(target_ptr->focus_loss_count == 1);
        }
    }

    TEST_CASE("Focus manager - focus transitions") {
        focus_manager<test_backend> manager;
        auto target1 = std::make_unique<TestTarget>();
        auto target2 = std::make_unique<TestTarget>();
        auto* t1 = target1.get();
        auto* t2 = target2.get();

        SUBCASE("moving focus between targets") {
            manager.set_focus(t1);
            CHECK(t1->focus_gain_count == 1);
            CHECK(t1->focus_loss_count == 0);

            manager.set_focus(t2);
            CHECK(t1->focus_loss_count == 1);
            CHECK(t2->focus_gain_count == 1);
            CHECK(manager.get_focused() == t2);
        }

        SUBCASE("set_focus on already focused target") {
            manager.set_focus(t1);
            CHECK(t1->focus_gain_count == 1);

            manager.set_focus(t1);
            // Should not trigger duplicate events
            CHECK(t1->focus_gain_count == 1);
            CHECK(t1->focus_loss_count == 0);
        }
    }

    TEST_CASE("Focus manager - clear_focus") {
        focus_manager<test_backend> manager;
        auto target = std::make_unique<TestTarget>();
        auto* target_ptr = target.get();

        SUBCASE("clear_focus when something is focused") {
            manager.set_focus(target_ptr);
            CHECK(manager.get_focused() == target_ptr);

            manager.clear_focus();
            CHECK(manager.get_focused() == nullptr);
            CHECK_FALSE(target_ptr->has_focus());
            CHECK(target_ptr->focus_loss_count == 1);
        }

        SUBCASE("clear_focus when nothing is focused") {
            CHECK(manager.get_focused() == nullptr);
            manager.clear_focus(); // Should not crash
            CHECK(manager.get_focused() == nullptr);
        }
    }

    TEST_CASE("Focus manager - has_focus") {
        focus_manager<test_backend> manager;
        auto target1 = std::make_unique<TestTarget>();
        auto target2 = std::make_unique<TestTarget>();
        auto* t1 = target1.get();
        auto* t2 = target2.get();

        SUBCASE("has_focus returns true for focused target") {
            manager.set_focus(t1);
            CHECK(manager.has_focus(t1));
            CHECK_FALSE(manager.has_focus(t2));
        }

        SUBCASE("has_focus returns false after focus cleared") {
            manager.set_focus(t1);
            manager.clear_focus();
            CHECK_FALSE(manager.has_focus(t1));
        }

        SUBCASE("has_focus with nullptr when nothing focused") {
            // has_focus(nullptr) returns true if nothing is focused
            CHECK(manager.has_focus(nullptr));

            // After focusing something, has_focus(nullptr) returns false
            manager.set_focus(t1);
            CHECK_FALSE(manager.has_focus(nullptr));
        }
    }

    // ======================================================================
    // Tab Navigation Tests
    // ======================================================================

    TEST_CASE("Focus manager - focus_next with implicit tab order") {
        focus_manager<test_backend> manager;
        std::vector<std::unique_ptr<TestTarget>> targets;
        std::vector<event_target<test_backend>*> target_ptrs;

        // Create 3 targets with implicit tab indices (-1)
        for (int i = 0; i < 3; ++i) {
            targets.push_back(std::make_unique<TestTarget>());
            target_ptrs.push_back(targets.back().get());
        }

        SUBCASE("focus_next cycles through all targets") {
            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[0]);

            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[1]);

            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[2]);
        }

        SUBCASE("focus_next wraps around to beginning") {
            manager.set_focus(target_ptrs[2]);
            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[0]);
        }

        SUBCASE("focus_next skips disabled targets") {
            static_cast<TestTarget*>(target_ptrs[1])->set_enabled(false);

            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[0]);

            manager.focus_next(target_ptrs);
            // Should skip target 1
            CHECK(manager.get_focused() == target_ptrs[2]);
        }

        SUBCASE("focus_next does not filter by visibility") {
            // Note: collect_focusable_targets() only checks focusable and enabled
            // It does NOT check is_visible() - that's only used in hierarchical collection
            static_cast<TestTarget*>(target_ptrs[1])->set_visible(false);

            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[0]);

            manager.focus_next(target_ptrs);
            // Invisible target is still in the focus order
            CHECK(manager.get_focused() == target_ptrs[1]);
        }
    }

    TEST_CASE("Focus manager - focus_next with explicit tab order") {
        focus_manager<test_backend> manager;
        std::vector<std::unique_ptr<TestTarget>> targets;
        std::vector<event_target<test_backend>*> target_ptrs;

        // Create targets with explicit tab indices: 2, 0, 1
        // Expected order: target[1] (0), target[2] (1), target[0] (2)
        targets.push_back(std::make_unique<TestTarget>(2));
        targets.push_back(std::make_unique<TestTarget>(0));
        targets.push_back(std::make_unique<TestTarget>(1));

        for (auto& t : targets) {
            target_ptrs.push_back(t.get());
        }

        SUBCASE("focus_next respects tab index order") {
            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[1]); // tab_index 0

            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[2]); // tab_index 1

            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[0]); // tab_index 2
        }
    }

    TEST_CASE("Focus manager - focus_previous with implicit tab order") {
        focus_manager<test_backend> manager;
        std::vector<std::unique_ptr<TestTarget>> targets;
        std::vector<event_target<test_backend>*> target_ptrs;

        for (int i = 0; i < 3; ++i) {
            targets.push_back(std::make_unique<TestTarget>());
            target_ptrs.push_back(targets.back().get());
        }

        SUBCASE("focus_previous cycles backward") {
            manager.set_focus(target_ptrs[1]);
            manager.focus_previous(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[0]);
        }

        SUBCASE("focus_previous wraps to end") {
            manager.set_focus(target_ptrs[0]);
            manager.focus_previous(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[2]);
        }

        SUBCASE("focus_previous from nothing focuses last") {
            manager.focus_previous(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[2]);
        }
    }

    TEST_CASE("Focus manager - focus_previous with explicit tab order") {
        focus_manager<test_backend> manager;
        std::vector<std::unique_ptr<TestTarget>> targets;
        std::vector<event_target<test_backend>*> target_ptrs;

        // Tab indices: 2, 0, 1 -> sorted order: 0, 1, 2
        targets.push_back(std::make_unique<TestTarget>(2));
        targets.push_back(std::make_unique<TestTarget>(0));
        targets.push_back(std::make_unique<TestTarget>(1));

        for (auto& t : targets) {
            target_ptrs.push_back(t.get());
        }

        SUBCASE("focus_previous respects tab index order") {
            manager.set_focus(target_ptrs[2]); // tab_index 1
            manager.focus_previous(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[1]); // tab_index 0
        }
    }

    TEST_CASE("Focus manager - handle_tab_navigation integration") {
        focus_manager<test_backend> manager;
        std::vector<std::unique_ptr<TestTarget>> targets;
        std::vector<event_target<test_backend>*> target_ptrs;

        for (int i = 0; i < 3; ++i) {
            targets.push_back(std::make_unique<TestTarget>());
            target_ptrs.push_back(targets.back().get());
        }

        SUBCASE("Tab key moves forward") {
            test_backend::test_keyboard_event evt;
            evt.key_code = '\t';
            evt.pressed = true;
            evt.shift = false;

            bool handled = manager.handle_tab_navigation(evt, target_ptrs);
            CHECK(handled);
            CHECK(manager.get_focused() == target_ptrs[0]);

            handled = manager.handle_tab_navigation(evt, target_ptrs);
            CHECK(handled);
            CHECK(manager.get_focused() == target_ptrs[1]);
        }

        SUBCASE("Shift+Tab moves backward") {
            test_backend::test_keyboard_event evt;
            evt.key_code = '\t';
            evt.pressed = true;
            evt.shift = true;

            manager.set_focus(target_ptrs[1]);

            bool handled = manager.handle_tab_navigation(evt, target_ptrs);
            CHECK(handled);
            CHECK(manager.get_focused() == target_ptrs[0]);
        }

        SUBCASE("Non-tab key not handled") {
            test_backend::test_keyboard_event evt;
            evt.key_code = 'A';
            evt.pressed = true;
            evt.shift = false;

            bool handled = manager.handle_tab_navigation(evt, target_ptrs);
            CHECK_FALSE(handled);
            CHECK(manager.get_focused() == nullptr);
        }

        SUBCASE("Tab key release not handled") {
            test_backend::test_keyboard_event evt;
            evt.key_code = '\t';
            evt.pressed = false;
            evt.shift = false;

            bool handled = manager.handle_tab_navigation(evt, target_ptrs);
            CHECK_FALSE(handled);
        }
    }

    // ======================================================================
    // Tab Order Stability Tests
    // ======================================================================

    TEST_CASE("Focus manager - stable tab order with equal indices") {
        focus_manager<test_backend> manager;
        std::vector<std::unique_ptr<TestTarget>> targets;
        std::vector<event_target<test_backend>*> target_ptrs;

        // Create 5 targets all with tab_index = 0
        for (int i = 0; i < 5; ++i) {
            targets.push_back(std::make_unique<TestTarget>(0));
            target_ptrs.push_back(targets.back().get());
        }

        SUBCASE("equal indices maintain deterministic order") {
            // Get pointer addresses for verification
            std::vector<const void*> addresses;
            for (auto* ptr : target_ptrs) {
                addresses.push_back(static_cast<const void*>(ptr));
            }

            // Sort by pointer address to get expected order
            std::vector<size_t> expected_order;
            for (size_t i = 0; i < addresses.size(); ++i) {
                expected_order.push_back(i);
            }
            std::stable_sort(expected_order.begin(), expected_order.end(),
                [&addresses](size_t a, size_t b) {
                    return std::less<const void*>()(addresses[a], addresses[b]);
                });

            // Navigate through all and verify deterministic order
            std::vector<event_target<test_backend>*> visited;
            for (size_t i = 0; i < target_ptrs.size(); ++i) {
                manager.focus_next(target_ptrs);
                visited.push_back(manager.get_focused());
            }

            // Order should be deterministic based on pointer addresses
            CHECK(visited.size() == target_ptrs.size());

            // Verify order is consistent across multiple iterations
            std::vector<event_target<test_backend>*> visited2;
            for (size_t i = 0; i < target_ptrs.size(); ++i) {
                manager.focus_next(target_ptrs);
                visited2.push_back(manager.get_focused());
            }

            CHECK(visited == visited2);
        }
    }

    TEST_CASE("Focus manager - mixed explicit and implicit indices") {
        focus_manager<test_backend> manager;
        std::vector<std::unique_ptr<TestTarget>> targets;
        std::vector<event_target<test_backend>*> target_ptrs;

        // Create mix: explicit (0, 2), implicit (-1, -1), explicit (1)
        targets.push_back(std::make_unique<TestTarget>(0));   // Should be 1st
        targets.push_back(std::make_unique<TestTarget>(-1));  // Should be last
        targets.push_back(std::make_unique<TestTarget>(2));   // Should be 3rd
        targets.push_back(std::make_unique<TestTarget>(-1));  // Should be last
        targets.push_back(std::make_unique<TestTarget>(1));   // Should be 2nd

        for (auto& t : targets) {
            target_ptrs.push_back(t.get());
        }

        SUBCASE("explicit indices come before implicit") {
            // First focus should be explicit tab_index 0
            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[0]);

            // Then explicit tab_index 1
            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[4]);

            // Then explicit tab_index 2
            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[2]);

            // Now implicit ones follow
            manager.focus_next(target_ptrs);
            bool is_implicit = (manager.get_focused() == target_ptrs[1] ||
                                manager.get_focused() == target_ptrs[3]);
            CHECK(is_implicit);
        }
    }

    // ======================================================================
    // Edge Cases and Boundary Tests
    // ======================================================================

    TEST_CASE("Focus manager - edge cases") {
        focus_manager<test_backend> manager;

        SUBCASE("focus_next with empty targets") {
            std::vector<event_target<test_backend>*> empty;
            manager.focus_next(empty); // Should not crash
            CHECK(manager.get_focused() == nullptr);
        }

        SUBCASE("focus_previous with empty targets") {
            std::vector<event_target<test_backend>*> empty;
            manager.focus_previous(empty); // Should not crash
            CHECK(manager.get_focused() == nullptr);
        }

        SUBCASE("focus_next with all targets disabled") {
            std::vector<std::unique_ptr<TestTarget>> targets;
            std::vector<event_target<test_backend>*> target_ptrs;

            for (int i = 0; i < 3; ++i) {
                targets.push_back(std::make_unique<TestTarget>());
                targets.back()->set_enabled(false);
                target_ptrs.push_back(targets.back().get());
            }

            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == nullptr);
        }

        SUBCASE("focus_next with single target") {
            auto target = std::make_unique<TestTarget>();
            std::vector<event_target<test_backend>*> single = { target.get() };

            manager.focus_next(single);
            CHECK(manager.get_focused() == target.get());

            manager.focus_next(single);
            // Should stay on same target (wrap around)
            CHECK(manager.get_focused() == target.get());
        }

        SUBCASE("setting focus on target not in list") {
            auto target1 = std::make_unique<TestTarget>();
            auto target2 = std::make_unique<TestTarget>();
            std::vector<event_target<test_backend>*> targets = { target1.get() };

            manager.set_focus(target2.get());
            CHECK(manager.get_focused() == target2.get());

            // focus_next should move to first in provided list
            manager.focus_next(targets);
            CHECK(manager.get_focused() == target1.get());
        }
    }

    TEST_CASE("Focus manager - focus change during navigation") {
        focus_manager<test_backend> manager;
        std::vector<std::unique_ptr<TestTarget>> targets;
        std::vector<event_target<test_backend>*> target_ptrs;

        for (int i = 0; i < 3; ++i) {
            targets.push_back(std::make_unique<TestTarget>());
            target_ptrs.push_back(targets.back().get());
        }

        SUBCASE("target becomes disabled after gaining focus") {
            manager.set_focus(target_ptrs[1]);
            CHECK(manager.get_focused() == target_ptrs[1]);

            static_cast<TestTarget*>(target_ptrs[1])->set_enabled(false);

            // Focus should still be on disabled target until explicitly moved
            CHECK(manager.get_focused() == target_ptrs[1]);

            // Tab navigation finds current focus not in list, so wraps to first
            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[0]);
        }

        SUBCASE("target becomes invisible after gaining focus") {
            // Visibility doesn't affect focus in the basic focus_manager
            manager.set_focus(target_ptrs[1]);
            static_cast<TestTarget*>(target_ptrs[1])->set_visible(false);

            CHECK(manager.get_focused() == target_ptrs[1]);

            // Invisible targets are still navigable
            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[2]);
        }
    }

    TEST_CASE("Focus manager - callback verification") {
        focus_manager<test_backend> manager;
        auto target1 = std::make_unique<TestTarget>();
        auto target2 = std::make_unique<TestTarget>();
        auto* t1 = target1.get();
        auto* t2 = target2.get();

        SUBCASE("on_focus_gained called exactly once") {
            manager.set_focus(t1);
            CHECK(t1->focus_gain_count == 1);

            manager.set_focus(t1);
            CHECK(t1->focus_gain_count == 1); // No duplicate
        }

        SUBCASE("on_focus_lost called exactly once") {
            manager.set_focus(t1);
            manager.clear_focus();
            CHECK(t1->focus_loss_count == 1);

            manager.clear_focus();
            CHECK(t1->focus_loss_count == 1); // No duplicate
        }

        SUBCASE("callbacks called in correct order on transition") {
            manager.set_focus(t1);
            CHECK(t1->focus_gain_count == 1);
            CHECK(t2->focus_gain_count == 0);

            manager.set_focus(t2);
            // t1 loses focus, t2 gains focus
            CHECK(t1->focus_loss_count == 1);
            CHECK(t2->focus_gain_count == 1);
        }
    }

    // ======================================================================
    // Real-world Usage Scenarios
    // ======================================================================

    TEST_CASE("Focus manager - dialog box navigation") {
        focus_manager<test_backend> manager;
        std::vector<std::unique_ptr<TestTarget>> targets;
        std::vector<event_target<test_backend>*> target_ptrs;

        // Simulate dialog: OK button (0), Cancel button (1), text input (2)
        targets.push_back(std::make_unique<TestTarget>(2)); // Text input first
        targets.push_back(std::make_unique<TestTarget>(0)); // OK button
        targets.push_back(std::make_unique<TestTarget>(1)); // Cancel button

        for (auto& t : targets) {
            target_ptrs.push_back(t.get());
        }

        SUBCASE("tab through dialog in logical order") {
            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[1]); // OK (tab_index 0)

            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[2]); // Cancel (tab_index 1)

            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[0]); // Text (tab_index 2)
        }
    }

    TEST_CASE("Focus manager - form with disabled field") {
        focus_manager<test_backend> manager;
        std::vector<std::unique_ptr<TestTarget>> targets;
        std::vector<event_target<test_backend>*> target_ptrs;

        targets.push_back(std::make_unique<TestTarget>(0)); // Name
        targets.push_back(std::make_unique<TestTarget>(1)); // Email (will disable)
        targets.push_back(std::make_unique<TestTarget>(2)); // Phone

        for (auto& t : targets) {
            target_ptrs.push_back(t.get());
        }

        static_cast<TestTarget*>(target_ptrs[1])->set_enabled(false);

        SUBCASE("skip disabled email field") {
            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[0]); // Name

            manager.focus_next(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[2]); // Phone (skipped email)

            manager.focus_previous(target_ptrs);
            CHECK(manager.get_focused() == target_ptrs[0]); // Back to name (skipped email)
        }
    }
}
