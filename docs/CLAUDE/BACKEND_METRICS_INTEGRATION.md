# Backend Metrics Integration Plan

## Executive Summary

The `backend_metrics` system provides logical-to-physical coordinate conversion but was never integrated into the framework. This document describes the complete integration plan to enable proper scaling between terminal (1 char = 1 logical unit) and GUI (8 pixels = 1 logical unit) backends.

---

## 1. Current State Analysis

### 1.1 The Problem

The framework uses `logical_unit` throughout layout, but conversion to physical coordinates is broken:

```cpp
// ui_handle.hh - viewport passed in RAW PHYSICAL UNITS
m_root->measure(
    logical_unit(static_cast<double>(rect_utils::get_width(bounds))),  // pixels!
    logical_unit(static_cast<double>(rect_utils::get_height(bounds))));
```

```cpp
// types.hh - no scaling applied
[[nodiscard]] constexpr int to_int() const noexcept {
    return static_cast<int>(std::round(value));  // Just rounds, no scaling!
}
```

**Result**: 1 logical unit = 1 pixel in sdlpp, 1 logical unit = 1 char in conio. Widgets returning `logical_unit(1.0)` for height render as 1 pixel (invisible) instead of 8 pixels.

### 1.2 What Exists

`backend_metrics.hh` provides complete conversion infrastructure:

| Function | Purpose |
|----------|---------|
| `snap_to_physical_x/y()` | Convert logical → physical with snapping |
| `snap_size()` | Convert `logical_size` → `size_type` |
| `snap_rect()` | Convert `logical_rect` → `rect_type` (edge-based) |
| `physical_to_logical_x/y()` | Convert physical → logical |
| `physical_to_logical_size/rect()` | Convert physical types → logical |
| `make_terminal_metrics()` | Factory for 1:1 terminal scaling |
| `make_gui_metrics()` | Factory for 8:1 GUI scaling |

### 1.3 Affected Widgets

Widgets that hardcode logical sizes expecting scaling:
- `slider` - returns `logical_unit(1.0)` for track height
- `scrollbar` - arrow buttons, thumb sizes
- `progress_bar` - track height
- Any widget using fixed logical dimensions

---

## 2. Design Goals

### 2.1 Core Principles

1. **Semantic Logical Units**: Widgets express sizes in meaningful units (e.g., "1 line tall")
2. **Backend-Agnostic Layout**: Layout algorithm works identically across backends
3. **Late Physical Conversion**: Convert to physical only at render boundaries
4. **Zero Breaking Changes**: Existing code continues to work during migration

### 2.2 Coordinate Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                        PHYSICAL DOMAIN                          │
│  (pixels for SDL, characters for conio)                         │
│                                                                 │
│  Viewport: 1280×720 pixels (SDL) or 80×25 chars (conio)        │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼ physical_to_logical_size()
┌─────────────────────────────────────────────────────────────────┐
│                        LOGICAL DOMAIN                           │
│  (backend-agnostic units)                                       │
│                                                                 │
│  Viewport: 160×90 logical units (both backends!)               │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  measure() / arrange() / layout algorithms              │   │
│  │  All widget sizes and positions in logical units        │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼ snap_rect() / snap_size()
┌─────────────────────────────────────────────────────────────────┐
│                        PHYSICAL DOMAIN                          │
│                                                                 │
│  render() calls use physical coordinates for actual drawing    │
│  fill_rect({10, 20, 80, 56}) - pixels or chars                │
└─────────────────────────────────────────────────────────────────┘
```

### 2.3 Scaling Factors

| Backend | logical_to_physical | Meaning |
|---------|---------------------|---------|
| conio | 1.0 | 1 logical unit = 1 character cell |
| sdlpp | 8.0 | 1 logical unit = 8 pixels |
| sdlpp @2x | 8.0 × 2.0 = 16.0 | 1 logical unit = 16 pixels (HiDPI) |

With 8:1 scaling, a 1280×720 pixel window becomes 160×90 logical units—comparable to an 80×25 terminal doubled.

---

## 3. Implementation Plan

### Phase 1: Infrastructure (No Behavior Change)

#### 3.1.1 Add Metrics to UI Context

**File**: `include/onyxui/services/ui_context.hh`

```cpp
template<UIBackend Backend>
class ui_context {
    // ... existing members ...

    backend_metrics<Backend> m_metrics;

public:
    // Constructor accepts metrics
    explicit ui_context(backend_metrics<Backend> metrics = {})
        : m_metrics(std::move(metrics)) {
        // ... existing initialization ...
    }

    [[nodiscard]] const backend_metrics<Backend>& metrics() const noexcept {
        return m_metrics;
    }

    [[nodiscard]] backend_metrics<Backend>& metrics() noexcept {
        return m_metrics;
    }
};
```

#### 3.1.2 Add Metrics to UI Services

**File**: `include/onyxui/services/ui_services.hh`

```cpp
template<UIBackend Backend>
class ui_services {
    // ... existing static members ...

    static inline const backend_metrics<Backend>* s_metrics = nullptr;

public:
    static void set_metrics(const backend_metrics<Backend>* m) { s_metrics = m; }
    static const backend_metrics<Backend>* metrics() { return s_metrics; }
};
```

#### 3.1.3 Update scoped_ui_context

**File**: `include/onyxui/services/scoped_ui_context.hh`

```cpp
template<UIBackend Backend>
class scoped_ui_context {
public:
    explicit scoped_ui_context(backend_metrics<Backend> metrics = {})
        : m_context(std::move(metrics)) {
        // ... existing setup ...
        ui_services<Backend>::set_metrics(&m_context.metrics());
    }

    ~scoped_ui_context() {
        ui_services<Backend>::set_metrics(nullptr);
        // ... existing cleanup ...
    }
};
```

#### 3.1.4 Backend-Specific Context Creation

**File**: `backends/conio/main.cc`

```cpp
// Create context with terminal metrics
scoped_ui_context<conio_backend> ctx(make_terminal_metrics<conio_backend>());
```

**File**: `backends/sdlpp/demo.cc`

```cpp
// Create context with GUI metrics (optionally with DPI scale)
double dpi_scale = get_display_dpi_scale();  // Platform-specific
scoped_ui_context<sdlpp_backend> ctx(make_gui_metrics<sdlpp_backend>(dpi_scale));
```

---

### Phase 2: Viewport Conversion

#### 3.2.1 Convert Viewport at Entry Point

**File**: `include/onyxui/ui_handle.hh`

```cpp
void display_impl(const rect_type& physical_bounds) {
    // Get metrics from services
    const auto* metrics = ui_services<Backend>::metrics();
    if (!metrics) {
        // Fallback: use 1:1 mapping (backward compatibility)
        static const backend_metrics<Backend> default_metrics{};
        metrics = &default_metrics;
    }

    // Convert physical viewport to logical
    const logical_size logical_viewport = metrics->physical_to_logical_size(
        size_type{
            rect_utils::get_width(physical_bounds),
            rect_utils::get_height(physical_bounds)
        }
    );

    // Layout in logical coordinates
    [[maybe_unused]] auto measured_size = m_root->measure(
        logical_viewport.width,
        logical_viewport.height
    );

    m_root->arrange(logical_rect{
        logical_unit(0.0),
        logical_unit(0.0),
        logical_viewport.width,
        logical_viewport.height
    });

    // ... rest of rendering ...
}
```

---

### Phase 3: Render Context Conversion

#### 3.3.1 Add Metrics to render_context

**File**: `include/onyxui/core/rendering/render_context.hh`

```cpp
template<UIBackend Backend>
class render_context {
protected:
    const backend_metrics<Backend>* m_metrics = nullptr;

public:
    // Add metrics to constructor
    explicit render_context(
        const resolved_style<Backend>& style,
        const point_type& position,
        const size_type& available_size,
        const theme<Backend>* theme,
        const backend_metrics<Backend>* metrics = nullptr
    ) : /* ... existing ... */, m_metrics(metrics) {}

    [[nodiscard]] const backend_metrics<Backend>* metrics() const noexcept {
        return m_metrics;
    }

    // Convenience: convert logical rect to physical for drawing
    [[nodiscard]] rect_type to_physical(const logical_rect& rect) const {
        if (m_metrics) {
            return m_metrics->snap_rect(rect);
        }
        // Fallback: direct conversion
        return rect_type{
            rect.x.to_int(), rect.y.to_int(),
            rect.width.to_int(), rect.height.to_int()
        };
    }
};
```

#### 3.3.2 Update draw_context

**File**: `include/onyxui/core/rendering/draw_context.hh`

```cpp
template<UIBackend Backend>
class draw_context : public render_context<Backend> {
public:
    explicit draw_context(
        renderer_type& renderer,
        const resolved_style<Backend>& style,
        const point_type& position,
        const size_type& available_size,
        const backend_metrics<Backend>* metrics = nullptr
    ) : base(style, position, available_size, nullptr, metrics)
      , m_renderer(&renderer) {}

    // Logical rect version of fill_rect
    void fill_rect_logical(const logical_rect& bounds, const color_type& color) {
        fill_rect(this->to_physical(bounds), color);
    }
};
```

#### 3.3.3 Pass Metrics Through Render Chain

**File**: `include/onyxui/core/element.hh` (render method)

```cpp
void render(renderer_type& renderer, const theme<Backend>* theme) {
    // ... existing visibility/clipping checks ...

    // Get metrics from services
    const auto* metrics = ui_services<Backend>::metrics();

    // Convert logical bounds to physical for rendering
    rect_type physical_bounds;
    if (metrics) {
        physical_bounds = metrics->snap_rect(m_bounds);
    } else {
        physical_bounds = rect_type{
            m_bounds.x.to_int(), m_bounds.y.to_int(),
            m_bounds.width.to_int(), m_bounds.height.to_int()
        };
    }

    // Create draw context with physical position/size
    draw_context<Backend> ctx(
        renderer,
        resolved_style,
        point_type{rect_utils::get_x(physical_bounds), rect_utils::get_y(physical_bounds)},
        size_type{rect_utils::get_width(physical_bounds), rect_utils::get_height(physical_bounds)},
        metrics
    );

    do_render(ctx);

    // ... render children ...
}
```

---

### Phase 4: Widget Updates

#### 3.4.1 Update Slider

**File**: `include/onyxui/widgets/input/slider.hh`

```cpp
[[nodiscard]] logical_size get_content_size() const override {
    // Sizes in logical units - backend-agnostic!
    constexpr double TRACK_LENGTH = 20.0;   // 20 logical units long
    constexpr double TRACK_THICKNESS = 1.5; // 1.5 logical units thick

    if (m_orientation == slider_orientation::horizontal) {
        return logical_size{logical_unit(TRACK_LENGTH), logical_unit(TRACK_THICKNESS)};
    } else {
        return logical_size{logical_unit(TRACK_THICKNESS), logical_unit(TRACK_LENGTH)};
    }
}

void do_render(render_context<Backend>& ctx) const override {
    // Widget receives physical coordinates in ctx.position() and ctx.available_size()
    // Just draw using those - no conversion needed in widget code!

    int x = point_utils::get_x(ctx.position());
    int y = point_utils::get_y(ctx.position());
    int width = size_utils::get_width(ctx.available_size());
    int height = size_utils::get_height(ctx.available_size());

    // Draw track, thumb, etc. using physical coordinates
    ctx.fill_rect(rect_type{x, y, width, height}, track_color);
}
```

#### 3.4.2 Update Other Widgets

Similar updates for:
- `scrollbar` - button sizes, thumb dimensions
- `progress_bar` - track height
- `checkbox` - box size
- `radio_button` - circle size

---

### Phase 5: Hit Testing Conversion

#### 3.5.1 Convert Mouse Coordinates

**File**: `include/onyxui/ui_handle.hh` (handle_event)

```cpp
if (auto* mouse_evt = std::get_if<mouse_event>(&native.event)) {
    const auto* metrics = ui_services<Backend>::metrics();

    // Convert physical mouse position to logical
    int logical_x = mouse_evt->x;
    int logical_y = mouse_evt->y;

    if (metrics) {
        logical_x = metrics->physical_to_logical_x(mouse_evt->x).to_int();
        logical_y = metrics->physical_to_logical_y(mouse_evt->y).to_int();
    }

    // Hit test uses logical coordinates
    auto* target = m_root->hit_test(logical_x, logical_y, path);
    // ...
}
```

---

## 4. Migration Strategy

### 4.1 Backward Compatibility

The default `backend_metrics{}` uses 1:1 scaling, so existing code works unchanged:

```cpp
// Old code - still works (1:1 scaling)
scoped_ui_context<Backend> ctx;

// New code - explicit metrics
scoped_ui_context<Backend> ctx(make_gui_metrics<Backend>());
```

### 4.2 Phased Rollout

| Phase | Changes | Risk |
|-------|---------|------|
| 1 | Add metrics infrastructure | None - no behavior change |
| 2 | Convert viewport | Low - logical coordinates flow through |
| 3 | Convert render context | Medium - widgets receive scaled coords |
| 4 | Update widgets | Low - use new logical sizes |
| 5 | Convert hit testing | Medium - mouse coords need conversion |

### 4.3 Feature Flag (Optional)

```cpp
// In backend_metrics.hh
constexpr bool ENABLE_METRICS_SCALING = true;  // Toggle for testing

// In conversion code
if constexpr (ENABLE_METRICS_SCALING) {
    return metrics->snap_rect(logical_bounds);
} else {
    return direct_conversion(logical_bounds);
}
```

---

## 5. Testing Strategy

### 5.1 Unit Tests

**File**: `unittest/core/test_backend_metrics.cc`

```cpp
TEST_CASE("backend_metrics - GUI scaling") {
    auto metrics = make_gui_metrics<test_backend>();

    SUBCASE("logical to physical") {
        CHECK(metrics.snap_to_physical_x(logical_unit(1.0), snap_mode::round) == 8);
        CHECK(metrics.snap_to_physical_y(logical_unit(2.0), snap_mode::round) == 16);
    }

    SUBCASE("physical to logical") {
        CHECK(metrics.physical_to_logical_x(8).value == doctest::Approx(1.0));
        CHECK(metrics.physical_to_logical_y(16).value == doctest::Approx(2.0));
    }

    SUBCASE("size conversion") {
        logical_size ls{logical_unit(10.0), logical_unit(5.0)};
        auto ps = metrics.snap_size(ls);
        CHECK(size_utils::get_width(ps) == 80);
        CHECK(size_utils::get_height(ps) == 40);
    }

    SUBCASE("rect edge snapping") {
        // Ensures no gaps between adjacent rects
        logical_rect r1{logical_unit(0.0), logical_unit(0.0), logical_unit(1.5), logical_unit(1.0)};
        logical_rect r2{logical_unit(1.5), logical_unit(0.0), logical_unit(1.5), logical_unit(1.0)};

        auto p1 = metrics.snap_rect(r1);
        auto p2 = metrics.snap_rect(r2);

        // Right edge of r1 should equal left edge of r2
        CHECK(rect_utils::get_x(p1) + rect_utils::get_width(p1) == rect_utils::get_x(p2));
    }
}

TEST_CASE("backend_metrics - Terminal scaling") {
    auto metrics = make_terminal_metrics<test_backend>();

    SUBCASE("1:1 mapping") {
        CHECK(metrics.snap_to_physical_x(logical_unit(10.0), snap_mode::round) == 10);
        CHECK(metrics.snap_to_physical_y(logical_unit(25.0), snap_mode::round) == 25);
    }
}
```

### 5.2 Integration Tests

```cpp
TEST_CASE("slider renders correctly with metrics") {
    // Test with GUI metrics
    auto gui_metrics = make_gui_metrics<test_backend>();
    test_context ctx(gui_metrics);

    auto slider = std::make_unique<slider<test_backend>>();
    slider->measure(logical_unit(100.0), logical_unit(20.0));
    slider->arrange(logical_rect{0, 0, 100, 20});

    // Verify physical render size
    auto physical_bounds = gui_metrics.snap_rect(slider->bounds());
    CHECK(rect_utils::get_height(physical_bounds) == 12);  // 1.5 * 8 = 12 pixels
}
```

### 5.3 Visual Tests

1. Run `conio` demo - verify slider is 1 char tall
2. Run `sdlpp` demo - verify slider is ~12 pixels tall
3. Resize sdlpp window - verify layout scales correctly
4. Test HiDPI scaling (if available)

---

## 6. Files to Modify

### Core Framework

| File | Changes |
|------|---------|
| `include/onyxui/core/backend_metrics.hh` | Already complete |
| `include/onyxui/core/types.hh` | No changes needed |
| `include/onyxui/services/ui_context.hh` | Add metrics storage |
| `include/onyxui/services/ui_services.hh` | Add metrics accessor |
| `include/onyxui/services/scoped_ui_context.hh` | Pass metrics to context |
| `include/onyxui/ui_handle.hh` | Convert viewport, hit test coords |
| `include/onyxui/core/element.hh` | Convert bounds in render() |
| `include/onyxui/core/rendering/render_context.hh` | Add metrics, to_physical() |
| `include/onyxui/core/rendering/draw_context.hh` | Pass metrics to base |
| `include/onyxui/core/rendering/measure_context.hh` | Pass metrics to base |

### Widgets

| File | Changes |
|------|---------|
| `include/onyxui/widgets/input/slider.hh` | Use logical sizes |
| `include/onyxui/widgets/input/progress_bar.hh` | Use logical sizes |
| `include/onyxui/widgets/input/checkbox.hh` | Use logical sizes |
| `include/onyxui/widgets/input/radio_button.hh` | Use logical sizes |
| `include/onyxui/widgets/containers/scroll/scrollbar.hh` | Use logical sizes |

### Backends

| File | Changes |
|------|---------|
| `backends/conio/main.cc` | Create context with terminal metrics |
| `backends/sdlpp/demo.cc` | Create context with GUI metrics |

### Tests

| File | Changes |
|------|---------|
| `unittest/core/test_backend_metrics.cc` | New file - comprehensive tests |
| `unittest/widgets/test_slider.cc` | Update for metrics |

---

## 7. Open Questions

### 7.1 Text Measurement

Text is currently measured in physical pixels. Options:

1. **Keep physical text measurement** - Text sizes remain in pixels, converted to logical for layout
2. **Add logical text measurement** - New API returns logical sizes

Recommendation: Option 1 - less invasive, text rendering is already physical.

### 7.2 Theme Values

Theme padding/margin values are currently physical. Options:

1. **Keep physical theme values** - Different themes for different backends
2. **Convert to logical** - One theme works everywhere

Recommendation: Option 1 for now - allows backend-specific fine-tuning.

### 7.3 DPI Scaling

How to detect and apply DPI scale:

```cpp
// Platform-specific DPI detection
#ifdef _WIN32
    // GetDpiForWindow()
#elif defined(__APPLE__)
    // NSScreen backingScaleFactor
#else
    // SDL_GetDisplayDPI() or environment variable
#endif
```

---

## 8. Timeline Estimate

| Phase | Effort | Dependencies |
|-------|--------|--------------|
| Phase 1: Infrastructure | 2-3 hours | None |
| Phase 2: Viewport | 1-2 hours | Phase 1 |
| Phase 3: Render Context | 3-4 hours | Phase 2 |
| Phase 4: Widgets | 2-3 hours | Phase 3 |
| Phase 5: Hit Testing | 1-2 hours | Phase 2 |
| Testing & Polish | 2-3 hours | All phases |

**Total: ~12-17 hours**

---

## 9. Success Criteria

1. Slider renders at appropriate size in both backends
2. All 1184 existing tests pass
3. No visual regressions in demos
4. Mouse interaction works correctly in both backends
5. Window resize works correctly
6. (Optional) HiDPI displays scale correctly

---

## Appendix A: Snap Mode Reference

```cpp
enum class snap_mode {
    floor,  // Round down - for positions (left, top)
    ceil,   // Round up - for far edges (right, bottom)
    round,  // Nearest - for sizes computed directly
    none    // No rounding - intermediate calculations
};
```

Edge-based snapping prevents gaps:

```
Logical:  [0.0 ──── 1.5][1.5 ──── 3.0]
Physical: [0 ─────── 12][12 ────── 24]  (no gap!)
```

---

## Appendix B: Quick Reference

```cpp
// Create metrics
auto terminal = make_terminal_metrics<Backend>();  // 1:1
auto gui = make_gui_metrics<Backend>(1.0);         // 8:1
auto hidpi = make_gui_metrics<Backend>(2.0);       // 16:1

// Convert to physical
int px = metrics.snap_to_physical_x(lu, snap_mode::floor);
rect_type rect = metrics.snap_rect(logical_rect);
size_type size = metrics.snap_size(logical_size);

// Convert to logical
logical_unit lu = metrics.physical_to_logical_x(px);
logical_rect rect = metrics.physical_to_logical_rect(physical_rect);
```
