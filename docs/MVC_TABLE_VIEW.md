# Table View Design Document

**Status:** Design Phase
**Created:** 2026-01-02
**Author:** Claude Code

---

## Executive Summary

This document specifies the design for `table_view` and `table_model` - the tabular data components of the OnyxUI MVC framework. The implementation follows Qt-inspired patterns already established by `list_view` and `list_model`.

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [table_model](#table_model)
4. [table_view](#table_view)
5. [Column Management](#column-management)
6. [Headers](#headers)
7. [Selection](#selection)
8. [Keyboard Navigation](#keyboard-navigation)
9. [Theme Integration](#theme-integration)
10. [Simple Wrapper (table)](#simple-wrapper-table)
11. [Implementation Plan](#implementation-plan)
12. [API Reference](#api-reference)
13. [Examples](#examples)

---

## Overview

### Purpose

`table_view` displays 2D tabular data with rows and columns, similar to spreadsheets, database tables, or file explorers. It builds on the existing MVC infrastructure:

- **table_model**: Stores and manages 2D data (rows × columns)
- **table_view**: Renders the table with headers, scrolling, selection
- **Delegates**: Render individual cells (reuses existing delegate system)
- **Selection Model**: Manages cell/row/column selection (extends existing)

### Design Goals

1. **Consistency**: Follow patterns established by `list_view`
2. **Column-aware**: Support column headers, resizing, sorting
3. **Flexible selection**: Row, column, cell, or range selection
4. **Virtual scrolling**: Only render visible cells for large tables
5. **Theme integration**: Use existing `list_style` with table extensions
6. **Dual-API**: Both simple `table` widget and advanced `table_view`

### Non-Goals (Future Work)

- Cell editing (inline edit widgets)
- Column reordering via drag-drop
- Frozen rows/columns
- Cell merging/spanning
- Rich cell types (checkboxes, progress bars, images)

---

## Architecture

### Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                         table_view                               │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    Column Headers                         │    │
│  │  ┌──────────┬──────────┬──────────┬──────────┐          │    │
│  │  │  Name    │   Age    │  Email   │  Status  │          │    │
│  │  └──────────┴──────────┴──────────┴──────────┘          │    │
│  └─────────────────────────────────────────────────────────┘    │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    Cell Grid (virtual scrolling)          │    │
│  │  ┌──────────┬──────────┬──────────┬──────────┐          │    │
│  │  │ Alice    │    30    │ a@x.com  │  Active  │ ← Row 0  │    │
│  │  ├──────────┼──────────┼──────────┼──────────┤          │    │
│  │  │ Bob      │    25    │ b@x.com  │  Pending │ ← Row 1  │    │
│  │  ├──────────┼──────────┼──────────┼──────────┤          │    │
│  │  │ Charlie  │    35    │ c@x.com  │  Active  │ ← Row 2  │    │
│  │  └──────────┴──────────┴──────────┴──────────┘          │    │
│  │     Col 0      Col 1      Col 2      Col 3               │    │
│  └─────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
         │                    │                    │
         ▼                    ▼                    ▼
   ┌──────────┐        ┌──────────┐        ┌────────────────┐
   │ table_   │        │ selection│        │ default_item_  │
   │ model    │        │ _model   │        │ delegate       │
   └──────────┘        └──────────┘        └────────────────┘
```

### Inheritance Hierarchy

```
ui_element<Backend>
    └── widget<Backend>
            └── abstract_item_view<Backend>
                    ├── list_view<Backend>    (existing)
                    └── table_view<Backend>   (NEW)
```

### Relationship to Existing Components

| Component | Reuse Strategy |
|-----------|----------------|
| `abstract_item_model` | Base class for `table_model` |
| `abstract_item_view` | Base class for `table_view` |
| `model_index` | Unchanged - uses row + column |
| `item_selection_model` | Extended for cell/row/column selection |
| `default_item_delegate` | Reused for cell rendering |
| `list_style` | Extended to `table_style` |

---

## table_model

### Overview

`table_model` provides a 2D data container with typed columns. Unlike `list_model` which stores `vector<T>`, `table_model` stores `vector<Row>` where each `Row` contains column values.

### Design Approach

Two approaches considered:

1. **Generic Row Type** (chosen): `table_model<Row, Backend>` where Row is user-defined
2. **Dynamic Columns**: Runtime column definition with `std::variant` storage

We choose **Generic Row Type** for type safety and simplicity, matching Qt's pattern.

### Header File Structure

```cpp
// include/onyxui/mvc/models/table_model.hh

template<typename Row, UIBackend Backend>
class table_model : public abstract_item_model<Backend> {
public:
    using value_type = Row;
    using container_type = std::vector<Row>;

    // === Column Configuration ===

    /// Number of columns in the model
    [[nodiscard]] int column_count(const model_index& parent = {}) const override;

    /// Set column headers (call before populating data)
    void set_headers(std::vector<std::string> headers);

    /// Get header for a column
    [[nodiscard]] std::string header(int column) const;

    // === Data Access (row-based) ===

    [[nodiscard]] int row_count(const model_index& parent = {}) const override;
    [[nodiscard]] model_index index(int row, int column,
                                     const model_index& parent = {}) const override;
    [[nodiscard]] model_index parent(const model_index& child) const override;
    [[nodiscard]] variant_type data(const model_index& index,
                                     item_data_role role = item_data_role::display) const override;

    // === Data Modification ===

    void set_rows(container_type rows);
    void append(const Row& row);
    void append(Row&& row);
    void insert(int row, const Row& item);
    void remove(int row);
    void clear();

    // === Direct Access ===

    [[nodiscard]] const container_type& rows() const noexcept;
    [[nodiscard]] const Row& at(int row) const;

    // === Sorting ===

    void sort(int column, sort_order order = sort_order::ascending) override;

protected:
    /// Subclass must implement: extract column value from row
    /// This is the key customization point
    [[nodiscard]] virtual variant_type column_data(const Row& row, int column,
                                                    item_data_role role) const = 0;
};
```

### Key Design Decision: Column Data Extraction

The model needs to know how to extract data from each column. Options:

1. **Virtual method** (chosen): Subclass overrides `column_data()`
2. **Callback**: Pass `std::function` for each column
3. **Reflection**: Use compile-time reflection (C++26)

We choose **virtual method** for clarity and type safety:

```cpp
// Example: PersonTableModel
struct Person {
    std::string name;
    int age;
    std::string email;
};

template<UIBackend Backend>
class person_table_model : public table_model<Person, Backend> {
protected:
    variant_type column_data(const Person& p, int col, item_data_role role) const override {
        if (role != item_data_role::display) return std::monostate{};
        switch (col) {
            case 0: return p.name;
            case 1: return std::to_string(p.age);
            case 2: return p.email;
            default: return std::monostate{};
        }
    }
};
```

### Alternative: Convenience Macro

For simple cases, provide a macro to reduce boilerplate:

```cpp
// Define column accessors as lambdas
auto model = make_table_model<Person, Backend>({
    {"Name",  [](const Person& p) { return p.name; }},
    {"Age",   [](const Person& p) { return std::to_string(p.age); }},
    {"Email", [](const Person& p) { return p.email; }}
});
```

---

## table_view

### Overview

`table_view` extends `abstract_item_view` with table-specific features:
- Column headers (optional)
- Column widths (fixed, auto, stretch)
- Horizontal + vertical scrolling
- Cell-level hit testing
- Grid lines (optional)

### Header File Structure

```cpp
// include/onyxui/mvc/views/table_view.hh

template<UIBackend Backend>
class table_view : public abstract_item_view<Backend> {
public:
    using base = abstract_item_view<Backend>;
    using rect_type = typename Backend::rect_type;
    using size_type = typename Backend::size_type;

    // === Construction ===

    table_view();

    // === Column Configuration ===

    /// Set column width (logical units, 0 = auto)
    void set_column_width(int column, int width);

    /// Get column width
    [[nodiscard]] int column_width(int column) const;

    /// Set minimum column width
    void set_minimum_column_width(int column, int min_width);

    /// Enable column to stretch to fill remaining space
    void set_column_stretch(int column, int stretch_factor);

    /// Get total width of all columns
    [[nodiscard]] int total_column_width() const;

    // === Headers ===

    /// Show/hide column headers
    void set_headers_visible(bool visible);
    [[nodiscard]] bool headers_visible() const noexcept;

    /// Header height (0 = hide headers)
    void set_header_height(int height);
    [[nodiscard]] int header_height() const noexcept;

    // === Grid Lines ===

    /// Show/hide horizontal grid lines between rows
    void set_horizontal_grid_visible(bool visible);
    [[nodiscard]] bool horizontal_grid_visible() const noexcept;

    /// Show/hide vertical grid lines between columns
    void set_vertical_grid_visible(bool visible);
    [[nodiscard]] bool vertical_grid_visible() const noexcept;

    // === Selection Behavior ===

    /// Selection granularity
    enum class selection_behavior : std::uint8_t {
        select_items,    ///< Select individual cells
        select_rows,     ///< Click selects entire row (default)
        select_columns   ///< Click selects entire column
    };

    void set_selection_behavior(selection_behavior behavior);
    [[nodiscard]] selection_behavior get_selection_behavior() const noexcept;

    // === Scrolling ===

    [[nodiscard]] double scroll_offset_x() const noexcept;
    [[nodiscard]] double scroll_offset_y() const noexcept;
    void set_scroll_offset(double x, double y);
    void scroll_to_column(int column);

    // === Sorting ===

    /// Enable/disable clickable headers for sorting
    void set_sorting_enabled(bool enabled);
    [[nodiscard]] bool sorting_enabled() const noexcept;

    /// Get current sort column (-1 = none)
    [[nodiscard]] int sort_column() const noexcept;

    /// Get current sort order
    [[nodiscard]] sort_order get_sort_order() const noexcept;

    // === Signals ===

    /// Emitted when header is clicked (for sorting)
    signal<int> header_clicked;

    /// Emitted when column is resized
    signal<int, int> column_resized;  // column, new_width

    // === Required Overrides from abstract_item_view ===

    [[nodiscard]] model_index index_at(logical_unit x, logical_unit y) const override;
    [[nodiscard]] rect_type visual_rect(const model_index& index) const override;
    void scroll_to(const model_index& index) override;
    void update_geometries() override;

protected:
    void do_render(render_context<Backend>& ctx) const override;
    [[nodiscard]] logical_size get_content_size() const override;
    void do_arrange(const logical_rect& final_bounds) override;

    // === Column-aware Navigation ===

    [[nodiscard]] model_index move_cursor_left(const model_index& current) const;
    [[nodiscard]] model_index move_cursor_right(const model_index& current) const;
};
```

### Rendering Strategy

```
┌─────────────────────────────────────────────────────────────────┐
│ Header Row (fixed, doesn't scroll vertically)                    │
│ ┌───────────┬───────────┬───────────┬───────────┐              │
│ │   Name ▼  │    Age    │   Email   │  Status   │              │
│ └───────────┴───────────┴───────────┴───────────┘              │
├─────────────────────────────────────────────────────────────────┤
│ Scrollable Cell Grid                                             │
│ ┌───────────┬───────────┬───────────┬───────────┐              │
│ │ Cell(0,0) │ Cell(0,1) │ Cell(0,2) │ Cell(0,3) │              │
│ ├───────────┼───────────┼───────────┼───────────┤              │
│ │ Cell(1,0) │ Cell(1,1) │ Cell(1,2) │ Cell(1,3) │              │
│ ├───────────┼───────────┼───────────┼───────────┤              │
│ │ Cell(2,0) │ Cell(2,1) │ Cell(2,2) │ Cell(2,3) │              │
│ └───────────┴───────────┴───────────┴───────────┘              │
│                                                                  │
│ (virtual scrolling - only visible cells rendered)                │
└─────────────────────────────────────────────────────────────────┘
```

### Virtual Scrolling Algorithm

```cpp
void do_render(render_context<Backend>& ctx) const override {
    // 1. Render headers (if visible)
    if (m_headers_visible) {
        render_headers(ctx);
    }

    // 2. Calculate visible region
    int first_visible_row = calculate_first_visible_row();
    int last_visible_row = calculate_last_visible_row();
    int first_visible_col = calculate_first_visible_col();
    int last_visible_col = calculate_last_visible_col();

    // 3. Render only visible cells
    for (int row = first_visible_row; row <= last_visible_row; ++row) {
        for (int col = first_visible_col; col <= last_visible_col; ++col) {
            model_index idx = m_model->index(row, col);
            rect_type cell_rect = calculate_cell_rect(row, col);

            bool selected = is_cell_selected(idx);
            bool focused = is_current_cell(idx);

            m_delegate->paint(ctx, idx, cell_rect, selected, focused);
        }

        // Optional: horizontal grid line
        if (m_horizontal_grid_visible) {
            render_horizontal_grid_line(ctx, row);
        }
    }

    // 4. Render vertical grid lines
    if (m_vertical_grid_visible) {
        for (int col = first_visible_col; col <= last_visible_col; ++col) {
            render_vertical_grid_line(ctx, col);
        }
    }
}
```

---

## Column Management

### Column Width Modes

| Mode | Description | Use Case |
|------|-------------|----------|
| **Fixed** | Exact width in logical units | Known content width |
| **Auto** | Width based on content | Dynamic content |
| **Stretch** | Fill remaining space proportionally | Flexible layouts |

### Column Configuration

```cpp
struct column_info {
    std::string header;           ///< Header text
    int width = 0;                ///< Width (0 = auto)
    int min_width = 20;           ///< Minimum width
    int stretch_factor = 0;       ///< Stretch factor (0 = don't stretch)
    horizontal_alignment align = horizontal_alignment::left;  ///< Text alignment
    bool sortable = true;         ///< Can sort by this column?
    bool resizable = true;        ///< Can user resize?
};

// Usage
table_view->set_column_width(0, 150);    // Column 0: fixed 150px
table_view->set_column_width(1, 0);      // Column 1: auto
table_view->set_column_stretch(2, 1);    // Column 2: stretch to fill
```

### Auto-Width Calculation

```cpp
int calculate_auto_width(int column) const {
    int max_width = 0;

    // Measure header
    max_width = std::max(max_width, measure_header_width(column));

    // Sample visible rows (performance optimization)
    int sample_count = std::min(row_count(), 100);
    for (int row = 0; row < sample_count; ++row) {
        model_index idx = m_model->index(row, column);
        auto size = m_delegate->size_hint(idx);
        max_width = std::max(max_width, size.w);
    }

    return max_width + m_cell_padding * 2;
}
```

---

## Headers

### Header Rendering

Headers are rendered as a separate row above the data:

```cpp
void render_headers(render_context<Backend>& ctx) const {
    int x = -m_scroll_offset_x;
    int y = 0;  // Headers don't scroll vertically

    for (int col = 0; col < column_count(); ++col) {
        rect_type header_rect{x, y, column_width(col), m_header_height};

        // Draw header background
        ctx.fill_rect(header_rect, theme->table.header_background);

        // Draw header text
        std::string text = m_model->header(col);
        ctx.draw_text(text, {x + padding, y + padding}, font, fg_color);

        // Draw sort indicator if sorted by this column
        if (m_sort_column == col) {
            draw_sort_indicator(ctx, header_rect, m_sort_order);
        }

        x += column_width(col);
    }
}
```

### Sort Indicators

```
Ascending:  ▲  or  ↑  or  △
Descending: ▼  or  ↓  or  ▽
```

### Header Click Handling

```cpp
bool handle_header_click(int x, int y) {
    if (y > m_header_height) return false;  // Click in data area
    if (!m_sorting_enabled) return false;

    int column = column_at_x(x + m_scroll_offset_x);
    if (column < 0) return false;

    // Toggle sort order if same column, else ascending
    sort_order new_order = (column == m_sort_column && m_sort_order == sort_order::ascending)
                         ? sort_order::descending
                         : sort_order::ascending;

    m_sort_column = column;
    m_sort_order = new_order;

    // Notify model to sort
    if (m_model) {
        m_model->sort(column, new_order);
    }

    header_clicked.emit(column);
    return true;
}
```

---

## Selection

### Selection Behavior

```cpp
enum class selection_behavior : std::uint8_t {
    select_items,    ///< Individual cells (spreadsheet-like)
    select_rows,     ///< Entire rows (file manager, table views)
    select_columns   ///< Entire columns (spreadsheet column operations)
};
```

### Row Selection (Default)

Most common mode - clicking anywhere in a row selects the entire row:

```cpp
void handle_click_select_rows(int row, bool ctrl, bool shift) {
    // Select all columns in the row
    for (int col = 0; col < column_count(); ++col) {
        model_index idx = m_model->index(row, col);
        // ... apply selection based on mode and modifiers
    }
}
```

### Cell Selection

For spreadsheet-like behavior:

```cpp
void handle_click_select_items(int row, int col, bool ctrl, bool shift) {
    model_index idx = m_model->index(row, col);

    if (shift && m_selection_anchor.is_valid()) {
        // Rectangle selection from anchor to clicked cell
        select_rectangle(m_selection_anchor, idx);
    } else if (ctrl) {
        m_selection_model->toggle(idx);
    } else {
        m_selection_model->clear_selection();
        m_selection_model->select(idx);
    }
}
```

### Rectangle Selection

For cell selection mode with Shift+Click:

```cpp
void select_rectangle(const model_index& top_left, const model_index& bottom_right) {
    int r1 = std::min(top_left.row, bottom_right.row);
    int r2 = std::max(top_left.row, bottom_right.row);
    int c1 = std::min(top_left.column, bottom_right.column);
    int c2 = std::max(top_left.column, bottom_right.column);

    for (int row = r1; row <= r2; ++row) {
        for (int col = c1; col <= c2; ++col) {
            model_index idx = m_model->index(row, col);
            m_selection_model->select(idx);
        }
    }
}
```

---

## Keyboard Navigation

### Navigation Keys

| Key | Action |
|-----|--------|
| **↑ / ↓** | Move up/down one row |
| **← / →** | Move left/right one column (cell mode) or no-op (row mode) |
| **Page Up/Down** | Move by visible page |
| **Home** | Move to first row (+ Ctrl: first cell) |
| **End** | Move to last row (+ Ctrl: last cell) |
| **Tab** | Move to next column (cell mode) |
| **Shift+Tab** | Move to previous column (cell mode) |
| **Enter** | Activate current row/cell |
| **Space** | Toggle selection (multi/extended mode) |

### Implementation

```cpp
model_index move_cursor_left(const model_index& current) const {
    if (m_selection_behavior != selection_behavior::select_items) {
        return current;  // No horizontal movement in row mode
    }

    if (!current.is_valid()) {
        return m_model->index(0, column_count() - 1);
    }

    if (current.column > 0) {
        return m_model->index(current.row, current.column - 1);
    }

    // Wrap to previous row's last column
    if (current.row > 0) {
        return m_model->index(current.row - 1, column_count() - 1);
    }

    return current;  // Already at top-left
}

model_index move_cursor_right(const model_index& current) const {
    if (m_selection_behavior != selection_behavior::select_items) {
        return current;  // No horizontal movement in row mode
    }

    if (!current.is_valid()) {
        return m_model->index(0, 0);
    }

    if (current.column < column_count() - 1) {
        return m_model->index(current.row, current.column + 1);
    }

    // Wrap to next row's first column
    if (current.row < m_model->row_count() - 1) {
        return m_model->index(current.row + 1, 0);
    }

    return current;  // Already at bottom-right
}
```

---

## Theme Integration

### Table-Specific Theme Extensions

Extend `list_style` to include table-specific styling:

```cpp
struct table_style : list_style {
    // Header styling
    color_type header_background;         ///< Header row background
    color_type header_foreground;         ///< Header text color
    color_type header_border;             ///< Header bottom border
    font_type header_font{};              ///< Header font (often bold)
    int header_height = 24;               ///< Header row height

    // Grid lines
    color_type grid_line_color;           ///< Grid line color
    int grid_line_width = 1;              ///< Grid line thickness

    // Sort indicators
    icon_style_type sort_ascending{};     ///< ▲ icon
    icon_style_type sort_descending{};    ///< ▼ icon

    // Column resizing
    color_type resize_handle_color;       ///< Column resize handle
    int resize_handle_width = 3;          ///< Resize handle hit area
};
```

### Theme Usage in Rendering

```cpp
void render_cell(render_context<Backend>& ctx, const model_index& idx,
                 const rect_type& rect, bool selected, bool focused) const {
    auto* theme = ui_services<Backend>::themes()->get_current_theme();

    // Background
    if (selected) {
        ctx.fill_rect(rect, theme->table.selection_background);
    } else if (idx.row % 2 == 1 && m_alternating_rows) {
        ctx.fill_rect(rect, theme->table.item_background_alt);
    }

    // Text
    color_type fg = selected ? theme->table.selection_foreground
                             : theme->table.item_foreground;

    auto text_data = m_model->data(idx, item_data_role::display);
    if (std::holds_alternative<std::string>(text_data)) {
        ctx.draw_text(std::get<std::string>(text_data),
                      {rect.x + padding, rect.y + padding},
                      theme->table.font, fg);
    }

    // Focus rectangle
    if (focused) {
        ctx.draw_rect(rect, theme->table.focus_box_style);
    }
}
```

---

## Simple Wrapper (table)

### Overview

Following the dual-API pattern, provide a simple `table` widget that wraps `table_view`:

```cpp
// Simple API - no MVC knowledge needed
auto tbl = std::make_unique<table<Backend>>();

// Set columns
tbl->set_columns({"Name", "Age", "Email"});

// Add rows
tbl->add_row({"Alice", "30", "alice@example.com"});
tbl->add_row({"Bob", "25", "bob@example.com"});

// Selection
tbl->selection_changed.connect([](int row) {
    std::cout << "Selected row: " << row << "\n";
});
```

### Implementation

```cpp
template<UIBackend Backend>
class table : public widget<Backend> {
public:
    // Column setup
    void set_columns(std::vector<std::string> headers);

    // Row manipulation (string-based for simplicity)
    void add_row(std::vector<std::string> values);
    void insert_row(int row, std::vector<std::string> values);
    void remove_row(int row);
    void clear();

    // Cell access
    void set_cell(int row, int col, const std::string& value);
    [[nodiscard]] std::string cell(int row, int col) const;

    // Selection
    [[nodiscard]] int current_row() const;
    void set_current_row(int row);

    // Signals
    signal<int> selection_changed;  // row index
    signal<int> row_activated;      // row index (double-click/Enter)

private:
    std::unique_ptr<table_view<Backend>> m_view;
    std::shared_ptr<string_table_model<Backend>> m_model;
};
```

---

## Implementation Plan

### Phase 1: table_model (Foundation)

**Files:**
- `include/onyxui/mvc/models/table_model.hh`

**Tasks:**
1. Create `table_model<Row, Backend>` template
2. Implement row_count, column_count, index, parent, data
3. Implement set_rows, append, insert, remove, clear
4. Implement headers management
5. Implement sorting support
6. Add tests (40+ test cases)

**Estimated Tests:** 40

### Phase 2: table_view Core

**Files:**
- `include/onyxui/mvc/views/table_view.hh`

**Tasks:**
1. Create `table_view` class inheriting from `abstract_item_view`
2. Implement column width management
3. Implement `index_at()` for cell hit testing
4. Implement `visual_rect()` for cell rectangles
5. Implement basic rendering (cells only, no headers)
6. Add horizontal scrolling support
7. Add tests (50+ test cases)

**Estimated Tests:** 50

### Phase 3: Headers and Grid Lines

**Tasks:**
1. Implement header rendering
2. Implement sort indicators
3. Implement header click handling
4. Implement grid line rendering
5. Add tests (30+ test cases)

**Estimated Tests:** 30

### Phase 4: Selection and Navigation

**Tasks:**
1. Implement selection_behavior modes
2. Implement rectangle selection for cell mode
3. Implement horizontal keyboard navigation
4. Implement Tab/Shift+Tab navigation
5. Add tests (40+ test cases)

**Estimated Tests:** 40

### Phase 5: Theme Integration

**Files:**
- Update `include/onyxui/theming/theme.hh`

**Tasks:**
1. Add `table_style` to theme
2. Update default themes with table styling
3. Integrate theme usage in table_view rendering
4. Add tests (15+ test cases)

**Estimated Tests:** 15

### Phase 6: Simple table Wrapper

**Files:**
- `include/onyxui/widgets/input/table.hh`

**Tasks:**
1. Create string-based `string_table_model`
2. Create `table` widget wrapping `table_view`
3. Implement simple API (set_columns, add_row, etc.)
4. Add tests (25+ test cases)

**Estimated Tests:** 25

### Total Estimated Tests: ~200

---

## API Reference

### table_model

```cpp
template<typename Row, UIBackend Backend>
class table_model : public abstract_item_model<Backend>;
```

| Method | Description |
|--------|-------------|
| `row_count()` | Number of rows |
| `column_count()` | Number of columns |
| `set_headers(headers)` | Set column headers |
| `header(column)` | Get header text |
| `set_rows(rows)` | Replace all rows |
| `append(row)` | Add row at end |
| `insert(row, item)` | Insert row at position |
| `remove(row)` | Remove row |
| `clear()` | Remove all rows |
| `sort(column, order)` | Sort by column |

### table_view

```cpp
template<UIBackend Backend>
class table_view : public abstract_item_view<Backend>;
```

| Method | Description |
|--------|-------------|
| `set_column_width(col, width)` | Set column width |
| `column_width(col)` | Get column width |
| `set_headers_visible(visible)` | Show/hide headers |
| `set_horizontal_grid_visible(visible)` | Show/hide h-grid |
| `set_vertical_grid_visible(visible)` | Show/hide v-grid |
| `set_selection_behavior(behavior)` | Row/cell/column selection |
| `set_sorting_enabled(enabled)` | Enable header click sorting |
| `scroll_to_column(column)` | Scroll column into view |

---

## Examples

### Example 1: Person Table (Advanced API)

```cpp
// Define data structure
struct Person {
    std::string name;
    int age;
    std::string email;
    bool active;
};

// Create custom model
template<UIBackend Backend>
class person_model : public table_model<Person, Backend> {
public:
    person_model() {
        this->set_headers({"Name", "Age", "Email", "Status"});
    }

protected:
    variant_type column_data(const Person& p, int col, item_data_role role) const override {
        if (role == item_data_role::display) {
            switch (col) {
                case 0: return p.name;
                case 1: return std::to_string(p.age);
                case 2: return p.email;
                case 3: return p.active ? "Active" : "Inactive";
            }
        }
        // Custom background for inactive users
        if (role == item_data_role::background && col == 3 && !p.active) {
            return typename Backend::color_type{255, 200, 200};  // Light red
        }
        return std::monostate{};
    }
};

// Usage
auto model = std::make_shared<person_model<Backend>>();
model->set_rows({
    {"Alice", 30, "alice@x.com", true},
    {"Bob", 25, "bob@x.com", false},
    {"Charlie", 35, "charlie@x.com", true}
});

auto view = std::make_unique<table_view<Backend>>();
view->set_model(model.get());
view->set_column_width(0, 100);
view->set_column_width(1, 50);
view->set_column_stretch(2, 1);  // Email stretches
view->set_sorting_enabled(true);
```

### Example 2: Simple String Table

```cpp
// Simple API
auto tbl = std::make_unique<table<Backend>>();
tbl->set_columns({"Product", "Price", "Quantity"});

tbl->add_row({"Widget", "$9.99", "100"});
tbl->add_row({"Gadget", "$19.99", "50"});
tbl->add_row({"Gizmo", "$14.99", "75"});

tbl->row_activated.connect([&](int row) {
    std::cout << "Editing row " << row << "\n";
});

container->add_child(std::move(tbl));
```

### Example 3: File List

```cpp
struct FileInfo {
    std::string name;
    std::size_t size;
    std::time_t modified;
    bool is_directory;
};

template<UIBackend Backend>
class file_model : public table_model<FileInfo, Backend> {
public:
    file_model() {
        this->set_headers({"Name", "Size", "Modified"});
    }

protected:
    variant_type column_data(const FileInfo& f, int col, item_data_role role) const override {
        if (role == item_data_role::display) {
            switch (col) {
                case 0: return f.name;
                case 1: return f.is_directory ? "<DIR>" : format_size(f.size);
                case 2: return format_date(f.modified);
            }
        }
        if (role == item_data_role::decoration && col == 0) {
            // Icon based on file type (future)
        }
        return std::monostate{};
    }
};
```

---

## Testing Strategy

### Unit Tests

1. **table_model tests** (`unittest/mvc/test_table_model.cc`)
   - Row/column count
   - Index creation
   - Data access
   - Row insertion/removal
   - Sorting
   - Signal emission

2. **table_view tests** (`unittest/widgets/test_table_view.cc`)
   - Column width management
   - Hit testing (index_at)
   - Cell rectangles (visual_rect)
   - Scrolling
   - Header visibility
   - Grid lines

3. **Selection tests** (`unittest/widgets/test_table_selection.cc`)
   - Row selection mode
   - Cell selection mode
   - Rectangle selection
   - Keyboard navigation

4. **Integration tests** (`unittest/widgets/test_table_integration.cc`)
   - Model + view + selection together
   - Sorting via headers
   - Theme integration

---

## Open Questions

1. **Column resizing**: Should we support mouse-drag column resizing in this iteration?
   - Recommendation: Defer to future work

2. **Frozen columns**: Should we support frozen left columns (like Excel)?
   - Recommendation: Defer to future work

3. **Cell types**: Should cells support different types (checkbox, progress)?
   - Recommendation: Defer - use custom delegates for now

4. **Performance**: For very large tables (100k+ rows), should we implement data fetching callbacks?
   - Recommendation: Virtual scrolling handles this; defer advanced caching

---

## Revision History

| Date | Author | Changes |
|------|--------|---------|
| 2026-01-02 | Claude Code | Initial design document |
