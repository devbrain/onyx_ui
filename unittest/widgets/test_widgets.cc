/**
 * @file test_widgets.cc
 * @brief Tests for basic widgets
 * @author igor
 * @date 16/10/2025
 */

#include <doctest/doctest.h>
#include "../utils/test_backend.hh"
#include "../../include/onyxui/core/signal.hh"
#include "widgets.hh"

using namespace onyxui;




TEST_CASE("Widget - Base widget functionality") {
    SUBCASE("Construction and enabled state") {
        test_widget<test_backend> const w;

        CHECK(w.is_enabled());
        CHECK_FALSE(w.is_hovered());
        CHECK_FALSE(w.is_pressed());
    }

    SUBCASE("Enable/disable signals") {
        test_widget<test_backend> w;
        bool signal_received = false;
        bool new_state = false;

        w.enabled_changed.connect([&](bool enabled) {
            signal_received = true;
            new_state = enabled;
        });

        w.set_enabled(false);
        CHECK(signal_received);
        CHECK_FALSE(new_state);
        CHECK_FALSE(w.is_enabled());
    }

    SUBCASE("Click signal") {
        test_widget<test_backend> w;
        int click_count = 0;

        w.clicked.connect([&]() {
            click_count++;
        });

        // Simulate click
        w.simulate_click();

        CHECK(click_count == 1);
    }

    SUBCASE("Mouse enter/exit signals") {
        test_widget<test_backend> w;
        // Set bounds for hit testing
        w.arrange(logical_rect{0_lu, 0_lu, 100_lu, 50_lu});

        int enter_count = 0;
        int exit_count = 0;

        w.mouse_entered.connect([&]() { enter_count++; });
        w.mouse_exited.connect([&]() { exit_count++; });

        // Trigger mouse enter by moving inside bounds
        mouse_event enter{.x = 50, .y = 25, .btn = mouse_event::button::none, .act = mouse_event::action::move, .modifiers = {}};
        w.handle_event(ui_event{enter}, event_phase::target);
        CHECK(enter_count == 1);

        // Trigger mouse exit by moving outside bounds
        mouse_event exit{.x = 200, .y = 25, .btn = mouse_event::button::none, .act = mouse_event::action::move, .modifiers = {}};
        w.handle_event(ui_event{exit}, event_phase::target);
        CHECK(exit_count == 1);
    }

    SUBCASE("Focus signals") {
        test_widget<test_backend> w;
        w.set_focusable(true);

        int gained_count = 0;
        int lost_count = 0;

        w.focus_gained.connect([&]() { gained_count++; });
        w.focus_lost.connect([&]() { lost_count++; });

        // Set focus triggers focus_gained signal via on_focus_changed(true)
        w.test_set_focus(true);
        CHECK(gained_count == 1);

        // Remove focus triggers focus_lost signal via on_focus_changed(false)
        w.test_set_focus(false);
        CHECK(lost_count == 1);
    }

    SUBCASE("Disabled widget doesn't emit click") {
        test_widget<test_backend> w;
        int click_count = 0;

        w.clicked.connect([&]() { click_count++; });

        w.set_enabled(false);

        // Simulate click on disabled widget
        w.simulate_click();

        // With unified event API, handle_mouse() checks is_enabled() internally
        // so disabled widgets properly don't emit clicks even when handlers are called directly
        CHECK(click_count == 0);
    }
}

TEST_CASE("Widgets - Signal composition") {
    SUBCASE("Connect multiple slots to one signal") {
        test_button<test_backend> btn("Test");

        int handler1_count = 0;
        int handler2_count = 0;
        int handler3_count = 0;

        btn.clicked.connect([&]() { handler1_count++; });
        btn.clicked.connect([&]() { handler2_count++; });
        btn.clicked.connect([&]() { handler3_count++; });

        // Trigger click
        btn.simulate_click();

        CHECK(handler1_count == 1);
        CHECK(handler2_count == 1);
        CHECK(handler3_count == 1);
    }

    SUBCASE("Scoped connection lifecycle") {
        test_button<test_backend> btn("Test");
        int click_count = 0;

        {
            scoped_connection const conn(btn.clicked, [&]() { click_count++; });

            btn.simulate_click();

            CHECK(click_count == 1);
        }  // conn destroyed, connection auto-disconnected

        // Click again - should not increment
        btn.simulate_click();

        CHECK(click_count == 1);  // Still 1
    }
}






