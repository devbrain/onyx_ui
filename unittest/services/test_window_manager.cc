/**
 * @file test_window_manager.cc
 * @brief Comprehensive tests for window_manager service
 * @author Claude Code
 * @date 2025-11-08
 */

#include <doctest/doctest.h>
#include <onyxui/widgets/window/window.hh>
#include <onyxui/services/ui_context.hh>
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;
using namespace onyxui::testing;

// ============================================================================
// Window Registration/Unregistration
// ============================================================================

TEST_CASE("window_manager - Window registration") {
    scoped_ui_context<test_canvas_backend> ctx{make_terminal_metrics<test_canvas_backend>()};
    auto* mgr = ui_services<test_canvas_backend>::windows();
    REQUIRE(mgr != nullptr);

    SUBCASE("New window auto-registers") {
        CHECK(mgr->get_window_count() == 0);
        CHECK(mgr->get_all_windows().empty());

        {
            window<test_canvas_backend> win("Test Window");

            CHECK(mgr->get_window_count() == 1);
            auto all = mgr->get_all_windows();
            REQUIRE(all.size() == 1);
            CHECK(all[0] == &win);
        }

        // Window auto-unregisters on destruction
        CHECK(mgr->get_window_count() == 0);
        CHECK(mgr->get_all_windows().empty());
    }

    SUBCASE("Multiple windows register correctly") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");
        window<test_canvas_backend> win3("Window 3");

        CHECK(mgr->get_window_count() == 3);
        auto all = mgr->get_all_windows();
        REQUIRE(all.size() == 3);

        // Verify all windows are registered
        bool found1 = false, found2 = false, found3 = false;
        for (auto* w : all) {
            if (w == &win1) found1 = true;
            if (w == &win2) found2 = true;
            if (w == &win3) found3 = true;
        }
        CHECK(found1);
        CHECK(found2);
        CHECK(found3);
    }

    SUBCASE("Duplicate registration is idempotent") {
        window<test_canvas_backend> win("Test");
        CHECK(mgr->get_window_count() == 1);

        // Manually try to re-register (shouldn't duplicate)
        mgr->register_window(&win);
        CHECK(mgr->get_window_count() == 1);
    }

    SUBCASE("Null window registration is ignored") {
        CHECK(mgr->get_window_count() == 0);

        mgr->register_window(nullptr);

        CHECK(mgr->get_window_count() == 0);
    }
}

TEST_CASE("window_manager - Window unregistration") {
    scoped_ui_context<test_canvas_backend> ctx{make_terminal_metrics<test_canvas_backend>()};
    auto* mgr = ui_services<test_canvas_backend>::windows();
    REQUIRE(mgr != nullptr);

    SUBCASE("Window unregisters on destruction") {
        CHECK(mgr->get_window_count() == 0);

        {
            window<test_canvas_backend> win("Test");
            CHECK(mgr->get_window_count() == 1);
        }

        CHECK(mgr->get_window_count() == 0);
    }

    SUBCASE("Unregistering non-existent window is safe") {
        window<test_canvas_backend> win("Test");
        CHECK(mgr->get_window_count() == 1);

        mgr->unregister_window(&win);
        CHECK(mgr->get_window_count() == 0);

        // Try again - should be safe
        mgr->unregister_window(&win);
        CHECK(mgr->get_window_count() == 0);
    }

    SUBCASE("Null window unregistration is ignored") {
        window<test_canvas_backend> win("Test");
        CHECK(mgr->get_window_count() == 1);

        mgr->unregister_window(nullptr);

        CHECK(mgr->get_window_count() == 1);
    }

    SUBCASE("Partial destruction unregisters correctly") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");
        window<test_canvas_backend> win3("Window 3");

        CHECK(mgr->get_window_count() == 3);

        {
            window<test_canvas_backend> win_temp("Temp");
            CHECK(mgr->get_window_count() == 4);
        }

        // win_temp destroyed, others remain
        CHECK(mgr->get_window_count() == 3);

        auto all = mgr->get_all_windows();
        REQUIRE(all.size() == 3);

        // Verify correct windows remain
        bool found1 = false, found2 = false, found3 = false;
        for (auto* w : all) {
            if (w == &win1) found1 = true;
            if (w == &win2) found2 = true;
            if (w == &win3) found3 = true;
        }
        CHECK(found1);
        CHECK(found2);
        CHECK(found3);
    }
}

// ============================================================================
// Window Enumeration
// ============================================================================

TEST_CASE("window_manager - Get all windows") {
    scoped_ui_context<test_canvas_backend> ctx{make_terminal_metrics<test_canvas_backend>()};
    auto* mgr = ui_services<test_canvas_backend>::windows();
    REQUIRE(mgr != nullptr);

    SUBCASE("Empty manager returns empty list") {
        auto all = mgr->get_all_windows();
        CHECK(all.empty());
        CHECK(mgr->get_window_count() == 0);
    }

    SUBCASE("Returns all registered windows") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");
        window<test_canvas_backend> win3("Window 3");

        auto all = mgr->get_all_windows();
        CHECK(all.size() == 3);
        CHECK(mgr->get_window_count() == 3);
    }

    SUBCASE("Returns copy, not reference") {
        window<test_canvas_backend> win("Test");

        auto all1 = mgr->get_all_windows();
        REQUIRE(all1.size() == 1);

        {
            window<test_canvas_backend> win2("Test 2");
            auto all2 = mgr->get_all_windows();
            REQUIRE(all2.size() == 2);

            // Original copy unchanged
            CHECK(all1.size() == 1);
        }
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window_manager - Get visible windows") {
    auto* mgr = ui_services<test_canvas_backend>::windows();
    REQUIRE(mgr != nullptr);

    SUBCASE("Empty manager returns empty list") {
        auto visible = mgr->get_visible_windows();
        CHECK(visible.empty());
    }

    SUBCASE("All visible windows are returned") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");
        window<test_canvas_backend> win3("Window 3");

        win1.show();
        win2.show();
        win3.show();

        auto visible = mgr->get_visible_windows();
        CHECK(visible.size() == 3);
    }

    SUBCASE("Hidden windows are excluded") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");
        window<test_canvas_backend> win3("Window 3");

        win1.show();
        win2.show();
        win3.hide();

        auto visible = mgr->get_visible_windows();
        CHECK(visible.size() == 2);

        bool found1 = false, found2 = false, found3 = false;
        for (auto* w : visible) {
            if (w == &win1) found1 = true;
            if (w == &win2) found2 = true;
            if (w == &win3) found3 = true;
        }
        CHECK(found1);
        CHECK(found2);
        CHECK_FALSE(found3);
    }

    SUBCASE("Minimized windows are excluded") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");

        win1.show();
        win2.show();
        win2.minimize();

        auto visible = mgr->get_visible_windows();
        CHECK(visible.size() == 1);
        REQUIRE(!visible.empty());
        CHECK(visible[0] == &win1);
    }

    SUBCASE("Mix of states filtered correctly") {
        window<test_canvas_backend> win1("Visible 1");
        window<test_canvas_backend> win2("Visible 2");
        window<test_canvas_backend> win3("Hidden");
        window<test_canvas_backend> win4("Minimized");
        window<test_canvas_backend> win5("Maximized");

        win1.show();
        win2.show();
        win3.hide();
        win4.show();
        win4.minimize();
        win5.show();
        win5.maximize();

        auto visible = mgr->get_visible_windows();
        CHECK(visible.size() == 3);  // win1, win2, win5

        bool found1 = false, found2 = false, found5 = false;
        for (auto* w : visible) {
            if (w == &win1) found1 = true;
            if (w == &win2) found2 = true;
            if (w == &win5) found5 = true;
        }
        CHECK(found1);
        CHECK(found2);
        CHECK(found5);
    }
}

TEST_CASE("window_manager - Get minimized windows") {
    scoped_ui_context<test_canvas_backend> ctx{make_terminal_metrics<test_canvas_backend>()};
    auto* mgr = ui_services<test_canvas_backend>::windows();
    REQUIRE(mgr != nullptr);

    SUBCASE("Empty manager returns empty list") {
        auto minimized = mgr->get_minimized_windows();
        CHECK(minimized.empty());
    }

    SUBCASE("Only minimized windows are returned") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");
        window<test_canvas_backend> win3("Window 3");

        win1.show();
        win2.show();
        win2.minimize();
        win3.show();

        auto minimized = mgr->get_minimized_windows();
        CHECK(minimized.size() == 1);
        REQUIRE(!minimized.empty());
        CHECK(minimized[0] == &win2);
    }

    SUBCASE("Multiple minimized windows") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");
        window<test_canvas_backend> win3("Window 3");

        win1.show();
        win1.minimize();
        win2.show();
        win2.minimize();
        win3.show();

        auto minimized = mgr->get_minimized_windows();
        CHECK(minimized.size() == 2);

        bool found1 = false, found2 = false;
        for (auto* w : minimized) {
            if (w == &win1) found1 = true;
            if (w == &win2) found2 = true;
        }
        CHECK(found1);
        CHECK(found2);
    }

    SUBCASE("Restored window is removed from minimized list") {
        window<test_canvas_backend> win("Test");
        win.show();
        win.minimize();

        auto minimized1 = mgr->get_minimized_windows();
        CHECK(minimized1.size() == 1);

        win.restore();

        auto minimized2 = mgr->get_minimized_windows();
        CHECK(minimized2.empty());
    }
}

TEST_CASE("window_manager - Get modal windows") {
    scoped_ui_context<test_canvas_backend> ctx{make_terminal_metrics<test_canvas_backend>()};
    auto* mgr = ui_services<test_canvas_backend>::windows();
    REQUIRE(mgr != nullptr);

    SUBCASE("Empty manager returns empty list") {
        auto modal = mgr->get_modal_windows();
        CHECK(modal.empty());
    }

    SUBCASE("Placeholder returns empty (Phase 5 TODO)") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");

        // Phase 5: Modal support not yet implemented
        auto modal = mgr->get_modal_windows();
        CHECK(modal.empty());
    }
}

// ============================================================================
// Signal Emission
// ============================================================================

TEST_CASE("window_manager - Signals - window_registered") {
    scoped_ui_context<test_canvas_backend> ctx{make_terminal_metrics<test_canvas_backend>()};
    auto* mgr = ui_services<test_canvas_backend>::windows();
    REQUIRE(mgr != nullptr);

    SUBCASE("Signal emitted on registration") {
        window<test_canvas_backend>* registered_win = nullptr;
        int emit_count = 0;

        mgr->window_registered.connect([&](window<test_canvas_backend>* win) {
            registered_win = win;
            emit_count++;
        });

        window<test_canvas_backend> win("Test");

        CHECK(emit_count == 1);
        CHECK(registered_win == &win);
    }

    SUBCASE("Signal emitted for each window") {
        int emit_count = 0;
        std::vector<window<test_canvas_backend>*> registered;

        mgr->window_registered.connect([&](window<test_canvas_backend>* win) {
            registered.push_back(win);
            emit_count++;
        });

        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");
        window<test_canvas_backend> win3("Window 3");

        CHECK(emit_count == 3);
        CHECK(registered.size() == 3);
    }

    SUBCASE("Duplicate registration doesn't re-emit") {
        int emit_count = 0;

        mgr->window_registered.connect([&](window<test_canvas_backend>*) {
            emit_count++;
        });

        window<test_canvas_backend> win("Test");
        CHECK(emit_count == 1);

        mgr->register_window(&win);
        CHECK(emit_count == 1);  // No second emission
    }
}

TEST_CASE("window_manager - Signals - window_unregistered") {
    scoped_ui_context<test_canvas_backend> ctx{make_terminal_metrics<test_canvas_backend>()};
    auto* mgr = ui_services<test_canvas_backend>::windows();
    REQUIRE(mgr != nullptr);

    SUBCASE("Signal emitted on unregistration") {
        window<test_canvas_backend>* unregistered_win = nullptr;
        int emit_count = 0;

        mgr->window_unregistered.connect([&](window<test_canvas_backend>* win) {
            unregistered_win = win;
            emit_count++;
        });

        {
            window<test_canvas_backend> win("Test");
            CHECK(emit_count == 0);
        }

        CHECK(emit_count == 1);
        CHECK(unregistered_win != nullptr);
    }

    SUBCASE("Signal emitted for each window destruction") {
        int emit_count = 0;

        mgr->window_unregistered.connect([&](window<test_canvas_backend>*) {
            emit_count++;
        });

        {
            window<test_canvas_backend> win1("Window 1");
            window<test_canvas_backend> win2("Window 2");
            window<test_canvas_backend> win3("Window 3");
            CHECK(emit_count == 0);
        }

        CHECK(emit_count == 3);
    }
}

TEST_CASE("window_manager - Signals - window_minimized") {
    scoped_ui_context<test_canvas_backend> ctx{make_terminal_metrics<test_canvas_backend>()};
    auto* mgr = ui_services<test_canvas_backend>::windows();
    REQUIRE(mgr != nullptr);

    SUBCASE("Signal emitted on minimize") {
        window<test_canvas_backend>* minimized_win = nullptr;
        int emit_count = 0;

        mgr->window_minimized.connect([&](window<test_canvas_backend>* win) {
            minimized_win = win;
            emit_count++;
        });

        window<test_canvas_backend> win("Test");
        win.show();
        CHECK(emit_count == 0);

        win.minimize();

        CHECK(emit_count == 1);
        CHECK(minimized_win == &win);
    }

    SUBCASE("Signal not emitted if already minimized") {
        int emit_count = 0;

        mgr->window_minimized.connect([&](window<test_canvas_backend>*) {
            emit_count++;
        });

        window<test_canvas_backend> win("Test");
        win.show();
        win.minimize();
        CHECK(emit_count == 1);

        win.minimize();
        CHECK(emit_count == 1);  // No second emission
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window_manager - Signals - window_restored") {
    auto* mgr = ui_services<test_canvas_backend>::windows();
    REQUIRE(mgr != nullptr);

    SUBCASE("Signal emitted on restore from minimized") {
        window<test_canvas_backend>* restored_win = nullptr;
        int emit_count = 0;

        mgr->window_restored.connect([&](window<test_canvas_backend>* win) {
            restored_win = win;
            emit_count++;
        });

        window<test_canvas_backend> win("Test");
        win.show();
        win.minimize();
        CHECK(emit_count == 0);

        win.restore();

        CHECK(emit_count == 1);
        CHECK(restored_win == &win);
    }

    SUBCASE("Signal emitted on restore from maximized") {
        window<test_canvas_backend>* restored_win = nullptr;
        int emit_count = 0;

        mgr->window_restored.connect([&](window<test_canvas_backend>* win) {
            restored_win = win;
            emit_count++;
        });

        window<test_canvas_backend> win("Test");
        win.show();
        win.maximize();
        CHECK(emit_count == 0);

        win.restore();

        CHECK(emit_count == 1);
        CHECK(restored_win == &win);
    }

    SUBCASE("Signal not emitted if already in normal state") {
        int emit_count = 0;

        mgr->window_restored.connect([&](window<test_canvas_backend>*) {
            emit_count++;
        });

        window<test_canvas_backend> win("Test");
        win.show();
        CHECK(emit_count == 0);

        win.restore();
        CHECK(emit_count == 0);  // No emission - already normal
    }
}

// ============================================================================
// Custom Minimize Handler
// ============================================================================

TEST_CASE("window_manager - Custom minimize handler") {
    scoped_ui_context<test_canvas_backend> ctx{make_terminal_metrics<test_canvas_backend>()};
    auto* mgr = ui_services<test_canvas_backend>::windows();
    REQUIRE(mgr != nullptr);

    SUBCASE("No custom handler by default") {
        CHECK_FALSE(mgr->has_custom_minimize_handler());
    }

    SUBCASE("Custom handler can be set") {
        bool handler_called = false;
        window<test_canvas_backend>* handler_win = nullptr;

        mgr->set_minimize_handler([&](window<test_canvas_backend>* win) {
            handler_called = true;
            handler_win = win;
        });

        CHECK(mgr->has_custom_minimize_handler());

        window<test_canvas_backend> win("Test");
        win.show();

        // Minimize should call custom handler
        win.minimize();

        CHECK(handler_called);
        CHECK(handler_win == &win);
    }

    SUBCASE("Custom handler can be cleared") {
        mgr->set_minimize_handler([](window<test_canvas_backend>*) {});
        CHECK(mgr->has_custom_minimize_handler());

        mgr->clear_minimize_handler();
        CHECK_FALSE(mgr->has_custom_minimize_handler());
    }

    SUBCASE("Custom handler called for each minimize") {
        int call_count = 0;

        mgr->set_minimize_handler([&](window<test_canvas_backend>*) {
            call_count++;
        });

        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");

        win1.show();
        win2.show();

        win1.minimize();
        CHECK(call_count == 1);

        win2.minimize();
        CHECK(call_count == 2);
    }

    SUBCASE("Window still enters minimized state with custom handler") {
        mgr->set_minimize_handler([](window<test_canvas_backend>*) {
            // Custom behavior (e.g., add to taskbar)
        });

        window<test_canvas_backend> win("Test");
        win.show();
        win.minimize();

        CHECK(win.get_state() == window<test_canvas_backend>::window_state::minimized);
    }
}

// ============================================================================
// Window Cycling
// ============================================================================

TEST_CASE("window_manager - Window cycling") {
    scoped_ui_context<test_canvas_backend> ctx{make_terminal_metrics<test_canvas_backend>()};
    auto* mgr = ui_services<test_canvas_backend>::windows();
    REQUIRE(mgr != nullptr);

    SUBCASE("Cycle next with no windows") {
        CHECK(mgr->get_window_count() == 0);
        CHECK(mgr->get_active_window() == nullptr);

        mgr->cycle_next_window();

        CHECK(mgr->get_active_window() == nullptr);
    }

    SUBCASE("Cycle next with single window") {
        window<test_canvas_backend> win("Window 1");
        win.show();

        CHECK(mgr->get_active_window() == nullptr);

        mgr->cycle_next_window();

        CHECK(mgr->get_active_window() == &win);

        // Cycle again - should stay on same window (wraps to self)
        mgr->cycle_next_window();
        CHECK(mgr->get_active_window() == &win);
    }

    SUBCASE("Cycle next wraps around to first") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");
        window<test_canvas_backend> win3("Window 3");

        win1.show();
        win2.show();
        win3.show();

        // No active window - should activate first
        mgr->cycle_next_window();
        CHECK(mgr->get_active_window() == &win1);

        // Cycle to second
        mgr->cycle_next_window();
        CHECK(mgr->get_active_window() == &win2);

        // Cycle to third
        mgr->cycle_next_window();
        CHECK(mgr->get_active_window() == &win3);

        // Cycle should wrap to first
        mgr->cycle_next_window();
        CHECK(mgr->get_active_window() == &win1);
    }

    SUBCASE("Cycle previous with no windows") {
        CHECK(mgr->get_window_count() == 0);
        CHECK(mgr->get_active_window() == nullptr);

        mgr->cycle_previous_window();

        CHECK(mgr->get_active_window() == nullptr);
    }

    SUBCASE("Cycle previous with single window") {
        window<test_canvas_backend> win("Window 1");
        win.show();

        mgr->cycle_previous_window();

        CHECK(mgr->get_active_window() == &win);

        // Cycle again - should stay on same window
        mgr->cycle_previous_window();
        CHECK(mgr->get_active_window() == &win);
    }

    SUBCASE("Cycle previous wraps around to last") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");
        window<test_canvas_backend> win3("Window 3");

        win1.show();
        win2.show();
        win3.show();

        // No active window - should activate last
        mgr->cycle_previous_window();
        CHECK(mgr->get_active_window() == &win3);

        // Cycle to second
        mgr->cycle_previous_window();
        CHECK(mgr->get_active_window() == &win2);

        // Cycle to first
        mgr->cycle_previous_window();
        CHECK(mgr->get_active_window() == &win1);

        // Cycle should wrap to last
        mgr->cycle_previous_window();
        CHECK(mgr->get_active_window() == &win3);
    }

    SUBCASE("Cycling skips hidden windows") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");
        window<test_canvas_backend> win3("Window 3");

        win1.show();
        win2.hide();  // Hidden
        win3.show();

        // Cycle next should skip win2
        mgr->cycle_next_window();
        CHECK(mgr->get_active_window() == &win1);

        mgr->cycle_next_window();
        CHECK(mgr->get_active_window() == &win3);  // Skipped win2

        mgr->cycle_next_window();
        CHECK(mgr->get_active_window() == &win1);  // Wrapped, still skipping win2
    }

    SUBCASE("Cycling skips minimized windows") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");
        window<test_canvas_backend> win3("Window 3");

        win1.show();
        win2.show();
        win2.minimize();  // Minimized
        win3.show();

        // Cycle should skip minimized
        mgr->cycle_next_window();
        CHECK(mgr->get_active_window() == &win1);

        mgr->cycle_next_window();
        CHECK(mgr->get_active_window() == &win3);  // Skipped win2

        mgr->cycle_next_window();
        CHECK(mgr->get_active_window() == &win1);  // Wrapped
    }

    SUBCASE("Set active window directly") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");

        CHECK(mgr->get_active_window() == nullptr);

        mgr->set_active_window(&win1);
        CHECK(mgr->get_active_window() == &win1);

        mgr->set_active_window(&win2);
        CHECK(mgr->get_active_window() == &win2);

        mgr->set_active_window(nullptr);
        CHECK(mgr->get_active_window() == nullptr);
    }

    SUBCASE("Cycling with active window not in visible list") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");
        window<test_canvas_backend> win3("Window 3");

        win1.show();
        win2.show();
        win3.show();

        // Set active to win2
        mgr->set_active_window(&win2);
        CHECK(mgr->get_active_window() == &win2);

        // Hide win2 (active window becomes invisible)
        win2.hide();

        // Cycle next should reset to first visible window
        mgr->cycle_next_window();
        CHECK(mgr->get_active_window() == &win1);

        mgr->cycle_next_window();
        CHECK(mgr->get_active_window() == &win3);  // Skips hidden win2
    }

    SUBCASE("Active window tracking survives minimize/restore") {
        window<test_canvas_backend> win("Active Window");
        win.show();

        mgr->set_active_window(&win);
        CHECK(mgr->get_active_window() == &win);

        // Active window pointer persists even when minimized
        win.minimize();
        CHECK(mgr->get_active_window() == &win);

        // But cycling should skip it (minimized not visible)
        mgr->cycle_next_window();
        // With only one window (minimized), active becomes nullptr
        CHECK(mgr->get_active_window() == nullptr);
    }
}

// ============================================================================
// Integration Scenarios
// ============================================================================

TEST_CASE("window_manager - Multi-window scenarios") {
    scoped_ui_context<test_canvas_backend> ctx{make_terminal_metrics<test_canvas_backend>()};
    auto* mgr = ui_services<test_canvas_backend>::windows();
    REQUIRE(mgr != nullptr);

    SUBCASE("Complex window lifecycle") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");

        CHECK(mgr->get_window_count() == 2);

        win1.show();
        win2.show();

        auto visible1 = mgr->get_visible_windows();
        CHECK(visible1.size() == 2);

        win1.minimize();
        auto visible2 = mgr->get_visible_windows();
        CHECK(visible2.size() == 1);

        auto minimized1 = mgr->get_minimized_windows();
        CHECK(minimized1.size() == 1);

        win2.hide();
        auto visible3 = mgr->get_visible_windows();
        CHECK(visible3.empty());

        win1.restore();
        auto visible4 = mgr->get_visible_windows();
        CHECK(visible4.size() == 1);
    }

    SUBCASE("Signal order during lifecycle") {
        std::vector<std::string> events;

        mgr->window_registered.connect([&](window<test_canvas_backend>*) {
            events.push_back("registered");
        });

        mgr->window_minimized.connect([&](window<test_canvas_backend>*) {
            events.push_back("minimized");
        });

        mgr->window_restored.connect([&](window<test_canvas_backend>*) {
            events.push_back("restored");
        });

        mgr->window_unregistered.connect([&](window<test_canvas_backend>*) {
            events.push_back("unregistered");
        });

        {
            window<test_canvas_backend> win("Test");
            win.show();
            win.minimize();
            win.restore();
        }

        REQUIRE(events.size() == 4);
        CHECK(events[0] == "registered");
        CHECK(events[1] == "minimized");
        CHECK(events[2] == "restored");
        CHECK(events[3] == "unregistered");
    }

    SUBCASE("Enumeration consistency during modifications") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");
        window<test_canvas_backend> win3("Window 3");

        win1.show();
        win2.show();
        win3.show();

        auto all1 = mgr->get_all_windows();
        auto visible1 = mgr->get_visible_windows();
        CHECK(all1.size() == 3);
        CHECK(visible1.size() == 3);

        win2.minimize();

        auto all2 = mgr->get_all_windows();
        auto visible2 = mgr->get_visible_windows();
        auto minimized2 = mgr->get_minimized_windows();

        CHECK(all2.size() == 3);
        CHECK(visible2.size() == 2);
        CHECK(minimized2.size() == 1);

        // Original snapshots unchanged
        CHECK(all1.size() == 3);
        CHECK(visible1.size() == 3);
    }
}

TEST_CASE("window_manager - Per-context isolation") {
    SUBCASE("Each context has independent window_manager") {
        scoped_ui_context<test_canvas_backend> ctx1{make_terminal_metrics<test_canvas_backend>()};
        auto* mgr1 = ui_services<test_canvas_backend>::windows();
        REQUIRE(mgr1 != nullptr);

        window<test_canvas_backend> win1("Context 1 Window");
        CHECK(mgr1->get_window_count() == 1);

        {
            scoped_ui_context<test_canvas_backend> ctx2{make_terminal_metrics<test_canvas_backend>()};
            auto* mgr2 = ui_services<test_canvas_backend>::windows();
            REQUIRE(mgr2 != nullptr);
            CHECK(mgr2 != mgr1);  // Different instances

            window<test_canvas_backend> win2("Context 2 Window");
            CHECK(mgr2->get_window_count() == 1);
            CHECK(mgr1->get_window_count() == 1);  // Isolated
        }

        // ctx2 destroyed, mgr1 still has its window
        CHECK(mgr1->get_window_count() == 1);
    }
}
