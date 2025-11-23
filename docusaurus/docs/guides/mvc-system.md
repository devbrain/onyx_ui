---
sidebar_position: 6
---

# MVC System

**Status**: Phase 2 Complete ✅
**Version**: v1.0
**Since**: 2025-11-22

OnyxUI's Model-View-Controller (MVC) system provides Qt-inspired separation of data and presentation, enabling:

- **Separation of Concerns**: Models store data, views display it, delegates render items
- **Automatic Updates**: Views auto-refresh when models emit change signals
- **Reusability**: Same model can drive multiple views
- **Customization**: Delegates allow custom rendering without subclassing views

## Quick Example

```cpp
// Create model with data
auto model = std::make_shared<list_model<std::string, Backend>>();
model->set_items({"Apple", "Banana", "Cherry", "Date"});

// Create view
auto view = std::make_unique<list_view<Backend>>();
view->set_model(model.get());

// Add to UI
root->add_child(std::move(view));

// Later: Add more items (view auto-updates!)
model->append("Elderberry");
```

## Architecture Overview

The MVC system consists of four main components:

```
┌─────────────┐      ┌─────────────┐      ┌─────────────┐
│   MODEL     │      │    VIEW     │      │  DELEGATE   │
│             │      │             │      │             │
│ Data storage│─────▶│ Display data│─────▶│ Render items│
│ Change      │      │ Handle      │      │ Size hints  │
│ signals     │      │ selection   │      │             │
└─────────────┘      └─────────────┘      └─────────────┘
      │                      │
      │                      ▼
      │              ┌─────────────┐
      └─────────────▶│  SELECTION  │
                     │   MODEL     │
                     │             │
                     │ Track       │
                     │ selected    │
                     │ items       │
                     └─────────────┘
```

### Model Index

Uniquely identifies an item in the model:

```cpp
struct model_index {
    int row;           // Row number (0-based)
    int column;        // Column number (0-based, 0 for lists)
    void* internal_id; // Model-specific identifier
    const void* model; // Pointer to owning model

    [[nodiscard]] bool is_valid() const noexcept;
};
```

### Data Roles

Different aspects of the same item:

```cpp
enum class item_data_role : uint8_t {
    display,      // Text to display
    edit,         // Data for editing
    decoration,   // Icon or image
    tooltip,      // Tooltip text
    background,   // Background color
    foreground,   // Text color
    user_role     // Custom application data
};
```

## Core Components

### 1. Models

**`abstract_item_model<Backend>`** - Base interface for all models

**`list_model<T, Backend>`** - Concrete list model

```cpp
// Create and populate
auto model = std::make_shared<list_model<std::string, Backend>>();
model->set_items({"Item 1", "Item 2", "Item 3"});

// Modify
model->append("Item 4");
model->insert(1, "New Item");
model->remove(2);
model->clear();

// Access
int count = model->row_count();
std::string item = model->at(0);
```

**Key Features**:
- Automatic signal emission on changes
- Template-based (works with any type)
- Convert types to string for display via `to_string()`

### 2. Views

**`abstract_item_view<Backend>`** - Base class for all views

**`list_view<Backend>`** - Scrollable list view

```cpp
auto view = std::make_unique<list_view<Backend>>();
view->set_model(model.get());

// Selection
view->set_selection_mode(selection_mode::single);
view->clicked.connect([](const model_index& index) {
    std::cout << "Clicked row " << index.row << "\n";
});

// Current item (keyboard focus)
view->set_current_index(model->index(0, 0));
```

**Selection Modes**:
- `single_selection` - Only one item selected
- `multi_selection` - Multiple items (Ctrl+click)
- `extended_selection` - Range selection (Shift+click)
- `contiguous_selection` - Only contiguous ranges
- `no_selection` - Selection disabled

### 3. Delegates

**`abstract_item_delegate<Backend>`** - Base interface

**`default_item_delegate<Backend>`** - Default rendering

```cpp
// Custom delegate for colorful items
template<UIBackend Backend>
class colorful_delegate : public abstract_item_delegate<Backend> {
    void paint(render_context<Backend>& ctx,
               const model_index& index,
               const rect_type& rect,
               bool is_selected,
               bool has_focus) const override {
        // Alternate row colors
        color_type bg = (index.row % 2 == 0)
            ? color_type{240, 240, 240}  // Light gray
            : color_type{255, 255, 255}; // White

        if (is_selected) {
            bg = {255, 200, 100};  // Orange selection
        }

        ctx.draw_rect(rect, bg);

        // Draw text...
    }
};

// Use custom delegate
view->set_delegate(std::make_shared<colorful_delegate<Backend>>());
```

### 4. Selection Model

**`item_selection_model<Backend>`** - Tracks selection state

```cpp
auto* sel = view->selection_model();

// Programmatic selection
sel->select(model->index(2, 0));
sel->deselect(model->index(1, 0));
sel->toggle(model->index(0, 0));
sel->clear_selection();

// Query selection
bool selected = sel->is_selected(index);
auto selected_indices = sel->selected_indices();

// Signals
sel->current_changed.connect([](const model_index& current,
                                 const model_index& previous) {
    // Current item changed
});

sel->selection_changed.connect([]() {
    // Selection set changed
});
```

## MVC Widgets

### combo_box

Dropdown selection widget using MVC architecture:

```cpp
auto model = std::make_shared<list_model<std::string, Backend>>();
model->set_items({"Small", "Medium", "Large", "X-Large"});

auto combo = std::make_unique<combo_box<Backend>>();
combo->set_model(model.get());
combo->set_current_index(1);  // Select "Medium"

// Handle selection changes
combo->current_index_changed.connect([](int index) {
    std::cout << "Selected index: " << index << "\n";
});
```

**Status**: Phase 2 complete
- ✅ Keyboard navigation (arrow keys, Home/End)
- ✅ Selection signals
- ✅ State-based theming
- ⚠️ Popup rendering TODO (requires layer_manager integration)

See [combo_box API Reference](../api-reference/widgets/combo-box.md) for details.

### list_view

Scrollable list view with selection support:

```cpp
auto view = std::make_unique<list_view<Backend>>();
view->set_model(model.get());
view->set_selection_mode(selection_mode::multi);

// Handle clicks
view->clicked.connect([](const model_index& index) {
    std::cout << "Clicked: " << index.row << "\n";
});

// Handle activation (double-click or Enter)
view->activated.connect([](const model_index& index) {
    std::cout << "Activated: " << index.row << "\n";
});
```

See [list_view API Reference](../api-reference/widgets/list-view.md) for details.

## Data Flow

### Model Updates View

```
User modifies model
       │
       ▼
model->append("New Item")
       │
       ├─── begin_insert_rows()
       │         │
       │         ▼
       │    rows_about_to_be_inserted signal
       │         │
       │         ▼
       │    View prepares for change
       │
       ├─── m_items.push_back()
       │
       ├─── end_insert_rows()
       │
       └─── rows_inserted signal
                 │
                 ▼
            View::on_rows_inserted()
                 │
                 ├─── invalidate_layout()
                 └─── mark_dirty()
                       │
                       ▼
                 View repaints automatically
```

### User Clicks Item

```
Mouse click (150, 75)
       │
       ▼
View::handle_event(mouse_press)
       │
       ├─── index = index_at(150, 75)
       │         │
       │         └─── Returns model_index{row=3, col=0}
       │
       ├─── selection_model->set_current_index(index)
       │         │
       │         ├─── Clear previous selection (if single mode)
       │         ├─── Add to selected set
       │         └─── selection_changed.emit()
       │
       ├─── clicked.emit(index)
       │
       └─── mark_dirty()  // Repaint with new selection
```

## Advanced Usage

### Multiple Views on Same Model

```cpp
// Create shared model
auto model = std::make_shared<list_model<Person, Backend>>();
model->set_items({
    {"Alice", 30, "alice@example.com"},
    {"Bob", 25, "bob@example.com"}
});

// Create TWO views of the same model
auto list1 = std::make_unique<list_view<Backend>>();
list1->set_model(model.get());

auto list2 = std::make_unique<list_view<Backend>>();
list2->set_model(model.get());

// Both views update when model changes!
model->append({"Charlie", 35, "charlie@example.com"});
```

### Custom Struct Types

```cpp
struct Person {
    std::string name;
    int age;
    std::string email;
};

// Specialize to_string for Person
template<>
std::string list_model<Person, Backend>::to_string(const Person& p) {
    return p.name + " (" + std::to_string(p.age) + ")";
}

// Now you can use Person in list_model
auto model = std::make_shared<list_model<Person, Backend>>();
```

### Scrolling Integration

For large datasets, wrap views in `scroll_view`:

```cpp
auto model = std::make_shared<list_model<std::string, Backend>>();
model->set_items(large_dataset);  // 10,000 items

auto view = std::make_unique<list_view<Backend>>();
view->set_model(model.get());

// Wrap in scroll_view for automatic scrolling
auto scroll = modern_scroll_view<Backend>();
scroll->set_content(std::move(view));

root->add_child(std::move(scroll));
```

## Implementation Status

### ✅ Phase 1: Core Infrastructure (Complete)
- **Date**: 2025-11-22
- **Tests**: 108 test cases, all passing
- **Components**:
  - ✅ `model_index` - Index structure
  - ✅ `item_data_role` - Data role enum
  - ✅ `abstract_item_model<Backend>` - Base model interface
  - ✅ `list_model<T, Backend>` - Concrete list model
  - ✅ `abstract_item_view<Backend>` - Base view class
  - ✅ `list_view<Backend>` - List view implementation
  - ✅ `abstract_item_delegate<Backend>` - Delegate interface
  - ✅ `default_item_delegate<Backend>` - Default item rendering
  - ✅ `item_selection_model<Backend>` - Selection tracking

### ✅ Phase 2: combo_box Widget (Complete)
- **Date**: 2025-11-23
- **Tests**: 7 test cases, all passing
- **Components**:
  - ✅ `combo_box<Backend>` - MVC-based dropdown widget
  - ✅ Keyboard navigation
  - ✅ Selection signals
  - ✅ State-based theming
  - ⚠️ Popup rendering TODO (requires layer_manager)

### 📋 Future Phases
- Phase 3: Scrollable Integration + Virtual Scrolling
- Phase 4: Custom Delegates
- Phase 5: Pagination (for very large datasets)
- Phase 6: Table View (multi-column support)
- Phase 7: Sorting and Filtering
- Phase 8: Tree View (hierarchical data)

## API Files

- **Models**: `include/onyxui/mvc/models/`
  - `abstract_item_model.hh`
  - `list_model.hh`
- **Views**: `include/onyxui/mvc/views/`
  - `abstract_item_view.hh`
  - `list_view.hh`
- **Widgets**: `include/onyxui/widgets/input/`
  - `combo_box.hh`
- **Delegates**: `include/onyxui/mvc/delegates/`
  - `abstract_item_delegate.hh`
  - `default_item_delegate.hh`
- **Selection**: `include/onyxui/mvc/selection/`
  - `item_selection_model.hh`
  - `selection_mode.hh`
- **Core**: `include/onyxui/mvc/`
  - `model_index.hh`
  - `item_data_role.hh`

## Design Documentation

For comprehensive design documentation including all future phases, see:
- **[MVC_DESIGN.md](https://github.com/anthropics/onyxui/blob/master/docs/MVC_DESIGN.md)** - Complete design specification

## Testing

**Total Test Coverage**: 1552 tests, 8828 assertions, all passing

**MVC-specific tests**:
- `unittest/mvc/test_mvc_basic.cc` - 108 comprehensive tests
- `unittest/widgets/test_combo_box.cc` - 7 combo_box tests

Run tests:
```bash
./build/bin/ui_unittest --test-case="*mvc*"
./build/bin/ui_unittest --test-case="*combo_box*"
```

## Next Steps

1. **Use in your application**: The MVC system is production-ready for keyboard-driven UIs
2. **Explore examples**: Check `examples/demo_ui_builder.hh` for combo_box and list_view usage
3. **Customize delegates**: Create custom item rendering for your use case
4. **Large datasets**: Wrap views in `scroll_view` for 10k+ items
5. **Phase 3+**: Watch for virtual scrolling and pagination support

## See Also

- [combo_box Widget](../api-reference/widgets/combo-box.md)
- [list_view Widget](../api-reference/widgets/list-view.md)
- [Widget Development Guide](./widget-development.md)
- [Scrolling System](./scrolling-system.md)
