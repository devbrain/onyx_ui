/**
 * @file test_menu_visual.cc
 * @brief Visual rendering tests for menu widget
 * @author Assistant
 * @date 2025-11-03
 *
 * @details
 * Tests menu rendering to catch visual bugs:
 * - Menu items positioned correctly inside border
 * - No offset/overlap issues
 * - Proper border rendering
 */

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/widgets/menu/menu.hh>
#include <../../include/onyxui/widgets/menu/menu_item.hh>
#include <../../include/onyxui/services/ui_context.hh>

using namespace onyxui;
using namespace onyxui::testing;
using Backend = test_canvas_backend;

TEST_SUITE("Menu Visual Rendering") {

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Menu renders with border and items inside") {

    // Create menu with 3 items
    auto test_menu = std::make_unique<menu<Backend>>();

    auto item1 = std::make_unique<menu_item<Backend>>();
    item1->set_mnemonic_text("&New");
    test_menu->add_item(std::move(item1));

    auto item2 = std::make_unique<menu_item<Backend>>();
    item2->set_mnemonic_text("&Open");
    test_menu->add_item(std::move(item2));

    auto item3 = std::make_unique<menu_item<Backend>>();
    item3->set_mnemonic_text("&Save");
    test_menu->add_item(std::move(item3));

    // Measure (render_to_canvas will arrange at 0,0)
    auto measured_size = test_menu->measure(100, 100);

    // Render to canvas (arranges at 0,0 internally)
    auto canvas = render_to_canvas(*test_menu, 40, 20);

    INFO("Menu rendered at (0, 0) with size ",
         size_utils::get_width(measured_size), "x", size_utils::get_height(measured_size));

    SUBCASE("First item renders at expected position") {
        // NOTE: test_canvas doesn't render visible borders, only tracks bounds
        // Menu items should start at row 1 (accounting for border offset)
        std::string row1 = canvas->get_row(1);
        INFO("Row 1 (first item): ", row1);

        // First item should contain "New" text
        CHECK(row1.find("New") != std::string::npos);

        // Row 0 should NOT contain items (border area)
        std::string row0 = canvas->get_row(0);
        CHECK(row0.find("New") == std::string::npos);
    }

    SUBCASE("Items are vertically stacked (no overlap)") {
        std::string row1 = canvas->get_row(1);  // First item
        std::string row2 = canvas->get_row(2);  // Second item
        std::string row3 = canvas->get_row(3);  // Third item

        INFO("Row 1: ", row1);
        INFO("Row 2: ", row2);
        INFO("Row 3: ", row3);

        // Each row should have different content
        CHECK(row1.find("New") != std::string::npos);
        CHECK(row2.find("Open") != std::string::npos);
        CHECK(row3.find("Save") != std::string::npos);

        // Verify no overlap: "New" should only be on row 1
        CHECK(row2.find("New") == std::string::npos);
        CHECK(row3.find("New") == std::string::npos);
    }

    SUBCASE("Menu items positioned at correct X coordinate (inside left border)") {
        std::string row1 = canvas->get_row(1);

        // Find where "New" appears
        size_t new_pos = row1.find("New");
        REQUIRE(new_pos != std::string::npos);

        INFO("'New' found at column ", new_pos);

        // With menu at x=0, border at x=0, item should be at x=1 or later
        // (inside the border, accounting for 1-pixel border)
        CHECK(new_pos >= 1);

        // Should not be too far right (no excessive offset)
        CHECK(new_pos <= 5);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Menu with single item renders correctly") {

    // Create simple menu
    auto test_menu = std::make_unique<menu<Backend>>();

    auto item = std::make_unique<menu_item<Backend>>();
    item->set_mnemonic_text("&File");
    test_menu->add_item(std::move(item));

    // Render to canvas (arranges at 0,0 internally)
    auto canvas = render_to_canvas(*test_menu, 20, 10);

    INFO("Menu with single item");

    // Item should be at row 1 (accounting for border offset)
    std::string row1 = canvas->get_row(1);
    CHECK(row1.find("File") != std::string::npos);

    // Row 0 should NOT contain items (border area)
    std::string row0 = canvas->get_row(0);
    CHECK(row0.find("File") == std::string::npos);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Menu with many items (visual stress test)") {

    auto test_menu = std::make_unique<menu<Backend>>();

    // Create 5 items
    for (int i = 1; i <= 5; ++i) {
        auto item = std::make_unique<menu_item<Backend>>();
        item->set_mnemonic_text("Item &" + std::to_string(i));
        test_menu->add_item(std::move(item));
    }

    // Render to canvas (arranges at 0,0 internally)
    auto canvas = render_to_canvas(*test_menu, 30, 20);

    INFO("Menu with 5 items");

    // Check that all items are visible and in order
    bool found_item1 = false, found_item2 = false, found_item3 = false;
    bool found_item4 = false, found_item5 = false;

    for (int y = 0; y < 20; ++y) {
        std::string row = canvas->get_row(y);
        if (row.find("Item 1") != std::string::npos) found_item1 = true;
        if (row.find("Item 2") != std::string::npos) found_item2 = true;
        if (row.find("Item 3") != std::string::npos) found_item3 = true;
        if (row.find("Item 4") != std::string::npos) found_item4 = true;
        if (row.find("Item 5") != std::string::npos) found_item5 = true;
    }

    CHECK(found_item1);
    CHECK(found_item2);
    CHECK(found_item3);
    CHECK(found_item4);
    CHECK(found_item5);

    // Verify items are in order (no overlap)
    int item1_row = -1, item2_row = -1, item3_row = -1, item4_row = -1, item5_row = -1;
    for (int y = 0; y < 20; ++y) {
        std::string row = canvas->get_row(y);
        if (row.find("Item 1") != std::string::npos && item1_row == -1) item1_row = y;
        if (row.find("Item 2") != std::string::npos && item2_row == -1) item2_row = y;
        if (row.find("Item 3") != std::string::npos && item3_row == -1) item3_row = y;
        if (row.find("Item 4") != std::string::npos && item4_row == -1) item4_row = y;
        if (row.find("Item 5") != std::string::npos && item5_row == -1) item5_row = y;
    }

    INFO("Item positions - Item1:", item1_row, " Item2:", item2_row,
         " Item3:", item3_row, " Item4:", item4_row, " Item5:", item5_row);

    // Items should be in increasing row order
    CHECK(item1_row < item2_row);
    CHECK(item2_row < item3_row);
    CHECK(item3_row < item4_row);
    CHECK(item4_row < item5_row);
}

} // TEST_SUITE
