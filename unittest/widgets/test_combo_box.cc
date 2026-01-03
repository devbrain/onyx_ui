//
// Unit tests for combo_box widget (simple API)
//

#include <doctest/doctest.h>
#include <onyxui/widgets/input/combo_box.hh>
#include "../utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

TEST_CASE("combo_box - Construction") {
    auto combo = std::make_unique<combo_box<Backend>>();

    CHECK(combo != nullptr);
    CHECK(combo->count() == 0);
    CHECK(combo->current_index() == -1);
    CHECK(combo->current_text().empty());
    CHECK_FALSE(combo->is_popup_open());
}

TEST_CASE("combo_box - Construction with items") {
    auto combo = std::make_unique<combo_box<Backend>>(
        std::vector<std::string>{"Apple", "Banana", "Cherry"}
    );

    CHECK(combo->count() == 3);
    CHECK(combo->item_text(0) == "Apple");
    CHECK(combo->item_text(1) == "Banana");
    CHECK(combo->item_text(2) == "Cherry");
    CHECK(combo->current_index() == -1);  // No selection by default
}

TEST_CASE("combo_box - add_item") {
    auto combo = std::make_unique<combo_box<Backend>>();

    combo->add_item("First");
    combo->add_item("Second");
    combo->add_item("Third");

    CHECK(combo->count() == 3);
    CHECK(combo->item_text(0) == "First");
    CHECK(combo->item_text(1) == "Second");
    CHECK(combo->item_text(2) == "Third");
}

TEST_CASE("combo_box - set_items") {
    auto combo = std::make_unique<combo_box<Backend>>();
    combo->add_item("Old Item");

    combo->set_items({"Small", "Medium", "Large"});

    CHECK(combo->count() == 3);
    CHECK(combo->item_text(0) == "Small");
    CHECK(combo->item_text(1) == "Medium");
    CHECK(combo->item_text(2) == "Large");
}

TEST_CASE("combo_box - insert_item") {
    auto combo = std::make_unique<combo_box<Backend>>();
    combo->set_items({"A", "C"});

    combo->insert_item(1, "B");

    CHECK(combo->count() == 3);
    CHECK(combo->item_text(0) == "A");
    CHECK(combo->item_text(1) == "B");
    CHECK(combo->item_text(2) == "C");
}

TEST_CASE("combo_box - remove_item") {
    auto combo = std::make_unique<combo_box<Backend>>();
    combo->set_items({"A", "B", "C"});

    combo->remove_item(1);

    CHECK(combo->count() == 2);
    CHECK(combo->item_text(0) == "A");
    CHECK(combo->item_text(1) == "C");
}

TEST_CASE("combo_box - clear") {
    auto combo = std::make_unique<combo_box<Backend>>();
    combo->set_items({"A", "B", "C"});
    combo->set_current_index(1);

    combo->clear();

    CHECK(combo->count() == 0);
    CHECK(combo->current_index() == -1);
}

TEST_CASE("combo_box - set_current_index") {
    auto combo = std::make_unique<combo_box<Backend>>();
    combo->set_items({"Small", "Medium", "Large"});

    combo->set_current_index(1);

    CHECK(combo->current_index() == 1);
    CHECK(combo->current_text() == "Medium");
}

TEST_CASE("combo_box - current_index_changed signal") {
    auto combo = std::make_unique<combo_box<Backend>>();
    combo->set_items({"A", "B", "C"});

    int emitted_index = -999;
    combo->current_index_changed.connect([&](int index) {
        emitted_index = index;
    });

    combo->set_current_index(2);

    CHECK(emitted_index == 2);
}

TEST_CASE("combo_box - invalid index") {
    auto combo = std::make_unique<combo_box<Backend>>();
    combo->set_items({"A", "B", "C"});

    combo->set_current_index(100);  // Out of range

    CHECK(combo->current_index() == -1);
    CHECK(combo->current_text().empty());
}

TEST_CASE("combo_box - clear selection") {
    auto combo = std::make_unique<combo_box<Backend>>();
    combo->set_items({"A", "B", "C"});
    combo->set_current_index(1);

    combo->set_current_index(-1);  // Clear selection

    CHECK(combo->current_index() == -1);
    CHECK(combo->current_text().empty());
}

TEST_CASE("combo_box - find_text") {
    auto combo = std::make_unique<combo_box<Backend>>();
    combo->set_items({"Apple", "Banana", "Cherry"});

    CHECK(combo->find_text("Banana") == 1);
    CHECK(combo->find_text("NotFound") == -1);
}

TEST_CASE("combo_box - set_current_text") {
    auto combo = std::make_unique<combo_box<Backend>>();
    combo->set_items({"Apple", "Banana", "Cherry"});

    bool found = combo->set_current_text("Cherry");

    CHECK(found == true);
    CHECK(combo->current_index() == 2);
    CHECK(combo->current_text() == "Cherry");
}

TEST_CASE("combo_box - set_current_text not found") {
    auto combo = std::make_unique<combo_box<Backend>>();
    combo->set_items({"Apple", "Banana", "Cherry"});

    bool found = combo->set_current_text("Orange");

    CHECK(found == false);
    CHECK(combo->current_index() == -1);
}

TEST_CASE("combo_box - set_item_text") {
    auto combo = std::make_unique<combo_box<Backend>>();
    combo->set_items({"A", "B", "C"});

    combo->set_item_text(1, "Updated");

    CHECK(combo->item_text(1) == "Updated");
}

TEST_CASE("combo_box - item_text invalid index") {
    auto combo = std::make_unique<combo_box<Backend>>();
    combo->set_items({"A", "B", "C"});

    CHECK(combo->item_text(-1).empty());
    CHECK(combo->item_text(100).empty());
}

TEST_CASE("combo_box - remove_item adjusts selection") {
    auto combo = std::make_unique<combo_box<Backend>>();
    combo->set_items({"A", "B", "C", "D"});
    combo->set_current_index(2);  // Select "C"

    combo->remove_item(0);  // Remove "A"

    // Selection should be adjusted (was 2, now 1)
    CHECK(combo->current_index() == 1);
    CHECK(combo->current_text() == "C");
}

TEST_CASE("combo_box - remove_item clears selection if current removed") {
    auto combo = std::make_unique<combo_box<Backend>>();
    combo->set_items({"A", "B", "C"});
    combo->set_current_index(1);  // Select "B"

    combo->remove_item(1);  // Remove selected item

    CHECK(combo->current_index() == -1);
    CHECK(combo->current_text().empty());
}

TEST_CASE("combo_box - model accessor") {
    auto combo = std::make_unique<combo_box<Backend>>();
    combo->set_items({"A", "B", "C"});

    const auto* model = combo->model();

    CHECK(model != nullptr);
    CHECK(model->row_count() == 3);
}

TEST_CASE("combo_box - view accessor") {
    auto combo = std::make_unique<combo_box<Backend>>();

    auto* view = combo->view();

    CHECK(view != nullptr);
}
