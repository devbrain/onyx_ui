---
sidebar_position: 7
---

# Focus Manager

The `FocusManager` is a crucial service in `onyxui` responsible for handling keyboard focus within the UI. It determines which widget receives keyboard events at any given time, and manages the focus traversal (e.g., moving focus with the Tab key).

## Theory of Operation

The `FocusManager` maintains a stack of focus scopes and, within each scope, a list of focusable widgets. When a key event occurs, the `FocusManager` directs it to the currently focused widget.

### Focus Scopes

Focus scopes are typically associated with layers or windows. When a new layer is created (e.g., a modal dialog), a new focus scope is pushed onto the stack. This ensures that focus is contained within the top-most layer.

### Focus Traversal

The `FocusManager` implements the logic for moving focus between widgets. This includes:

- **Tab Traversal:** Moving to the next or previous focusable widget using the `Tab` and `Shift+Tab` keys.
- **Directional Navigation:** (If applicable) Using arrow keys to navigate between widgets in a grid or other structured layout.

## Key Concepts

- **Focusable:** A widget must be explicitly marked as "focusable" to be included in the focus traversal.
- **Focus Policy:** A widget can have a focus policy that determines how it receives focus (e.g., by click, by tab, or both).
- **Focus Ring:** The visual indication of which widget currently has focus (e.g., a border or an outline).

## Usage

You usually don't need to interact with the `FocusManager` directly. It works automatically behind the scenes. However, you can programmatically set focus to a widget if needed.

```cpp
#include <onyxui/ui_context.hh>
#include <onyxui/focus_manager.hh>
#include <onyxui/widgets/button.hh>

void set_focus_to_button(onyxui::ui_context& ctx, onyxui::Button& button) {
    auto& focus_manager = ctx.get<onyxui::FocusManager>();
    focus_manager.set_focus(&button);
}
