//
// Unit tests for combo_box widget
//

#include <doctest/doctest.h>
#include <onyxui/widgets/input/combo_box.hh>
#include <onyxui/mvc/models/list_model.hh>
#include "../utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

TEST_CASE("combo_box - Construction") {
    auto combo = std::make_unique<combo_box<Backend>>();

    CHECK(combo != nullptr);
    CHECK(combo->model() == nullptr);
    CHECK(combo->current_index() == -1);
    CHECK(combo->current_text().empty());
    CHECK_FALSE(combo->is_popup_open());
}

TEST_CASE("combo_box - Set model") {
    auto combo = std::make_unique<combo_box<Backend>>();
    auto model = std::make_shared<list_model<std::string, Backend>>();
    model->set_items({"Small", "Medium", "Large"});

    combo->set_model(model.get());

    CHECK(combo->model() == model.get());
    CHECK(combo->current_index() == -1);  // No selection by default
}

TEST_CASE("combo_box - Set current index") {
    auto combo = std::make_unique<combo_box<Backend>>();
    auto model = std::make_shared<list_model<std::string, Backend>>();
    model->set_items({"Small", "Medium", "Large"});
    combo->set_model(model.get());

    combo->set_current_index(1);

    CHECK(combo->current_index() == 1);
    CHECK(combo->current_text() == "Medium");
}

TEST_CASE("combo_box - Current index changed signal") {
    auto combo = std::make_unique<combo_box<Backend>>();
    auto model = std::make_shared<list_model<std::string, Backend>>();
    model->set_items({"A", "B", "C"});
    combo->set_model(model.get());

    int emitted_index = -999;
    combo->current_index_changed.connect([&](int index) {
        emitted_index = index;
    });

    combo->set_current_index(2);

    CHECK(emitted_index == 2);
}

TEST_CASE("combo_box - Invalid index") {
    auto combo = std::make_unique<combo_box<Backend>>();
    auto model = std::make_shared<list_model<std::string, Backend>>();
    model->set_items({"A", "B", "C"});
    combo->set_model(model.get());

    combo->set_current_index(100);  // Out of range

    CHECK(combo->current_index() == -1);
    CHECK(combo->current_text().empty());
}

TEST_CASE("combo_box - Clear selection") {
    auto combo = std::make_unique<combo_box<Backend>>();
    auto model = std::make_shared<list_model<std::string, Backend>>();
    model->set_items({"A", "B", "C"});
    combo->set_model(model.get());
    combo->set_current_index(1);

    combo->set_current_index(-1);  // Clear selection

    CHECK(combo->current_index() == -1);
    CHECK(combo->current_text().empty());
}

TEST_CASE("combo_box - Change model resets selection") {
    auto combo = std::make_unique<combo_box<Backend>>();
    auto model1 = std::make_shared<list_model<std::string, Backend>>();
    model1->set_items({"A", "B", "C"});
    combo->set_model(model1.get());
    combo->set_current_index(1);

    auto model2 = std::make_shared<list_model<std::string, Backend>>();
    model2->set_items({"X", "Y", "Z"});
    combo->set_model(model2.get());

    CHECK(combo->current_index() == -1);
    CHECK(combo->current_text().empty());
}
