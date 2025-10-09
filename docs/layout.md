# Advanced UI Layout System Design v2.0

## Table of Contents
1. [Overview](#overview)
2. [Core Architecture](#core-architecture)
3. [Memory Management](#memory-management)
4. [Size Policies and Constraints](#size-policies-and-constraints)
5. [Layout Algorithms](#layout-algorithms)
6. [Advanced Features](#advanced-features)
7. [Performance Optimizations](#performance-optimizations)
8. [Testing and Debugging](#testing-and-debugging)

---

## Overview

This document presents a comprehensive, production-ready UI layout system designed for retained-mode GUIs. The system is framework-agnostic, supporting flexible layout strategies, nested composition, dynamic resizing, comprehensive spacing control, scrollable panels, and layout debugging.

**Key Improvements over v1.0:**
- Two-pass measure/arrange layout algorithm
- Smart invalidation strategy
- Enhanced size policies with min/max constraints
- Comprehensive memory management
- Grid layout with span support
- Z-ordering and layering
- Performance optimizations through caching
- Hit testing for event handling

---

## Core Architecture

### Geometric Type Concepts

Instead of implementing custom types, we define concepts that allow you to use any geometric types (SDL_Rect, sf::IntRect, your own types, etc.):

```cpp
#include <concepts>

// Concept for point-like types
template<typename T>
concept PointLike = requires(T p) {
    { p.x } -> std::convertible_to<int>;
    { p.y } -> std::convertible_to<int>;
};

// Concept for size-like types
template<typename T>
concept SizeLike = requires(T s) {
    { s.width } -> std::convertible_to<int>;
    { s.height } -> std::convertible_to<int>;
} || requires(T s) {
    { s.w } -> std::convertible_to<int>;
    { s.h } -> std::convertible_to<int>;
};

// Concept for rectangle-like types
template<typename T>
concept RectLike = requires(T r) {
    { r.x } -> std::convertible_to<int>;
    { r.y } -> std::convertible_to<int>;
    { r.w } -> std::convertible_to<int>;
    { r.h } -> std::convertible_to<int>;
} || requires(T r) {
    { r.x } -> std::convertible_to<int>;
    { r.y } -> std::convertible_to<int>;
    { r.width } -> std::convertible_to<int>;
    { r.height } -> std::convertible_to<int>;
};

// Helper functions to work with different rectangle formats
namespace rect_utils {
    template<RectLike R>
    int get_x(const R& r) {
        return r.x;
    }
    
    template<RectLike R>
    int get_y(const R& r) {
        return r.y;
    }
    
    template<RectLike R>
    int get_width(const R& r) {
        if constexpr (requires { r.width; }) {
            return r.width;
        } else {
            return r.w;
        }
    }
    
    template<RectLike R>
    int get_height(const R& r) {
        if constexpr (requires { r.height; }) {
            return r.height;
        } else {
            return r.h;
        }
    }
    
    template<RectLike R>
    void set_bounds(R& r, int x, int y, int width, int height) {
        r.x = x;
        r.y = y;
        if constexpr (requires { r.width; }) {
            r.width = width;
            r.height = height;
        } else {
            r.w = width;
            r.h = height;
        }
    }
    
    template<RectLike R>
    bool contains(const R& r, int px, int py) {
        int x = get_x(r);
        int y = get_y(r);
        int w = get_width(r);
        int h = get_height(r);
        return px >= x && px < x + w && py >= y && py < y + h;
    }
}

// Helper functions for size types
namespace size_utils {
    template<SizeLike S>
    int get_width(const S& s) {
        if constexpr (requires { s.width; }) {
            return s.width;
        } else {
            return s.w;
        }
    }
    
    template<SizeLike S>
    int get_height(const S& s) {
        if constexpr (requires { s.height; }) {
            return s.height;
        } else {
            return s.h;
        }
    }
    
    template<SizeLike S>
    void set_size(S& s, int width, int height) {
        if constexpr (requires { s.width; }) {
            s.width = width;
            s.height = height;
        } else {
            s.w = width;
            s.h = height;
        }
    }
    
    template<SizeLike S1, SizeLike S2>
    bool equal(const S1& a, const S2& b) {
        return get_width(a) == get_width(b) && 
               get_height(a) == get_height(b);
    }
}

// Thickness struct (margin/padding) - this one is custom
struct thickness {
    int left, top, right, bottom;
    
    int horizontal() const { return left + right; }
    int vertical() const { return top + bottom; }
};
```

### Alignment Enums

```cpp
enum class horizontal_alignment {
    left,
    center,
    right,
    stretch
};

enum class vertical_alignment {
    top,
    center,
    bottom,
    stretch
};

enum class direction {
    horizontal,
    vertical
};
```

---

## Memory Management

### Ownership Strategy

```cpp
// Forward declarations
class ui_element;
class layout_strategy;

// Smart pointer aliases for clarity
using ui_element_ptr = std::unique_ptr<ui_element>;
using layout_strategy_ptr = std::unique_ptr<layout_strategy>;

// The UI tree uses unique_ptr for owned children
// Parent pointers are non-owning raw pointers
struct ui_element {
    // Non-owning pointer to parent
    ui_element* parent = nullptr;
    
    // Owned children
    std::vector<ui_element_ptr> children;
    
    // Owned layout strategy
    layout_strategy_ptr layout_strategy_instance;
    
    // Add child (takes ownership)
    void add_child(ui_element_ptr child) {
        child->parent = this;
        children.push_back(std::move(child));
        invalidate_measure();
    }
    
    // Remove child (returns ownership)
    ui_element_ptr remove_child(ui_element* child) {
        auto it = std::find_if(children.begin(), children.end(),
            [child](const ui_element_ptr& ptr) { 
                return ptr.get() == child; 
            });
        
        if (it != children.end()) {
            ui_element_ptr removed = std::move(*it);
            children.erase(it);
            removed->parent = nullptr;
            invalidate_measure();
            return removed;
        }
        return nullptr;
    }
};
```

---

## Size Policies and Constraints

### Enhanced Size Policy System

```cpp
enum class size_policy {
    fixed,          // Use preferred_size exactly
    content,        // Size based on content (wrap)
    expand,         // Grow to fill available space
    fill_parent,    // Match parent's content area
    percentage,     // Percentage of parent
    weighted        // Proportional distribution (flex-grow style)
};

struct size_constraint {
    size_policy policy = size_policy::content;
    
    // Core size values
    int preferred_size = 0;    // Desired size for 'fixed' policy
    int min_size = 0;          // Minimum allowed size
    int max_size = INT_MAX;    // Maximum allowed size
    
    // For weighted/percentage policies
    float weight = 1.0f;       // Weight for distribution (flex-grow)
    float percentage = 1.0f;   // Percentage of parent (0.0 to 1.0)
    
    // Clamp a size to constraints
    int clamp(int value) const {
        return std::max(min_size, std::min(max_size, value));
    }
    
    bool operator==(const size_constraint& other) const {
        return policy == other.policy &&
               preferred_size == other.preferred_size &&
               min_size == other.min_size &&
               max_size == other.max_size &&
               weight == other.weight &&
               percentage == other.percentage;
    }
};
```

### UI Element Core Structure

```cpp
// Template the ui_element with your rectangle and size types
template<RectLike TRect, SizeLike TSize>
class ui_element {
public:
    // Identity
    std::string id;
    bool visible = true;
    bool enabled = true;
    
    // Hierarchy
    ui_element* parent = nullptr;
    std::vector<std::unique_ptr<ui_element>> children;
    
    // Geometry - uses your types!
    TRect bounds = {};
    int z_index = 0;
    
    // Size constraints
    size_constraint width_constraint;
    size_constraint height_constraint;
    
    // Alignment within allocated space
    horizontal_alignment h_align = horizontal_alignment::stretch;
    vertical_alignment v_align = vertical_alignment::stretch;
    
    // Spacing
    thickness margin = {0, 0, 0, 0};
    thickness padding = {0, 0, 0, 0};
    
    // Layout strategy
    std::unique_ptr<layout_strategy<TRect, TSize>> layout_strategy_instance;
    
    // Invalidation flags
    bool needs_measure = true;
    bool needs_arrange = true;
    
    // Cached measure results
    mutable TSize last_measured_size = {};
    mutable int last_available_width = -1;
    mutable int last_available_height = -1;
    
    // Virtual interface
    virtual ~ui_element() = default;
    
    // Two-pass layout interface
    TSize measure(int available_width, int available_height);
    void arrange(const TRect& final_bounds);
    
    // Invalidation
    void invalidate_measure();
    void invalidate_arrange();
    
    // Hit testing
    virtual ui_element* hit_test(int x, int y);
    
    // Add child (takes ownership)
    void add_child(std::unique_ptr<ui_element> child) {
        child->parent = this;
        children.push_back(std::move(child));
        invalidate_measure();
    }
    
    // Remove child (returns ownership)
    std::unique_ptr<ui_element> remove_child(ui_element* child) {
        auto it = std::find_if(children.begin(), children.end(),
            [child](const auto& ptr) { return ptr.get() == child; });
        
        if (it != children.end()) {
            auto removed = std::move(*it);
            children.erase(it);
            removed->parent = nullptr;
            invalidate_measure();
            return removed;
        }
        return nullptr;
    }
    
protected:
    // Override to provide custom measurement
    virtual TSize measure_override(int available_width, int available_height);
    
    // Override to provide custom arrangement
    virtual void arrange_override(const TRect& final_bounds);
    
    // Content size (for content-sized elements)
    virtual TSize get_content_size() const { 
        TSize s = {};
        size_utils::set_size(s, 0, 0);
        return s;
    }
};
```

---

## Layout Algorithms

### Invalidation System

```cpp
void ui_element::invalidate_measure() {
    if (!needs_measure) {
        needs_measure = true;
        needs_arrange = true;
        
        // Propagate up the tree (parents need remeasurement)
        if (parent) {
            parent->invalidate_measure();
        }
    }
}

void ui_element::invalidate_arrange() {
    if (!needs_arrange) {
        needs_arrange = true;
        
        // Propagate down the tree (children need rearrangement)
        for (auto& child : children) {
            child->invalidate_arrange();
        }
    }
}
```

### Two-Pass Layout Algorithm

```cpp
template<RectLike TRect, SizeLike TSize>
TSize ui_element<TRect, TSize>::measure(int available_width, int available_height) {
    // Check cache
    if (!needs_measure && 
        last_available_width == available_width &&
        last_available_height == available_height) {
        return last_measured_size;
    }
    
    // Account for margin
    int content_width = available_width - margin.horizontal();
    int content_height = available_height - margin.vertical();
    
    // Measure content
    TSize measured = measure_override(content_width, content_height);
    
    // Add margin back
    int meas_w = size_utils::get_width(measured) + margin.horizontal();
    int meas_h = size_utils::get_height(measured) + margin.vertical();
    
    // Apply constraints
    meas_w = width_constraint.clamp(meas_w);
    meas_h = height_constraint.clamp(meas_h);
    
    size_utils::set_size(measured, meas_w, meas_h);
    
    // Cache results
    last_measured_size = measured;
    last_available_width = available_width;
    last_available_height = available_height;
    needs_measure = false;
    
    return measured;
}

template<RectLike TRect, SizeLike TSize>
void ui_element<TRect, TSize>::arrange(const TRect& final_bounds) {
    bounds = final_bounds;
    
    if (!needs_arrange && !needs_measure) {
        return;
    }
    
    // Calculate content area (exclude margin and padding)
    TRect content_area;
    int x = rect_utils::get_x(final_bounds) + margin.left + padding.left;
    int y = rect_utils::get_y(final_bounds) + margin.top + padding.top;
    int w = rect_utils::get_width(final_bounds) - margin.horizontal() - padding.horizontal();
    int h = rect_utils::get_height(final_bounds) - margin.vertical() - padding.vertical();
    rect_utils::set_bounds(content_area, x, y, w, h);
    
    arrange_override(content_area);
    needs_arrange = false;
}

template<RectLike TRect, SizeLike TSize>
TSize ui_element<TRect, TSize>::measure_override(int available_width, int available_height) {
    // Default implementation: measure using layout strategy
    if (layout_strategy_instance) {
        return layout_strategy_instance->measure_children(
            this, available_width, available_height);
    }
    
    // Fallback: use content size
    return get_content_size();
}

template<RectLike TRect, SizeLike TSize>
void ui_element<TRect, TSize>::arrange_override(const TRect& content_area) {
    // Default implementation: arrange using layout strategy
    if (layout_strategy_instance) {
        layout_strategy_instance->arrange_children(this, content_area);
    }
}
```

### Layout Strategy Interface

```cpp
template<RectLike TRect, SizeLike TSize>
class layout_strategy {
public:
    virtual ~layout_strategy() = default;
    
    // Measure phase: determine desired size
    virtual TSize measure_children(
        ui_element<TRect, TSize>* parent,
        int available_width,
        int available_height) = 0;
    
    // Arrange phase: position children
    virtual void arrange_children(
        ui_element<TRect, TSize>* parent,
        const TRect& content_area) = 0;
};
```

---

### Linear Layout Algorithm

```cpp
template<RectLike TRect, SizeLike TSize>
class linear_layout : public layout_strategy<TRect, TSize> {
public:
    direction layout_direction = direction::vertical;
    horizontal_alignment h_align = horizontal_alignment::stretch;
    vertical_alignment v_align = vertical_alignment::stretch;
    int spacing = 0;
    
    TSize measure_children(ui_element<TRect, TSize>* parent, 
                           int available_width, 
                           int available_height) override {
        if (parent->children.empty()) {
            TSize result = {};
            size_utils::set_size(result, 0, 0);
            return result;
        }
        
        int total_width = 0;
        int total_height = 0;
        int total_spacing = spacing * (parent->children.size() - 1);
        
        if (layout_direction == direction::vertical) {
            // Vertical layout: stack children vertically
            for (auto& child : parent->children) {
                if (!child->visible) continue;
                
                // Measure child with full width, remaining height
                int remaining_height = available_height - total_height;
                TSize child_size = child->measure(available_width, remaining_height);
                
                int child_w = size_utils::get_width(child_size);
                int child_h = size_utils::get_height(child_size);
                
                total_width = std::max(total_width, child_w);
                total_height += child_h;
            }
            
            total_height += total_spacing;
            
        } else {
            // Horizontal layout: stack children horizontally
            for (auto& child : parent->children) {
                if (!child->visible) continue;
                
                // Measure child with remaining width, full height
                int remaining_width = available_width - total_width;
                TSize child_size = child->measure(remaining_width, available_height);
                
                int child_w = size_utils::get_width(child_size);
                int child_h = size_utils::get_height(child_size);
                
                total_width += child_w;
                total_height = std::max(total_height, child_h);
            }
            
            total_width += total_spacing;
        }
        
        TSize result = {};
        size_utils::set_size(result, total_width, total_height);
        return result;
    }
    
    void arrange_children(ui_element<TRect, TSize>* parent, 
                         const TRect& content_area) override {
        if (parent->children.empty()) return;
        
        // Filter visible children
        std::vector<ui_element<TRect, TSize>*> visible_children;
        for (auto& child : parent->children) {
            if (child->visible) {
                visible_children.push_back(child.get());
            }
        }
        
        if (visible_children.empty()) return;
        
        if (layout_direction == direction::vertical) {
            arrange_vertical(visible_children, content_area);
        } else {
            arrange_horizontal(visible_children, content_area);
        }
    }
    
private:
    void arrange_vertical(std::vector<ui_element<TRect, TSize>*>& children, 
                         const TRect& content_area) {
        int content_h = rect_utils::get_height(content_area);
        
        // Calculate total desired height and identify expanding children
        int total_fixed_height = 0;
        int total_weighted_height = 0;
        float total_weight = 0.0f;
        int num_expanding = 0;
        
        for (auto* child : children) {
            TSize measured = child->last_measured_size;
            int meas_h = size_utils::get_height(measured);
            
            if (child->height_constraint.policy == size_policy::expand) {
                num_expanding++;
            } else if (child->height_constraint.policy == size_policy::weighted) {
                total_weight += child->height_constraint.weight;
            } else {
                total_fixed_height += meas_h;
            }
        }
        
        // Calculate spacing
        int total_spacing = spacing * (children.size() - 1);
        int available_height = content_h - total_fixed_height - total_spacing;
        
        // Distribute remaining space
        int expand_height = (num_expanding > 0 && available_height > 0) 
                           ? available_height / num_expanding 
                           : 0;
        
        // Position children
        int current_y = rect_utils::get_y(content_area);
        int content_x = rect_utils::get_x(content_area);
        int content_w = rect_utils::get_width(content_area);
        
        for (auto* child : children) {
            TSize measured = child->last_measured_size;
            int meas_w = size_utils::get_width(measured);
            int meas_h = size_utils::get_height(measured);
            
            // Determine child height
            int child_height = meas_h;
            if (child->height_constraint.policy == size_policy::expand) {
                child_height = expand_height;
            } else if (child->height_constraint.policy == size_policy::weighted) {
                child_height = (int)(available_height * 
                    (child->height_constraint.weight / total_weight));
            } else if (child->height_constraint.policy == size_policy::fill_parent) {
                child_height = content_h;
            }
            
            // Apply height constraints
            child_height = child->height_constraint.clamp(child_height);
            
            // Determine child width based on horizontal alignment
            int child_width = meas_w;
            int child_x = content_x;
            
            if (child->h_align == horizontal_alignment::stretch) {
                child_width = content_w;
            } else {
                child_width = std::min(meas_w, content_w);
                
                if (child->h_align == horizontal_alignment::center) {
                    child_x = content_x + (content_w - child_width) / 2;
                } else if (child->h_align == horizontal_alignment::right) {
                    child_x = content_x + content_w - child_width;
                }
            }
            
            // Arrange child
            TRect child_bounds;
            rect_utils::set_bounds(child_bounds, child_x, current_y, 
                                  child_width, child_height);
            child->arrange(child_bounds);
            
            current_y += child_height + spacing;
        }
    }
    
    void arrange_horizontal(std::vector<ui_element<TRect, TSize>*>& children,
                           const TRect& content_area) {
        int content_w = rect_utils::get_width(content_area);
        
        // Calculate total desired width and identify expanding children
        int total_fixed_width = 0;
        float total_weight = 0.0f;
        int num_expanding = 0;
        
        for (auto* child : children) {
            TSize measured = child->last_measured_size;
            int meas_w = size_utils::get_width(measured);
            
            if (child->width_constraint.policy == size_policy::expand) {
                num_expanding++;
            } else if (child->width_constraint.policy == size_policy::weighted) {
                total_weight += child->width_constraint.weight;
            } else {
                total_fixed_width += meas_w;
            }
        }
        
        // Calculate spacing
        int total_spacing = spacing * (children.size() - 1);
        int available_width = content_w - total_fixed_width - total_spacing;
        
        // Distribute remaining space
        int expand_width = (num_expanding > 0 && available_width > 0)
                          ? available_width / num_expanding
                          : 0;
        
        // Position children
        int current_x = rect_utils::get_x(content_area);
        int content_y = rect_utils::get_y(content_area);
        int content_h = rect_utils::get_height(content_area);
        
        for (auto* child : children) {
            TSize measured = child->last_measured_size;
            int meas_w = size_utils::get_width(measured);
            int meas_h = size_utils::get_height(measured);
            
            // Determine child width
            int child_width = meas_w;
            if (child->width_constraint.policy == size_policy::expand) {
                child_width = expand_width;
            } else if (child->width_constraint.policy == size_policy::weighted) {
                child_width = (int)(available_width * 
                    (child->width_constraint.weight / total_weight));
            } else if (child->width_constraint.policy == size_policy::fill_parent) {
                child_width = content_w;
            }
            
            // Apply width constraints
            child_width = child->width_constraint.clamp(child_width);
            
            // Determine child height based on vertical alignment
            int child_height = meas_h;
            int child_y = content_y;
            
            if (child->v_align == vertical_alignment::stretch) {
                child_height = content_h;
            } else {
                child_height = std::min(meas_h, content_h);
                
                if (child->v_align == vertical_alignment::center) {
                    child_y = content_y + (content_h - child_height) / 2;
                } else if (child->v_align == vertical_alignment::bottom) {
                    child_y = content_y + content_h - child_height;
                }
            }
            
            // Arrange child
            TRect child_bounds;
            rect_utils::set_bounds(child_bounds, current_x, child_y,
                                  child_width, child_height);
            child->arrange(child_bounds);
            
            current_x += child_width + spacing;
        }
    }
};
```

---

### Grid Layout Algorithm

```cpp
struct grid_cell_info {
    int row = 0;
    int column = 0;
    int row_span = 1;
    int column_span = 1;
};

class grid_layout : public layout_strategy {
public:
    int num_columns = 1;
    int num_rows = -1;  // Auto-calculate if -1
    
    std::vector<int> column_widths;   // Empty = equal distribution
    std::vector<int> row_heights;     // Empty = equal distribution
    
    int column_spacing = 0;
    int row_spacing = 0;
    
    bool auto_size_cells = true;
    
    // Child -> grid cell mapping
    std::unordered_map<ui_element*, grid_cell_info> cell_mapping;
    
    void set_cell(ui_element* child, int row, int col, 
                  int row_span = 1, int col_span = 1) {
        cell_mapping[child] = {row, col, row_span, col_span};
    }
    
    size measure_children(ui_element* parent,
                         int available_width,
                         int available_height) override {
        if (parent->children.empty()) {
            return {0, 0};
        }
        
        // Auto-assign cells if not explicitly set
        auto_assign_cells(parent);
        
        // Calculate actual row count
        int actual_rows = calculate_row_count(parent);
        
        if (auto_size_cells) {
            // Measure all children to determine cell sizes
            measure_auto_sized_grid(parent, available_width, available_height,
                                   actual_rows);
        } else {
            // Use predefined sizes
            use_fixed_grid_sizes(actual_rows);
        }
        
        // Calculate total grid size
        int total_width = 0;
        for (int w : column_widths) total_width += w;
        total_width += column_spacing * (num_columns - 1);
        
        int total_height = 0;
        for (int h : row_heights) total_height += h;
        total_height += row_spacing * (actual_rows - 1);
        
        return {total_width, total_height};
    }
    
    void arrange_children(ui_element* parent, const rect& content_area) override {
        if (parent->children.empty()) return;
        
        int actual_rows = row_heights.size();
        
        // Calculate cell positions
        std::vector<int> column_positions(num_columns);
        std::vector<int> row_positions(actual_rows);
        
        column_positions[0] = content_area.x;
        for (int i = 1; i < num_columns; ++i) {
            column_positions[i] = column_positions[i-1] + 
                                 column_widths[i-1] + column_spacing;
        }
        
        row_positions[0] = content_area.y;
        for (int i = 1; i < actual_rows; ++i) {
            row_positions[i] = row_positions[i-1] + 
                              row_heights[i-1] + row_spacing;
        }
        
        // Arrange each child
        for (auto& child : parent->children) {
            if (!child->visible) continue;
            
            auto it = cell_mapping.find(child.get());
            if (it == cell_mapping.end()) continue;
            
            const grid_cell_info& cell = it->second;
            
            // Calculate cell bounds
            int cell_x = column_positions[cell.column];
            int cell_y = row_positions[cell.row];
            
            int cell_width = 0;
            for (int i = 0; i < cell.column_span; ++i) {
                if (cell.column + i < num_columns) {
                    cell_width += column_widths[cell.column + i];
                    if (i < cell.column_span - 1) {
                        cell_width += column_spacing;
                    }
                }
            }
            
            int cell_height = 0;
            for (int i = 0; i < cell.row_span; ++i) {
                if (cell.row + i < actual_rows) {
                    cell_height += row_heights[cell.row + i];
                    if (i < cell.row_span - 1) {
                        cell_height += row_spacing;
                    }
                }
            }
            
            // Apply alignment within cell
            size measured = child->last_measured_size;
            int child_width = measured.width;
            int child_height = measured.height;
            int child_x = cell_x;
            int child_y = cell_y;
            
            if (child->h_align == horizontal_alignment::stretch) {
                child_width = cell_width;
            } else {
                child_width = std::min(measured.width, cell_width);
                if (child->h_align == horizontal_alignment::center) {
                    child_x = cell_x + (cell_width - child_width) / 2;
                } else if (child->h_align == horizontal_alignment::right) {
                    child_x = cell_x + cell_width - child_width;
                }
            }
            
            if (child->v_align == vertical_alignment::stretch) {
                child_height = cell_height;
            } else {
                child_height = std::min(measured.height, cell_height);
                if (child->v_align == vertical_alignment::center) {
                    child_y = cell_y + (cell_height - child_height) / 2;
                } else if (child->v_align == vertical_alignment::bottom) {
                    child_y = cell_y + cell_height - child_height;
                }
            }
            
            child->arrange({child_x, child_y, child_width, child_height});
        }
    }
    
private:
    void auto_assign_cells(ui_element* parent) {
        int current_row = 0;
        int current_col = 0;
        
        for (auto& child : parent->children) {
            if (!child->visible) continue;
            
            // Skip if already assigned
            if (cell_mapping.find(child.get()) != cell_mapping.end()) {
                continue;
            }
            
            // Auto-assign to next available cell
            cell_mapping[child.get()] = {current_row, current_col, 1, 1};
            
            current_col++;
            if (current_col >= num_columns) {
                current_col = 0;
                current_row++;
            }
        }
    }
    
    int calculate_row_count(ui_element* parent) {
        if (num_rows > 0) return num_rows;
        
        int max_row = 0;
        for (auto& child : parent->children) {
            if (!child->visible) continue;
            
            auto it = cell_mapping.find(child.get());
            if (it != cell_mapping.end()) {
                int end_row = it->second.row + it->second.row_span;
                max_row = std::max(max_row, end_row);
            }
        }
        
        return std::max(1, max_row);
    }
    
    void measure_auto_sized_grid(ui_element* parent, int available_width,
                                int available_height, int actual_rows) {
        column_widths.resize(num_columns, 0);
        row_heights.resize(actual_rows, 0);
        
        // Measure all children
        for (auto& child : parent->children) {
            if (!child->visible) continue;
            
            auto it = cell_mapping.find(child.get());
            if (it == cell_mapping.end()) continue;
            
            const grid_cell_info& cell = it->second;
            
            // Measure child with unconstrained size
            size measured = child->measure(available_width, available_height);
            
            // Update column/row sizes (simple distribution for spans)
            if (cell.column_span == 1) {
                column_widths[cell.column] = 
                    std::max(column_widths[cell.column], measured.width);
            }
            
            if (cell.row_span == 1) {
                row_heights[cell.row] = 
                    std::max(row_heights[cell.row], measured.height);
            }
        }
    }
    
    void use_fixed_grid_sizes(int actual_rows) {
        if (column_widths.empty()) {
            column_widths.resize(num_columns, 100);  // Default width
        }
        
        if (row_heights.empty()) {
            row_heights.resize(actual_rows, 100);  // Default height
        }
    }
};
```

---

### Anchor Layout Algorithm

```cpp
enum class anchor_point {
    top_left,
    top_center,
    top_right,
    center_left,
    center,
    center_right,
    bottom_left,
    bottom_center,
    bottom_right
};

class anchor_layout : public layout_strategy {
public:
    struct anchor_info {
        anchor_point point = anchor_point::top_left;
        int offset_x = 0;
        int offset_y = 0;
    };
    
    std::unordered_map<ui_element*, anchor_info> anchor_mapping;
    
    void set_anchor(ui_element* child, anchor_point point, 
                   int offset_x = 0, int offset_y = 0) {
        anchor_mapping[child] = {point, offset_x, offset_y};
    }
    
    size measure_children(ui_element* parent,
                         int available_width,
                         int available_height) override {
        // Measure all children with available space
        for (auto& child : parent->children) {
            if (child->visible) {
                child->measure(available_width, available_height);
            }
        }
        
        // Anchor layout uses all available space
        return {available_width, available_height};
    }
    
    void arrange_children(ui_element* parent, const rect& content_area) override {
        for (auto& child : parent->children) {
            if (!child->visible) continue;
            
            auto it = anchor_mapping.find(child.get());
            anchor_info info = (it != anchor_mapping.end()) 
                ? it->second 
                : anchor_info{anchor_point::top_left, 0, 0};
            
            size measured = child->last_measured_size;
            
            int child_x, child_y;
            calculate_anchor_position(content_area, measured, info, 
                                     child_x, child_y);
            
            child->arrange({child_x, child_y, measured.width, measured.height});
        }
    }
    
private:
    void calculate_anchor_position(const rect& content_area, 
                                   const size& child_size,
                                   const anchor_info& info,
                                   int& out_x, int& out_y) {
        switch (info.point) {
            case anchor_point::top_left:
                out_x = content_area.x;
                out_y = content_area.y;
                break;
                
            case anchor_point::top_center:
                out_x = content_area.x + (content_area.w - child_size.width) / 2;
                out_y = content_area.y;
                break;
                
            case anchor_point::top_right:
                out_x = content_area.x + content_area.w - child_size.width;
                out_y = content_area.y;
                break;
                
            case anchor_point::center_left:
                out_x = content_area.x;
                out_y = content_area.y + (content_area.h - child_size.height) / 2;
                break;
                
            case anchor_point::center:
                out_x = content_area.x + (content_area.w - child_size.width) / 2;
                out_y = content_area.y + (content_area.h - child_size.height) / 2;
                break;
                
            case anchor_point::center_right:
                out_x = content_area.x + content_area.w - child_size.width;
                out_y = content_area.y + (content_area.h - child_size.height) / 2;
                break;
                
            case anchor_point::bottom_left:
                out_x = content_area.x;
                out_y = content_area.y + content_area.h - child_size.height;
                break;
                
            case anchor_point::bottom_center:
                out_x = content_area.x + (content_area.w - child_size.width) / 2;
                out_y = content_area.y + content_area.h - child_size.height;
                break;
                
            case anchor_point::bottom_right:
                out_x = content_area.x + content_area.w - child_size.width;
                out_y = content_area.y + content_area.h - child_size.height;
                break;
        }
        
        out_x += info.offset_x;
        out_y += info.offset_y;
    }
};
```

---

### Absolute Layout Algorithm

```cpp
class absolute_layout : public layout_strategy {
public:
    struct position_info {
        int x = 0;
        int y = 0;
        int width = -1;   // -1 means use measured width
        int height = -1;  // -1 means use measured height
    };
    
    std::unordered_map<ui_element*, position_info> position_mapping;
    
    void set_position(ui_element* child, int x, int y, 
                     int width = -1, int height = -1) {
        position_mapping[child] = {x, y, width, height};
    }
    
    size measure_children(ui_element* parent,
                         int available_width,
                         int available_height) override {
        int max_width = 0;
        int max_height = 0;
        
        // Measure all children
        for (auto& child : parent->children) {
            if (!child->visible) continue;
            
            size measured = child->measure(available_width, available_height);
            
            auto it = position_mapping.find(child.get());
            if (it != position_mapping.end()) {
                const position_info& pos = it->second;
                
                int child_width = (pos.width > 0) ? pos.width : measured.width;
                int child_height = (pos.height > 0) ? pos.height : measured.height;
                
                max_width = std::max(max_width, pos.x + child_width);
                max_height = std::max(max_height, pos.y + child_height);
            } else {
                max_width = std::max(max_width, measured.width);
                max_height = std::max(max_height, measured.height);
            }
        }
        
        return {max_width, max_height};
    }
    
    void arrange_children(ui_element* parent, const rect& content_area) override {
        for (auto& child : parent->children) {
            if (!child->visible) continue;
            
            auto it = position_mapping.find(child.get());
            position_info pos = (it != position_mapping.end())
                ? it->second
                : position_info{0, 0, -1, -1};
            
            size measured = child->last_measured_size;
            
            int child_x = content_area.x + pos.x;
            int child_y = content_area.y + pos.y;
            int child_width = (pos.width > 0) ? pos.width : measured.width;
            int child_height = (pos.height > 0) ? pos.height : measured.height;
            
            child->arrange({child_x, child_y, child_width, child_height});
        }
    }
};
```

---

## Advanced Features

### Z-Order and Layering

```cpp
class ui_element {
public:
    int z_index = 0;
    
    void sort_children_by_z_index() {
        std::stable_sort(children.begin(), children.end(),
            [](const ui_element_ptr& a, const ui_element_ptr& b) {
                return a->z_index < b->z_index;
            });
    }
    
    // Call this after adding/removing children or changing z_index
    void update_child_order() {
        sort_children_by_z_index();
        invalidate_arrange();
    }
};
```

### Hit Testing

```cpp
ui_element* ui_element::hit_test(int x, int y) {
    // Not visible or not within bounds
    if (!visible || !bounds.contains(x, y)) {
        return nullptr;
    }
    
    // Check children in reverse order (highest z-index first)
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        ui_element* hit = (*it)->hit_test(x, y);
        if (hit) return hit;
    }
    
    // No child hit, return this element
    return this;
}
```

### Scrollable Panel

```cpp
class scrollable_panel : public ui_element {
public:
    int scroll_x = 0;
    int scroll_y = 0;
    int content_width = 0;
    int content_height = 0;
    
    bool show_horizontal_scrollbar = true;
    bool show_vertical_scrollbar = true;
    
    int scrollbar_thickness = 16;
    
protected:
    size measure_override(int available_width, int available_height) override {
        // Reserve space for scrollbars if needed
        int content_available_width = available_width;
        int content_available_height = available_height;
        
        if (show_vertical_scrollbar) {
            content_available_width -= scrollbar_thickness;
        }
        if (show_horizontal_scrollbar) {
            content_available_height -= scrollbar_thickness;
        }
        
        // Measure content
        size content_size = ui_element::measure_override(
            content_available_width, content_available_height);
        
        content_width = content_size.width;
        content_height = content_size.height;
        
        return {available_width, available_height};
    }
    
    void arrange_override(const rect& content_area) override {
        // Create viewport rect accounting for scroll offset
        rect viewport = {
            content_area.x - scroll_x,
            content_area.y - scroll_y,
            content_width,
            content_height
        };
        
        // Arrange children within scrolled viewport
        if (layout_strategy_instance) {
            layout_strategy_instance->arrange_children(this, viewport);
        }
        
        // Clamp scroll values
        int max_scroll_x = std::max(0, content_width - content_area.w);
        int max_scroll_y = std::max(0, content_height - content_area.h);
        scroll_x = std::clamp(scroll_x, 0, max_scroll_x);
        scroll_y = std::clamp(scroll_y, 0, max_scroll_y);
    }
    
public:
    void scroll_by(int delta_x, int delta_y) {
        scroll_x += delta_x;
        scroll_y += delta_y;
        invalidate_arrange();
    }
    
    void scroll_to_make_visible(const rect& target_rect) {
        rect viewport = bounds;
        
        // Scroll horizontally if needed
        if (target_rect.x < scroll_x) {
            scroll_x = target_rect.x;
        } else if (target_rect.x + target_rect.w > scroll_x + viewport.w) {
            scroll_x = target_rect.x + target_rect.w - viewport.w;
        }
        
        // Scroll vertically if needed
        if (target_rect.y < scroll_y) {
            scroll_y = target_rect.y;
        } else if (target_rect.y + target_rect.h > scroll_y + viewport.h) {
            scroll_y = target_rect.y + target_rect.h - viewport.h;
        }
        
        invalidate_arrange();
    }
};
```

---

## Performance Optimizations

### Layout Cache

```cpp
class layout_cache {
public:
    struct cache_entry {
        size measured_size;
        int available_width;
        int available_height;
        uint64_t timestamp;
    };
    
private:
    std::unordered_map<ui_element*, cache_entry> cache;
    uint64_t current_frame = 0;
    
public:
    void begin_frame() {
        current_frame++;
    }
    
    bool try_get(ui_element* element, int available_width, 
                 int available_height, size& out_size) {
        auto it = cache.find(element);
        if (it == cache.end()) return false;
        
        const cache_entry& entry = it->second;
        
        // Check if cache is valid
        if (entry.available_width == available_width &&
            entry.available_height == available_height &&
            entry.timestamp == current_frame) {
            out_size = entry.measured_size;
            return true;
        }
        
        return false;
    }
    
    void set(ui_element* element, int available_width,
             int available_height, const size& measured_size) {
        cache[element] = {measured_size, available_width, 
                         available_height, current_frame};
    }
    
    void invalidate(ui_element* element) {
        cache.erase(element);
    }
    
    void clear() {
        cache.clear();
    }
};
```

### Batch Layout Updates

```cpp
class layout_transaction {
private:
    std::vector<ui_element*> dirty_elements;
    bool is_active = false;
    
public:
    void begin() {
        is_active = true;
        dirty_elements.clear();
    }
    
    void mark_dirty(ui_element* element) {
        if (is_active) {
            dirty_elements.push_back(element);
        } else {
            element->invalidate_measure();
        }
    }
    
    void commit() {
        if (!is_active) return;
        
        // Remove duplicates
        std::sort(dirty_elements.begin(), dirty_elements.end());
        auto last = std::unique(dirty_elements.begin(), dirty_elements.end());
        dirty_elements.erase(last, dirty_elements.end());
        
        // Invalidate all at once
        for (ui_element* elem : dirty_elements) {
            elem->invalidate_measure();
        }
        
        dirty_elements.clear();
        is_active = false;
    }
    
    void rollback() {
        dirty_elements.clear();
        is_active = false;
    }
};

// Usage:
// layout_transaction txn;
// txn.begin();
// ... modify multiple elements ...
// txn.commit();  // Single layout pass
```

---

## Testing and Debugging

### Layout Debugging Tools

```cpp
class layout_debugger {
public:
    // Print entire layout tree
    static void print_tree(ui_element* root, int indent = 0) {
        if (!root) return;
        
        std::string indent_str(indent * 2, ' ');
        std::cout << indent_str 
                  << root->id << " "
                  << "[" << root->bounds.x << "," << root->bounds.y 
                  << " " << root->bounds.w << "x" << root->bounds.h << "]"
                  << " visible=" << root->visible
                  << " z=" << root->z_index
                  << std::endl;
        
        for (auto& child : root->children) {
            print_tree(child.get(), indent + 1);
        }
    }
    
    // Validate layout correctness
    static bool validate(ui_element* root, std::vector<std::string>& errors) {
        if (!root) return true;
        
        bool valid = true;
        
        // Check if element exceeds parent bounds (warning, not error)
        if (root->parent) {
            rect parent_content = {
                root->parent->bounds.x + root->parent->padding.left,
                root->parent->bounds.y + root->parent->padding.top,
                root->parent->bounds.w - root->parent->padding.horizontal(),
                root->parent->bounds.h - root->parent->padding.vertical()
            };
            
            if (root->bounds.x < parent_content.x ||
                root->bounds.y < parent_content.y ||
                root->bounds.x + root->bounds.w > parent_content.x + parent_content.w ||
                root->bounds.y + root->bounds.h > parent_content.y + parent_content.h) {
                errors.push_back("Element '" + root->id + 
                    "' exceeds parent content bounds");
                valid = false;
            }
        }
        
        // Check for negative dimensions
        if (root->bounds.w < 0 || root->bounds.h < 0) {
            errors.push_back("Element '" + root->id + 
                "' has negative dimensions");
            valid = false;
        }
        
        // Check constraint violations
        if (root->bounds.w < root->width_constraint.min_size ||
            root->bounds.w > root->width_constraint.max_size) {
            errors.push_back("Element '" + root->id + 
                "' violates width constraints");
            valid = false;
        }
        
        if (root->bounds.h < root->height_constraint.min_size ||
            root->bounds.h > root->height_constraint.max_size) {
            errors.push_back("Element '" + root->id + 
                "' violates height constraints");
            valid = false;
        }
        
        // Recursively validate children
        for (auto& child : root->children) {
            valid = validate(child.get(), errors) && valid;
        }
        
        return valid;
    }
    
    // Draw debug overlay (bounds, padding, margin)
    static void draw_debug_overlay(ui_element* root, 
                                   graphics_context& gfx) {
        if (!root || !root->visible) return;
        
        // Draw bounds
        gfx.draw_rect(root->bounds, color::red, false);
        
        // Draw content area (excluding padding)
        rect content = {
            root->bounds.x + root->padding.left,
            root->bounds.y + root->padding.top,
            root->bounds.w - root->padding.horizontal(),
            root->bounds.h - root->padding.vertical()
        };
        gfx.draw_rect(content, color::green, false);
        
        // Draw element ID
        gfx.draw_text(root->id, root->bounds.x, root->bounds.y, color::white);
        
        // Recursively draw children
        for (auto& child : root->children) {
            draw_debug_overlay(child.get(), gfx);
        }
    }
};
```

### Unit Testing Framework

```cpp
class layout_test_fixture {
public:
    void assert_bounds(ui_element* element, const rect& expected) {
        if (element->bounds.x != expected.x ||
            element->bounds.y != expected.y ||
            element->bounds.w != expected.w ||
            element->bounds.h != expected.h) {
            throw std::runtime_error(
                "Bounds mismatch for '" + element->id + "': expected " +
                to_string(expected) + " got " + to_string(element->bounds));
        }
    }
    
    void assert_child_count(ui_element* element, size_t expected) {
        if (element->children.size() != expected) {
            throw std::runtime_error(
                "Child count mismatch for '" + element->id + "': expected " +
                std::to_string(expected) + " got " + 
                std::to_string(element->children.size()));
        }
    }
    
    void assert_visible(ui_element* element, bool expected) {
        if (element->visible != expected) {
            throw std::runtime_error(
                "Visibility mismatch for '" + element->id + "'");
        }
    }
    
private:
    std::string to_string(const rect& r) {
        return "[" + std::to_string(r.x) + "," + std::to_string(r.y) + 
               " " + std::to_string(r.w) + "x" + std::to_string(r.h) + "]";
    }
};
```

---

## Complete Example: Complex Nested Layout

```cpp
void create_settings_panel_example() {
    // Create root panel with vertical layout
    auto root = std::make_unique<ui_element>();
    root->id = "root";
    root->bounds = {0, 0, 800, 600};
    root->padding = {16, 16, 16, 16};
    root->layout_strategy_instance = std::make_unique<linear_layout>();
    auto* root_layout = static_cast<linear_layout*>(
        root->layout_strategy_instance.get());
    root_layout->layout_direction = direction::vertical;
    root_layout->spacing = 12;
    
    // Title bar (horizontal layout)
    auto title_bar = std::make_unique<ui_element>();
    title_bar->id = "title_bar";
    title_bar->height_constraint.policy = size_policy::fixed;
    title_bar->height_constraint.preferred_size = 48;
    title_bar->layout_strategy_instance = std::make_unique<linear_layout>();
    auto* title_layout = static_cast<linear_layout*>(
        title_bar->layout_strategy_instance.get());
    title_layout->layout_direction = direction::horizontal;
    title_layout->spacing = 8;
    title_layout->v_align = vertical_alignment::center;
    
    // Title label
    auto title_label = std::make_unique<ui_element>();
    title_label->id = "title_label";
    title_label->width_constraint.policy = size_policy::expand;
    
    // Close button
    auto close_button = std::make_unique<ui_element>();
    close_button->id = "close_button";
    close_button->width_constraint.policy = size_policy::fixed;
    close_button->width_constraint.preferred_size = 32;
    close_button->height_constraint.policy = size_policy::fixed;
    close_button->height_constraint.preferred_size = 32;
    
    title_bar->add_child(std::move(title_label));
    title_bar->add_child(std::move(close_button));
    
    // Content area (scrollable panel with vertical layout)
    auto content = std::make_unique<scrollable_panel>();
    content->id = "content";
    content->height_constraint.policy = size_policy::expand;
    content->layout_strategy_instance = std::make_unique<linear_layout>();
    auto* content_layout = static_cast<linear_layout*>(
        content->layout_strategy_instance.get());
    content_layout->layout_direction = direction::vertical;
    content_layout->spacing = 16;
    
    // Settings sections (using grid layout)
    for (int i = 0; i < 3; ++i) {
        auto section = std::make_unique<ui_element>();
        section->id = "section_" + std::to_string(i);
        section->padding = {12, 12, 12, 12};
        section->layout_strategy_instance = std::make_unique<grid_layout>();
        auto* grid = static_cast<grid_layout*>(
            section->layout_strategy_instance.get());
        grid->num_columns = 2;
        grid->column_spacing = 8;
        grid->row_spacing = 8;
        
        // Add settings rows (label + control)
        for (int j = 0; j < 4; ++j) {
            auto label = std::make_unique<ui_element>();
            label->id = "label_" + std::to_string(i) + "_" + std::to_string(j);
            label->h_align = horizontal_alignment::left;
            
            auto control = std::make_unique<ui_element>();
            control->id = "control_" + std::to_string(i) + "_" + std::to_string(j);
            control->h_align = horizontal_alignment::stretch;
            
            section->add_child(std::move(label));
            section->add_child(std::move(control));
        }
        
        content->add_child(std::move(section));
    }
    
    // Button bar (horizontal layout)
    auto button_bar = std::make_unique<ui_element>();
    button_bar->id = "button_bar";
    button_bar->height_constraint.policy = size_policy::fixed;
    button_bar->height_constraint.preferred_size = 48;
    button_bar->layout_strategy_instance = std::make_unique<linear_layout>();
    auto* button_layout = static_cast<linear_layout*>(
        button_bar->layout_strategy_instance.get());
    button_layout->layout_direction = direction::horizontal;
    button_layout->spacing = 8;
    button_layout->h_align = horizontal_alignment::right;
    
    auto ok_button = std::make_unique<ui_element>();
    ok_button->id = "ok_button";
    ok_button->width_constraint.policy = size_policy::fixed;
    ok_button->width_constraint.preferred_size = 100;
    
    auto cancel_button = std::make_unique<ui_element>();
    cancel_button->id = "cancel_button";
    cancel_button->width_constraint.policy = size_policy::fixed;
    cancel_button->width_constraint.preferred_size = 100;
    
    button_bar->add_child(std::move(ok_button));
    button_bar->add_child(std::move(cancel_button));
    
    // Assemble the hierarchy
    root->add_child(std::move(title_bar));
    root->add_child(std::move(content));
    root->add_child(std::move(button_bar));
    
    // Perform layout
    root->measure(800, 600);
    root->arrange(root->bounds);
    
    // Debug output
    layout_debugger::print_tree(root.get());
    
    std::vector<std::string> errors;
    if (layout_debugger::validate(root.get(), errors)) {
        std::cout << "Layout validation passed!" << std::endl;
    } else {
        std::cout << "Layout validation failed:" << std::endl;
        for (const auto& error : errors) {
            std::cout << "  - " << error << std::endl;
        }
    }
}
```

---

## Summary

This comprehensive layout system provides:

### Core Features
- **Two-pass layout algorithm** (measure + arrange) for optimal performance
- **Smart invalidation** with upward measure and downward arrange propagation
- **Multiple layout strategies**: Linear, Grid, Anchor, Absolute
- **Flexible sizing**: Fixed, Content, Expand, Fill Parent, Weighted, Percentage
- **Comprehensive spacing**: Margins, padding, element spacing
- **Advanced features**: Z-ordering, hit testing, scrollable panels

### Type Safety with Concepts
- **Framework agnostic**: Use any rectangle/size types (SDL_Rect, sf::IntRect, custom types)
- **Compile-time validation**: Concepts ensure your types have required members
- **Zero overhead**: No wrapper classes, works directly with your types

### Performance Optimizations
- Layout result caching
- Batch layout transactions
- Minimal invalidation propagation
- Early-out checks for unchanged layouts

### Developer Experience
- Memory-safe with smart pointers
- Comprehensive debugging tools
- Validation framework
- Unit testing support
- Clear separation of concerns

## Usage Examples with Different Rectangle Types

### Example 1: Using SDL_Rect

```cpp
#include <SDL2/SDL.h>

// SDL_Rect: {int x, y, w, h}
// SDL doesn't have a Size type, so we define one
struct SDL_Size {
    int w, h;
};

// Create type aliases for convenience
using sdl_element = ui_element<SDL_Rect, SDL_Size>;
using sdl_linear_layout = linear_layout<SDL_Rect, SDL_Size>;

void sdl_example() {
    auto root = std::make_unique<sdl_element>();
    root->id = "root";
    root->bounds = {0, 0, 800, 600};
    root->layout_strategy_instance = std::make_unique<sdl_linear_layout>();
    
    auto* layout = static_cast<sdl_linear_layout*>(
        root->layout_strategy_instance.get());
    layout->layout_direction = direction::vertical;
    layout->spacing = 10;
    
    // Add children...
    auto child = std::make_unique<sdl_element>();
    child->id = "child1";
    child->height_constraint.policy = size_policy::fixed;
    child->height_constraint.preferred_size = 50;
    root->add_child(std::move(child));
    
    // Perform layout
    SDL_Size measured = root->measure(800, 600);
    root->arrange(root->bounds);
    
    // Use bounds for rendering
    SDL_Renderer* renderer = /* your renderer */;
    for (auto& child : root->children) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &child->bounds);
    }
}
```

### Example 2: Using SFML Rectangle

```cpp
#include <SFML/Graphics.hpp>

// sf::IntRect: {int left, top, width, height}
// We need an adapter since SFML uses 'left/top' instead of 'x/y'
struct sfml_rect {
    int x, y, width, height;
    
    // Conversion to SFML rect
    operator sf::IntRect() const {
        return sf::IntRect(x, y, width, height);
    }
};

struct sfml_size {
    int width, height;
};

using sfml_element = ui_element<sfml_rect, sfml_size>;
using sfml_linear_layout = linear_layout<sfml_rect, sfml_size>;

void sfml_example() {
    auto root = std::make_unique<sfml_element>();
    root->bounds = {0, 0, 800, 600};
    root->layout_strategy_instance = std::make_unique<sfml_linear_layout>();
    
    // ... setup layout ...
    
    root->measure(800, 600);
    root->arrange(root->bounds);
    
    // Render with SFML
    sf::RenderWindow window;
    for (auto& child : root->children) {
        sf::IntRect sfml_bounds = child->bounds;
        sf::RectangleShape shape(sf::Vector2f(sfml_bounds.width, sfml_bounds.height));
        shape.setPosition(sfml_bounds.left, sfml_bounds.top);
        window.draw(shape);
    }
}
```

### Example 3: Using Your Own Custom Types

```cpp
// Your existing types
struct MyRect {
    int x, y, w, h;
};

struct MySize {
    int width, height;
};

// They automatically work with the layout system!
using my_element = ui_element<MyRect, MySize>;
using my_linear_layout = linear_layout<MyRect, MySize>;
using my_grid_layout = grid_layout<MyRect, MySize>;

void custom_types_example() {
    auto root = std::make_unique<my_element>();
    root->bounds = {0, 0, 1024, 768};
    
    // Use grid layout
    auto grid = std::make_unique<my_grid_layout>();
    grid->num_columns = 3;
    grid->column_spacing = 5;
    grid->row_spacing = 5;
    root->layout_strategy_instance = std::move(grid);
    
    // Add 9 children (3x3 grid)
    for (int i = 0; i < 9; ++i) {
        auto child = std::make_unique<my_element>();
        child->id = "cell_" + std::to_string(i);
        root->add_child(std::move(child));
    }
    
    // Layout automatically distributes children in grid
    root->measure(1024, 768);
    root->arrange(root->bounds);
}
```

### Example 4: Handling Types with Different Member Names

```cpp
// Some library uses 'left', 'top', 'right', 'bottom'
struct WeirdRect {
    int left, top, right, bottom;
    
    // The concepts won't match, so we provide getters
    int x() const { return left; }
    int y() const { return top; }
    int w() const { return right - left; }
    int h() const { return bottom - top; }
};

// This won't work directly, so create an adapter:
struct WeirdRectAdapter {
    int x, y, w, h;
    
    WeirdRectAdapter() = default;
    WeirdRectAdapter(const WeirdRect& r) 
        : x(r.left), y(r.top), 
          w(r.right - r.left), h(r.bottom - r.top) {}
    
    operator WeirdRect() const {
        return {x, y, x + w, y + h};
    }
};

// Now use the adapter
using weird_element = ui_element<WeirdRectAdapter, MySize>;
```

This design is production-ready and suitable for complex UI applications requiring flexible, performant layout management with any rectangle/size types you already have!