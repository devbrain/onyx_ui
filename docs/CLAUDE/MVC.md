# OnyxUI MVC Guide

This document explains the Model-View-Controller (MVC) framework in OnyxUI, including the Qt-inspired dual-API pattern for list and combo box widgets.

## Table of Contents

- [Overview](#overview)
- [Dual-API Pattern](#dual-api-pattern)
- [Simple Widgets](#simple-widgets)
- [Advanced *View Widgets](#advanced-view-widgets)
- [Core MVC Components](#core-mvc-components)
- [Selection Management](#selection-management)
- [Delegates](#delegates)
- [Examples](#examples)
- [Migration Guide](#migration-guide)

---

## Overview

OnyxUI provides a Qt-inspired MVC architecture for data-driven widgets like lists and combo boxes. The framework offers two levels of API:

| Simple Widget | Advanced Widget | Use Case |
|---------------|-----------------|----------|
| `combo_box` | `combo_box_view` | Dropdown selection |
| `list_box` | `list_view` | Vertical item list |
| `table` (future) | `table_view` (future) | Grid of cells |
| `tree` (future) | `tree_view` (future) | Hierarchical items |

**Choose Simple Widgets when:**
- You want a quick, self-contained widget
- Your data is simple strings
- You don't need custom rendering

**Choose Advanced *View Widgets when:**
- You need custom models (database, network, etc.)
- You want custom item rendering (delegates)
- You need fine-grained selection control
- You're sharing a model across multiple views

---

## Dual-API Pattern

### Philosophy

Following Qt's successful pattern, each MVC widget has two variants:

1. **Simple widgets** (`combo_box`, `list_box`) - Own their data, provide easy string-based API
2. **Advanced *View widgets** (`combo_box_view`, `list_view`) - Require external model, full MVC control

Simple widgets internally wrap the advanced views, so you get the same rendering and behavior with less code.

### Architecture

```
┌─────────────────────────────────────────┐
│            combo_box (simple)           │
│  ┌─────────────────────────────────┐    │
│  │  list_model<std::string>        │    │  ← Internal model (owned)
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │  combo_box_view (child widget)  │    │  ← Internal view (child)
│  └─────────────────────────────────┘    │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│         combo_box_view (advanced)       │
│  model ──────► abstract_item_model*     │  ← External model (borrowed)
│  delegate ───► abstract_item_delegate*  │  ← Optional custom delegate
│  selection ──► item_selection_model*    │  ← Optional custom selection
└─────────────────────────────────────────┘
```

---

## Simple Widgets

### combo_box

A dropdown selection widget with a simple string-based API.

```cpp
#include <onyxui/widgets/input/combo_box.hh>

// Create and populate
auto combo = std::make_unique<combo_box<Backend>>();
combo->add_item("Small");
combo->add_item("Medium");
combo->add_item("Large");

// Or set all items at once
combo->set_items({"Red", "Green", "Blue"});

// Selection
combo->set_current_index(1);        // Select "Medium" (or "Green")
combo->set_current_text("Large");   // Select by text

// Read selection
int idx = combo->current_index();      // 1
std::string text = combo->current_text();  // "Medium"

// Signals
combo->current_index_changed.connect([](int index) {
    std::cout << "Selected index: " << index << "\n";
});

combo->current_text_changed.connect([](const std::string& text) {
    std::cout << "Selected: " << text << "\n";
});
```

#### combo_box API Reference

| Method | Description |
|--------|-------------|
| `add_item(text)` | Add item to end |
| `insert_item(index, text)` | Insert at position |
| `remove_item(index)` | Remove at position |
| `clear()` | Remove all items |
| `set_items(vector)` | Replace all items |
| `count()` | Get item count |
| `item_text(index)` | Get text at index |
| `set_item_text(index, text)` | Modify text |
| `find_text(text)` | Search for text, returns index or -1 |
| `current_index()` | Get selected index (-1 if none) |
| `set_current_index(index)` | Set selection |
| `current_text()` | Get selected text |
| `set_current_text(text)` | Select by text |
| `is_popup_open()` | Check if dropdown is open |
| `open_popup()` | Open dropdown |
| `close_popup()` | Close dropdown |

### list_box

A vertical list widget with optional multi-selection.

```cpp
#include <onyxui/widgets/input/list_box.hh>

// Create and populate
auto list = std::make_unique<list_box<Backend>>();
list->set_items({"Apple", "Banana", "Cherry", "Date"});

// Single selection (default)
list->set_current_index(1);  // Select "Banana"

// Multi-selection
list->set_selection_mode(selection_mode::multi_selection);
list->select(0);
list->select(2);
auto selected = list->selected_indices();  // {0, 2}

// Scrolling
list->scroll_to(10);  // Scroll to make item 10 visible

// Signals
list->current_index_changed.connect([](int index) {
    std::cout << "Current: " << index << "\n";
});

list->selection_changed.connect([]() {
    std::cout << "Selection changed\n";
});

list->activated.connect([](int index) {
    std::cout << "Activated (double-click): " << index << "\n";
});
```

#### list_box API Reference

| Method | Description |
|--------|-------------|
| *Item management* | Same as combo_box |
| `current_index()` | Get current/focused index |
| `set_current_index(index)` | Set current item |
| `is_selected(index)` | Check if item is selected |
| `selected_indices()` | Get all selected indices |
| `select(index)` | Add to selection |
| `deselect(index)` | Remove from selection |
| `clear_selection()` | Deselect all |
| `select_all()` | Select all items |
| `set_selection_mode(mode)` | Set single/multi selection |
| `get_selection_mode()` | Get current mode |
| `scroll_to(index)` | Scroll to make item visible |
| `scroll_offset()` | Get current scroll position |
| `set_scroll_offset(offset)` | Set scroll position |

---

## Advanced *View Widgets

### combo_box_view

Full MVC combo box with external model support.

```cpp
#include <onyxui/mvc/views/combo_box_view.hh>
#include <onyxui/mvc/models/list_model.hh>

// Create model (you own this)
auto model = std::make_shared<list_model<std::string, Backend>>();
model->set_items({"Option A", "Option B", "Option C"});

// Create view
auto combo = std::make_unique<combo_box_view<Backend>>();
combo->set_model(model.get());

// Optional: custom delegate for rendering
combo->set_delegate(std::make_shared<my_custom_delegate<Backend>>());

// Selection via model_index
combo->set_current_index(model->index(1, 0));
model_index current = combo->current_index();

// Or via simple row API
combo->set_current_row(2);
int row = combo->current_row();

// Signals use model_index
combo->current_changed.connect([](const model_index& idx) {
    std::cout << "Selected row: " << idx.row << "\n";
});

combo->activated.connect([](const model_index& idx) {
    std::cout << "Activated row: " << idx.row << "\n";
});
```

### list_view

Full MVC list view with virtual scrolling.

```cpp
#include <onyxui/mvc/views/list_view.hh>
#include <onyxui/mvc/models/list_model.hh>

// Create model
auto model = std::make_shared<list_model<std::string, Backend>>();
model->set_items(large_string_vector);  // Can handle 10,000+ items

// Create view
auto list = std::make_unique<list_view<Backend>>();
list->set_model(model.get());

// Optional: custom selection model (shared across views)
auto selection = std::make_shared<item_selection_model<Backend>>();
selection->set_selection_mode(selection_mode::extended_selection);
list->set_selection_model(selection);

// Signals
list->clicked.connect([](const model_index& idx) {
    std::cout << "Clicked: " << idx.row << "\n";
});

list->double_clicked.connect([](const model_index& idx) {
    std::cout << "Double-clicked: " << idx.row << "\n";
});

list->activated.connect([](const model_index& idx) {
    std::cout << "Activated: " << idx.row << "\n";
});
```

---

## Core MVC Components

### model_index

A lightweight reference to a location in a model.

```cpp
struct model_index {
    int row = -1;
    int column = 0;
    void* internal_ptr = nullptr;  // For tree models

    bool is_valid() const noexcept { return row >= 0; }
};

// Usage
model_index idx = model->index(5, 0);  // Row 5, column 0
if (idx.is_valid()) {
    auto data = model->data(idx, item_data_role::display);
}
```

### abstract_item_model

Base class for all models. Provides data to views.

```cpp
template<UIBackend Backend>
class abstract_item_model {
public:
    // Required overrides
    virtual int row_count(const model_index& parent = {}) const = 0;
    virtual int column_count(const model_index& parent = {}) const = 0;
    virtual item_data data(const model_index& index, item_data_role role) const = 0;
    virtual model_index index(int row, int column, const model_index& parent = {}) const = 0;

    // Optional overrides
    virtual bool set_data(const model_index& index, const std::any& value, item_data_role role);
    virtual item_flags flags(const model_index& index) const;

    // Signals (emit when data changes)
    signal<const model_index&, const model_index&> data_changed;
    signal<const model_index&, int, int> rows_inserted;
    signal<const model_index&, int, int> rows_removed;
    signal<> model_reset;
};
```

### list_model

A simple list model for string data.

```cpp
template<typename T, UIBackend Backend>
class list_model : public abstract_item_model<Backend> {
public:
    void append(const T& item);
    void insert(int row, const T& item);
    void remove(int row);
    void clear();
    void set_items(std::vector<T> items);

    const T& item_at(int row) const;
    void set_item(int row, const T& item);
};

// Usage
auto model = std::make_shared<list_model<std::string, Backend>>();
model->append("First");
model->append("Second");
model->insert(1, "Inserted");  // Now: First, Inserted, Second
model->remove(0);              // Now: Inserted, Second
```

### item_data_role

Specifies what type of data is being requested.

```cpp
enum class item_data_role {
    display,        // Text to display
    edit,           // Text for editing
    decoration,     // Icon or image
    tooltip,        // Tooltip text
    status_tip,     // Status bar text
    background,     // Background color
    foreground,     // Text color
    font,           // Font override
    text_alignment, // Alignment
    check_state,    // Checkbox state
    user_role = 256 // Custom roles start here
};
```

---

## Selection Management

### item_selection_model

Manages selection state independently from the view. Selection models are
shared via `shared_ptr`, allowing multiple views to share selection state.

```cpp
template<UIBackend Backend>
class item_selection_model {
public:
    // Selection mode
    void set_selection_mode(selection_mode mode);
    selection_mode get_selection_mode() const;

    // Current item (focus)
    void set_current_index(const model_index& index);
    model_index current_index() const;

    // Selection operations
    void select(const model_index& index, selection_flags flags = selection_flag::select);
    void deselect(const model_index& index);
    void clear_selection();
    void toggle(const model_index& index);

    // Query
    bool is_selected(const model_index& index) const;
    std::vector<model_index> selected_indices() const;

    // Signals
    signal<const model_index&, const model_index&> current_changed;
    signal<const std::vector<model_index>&, const std::vector<model_index>&> selection_changed;
};
```

### selection_mode

```cpp
enum class selection_mode {
    no_selection,         // No selection allowed
    single_selection,     // Only one item at a time
    multi_selection,      // Multiple items, toggle on click
    extended_selection,   // Multiple with Shift/Ctrl modifiers
    contiguous_selection  // Multiple contiguous items only
};
```

### selection_flag

Flags for selection operations.

```cpp
enum class selection_flag {
    clear   = 1 << 0,  // Clear existing selection first
    select  = 1 << 1,  // Select the item
    deselect = 1 << 2, // Deselect the item
    toggle  = 1 << 3,  // Toggle selection state
    current = 1 << 4   // Set as current item
};

// Usage
selection->select(idx, selection_flag::clear | selection_flag::select);  // Clear and select
selection->select(idx, selection_flag::toggle);  // Toggle
```

---

## Delegates

Delegates control how items are rendered.

### Coordinate System Contract

**IMPORTANT**: Delegates work in **backend physical units**, not logical units.

- `paint()` receives coordinates in physical units (pixels for SDL, characters for conio)
- `size_hint()` returns sizes in physical units
- The view handles logical↔physical conversion before calling delegates
- This design avoids conversion overhead in the hot rendering path

**Rationale**:
1. Delegates call renderer methods directly (`draw_text`, `fill_rect`) which work in physical coordinates
2. Text measurement (`renderer::measure_text`) returns physical dimensions
3. Font sizes and padding are specified in physical units

**For view implementers**: Do NOT scale delegate sizes by DPI factors - they are already in the correct coordinate space.

### abstract_item_delegate

```cpp
template<UIBackend Backend>
class abstract_item_delegate {
public:
    using rect_type = typename Backend::rect_type;   // Physical coordinates
    using size_type = typename Backend::size_type;   // Physical dimensions

    // Render an item at physical coordinates
    virtual void paint(
        render_context<Backend>& ctx,
        const model_index& index,
        const rect_type& bounds,      // Physical bounds (already converted)
        bool is_selected,
        bool has_focus
    ) const = 0;

    // Calculate item size in physical units
    virtual size_type size_hint(
        const model_index& index
    ) const = 0;
};
```

### default_item_delegate

The built-in delegate that renders text with theme colors.

```cpp
// Automatically used if no delegate is set
// Renders:
// - Selection background (from theme)
// - Text (from model's display role)
// - Focus rectangle (when focused)
```

### Custom Delegate Example

```cpp
template<UIBackend Backend>
class icon_delegate : public abstract_item_delegate<Backend> {
public:
    using rect_type = typename Backend::rect_type;
    using size_type = typename Backend::size_type;

    void paint(
        render_context<Backend>& ctx,
        const model_index& index,
        const rect_type& bounds,      // Physical coordinates
        bool is_selected,
        bool has_focus
    ) const override {
        // Draw background (bounds already in physical units)
        if (is_selected) {
            ctx.fill_rect(bounds, get_selection_color());
        }

        // Draw icon
        auto icon_data = model->data(index, item_data_role::decoration);
        if (auto* icon = std::any_cast<icon_type>(&icon_data)) {
            ctx.draw_icon(*icon, {bounds.x + 2, bounds.y + 2});
        }

        // Draw text
        auto text = std::get<std::string>(model->data(index, item_data_role::display));
        ctx.draw_text(text, {bounds.x + 20, bounds.y + 2}, font, fg_color);

        // Draw focus
        if (has_focus) {
            ctx.draw_focus_rect(bounds);
        }
    }

    size_type size_hint(const model_index& index) const override {
        // Return physical dimensions (pixels for SDL, characters for conio)
        return {200, 24};  // 200px wide, 24px tall
    }
};

// Usage
view->set_delegate(std::make_shared<icon_delegate<Backend>>());
```

---

## Examples

### Example 1: Simple File List

```cpp
// Simple usage - no MVC knowledge needed
auto file_list = std::make_unique<list_box<Backend>>();
file_list->set_items({"document.txt", "image.png", "data.csv"});

file_list->activated.connect([](int index) {
    open_file(index);
});
```

### Example 2: Database-Backed Combo Box

```cpp
// Custom model for database data
class database_model : public abstract_item_model<Backend> {
    // Fetch data from database on demand
    item_data data(const model_index& idx, item_data_role role) const override {
        if (role == item_data_role::display) {
            return m_db.query_cell(idx.row, idx.column);
        }
        return {};
    }
};

// Use with combo_box_view
auto model = std::make_shared<database_model>(connection);
auto combo = std::make_unique<combo_box_view<Backend>>();
combo->set_model(model.get());
```

### Example 3: Multi-Selection with Custom Rendering

```cpp
// Create model and view
auto model = std::make_shared<list_model<std::string, Backend>>();
auto list = std::make_unique<list_view<Backend>>();
list->set_model(model.get());

// Enable multi-selection (shared_ptr allows sharing across views)
auto selection = std::make_shared<item_selection_model<Backend>>();
selection->set_selection_mode(selection_mode::extended_selection);
list->set_selection_model(selection);

// Custom delegate with checkboxes
list->set_delegate(std::make_shared<checkbox_delegate<Backend>>());

// Handle selection
selection->selection_changed.connect(
    [](const std::vector<model_index>& selected,
       const std::vector<model_index>& deselected) {
        std::cout << selected.size() << " items now selected\n";
    }
);
```

---

## Migration Guide

### From Old combo_box (pre-Phase 6)

If you were using the old `combo_box` that required an external model:

```cpp
// OLD (pre-Phase 6)
auto model = std::make_shared<list_model<Backend>>();
model->set_items(items);
auto combo = std::make_unique<combo_box<Backend>>();
combo->set_model(model.get());

// NEW (Phase 6+)
auto combo = std::make_unique<combo_box<Backend>>();
combo->set_items(items);  // Model is internal now
```

### Using Advanced Features with Simple Widgets

Simple widgets provide access to their internal components:

```cpp
auto combo = std::make_unique<combo_box<Backend>>();
combo->set_items({"A", "B", "C"});

// Access internal view for advanced configuration
auto* view = combo->view();
view->set_delegate(my_custom_delegate);

// Access internal model (read-only)
const auto* model = combo->model();
int count = model->row_count();
```

---

## File Structure

```
include/onyxui/
├── mvc/
│   ├── models/
│   │   ├── abstract_item_model.hh   # Base model interface
│   │   └── list_model.hh            # Simple list model
│   ├── views/
│   │   ├── abstract_item_view.hh    # Base view interface
│   │   ├── list_view.hh             # MVC list view
│   │   └── combo_box_view.hh        # MVC combo box view
│   ├── delegates/
│   │   ├── abstract_item_delegate.hh # Base delegate interface
│   │   └── default_item_delegate.hh  # Default text rendering
│   ├── selection/
│   │   └── item_selection_model.hh   # Selection management
│   ├── model_index.hh
│   └── item_data_role.hh
└── widgets/
    └── input/
        ├── combo_box.hh              # Simple combo box
        └── list_box.hh               # Simple list box
```

---

## See Also

- [Architecture Guide](ARCHITECTURE.md) - Core framework patterns
- [Theming Guide](THEMING.md) - Theme integration for MVC widgets
- [Testing Guide](TESTING.md) - Writing tests for MVC components
- [Scrolling Guide](../scrolling_guide.md) - Virtual scrolling in list_view
