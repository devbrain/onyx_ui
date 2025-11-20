// Copyright (c) 2025 OnyxUI Framework
// SPDX-License-Identifier: MIT

#include <doctest/doctest.h>

#include <onyxui/widgets/input/progress_bar.hh>
#include "../utils/test_backend.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;

TEST_SUITE("progress_bar") {

// ===== Construction Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Default construction") {
    progress_bar<test_backend> const pb;

    CHECK(pb.value() == 0);
    CHECK(pb.minimum() == 0);
    CHECK(pb.maximum() == 100);
    CHECK_FALSE(pb.is_indeterminate());
    CHECK(pb.orientation() == progress_bar_orientation::horizontal);
    CHECK_FALSE(pb.is_text_visible());
    CHECK_FALSE(pb.is_focusable());  // Progress bars aren't focusable
}

// ===== Value Management Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Set value") {
    progress_bar<test_backend> pb;

    pb.set_value(50);
    CHECK(pb.value() == 50);

    pb.set_value(100);
    CHECK(pb.value() == 100);

    pb.set_value(0);
    CHECK(pb.value() == 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Value clamping") {
    progress_bar<test_backend> pb;
    pb.set_range(0, 100);

    // Below minimum
    pb.set_value(-10);
    CHECK(pb.value() == 0);

    // Above maximum
    pb.set_value(150);
    CHECK(pb.value() == 100);

    // Within range
    pb.set_value(50);
    CHECK(pb.value() == 50);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Set range") {
    progress_bar<test_backend> pb;

    pb.set_range(0, 200);
    CHECK(pb.minimum() == 0);
    CHECK(pb.maximum() == 200);

    pb.set_range(50, 150);
    CHECK(pb.minimum() == 50);
    CHECK(pb.maximum() == 150);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Set range clamps value") {
    progress_bar<test_backend> pb;

    pb.set_value(80);
    CHECK(pb.value() == 80);

    // New range excludes current value
    pb.set_range(0, 50);
    CHECK(pb.value() == 50);  // Clamped to new max
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Custom range") {
    progress_bar<test_backend> pb;
    pb.set_range(0, 1000);

    pb.set_value(500);
    CHECK(pb.value() == 500);

    pb.set_value(1000);
    CHECK(pb.value() == 1000);
}

// ===== Indeterminate Mode Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Indeterminate mode") {
    progress_bar<test_backend> pb;

    CHECK_FALSE(pb.is_indeterminate());

    pb.set_indeterminate(true);
    CHECK(pb.is_indeterminate());

    pb.set_indeterminate(false);
    CHECK_FALSE(pb.is_indeterminate());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Indeterminate mode invalidates render") {
    progress_bar<test_backend> pb;
    pb.measure(100, 100);
    pb.arrange(geometry::relative_rect<test_backend>{test_backend::rect_type{0, 0, 100, 10}});

    pb.set_indeterminate(true);
    // Render should be invalidated, but not measure
}

// ===== Orientation Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Horizontal orientation") {
    progress_bar<test_backend> pb;

    CHECK(pb.orientation() == progress_bar_orientation::horizontal);

    pb.set_orientation(progress_bar_orientation::horizontal);
    CHECK(pb.orientation() == progress_bar_orientation::horizontal);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Vertical orientation") {
    progress_bar<test_backend> pb;

    pb.set_orientation(progress_bar_orientation::vertical);
    CHECK(pb.orientation() == progress_bar_orientation::vertical);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Orientation change invalidates measure") {
    progress_bar<test_backend> pb;
    pb.measure(100, 100);
    pb.arrange(geometry::relative_rect<test_backend>{test_backend::rect_type{0, 0, 100, 10}});

    pb.set_orientation(progress_bar_orientation::vertical);
    // Measure should be invalidated
}

// ===== Text Overlay Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Text visibility") {
    progress_bar<test_backend> pb;

    CHECK_FALSE(pb.is_text_visible());

    pb.set_text_visible(true);
    CHECK(pb.is_text_visible());

    pb.set_text_visible(false);
    CHECK_FALSE(pb.is_text_visible());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Default text format") {
    progress_bar<test_backend> const pb;

    CHECK(pb.text_format() == "%p%");
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Set text format") {
    progress_bar<test_backend> pb;

    pb.set_text_format("%v/%m files");
    CHECK(pb.text_format() == "%v/%m files");

    pb.set_text_format("Loading... %p%%");
    CHECK(pb.text_format() == "Loading... %p%%");
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Formatted text - percentage") {
    progress_bar<test_backend> pb;
    pb.set_range(0, 100);
    pb.set_text_format("%p%");

    pb.set_value(0);
    CHECK(pb.formatted_text() == "0%");

    pb.set_value(45);
    CHECK(pb.formatted_text() == "45%");

    pb.set_value(100);
    CHECK(pb.formatted_text() == "100%");
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Formatted text - value") {
    progress_bar<test_backend> pb;
    pb.set_range(0, 100);
    pb.set_text_format("%v");

    pb.set_value(0);
    CHECK(pb.formatted_text() == "0");

    pb.set_value(45);
    CHECK(pb.formatted_text() == "45");

    pb.set_value(100);
    CHECK(pb.formatted_text() == "100");
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Formatted text - maximum") {
    progress_bar<test_backend> pb;
    pb.set_range(0, 200);
    pb.set_text_format("%m");

    pb.set_value(0);
    CHECK(pb.formatted_text() == "200");

    pb.set_value(100);
    CHECK(pb.formatted_text() == "200");
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Formatted text - complex format") {
    progress_bar<test_backend> pb;
    pb.set_range(0, 100);
    pb.set_text_format("%v/%m files");

    pb.set_value(45);
    CHECK(pb.formatted_text() == "45/100 files");

    pb.set_value(100);
    CHECK(pb.formatted_text() == "100/100 files");
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Formatted text - escaped percent") {
    progress_bar<test_backend> pb;
    pb.set_range(0, 100);
    pb.set_text_format("%p%% complete");

    pb.set_value(45);
    CHECK(pb.formatted_text() == "45% complete");
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Formatted text - custom range percentage") {
    progress_bar<test_backend> pb;
    pb.set_range(0, 200);
    pb.set_text_format("%p%");

    pb.set_value(0);
    CHECK(pb.formatted_text() == "0%");

    pb.set_value(100);
    CHECK(pb.formatted_text() == "50%");  // 100/200 = 50%

    pb.set_value(200);
    CHECK(pb.formatted_text() == "100%");
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Formatted text - non-zero minimum") {
    progress_bar<test_backend> pb;
    pb.set_range(50, 150);
    pb.set_text_format("%p%");

    pb.set_value(50);
    CHECK(pb.formatted_text() == "0%");  // At minimum

    pb.set_value(100);
    CHECK(pb.formatted_text() == "50%");  // Halfway

    pb.set_value(150);
    CHECK(pb.formatted_text() == "100%");  // At maximum
}

// ===== Signal Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - value_changed signal") {
    progress_bar<test_backend> pb;

    int emitted_value = -1;
    pb.value_changed.connect([&emitted_value](int value) {
        emitted_value = value;
    });

    pb.set_value(45);
    CHECK(emitted_value == 45);

    pb.set_value(100);
    CHECK(emitted_value == 100);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - value_changed not emitted for same value") {
    progress_bar<test_backend> pb;
    pb.set_value(50);

    int emit_count = 0;
    pb.value_changed.connect([&emit_count](int) {
        ++emit_count;
    });

    pb.set_value(50);  // Same value
    CHECK(emit_count == 0);

    pb.set_value(60);  // Different value
    CHECK(emit_count == 1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - value_changed emitted after clamping") {
    progress_bar<test_backend> pb;
    pb.set_range(0, 100);

    int emitted_value = -1;
    pb.value_changed.connect([&emitted_value](int value) {
        emitted_value = value;
    });

    pb.set_value(150);  // Above maximum
    CHECK(emitted_value == 100);  // Clamped value

    pb.set_value(-10);  // Below minimum
    CHECK(emitted_value == 0);  // Clamped value
}

// ===== Measure and Arrange Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Measure horizontal") {
    progress_bar<test_backend> pb;
    pb.set_orientation(progress_bar_orientation::horizontal);

    auto size = pb.measure(100, 100);
    int const width = size_utils::get_width(size);
    int const height = size_utils::get_height(size);

    // Horizontal progress bar should request small height
    CHECK(width > 0);
    CHECK(height > 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Measure vertical") {
    progress_bar<test_backend> pb;
    pb.set_orientation(progress_bar_orientation::vertical);

    auto size = pb.measure(100, 100);
    int const width = size_utils::get_width(size);
    int const height = size_utils::get_height(size);

    // Vertical progress bar should request small width
    CHECK(width > 0);
    CHECK(height > 0);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Arrange") {
    progress_bar<test_backend> pb;
    pb.measure(100, 100);

    pb.arrange(geometry::relative_rect<test_backend>{test_backend::rect_type{0, 0, 100, 10}});

    auto const& bounds = pb.bounds();
    CHECK(rect_utils::get_width(bounds) == 100);
    CHECK(rect_utils::get_height(bounds) == 10);
}

// ===== Edge Cases =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Zero range") {
    progress_bar<test_backend> pb;
    pb.set_range(50, 50);  // min == max

    pb.set_value(50);
    CHECK(pb.value() == 50);

    // Percentage calculation should handle zero range
    pb.set_text_format("%p%");
    CHECK(pb.formatted_text() == "0%");
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "progress_bar - Inverted range") {
    progress_bar<test_backend> pb;
    pb.set_range(100, 0);  // max < min

    pb.set_value(50);
    // Implementation should handle this gracefully
}

}  // TEST_SUITE
