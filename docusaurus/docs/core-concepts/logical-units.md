---
sidebar_position: 3
---

# Logical Units

OnyxUI uses a **logical units** coordinate system that provides backend-agnostic positioning and sizing. This allows you to write UI code once that works correctly on both terminal (TUI) and graphical (GUI) backends.

## What Are Logical Units?

Logical units are abstract coordinates that remain consistent across different rendering backends. Think of them like:

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

## Basic Usage

### Creating Logical Units

```cpp
using namespace onyxui;

// Using the _lu literal (recommended)
auto x = 10_lu;
auto y = 5.5_lu;

// Explicit construction
auto width = logical_unit(100.0);
auto height = logical_unit(50.0);
```

### Arithmetic Operations

```cpp
auto a = 10_lu;
auto b = 5_lu;

auto sum = a + b;           // 15_lu
auto diff = a - b;          // 5_lu
auto product = a * 2.0;     // 20_lu
auto quotient = a / 2.0;    // 5_lu
```

### Conversion to Physical Coordinates

```cpp
auto logical_pos = 10.5_lu;

// Convert to int (with rounding)
int screen_pos = logical_pos.to_int();  // 11

// Access raw value
double value = logical_pos.value;  // 10.5
```

## Core Types

### logical_size

Represents width and height in logical units:

```cpp
logical_size size{80_lu, 25_lu};

// Access members directly
logical_unit w = size.width;   // 80_lu
logical_unit h = size.height;  // 25_lu

// Convert for rendering
int pixel_width = size.width.to_int();
int pixel_height = size.height.to_int();
```

### logical_rect

Represents position and size in logical units:

```cpp
logical_rect bounds{10_lu, 5_lu, 80_lu, 25_lu};

// Access members directly
logical_unit x = bounds.x;
logical_unit y = bounds.y;
logical_unit w = bounds.width;
logical_unit h = bounds.height;

// Convert for rendering
int screen_x = bounds.x.to_int();
int screen_y = bounds.y.to_int();
```

### logical_thickness

Represents padding/margins:

```cpp
// All sides equal
auto padding = logical_thickness(5_lu);

// Individual sides
auto margin = logical_thickness{
    5_lu,   // left
    3_lu,   // top
    5_lu,   // right
    3_lu    // bottom
};
```

## Widget Layout API

### Measure Phase

Widgets implement `do_measure()` to calculate their desired size:

```cpp
logical_size do_measure(logical_unit available_width,
                        logical_unit available_height) override {
    // Calculate desired size based on content
    auto text_size = measure_text(m_text);

    return logical_size{
        logical_unit(text_size.width),
        logical_unit(text_size.height)
    };
}
```

### Arrange Phase

Widgets implement `do_arrange()` to position children:

```cpp
void do_arrange(const logical_rect& final_bounds) override {
    // Position child within our bounds
    child->arrange(logical_rect{
        2_lu,   // x relative to us
        1_lu,   // y relative to us
        final_bounds.width - 4_lu,
        final_bounds.height - 2_lu
    });
}
```

## Size Constraints

Control how widgets size themselves:

```cpp
size_constraint width_constraint;
width_constraint.policy = size_policy::fixed;
width_constraint.preferred_size = 80_lu;
width_constraint.min_size = 40_lu;
width_constraint.max_size = 120_lu;

widget->set_width_constraint(width_constraint);
```

### Size Policies

- **content** - Size based on content (default)
- **fixed** - Use `preferred_size` exactly
- **expand** - Expand to fill available space
- **weighted** - Proportional sizing (e.g., 2:1 ratio)
- **percentage** - Percentage of parent (e.g., 50%)
- **fill_parent** - Fill entire parent dimension

## Relative Coordinates

OnyxUI uses a **relative coordinate system** where children's positions are relative to their parent's content area:

```cpp
// Parent at screen position (100, 50) with size 200×100
auto parent_bounds = logical_rect{100_lu, 50_lu, 200_lu, 100_lu};

// Child at (10, 5) RELATIVE to parent
// Absolute screen position: (110, 55)
child->arrange(logical_rect{10_lu, 5_lu, 50_lu, 25_lu});
```

### Why Relative Coordinates?

1. **Easier repositioning** - Move parent, all children move automatically
2. **Cleaner architecture** - Children don't need to know screen position
3. **Simpler hit testing** - Convert once at root, then use relative coords
4. **Better encapsulation** - Widgets only know their local coordinate space

## Backend Integration

Each backend defines how logical units map to physical coordinates:

### Terminal Backend (conio)

```cpp
// 1 logical unit = 1 character cell
struct conio_backend {
    static constexpr double logical_units_per_char = 1.0;
};
```

### Graphical Backend (SDL)

```cpp
// 1 logical unit = 8 pixels (configurable)
struct sdl_backend {
    static constexpr double logical_units_per_pixel = 0.125;  // 1/8
};
```

## Best Practices

### DO ✓

```cpp
// Use _lu literals for clarity
widget->set_size(80_lu, 25_lu);

// Use fractional values for precise layout
auto center_x = parent_width / 2.0 - widget_width / 2.0;

// Store logical values in logical types
logical_unit spacing = 5_lu;
```

### DON'T ✗

```cpp
// Don't mix int and logical_unit
widget->set_size(80, 25);  // WRONG - won't compile

// Don't perform integer division on logical_unit
auto half = width.to_int() / 2;  // WRONG - loses precision
auto half = width / 2.0;         // CORRECT

// Don't use backend-specific values
widget->set_size(640, 200);  // WRONG - assumes SDL backend
```

## Common Patterns

### Centering a Widget

```cpp
void arrange_centered(const logical_rect& parent_bounds) {
    auto child_size = child->measure(parent_bounds.width,
                                     parent_bounds.height);

    logical_unit x = (parent_bounds.width - child_size.width) / 2.0;
    logical_unit y = (parent_bounds.height - child_size.height) / 2.0;

    child->arrange(logical_rect{x, y,
                                child_size.width,
                                child_size.height});
}
```

### Responsive Sizing

```cpp
size_constraint width;
width.policy = size_policy::percentage;
width.percentage = 0.8f;  // 80% of parent width
widget->set_width_constraint(width);
```

### Expanding to Fill Space

```cpp
size_constraint height;
height.policy = size_policy::expand;
widget->set_height_constraint(height);
```

## Troubleshooting

### Widgets Appear Too Small/Large

Check your backend's logical units configuration:

```cpp
// Terminal: Should be 1 char = 1 lu
// GUI: Typically 1 lu = 4-16 pixels

// Adjust backend metrics if needed
backend->set_logical_unit_scale(8.0);  // 8 pixels per lu
```

### Layout Not Updating

Invalidate layout when properties change:

```cpp
void set_text(const std::string& text) {
    m_text = text;
    invalidate_measure();  // Size may have changed
}
```

### Precision Loss

Use logical_unit arithmetic, not int:

```cpp
// WRONG - loses fractional part
auto x = widget->bounds().x.to_int() / 2;

// CORRECT - preserves precision
auto x = widget->bounds().x / 2.0;
```

## Migration from Integer Coordinates

If migrating from int-based coordinates:

1. Replace `int` parameters with `logical_unit`
2. Use `_lu` literals: `80` → `80_lu`
3. Convert backend rects: Use `rect_utils` for backend types
4. Update size constraints: Wrap values in `logical_unit()`
5. Fix member access: Backend rects use `rect_utils::get_*()`, logical rects use direct access

## Further Reading

- [Two-Pass Layout](./two-pass-layout.md) - How measure and arrange work together
- [Backend Pattern](./backend-pattern.md) - Creating custom backends
- [Size Constraints](../guides/layout-guide.md) - Advanced sizing strategies
