/**
 * @file test_layer_manager_lifetime.cc
 * @brief Tests for layer manager lifetime safety and dangling pointer scenarios
 * @author Claude Code
 * @date October 19, 2025
 *
 * @details
 * This test suite verifies that the layer manager handles widget lifetime correctly.
 * Many of these tests will FAIL initially and are expected to PASS after implementing
 * lifetime tracking with weak_ptr in Phase 1.2.
 *
 * EXPECTED FAILURES (Phase 1.1 Baseline):
 * - All "Dangling pointer" tests (expect segfaults)
 * - "Multiple removes" tests (may pass, but verify)
 * - "Invalid ID" tests (should pass, but verify error handling)
 */

#include <doctest/doctest.h>
#include <../../include/onyxui/services/layer_manager.hh>
#include "../utils/test_helpers.hh"
#include "utils/test_backend.hh"
#include <memory>

using namespace onyxui;

// Type aliases for test backend types
using Backend = test_backend;
using TestEvent = test_backend::event_type;
using TestRenderer = test_backend::renderer_type;
using TestRect = test_backend::rect_type;
// TestElement is already defined in test_helpers.hh

/**
 * @class SelfRemovingElement
 * @brief Element that removes its own layer when it receives an event
 */
class SelfRemovingElement : public TestElement {
public:
    explicit SelfRemovingElement(layer_manager<Backend>* mgr)
        : TestElement(nullptr)
        , m_manager(mgr)
        , m_layer_id(layer_id::invalid()) {
        arrange(TestRect{0, 0, 100, 50});
    }

    void set_layer_id(layer_id id) { m_layer_id = id; }

    bool process_event(const TestEvent& /*event*/) override {
        // Remove self during event processing
        if (m_manager && m_layer_id.is_valid()) {
            m_manager->remove_layer(m_layer_id);
            m_layer_id = layer_id::invalid();
        }
        return true;
    }

private:
    layer_manager<Backend>* m_manager;
    layer_id m_layer_id;
};

/**
 * @class EventCountingElement
 * @brief Element that counts how many events it receives
 */
class EventCountingElement : public TestElement {
public:
    explicit EventCountingElement() : TestElement(nullptr), event_count(0) {
        arrange(TestRect{0, 0, 100, 50});
    }

    bool process_event(const TestEvent& /*event*/) override {
        ++event_count;
        return false;  // Don't handle, let it propagate
    }

    int event_count;
};

// ============================================================================
// Test Suite 1: Dangling Pointer Detection
// ============================================================================

TEST_SUITE("Layer Manager - Lifetime Safety") {

    TEST_CASE("Dangling pointer detection - basic case" * doctest::skip()) {
        // Create minimal theme for testing (this test is skipped anyway)
        ui_theme<Backend> test_theme;
        test_theme.name = "TestTheme";

        SUBCASE("Destroy widget then route event") {
            layer_manager<Backend> mgr;
            auto elem = std::make_shared<TestElement>();  // Phase 1.2: Use shared_ptr

            (void)mgr.add_layer(layer_type::popup, elem);  // New safe API
            CHECK(mgr.layer_count() == 1);

            // Destroy element while layer exists
            elem.reset();

            // Phase 1.2: Automatic cleanup of expired layers
            TestEvent const event;
            CHECK_NOTHROW(mgr.route_event(event));  // Should auto-cleanup and not crash

            // Verify layer was removed
            CHECK(mgr.layer_count() == 0);
        }

        SUBCASE("Destroy widget then render") {
            layer_manager<Backend> mgr;
            auto elem = std::make_shared<TestElement>();  // Phase 1.2: Use shared_ptr

            (void)mgr.add_layer(layer_type::popup, elem);  // New safe API
            elem.reset();  // Destroy element

            TestRenderer renderer;
            TestRect const viewport{0, 0, 800, 600};

            // Phase 1.2: Automatic cleanup of expired layers
            CHECK_NOTHROW(mgr.render_all_layers(renderer, viewport, &test_theme));

            // Verify layer was removed
            CHECK(mgr.layer_count() == 0);
        }

        SUBCASE("Destroy widget then query visibility") {
            layer_manager<Backend> mgr;
            auto elem = std::make_shared<TestElement>();  // Phase 1.2: Use shared_ptr

            layer_id const id = mgr.add_layer(layer_type::popup, elem);  // New safe API
            elem.reset();

            // Phase 1.2: Should handle gracefully (expired layer returns false)
            CHECK_FALSE(mgr.is_layer_visible(id));

            // Query doesn't cleanup, but layer is logically expired
            // Cleanup happens on next route_event() or render_all_layers()
            TestEvent const event;
            mgr.route_event(event);
            CHECK(mgr.layer_count() == 0);
        }
    }

    TEST_CASE("Dangling pointer - multiple widgets destroyed" * doctest::skip()) {
        layer_manager<Backend> mgr;

        auto elem1 = std::make_shared<TestElement>();  // Phase 1.2: Use shared_ptr
        auto elem2 = std::make_shared<TestElement>();
        auto elem3 = std::make_shared<TestElement>();

        mgr.add_layer(layer_type::popup, elem1);  // New safe API
        mgr.add_layer(layer_type::dialog, elem2);
        mgr.add_layer(layer_type::tooltip, elem3);

        CHECK(mgr.layer_count() == 3);

        // Destroy some elements
        elem1.reset();
        elem3.reset();

        TestEvent const event;
        // Phase 1.2: Should auto-cleanup expired layers, only route to elem2
        CHECK_NOTHROW(mgr.route_event(event));

        // Verify expired layers were removed (elem1 and elem3)
        CHECK(mgr.layer_count() == 1);
    }

    TEST_CASE("Dangling pointer - during iteration" * doctest::skip()) {
        SUBCASE("First element destroyed") {
            layer_manager<Backend> mgr;
            auto elem1 = std::make_shared<EventCountingElement>();
            auto elem2 = std::make_shared<EventCountingElement>();
            auto elem3 = std::make_shared<EventCountingElement>();

            mgr.add_layer(layer_type::base, elem1, 0);
            mgr.add_layer(layer_type::popup, elem2, 100);
            mgr.add_layer(layer_type::tooltip, elem3, 200);

            elem1.reset();  // Destroy first (lowest z-index)

            TestEvent const event;
            CHECK_NOTHROW(mgr.route_event(event));

            // After Phase 1.2, only elem2 and elem3 should receive event
            // Currently: EXPECTED TO FAIL
        }

        SUBCASE("Middle element destroyed") {
            layer_manager<Backend> mgr;
            auto elem1 = std::make_shared<EventCountingElement>();
            auto elem2 = std::make_shared<EventCountingElement>();
            auto elem3 = std::make_shared<EventCountingElement>();

            mgr.add_layer(layer_type::base, elem1, 0);
            mgr.add_layer(layer_type::popup, elem2, 100);
            mgr.add_layer(layer_type::tooltip, elem3, 200);

            elem2.reset();  // Destroy middle

            TestEvent const event;
            CHECK_NOTHROW(mgr.route_event(event));
        }

        SUBCASE("Last element destroyed") {
            layer_manager<Backend> mgr;
            auto elem1 = std::make_shared<EventCountingElement>();
            auto elem2 = std::make_shared<EventCountingElement>();
            auto elem3 = std::make_shared<EventCountingElement>();

            mgr.add_layer(layer_type::base, elem1, 0);
            mgr.add_layer(layer_type::popup, elem2, 100);
            mgr.add_layer(layer_type::tooltip, elem3, 200);

            elem3.reset();  // Destroy last (highest z-index)

            TestEvent const event;
            CHECK_NOTHROW(mgr.route_event(event));
        }
    }

    TEST_CASE("Dangling pointer - rendering scenarios" * doctest::skip()) {
        // Create minimal theme for testing (this test is skipped anyway)
        ui_theme<Backend> test_theme;
        test_theme.name = "TestTheme";


        // SKIPPED: These tests document known limitations of non-owning pointers.
        // show_popup/show_tooltip/show_modal_dialog use non-owning shared_ptr,
        // so if the caller incorrectly destroys the element, we can't detect it.
        //
        // CORRECT USAGE:
        // 1. Keep element alive while layer exists, OR
        // 2. Remove layer before destroying element, OR
        // 3. Use add_scoped_layer() for automatic lifetime management

        SUBCASE("Positioning after destruction") {
            layer_manager<Backend> mgr;
            auto elem = std::make_shared<TestElement>();

            TestRect const anchor{10, 10, 50, 20};
            (void)mgr.show_popup(elem.get(), anchor, popup_placement::below);

            elem.reset();

            TestRenderer renderer;
            TestRect const viewport{0, 0, 800, 600};

            // Should handle gracefully (skip expired layer)
            // Currently: EXPECTED TO FAIL
            CHECK_NOTHROW(mgr.render_all_layers(renderer, viewport, &test_theme));
        }

        SUBCASE("Modal dialog after destruction") {
            layer_manager<Backend> mgr;
            auto dialog = std::make_shared<TestElement>();

            (void)mgr.show_modal_dialog(dialog.get(), dialog_position::center);
            dialog.reset();

            TestRenderer renderer;
            TestRect const viewport{0, 0, 800, 600};

            CHECK_NOTHROW(mgr.render_all_layers(renderer, viewport, &test_theme));
        }
    }

    TEST_CASE("Dangling pointer - tooltip scenarios" * doctest::skip()) {
        // Create minimal theme for testing (this test is skipped anyway)
        ui_theme<Backend> test_theme;
        test_theme.name = "TestTheme";


        layer_manager<Backend> mgr;
        auto tooltip = std::make_shared<TestElement>();

        layer_id const id = mgr.show_tooltip(tooltip.get(), 100, 100);
        tooltip.reset();

        SUBCASE("Route event after tooltip destroyed") {
            TestEvent const event;
            CHECK_NOTHROW(mgr.route_event(event));
        }

        SUBCASE("Render after tooltip destroyed") {
            TestRenderer renderer;
            TestRect const viewport{0, 0, 800, 600};
            CHECK_NOTHROW(mgr.render_all_layers(renderer, viewport, &test_theme));
        }

        SUBCASE("Hide after tooltip destroyed") {
            CHECK_NOTHROW(mgr.hide_layer(id));
        }

        SUBCASE("Show after tooltip destroyed") {
            CHECK_NOTHROW(mgr.show_layer(id));
        }
    }
}

// ============================================================================
// Test Suite 2: Multiple Layer Removal
// ============================================================================

TEST_SUITE("Layer Manager - Multiple Removals") {

    TEST_CASE("Remove same layer multiple times") {
        layer_manager<Backend> mgr;
        auto elem = std::make_shared<TestElement>();

        layer_id const id = mgr.add_layer(layer_type::popup, elem);
        CHECK(mgr.layer_count() == 1);

        mgr.remove_layer(id);
        CHECK(mgr.layer_count() == 0);

        // Should be safe to call multiple times
        CHECK_NOTHROW(mgr.remove_layer(id));
        CHECK_NOTHROW(mgr.remove_layer(id));
        CHECK_NOTHROW(mgr.remove_layer(id));
        CHECK(mgr.layer_count() == 0);
    }

    TEST_CASE("Remove invalid layer ID") {
        layer_manager<Backend> mgr;

        layer_id const invalid = layer_id::invalid();
        CHECK_NOTHROW(mgr.remove_layer(invalid));

        layer_id const non_existent(999999);
        CHECK_NOTHROW(mgr.remove_layer(non_existent));
    }

    TEST_CASE("Remove all layers of type - no layers exist") {
        layer_manager<Backend> mgr;

        // Should be safe even with no layers
        CHECK_NOTHROW(mgr.clear_layers(layer_type::popup));
        CHECK_NOTHROW(mgr.clear_layers(layer_type::modal));
        CHECK_NOTHROW(mgr.clear_layers(layer_type::tooltip));
    }

    TEST_CASE("Remove all layers - empty manager") {
        layer_manager<Backend> mgr;

        CHECK_NOTHROW(mgr.clear_all_layers());
        CHECK(mgr.layer_count() == 0);

        // Should be safe to call multiple times
        CHECK_NOTHROW(mgr.clear_all_layers());
        CHECK(mgr.layer_count() == 0);
    }
}

// ============================================================================
// Test Suite 3: Invalid ID Handling
// ============================================================================

TEST_SUITE("Layer Manager - Invalid ID Handling") {

    TEST_CASE("Query visibility of invalid IDs") {
        layer_manager<Backend> const mgr;

        layer_id const invalid = layer_id::invalid();
        CHECK_FALSE(mgr.is_layer_visible(invalid));

        layer_id const non_existent(888888);
        CHECK_FALSE(mgr.is_layer_visible(non_existent));
    }

    TEST_CASE("Show/hide invalid IDs") {
        layer_manager<Backend> mgr;

        layer_id const invalid = layer_id::invalid();
        CHECK_NOTHROW(mgr.show_layer(invalid));
        CHECK_NOTHROW(mgr.hide_layer(invalid));

        layer_id const non_existent(777777);
        CHECK_NOTHROW(mgr.show_layer(non_existent));
        CHECK_NOTHROW(mgr.hide_layer(non_existent));
    }

    TEST_CASE("Operations after layer removed") {
        layer_manager<Backend> mgr;
        auto elem = std::make_shared<TestElement>();

        layer_id const id = mgr.add_layer(layer_type::popup, elem);
        mgr.remove_layer(id);

        // All operations should handle gracefully
        CHECK_NOTHROW(mgr.show_layer(id));
        CHECK_NOTHROW(mgr.hide_layer(id));
        CHECK_FALSE(mgr.is_layer_visible(id));
        CHECK_NOTHROW(mgr.remove_layer(id));  // Already removed
    }
}

// ============================================================================
// Test Suite 4: Cleanup and Memory Safety
// ============================================================================

TEST_SUITE("Layer Manager - Cleanup Safety") {

    TEST_CASE("Clear layers while widgets alive") {
        layer_manager<Backend> mgr;

        auto elem1 = std::make_shared<TestElement>();
        auto elem2 = std::make_shared<TestElement>();
        auto elem3 = std::make_shared<TestElement>();

        mgr.add_layer(layer_type::popup, elem1);
        mgr.add_layer(layer_type::popup, elem2);
        mgr.add_layer(layer_type::dialog, elem3);

        CHECK(mgr.layer_count() == 3);

        mgr.clear_layers(layer_type::popup);
        CHECK(mgr.layer_count() == 1);

        // Widgets should still be valid
        CHECK(elem1 != nullptr);
        CHECK(elem2 != nullptr);
        CHECK(elem3 != nullptr);
    }

    TEST_CASE("Clear all layers then destroy widgets") {
        layer_manager<Backend> mgr;

        auto elem1 = std::make_shared<TestElement>();
        auto elem2 = std::make_shared<TestElement>();

        mgr.add_layer(layer_type::popup, elem1);
        mgr.add_layer(layer_type::modal, elem2);

        mgr.clear_all_layers();
        CHECK(mgr.layer_count() == 0);

        // Should be safe to destroy widgets after clearing layers
        CHECK_NOTHROW(elem1.reset());
        CHECK_NOTHROW(elem2.reset());
    }

    TEST_CASE("Layer manager destroyed before widgets") {
        auto elem1 = std::make_shared<TestElement>();
        auto elem2 = std::make_shared<TestElement>();

        {
            layer_manager<Backend> mgr;
            mgr.add_layer(layer_type::popup, elem1);
            mgr.add_layer(layer_type::modal, elem2);
        }  // mgr destroyed here

        // Widgets should still be valid
        CHECK(elem1 != nullptr);
        CHECK(elem2 != nullptr);
        CHECK_NOTHROW(elem1.reset());
        CHECK_NOTHROW(elem2.reset());
    }

    TEST_CASE("Widgets destroyed before layer manager") {
        layer_manager<Backend> mgr;

        {
            auto elem1 = std::make_shared<TestElement>();  // Use shared_ptr for Phase 1.2
            auto elem2 = std::make_shared<TestElement>();

            mgr.add_layer(layer_type::popup, elem1);  // New safe API
            mgr.add_layer(layer_type::modal, elem2);
        }  // Widgets destroyed here (shared_ptr goes out of scope)

        // Phase 1.2: Automatic cleanup of expired layers
        TestEvent const event;
        CHECK_NOTHROW(mgr.route_event(event));  // Should auto-cleanup expired layers

        // Verify layers were cleaned up
        CHECK(mgr.layer_count() == 0);
    }
}

// ============================================================================
// Test Suite 5: Self-Removal During Event Processing
// ============================================================================

TEST_SUITE("Layer Manager - Self-Removal") {

    TEST_CASE("Element removes itself during event processing") {
        layer_manager<Backend> mgr;

        auto self_removing = std::make_shared<SelfRemovingElement>(&mgr);

        layer_id const id = mgr.add_layer(layer_type::popup, self_removing);
        self_removing->set_layer_id(id);
        CHECK(mgr.layer_count() == 1);

        TestEvent const event;
        // Element will remove itself when it processes the event
        // Should not crash from iterator invalidation
        CHECK_NOTHROW(mgr.route_event(event));

        // Layer should be removed
        CHECK(mgr.layer_count() == 0);
    }

    TEST_CASE("Multiple self-removing elements") {
        layer_manager<Backend> mgr;

        auto elem1 = std::make_shared<SelfRemovingElement>(&mgr);
        auto elem2 = std::make_shared<SelfRemovingElement>(&mgr);
        auto elem3 = std::make_shared<SelfRemovingElement>(&mgr);

        layer_id const id1 = mgr.add_layer(layer_type::popup, elem1, 100);
        layer_id const id2 = mgr.add_layer(layer_type::popup, elem2, 200);
        layer_id const id3 = mgr.add_layer(layer_type::popup, elem3, 300);

        elem1->set_layer_id(id1);
        elem2->set_layer_id(id2);
        elem3->set_layer_id(id3);

        CHECK(mgr.layer_count() == 3);

        TestEvent const event;
        // Highest z-index (elem3) will remove itself first
        // Should not crash from concurrent modification
        CHECK_NOTHROW(mgr.route_event(event));

        // Only one layer should be removed (first handler wins)
        CHECK(mgr.layer_count() == 2);
    }
}
