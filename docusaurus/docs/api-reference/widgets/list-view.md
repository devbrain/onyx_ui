---
sidebar_position: 10
---

# list_view

**Category**: MVC View Widget
**MVC Component**: View
**Header**: `<onyxui/mvc/views/list_view.hh>`
**Since**: v2025.11 (Phase 1)

A scrollable list view widget that displays items from a model using the MVC (Model-View-Controller) pattern with full selection support.

## Overview

`list_view` provides:
- MVC model integration for data binding
- Multiple selection modes (single, multi, extended, contiguous)
- Keyboard and mouse navigation
- Custom item rendering via delegates
- Automatic updates when model changes
- Integration with scrolling system

## Basic Usage

```cpp
#include <onyxui/mvc/views/list_view.hh>
#include <onyxui/mvc/models/list_model.hh>

// Create model with data
auto model = std::make_shared<list_model<std::string, Backend>>();
model->set_items({"Apple", "Banana", "Cherry", "Date"});

// Create view
auto view = std::make_unique<list_view<Backend>>();
view->set_model(model.get());

// Handle selection
view->clicked.connect([](const model_index& index) {
    std::cout << "Clicked row " << index.row << "\n";
});

root->add_child(std::move(view));
```

## Template Parameters

- **`Backend`**: UI backend type (must satisfy `UIBackend` concept)

## Public API

### Construction

```cpp
explicit list_view(ui_element<Backend>* parent = nullptr)
```
Constructs an empty list view.

### Model Management

```cpp
void set_model(model_type* model)
```
Set the data model (not owned by list view).

```cpp
[[nodiscard]] model_type* model() const noexcept
```
Get the current model pointer.

### Delegate Management

```cpp
void set_delegate(std::shared_ptr<delegate_type> delegate)
```
Set custom item delegate for rendering.

```cpp
[[nodiscard]] delegate_type* delegate() const noexcept
```
Get the current delegate pointer.

### Selection

```cpp
void set_selection_mode(selection_mode mode)
```
Set the selection mode.

**Selection Modes**:
- `selection_mode::single_selection` - Only one item (default)
- `selection_mode::multi_selection` - Multiple items (Ctrl+click)
- `selection_mode::extended_selection` - Range selection (Shift+click)
- `selection_mode::contiguous_selection` - Only contiguous ranges
- `selection_mode::no_selection` - Selection disabled

```cpp
[[nodiscard]] selection_model_type* selection_model() const noexcept
```
Get the selection model for advanced selection control.

```cpp
[[nodiscard]] model_index current_index() const noexcept
```
Get the current item (keyboard focus).

```cpp
void set_current_index(const model_index& index)
```
Set the current item (keyboard focus).

## Signals

### clicked

```cpp
signal<const model_index&> clicked
```

Emitted when an item is clicked with the mouse.

**Example**:
```cpp
view->clicked.connect([](const model_index& index) {
    std::cout << "Clicked row " << index.row << "\n";
});
```

### double_clicked

```cpp
signal<const model_index&> double_clicked
```

Emitted when an item is double-clicked.

### activated

```cpp
signal<const model_index&> activated
```

Emitted when an item is activated (Enter key or double-click).

**Example**:
```cpp
view->activated.connect([&](const model_index& index) {
    auto* model = view->model();
    auto data = model->data(index, item_data_role::display);
    std::string text = std::get<std::string>(data);
    open_item(text);
});
```

## Keyboard Navigation

| Key | Action |
|-----|--------|
| **↓** (Down Arrow) | Move current item down |
| **↑** (Up Arrow) | Move current item up |
| **Home** | Move to first item |
| **End** | Move to last item |
| **Page Down** | Move down one page |
| **Page Up** | Move up one page |
| **Enter** | Activate current item |
| **Space** | Toggle selection (multi-selection mode) |

## Examples

### Simple String List

```cpp
auto model = std::make_shared<list_model<std::string, Backend>>();
model->set_items({"Apple", "Banana", "Cherry", "Date"});

auto view = std::make_unique<list_view<Backend>>();
view->set_model(model.get());

root->add_child(std::move(view));
```

### Multi-Selection

```cpp
auto view = std::make_unique<list_view<Backend>>();
view->set_model(model.get());

// Enable multi-selection
view->set_selection_mode(selection_mode::multi_selection);

// Handle selection changes
view->selection_model()->selection_changed.connect([&view]() {
    auto& selected = view->selection_model()->selected_indices();
    std::cout << "Selected " << selected.size() << " items\n";
});
```

### Custom Item Type

```cpp
struct Task {
    std::string title;
    bool completed;
    int priority;
};

// Specialize to_string for Task
template<>
std::string list_model<Task, Backend>::to_string(const Task& t) {
    std::string status = t.completed ? "[✓]" : "[ ]";
    return status + " " + t.title + " (Priority: " + std::to_string(t.priority) + ")";
}

// Use Task in list_view
auto model = std::make_shared<list_model<Task, Backend>>();
model->set_items({
    {"Write documentation", false, 1},
    {"Fix bug #123", true, 2},
    {"Implement feature", false, 1}
});

auto view = std::make_unique<list_view<Backend>>();
view->set_model(model.get());
```

### Scrollable List (Large Dataset)

```cpp
auto model = std::make_shared<list_model<std::string, Backend>>();
std::vector<std::string> large_dataset;
for (int i = 0; i < 10000; ++i) {
    large_dataset.push_back("Item " + std::to_string(i));
}
model->set_items(large_dataset);

auto view = std::make_unique<list_view<Backend>>();
view->set_model(model.get());

// Wrap in scroll_view for automatic scrolling
auto scroll = modern_scroll_view<Backend>();
scroll->set_content(std::move(view));

root->add_child(std::move(scroll));
```

### Custom Delegate

```cpp
// Custom delegate for colorful alternating rows
template<UIBackend Backend>
class striped_delegate : public abstract_item_delegate<Backend> {
public:
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
            bg = {0, 120, 215};  // Blue selection
        }

        ctx.draw_rect(rect, bg);

        // Draw text
        auto* model = static_cast<const abstract_item_model<Backend>*>(index.model);
        auto text_data = model->data(index, item_data_role::display);
        std::string text = std::get<std::string>(text_data);

        color_type fg = is_selected ? color_type{255, 255, 255} : color_type{0, 0, 0};
        point_type text_pos{rect.x + 4, rect.y + 2};
        ctx.draw_text(text, text_pos, fg);
    }

    size_type size_hint(const model_index& index) const override {
        return {200, 24};
    }
};

// Use custom delegate
view->set_delegate(std::make_shared<striped_delegate<Backend>>());
```

### Programmatic Selection

```cpp
auto* sel = view->selection_model();

// Select specific items
sel->select(model->index(0, 0));
sel->select(model->index(2, 0));
sel->select(model->index(4, 0));

// Select range
sel->select_range(model->index(0, 0), model->index(5, 0));

// Clear selection
sel->clear_selection();

// Toggle item
sel->toggle(model->index(3, 0));
```

## MVC Integration

### Model Changes Auto-Update View

```cpp
auto model = std::make_shared<list_model<std::string, Backend>>();
model->set_items({"A", "B", "C"});

auto view = std::make_unique<list_view<Backend>>();
view->set_model(model.get());

// View displays: A, B, C

model->append("D");
// View automatically updates to: A, B, C, D

model->insert(1, "X");
// View automatically updates to: A, X, B, C, D

model->remove(2);
// View automatically updates to: A, X, C, D
```

### Multiple Views on Same Model

```cpp
auto model = std::make_shared<list_model<std::string, Backend>>();
model->set_items({"Item 1", "Item 2", "Item 3"});

// Create two views of the same model
auto view1 = std::make_unique<list_view<Backend>>();
view1->set_model(model.get());

auto view2 = std::make_unique<list_view<Backend>>();
view2->set_model(model.get());

// Both views update when model changes!
model->append("Item 4");
```

## Theming

`list_view` uses the default item delegate for rendering, which respects theme colors:

```yaml
# Default colors from theme
list_view:
  background: { r: 255, g: 255, b: 255 }  # White
  foreground: { r: 0, g: 0, b: 0 }        # Black
  selection_background: { r: 0, g: 120, b: 215 }  # Blue
  selection_foreground: { r: 255, g: 255, b: 255 }  # White
```

For custom theming, use a custom delegate.

## Implementation Details

### Inheritance Hierarchy

```
ui_element<Backend>
    └── widget<Backend>
            └── abstract_item_view<Backend>
                    └── list_view<Backend>
```

### Member Components

- **Model**: `abstract_item_model<Backend>*` (not owned)
- **Delegate**: `std::shared_ptr<abstract_item_delegate<Backend>>`
- **Selection**: `std::unique_ptr<item_selection_model<Backend>>`

### Automatic Updates

The view connects to model signals:
- `data_changed` → Repaint affected items
- `rows_inserted` → Relayout and repaint
- `rows_removed` → Relayout and repaint
- `layout_changed` → Full relayout and repaint

## Performance Considerations

### Virtual Scrolling (Future)

For datasets with 100k+ items, Phase 3 will add virtual scrolling:
- Only render visible items
- Constant rendering time regardless of dataset size
- Query parent `scrollable` for scroll position

### Current Limitations

- Renders all items (suitable for <10k items)
- Full relayout on model changes
- No item caching

## Related

- **[MVC System Guide](../../guides/mvc-system.md)** - Complete MVC documentation
- **[combo_box](./combo-box.md)** - Dropdown widget using list_view
- **[scroll_view](./scroll-view.md)** - Scrolling container for large lists
- **[list_model](../../guides/mvc-system.md#1-models)** - Data model for list_view

## See Also

- **Header**: `include/onyxui/mvc/views/list_view.hh`
- **Base**: `include/onyxui/mvc/views/abstract_item_view.hh`
- **Tests**: `unittest/mvc/test_mvc_basic.cc`
- **Demo**: `examples/demo_ui_builder.hh` (Tab Widget → "MVC Demo" tab)
