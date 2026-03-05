/**
 * @file test_layer_dirty_regions.cc
 * @brief Tests for layer removal dirty region tracking
 * @author Assistant
 * @date 2025-10-25
 *
 * @details
 * Reproduces and tests fix for menu switching bug:
 * - Open File menu at position (0, 0, 40, 20)
 * - Switch to Theme menu at position (50, 0, 40, 30)
 * - File menu layer is removed, but pixels remain on screen
 * - EXPECTED: File menu area (0, 0, 40, 20) should be marked dirty
 */

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/services/layer_manager.hh>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/services/ui_context.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/ui_handle.hh>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/widgets/containers/panel.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;
using namespace onyxui::testing;
using Backend = test_backend;

TEST_SUITE("Layer Dirty Region Tracking") {

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "Layer removal marks area as dirty") {
    // Use the fixture's context which already has a properly registered theme

    auto* layer_mgr = ui_services<Backend>::layers();
    REQUIRE(layer_mgr != nullptr);

    // Create two menus at different positions
    auto menu1 = std::make_unique<panel<Backend>>();
    auto menu2 = std::make_unique<panel<Backend>>();

    auto* menu1_ptr = menu1.get();
    auto* menu2_ptr = menu2.get();

    // Show first menu at position (10, 10) with size (40, 20)
    logical_rect const anchor1{10.0_lu, 10.0_lu, 5.0_lu, 5.0_lu};

    layer_id menu1_id = layer_mgr->show_popup(
        menu1_ptr,
        anchor1,
        popup_placement::below
    );

    REQUIRE(menu1_id != layer_id::invalid());
    CHECK(layer_mgr->layer_count() == 1);

    // Position and measure menu1
    logical_rect const menu1_bounds{10.0_lu, 15.0_lu, 40.0_lu, 20.0_lu};
    layer_mgr->set_layer_bounds(menu1_id, menu1_bounds);
    [[maybe_unused]] auto size1 = menu1_ptr->measure(logical_unit(static_cast<double>(40)), logical_unit(static_cast<double>(20)));
    menu1_ptr->arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

    SUBCASE("Single layer removal marks its bounds as dirty") {
        // Get dirty regions before removal
        auto dirty_before = layer_mgr->get_removed_layer_dirty_regions();
        CHECK(dirty_before.empty());

        // Remove menu1
        layer_mgr->remove_layer(menu1_id);
        CHECK(layer_mgr->layer_count() == 0);

        // Check that removed region is marked dirty
        auto dirty_after = layer_mgr->get_removed_layer_dirty_regions();
        REQUIRE(dirty_after.size() == 1);

        // Verify the dirty region matches menu1's bounds + shadow margin on ALL sides
        // Shadow margin = 2 units, applied symmetrically: x-2, y-2, width+4, height+4
        CHECK(dirty_after[0].x == 8.0_lu);       // 10 - 2 for shadow
        CHECK(dirty_after[0].y == 13.0_lu);      // 15 - 2 for shadow
        CHECK(dirty_after[0].width == 44.0_lu);  // 40 + 2*2 for shadow on both sides
        CHECK(dirty_after[0].height == 24.0_lu); // 20 + 2*2 for shadow on both sides

        // Clear dirty regions should reset the list
        layer_mgr->clear_removed_layer_dirty_regions();
        auto dirty_cleared = layer_mgr->get_removed_layer_dirty_regions();
        CHECK(dirty_cleared.empty());
    }

    SUBCASE("Menu switching: old menu area marked dirty") {
        // Show second menu at different position (60, 10) with size (40, 30)
        logical_rect const anchor2{60.0_lu, 10.0_lu, 5.0_lu, 5.0_lu};

        layer_id menu2_id = layer_mgr->show_popup(
            menu2_ptr,
            anchor2,
            popup_placement::below
        );

        REQUIRE(menu2_id != layer_id::invalid());
        CHECK(layer_mgr->layer_count() == 2);

        // Position menu2
        logical_rect const menu2_bounds{60.0_lu, 15.0_lu, 40.0_lu, 30.0_lu};
        layer_mgr->set_layer_bounds(menu2_id, menu2_bounds);
        [[maybe_unused]] auto size2 = menu2_ptr->measure(logical_unit(static_cast<double>(40)), logical_unit(static_cast<double>(30)));
        menu2_ptr->arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Simulate menu switching: remove menu1, keep menu2
        layer_mgr->remove_layer(menu1_id);
        CHECK(layer_mgr->layer_count() == 1);

        // Check that menu1's area is marked dirty (shadow margin on ALL sides)
        auto dirty = layer_mgr->get_removed_layer_dirty_regions();
        REQUIRE(dirty.size() == 1);
        CHECK(dirty[0].x == 8.0_lu);       // 10 - 2 for shadow
        CHECK(dirty[0].y == 13.0_lu);      // 15 - 2 for shadow
        CHECK(dirty[0].width == 44.0_lu);  // 40 + 2*2 for shadow on both sides
        CHECK(dirty[0].height == 24.0_lu); // 20 + 2*2 for shadow on both sides

        // menu2 should still be visible
        CHECK(layer_mgr->is_layer_visible(menu2_id));
    }

    SUBCASE("Multiple layer removals accumulate dirty regions") {
        // Show second menu
        logical_rect const anchor2{60.0_lu, 10.0_lu, 5.0_lu, 5.0_lu};

        layer_id menu2_id = layer_mgr->show_popup(
            menu2_ptr,
            anchor2,
            popup_placement::below
        );

        logical_rect const menu2_bounds{60.0_lu, 15.0_lu, 40.0_lu, 30.0_lu};
        layer_mgr->set_layer_bounds(menu2_id, menu2_bounds);
        [[maybe_unused]] auto sz = menu2_ptr->measure(logical_unit(static_cast<double>(40)), logical_unit(static_cast<double>(30)));
        menu2_ptr->arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Remove both menus
        layer_mgr->remove_layer(menu1_id);
        layer_mgr->remove_layer(menu2_id);

        // Both regions should be marked dirty
        auto dirty = layer_mgr->get_removed_layer_dirty_regions();
        REQUIRE(dirty.size() == 2);

        // First removed (menu1) - shadow margin subtracted from x/y
        CHECK(dirty[0].x == 8.0_lu);   // 10 - 2
        CHECK(dirty[0].y == 13.0_lu);  // 15 - 2

        // Second removed (menu2) - shadow margin subtracted from x/y
        CHECK(dirty[1].x == 58.0_lu);  // 60 - 2
        CHECK(dirty[1].y == 13.0_lu);  // 15 - 2
    }

    SUBCASE("Removing non-existent layer doesn't add dirty region") {
        auto dirty_before = layer_mgr->get_removed_layer_dirty_regions();
        auto count_before = dirty_before.size();

        // Try to remove invalid layer
        layer_mgr->remove_layer(layer_id::invalid());

        auto dirty_after = layer_mgr->get_removed_layer_dirty_regions();
        CHECK(dirty_after.size() == count_before);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "UI handle integrates removed layer dirty regions") {
    // Use the fixture's context which already has a properly registered theme

    // Create root widget
    auto root = std::make_unique<panel<Backend>>();
    root->set_has_border(true);

    // Create UI handle
    ui_handle<Backend> ui(std::move(root));

    auto* layer_mgr = ui_services<Backend>::layers();
    REQUIRE(layer_mgr != nullptr);

    // Create and show a popup layer
    auto popup = std::make_unique<panel<Backend>>();
    auto* popup_ptr = popup.get();

    logical_rect const anchor{20.0_lu, 20.0_lu, 5.0_lu, 5.0_lu};

    layer_id popup_id = layer_mgr->show_popup(popup_ptr, anchor, popup_placement::below);
    REQUIRE(popup_id != layer_id::invalid());

    // Position popup
    logical_rect const popup_bounds{20.0_lu, 25.0_lu, 30.0_lu, 15.0_lu};
    layer_mgr->set_layer_bounds(popup_id, popup_bounds);
    [[maybe_unused]] auto popup_size = popup_ptr->measure(logical_unit(static_cast<double>(30)), logical_unit(static_cast<double>(15)));
    popup_ptr->arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

    // Display once (popup visible)
    ui.display();

    // Remove popup
    layer_mgr->remove_layer(popup_id);

    // Verify dirty regions are tracked (shadow margin subtracted from x/y)
    auto dirty = layer_mgr->get_removed_layer_dirty_regions();
    REQUIRE(dirty.size() == 1);
    CHECK(dirty[0].x == 18.0_lu);  // 20 - 2 for shadow margin
    CHECK(dirty[0].y == 23.0_lu);  // 25 - 2 for shadow margin

    // Display again - dirty regions should be used and cleared
    ui.display();

    // After display, dirty regions should be cleared
    auto dirty_after = layer_mgr->get_removed_layer_dirty_regions();
    CHECK(dirty_after.empty());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "hide_layer marks area as dirty") {
    auto* layer_mgr = ui_services<Backend>::layers();
    REQUIRE(layer_mgr != nullptr);

    // Create a popup layer
    auto popup = std::make_unique<panel<Backend>>();
    auto* popup_ptr = popup.get();

    logical_rect const anchor{30.0_lu, 30.0_lu, 5.0_lu, 5.0_lu};
    layer_id popup_id = layer_mgr->show_popup(popup_ptr, anchor, popup_placement::below);

    // Position popup
    logical_rect const popup_bounds{30.0_lu, 35.0_lu, 50.0_lu, 25.0_lu};
    layer_mgr->set_layer_bounds(popup_id, popup_bounds);

    // Clear any dirty regions from setup
    layer_mgr->clear_removed_layer_dirty_regions();
    layer_mgr->clear_layers_changed_flag();

    // Hide layer (not remove)
    layer_mgr->hide_layer(popup_id);

    // Check that hiding marks the area dirty
    CHECK(layer_mgr->layers_changed());
    auto dirty = layer_mgr->get_removed_layer_dirty_regions();
    REQUIRE(dirty.size() == 1);
    // Expanded by 2 units on all sides, clamped to non-negative
    CHECK(dirty[0].x == 28.0_lu);  // 30 - 2
    CHECK(dirty[0].y == 33.0_lu);  // 35 - 2

    // Cleanup
    layer_mgr->remove_layer(popup_id);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "Dirty region expansion clamps to non-negative") {
    auto* layer_mgr = ui_services<Backend>::layers();
    REQUIRE(layer_mgr != nullptr);

    // Create a popup at x=0,y=0 (edge case)
    auto popup = std::make_unique<panel<Backend>>();
    auto* popup_ptr = popup.get();

    logical_rect const anchor{0.0_lu, 0.0_lu, 5.0_lu, 5.0_lu};
    layer_id popup_id = layer_mgr->show_popup(popup_ptr, anchor, popup_placement::below);

    // Position popup at origin
    logical_rect const popup_bounds{0.0_lu, 5.0_lu, 40.0_lu, 20.0_lu};
    layer_mgr->set_layer_bounds(popup_id, popup_bounds);

    // Clear any dirty regions from setup
    layer_mgr->clear_removed_layer_dirty_regions();

    // Remove layer
    layer_mgr->remove_layer(popup_id);

    // Check dirty region - x should be clamped to 0, y should be 3 (5-2)
    auto dirty = layer_mgr->get_removed_layer_dirty_regions();
    REQUIRE(dirty.size() == 1);
    CHECK(dirty[0].x >= 0.0_lu);  // Clamped to non-negative
    CHECK(dirty[0].y == 3.0_lu);  // 5 - 2
    // Width should still expand to cover the right side
    CHECK(dirty[0].width >= 40.0_lu);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "bring_to_front respects z-index bands") {
    auto* layer_mgr = ui_services<Backend>::layers();
    REQUIRE(layer_mgr != nullptr);

    // Create three layers: two windows (z=150) and one popup (z=200)
    auto window1 = std::make_shared<panel<Backend>>();
    auto window2 = std::make_shared<panel<Backend>>();
    auto popup = std::make_shared<panel<Backend>>();

    layer_id w1_id = layer_mgr->add_layer(layer_type::window, window1);  // z=150
    layer_id w2_id = layer_mgr->add_layer(layer_type::window, window2);  // z=150
    layer_id p_id = layer_mgr->add_layer(layer_type::popup, popup);      // z=200

    // Verify initial ordering: popup should be on top
    CHECK(layer_mgr->get_topmost_layer() == p_id);

    // Bring window1 to front
    layer_mgr->bring_to_front(w1_id);

    // Popup should STILL be the topmost layer (z=200 > z=150)
    CHECK(layer_mgr->get_topmost_layer() == p_id);

    // Cleanup
    layer_mgr->clear_all_layers();
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "stable sort preserves bring_to_front ordering") {
    auto* layer_mgr = ui_services<Backend>::layers();
    REQUIRE(layer_mgr != nullptr);

    // Create three layers at the same z-index (windows)
    auto w1 = std::make_shared<panel<Backend>>();
    auto w2 = std::make_shared<panel<Backend>>();
    auto w3 = std::make_shared<panel<Backend>>();

    layer_id w1_id = layer_mgr->add_layer(layer_type::window, w1);  // z=150
    layer_id w2_id = layer_mgr->add_layer(layer_type::window, w2);  // z=150
    layer_id w3_id = layer_mgr->add_layer(layer_type::window, w3);  // z=150

    // w3 should be on top (added last)
    CHECK(layer_mgr->get_topmost_layer() == w3_id);

    // Bring w1 to front
    layer_mgr->bring_to_front(w1_id);

    // Now w1 should be on top (within the z=150 band)
    CHECK(layer_mgr->get_topmost_layer() == w1_id);

    // Add a new layer to trigger sorting
    auto popup = std::make_shared<panel<Backend>>();
    layer_id p_id = layer_mgr->add_layer(layer_type::popup, popup);  // z=200

    // Remove popup to get back to just windows
    layer_mgr->remove_layer(p_id);

    // w1 should still be on top after sort (stable_sort preserves order)
    CHECK(layer_mgr->get_topmost_layer() == w1_id);

    // Cleanup
    layer_mgr->clear_all_layers();
}

} // TEST_SUITE
