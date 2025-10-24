---
sidebar_position: 9
---

# Hotkeys Manager

The `HotkeysManager` provides a way to define and handle global keyboard shortcuts, often referred to as hotkeys. These are key combinations that trigger actions regardless of which widget has focus.

## Theory of Operation

The `HotkeysManager` maintains a registry of hotkeys and their associated actions. When a keyboard event is received, it's first processed by the `HotkeysManager` before being passed to the `FocusManager`. If the key combination matches a registered hotkey, the corresponding action is executed, and the event is typically consumed.

### Hotkey Scopes

Hotkeys can be registered globally or within a specific scope (e.g., a window or a specific part of the UI). This allows you to have different sets of hotkeys active depending on the context.

### Action Binding

Hotkeys are bound to actions, which are typically functions or methods that perform a specific task. `onyxui` uses a signal and slot mechanism to connect hotkeys to actions.

## Usage

To use the `HotkeysManager`, you first need to define a hotkey and then connect it to a slot (a function or a method).

```cpp
#include <onyxui/ui_context.hh>
#include <onyxui/hotkeys/hotkey_manager.hh>
#include <onyxui/hotkeys/hotkey.hh>

class MyWindow {
public:
    MyWindow(onyxui::ui_context& ctx) : _ctx(ctx) {
        auto& hotkeys_manager = _ctx.get<onyxui::HotkeysManager>();
        
        // Register Ctrl+S to call the on_save method.
        hotkeys_manager.add_hotkey(
            onyxui::Hotkey(onyxui::Key::S, onyxui::Modifier::Control),
            [this]() { on_save(); }
        );
    }

    void on_save() {
        // ... save the document ...
    }

private:
    onyxui::ui_context& _ctx;
};
