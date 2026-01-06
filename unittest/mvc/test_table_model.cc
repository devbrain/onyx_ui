//
// OnyxUI MVC System - Table Model Tests
// Created: 2026-01-05
//

#include <doctest/doctest.h>

#include <onyxui/mvc/models/table_model.hh>
#include <onyxui/mvc/model_index.hh>
#include <onyxui/mvc/item_data_role.hh>

#include "../utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

// =========================================================================
// Test Row Types
// =========================================================================

struct Person {
    std::string name;
    int age;
    std::string email;
};

/**
 * @brief Concrete table model for Person rows
 */
template<UIBackend B>
class person_table_model : public table_model<Person, B> {
public:
    using base = table_model<Person, B>;
    using variant_type = typename base::variant_type;

    person_table_model() {
        this->set_headers({"Name", "Age", "Email"});
    }

protected:
    [[nodiscard]] variant_type column_data(
        const Person& p,
        int column,
        item_data_role role
    ) const override {
        if (role == item_data_role::display) {
            switch (column) {
                case 0: return p.name;
                case 1: return std::to_string(p.age);
                case 2: return p.email;
                default: return std::monostate{};
            }
        }
        if (role == item_data_role::edit) {
            // For edit role, return the raw value
            switch (column) {
                case 0: return std::any(p.name);
                case 1: return std::any(p.age);
                case 2: return std::any(p.email);
                default: return std::monostate{};
            }
        }
        return std::monostate{};
    }
};

// =========================================================================
// string_table_model Tests
// =========================================================================

TEST_CASE("string_table_model - Default construction is empty") {
    string_table_model<Backend> model;

    CHECK(model.row_count() == 0);
    CHECK(model.column_count() == 0);
    CHECK(model.rows().empty());
}

TEST_CASE("string_table_model - Set headers establishes column count") {
    string_table_model<Backend> model;
    model.set_headers({"A", "B", "C", "D"});

    CHECK(model.column_count() == 4);
    CHECK(model.header(0) == "A");
    CHECK(model.header(1) == "B");
    CHECK(model.header(2) == "C");
    CHECK(model.header(3) == "D");
}

TEST_CASE("string_table_model - header() for invalid column returns empty") {
    string_table_model<Backend> model;
    model.set_headers({"A", "B"});

    CHECK(model.header(-1) == "");
    CHECK(model.header(5) == "");
}

TEST_CASE("string_table_model - Append and access rows") {
    string_table_model<Backend> model;
    model.set_headers({"Name", "Value"});
    model.append({"Alice", "100"});
    model.append({"Bob", "200"});

    CHECK(model.row_count() == 2);
    CHECK(model.at(0) == std::vector<std::string>{"Alice", "100"});
    CHECK(model.at(1) == std::vector<std::string>{"Bob", "200"});
}

TEST_CASE("string_table_model - set_rows replaces all rows") {
    string_table_model<Backend> model;
    model.set_headers({"X", "Y"});
    model.set_rows({{"1", "2"}, {"3", "4"}, {"5", "6"}});

    CHECK(model.row_count() == 3);

    model.set_rows({{"A", "B"}});

    CHECK(model.row_count() == 1);
    CHECK(model.at(0) == std::vector<std::string>{"A", "B"});
}

TEST_CASE("string_table_model - insert row at specific position") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.append({"First"});
    model.append({"Third"});
    model.insert(1, {"Second"});

    CHECK(model.row_count() == 3);
    CHECK(model.at(0) == std::vector<std::string>{"First"});
    CHECK(model.at(1) == std::vector<std::string>{"Second"});
    CHECK(model.at(2) == std::vector<std::string>{"Third"});
}

TEST_CASE("string_table_model - remove row") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.set_rows({{"A"}, {"B"}, {"C"}});
    model.remove(1);

    CHECK(model.row_count() == 2);
    CHECK(model.at(0) == std::vector<std::string>{"A"});
    CHECK(model.at(1) == std::vector<std::string>{"C"});
}

TEST_CASE("string_table_model - clear removes all rows") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.set_rows({{"A"}, {"B"}, {"C"}});
    model.clear();

    CHECK(model.row_count() == 0);
    CHECK(model.rows().empty());
}

TEST_CASE("string_table_model - index() creates valid indices") {
    string_table_model<Backend> model;
    model.set_headers({"Col1", "Col2"});
    model.set_rows({{"A", "B"}, {"C", "D"}});

    auto idx00 = model.index(0, 0);
    auto idx01 = model.index(0, 1);
    auto idx10 = model.index(1, 0);
    auto idx_invalid_row = model.index(5, 0);
    auto idx_invalid_col = model.index(0, 5);

    CHECK(idx00.is_valid());
    CHECK(idx00.row == 0);
    CHECK(idx00.column == 0);

    CHECK(idx01.is_valid());
    CHECK(idx01.row == 0);
    CHECK(idx01.column == 1);

    CHECK(idx10.is_valid());
    CHECK(idx10.row == 1);
    CHECK(idx10.column == 0);

    CHECK_FALSE(idx_invalid_row.is_valid());
    CHECK_FALSE(idx_invalid_col.is_valid());
}

TEST_CASE("string_table_model - data() returns cell text") {
    string_table_model<Backend> model;
    model.set_headers({"Name", "Age"});
    model.append({"Alice", "30"});
    model.append({"Bob", "25"});

    auto idx_alice_name = model.index(0, 0);
    auto idx_alice_age = model.index(0, 1);
    auto idx_bob_name = model.index(1, 0);

    auto data_alice_name = model.data(idx_alice_name, item_data_role::display);
    auto data_alice_age = model.data(idx_alice_age, item_data_role::display);
    auto data_bob_name = model.data(idx_bob_name, item_data_role::display);

    REQUIRE(std::holds_alternative<std::string>(data_alice_name));
    CHECK(std::get<std::string>(data_alice_name) == "Alice");

    REQUIRE(std::holds_alternative<std::string>(data_alice_age));
    CHECK(std::get<std::string>(data_alice_age) == "30");

    REQUIRE(std::holds_alternative<std::string>(data_bob_name));
    CHECK(std::get<std::string>(data_bob_name) == "Bob");
}

TEST_CASE("string_table_model - data() with invalid index returns monostate") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.append({"A"});

    model_index invalid_idx;
    auto data = model.data(invalid_idx, item_data_role::display);

    CHECK(std::holds_alternative<std::monostate>(data));
}

TEST_CASE("string_table_model - data() for out-of-range column returns monostate") {
    string_table_model<Backend> model;
    model.set_headers({"Col1", "Col2"});
    model.append({"A"});  // Row has only 1 element, but we have 2 columns

    auto idx = model.index(0, 1);  // Column 1 is out of range for row data
    auto data = model.data(idx, item_data_role::display);

    CHECK(std::holds_alternative<std::monostate>(data));
}

TEST_CASE("string_table_model - parent() always returns invalid") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.append({"A"});

    auto idx = model.index(0, 0);
    auto parent = model.parent(idx);

    CHECK_FALSE(parent.is_valid());
}

// =========================================================================
// Custom table_model (Person) Tests
// =========================================================================

TEST_CASE("person_table_model - Headers are set in constructor") {
    person_table_model<Backend> model;

    CHECK(model.column_count() == 3);
    CHECK(model.header(0) == "Name");
    CHECK(model.header(1) == "Age");
    CHECK(model.header(2) == "Email");
}

TEST_CASE("person_table_model - Append and access Person rows") {
    person_table_model<Backend> model;
    model.append(Person{"Alice", 30, "alice@example.com"});
    model.append(Person{"Bob", 25, "bob@example.com"});

    CHECK(model.row_count() == 2);

    const Person& alice = model.at(0);
    CHECK(alice.name == "Alice");
    CHECK(alice.age == 30);
    CHECK(alice.email == "alice@example.com");

    const Person& bob = model.at(1);
    CHECK(bob.name == "Bob");
    CHECK(bob.age == 25);
}

TEST_CASE("person_table_model - data() returns column values") {
    person_table_model<Backend> model;
    model.append(Person{"Alice", 30, "alice@example.com"});

    auto idx_name = model.index(0, 0);
    auto idx_age = model.index(0, 1);
    auto idx_email = model.index(0, 2);

    auto data_name = model.data(idx_name, item_data_role::display);
    auto data_age = model.data(idx_age, item_data_role::display);
    auto data_email = model.data(idx_email, item_data_role::display);

    REQUIRE(std::holds_alternative<std::string>(data_name));
    CHECK(std::get<std::string>(data_name) == "Alice");

    REQUIRE(std::holds_alternative<std::string>(data_age));
    CHECK(std::get<std::string>(data_age) == "30");

    REQUIRE(std::holds_alternative<std::string>(data_email));
    CHECK(std::get<std::string>(data_email) == "alice@example.com");
}

TEST_CASE("person_table_model - set_rows with Person vector") {
    person_table_model<Backend> model;
    model.set_rows({
        Person{"Alice", 30, "alice@example.com"},
        Person{"Bob", 25, "bob@example.com"},
        Person{"Charlie", 35, "charlie@example.com"}
    });

    CHECK(model.row_count() == 3);
    CHECK(model.at(2).name == "Charlie");
    CHECK(model.at(2).age == 35);
}

// =========================================================================
// Signal Tests
// =========================================================================

TEST_CASE("table_model - rows_inserted signal emitted on append") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});

    bool signal_emitted = false;
    int first_row = -1;
    int last_row = -1;

    model.rows_inserted.connect([&](const model_index& parent, int first, int last) {
        (void)parent;
        signal_emitted = true;
        first_row = first;
        last_row = last;
    });

    model.append({"Item"});

    CHECK(signal_emitted);
    CHECK(first_row == 0);
    CHECK(last_row == 0);
}

TEST_CASE("table_model - rows_inserted signal emitted on insert") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.append({"A"});

    bool signal_emitted = false;
    int first_row = -1;

    model.rows_inserted.connect([&](const model_index& parent, int first, int last) {
        (void)parent;
        (void)last;
        signal_emitted = true;
        first_row = first;
    });

    model.insert(0, {"B"});

    CHECK(signal_emitted);
    CHECK(first_row == 0);
}

TEST_CASE("table_model - rows_removed signal emitted on remove") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.set_rows({{"A"}, {"B"}, {"C"}});

    bool signal_emitted = false;
    int removed_row = -1;

    model.rows_removed.connect([&](const model_index& parent, int first, int last) {
        (void)parent;
        (void)last;
        signal_emitted = true;
        removed_row = first;
    });

    model.remove(1);

    CHECK(signal_emitted);
    CHECK(removed_row == 1);
}

TEST_CASE("table_model - rows_removed signal emitted on clear") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.set_rows({{"A"}, {"B"}, {"C"}});

    bool signal_emitted = false;
    int first = -1;
    int last = -1;

    model.rows_removed.connect([&](const model_index& parent, int f, int l) {
        (void)parent;
        signal_emitted = true;
        first = f;
        last = l;
    });

    model.clear();

    CHECK(signal_emitted);
    CHECK(first == 0);
    CHECK(last == 2);
}

TEST_CASE("table_model - layout_changed signal emitted on set_headers") {
    string_table_model<Backend> model;

    bool signal_emitted = false;

    model.layout_changed.connect([&]() {
        signal_emitted = true;
    });

    model.set_headers({"A", "B", "C"});

    CHECK(signal_emitted);
}

TEST_CASE("table_model - set_rows emits both rows_removed and rows_inserted") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.set_rows({{"A"}, {"B"}});

    bool removed_emitted = false;
    bool inserted_emitted = false;

    model.rows_removed.connect([&](const model_index&, int, int) {
        removed_emitted = true;
    });

    model.rows_inserted.connect([&](const model_index&, int, int) {
        inserted_emitted = true;
    });

    model.set_rows({{"C"}, {"D"}, {"E"}});

    CHECK(removed_emitted);
    CHECK(inserted_emitted);
}

// =========================================================================
// Edge Cases
// =========================================================================

TEST_CASE("table_model - insert at beginning") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.append({"B"});
    model.insert(0, {"A"});

    CHECK(model.at(0) == std::vector<std::string>{"A"});
    CHECK(model.at(1) == std::vector<std::string>{"B"});
}

TEST_CASE("table_model - insert at end") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.append({"A"});
    model.insert(1, {"B"});

    CHECK(model.at(0) == std::vector<std::string>{"A"});
    CHECK(model.at(1) == std::vector<std::string>{"B"});
}

TEST_CASE("table_model - remove first row") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.set_rows({{"A"}, {"B"}, {"C"}});
    model.remove(0);

    CHECK(model.row_count() == 2);
    CHECK(model.at(0) == std::vector<std::string>{"B"});
}

TEST_CASE("table_model - remove last row") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.set_rows({{"A"}, {"B"}, {"C"}});
    model.remove(2);

    CHECK(model.row_count() == 2);
    CHECK(model.at(1) == std::vector<std::string>{"B"});
}

TEST_CASE("table_model - remove from single-row model") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.append({"A"});
    model.remove(0);

    CHECK(model.row_count() == 0);
}

TEST_CASE("table_model - clear empty model is no-op") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});

    bool signal_emitted = false;
    model.rows_removed.connect([&](const model_index&, int, int) {
        signal_emitted = true;
    });

    model.clear();

    CHECK_FALSE(signal_emitted);
}

TEST_CASE("table_model - remove invalid index is no-op") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.append({"A"});

    model.remove(-1);
    CHECK(model.row_count() == 1);

    model.remove(5);
    CHECK(model.row_count() == 1);
}

TEST_CASE("table_model - insert invalid index is no-op") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.append({"A"});

    model.insert(-1, {"B"});
    CHECK(model.row_count() == 1);

    model.insert(10, {"C"});
    CHECK(model.row_count() == 1);
}

TEST_CASE("table_model - at() throws for invalid row") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.append({"A"});

    CHECK_THROWS_AS(model.at(5), std::out_of_range);
    CHECK_THROWS_AS(model.at(-1), std::out_of_range);
}

TEST_CASE("table_model - headers() returns reference to headers") {
    string_table_model<Backend> model;
    model.set_headers({"A", "B", "C"});

    const auto& headers = model.headers();
    CHECK(headers.size() == 3);
    CHECK(headers[0] == "A");
    CHECK(headers[1] == "B");
    CHECK(headers[2] == "C");
}

TEST_CASE("table_model - index with parent is invalid") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.append({"A"});

    int dummy = 42;
    model_index fake_parent{0, 0, nullptr, &dummy};
    auto idx = model.index(0, 0, fake_parent);

    CHECK_FALSE(idx.is_valid());
}

TEST_CASE("table_model - sort default is no-op") {
    string_table_model<Backend> model;
    model.set_headers({"Col"});
    model.set_rows({{"C"}, {"A"}, {"B"}});

    // Default sort does nothing
    model.sort(0, sort_order::ascending);

    CHECK(model.at(0) == std::vector<std::string>{"C"});
    CHECK(model.at(1) == std::vector<std::string>{"A"});
    CHECK(model.at(2) == std::vector<std::string>{"B"});
}
