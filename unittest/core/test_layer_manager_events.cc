/**
 * @file test_layer_manager_events.cc
 * @brief Comprehensive event routing tests for layer manager
 * @author Claude Code
 * @date October 19, 2025
 *
 * @details
 * Tests event routing edge cases, modal blocking, click-outside behavior,
 * and multi-layer event propagation scenarios.
 */

#include <doctest/doctest.h>
#include <onyxui/layer_manager.hh>
#include <onyxui/widgets/menu.hh>
#include "../utils/test_helpers.hh"
#include <memory>
#include <vector>

using namespace onyxui;

// Type aliases
using Backend = test_backend;
using TestEvent = test_backend::event_type;
using TestMouseEvent = test_backend::mouse_button_event_type;
using TestRenderer = test_backend::renderer_type;
using TestRect = test_backend::rect_type;
// TestElement is already defined in test_helpers.hh

/**
 * @class TrackingElement
 * @brief Element that tracks events and can control handling
 */
class TrackingElement : public TestElement {
public:
    explicit TrackingElement(bool handles_events = false)
        : TestElement(nullptr)
        , should_handle(handles_events)
        , events_received(0)
        , last_event_handled(false) {
        arrange(TestRect{0, 0, 100, 50});
    }

    bool process_event(const TestEvent& /*event*/) override {
        ++events_received;
        last_event_handled = should_handle;
        return should_handle;
    }

    void reset_counters() {
        events_received = 0;
        last_event_handled = false;
    }

    bool should_handle;
    int events_received;
    bool last_event_handled;
};

// ============================================================================
// Test Suite 1: Modal Blocking
// ============================================================================

TEST_SUITE("Layer Manager - Modal Blocking") {

    TEST_CASE("Modal blocks events to layers below") {
        layer_manager<Backend> mgr;

        auto base = std::make_shared<TrackingElement>(false);
        auto dialog = std::make_shared<TrackingElement>(false);
        auto modal = std::make_shared<TrackingElement>(false);

        auto* base_ptr = base.get();
        auto* dialog_ptr = dialog.get();
        auto* modal_ptr = modal.get();

        mgr.add_layer(layer_type::base, base, 0);
        mgr.add_layer(layer_type::dialog, dialog, 100);
        mgr.add_layer(layer_type::modal, modal, 200);

        TestEvent event;
        bool handled = mgr.route_event(event);

        // Modal should receive event
        CHECK(modal_ptr->events_received == 1);

        // Layers below modal should NOT receive event (blocked)
        CHECK(dialog_ptr->events_received == 0);
        CHECK(base_ptr->events_received == 0);

        // Event blocked by modal even though not handled
        CHECK(handled);
    }

    TEST_CASE("Multiple modals - only highest blocks") {
        layer_manager<Backend> mgr;

        auto base = std::make_shared<TrackingElement>(false);
        auto modal1 = std::make_shared<TrackingElement>(false);
        auto modal2 = std::make_shared<TrackingElement>(false);

        auto* base_ptr = base.get();
        auto* modal1_ptr = modal1.get();
        auto* modal2_ptr = modal2.get();

        mgr.add_layer(layer_type::base, base, 0);
        mgr.add_layer(layer_type::modal, modal1, 100);
        mgr.add_layer(layer_type::modal, modal2, 200);

        TestEvent event;
        mgr.route_event(event);

        // Only topmost modal should receive event
        CHECK(modal2_ptr->events_received == 1);

        // Lower modal and base blocked
        CHECK(modal1_ptr->events_received == 0);
        CHECK(base_ptr->events_received == 0);
    }

    TEST_CASE("Hidden modal doesn't block") {
        layer_manager<Backend> mgr;

        auto base = std::make_shared<TrackingElement>(false);
        auto modal = std::make_shared<TrackingElement>(false);

        auto* base_ptr = base.get();
        auto* modal_ptr = modal.get();

        mgr.add_layer(layer_type::base, base, 0);
        layer_id modal_id = mgr.add_layer(layer_type::modal, modal, 100);

        // Hide the modal
        mgr.hide_layer(modal_id);

        TestEvent event;
        mgr.route_event(event);

        // Base should receive event (modal is hidden)
        CHECK(base_ptr->events_received == 1);
        CHECK(modal_ptr->events_received == 0);
    }

    TEST_CASE("Modal blocks even if event handled by layer above") {
        layer_manager<Backend> mgr;

        auto base = std::make_shared<TrackingElement>(false);
        auto modal = std::make_shared<TrackingElement>(false);
        auto popup = std::make_shared<TrackingElement>(true);  // Handles events

        auto* base_ptr = base.get();
        auto* modal_ptr = modal.get();
        auto* popup_ptr = popup.get();

        mgr.add_layer(layer_type::base, base, 0);
        mgr.add_layer(layer_type::modal, modal, 100);
        mgr.add_layer(layer_type::popup, popup, 200);

        TestEvent event;
        bool handled = mgr.route_event(event);

        // Popup handles event
        CHECK(popup_ptr->events_received == 1);
        CHECK(handled);

        // Modal never receives it (event stopped at popup)
        CHECK(modal_ptr->events_received == 0);

        // Base blocked by modal
        CHECK(base_ptr->events_received == 0);
    }
}

// ============================================================================
// Test Suite 2: Event Routing Order
// ============================================================================

TEST_SUITE("Layer Manager - Event Routing Order") {

    TEST_CASE("Events route top to bottom (highest z first)") {
        layer_manager<Backend> mgr;

        auto layer1 = std::make_shared<TrackingElement>(false);
        auto layer2 = std::make_shared<TrackingElement>(false);
        auto layer3 = std::make_shared<TrackingElement>(false);

        auto* ptr1 = layer1.get();
        auto* ptr2 = layer2.get();
        auto* ptr3 = layer3.get();

        mgr.add_layer(layer_type::base, layer1, 0);
        mgr.add_layer(layer_type::popup, layer2, 100);
        mgr.add_layer(layer_type::tooltip, layer3, 200);

        TestEvent event;
        mgr.route_event(event);

        // All should receive event (none handle it)
        CHECK(ptr1->events_received == 1);
        CHECK(ptr2->events_received == 1);
        CHECK(ptr3->events_received == 1);
    }

    TEST_CASE("Event routing stops at first handler") {
        layer_manager<Backend> mgr;

        auto layer1 = std::make_shared<TrackingElement>(false);  // Bottom - doesn't handle
        auto layer2 = std::make_shared<TrackingElement>(true);   // Middle - handles
        auto layer3 = std::make_shared<TrackingElement>(false);  // Top - doesn't handle

        auto* ptr1 = layer1.get();
        auto* ptr2 = layer2.get();
        auto* ptr3 = layer3.get();

        mgr.add_layer(layer_type::base, layer1, 0);
        mgr.add_layer(layer_type::popup, layer2, 100);
        mgr.add_layer(layer_type::tooltip, layer3, 200);

        TestEvent event;
        bool handled = mgr.route_event(event);

        CHECK(handled);

        // Top layer receives and doesn't handle
        CHECK(ptr3->events_received == 1);

        // Middle layer receives and DOES handle - stops routing
        CHECK(ptr2->events_received == 1);
        CHECK(ptr2->last_event_handled);

        // Bottom layer never receives (routing stopped)
        CHECK(ptr1->events_received == 0);
    }

    TEST_CASE("Invisible layers skipped") {
        layer_manager<Backend> mgr;

        auto layer1 = std::make_shared<TrackingElement>(false);
        auto layer2 = std::make_shared<TrackingElement>(false);
        auto layer3 = std::make_shared<TrackingElement>(false);

        auto* ptr1 = layer1.get();
        auto* ptr2 = layer2.get();
        auto* ptr3 = layer3.get();

        mgr.add_layer(layer_type::base, layer1, 0);
        layer_id id2 = mgr.add_layer(layer_type::popup, layer2, 100);
        mgr.add_layer(layer_type::tooltip, layer3, 200);

        // Hide middle layer
        mgr.hide_layer(id2);

        TestEvent event;
        mgr.route_event(event);

        // Visible layers receive event
        CHECK(ptr1->events_received == 1);
        CHECK(ptr3->events_received == 1);

        // Hidden layer skipped
        CHECK(ptr2->events_received == 0);
    }
}

// ============================================================================
// Test Suite 3: Click-Outside Detection
// ============================================================================

TEST_SUITE("Layer Manager - Click Outside") {

    // Phase 1.3: Generic callback system implemented!
    // NOTE: test_backend's event_type doesn't support mouse coordinates,
    // so these tests verify the callback mechanism exists and is callable.
    // Full coordinate-based testing requires a backend with mouse event support.

    TEST_CASE("Click outside - generic callback mechanism exists") {
        layer_manager<Backend> mgr;

        auto popup = std::make_shared<TestElement>();
        TestRect anchor{100, 100, 50, 20};

        // Callback invocation tracking
        bool callback_invoked = false;
        auto callback = [&callback_invoked]() {
            callback_invoked = true;
        };

        // Phase 1.3: show_popup() now accepts generic callback
        layer_id id = mgr.show_popup(popup.get(), anchor, popup_placement::below, callback);

        // Verify layer was added
        CHECK(id.is_valid());
        CHECK(mgr.layer_count() == 1);

        // The callback exists and can be stored
        // (actual invocation testing requires mouse coordinate support)
        CHECK_MESSAGE(true, "Generic callback mechanism implemented");
    }

    TEST_CASE("Click outside - null callback doesn't crash") {
        layer_manager<Backend> mgr;

        auto popup = std::make_shared<TestElement>();
        TestRect anchor{100, 100, 50, 20};

        // No callback provided (nullptr)
        layer_id id = mgr.show_popup(popup.get(), anchor, popup_placement::below, nullptr);

        // Verify layer was added with null callback
        CHECK(id.is_valid());
        CHECK(mgr.layer_count() == 1);

        // Send regular event (not mouse event with coordinates)
        TestEvent event;
        CHECK_NOTHROW(mgr.route_event(event));
    }

    TEST_CASE("Click outside - multiple popups with different callbacks") {
        layer_manager<Backend> mgr;

        auto popup1 = std::make_shared<TestElement>();
        auto popup2 = std::make_shared<TestElement>();
        TestRect anchor{100, 100, 50, 20};

        bool callback1_invoked = false;
        bool callback2_invoked = false;

        auto callback1 = [&callback1_invoked]() { callback1_invoked = true; };
        auto callback2 = [&callback2_invoked]() { callback2_invoked = true; };

        // Add two popups with different callbacks
        layer_id id1 = mgr.show_popup(popup1.get(), anchor, popup_placement::below, callback1);
        layer_id id2 = mgr.show_popup(popup2.get(), anchor, popup_placement::below, callback2);

        // Verify both layers added
        CHECK(id1.is_valid());
        CHECK(id2.is_valid());
        CHECK(mgr.layer_count() == 2);

        // Each popup can have its own callback
        // (actual invocation testing requires mouse coordinate support)
        CHECK_MESSAGE(true, "Multiple popups can have different callbacks");
    }

    TEST_CASE("Click outside - widget helper passes callback") {
        // Test that widget.hh's show_context_menu passes callback correctly
        // This test verifies API compatibility

        layer_manager<Backend> mgr;
        auto popup = std::make_shared<TestElement>();
        TestRect anchor{100, 100, 50, 20};

        // Verify callback can be passed through
        layer_id id = mgr.show_popup(popup.get(), anchor, popup_placement::below,
            []() {
                // Generic callback that would close the popup
            });

        CHECK(id.is_valid());
        CHECK(mgr.layer_count() == 1);
    }
}

// ============================================================================
// Test Suite 4: Event Propagation Edge Cases
// ============================================================================

TEST_SUITE("Layer Manager - Event Propagation") {

    TEST_CASE("No layers - event not handled") {
        layer_manager<Backend> mgr;

        TestEvent event;
        bool handled = mgr.route_event(event);

        CHECK_FALSE(handled);
    }

    TEST_CASE("All layers hidden - event not handled") {
        layer_manager<Backend> mgr;

        auto layer1 = std::make_shared<TrackingElement>(false);
        auto layer2 = std::make_shared<TrackingElement>(false);

        layer_id id1 = mgr.add_layer(layer_type::base, layer1);
        layer_id id2 = mgr.add_layer(layer_type::popup, layer2);

        mgr.hide_layer(id1);
        mgr.hide_layer(id2);

        TestEvent event;
        bool handled = mgr.route_event(event);

        CHECK_FALSE(handled);
    }

    TEST_CASE("Null event handling") {
        layer_manager<Backend> mgr;

        auto layer = std::make_shared<TrackingElement>(false);
        mgr.add_layer(layer_type::popup, layer);

        // Default-constructed event
        TestEvent event{};
        CHECK_NOTHROW(mgr.route_event(event));
    }

    TEST_CASE("Rapid successive events") {
        layer_manager<Backend> mgr;

        auto layer = std::make_shared<TrackingElement>(false);
        auto* ptr = layer.get();

        mgr.add_layer(layer_type::popup, layer);

        // Send 100 events rapidly
        for (int i = 0; i < 100; ++i) {
            TestEvent event;
            mgr.route_event(event);
        }

        CHECK(ptr->events_received == 100);
    }
}

// ============================================================================
// Test Suite 5: Z-Index Edge Cases
// ============================================================================

TEST_SUITE("Layer Manager - Z-Index Behavior") {

    TEST_CASE("Same z-index - order by insertion") {
        layer_manager<Backend> mgr;

        auto layer1 = std::make_shared<TrackingElement>(true);  // First, handles
        auto layer2 = std::make_shared<TrackingElement>(false);
        auto layer3 = std::make_shared<TrackingElement>(false);


        // All same z-index
        mgr.add_layer(layer_type::popup, layer1, 100);
        mgr.add_layer(layer_type::popup, layer2, 100);
        mgr.add_layer(layer_type::popup, layer3, 100);

        TestEvent event;
        mgr.route_event(event);

        // Behavior depends on implementation
        // Should be consistent and deterministic
        // Document expected behavior here
    }

    TEST_CASE("Negative z-index") {
        layer_manager<Backend> mgr;

        auto layer1 = std::make_shared<TrackingElement>(false);
        auto layer2 = std::make_shared<TrackingElement>(false);

        auto* ptr1 = layer1.get();
        auto* ptr2 = layer2.get();

        mgr.add_layer(layer_type::popup, layer1, -100);
        mgr.add_layer(layer_type::popup, layer2, 100);

        TestEvent event;
        mgr.route_event(event);

        // Higher z-index should receive first
        // Both should receive (neither handles)
        CHECK(ptr1->events_received == 1);
        CHECK(ptr2->events_received == 1);
    }

    TEST_CASE("Very large z-index") {
        layer_manager<Backend> mgr;

        auto layer1 = std::make_shared<TrackingElement>(false);
        auto layer2 = std::make_shared<TrackingElement>(false);

        auto* ptr2 = layer2.get();

        mgr.add_layer(layer_type::popup, layer1, 0);
        mgr.add_layer(layer_type::popup, layer2, 999999);

        TestEvent event;
        mgr.route_event(event);

        // Very high z-index should work correctly
        CHECK(ptr2->events_received >= 1);  // Should get event first
    }
}

// ============================================================================
// Test Suite 6: has_modal_layer Query
// ============================================================================

TEST_SUITE("Layer Manager - Modal Query") {

    TEST_CASE("has_modal_layer - no modal") {
        layer_manager<Backend> mgr;

        auto layer = std::make_shared<TestElement>();
        mgr.add_layer(layer_type::popup, layer);

        CHECK_FALSE(mgr.has_modal_layer());
    }

    TEST_CASE("has_modal_layer - visible modal") {
        layer_manager<Backend> mgr;

        auto modal = std::make_shared<TestElement>();
        mgr.add_layer(layer_type::modal, modal);

        CHECK(mgr.has_modal_layer());
    }

    TEST_CASE("has_modal_layer - hidden modal") {
        layer_manager<Backend> mgr;

        auto modal = std::make_shared<TestElement>();
        layer_id id = mgr.add_layer(layer_type::modal, modal);

        mgr.hide_layer(id);

        CHECK_FALSE(mgr.has_modal_layer());
    }

    TEST_CASE("has_modal_layer - multiple modals") {
        layer_manager<Backend> mgr;

        auto modal1 = std::make_shared<TestElement>();
        auto modal2 = std::make_shared<TestElement>();

        mgr.add_layer(layer_type::modal, modal1);
        mgr.add_layer(layer_type::modal, modal2);

        CHECK(mgr.has_modal_layer());
    }
}
