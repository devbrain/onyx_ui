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

        // Verify the dirty region matches menu1's bounds + shadow margin (2 units)
        CHECK(dirty_after[0].x == 10.0_lu);
        CHECK(dirty_after[0].y == 15.0_lu);
        CHECK(dirty_after[0].width == 42.0_lu);  // 40 + 2 for shadow
        CHECK(dirty_after[0].height == 22.0_lu);  // 20 + 2 for shadow

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

        // Check that menu1's area is marked dirty (including shadow margin)
        auto dirty = layer_mgr->get_removed_layer_dirty_regions();
        REQUIRE(dirty.size() == 1);
        CHECK(dirty[0].x == 10.0_lu);
        CHECK(dirty[0].y == 15.0_lu);
        CHECK(dirty[0].width == 42.0_lu);  // 40 + 2 for shadow
        CHECK(dirty[0].height == 22.0_lu);  // 20 + 2 for shadow

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

        // First removed (menu1)
        CHECK(dirty[0].x == 10.0_lu);
        CHECK(dirty[0].y == 15.0_lu);

        // Second removed (menu2)
        CHECK(dirty[1].x == 60.0_lu);
        CHECK(dirty[1].y == 15.0_lu);
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

    // Verify dirty regions are tracked
    auto dirty = layer_mgr->get_removed_layer_dirty_regions();
    REQUIRE(dirty.size() == 1);
    CHECK(dirty[0].x == 20.0_lu);
    CHECK(dirty[0].y == 25.0_lu);

    // Display again - dirty regions should be used and cleared
    ui.display();

    // After display, dirty regions should be cleared
    auto dirty_after = layer_mgr->get_removed_layer_dirty_regions();
    CHECK(dirty_after.empty());
}

} // TEST_SUITE
