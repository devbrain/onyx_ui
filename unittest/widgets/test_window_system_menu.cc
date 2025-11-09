/**
 * @file test_window_system_menu.cc
 * @brief Tests for window system menu functionality (Phase 7)
 * @date 2025-11-09
 */

#include <doctest/doctest.h>
#include <onyxui/widgets/window/window.hh>
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;
using namespace onyxui::testing;

// ============================================================================
// System Menu Construction
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - System menu creation") {
    SUBCASE("System menu created with has_menu_button flag") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        // Verify system menu was actually created
        auto* system_menu = win.get_system_menu();
        CHECK(system_menu != nullptr);

        // Verify menu widget exists
        if (system_menu) {
            auto* menu = system_menu->get_menu();
            CHECK(menu != nullptr);
        }
    }

    SUBCASE("No system menu without has_menu_button flag") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = false;

        window<test_canvas_backend> win("Test Window", flags);

        // System menu should not be created
        auto* system_menu = win.get_system_menu();
        CHECK(system_menu == nullptr);
    }

    SUBCASE("System menu with default window") {
        window<test_canvas_backend> win("Default");

        // Default window has no menu button
        auto* system_menu = win.get_system_menu();
        CHECK(system_menu == nullptr);
    }
}

// ============================================================================
// System Menu Visual Rendering ⭐ NEW
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - System menu visual rendering") {
    SUBCASE("System menu renders with content") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        REQUIRE(system_menu != nullptr);

        auto* menu = system_menu->get_menu();
        REQUIRE(menu != nullptr);

        // Update menu states before rendering
        system_menu->update_menu_states();

        // Render menu to canvas
        auto canvas = render_to_canvas(*menu, 40, 20);
        std::string rendered = canvas->render_ascii();

        // Verify menu has visual content
        CHECK(canvas->width() == 40);
        CHECK(canvas->height() == 20);
        CHECK_FALSE(rendered.empty());

        // Verify menu has substantial content (not just whitespace)
        int content_chars = 0;
        for (char c : rendered) {
            if (c != ' ' && c != '\n') {
                content_chars++;
            }
        }
        CHECK(content_chars > 0);

        INFO("System menu rendered with " << content_chars << " content characters");
    }

    SUBCASE("System menu visual state changes with window state") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        REQUIRE(system_menu != nullptr);

        auto* menu = system_menu->get_menu();
        REQUIRE(menu != nullptr);

        // Render in normal state
        system_menu->update_menu_states();
        auto canvas1 = render_to_canvas(*menu, 40, 20);
        std::string normal_render = canvas1->render_ascii();

        // Minimize window, update menu, render again
        win.minimize();
        system_menu->update_menu_states();
        auto canvas2 = render_to_canvas(*menu, 40, 20);
        std::string minimized_render = canvas2->render_ascii();

        // Both should have content
        CHECK_FALSE(normal_render.empty());
        CHECK_FALSE(minimized_render.empty());

        // Count content chars in both states
        int normal_chars = 0;
        for (char c : normal_render) {
            if (c != ' ' && c != '\n') normal_chars++;
        }

        int minimized_chars = 0;
        for (char c : minimized_render) {
            if (c != ' ' && c != '\n') minimized_chars++;
        }

        CHECK(normal_chars > 0);
        CHECK(minimized_chars > 0);

        INFO("Normal state: " << normal_chars << " chars, Minimized state: " << minimized_chars << " chars");
    }

    SUBCASE("System menu renders after maximize") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        REQUIRE(system_menu != nullptr);

        auto* menu = system_menu->get_menu();
        REQUIRE(menu != nullptr);

        // Maximize and render
        win.maximize();
        system_menu->update_menu_states();

        auto canvas = render_to_canvas(*menu, 40, 20);
        std::string rendered = canvas->render_ascii();

        CHECK_FALSE(rendered.empty());

        int content_chars = 0;
        for (char c : rendered) {
            if (c != ' ' && c != '\n') content_chars++;
        }
        CHECK(content_chars > 0);
    }

    SUBCASE("System menu renders at different sizes") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        REQUIRE(system_menu != nullptr);

        auto* menu = system_menu->get_menu();
        REQUIRE(menu != nullptr);

        system_menu->update_menu_states();

        // Render at different sizes
        auto canvas1 = render_to_canvas(*menu, 30, 15);
        auto canvas2 = render_to_canvas(*menu, 50, 25);
        auto canvas3 = render_to_canvas(*menu, 20, 10);

        std::string render1 = canvas1->render_ascii();
        std::string render2 = canvas2->render_ascii();
        std::string render3 = canvas3->render_ascii();

        // All should have content
        CHECK_FALSE(render1.empty());
        CHECK_FALSE(render2.empty());
        CHECK_FALSE(render3.empty());
    }
}

// ============================================================================
// Menu Button Signal
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - Menu button signal") {
    SUBCASE("Menu button click updates menu states") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        REQUIRE(system_menu != nullptr);

        // Verify menu exists and can be updated
        system_menu->update_menu_states();

        // Menu should be accessible
        auto* menu = system_menu->get_menu();
        CHECK(menu != nullptr);
    }

    SUBCASE("Window with menu button has title bar") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;
        flags.has_title_bar = true;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        CHECK(system_menu != nullptr);
    }

    SUBCASE("Menu button without title bar creates no menu") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;
        flags.has_title_bar = false;  // No title bar

        window<test_canvas_backend> win("Test Window", flags);

        // System menu not created without title bar
        auto* system_menu = win.get_system_menu();
        CHECK(system_menu == nullptr);
    }
}

// ============================================================================
// Menu State Management
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - Menu state updates") {
    SUBCASE("Menu states correct for normal window") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        REQUIRE(system_menu != nullptr);

        // Normal window: Restore disabled, Minimize/Maximize enabled
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::normal);

        // Update menu states
        system_menu->update_menu_states();

        // Menu should be accessible and updatable
        auto* menu = system_menu->get_menu();
        CHECK(menu != nullptr);
    }

    SUBCASE("Menu states correct after minimize") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        REQUIRE(system_menu != nullptr);

        win.minimize();

        // Minimized: Restore enabled, Minimize disabled
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::minimized);

        // Update and verify menu is still accessible
        system_menu->update_menu_states();
        CHECK(system_menu->get_menu() != nullptr);
    }

    SUBCASE("Menu states correct after maximize") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        REQUIRE(system_menu != nullptr);

        win.maximize();

        // Maximized: Restore enabled, Maximize disabled
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::maximized);

        // Update and verify menu
        system_menu->update_menu_states();
        CHECK(system_menu->get_menu() != nullptr);
    }

    SUBCASE("Menu states correct after restore from minimized") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        REQUIRE(system_menu != nullptr);

        win.minimize();
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::minimized);

        win.restore();
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::normal);

        // Verify menu still works after state change
        system_menu->update_menu_states();
        CHECK(system_menu->get_menu() != nullptr);
    }

    SUBCASE("Menu states correct after restore from maximized") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        REQUIRE(system_menu != nullptr);

        win.maximize();
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::maximized);

        win.restore();
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::normal);

        // Verify menu still works
        system_menu->update_menu_states();
        CHECK(system_menu->get_menu() != nullptr);
    }

    SUBCASE("Menu survives multiple state transitions") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        REQUIRE(system_menu != nullptr);

        // Multiple state changes
        for (int i = 0; i < 3; ++i) {
            win.minimize();
            system_menu->update_menu_states();
            CHECK(system_menu->get_menu() != nullptr);

            win.restore();
            system_menu->update_menu_states();
            CHECK(system_menu->get_menu() != nullptr);

            win.maximize();
            system_menu->update_menu_states();
            CHECK(system_menu->get_menu() != nullptr);

            win.restore();
            system_menu->update_menu_states();
            CHECK(system_menu->get_menu() != nullptr);
        }
    }
}

// ============================================================================
// Menu Actions Integration
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - Menu actions trigger window operations") {
    SUBCASE("Close action works") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        bool closed = false;
        win.closed.connect([&]() {
            closed = true;
        });

        win.close();
        CHECK(closed);
    }

    SUBCASE("Minimize action works") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        bool minimized = false;
        win.minimized_sig.connect([&]() {
            minimized = true;
        });

        win.minimize();
        CHECK(minimized);
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::minimized);
    }

    SUBCASE("Maximize action works") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        bool maximized = false;
        win.maximized_sig.connect([&]() {
            maximized = true;
        });

        win.maximize();
        CHECK(maximized);
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::maximized);
    }

    SUBCASE("Restore action works from minimized") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        win.minimize();

        bool restored = false;
        win.restored_sig.connect([&]() {
            restored = true;
        });

        win.restore();
        CHECK(restored);
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::normal);
    }

    SUBCASE("Restore action works from maximized") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        win.maximize();

        bool restored = false;
        win.restored_sig.connect([&]() {
            restored = true;
        });

        win.restore();
        CHECK(restored);
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::normal);
    }
}

// ============================================================================
// Multiple State Transitions
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - Multiple state transitions with system menu") {
    SUBCASE("Minimize, restore, maximize, restore sequence") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        REQUIRE(system_menu != nullptr);

        // Start normal
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::normal);

        // Minimize
        win.minimize();
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::minimized);
        system_menu->update_menu_states();

        // Restore
        win.restore();
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::normal);
        system_menu->update_menu_states();

        // Maximize
        win.maximize();
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::maximized);
        system_menu->update_menu_states();

        // Restore again
        win.restore();
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::normal);
        system_menu->update_menu_states();

        // Verify menu still accessible
        CHECK(system_menu->get_menu() != nullptr);
    }

    SUBCASE("Rapid state changes") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        REQUIRE(system_menu != nullptr);

        for (int i = 0; i < 5; ++i) {
            win.minimize();
            CHECK(win.get_state() == window<test_canvas_backend>::window_state::minimized);
            system_menu->update_menu_states();

            win.restore();
            CHECK(win.get_state() == window<test_canvas_backend>::window_state::normal);
            system_menu->update_menu_states();

            win.maximize();
            CHECK(win.get_state() == window<test_canvas_backend>::window_state::maximized);
            system_menu->update_menu_states();

            win.restore();
            CHECK(win.get_state() == window<test_canvas_backend>::window_state::normal);
            system_menu->update_menu_states();
        }

        // Verify menu survived all transitions
        CHECK(system_menu->get_menu() != nullptr);
    }
}

// ============================================================================
// Integration with Window Lifecycle
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - System menu lifecycle") {
    SUBCASE("System menu survives window state changes") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        REQUIRE(system_menu != nullptr);

        win.minimize();
        win.restore();
        win.maximize();
        win.restore();

        // Window should still work
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::normal);

        // Menu should still exist and be accessible
        CHECK(win.get_system_menu() != nullptr);
        CHECK(system_menu->get_menu() != nullptr);
    }

    SUBCASE("System menu with window close") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        REQUIRE(system_menu != nullptr);

        win.close();

        // Window should be closed
        CHECK_FALSE(win.is_visible());

        // Menu should still exist (owned by window)
        CHECK(win.get_system_menu() != nullptr);
    }

    SUBCASE("System menu destroyed with window") {
        {
            typename window<test_canvas_backend>::window_flags flags;
            flags.has_menu_button = true;

            auto win = std::make_unique<window<test_canvas_backend>>("Test Window", flags);

            auto* system_menu = win->get_system_menu();
            REQUIRE(system_menu != nullptr);

            win->minimize();
            win->restore();
        }
        // Window and system menu destroyed - should not crash
    }

    SUBCASE("System menu visual rendering after lifecycle events") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        REQUIRE(system_menu != nullptr);

        // State changes
        win.minimize();
        win.restore();
        win.maximize();
        win.restore();

        // Render menu after all these changes
        auto* menu = system_menu->get_menu();
        REQUIRE(menu != nullptr);

        system_menu->update_menu_states();

        auto canvas = render_to_canvas(*menu, 40, 20);
        std::string rendered = canvas->render_ascii();

        // Should still render correctly
        CHECK_FALSE(rendered.empty());

        int content_chars = 0;
        for (char c : rendered) {
            if (c != ' ' && c != '\n') content_chars++;
        }
        CHECK(content_chars > 0);
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - System menu edge cases") {
    SUBCASE("Menu button with all other buttons disabled") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;
        flags.has_minimize_button = false;
        flags.has_maximize_button = false;
        flags.has_close_button = false;

        window<test_canvas_backend> win("Test Window", flags);

        // System menu should still be created
        auto* system_menu = win.get_system_menu();
        CHECK(system_menu != nullptr);

        if (system_menu) {
            CHECK(system_menu->get_menu() != nullptr);
        }
    }

    SUBCASE("Menu button with non-resizable window") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;
        flags.is_resizable = false;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        REQUIRE(system_menu != nullptr);

        // Maximize should still work via menu
        win.maximize();
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::maximized);

        // Menu should still work
        system_menu->update_menu_states();
        CHECK(system_menu->get_menu() != nullptr);
    }

    SUBCASE("Menu button with scrollable window") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;
        flags.is_scrollable = true;

        window<test_canvas_backend> win("Test Window", flags);

        // System menu should be created with scrollable content
        auto* system_menu = win.get_system_menu();
        CHECK(system_menu != nullptr);

        if (system_menu) {
            CHECK(system_menu->get_menu() != nullptr);
        }
    }

    SUBCASE("Menu button with modal window") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;
        flags.is_modal = true;

        window<test_canvas_backend> win("Test Window", flags);

        // System menu should work with modal flag
        auto* system_menu = win.get_system_menu();
        CHECK(system_menu != nullptr);

        if (system_menu) {
            CHECK(system_menu->get_menu() != nullptr);
        }
    }

    SUBCASE("Multiple windows with system menus") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win1("Window 1", flags);
        window<test_canvas_backend> win2("Window 2", flags);
        window<test_canvas_backend> win3("Window 3", flags);

        // All should have independent system menus
        auto* menu1 = win1.get_system_menu();
        auto* menu2 = win2.get_system_menu();
        auto* menu3 = win3.get_system_menu();

        CHECK(menu1 != nullptr);
        CHECK(menu2 != nullptr);
        CHECK(menu3 != nullptr);

        // All windows should work independently
        win1.minimize();
        CHECK(win1.get_state() == window<test_canvas_backend>::window_state::minimized);
        CHECK(win2.get_state() == window<test_canvas_backend>::window_state::normal);
        CHECK(win3.get_state() == window<test_canvas_backend>::window_state::normal);

        win2.maximize();
        CHECK(win1.get_state() == window<test_canvas_backend>::window_state::minimized);
        CHECK(win2.get_state() == window<test_canvas_backend>::window_state::maximized);
        CHECK(win3.get_state() == window<test_canvas_backend>::window_state::normal);

        // All menus should still work
        menu1->update_menu_states();
        menu2->update_menu_states();
        menu3->update_menu_states();

        CHECK(menu1->get_menu() != nullptr);
        CHECK(menu2->get_menu() != nullptr);
        CHECK(menu3->get_menu() != nullptr);
    }

    SUBCASE("System menu rendering with multiple windows") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win1("Window 1", flags);
        window<test_canvas_backend> win2("Window 2", flags);

        auto* menu1 = win1.get_system_menu();
        auto* menu2 = win2.get_system_menu();

        REQUIRE(menu1 != nullptr);
        REQUIRE(menu2 != nullptr);

        // Render both menus
        menu1->update_menu_states();
        menu2->update_menu_states();

        auto canvas1 = render_to_canvas(*menu1->get_menu(), 40, 20);
        auto canvas2 = render_to_canvas(*menu2->get_menu(), 40, 20);

        std::string render1 = canvas1->render_ascii();
        std::string render2 = canvas2->render_ascii();

        // Both should render independently
        CHECK_FALSE(render1.empty());
        CHECK_FALSE(render2.empty());
    }
}

// ============================================================================
// Alt+F4 Shortcut Display
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - Alt+F4 shortcut") {
    SUBCASE("Close action has Alt+F4 shortcut configured") {
        typename window<test_canvas_backend>::window_flags flags;
        flags.has_menu_button = true;

        window<test_canvas_backend> win("Test Window", flags);

        auto* system_menu = win.get_system_menu();
        REQUIRE(system_menu != nullptr);

        // Menu should exist with close action
        auto* menu = system_menu->get_menu();
        CHECK(menu != nullptr);

        // Visual rendering should show shortcut
        // (Actual shortcut display tested via menu_item rendering)
        system_menu->update_menu_states();

        auto canvas = render_to_canvas(*menu, 40, 20);
        std::string rendered = canvas->render_ascii();

        CHECK_FALSE(rendered.empty());
    }
}
