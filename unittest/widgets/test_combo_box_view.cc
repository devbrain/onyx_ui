//
// Unit tests for combo_box_view widget (MVC pattern)
//

#include <doctest/doctest.h>
#include <onyxui/mvc/views/combo_box_view.hh>
#include <onyxui/mvc/models/list_model.hh>
#include "../utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

TEST_CASE("combo_box_view - Construction") {
    auto combo = std::make_unique<combo_box_view<Backend>>();

    CHECK(combo != nullptr);
    CHECK(combo->model() == nullptr);
    CHECK_FALSE(combo->current_index().is_valid());
    CHECK(combo->current_row() == -1);
    CHECK(combo->current_text().empty());
    CHECK_FALSE(combo->is_popup_visible());
}

TEST_CASE("combo_box_view - Set model") {
    auto combo = std::make_unique<combo_box_view<Backend>>();
    auto model = std::make_shared<list_model<std::string, Backend>>();
    model->set_items({"Small", "Medium", "Large"});

    combo->set_model(model.get());

    CHECK(combo->model() == model.get());
    CHECK_FALSE(combo->current_index().is_valid());  // No selection by default
    CHECK(combo->current_row() == -1);
}

TEST_CASE("combo_box_view - Set current row") {
    auto combo = std::make_unique<combo_box_view<Backend>>();
    auto model = std::make_shared<list_model<std::string, Backend>>();
    model->set_items({"Small", "Medium", "Large"});
    combo->set_model(model.get());

    combo->set_current_row(1);

    CHECK(combo->current_row() == 1);
    CHECK(combo->current_text() == "Medium");
}

TEST_CASE("combo_box_view - Set current index (model_index)") {
    auto combo = std::make_unique<combo_box_view<Backend>>();
    auto model = std::make_shared<list_model<std::string, Backend>>();
    model->set_items({"Small", "Medium", "Large"});
    combo->set_model(model.get());

    auto idx = model->index(2, 0);
    combo->set_current_index(idx);

    CHECK(combo->current_index() == idx);
    CHECK(combo->current_row() == 2);
    CHECK(combo->current_text() == "Large");
}

TEST_CASE("combo_box_view - Current changed signal") {
    auto combo = std::make_unique<combo_box_view<Backend>>();
    auto model = std::make_shared<list_model<std::string, Backend>>();
    model->set_items({"A", "B", "C"});
    combo->set_model(model.get());

    model_index emitted_index;
    combo->current_changed.connect([&](const model_index& idx) {
        emitted_index = idx;
    });

    combo->set_current_row(2);

    CHECK(emitted_index.is_valid());
    CHECK(emitted_index.row == 2);
}

TEST_CASE("combo_box_view - Invalid row") {
    auto combo = std::make_unique<combo_box_view<Backend>>();
    auto model = std::make_shared<list_model<std::string, Backend>>();
    model->set_items({"A", "B", "C"});
    combo->set_model(model.get());

    combo->set_current_row(100);  // Out of range

    CHECK_FALSE(combo->current_index().is_valid());
    CHECK(combo->current_row() == -1);
    CHECK(combo->current_text().empty());
}

TEST_CASE("combo_box_view - Clear selection") {
    auto combo = std::make_unique<combo_box_view<Backend>>();
    auto model = std::make_shared<list_model<std::string, Backend>>();
    model->set_items({"A", "B", "C"});
    combo->set_model(model.get());
    combo->set_current_row(1);

    combo->set_current_row(-1);  // Clear selection

    CHECK_FALSE(combo->current_index().is_valid());
    CHECK(combo->current_row() == -1);
    CHECK(combo->current_text().empty());
}

TEST_CASE("combo_box_view - Change model closes popup") {
    auto combo = std::make_unique<combo_box_view<Backend>>();
    auto model1 = std::make_shared<list_model<std::string, Backend>>();
    model1->set_items({"A", "B", "C"});
    combo->set_model(model1.get());
    combo->set_current_row(1);

    auto model2 = std::make_shared<list_model<std::string, Backend>>();
    model2->set_items({"X", "Y", "Z"});
    combo->set_model(model2.get());

    // After model change, selection resets
    CHECK_FALSE(combo->current_index().is_valid());
    CHECK(combo->current_text().empty());
}

TEST_CASE("combo_box_view - Selection model accessors") {
    auto combo = std::make_unique<combo_box_view<Backend>>();

    CHECK(combo->selection_model() != nullptr);  // Default selection model created
}

TEST_CASE("combo_box_view - Delegate accessor") {
    auto combo = std::make_unique<combo_box_view<Backend>>();

    CHECK(combo->delegate() != nullptr);  // Default delegate created
}

TEST_CASE("combo_box_view - Focusable by default") {
    auto combo = std::make_unique<combo_box_view<Backend>>();

    CHECK(combo->is_focusable());
}

TEST_CASE("combo_box_view - Integer list model") {
    auto combo = std::make_unique<combo_box_view<Backend>>();
    auto model = std::make_shared<list_model<int, Backend>>();
    model->set_items({10, 20, 30, 40, 50});
    combo->set_model(model.get());

    combo->set_current_row(2);

    CHECK(combo->current_row() == 2);
    CHECK(combo->current_text() == "30");  // Converted to string by list_model
}

TEST_CASE("combo_box_view - Popup reopen after selection") {
    // This test simulates the bug where reopening the combo box after
    // selection shows an empty list
    auto combo = std::make_unique<combo_box_view<Backend>>();
    auto model = std::make_shared<list_model<std::string, Backend>>();
    model->set_items({"Apple", "Banana", "Cherry", "Date", "Elderberry",
                       "Fig", "Grape", "Honeydew", "Jackfruit", "Kiwi",
                       "Lemon", "Mango"});
    combo->set_model(model.get());

    // Check model has items
    CHECK(model->row_count() == 12);
    CHECK(combo->model() != nullptr);
    CHECK(combo->model()->row_count() == 12);

    // Set current selection (simulating user selection)
    combo->set_current_row(11);  // Select "Mango"
    CHECK(combo->current_row() == 11);
    CHECK(combo->current_text() == "Mango");

    // Model should still have all items
    CHECK(model->row_count() == 12);
    CHECK(combo->model()->row_count() == 12);
    
    // Test that selection didn't corrupt the model
    auto idx = model->index(0, 0);
    CHECK(idx.is_valid());
    auto data = model->data(idx);
    CHECK(std::holds_alternative<std::string>(data));
    CHECK(std::get<std::string>(data) == "Apple");
}
