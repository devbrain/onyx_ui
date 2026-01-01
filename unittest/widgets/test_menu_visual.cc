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
    auto measured_size = test_menu->measure(100_lu, 100_lu);

    // Render to canvas (arranges at 0,0 internally)
    auto canvas = render_to_canvas(*test_menu, 40, 20);

    INFO("Menu rendered at (0, 0) with size ",
         measured_size.width.to_int(), "x", measured_size.height.to_int());

    // Find row positions for each item (items may have vertical padding)
    int new_row = -1, open_row = -1, save_row = -1;
    for (int y = 0; y < 20; ++y) {
        std::string row = canvas->get_row(y);
        if (new_row == -1 && row.find("New") != std::string::npos) new_row = y;
        if (open_row == -1 && row.find("Open") != std::string::npos) open_row = y;
        if (save_row == -1 && row.find("Save") != std::string::npos) save_row = y;
    }

    SUBCASE("All items are rendered") {
        INFO("New at row ", new_row, ", Open at row ", open_row, ", Save at row ", save_row);
        CHECK(new_row != -1);
        CHECK(open_row != -1);
        CHECK(save_row != -1);
    }

    SUBCASE("First item renders inside border") {
        // Row 0 should NOT contain items (border area)
        std::string row0 = canvas->get_row(0);
        CHECK(row0.find("New") == std::string::npos);

        // First item should be at row 1 or later (inside content area)
        CHECK(new_row >= 1);
    }

    SUBCASE("Items are vertically stacked (no overlap)") {
        INFO("New at row ", new_row, ", Open at row ", open_row, ", Save at row ", save_row);

        // Items should be in order
        CHECK(new_row < open_row);
        CHECK(open_row < save_row);

        // Verify no overlap: each item text appears only on its row
        if (new_row != -1) {
            std::string new_row_str = canvas->get_row(new_row);
            CHECK(new_row_str.find("Open") == std::string::npos);
            CHECK(new_row_str.find("Save") == std::string::npos);
        }
    }

    SUBCASE("Menu items positioned at correct X coordinate (inside left border)") {
        REQUIRE(new_row != -1);
        std::string row = canvas->get_row(new_row);

        // Find where "New" appears
        size_t new_pos = row.find("New");
        REQUIRE(new_pos != std::string::npos);

        INFO("'New' found at column ", new_pos);

        // With menu at x=0, border at x=0, item should be at x=1 or later
        // (inside the border, accounting for 1-pixel border + padding)
        CHECK(new_pos >= 1);

        // Should not be too far right (no excessive offset)
        CHECK(new_pos <= 10);
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

    // Find where "File" is rendered (may have vertical padding)
    int file_row = -1;
    for (int y = 0; y < 10; ++y) {
        std::string row = canvas->get_row(y);
        if (row.find("File") != std::string::npos) {
            file_row = y;
            break;
        }
    }

    // Item should be found somewhere
    CHECK(file_row != -1);

    // Item should be inside content area (not at row 0 which is border)
    CHECK(file_row >= 1);

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
