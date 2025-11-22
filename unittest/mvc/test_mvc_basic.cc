//
// OnyxUI MVC System - Basic Tests
// Created: 2025-11-22
//

#include <doctest/doctest.h>

#include <onyxui/mvc/model_index.hh>
#include <onyxui/mvc/item_data_role.hh>
#include <onyxui/mvc/models/list_model.hh>
#include <onyxui/mvc/selection/item_selection_model.hh>
#include <onyxui/mvc/delegates/default_item_delegate.hh>

#include "../utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

// =========================================================================
// model_index Tests
// =========================================================================

TEST_CASE("model_index - Default construction creates invalid index") {
    model_index idx;

    CHECK_FALSE(idx.is_valid());
    CHECK(idx.row == -1);
    CHECK(idx.column == -1);
    CHECK(idx.model == nullptr);
}

TEST_CASE("model_index - Valid construction") {
    int dummy_model = 42;
    model_index idx{3, 0, nullptr, &dummy_model};

    CHECK(idx.is_valid());
    CHECK(idx.row == 3);
    CHECK(idx.column == 0);
    CHECK(idx.model == &dummy_model);
}

TEST_CASE("model_index - Equality comparison") {
    int model1 = 1;
    int model2 = 2;

    model_index idx1{3, 0, nullptr, &model1};
    model_index idx2{3, 0, nullptr, &model1};
    model_index idx3{4, 0, nullptr, &model1};
    model_index idx4{3, 0, nullptr, &model2};

    CHECK(idx1 == idx2);
    CHECK(idx1 != idx3);
    CHECK(idx1 != idx4);
}

TEST_CASE("model_index - Hash function") {
    int model = 42;
    model_index idx1{3, 0, nullptr, &model};
    model_index idx2{3, 0, nullptr, &model};
    model_index idx3{4, 0, nullptr, &model};

    std::hash<model_index> hasher;
    CHECK(hasher(idx1) == hasher(idx2));
    // Note: Different indices might have same hash (collision), but unlikely
}

// =========================================================================
// list_model<std::string> Tests
// =========================================================================

TEST_CASE("list_model - Default construction is empty") {
    list_model<std::string, Backend> model;

    CHECK(model.row_count() == 0);
    CHECK(model.column_count() == 1);
    CHECK(model.items().empty());
}

TEST_CASE("list_model - Construction with initial items") {
    std::vector<std::string> items = {"Apple", "Banana", "Cherry"};
    list_model<std::string, Backend> model(items);

    CHECK(model.row_count() == 3);
    CHECK(model.items().size() == 3);
}

TEST_CASE("list_model - set_items replaces all items") {
    list_model<std::string, Backend> model;
    model.set_items({"Item1", "Item2", "Item3"});

    CHECK(model.row_count() == 3);

    model.set_items({"NewItem1", "NewItem2"});

    CHECK(model.row_count() == 2);
    CHECK(model.at(0) == "NewItem1");
    CHECK(model.at(1) == "NewItem2");
}

TEST_CASE("list_model - append adds item at end") {
    list_model<std::string, Backend> model;
    model.append("First");
    model.append("Second");

    CHECK(model.row_count() == 2);
    CHECK(model.at(0) == "First");
    CHECK(model.at(1) == "Second");
}

TEST_CASE("list_model - insert at specific position") {
    list_model<std::string, Backend> model;
    model.append("First");
    model.append("Third");
    model.insert(1, "Second");

    CHECK(model.row_count() == 3);
    CHECK(model.at(0) == "First");
    CHECK(model.at(1) == "Second");
    CHECK(model.at(2) == "Third");
}

TEST_CASE("list_model - remove item") {
    list_model<std::string, Backend> model;
    model.set_items({"A", "B", "C"});
    model.remove(1);

    CHECK(model.row_count() == 2);
    CHECK(model.at(0) == "A");
    CHECK(model.at(1) == "C");
}

TEST_CASE("list_model - clear removes all items") {
    list_model<std::string, Backend> model;
    model.set_items({"A", "B", "C"});
    model.clear();

    CHECK(model.row_count() == 0);
    CHECK(model.items().empty());
}

TEST_CASE("list_model - index() creates valid indices") {
    list_model<std::string, Backend> model;
    model.set_items({"A", "B", "C"});

    auto idx0 = model.index(0, 0);
    auto idx1 = model.index(1, 0);
    auto idx_invalid = model.index(5, 0);

    CHECK(idx0.is_valid());
    CHECK(idx0.row == 0);
    CHECK(idx0.column == 0);

    CHECK(idx1.is_valid());
    CHECK(idx1.row == 1);

    CHECK_FALSE(idx_invalid.is_valid());
}

TEST_CASE("list_model - data() returns display text") {
    list_model<std::string, Backend> model;
    model.set_items({"Apple", "Banana"});

    auto idx = model.index(0, 0);
    auto data = model.data(idx, item_data_role::display);

    REQUIRE(std::holds_alternative<std::string>(data));
    CHECK(std::get<std::string>(data) == "Apple");
}

TEST_CASE("list_model - data() with invalid index returns monostate") {
    list_model<std::string, Backend> model;
    model.set_items({"A"});

    model_index invalid_idx;
    auto data = model.data(invalid_idx, item_data_role::display);

    CHECK(std::holds_alternative<std::monostate>(data));
}

TEST_CASE("list_model - set_data modifies item") {
    list_model<std::string, Backend> model;
    model.set_items({"Original"});

    auto idx = model.index(0, 0);
    bool success = model.set_data(idx, std::any(std::string("Modified")), item_data_role::edit);

    CHECK(success);
    CHECK(model.at(0) == "Modified");
}

TEST_CASE("list_model - rows_inserted signal emitted on append") {
    list_model<std::string, Backend> model;

    bool signal_emitted = false;
    int first_row = -1;
    int last_row = -1;

    model.rows_inserted.connect([&](const model_index& parent, int first, int last) {
        signal_emitted = true;
        first_row = first;
        last_row = last;
    });

    model.append("Item");

    CHECK(signal_emitted);
    CHECK(first_row == 0);
    CHECK(last_row == 0);
}

TEST_CASE("list_model - rows_removed signal emitted on remove") {
    list_model<std::string, Backend> model;
    model.set_items({"A", "B", "C"});

    bool signal_emitted = false;

    model.rows_removed.connect([&](const model_index&, int, int) {
        signal_emitted = true;
    });

    model.remove(1);

    CHECK(signal_emitted);
}

TEST_CASE("list_model - data_changed signal emitted on set_data") {
    list_model<std::string, Backend> model;
    model.set_items({"Original"});

    bool signal_emitted = false;

    model.data_changed.connect([&](const model_index&, const model_index&) {
        signal_emitted = true;
    });

    auto idx = model.index(0, 0);
    model.set_data(idx, std::any(std::string("Modified")), item_data_role::edit);

    CHECK(signal_emitted);
}

// =========================================================================
// list_model<int> Tests
// =========================================================================

TEST_CASE("list_model<int> - Basic operations") {
    list_model<int, Backend> model;
    model.set_items({1, 2, 3});

    CHECK(model.row_count() == 3);
    CHECK(model.at(0) == 1);
    CHECK(model.at(1) == 2);

    auto idx = model.index(0, 0);
    auto data = model.data(idx, item_data_role::display);

    REQUIRE(std::holds_alternative<std::string>(data));
    CHECK(std::get<std::string>(data) == "1");
}

// =========================================================================
// item_selection_model Tests
// =========================================================================

TEST_CASE("item_selection_model - Default mode is single_selection") {
    item_selection_model<test_backend> sel;

    CHECK(sel.get_selection_mode() == selection_mode::single_selection);
    CHECK_FALSE(sel.has_selection());
}

TEST_CASE("item_selection_model - set_selection_mode changes mode") {
    item_selection_model<test_backend> sel;
    sel.set_selection_mode(selection_mode::multi_selection);

    CHECK(sel.get_selection_mode() == selection_mode::multi_selection);
}

TEST_CASE("item_selection_model - current_index initially invalid") {
    item_selection_model<test_backend> sel;

    auto current = sel.current_index();
    CHECK_FALSE(current.is_valid());
}

TEST_CASE("item_selection_model - set_current_index updates current") {
    item_selection_model<test_backend> sel;

    int dummy_model = 42;
    model_index idx{3, 0, nullptr, &dummy_model};

    sel.set_current_index(idx);

    CHECK(sel.current_index() == idx);
}

TEST_CASE("item_selection_model - current_changed signal emitted") {
    item_selection_model<test_backend> sel;

    bool signal_emitted = false;
    model_index emitted_current;

    sel.current_changed.connect([&](const model_index& current, const model_index&) {
        signal_emitted = true;
        emitted_current = current;
    });

    int dummy = 1;
    model_index idx{5, 0, nullptr, &dummy};
    sel.set_current_index(idx);

    CHECK(signal_emitted);
    CHECK(emitted_current == idx);
}

TEST_CASE("item_selection_model - select adds to selection") {
    item_selection_model<test_backend> sel;

    int dummy = 1;
    model_index idx{2, 0, nullptr, &dummy};

    sel.select(idx);

    CHECK(sel.has_selection());
    CHECK(sel.is_selected(idx));
}

TEST_CASE("item_selection_model - single_selection mode clears others") {
    item_selection_model<test_backend> sel;
    sel.set_selection_mode(selection_mode::single_selection);

    int dummy = 1;
    model_index idx1{0, 0, nullptr, &dummy};
    model_index idx2{1, 0, nullptr, &dummy};

    sel.select(idx1);
    sel.select(idx2);

    CHECK(sel.is_selected(idx2));
    CHECK_FALSE(sel.is_selected(idx1));
}

TEST_CASE("item_selection_model - multi_selection mode keeps multiple") {
    item_selection_model<test_backend> sel;
    sel.set_selection_mode(selection_mode::multi_selection);

    int dummy = 1;
    model_index idx1{0, 0, nullptr, &dummy};
    model_index idx2{1, 0, nullptr, &dummy};

    sel.select(idx1);
    sel.select(idx2);

    CHECK(sel.is_selected(idx1));
    CHECK(sel.is_selected(idx2));
}

TEST_CASE("item_selection_model - deselect removes from selection") {
    item_selection_model<test_backend> sel;

    int dummy = 1;
    model_index idx{2, 0, nullptr, &dummy};

    sel.select(idx);
    CHECK(sel.is_selected(idx));

    sel.deselect(idx);
    CHECK_FALSE(sel.is_selected(idx));
}

TEST_CASE("item_selection_model - toggle flips selection state") {
    item_selection_model<test_backend> sel;
    sel.set_selection_mode(selection_mode::multi_selection);

    int dummy = 1;
    model_index idx{2, 0, nullptr, &dummy};

    sel.toggle(idx);
    CHECK(sel.is_selected(idx));

    sel.toggle(idx);
    CHECK_FALSE(sel.is_selected(idx));
}

TEST_CASE("item_selection_model - clear_selection removes all") {
    item_selection_model<test_backend> sel;
    sel.set_selection_mode(selection_mode::multi_selection);

    int dummy = 1;
    model_index idx1{0, 0, nullptr, &dummy};
    model_index idx2{1, 0, nullptr, &dummy};

    sel.select(idx1);
    sel.select(idx2);
    CHECK(sel.has_selection());

    sel.clear_selection();
    CHECK_FALSE(sel.has_selection());
    CHECK_FALSE(sel.is_selected(idx1));
    CHECK_FALSE(sel.is_selected(idx2));
}

TEST_CASE("item_selection_model - selected_indices returns all selected") {
    item_selection_model<test_backend> sel;
    sel.set_selection_mode(selection_mode::multi_selection);

    int dummy = 1;
    model_index idx1{0, 0, nullptr, &dummy};
    model_index idx2{2, 0, nullptr, &dummy};

    sel.select(idx1);
    sel.select(idx2);

    auto selected = sel.selected_indices();
    CHECK(selected.size() == 2);

    // Order is unspecified, so check both are present
    bool has_idx1 = false;
    bool has_idx2 = false;
    for (const auto& idx : selected) {
        if (idx == idx1) has_idx1 = true;
        if (idx == idx2) has_idx2 = true;
    }
    CHECK(has_idx1);
    CHECK(has_idx2);
}

TEST_CASE("item_selection_model - selection_changed signal emitted") {
    item_selection_model<test_backend> sel;

    bool signal_emitted = false;

    sel.selection_changed.connect([&](const auto& selected, const auto& deselected) {
        signal_emitted = true;
        CHECK(selected.size() == 1);
        CHECK(deselected.empty());
    });

    int dummy = 1;
    model_index idx{3, 0, nullptr, &dummy};
    sel.select(idx);

    CHECK(signal_emitted);
}

TEST_CASE("item_selection_model - no_selection mode rejects selections") {
    item_selection_model<test_backend> sel;
    sel.set_selection_mode(selection_mode::no_selection);

    int dummy = 1;
    model_index idx{0, 0, nullptr, &dummy};

    sel.select(idx);

    CHECK_FALSE(sel.has_selection());
    CHECK_FALSE(sel.is_selected(idx));
}

TEST_CASE("item_selection_model - select_range selects multiple items") {
    item_selection_model<test_backend> sel;
    sel.set_selection_mode(selection_mode::multi_selection);

    int dummy = 1;
    model_index idx1{1, 0, nullptr, &dummy};
    model_index idx2{3, 0, nullptr, &dummy};

    sel.select_range(idx1, idx2);

    // Should select rows 1, 2, 3
    CHECK(sel.is_selected(model_index{1, 0, nullptr, &dummy}));
    CHECK(sel.is_selected(model_index{2, 0, nullptr, &dummy}));
    CHECK(sel.is_selected(model_index{3, 0, nullptr, &dummy}));
}

// =========================================================================
// default_item_delegate Tests
// =========================================================================

TEST_CASE("default_item_delegate - Construction succeeds") {
    default_item_delegate<Backend> delegate;

    // Just check it constructs without throwing
    CHECK(true);
}

TEST_CASE("default_item_delegate - size_hint returns reasonable size") {
    default_item_delegate<Backend> delegate;
    list_model<std::string, Backend> model;
    model.append("Test Item");

    auto idx = model.index(0, 0);
    auto size = delegate.size_hint(idx);

    // Size should be positive
    CHECK(size_utils::get_width(size) > 0);
    CHECK(size_utils::get_height(size) >= 20);  // MIN_HEIGHT is 20
}

TEST_CASE("default_item_delegate - size_hint for invalid index returns minimum") {
    default_item_delegate<Backend> delegate;

    model_index invalid;
    auto size = delegate.size_hint(invalid);

    CHECK(size_utils::get_width(size) == 0);
    CHECK(size_utils::get_height(size) == 20);  // MIN_HEIGHT
}
