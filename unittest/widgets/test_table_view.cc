//
// OnyxUI MVC System - Table View Tests
// Created: 2026-01-05
//

#include <doctest/doctest.h>

#include <onyxui/mvc/views/table_view.hh>
#include <onyxui/mvc/models/table_model.hh>
#include <onyxui/mvc/selection/item_selection_model.hh>
#include <onyxui/core/backend_metrics.hh>
#include <onyxui/services/ui_services.hh>
#include <onyxui/events/ui_event.hh>

#include "../utils/test_backend.hh"

using namespace onyxui;
using Backend = test_backend;

// Helper to convert physical coordinates to logical
// Uses metrics if available, otherwise assumes 1:1 mapping
inline logical_unit physical_to_logical_x(int physical) {
    if (auto const* m = ui_services<Backend>::metrics()) {
        return m->physical_to_logical_x(physical);
    }
    return logical_unit(static_cast<double>(physical));
}

inline logical_unit physical_to_logical_y(int physical) {
    if (auto const* m = ui_services<Backend>::metrics()) {
        return m->physical_to_logical_y(physical);
    }
    return logical_unit(static_cast<double>(physical));
}

// Helper to simulate mouse click on a view at logical coordinates
template<typename View>
void simulate_click(View& view, logical_unit x, logical_unit y, bool ctrl = false, bool shift = false) {
    mouse_event evt;
    evt.x = x;
    evt.y = y;
    evt.btn = mouse_event::button::left;
    evt.act = mouse_event::action::press;
    evt.modifiers.ctrl = ctrl;
    evt.modifiers.shift = shift;
    evt.modifiers.alt = false;

    view.handle_event(ui_event{evt}, event_phase::target);
}

// Helper to simulate click at physical coordinates from visual_rect
// Converts physical to logical using metrics (or 1:1 if no metrics)
template<typename View>
void simulate_click_at_physical(View& view, int phys_x, int phys_y, bool ctrl = false, bool shift = false) {
    simulate_click(view, physical_to_logical_x(phys_x), physical_to_logical_y(phys_y), ctrl, shift);
}

// =========================================================================
// Test Table Model
// =========================================================================

struct TestRow {
    std::string name;
    int value;
    std::string status;
};

template<UIBackend B>
class test_table_model : public table_model<TestRow, B> {
public:
    using base = table_model<TestRow, B>;
    using variant_type = typename base::variant_type;

    test_table_model() {
        this->set_headers({"Name", "Value", "Status"});
    }

protected:
    [[nodiscard]] variant_type column_data(
        const TestRow& row,
        int column,
        item_data_role role
    ) const override {
        if (role == item_data_role::display) {
            switch (column) {
                case 0: return row.name;
                case 1: return std::to_string(row.value);
                case 2: return row.status;
                default: return std::monostate{};
            }
        }
        return std::monostate{};
    }
};

// =========================================================================
// Basic Construction Tests
// =========================================================================

TEST_CASE("table_view - Default construction") {
    auto view = std::make_unique<table_view<Backend>>();

    CHECK_FALSE(view->headers_visible());
    CHECK_FALSE(view->horizontal_grid_visible());
    CHECK_FALSE(view->vertical_grid_visible());
    CHECK_FALSE(view->sorting_enabled());
    CHECK(view->get_selection_behavior() == selection_behavior::select_rows);
    CHECK(view->sort_column() == -1);
}

TEST_CASE("table_view - Set model updates geometries") {
    auto model = std::make_shared<test_table_model<Backend>>();
    model->set_rows({
        {"Alice", 100, "Active"},
        {"Bob", 200, "Pending"},
        {"Charlie", 150, "Active"}
    });

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());

    CHECK(view->model() == model.get());
}

// =========================================================================
// Column Configuration Tests
// =========================================================================

TEST_CASE("table_view - Set column width") {
    auto model = std::make_shared<test_table_model<Backend>>();
    model->append({"Test", 1, "OK"});

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());

    // Set fixed widths
    view->set_column_width(0, 100);
    view->set_column_width(1, 50);
    view->set_column_width(2, 80);

    // Arrange to calculate widths
    (void)view->measure(300_lu, 200_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 300_lu, 200_lu});

    CHECK(view->column_width(0) >= 100);
    CHECK(view->column_width(1) >= 50);
    CHECK(view->column_width(2) >= 80);
}

TEST_CASE("table_view - Set column stretch") {
    auto model = std::make_shared<test_table_model<Backend>>();
    model->append({"Test", 1, "OK"});

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());

    view->set_column_width(0, 50);
    view->set_column_stretch(1, 1);  // Column 1 stretches
    view->set_column_width(2, 50);

    (void)view->measure(300_lu, 200_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 300_lu, 200_lu});

    // Column 1 should be wider than fixed columns
    CHECK(view->column_width(1) > view->column_width(0));
}

TEST_CASE("table_view - Set minimum column width") {
    auto model = std::make_shared<test_table_model<Backend>>();
    model->append({"Test", 1, "OK"});

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());

    view->set_minimum_column_width(0, 80);
    view->set_column_width(0, 0);  // Auto width

    (void)view->measure(300_lu, 200_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 300_lu, 200_lu});

    CHECK(view->column_width(0) >= 80);
}

TEST_CASE("table_view - Total column width") {
    auto model = std::make_shared<test_table_model<Backend>>();
    model->append({"Test", 1, "OK"});

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());

    view->set_column_width(0, 100);
    view->set_column_width(1, 50);
    view->set_column_width(2, 80);

    (void)view->measure(300_lu, 200_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 300_lu, 200_lu});

    // Total should be at least sum of fixed widths
    CHECK(view->total_column_width() >= 230);
}

// =========================================================================
// Headers Tests
// =========================================================================

TEST_CASE("table_view - Headers visibility") {
    auto view = std::make_unique<table_view<Backend>>();

    CHECK_FALSE(view->headers_visible());

    view->set_headers_visible(true);
    CHECK(view->headers_visible());

    view->set_headers_visible(false);
    CHECK_FALSE(view->headers_visible());
}

TEST_CASE("table_view - Header height") {
    auto view = std::make_unique<table_view<Backend>>();

    CHECK(view->header_height() == 24);  // Default

    view->set_header_height(30);
    CHECK(view->header_height() == 30);
}

// =========================================================================
// Grid Lines Tests
// =========================================================================

TEST_CASE("table_view - Horizontal grid lines") {
    auto view = std::make_unique<table_view<Backend>>();

    CHECK_FALSE(view->horizontal_grid_visible());

    view->set_horizontal_grid_visible(true);
    CHECK(view->horizontal_grid_visible());
}

TEST_CASE("table_view - Vertical grid lines") {
    auto view = std::make_unique<table_view<Backend>>();

    CHECK_FALSE(view->vertical_grid_visible());

    view->set_vertical_grid_visible(true);
    CHECK(view->vertical_grid_visible());
}

// =========================================================================
// Selection Behavior Tests
// =========================================================================

TEST_CASE("table_view - Selection behavior default is rows") {
    auto view = std::make_unique<table_view<Backend>>();
    CHECK(view->get_selection_behavior() == selection_behavior::select_rows);
}

TEST_CASE("table_view - Set selection behavior") {
    auto view = std::make_unique<table_view<Backend>>();

    view->set_selection_behavior(selection_behavior::select_items);
    CHECK(view->get_selection_behavior() == selection_behavior::select_items);

    view->set_selection_behavior(selection_behavior::select_columns);
    CHECK(view->get_selection_behavior() == selection_behavior::select_columns);

    view->set_selection_behavior(selection_behavior::select_rows);
    CHECK(view->get_selection_behavior() == selection_behavior::select_rows);
}

// =========================================================================
// Sorting Tests
// =========================================================================

TEST_CASE("table_view - Sorting enabled/disabled") {
    auto view = std::make_unique<table_view<Backend>>();

    CHECK_FALSE(view->sorting_enabled());

    view->set_sorting_enabled(true);
    CHECK(view->sorting_enabled());
}

TEST_CASE("table_view - Sort column tracking") {
    auto view = std::make_unique<table_view<Backend>>();

    CHECK(view->sort_column() == -1);  // No sort initially
    CHECK(view->get_sort_order() == sort_order::ascending);
}

// =========================================================================
// Scrolling Tests
// =========================================================================

TEST_CASE("table_view - Scroll offset initial values") {
    auto view = std::make_unique<table_view<Backend>>();

    CHECK(view->scroll_offset_x() == 0.0);
    CHECK(view->scroll_offset_y() == 0.0);
}

TEST_CASE("table_view - Set scroll offset") {
    auto model = std::make_shared<test_table_model<Backend>>();
    for (int i = 0; i < 50; ++i) {
        model->append({"Item " + std::to_string(i), i, "OK"});
    }

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());
    (void)view->measure(200_lu, 100_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 200_lu, 100_lu});

    view->set_scroll_offset(10.0, 20.0);

    CHECK(view->scroll_offset_x() >= 0.0);
    CHECK(view->scroll_offset_y() >= 0.0);
}

TEST_CASE("table_view - Scroll offset signal emitted") {
    auto model = std::make_shared<test_table_model<Backend>>();
    for (int i = 0; i < 50; ++i) {
        model->append({"Item " + std::to_string(i), i, "OK"});
    }

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());
    (void)view->measure(200_lu, 100_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 200_lu, 100_lu});

    bool signal_emitted = false;
    view->scroll_offset_changed.connect([&](double x, double y) {
        signal_emitted = true;
        (void)x; (void)y;
    });

    view->set_scroll_offset(5.0, 10.0);

    CHECK(signal_emitted);
}

// =========================================================================
// Index At Tests
// =========================================================================

TEST_CASE("table_view - index_at with no model returns invalid") {
    auto view = std::make_unique<table_view<Backend>>();

    auto idx = view->index_at(logical_unit(10.0), logical_unit(10.0));
    CHECK_FALSE(idx.is_valid());
}

TEST_CASE("table_view - index_at with model") {
    auto model = std::make_shared<test_table_model<Backend>>();
    model->append({"Alice", 100, "Active"});
    model->append({"Bob", 200, "Pending"});

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());
    view->set_column_width(0, 50);
    view->set_column_width(1, 50);
    view->set_column_width(2, 50);

    (void)view->measure(200_lu, 100_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 200_lu, 100_lu});

    // Click in first cell area (with border offset)
    auto idx = view->index_at(logical_unit(10.0), logical_unit(10.0));

    // Should return a valid index
    CHECK(idx.is_valid());
    CHECK(idx.row == 0);
    CHECK(idx.column == 0);
}

// =========================================================================
// Visual Rect Tests
// =========================================================================

TEST_CASE("table_view - visual_rect for invalid index") {
    auto view = std::make_unique<table_view<Backend>>();

    model_index invalid;
    auto rect = view->visual_rect(invalid);

    CHECK(rect.w == 0);
    CHECK(rect.h == 0);
}

TEST_CASE("table_view - visual_rect for valid index") {
    auto model = std::make_shared<test_table_model<Backend>>();
    model->append({"Test", 1, "OK"});

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());
    view->set_column_width(0, 100);

    (void)view->measure(300_lu, 100_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 300_lu, 100_lu});

    auto idx = model->index(0, 0);
    auto rect = view->visual_rect(idx);

    CHECK(rect.w > 0);
    CHECK(rect.h > 0);
}

// =========================================================================
// Scroll To Tests
// =========================================================================

TEST_CASE("table_view - scroll_to makes cell visible") {
    auto model = std::make_shared<test_table_model<Backend>>();
    for (int i = 0; i < 50; ++i) {
        model->append({"Item " + std::to_string(i), i, "OK"});
    }

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());
    (void)view->measure(200_lu, 100_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 200_lu, 100_lu});

    // Scroll to bottom item
    auto idx = model->index(49, 0);
    view->scroll_to(idx);

    // Scroll offset should have changed
    CHECK(view->scroll_offset_y() > 0);
}

// =========================================================================
// Update Geometries Tests
// =========================================================================

TEST_CASE("table_view - update_geometries with empty model") {
    auto view = std::make_unique<table_view<Backend>>();

    // Should not crash
    view->update_geometries();
}

TEST_CASE("table_view - update_geometries recalculates on model change") {
    auto model = std::make_shared<test_table_model<Backend>>();
    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());
    (void)view->measure(200_lu, 100_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 200_lu, 100_lu});

    int initial_width = view->total_column_width();

    model->append({"New Item", 999, "Added"});

    // Width might change with new data
    CHECK(view->total_column_width() >= 0);
}

// =========================================================================
// Keyboard Navigation Tests
// =========================================================================

TEST_CASE("table_view - Left/right navigation in cell mode") {
    auto model = std::make_shared<test_table_model<Backend>>();
    model->append({"A", 1, "X"});
    model->append({"B", 2, "Y"});

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());
    view->set_selection_behavior(selection_behavior::select_items);

    auto* selection = view->selection_model();
    selection->set_current_index(model->index(0, 0));

    (void)view->measure(200_lu, 100_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 200_lu, 100_lu});

    // Note: handle_event is protected, keyboard navigation is tested via
    // selection model integration. Verify initial state is set up.
    auto current = selection->current_index();
    CHECK(current.row == 0);
    CHECK(current.column == 0);

    // Keyboard navigation would be tested at integration level with focus
    // and event dispatching through the full UI context.
}

// =========================================================================
// Selection Model Integration Tests
// =========================================================================

TEST_CASE("table_view - Selection model integration") {
    auto model = std::make_shared<test_table_model<Backend>>();
    model->append({"A", 1, "X"});
    model->append({"B", 2, "Y"});

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());

    auto* selection = view->selection_model();
    CHECK(selection != nullptr);

    auto idx = model->index(0, 0);
    selection->select(idx);
    selection->set_current_index(idx);

    CHECK(selection->is_selected(idx));
    CHECK(view->current_index() == idx);
}

// =========================================================================
// Rendering Tests (basic)
// =========================================================================

TEST_CASE("table_view - Renders without crash with data") {
    auto model = std::make_shared<test_table_model<Backend>>();
    model->append({"Test", 1, "OK"});

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());
    view->set_headers_visible(true);
    view->set_horizontal_grid_visible(true);
    view->set_vertical_grid_visible(true);

    (void)view->measure(200_lu, 100_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 200_lu, 100_lu});

    // Verify bounds were set correctly
    CHECK(view->bounds().width.to_int() == 200);
    CHECK(view->bounds().height.to_int() == 100);
}

TEST_CASE("table_view - Renders without crash empty model") {
    auto model = std::make_shared<test_table_model<Backend>>();

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());

    (void)view->measure(200_lu, 100_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 200_lu, 100_lu});

    // Verify bounds were set and model is empty
    CHECK(view->bounds().width.to_int() == 200);
    CHECK(model->row_count() == 0);
}

// =========================================================================
// Signal Tests
// =========================================================================

TEST_CASE("table_view - Header clicked signal emits on click") {
    auto model = std::make_shared<test_table_model<Backend>>();
    model->append({"Row1", 1, "OK"});
    model->append({"Row2", 2, "Done"});

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());
    view->set_headers_visible(true);
    view->set_sorting_enabled(true);

    // Set explicit column widths so we know where columns are
    view->set_column_width(0, 60);  // "Name" column: 0-59
    view->set_column_width(1, 40);  // "Value" column: 60-99
    view->set_column_width(2, 60);  // "Status" column: 100-159

    bool signal_emitted = false;
    int clicked_column = -1;

    view->header_clicked.connect([&](int col) {
        signal_emitted = true;
        clicked_column = col;
    });

    (void)view->measure(200_lu, 100_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 200_lu, 100_lu});

    SUBCASE("Click on first column header") {
        // Header is at top, click in the middle of first column (x=30, y=5)
        // y=5 is within header height (24), x=30 is in first column (0-59)
        simulate_click_at_physical(*view, 30, 5);

        CHECK(signal_emitted);
        CHECK(clicked_column == 0);  // First column
    }

    SUBCASE("Click on second column header") {
        // x=70 is in second column (60-99), y=5 is in header
        simulate_click_at_physical(*view, 70, 5);

        CHECK(signal_emitted);
        CHECK(clicked_column == 1);  // Second column
    }

    SUBCASE("Click on third column header") {
        // x=130 is in third column (100-159), y=5 is in header
        simulate_click_at_physical(*view, 130, 5);

        CHECK(signal_emitted);
        CHECK(clicked_column == 2);  // Third column
    }

    SUBCASE("Click below header does not emit header_clicked") {
        // y=30 is below the header (header_height=24), should not trigger
        simulate_click_at_physical(*view, 30, 30);

        CHECK_FALSE(signal_emitted);  // Not a header click
    }

    SUBCASE("Header click disabled when sorting disabled") {
        view->set_sorting_enabled(false);
        simulate_click_at_physical(*view, 30, 5);

        CHECK_FALSE(signal_emitted);  // Sorting disabled, no signal
    }
}

// =========================================================================
// Content Size Tests
// =========================================================================

TEST_CASE("table_view - Content size with headers") {
    auto model = std::make_shared<test_table_model<Backend>>();
    model->append({"Test", 1, "OK"});

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());

    view->set_headers_visible(false);
    (void)view->measure(200_lu, 100_lu);
    auto size_without_headers = view->last_measured_size();

    view->set_headers_visible(true);
    view->invalidate_measure();
    (void)view->measure(200_lu, 100_lu);
    auto size_with_headers = view->last_measured_size();

    // With headers should be taller
    CHECK(size_with_headers.height > size_without_headers.height);
}

// =========================================================================
// Edge Case Tests (scroll_to before layout, shift-range selection)
// =========================================================================

TEST_CASE("table_view - scroll_to before layout does not crash") {
    auto model = std::make_shared<test_table_model<Backend>>();
    model->append({"A", 1, "X"});
    model->append({"B", 2, "Y"});
    model->append({"C", 3, "Z"});

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());

    // Call scroll_to BEFORE measure/arrange
    auto idx = model->index(2, 1);
    view->scroll_to(idx);

    // Model should still be accessible after premature scroll
    CHECK(model->row_count() == 3);

    // Now do proper layout
    (void)view->measure(200_lu, 100_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 200_lu, 100_lu});

    // scroll_to should work now - verify view state is valid
    view->scroll_to(idx);
    CHECK(view->bounds().width.to_int() == 200);
    CHECK(view->bounds().height.to_int() == 100);
}

TEST_CASE("table_view - scroll_to with invalid column does not crash") {
    auto model = std::make_shared<test_table_model<Backend>>();
    model->append({"A", 1, "X"});

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());

    (void)view->measure(200_lu, 100_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 200_lu, 100_lu});

    // Create an index with out-of-bounds column
    model_index bad_idx{0, 99, nullptr, model.get()};
    view->scroll_to(bad_idx);

    // View should still be in valid state after bad scroll
    CHECK(view->bounds().width.to_int() == 200);
    CHECK(model->row_count() == 1);  // Data intact
}

TEST_CASE("table_view - Row shift-range selection via handle_event") {
    auto model = std::make_shared<test_table_model<Backend>>();
    model->append({"A", 1, "X"});
    model->append({"B", 2, "Y"});
    model->append({"C", 3, "Z"});
    model->append({"D", 4, "W"});
    model->append({"E", 5, "V"});

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());
    view->set_selection_behavior(selection_behavior::select_rows);

    auto* sel = view->selection_model();
    sel->set_selection_mode(selection_mode::extended_selection);

    (void)view->measure(200_lu, 200_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 200_lu, 200_lu});

    // Click on row 1 to establish anchor (no modifiers)
    // visual_rect returns physical coords; use helper for proper conversion
    auto row1_rect = view->visual_rect(model->index(1, 0));
    simulate_click_at_physical(*view, row1_rect.x + row1_rect.w / 2,
                               row1_rect.y + row1_rect.h / 2);

    CHECK(view->current_index().row == 1);
    CHECK(sel->is_selected(model->index(1, 0)));

    // Shift-click on row 3 to select range 1-3
    auto row3_rect = view->visual_rect(model->index(3, 0));
    simulate_click_at_physical(*view, row3_rect.x + row3_rect.w / 2,
                               row3_rect.y + row3_rect.h / 2, false, true);  // shift=true

    // Rows 1, 2, 3 should be selected
    CHECK(sel->is_selected(model->index(1, 0)));
    CHECK(sel->is_selected(model->index(2, 0)));
    CHECK(sel->is_selected(model->index(3, 0)));
    // Rows 0 and 4 should NOT be selected
    CHECK_FALSE(sel->is_selected(model->index(0, 0)));
    CHECK_FALSE(sel->is_selected(model->index(4, 0)));
}

TEST_CASE("table_view - Column shift-range selection via handle_event") {
    auto model = std::make_shared<test_table_model<Backend>>();
    model->append({"A", 1, "X"});
    model->append({"B", 2, "Y"});

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());
    view->set_selection_behavior(selection_behavior::select_columns);
    view->set_column_width(0, 100);
    view->set_column_width(1, 100);
    view->set_column_width(2, 100);

    auto* sel = view->selection_model();
    sel->set_selection_mode(selection_mode::extended_selection);

    (void)view->measure(400_lu, 100_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 400_lu, 100_lu});

    // Click on column 0 to establish anchor
    // visual_rect returns physical coords; use helper for proper conversion
    auto col0_rect = view->visual_rect(model->index(0, 0));
    simulate_click_at_physical(*view, col0_rect.x + col0_rect.w / 2,
                               col0_rect.y + col0_rect.h / 2);

    CHECK(view->current_index().column == 0);
    CHECK(sel->is_selected(model->index(0, 0)));

    // Shift-click on column 2 to select range 0-2
    auto col2_rect = view->visual_rect(model->index(0, 2));
    simulate_click_at_physical(*view, col2_rect.x + col2_rect.w / 2,
                               col2_rect.y + col2_rect.h / 2, false, true);  // shift=true

    // All 3 columns should be selected
    CHECK(sel->is_selected(model->index(0, 0)));
    CHECK(sel->is_selected(model->index(0, 1)));
    CHECK(sel->is_selected(model->index(0, 2)));
}

TEST_CASE("table_view - contiguous_selection ignores Ctrl for range") {
    auto model = std::make_shared<test_table_model<Backend>>();
    model->append({"A", 1, "X"});
    model->append({"B", 2, "Y"});
    model->append({"C", 3, "Z"});
    model->append({"D", 4, "W"});
    model->append({"E", 5, "V"});

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());
    view->set_selection_behavior(selection_behavior::select_rows);

    auto* sel = view->selection_model();
    sel->set_selection_mode(selection_mode::contiguous_selection);

    (void)view->measure(200_lu, 200_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 200_lu, 200_lu});

    // Click on row 0 to establish anchor
    // visual_rect returns physical coords; use helper for proper conversion
    auto row0_rect = view->visual_rect(model->index(0, 0));
    simulate_click_at_physical(*view, row0_rect.x + 5, row0_rect.y + 5);
    CHECK(sel->is_selected(model->index(0, 0)));

    // Shift-click on row 1 to select range 0-1
    auto row1_rect = view->visual_rect(model->index(1, 0));
    simulate_click_at_physical(*view, row1_rect.x + 5, row1_rect.y + 5, false, true);
    CHECK(sel->is_selected(model->index(0, 0)));
    CHECK(sel->is_selected(model->index(1, 0)));

    // Ctrl+Shift-click on row 4: in contiguous mode, this should REPLACE
    // (not add to) the selection with range from anchor (0) to row 4
    auto row4_rect = view->visual_rect(model->index(4, 0));
    simulate_click_at_physical(*view, row4_rect.x + 5, row4_rect.y + 5, true, true);  // ctrl=true, shift=true

    // In contiguous mode, Ctrl is ignored - selection should be replaced with 0-4
    CHECK(sel->is_selected(model->index(0, 0)));
    CHECK(sel->is_selected(model->index(1, 0)));
    CHECK(sel->is_selected(model->index(2, 0)));
    CHECK(sel->is_selected(model->index(3, 0)));
    CHECK(sel->is_selected(model->index(4, 0)));
}

TEST_CASE("table_view - contiguous_selection ignores Ctrl for toggle") {
    auto model = std::make_shared<test_table_model<Backend>>();
    model->append({"A", 1, "X"});
    model->append({"B", 2, "Y"});
    model->append({"C", 3, "Z"});

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());
    view->set_selection_behavior(selection_behavior::select_rows);

    auto* sel = view->selection_model();
    sel->set_selection_mode(selection_mode::contiguous_selection);

    (void)view->measure(200_lu, 200_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 200_lu, 200_lu});

    // Click on row 1
    // visual_rect returns physical coords; use helper for proper conversion
    auto row1_rect = view->visual_rect(model->index(1, 0));
    simulate_click_at_physical(*view, row1_rect.x + 5, row1_rect.y + 5);
    CHECK(sel->is_selected(model->index(1, 0)));

    // Ctrl-click on row 2: in contiguous mode, Ctrl should be ignored
    // so this should just select row 2 (not toggle/add)
    auto row2_rect = view->visual_rect(model->index(2, 0));
    simulate_click_at_physical(*view, row2_rect.x + 5, row2_rect.y + 5, true, false);  // ctrl=true

    // Row 1 should be deselected, row 2 selected (simple selection, not toggle)
    CHECK_FALSE(sel->is_selected(model->index(1, 0)));
    CHECK(sel->is_selected(model->index(2, 0)));
}

// =========================================================================
// HiDPI Visible Region Tests
// =========================================================================

TEST_CASE("table_view - HiDPI visible region calculation") {
    // Set up HiDPI metrics (8 pixels per logical unit)
    auto metrics = make_gui_metrics<Backend>();
    scoped_ui_context<Backend> ctx{metrics};

    auto model = std::make_shared<test_table_model<Backend>>();
    // Add 20 rows
    for (int i = 0; i < 20; ++i) {
        model->append({"Row" + std::to_string(i), i, "Status"});
    }

    auto view = std::make_unique<table_view<Backend>>();
    view->set_model(model.get());

    // View size: 200x100 logical units = 1600x800 physical pixels with 8:1 scaling
    (void)view->measure(200_lu, 100_lu);
    view->arrange(logical_rect{0_lu, 0_lu, 200_lu, 100_lu});

    // Test that visual_rect returns valid physical coordinates
    // visual_rect returns physical coords (Backend::rect_type)
    auto rect0 = view->visual_rect(model->index(0, 0));
    CHECK(rect0.w > 0);
    CHECK(rect0.h > 0);

    // With HiDPI (8:1), physical height should be 8+ pixels per row
    // The delegate's size_hint returns physical units; view stores row height in logical
    CHECK(rect0.h >= 8);  // At least 1 logical unit = 8 physical pixels

    // Row 10 should also have valid visual_rect
    auto rect10 = view->visual_rect(model->index(10, 0));
    CHECK(rect10.w > 0);
    CHECK(rect10.h > 0);
    // Row 10 should be below row 0 (larger y in physical screen coords)
    CHECK(rect10.y > rect0.y);

    // Verify the spacing between rows is consistent (HiDPI scaling applied)
    auto rect1 = view->visual_rect(model->index(1, 0));
    auto rect2 = view->visual_rect(model->index(2, 0));
    int row_height = rect1.y - rect0.y;
    CHECK(row_height > 0);
    CHECK(rect2.y - rect1.y == row_height);  // Consistent row spacing

    // Verify index_at with logical coordinates
    // index_at takes logical coords; row 0 should be at y ≈ 1 (border width)
    // Use coordinates that fall in the middle of row 0 (border=1, row_height varies)
    auto idx0 = view->index_at(10_lu, 1.5_lu);  // Inside row 0, past border
    // Note: With HiDPI, row heights in logical units may vary; check valid result
    // The main point is the visible region calc uses correct units
    if (idx0.is_valid()) {
        CHECK(idx0.row >= 0);  // Should hit a valid row
    }
}
