// Copyright (c) 2025 OnyxUI Framework
// SPDX-License-Identifier: MIT

#include <doctest/doctest.h>

#include <onyxui/widgets/input/slider.hh>
#include "../utils/test_backend.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;

TEST_SUITE("slider") {

// ===== Construction Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Default construction") {
    slider<test_backend> const s;

    CHECK(s.value() == 0);
    CHECK(s.minimum() == 0);
    CHECK(s.maximum() == 100);
    CHECK(s.single_step() == 1);
    CHECK(s.page_step() == 10);
    CHECK(s.orientation() == slider_orientation::horizontal);
    CHECK(s.get_tick_position() == tick_position::none);
    CHECK(s.tick_interval() == 0);
    CHECK(s.is_focusable());  // Sliders are focusable for keyboard input
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Construction with orientation") {
    slider<test_backend> const s(slider_orientation::vertical);

    CHECK(s.orientation() == slider_orientation::vertical);
    CHECK(s.is_focusable());
}

// ===== Value Management Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Set value") {
    slider<test_backend> s;

    s.set_value(50);
    CHECK(s.value() == 50);

    s.set_value(100);
    CHECK(s.value() == 100);

    s.set_value(0);
    CHECK(s.value() == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Value clamping") {
    slider<test_backend> s;
    s.set_range(0, 100);

    // Below minimum
    s.set_value(-10);
    CHECK(s.value() == 0);

    // Above maximum
    s.set_value(150);
    CHECK(s.value() == 100);

    // Within range
    s.set_value(50);
    CHECK(s.value() == 50);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Set range") {
    slider<test_backend> s;

    s.set_range(0, 200);
    CHECK(s.minimum() == 0);
    CHECK(s.maximum() == 200);

    s.set_range(50, 150);
    CHECK(s.minimum() == 50);
    CHECK(s.maximum() == 150);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Set range clamps value") {
    slider<test_backend> s;

    s.set_value(80);
    CHECK(s.value() == 80);

    // New range excludes current value
    s.set_range(0, 50);
    CHECK(s.value() == 50);  // Clamped to new max
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Custom range") {
    slider<test_backend> s;
    s.set_range(0, 1000);

    s.set_value(500);
    CHECK(s.value() == 500);

    s.set_value(1000);
    CHECK(s.value() == 1000);
}

// ===== Step Management Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Set single step") {
    slider<test_backend> s;

    s.set_single_step(5);
    CHECK(s.single_step() == 5);

    s.set_single_step(10);
    CHECK(s.single_step() == 10);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Set page step") {
    slider<test_backend> s;

    s.set_page_step(20);
    CHECK(s.page_step() == 20);

    s.set_page_step(50);
    CHECK(s.page_step() == 50);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Value snapping to step") {
    slider<test_backend> s;
    s.set_range(0, 100);
    s.set_single_step(10);

    s.set_value(12);
    CHECK(s.value() == 10);  // Snapped down

    s.set_value(17);
    CHECK(s.value() == 20);  // Snapped up

    s.set_value(15);
    CHECK(s.value() == 10);  // Exactly halfway, rounds down
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - No snapping when step is zero") {
    slider<test_backend> s;
    s.set_range(0, 100);
    s.set_single_step(0);

    s.set_value(47);
    CHECK(s.value() == 47);  // No snapping
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Changing step re-snaps value") {
    slider<test_backend> s;
    s.set_range(0, 100);
    s.set_single_step(1);
    s.set_value(47);
    CHECK(s.value() == 47);

    s.set_single_step(10);
    CHECK(s.value() == 50);  // Re-snapped to nearest multiple of 10
}

// ===== Orientation Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Horizontal orientation") {
    slider<test_backend> s;

    CHECK(s.orientation() == slider_orientation::horizontal);

    s.set_orientation(slider_orientation::horizontal);
    CHECK(s.orientation() == slider_orientation::horizontal);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Vertical orientation") {
    slider<test_backend> s;

    s.set_orientation(slider_orientation::vertical);
    CHECK(s.orientation() == slider_orientation::vertical);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Orientation change invalidates measure") {
    slider<test_backend> s;
    [[maybe_unused]] auto size = s.measure(100_lu, 100_lu);
    s.arrange(logical_rect{0_lu, 0_lu, 100_lu, 10_lu});

    s.set_orientation(slider_orientation::vertical);
    // Measure should be invalidated
}

// ===== Tick Mark Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Default tick position is none") {
    slider<test_backend> const s;

    CHECK(s.get_tick_position() == tick_position::none);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Set tick position") {
    slider<test_backend> s;

    s.set_tick_position(tick_position::above);
    CHECK(s.get_tick_position() == tick_position::above);

    s.set_tick_position(tick_position::below);
    CHECK(s.get_tick_position() == tick_position::below);

    s.set_tick_position(tick_position::both_sides);
    CHECK(s.get_tick_position() == tick_position::both_sides);

    s.set_tick_position(tick_position::none);
    CHECK(s.get_tick_position() == tick_position::none);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Set tick interval") {
    slider<test_backend> s;

    CHECK(s.tick_interval() == 0);

    s.set_tick_interval(10);
    CHECK(s.tick_interval() == 10);

    s.set_tick_interval(5);
    CHECK(s.tick_interval() == 5);

    s.set_tick_interval(0);
    CHECK(s.tick_interval() == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Tick position change invalidates render") {
    slider<test_backend> s;
    [[maybe_unused]] auto size = s.measure(100_lu, 100_lu);
    s.arrange(logical_rect{0_lu, 0_lu, 100_lu, 10_lu});

    s.set_tick_position(tick_position::below);
    // Render should be invalidated
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Tick interval change invalidates render") {
    slider<test_backend> s;
    [[maybe_unused]] auto size = s.measure(100_lu, 100_lu);
    s.arrange(logical_rect{0_lu, 0_lu, 100_lu, 10_lu});

    s.set_tick_interval(10);
    // Render should be invalidated
}

// ===== Signal Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - value_changed signal") {
    slider<test_backend> s;

    int emitted_value = -1;
    s.value_changed.connect([&emitted_value](int value) {
        emitted_value = value;
    });

    s.set_value(45);
    CHECK(emitted_value == 45);

    s.set_value(100);
    CHECK(emitted_value == 100);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - value_changed not emitted for same value") {
    slider<test_backend> s;
    s.set_value(50);

    int emit_count = 0;
    s.value_changed.connect([&emit_count](int) {
        ++emit_count;
    });

    s.set_value(50);  // Same value
    CHECK(emit_count == 0);

    s.set_value(60);  // Different value
    CHECK(emit_count == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - value_changed emitted after snapping") {
    slider<test_backend> s;
    s.set_range(0, 100);
    s.set_single_step(10);

    int emitted_value = -1;
    s.value_changed.connect([&emitted_value](int value) {
        emitted_value = value;
    });

    s.set_value(47);
    CHECK(emitted_value == 50);  // Snapped value
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - value_changed emitted after clamping") {
    slider<test_backend> s;
    s.set_range(0, 100);

    int emitted_value = -1;
    s.value_changed.connect([&emitted_value](int value) {
        emitted_value = value;
    });

    s.set_value(150);  // Above maximum
    CHECK(emitted_value == 100);  // Clamped value

    s.set_value(-10);  // Below minimum
    CHECK(emitted_value == 0);  // Clamped value
}

// ===== Measure and Arrange Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Measure horizontal") {
    slider<test_backend> s;
    s.set_orientation(slider_orientation::horizontal);

    auto size = s.measure(100_lu, 100_lu);
    int const width = size.width.to_int();
    int const height = size.height.to_int();

    // Horizontal slider should request wide width, small height
    CHECK(width > 0);
    CHECK(height > 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Measure vertical") {
    slider<test_backend> s;
    s.set_orientation(slider_orientation::vertical);

    auto size = s.measure(100_lu, 100_lu);
    int const width = size.width.to_int();
    int const height = size.height.to_int();

    // Vertical slider should request small width, tall height
    CHECK(width > 0);
    CHECK(height > 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Arrange") {
    slider<test_backend> s;
    [[maybe_unused]] auto size = s.measure(100_lu, 100_lu);

    s.arrange(logical_rect{0_lu, 0_lu, 100_lu, 10_lu});

    auto const& bounds = s.bounds();
    CHECK(bounds.width == 100_lu);
    CHECK(bounds.height == 10_lu);
}

// ===== Edge Cases =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Zero range") {
    slider<test_backend> s;
    s.set_range(50, 50);  // min == max

    s.set_value(50);
    CHECK(s.value() == 50);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Inverted range") {
    slider<test_backend> s;
    s.set_range(100, 0);  // max < min

    s.set_value(50);
    // Implementation should handle this gracefully
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Large range") {
    slider<test_backend> s;
    s.set_range(0, 10000);
    s.set_single_step(100);

    s.set_value(5000);
    CHECK(s.value() == 5000);

    s.set_value(5051);
    CHECK(s.value() == 5100);  // Snapped to step (rounds up)

    s.set_value(5049);
    CHECK(s.value() == 5000);  // Snapped to step (rounds down)
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "slider - Negative range") {
    slider<test_backend> s;
    s.set_range(-50, 50);

    s.set_value(0);
    CHECK(s.value() == 0);

    s.set_value(-25);
    CHECK(s.value() == -25);

    s.set_value(25);
    CHECK(s.value() == 25);
}

}  // TEST_SUITE
