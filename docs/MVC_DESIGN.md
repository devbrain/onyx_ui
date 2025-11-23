# OnyxUI MVC System Design

**Status**: Phase 2 Complete ✅
**Version**: v1.0
**Created**: 2025-11-18
**Last Updated**: 2025-11-23
**Author**: Architecture Team

## Implementation Status

### ✅ Phase 1: Core Infrastructure (COMPLETE)
- **Commits**: 3eeb378, a8f77ec, 7e6adf8
- **Date**: 2025-11-22
- **Tests**: 108 test cases, all passing
- **Components Implemented**:
  - ✅ `model_index` - Index structure
  - ✅ `item_data_role` - Data role enum
  - ✅ `abstract_item_model<Backend>` - Base model interface
  - ✅ `list_model<T, Backend>` - Concrete list model
  - ✅ `abstract_item_view<Backend>` - Base view class
  - ✅ `list_view<Backend>` - List view implementation
  - ✅ `abstract_item_delegate<Backend>` - Delegate interface
  - ✅ `default_item_delegate<Backend>` - Default item rendering
  - ✅ `item_selection_model<Backend>` - Selection tracking
  - ✅ Signal connections for automatic view updates
  - ✅ Demo integration in widgets_demo

### ✅ Phase 2: combo_box Widget (COMPLETE)
- **Commits**: 67c9891, 6751c71
- **Date**: 2025-11-23
- **Tests**: 7 test cases, all passing
- **Components Implemented**:
  - ✅ `combo_box<Backend>` - MVC-based dropdown widget
  - ✅ Keyboard navigation (arrow keys, Home/End)
  - ✅ Selection signals (current_index_changed)
  - ✅ State-based theming (hover/pressed/disabled)
  - ✅ Basic rendering (button + dropdown arrow "▼")
  - ✅ Model integration (set_model, get current selection)
  - ✅ Demo integration with size selection example

**Limitations**:
- ⚠️ Popup rendering TODO (requires layer_manager integration)
- ⚠️ Mouse selection TODO (no visual dropdown list yet)
- ✅ Keyboard functionality fully working

**Total Test Coverage**: 1552 tests, 8828 assertions, all passing

### 🔧 In Progress
- None (Phase 2 complete)

### 📋 Next Phases
- Phase 3: Scrollable Integration + Virtual Scrolling
- Phase 4: Custom Delegates
- Phase 5: Pagination
- Phase 6: Table View
- Phase 7: Sorting and Filtering
- Phase 8: Tree View

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Core Components](#core-components)
4. [Data Flow](#data-flow)
5. [API Design](#api-design)
6. [Usage Examples](#usage-examples)
7. [Implementation Phases](#implementation-phases)
8. [Testing Strategy](#testing-strategy)

---

## Overview

### Motivation

OnyxUI currently requires widgets to directly manage their own data, making it difficult to:
- Reuse the same data across multiple views
- Separate business logic from presentation
- Build data-driven composite widgets
- Implement complex features like sorting, filtering, and multi-selection

This MVC system introduces **Model-View-Controller** separation inspired by Qt's architecture, enabling:

```cpp
// Before MVC: Data coupled to widget
auto list = std::make_unique<vbox<Backend>>();
for (const auto& item : data) {
    list->emplace_child<label>(item);  // Widget owns data
}

// After MVC: Data separated from presentation
auto model = std::make_shared<list_model<std::string, Backend>>();
model->set_items(data);

auto view = std::make_unique<list_view<Backend>>();
view->set_model(model);  // View displays model data

// Multiple views can share the same model!
auto view2 = std::make_unique<table_view<Backend>>();
view2->set_model(model);  // Both views auto-update when model changes
```

### Design Goals

1. **Separation of Concerns**: Models store data, views display it, delegates render items
2. **Automatic Updates**: Views auto-refresh when models emit change signals
3. **Reusability**: Same model can drive multiple views
4. **Customization**: Delegates allow custom rendering without subclassing views
5. **Integration**: Seamless integration with OnyxUI's signals, theming, and layout system

---

## Architecture

### Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                         MVC SYSTEM                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────┐      ┌─────────────┐      ┌─────────────┐    │
│  │   MODEL     │      │    VIEW     │      │  DELEGATE   │    │
│  │             │      │             │      │             │    │
│  │ Data storage│─────▶│ Display data│─────▶│ Render items│    │
│  │ Change      │      │ Handle      │      │ Edit items  │    │
│  │ signals     │      │ selection   │      │ Size hints  │    │
│  └─────────────┘      └─────────────┘      └─────────────┘    │
│        │                      │                                 │
│        │                      │                                 │
│        │                      ▼                                 │
│        │              ┌─────────────┐                          │
│        └─────────────▶│  SELECTION  │                          │
│                       │   MODEL     │                          │
│                       │             │                          │
│                       │ Track       │                          │
│                       │ selected    │                          │
│                       │ items       │                          │
│                       └─────────────┘                          │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### Key Concepts

#### 1. Model Index
Uniquely identifies an item in the model:

```cpp
struct model_index {
    int row;           // Row number (0-based)
    int column;        // Column number (0-based, 0 for lists)
    void* internal_id; // Model-specific identifier (e.g., tree node pointer)
    const void* model; // Pointer to owning model (for validity checks)

    [[nodiscard]] bool is_valid() const noexcept {
        return row >= 0 && column >= 0 && model != nullptr;
    }
};
```

#### 2. Data Roles
Different aspects of the same item:

```cpp
enum class item_data_role : uint8_t {
    display,      // Text to display (std::string)
    edit,         // Data for editing (any type)
    decoration,   // Icon or image (optional)
    tooltip,      // Tooltip text (std::string)
    background,   // Background color (Backend::color_type)
    foreground,   // Text color (Backend::color_type)
    user_role     // Custom application data (starting point for user-defined roles)
};
```

#### 3. Change Notifications
Models emit signals when data changes:

```cpp
// Signals on abstract_item_model
signal<model_index, model_index> data_changed;      // Range of items changed
signal<model_index, int, int> rows_inserted;        // Parent, first, last
signal<model_index, int, int> rows_removed;         // Parent, first, last
signal<> layout_changed;                            // Major structural change
```

---

## Core Components

### 1. Abstract Item Model

**File**: `include/onyxui/mvc/models/abstract_item_model.hh`

```cpp
template<UIBackend Backend>
class abstract_item_model {
public:
    using variant_type = std::variant<
        std::string,
        int,
        double,
        bool,
        typename Backend::color_type,
        std::any  // For custom types
    >;

    virtual ~abstract_item_model() = default;

    // Data access
    [[nodiscard]] virtual int row_count(const model_index& parent = {}) const = 0;
    [[nodiscard]] virtual int column_count(const model_index& parent = {}) const = 0;
    [[nodiscard]] virtual model_index index(int row, int column, const model_index& parent = {}) const = 0;
    [[nodiscard]] virtual model_index parent(const model_index& child) const = 0;
    [[nodiscard]] virtual variant_type data(const model_index& index, item_data_role role = item_data_role::display) const = 0;

    // Data modification
    virtual bool set_data(const model_index& index, const variant_type& value, item_data_role role = item_data_role::edit) {
        return false;  // Default: read-only
    }

    // Flags
    [[nodiscard]] virtual item_flags flags(const model_index& index) const {
        return item_flag::enabled | item_flag::selectable;
    }

    // Sorting
    virtual void sort(int column, sort_order order = sort_order::ascending) {
        // Default: no-op
    }

    // Change notification signals
    signal<model_index, model_index> data_changed;
    signal<model_index, int, int> rows_inserted;
    signal<model_index, int, int> rows_removed;
    signal<model_index, int, int> rows_about_to_be_inserted;
    signal<model_index, int, int> rows_about_to_be_removed;
    signal<> layout_changed;
    signal<> layout_about_to_be_changed;

protected:
    // Helper methods for subclasses
    void begin_insert_rows(const model_index& parent, int first, int last) {
        rows_about_to_be_inserted.emit(parent, first, last);
    }

    void end_insert_rows() {
        // Views handle rows_about_to_be_inserted to prepare for changes
    }

    void begin_remove_rows(const model_index& parent, int first, int last) {
        rows_about_to_be_removed.emit(parent, first, last);
    }

    void end_remove_rows() {
        // Views handle rows_about_to_be_removed to prepare for changes
    }
};
```

### 2. List Model (Concrete Implementation)

**File**: `include/onyxui/mvc/models/list_model.hh`

```cpp
template<typename T, UIBackend Backend>
class list_model : public abstract_item_model<Backend> {
public:
    using value_type = T;
    using container_type = std::vector<T>;

    // Construct with items
    explicit list_model(container_type items = {})
        : m_items(std::move(items)) {}

    // Data access
    [[nodiscard]] int row_count(const model_index& parent = {}) const override {
        if (parent.is_valid()) return 0;  // Lists have no children
        return static_cast<int>(m_items.size());
    }

    [[nodiscard]] int column_count(const model_index& parent = {}) const override {
        return 1;  // Single column for lists
    }

    [[nodiscard]] model_index index(int row, int column, const model_index& parent = {}) const override {
        if (parent.is_valid() || row < 0 || row >= static_cast<int>(m_items.size()) || column != 0) {
            return {};  // Invalid
        }
        return {row, column, nullptr, this};
    }

    [[nodiscard]] model_index parent(const model_index& child) const override {
        return {};  // Lists have no parent
    }

    [[nodiscard]] typename abstract_item_model<Backend>::variant_type
    data(const model_index& index, item_data_role role = item_data_role::display) const override {
        if (!index.is_valid() || index.row >= static_cast<int>(m_items.size())) {
            return {};
        }

        if (role == item_data_role::display || role == item_data_role::edit) {
            return to_string(m_items[index.row]);  // Convert T to string
        }

        return {};
    }

    // Modification
    void set_items(container_type items) {
        this->begin_remove_rows({}, 0, static_cast<int>(m_items.size()) - 1);
        m_items.clear();
        this->end_remove_rows();

        this->begin_insert_rows({}, 0, static_cast<int>(items.size()) - 1);
        m_items = std::move(items);
        this->end_insert_rows();

        this->rows_inserted.emit({}, 0, static_cast<int>(m_items.size()) - 1);
    }

    void append(const T& item) {
        int row = static_cast<int>(m_items.size());
        this->begin_insert_rows({}, row, row);
        m_items.push_back(item);
        this->end_insert_rows();
        this->rows_inserted.emit({}, row, row);
    }

    void insert(int row, const T& item) {
        if (row < 0 || row > static_cast<int>(m_items.size())) return;

        this->begin_insert_rows({}, row, row);
        m_items.insert(m_items.begin() + row, item);
        this->end_insert_rows();
        this->rows_inserted.emit({}, row, row);
    }

    void remove(int row) {
        if (row < 0 || row >= static_cast<int>(m_items.size())) return;

        this->begin_remove_rows({}, row, row);
        m_items.erase(m_items.begin() + row);
        this->end_remove_rows();
        this->rows_removed.emit({}, row, row);
    }

    // Direct access (for custom use cases)
    [[nodiscard]] const container_type& items() const noexcept { return m_items; }
    [[nodiscard]] const T& at(int row) const { return m_items.at(row); }

private:
    container_type m_items;

    // Convert T to string for display
    static std::string to_string(const T& value) {
        if constexpr (std::is_same_v<T, std::string>) {
            return value;
        } else if constexpr (std::is_arithmetic_v<T>) {
            return std::to_string(value);
        } else {
            // Custom types should specialize this or provide operator<<
            std::ostringstream oss;
            oss << value;
            return oss.str();
        }
    }
};
```

### 3. Abstract Item View

**File**: `include/onyxui/mvc/views/abstract_item_view.hh`

```cpp
template<UIBackend Backend>
class abstract_item_view : public widget<Backend> {
public:
    using model_type = abstract_item_model<Backend>;
    using delegate_type = abstract_item_delegate<Backend>;
    using selection_model_type = item_selection_model<Backend>;

    explicit abstract_item_view(ui_element<Backend>* parent = nullptr)
        : widget<Backend>(parent)
        , m_selection_model(std::make_unique<selection_model_type>())
    {
        // Default delegate
        m_delegate = std::make_shared<default_item_delegate<Backend>>();
    }

    // Model management
    void set_model(std::shared_ptr<model_type> model) {
        if (m_model) {
            // Disconnect old model signals
            m_data_changed_conn.disconnect();
            m_rows_inserted_conn.disconnect();
            m_rows_removed_conn.disconnect();
            m_layout_changed_conn.disconnect();
        }

        m_model = std::move(model);

        if (m_model) {
            // Connect to model change signals
            m_data_changed_conn = m_model->data_changed.connect(
                [this](const model_index& tl, const model_index& br) {
                    on_data_changed(tl, br);
                }
            );

            m_rows_inserted_conn = m_model->rows_inserted.connect(
                [this](const model_index& parent, int first, int last) {
                    on_rows_inserted(parent, first, last);
                }
            );

            m_rows_removed_conn = m_model->rows_removed.connect(
                [this](const model_index& parent, int first, int last) {
                    on_rows_removed(parent, first, last);
                }
            );

            m_layout_changed_conn = m_model->layout_changed.connect(
                [this]() {
                    on_layout_changed();
                }
            );

            // Update selection model
            m_selection_model->set_model(m_model.get());
        }

        // Trigger full repaint
        this->invalidate_layout();
        this->mark_dirty();
    }

    [[nodiscard]] model_type* model() const noexcept { return m_model.get(); }

    // Delegate management
    void set_delegate(std::shared_ptr<delegate_type> delegate) {
        m_delegate = std::move(delegate);
        this->mark_dirty();
    }

    [[nodiscard]] delegate_type* delegate() const noexcept { return m_delegate.get(); }

    // Selection
    [[nodiscard]] selection_model_type* selection_model() const noexcept {
        return m_selection_model.get();
    }

    void set_selection_mode(selection_mode mode) {
        m_selection_model->set_selection_mode(mode);
    }

    // Current item (keyboard focus)
    [[nodiscard]] model_index current_index() const noexcept {
        return m_selection_model->current_index();
    }

    void set_current_index(const model_index& index) {
        m_selection_model->set_current_index(index);
    }

    // Signals
    signal<const model_index&> clicked;
    signal<const model_index&> double_clicked;
    signal<const model_index&> activated;  // Enter key or double-click

protected:
    // Model change handlers (for subclasses to implement)
    virtual void on_data_changed(const model_index& top_left, const model_index& bottom_right) {
        this->mark_dirty();  // Default: repaint entire view
    }

    virtual void on_rows_inserted(const model_index& parent, int first, int last) {
        this->invalidate_layout();  // Default: relayout
        this->mark_dirty();
    }

    virtual void on_rows_removed(const model_index& parent, int first, int last) {
        this->invalidate_layout();  // Default: relayout
        this->mark_dirty();
    }

    virtual void on_layout_changed() {
        this->invalidate_layout();  // Default: full relayout
        this->mark_dirty();
    }

    // Subclasses override these for custom rendering
    virtual void paint_item(
        render_context<Backend>& ctx,
        const model_index& index,
        const typename Backend::rect_type& item_rect,
        bool is_selected,
        bool has_focus
    ) const = 0;

    virtual typename Backend::size_type item_size_hint(const model_index& index) const = 0;

    // Helper: Get visual rect for an index (in view coordinates)
    [[nodiscard]] virtual typename Backend::rect_type visual_rect(const model_index& index) const = 0;

    // Helper: Get index at a point (in view coordinates)
    [[nodiscard]] virtual model_index index_at(int x, int y) const = 0;

protected:
    std::shared_ptr<model_type> m_model;
    std::shared_ptr<delegate_type> m_delegate;
    std::unique_ptr<selection_model_type> m_selection_model;

    // Signal connections (RAII cleanup)
    scoped_connection m_data_changed_conn;
    scoped_connection m_rows_inserted_conn;
    scoped_connection m_rows_removed_conn;
    scoped_connection m_layout_changed_conn;
};
```

### 4. List View (Concrete View)

**File**: `include/onyxui/mvc/views/list_view.hh`

```cpp
template<UIBackend Backend>
class list_view : public abstract_item_view<Backend> {
public:
    using base = abstract_item_view<Backend>;
    using size_type = typename Backend::size_type;
    using rect_type = typename Backend::rect_type;

    explicit list_view(ui_element<Backend>* parent = nullptr)
        : base(parent)
        , m_item_height(20)  // Default item height
        , m_spacing(2)       // Default spacing between items
    {}

    // Configuration
    void set_item_height(int height) {
        m_item_height = height;
        this->invalidate_layout();
    }

    void set_spacing(int spacing) {
        m_spacing = spacing;
        this->invalidate_layout();
    }

protected:
    void do_render(render_context<Backend>& ctx) const override {
        if (!this->m_model) return;

        // Draw background
        ctx.fill_rect(this->bounds(), this->get_theme_background_color());

        // Draw items
        int row_count = this->m_model->row_count();
        int y = rect_utils::get_y(this->bounds());

        for (int row = 0; row < row_count; ++row) {
            model_index index = this->m_model->index(row, 0);
            if (!index.is_valid()) continue;

            // Item rect
            rect_type item_rect = rect_utils::make_rect(
                rect_utils::get_x(this->bounds()),
                y,
                rect_utils::get_width(this->bounds()),
                m_item_height
            );

            // Check if selected
            bool is_selected = this->m_selection_model->is_selected(index);
            bool has_focus = (index == this->m_selection_model->current_index());

            // Delegate renders the item
            this->m_delegate->paint(ctx, index, item_rect, is_selected, has_focus);

            y += m_item_height + m_spacing;
        }
    }

    typename Backend::size_type measure_override(int available_width, int available_height) override {
        if (!this->m_model) {
            return {0, 0};
        }

        int row_count = this->m_model->row_count();
        int total_height = row_count * (m_item_height + m_spacing);
        if (row_count > 0) {
            total_height -= m_spacing;  // No spacing after last item
        }

        return {available_width, total_height};
    }

    [[nodiscard]] rect_type visual_rect(const model_index& index) const override {
        if (!index.is_valid()) return {};

        int y = rect_utils::get_y(this->bounds()) + index.row * (m_item_height + m_spacing);

        return rect_utils::make_rect(
            rect_utils::get_x(this->bounds()),
            y,
            rect_utils::get_width(this->bounds()),
            m_item_height
        );
    }

    [[nodiscard]] model_index index_at(int x, int y) const override {
        if (!this->m_model) return {};

        int local_y = y - rect_utils::get_y(this->bounds());
        int row = local_y / (m_item_height + m_spacing);

        if (row < 0 || row >= this->m_model->row_count()) {
            return {};
        }

        return this->m_model->index(row, 0);
    }

    void paint_item(
        render_context<Backend>& ctx,
        const model_index& index,
        const rect_type& item_rect,
        bool is_selected,
        bool has_focus
    ) const override {
        // Delegate to the item delegate
        this->m_delegate->paint(ctx, index, item_rect, is_selected, has_focus);
    }

    size_type item_size_hint(const model_index& index) const override {
        return {0, m_item_height};  // Width is flexible, height is fixed
    }

    bool handle_event(const ui_event& event, event_phase phase) override {
        if (phase != event_phase::target) {
            return base::handle_event(event, phase);
        }

        if (event.type == ui_event::event_type::mouse_press) {
            model_index clicked_index = index_at(event.x, event.y);
            if (clicked_index.is_valid()) {
                // Update selection
                this->m_selection_model->set_current_index(clicked_index);
                this->clicked.emit(clicked_index);
                this->mark_dirty();
                return true;
            }
        }

        return base::handle_event(event, phase);
    }

private:
    int m_item_height;
    int m_spacing;
};
```

### 5. Item Delegate

**File**: `include/onyxui/mvc/delegates/abstract_item_delegate.hh`

```cpp
template<UIBackend Backend>
class abstract_item_delegate {
public:
    virtual ~abstract_item_delegate() = default;

    // Render an item
    virtual void paint(
        render_context<Backend>& ctx,
        const model_index& index,
        const typename Backend::rect_type& rect,
        bool is_selected,
        bool has_focus
    ) const = 0;

    // Size hint for an item
    [[nodiscard]] virtual typename Backend::size_type size_hint(
        const model_index& index
    ) const = 0;

    // TODO: Future editing support
    // virtual ui_element<Backend>* create_editor(ui_element<Backend>* parent, const model_index& index) const;
    // virtual void set_editor_data(ui_element<Backend>* editor, const model_index& index) const;
    // virtual void set_model_data(ui_element<Backend>* editor, const model_index& index) const;
};
```

**File**: `include/onyxui/mvc/delegates/default_item_delegate.hh`

```cpp
template<UIBackend Backend>
class default_item_delegate : public abstract_item_delegate<Backend> {
public:
    void paint(
        render_context<Backend>& ctx,
        const model_index& index,
        const typename Backend::rect_type& rect,
        bool is_selected,
        bool has_focus
    ) const override {
        auto* model = static_cast<const abstract_item_model<Backend>*>(index.model);
        if (!model) return;

        // Get display text
        auto display_data = model->data(index, item_data_role::display);
        std::string text;
        if (std::holds_alternative<std::string>(display_data)) {
            text = std::get<std::string>(display_data);
        }

        // Background color
        typename Backend::color_type bg_color;
        if (is_selected) {
            bg_color = {0, 120, 215};  // Blue selection (Windows style)
        } else {
            auto bg_data = model->data(index, item_data_role::background);
            if (std::holds_alternative<typename Backend::color_type>(bg_data)) {
                bg_color = std::get<typename Backend::color_type>(bg_data);
            } else {
                bg_color = {255, 255, 255};  // White
            }
        }

        // Text color
        typename Backend::color_type fg_color;
        if (is_selected) {
            fg_color = {255, 255, 255};  // White text on blue
        } else {
            auto fg_data = model->data(index, item_data_role::foreground);
            if (std::holds_alternative<typename Backend::color_type>(fg_data)) {
                fg_color = std::get<typename Backend::color_type>(fg_data);
            } else {
                fg_color = {0, 0, 0};  // Black
            }
        }

        // Draw background
        ctx.fill_rect(rect, bg_color);

        // Draw text
        int text_x = rect_utils::get_x(rect) + 4;  // 4px left padding
        int text_y = rect_utils::get_y(rect) + 2;  // 2px top padding
        ctx.draw_text(text, text_x, text_y, fg_color);

        // Draw focus rect
        if (has_focus) {
            ctx.draw_rect(rect, {0, 0, 0}, box_border_style::dotted);
        }
    }

    [[nodiscard]] typename Backend::size_type size_hint(
        const model_index& index
    ) const override {
        // Default size: measure text
        auto* model = static_cast<const abstract_item_model<Backend>*>(index.model);
        if (!model) return {100, 20};

        auto display_data = model->data(index, item_data_role::display);
        std::string text;
        if (std::holds_alternative<std::string>(display_data)) {
            text = std::get<std::string>(display_data);
        }

        // Rough estimate: 8px per char + padding
        int width = static_cast<int>(text.size()) * 8 + 8;
        int height = 20;  // Fixed height

        return {width, height};
    }
};
```

### 6. Selection Model

**File**: `include/onyxui/mvc/selection/item_selection_model.hh`

```cpp
enum class selection_mode : uint8_t {
    single,       // Only one item can be selected
    multi,        // Multiple items can be selected (Ctrl+click)
    extended,     // Extended selection (Shift+click for ranges)
    no_selection  // No selection allowed
};

template<UIBackend Backend>
class item_selection_model {
public:
    explicit item_selection_model(
        abstract_item_model<Backend>* model = nullptr,
        selection_mode mode = selection_mode::single
    )
        : m_model(model)
        , m_mode(mode)
    {}

    // Model
    void set_model(abstract_item_model<Backend>* model) {
        clear();
        m_model = model;
    }

    // Selection mode
    void set_selection_mode(selection_mode mode) {
        if (mode != m_mode) {
            clear();
            m_mode = mode;
        }
    }

    [[nodiscard]] selection_mode get_selection_mode() const noexcept { return m_mode; }

    // Current index (keyboard focus)
    void set_current_index(const model_index& index) {
        if (m_current_index == index) return;

        model_index previous = m_current_index;
        m_current_index = index;

        // Auto-select in single-selection mode
        if (m_mode == selection_mode::single && index.is_valid()) {
            clear();
            m_selected_indices.insert(index);
            selection_changed.emit();
        }

        current_changed.emit(m_current_index, previous);
    }

    [[nodiscard]] model_index current_index() const noexcept { return m_current_index; }

    // Selection
    void select(const model_index& index) {
        if (!index.is_valid() || m_mode == selection_mode::no_selection) return;

        if (m_mode == selection_mode::single) {
            clear();
        }

        m_selected_indices.insert(index);
        selection_changed.emit();
    }

    void deselect(const model_index& index) {
        if (m_selected_indices.erase(index) > 0) {
            selection_changed.emit();
        }
    }

    void toggle(const model_index& index) {
        if (is_selected(index)) {
            deselect(index);
        } else {
            select(index);
        }
    }

    void clear() {
        if (!m_selected_indices.empty()) {
            m_selected_indices.clear();
            selection_changed.emit();
        }
    }

    [[nodiscard]] bool is_selected(const model_index& index) const {
        return m_selected_indices.contains(index);
    }

    [[nodiscard]] const std::unordered_set<model_index>& selected_indices() const noexcept {
        return m_selected_indices;
    }

    // Signals
    signal<const model_index&, const model_index&> current_changed;  // New, previous
    signal<> selection_changed;

private:
    abstract_item_model<Backend>* m_model = nullptr;
    selection_mode m_mode = selection_mode::single;
    model_index m_current_index;
    std::unordered_set<model_index> m_selected_indices;
};

// Hash function for model_index (needed for unordered_set)
template<>
struct std::hash<model_index> {
    std::size_t operator()(const model_index& idx) const noexcept {
        std::size_t h1 = std::hash<int>{}(idx.row);
        std::size_t h2 = std::hash<int>{}(idx.column);
        std::size_t h3 = std::hash<const void*>{}(idx.model);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

// Equality for model_index
inline bool operator==(const model_index& lhs, const model_index& rhs) noexcept {
    return lhs.row == rhs.row
        && lhs.column == rhs.column
        && lhs.model == rhs.model
        && lhs.internal_id == rhs.internal_id;
}
```

---

## Data Flow

### 1. Model Updates View

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

### 2. User Clicks Item

```
Mouse click (150, 75)
       │
       ▼
View::handle_event(mouse_press)
       │
       ├─── index = index_at(150, 75)  // Find which item
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

### 3. Custom Delegate Rendering

```
View needs to repaint
       │
       ▼
View::do_render(ctx)
       │
       └─── For each visible item:
                 │
                 ├─── index = model->index(row, 0)
                 ├─── is_selected = selection_model->is_selected(index)
                 ├─── has_focus = (index == current_index)
                 │
                 └─── delegate->paint(ctx, index, rect, is_selected, has_focus)
                           │
                           ├─── Get display data from model
                           ├─── Get colors from model or theme
                           ├─── Draw background (blue if selected)
                           ├─── Draw text
                           └─── Draw focus rect (dotted border)
```

---

## Usage Examples

### Example 1: Simple String List

```cpp
#include <onyxui/mvc/models/list_model.hh>
#include <onyxui/mvc/views/list_view.hh>

// Create model with data
auto model = std::make_shared<list_model<std::string, Backend>>();
model->set_items({"Apple", "Banana", "Cherry", "Date"});

// Create view
auto view = std::make_unique<list_view<Backend>>();
view->set_model(model);

// Add to UI
root->add_child(std::move(view));

// Later: Add more items (view auto-updates!)
model->append("Elderberry");
model->insert(0, "Apricot");  // Insert at beginning
```

### Example 2: Custom Struct with Multiple Views

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

// Create model
auto model = std::make_shared<list_model<Person, Backend>>();
model->set_items({
    {"Alice", 30, "alice@example.com"},
    {"Bob", 25, "bob@example.com"},
    {"Charlie", 35, "charlie@example.com"}
});

// Create TWO views of the same model
auto list = std::make_unique<list_view<Backend>>();
list->set_model(model);

auto table = std::make_unique<table_view<Backend>>();  // Future: multi-column
table->set_model(model);

// Both views update when model changes!
model->append({"Diana", 28, "diana@example.com"});
```

### Example 3: Custom Delegate

```cpp
// Custom delegate for colorful items
template<UIBackend Backend>
class colorful_delegate : public abstract_item_delegate<Backend> {
public:
    void paint(
        render_context<Backend>& ctx,
        const model_index& index,
        const typename Backend::rect_type& rect,
        bool is_selected,
        bool has_focus
    ) const override {
        // Alternate row colors
        typename Backend::color_type bg = (index.row % 2 == 0)
            ? typename Backend::color_type{240, 240, 240}  // Light gray
            : typename Backend::color_type{255, 255, 255}; // White

        if (is_selected) {
            bg = {255, 200, 100};  // Orange selection
        }

        ctx.fill_rect(rect, bg);

        // Draw text with custom color
        auto* model = static_cast<const abstract_item_model<Backend>*>(index.model);
        auto text_data = model->data(index, item_data_role::display);
        std::string text = std::get<std::string>(text_data);

        ctx.draw_text(text, rect_utils::get_x(rect) + 8, rect_utils::get_y(rect) + 4, {0, 0, 0});
    }

    [[nodiscard]] typename Backend::size_type size_hint(const model_index& index) const override {
        return {200, 24};
    }
};

// Use custom delegate
auto view = std::make_unique<list_view<Backend>>();
view->set_model(model);
view->set_delegate(std::make_shared<colorful_delegate<Backend>>());
```

### Example 4: Selection Handling

```cpp
auto view = std::make_unique<list_view<Backend>>();
view->set_model(model);

// Enable multi-selection
view->set_selection_mode(selection_mode::multi);

// Handle clicks
view->clicked.connect([](const model_index& index) {
    std::cout << "Clicked row " << index.row << "\n";
});

// Handle selection changes
view->selection_model()->selection_changed.connect([&view]() {
    auto& selected = view->selection_model()->selected_indices();
    std::cout << "Selected " << selected.size() << " items\n";
});

// Programmatic selection
view->selection_model()->select(model->index(2, 0));  // Select 3rd item
```

### Example 5: Sorting

```cpp
// Create sortable model (future: table_model with column sorting)
auto model = std::make_shared<list_model<int, Backend>>();
model->set_items({42, 17, 91, 3, 56});

auto view = std::make_unique<list_view<Backend>>();
view->set_model(model);

// Sort model (view auto-updates)
model->sort(0, sort_order::ascending);  // [3, 17, 42, 56, 91]
```

---

## Implementation Phases

### ✅ Phase 1: Core Infrastructure (COMPLETE)
**Status**: Completed 2025-11-22 | Commits: 3eeb378, a8f77ec, 7e6adf8

**Files Created**:
- ✅ `include/onyxui/mvc/model_index.hh` - Index struct
- ✅ `include/onyxui/mvc/item_data_role.hh` - Data role enum
- ✅ `include/onyxui/mvc/models/abstract_item_model.hh` - Model base class
- ✅ `include/onyxui/mvc/models/list_model.hh` - List model implementation
- ✅ `include/onyxui/mvc/views/abstract_item_view.hh` - View base class
- ✅ `include/onyxui/mvc/views/list_view.hh` - List view implementation
- ✅ `include/onyxui/mvc/delegates/abstract_item_delegate.hh` - Delegate base
- ✅ `include/onyxui/mvc/delegates/default_item_delegate.hh` - Default delegate
- ✅ `include/onyxui/mvc/selection/item_selection_model.hh` - Selection tracking

**Tests**:
- ✅ `unittest/mvc/test_mvc_basic.cc` - 108 comprehensive test cases
  - Model data access, modification, signals
  - View rendering, selection
  - Auto-update on model changes
  - Selection modes (single/multi/extended/contiguous)
  - Delegate rendering

**Success Criteria**:
- ✅ Create list_model with items
- ✅ Attach to list_view
- ✅ View displays items correctly
- ✅ Model changes trigger view updates
- ✅ Click selection works
- ✅ All 108 tests passing

### ✅ Phase 2: combo_box Widget (COMPLETE)
**Status**: Completed 2025-11-23 | Commits: 67c9891, 6751c71

**Files Created**:
- ✅ `include/onyxui/widgets/input/combo_box.hh` - Combo box widget (348 lines)

**Implementation Details**:
- ✅ MVC integration with `abstract_item_model`
- ✅ Keyboard navigation (arrow keys, Home/End)
- ✅ Selection tracking via `current_index_changed` signal
- ✅ State-based theming (normal/hover/pressed/disabled)
- ✅ Visual rendering (button box + dropdown arrow "▼")
- ✅ Display text management (shows current selection or "(select)")
- ✅ Model/selection synchronization

**Tests**:
- ✅ `unittest/widgets/test_combo_box.cc` - 7 test cases
  - Construction and initialization
  - Model management (set_model, model changes)
  - Selection management (set_current_index, bounds checking)
  - Signal emission (current_index_changed)
  - Edge cases (invalid index, empty model, selection clearing)

**Demo Integration**:
- ✅ Added to `examples/demo_ui_builder.hh`
- ✅ Example: Size selection ("Small", "Medium", "Large", "X-Large")
- ✅ Default selection: "Medium" (index 1)
- ✅ Visual appearance in widgets_demo

**Success Criteria**:
- ✅ Single selection mode works (default)
- ✅ Keyboard navigation updates selection (arrow keys, Home/End)
- ✅ Selection signals emit properly
- ✅ Combo box displays current selection text
- ✅ State-based visual feedback (hover/press)
- ✅ All 7 tests passing
- ⚠️ Popup rendering deferred (requires layer_manager integration)
- ⚠️ Mouse dropdown interaction deferred (requires popup)

**Known Limitations**:
- Popup list not visually shown (layer_manager integration TODO)
- Mouse-based selection not available (requires popup)
- Keyboard navigation fully functional as workaround

### Phase 3: Scrollable Integration + Virtual Scrolling (Week 5)
**Goal**: Views work inside scroll_view with efficient rendering

**Files to Create**:
- `include/onyxui/core/element.hh`: Add `find_ancestor<T>()` helper

**Enhancements**:
- `list_view.hh`: Virtual scrolling (only render visible items)
- `list_view.hh`: Query parent scrollable for scroll position
- `table_view.hh`: Virtual scrolling support (when implemented)

**Tests**:
- `unittest/mvc/test_scrolling_integration.cc` - Wrap views in scroll_view
- `unittest/mvc/test_virtual_scrolling.cc` - Large datasets (10k+ items)
- `unittest/core/test_element_ancestor.cc` - find_ancestor() helper

**Success Criteria**:
- ✅ Views work correctly inside scroll_view
- ✅ Scrollbars appear for large datasets
- ✅ Virtual scrolling renders only visible items
- ✅ Smooth scrolling with 10k+ items

### Phase 4: Custom Delegates (Week 6)
**Goal**: Allow users to customize item rendering

**Enhancements**:
- `abstract_item_delegate.hh`: Document custom delegate pattern
- Example delegates: `icon_delegate.hh`, `progress_bar_delegate.hh`

**Tests**:
- `unittest/mvc/test_custom_delegate.cc` - Custom rendering

**Success Criteria**:
- ✅ Users can create custom delegates
- ✅ Delegates receive correct parameters
- ✅ View uses delegate for all rendering

### Phase 5: Pagination (Week 7)
**Goal**: Load large datasets in chunks (REQUIRED for database/API integration)

**Files to Create**:
- `include/onyxui/mvc/models/paginated_model.hh` - Page-based model

**Features**:
- LRU page cache (10 pages in memory)
- On-demand page loading via callback
- Prefetching for smooth scrolling
- Works transparently with existing views

**Tests**:
- `unittest/mvc/test_paginated_model.cc` - Page loading and caching
- `unittest/mvc/test_pagination_scrolling.cc` - Integration with scrolling
- `unittest/mvc/test_pagination_database.cc` - SQLite integration (1M rows)

**Success Criteria**:
- ✅ Load data in 100-item pages
- ✅ LRU cache evicts old pages
- ✅ Smooth scrolling with prefetching
- ✅ Database integration works correctly

### Phase 6: Table View (Week 8-9)
**Goal**: Multi-column table support

**Files to Create**:
- `include/onyxui/mvc/models/table_model.hh` - 2D table model
- `include/onyxui/mvc/views/table_view.hh` - Table view with headers
- `include/onyxui/mvc/views/table_header.hh` - Column headers

**Features**:
- Column headers with click-to-sort
- Column resizing
- Row selection (full row, not individual cells)

**Tests**:
- `unittest/mvc/test_table_model.cc` - 2D data access
- `unittest/mvc/test_table_view.cc` - Multi-column rendering
- `unittest/mvc/test_table_sorting.cc` - Column sorting

**Success Criteria**:
- ✅ Display multi-column data
- ✅ Click column headers to sort
- ✅ Resize columns with mouse
- ✅ Select entire rows

### Phase 7: Sorting and Filtering (Week 10)
**Goal**: Data manipulation without changing underlying model

**Files to Create**:
- `include/onyxui/mvc/models/sort_filter_proxy_model.hh` - Proxy model

**Features**:
- Sort by column (ascending/descending)
- Filter by predicate function
- Proxy maintains stable indices

**Tests**:
- `unittest/mvc/test_sorting.cc` - Sort operations
- `unittest/mvc/test_filtering.cc` - Filter operations
- `unittest/mvc/test_proxy_model.cc` - Index mapping

**Success Criteria**:
- ✅ Sort without modifying source model
- ✅ Filter rows dynamically
- ✅ Views update when sort/filter changes

### Phase 8: Tree View (Week 11-13)
**Goal**: Hierarchical data support

**Files to Create**:
- `include/onyxui/mvc/models/tree_model.hh` - Hierarchical model
- `include/onyxui/mvc/views/tree_view.hh` - Tree view with expand/collapse

**Features**:
- Parent/child relationships
- Expand/collapse nodes
- Indentation for hierarchy levels

**Tests**:
- `unittest/mvc/test_tree_model.cc` - Hierarchical access
- `unittest/mvc/test_tree_view.cc` - Expand/collapse

**Success Criteria**:
- ✅ Display hierarchical data
- ✅ Expand/collapse works
- ✅ Navigate tree with keyboard

---

## Testing Strategy

### Unit Tests

1. **Model Tests** (`unittest/mvc/test_*_model.cc`):
   - Data access correctness
   - Modification operations
   - Signal emission
   - Edge cases (empty, single item, large datasets)

2. **View Tests** (`unittest/mvc/test_*_view.cc`):
   - Rendering correctness
   - Layout calculations
   - Event handling
   - Selection updates

3. **Integration Tests** (`unittest/mvc/test_integration.cc`):
   - Model changes → view updates
   - Multiple views on same model
   - Delegate customization
   - Performance with large datasets (10k+ items)

### Test Infrastructure

```cpp
// Fixture for MVC tests
template<UIBackend Backend>
class mvc_test_fixture {
public:
    mvc_test_fixture() {
        ctx = std::make_unique<scoped_ui_context<Backend>>();
        model = std::make_shared<list_model<std::string, Backend>>();
        view = std::make_unique<list_view<Backend>>();
        view->set_model(model);
    }

    std::unique_ptr<scoped_ui_context<Backend>> ctx;
    std::shared_ptr<list_model<std::string, Backend>> model;
    std::unique_ptr<list_view<Backend>> view;
};

// Example test
TEST_CASE("List view updates when model changes") {
    mvc_test_fixture<test_canvas_backend> fixture;

    // Add items to model
    fixture.model->set_items({"A", "B", "C"});

    // View should reflect model
    REQUIRE(fixture.view->model()->row_count() == 3);

    // Modify model
    fixture.model->append("D");

    // View auto-updates
    REQUIRE(fixture.view->model()->row_count() == 4);
}
```

---

## Scrollable Integration

### Overview

Views must integrate with OnyxUI's scrolling system to handle large datasets. The scrolling architecture has three layers:

1. **scroll_view** - High-level container with automatic scrollbars
2. **scrollable** - Content viewport with scroll controller
3. **View widgets** - Implement virtual scrolling for efficiency

### Design Approach

**Option 1: Views inherit from scrollable** (REJECTED)
- Problem: Multiple inheritance issues (view already inherits from widget)
- Problem: Forces views to always be scrollable

**Option 2: Wrap views in scroll_view** (RECOMMENDED)
- Views remain simple widgets
- Users wrap in scroll_view when needed
- Leverages existing scrolling infrastructure

**Option 3: Views embed scrollable internally** (FUTURE)
- For advanced cases (e.g., tree_view with horizontal scrolling)
- Views manage their own scroll_view child

### Implementation

```cpp
// User code: Wrap view in scroll_view for large datasets
auto model = std::make_shared<list_model<std::string, Backend>>();
model->set_items(large_dataset);  // 10,000 items

auto view = std::make_unique<list_view<Backend>>();
view->set_model(model);

// Wrap in scroll_view for automatic scrolling
auto scroll = modern_scroll_view<Backend>();
scroll->set_content(std::move(view));

root->add_child(std::move(scroll));
```

### Virtual Scrolling (Phase 3+)

For optimal performance with 100k+ items, views should only render visible items:

```cpp
template<UIBackend Backend>
class list_view : public abstract_item_view<Backend> {
protected:
    void do_render(render_context<Backend>& ctx) const override {
        if (!this->m_model) return;

        // Calculate visible range based on scroll position
        int scroll_offset_y = get_scroll_offset_y();  // From parent scroll_view
        int viewport_height = rect_utils::get_height(this->bounds());

        int first_visible = scroll_offset_y / m_item_height;
        int last_visible = (scroll_offset_y + viewport_height) / m_item_height;

        // Only render visible items (virtual scrolling)
        for (int row = first_visible; row <= last_visible; ++row) {
            if (row >= this->m_model->row_count()) break;

            model_index index = this->m_model->index(row, 0);
            // Render item...
        }
    }

    typename Backend::size_type measure_override(int available_width, int available_height) override {
        // Total content height (for scrollbar sizing)
        int row_count = this->m_model->row_count();
        int total_height = row_count * m_item_height;
        return {available_width, total_height};
    }

private:
    int get_scroll_offset_y() const {
        // Get scroll position from parent scroll_view or scrollable
        // This requires communication with parent widget
        auto* parent_scrollable = this->template find_ancestor<scrollable<Backend>>();
        if (parent_scrollable) {
            return parent_scrollable->get_scroll_position_y();
        }
        return 0;
    }
};
```

**Benefits**:
- Render only ~50 visible items instead of 100,000
- Constant rendering time regardless of dataset size
- Smooth scrolling performance

**Phase 3+ Implementation**:
1. Add `find_ancestor<T>()` helper to ui_element
2. Views query parent scrollable for scroll position
3. Views calculate visible range dynamically
4. Measure returns full content size for accurate scrollbars

---

## Pagination Design

### Overview

For very large datasets (e.g., database queries returning millions of rows), loading all data at once is impractical. Pagination allows models to load data in chunks on-demand.

### Architecture

```
┌─────────────────────────────────────────┐
│         PAGINATED MODEL                  │
├─────────────────────────────────────────┤
│                                          │
│  ┌────────┬────────┬────────┬────────┐ │
│  │ Page 0 │ Page 1 │ Page 2 │ Page 3 │ │  Loaded pages (cached)
│  │ (100)  │ (100)  │ (100)  │ (100)  │ │
│  └────────┴────────┴────────┴────────┘ │
│       │        │        │        │      │
│       └────────┴────────┴────────┘      │
│                 │                        │
│                 ▼                        │
│         Page Cache (LRU)                │
│         Max 10 pages in memory          │
│                                          │
│  ┌──────────────────────────┐          │
│  │  Data Provider           │          │  Backend data source
│  │  (Database, API, etc.)   │          │
│  └──────────────────────────┘          │
└─────────────────────────────────────────┘
```

### API Design

**File**: `include/onyxui/mvc/models/paginated_model.hh`

```cpp
template<typename T, UIBackend Backend>
class paginated_model : public abstract_item_model<Backend> {
public:
    using page_loader = std::function<std::vector<T>(int page_index, int page_size)>;

    /**
     * @brief Construct paginated model
     * @param total_items Total number of items (from COUNT query)
     * @param page_size Items per page (default: 100)
     * @param loader Function to load a specific page
     */
    paginated_model(int total_items, int page_size, page_loader loader)
        : m_total_items(total_items)
        , m_page_size(page_size)
        , m_loader(std::move(loader))
        , m_max_cached_pages(10)
    {}

    [[nodiscard]] int row_count(const model_index& parent = {}) const override {
        return m_total_items;  // Total items (not just loaded)
    }

    [[nodiscard]] typename abstract_item_model<Backend>::variant_type
    data(const model_index& index, item_data_role role = item_data_role::display) const override {
        if (!index.is_valid() || index.row >= m_total_items) {
            return {};
        }

        // Calculate which page contains this row
        int page_index = index.row / m_page_size;
        int offset_in_page = index.row % m_page_size;

        // Load page if not cached
        ensure_page_loaded(page_index);

        // Get data from cache
        const auto& page = m_page_cache.at(page_index);
        if (offset_in_page >= static_cast<int>(page.size())) {
            return {};
        }

        if (role == item_data_role::display || role == item_data_role::edit) {
            return to_string(page[offset_in_page]);
        }

        return {};
    }

    // Cache management
    void set_max_cached_pages(int max_pages) {
        m_max_cached_pages = max_pages;
        evict_old_pages();
    }

    void clear_cache() {
        m_page_cache.clear();
        m_page_access_order.clear();
    }

    // Prefetch pages for smooth scrolling
    void prefetch_pages(int first_page, int last_page) {
        for (int page = first_page; page <= last_page; ++page) {
            if (page >= 0 && page < total_pages()) {
                ensure_page_loaded(page);
            }
        }
    }

private:
    int m_total_items;
    int m_page_size;
    page_loader m_loader;
    int m_max_cached_pages;

    // LRU cache
    mutable std::unordered_map<int, std::vector<T>> m_page_cache;
    mutable std::list<int> m_page_access_order;  // Most recent at front

    [[nodiscard]] int total_pages() const {
        return (m_total_items + m_page_size - 1) / m_page_size;
    }

    void ensure_page_loaded(int page_index) const {
        if (m_page_cache.contains(page_index)) {
            // Update LRU order
            m_page_access_order.remove(page_index);
            m_page_access_order.push_front(page_index);
            return;
        }

        // Load page from backend
        std::vector<T> page_data = m_loader(page_index, m_page_size);
        m_page_cache[page_index] = std::move(page_data);
        m_page_access_order.push_front(page_index);

        // Evict old pages if cache is full
        evict_old_pages();
    }

    void evict_old_pages() const {
        while (static_cast<int>(m_page_cache.size()) > m_max_cached_pages) {
            int lru_page = m_page_access_order.back();
            m_page_access_order.pop_back();
            m_page_cache.erase(lru_page);
        }
    }
};
```

### Usage Example

```cpp
// Connect to database or API
auto load_page = [](int page_index, int page_size) -> std::vector<Person> {
    // SQL: SELECT * FROM persons LIMIT page_size OFFSET (page_index * page_size)
    return database->query_page(page_index * page_size, page_size);
};

// Count total items
int total_count = database->count("SELECT COUNT(*) FROM persons");

// Create paginated model
auto model = std::make_shared<paginated_model<Person, Backend>>(
    total_count,  // Total: 1,000,000 rows
    100,          // Page size: 100 rows
    load_page
);

// Create view (same as regular list_view!)
auto view = std::make_unique<list_view<Backend>>();
view->set_model(model);

// Pages load automatically as user scrolls
// Only 10 pages (1000 items) in memory at any time
```

### Scrolling Integration

When combined with virtual scrolling, pagination provides two-tier efficiency:

```
User scrolls to row 50,000:
  1. Virtual scrolling: Only render visible rows (50,000-50,050)
  2. Pagination: Only load pages 500-501 (200 items) from database
  3. Total memory: ~200 items instead of 1,000,000
```

**Performance**:
- Database queries: ~10ms per page (100 items)
- Page cache lookup: O(1)
- Smooth scrolling with prefetching

**Phase 4 Implementation**:
1. Implement `paginated_model<T, Backend>`
2. Add prefetching based on scroll direction
3. Add loading indicators for pending pages
4. Test with SQLite database (1M rows)

---

## Combo Box Design

### Overview

A combo box is a dropdown selection widget that combines:
- A button showing the current selection
- A popup list_view for choosing items
- Model-based data binding

### Visual Design

```
Closed state:
┌─────────────────────────┐
│ Selected Item      ▼    │  ← Button with dropdown arrow
└─────────────────────────┘

Open state:
┌─────────────────────────┐
│ Selected Item      ▲    │  ← Button (active)
└─────────────────────────┘
  ┌─────────────────────────┐
  │ Item 1                  │  ← Popup list_view
  │ Item 2                  │     (in floating layer)
  │ Item 3         ←        │     Selected item marked
  │ Item 4                  │
  │ Item 5                  │
  └─────────────────────────┘
```

### API Design

**File**: `include/onyxui/mvc/views/combo_box.hh`

```cpp
template<UIBackend Backend>
class combo_box : public widget<Backend> {
public:
    using model_type = abstract_item_model<Backend>;

    explicit combo_box(ui_element<Backend>* parent = nullptr)
        : widget<Backend>(parent)
        , m_current_index(-1)
    {
        // Create button for closed state
        m_button = this->template create_child<button<Backend>>();
        m_button->clicked.connect([this]() { toggle_popup(); });

        // Create popup list_view (hidden by default)
        m_popup_list = std::make_unique<list_view<Backend>>();
        m_popup_list->clicked.connect([this](const model_index& index) {
            select_item(index.row);
            close_popup();
        });
    }

    // Model management
    void set_model(std::shared_ptr<model_type> model) {
        m_model = std::move(model);
        m_popup_list->set_model(m_model);

        // Set first item as current if model has items
        if (m_model && m_model->row_count() > 0 && m_current_index < 0) {
            set_current_index(0);
        }
    }

    [[nodiscard]] model_type* model() const noexcept { return m_model.get(); }

    // Current selection
    void set_current_index(int index) {
        if (m_current_index == index) return;

        m_current_index = index;
        update_button_text();
        current_index_changed.emit(index);
    }

    [[nodiscard]] int current_index() const noexcept { return m_current_index; }

    [[nodiscard]] model_index current_model_index() const {
        if (!m_model || m_current_index < 0) return {};
        return m_model->index(m_current_index, 0);
    }

    // Popup control
    void show_popup() {
        if (m_popup_layer) return;  // Already open

        // Add popup to floating layer
        auto& layer_mgr = ui_context<Backend>::instance().layers();
        m_popup_layer = layer_mgr.create_layer(
            m_popup_list.get(),
            layer_type::popup,
            nullptr,  // No click-outside handler (we handle it)
            calculate_popup_position(),
            calculate_popup_size()
        );

        // Highlight current selection
        if (m_current_index >= 0) {
            m_popup_list->set_current_index(current_model_index());
        }

        m_is_open = true;
        this->mark_dirty();
    }

    void close_popup() {
        if (!m_popup_layer) return;

        m_popup_layer.reset();  // RAII cleanup
        m_is_open = false;
        this->mark_dirty();
    }

    void toggle_popup() {
        if (m_is_open) {
            close_popup();
        } else {
            show_popup();
        }
    }

    // Editable mode (future: when text_edit exists)
    void set_editable(bool editable) {
        m_is_editable = editable;
        // TODO: Replace button with text_edit when editable
    }

    [[nodiscard]] bool is_editable() const noexcept { return m_is_editable; }

    // Signals
    signal<int> current_index_changed;   // Emitted when selection changes
    signal<const std::string&> current_text_changed;

protected:
    void do_render(render_context<Backend>& ctx) const override {
        // Button renders itself (shows current selection)
        // Popup list is in separate layer (renders separately)
    }

    bool handle_event(const ui_event& event, event_phase phase) override {
        if (phase != event_phase::target) {
            return widget<Backend>::handle_event(event, phase);
        }

        // Close popup if user clicks outside
        if (m_is_open && event.type == ui_event::event_type::mouse_press) {
            if (!is_inside(event.x, event.y) && !m_popup_list->is_inside(event.x, event.y)) {
                close_popup();
                return true;
            }
        }

        // Keyboard navigation when closed
        if (!m_is_open && event.type == ui_event::event_type::key_press) {
            if (event.key_code == key_code::down || event.key_code == key_code::up) {
                show_popup();
                return true;
            }
        }

        return widget<Backend>::handle_event(event, phase);
    }

private:
    std::shared_ptr<model_type> m_model;
    button<Backend>* m_button = nullptr;  // Owned by children
    std::unique_ptr<list_view<Backend>> m_popup_list;  // Owned directly (not in tree when closed)
    scoped_layer<Backend> m_popup_layer;  // RAII popup management
    int m_current_index = -1;
    bool m_is_open = false;
    bool m_is_editable = false;

    void update_button_text() {
        if (!m_model || m_current_index < 0) {
            m_button->set_text("");
            return;
        }

        model_index index = current_model_index();
        auto display_data = m_model->data(index, item_data_role::display);
        if (std::holds_alternative<std::string>(display_data)) {
            std::string text = std::get<std::string>(display_data);
            m_button->set_text(text);
            current_text_changed.emit(text);
        }
    }

    void select_item(int index) {
        set_current_index(index);
    }

    typename Backend::rect_type calculate_popup_position() const {
        // Position popup directly below button
        rect_type button_bounds = m_button->bounds();
        int x = rect_utils::get_x(button_bounds);
        int y = rect_utils::get_y(button_bounds) + rect_utils::get_height(button_bounds);
        return rect_utils::make_rect(x, y, 0, 0);
    }

    typename Backend::size_type calculate_popup_size() const {
        // Popup width matches button, height based on items (max 10 visible)
        int width = rect_utils::get_width(m_button->bounds());
        int item_height = 20;  // From list_view
        int visible_items = std::min(10, m_model ? m_model->row_count() : 0);
        int height = visible_items * item_height;
        return {width, height};
    }
};
```

### Usage Example

```cpp
// Create model
auto model = std::make_shared<list_model<std::string, Backend>>();
model->set_items({"Red", "Green", "Blue", "Yellow"});

// Create combo box
auto combo = std::make_unique<combo_box<Backend>>();
combo->set_model(model);
combo->set_current_index(0);  // Select "Red"

// Handle selection changes
combo->current_index_changed.connect([](int index) {
    std::cout << "Selected index: " << index << "\n";
});

combo->current_text_changed.connect([](const std::string& text) {
    std::cout << "Selected: " << text << "\n";
});

root->add_child(std::move(combo));
```

**Phase 2 Implementation**:
1. Create `combo_box<Backend>` widget
2. Use existing `button` and `list_view` widgets
3. Use `scoped_layer` for popup management
4. Test with keyboard and mouse interaction
5. Add to widgets demo

---

## Design Decisions

### ✅ Confirmed Features (This Release)

1. **Combo Box Widget** - Dropdown selection using models
   - Will be implemented alongside list_view and table_view
   - Uses popup list_view for dropdown
   - See Phase 2 implementation plan

2. **Scrollable Integration** - Views work inside scroll_view
   - **REQUIRED** for large datasets
   - Views implement scrollable interface
   - Automatic scrollbar management
   - See "Scrollable Integration" section below

3. **Pagination** - Load data in chunks for very large datasets
   - **REQUIRED** for datasets with 10k+ items
   - Model supports page-based loading
   - Views request pages on-demand
   - See "Pagination Design" section below

### 🔮 Future Features (Next Release)

1. **Editing Support** - In-place editing (double-click to edit)
   - Design API now, implement when editor widgets exist
   - Blocked on: text_edit, spin_box, date_picker widgets
   - Delegates would create editor widgets
   - Views manage editor lifecycle

2. **Drag and Drop** - Reorder items via drag-drop
   - Deferred to next release
   - Would require drag-drop event handling
   - Model would need reorder operations

3. **Performance Optimization** - Advanced caching and dirty tracking
   - Deferred to next release
   - Focus on correctness first, optimize later
   - Target: 60 FPS with 10k visible items

### ❌ Non-Requirements

1. **Thread Safety** - Multi-threaded model access not required
   - Models accessed from UI thread only
   - Simplifies design (no mutexes needed)
   - Background data loading can use separate models, then swap on UI thread

---

## Next Steps

### Design Finalized ✅

Based on your feedback, the MVC design is complete with:
- **Combo box** added to Phase 2
- **Scrollable integration** required in Phase 3
- **Pagination** required in Phase 5
- **Editing support** deferred (blocked on editor widgets)
- **Drag and drop** deferred to next release
- **Performance optimization** deferred to next release
- **Thread safety** not required

### Ready to Implement

**Option 1: Start Phase 1 Now**
- Begin implementing `list_model<T, Backend>` and `list_view<Backend>`
- Create basic delegate system
- Write comprehensive tests
- Timeline: 2 weeks

**Option 2: Prototype First**
- Create minimal working prototype (single file, no tests)
- Validate design decisions with real code
- Refine API based on prototype experience
- Then start Phase 1 properly

**Option 3: Refine Design Further**
- Discuss specific use cases you have in mind
- Prototype delegate API with example code
- Design editing API (for future implementation)

**Recommended**: Start with Phase 1 implementation using TDD approach. The design is comprehensive and ready for implementation.
