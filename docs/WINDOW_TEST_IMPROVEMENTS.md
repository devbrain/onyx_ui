# Window Test Coverage Improvements

## Current Status (As of Phase 3)

**Test Count**: 16 test cases, 86 assertions
**Coverage**: ~40% meaningful coverage

## Missing Test Categories

### 1. Visual/Rendering Tests ❌ CRITICAL

**Problem**: Zero rendering verification. Tests only check state, not visual output.

**Needed**:
```cpp
TEST_CASE("Window - Rendering") {
    SUBCASE("Window border rendered when m_has_border=true") {
        ui_context_fixture<test_canvas_backend> fixture;

        window<test_canvas_backend> win("Test");
        win.set_has_border(true);
        win.set_position(10, 10);
        win.set_size(100, 50);

        fixture.render(win);

        // Verify border drawn at window bounds
        auto& canvas = fixture.backend().canvas();
        CHECK(canvas.has_rect_at(10, 10, 100, 50));
    }

    SUBCASE("Title bar rendered when has_title_bar=true") {
        ui_context_fixture<test_canvas_backend> fixture;

        window<test_canvas_backend> win("My Title");
        win.set_position(0, 0);
        win.set_size(200, 100);

        fixture.render(win);

        auto& canvas = fixture.backend().canvas();
        // Verify title bar background filled
        CHECK(canvas.has_filled_rect_at(0, 0, 200, TITLE_BAR_HEIGHT));
        // Verify title text drawn
        CHECK(canvas.has_text("My Title"));
    }

    SUBCASE("Control buttons rendered based on flags") {
        ui_context_fixture<test_canvas_backend> fixture;

        typename window<test_canvas_backend>::window_flags flags;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_close_button = true;

        window<test_canvas_backend> win("Test", flags);
        fixture.render(win);

        auto& canvas = fixture.backend().canvas();
        // Verify button symbols rendered
        CHECK(canvas.has_text("[_]"));  // Minimize
        CHECK(canvas.has_text("[□]"));  // Maximize
        CHECK(canvas.has_text("[X]"));  // Close
    }
}
```

### 2. Structural/Composition Tests ❌ MISSING

**Problem**: No verification that child widgets are actually created.

**Needed**:
```cpp
TEST_CASE("Window - Child Widget Creation") {
    SUBCASE("Title bar created when has_title_bar=true") {
        window<test_backend> win("Test");

        // Access title bar (need to add public getter or friend test)
        auto children = win.get_children();
        CHECK(children.size() >= 1);  // At least title bar

        // Verify title bar is first child
        // Note: Need to expose this via API
    }

    SUBCASE("No title bar when has_title_bar=false") {
        typename window<test_backend>::window_flags flags;
        flags.has_title_bar = false;

        window<test_backend> win("Test", flags);

        auto children = win.get_children();
        CHECK(children.size() == 1);  // Only content area
    }

    SUBCASE("Title propagates to title bar label") {
        window<test_backend> win("Initial");

        // Need API to inspect title bar's label text
        // Or use rendering test

        win.set_title("Updated");

        // Verify title bar label updated
        // CHECK(get_title_bar_label_text(win) == "Updated");
    }
}
```

### 3. Resize Handle Detection Tests ❌ MISSING

**Problem**: No test for `get_resize_handle_at()` logic.

**Needed**:
```cpp
TEST_CASE("Window - Resize Handle Detection") {
    SUBCASE("Detect corner handles") {
        window<test_backend> win;
        win.set_position(100, 100);
        win.set_size(200, 150);

        // Need to expose get_resize_handle_at() or test via protected access
        // Or create test-friendly window subclass

        // Top-left corner (100, 100) within 4px border
        CHECK(win.get_resize_handle_at(101, 101) == resize_handle::north_west);

        // Top-right corner
        CHECK(win.get_resize_handle_at(299, 101) == resize_handle::north_east);

        // Bottom-left corner
        CHECK(win.get_resize_handle_at(101, 249) == resize_handle::south_west);

        // Bottom-right corner
        CHECK(win.get_resize_handle_at(299, 249) == resize_handle::south_east);
    }

    SUBCASE("Detect edge handles") {
        window<test_backend> win;
        win.set_position(100, 100);
        win.set_size(200, 150);

        // Top edge (center)
        CHECK(win.get_resize_handle_at(200, 101) == resize_handle::north);

        // Bottom edge
        CHECK(win.get_resize_handle_at(200, 249) == resize_handle::south);

        // Left edge
        CHECK(win.get_resize_handle_at(101, 175) == resize_handle::west);

        // Right edge
        CHECK(win.get_resize_handle_at(299, 175) == resize_handle::east);
    }

    SUBCASE("No handle in center") {
        window<test_backend> win;
        win.set_position(100, 100);
        win.set_size(200, 150);

        // Center of window
        CHECK(win.get_resize_handle_at(200, 175) == resize_handle::none);
    }

    SUBCASE("No handle when not resizable") {
        typename window<test_backend>::window_flags flags;
        flags.is_resizable = false;

        window<test_backend> win("Test", flags);
        win.set_position(100, 100);
        win.set_size(200, 150);

        // Should return none even on borders
        CHECK(win.get_resize_handle_at(101, 101) == resize_handle::none);
    }
}
```

### 4. Size Constraint Enforcement Tests ❌ INCOMPLETE

**Problem**: Tests only check flag values, not actual enforcement.

**Needed**:
```cpp
TEST_CASE("Window - Size Constraint Enforcement") {
    SUBCASE("Minimum size enforced") {
        typename window<test_backend>::window_flags flags;
        flags.min_width = 200;
        flags.min_height = 150;

        window<test_backend> win("Test", flags);

        // Try to set size below minimum
        win.set_size(100, 50);

        // Should be clamped to minimum
        CHECK(win.bounds().w >= 200);
        CHECK(win.bounds().h >= 150);
    }

    SUBCASE("Maximum size enforced") {
        typename window<test_backend>::window_flags flags;
        flags.max_width = 500;
        flags.max_height = 400;

        window<test_backend> win("Test", flags);

        // Try to set size above maximum
        win.set_size(1000, 800);

        // Should be clamped to maximum
        CHECK(win.bounds().w <= 500);
        CHECK(win.bounds().h <= 400);
    }

    SUBCASE("Constraints applied during resize") {
        // Need to simulate resize events
        // Or expose apply_size_constraints() for testing
    }
}
```

### 5. Drag/Resize Event Integration Tests ❌ MISSING

**Problem**: Can't test event handling because `handle_event()` is protected.

**Solutions**:
1. Create test-friendly window subclass that exposes handle_event
2. Use friend class for tests
3. Test via signal emission (indirect)

**Needed**:
```cpp
// Option 1: Test-friendly subclass
template<UIBackend Backend>
class test_window : public window<Backend> {
public:
    using window<Backend>::window;
    using window<Backend>::handle_event;  // Expose for testing
};

TEST_CASE("Window - Drag Event Handling") {
    SUBCASE("Drag updates window position") {
        test_window<test_backend> win("Test");
        win.set_position(100, 100);
        win.set_size(300, 200);

        // Simulate mouse press on title bar
        mouse_event press{150, 110, mouse_event::button::left,
                         mouse_event::action::press, {false, false, false}};
        win.handle_event(press, event_phase::bubble);

        // Simulate drag
        mouse_event move{200, 150, mouse_event::button::none,
                        mouse_event::action::move, {false, false, false}};
        win.handle_event(move, event_phase::bubble);

        // Window should have moved by delta (+50, +40)
        CHECK(win.bounds().x == 150);
        CHECK(win.bounds().y == 140);
    }
}

TEST_CASE("Window - Resize Event Handling") {
    SUBCASE("Southeast corner resize") {
        test_window<test_backend> win("Test");
        win.set_position(100, 100);
        win.set_size(300, 200);

        // Press on southeast corner (398, 298)
        mouse_event press{398, 298, mouse_event::button::left,
                         mouse_event::action::press, {false, false, false}};
        win.handle_event(press, event_phase::bubble);

        // Drag to (450, 350)
        mouse_event move{450, 350, mouse_event::button::none,
                        mouse_event::action::move, {false, false, false}};
        win.handle_event(move, event_phase::bubble);

        // Size should increase by delta (+52, +52)
        CHECK(win.bounds().w == 352);
        CHECK(win.bounds().h == 252);
    }
}
```

### 6. Content Area Tests ❌ MISSING

**Needed**:
```cpp
TEST_CASE("Window - Content Area") {
    SUBCASE("Content area scrollable when flag set") {
        typename window<test_backend>::window_flags flags;
        flags.is_scrollable = true;

        window<test_backend> win("Test", flags);

        // Need to verify scroll_view was created
        // This requires inspecting content_area's children
    }

    SUBCASE("Content positioned correctly within window") {
        window<test_backend> win("Test");
        win.set_position(50, 50);
        win.set_size(400, 300);

        auto label = std::make_unique<label<test_backend>>("Content");
        win.set_content(std::move(label));

        // Need to measure/arrange
        win.measure(400, 300);
        win.arrange({50, 50, 400, 300});

        // Verify content is positioned relative to window
        // Content should be below title bar if present
    }
}
```

## Summary

### Coverage Gaps:
- **Visual/Rendering**: 0% (critical gap)
- **Structural Verification**: 20%
- **Resize Logic**: 30%
- **Drag Logic**: 10%
- **Size Constraints**: 40%
- **Event Handling**: 0% (blocked by protected methods)

### Recommendations:

1. **Add test_canvas_backend fixture** for visual tests
2. **Create test_window subclass** to expose protected methods
3. **Add public inspection API** (get_title_bar, get_content_area, etc.) or friend test classes
4. **Test resize handle detection** comprehensively
5. **Test size constraint enforcement** in resize scenarios
6. **Add rendering verification** for all visual elements

### Priority:
1. 🔴 HIGH: Resize handle detection tests
2. 🔴 HIGH: Size constraint enforcement tests
3. 🟡 MEDIUM: Visual/rendering tests (requires test infrastructure)
4. 🟡 MEDIUM: Structural composition tests
5. 🟢 LOW: Event handling integration tests (blocked by architecture)
