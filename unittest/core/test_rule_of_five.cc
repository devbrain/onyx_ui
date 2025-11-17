/**
 * @file test_rule_of_five.cc
 * @brief Comprehensive tests for Rule of Five implementations
 *
 * Tests that all move operations correctly transfer all fields and leave
 * source objects in valid states. Also verifies copy operations are properly
 * deleted where appropriate.
 */

#include "../../include/onyxui/theming/theme.hh"
#include "../../include/onyxui/layout/layout_strategy.hh"
#include <doctest/doctest.h>
#include <../../include/onyxui/core/element.hh>
#include <../../include/onyxui/core/event_target.hh>
#include <../../include/onyxui/services/focus_manager.hh>
#include <onyxui/layout/linear_layout.hh>
#include <type_traits>
#include <utils/test_backend.hh>
#include <memory>
#include <utility>
#include <vector>

#include "utils/test_helpers.hh"

using namespace onyxui;

// Test helper class that implements required pure virtual methods
class test_element : public ui_element<test_backend> {
public:
    using ui_element<test_backend>::ui_element;

protected:
    // Implement required pure virtual from themeable
        // Test element - no theme application needed

};

// Test event target with tracking
class test_event_target : public event_target<test_backend> {
public:
    bool is_inside(int x, int y) const override {
        return x >= 0 && x < 100 && y >= 0 && y < 100;
    }

    int click_count = 0;

    // Override handle_mouse to track clicks
    bool handle_mouse(const mouse_event& mouse) override {
        if (mouse.act == mouse_event::action::release) {
            click_count++;
            return true;
        }
        return event_target<test_backend>::handle_mouse(mouse);
    }
};

TEST_SUITE("RuleOfFive") {
    // ========================================================================
    // ui_element Move Operations Tests
    // ========================================================================

    TEST_CASE("ui_element - move constructor transfers all fields") {
        test_element source(nullptr);

        // Set up source with various properties
        source.set_visible(false);
        source.set_z_order(42);

        size_constraint width_c;
        width_c.policy = size_policy::fixed;
        width_c.preferred_size = 200;
        width_c.min_size = 100;
        width_c.max_size = 300;
        source.set_width_constraint(width_c);

        size_constraint height_c;
        height_c.policy = size_policy::expand;
        height_c.weight = 2.5F;
        source.set_height_constraint(height_c);

        source.set_horizontal_align(horizontal_alignment::right);
        source.set_vertical_align(vertical_alignment::bottom);
        source.set_margin({10, 20, 30, 40});
        source.set_padding({5, 6, 7, 8});

        // Add a layout strategy
        source.set_layout_strategy(
            std::make_unique<linear_layout<test_backend>>(direction::horizontal, 15)
        );

        // Add children
        auto child1 = std::make_unique<test_element>(nullptr);
        auto child2 = std::make_unique<test_element>(nullptr);
        auto* child1_ptr = child1.get();
        auto* child2_ptr = child2.get();
        source.add_child(std::move(child1));
        source.add_child(std::move(child2));

        REQUIRE(source.children().size() == 2);
        REQUIRE(child1_ptr->parent() == &source);
        REQUIRE(child2_ptr->parent() == &source);

        // Move construct
        test_element dest(std::move(source));

        SUBCASE("destination has all source properties") {
            CHECK(dest.is_visible() == false);
            CHECK(dest.z_order() == 42);

            // Check width constraint
            const auto& w = dest.width_constraint();
            CHECK(w.policy == size_policy::fixed);
            CHECK(w.preferred_size == 200);
            CHECK(w.min_size == 100);
            CHECK(w.max_size == 300);

            // Check height constraint
            const auto& h = dest.height_constraint();
            CHECK(h.policy == size_policy::expand);
            CHECK(h.weight == 2.5F);

            CHECK(dest.h_align() == horizontal_alignment::right);
            CHECK(dest.v_align() == vertical_alignment::bottom);

            const auto& margin = dest.margin();
            CHECK(margin.left == 10);
            CHECK(margin.top == 20);
            CHECK(margin.right == 30);
            CHECK(margin.bottom == 40);

            const auto& padding = dest.padding();
            CHECK(padding.left == 5);
            CHECK(padding.top == 6);
            CHECK(padding.right == 7);
            CHECK(padding.bottom == 8);
        }

        SUBCASE("destination has layout strategy") {
            CHECK(dest.has_layout_strategy() == true);
        }

        SUBCASE("destination has children with correct parent pointers") {
            REQUIRE(dest.children().size() == 2);
            CHECK(dest.children()[0].get() == child1_ptr);
            CHECK(dest.children()[1].get() == child2_ptr);

            // CRITICAL: Children's parent pointers must point to destination
            CHECK(child1_ptr->parent() == &dest);
            CHECK(child2_ptr->parent() == &dest);
        }

        SUBCASE("source is left in valid state") {
            // Intentionally checking moved-from object state (valid but unspecified)
            // NOLINTNEXTLINE(bugprone-use-after-move)
            CHECK(source.parent() == nullptr);
            CHECK(source.children().empty());
            CHECK(source.has_layout_strategy() == false);
        }
    }

    TEST_CASE("ui_element - move assignment transfers all fields") {
        test_element source(nullptr);
        test_element dest(nullptr);

        // Set up destination with initial state
        dest.set_z_order(99);
        auto old_child = std::make_unique<test_element>(nullptr);
        auto* old_child_ptr = old_child.get();
        dest.add_child(std::move(old_child));

        // Set up source with different state
        source.set_visible(false);
        source.set_z_order(42);

        size_constraint c;
        c.policy = size_policy::weighted;
        c.weight = 3.14F;
        source.set_width_constraint(c);

        source.set_horizontal_align(horizontal_alignment::center);
        source.set_margin({1, 2, 3, 4});

        auto new_child1 = std::make_unique<test_element>(nullptr);
        auto new_child2 = std::make_unique<test_element>(nullptr);
        auto* new_child1_ptr = new_child1.get();
        auto* new_child2_ptr = new_child2.get();
        source.add_child(std::move(new_child1));
        source.add_child(std::move(new_child2));

        // Move assign
        dest = std::move(source);

        SUBCASE("destination has new properties") {
            CHECK(dest.is_visible() == false);
            CHECK(dest.z_order() == 42);

            const auto& w = dest.width_constraint();
            CHECK(w.policy == size_policy::weighted);
            CHECK(w.weight == 3.14F);

            CHECK(dest.h_align() == horizontal_alignment::center);

            const auto& margin = dest.margin();
            CHECK(margin.left == 1);
        }

        SUBCASE("destination has new children with correct parent pointers") {
            REQUIRE(dest.children().size() == 2);
            CHECK(dest.children()[0].get() == new_child1_ptr);
            CHECK(dest.children()[1].get() == new_child2_ptr);

            CHECK(new_child1_ptr->parent() == &dest);
            CHECK(new_child2_ptr->parent() == &dest);
        }

        SUBCASE("old destination children were properly destroyed") {
            // Old child should have been destroyed in the move assignment
            // We can't directly test this, but we can verify it's not in dest's children
            for (const auto& child : dest.children()) {
                CHECK(child.get() != old_child_ptr);
            }
        }

        SUBCASE("source is left in valid state") {
            // Intentionally checking moved-from object state (valid but unspecified)
            // NOLINTNEXTLINE(bugprone-use-after-move)
            CHECK(source.parent() == nullptr);
            CHECK(source.children().empty());
        }
    }

    TEST_CASE("ui_element - move operations with nested hierarchy") {
        test_element root(nullptr);

        // Create a hierarchy: root -> parent -> child
        auto parent = std::make_unique<test_element>(nullptr);
        auto* parent_ptr = parent.get();

        auto child = std::make_unique<test_element>(nullptr);
        auto* child_ptr = child.get();

        parent->add_child(std::move(child));
        root.add_child(std::move(parent));

        REQUIRE(root.children().size() == 1);
        REQUIRE(parent_ptr->parent() == &root);
        REQUIRE(parent_ptr->children().size() == 1);
        REQUIRE(child_ptr->parent() == parent_ptr);

        // Move the root
        test_element moved_root(std::move(root));

        SUBCASE("entire hierarchy is preserved") {
            REQUIRE(moved_root.children().size() == 1);
            CHECK(moved_root.children()[0].get() == parent_ptr);
            CHECK(parent_ptr->parent() == &moved_root);

            REQUIRE(parent_ptr->children().size() == 1);
            CHECK(parent_ptr->children()[0].get() == child_ptr);
            CHECK(child_ptr->parent() == parent_ptr);
        }

        SUBCASE("parent pointers are correctly updated at all levels") {
            // Top level
            CHECK(parent_ptr->parent() == &moved_root);

            // Second level - should NOT point to moved_root
            CHECK(child_ptr->parent() == parent_ptr);
            CHECK(child_ptr->parent() != &moved_root);
        }
    }

    TEST_CASE("ui_element - copy operations are deleted") {
        // These should not compile:
        // test_element source(nullptr);
        // test_element copy(source);  // Copy constructor deleted
        // test_element other(nullptr);
        // other = source;  // Copy assignment deleted

        // Test on concrete type since ui_element is abstract
        CHECK(std::is_copy_constructible_v<test_element> == false);
        CHECK(std::is_copy_assignable_v<test_element> == false);
    }

    TEST_CASE("ui_element - move operations are available") {
        // Test on concrete type since ui_element is abstract
        CHECK(std::is_move_constructible_v<test_element> == true);
        CHECK(std::is_move_assignable_v<test_element> == true);

        // Note: Move constructor is NOT noexcept because event_target<Backend> contains
        // std::function members which are not nothrow move constructible.
        // Move assignment IS noexcept due to defaulted implementation in event_target.
        // The conditional noexcept correctly reflects the base class capabilities.
        CHECK(std::is_nothrow_move_constructible_v<test_element> == false);
        CHECK(std::is_nothrow_move_assignable_v<test_element> == true);
    }

    // ========================================================================
    // event_target Move Operations Tests
    // ========================================================================

    TEST_CASE("event_target - move constructor transfers state") {
        test_event_target source;

        // Set up state
        source.set_focusable(true);
        source.set_enabled(true);
        source.set_tab_index(5);

        // Simulate mouse interaction to set pressed state
        mouse_event press{.x = 50, .y = 50, .btn = mouse_event::button::left, .act = mouse_event::action::press, .modifiers = {}};
        source.handle_event(ui_event{press}, event_phase::target);
        CHECK(source.is_pressed());

        // Move construct
        test_event_target dest(std::move(source));

        SUBCASE("destination retains state") {
            CHECK(dest.is_focusable());
            CHECK(dest.is_enabled());
            CHECK(dest.tab_index() == 5);
            CHECK(dest.is_pressed());  // Pressed state transferred
        }

        SUBCASE("destination can handle events") {
            // Complete the click
            mouse_event release{.x = 50, .y = 50, .btn = mouse_event::button::left, .act = mouse_event::action::release, .modifiers = {}};
            dest.handle_event(ui_event{release}, event_phase::target);
            CHECK(dest.click_count == 1);
        }
    }

    TEST_CASE("event_target - move constructor transfers state flags") {
        test_event_target source;

        // Set up state
        source.set_focusable(true);
        source.set_enabled(false);
        source.set_tab_index(42);

        // Move construct
        test_event_target const dest(std::move(source));

        SUBCASE("destination has correct state") {
            CHECK(dest.is_focusable() == true);
            CHECK(dest.is_enabled() == false);
            CHECK(dest.tab_index() == 42);
        }
    }

    TEST_CASE("event_target - move assignment transfers all state") {
        test_event_target source;
        test_event_target dest;

        // Set up source state
        source.set_focusable(true);
        source.set_enabled(false);
        source.set_tab_index(99);

        // Set up destination with different state
        dest.set_focusable(false);
        dest.set_enabled(true);
        dest.set_tab_index(1);

        // Move assign
        dest = std::move(source);

        SUBCASE("destination has source state") {
            CHECK(dest.is_focusable() == true);
            CHECK(dest.is_enabled() == false);
            CHECK(dest.tab_index() == 99);
        }

        SUBCASE("destination can handle events") {
            // Re-enable to allow event handling
            dest.set_enabled(true);

            mouse_event press{.x = 50, .y = 50, .btn = mouse_event::button::left, .act = mouse_event::action::press, .modifiers = {}};
            dest.handle_event(ui_event{press}, event_phase::target);

            mouse_event release{.x = 50, .y = 50, .btn = mouse_event::button::left, .act = mouse_event::action::release, .modifiers = {}};
            dest.handle_event(ui_event{release}, event_phase::target);

            CHECK(dest.click_count == 1);
        }
    }

    TEST_CASE("event_target - copy operations are deleted") {
        // Test on concrete type since event_target is abstract
        CHECK(std::is_copy_constructible_v<test_event_target> == false);
        CHECK(std::is_copy_assignable_v<test_event_target> == false);
    }

    TEST_CASE("event_target - move operations are available and noexcept") {
        // Test on concrete type since event_target is abstract
        CHECK(std::is_move_constructible_v<test_event_target> == true);
        CHECK(std::is_move_assignable_v<test_event_target> == true);

        // Note: test_event_target has defaulted move operations and no data members,
        // so it IS noexcept even though the base class event_target<Backend> contains
        // std::function members. The derived class's defaulted move constructor
        // is noexcept because it has no members that would make it non-noexcept.
        CHECK(std::is_nothrow_move_constructible_v<test_event_target> == true);
        CHECK(std::is_nothrow_move_assignable_v<test_event_target> == true);
    }

    // ========================================================================
    // focus_manager Move Operations Tests
    // ========================================================================

    TEST_CASE("focus_manager - move constructor transfers focused target") {
        focus_manager<test_backend> source;
        test_event_target target;

        target.set_focusable(true);
        source.set_focus(&target);

        REQUIRE(source.get_focused() == &target);

        // Move construct
        focus_manager<test_backend> const dest(std::move(source));

        SUBCASE("destination has focused target") {
            CHECK(dest.get_focused() == &target);
        }

        SUBCASE("source has null focused target") {
            // Intentionally checking moved-from object state (valid but unspecified)
            // NOLINTNEXTLINE(bugprone-use-after-move)
            CHECK(source.get_focused() == nullptr);
        }
    }

    TEST_CASE("focus_manager - move assignment transfers focused target") {
        focus_manager<test_backend> source;
        focus_manager<test_backend> dest;

        test_event_target target1;
        test_event_target target2;

        target1.set_focusable(true);
        target2.set_focusable(true);

        // Source has focus on target1
        source.set_focus(&target1);
        REQUIRE(source.get_focused() == &target1);

        // Dest has focus on target2
        dest.set_focus(&target2);
        REQUIRE(dest.get_focused() == &target2);

        // Move assign
        dest = std::move(source);

        SUBCASE("destination has source's focused target") {
            CHECK(dest.get_focused() == &target1);
        }

        SUBCASE("source has null focused target") {
            // Intentionally checking moved-from object state (valid but unspecified)
            // NOLINTNEXTLINE(bugprone-use-after-move)
            CHECK(source.get_focused() == nullptr);
        }
    }

    TEST_CASE("focus_manager - move operations transfer focus state") {
        focus_manager<test_backend> source;
        test_event_target target;

        target.set_focusable(true);

        source.set_focus(&target);
        CHECK(target.has_focus());

        SUBCASE("move constructor transfers focus") {
            focus_manager<test_backend> const dest(std::move(source));

            // Focus state transferred
            CHECK(dest.get_focused() == &target);
            CHECK(target.has_focus());
        }

        SUBCASE("move assignment transfers focus") {
            focus_manager<test_backend> dest;
            dest = std::move(source);

            // Focus state transferred
            CHECK(dest.get_focused() == &target);
            CHECK(target.has_focus());
        }
    }

    TEST_CASE("focus_manager - copy operations are deleted") {
        CHECK(std::is_copy_constructible_v<focus_manager<test_backend>> == false);
        CHECK(std::is_copy_assignable_v<focus_manager<test_backend>> == false);
    }

    TEST_CASE("focus_manager - move operations are available and noexcept") {
        CHECK(std::is_move_constructible_v<focus_manager<test_backend>> == true);
        CHECK(std::is_move_assignable_v<focus_manager<test_backend>> == true);
        CHECK(std::is_nothrow_move_constructible_v<focus_manager<test_backend>> == true);
        CHECK(std::is_nothrow_move_assignable_v<focus_manager<test_backend>> == true);
    }

    // ========================================================================
    // layout_strategy Rule of Five Tests
    // ========================================================================

    TEST_CASE("layout_strategy - copy and move operations are deleted") {
        using strategy_type = layout_strategy<test_backend>;

        // All copy and move operations should be deleted for polymorphic base
        CHECK(std::is_copy_constructible_v<strategy_type> == false);
        CHECK(std::is_copy_assignable_v<strategy_type> == false);
        CHECK(std::is_move_constructible_v<strategy_type> == false);
        CHECK(std::is_move_assignable_v<strategy_type> == false);
    }

    TEST_CASE("layout_strategy - concrete implementations can be moved via unique_ptr") {
        // While layout_strategy itself can't be moved, we use unique_ptr
        // which provides move semantics at the ownership level
        auto strategy1 = std::make_unique<linear_layout<test_backend>>(
            direction::horizontal, 10
        );

        auto strategy2 = std::move(strategy1);

        CHECK(strategy1 == nullptr);
        CHECK(strategy2 != nullptr);
    }

    // ========================================================================
    // Integration Tests - Move with Active UI
    // ========================================================================

    TEST_CASE("Integration - move ui_element with children") {
        test_element source(nullptr);

        // Add child
        auto child = std::make_unique<test_element>(nullptr);
        auto* child_ptr = child.get();
        source.add_child(std::move(child));

        // Move
        test_element dest(std::move(source));

        SUBCASE("moved element retains children and can handle events") {
            // Verify children moved
            CHECK(dest.children().size() == 1);
            CHECK(dest.children()[0].get() == child_ptr);

            // Verify element can handle events
            // Need to set bounds first for hit testing with unified event API
            dest.arrange(testing::make_relative_rect<TestBackend>(0, 0, 100, 100));
            mouse_event press{.x = 50, .y = 50, .btn = mouse_event::button::left, .act = mouse_event::action::press, .modifiers = {}};
            dest.handle_event(ui_event{press}, event_phase::target);
            CHECK(dest.is_pressed());
        }

        SUBCASE("child hierarchy still works after move") {
            CHECK(child_ptr->parent() == &dest);

            // Can still manipulate children
            [[maybe_unused]] auto removed = dest.remove_child(child_ptr);
            CHECK(dest.children().empty());
        }
    }

    TEST_CASE("Integration - move element with layout and measure/arrange") {
        test_element source(nullptr);

        // Set up layout
        source.set_layout_strategy(
            std::make_unique<linear_layout<test_backend>>(direction::vertical, 10)
        );

        // Add children
        auto child1 = std::make_unique<test_element>(nullptr);
        auto child2 = std::make_unique<test_element>(nullptr);
        source.add_child(std::move(child1));
        source.add_child(std::move(child2));

        // Measure source (may return 0 if no children - that's ok)
        auto size = source.measure(200, 200);
        (void)size;  // Suppress unused warning

        // Move
        test_element dest(std::move(source));

        SUBCASE("can still perform layout operations after move") {
            auto size2 = dest.measure(200, 200);
            // Size may be 0 if children have no intrinsic size - verify no crash
            (void)size2;

            auto bounds = testing::make_relative_rect<TestBackend>(0,0,200,200);


            // Should not crash
            CHECK_NOTHROW(dest.arrange(bounds));
        }
    }

    TEST_CASE("Integration - move focus_manager during active navigation") {
        focus_manager<test_backend> source;

        test_event_target target1, target2, target3;
        target1.set_focusable(true);
        target1.set_tab_index(0);
        target2.set_focusable(true);
        target2.set_tab_index(1);
        target3.set_focusable(true);
        target3.set_tab_index(2);

        std::vector<event_target<test_backend>*> const targets = {&target1, &target2, &target3};

        // Start navigation
        source.focus_next(targets);
        CHECK(source.get_focused() == &target1);

        // Move manager
        focus_manager<test_backend> dest(std::move(source));

        SUBCASE("navigation continues correctly after move") {
            dest.focus_next(targets);
            CHECK(dest.get_focused() == &target2);

            dest.focus_next(targets);
            CHECK(dest.get_focused() == &target3);

            dest.focus_previous(targets);
            CHECK(dest.get_focused() == &target2);
        }
    }
}
