/**
 * @file test_layer_manager.cc
 * @brief Comprehensive unit tests for layer manager functionality
 * @author Assistant
 * @date 2024
 */

#include <doctest/doctest.h>
#include <../../include/onyxui/services/layer_manager.hh>
#include "../utils/test_helpers.hh"
#include "../../include/onyxui/core/types.hh"
#include "../../include/onyxui/core/geometry.hh"
#include "utils/test_backend.hh"
#include "../../include/onyxui/core/rendering/render_context.hh"
#include "../../include/onyxui/layout/layout_strategy.hh"
#include <memory>
#include <limits>

using namespace onyxui;
using testing::make_relative_rect;

using namespace onyxui;

// Type aliases for test backend types
using Backend = test_backend;
using TestEvent = test_backend::event_type;
using TestRenderer = test_backend::renderer_type;
using TestMouseEvent = test_backend::mouse_button_event_type;

// Helper function to create ui_event from backend event
inline ui_event make_ui_event() {
    TestEvent backend_event;
    backend_event.type = TestEvent::mouse_down;
    auto ui_event_opt = test_backend::create_event(backend_event);
    if (!ui_event_opt) {
        throw std::runtime_error("Failed to create ui_event");
    }
    return *ui_event_opt;
}

/**
 * @class TestLayer
 * @brief Test element that tracks events and rendering
 */
class TestLayer : public TestElement {
public:
    explicit TestLayer(TestElement* parent = nullptr)
        : TestElement(parent)
        , render_called(false)      // Must match declaration order
        , event_handled(false)
        , process_called(false)
        , last_event_received(false) {
        set_focusable(true);
        // Give default bounds so is_inside works
        arrange(logical_rect{0_lu, 0_lu, 100_lu, 50_lu});
    }

    // Override virtual process_event for polymorphic dispatch
    bool process_event(const TestEvent& event) override {
        process_called = true;
        last_event_received = true;

        // For testing: if configured to handle, return true
        // Otherwise delegate to base class
        if (event_handled) {
            return true;
        }

        // Call base implementation which will use process_event_impl
        return TestElement::process_event(event);
    }

    // Override mouse handler
    bool handle_mouse(const mouse_event& /*mouse*/) override {
        process_called = true;
        last_event_received = true;
        return event_handled;  // Return configured value
    }

    // Override rendering
    void do_render(render_context<Backend>& ctx) const override {
        render_called = true;
        TestElement::do_render(ctx);
    }

    mutable bool render_called = false;  // Made mutable for const do_render

    // Override hit testing for event routing
    bool is_inside(logical_unit x, logical_unit y) const override {
        auto const b = bounds();
        return x >= b.x && x < b.x + b.width &&
               y >= b.y && y < b.y + b.height;
    }

    // Set preferred size for layout testing
    void set_preferred_size(int w, int h) {
        preferred_width = w;
        preferred_height = h;

        size_constraint width_constraint;
        width_constraint.policy = size_policy::fixed;
        width_constraint.preferred_size = logical_unit(static_cast<double>(w));
        width_constraint.min_size = logical_unit(0.0);
        width_constraint.max_size = logical_unit(static_cast<double>(std::numeric_limits<int>::max()));
        set_width_constraint(width_constraint);

        size_constraint height_constraint;
        height_constraint.policy = size_policy::fixed;
        height_constraint.preferred_size = logical_unit(static_cast<double>(h));
        height_constraint.min_size = logical_unit(0.0);
        height_constraint.max_size = logical_unit(static_cast<double>(std::numeric_limits<int>::max()));
        set_height_constraint(height_constraint);
    }

protected:
    logical_size get_content_size() const override {
        return logical_size{logical_unit(static_cast<double>(preferred_width)),
                           logical_unit(static_cast<double>(preferred_height))};
    }

public:
    // Test configuration
    bool event_handled;       // What to return from process_event
    // render_called is now mutable (declared with do_render override)
    bool process_called;       // Track if process_event was called
    bool last_event_received;  // Track if event was received

    int preferred_width = 100;
    int preferred_height = 50;

    // Reset test state
    void reset() {
        event_handled = false;
        render_called = false;
        process_called = false;
        last_event_received = false;
    }
};

TEST_SUITE("Layer Manager") {

    TEST_CASE("Basic layer management") {
        layer_manager<test_backend> mgr;

        SUBCASE("Initially empty") {
            CHECK(mgr.layer_count() == 0);
            CHECK(!mgr.has_modal_layer());
        }

        SUBCASE("Add a layer") {
            auto layer = std::make_shared<TestLayer>();
            layer_id const id = mgr.add_layer(layer_type::popup, layer);

            CHECK(id.is_valid());
            CHECK(mgr.layer_count() == 1);
        }

        SUBCASE("Remove a layer") {
            auto layer = std::make_shared<TestLayer>();
            layer_id const id = mgr.add_layer(layer_type::popup, layer);

            mgr.remove_layer(id);
            CHECK(mgr.layer_count() == 0);
        }

        SUBCASE("Remove invalid layer ID does nothing") {
            auto layer = std::make_shared<TestLayer>();
            mgr.add_layer(layer_type::popup, layer);

            mgr.remove_layer(layer_id::invalid());
            CHECK(mgr.layer_count() == 1);
        }

        SUBCASE("Clear layers by type") {
            auto popup1 = std::make_shared<TestLayer>();
            auto popup2 = std::make_shared<TestLayer>();
            auto dialog = std::make_shared<TestLayer>();

            mgr.add_layer(layer_type::popup, popup1);
            mgr.add_layer(layer_type::popup, popup2);
            mgr.add_layer(layer_type::dialog, dialog);

            CHECK(mgr.layer_count() == 3);

            mgr.clear_layers(layer_type::popup);
            CHECK(mgr.layer_count() == 1); // Only dialog remains
        }

        SUBCASE("Clear all layers") {
            auto layer1 = std::make_shared<TestLayer>();
            auto layer2 = std::make_shared<TestLayer>();

            mgr.add_layer(layer_type::popup, layer1);
            mgr.add_layer(layer_type::dialog, layer2);

            mgr.clear_all_layers();
            CHECK(mgr.layer_count() == 0);
        }
    }

    TEST_CASE("Z-ordering") {
        layer_manager<test_backend> mgr;

        SUBCASE("Default z-indices for layer types") {
            CHECK(get_default_z_index(layer_type::base) == 0);
            CHECK(get_default_z_index(layer_type::tooltip) == 100);
            CHECK(get_default_z_index(layer_type::popup) == 200);
            CHECK(get_default_z_index(layer_type::dialog) == 300);
            CHECK(get_default_z_index(layer_type::modal) == 400);
            CHECK(get_default_z_index(layer_type::notification) == 500);
            CHECK(get_default_z_index(layer_type::debug) == 1000);
        }

        SUBCASE("Layers sorted by z-index") {
            auto tooltip = std::make_shared<TestLayer>();
            auto popup = std::make_shared<TestLayer>();
            auto dialog = std::make_shared<TestLayer>();

            // Add in reverse z-order
            mgr.add_layer(layer_type::dialog, dialog);   // z=300
            mgr.add_layer(layer_type::tooltip, tooltip); // z=100
            mgr.add_layer(layer_type::popup, popup);     // z=200

            // Layers should be internally sorted by z-index
            CHECK(mgr.layer_count() == 3);
        }

        SUBCASE("Custom z-index") {
            auto layer = std::make_shared<TestLayer>();

            // Use custom z-index instead of default
            mgr.add_layer(layer_type::popup, layer, 999);

            CHECK(mgr.layer_count() == 1);
        }
    }

    TEST_CASE("Layer visibility") {
        layer_manager<test_backend> mgr;

        auto layer = std::make_shared<TestLayer>();
        layer_id const id = mgr.add_layer(layer_type::popup, layer);

        SUBCASE("Layers are visible by default") {
            CHECK(mgr.is_layer_visible(id));
        }

        SUBCASE("Hide layer") {
            mgr.hide_layer(id);
            CHECK(!mgr.is_layer_visible(id));
            CHECK(mgr.layer_count() == 1); // Still exists, just hidden
        }

        SUBCASE("Show hidden layer") {
            mgr.hide_layer(id);
            mgr.show_layer(id);
            CHECK(mgr.is_layer_visible(id));
        }

        SUBCASE("Check visibility of invalid ID") {
            CHECK(!mgr.is_layer_visible(layer_id::invalid()));
        }
    }

    TEST_CASE("Modal layer detection") {
        layer_manager<test_backend> mgr;

        SUBCASE("No modal initially") {
            CHECK(!mgr.has_modal_layer());
        }

        SUBCASE("Modal layer detected") {
            auto modal = std::make_shared<TestLayer>();
            mgr.add_layer(layer_type::modal, modal);

            CHECK(mgr.has_modal_layer());
        }

        SUBCASE("Hidden modal not counted") {
            auto modal = std::make_shared<TestLayer>();
            layer_id const id = mgr.add_layer(layer_type::modal, modal);

            mgr.hide_layer(id);
            CHECK(!mgr.has_modal_layer());
        }

        SUBCASE("Non-modal layers not counted") {
            auto popup = std::make_shared<TestLayer>();
            auto dialog = std::make_shared<TestLayer>();

            mgr.add_layer(layer_type::popup, popup);
            mgr.add_layer(layer_type::dialog, dialog);

            CHECK(!mgr.has_modal_layer());
        }
    }

    TEST_CASE("Event routing") {
        layer_manager<test_backend> mgr;

        // Use the generic event type
        TestEvent backend_event;
        backend_event.type = TestEvent::mouse_down;

        // Convert to ui_event for routing (layer_manager now uses unified event API)
        auto ui_event_opt = test_backend::create_event(backend_event);
        REQUIRE(ui_event_opt.has_value());
        ui_event event = *ui_event_opt;

        SUBCASE("No layers - event not handled") {
            CHECK(!mgr.route_event(event));
        }

        SUBCASE("Single layer handles event") {
            auto layer = std::make_shared<TestLayer>();
            layer->event_handled = true; // Configure to handle

            mgr.add_layer(layer_type::popup, layer);

            CHECK(mgr.route_event(event));
            CHECK(layer->last_event_received);
        }

        SUBCASE("Single layer doesn't handle event") {
            auto layer = std::make_shared<TestLayer>();
            layer->event_handled = false; // Configure to not handle

            mgr.add_layer(layer_type::popup, layer);

            CHECK(!mgr.route_event(event));
            CHECK(layer->last_event_received); // Still received it
        }

        SUBCASE("Higher z-index layer gets event first") {
            auto lower = std::make_shared<TestLayer>();
            auto higher = std::make_shared<TestLayer>();

            lower->event_handled = true;
            higher->event_handled = true;

            mgr.add_layer(layer_type::tooltip, lower);  // z=100
            mgr.add_layer(layer_type::popup, higher);   // z=200

            CHECK(mgr.route_event(event));

            CHECK(higher->last_event_received); // Higher got it
            CHECK(!lower->last_event_received); // Lower didn't (event was handled)
        }

        SUBCASE("Event passes through if not handled") {
            auto lower = std::make_shared<TestLayer>();
            auto higher = std::make_shared<TestLayer>();

            lower->event_handled = true;
            higher->event_handled = false; // Higher doesn't handle

            mgr.add_layer(layer_type::tooltip, lower);
            mgr.add_layer(layer_type::popup, higher);

            CHECK(mgr.route_event(event));

            CHECK(higher->last_event_received); // Higher got it first
            CHECK(lower->last_event_received);  // Lower also got it
        }

        SUBCASE("Hidden layers don't receive events") {
            auto layer = std::make_shared<TestLayer>();
            layer->event_handled = true;

            layer_id const id = mgr.add_layer(layer_type::popup, layer);
            mgr.hide_layer(id);

            CHECK(!mgr.route_event(event));
            CHECK(!layer->last_event_received);
        }

        SUBCASE("Modal blocks events to lower layers") {
            auto base = std::make_shared<TestLayer>();
            auto modal = std::make_shared<TestLayer>();

            base->event_handled = true;
            modal->event_handled = false; // Modal doesn't handle

            mgr.add_layer(layer_type::base, base);    // z=0
            mgr.add_layer(layer_type::modal, modal);  // z=400

            CHECK(mgr.route_event(event)); // Returns true (blocked by modal)

            CHECK(modal->last_event_received); // Modal got it
            CHECK(!base->last_event_received); // Base blocked
        }

        SUBCASE("Layers above modal still get events") {
            auto modal = std::make_shared<TestLayer>();
            auto debug = std::make_shared<TestLayer>();

            modal->event_handled = false;
            debug->event_handled = true;

            mgr.add_layer(layer_type::modal, modal);  // z=400
            mgr.add_layer(layer_type::debug, debug);  // z=1000

            CHECK(mgr.route_event(event));

            CHECK(debug->last_event_received); // Debug got it (above modal)
            CHECK(!modal->last_event_received); // Modal didn't (event handled)
        }
    }

    TEST_CASE("Popup helpers") {
        layer_manager<test_backend> mgr;

        SUBCASE("Show popup") {
            auto popup = std::make_shared<TestLayer>();
            logical_rect const anchor{10.0_lu, 20.0_lu, 100.0_lu, 30.0_lu};

            layer_id const id = mgr.show_popup(popup.get(), anchor, popup_placement::below);

            CHECK(id.is_valid());
            CHECK(mgr.layer_count() == 1);
            CHECK(mgr.is_layer_visible(id));
        }

        SUBCASE("Show tooltip") {
            auto tooltip = std::make_shared<TestLayer>();

            layer_id const id = mgr.show_tooltip(tooltip.get(), 50.0_lu, 60.0_lu);

            CHECK(id.is_valid());
            CHECK(mgr.layer_count() == 1);
            CHECK(mgr.is_layer_visible(id));
        }

        SUBCASE("Show modal dialog") {
            auto dialog = std::make_shared<TestLayer>();

            layer_id const id = mgr.show_modal_dialog(dialog.get(), dialog_position::center);

            CHECK(id.is_valid());
            CHECK(mgr.layer_count() == 1);
            CHECK(mgr.has_modal_layer());
        }
    }

    TEST_CASE("Rendering") {
        layer_manager<test_backend> mgr;
        TestRenderer renderer;
        TestRect const viewport{0, 0, 800, 600};

        // Create a minimal theme for testing (layer tests don't use theme styling, just need valid reference)
        ui_theme<test_backend> test_theme;
        test_theme.name = "TestTheme";

        SUBCASE("No layers - nothing rendered") {
            mgr.render_all_layers(renderer, viewport, &test_theme, make_terminal_metrics<test_backend>());
            // No assertions needed - just shouldn't crash
        }

        SUBCASE("Single layer rendered") {
            auto layer = std::make_shared<TestLayer>();
            mgr.add_layer(layer_type::popup, layer);

            mgr.render_all_layers(renderer, viewport, &test_theme, make_terminal_metrics<test_backend>());

            CHECK(layer->render_called);
        }

        SUBCASE("Hidden layers not rendered") {
            auto layer = std::make_shared<TestLayer>();
            layer_id const id = mgr.add_layer(layer_type::popup, layer);
            mgr.hide_layer(id);

            mgr.render_all_layers(renderer, viewport, &test_theme, make_terminal_metrics<test_backend>());

            CHECK(!layer->render_called);
        }

        SUBCASE("Multiple layers rendered in z-order") {
            auto layer1 = std::make_shared<TestLayer>();
            auto layer2 = std::make_shared<TestLayer>();
            auto layer3 = std::make_shared<TestLayer>();

            mgr.add_layer(layer_type::popup, layer1);    // z=200
            mgr.add_layer(layer_type::tooltip, layer2);  // z=100
            mgr.add_layer(layer_type::dialog, layer3);   // z=300

            mgr.render_all_layers(renderer, viewport, &test_theme, make_terminal_metrics<test_backend>());

            CHECK(layer1->render_called);
            CHECK(layer2->render_called);
            CHECK(layer3->render_called);
        }

        SUBCASE("Layer positioning for popup") {
            auto popup = std::make_shared<TestLayer>();
            logical_rect const anchor{100.0_lu, 50.0_lu, 80.0_lu, 30.0_lu};

            // Set a size for the popup
            popup->set_preferred_size(120, 60);

            mgr.show_popup(popup.get(), anchor, popup_placement::below);
            mgr.render_all_layers(renderer, viewport, &test_theme, make_terminal_metrics<test_backend>());

            // Popup should be positioned below anchor
            // Expected position: x=anchor.x, y=anchor.y+anchor.h
            auto bounds = popup->bounds();
            CHECK(bounds.x == 100_lu);
            CHECK(bounds.y == 80_lu); // 50 + 30
        }

        SUBCASE("Layer positioning for dialog") {
            auto dialog = std::make_shared<TestLayer>();

            // Set a size for the dialog
            dialog->set_preferred_size(200, 150);

            mgr.show_modal_dialog(dialog.get(), dialog_position::center);
            mgr.render_all_layers(renderer, viewport, &test_theme, make_terminal_metrics<test_backend>());

            // Dialog should be centered in viewport
            auto bounds = dialog->bounds();
            CHECK(bounds.x == 300_lu); // (800 - 200) / 2
            CHECK(bounds.y == 225_lu); // (600 - 150) / 2
        }
    }

    TEST_CASE("Popup placement") {
        layer_manager<test_backend> mgr;
        TestRenderer renderer;
        TestRect const viewport{0, 0, 800, 600};
        logical_rect const anchor{400.0_lu, 300.0_lu, 100.0_lu, 50.0_lu};

        // Create a minimal theme for testing
        ui_theme<test_backend> test_theme;
        test_theme.name = "TestTheme";

        // Create popup with fixed size
        auto create_popup = []() {
            auto popup = std::make_shared<TestLayer>();
            popup->set_preferred_size(150, 80);
            return popup;
        };

        SUBCASE("Placement below") {
            auto popup = create_popup();
            mgr.show_popup(popup.get(), anchor, popup_placement::below);
            mgr.render_all_layers(renderer, viewport, &test_theme, make_terminal_metrics<test_backend>());

            auto bounds = popup->bounds();
            CHECK(bounds.x == 400_lu);
            CHECK(bounds.y == 350_lu); // anchor.y + anchor.h
        }

        SUBCASE("Placement above") {
            auto popup = create_popup();
            mgr.show_popup(popup.get(), anchor, popup_placement::above);
            mgr.render_all_layers(renderer, viewport, &test_theme, make_terminal_metrics<test_backend>());

            auto bounds = popup->bounds();
            CHECK(bounds.x == 400_lu);
            CHECK(bounds.y == 220_lu); // anchor.y - popup.h
        }

        SUBCASE("Placement left") {
            auto popup = create_popup();
            mgr.show_popup(popup.get(), anchor, popup_placement::left);
            mgr.render_all_layers(renderer, viewport, &test_theme, make_terminal_metrics<test_backend>());

            auto bounds = popup->bounds();
            CHECK(bounds.x == 250_lu); // anchor.x - popup.w
            CHECK(bounds.y == 300_lu);
        }

        SUBCASE("Placement right") {
            auto popup = create_popup();
            mgr.show_popup(popup.get(), anchor, popup_placement::right);
            mgr.render_all_layers(renderer, viewport, &test_theme, make_terminal_metrics<test_backend>());

            auto bounds = popup->bounds();
            CHECK(bounds.x == 500_lu); // anchor.x + anchor.w
            CHECK(bounds.y == 300_lu);
        }

        SUBCASE("Placement below_right") {
            auto popup = create_popup();
            mgr.show_popup(popup.get(), anchor, popup_placement::below_right);
            mgr.render_all_layers(renderer, viewport, &test_theme, make_terminal_metrics<test_backend>());

            auto bounds = popup->bounds();
            CHECK(bounds.x == 350_lu); // anchor.x + anchor.w - popup.w
            CHECK(bounds.y == 350_lu); // anchor.y + anchor.h
        }

        SUBCASE("Placement above_right") {
            auto popup = create_popup();
            mgr.show_popup(popup.get(), anchor, popup_placement::above_right);
            mgr.render_all_layers(renderer, viewport, &test_theme, make_terminal_metrics<test_backend>());

            auto bounds = popup->bounds();
            CHECK(bounds.x == 350_lu); // anchor.x + anchor.w - popup.w
            CHECK(bounds.y == 220_lu); // anchor.y - popup.h
        }

        SUBCASE("Auto placement when below fits") {
            auto popup = create_popup();
            mgr.show_popup(popup.get(), anchor, popup_placement::auto_best);
            mgr.render_all_layers(renderer, viewport, &test_theme, make_terminal_metrics<test_backend>());

            auto bounds = popup->bounds();
            CHECK(bounds.y == 350_lu); // Chose below
        }

        SUBCASE("Auto placement when below doesn't fit") {
            logical_rect const bottom_anchor{400.0_lu, 550.0_lu, 100.0_lu, 30.0_lu}; // Near bottom
            auto popup = create_popup();
            mgr.show_popup(popup.get(), bottom_anchor, popup_placement::auto_best);
            mgr.render_all_layers(renderer, viewport, &test_theme, make_terminal_metrics<test_backend>());

            auto bounds = popup->bounds();
            CHECK(bounds.y == 470_lu); // Chose above (550 - 80)
        }

        SUBCASE("Clamping to viewport") {
            logical_rect const edge_anchor{750.0_lu, 100.0_lu, 100.0_lu, 50.0_lu}; // Near right edge
            auto popup = create_popup();
            mgr.show_popup(popup.get(), edge_anchor, popup_placement::below);
            mgr.render_all_layers(renderer, viewport, &test_theme, make_terminal_metrics<test_backend>());

            auto bounds = popup->bounds();
            CHECK(bounds.x == 650_lu); // Clamped to fit (800 - 150)
            CHECK(bounds.y == 150_lu); // Below anchor
        }
    }

    TEST_CASE("Layer ID") {
        SUBCASE("Invalid ID") {
            layer_id const invalid = layer_id::invalid();
            CHECK(!invalid.is_valid());
            CHECK(invalid.value == 0);
        }

        SUBCASE("Valid ID") {
            layer_id const valid(42);
            CHECK(valid.is_valid());
            CHECK(valid.value == 42);
        }

        SUBCASE("ID comparison") {
            layer_id const id1(1);
            layer_id id2(2);
            layer_id id1_copy(1);

            CHECK(id1 == id1_copy);
            CHECK(id1 != id2);
        }
    }

    TEST_CASE("Multiple operations sequence") {
        layer_manager<test_backend> mgr;
        TestRect const viewport{0, 0, 800, 600};

        // Create multiple layers
        auto base = std::make_shared<TestLayer>();
        auto popup = std::make_shared<TestLayer>();
        auto modal = std::make_shared<TestLayer>();
        auto tooltip = std::make_shared<TestLayer>();

        // Add layers in various orders
        (void)mgr.add_layer(layer_type::base, base);  // base_id not needed
        layer_id const popup_id = mgr.show_popup(popup.get(), logical_rect{100.0_lu, 100.0_lu, 50.0_lu, 30.0_lu});
        layer_id const modal_id = mgr.show_modal_dialog(modal.get());
        (void)mgr.show_tooltip(tooltip.get(), 200.0_lu, 200.0_lu);  // tooltip_id not needed

        CHECK(mgr.layer_count() == 4);
        CHECK(mgr.has_modal_layer());

        // Hide modal
        mgr.hide_layer(modal_id);
        CHECK(!mgr.has_modal_layer());

        // Remove popup
        mgr.remove_layer(popup_id);
        CHECK(mgr.layer_count() == 3);

        // Show modal again
        mgr.show_layer(modal_id);
        CHECK(mgr.has_modal_layer());

        // Clear all tooltips
        mgr.clear_layers(layer_type::tooltip);
        CHECK(mgr.layer_count() == 2);

        // Test event routing with remaining layers
        ui_event const event = make_ui_event();
        base->event_handled = true;
        modal->event_handled = false;

        CHECK(mgr.route_event(event)); // Blocked by modal
        CHECK(!base->last_event_received); // Base didn't get it

        // Clear everything
        mgr.clear_all_layers();
        CHECK(mgr.layer_count() == 0);
    }

    TEST_CASE("Modal dialog event routing with deferred positioning") {
        // This test verifies that modal dialogs added after initial render
        // can receive mouse events properly (ensure_layers_positioned fix)
        layer_manager<test_backend> mgr;

        // First, simulate a render with viewport to set up metrics
        test_backend::renderer_type renderer;
        test_backend::rect_type viewport;
        rect_utils::set_bounds(viewport, 0, 0, 80, 25);
        ui_theme<test_backend> test_theme;
        test_theme.name = "TestTheme";

        auto base_layer = std::make_shared<TestLayer>();
        mgr.add_layer(layer_type::base, base_layer);
        mgr.render_all_layers(renderer, viewport, &test_theme, make_terminal_metrics<test_backend>());

        // Now create a modal dialog (will be positioned on next render/event)
        auto modal = std::make_shared<TestLayer>();
        modal->set_preferred_size(40, 10);
        modal->arrange(logical_rect{0_lu, 0_lu, 40_lu, 10_lu});
        modal->event_handled = true;

        mgr.show_modal_dialog(modal.get());
        CHECK(mgr.layer_count() == 2);

        // Create mouse event that should hit the modal (center of screen)
        // The modal should be positioned in the center by ensure_layers_positioned()
        mouse_event mouse{
            .x = 40.0_lu,  // Center X
            .y = 12.0_lu,  // Center Y
            .btn = mouse_event::button::left,
            .act = mouse_event::action::press,
            .modifiers = {}
        };
        ui_event event{mouse};

        // Route event - this should trigger ensure_layers_positioned() internally
        // which positions and arranges the modal, allowing hit testing to work
        bool handled = mgr.route_event(event);

        // Modal should have received the event
        CHECK(handled);
        CHECK(modal->last_event_received);
    }
}