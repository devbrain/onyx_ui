# Backend Pattern Migration Guide

This guide explains how to migrate from the old multi-parameter template approach to the new unified Backend pattern in onyx_ui.

## Overview of Changes

The library has been refactored to use a single Backend template parameter instead of multiple parameters (TRect, TSize, TEvent, etc.). This simplifies the API and ensures type consistency across the entire system.

### Old Pattern (Before)
```cpp
// Multiple template parameters everywhere
template<RectLike TRect, SizeLike TSize>
class ui_element { ... };

template<RectLike TRect, SizeLike TSize>
class linear_layout { ... };

template<typename EventType>
class event_target { ... };

// Usage required specifying all types
using MyElement = ui_element<SDL_Rect, SDL_Size>;
using MyLayout = linear_layout<SDL_Rect, SDL_Size>;
```

### New Pattern (After)
```cpp
// Single Backend parameter
template<UIBackend Backend>
class ui_element { ... };

template<UIBackend Backend>
class linear_layout { ... };

template<UIBackend Backend>
class event_target { ... };

// Usage with backend trait
using MyElement = ui_element<sdl_backend>;
using MyLayout = linear_layout<sdl_backend>;
```

## Migration Steps

### Step 1: Create Your Backend Structure

Define a backend structure that provides all necessary type aliases:

```cpp
// Old approach: using types directly
using MyRect = SDL_Rect;
using MySize = SDL_Size;

// New approach: backend structure
struct sdl_backend {
    // Required geometric types
    using rect_type = SDL_Rect;
    using size_type = SDL_Size;
    using point_type = SDL_Point;

    // Required event types
    using event_type = SDL_Event;
    using keyboard_event_type = SDL_KeyboardEvent;
    using mouse_button_event_type = SDL_MouseButtonEvent;
    using mouse_motion_event_type = SDL_MouseMotionEvent;

    // Optional rendering types
    using color_type = SDL_Color;
    using renderer_type = SDL_Renderer*;
    using texture_type = SDL_Texture*;
    using font_type = TTF_Font*;

    // Backend name for debugging
    static constexpr const char* name() { return "SDL2"; }
};
```

### Step 2: Update Template Parameters

#### UI Elements

**Before:**
```cpp
template<RectLike TRect, SizeLike TSize>
class my_widget : public ui_element<TRect, TSize> {
    using base = ui_element<TRect, TSize>;
    // ...
};

// Instantiation
auto widget = std::make_unique<my_widget<SDL_Rect, SDL_Size>>();
```

**After:**
```cpp
template<UIBackend Backend>
class my_widget : public ui_element<Backend> {
    using base = ui_element<Backend>;
    using rect_type = typename Backend::rect_type;
    using size_type = typename Backend::size_type;
    // ...
};

// Instantiation
auto widget = std::make_unique<my_widget<sdl_backend>>();
```

#### Layout Strategies

**Before:**
```cpp
auto layout = std::make_unique<linear_layout<SDL_Rect, SDL_Size>>(
    direction::vertical, 10);
```

**After:**
```cpp
auto layout = std::make_unique<linear_layout<sdl_backend>>(
    direction::vertical, 10);
```

#### Event Handling

**Before:**
```cpp
class my_button : public event_target<SDL_Event> {
    bool process_event(const SDL_Event& event) override { ... }
};
```

**After:**
```cpp
class my_button : public event_target<sdl_backend> {
    template<typename E>
    bool process_event(const E& event) { ... }
};
```

### Step 3: Update Event Traits

Define event traits for your backend's event types:

```cpp
// Event traits for keyboard events
template<>
struct event_traits<SDL_KeyboardEvent> {
    static bool is_key_press(const SDL_KeyboardEvent& e) {
        return e.type == SDL_KEYDOWN;
    }

    static int key_code(const SDL_KeyboardEvent& e) {
        return e.keysym.sym;
    }

    static bool shift_pressed(const SDL_KeyboardEvent& e) {
        return e.keysym.mod & KMOD_SHIFT;
    }

    static bool is_tab_key(const SDL_KeyboardEvent& e) {
        return e.keysym.sym == SDLK_TAB;
    }

    // ... other required methods
};
```

### Step 4: Update Focus Manager

**Before:**
```cpp
focus_manager<SDL_Event> focus_mgr;
```

**After:**
```cpp
focus_manager<sdl_backend> focus_mgr;
```

## Type Aliases for Convenience

Create type aliases to simplify usage:

```cpp
// Define aliases for your backend
namespace my_app {
    using Backend = sdl_backend;
    using Element = ui_element<Backend>;
    using LinearLayout = linear_layout<Backend>;
    using GridLayout = grid_layout<Backend>;
    using EventTarget = event_target<Backend>;
    using FocusManager = focus_manager<Backend>;
}

// Usage becomes cleaner
auto root = std::make_unique<my_app::Element>(nullptr);
root->set_layout_strategy(std::make_unique<my_app::LinearLayout>());
```

## Common Migration Issues and Solutions

### Issue 1: Accessing Rect/Size Types

**Problem:** Direct access to TRect/TSize is no longer available.

**Solution:** Use type aliases from Backend:
```cpp
// Before
TRect bounds;
TSize size;

// After
using rect_type = typename Backend::rect_type;
using size_type = typename Backend::size_type;
rect_type bounds;
size_type size;
```

### Issue 2: Template Specialization

**Problem:** Existing template specializations need updating.

**Solution:** Specialize on the backend instead:
```cpp
// Before
template<>
class my_widget<SDL_Rect, SDL_Size> { ... };

// After
template<>
class my_widget<sdl_backend> { ... };
```

### Issue 3: Factory Functions

**Problem:** Factory functions with multiple template parameters.

**Solution:** Update to use single Backend parameter:
```cpp
// Before
template<RectLike TRect, SizeLike TSize>
auto create_button(const char* text) {
    return std::make_unique<button<TRect, TSize>>(text);
}

// After
template<UIBackend Backend>
auto create_button(const char* text) {
    return std::make_unique<button<Backend>>(text);
}
```

## Testing During Migration

Use the test backend for unit tests to avoid external dependencies:

```cpp
// test_backend is provided for testing
using TestElement = ui_element<test_backend>;
using TestLayout = linear_layout<test_backend>;

// Create test fixtures
class LayoutTest {
    void test_linear_layout() {
        auto root = std::make_unique<TestElement>(nullptr);
        root->set_layout_strategy(
            std::make_unique<TestLayout>(direction::vertical));

        // Test without SDL/GLFW dependencies
        root->measure(100, 100);
        root->arrange({0, 0, 100, 100});

        ASSERT_EQ(root->bounds(), test_backend::rect{0, 0, 100, 100});
    }
};
```

## Benefits After Migration

1. **Simpler API**: Single template parameter instead of multiple
2. **Type Safety**: All types come from the same backend
3. **Easier Extension**: Add new backends by implementing the trait
4. **Better Error Messages**: Concept constraints provide clear requirements
5. **Consistent Types**: No mixing incompatible types from different libraries

## Example: Complete Migration

### Before (Old Multi-Parameter Style)
```cpp
#include <SDL2/SDL.h>
#include <onyxui/element.hh>
#include <onyxui/linear_layout.hh>

class SDLApp {
    using Element = ui_element<SDL_Rect, SDL_Size>;
    using Layout = linear_layout<SDL_Rect, SDL_Size>;

    std::unique_ptr<Element> root;

    void init() {
        root = std::make_unique<Element>(nullptr);
        root->set_layout_strategy(
            std::make_unique<Layout>(direction::vertical, 10));

        auto button = std::make_unique<Element>(nullptr);
        button->set_width_constraint({size_policy::fixed, 100});
        root->add_child(std::move(button));
    }

    void render(SDL_Renderer* renderer) {
        root->measure(800, 600);
        SDL_Rect viewport = {0, 0, 800, 600};
        root->arrange(viewport);
        // ... render using bounds
    }
};
```

### After (New Backend Pattern)
```cpp
#include <SDL2/SDL.h>
#include <onyxui/backends/sdl_backend.hh>  // Pre-defined SDL backend
#include <onyxui/element.hh>
#include <onyxui/layout/linear_layout.hh>

class SDLApp {
    using Backend = sdl_backend;
    using Element = ui_element<Backend>;
    using Layout = linear_layout<Backend>;

    std::unique_ptr<Element> root;

    void init() {
        root = std::make_unique<Element>(nullptr);
        root->set_layout_strategy(
            std::make_unique<Layout>(direction::vertical, 10));

        auto button = std::make_unique<Element>(nullptr);
        button->set_width_constraint({size_policy::fixed, 100});
        root->add_child(std::move(button));
    }

    void render(SDL_Renderer* renderer) {
        root->measure(800, 600);
        SDL_Rect viewport = {0, 0, 800, 600};
        root->arrange(viewport);
        // ... render using bounds (no changes needed!)
    }
};
```

## Conclusion

The Backend pattern migration simplifies the library's API while maintaining all functionality. The key is to:

1. Define your backend structure with all required types
2. Update template parameters to use Backend instead of individual types
3. Add type aliases for convenience
4. Implement event traits for your event types

The migration can be done incrementally, updating one component at a time. The test backend allows you to verify functionality without external dependencies.