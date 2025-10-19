# Best Practices Guide

## Overview

This guide documents best practices for using the onyxui framework effectively and safely. Following these patterns will help you write robust, maintainable, and performant UI code.

## Exception Safety

### Exception Safety Guarantees

The framework provides different exception safety guarantees for different operations:

#### Strong Exception Safety

**Guarantee**: Operation succeeds completely or has no effect.

```cpp
// add_child() provides strong exception safety
void add_child(ui_element_ptr child) {
    if (child) {
        // If push_back throws, no state is modified
        m_children.push_back(std::move(child));
        // Only modify child's parent after successful insertion
        m_children.back()->m_parent = this;
        invalidate_measure();
    }
}
```

**Strong guarantee operations**:
- `add_child()`
- `set_layout_strategy()`
- Signal connections (`signal::connect()`)

#### Basic Exception Safety

**Guarantee**: Operation may partially succeed, but no resources are leaked and invariants are maintained.

```cpp
// remove_child() provides basic exception safety
ui_element_ptr remove_child(size_t index) {
    // Child is removed even if cleanup throws
    ui_element_ptr removed = std::move(m_children[index]);
    m_children.erase(m_children.begin() + index);
    removed->m_parent = nullptr;

    // If this throws, child is still safely removed
    if (m_layout_strategy) {
        m_layout_strategy->on_child_removed(removed.get());
    }
    return removed;
}
```

**Basic guarantee operations**:
- `remove_child()`
- `clear_children()`
- Layout calculations (if user code throws)

#### No-throw Guarantee

**Guarantee**: Operation never throws exceptions.

```cpp
// Destructor is noexcept
~ui_element() noexcept {
    for (auto& child : m_children) {
        if (child) {
            child->m_parent = nullptr;
        }
    }
}
```

**No-throw operations**:
- Destructors
- Move constructors (conditionally noexcept)
- Move assignment operators (conditionally noexcept)
- `set_visible()`, `set_enabled()`, `set_z_order()`

### Best Practices for Exception Safety

#### ✅ DO: Use RAII for Resource Management

```cpp
class my_widget : public widget<Backend> {
    std::unique_ptr<resource> m_resource;  // Automatic cleanup

public:
    my_widget() {
        m_resource = std::make_unique<resource>();
        // If constructor throws, m_resource is automatically cleaned up
    }

    // No manual cleanup needed - destructor handles it
};
```

#### ✅ DO: Use scoped_connection for Signal Lifetime

```cpp
class my_widget : public widget<Backend> {
    signal<> m_clicked;           // Signal declared first
    scoped_connection m_conn;     // Connection declared after

public:
    my_widget() {
        // Safe: m_conn destroyed before m_clicked
        m_conn = scoped_connection(m_clicked, []() {
            // Handler code
        });
    }
};
```

#### ❌ DON'T: Throw from Destructors

```cpp
// BAD: Throwing destructor
~my_widget() {
    cleanup();  // If this throws, std::terminate is called
}

// GOOD: Catch and handle exceptions
~my_widget() noexcept {
    try {
        cleanup();
    } catch (...) {
        // Log error, but don't throw
    }
}
```

#### ❌ DON'T: Throw from Move Operations

```cpp
// BAD: Move constructor that can throw
my_widget(my_widget&& other) {
    // If allocation throws, object is in moved-from state
    m_data = std::make_unique<data>(other.m_data->size());
}

// GOOD: Noexcept move constructor
my_widget(my_widget&& other) noexcept
    : m_data(std::move(other.m_data)) {
    // Simple moves, no allocations
}
```

## Memory Management

### Ownership Patterns

#### Use std::unique_ptr for Tree Ownership

```cpp
// The UI tree owns its children via std::unique_ptr
class ui_element {
    std::vector<std::unique_ptr<ui_element>> m_children;
};

// Add child (transfers ownership)
auto button = std::make_unique<button<Backend>>("Click");
parent->add_child(std::move(button));  // parent now owns button

// Remove child (returns ownership)
auto removed = parent->remove_child(0);  // Caller now owns removed element
```

#### Raw Pointers for Non-Owning References

```cpp
class my_widget : public widget<Backend> {
    ui_element<Backend>* m_parent;  // Non-owning (parent outlives child)
    button<Backend>* m_ok_button;   // Non-owning (for quick access)

public:
    my_widget() {
        auto btn = std::make_unique<button<Backend>>("OK");
        m_ok_button = btn.get();  // Save raw pointer before transferring
        add_child(std::move(btn));  // Transfer ownership
    }
};
```

#### ✅ DO: Transfer Ownership Explicitly

```cpp
// Clear ownership transfer
auto child = std::make_unique<label<Backend>>("Text");
parent->add_child(std::move(child));  // child is now nullptr
// Can't use child anymore - ownership transferred
```

#### ❌ DON'T: Use Shared Ownership Unless Necessary

```cpp
// BAD: Unclear ownership
std::shared_ptr<button<Backend>> button = std::make_shared<button<Backend>>();
parent->add_child(button);  // Who owns button? Parent or shared_ptr?

// GOOD: Clear exclusive ownership
auto button = std::make_unique<button<Backend>>();
parent->add_child(std::move(button));  // Parent owns button
```

### Memory Leak Prevention

#### ✅ DO: Use Scoped Connections

```cpp
class temporary_window : public widget<Backend> {
    signal<> m_closed;
    scoped_connection m_close_conn;  // Automatic disconnection

public:
    temporary_window() {
        m_close_conn = scoped_connection(m_closed, [this]() {
            on_close();
        });
    }
    // m_close_conn automatically disconnects when destroyed
};
```

#### ✅ DO: Clear Children When Rebuilding UI

```cpp
void rebuild_ui() {
    m_container->clear_children();  // Clean up old UI
    // Build new UI
    auto new_child = std::make_unique<label<Backend>>("New");
    m_container->add_child(std::move(new_child));
}
```

#### ❌ DON'T: Create Circular References

```cpp
// BAD: Circular reference (memory leak)
class parent_widget : public widget<Backend> {
    std::unique_ptr<child_widget> m_child;
};

class child_widget : public widget<Backend> {
    std::unique_ptr<parent_widget> m_parent;  // CIRCULAR!
};

// GOOD: Parent owns child, child has non-owning pointer
class child_widget : public widget<Backend> {
    parent_widget* m_parent;  // Non-owning
};
```

## Signal/Slot Best Practices

### Connection Lifetime Management

#### ✅ DO: Use scoped_connection for Member Functions

```cpp
class data_view : public widget<Backend> {
    data_model* m_model;
    scoped_connection m_data_changed_conn;

public:
    data_view(data_model* model) : m_model(model) {
        m_data_changed_conn = scoped_connection(
            m_model->data_changed(),
            [this](const data& d) { on_data_changed(d); }
        );
    }

    void on_data_changed(const data& d) {
        // Update view
    }
};
```

#### ✅ DO: Declare Signals Before Connections

```cpp
class widget {
    signal<> m_clicked;           // Declared first
    scoped_connection m_conn;     // Declared after

    // Destructor: m_conn destroyed first, then m_clicked
    // This is safe - m_conn can still disconnect
};
```

#### ❌ DON'T: Capture Raw 'this' Without Lifetime Guarantees

```cpp
// BAD: 'this' might be destroyed before signal emits
void register_handler() {
    global_signal.connect([this]() {
        this->on_event();  // Dangling 'this' if widget destroyed!
    });
}

// GOOD: Use scoped_connection or weak_ptr
class my_widget {
    scoped_connection m_conn;

public:
    void register_handler() {
        m_conn = scoped_connection(global_signal, [this]() {
            this->on_event();  // Safe: m_conn disconnects in destructor
        });
    }
};
```

### Signal Emission Patterns

#### ✅ DO: Emit After State Changes

```cpp
void set_value(int value) {
    if (m_value != value) {
        m_value = value;
        value_changed.emit(value);  // Emit after state is consistent
    }
}
```

#### ✅ DO: Use Const References for Large Arguments

```cpp
signal<const std::vector<data>&> batch_updated;  // Pass by const ref

// Emit without copying
std::vector<data> large_dataset = compute_data();
batch_updated.emit(large_dataset);  // No copy, just reference
```

#### ❌ DON'T: Emit During Object Construction

```cpp
// BAD: Object not fully constructed yet
class widget {
public:
    widget() {
        // Virtual table not fully set up, callbacks might fail
        initialized.emit();  // Dangerous!
    }
};

// GOOD: Separate initialization method
class widget {
public:
    widget() { /* basic setup */ }

    void initialize() {
        // Object fully constructed, safe to emit
        initialized.emit();
    }
};
```

## Layout Best Practices

### Hierarchy Design

#### ✅ DO: Keep Hierarchies Shallow

```cpp
// GOOD: 3 levels deep, clear structure
window
  ├─ vbox (main container)
  │   ├─ menu_bar
  │   ├─ content_area
  │   └─ status_bar
```

```cpp
// BAD: 8 levels deep, unnecessary nesting
window
  └─ panel
      └─ vbox
          └─ panel
              └─ hbox
                  └─ panel
                      └─ vbox
                          └─ button  // Finally!
```

**Target**: Keep depth under 15 levels for optimal performance.

#### ✅ DO: Use Appropriate Layout Strategies

```cpp
// For forms: Use grid_layout
auto form = std::make_unique<panel<Backend>>();
form->set_layout_strategy(std::make_unique<grid_layout<Backend>>(2, 3));

// For lists: Use linear_layout (vertical)
auto list = std::make_unique<vbox<Backend>>();
// Children stack vertically automatically

// For overlays: Use absolute_layout
auto overlay = std::make_unique<panel<Backend>>();
overlay->set_layout_strategy(std::make_unique<absolute_layout<Backend>>());
```

### Invalidation Patterns

#### ✅ DO: Batch Property Changes

```cpp
// GOOD: Single invalidation
void configure_button() {
    m_updating = true;  // Optional flag
    m_button->set_width(100);
    m_button->set_height(50);
    m_button->set_margin({10});
    m_updating = false;
    m_button->invalidate_measure();  // One invalidation for all changes
}
```

#### ✅ DO: Only Invalidate What Changed

```cpp
void set_background_color(color_type color) {
    if (m_background != color) {
        m_background = color;
        // Color change doesn't affect layout
        invalidate_render();  // NOT invalidate_measure()
    }
}

void set_width(int width) {
    if (m_width != width) {
        m_width = width;
        // Size change affects layout
        invalidate_measure();  // Measure needs recalculation
    }
}
```

#### ❌ DON'T: Invalidate in Hot Paths

```cpp
// BAD: Invalidation on every mouse move
void on_mouse_move(int x, int y) {
    m_hover_position = {x, y};
    invalidate_measure();  // Expensive! Don't do this
}

// GOOD: Only invalidate if layout actually changes
void on_mouse_move(int x, int y) {
    m_hover_position = {x, y};
    invalidate_render();  // Visual-only change
}
```

### Size Constraint Patterns

#### ✅ DO: Use Semantic Size Policies

```cpp
// Content-sized label (shrinks to fit text)
label->set_width_constraint({size_policy::content});

// Fixed-size button (always 100px wide)
button->set_width_constraint({size_policy::fixed, 100});

// Expanding panel (fills available space)
panel->set_width_constraint({size_policy::expand});
```

#### ✅ DO: Specify Min/Max Constraints

```cpp
// Resizable with limits
textbox->set_width_constraint({
    size_policy::expand,  // Grow to fill space
    100,                  // Preferred: 100px
    50,                   // Min: 50px
    300                   // Max: 300px
});
```

## Recursion and Stack Safety

### Deep Hierarchy Considerations

The framework uses recursion for tree traversal. While tested to 100 levels, best practices help avoid stack issues:

#### ✅ DO: Keep UI Hierarchies Reasonable

```cpp
// GOOD: Typical depth 5-15 levels
window -> panel -> vbox -> form -> fields (3-5 levels of form controls)
```

#### ✅ DO: Flatten When Possible

```cpp
// Instead of deep nesting:
vbox -> panel -> vbox -> panel -> vbox -> items

// Use direct children:
vbox -> items (all direct children)
```

#### ⚠️ CAUTION: Very Deep Hierarchies

If you must support >100 levels (rare):
- Increase thread stack size via platform APIs
- Profile stack usage in stress tests
- Consider iterative algorithms for custom traversal

**Typical stack usage**:
- Each recursion frame: ~100-200 bytes
- 100 levels: ~10-20 KB stack
- 1000 levels: ~100-200 KB stack
- Default stack size: 1-8 MB (platform dependent)

## Threading Best Practices

### UI Thread Model

#### ✅ DO: Perform UI Operations on Main Thread

```cpp
// GOOD: UI update on main thread
void on_background_work_complete(const result& r) {
    app.queue_ui_task([this, r]() {
        m_status_label->set_text("Complete: " + r.message);
        m_progress_bar->set_value(100);
    });
}
```

#### ✅ DO: Use Signals for Cross-Thread Communication

```cpp
class background_worker {
    signal<std::string> m_status_changed;  // Thread-safe signal

public:
    void work() {
        std::thread([this]() {
            // Background thread
            auto result = do_work();
            m_status_changed.emit(result);  // Safe: signal is thread-safe
        }).detach();
    }

    signal<std::string>& status_changed() { return m_status_changed; }
};

// In UI code (main thread)
worker.status_changed().connect([this](const std::string& status) {
    // This runs on background thread!
    // Queue for UI thread
    app.queue_ui_task([this, status]() {
        m_label->set_text(status);  // Now safe on UI thread
    });
});
```

#### ❌ DON'T: Modify UI from Background Threads

```cpp
// BAD: UI modification from background thread
std::thread worker([&label]() {
    label->set_text("Done");  // CRASH! Not thread-safe
});

// GOOD: Queue for UI thread
std::thread worker([&app, &label]() {
    app.queue_ui_task([&label]() {
        label->set_text("Done");  // Safe on UI thread
    });
});
```

### Signal Thread Safety

#### ✅ DO: Enable Thread Safety for Multi-Threaded Apps

```cmake
# CMakeLists.txt
option(ONYXUI_THREAD_SAFE "Enable thread-safe signals" ON)  # Default
```

#### ✅ DO: Be Aware of Callback Thread

```cpp
signal<int> value_changed;

// Background thread emits
std::thread worker([&]() {
    value_changed.emit(42);  // Safe: signal is thread-safe
});

// Callback runs on EMITTING thread (background thread)
value_changed.connect([](int value) {
    // WARNING: This runs on background thread!
    // Don't modify UI here
});
```

## Code Style and Conventions

### Naming Conventions

```cpp
// Classes: lower_case
class my_custom_widget : public widget<Backend> { };

// Functions: lower_case
void process_events();

// Member variables: m_ prefix
class widget {
    int m_width;
    color_type m_background;
};

// Constants: UPPER_CASE or constexpr with clear name
static constexpr int MAX_DEPTH = 100;
constexpr int default_button_width = 80;

// Template parameters: PascalCase
template<UIBackend Backend, typename ValueType>
class my_template { };
```

### Documentation

#### ✅ DO: Document Public APIs

```cpp
/**
 * @brief Sets the element's width constraint.
 *
 * @param constraint The new width constraint (policy, preferred, min, max)
 *
 * @thread_safety Safe to call from any thread (invalidation queued to UI thread)
 * @exception_safety Strong guarantee - no effect if invalidation fails
 *
 * @see size_constraint, set_height_constraint
 */
void set_width_constraint(size_constraint constraint);
```

#### ✅ DO: Document Thread Safety

```cpp
/**
 * @class signal
 * @thread_safety Thread-safe when ONYXUI_THREAD_SAFE is enabled.
 *                All operations protected by std::shared_mutex.
 * @performance Mutex overhead: ~20-50ns per emission with thread safety.
 */
template<typename... Args>
class signal { };
```

### Type Aliases

```cpp
template<UIBackend Backend>
class my_widget : public widget<Backend> {
    // Use type aliases for clarity
    using base = widget<Backend>;
    using element_type = ui_element<Backend>;
    using size_type = typename Backend::size_type;
    using color_type = typename Backend::color_type;

    // Now use clear names
    size_type calculate_size() const;
    color_type get_background() const;
};
```

## Testing Best Practices

### Write Comprehensive Tests

```cpp
TEST_CASE("Button - Click handling") {
    SUBCASE("Enabled button emits clicked signal") {
        auto button = std::make_unique<button<TestBackend>>("Click");
        bool clicked = false;
        button->clicked().connect([&]() { clicked = true; });

        // Simulate click
        button->on_mouse_down(make_mouse_event(10, 10));
        button->on_mouse_up(make_mouse_event(10, 10));

        CHECK(clicked);
    }

    SUBCASE("Disabled button doesn't emit clicked signal") {
        auto button = std::make_unique<button<TestBackend>>("Click");
        button->set_enabled(false);

        bool clicked = false;
        button->clicked().connect([&]() { clicked = true; });

        button->on_mouse_down(make_mouse_event(10, 10));
        button->on_mouse_up(make_mouse_event(10, 10));

        CHECK_FALSE(clicked);
    }
}
```

### Test Exception Safety

```cpp
TEST_CASE("Exception safety - add_child with throwing move") {
    auto parent = std::make_unique<TestElement>();
    size_t initial_count = parent->child_count();

    // Simulate exception during add
    try {
        auto child = std::make_unique<ThrowingElement>();
        parent->add_child(std::move(child));
        FAIL("Should have thrown");
    } catch (...) {
        // Verify no children were added
        CHECK(parent->child_count() == initial_count);
    }
}
```

## Summary Checklist

### Exception Safety
- [ ] Use RAII for all resources
- [ ] Never throw from destructors
- [ ] Make move operations noexcept
- [ ] Use scoped_connection for signal lifetime

### Memory Management
- [ ] Use std::unique_ptr for ownership
- [ ] Use raw pointers for non-owning references
- [ ] Avoid circular references
- [ ] Clear children when rebuilding UI

### Signals
- [ ] Declare signals before scoped_connections
- [ ] Use const references for large arguments
- [ ] Don't emit during construction
- [ ] Be aware of callback thread in multi-threaded apps

### Layout
- [ ] Keep hierarchy depth under 15 levels
- [ ] Batch property changes
- [ ] Only invalidate what changed
- [ ] Use appropriate size policies

### Threading
- [ ] Enable ONYXUI_THREAD_SAFE for multi-threaded apps
- [ ] Keep UI operations on main thread
- [ ] Use message queue for cross-thread updates
- [ ] Be aware of which thread callbacks run on

### Code Quality
- [ ] Follow naming conventions
- [ ] Document public APIs
- [ ] Write comprehensive tests
- [ ] Profile before optimizing

Following these best practices will help you build robust, maintainable, and performant UIs with onyxui.
