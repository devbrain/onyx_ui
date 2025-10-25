---
sidebar_position: 4
---

# Background Renderer

The Background Renderer is a specialized component in OnyxUI that manages viewport background rendering. Unlike traditional UI toolkits where the background might be a widget or a simple property, OnyxUI treats background rendering as a **global UI concern** managed by the UI context.

## Why Not a Widget?

You might wonder: "Why isn't the background just another widget in the tree?" This design decision was made after careful consideration of several key architectural principles:

### 1. **Rendering Pipeline Position**

The background must render **before** the widget tree to establish the drawing surface. Making it a widget would:
- Force it to be the first child in the tree (awkward API)
- Require special layout handling (wasted overhead)
- Complicate z-ordering (background must always be bottom)

### 2. **Not Part of Layout Hierarchy**

A background doesn't participate in layout:
- No measure/arrange needed (always fills viewport)
- No parent-child relationships
- No size constraints or policies

Adding it to the widget tree would incur unnecessary overhead for measure/arrange operations that always do the same thing.

### 3. **Supports Modes Impossible for Widgets**

The background renderer supports **transparent mode** for game overlays:

```cpp
// UI widgets overlay a 3D game engine
bg->set_mode(background_mode::transparent);
// No background rendered - widgets appear over the game
```

This is fundamentally different from a transparent widget (which still participates in layout and event handling).

### 4. **Performance Optimization**

Background rendering is optimized for dirty regions:
```cpp
// Only fill the regions that changed
bg->render(renderer, viewport, dirty_regions);
```

A widget-based approach would require traversing the widget tree to reach the background, adding unnecessary overhead.

## Architecture

The background renderer follows the **Service Locator** pattern:

```
ui_context<Backend>
  ├─ layer_manager         (per-context service)
  ├─ focus_manager         (per-context service)
  ├─ background_renderer   (per-context service)  ← HERE
  └─ theme_registry        (shared service)
```

Access is provided through `ui_services`:

```cpp
auto* bg = ui_services<Backend>::background();
if (bg) {
    // Configure background
    bg->set_mode(background_mode::solid);
    bg->set_color({0, 0, 170});  // Blue
}
```

## Three Rendering Modes

### 1. Solid Mode (Default)

Fills the viewport with a solid color, optimized for dirty regions:

```cpp
bg->set_mode(background_mode::solid);
bg->set_color({255, 255, 255});  // White background
```

**Performance Characteristics:**
- **Empty dirty regions:** Fills entire viewport (1 draw call)
- **Non-empty dirty regions:** Fills only changed areas (N draw calls)
- **Optimization:** O(dirty_regions) complexity

**Use Cases:**
- Traditional desktop applications
- Document editors
- Dialogs and forms

### 2. Transparent Mode

No background rendering - UI widgets overlay existing content:

```cpp
bg->set_mode(background_mode::transparent);
// Zero overhead - no rendering at all
```

**Performance Characteristics:**
- **Zero rendering overhead:** No draw calls
- **Zero memory overhead:** No buffers allocated

**Use Cases:**
- Game HUDs (health bars, minimaps)
- In-game menus
- Video player overlays
- AR/VR interfaces

### 3. Pattern Mode (Future)

Reserved for future texture support:

```cpp
bg->set_mode(background_mode::pattern);
// Currently falls back to solid mode
```

**Planned Features:**
- Tiled textures
- Stretched backgrounds
- Centered images

## Rendering Pipeline Integration

The background renderer integrates into `ui_handle::display()` at a specific point in the pipeline:

```cpp
void ui_handle::display() {
    auto viewport = m_renderer.get_viewport();
    auto dirty_regions = m_root->get_and_clear_dirty_regions();

    // ┌─────────────────────────────────────────────────┐
    // │ 1. BACKGROUND (establishes drawing surface)     │
    // └─────────────────────────────────────────────────┘
    if (auto* bg = ui_services<Backend>::background()) {
        bg->render(m_renderer, viewport, dirty_regions);
    }

    // ┌─────────────────────────────────────────────────┐
    // │ 2. WIDGET TREE (main UI content)                │
    // └─────────────────────────────────────────────────┘
    m_root->measure(viewport.w, viewport.h);
    m_root->arrange(viewport);
    m_root->render(m_renderer, dirty_regions);

    // ┌─────────────────────────────────────────────────┐
    // │ 3. POPUP LAYERS (menus, dialogs, tooltips)      │
    // └─────────────────────────────────────────────────┘
    if (auto* layers = ui_services<Backend>::layers()) {
        layers->render_all_layers(m_renderer, viewport);
    }
}
```

This ordering ensures:
1. Background is drawn first (bottom z-order)
2. Widgets render on top of background
3. Popups render on top of everything

## Frame-Based Behavior

Background changes apply on the **next frame**, not immediately:

```cpp
auto* bg = ui_services<Backend>::background();
bg->set_color({255, 0, 0});  // Change to red
// ... change is pending ...
ui.display();  // NOW the red background is rendered
```

This design is **intentional** and follows the event-driven architecture:
- **No automatic invalidation:** Application controls when to redraw
- **Predictable performance:** No surprise redraws
- **Consistent with widget behavior:** Widgets also don't auto-invalidate on property changes

### Why Frame-Based?

1. **Performance Control:** Application batches multiple changes before rendering
2. **Animation Support:** Can interpolate colors/modes between frames
3. **Event Loop Integration:** Fits naturally with event-driven design

## Per-Context Independence

Each `ui_context` has its own independent `background_renderer`:

```cpp
{
    scoped_ui_context<Backend> ctx1;
    auto* bg1 = ui_services<Backend>::background();
    bg1->set_color({255, 0, 0});  // Red
    bg1->set_mode(background_mode::solid);

    {
        scoped_ui_context<Backend> ctx2;
        auto* bg2 = ui_services<Backend>::background();
        bg2->set_color({0, 255, 0});  // Green
        bg2->set_mode(background_mode::transparent);

        // ctx2 is active - green, transparent
        ui2.display();
    }

    // ctx1 is active again - red, solid
    ui1.display();
}
```

This is useful for:
- **Multi-window applications:** Each window has independent background
- **Split views:** Different background modes per view
- **Testing:** Isolated contexts don't interfere

## Typical Usage Patterns

### Pattern 1: Simple Solid Background

```cpp
// Initialize UI context
scoped_ui_context<Backend> ctx;

// Configure background
auto* bg = ui_services<Backend>::background();
bg->set_mode(background_mode::solid);
bg->set_color({240, 240, 240});  // Light gray

// Build UI
auto root = create_vbox();
// ... add widgets ...

// Main loop
ui_handle<Backend> ui(std::move(root));
while (!quit) {
    ui.display();
    ui.present();
    // Handle events...
}
```

### Pattern 2: Theme-Based Background

```cpp
scoped_ui_context<Backend> ctx;

// Get theme background color
auto* themes = ui_services<Backend>::themes();
auto* theme = themes->get_theme("Dark Theme");

// Apply theme's window background
auto* bg = ui_services<Backend>::background();
bg->set_color(theme->window_bg);
bg->set_mode(background_mode::solid);
```

### Pattern 3: Game Overlay (Transparent)

```cpp
// Game engine owns the renderer
GameRenderer game_renderer;

// Create UI over game
scoped_ui_context<Backend> ctx;
auto* bg = ui_services<Backend>::background();
bg->set_mode(background_mode::transparent);  // Don't fill background

// Build HUD
auto hud = create_game_hud();

// Game loop
ui_handle<Backend> ui(std::move(hud));
while (game.is_running()) {
    // Render game first
    game.render();

    // Render UI on top (transparent background)
    ui.display();
    ui.present();
}
```

### Pattern 4: Dynamic Background Changes

```cpp
auto* bg = ui_services<Backend>::background();

// React to application state
if (app.is_loading()) {
    bg->set_color({0, 0, 0});  // Black during loading
} else if (app.has_error()) {
    bg->set_color({128, 0, 0});  // Dark red for errors
} else {
    bg->set_color({255, 255, 255});  // White for normal
}

ui.display();  // Background color applied
```

## Implementation Details

### Class Structure

```cpp
template<UIBackend Backend>
class background_renderer {
public:
    using color_type = typename Backend::color_type;
    using rect_type = typename Backend::rect_type;
    using renderer_type = typename Backend::renderer_type;

    // Constructors
    background_renderer() = default;
    explicit background_renderer(const color_type& color);

    // Rule of Five (explicitly defaulted)
    background_renderer(const background_renderer&) = default;
    background_renderer(background_renderer&&) noexcept = default;
    background_renderer& operator=(const background_renderer&) = default;
    background_renderer& operator=(background_renderer&&) noexcept = default;
    ~background_renderer() = default;

    // Configuration
    void set_mode(background_mode mode) noexcept;
    void set_color(const color_type& color) noexcept;

    // Queries
    [[nodiscard]] background_mode get_mode() const noexcept;
    [[nodiscard]] const color_type& get_color() const noexcept;

    // Rendering (called by ui_handle)
    void render(renderer_type& renderer,
                const rect_type& viewport,
                const std::vector<rect_type>& dirty_regions);
};
```

### Memory and Performance

**Memory Footprint:**
- Enum (mode): 1 byte
- Color: 3-4 bytes (RGB/RGBA)
- **Total:** ~8 bytes per instance

**Performance:**
- **Solid mode (empty dirty):** 1 draw call
- **Solid mode (N dirty):** N draw calls
- **Transparent mode:** 0 draw calls
- **No allocations:** All operations are `noexcept`

### Thread Safety

The background renderer is **not thread-safe**:
- All operations must occur on the **UI thread**
- Per-context instances avoid cross-context races
- Consistent with other UI components

## Testing

Comprehensive test coverage in `unittest/core/test_background_renderer.cc`:

```cpp
TEST_CASE("Background Renderer - Mode switching") {
    background_renderer<Backend> bg;

    // Default mode is solid
    CHECK(bg.get_mode() == background_mode::solid);

    // Can switch to transparent
    bg.set_mode(background_mode::transparent);
    CHECK(bg.get_mode() == background_mode::transparent);
}

TEST_CASE("Background Renderer - Dirty region optimization") {
    background_renderer<Backend> bg;
    bg.set_mode(background_mode::solid);

    mock_renderer renderer;
    rect viewport{0, 0, 80, 25};
    std::vector<rect> dirty_regions{{10, 10, 20, 10}, {40, 5, 15, 8}};

    bg.render(renderer, viewport, dirty_regions);

    // Should only fill dirty regions
    REQUIRE(renderer.draw_box_calls.size() == 2);
    CHECK(renderer.draw_box_calls[0] == dirty_regions[0]);
    CHECK(renderer.draw_box_calls[1] == dirty_regions[1]);
}
```

**Coverage:**
- 8 test cases
- 67 assertions
- 100% API coverage

## Comparison with Other Approaches

### Approach 1: Background as Widget (Rejected)

```cpp
// ❌ What we DON'T do
auto root = create_desktop_widget();
root->set_background_color(color);
root->add_child(my_content);  // Content is child of background

// Problems:
// - Background participates in layout (overhead)
// - Transparent mode impossible (widget always exists)
// - Awkward API (background is parent of content)
// - Z-ordering issues (background must be first child)
```

### Approach 2: Background as ui_handle Property (Rejected)

```cpp
// ❌ What we DON'T do
ui_handle ui(root);
ui.set_background_color(color);  // Property of ui_handle

// Problems:
// - Not accessible from ui_services pattern
// - Can't configure before creating ui_handle
// - Inconsistent with other services (layers, focus)
```

### Approach 3: Background as Service (✅ What We Do)

```cpp
// ✅ What we DO
scoped_ui_context<Backend> ctx;
auto* bg = ui_services<Backend>::background();
bg->set_mode(background_mode::solid);
bg->set_color(color);

ui_handle ui(root);  // Background is independent

// Benefits:
// ✓ Consistent with service locator pattern
// ✓ Configure before ui_handle creation
// ✓ Not part of layout hierarchy
// ✓ Supports all three modes naturally
```

## See Also

- **[Service Locator](./service-locator.md)** - Pattern used for background access
- **[UI Context](./ui-context.md)** - Service container
- **[UI Handle](./ui-handle.md)** - Rendering pipeline
- **[Render Context](./render-context.md)** - Widget rendering

## API Reference

For complete API documentation, see:
- `include/onyxui/background_renderer.hh` - Full implementation
- `include/onyxui/ui_context.hh` - Service integration
- `unittest/core/test_background_renderer.cc` - Usage examples
