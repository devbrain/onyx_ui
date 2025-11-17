/**
 * @file test_menu_border_layout.cc
 * @brief Tests for menu border and item layout
 * @author Assistant
 * @date 2025-10-25
 *
 * @details
 * Reproduces and tests fix for menu border layout bug:
 * - Menu border should contain ALL items
 * - All items should be positioned inside the border, not on it
 * - Last items should not extend beyond the menu bounds
 */

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/widgets/menu/menu.hh>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/widgets/menu/menu_item.hh>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/services/ui_context.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;
using namespace onyxui::testing;
using Backend = test_canvas_backend;

TEST_SUITE("Menu Border Layout") {

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Menu border contains all items") {

    // Create menu with 5 items
    auto test_menu = std::make_unique<menu<Backend>>();

    auto item1 = std::make_unique<menu_item<Backend>>();
    item1->set_mnemonic_text("&New");
    auto* item1_ptr = item1.get();
    test_menu->add_item(std::move(item1));

    auto item2 = std::make_unique<menu_item<Backend>>();
    item2->set_mnemonic_text("&Open");
    auto* item2_ptr = item2.get();
    test_menu->add_item(std::move(item2));

    auto item3 = std::make_unique<menu_item<Backend>>();
    item3->set_mnemonic_text("&Save");
    auto* item3_ptr = item3.get();
    test_menu->add_item(std::move(item3));

    auto item4 = std::make_unique<menu_item<Backend>>();
    item4->set_mnemonic_text("Save &As...");
    auto* item4_ptr = item4.get();
    test_menu->add_item(std::move(item4));

    auto item5 = std::make_unique<menu_item<Backend>>();
    item5->set_mnemonic_text("E&xit");
    auto* item5_ptr = item5.get();
    test_menu->add_item(std::move(item5));

    // Measure and arrange the menu
    auto measured_size = test_menu->measure(100, 100);

    Backend::rect_type menu_bounds;
    rect_utils::set_bounds(menu_bounds, 10, 10,
                          size_utils::get_width(measured_size),
                          size_utils::get_height(measured_size));
    test_menu->arrange(geometry::relative_rect<Backend>{menu_bounds});

    auto menu_final_bounds = test_menu->bounds();

    SUBCASE("All items are within menu bounds") {
        // Get all item bounds
        auto b1 = item1_ptr->bounds();
        auto b2 = item2_ptr->bounds();
        auto b3 = item3_ptr->bounds();
        auto b4 = item4_ptr->bounds();
        auto b5 = item5_ptr->bounds();

        // Check that each item is fully contained within menu bounds
        // Item bottom should be <= menu bottom
        int menu_bottom = rect_utils::get_y(menu_final_bounds) + rect_utils::get_height(menu_final_bounds);

        int item1_bottom = rect_utils::get_y(b1) + rect_utils::get_height(b1);
        int item2_bottom = rect_utils::get_y(b2) + rect_utils::get_height(b2);
        int item3_bottom = rect_utils::get_y(b3) + rect_utils::get_height(b3);
        int item4_bottom = rect_utils::get_y(b4) + rect_utils::get_height(b4);
        int item5_bottom = rect_utils::get_y(b5) + rect_utils::get_height(b5);

        // CRITICAL: Last items (4 and 5) should be within menu bounds
        CHECK(item4_bottom <= menu_bottom);
        CHECK(item5_bottom <= menu_bottom);

        // All items should be within bounds
        CHECK(item1_bottom <= menu_bottom);
        CHECK(item2_bottom <= menu_bottom);
        CHECK(item3_bottom <= menu_bottom);
    }

    SUBCASE("All items are inside border (not on border edge)") {
        // RELATIVE COORDINATES: items positioned at (0,0) relative to menu's content area
        // Menu's content area already accounts for border offset internally

        // First item should start at content area origin
        auto b1 = item1_ptr->bounds();
        CHECK(rect_utils::get_y(b1) == 0);  // Relative to menu's content area

        // Last item should end before border (menu_bottom - 1)
        auto b5 = item5_ptr->bounds();
        int menu_bottom = rect_utils::get_y(menu_final_bounds) + rect_utils::get_height(menu_final_bounds);
        int item5_bottom = rect_utils::get_y(b5) + rect_utils::get_height(b5);

        // Item should end at or before (menu_bottom - 1) to stay inside border
        CHECK(item5_bottom <= menu_bottom - 1);
    }

    SUBCASE("Menu size accounts for all children plus border") {
        // Each item has height (measured), they stack vertically
        auto h1 = rect_utils::get_height(item1_ptr->bounds());
        auto h2 = rect_utils::get_height(item2_ptr->bounds());
        auto h3 = rect_utils::get_height(item3_ptr->bounds());
        auto h4 = rect_utils::get_height(item4_ptr->bounds());
        auto h5 = rect_utils::get_height(item5_ptr->bounds());

        int total_items_height = h1 + h2 + h3 + h4 + h5;

        // Menu bounds should be at least as tall as all items + border (2px total)
        int menu_height = rect_utils::get_height(menu_final_bounds);
        CHECK(menu_height >= total_items_height + 2);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Menu with many items (stress test)") {

    auto test_menu = std::make_unique<menu<Backend>>();

    // Create 10 items to stress-test the layout
    std::vector<menu_item<Backend>*> item_ptrs;
    for (int i = 0; i < 10; ++i) {
        auto item = std::make_unique<menu_item<Backend>>();
        item->set_mnemonic_text("Item " + std::to_string(i));
        item_ptrs.push_back(item.get());
        test_menu->add_item(std::move(item));
    }

    // Measure and arrange
    auto measured_size = test_menu->measure(200, 300);

    Backend::rect_type menu_bounds;
    rect_utils::set_bounds(menu_bounds, 0, 0,
                          size_utils::get_width(measured_size),
                          size_utils::get_height(measured_size));
    test_menu->arrange(geometry::relative_rect<Backend>{menu_bounds});

    auto menu_final_bounds = test_menu->bounds();
    int menu_bottom = rect_utils::get_y(menu_final_bounds) + rect_utils::get_height(menu_final_bounds);

    // Check that ALL items are within menu bounds
    for (size_t i = 0; i < item_ptrs.size(); ++i) {
        auto item_bounds = item_ptrs[i]->bounds();
        int item_bottom = rect_utils::get_y(item_bounds) + rect_utils::get_height(item_bounds);

        INFO("Item " << i << " bottom: " << item_bottom << ", menu bottom: " << menu_bottom);
        CHECK(item_bottom <= menu_bottom);
    }

    // Last item should be inside border
    auto last_item_bounds = item_ptrs.back()->bounds();
    int last_item_bottom = rect_utils::get_y(last_item_bounds) + rect_utils::get_height(last_item_bounds);
    CHECK(last_item_bottom <= menu_bottom - 1);  // -1 for border
}

} // TEST_SUITE
