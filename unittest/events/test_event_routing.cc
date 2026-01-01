/**
 * @file test_event_routing.cc
 * @brief Tests for three-phase event routing engine
 *
 * Tests the route_event() function and complete capture/target/bubble routing.
 * This is Phase 3 of the event routing implementation.
 */

#include <doctest/doctest.h>
#include <onyxui/events/event_router.hh>
#include <onyxui/events/event_phase.hh>
#include <onyxui/events/hit_test_path.hh>
#include <onyxui/core/element.hh>
#include <utils/test_backend.hh>
#include <vector>
#include <string>

using namespace onyxui;

// ======================================================================
// Test Helpers
// ======================================================================

/**
 * Test element that records which phases it received
 */
class tracking_element : public ui_element<test_backend> {
public:
    std::vector<event_phase> phases_received;
    std::string name;
    bool should_consume = false;
    event_phase consume_phase = event_phase::capture;  // Which phase to consume in

    explicit tracking_element(ui_element<test_backend>* parent = nullptr, const std::string& n = "unnamed")
        : ui_element<test_backend>(parent), name(n)
    {
    }

    // Bring both overloads into scope to avoid hiding base class methods
    using ui_element<test_backend>::handle_event;

    bool handle_event(const ui_event& evt, event_phase phase) override {
        (void)evt;  // Unused
        phases_received.push_back(phase);
        // Only consume in the specified phase
        return should_consume && (phase == consume_phase);
    }

    void reset() {
        phases_received.clear();
    }
};

// ======================================================================
// Basic Routing Tests
// ======================================================================

TEST_CASE("route_event() - Empty path") {
    hit_test_path<test_backend> path;

    mouse_event evt{
        .x = 10.0_lu,
        .y = 10.0_lu,
        .btn = mouse_event::button::left,
        .act = mouse_event::action::press,
        .modifiers = {.ctrl = false, .alt = false, .shift = false}
    };
    ui_event ui_evt = evt;

    bool handled = route_event(ui_evt, path);
    CHECK_FALSE(handled);  // Empty path = no handler
}

TEST_CASE("route_event() - Single element") {
    tracking_element root(nullptr, "root");
    hit_test_path<test_backend> path;
    path.push(&root);

    mouse_event evt{
        .x = 10.0_lu,
        .y = 10.0_lu,
        .btn = mouse_event::button::left,
        .act = mouse_event::action::press,
        .modifiers = {.ctrl = false, .alt = false, .shift = false}
    };
    ui_event ui_evt = evt;

    SUBCASE("All three phases delivered") {
        bool handled = route_event(ui_evt, path);
        CHECK_FALSE(handled);  // Not consumed

        // Single element gets all three phases
        REQUIRE(root.phases_received.size() == 3);
        CHECK(root.phases_received[0] == event_phase::capture);
        CHECK(root.phases_received[1] == event_phase::target);
        CHECK(root.phases_received[2] == event_phase::bubble);
    }

    SUBCASE("Consumption in capture phase") {
        root.reset();
        root.should_consume = true;

        bool handled = route_event(ui_evt, path);
        CHECK(handled);  // Consumed

        // Should stop after capture
        REQUIRE(root.phases_received.size() == 1);
        CHECK(root.phases_received[0] == event_phase::capture);
    }
}

TEST_CASE("route_event() - Three-phase ordering with multiple elements") {
    // Create hierarchy: root -> child1 -> child2
    tracking_element root(nullptr, "root");
    tracking_element child1(nullptr, "child1");
    tracking_element child2(nullptr, "child2");

    hit_test_path<test_backend> path;
    path.push(&root);
    path.push(&child1);
    path.push(&child2);

    mouse_event evt{
        .x = 10.0_lu,
        .y = 10.0_lu,
        .btn = mouse_event::button::left,
        .act = mouse_event::action::press,
        .modifiers = {.ctrl = false, .alt = false, .shift = false}
    };
    ui_event ui_evt = evt;

    SUBCASE("Capture phase: root → child1 → child2") {
        route_event(ui_evt, path);

        // Each element should receive at least capture
        REQUIRE(!root.phases_received.empty());
        REQUIRE(!child1.phases_received.empty());
        REQUIRE(!child2.phases_received.empty());

        // First phase for all should be capture
        CHECK(root.phases_received[0] == event_phase::capture);
        CHECK(child1.phases_received[0] == event_phase::capture);
        CHECK(child2.phases_received[0] == event_phase::capture);
    }

    SUBCASE("Target phase: only child2") {
        route_event(ui_evt, path);

        // Target should be in child2's phases
        bool child2_got_target = false;
        for (auto phase : child2.phases_received) {
            if (phase == event_phase::target) {
                child2_got_target = true;
                break;
            }
        }
        CHECK(child2_got_target);

        // Root and child1 should NOT get target phase
        for (auto phase : root.phases_received) {
            CHECK(phase != event_phase::target);
        }
        for (auto phase : child1.phases_received) {
            CHECK(phase != event_phase::target);
        }
    }

    SUBCASE("Bubble phase: child2 → child1 → root") {
        route_event(ui_evt, path);

        // All should receive bubble
        bool root_got_bubble = false;
        bool child1_got_bubble = false;
        bool child2_got_bubble = false;

        for (auto phase : root.phases_received) {
            if (phase == event_phase::bubble) root_got_bubble = true;
        }
        for (auto phase : child1.phases_received) {
            if (phase == event_phase::bubble) child1_got_bubble = true;
        }
        for (auto phase : child2.phases_received) {
            if (phase == event_phase::bubble) child2_got_bubble = true;
        }

        CHECK(root_got_bubble);
        CHECK(child1_got_bubble);
        CHECK(child2_got_bubble);
    }

    SUBCASE("Complete phase order") {
        route_event(ui_evt, path);

        // root: capture, bubble (no target)
        REQUIRE(root.phases_received.size() == 2);
        CHECK(root.phases_received[0] == event_phase::capture);
        CHECK(root.phases_received[1] == event_phase::bubble);

        // child1: capture, bubble (no target)
        REQUIRE(child1.phases_received.size() == 2);
        CHECK(child1.phases_received[0] == event_phase::capture);
        CHECK(child1.phases_received[1] == event_phase::bubble);

        // child2: capture, target, bubble (all three!)
        REQUIRE(child2.phases_received.size() == 3);
        CHECK(child2.phases_received[0] == event_phase::capture);
        CHECK(child2.phases_received[1] == event_phase::target);
        CHECK(child2.phases_received[2] == event_phase::bubble);
    }
}

TEST_CASE("route_event() - Event consumption stops propagation") {
    tracking_element root(nullptr, "root");
    tracking_element child1(nullptr, "child1");
    tracking_element child2(nullptr, "child2");

    hit_test_path<test_backend> path;
    path.push(&root);
    path.push(&child1);
    path.push(&child2);

    mouse_event evt{
        .x = 10.0_lu,
        .y = 10.0_lu,
        .btn = mouse_event::button::left,
        .act = mouse_event::action::press,
        .modifiers = {.ctrl = false, .alt = false, .shift = false}
    };
    ui_event ui_evt = evt;

    SUBCASE("Consumption in root's capture phase") {
        root.should_consume = true;

        bool handled = route_event(ui_evt, path);
        CHECK(handled);

        // Root gets capture, then stops
        REQUIRE(root.phases_received.size() == 1);
        CHECK(root.phases_received[0] == event_phase::capture);

        // Children get nothing
        CHECK(child1.phases_received.empty());
        CHECK(child2.phases_received.empty());
    }

    SUBCASE("Consumption in child1's capture phase") {
        child1.should_consume = true;

        bool handled = route_event(ui_evt, path);
        CHECK(handled);

        // Root gets capture (passes through)
        REQUIRE(root.phases_received.size() == 1);
        CHECK(root.phases_received[0] == event_phase::capture);

        // child1 gets capture, then stops
        REQUIRE(child1.phases_received.size() == 1);
        CHECK(child1.phases_received[0] == event_phase::capture);

        // child2 gets nothing (stopped before reaching it)
        CHECK(child2.phases_received.empty());
    }

    SUBCASE("Consumption in target phase") {
        child2.should_consume = true;
        child2.consume_phase = event_phase::target;  // Consume in target phase specifically

        bool handled = route_event(ui_evt, path);
        CHECK(handled);

        // All get capture (root, child1, child2)
        REQUIRE(root.phases_received.size() == 1);
        REQUIRE(child1.phases_received.size() == 1);
        CHECK(root.phases_received[0] == event_phase::capture);
        CHECK(child1.phases_received[0] == event_phase::capture);

        // child2 gets capture and target, then stops (no bubble)
        REQUIRE(child2.phases_received.size() == 2);
        CHECK(child2.phases_received[0] == event_phase::capture);
        CHECK(child2.phases_received[1] == event_phase::target);

        // No bubble phases delivered
    }

    SUBCASE("Consumption in bubble phase") {
        child2.reset();
        child1.reset();
        root.reset();

        // Only consume in bubble phase (not capture or target)
        child1.should_consume = true;
        child1.consume_phase = event_phase::bubble;  // Consume in bubble phase specifically

        bool handled = route_event(ui_evt, path);
        CHECK(handled);

        // All get capture
        CHECK(root.phases_received[0] == event_phase::capture);
        CHECK(child1.phases_received[0] == event_phase::capture);
        CHECK(child2.phases_received[0] == event_phase::capture);

        // child2 gets target (it's the target)
        CHECK(child2.phases_received[1] == event_phase::target);

        // child2 gets bubble, then child1 gets bubble and consumes
        CHECK(child2.phases_received[2] == event_phase::bubble);
        REQUIRE(child1.phases_received.size() == 2);
        CHECK(child1.phases_received[1] == event_phase::bubble);

        // root should NOT get bubble (stopped at child1)
        REQUIRE(root.phases_received.size() == 1);  // Only capture
    }
}

TEST_CASE("route_event() - Different event types") {
    tracking_element root(nullptr, "root");
    tracking_element child(nullptr, "child");

    hit_test_path<test_backend> path;
    path.push(&root);
    path.push(&child);

    SUBCASE("Keyboard event") {
        keyboard_event kbd{
            .key = key_code::a,
            .modifiers = key_modifier::none,
            .pressed = true
        };
        ui_event ui_evt = kbd;

        bool handled = route_event(ui_evt, path);
        CHECK_FALSE(handled);

        // Both should receive all three phases
        REQUIRE(root.phases_received.size() == 2);  // capture, bubble (no target)
        REQUIRE(child.phases_received.size() == 3);  // capture, target, bubble
    }

    SUBCASE("Resize event") {
        root.reset();
        child.reset();

        resize_event resize{
            .width = 800,
            .height = 600
        };
        ui_event ui_evt = resize;

        bool handled = route_event(ui_evt, path);
        CHECK_FALSE(handled);

        // Both should receive all phases
        REQUIRE(root.phases_received.size() == 2);
        REQUIRE(child.phases_received.size() == 3);
    }

    SUBCASE("Mouse wheel event") {
        root.reset();
        child.reset();

        mouse_event wheel{
            .x = 50.0_lu,
            .y = 50.0_lu,
            .btn = mouse_event::button::none,
            .act = mouse_event::action::wheel_up,
            .modifiers = {.ctrl = false, .alt = false, .shift = false}
        };
        ui_event ui_evt = wheel;

        bool handled = route_event(ui_evt, path);
        CHECK_FALSE(handled);

        // Both should receive all phases
        REQUIRE(root.phases_received.size() == 2);
        REQUIRE(child.phases_received.size() == 3);
    }
}

TEST_CASE("route_event_custom() - Custom phase order") {
    tracking_element root(nullptr, "root");
    tracking_element child(nullptr, "child");

    hit_test_path<test_backend> path;
    path.push(&root);
    path.push(&child);

    mouse_event evt{
        .x = 10.0_lu,
        .y = 10.0_lu,
        .btn = mouse_event::button::left,
        .act = mouse_event::action::press,
        .modifiers = {.ctrl = false, .alt = false, .shift = false}
    };
    ui_event ui_evt = evt;

    SUBCASE("Only capture phase") {
        event_phase phases[] = {event_phase::capture};
        bool handled = route_event_custom(ui_evt, path, phases, 1);
        CHECK_FALSE(handled);

        // Both should only receive capture
        REQUIRE(root.phases_received.size() == 1);
        REQUIRE(child.phases_received.size() == 1);
        CHECK(root.phases_received[0] == event_phase::capture);
        CHECK(child.phases_received[0] == event_phase::capture);
    }

    SUBCASE("Only target phase") {
        root.reset();
        child.reset();

        event_phase phases[] = {event_phase::target};
        bool handled = route_event_custom(ui_evt, path, phases, 1);
        CHECK_FALSE(handled);

        // Only child (target) should receive event
        CHECK(root.phases_received.empty());
        REQUIRE(child.phases_received.size() == 1);
        CHECK(child.phases_received[0] == event_phase::target);
    }

    SUBCASE("Capture then target (no bubble)") {
        root.reset();
        child.reset();

        event_phase phases[] = {event_phase::capture, event_phase::target};
        bool handled = route_event_custom(ui_evt, path, phases, 2);
        CHECK_FALSE(handled);

        // Root gets capture only
        REQUIRE(root.phases_received.size() == 1);
        CHECK(root.phases_received[0] == event_phase::capture);

        // Child gets capture and target
        REQUIRE(child.phases_received.size() == 2);
        CHECK(child.phases_received[0] == event_phase::capture);
        CHECK(child.phases_received[1] == event_phase::target);
    }

    SUBCASE("Reversed order: target then capture") {
        root.reset();
        child.reset();

        event_phase phases[] = {event_phase::target, event_phase::capture};
        bool handled = route_event_custom(ui_evt, path, phases, 2);
        CHECK_FALSE(handled);

        // Target phase first (only child)
        REQUIRE(child.phases_received.size() == 2);
        CHECK(child.phases_received[0] == event_phase::target);
        CHECK(child.phases_received[1] == event_phase::capture);

        // Root only gets capture (from second phase)
        REQUIRE(root.phases_received.size() == 1);
        CHECK(root.phases_received[0] == event_phase::capture);
    }
}
