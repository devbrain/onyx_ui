---
sidebar_position: 2
---

# The Two-Pass Layout System

At the core of OnyxUI's ability to render complex and dynamic user interfaces is its powerful two-pass layout system. This system is responsible for determining the size and position of every element on the screen. Understanding how it works is key to building efficient and predictable UIs with OnyxUI.

The layout process is divided into two distinct phases or "passes":

1.  **The Measure Pass (Bottom-Up):** In this phase, the system determines the desired size of each element.
2.  **The Arrange Pass (Top-Down):** In this phase, the system assigns a final size and position to each element.

This two-pass approach is inspired by modern UI frameworks and is designed to be highly efficient, avoiding redundant calculations through a smart caching and invalidation system.

## Phase 1: The Measure Pass

The measure pass is a bottom-up process, meaning it starts with the deepest elements in the UI tree (the leaves) and works its way up to the root.

Here's how it works:

1.  **Parent asks child to measure:** A parent element asks each of its children to determine its desired size by calling the `measure()` method. The parent provides the available space as an argument.
2.  **Child calculates its desired size:** The child element, based on its content (e.g., text, an image, or other children), its layout strategy, and any size constraints, calculates its preferred size.
3.  **Child returns desired size to parent:** The child returns its calculated size to the parent.
4.  **Parent aggregates child sizes:** The parent element collects the desired sizes of all its children and, based on its own layout strategy (e.g., `linear_layout`, `grid_layout`), calculates its own desired size.
5.  **Process repeats up the tree:** This process continues up the UI tree until the root element has determined its desired size.

```cpp
// During the measure pass, this is what's happening conceptually:
logical_size desired_size = parent_element->measure(available_width, available_height);

// Example with actual values:
logical_size size = widget->measure(80_lu, 25_lu);
```

The result of the measure pass is that every element in the tree has determined its ideal size, but no final positions have been assigned yet.

## Phase 2: The Arrange Pass

The arrange pass is a top-down process. It starts at the root of the UI tree and works its way down to the leaves.

Here's how it works:

1.  **Root element receives final bounds:** The process begins when the root element is given its final size and position (usually the size of the window or screen).
2.  **Parent arranges its children:** The parent element, knowing its own final bounds, uses its layout strategy to determine the final size and position for each of its children.
3.  **Parent calls `arrange()` on each child:** The parent calls the `arrange()` method on each child, passing the calculated final bounds as an argument.
4.  **Process repeats down the tree:** Each child, now knowing its own final bounds, repeats the process, arranging its own children within its new boundaries. This continues until every element in the tree has been assigned a final size and position.

```cpp
// During the arrange pass, this is what's happening conceptually:
root_element->arrange(logical_rect{x, y, width, height});

// Example with actual values:
widget->arrange(logical_rect{10_lu, 5_lu, 80_lu, 25_lu});
```

## Smart Caching and Invalidation

To avoid performing these calculations on every frame, OnyxUI employs a smart caching and invalidation system.

-   **Caching:** The results of the `measure()` method are cached. If `measure()` is called again with the same available space, and the element's state has not changed, the cached value is returned immediately without any recalculation.
-   **Invalidation:** When a property of an element that affects its size or layout changes (e.g., its text, padding, or a child is added), the element's layout state is marked as "dirty".
    -   `invalidate_measure()`: If a size-affecting property changes, this method is called. It marks the current element and all of its ancestors as needing to be remeasured. This propagates **up** the tree.
    -   `invalidate_arrange()`: If a position-affecting property changes, this method is called. It marks the current element and all of its descendants as needing to be rearranged. This propagates **down** the tree.

This system ensures that only the parts of the UI tree that are affected by a change are recalculated, which is a key factor in OnyxUI's performance.

## Conclusion

The two-pass layout system is a powerful and efficient mechanism for managing complex UI layouts. By understanding the measure and arrange passes, and the role of the smart caching and invalidation system, you can build UIs that are not only beautiful and functional but also highly performant.
