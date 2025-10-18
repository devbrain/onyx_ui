# Layer Manager - Architecture Design

## Executive Summary

This document proposes a **layer manager** system for onyx_ui that provides:
- Multi-layer UI rendering (base UI, popups, dialogs, tooltips)
- Z-ordering and stacking context management
- Event routing with layer priority
- Modal blocking and focus management
- Popup positioning and constraint solving
- Animation support for layer transitions

## Problem Statement

### Current Limitations

The current system renders all UI in a single flat tree:
```
ui_element tree → render() → single output
```

**Issues:**
1. ❌ **No overlays**: Cannot show dropdowns/popups on top of other UI
2. ❌ **No z-ordering**: All elements at same depth
3. ❌ **Event routing**: Events go to first hit, no layer priority
4. ❌ **Modal dialogs**: Cannot block interaction with underlying UI
5. ❌ **Positioning**: Popups can't position relative to anchor elements

### Real-World Example (menu_bar issue)

```cpp
// This DOESN'T work currently:
menu_bar->add_menu("File", file_menu);
// When clicked:
//   1. menu_bar sets m_open_menu_index
//   2. Emits menu_opened signal
//   3. ❌ Nothing shows the dropdown (no popup layer system)
//   4. ❌ UI gets stuck (thinks menu is open but it's not visible)
```

**What we need:**
```cpp
// With layer manager:
menu_bar->add_menu("File", file_menu);
// When clicked:
//   1. menu_bar emits menu_opened signal
//   2. layer_mgr.show_popup(file_menu, position, anchor)
//   3. ✅ Dropdown renders on POPUP layer (above everything)
//   4. ✅ Events route to popup first (click outside closes menu)
//   5. ✅ Escape key closes popup
```

## Proposed Architecture

### Layer Types

```cpp
enum class layer_type : uint8_t {
    base,         // Main UI (z-index: 0)
    tooltip,      // Tooltips (z-index: 100)
    popup,        // Dropdowns, context menus (z-index: 200)
    dialog,       // Non-modal dialogs (z-index: 300)
    modal,        // Modal dialogs (z-index: 400)
    notification, // Toast/snackbar (z-index: 500)
    debug         // Debug overlay (z-index: 1000)
};
```

### Core Components

```
┌─────────────────────────────────────────────────────────────┐
│                      layer_manager                           │
│                                                              │
│  Manages:                                                    │
│  • Layer stack (ordered by z-index)                         │
│  • Event routing (top layer first)                          │
│  • Rendering order (bottom to top)                          │
│  • Modal blocking                                            │
│  • Focus management                                          │
└─────────────────────────────────────────────────────────────┘
        │
        │ contains
        ▼
┌─────────────────────────────────────────────────────────────┐
│                         layer                                │
│                                                              │
│  Properties:                                                 │
│  • type: layer_type                                         │
│  • z_index: int                                             │
│  • root: ui_element*                                        │
│  • visible: bool                                            │
│  • modal: bool                                              │
│  • blocks_events: bool                                      │
└─────────────────────────────────────────────────────────────┘
```

## Detailed Design

### 1. Layer Manager Core

```cpp
template<UIBackend Backend>
class layer_manager {
public:
    using element_type = ui_element<Backend>;
    using rect_type = typename Backend::rect_type;
    using event_type = typename Backend::event_type;
    using renderer_type = typename Backend::renderer_type;

    // Configuration
    struct config {
        bool enable_animations = true;
        int animation_duration_ms = 200;
        bool debug_show_layers = false;
    };

    explicit layer_manager(config cfg = {});

    // Layer management
    layer_id add_layer(layer_type type,
                      std::unique_ptr<element_type> root,
                      int custom_z_index = -1);

    void remove_layer(layer_id id);
    void clear_layers(layer_type type);  // Clear all of specific type
    void clear_all_layers();

    // Layer visibility
    void show_layer(layer_id id, bool animate = true);
    void hide_layer(layer_id id, bool animate = true);
    bool is_layer_visible(layer_id id) const;

    // Popup helpers (most common use case)
    layer_id show_popup(std::unique_ptr<element_type> content,
                       const rect_type& anchor_bounds,
                       popup_placement placement = popup_placement::below);

    layer_id show_tooltip(std::unique_ptr<element_type> content,
                         int x, int y);

    layer_id show_modal_dialog(std::unique_ptr<element_type> content,
                               dialog_position pos = dialog_position::center);

    // Event routing
    bool route_event(const event_type& event);

    // Rendering (called by ui_handle)
    void render_all_layers(renderer_type& renderer, const rect_type& viewport);

    // Modal state
    bool has_modal_layer() const;
    layer_id top_modal_layer() const;

    // Layer queries
    [[nodiscard]] std::vector<layer_id> get_layers_of_type(layer_type type) const;
    [[nodiscard]] layer_info get_layer_info(layer_id id) const;

    // Signals
    signal<layer_id> layer_shown;
    signal<layer_id> layer_hidden;
    signal<layer_id> layer_removed;

private:
    struct layer_data {
        layer_id id;
        layer_type type;
        int z_index;
        std::unique_ptr<element_type> root;
        bool visible;
        bool modal;
        bool blocks_events;

        // Animation state
        float opacity;
        animation_state anim_state;
    };

    config m_config;
    std::vector<layer_data> m_layers;  // Sorted by z_index
    layer_id m_next_id = 0;

    // Event routing
    element_type* hit_test_layers(int x, int y);
    bool should_block_event(layer_id id, const event_type& event) const;

    // Rendering
    void render_layer(const layer_data& layer, renderer_type& renderer);
    void sort_layers_by_z_index();
};
```

### 2. Layer ID System

```cpp
// Strongly-typed layer ID
struct layer_id {
    uint32_t value;

    constexpr layer_id() noexcept : value(0) {}
    explicit constexpr layer_id(uint32_t v) noexcept : value(v) {}

    bool operator==(layer_id other) const noexcept { return value == other.value; }
    bool operator!=(layer_id other) const noexcept { return value != other.value; }

    [[nodiscard]] bool is_valid() const noexcept { return value != 0; }

    static constexpr layer_id invalid() noexcept { return layer_id(0); }
};
```

### 3. Popup Positioning

```cpp
enum class popup_placement {
    below,        // Below anchor, left-aligned
    above,        // Above anchor, left-aligned
    left,         // Left of anchor, top-aligned
    right,        // Right of anchor, top-aligned
    below_right,  // Below anchor, right-aligned
    auto_best     // Choose best position to fit viewport
};

enum class dialog_position {
    center,       // Center of viewport
    top,          // Top center
    bottom,       // Bottom center
    custom        // Use provided position
};

template<UIBackend Backend>
class popup_positioner {
public:
    using rect_type = typename Backend::rect_type;

    // Calculate popup position
    struct result {
        rect_type bounds;
        popup_placement actual_placement;  // May differ if auto_best
    };

    [[nodiscard]] static result calculate_position(
        const rect_type& popup_size,
        const rect_type& anchor_bounds,
        const rect_type& viewport,
        popup_placement placement);

    // Check if popup fits in viewport at position
    [[nodiscard]] static bool fits_in_viewport(
        const rect_type& popup_bounds,
        const rect_type& viewport);

    // Find best placement that fits
    [[nodiscard]] static popup_placement find_best_placement(
        const rect_type& popup_size,
        const rect_type& anchor_bounds,
        const rect_type& viewport);
};
```

### 4. Modal Blocking

```cpp
template<UIBackend Backend>
class modal_blocker {
public:
    // Render semi-transparent overlay
    void render_overlay(typename Backend::renderer_type& renderer,
                       const typename Backend::rect_type& bounds);

    // Check if event should be blocked
    [[nodiscard]] bool should_block_event(
        const typename Backend::event_type& event,
        const typename Backend::rect_type& modal_bounds) const;

    // Configuration
    void set_overlay_color(const typename Backend::color_type& color);
    void set_overlay_opacity(float opacity);  // 0.0 - 1.0
    void set_allow_click_to_close(bool allow);

private:
    typename Backend::color_type m_overlay_color{0, 0, 0, 128};
    float m_overlay_opacity = 0.5f;
    bool m_allow_click_to_close = false;
};
```

### 5. Layer Animation

```cpp
enum class animation_state {
    none,
    showing,    // Fade in / slide in
    visible,    // Fully shown
    hiding,     // Fade out / slide out
    hidden      // Fully hidden
};

template<UIBackend Backend>
class layer_animator {
public:
    // Animation types
    enum class type {
        none,
        fade,
        slide_up,
        slide_down,
        slide_left,
        slide_right,
        scale
    };

    struct animation {
        type anim_type;
        int duration_ms;
        float progress;  // 0.0 to 1.0

        std::chrono::steady_clock::time_point start_time;
    };

    // Start animations
    void start_show(layer_id id, type anim_type, int duration_ms);
    void start_hide(layer_id id, type anim_type, int duration_ms);

    // Update animations (called each frame)
    void update(int delta_time_ms);

    // Check if animation complete
    [[nodiscard]] bool is_animating(layer_id id) const;
    [[nodiscard]] float get_opacity(layer_id id) const;
    [[nodiscard]] typename Backend::rect_type get_transform(
        layer_id id,
        const typename Backend::rect_type& base_bounds) const;

private:
    std::unordered_map<layer_id, animation> m_animations;

    float calculate_opacity(const animation& anim) const;
    typename Backend::rect_type calculate_transform(
        const animation& anim,
        const typename Backend::rect_type& bounds) const;
};
```

### 6. Event Routing with Layers

```cpp
template<UIBackend Backend>
class layer_event_router {
public:
    using event_type = typename Backend::event_type;
    using element_type = ui_element<Backend>;

    // Route event through layer stack (top to bottom)
    struct route_result {
        bool handled;
        layer_id handled_by;
        element_type* target_element;
    };

    [[nodiscard]] route_result route(
        const event_type& event,
        const std::vector<layer_data>& layers);

    // Event capture (for modals)
    void set_capture_layer(layer_id id);
    void release_capture();
    [[nodiscard]] bool has_capture() const;

private:
    layer_id m_capture_layer;

    // Hit testing
    element_type* hit_test_layer(
        const layer_data& layer,
        int x, int y);

    // Event type checks
    bool is_mouse_event(const event_type& event) const;
    bool is_keyboard_event(const event_type& event) const;
};
```

## Integration with Existing Code

### Minimal Changes to ui_handle

```cpp
template<UIBackend Backend>
class ui_handle {
private:
    std::unique_ptr<widget_type> m_root;
    renderer_type m_renderer;
    hierarchical_focus_manager<Backend, widget_type> m_focus_manager;

    // NEW: Layer manager
    layer_manager<Backend> m_layer_mgr;

public:
    void display() {
        if (!m_root) return;

        auto bounds = m_renderer.get_viewport();

        // Measure and arrange base UI (existing code)
        m_root->measure(rect_utils::get_width(bounds),
                       rect_utils::get_height(bounds));
        m_root->arrange(bounds);

        // Render base layer
        m_root->render(m_renderer);

        // NEW: Render all overlay layers
        m_layer_mgr.render_all_layers(m_renderer, bounds);
    }

    bool handle_event(const event_type& event) {
        if (!m_root) return false;

        // NEW: Route event through layers first
        if (m_layer_mgr.route_event(event)) {
            return true;  // Layer handled it
        }

        // Existing event handling for base UI
        // ... (resize, keyboard, mouse) ...

        return false;
    }

    // NEW: Layer access
    layer_manager<Backend>& layers() { return m_layer_mgr; }
};
```

### Using Layers - Dropdown Menu Example

```cpp
// In menu_bar implementation:
class menu_bar : public hbox<Backend> {
    void open_menu(std::size_t index) {
        if (index >= m_menus.size()) return;

        auto& entry = m_menus[index];

        // Get anchor button bounds
        rect_type anchor_bounds = entry.title_button->bounds();

        // NEW: Show dropdown as popup layer
        auto menu_clone = clone_menu(entry.dropdown_menu);

        layer_id popup_id = get_layer_manager()->show_popup(
            std::move(menu_clone),
            anchor_bounds,
            popup_placement::below
        );

        // Store popup ID for closing later
        m_current_popup = popup_id;
        m_open_menu_index = index;
    }

    void close_menu() {
        if (m_current_popup.is_valid()) {
            get_layer_manager()->hide_layer(m_current_popup);
            m_current_popup = layer_id::invalid();
        }
        m_open_menu_index = std::nullopt;
    }

private:
    layer_id m_current_popup;

    // Get layer manager from ui_handle (via parent chain or context)
    layer_manager<Backend>* get_layer_manager();
};
```

### Using Layers - Modal Dialog Example

```cpp
// Show a modal dialog
void show_save_dialog(ui_handle<Backend>& ui) {
    auto dialog = std::make_unique<panel<Backend>>(nullptr);
    dialog->set_padding(thickness::all(20));
    dialog->set_has_border(true);

    // Build dialog content
    dialog->add_child(create_label("Save changes?"));

    auto button_row = dialog->add_child(
        std::make_unique<hbox<Backend>>());

    auto save_btn = button_row->add_child(create_button("Save"));
    save_btn->clicked.connect([&ui]() {
        // Save logic
        ui.layers().remove_layer(current_dialog_id);
    });

    auto cancel_btn = button_row->add_child(create_button("Cancel"));
    cancel_btn->clicked.connect([&ui]() {
        ui.layers().remove_layer(current_dialog_id);
    });

    // Show as modal dialog (blocks base UI)
    layer_id dialog_id = ui.layers().show_modal_dialog(
        std::move(dialog),
        dialog_position::center
    );
}
```

### Using Layers - Tooltip Example

```cpp
// Show tooltip on hover
void show_tooltip(ui_handle<Backend>& ui,
                 const std::string& text,
                 int x, int y) {
    auto tooltip = std::make_unique<label<Backend>>(nullptr);
    tooltip->set_text(text);
    tooltip->set_padding(thickness::all(4));
    tooltip->set_has_border(true);

    layer_id id = ui.layers().show_tooltip(std::move(tooltip), x, y);

    // Auto-hide after 3 seconds
    schedule_timer(3000ms, [&ui, id]() {
        ui.layers().hide_layer(id);
    });
}
```

## Z-Index and Stacking Order

### Default Z-Indices

```cpp
constexpr int Z_INDEX_BASE         = 0;
constexpr int Z_INDEX_TOOLTIP      = 100;
constexpr int Z_INDEX_POPUP        = 200;
constexpr int Z_INDEX_DIALOG       = 300;
constexpr int Z_INDEX_MODAL        = 400;
constexpr int Z_INDEX_NOTIFICATION = 500;
constexpr int Z_INDEX_DEBUG        = 1000;
```

### Rendering Order (Bottom to Top)

```
┌─────────────────────────────────────────────┐
│  Z-INDEX 1000: Debug Overlay                │
├─────────────────────────────────────────────┤
│  Z-INDEX 500:  Notifications (toast)        │
├─────────────────────────────────────────────┤
│  Z-INDEX 400:  Modal Dialogs + Blocker      │
├─────────────────────────────────────────────┤
│  Z-INDEX 300:  Non-Modal Dialogs            │
├─────────────────────────────────────────────┤
│  Z-INDEX 200:  Popups (dropdowns, menus)    │
├─────────────────────────────────────────────┤
│  Z-INDEX 100:  Tooltips                     │
├─────────────────────────────────────────────┤
│  Z-INDEX 0:    Base UI (main application)   │
└─────────────────────────────────────────────┘
```

### Event Routing Order (Top to Bottom)

Events are offered to layers from **highest z-index to lowest**:
1. Debug overlay (if enabled)
2. Notifications
3. Modal dialogs (blocks events from going lower)
4. Non-modal dialogs
5. Popups
6. Tooltips
7. Base UI

If a modal is active, events never reach base UI.

## Advanced Features

### 1. Nested Popups

```cpp
// Context menu from dropdown menu
layer_id dropdown_id = ui.layers().show_popup(dropdown, anchor, below);

// Later, from an item in the dropdown:
layer_id context_id = ui.layers().show_popup(
    context_menu,
    item_bounds,
    popup_placement::right
);

// Both popups visible, correct z-order maintained
```

### 2. Click-Outside to Close

```cpp
// Automatically close popup when clicking outside
layer_id id = ui.layers().show_popup(menu, anchor, below);

ui.layers().layer_hidden.connect([](layer_id closed_id) {
    // Cleanup when popup closes
});

// Layer manager handles click-outside detection automatically
// for non-modal layers
```

### 3. Escape Key Handling

```cpp
// In layer_manager::route_event():
if (is_escape_key(event)) {
    // Close top-most non-modal layer
    if (auto top = get_top_closeable_layer()) {
        hide_layer(top);
        return true;  // Event handled
    }
}
```

### 4. Focus Trapping (Modal Dialogs)

```cpp
// When modal is shown:
void show_modal_dialog(...) {
    // Save current focus
    m_saved_focus = m_focus_mgr.get_focused();

    // Focus first element in dialog
    modal_layer->focus_first_element();

    // Trap Tab navigation within modal
    m_focus_trap_active = true;
}

// When modal is hidden:
void hide_modal_dialog(...) {
    // Restore previous focus
    m_focus_mgr.set_focus(m_saved_focus);
    m_focus_trap_active = false;
}
```

## Performance Considerations

### Rendering Optimization

```cpp
// Only render visible layers
void layer_manager::render_all_layers(renderer& r, const rect& viewport) {
    for (const auto& layer : m_layers) {
        if (!layer.visible) continue;  // Skip hidden
        if (layer.opacity < 0.01f) continue;  // Skip transparent

        render_layer(layer, r);
    }
}
```

### Event Routing Optimization

```cpp
// Early exit for modal blocking
bool layer_manager::route_event(const event& e) {
    // If modal active, only route to modal and above
    if (auto modal_id = top_modal_layer(); modal_id.is_valid()) {
        for (auto& layer : reverse_z_order()) {
            if (layer.id == modal_id) {
                // Route to modal
                return layer.root->process_event(e);
            }
            if (layer.z_index > modal_z_index) {
                // Still above modal (notifications, debug)
                if (layer.root->process_event(e)) return true;
            }
        }
        return false;  // Modal blocks all other events
    }

    // Normal routing (top to bottom)
    // ...
}
```

### Memory Management

```cpp
// Layer cleanup on hide
void layer_manager::hide_layer(layer_id id, bool animate) {
    auto it = find_layer(id);
    if (it == m_layers.end()) return;

    if (animate && m_config.enable_animations) {
        // Animate out, then remove
        m_animator.start_hide(id, animation::fade, 200);
        // Schedule removal after animation
        schedule_removal(id, 200ms);
    } else {
        // Remove immediately
        m_layers.erase(it);
    }
}
```

## Implementation Phases

### Phase 1: Core Infrastructure (Week 1)
- [ ] `layer_manager` basic implementation
- [ ] Layer data structure and z-ordering
- [ ] Basic show/hide functionality
- [ ] Event routing (without animation)
- [ ] Unit tests

### Phase 2: Popup Positioning (Week 1-2)
- [ ] `popup_positioner` implementation
- [ ] Auto-placement algorithm
- [ ] Viewport constraint solving
- [ ] Tests for all placements

### Phase 3: Modal Support (Week 2)
- [ ] Modal blocker rendering
- [ ] Event blocking for modals
- [ ] Focus trapping
- [ ] Escape key handling

### Phase 4: Animation (Week 3)
- [ ] `layer_animator` implementation
- [ ] Fade in/out
- [ ] Slide animations
- [ ] Integration with layer show/hide

### Phase 5: Integration (Week 3)
- [ ] Integrate with `ui_handle`
- [ ] Update `menu_bar` to use layers
- [ ] Example applications
- [ ] Documentation

### Phase 6: Polish (Week 4)
- [ ] Click-outside detection
- [ ] Nested popup support
- [ ] Tooltip auto-hide
- [ ] Performance optimization
- [ ] Debug visualization

## Alternative Designs Considered

### Alternative 1: Single Root with Z-Index Property
Add `z_index` property to every `ui_element`:
```cpp
element->set_z_index(100);
```

**Pros**: Simple, no separate layer concept
**Cons**:
- Elements mixed in tree, hard to manage
- No clean separation of base UI vs overlays
- Performance: must sort entire tree on every render

### Alternative 2: Multiple ui_handle Instances
Create separate `ui_handle` for each layer:
```cpp
ui_handle base_ui;
ui_handle popup_ui;
ui_handle modal_ui;
```

**Pros**: Complete isolation
**Cons**:
- Multiple renderers, multiple focus managers
- No coordination for events
- Memory overhead
- Complex to manage

### Alternative 3: Render to Texture
Render each layer to separate texture/surface:
```cpp
layer->render_to_texture();
composite_textures();
```

**Pros**: Could enable effects (blur, shadow)
**Cons**:
- Not all backends support this (termbox2!)
- Performance overhead
- Memory overhead
- Complexity

**Chosen Approach**: Layer manager (Proposed) provides best balance of flexibility, performance, and integration with existing code.

## Success Criteria

✅ Menu dropdowns work (fixes current menu_bar issue)
✅ Modal dialogs block underlying UI
✅ Tooltips show on top without breaking layout
✅ Context menus can nest properly
✅ Escape key closes top layer
✅ Click outside closes popups
✅ Animations are smooth
✅ Performance: <5% overhead for layers
✅ Backward compatible (base UI unchanged)
✅ Comprehensive tests
✅ Documentation and examples

## Open Questions for Review

1. **Ownership**: Should layer_manager own layer content, or use weak references?
   - Current design: Owns via unique_ptr (simpler)
   - Alternative: Weak references (more flexible, but complex lifetime)

2. **Layer Limits**: Should we limit number of active layers?
   - Proposal: Max 32 layers (unlikely to hit in practice)

3. **Animation Library**: Build our own or integrate existing?
   - Proposal: Simple built-in (fade, slide) for MVP

4. **Click-Outside**: Should ALL non-modal layers close on click-outside?
   - Proposal: Configurable per-layer via flag

5. **Focus Management**: Should layer_manager own a separate focus manager?
   - Proposal: Use existing hierarchical_focus_manager, trap within layers

6. **Backend Support**: How to handle backends without alpha blending?
   - Proposal: Fallback to no animation, still functional

7. **Memory**: Keep hidden layers in memory or destroy?
   - Proposal: Destroy after hide animation completes

## Conclusion

The **layer manager** system provides essential infrastructure for modern UI patterns:
- Solves the menu dropdown problem immediately
- Enables modal dialogs and tooltips
- Maintains backward compatibility
- Follows existing architecture patterns
- Provides smooth animations and good UX

**Next Steps**: Review design, discuss open questions, implement Phase 1 core infrastructure.
