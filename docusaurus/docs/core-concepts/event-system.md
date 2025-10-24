---
sidebar_position: 4
---

# The Event System

OnyxUI provides a flexible and powerful event system that allows you to respond to user input and other important events in your application. The system is composed of two complementary mechanisms:

1.  **The `event_target` Observer Pattern:** A classic, inheritance-based approach for handling low-level events like mouse clicks and key presses.
2.  **The Signal/Slot System:** A modern, publish-subscribe mechanism for creating high-level, custom events.

Understanding both of these systems is key to building interactive and responsive UIs with OnyxUI.

## The `event_target` Observer Pattern

The `event_target` class is a base class that all `ui_element`s inherit from. It provides a set of virtual methods that you can override to handle low-level UI events.

Here's a simplified look at the `event_target` interface:

```cpp
template<UIBackend Backend>
class event_target {
public:
    virtual void on_mouse_down(const typename Backend::event_type& e);
    virtual void on_mouse_up(const typename Backend::event_type& e);
    virtual void on_mouse_move(const typename Backend::event_type& e);
    virtual void on_key_down(const typename Backend::event_type& e);
    // ... and so on
};
```

To handle an event, you simply create a custom widget that inherits from `ui_element` (or one of its subclasses) and override the appropriate `on_...` method.

```cpp
template<UIBackend Backend>
class my_button : public onyxui::button<Backend> {
public:
    void on_mouse_down(const typename Backend::event_type& e) override {
        // Handle the mouse down event here
        std::cout << "Mouse down!" << std::endl;

        // It's often a good idea to call the base class implementation
        onyxui::button<Backend>::on_mouse_down(e);
    }
};
```

### When to use `event_target`

The `event_target` system is best suited for:

-   **Handling low-level input:** When you need to respond directly to mouse clicks, key presses, etc.
-   **Implementing widget logic:** It's the primary mechanism used internally by the OnyxUI widgets to implement their behavior.
-   **When you need to modify or consume an event:** The `on_...` methods can be used to intercept an event and prevent it from propagating further up the UI tree.

## The Signal/Slot System

The signal/slot system is a more modern and flexible way to handle events. It's based on the publish-subscribe pattern, where an object (the "signal") can emit an event, and any number of other objects (the "slots") can subscribe to that event.

Here's how it works:

1.  **Define a signal:** A signal is a member variable of a class. It's defined using the `onyxui::signal` template.

    ```cpp
    #include <onyxui/signal.hh>

    class my_button {
    public:
        onyxui::signal<> clicked; // A signal with no arguments
        onyxui::signal<int> value_changed; // A signal with an int argument
    };
    ```

2.  **Connect a slot:** A slot is a function, lambda, or member function that will be called when the signal is emitted. You connect a slot to a signal using the `connect()` method.

    ```cpp
    my_button button;
    button.clicked.connect([]() {
        std::cout << "Button was clicked!" << std::endl;
    });

    button.value_changed.connect([](int new_value) {
        std::cout << "Value changed to: " << new_value << std::endl;
    });
    ```

3.  **Emit the signal:** To trigger the event, you call the `emit()` method on the signal.

    ```cpp
    // This will call all connected slots
    button.clicked.emit();
    button.value_changed.emit(42);
    ```

### `scoped_connection` for Automatic Cleanup

The `connect()` method returns a connection ID that can be used to disconnect the slot later. However, a more convenient and safer way to manage the lifetime of a connection is to use the `onyxui::scoped_connection` class. This is an RAII wrapper that automatically disconnects the slot when it goes out of scope.

```cpp
class my_dialog {
    my_button m_ok_button;
    onyxui::scoped_connection m_ok_connection;

public:
    my_dialog() {
        // The connection is automatically disconnected when the dialog is destroyed
        m_ok_connection = onyxui::scoped_connection(
            m_ok_button.clicked,
            [this]() { this->on_ok(); }
        );
    }

    void on_ok() { /* ... */ }
};
```

### When to use Signals and Slots

The signal/slot system is ideal for:

-   **High-level, custom events:** When you want to create your own events that are specific to your application's logic (e.g., `document_saved`, `user_logged_in`).
-   **Decoupling components:** Signals and slots allow you to create components that can communicate with each other without having direct dependencies.
-   **One-to-many communication:** When you need to notify multiple objects that an event has occurred.

## Conclusion

OnyxUI's dual event systems provide a comprehensive and flexible solution for handling events in your application. By understanding the strengths of both the `event_target` and signal/slot systems, you can choose the right tool for the job and build UIs that are both interactive and well-structured.
