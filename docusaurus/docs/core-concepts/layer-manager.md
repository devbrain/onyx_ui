---
sidebar_position: 8
---

# Layer Manager

The `LayerManager` is responsible for managing the stacking order of UI elements, particularly for pop-ups, dialogs, and other elements that appear on top of the main UI. It ensures that events are correctly routed to the top-most layer and that rendering happens in the correct order.

## Theory of Operation

The `LayerManager` maintains a stack of layers. Each layer is a container for UI elements. The top-most layer in the stack is considered the "active" layer and receives all input events.

### Layer Types

`onyxui` supports several types of layers:

- **Main Layer:** The base layer that contains the main application UI.
- **Popup Layer:** Used for transient elements like menus and tooltips.
- **Modal Layer:** Used for dialogs that block interaction with the rest of the UI.

### Event Handling

When an event occurs, the `LayerManager` dispatches it to the top-most layer. If the event is not handled by the top-most layer, it may be passed down to lower layers, depending on the layer's configuration. For modal layers, events are typically not passed down.

## Usage

The `LayerManager` is mostly used internally by widgets like `Menu`, `Dialog`, and `Tooltip`. However, you can use it to create custom layered UI.

The `scoped_layer` is a convenient RAII-style helper for managing the lifetime of a layer.

```cpp
#include <onyxui/ui_context.hh>
#include <onyxui/layer_manager.hh>
#include <onyxui/scoped_layer.hh>
#include <onyxui/widgets/panel.hh>

void show_popup(onyxui::ui_context& ctx) {
    auto panel = onyxui::Panel::create();
    // ... configure panel ...

    // The layer is automatically removed when popup_layer goes out of scope.
    onyxui::scoped_layer popup_layer(ctx, panel.get());
    
    // ... interact with the popup ...
}
