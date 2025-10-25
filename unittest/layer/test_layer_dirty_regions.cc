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
#include <onyxui/layer_manager.hh>
#include <onyxui/ui_context.hh>
#include <onyxui/ui_handle.hh>
#include <onyxui/widgets/panel.hh>
#include "../utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

TEST_SUITE("Layer Dirty Region Tracking") {

TEST_CASE("Layer removal marks area as dirty") {
    scoped_ui_context<Backend> ctx;
    auto* layer_mgr = ui_services<Backend>::layers();
    REQUIRE(layer_mgr != nullptr);

    // Create two menus at different positions
    auto menu1 = std::make_unique<panel<Backend>>();
    auto menu2 = std::make_unique<panel<Backend>>();

    auto* menu1_ptr = menu1.get();
    auto* menu2_ptr = menu2.get();

    // Show first menu at position (10, 10) with size (40, 20)
    Backend::rect_type anchor1;
    rect_utils::set_bounds(anchor1, 10, 10, 5, 5);

    layer_id menu1_id = layer_mgr->show_popup(
        menu1_ptr,
        anchor1,
        popup_placement::below
    );

    REQUIRE(menu1_id != layer_id::invalid());
    CHECK(layer_mgr->layer_count() == 1);

    // Position and measure menu1
    Backend::rect_type menu1_bounds;
    rect_utils::set_bounds(menu1_bounds, 10, 15, 40, 20);
    [[maybe_unused]] auto size1 = menu1_ptr->measure(40, 20);
    menu1_ptr->arrange(menu1_bounds);

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

        // Verify the dirty region matches menu1's bounds
        CHECK(rect_utils::get_x(dirty_after[0]) == 10);
        CHECK(rect_utils::get_y(dirty_after[0]) == 15);
        CHECK(rect_utils::get_width(dirty_after[0]) == 40);
        CHECK(rect_utils::get_height(dirty_after[0]) == 20);

        // Clear dirty regions should reset the list
        layer_mgr->clear_removed_layer_dirty_regions();
        auto dirty_cleared = layer_mgr->get_removed_layer_dirty_regions();
        CHECK(dirty_cleared.empty());
    }

    SUBCASE("Menu switching: old menu area marked dirty") {
        // Show second menu at different position (60, 10) with size (40, 30)
        Backend::rect_type anchor2;
        rect_utils::set_bounds(anchor2, 60, 10, 5, 5);

        layer_id menu2_id = layer_mgr->show_popup(
            menu2_ptr,
            anchor2,
            popup_placement::below
        );

        REQUIRE(menu2_id != layer_id::invalid());
        CHECK(layer_mgr->layer_count() == 2);

        // Position menu2
        Backend::rect_type menu2_bounds;
        rect_utils::set_bounds(menu2_bounds, 60, 15, 40, 30);
        [[maybe_unused]] auto size2 = menu2_ptr->measure(40, 30);
        menu2_ptr->arrange(menu2_bounds);

        // Simulate menu switching: remove menu1, keep menu2
        layer_mgr->remove_layer(menu1_id);
        CHECK(layer_mgr->layer_count() == 1);

        // Check that menu1's area is marked dirty
        auto dirty = layer_mgr->get_removed_layer_dirty_regions();
        REQUIRE(dirty.size() == 1);
        CHECK(rect_utils::get_x(dirty[0]) == 10);
        CHECK(rect_utils::get_y(dirty[0]) == 15);
        CHECK(rect_utils::get_width(dirty[0]) == 40);
        CHECK(rect_utils::get_height(dirty[0]) == 20);

        // menu2 should still be visible
        CHECK(layer_mgr->is_layer_visible(menu2_id));
    }

    SUBCASE("Multiple layer removals accumulate dirty regions") {
        // Show second menu
        Backend::rect_type anchor2;
        rect_utils::set_bounds(anchor2, 60, 10, 5, 5);

        layer_id menu2_id = layer_mgr->show_popup(
            menu2_ptr,
            anchor2,
            popup_placement::below
        );

        Backend::rect_type menu2_bounds;
        rect_utils::set_bounds(menu2_bounds, 60, 15, 40, 30);
        [[maybe_unused]] auto sz = menu2_ptr->measure(40, 30);
        menu2_ptr->arrange(menu2_bounds);

        // Remove both menus
        layer_mgr->remove_layer(menu1_id);
        layer_mgr->remove_layer(menu2_id);

        // Both regions should be marked dirty
        auto dirty = layer_mgr->get_removed_layer_dirty_regions();
        REQUIRE(dirty.size() == 2);

        // First removed (menu1)
        CHECK(rect_utils::get_x(dirty[0]) == 10);
        CHECK(rect_utils::get_y(dirty[0]) == 15);

        // Second removed (menu2)
        CHECK(rect_utils::get_x(dirty[1]) == 60);
        CHECK(rect_utils::get_y(dirty[1]) == 15);
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

TEST_CASE("UI handle integrates removed layer dirty regions") {
    scoped_ui_context<Backend> ctx;

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

    Backend::rect_type anchor;
    rect_utils::set_bounds(anchor, 20, 20, 5, 5);

    layer_id popup_id = layer_mgr->show_popup(popup_ptr, anchor, popup_placement::below);
    REQUIRE(popup_id != layer_id::invalid());

    // Position popup
    Backend::rect_type popup_bounds;
    rect_utils::set_bounds(popup_bounds, 20, 25, 30, 15);
    [[maybe_unused]] auto popup_size = popup_ptr->measure(30, 15);
    popup_ptr->arrange(popup_bounds);

    // Display once (popup visible)
    ui.display();

    // Remove popup
    layer_mgr->remove_layer(popup_id);

    // Verify dirty regions are tracked
    auto dirty = layer_mgr->get_removed_layer_dirty_regions();
    REQUIRE(dirty.size() == 1);
    CHECK(rect_utils::get_x(dirty[0]) == 20);
    CHECK(rect_utils::get_y(dirty[0]) == 25);

    // Display again - dirty regions should be used and cleared
    ui.display();

    // After display, dirty regions should be cleared
    auto dirty_after = layer_mgr->get_removed_layer_dirty_regions();
    CHECK(dirty_after.empty());
}

} // TEST_SUITE
