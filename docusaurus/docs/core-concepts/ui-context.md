---
sidebar_position: 10
---

# UI Context

The `ui_context` is the heart of an `onyxui` application. It's a central object that owns and manages all the global UI services. It acts as a service locator, providing access to services like the `FocusManager`, `LayerManager`, `HotkeysManager`, and the theme engine.

## Theory of Operation

An application will typically create a single `ui_context` instance at startup. This instance is then passed to all parts of the UI that need access to global services.

The `ui_context` is responsible for:

- **Service Lifetime:** It owns the service instances, ensuring they are created at startup and destroyed at shutdown in the correct order.
- **Service Access:** It provides a `get<T>()` method to retrieve a reference to a service of type `T`.
- **Event Loop Integration:** It integrates with the application's main event loop, processing events and updating the UI.

## Usage

You create a `ui_context` by providing it with a backend instance. The backend provides the bridge to the underlying windowing and rendering system (e.g., SDL2, conio).

```cpp
#include <onyxui/ui_context.hh>
#include <onyxui/backends/sdl2/sdl2_backend.hh>

int main() {
    auto backend = onyxui::Sdl2Backend::create();
    onyxui::ui_context ctx(backend.get());

    // ... create widgets and build the UI ...

    return ctx.run(); // Starts the main event loop.
}
```

Within your UI code, you can get access to the `ui_context` from any `ui_handle`.

```cpp
#include <onyxui/ui_handle.hh>
#include <onyxui/ui_context.hh>

void do_something(onyxui::ui_handle& handle) {
    onyxui::ui_context& ctx = handle.context();
    // ... use the context to access services ...
}
