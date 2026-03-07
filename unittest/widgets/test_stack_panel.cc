#include <doctest/doctest.h>

#include <onyxui/widgets/containers/stack_panel.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/button.hh>
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "StackPanel - Single-visible-child container") {

    SUBCASE("Empty state") {
        stack_panel<test_canvas_backend> sp;
        CHECK(sp.count() == 0);
        CHECK(sp.current_index() == -1);
        CHECK(sp.current_widget() == nullptr);
    }

    SUBCASE("Add first page becomes current") {
        stack_panel<test_canvas_backend> sp;
        auto lbl = std::make_unique<label<test_canvas_backend>>("Page 1");
        auto* ptr = lbl.get();
        sp.add_page(std::move(lbl));

        CHECK(sp.count() == 1);
        CHECK(sp.current_index() == 0);
        CHECK(sp.current_widget() == ptr);
        CHECK(ptr->is_visible());
    }

    SUBCASE("Second page is hidden") {
        stack_panel<test_canvas_backend> sp;
        sp.add_page(std::make_unique<label<test_canvas_backend>>("Page 1"));
        auto lbl2 = std::make_unique<label<test_canvas_backend>>("Page 2");
        auto* ptr2 = lbl2.get();
        sp.add_page(std::move(lbl2));

        CHECK(sp.count() == 2);
        CHECK(sp.current_index() == 0);
        CHECK_FALSE(ptr2->is_visible());
    }

    SUBCASE("Switch index shows correct page") {
        stack_panel<test_canvas_backend> sp;
        auto* p0 = sp.emplace_page<label>("A");
        auto* p1 = sp.emplace_page<label>("B");

        sp.set_current_index(1);

        CHECK(sp.current_index() == 1);
        CHECK(sp.current_widget() == p1);
        CHECK_FALSE(p0->is_visible());
        CHECK(p1->is_visible());
    }

    SUBCASE("Same index is no-op - no signal") {
        stack_panel<test_canvas_backend> sp;
        sp.emplace_page<label>("A");
        int signal_count = 0;
        sp.current_changed.connect([&](int) { signal_count++; });

        sp.set_current_index(0);
        CHECK(signal_count == 0);
    }

    SUBCASE("Signal emitted on change") {
        stack_panel<test_canvas_backend> sp;
        sp.emplace_page<label>("A");
        sp.emplace_page<label>("B");

        int received = -1;
        sp.current_changed.connect([&](int idx) { received = idx; });

        sp.set_current_index(1);
        CHECK(received == 1);
    }

    SUBCASE("Out-of-range index clamped") {
        stack_panel<test_canvas_backend> sp;
        sp.emplace_page<label>("A");
        sp.emplace_page<label>("B");
        sp.emplace_page<label>("C");

        sp.set_current_index(99);
        CHECK(sp.current_index() == 2);

        sp.set_current_index(-5);
        CHECK(sp.current_index() == 0);
    }

    SUBCASE("set_current_index on empty is no-op") {
        stack_panel<test_canvas_backend> sp;
        sp.set_current_index(0);
        CHECK(sp.current_index() == -1);
    }

    SUBCASE("Remove current page adjusts index") {
        stack_panel<test_canvas_backend> sp;
        sp.emplace_page<label>("A");
        auto* b = sp.emplace_page<label>("B");
        sp.emplace_page<label>("C");

        sp.set_current_index(2);
        sp.remove_page(2);

        CHECK(sp.count() == 2);
        CHECK(sp.current_index() == 1);
        CHECK(sp.current_widget() == b);
    }

    SUBCASE("Remove all pages returns to empty state") {
        stack_panel<test_canvas_backend> sp;
        sp.emplace_page<label>("A");
        sp.emplace_page<label>("B");

        sp.remove_page(1);
        sp.remove_page(0);

        CHECK(sp.count() == 0);
        CHECK(sp.current_index() == -1);
        CHECK(sp.current_widget() == nullptr);
    }

    SUBCASE("Remove page before current adjusts index") {
        stack_panel<test_canvas_backend> sp;
        sp.emplace_page<label>("A");
        sp.emplace_page<label>("B");
        auto* c = sp.emplace_page<label>("C");

        sp.set_current_index(2);
        sp.remove_page(0);

        CHECK(sp.current_index() == 1);
        CHECK(sp.current_widget() == c);
    }

    SUBCASE("Measure returns current child size") {
        stack_panel<test_canvas_backend> sp;
        auto* small_lbl = sp.emplace_page<label>("Hi");
        auto* big_lbl = sp.emplace_page<label>("A much longer label text");

        auto size0 = sp.measure(logical_unit(200), logical_unit(200));
        auto small_size = small_lbl->measure(logical_unit(200), logical_unit(200));
        CHECK(size0.width == small_size.width);

        sp.set_current_index(1);
        auto size1 = sp.measure(logical_unit(200), logical_unit(200));
        auto big_size = big_lbl->measure(logical_unit(200), logical_unit(200));
        CHECK(size1.width == big_size.width);
    }

    SUBCASE("emplace_page returns correct pointer") {
        stack_panel<test_canvas_backend> sp;
        auto* btn = sp.emplace_page<button>("Click");
        CHECK(btn != nullptr);
        CHECK(sp.current_widget() == btn);
    }

    SUBCASE("Remove out-of-range is no-op") {
        stack_panel<test_canvas_backend> sp;
        sp.emplace_page<label>("A");

        sp.remove_page(-1);
        sp.remove_page(5);
        CHECK(sp.count() == 1);
    }
}
