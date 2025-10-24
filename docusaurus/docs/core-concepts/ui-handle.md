---
sidebar_position: 11
---

# UI Handle

The `ui_handle` is a non-owning, lightweight reference to a UI element. It provides a safe and convenient way to interact with widgets and other UI elements without needing to manage their lifetime. It's similar to a raw pointer, but with some added safety and convenience features.

## Theory of Operation

Every UI element in `onyxui` has a stable `ui_handle`. This handle remains valid even if the element is moved in the UI tree. The `ui_handle` provides access to the element's properties and methods, as well as to the `ui_context`.

### Lifetime Management

The `ui_handle` does not own the UI element. The element's lifetime is managed by its parent in the UI tree. If the element is destroyed, any existing `ui_handle`s that refer to it will become invalid. Accessing an invalid handle will result in a crash, so it's important to ensure that the handle is valid before using it.

## Usage

You get a `ui_handle` when you create a widget or add it to a parent. You can then use this handle to interact with the widget.

```cpp
#include <onyxui/ui_handle.hh>
#include <onyxui/widgets/panel.hh>
#include <onyxui/widgets/button.hh>

void setup_ui(onyxui::Panel& parent) {
    auto button_handle = parent.add_child(onyxui::Button::create("Click me"));

    // You can now use the handle to interact with the button.
    button_handle->set_text("New Text");

    // You can also get the raw pointer to the widget if needed.
    onyxui::Button* button_ptr = button_handle.get();
}
```

The `ui_handle` also provides access to the `ui_context`.

```cpp
#include <onyxui/ui_handle.hh>
#include <onyxui/ui_context.hh>
#include <onyxui/focus_manager.hh>

void set_focus_to_widget(onyxui::ui_handle& handle) {
    auto& focus_manager = handle.context().get<onyxui::FocusManager>();
    focus_manager.set_focus(handle.get());
}
