//
// test_content_area.cc - Content area calculation tests
//
// Tests the get_content_area() method that was completely untested
// and caused the border layout bug.
//

#include <climits>
#include <doctest/doctest.h>
#include <onyxui/element.hh>
#include <onyxui/widgets/panel.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/ui_context.hh>
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "onyxui/concepts/rect_like.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_SUITE("Content Area - Core Layout Calculation") {
    using Backend = test_backend;

    // Helper to access protected get_content_area()
    class test_element_accessor : public ui_element<Backend> {
    public:
        using ui_element<Backend>::get_content_area;

        explicit test_element_accessor() : ui_element<Backend>(nullptr) {}

        void do_apply_theme([[maybe_unused]] const theme_type& theme) override {}
    };

    TEST_CASE("Content area - no margin or padding") {
        test_element_accessor elem;
        elem.arrange({10, 20, 100, 50});

        auto content = elem.get_content_area();

        // Should match bounds exactly
        CHECK(rect_utils::get_x(content) == 10);
        CHECK(rect_utils::get_y(content) == 20);
        CHECK(rect_utils::get_width(content) == 100);
        CHECK(rect_utils::get_height(content) == 50);
    }

    TEST_CASE("Content area - with padding only") {
        test_element_accessor elem;
        elem.set_padding({5, 3, 7, 9});  // L, T, R, B
        elem.arrange({0, 0, 100, 100});

        auto content = elem.get_content_area();

        // Position offset by padding
        CHECK(rect_utils::get_x(content) == 5);  // left padding
        CHECK(rect_utils::get_y(content) == 3);  // top padding

        // Size reduced by padding
        CHECK(rect_utils::get_width(content) == 88);   // 100 - 5 - 7
        CHECK(rect_utils::get_height(content) == 88);  // 100 - 3 - 9
    }

    TEST_CASE("Content area - with margin only") {
        test_element_accessor elem;
        elem.set_margin({10, 20, 10, 20});  // L, T, R, B
        elem.arrange({0, 0, 200, 200});

        auto content = elem.get_content_area();

        // Margin affects bounds, not content area - content area is calculated from bounds
        // After margin is applied, bounds should be (10, 20, 180, 160)
        // Content area should match those bounds
        CHECK(rect_utils::get_x(content) == 10);  // left margin
        CHECK(rect_utils::get_y(content) == 20);  // top margin
        CHECK(rect_utils::get_width(content) == 180);   // 200 - 10 - 10
        CHECK(rect_utils::get_height(content) == 160);  // 200 - 20 - 20
    }

    TEST_CASE("Content area - with margin AND padding") {
        test_element_accessor elem;
        elem.set_margin({5, 5, 5, 5});
        elem.set_padding({3, 3, 3, 3});
        elem.arrange({0, 0, 100, 100});

        auto content = elem.get_content_area();

        // Position offset by margin + padding
        CHECK(rect_utils::get_x(content) == 8);   // 5 + 3
        CHECK(rect_utils::get_y(content) == 8);   // 5 + 3

        // Size reduced by margin + padding
        CHECK(rect_utils::get_width(content) == 84);   // 100 - 2*8
        CHECK(rect_utils::get_height(content) == 84);  // 100 - 2*8
    }

    TEST_CASE("Content area - clamped to non-negative") {
        test_element_accessor elem;
        elem.set_padding({100, 100, 100, 100});  // Huge padding
        elem.arrange({0, 0, 50, 50});  // Small bounds

        auto content = elem.get_content_area();

        // Should be clamped to 0, not negative
        CHECK(rect_utils::get_width(content) >= 0);
        CHECK(rect_utils::get_height(content) >= 0);
    }

    // Tests using actual panel widget with canvas backend for theme support
    struct test_fixture {
        scoped_ui_context<test_canvas_backend> ctx;

        template<typename Widget>
        void apply_default_theme(Widget& w) {
            if (auto* theme = ctx.themes().get_theme("Test Theme")) {
                w.apply_theme(*theme);
            }
        }
    };

    TEST_CASE_FIXTURE(test_fixture, "Panel - get_content_area() accounts for border") {
        panel<test_canvas_backend> p;
        apply_default_theme(p);
        p.set_has_border(true);
        p.set_vbox_layout(0);

        auto* child = p.emplace_child<label>("Test");

        (void)p.measure(100, 100);
        p.arrange({0, 0, 100, 100});

        auto child_bounds = child->bounds();

        // Child should be inset by border
        CHECK(rect_utils::get_x(child_bounds) == 1);
        CHECK(rect_utils::get_y(child_bounds) == 1);
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel - border + padding compound offset") {
        panel<test_canvas_backend> p;
        apply_default_theme(p);
        p.set_has_border(true);      // +1
        p.set_padding({2, 3, 2, 3});  // L,T,R,B
        p.set_vbox_layout(0);

        auto* child = p.emplace_child<label>("Test");

        (void)p.measure(100, 100);
        p.arrange({0, 0, 100, 100});

        auto child_bounds = child->bounds();

        // Content area should account for border THEN padding
        CHECK(rect_utils::get_x(child_bounds) == 3);  // border(1) + left_pad(2)
        CHECK(rect_utils::get_y(child_bounds) == 4);  // border(1) + top_pad(3)
    }

    TEST_CASE("Content area - safe math prevents overflow") {
        test_element_accessor elem;
        elem.set_padding({INT_MAX / 2, INT_MAX / 2, INT_MAX / 2, INT_MAX / 2});
        elem.arrange({0, 0, 1000, 1000});

        // Should not crash or produce invalid values
        auto content = elem.get_content_area();

        // Should clamp to valid range
        CHECK(rect_utils::get_width(content) >= 0);
        CHECK(rect_utils::get_height(content) >= 0);
    }

    TEST_CASE("Content area - padding all sides equal") {
        test_element_accessor elem;
        elem.set_padding(thickness::all(10));
        elem.arrange({0, 0, 100, 100});

        auto content = elem.get_content_area();

        CHECK(rect_utils::get_x(content) == 10);
        CHECK(rect_utils::get_y(content) == 10);
        CHECK(rect_utils::get_width(content) == 80);   // 100 - 2*10
        CHECK(rect_utils::get_height(content) == 80);  // 100 - 2*10
    }
}
