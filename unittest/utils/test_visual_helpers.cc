/**
 * @file test_visual_helpers.cc
 * @brief Tests for visual_test_harness utility
 * @author Testing Infrastructure Team
 * @date 2025-10-28
 */

#include <doctest/doctest.h>
#include "visual_test_helpers.hh"
#include "test_canvas_backend.hh"
#include <onyxui/widgets/panel.hh>

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE("Visual Test Harness - Construction") {
    visual_test_harness<test_canvas_backend> harness(80, 25);

    CHECK(harness.width() == 80);
    CHECK(harness.height() == 25);
    CHECK(harness.canvas() != nullptr);
}

TEST_CASE("Visual Test Harness - Canvas dump") {
    visual_test_harness<test_canvas_backend> harness(10, 3);

    std::string dump = harness.dump_canvas();

    // Should have borders and line numbers
    CHECK_FALSE(dump.empty());
    CHECK(dump.find("+----------+") != std::string::npos);  // Top border
}

TEST_CASE("Visual Test Harness - expect_char_at") {
    visual_test_harness<test_canvas_backend> harness(10, 10);

    // Canvas should be initially filled with spaces
    harness.expect_char_at(0, 0, ' ');
    harness.expect_char_at(5, 5, ' ');
    harness.expect_char_at(9, 9, ' ');
}

TEST_CASE("Visual Test Harness - expect_empty_region") {
    visual_test_harness<test_canvas_backend> harness(20, 20);

    typename test_canvas_backend::rect_type rect{5, 5, 10, 10};

    // Initially empty canvas should pass
    harness.expect_empty_region(rect);
}

// Note: Out-of-bounds coordinates are handled gracefully by expect_char_at()
// They will log errors via CHECK_MESSAGE but won't crash
// This is intentional - tests should validate within bounds
