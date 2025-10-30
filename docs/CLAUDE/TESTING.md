# OnyxUI Testing Guide

This document describes the testing strategy and best practices for the OnyxUI framework.

## Table of Contents

- [Overview](#overview)
- [Test Organization](#test-organization)
- [Writing Widget Tests](#writing-widget-tests)
- [Writing Non-Widget Tests](#writing-non-widget-tests)
- [Best Practices](#best-practices)
- [Running Tests](#running-tests)

---

## Overview

The project uses **doctest** (fetched via CMake FetchContent) for unit testing.

**Current Coverage:**
- **996 test cases** across 34 test files
- **5533 assertions** total
- **All tests must pass with zero warnings**

---

## Test Organization

```
unittest/
  core/                  # Core functionality tests
    test_signal_slot.cc  # Signal/slot tests (92 test cases)
    test_rule_of_five.cc # Move semantics tests
    test_background_renderer.cc # Background rendering (7 cases)

  layout/                # Layout algorithm tests
    test_composition.cc  # Nested layout tests
    test_linear_layout.cc
    test_grid_layout.cc

  widgets/               # Widget tests (200+ test cases)
    test_button.cc
    test_label.cc
    test_menus.cc
    test_group_box.cc
    test_status_bar.cc
    test_scroll_info.cc
    test_scrollable.cc
    test_scrollbar.cc
    test_scroll_controller.cc
    test_scroll_view.cc
    test_scroll_view_presets.cc
    test_scrolling_integration.cc

  hotkeys/               # Hotkey system tests
    test_hotkeys.cc
    test_hotkey_manager.cc

  focus/                 # Focus management tests
    test_focus_manager.cc

  theming/               # Theme system tests
    test_resolved_style.cc
    test_style_inheritance.cc
    test_style_edge_cases.cc
```

---

## Writing Widget Tests

Widgets need a `ui_context` with registered themes. Use the `ui_context_fixture` helper:

```cpp
#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include "../utils/test_canvas_backend.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Widget - Basic functionality") {
    SUBCASE("First scenario") {
        button<test_canvas_backend> btn("Click me");

        // The fixture provides ctx which sets up themes automatically
        auto size = btn.measure(100, 50);
        CHECK(size_utils::get_width(size) > 0);
    }

    SUBCASE("Second scenario") {
        label<test_canvas_backend> lbl("Text");
        CHECK_FALSE(lbl.is_focusable());
    }
}
```

**Why use the fixture?**
- Widgets require a `ui_context` with registered themes
- The fixture automatically sets up themes before each test
- Ensures consistent test environment

---

## Writing Non-Widget Tests

For non-widget tests (core functionality, algorithms, etc.), use regular `TEST_CASE`:

```cpp
#include <doctest/doctest.h>

TEST_CASE("Core - Basic functionality") {
    SUBCASE("First scenario") {
        // Test code
        CHECK(condition);
    }

    SUBCASE("Second scenario") {
        // More test code
        REQUIRE(critical_condition);
    }
}
```

---

## Best Practices

### General Guidelines

- **Use `ui_context_fixture<test_canvas_backend>`** for widget tests
- **Use `test_canvas_backend`** (not `test_backend`) for consistency
- **Test both measure and arrange phases** for layout widgets
- **Verify exact bounds**, not just success
- **Test edge cases**: empty, zero size, max values, negative values
- **Use SUBCASEs** for related scenarios within a test
- **Each test should be independent** - no shared state between tests

### Widget Testing Checklist

When testing a new widget:

- [ ] Constructor with default parameters
- [ ] Constructor with custom parameters
- [ ] Measure pass (various available sizes)
- [ ] Arrange pass (various final bounds)
- [ ] Rendering (visual validation)
- [ ] Event handling (mouse, keyboard)
- [ ] Theme application and inheritance
- [ ] Focus behavior (if focusable)
- [ ] Edge cases (empty content, zero size, overflow)
- [ ] Move semantics (copy/move construction and assignment)

### Layout Testing Checklist

When testing a layout strategy:

- [ ] Empty container
- [ ] Single child
- [ ] Multiple children
- [ ] Children with various size policies (fixed, content, expand, etc.)
- [ ] Constrained space (available size < desired size)
- [ ] Unconstrained space
- [ ] Nested layouts
- [ ] Edge cases (negative sizes, zero sizes, max sizes)

### Example: Comprehensive Widget Test

```cpp
TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "button - Comprehensive") {
    SUBCASE("Construction") {
        button<test_canvas_backend> btn1;
        CHECK(btn1.get_text().empty());

        button<test_canvas_backend> btn2("Click me");
        CHECK(btn2.get_text() == "Click me");
    }

    SUBCASE("Measure pass") {
        button<test_canvas_backend> btn("OK");

        auto size = btn.measure(100, 50);
        CHECK(size_utils::get_width(size) > 0);
        CHECK(size_utils::get_height(size) > 0);
        CHECK(size_utils::get_width(size) <= 100);
        CHECK(size_utils::get_height(size) <= 50);
    }

    SUBCASE("Arrange pass") {
        button<test_canvas_backend> btn("OK");

        auto size = btn.measure(100, 50);
        btn.arrange({10, 20, size_utils::get_width(size), size_utils::get_height(size)});

        auto bounds = btn.bounds();
        CHECK(rect_utils::get_x(bounds) == 10);
        CHECK(rect_utils::get_y(bounds) == 20);
    }

    SUBCASE("Rendering") {
        button<test_canvas_backend> btn("OK");
        btn.measure(100, 50);
        btn.arrange({0, 0, 100, 50});

        test_canvas_renderer renderer(100, 100);
        resolved_style<test_canvas_backend> style = /* ... */;
        draw_context<test_canvas_backend> ctx(renderer, style, {});

        btn.do_render(ctx);
        // Verify rendered content using renderer's canvas
    }

    SUBCASE("Event handling") {
        button<test_canvas_backend> btn("OK");
        int click_count = 0;

        btn.clicked.connect([&]() { ++click_count; });

        // Simulate click
        typename test_canvas_backend::event_type event = /* ... */;
        btn.on_mouse_down(event);
        btn.on_mouse_up(event);

        CHECK(click_count == 1);
    }

    SUBCASE("Edge cases") {
        button<test_canvas_backend> btn("");  // Empty text
        auto size = btn.measure(100, 50);
        CHECK(size_utils::get_width(size) >= 0);

        auto size_zero = btn.measure(0, 0);  // Zero space
        CHECK(size_utils::get_width(size_zero) >= 0);
    }

    SUBCASE("Move semantics") {
        button<test_canvas_backend> btn1("Original");
        button<test_canvas_backend> btn2(std::move(btn1));

        CHECK(btn2.get_text() == "Original");
        CHECK(btn1.get_text().empty());  // Moved-from state
    }
}
```

---

## Running Tests

### All Tests

```bash
# Run all 996 unit tests
./build/bin/ui_unittest
```

### List Available Tests

```bash
# List all test cases
./build/bin/ui_unittest --list-test-cases
```

### Run Specific Test

```bash
# Run specific test by name
./build/bin/ui_unittest --test-case="Signal - Basic connection and emission"
```

### Run Tests Matching Pattern

```bash
# Run all button tests
./build/bin/ui_unittest --test-case="button*"

# Exclude performance tests
./build/bin/ui_unittest --test-case-exclude="*Performance*"
```

### Build and Run Tests

```bash
# Build only tests
cmake --build build --target ui_unittest -j8

# Run tests
./build/bin/ui_unittest
```

### With Code Quality Checks

```bash
# Build with clang-tidy enabled
cmake -B build -DONYX_UI_ENABLE_CLANG_TIDY=ON
cmake --build build 2>&1 | grep "warning:"

# All 996 tests should pass with zero warnings
```

### With Sanitizers

```bash
# Build with sanitizers enabled
cmake -B build -DONYX_UI_ENABLE_SANITIZERS=ON
cmake --build build -j8

# Run tests with sanitizer checks
./build/bin/ui_unittest
```

---

## Test Coverage Goals

- **Line coverage**: >90%
- **Branch coverage**: >85%
- **Widget coverage**: 100% (all widgets must have tests)
- **Layout coverage**: 100% (all layout strategies must have tests)
- **Edge case coverage**: All edge cases documented in code must have tests

---

## Key Testing Utilities

- `unittest/utils/test_helpers.hh` - Test fixtures and utilities
- `unittest/utils/test_canvas_backend.hh` - Test backend for widget testing
- `unittest/utils/test_canvas_renderer.hh` - Canvas-based renderer for validation

---

## Common Pitfalls

1. **Forgetting to use `ui_context_fixture`** for widget tests → Theme not available
2. **Not testing edge cases** → Bugs in production
3. **Tests with shared state** → Flaky tests
4. **Not checking exact bounds** → Layout bugs missed
5. **Testing too much in one test case** → Hard to debug failures

---

## Adding New Tests

When adding a new widget or feature:

1. Create test file in appropriate directory (`unittest/widgets/`, `unittest/core/`, etc.)
2. Use `ui_context_fixture` if testing widgets
3. Follow naming convention: `test_<feature>.cc`
4. Add comprehensive test cases covering all functionality
5. Include edge case tests
6. Run all tests to ensure no regressions
7. Update this document if adding new testing patterns

---

## Key Reference Files

- `unittest/utils/test_helpers.hh` - Test fixtures
- `unittest/utils/test_canvas_backend.hh` - Test backend
- `unittest/widgets/test_button.cc` - Example comprehensive widget test
- `unittest/widgets/test_scrolling_integration.cc` - Example integration test
