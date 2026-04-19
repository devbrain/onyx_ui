/// @file test_window_drag.cc
/// @brief Tests that title-bar dragging moves the window, and that
/// `clamp_drag_bounds` keeps the title bar reachable.
///
/// Windows are overlay-only — they live in a `layer_manager`, not inside a
/// widget tree. These tests therefore exercise the window standalone
/// (constructed, arranged at specific bounds, title-bar events simulated).

#include <doctest/doctest.h>

#include "../utils/test_backend.hh"
#include "../utils/test_helpers.hh"

#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/containers/panel.hh>

using namespace onyxui;
using namespace onyxui::testing;
using Backend = test_backend;

namespace {
    // Exposes the protected handle_event override so tests can drive it.
    struct test_title_bar : window_title_bar<Backend> {
        using window_title_bar<Backend>::window_title_bar;
        using window_title_bar<Backend>::handle_event;
    };

    void press_capture(window_title_bar<Backend>& tb, double x, double y) {
        mouse_event evt{
            .x = logical_unit(x), .y = logical_unit(y),
            .btn = mouse_event::button::left,
            .act = mouse_event::action::press,
            .modifiers = {}
        };
        static_cast<test_title_bar&>(tb).handle_event(ui_event{evt}, event_phase::capture);
    }

    void move_target(window_title_bar<Backend>& tb, double x, double y) {
        mouse_event evt{
            .x = logical_unit(x), .y = logical_unit(y),
            .btn = mouse_event::button::none,
            .act = mouse_event::action::move,
            .modifiers = {}
        };
        static_cast<test_title_bar&>(tb).handle_event(ui_event{evt}, event_phase::target);
    }

    void release_target(window_title_bar<Backend>& tb, double x, double y) {
        mouse_event evt{
            .x = logical_unit(x), .y = logical_unit(y),
            .btn = mouse_event::button::left,
            .act = mouse_event::action::release,
            .modifiers = {}
        };
        static_cast<test_title_bar&>(tb).handle_event(ui_event{evt}, event_phase::target);
    }

    window_title_bar<Backend>* find_title_bar(window<Backend>& win) {
        for (auto& child : win.children()) {
            if (auto* tb = dynamic_cast<window_title_bar<Backend>*>(child.get())) {
                return tb;
            }
        }
        return nullptr;
    }

    // Build a window, arrange it at the given initial bounds, return both
    // the window and the title bar for event simulation.
    std::unique_ptr<window<Backend>> make_window_at(
        logical_unit x, logical_unit y,
        logical_unit w, logical_unit h) {
        typename window<Backend>::window_flags flags;
        flags.is_movable = true;
        flags.is_resizable = false;
        flags.has_close_button = false;
        flags.has_minimize_button = false;
        flags.has_maximize_button = false;
        auto win = std::make_unique<window<Backend>>("Test", flags);
        win->set_width_constraint({size_policy::fixed, w});
        win->set_height_constraint({size_policy::fixed, h});
        (void)win->measure(w, h);
        win->arrange(logical_rect{x, y, w, h});
        return win;
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "Window drag moves bounds by the drag delta") {
    auto win = make_window_at(150_lu, 110_lu, 100_lu, 80_lu);
    auto* tb = find_title_bar(*win);
    REQUIRE(tb != nullptr);

    const auto initial = win->bounds();
    const double start_x = initial.x.value + 20.0;
    const double start_y = initial.y.value + 2.0;

    press_capture(*tb, start_x, start_y);
    move_target(*tb, start_x + 50.0, start_y + 30.0);
    release_target(*tb, start_x + 50.0, start_y + 30.0);

    const auto after = win->bounds();
    CHECK(after.x.value == doctest::Approx(initial.x.value + 50.0));
    CHECK(after.y.value == doctest::Approx(initial.y.value + 30.0));
}

TEST_CASE_FIXTURE(ui_context_fixture<Backend>,
                   "Window drag keeps title bar reachable (clamping)") {
    // Give the window an explicit 400x300 workspace so the clamp has a
    // deterministic reference rect; tests don't need to rely on the
    // backend's viewport state.
    auto workspace = std::make_unique<panel<Backend>>();
    (void)workspace->measure(400_lu, 300_lu);
    workspace->arrange(logical_rect{0_lu, 0_lu, 400_lu, 300_lu});

    auto win = make_window_at(150_lu, 110_lu, 100_lu, 80_lu);
    win->set_workspace(workspace.get());
    auto* tb = find_title_bar(*win);
    REQUIRE(tb != nullptr);
    const auto initial = win->bounds();
    const double start_x = initial.x.value + 20.0;
    const double start_y = initial.y.value + 2.0;

    SUBCASE("Drag far right: at least a grip remains visible from the left") {
        press_capture(*tb, start_x, start_y);
        move_target(*tb, start_x + 10000.0, start_y);
        release_target(*tb, start_x + 10000.0, start_y);

        // The clamp keeps the window's right edge at least min_grip (64 lu)
        // inside the container; equivalently, some overlap remains.
        CHECK(win->bounds().x.value + win->bounds().width.value >= 64.0);
    }

    SUBCASE("Drag far left: at least a grip remains visible from the right") {
        press_capture(*tb, start_x, start_y);
        move_target(*tb, start_x - 10000.0, start_y);
        release_target(*tb, start_x - 10000.0, start_y);

        CHECK(win->bounds().x.value + win->bounds().width.value >= 64.0);
    }

    SUBCASE("Drag up: title bar's top stays >= viewport.top") {
        press_capture(*tb, start_x, start_y);
        move_target(*tb, start_x, start_y - 10000.0);
        release_target(*tb, start_x, start_y - 10000.0);

        CHECK(win->bounds().y.value >= 0.0 - 0.5);
    }
}
