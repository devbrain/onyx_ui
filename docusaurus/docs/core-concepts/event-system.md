---
sidebar_position: 4
---

# The Event System

OnyxUI provides a flexible and powerful event system that allows you to respond to user input and other important events in your application. The system is composed of two complementary mechanisms:

1.  **Three-Phase Event Routing:** Industry-standard capture/target/bubble event propagation (DOM/WPF model)
2.  **The Signal/Slot System:** A modern, publish-subscribe mechanism for decoupled communication

Understanding both of these systems is key to building interactive and responsive UIs with OnyxUI.

## Three-Phase Event Routing

OnyxUI implements the industry-standard three-phase event routing model used in DOM (web) and WPF (Windows). Events propagate through three distinct phases:

1. **CAPTURE** - Event travels DOWN from root to target (parent intercepts BEFORE children)
2. **TARGET** - Event delivered to target element (the element at the click/interaction point)
3. **BUBBLE** - Event travels UP from target to root (child handles BEFORE parent)

### Example Event Flow

When a user clicks on a label nested inside a text_view:

```
Widget Tree:
  root
    └─ panel
         └─ text_view
              └─ scroll_view
                   └─ label  ← clicked here

Event Routing:
  Phase 1 (CAPTURE): root → panel → text_view → scroll_view
  Phase 2 (TARGET):  label
  Phase 3 (BUBBLE):  scroll_view → text_view → panel → root
```

Any handler can **stop propagation** by returning `true`.

### Handling Events by Phase

Override `handle_event` with phase awareness:

```cpp
#include <onyxui/events/event_phase.hh>

template<UIBackend Backend>
class my_composite_widget : public onyxui::widget<Backend> {
public:
    bool handle_event(const onyxui::ui_event& evt, onyxui::event_phase phase) override {
        // CAPTURE: Intercept before children
        if (phase == onyxui::event_phase::capture) {
            if (auto* mouse = std::get_if<onyxui::mouse_event>(&evt)) {
                if (mouse->act == onyxui::mouse_event::action::press) {
                    this->request_focus();  // Focus composite, not child
                    return false;           // Let children handle too
                }
            }
        }

        // TARGET: Handle if this widget is the target
        if (phase == onyxui::event_phase::target) {
            // Process event normally
            return this->base::handle_event(evt, phase);
        }

        // BUBBLE: Cleanup or logging after children
        if (phase == onyxui::event_phase::bubble) {
            log_interaction(evt);
            return false;  // Don't consume
        }

        return false;
    }
};
```

### When to Use Each Phase

#### CAPTURE Phase
Best for:
- **Composite widgets requesting focus** (text_view, custom input controls)
- **Input validation** before children process events
- **Event filtering or transformation**
- **Preventing child interaction** in disabled states

Example: text_view captures mouse press to request focus before any child label handles the click.

#### TARGET Phase
Best for:
- **Most widget interactions** (button clicks, text input)
- **Direct event handling**
- **Simple widgets** that don't need special routing

This is the **default phase** for widgets that don't override special behavior.

#### BUBBLE Phase
Best for:
- **Event logging and analytics**
- **Cleanup** after child processing
- **Event delegation** patterns
- **Parent notification** after child handling

Example: Panel logs all child interactions by handling bubble phase without consuming the event.

### Routing API

For advanced use cases, you can manually route events:

```cpp
#include <onyxui/events/event_router.hh>
#include <onyxui/events/hit_test_path.hh>

// Perform hit test to build path from root to target
onyxui::hit_test_path<Backend> path;
auto* target = root->hit_test(x, y, path);

if (target) {
    // Route event through all three phases
    bool handled = onyxui::route_event(ui_evt, path);

    if (handled) {
        // Event was consumed by a handler
        std::cout << "Event handled!" << std::endl;
    }
}
```

### Common Patterns

#### Pattern 1: Composite Focus Management

```cpp
// text_view requests focus on any click, not just on deepest label
bool handle_event(const ui_event& evt, event_phase phase) override {
    if (phase == event_phase::capture) {
        if (is_mouse_press(evt)) {
            request_focus();
            return false;  // Continue to children
        }
    }
    return base::handle_event(evt, phase);
}
```

#### Pattern 2: Input Validation

```cpp
// Validate input before child processes it
bool handle_event(const ui_event& evt, event_phase phase) override {
    if (phase == event_phase::capture) {
        if (auto* kbd = std::get_if<keyboard_event>(&evt)) {
            if (!is_valid_char(kbd->key)) {
                return true;  // Stop propagation - invalid input
            }
        }
    }
    return base::handle_event(evt, phase);
}
```

#### Pattern 3: Event Logging

```cpp
// Log all child interactions without consuming events
bool handle_event(const ui_event& evt, event_phase phase) override {
    if (phase == event_phase::bubble) {
        analytics.log_interaction(evt);
        return false;  // Don't consume - let others handle
    }
    return base::handle_event(evt, phase);
}
```

## The Signal/Slot System

The signal/slot system is a more modern and flexible way to handle high-level events. It's based on the publish-subscribe pattern, where an object (the "signal") can emit an event, and any number of other objects (the "slots") can subscribe to that event.

### Basic Usage

1.  **Define a signal:** A signal is a member variable of a class defined using the `onyxui::signal` template.

    ```cpp
    #include <onyxui/core/signal.hh>

    class my_button {
    public:
        onyxui::signal<> clicked;         // Signal with no arguments
        onyxui::signal<int> value_changed; // Signal with an int argument
    };
    ```

2.  **Connect a slot:** A slot is a function, lambda, or member function that will be called when the signal is emitted. Connect using `connect()`.

    ```cpp
    my_button button;
    button.clicked.connect([]() {
        std::cout << "Button was clicked!" << std::endl;
    });

    button.value_changed.connect([](int new_value) {
        std::cout << "Value changed to: " << new_value << std::endl;
    });
    ```

3.  **Emit the signal:** Trigger the event by calling `emit()` on the signal.

    ```cpp
    // This will call all connected slots
    button.clicked.emit();
    button.value_changed.emit(42);
    ```

### `scoped_connection` for Automatic Cleanup

The `scoped_connection` class provides RAII-based automatic disconnection:

```cpp
#include <onyxui/core/signal.hh>

class my_dialog {
    my_button m_ok_button;
    onyxui::scoped_connection m_ok_connection;

public:
    my_dialog() {
        // Connection automatically disconnected when dialog is destroyed
        m_ok_connection = onyxui::scoped_connection(
            m_ok_button.clicked,
            [this]() { this->on_ok(); }
        );
    }

    void on_ok() { /* ... */ }
};
```

### Common Widget Signals

Built-in widgets provide standard signals:

- **button::clicked** - Emitted when button is activated
- **text_input::text_changed** - Emitted when text changes
- **menu_item::triggered** - Emitted when menu item selected

### When to Use Signals and Slots

The signal/slot system is ideal for:

-   **High-level, custom events:** Application-specific events (e.g., `document_saved`, `user_logged_in`)
-   **Decoupling components:** Communicate without direct dependencies
-   **One-to-many communication:** Notify multiple objects of an event
-   **Widget communication:** Connect widgets together declaratively

## Choosing the Right System

Use **three-phase event routing** when:
- Handling low-level input (mouse, keyboard)
- Implementing widget behavior
- Need to intercept events before/after children
- Building composite widgets with focus management

Use **signals/slots** when:
- Creating high-level application events
- Connecting widgets together
- Decoupling application components
- Broadcasting state changes

## Advanced Topics

### Event Types

OnyxUI supports several event types wrapped in `ui_event`:

```cpp
#include <onyxui/events/ui_event.hh>

// Check event type and extract data
if (auto* mouse = std::get_if<onyxui::mouse_event>(&evt)) {
    // Handle mouse event
    if (mouse->act == onyxui::mouse_event::action::press) {
        // Mouse button pressed at (mouse->x, mouse->y)
    }
}

if (auto* kbd = std::get_if<onyxui::keyboard_event>(&evt)) {
    // Handle keyboard event
    if (kbd->pressed) {
        // Key pressed: kbd->key
    }
}
```

### Stopping Event Propagation

Return `true` from `handle_event()` to stop propagation:

```cpp
bool handle_event(const ui_event& evt, event_phase phase) override {
    if (phase == event_phase::capture) {
        if (should_block_event(evt)) {
            return true;  // Stop - children never see this event
        }
    }
    return false;  // Continue propagation
}
```

## Conclusion

OnyxUI's dual event systems provide a comprehensive and flexible solution for handling events in your application. The **three-phase routing** gives you fine-grained control over low-level input handling, while **signals/slots** enable clean, decoupled high-level communication. By understanding the strengths of both systems, you can build UIs that are both interactive and well-architected.

## See Also

- [Architecture Guide - Event System](/docs/CLAUDE/ARCHITECTURE.md#event-system)
- [Focus Manager](/docs/core-concepts/focus-manager.md)
- [Widget Development Guide](/docs/guides/widget-development.md)
