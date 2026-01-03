//
// Unit tests for list_box widget (simple API)
//

#include <doctest/doctest.h>
#include <onyxui/widgets/input/list_box.hh>
#include "../utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

TEST_CASE("list_box - Construction") {
    auto list = std::make_unique<list_box<Backend>>();

    CHECK(list != nullptr);
    CHECK(list->count() == 0);
    CHECK(list->current_index() == -1);
    CHECK(list->current_text().empty());
}

TEST_CASE("list_box - Construction with items") {
    auto list = std::make_unique<list_box<Backend>>(
        std::vector<std::string>{"Apple", "Banana", "Cherry"}
    );

    CHECK(list->count() == 3);
    CHECK(list->item_text(0) == "Apple");
    CHECK(list->item_text(1) == "Banana");
    CHECK(list->item_text(2) == "Cherry");
    CHECK(list->current_index() == -1);  // No selection by default
}

TEST_CASE("list_box - add_item") {
    auto list = std::make_unique<list_box<Backend>>();

    list->add_item("First");
    list->add_item("Second");
    list->add_item("Third");

    CHECK(list->count() == 3);
    CHECK(list->item_text(0) == "First");
    CHECK(list->item_text(1) == "Second");
    CHECK(list->item_text(2) == "Third");
}

TEST_CASE("list_box - set_items") {
    auto list = std::make_unique<list_box<Backend>>();
    list->add_item("Old Item");

    list->set_items({"Small", "Medium", "Large"});

    CHECK(list->count() == 3);
    CHECK(list->item_text(0) == "Small");
    CHECK(list->item_text(1) == "Medium");
    CHECK(list->item_text(2) == "Large");
}

TEST_CASE("list_box - insert_item") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"A", "C"});

    list->insert_item(1, "B");

    CHECK(list->count() == 3);
    CHECK(list->item_text(0) == "A");
    CHECK(list->item_text(1) == "B");
    CHECK(list->item_text(2) == "C");
}

TEST_CASE("list_box - remove_item") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"A", "B", "C"});

    list->remove_item(1);

    CHECK(list->count() == 2);
    CHECK(list->item_text(0) == "A");
    CHECK(list->item_text(1) == "C");
}

TEST_CASE("list_box - clear") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"A", "B", "C"});
    list->set_current_index(1);

    list->clear();

    CHECK(list->count() == 0);
    CHECK(list->current_index() == -1);
}

TEST_CASE("list_box - set_current_index") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"Small", "Medium", "Large"});

    list->set_current_index(1);

    CHECK(list->current_index() == 1);
    CHECK(list->current_text() == "Medium");
}

TEST_CASE("list_box - current_index_changed signal") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"A", "B", "C"});

    int emitted_index = -999;
    list->current_index_changed.connect([&](int index) {
        emitted_index = index;
    });

    list->set_current_index(2);

    CHECK(emitted_index == 2);
}

TEST_CASE("list_box - invalid index") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"A", "B", "C"});

    list->set_current_index(100);  // Out of range

    CHECK(list->current_index() == -1);
    CHECK(list->current_text().empty());
}

TEST_CASE("list_box - clear selection") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"A", "B", "C"});
    list->set_current_index(1);

    list->set_current_index(-1);  // Clear selection

    CHECK(list->current_index() == -1);
    CHECK(list->current_text().empty());
}

TEST_CASE("list_box - find_text") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"Apple", "Banana", "Cherry"});

    CHECK(list->find_text("Banana") == 1);
    CHECK(list->find_text("NotFound") == -1);
}

TEST_CASE("list_box - set_current_text") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"Apple", "Banana", "Cherry"});

    bool found = list->set_current_text("Cherry");

    CHECK(found == true);
    CHECK(list->current_index() == 2);
    CHECK(list->current_text() == "Cherry");
}

TEST_CASE("list_box - set_item_text") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"A", "B", "C"});

    list->set_item_text(1, "Updated");

    CHECK(list->item_text(1) == "Updated");
}

TEST_CASE("list_box - item_text invalid index") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"A", "B", "C"});

    CHECK(list->item_text(-1).empty());
    CHECK(list->item_text(100).empty());
}

TEST_CASE("list_box - remove_item adjusts selection") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"A", "B", "C", "D"});
    list->set_current_index(2);  // Select "C"

    list->remove_item(0);  // Remove "A"

    // Selection should be adjusted (was 2, now 1)
    CHECK(list->current_index() == 1);
    CHECK(list->current_text() == "C");
}

TEST_CASE("list_box - remove_item clears selection if current removed") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"A", "B", "C"});
    list->set_current_index(1);  // Select "B"

    list->remove_item(1);  // Remove selected item

    CHECK(list->current_index() == -1);
    CHECK(list->current_text().empty());
}

TEST_CASE("list_box - model accessor") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"A", "B", "C"});

    const auto* model = list->model();

    CHECK(model != nullptr);
    CHECK(model->row_count() == 3);
}

TEST_CASE("list_box - view accessor") {
    auto list = std::make_unique<list_box<Backend>>();

    auto* view = list->view();

    CHECK(view != nullptr);
}

TEST_CASE("list_box - is_selected") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"A", "B", "C"});

    list->set_current_index(1);

    CHECK_FALSE(list->is_selected(0));
    CHECK(list->is_selected(1));
    CHECK_FALSE(list->is_selected(2));
}

TEST_CASE("list_box - select and deselect") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"A", "B", "C"});
    list->set_selection_mode(selection_mode::multi_selection);

    list->select(0);
    list->select(2);

    CHECK(list->is_selected(0));
    CHECK_FALSE(list->is_selected(1));
    CHECK(list->is_selected(2));

    list->deselect(0);

    CHECK_FALSE(list->is_selected(0));
    CHECK(list->is_selected(2));
}

TEST_CASE("list_box - clear_selection") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"A", "B", "C"});
    list->set_selection_mode(selection_mode::multi_selection);
    list->select(0);
    list->select(1);
    list->select(2);

    list->clear_selection();

    CHECK_FALSE(list->is_selected(0));
    CHECK_FALSE(list->is_selected(1));
    CHECK_FALSE(list->is_selected(2));
}

TEST_CASE("list_box - select_all") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"A", "B", "C"});
    list->set_selection_mode(selection_mode::multi_selection);

    list->select_all();

    CHECK(list->is_selected(0));
    CHECK(list->is_selected(1));
    CHECK(list->is_selected(2));
}

TEST_CASE("list_box - selected_indices") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"A", "B", "C", "D"});
    list->set_selection_mode(selection_mode::multi_selection);
    list->select(1);
    list->select(3);

    auto indices = list->selected_indices();

    CHECK(indices.size() == 2);
    // Indices might be in any order, so check both are present
    bool has_1 = std::find(indices.begin(), indices.end(), 1) != indices.end();
    bool has_3 = std::find(indices.begin(), indices.end(), 3) != indices.end();
    CHECK(has_1);
    CHECK(has_3);
}

TEST_CASE("list_box - selection mode") {
    auto list = std::make_unique<list_box<Backend>>();

    CHECK(list->get_selection_mode() == selection_mode::single_selection);

    list->set_selection_mode(selection_mode::multi_selection);
    CHECK(list->get_selection_mode() == selection_mode::multi_selection);
}

TEST_CASE("list_box - scroll_to") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"A", "B", "C", "D", "E", "F", "G", "H", "I", "J"});

    // Should not throw
    list->scroll_to(5);
    list->scroll_to(0);
    list->scroll_to(9);
}

TEST_CASE("list_box - scroll offset") {
    auto list = std::make_unique<list_box<Backend>>();
    list->set_items({"A", "B", "C"});

    CHECK(list->scroll_offset() == 0.0);

    list->set_scroll_offset(10.0);
    // Note: actual scroll offset may be clamped by the view
}
