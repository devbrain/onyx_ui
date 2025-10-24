---
sidebar_position: 6
---

# Service Locator

The Service Locator is a design pattern used in `onyxui` to provide global access to various UI-related services without coupling the code to concrete implementations. This pattern is central to the framework's architecture, promoting loose coupling and improved testability.

## Theory of Operation

The core idea behind the Service Locator is to have a central registry where services are registered and can be retrieved from anywhere in the application. This avoids the need to pass service instances through multiple layers of the application or rely on singletons.

In `onyxui`, the `ui_context` acts as the service locator. It holds instances of all major services, such as the `FocusManager`, `LayerManager`, `HotkeysManager`, and others.

## Key Benefits

- **Decoupling:** Components that need a service don't need to know how to create it or what its concrete type is. They only need to know the interface of the service.
- **Testability:** During testing, you can register mock or stub implementations of services, allowing you to test components in isolation.
- **Flexibility:** It's easy to replace a service implementation with another one without changing the code that uses the service.

## Usage

Typically, you don't interact with the service locator directly. Instead, you access services through the `ui_handle` or `ui_context`, which is available to all widgets and UI components.

```cpp
#include <onyxui/ui_context.hh>
#include <onyxui/focus_manager.hh>

void set_focus_to_next(onyxui::ui_context& ctx) {
    auto& focus_manager = ctx.get<onyxui::FocusManager>();
    focus_manager.focus_next();
}
