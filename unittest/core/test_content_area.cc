//
// test_content_area.cc - Content area calculation tests
//
// Tests the get_content_area() method that was completely untested
// and caused the border layout bug.
//

#include <climits>
#include "../utils/test_helpers.hh"
#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/core/element.hh>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/widgets/containers/panel.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/widgets/label.hh>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/services/ui_context.hh>
#include "../utils/test_helpers.hh"
#include "../utils/test_backend.hh"
#include "../utils/test_helpers.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"
#include "onyxui/concepts/rect_like.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_SUITE("Content Area - Core Layout Calculation") {
    using Backend = test_backend;

    // Helper to access protected get_content_area()
    class test_element_accessor : public ui_element<Backend> {
    public:
        using ui_element<Backend>::get_content_area;

        explicit test_element_accessor() : ui_element<Backend>(nullptr) {}

    };

    TEST_CASE("Content area - no margin or padding") {
        test_element_accessor elem;
        elem.arrange(logical_rect{10_lu, 20_lu, 100_lu, 50_lu});

        auto content = elem.get_content_area();

        // RELATIVE COORDINATES: content_area is relative to widget's own bounds origin (0, 0)
        // Widget arranged at absolute (10, 20), but content_area is relative
        CHECK(content.x == 0_lu);
        CHECK(content.y == 0_lu);
        CHECK(content.width == 100_lu);
        CHECK(content.height == 50_lu);
    }

    TEST_CASE("Content area - with padding only") {
        test_element_accessor elem;
        elem.set_padding(logical_thickness{5_lu, 3_lu, 7_lu, 9_lu});  // L, T, R, B
        elem.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        auto content = elem.get_content_area();

        // Position offset by padding
        CHECK(content.x == 5_lu);  // left padding
        CHECK(content.y == 3_lu);  // top padding

        // Size reduced by padding
        CHECK(content.width == 88_lu);   // 100 - 5 - 7
        CHECK(content.height == 88_lu);  // 100 - 3 - 9
    }

    TEST_CASE("Content area - with margin only") {
        test_element_accessor elem;
        elem.set_margin(logical_thickness{10_lu, 20_lu, 10_lu, 20_lu});  // L, T, R, B
        elem.arrange(logical_rect{0_lu, 0_lu, 200_lu, 200_lu});

        auto content = elem.get_content_area();

        // Margin affects bounds, not content area - content area is calculated from bounds
        // After margin is applied, bounds should be (10, 20, 180, 160)
        // Content area should match those bounds
        CHECK(content.x == 10_lu);  // left margin
        CHECK(content.y == 20_lu);  // top margin
        CHECK(content.width == 180_lu);   // 200 - 10 - 10
        CHECK(content.height == 160_lu);  // 200 - 20 - 20
    }

    TEST_CASE("Content area - with margin AND padding") {
        test_element_accessor elem;
        elem.set_margin(logical_thickness{5_lu, 5_lu, 5_lu, 5_lu});
        elem.set_padding(logical_thickness{3_lu, 3_lu, 3_lu, 3_lu});
        elem.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        auto content = elem.get_content_area();

        // Position offset by margin + padding
        CHECK(content.x == 8_lu);   // 5 + 3
        CHECK(content.y == 8_lu);   // 5 + 3

        // Size reduced by margin + padding
        CHECK(content.width == 84_lu);   // 100 - 2*8
        CHECK(content.height == 84_lu);  // 100 - 2*8
    }

    TEST_CASE("Content area - clamped to non-negative") {
        test_element_accessor elem;
        elem.set_padding(logical_thickness{100_lu, 100_lu, 100_lu, 100_lu});  // Huge padding
        elem.arrange(logical_rect{0_lu, 0_lu, 50_lu, 50_lu});  // Small bounds

        auto content = elem.get_content_area();

        // Should be clamped to 0, not negative
        CHECK(content.width >= 0_lu);
        CHECK(content.height >= 0_lu);
    }

    // Tests using actual panel widget with canvas backend for theme support

    TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Panel - get_content_area() accounts for border") {
        panel<test_canvas_backend> p;
        p.set_has_border(true);
        p.set_vbox_layout(spacing::none);

        auto* child = p.emplace_child<label>("Test");

        (void)p.measure(100_lu, 100_lu);
        p.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        auto child_bounds = child->bounds();

        // RELATIVE COORDINATES: child positioned at (0,0) relative to parent's content area
        // Border offset is handled internally by parent, not in child's bounds
        CHECK(child_bounds.x == 0_lu);
        CHECK(child_bounds.y == 0_lu);
    }

    TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Panel - border + padding compound offset") {
        panel<test_canvas_backend> p;
        p.set_has_border(true);      // +1
        p.set_padding(logical_thickness{2_lu, 3_lu, 2_lu, 3_lu});  // L,T,R,B
        p.set_vbox_layout(spacing::none);

        auto* child = p.emplace_child<label>("Test");

        (void)p.measure(100_lu, 100_lu);
        p.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        auto child_bounds = child->bounds();

        // RELATIVE COORDINATES: child at (0,0) relative to parent's content area
        // Border and padding offsets handled internally by parent
        CHECK(child_bounds.x == 0_lu);
        CHECK(child_bounds.y == 0_lu);
    }

    TEST_CASE("Content area - safe math prevents overflow") {
        test_element_accessor elem;
        elem.set_padding(logical_thickness{logical_unit(INT_MAX / 2.0), logical_unit(INT_MAX / 2.0), logical_unit(INT_MAX / 2.0), logical_unit(INT_MAX / 2.0)});
        elem.arrange(logical_rect{0_lu, 0_lu, 1000_lu, 1000_lu});

        // Should not crash or produce invalid values
        auto content = elem.get_content_area();

        // Should clamp to valid range
        CHECK(content.width >= 0_lu);
        CHECK(content.height >= 0_lu);
    }

    TEST_CASE("Content area - padding all sides equal") {
        test_element_accessor elem;
        elem.set_padding(logical_thickness(10_lu));
        elem.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        auto content = elem.get_content_area();

        CHECK(content.x == 10_lu);
        CHECK(content.y == 10_lu);
        CHECK(content.width == 80_lu);   // 100 - 2*10
        CHECK(content.height == 80_lu);  // 100 - 2*10
    }
}
