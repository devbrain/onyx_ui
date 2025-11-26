# Logical Units - User Guide

**Version:** 3.0.0
**Audience:** OnyxUI Developers
**Level:** Beginner to Intermediate

---

## Table of Contents

1. [What Are Logical Units?](#what-are-logical-units)
2. [Understanding Coordinates](#understanding-coordinates)
3. [Screen Size and Layout](#screen-size-and-layout)
4. [Practical Examples](#practical-examples)
5. [Common Patterns](#common-patterns)
6. [Best Practices](#best-practices)
7. [Troubleshooting](#troubleshooting)

---

## What Are Logical Units?

### The Simple Answer

**Logical units are abstract coordinates** that work the same way on different backends (terminal vs graphical window).

Think of them like:
- **CSS `em` units** - relative to font size, browser converts to pixels
- **Android `dp`** - density-independent pixels, system converts to screen pixels
- **Inches on a ruler** - everyone understands "5 inches" regardless of the ruler used

### The Problem They Solve

**Before (integer coordinates):**
```cpp
widget->set_size(80, 25);

// What does this mean?
// - On terminal: 80 characters × 25 rows ✓
// - On SDL window: 80 pixels × 25 pixels (TINY!) ✗
// Same code, completely different results!
```

**After (logical units):**
```cpp
widget->set_size(80_lu, 25_lu);

// This means "80 logical units × 25 logical units"
// - On terminal (1 char/lu): 80 characters × 25 rows ✓
// - On SDL (8 px/lu): 640 pixels × 200 pixels ✓
// Same code, appropriate size on both!
```

### Real-World Example

**Your code (backend-agnostic):**
```cpp
widget->set_position(10.5_lu, 5.25_lu);
widget->set_size(80_lu, 25_lu);
```

**Terminal backend (conio):**
```
Configuration: 1 logical unit = 1 character cell

Position: 10.5 lu × 5.25 lu
  → 10.5 chars × 5.25 rows
  → Rounded: 11 columns, 5 rows

Size: 80 lu × 25 lu
  → 80 chars × 25 rows
  → Exact: 80 columns, 25 rows
```

**Graphical backend (SDL):**
```
Configuration: 1 logical unit = 8 pixels (default)

Position: 10.5 lu × 5.25 lu
  → 10.5 × 8 = 84 pixels
  → 5.25 × 8 = 42 pixels

Size: 80 lu × 25 lu
  → 80 × 8 = 640 pixels
  → 25 × 8 = 200 pixels
```

**Same code → appropriate results on both backends!**

---

## Understanding Coordinates

### Why Fractional Values?

**Question:** Why can I write `10.5_lu` (half a unit)?

**Answer:** Because logical units are **abstract** - they only become concrete pixels/chars when rendering.

#### Example 1: Perfect Centering

```cpp
// Parent is 100 logical units wide
// Child is 30 logical units wide

// Center the child
logical_unit center_x = (parent_width - child_width) * 0.5;
// center_x = (100 - 30) * 0.5 = 35.0 logical units

child->set_position(center_x, 0_lu);
```

**On terminal (1 char/lu):**
- 35.0 logical units → 35 columns (exact!)

**On SDL (8 px/lu):**
- 35.0 logical units → 280 pixels (exact!)

**Perfect centering on both!**

#### Example 2: Percentage Layouts (The Real Power!)

```cpp
// Divide 100 logical units into thirds
// OLD system (integers): 100 / 3 = 33, 33, 33 = 99 (LOST 1 UNIT!)

logical_unit total = 100_lu;

// NEW system (logical units):
logical_unit width1 = total * (1.0 / 3.0);  // 33.333... lu
logical_unit width2 = total * (1.0 / 3.0);  // 33.333... lu
logical_unit width3 = total * (1.0 / 3.0);  // 33.333... lu

// Sum: 33.333 + 33.333 + 33.333 = 100.0 (EXACT!)
```

**When rendering on terminal (1 char/lu):**
```
widget1: 33.333 → 33 chars (floor)
widget2: 33.333 → 34 chars (ceil to fill gap)
widget3: 33.333 → 33 chars (floor)
Total:   33 + 34 + 33 = 100 chars ✓ (no lost space!)
```

**When rendering on SDL (8 px/lu):**
```
widget1: 33.333 × 8 = 266.67 → 267 pixels (ceil)
widget2: 33.333 × 8 = 266.67 → 266 pixels (floor)
widget3: 33.333 × 8 = 266.67 → 267 pixels (ceil)
Total:   267 + 266 + 267 = 800 pixels ✓ (no gaps!)
```

**Key insight:** Fractions preserved during calculation, rounded only at final render!

#### Example 3: Smooth Animations

```cpp
// Animate widget from x=0 to x=100 over 60 frames
logical_unit start_x = 0_lu;
logical_unit end_x = 100_lu;

for (int frame = 0; frame < 60; ++frame) {
    double progress = frame / 60.0;  // 0.0 → 1.0

    logical_unit current_x = start_x + (end_x - start_x) * progress;
    widget->set_position(current_x, 10_lu);

    // Frame 0:  current_x = 0.0 lu
    // Frame 1:  current_x = 1.667 lu (fractional!)
    // Frame 2:  current_x = 3.333 lu
    // Frame 30: current_x = 50.0 lu
    // Frame 59: current_x = 98.333 lu
    // Frame 60: current_x = 100.0 lu

    render();
}
```

**Result:** Smooth animation (not choppy jumps by whole pixels/chars)!

### How Rounding Works

**Important:** You **never** round yourself. The rendering system does it automatically.

```cpp
// YOUR CODE: Use exact fractions
logical_unit x = 10.5_lu;
logical_unit y = 5.25_lu;
widget->set_position(x, y);

// RENDERING (automatic):
// Terminal: Rounds to (11 chars, 5 rows)
// SDL:      Converts to (84 pixels, 42 pixels) - no rounding needed!
```

**Rounding Strategy (automatic):**
- **Position:** Floor (round down) - prevents overlaps
- **Far edge:** Ceil (round up) - prevents gaps
- **Size:** Computed from edges - consistent total

```cpp
// Three widgets side-by-side, each 33.333 lu wide
// Position widget 1: floor(0.0) = 0
// Position widget 2: floor(33.333) = 33
// Position widget 3: floor(66.667) = 66
// Right edge: ceil(100.0) = 100
// Result: 0-33, 33-66, 66-100 (no gaps, no overlaps!)
```

---

## Screen Size and Layout

### How Screen Size is Determined

**The flow:**
```
Physical Device (80 chars × 25 rows, or 800px × 600px)
    ↓
Backend detects physical size
    ↓
Backend converts to logical units (using scaling factor)
    ↓
YOUR CODE gets logical screen size
    ↓
You layout widgets in logical space
```

### Example: Terminal (80×25)

```cpp
// Physical screen
Terminal size: 80 columns × 25 rows

// Backend configuration
backend_metrics<conio_backend> metrics;
metrics.logical_to_physical_x = 1.0;  // 1 logical unit = 1 char
metrics.logical_to_physical_y = 1.0;  // 1 logical unit = 1 row

// Conversion to logical units
logical_unit screen_width = metrics.from_physical_x(80);   // 80.0 lu
logical_unit screen_height = metrics.from_physical_y(25);  // 25.0 lu

// YOUR CODE: Query screen size
auto screen = get_screen_size();
std::cout << "Screen: "
          << screen.width.to_double() << " × "
          << screen.height.to_double() << " logical units\n";
// Output: "Screen: 80.0 × 25.0 logical units"

// Find the middle
logical_unit middle_x = screen.width * 0.5;   // 40.0 lu
logical_unit middle_y = screen.height * 0.5;  // 12.5 lu

// Position widget at center
widget->set_position(middle_x, middle_y);
// Result: Widget at (40 columns, 13 rows) - centered!
```

### Example: SDL Window (800×600 pixels)

```cpp
// Physical screen
SDL window: 800 pixels × 600 pixels

// Backend configuration
backend_metrics<sdl_backend> metrics;
metrics.logical_to_physical_x = 8.0;  // 1 logical unit = 8 pixels
metrics.logical_to_physical_y = 8.0;  // 1 logical unit = 8 pixels

// Conversion to logical units
logical_unit screen_width = metrics.from_physical_x(800);   // 100.0 lu
logical_unit screen_height = metrics.from_physical_y(600);  // 75.0 lu

// YOUR CODE: Query screen size (SAME AS TERMINAL EXAMPLE!)
auto screen = get_screen_size();
std::cout << "Screen: "
          << screen.width.to_double() << " × "
          << screen.height.to_double() << " logical units\n";
// Output: "Screen: 100.0 × 75.0 logical units"

// Find the middle (SAME CODE!)
logical_unit middle_x = screen.width * 0.5;   // 50.0 lu
logical_unit middle_y = screen.height * 0.5;  // 37.5 lu

// Position widget at center (SAME CODE!)
widget->set_position(middle_x, middle_y);
// Result: Widget at (400 pixels, 300 pixels) - centered!
```

### Visual Comparison: Same Code, Different Screens

**Your code:**
```cpp
auto screen = get_screen_size();
auto window = std::make_unique<window<Backend>>("My App");
window->set_size(60_lu, 20_lu);

// Center the window
logical_unit x = (screen.width - 60_lu) * 0.5;
logical_unit y = (screen.height - 20_lu) * 0.5;
window->set_position(x, y);
```

**Terminal (80×25 chars):**
```
Screen size: 80.0 × 25.0 logical units

┌──────────────────────────────────────────────────────────────────────────────┐
│                                                                              │
│                                                                              │
│          ┌───────────────────────My App──────────────────────────┐          │
│          │                                                        │          │
│          │                                                        │          │
│          │                                                        │          │
│          │                   Window Content                      │          │
│          │                 (60 chars × 20 rows)                  │          │
│          │                                                        │          │
│          │                                                        │          │
│          │                                                        │          │
│          │                                                        │          │
│          └────────────────────────────────────────────────────────┘          │
│                                                                              │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘

Calculations:
  x = (80.0 - 60.0) * 0.5 = 10.0 lu → 10 columns
  y = (25.0 - 20.0) * 0.5 = 2.5 lu → 3 rows (rounded up)
  Window position: (10, 3)
  Window size: 60×20 chars
```

**SDL (1600×900 pixels, 8 px/lu):**
```
Screen size: 200.0 × 112.5 logical units

┌──────────────────────────────────────────────────────────────────────────────┐
│                                                                              │
│                                                                              │
│                                                                              │
│                                                                              │
│                                                                              │
│                        ┌─────────My App──────────┐                          │
│                        │                         │                          │
│                        │                         │                          │
│                        │    Window Content       │                          │
│                        │  (60 lu × 20 lu)       │                          │
│                        │ (480px × 160px)        │                          │
│                        │                         │                          │
│                        │                         │                          │
│                        └─────────────────────────┘                          │
│                                                                              │
│                                                                              │
│                                                                              │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘

Calculations:
  x = (200.0 - 60.0) * 0.5 = 70.0 lu → 560 pixels
  y = (112.5 - 20.0) * 0.5 = 46.25 lu → 370 pixels
  Window position: (560, 370)
  Window size: 480×160 pixels
```

**Same code → perfect centering on both screens!**

### How You Get Screen Size

**Method 1: In measure() callback (automatic)**
```cpp
template<UIBackend Backend>
class my_widget : public widget<Backend> {
    logical_size measure(logical_unit available_width,
                        logical_unit available_height) override {
        // available_width and available_height are passed from parent
        // For root widget, these ARE the screen size!

        std::cout << "Available: "
                  << available_width.to_double() << " × "
                  << available_height.to_double() << " lu\n";

        // Return desired size (e.g., 80% of available)
        return logical_size{
            available_width * 0.8,
            available_height * 0.6
        };
    }
};
```

**Method 2: Query from ui_context**
```cpp
// Anywhere in your code
auto* ctx = ui_services<Backend>::context();
auto screen_size = ctx->get_screen_size();  // Returns logical_size

std::cout << "Screen: "
          << screen_size.width.to_double() << " × "
          << screen_size.height.to_double() << " logical units\n";
```

**Method 3: Calculate from physical size**
```cpp
// Get physical size from backend
auto& backend = get_backend<Backend>();
auto physical_size = backend.get_physical_screen_size();
// Returns {width: int, height: int} in pixels or chars

// Convert to logical
auto& metrics = backend.metrics();
logical_size logical_screen{
    metrics.from_physical_x(physical_size.width),
    metrics.from_physical_y(physical_size.height)
};
```

---

## Practical Examples

### Example 1: Full-Screen Window

```cpp
template<UIBackend Backend>
void create_fullscreen_window() {
    auto window = std::make_unique<window<Backend>>("Full Screen");

    // Get screen size
    auto screen = get_screen_size();

    // Fill entire screen
    window->set_position(0_lu, 0_lu);
    window->set_size(screen.width, screen.height);

    window->show();
}

// On terminal (80×25):   Window fills 80×25 chars
// On SDL (800×600, 8px): Window fills 100×75 lu (800×600 pixels)
```

### Example 2: Centered Dialog

```cpp
template<UIBackend Backend>
void show_centered_dialog(const std::string& message) {
    auto dialog = std::make_unique<window<Backend>>("Alert");

    // Fixed size dialog
    logical_unit dialog_width = 40_lu;
    logical_unit dialog_height = 10_lu;
    dialog->set_size(dialog_width, dialog_height);

    // Add message
    auto label = std::make_unique<label<Backend>>(message);
    dialog->set_content(std::move(label));

    // Center on screen
    auto screen = get_screen_size();
    logical_unit x = (screen.width - dialog_width) * 0.5;
    logical_unit y = (screen.height - dialog_height) * 0.5;
    dialog->set_position(x, y);

    dialog->show_modal();
}

// Works perfectly on any screen size!
```

### Example 3: Proportional Layout

```cpp
template<UIBackend Backend>
void create_split_layout() {
    auto screen = get_screen_size();

    // Left panel: 30% of screen width
    auto left_panel = std::make_unique<panel<Backend>>();
    left_panel->set_size(screen.width * 0.3, screen.height);
    left_panel->set_position(0_lu, 0_lu);

    // Right panel: 70% of screen width
    auto right_panel = std::make_unique<panel<Backend>>();
    right_panel->set_size(screen.width * 0.7, screen.height);
    right_panel->set_position(screen.width * 0.3, 0_lu);

    // Works on any screen size:
    // - Terminal 80×25:  left=24 chars, right=56 chars
    // - SDL 800×600:     left=30 lu (240px), right=70 lu (560px)
    // - SDL 1600×1200:   left=60 lu (480px), right=140 lu (1120px)
}
```

### Example 4: Bounds Checking

```cpp
template<UIBackend Backend>
void position_widget_safe(widget<Backend>* w, logical_unit x, logical_unit y) {
    auto screen = get_screen_size();
    auto widget_size = w->desired_size();

    // Clamp to screen bounds
    if (x < 0_lu) {
        x = 0_lu;
    }
    if (y < 0_lu) {
        y = 0_lu;
    }

    // Check right edge
    if (x + widget_size.width > screen.width) {
        x = screen.width - widget_size.width;
    }

    // Check bottom edge
    if (y + widget_size.height > screen.height) {
        y = screen.height - widget_size.height;
    }

    w->set_position(x, y);
}
```

### Example 5: Responsive Sizing

```cpp
template<UIBackend Backend>
void create_responsive_button() {
    auto screen = get_screen_size();
    auto button = std::make_unique<button<Backend>>("Click Me");

    // Small screen: fixed size button
    if (screen.width < 40_lu) {
        button->set_size(10_lu, 3_lu);
    }
    // Medium screen: larger button
    else if (screen.width < 100_lu) {
        button->set_size(15_lu, 4_lu);
    }
    // Large screen: percentage-based sizing
    else {
        button->set_size(screen.width * 0.15, 5_lu);
    }

    // Center the button
    auto button_size = button->desired_size();
    button->set_position(
        (screen.width - button_size.width) * 0.5,
        (screen.height - button_size.height) * 0.5
    );
}
```

---

## Common Patterns

### Pattern 1: Centering an Element

```cpp
// Center widget horizontally
logical_unit center_x = (parent_width - widget_width) * 0.5;
widget->set_x(center_x);

// Center widget vertically
logical_unit center_y = (parent_height - widget_height) * 0.5;
widget->set_y(center_y);

// Or use helper
widget->center_horizontally();
widget->center_vertically();
```

### Pattern 2: Positioning from Edges

```cpp
// Position 5 units from right edge
logical_unit x = parent_width - widget_width - 5_lu;
widget->set_x(x);

// Position 10 units from bottom edge
logical_unit y = parent_height - widget_height - 10_lu;
widget->set_y(y);

// Or use UDim (unified dimensions)
widget->set_width_udim(1.0, -5.0);   // 100% minus 5 units
widget->set_height_udim(1.0, -10.0); // 100% minus 10 units
```

### Pattern 3: Splitting Space

```cpp
// Split horizontally: 30% left, 70% right
left_panel->set_width(parent_width * 0.3);
left_panel->set_position(0_lu, 0_lu);

right_panel->set_width(parent_width * 0.7);
right_panel->set_position(parent_width * 0.3, 0_lu);
```

### Pattern 4: Margins and Padding

```cpp
// Add 10 logical unit margin
logical_rect content_bounds{
    bounds.x + 10_lu,
    bounds.y + 10_lu,
    bounds.width - 20_lu,  // Subtract both sides
    bounds.height - 20_lu
};

// Or use helper
logical_rect content_bounds = bounds.deflated(10_lu);
```

### Pattern 5: Grid Alignment

```cpp
// Snap to grid (e.g., 5 logical unit grid)
logical_unit grid_size = 5_lu;
logical_unit snapped_x = logical_unit(
    std::round(x.to_double() / grid_size.to_double()) * grid_size.to_double()
);
widget->set_x(snapped_x);
```

---

## Best Practices

### DO: Use Logical Units for Everything

```cpp
// Good: Backend-agnostic
widget->set_size(80_lu, 25_lu);
widget->set_position(10_lu, 5_lu);

// Bad: Don't try to use raw pixels/chars
// (This won't compile - logical_unit requires explicit construction)
widget->set_size(80, 25);  // ERROR
```

### DO: Use Semantic Sizing When Possible

```cpp
// Good: Backend-aware semantic sizing
line_edit->set_visible_chars(30);    // 30 characters (adapts to font)
text_view->set_visible_lines(10);    // 10 lines (adapts to line height)
slider->set_track_length(25_lu);     // 25 logical units

// Also good: Explicit logical units when semantic doesn't apply
custom_widget->set_size(50_lu, 20_lu);
```

### DO: Use Percentage Layouts for Flexibility

```cpp
// Good: Adapts to any screen size
left_panel->set_percent_width(30.0);   // 30% of parent
right_panel->set_percent_width(70.0);  // 70% of parent

// Also good: Exact fractions (no precision loss!)
child->set_width(parent_width * (1.0 / 3.0));  // Exactly 1/3
```

### DO: Let the Framework Handle Rounding

```cpp
// Good: Use exact fractions, framework rounds at render
logical_unit x = parent_width * 0.3333;  // Keep precision
widget->set_x(x);

// Bad: Don't round manually
logical_unit x = parent_width * 0.3333;
widget->set_x(logical_unit(std::round(x.to_double())));  // Unnecessary!
```

### DON'T: Mix Logical and Physical Coordinates

```cpp
// Bad: Mixing types
auto& metrics = get_metrics();
int physical_x = 100;  // pixels
logical_unit logical_x = 50_lu;
widget->set_x(physical_x);  // ERROR: doesn't compile

// Good: Convert explicitly if needed
logical_unit logical_x = metrics.from_physical_x(physical_x);
widget->set_x(logical_x);
```

### DON'T: Assume 1:1 Mapping

```cpp
// Bad: Assuming 1 logical unit = 1 pixel
widget->set_size(800_lu, 600_lu);  // Might be huge on terminal!

// Good: Think in abstract units, or query screen size
auto screen = get_screen_size();
widget->set_size(screen.width * 0.8, screen.height * 0.8);
```

---

## Troubleshooting

### Problem: "Widget appears at wrong position"

**Check:** Are you using logical units (`_lu` suffix)?
```cpp
// Wrong
widget->set_position(10, 20);  // Compile error

// Right
widget->set_position(10_lu, 20_lu);  // OK
```

### Problem: "Widget is too large/small"

**Check:** Do you understand the scaling factor?
```cpp
// On terminal: 1 lu = 1 char
widget->set_size(80_lu, 25_lu);  // 80 chars × 25 rows - good!

// On SDL (8 px/lu): 1 lu = 8 pixels
widget->set_size(80_lu, 25_lu);  // 640×200 pixels - might be small!
widget->set_size(100_lu, 75_lu);  // 800×600 pixels - better!
```

**Solution:** Query screen size and use percentages:
```cpp
auto screen = get_screen_size();
widget->set_size(screen.width * 0.8, screen.height * 0.6);
```

### Problem: "Widget is off-screen"

**Check:** Are you checking bounds?
```cpp
auto screen = get_screen_size();
logical_unit x = 150_lu;  // Might be > screen.width!

// Add bounds check
if (x + widget_width > screen.width) {
    x = screen.width - widget_width;  // Clamp to screen
}
```

### Problem: "Layout looks different on different screens"

**This is expected!** Same layout adapts to available space.

```cpp
// Same code:
widget->set_size(50_lu, 20_lu);

// Terminal (80×25):  Widget is 50 chars × 20 rows (fills screen vertically!)
// SDL (200×100 lu):  Widget is 50 lu × 20 lu (small window)

// Solution: Use percentage sizing
widget->set_size(screen.width * 0.6, screen.height * 0.8);
```

### Problem: "Fractional positions cause alignment issues"

**This shouldn't happen** - the rendering system handles rounding automatically.

If you see gaps/overlaps, file a bug report!

### Problem: "How do I get pixel-perfect control?"

**You can access physical coordinates** if needed:
```cpp
// Get backend metrics
auto& metrics = ui_services<Backend>::metrics();

// Convert logical → physical
int physical_x = metrics.snap_to_physical_x(10.5_lu);  // Returns int

// Convert physical → logical
logical_unit logical_x = metrics.from_physical_x(100);  // pixels → lu

// But usually you don't need this! Let the framework handle it.
```

---

## Quick Reference

### Creating Logical Units

```cpp
// Literal suffix (preferred)
logical_unit x = 10_lu;
logical_unit y = 5.5_lu;

// Explicit construction
logical_unit x = logical_unit(10.0);
logical_unit y = logical_unit(5.5);

// From integer (explicit)
logical_unit x = logical_unit(10);
```

### Arithmetic

```cpp
logical_unit a = 10_lu;
logical_unit b = 5_lu;

logical_unit sum = a + b;        // 15 lu
logical_unit diff = a - b;       // 5 lu
logical_unit scaled = a * 2.0;   // 20 lu
logical_unit halved = a / 2.0;   // 5 lu
```

### Comparison

```cpp
logical_unit a = 10_lu;
logical_unit b = 10.0_lu;

bool equal = (a == b);     // true (epsilon comparison)
bool less = (a < b);       // false
bool greater = (a > b);    // false
```

### Geometry Types

```cpp
// Size
logical_size size{80_lu, 25_lu};
logical_unit w = size.width;
logical_unit h = size.height;

// Point
logical_point pos{10_lu, 5_lu};
logical_unit x = pos.x;
logical_unit y = pos.y;

// Rectangle
logical_rect bounds{10_lu, 5_lu, 80_lu, 25_lu};  // x, y, width, height
logical_unit left = bounds.left();
logical_unit right = bounds.right();
logical_unit top = bounds.top();
logical_unit bottom = bounds.bottom();
```

### Screen Queries

```cpp
// Get screen size
auto screen = get_screen_size();  // Returns logical_size

// Get dimensions
logical_unit width = screen.width;
logical_unit height = screen.height;

// Find center
logical_unit center_x = width * 0.5;
logical_unit center_y = height * 0.5;
```

### Backend Metrics

```cpp
// Get metrics
auto& metrics = ui_services<Backend>::metrics();

// Query scaling
double x_scale = metrics.logical_to_physical_x;  // e.g., 8.0 pixels/lu
double y_scale = metrics.logical_to_physical_y;  // e.g., 8.0 pixels/lu
double dpi = metrics.dpi_scale;                  // e.g., 2.0 for Retina

// Convert (usually you don't need this)
double physical_x = metrics.to_physical_x(10_lu);           // 10 lu → px
logical_unit logical_x = metrics.from_physical_x(80.0);     // 80 px → lu
int snapped = metrics.snap_to_physical_x(10.5_lu);          // 10.5 lu → int
```

---

## Summary

**Key Takeaways:**

1. **Logical units are abstract coordinates** that work on any backend
2. **Fractional values are normal** (10.5_lu is perfectly fine)
3. **Rounding happens automatically** at render time (you never round manually)
4. **Screen size is in logical units** (query with `get_screen_size()`)
5. **Use `_lu` suffix** for clarity (`10_lu`, not `logical_unit(10)`)
6. **Think in percentages** for responsive layouts (`screen.width * 0.5`)
7. **Same code works everywhere** - that's the whole point!

**Mental Model:**

- Logical units = "UI units" (abstract)
- Backend metrics = conversion rules (1 lu = ? pixels/chars)
- Rendering = automatic translation (logical → physical)

Write your UI once in logical units, run it anywhere! 🎯

---

## Additional Resources

- **Design Document:** `LOGICAL_UNITS_DESIGN.md` - Architecture details
- **Implementation Plan:** `LOGICAL_UNITS_IMPLEMENTATION_PLAN.md` - Development roadmap
- **Migration Guide:** `LOGICAL_UNITS_MIGRATION.md` - Upgrading from v2.x
- **API Reference:** `include/onyxui/core/types.hh` - Complete API docs

**Questions?** Check the [FAQ in the migration guide](LOGICAL_UNITS_MIGRATION.md#faq) or file an issue on GitHub!
