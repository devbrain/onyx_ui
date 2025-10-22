/**
 * @file test_scoped_layer.cc
 * @brief Tests for RAII scoped_layer handle
 * @author Claude Code
 * @date October 19, 2025
 *
 * @details
 * Tests for the scoped_layer RAII handle that will be implemented in Phase 1.4.
 * All tests will FAIL initially until scoped_layer.hh is created.
 *
 * EXPECTED FAILURES (Phase 1.1-1.3 Baseline):
 * - All tests (scoped_layer doesn't exist yet)
 *
 * After Phase 1.4, all tests should PASS.
 */

#include <doctest/doctest.h>
#include <onyxui/layer_manager.hh>
#include <onyxui/scoped_layer.hh>  // Phase 1.4: Now implemented!
#include "../utils/test_helpers.hh"
#include "utils/test_backend.hh"
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace onyxui;

// Type aliases
using Backend = test_backend;
// TestElement is already defined in test_helpers.hh
using TestEvent = test_backend::event_type;
using TestRect = test_backend::rect_type;
using TestRenderer = test_backend::renderer_type;

// ============================================================================
// Test Suite 1: Basic RAII Behavior
// ============================================================================

// Phase 1.4: scoped_layer.hh has been implemented!
// All tests should now compile and pass.

TEST_SUITE("scoped_layer - RAII Behavior") {

    TEST_CASE("scoped_layer - automatic cleanup on scope exit") {
        layer_manager<Backend> mgr;
        auto elem = std::make_shared<TestElement>();

        {
            auto layer = mgr.add_scoped_layer(layer_type::popup, elem);
            CHECK(layer.is_valid());
            CHECK(mgr.layer_count() == 1);
        }  // layer destroyed here

        // Layer should be automatically removed
        CHECK(mgr.layer_count() == 0);
    }

    TEST_CASE("scoped_layer - multiple scoped layers") {
        layer_manager<Backend> mgr;

        auto elem1 = std::make_shared<TestElement>();
        auto elem2 = std::make_shared<TestElement>();
        auto elem3 = std::make_shared<TestElement>();

        {
            auto layer1 = mgr.add_scoped_layer(layer_type::popup, elem1);
            auto layer2 = mgr.add_scoped_layer(layer_type::dialog, elem2);
            auto layer3 = mgr.add_scoped_layer(layer_type::tooltip, elem3);

            CHECK(mgr.layer_count() == 3);
        }  // All layers destroyed here

        CHECK(mgr.layer_count() == 0);
    }

    TEST_CASE("scoped_layer - nested scopes") {
        layer_manager<Backend> mgr;

        auto elem1 = std::make_shared<TestElement>();
        auto elem2 = std::make_shared<TestElement>();

        {
            auto layer1 = mgr.add_scoped_layer(layer_type::popup, elem1);
            CHECK(mgr.layer_count() == 1);

            {
                auto layer2 = mgr.add_scoped_layer(layer_type::dialog, elem2);
                CHECK(mgr.layer_count() == 2);
            }  // layer2 destroyed

            CHECK(mgr.layer_count() == 1);
        }  // layer1 destroyed

        CHECK(mgr.layer_count() == 0);
    }

    TEST_CASE("scoped_layer - exception safety") {
        layer_manager<Backend> mgr;

        auto elem = std::make_shared<TestElement>();

        try {
            auto layer = mgr.add_scoped_layer(layer_type::popup, elem);
            CHECK(mgr.layer_count() == 1);

            throw std::runtime_error("Test exception");
        } catch (const std::runtime_error&) {
            // Layer should still be cleaned up
        }

        CHECK(mgr.layer_count() == 0);
    }
}

// ============================================================================
// Test Suite 2: Move Semantics
// ============================================================================

TEST_SUITE("scoped_layer - Move Semantics") {

    TEST_CASE("scoped_layer - move construction") {
        layer_manager<Backend> mgr;
        auto elem = std::make_shared<TestElement>();

        auto layer1 = mgr.add_scoped_layer(layer_type::popup, elem);
        layer_id id = layer1.get();

        CHECK(layer1.is_valid());
        CHECK(mgr.layer_count() == 1);

        // Move construct
        auto layer2(std::move(layer1));

        // layer1 should be invalid after move
        // Intentionally checking moved-from object state (RAII validity check)
        // NOLINTNEXTLINE(bugprone-use-after-move)
        CHECK_FALSE(layer1.is_valid());

        // layer2 should own the layer
        CHECK(layer2.is_valid());
        CHECK(layer2.get() == id);
        CHECK(mgr.layer_count() == 1);

        // When layer2 destroyed, layer should be removed
    }

    TEST_CASE("scoped_layer - move assignment") {
        layer_manager<Backend> mgr;

        auto elem1 = std::make_shared<TestElement>();
        auto elem2 = std::make_shared<TestElement>();

        auto layer1 = mgr.add_scoped_layer(layer_type::popup, elem1);
        layer_id id1 = layer1.get();

        auto layer2 = mgr.add_scoped_layer(layer_type::dialog, elem2);

        CHECK(mgr.layer_count() == 2);

        // Move assign layer1 to layer2
        layer2 = std::move(layer1);

        // layer2's original layer should be removed during move assignment
        // layer1's new state should be invalid
        // layer2 should now own id1

        // Intentionally checking moved-from object state (RAII validity check)
        // NOLINTNEXTLINE(bugprone-use-after-move)
        CHECK_FALSE(layer1.is_valid());
        CHECK(layer2.is_valid());
        CHECK(layer2.get() == id1);
        CHECK(mgr.layer_count() == 1);
    }

    TEST_CASE("scoped_layer - move to vector") {
        layer_manager<Backend> mgr;

        std::vector<scoped_layer<Backend>> layers;

        auto elem1 = std::make_shared<TestElement>();
        auto elem2 = std::make_shared<TestElement>();

        layers.push_back(mgr.add_scoped_layer(layer_type::popup, elem1));
        layers.push_back(mgr.add_scoped_layer(layer_type::dialog, elem2));

        CHECK(mgr.layer_count() == 2);
        CHECK(layers.size() == 2);

        layers.clear();

        CHECK(mgr.layer_count() == 0);
    }

    TEST_CASE("scoped_layer - return from function") {
        layer_manager<Backend> mgr;

        auto create_layer = [&mgr]() -> scoped_layer<Backend> {
            auto elem = std::make_shared<TestElement>();
            return mgr.add_scoped_layer(layer_type::popup, elem);
        };

        {
            auto layer = create_layer();
            CHECK(layer.is_valid());
            CHECK(mgr.layer_count() == 1);
        }

        CHECK(mgr.layer_count() == 0);
    }
}

// ============================================================================
// Test Suite 3: Manual Control
// ============================================================================

TEST_SUITE("scoped_layer - Manual Control") {

    TEST_CASE("scoped_layer - reset() manual cleanup") {
        layer_manager<Backend> mgr;
        auto elem = std::make_shared<TestElement>();

        auto layer = mgr.add_scoped_layer(layer_type::popup, elem);
        CHECK(mgr.layer_count() == 1);

        layer.reset();

        CHECK_FALSE(layer.is_valid());
        CHECK(mgr.layer_count() == 0);

        // reset() should be idempotent
        layer.reset();
        CHECK(mgr.layer_count() == 0);
    }

    TEST_CASE("scoped_layer - release() gives up ownership") {
        layer_manager<Backend> mgr;
        auto elem = std::make_shared<TestElement>();

        auto layer = mgr.add_scoped_layer(layer_type::popup, elem);
        layer_id id = layer.get();

        CHECK(layer.is_valid());
        CHECK(mgr.layer_count() == 1);

        // Release ownership
        layer_id const released_id = layer.release();

        CHECK(released_id == id);
        CHECK_FALSE(layer.is_valid());
        CHECK(mgr.layer_count() == 1);  // Layer still exists

        // When layer destroyed, should NOT remove the layer
        // (ownership was released)
    }

    TEST_CASE("scoped_layer - get() returns ID") {
        layer_manager<Backend> mgr;
        auto elem = std::make_shared<TestElement>();

        auto layer = mgr.add_scoped_layer(layer_type::popup, elem);
        layer_id const id = layer.get();

        CHECK(id.is_valid());
        CHECK(mgr.is_layer_visible(id));
    }
}

// ============================================================================
// Test Suite 4: Integration with layer_manager
// ============================================================================

TEST_SUITE("scoped_layer - Integration") {

    TEST_CASE("scoped_layer - with show_popup") {
        layer_manager<Backend> mgr;
        auto elem = std::make_shared<TestElement>();

        TestRect const anchor{100, 100, 50, 20};

        {
            // Note: Will need to add show_popup overload that returns scoped_layer
            // Or create manually:
            layer_id const id = mgr.show_popup(elem.get(), anchor, popup_placement::below);
            scoped_layer<Backend> const layer(&mgr, id);

            CHECK(mgr.layer_count() == 1);
        }

        CHECK(mgr.layer_count() == 0);
    }

    TEST_CASE("scoped_layer - with modal dialog") {
        layer_manager<Backend> mgr;
        auto dialog = std::make_shared<TestElement>();

        {
            layer_id const id = mgr.show_modal_dialog(dialog.get(), dialog_position::center);
            scoped_layer<Backend> const layer(&mgr, id);

            CHECK(mgr.has_modal_layer());
        }

        CHECK_FALSE(mgr.has_modal_layer());
    }

    TEST_CASE("scoped_layer - hide/show operations") {
        layer_manager<Backend> mgr;
        auto elem = std::make_shared<TestElement>();

        auto layer = mgr.add_scoped_layer(layer_type::popup, elem);
        layer_id const id = layer.get();

        CHECK(mgr.is_layer_visible(id));

        mgr.hide_layer(id);
        CHECK_FALSE(mgr.is_layer_visible(id));

        mgr.show_layer(id);
        CHECK(mgr.is_layer_visible(id));
    }

    TEST_CASE("scoped_layer - render with scoped layers") {
        layer_manager<Backend> mgr;
        auto elem = std::make_shared<TestElement>();

        {
            auto layer = mgr.add_scoped_layer(layer_type::popup, elem);

            TestRenderer renderer;
            TestRect const viewport{0, 0, 800, 600};

            CHECK_NOTHROW(mgr.render_all_layers(renderer, viewport));
        }

        // Layer removed, rendering should still work
        TestRenderer renderer;
        TestRect const viewport{0, 0, 800, 600};
        CHECK_NOTHROW(mgr.render_all_layers(renderer, viewport));
    }
}

// ============================================================================
// Test Suite 5: Edge Cases
// ============================================================================

TEST_SUITE("scoped_layer - Edge Cases") {

    TEST_CASE("scoped_layer - default constructed (invalid)") {
        scoped_layer<Backend> layer;

        CHECK_FALSE(layer.is_valid());
        CHECK(layer.get() == layer_id::invalid());

        // Operations on invalid handle should be safe
        CHECK_NOTHROW(layer.reset());

        layer_id released_id;
        CHECK_NOTHROW(released_id = layer.release());
        CHECK_FALSE(released_id.is_valid());
    }

    TEST_CASE("scoped_layer - with null manager") {
        scoped_layer<Backend> layer(nullptr, layer_id::invalid());

        CHECK_FALSE(layer.is_valid());

        // Should not crash
        CHECK_NOTHROW(layer.reset());
    }

    TEST_CASE("scoped_layer - invalid layer ID") {
        layer_manager<Backend> mgr;

        scoped_layer<Backend> layer(&mgr, layer_id::invalid());

        CHECK_FALSE(layer.is_valid());
        CHECK_NOTHROW(layer.reset());

        // Also test release() on invalid handle
        layer_id const released = layer.release();
        CHECK_FALSE(released.is_valid());
    }

    TEST_CASE("scoped_layer - self move assignment") {
        layer_manager<Backend> mgr;
        auto elem = std::make_shared<TestElement>();

        auto layer = mgr.add_scoped_layer(layer_type::popup, elem);
        [[maybe_unused]] layer_id const id = layer.get();

        // Self move assignment should be safe (intentional test)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
        layer = std::move(layer);
#pragma GCC diagnostic pop

        // Behavior after self-move is implementation-defined
        // but should not crash
    }
}
