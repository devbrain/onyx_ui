/**
 * @file test_submenu_infrastructure.cc
 * @brief Unit tests for Phase 3 submenu infrastructure
 * @author Assistant
 * @date 2025-10-28
 *
 * @details
 * Tests the submenu data structure and API added in Phase 3:
 * - set_submenu() / has_submenu() / submenu() / release_submenu()
 * - Ownership semantics
 * - Integration with menu_item_type
 *
 * NOTE: Rendering tests deferred to Phase 4/5
 */

#include <doctest/doctest.h>
#include <../../include/onyxui/widgets/menu/menu_item.hh>
#include <../../include/onyxui/widgets/menu/menu.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

// ======================================================================
// Test Suite: menu_item - Submenu Infrastructure (Phase 3)
// ======================================================================

TEST_SUITE("menu_item::submenu_infrastructure") {

    TEST_CASE("menu_item without submenu by default") {
        auto item = std::make_unique<menu_item<Backend>>("File");

        CHECK_FALSE(item->has_submenu());
        CHECK(item->submenu() == nullptr);
        CHECK(item->type() == menu_item_type::normal);
    }

    TEST_CASE("set_submenu() takes ownership") {
        auto item = std::make_unique<menu_item<Backend>>("File");
        auto submenu = std::make_unique<menu<Backend>>();

        // Add item to submenu to verify ownership
        auto submenu_item = std::make_unique<menu_item<Backend>>("Recent");
        submenu->add_item(std::move(submenu_item));

        item->set_submenu(std::move(submenu));

        CHECK(item->has_submenu());
        CHECK(item->submenu() != nullptr);
        CHECK(item->type() == menu_item_type::submenu);

        // Verify submenu contents are accessible
        CHECK(item->submenu()->items().size() == 1);
    }

    TEST_CASE("set_submenu(nullptr) removes submenu") {
        auto item = std::make_unique<menu_item<Backend>>("File");
        auto submenu = std::make_unique<menu<Backend>>();

        item->set_submenu(std::move(submenu));
        CHECK(item->has_submenu());

        item->set_submenu(nullptr);
        CHECK_FALSE(item->has_submenu());
        CHECK(item->submenu() == nullptr);
        CHECK(item->type() == menu_item_type::normal);
    }

    TEST_CASE("release_submenu() transfers ownership") {
        auto item = std::make_unique<menu_item<Backend>>("File");
        auto submenu = std::make_unique<menu<Backend>>();

        auto submenu_item = std::make_unique<menu_item<Backend>>("Recent");
        submenu->add_item(std::move(submenu_item));

        item->set_submenu(std::move(submenu));
        CHECK(item->has_submenu());

        // Release ownership
        auto released = item->release_submenu();

        CHECK_FALSE(item->has_submenu());
        CHECK(item->submenu() == nullptr);
        CHECK(item->type() == menu_item_type::normal);

        // Verify released submenu is valid
        CHECK(released != nullptr);
        CHECK(released->items().size() == 1);
    }

    TEST_CASE("Replacing submenu destroys previous") {
        auto item = std::make_unique<menu_item<Backend>>("File");

        auto submenu1 = std::make_unique<menu<Backend>>();
        auto item1 = std::make_unique<menu_item<Backend>>("Item1");
        submenu1->add_item(std::move(item1));

        auto submenu2 = std::make_unique<menu<Backend>>();
        auto item2 = std::make_unique<menu_item<Backend>>("Item2");
        auto item3 = std::make_unique<menu_item<Backend>>("Item3");
        submenu2->add_item(std::move(item2));
        submenu2->add_item(std::move(item3));

        item->set_submenu(std::move(submenu1));
        CHECK(item->submenu()->items().size() == 1);

        // Replace with submenu2 (submenu1 should be destroyed)
        item->set_submenu(std::move(submenu2));
        CHECK(item->submenu()->items().size() == 2);
    }

    TEST_CASE("Nested submenus (arbitrary depth)") {
        auto item = std::make_unique<menu_item<Backend>>("File");

        // Level 1 submenu
        auto level1 = std::make_unique<menu<Backend>>();
        auto level1_item = std::make_unique<menu_item<Backend>>("Recent Files");

        // Level 2 submenu
        auto level2 = std::make_unique<menu<Backend>>();
        auto level2_item = std::make_unique<menu_item<Backend>>("Project 1");
        level2->add_item(std::move(level2_item));

        level1_item->set_submenu(std::move(level2));
        level1->add_item(std::move(level1_item));
        item->set_submenu(std::move(level1));

        // Verify hierarchy
        CHECK(item->has_submenu());
        CHECK(item->submenu()->items().size() == 1);

        auto* recent_item = item->submenu()->items()[0];
        CHECK(recent_item->has_submenu());
        CHECK(recent_item->submenu()->items().size() == 1);
    }

    TEST_CASE("menu_item destruction cleans up submenu") {
        // RAII test: verify destruction doesn't crash or leak (checked by sanitizers)
        CHECK_NOTHROW({
            auto item = std::make_unique<menu_item<Backend>>("File");
            auto submenu = std::make_unique<menu<Backend>>();

            auto submenu_item = std::make_unique<menu_item<Backend>>("Recent");
            submenu->add_item(std::move(submenu_item));

            item->set_submenu(std::move(submenu));

            // item goes out of scope - submenu should be destroyed cleanly
        });
    }

    TEST_CASE("submenu() returns consistent pointer") {
        auto item = std::make_unique<menu_item<Backend>>("File");
        auto submenu = std::make_unique<menu<Backend>>();

        auto submenu_ptr = submenu.get();
        item->set_submenu(std::move(submenu));

        // Pointer should be consistent
        CHECK(item->submenu() == submenu_ptr);
        CHECK(item->submenu() == submenu_ptr);  // Second call returns same
    }

} // TEST_SUITE menu_item::submenu_infrastructure
