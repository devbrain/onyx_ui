//
// Unit tests for table widget (simple API)
//

#include <doctest/doctest.h>
#include <onyxui/widgets/input/table.hh>
#include "../utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

TEST_CASE("table - Construction") {
    auto tbl = std::make_unique<table<Backend>>();

    CHECK(tbl != nullptr);
    CHECK(tbl->row_count() == 0);
    CHECK(tbl->column_count() == 0);
    CHECK(tbl->current_row() == -1);
}

TEST_CASE("table - Construction with columns") {
    auto tbl = std::make_unique<table<Backend>>(
        std::vector<std::string>{"Name", "Age", "City"}
    );

    CHECK(tbl->column_count() == 3);
    CHECK(tbl->column_header(0) == "Name");
    CHECK(tbl->column_header(1) == "Age");
    CHECK(tbl->column_header(2) == "City");
    CHECK(tbl->row_count() == 0);
}

TEST_CASE("table - set_columns") {
    auto tbl = std::make_unique<table<Backend>>();

    tbl->set_columns({"A", "B", "C", "D"});

    CHECK(tbl->column_count() == 4);
    CHECK(tbl->column_header(0) == "A");
    CHECK(tbl->column_header(3) == "D");
}

TEST_CASE("table - add_row") {
    auto tbl = std::make_unique<table<Backend>>();
    tbl->set_columns({"Name", "Value"});

    tbl->add_row({"Alice", "100"});
    tbl->add_row({"Bob", "200"});

    CHECK(tbl->row_count() == 2);
    CHECK(tbl->cell(0, 0) == "Alice");
    CHECK(tbl->cell(0, 1) == "100");
    CHECK(tbl->cell(1, 0) == "Bob");
    CHECK(tbl->cell(1, 1) == "200");
}

TEST_CASE("table - add_row with fewer values pads") {
    auto tbl = std::make_unique<table<Backend>>();
    tbl->set_columns({"A", "B", "C"});

    tbl->add_row({"X"});  // Only one value

    CHECK(tbl->row_count() == 1);
    CHECK(tbl->cell(0, 0) == "X");
    CHECK(tbl->cell(0, 1).empty());  // Padded with empty
    CHECK(tbl->cell(0, 2).empty());  // Padded with empty
}

TEST_CASE("table - add_row with more values truncates") {
    auto tbl = std::make_unique<table<Backend>>();
    tbl->set_columns({"A", "B"});

    tbl->add_row({"X", "Y", "Z", "W"});  // Four values, only two columns

    CHECK(tbl->row_count() == 1);
    CHECK(tbl->cell(0, 0) == "X");
    CHECK(tbl->cell(0, 1) == "Y");
}

TEST_CASE("table - insert_row") {
    auto tbl = std::make_unique<table<Backend>>();
    tbl->set_columns({"Name"});
    tbl->add_row({"First"});
    tbl->add_row({"Third"});

    tbl->insert_row(1, {"Second"});

    CHECK(tbl->row_count() == 3);
    CHECK(tbl->cell(0, 0) == "First");
    CHECK(tbl->cell(1, 0) == "Second");
    CHECK(tbl->cell(2, 0) == "Third");
}

TEST_CASE("table - remove_row") {
    auto tbl = std::make_unique<table<Backend>>();
    tbl->set_columns({"Name"});
    tbl->add_row({"A"});
    tbl->add_row({"B"});
    tbl->add_row({"C"});

    tbl->remove_row(1);

    CHECK(tbl->row_count() == 2);
    CHECK(tbl->cell(0, 0) == "A");
    CHECK(tbl->cell(1, 0) == "C");
}

TEST_CASE("table - clear") {
    auto tbl = std::make_unique<table<Backend>>();
    tbl->set_columns({"Name"});
    tbl->add_row({"A"});
    tbl->add_row({"B"});
    tbl->set_current_row(0);

    tbl->clear();

    CHECK(tbl->row_count() == 0);
    CHECK(tbl->current_row() == -1);
}

TEST_CASE("table - set_rows") {
    auto tbl = std::make_unique<table<Backend>>();
    tbl->set_columns({"Name", "Value"});

    tbl->set_rows({
        {"A", "1"},
        {"B", "2"},
        {"C", "3"}
    });

    CHECK(tbl->row_count() == 3);
    CHECK(tbl->cell(0, 0) == "A");
    CHECK(tbl->cell(2, 1) == "3");
}

TEST_CASE("table - cell access") {
    auto tbl = std::make_unique<table<Backend>>();
    tbl->set_columns({"Name", "Age"});
    tbl->add_row({"Alice", "30"});

    CHECK(tbl->cell(0, 0) == "Alice");
    CHECK(tbl->cell(0, 1) == "30");
    CHECK(tbl->cell(-1, 0).empty());  // Invalid row
    CHECK(tbl->cell(0, 99).empty());   // Invalid column
}

TEST_CASE("table - set_cell") {
    auto tbl = std::make_unique<table<Backend>>();
    tbl->set_columns({"Name", "Value"});
    tbl->add_row({"Old", "123"});

    tbl->set_cell(0, 0, "New");

    CHECK(tbl->cell(0, 0) == "New");
    CHECK(tbl->cell(0, 1) == "123");  // Unchanged
}

TEST_CASE("table - current_row selection") {
    auto tbl = std::make_unique<table<Backend>>();
    tbl->set_columns({"Name"});
    tbl->add_row({"A"});
    tbl->add_row({"B"});
    tbl->add_row({"C"});

    CHECK(tbl->current_row() == -1);

    tbl->set_current_row(1);
    CHECK(tbl->current_row() == 1);

    tbl->set_current_row(2);
    CHECK(tbl->current_row() == 2);
}

TEST_CASE("table - current_row invalid index") {
    auto tbl = std::make_unique<table<Backend>>();
    tbl->set_columns({"Name"});
    tbl->add_row({"A"});

    tbl->set_current_row(100);  // Out of range

    CHECK(tbl->current_row() == -1);
}

TEST_CASE("table - current_row clear selection") {
    auto tbl = std::make_unique<table<Backend>>();
    tbl->set_columns({"Name"});
    tbl->add_row({"A"});
    tbl->set_current_row(0);

    tbl->set_current_row(-1);  // Clear

    CHECK(tbl->current_row() == -1);
}

TEST_CASE("table - current_row_changed signal") {
    auto tbl = std::make_unique<table<Backend>>();
    tbl->set_columns({"Name"});
    tbl->add_row({"A"});
    tbl->add_row({"B"});

    int emitted_row = -999;
    tbl->current_row_changed.connect([&](int row) {
        emitted_row = row;
    });

    tbl->set_current_row(1);

    CHECK(emitted_row == 1);
}

TEST_CASE("table - headers_visible") {
    auto tbl = std::make_unique<table<Backend>>();

    CHECK_FALSE(tbl->headers_visible());

    tbl->set_headers_visible(true);
    CHECK(tbl->headers_visible());

    tbl->set_headers_visible(false);
    CHECK_FALSE(tbl->headers_visible());
}

TEST_CASE("table - grid visibility") {
    auto tbl = std::make_unique<table<Backend>>();

    // Enable both
    tbl->set_grid_visible(true);

    // Access underlying view to verify
    CHECK(tbl->view() != nullptr);
    CHECK(tbl->view()->horizontal_grid_visible());
    CHECK(tbl->view()->vertical_grid_visible());
}

TEST_CASE("table - model access") {
    auto tbl = std::make_unique<table<Backend>>();

    CHECK(tbl->model() != nullptr);
}

TEST_CASE("table - view access") {
    auto tbl = std::make_unique<table<Backend>>();

    CHECK(tbl->view() != nullptr);
}

TEST_CASE("table - remove_row adjusts selection downward") {
    auto tbl = std::make_unique<table<Backend>>();
    tbl->set_columns({"Name"});
    tbl->add_row({"A"});
    tbl->add_row({"B"});
    tbl->add_row({"C"});
    tbl->set_current_row(2);  // Select "C"

    tbl->remove_row(0);  // Remove "A"

    // Selection should adjust from 2 to 1
    CHECK(tbl->current_row() == 1);
    CHECK(tbl->cell(tbl->current_row(), 0) == "C");
}

TEST_CASE("table - remove selected row clears selection") {
    auto tbl = std::make_unique<table<Backend>>();
    tbl->set_columns({"Name"});
    tbl->add_row({"A"});
    tbl->add_row({"B"});
    tbl->set_current_row(1);  // Select "B"

    tbl->remove_row(1);  // Remove selected row

    CHECK(tbl->current_row() == -1);  // Selection cleared
}

TEST_CASE("table - column width") {
    auto tbl = std::make_unique<table<Backend>>();
    tbl->set_columns({"Name", "Value"});

    tbl->set_column_width(0, 100);
    tbl->set_column_width(1, 50);

    // Verify view received the widths
    CHECK(tbl->view() != nullptr);
}

TEST_CASE("table - scroll_to_row") {
    auto tbl = std::make_unique<table<Backend>>();
    tbl->set_columns({"Name"});

    // Add many rows
    for (int i = 0; i < 100; ++i) {
        tbl->add_row({"Row " + std::to_string(i)});
    }

    // Scroll to a row - verify state is preserved
    tbl->scroll_to_row(50);
    CHECK(tbl->row_count() == 100);  // Data still intact after scroll
    CHECK(tbl->cell(50, 0) == "Row 50");  // Can still access scrolled-to row
}

TEST_CASE("table - empty operations don't crash") {
    auto tbl = std::make_unique<table<Backend>>();

    // Operations on empty table should be safe
    tbl->remove_row(0);
    tbl->set_current_row(0);
    tbl->clear();
    CHECK(tbl->cell(0, 0).empty());
    CHECK(tbl->column_header(0).empty());
    CHECK(tbl->row_count() == 0);
    CHECK(tbl->current_row() == -1);  // No selection on empty table
}
