// Copyright (c) 2025 OnyxUI Framework
// SPDX-License-Identifier: MIT

#include <doctest/doctest.h>
#include <onyxui/widgets/utils/range_helpers.hh>

using namespace onyxui::range_helpers;

TEST_SUITE("range_helpers") {

// ===== position_to_value Tests =====

TEST_CASE("position_to_value - Basic mapping") {
    // Track of 100 units, range 0-100
    REQUIRE(position_to_value(0, 100, 0, 100) == 0);      // Start
    REQUIRE(position_to_value(50, 100, 0, 100) == 50);    // Middle
    REQUIRE(position_to_value(100, 100, 0, 100) == 100);  // End
}

TEST_CASE("position_to_value - Different range") {
    // Track of 50 units, range 0-200
    REQUIRE(position_to_value(0, 50, 0, 200) == 0);     // Start
    REQUIRE(position_to_value(25, 50, 0, 200) == 100);  // Middle
    REQUIRE(position_to_value(50, 50, 0, 200) == 200);  // End
}

TEST_CASE("position_to_value - Negative range") {
    // Track of 100 units, range -50 to 50
    REQUIRE(position_to_value(0, 100, -50, 50) == -50);   // Start
    REQUIRE(position_to_value(50, 100, -50, 50) == 0);    // Middle
    REQUIRE(position_to_value(100, 100, -50, 50) == 50);  // End
}

TEST_CASE("position_to_value - Clamping") {
    // Position outside track bounds should be clamped
    REQUIRE(position_to_value(-10, 100, 0, 100) == 0);    // Below minimum
    REQUIRE(position_to_value(150, 100, 0, 100) == 100);  // Above maximum
}

TEST_CASE("position_to_value - Edge cases") {
    // Zero-length track
    REQUIRE(position_to_value(50, 0, 0, 100) == 0);  // Returns min_value

    // Invalid range (min >= max)
    REQUIRE(position_to_value(50, 100, 100, 0) == 100);  // Returns min_value

    // Single-unit track
    REQUIRE(position_to_value(0, 1, 0, 100) == 0);
    REQUIRE(position_to_value(1, 1, 0, 100) == 100);
}

// ===== value_to_position Tests =====

TEST_CASE("value_to_position - Basic mapping") {
    // Range 0-100, track of 100 units
    REQUIRE(value_to_position(0, 0, 100, 100) == 0);      // Start
    REQUIRE(value_to_position(50, 0, 100, 100) == 50);    // Middle
    REQUIRE(value_to_position(100, 0, 100, 100) == 100);  // End
}

TEST_CASE("value_to_position - Different track length") {
    // Range 0-200, track of 50 units
    REQUIRE(value_to_position(0, 0, 200, 50) == 0);     // Start
    REQUIRE(value_to_position(100, 0, 200, 50) == 25);  // Middle
    REQUIRE(value_to_position(200, 0, 200, 50) == 50);  // End
}

TEST_CASE("value_to_position - Clamping") {
    // Value outside range should be clamped
    REQUIRE(value_to_position(-10, 0, 100, 100) == 0);    // Below minimum
    REQUIRE(value_to_position(150, 0, 100, 100) == 100);  // Above maximum
}

TEST_CASE("value_to_position - Edge cases") {
    // Zero-length track
    REQUIRE(value_to_position(50, 0, 100, 0) == 0);

    // Invalid range (min >= max)
    REQUIRE(value_to_position(50, 100, 0, 100) == 0);
}

// ===== snap_to_step Tests =====

TEST_CASE("snap_to_step - Round down") {
    REQUIRE(snap_to_step(12, 5) == 10);  // Closer to 10
    REQUIRE(snap_to_step(23, 10) == 20); // Closer to 20
}

TEST_CASE("snap_to_step - Round up") {
    REQUIRE(snap_to_step(13, 5) == 15);  // Closer to 15
    REQUIRE(snap_to_step(27, 10) == 30); // Closer to 30
}

TEST_CASE("snap_to_step - Exact multiples") {
    REQUIRE(snap_to_step(20, 5) == 20);  // Already on step
    REQUIRE(snap_to_step(50, 10) == 50); // Already on step
}

TEST_CASE("snap_to_step - No snapping") {
    REQUIRE(snap_to_step(42, 0) == 42);   // Step = 0, no snapping
    REQUIRE(snap_to_step(42, -1) == 42);  // Negative step, no snapping
}

// ===== calculate_fill_length Tests =====

TEST_CASE("calculate_fill_length - Basic calculation") {
    // Value 50/100 = 50% fill on 100-unit track
    REQUIRE(calculate_fill_length(50, 0, 100, 100) == 50);

    // Value 75/100 = 75% fill on 100-unit track
    REQUIRE(calculate_fill_length(75, 0, 100, 100) == 75);
}

TEST_CASE("calculate_fill_length - Different range") {
    // Value 100/200 = 50% fill on 50-unit track
    REQUIRE(calculate_fill_length(100, 0, 200, 50) == 25);

    // Value 150/200 = 75% fill on 50-unit track
    REQUIRE(calculate_fill_length(150, 0, 200, 50) == 37);
}

TEST_CASE("calculate_fill_length - Boundary values") {
    REQUIRE(calculate_fill_length(0, 0, 100, 100) == 0);    // Empty (0%)
    REQUIRE(calculate_fill_length(100, 0, 100, 100) == 100); // Full (100%)
}

TEST_CASE("calculate_fill_length - Clamping") {
    // Value outside range should be clamped
    REQUIRE(calculate_fill_length(-10, 0, 100, 100) == 0);    // Below min
    REQUIRE(calculate_fill_length(150, 0, 100, 100) == 100);  // Above max
}

TEST_CASE("calculate_fill_length - Edge cases") {
    // Zero-length track
    REQUIRE(calculate_fill_length(50, 0, 100, 0) == 0);

    // Invalid range (min >= max)
    REQUIRE(calculate_fill_length(50, 100, 0, 100) == 0);
}

// ===== calculate_proportional_thumb_size Tests =====

TEST_CASE("calculate_proportional_thumb_size - Basic calculation") {
    // Viewport 50, content 100 = 50% thumb on 100-unit track
    REQUIRE(calculate_proportional_thumb_size(100, 100, 50, 10) == 50);

    // Viewport 25, content 100 = 25% thumb on 100-unit track
    REQUIRE(calculate_proportional_thumb_size(100, 100, 25, 10) == 25);
}

TEST_CASE("calculate_proportional_thumb_size - Minimum size enforcement") {
    // Viewport 5, content 100 = 5% would be 5 units, but min is 10
    REQUIRE(calculate_proportional_thumb_size(100, 100, 5, 10) == 10);

    // Viewport 1, content 100 = 1% would be 1 unit, but min is 15
    REQUIRE(calculate_proportional_thumb_size(100, 100, 1, 15) == 15);
}

TEST_CASE("calculate_proportional_thumb_size - No scrolling needed") {
    // Viewport >= content, no scrolling
    REQUIRE(calculate_proportional_thumb_size(100, 50, 50, 10) == 0);   // Equal
    REQUIRE(calculate_proportional_thumb_size(100, 50, 100, 10) == 0);  // Larger
}

TEST_CASE("calculate_proportional_thumb_size - Edge cases") {
    // Zero-length track
    REQUIRE(calculate_proportional_thumb_size(0, 100, 50, 10) == 0);

    // Zero viewport
    REQUIRE(calculate_proportional_thumb_size(100, 100, 0, 10) == 0);
}

// ===== calculate_proportional_thumb_position Tests =====

TEST_CASE("calculate_proportional_thumb_position - Basic calculation") {
    // Scroll 25, max scroll 50, track 100, thumb 20
    // 25/50 = 50% along available space (100 - 20 = 80)
    // Position = 0.5 * 80 = 40
    REQUIRE(calculate_proportional_thumb_position(25, 50, 100, 20) == 40);

    // Scroll 0, max scroll 50, track 100, thumb 20
    // 0% along track = position 0
    REQUIRE(calculate_proportional_thumb_position(0, 50, 100, 20) == 0);

    // Scroll 50, max scroll 50, track 100, thumb 20
    // 100% along track = position 80 (100 - 20)
    REQUIRE(calculate_proportional_thumb_position(50, 50, 100, 20) == 80);
}

TEST_CASE("calculate_proportional_thumb_position - Clamping") {
    // Scroll offset outside range should be clamped
    REQUIRE(calculate_proportional_thumb_position(-10, 50, 100, 20) == 0);  // Below min
    REQUIRE(calculate_proportional_thumb_position(100, 50, 100, 20) == 80); // Above max
}

TEST_CASE("calculate_proportional_thumb_position - Edge cases") {
    // Max scroll = 0 (no scrolling)
    REQUIRE(calculate_proportional_thumb_position(0, 0, 100, 20) == 0);

    // Thumb size >= track length
    REQUIRE(calculate_proportional_thumb_position(25, 50, 100, 100) == 0);
    REQUIRE(calculate_proportional_thumb_position(25, 50, 100, 150) == 0);
}

}  // TEST_SUITE
