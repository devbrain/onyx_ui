# Logical Units - Migration Guide

**Version:** 3.0.0
**Date:** 2025-11-26
**Status:** Draft

---

## Table of Contents

1. [Overview](#overview)
2. [Breaking Changes Summary](#breaking-changes-summary)
3. [API Migration](#api-migration)
4. [Common Patterns](#common-patterns)
5. [Widget-Specific Changes](#widget-specific-changes)
6. [Layout Strategy Changes](#layout-strategy-changes)
7. [Rendering Changes](#rendering-changes)
8. [Troubleshooting](#troubleshooting)
9. [FAQ](#faq)

---

## Overview

### What Changed?

OnyxUI 3.0 introduces **logical units** - a floating-point coordinate system that enables true multi-backend support. This replaces the previous integer-based system.

**Key Changes:**
- Coordinates use `logical_unit` (double) instead of `int`
- Geometry types use `logical_rect`, `logical_size`, `logical_point`
- Measure/arrange signatures changed
- Rendering methods accept logical coordinates
- Size constraints use logical units

### Why This Change?

**Problem:** The old system had "concept leakage" - the same integer meant different things on different backends:
- conio: 1 unit = 1 character cell
- SDL: 1 unit = 1 pixel

**Result:** UIs designed for one backend didn't work on another.

**Solution:** Logical units provide backend-agnostic coordinates that scale automatically.

### Migration Effort

**Estimated time:** 2-4 hours per 1000 lines of UI code

**Complexity:**
- Simple widgets: Low (mostly find/replace)
- Complex layouts: Medium (need to understand new constraints)
- Custom widgets: Medium to High (need to update rendering)

---

## Breaking Changes Summary

### Type Changes

| Old Type | New Type | Notes |
|----------|----------|-------|
| `int` (coordinates) | `logical_unit` | Explicit construction |
| `size_type` | `logical_size` | Backend-agnostic |
| `rect_type` | `logical_rect` | Backend-agnostic |
| `point_type` | `logical_point` | Backend-agnostic |

### API Changes

| Old API | New API | Notes |
|---------|---------|-------|
| `measure(int, int)` | `measure(logical_unit, logical_unit)` | Returns `logical_size` |
| `arrange(rect_type)` | `arrange(logical_rect)` | Logical coordinates |
| `set_size(int, int)` | `set_size(logical_unit, logical_unit)` | Use literals `_lu` |
| `set_position(int, int)` | `set_position(logical_unit, logical_unit)` | Use literals `_lu` |
| `bounds()` → `rect_type` | `bounds()` → `logical_rect` | Returns logical |
| `size_constraint::preferred_size` (int) | `size_constraint::preferred_size` (logical_unit) | Floating-point |

### Removed Features

**None.** All features preserved, just type changes.

### New Features

- **UDim** (unified dimensions): Mix relative + absolute sizing
- **Sub-pixel precision**: Smooth animations, exact percentages
- **DPI scaling**: User-configurable UI zoom
- **Aspect ratio compensation**: Handle non-square units

---

## API Migration

### Basic Types

#### Before (v2.x):
```cpp
int x = 10;
int y = 20;
int width = 80;
int height = 25;

// Create rect
rect_type bounds{x, y, width, height};

// Access
int left = bounds.x;
int top = bounds.y;
```

#### After (v3.0):
```cpp
logical_unit x = logical_unit(10.0);  // Explicit construction
logical_unit y = logical_unit(20.0);
logical_unit width = logical_unit(80.0);
logical_unit height = logical_unit(25.0);

// OR: Use literals
logical_unit x = 10_lu;
logical_unit y = 20_lu;
logical_unit width = 80_lu;
logical_unit height = 25_lu;

// Create rect
logical_rect bounds{x, y, width, height};

// Access
logical_unit left = bounds.x;
logical_unit top = bounds.y;
```

**Best Practice:** Use `_lu` literals for clarity:
```cpp
auto bounds = logical_rect(10_lu, 20_lu, 80_lu, 25_lu);
```

### Widget Sizing

#### Before (v2.x):
```cpp
// Set fixed size
widget->set_width_constraint({
    .policy = size_policy::fixed,
    .preferred_size = 80,  // int
    .min_size = 40,
    .max_size = 120
});

// Set position
widget->set_position(10, 5);
widget->set_size(80, 25);
```

#### After (v3.0):
```cpp
// Set fixed size
widget->set_width_constraint({
    .policy = size_policy::fixed,
    .preferred_size = 80_lu,  // logical_unit
    .min_size = 40_lu,
    .max_size = 120_lu
});

// Set position
widget->set_position(10_lu, 5_lu);
widget->set_size(80_lu, 25_lu);

// OR: Use helper methods
widget->set_fixed_width(80_lu);
widget->set_fixed_height(25_lu);
```

### Measure and Arrange

#### Before (v2.x):
```cpp
class my_widget : public widget<Backend> {
    size_type measure(int available_width, int available_height) override {
        // Measure text
        auto text_size = renderer_type::measure_text(m_text, m_font);
        int w = size_utils::get_width(text_size);
        int h = size_utils::get_height(text_size);

        // Add padding
        w += 10;  // 10px padding
        h += 4;   // 4px padding

        return size_type{w, h};
    }

    void arrange(const rect_type& final_bounds) override {
        // Store bounds
        m_bounds = final_bounds;

        // Position child
        rect_type child_bounds{
            final_bounds.x + 5,      // 5px margin
            final_bounds.y + 2,      // 2px margin
            final_bounds.width - 10, // Subtract margins
            final_bounds.height - 4
        };
        m_child->arrange(child_bounds);
    }
};
```

#### After (v3.0):
```cpp
class my_widget : public widget<Backend> {
    logical_size measure(logical_unit available_width,
                        logical_unit available_height) override {
        // Measure text (returns logical_size)
        auto text_size = ctx.measure_text(m_text, m_font);
        logical_unit w = text_size.width;
        logical_unit h = text_size.height;

        // Add padding (logical units)
        w = w + 10_lu;  // 10 logical units padding
        h = h + 4_lu;   // 4 logical units padding

        return logical_size{w, h};
    }

    void arrange(const logical_rect& final_bounds) override {
        // Store bounds
        m_bounds = final_bounds;

        // Position child
        logical_rect child_bounds{
            final_bounds.x + 5_lu,      // 5 lu margin
            final_bounds.y + 2_lu,      // 2 lu margin
            final_bounds.width - 10_lu, // Subtract margins
            final_bounds.height - 4_lu
        };
        m_child->arrange(child_bounds);
    }
};
```

### Rendering

#### Before (v2.x):
```cpp
void do_render(render_context<Backend>& ctx) const override {
    auto bounds = this->bounds();  // rect_type

    // Draw background
    ctx.draw_rect(bounds, m_box_style);

    // Draw text (manual positioning)
    auto text_size = renderer_type::measure_text(m_text, m_font);
    int text_x = bounds.x + (bounds.width - size_utils::get_width(text_size)) / 2;
    int text_y = bounds.y + (bounds.height - size_utils::get_height(text_size)) / 2;

    ctx.draw_text(m_text, {text_x, text_y}, m_font, m_color);
}
```

#### After (v3.0):
```cpp
void do_render(render_context<Backend>& ctx) const override {
    auto bounds = this->bounds();  // logical_rect

    // Draw background
    ctx.draw_rect(bounds, m_box_style);

    // Draw text (fractional positioning!)
    auto text_size = ctx.measure_text(m_text, m_font);  // logical_size
    logical_point text_pos{
        bounds.x + (bounds.width - text_size.width) * 0.5,  // Center X
        bounds.y + (bounds.height - text_size.height) * 0.5 // Center Y
    };

    ctx.draw_text(m_text, text_pos, m_font, m_color);
}
```

**Note:** Fractional positioning (0.5) works! Rendering snaps to physical grid internally.

---

## Common Patterns

### Pattern 1: Fixed Size Widget

#### Before:
```cpp
widget->set_width_constraint({
    .policy = size_policy::fixed,
    .preferred_size = 80,
    .min_size = 80,
    .max_size = 80
});
```

#### After:
```cpp
// Option 1: Verbose
widget->set_width_constraint({
    .policy = size_policy::fixed,
    .preferred_size = 80_lu,
    .min_size = 80_lu,
    .max_size = 80_lu
});

// Option 2: Helper method (preferred)
widget->set_fixed_width(80_lu);
```

### Pattern 2: Percentage Sizing

#### Before:
```cpp
widget->set_width_constraint({
    .policy = size_policy::percentage,
    .percentage = 50.0  // 50% of parent
});
```

#### After:
```cpp
// Same as before (percentage is still double)
widget->set_width_constraint({
    .policy = size_policy::percentage,
    .percentage = 50.0
});

// OR: Use helper
widget->set_percent_width(50.0);
```

**Note:** Percentages unchanged, but now resolve to `logical_unit` internally.

### Pattern 3: Weighted Distribution

#### Before:
```cpp
// Problem: integer division loses precision
child1->set_weight(1.0);
child2->set_weight(2.0);
child3->set_weight(3.0);
// 100 / 6 = 16, 33, 50 (lost 1 unit!)
```

#### After:
```cpp
// No precision loss with floating-point!
child1->set_weight(1.0);
child2->set_weight(2.0);
child3->set_weight(3.0);
// 100 / 6 = 16.666..., 33.333..., 50.0 (exact, rounded only at render)
```

### Pattern 4: Margins and Padding

#### Before:
```cpp
// Add 10px margin
rect_type content_bounds{
    bounds.x + 10,
    bounds.y + 10,
    bounds.width - 20,
    bounds.height - 20
};
```

#### After:
```cpp
// Add 10 logical units margin
logical_rect content_bounds{
    bounds.x + 10_lu,
    bounds.y + 10_lu,
    bounds.width - 20_lu,
    bounds.height - 20_lu
};

// OR: Use helper
logical_rect content_bounds = bounds.deflated(10_lu);
```

### Pattern 5: Centering

#### Before:
```cpp
int center_x = parent_width / 2 - widget_width / 2;
int center_y = parent_height / 2 - widget_height / 2;
widget->set_position(center_x, center_y);
```

#### After:
```cpp
// Fractional arithmetic (exact)!
logical_unit center_x = parent_width * 0.5 - widget_width * 0.5;
logical_unit center_y = parent_height * 0.5 - widget_height * 0.5;
widget->set_position(center_x, center_y);

// OR: Use helper
widget->center_horizontally();
widget->center_vertically();
```

### Pattern 6: Edge Positioning (NEW!)

#### After (v3.0 only):
```cpp
// Position 5 units from right edge
widget->set_width_udim(1.0, -5.0);  // 100% - 5 units

// Position 10 units from bottom edge
widget->set_height_udim(1.0, -10.0);  // 100% - 10 units

// OR: Use factory
widget->set_width_constraint({
    .policy = size_policy::unified,
    .udim = unified_dimension::from_right(5_lu)
});
```

---

## Widget-Specific Changes

### Label

**Before:**
```cpp
auto label = std::make_unique<label<Backend>>("Hello");
label->set_size(100, 20);
```

**After:**
```cpp
auto label = std::make_unique<label<Backend>>("Hello");
label->set_size(100_lu, 20_lu);
```

**No other changes** - label auto-sizes to text by default.

### Button

**Before:**
```cpp
auto button = std::make_unique<button<Backend>>("Click Me");
button->set_fixed_width(80);
button->set_fixed_height(25);
```

**After:**
```cpp
auto button = std::make_unique<button<Backend>>("Click Me");
button->set_fixed_width(80_lu);
button->set_fixed_height(25_lu);
```

### Line Edit

**Before:**
```cpp
auto edit = std::make_unique<line_edit<Backend>>();
edit->set_fixed_width(200);  // 200px/chars wide
```

**After:**
```cpp
auto edit = std::make_unique<line_edit<Backend>>();
edit->set_visible_chars(30);  // 30 characters wide (backend-agnostic!)
```

**Note:** Use semantic sizing for better cross-backend compatibility.

### Text View

**Before:**
```cpp
auto text_view = std::make_unique<text_view<Backend>>();
// Manual size constraint
text_view->set_height_constraint({
    .policy = size_policy::fixed,
    .preferred_size = 10,  // 10 rows?
    .min_size = 10,
    .max_size = 10
});
```

**After:**
```cpp
auto text_view = std::make_unique<text_view<Backend>>();
text_view->set_visible_lines(10);  // 10 lines visible (backend-agnostic!)
```

### Slider

**Before:**
```cpp
auto slider = std::make_unique<slider<Backend>>(slider_orientation::horizontal);
slider->set_fixed_width(100);  // Track length
```

**After:**
```cpp
auto slider = std::make_unique<slider<Backend>>(slider_orientation::horizontal);
slider->set_track_length(25_lu);  // 25 logical units (backend-agnostic!)
```

### Progress Bar

**Before:**
```cpp
auto progress = std::make_unique<progress_bar<Backend>>();
progress->set_fixed_width(200);
```

**After:**
```cpp
auto progress = std::make_unique<progress_bar<Backend>>();
progress->set_bar_width(30_lu);  // 30 logical units (backend-agnostic!)
```

### Window

**Before:**
```cpp
auto window = std::make_unique<window<Backend>>("My Window");
window->set_size(800, 600);
window->set_position(100, 100);
window->show();
```

**After:**
```cpp
auto window = std::make_unique<window<Backend>>("My Window");
window->set_size(80_lu, 25_lu);      // Logical units
window->set_position(10_lu, 5_lu);   // Logical units
window->show();

// OR: Auto-size to content
window->set_content(std::move(my_content));
window->fit_content();  // Auto-sizes based on content!
window->show();
```

---

## Layout Strategy Changes

### VBox / HBox

**Before:**
```cpp
auto vbox = std::make_unique<vbox<Backend>>(5);  // 5px spacing
```

**After:**
```cpp
auto vbox = std::make_unique<vbox<Backend>>(spacing::medium);  // Semantic spacing

// OR: Explicit logical units (if needed)
auto vbox = std::make_unique<vbox<Backend>>(5_lu);  // NOT SUPPORTED - use spacing enum
```

**Note:** Spacing now uses enum (see semantic sizing migration).

### Grid

**Before:**
```cpp
auto grid = std::make_unique<grid<Backend>>(
    3,  // 3 columns
    2,  // 2 rows
    5,  // 5px column spacing
    3   // 3px row spacing
);
```

**After:**
```cpp
auto grid = std::make_unique<grid<Backend>>(
    3,                // 3 columns
    2,                // 2 rows
    spacing::medium,  // Column spacing
    spacing::small    // Row spacing
);
```

### Anchor Panel

**Before:**
```cpp
auto panel = std::make_unique<anchor_panel<Backend>>();
panel->add_child(widget, anchor_point::top_left, 10, 20);  // int offsets
```

**After:**
```cpp
auto panel = std::make_unique<anchor_panel<Backend>>();
panel->add_child(widget, anchor_point::top_left, 10_lu, 20_lu);  // logical_unit offsets
```

### Absolute Panel

**Before:**
```cpp
auto panel = std::make_unique<absolute_panel<Backend>>();
panel->add_child(widget, 100, 50);  // Absolute position (int)
```

**After:**
```cpp
auto panel = std::make_unique<absolute_panel<Backend>>();
panel->add_child(widget, 100_lu, 50_lu);  // Absolute position (logical_unit)
```

---

## Rendering Changes

### Custom Rendering

If you have custom `do_render()` implementations:

**Before:**
```cpp
void do_render(render_context<Backend>& ctx) const override {
    auto bounds = this->bounds();  // rect_type (int x, y, width, height)

    // Draw rect at integer coordinates
    ctx.draw_rect(bounds, box_style);

    // Draw line from (0,0) to (100, 50)
    ctx.draw_line({0, 0}, {100, 50}, color);
}
```

**After:**
```cpp
void do_render(render_context<Backend>& ctx) const override {
    auto bounds = this->bounds();  // logical_rect (double x, y, width, height)

    // Draw rect at logical coordinates (snapped internally)
    ctx.draw_rect(bounds, box_style);

    // Draw line from (0,0) to (100, 50) in logical units
    ctx.draw_line({0_lu, 0_lu}, {100_lu, 50_lu}, color);
}
```

**Key Changes:**
- Accept that coordinates are now `double` (logical units)
- Don't round manually - rendering does it automatically
- Use `_lu` literals for clarity

### Text Measurement

**Before:**
```cpp
auto text_size = renderer_type::measure_text(text, font);  // size_type
int text_width = size_utils::get_width(text_size);
int text_height = size_utils::get_height(text_size);
```

**After:**
```cpp
auto text_size = ctx.measure_text(text, font);  // logical_size
logical_unit text_width = text_size.width;
logical_unit text_height = text_size.height;
```

**Note:** Use `render_context` for measurement (not direct renderer access).

---

## Troubleshooting

### Problem: "Cannot convert 'int' to 'logical_unit'"

**Error:**
```
error: cannot convert 'int' to 'logical_unit' without explicit cast
```

**Cause:** `logical_unit` requires explicit construction to prevent accidental conversions.

**Solution:** Use `_lu` literals or explicit construction:
```cpp
// Bad
widget->set_position(10, 20);  // ERROR

// Good
widget->set_position(10_lu, 20_lu);  // OK

// OR
widget->set_position(logical_unit(10.0), logical_unit(20.0));  // OK
```

### Problem: "Floating-point comparison failed"

**Error:** Tests fail with epsilon comparison errors.

**Cause:** Comparing `logical_unit` values requires epsilon tolerance.

**Solution:** Use `operator==` (has built-in epsilon):
```cpp
// Bad
if (value.to_double() == 10.0)  // May fail due to FP precision

// Good
if (value == logical_unit(10.0))  // OK (epsilon comparison)

// OR: Manual epsilon
if (std::abs(value.to_double() - 10.0) < 1e-9)  // OK
```

### Problem: "Widget sizes don't match pixel-perfect"

**Symptom:** Widgets appear slightly different sizes after migration.

**Cause:** Snapping rounding (floor vs ceil vs nearest).

**Solution:** This is expected and correct! Logical units snap to physical grid at render time.
- Differences should be < 1 pixel/char
- Visual appearance should be nearly identical
- If off by more, check scaling configuration

### Problem: "Performance degradation"

**Symptom:** Layout slower after migration.

**Cause:** Possible inefficient conversion or excessive snapping.

**Solution:**
1. Profile to find hot paths
2. Ensure snapping only at render (not during layout)
3. Avoid converting to/from physical coordinates repeatedly
4. Cache measurements when possible

### Problem: "Cannot use spacing enum with vbox/hbox"

**Error:**
```
error: no matching constructor for vbox<Backend>(int)
```

**Cause:** Spacing enum migration (separate from logical units).

**Solution:** Use spacing enum instead of integers:
```cpp
// Bad
auto vbox = std::make_unique<vbox<Backend>>(5);  // ERROR (post-3.0)

// Good
auto vbox = std::make_unique<vbox<Backend>>(spacing::medium);  // OK
```

### Problem: "How do I get integer coordinates for debugging?"

**Solution:** Use `to_int()`:
```cpp
logical_unit x = 10.5_lu;
int x_debug = x.to_int();  // 11 (rounded)

std::cout << "Position: " << x_debug << std::endl;
```

**Note:** Only for debugging! Don't use in layout calculations.

---

## FAQ

### Q: Can I mix logical units and integers?

**A:** No. Explicit conversion required to prevent bugs.

```cpp
// Bad
logical_unit x = 10;  // ERROR

// Good
logical_unit x = 10_lu;  // OK
logical_unit x = logical_unit(10);  // OK
```

### Q: What about existing integer APIs?

**A:** All removed. Clean break for v3.0 (major version bump).

### Q: How do logical units map to pixels/chars?

**A:** Via `backend_metrics`:
- conio: 1 lu = 1 char cell (default)
- SDL: 1 lu = 8 pixels (configurable)
- DPI scaling applied on top

### Q: Can I change the scaling factor?

**A:** Yes, configure `backend_metrics`:
```cpp
auto& metrics = ui_services<Backend>::backend_metrics();
metrics.set_pixel_scale(10.0);  // 1 lu = 10 pixels (SDL only)
metrics.set_dpi_scale(2.0);     // 2x DPI (Retina)
```

### Q: What is UDim?

**A:** Unified dimension - mix relative + absolute sizing:
```cpp
// "50% minus 5 units"
widget->set_width_udim(0.5, -5.0);

// "100% minus 10 units" (5 units from right edge)
widget->set_width_udim(1.0, -5.0);
```

### Q: Do I need to update all widgets at once?

**A:** Yes. No backward compatibility (breaking change).

### Q: Will my layout look different?

**A:** No. Visual output should be pixel/char-perfect identical.
- Same visual result
- Better precision (no accumulation errors)
- Fractional positioning for smooth animations

### Q: What about performance?

**A:** Target: < 5% overhead
- Floating-point is hardware-accelerated (same speed as int)
- Memory increase: 16 bytes per widget (negligible)
- Rendering overhead: < 1% (snapping is fast)

### Q: How do I migrate custom widgets?

**A:** Follow this checklist:
1. Update measure() signature (logical_unit params, return logical_size)
2. Update arrange() signature (logical_rect param)
3. Update do_render() to use logical coordinates
4. Update member variables (logical_unit, logical_rect)
5. Update size constraints (logical_unit fields)
6. Add unit tests
7. Verify visual output

### Q: What if I want exact pixel control?

**A:** Use physical coordinates via backend_metrics:
```cpp
// Get physical coordinates
auto& metrics = ctx.metrics();
int px = metrics.snap_to_physical_x(10_lu);  // 10 lu → 80 pixels (if scale=8)

// Set physical position (convert back)
logical_unit x = metrics.from_physical_x(100);  // 100 pixels → 12.5 lu
widget->set_position(x, 0_lu);
```

### Q: Can I still use percentage layouts?

**A:** Yes! Unchanged:
```cpp
widget->set_percent_width(50.0);  // 50% of parent
```

Now resolves to exact fractional logical units (no precision loss).

### Q: What about aspect ratio correction?

**A:** Built into backend_metrics:
```cpp
auto& metrics = ui_services<Backend>::backend_metrics();
std::cout << "Aspect ratio: " << metrics.aspect_ratio << std::endl;
// conio: 0.5 (chars are 8×16, i.e., 2:1)
// SDL: 1.0 (square pixels)
```

Use for compensating non-square units if needed.

---

## Migration Checklist

Use this checklist for each widget or component:

- [ ] Update type signatures (`int` → `logical_unit`)
- [ ] Replace integer literals with `_lu` suffix
- [ ] Update `measure()` signature and implementation
- [ ] Update `arrange()` signature and implementation
- [ ] Update `do_render()` to use logical coordinates
- [ ] Update size constraints (logical_unit fields)
- [ ] Update tests (logical_unit assertions)
- [ ] Verify visual output (screenshot comparison)
- [ ] Check performance (benchmark if complex widget)
- [ ] Update documentation/examples

---

## Additional Resources

- **Design Document:** `docs/LOGICAL_UNITS_DESIGN.md` - Architecture details
- **Implementation Plan:** `docs/LOGICAL_UNITS_IMPLEMENTATION_PLAN.md` - Phased rollout
- **User Guide:** `docs/LOGICAL_UNITS_USER_GUIDE.md` - Tutorial and examples
- **API Reference:** `include/onyxui/core/types.hh` - logical_unit documentation

---

## Getting Help

If you encounter issues during migration:

1. **Check this guide** - Most common issues covered
2. **Check examples** - `examples/logical_units_demo.cc`
3. **Check tests** - `unittest/core/test_logical_units.cc`
4. **File an issue** - GitHub issues for bugs
5. **Ask on Discord** - Community support

---

**Happy migrating!** 🚀

The logical unit system provides a solid foundation for multi-backend UI development. While migration requires effort, the benefits (precision, backend-agnosticism, sub-pixel accuracy) are worth it.
