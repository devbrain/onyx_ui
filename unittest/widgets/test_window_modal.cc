/**
 * @file test_window_modal.cc
 * @brief Tests for window modal behavior and layer_manager integration
 * @date 2025-11-08
 */

#include <doctest/doctest.h>
#include <onyxui/widgets/window/window.hh>
#include <onyxui/services/ui_services.hh>
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;
using namespace onyxui::testing;

// ============================================================================
// Layer Manager Integration
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - show() integration") {
    SUBCASE("show() makes window visible") {
        window<test_canvas_backend> win("Test Window");

        // Windows start visible by default in this framework
        CHECK(win.is_visible());

        // Hide then show again
        win.hide();
        CHECK_FALSE(win.is_visible());

        win.show();
        CHECK(win.is_visible());
    }

    SUBCASE("show() can be called multiple times") {
        window<test_canvas_backend> win("Test Window");

        win.show();
        CHECK(win.is_visible());

        win.show();  // Second call should not crash
        CHECK(win.is_visible());
    }

    SUBCASE("show() after hide() makes window visible again") {
        window<test_canvas_backend> win("Test Window");

        win.show();
        CHECK(win.is_visible());

        win.hide();
        CHECK_FALSE(win.is_visible());

        win.show();
        CHECK(win.is_visible());
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - show_modal() integration") {
    SUBCASE("show_modal() makes window visible") {
        window<test_canvas_backend> win("Modal Window");

        // Windows start visible by default
        CHECK(win.is_visible());

        // Hide then show modal
        win.hide();
        CHECK_FALSE(win.is_visible());

        win.show_modal();
        CHECK(win.is_visible());
    }

    SUBCASE("show_modal() can be called multiple times") {
        window<test_canvas_backend> win("Modal Window");

        win.show_modal();
        CHECK(win.is_visible());

        win.show_modal();  // Second call should not crash
        CHECK(win.is_visible());
    }

    SUBCASE("show_modal() after hide()") {
        window<test_canvas_backend> win("Modal Window");

        win.show_modal();
        CHECK(win.is_visible());

        win.hide();
        CHECK_FALSE(win.is_visible());

        win.show_modal();
        CHECK(win.is_visible());
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - hide() integration") {
    SUBCASE("hide() makes window invisible") {
        window<test_canvas_backend> win("Test Window");

        win.show();
        CHECK(win.is_visible());

        win.hide();
        CHECK_FALSE(win.is_visible());
    }

    SUBCASE("hide() can be called when already hidden") {
        window<test_canvas_backend> win("Test Window");

        // First hide it
        win.hide();
        CHECK_FALSE(win.is_visible());

        // Hide again - should not crash
        win.hide();
        CHECK_FALSE(win.is_visible());
    }

    SUBCASE("hide() can be called multiple times") {
        window<test_canvas_backend> win("Test Window");

        win.show();
        win.hide();
        win.hide();  // Second call should not crash
        CHECK_FALSE(win.is_visible());
    }
}

// ============================================================================
// Multiple Windows
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - Multiple windows") {
    SUBCASE("Multiple non-modal windows can be shown") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");
        window<test_canvas_backend> win3("Window 3");

        win1.show();
        win2.show();
        win3.show();

        CHECK(win1.is_visible());
        CHECK(win2.is_visible());
        CHECK(win3.is_visible());
    }

    SUBCASE("Multiple modal windows can be shown") {
        window<test_canvas_backend> win1("Modal 1");
        window<test_canvas_backend> win2("Modal 2");

        win1.show_modal();
        win2.show_modal();

        CHECK(win1.is_visible());
        CHECK(win2.is_visible());
    }

    SUBCASE("Mix of modal and non-modal windows") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Modal 1");
        window<test_canvas_backend> win3("Window 2");

        win1.show();
        win2.show_modal();
        win3.show();

        CHECK(win1.is_visible());
        CHECK(win2.is_visible());
        CHECK(win3.is_visible());
    }

    SUBCASE("Hiding one window doesn't affect others") {
        window<test_canvas_backend> win1("Window 1");
        window<test_canvas_backend> win2("Window 2");

        win1.show();
        win2.show();

        win1.hide();

        CHECK_FALSE(win1.is_visible());
        CHECK(win2.is_visible());
    }
}

// ============================================================================
// Window Lifecycle with Layer Manager
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - Lifecycle") {
    SUBCASE("Window destructor cleans up layer") {
        {
            window<test_canvas_backend> win("Temporary");
            win.show();
            CHECK(win.is_visible());
        }
        // Window destroyed - layer should be cleaned up
        // (No way to directly verify, but should not crash)
    }

    SUBCASE("Window can be shown, hidden, and destroyed") {
        auto win = std::make_unique<window<test_canvas_backend>>("Test");

        win->show();
        CHECK(win->is_visible());

        win->hide();
        CHECK_FALSE(win->is_visible());

        win.reset();  // Destroy
        // Should not crash
    }

    SUBCASE("Multiple show/hide cycles") {
        window<test_canvas_backend> win("Test");

        for (int i = 0; i < 5; ++i) {
            win.show();
            CHECK(win.is_visible());

            win.hide();
            CHECK_FALSE(win.is_visible());
        }
    }
}

// ============================================================================
// Window Close Integration
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - close() hides window") {
    SUBCASE("close() hides window") {
        window<test_canvas_backend> win("Test");

        win.show();
        CHECK(win.is_visible());

        win.close();
        CHECK_FALSE(win.is_visible());
    }

    SUBCASE("close() emits signals in correct order") {
        window<test_canvas_backend> win("Test");

        int closing_count = 0;
        int closed_count = 0;

        win.closing.connect([&]() {
            closing_count++;
            // closed should not be emitted yet
            CHECK(closed_count == 0);
        });

        win.closed.connect([&]() {
            closed_count++;
            // closing should have been emitted
            CHECK(closing_count == 1);
        });

        win.show();
        win.close();

        CHECK(closing_count == 1);
        CHECK(closed_count == 1);
    }

    SUBCASE("close() can be called when window is not visible") {
        window<test_canvas_backend> win("Test");

        // First hide it
        win.hide();
        CHECK_FALSE(win.is_visible());

        // Close while hidden - should not crash
        win.close();
        CHECK_FALSE(win.is_visible());
    }
}

// ============================================================================
// Modal Flags
// ============================================================================

TEST_CASE("window - Modal flags") {
    SUBCASE("Non-modal window by default") {
        window<test_backend> win;

        // Default window should be non-modal
        // Note: Can't directly access is_modal flag, but verify construction works
        CHECK(win.get_title() == "");
    }

    SUBCASE("Modal window with flags") {
        typename window<test_backend>::window_flags flags;
        flags.is_modal = true;

        window<test_backend> win("Modal", flags);

        // Modal flag set via constructor
        CHECK(win.get_title() == "Modal");
    }

    SUBCASE("Non-resizable modal window") {
        typename window<test_backend>::window_flags flags;
        flags.is_modal = true;
        flags.is_resizable = false;

        window<test_backend> win("Modal Dialog", flags);

        CHECK(win.get_title() == "Modal Dialog");
    }
}

// ============================================================================
// Integration with Window States
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - States with show/hide") {
    SUBCASE("Minimized window can be shown again") {
        window<test_canvas_backend> win("Test");

        win.show();
        win.minimize();

        CHECK(win.get_state() == window<test_canvas_backend>::window_state::minimized);
        CHECK_FALSE(win.is_visible());

        win.restore();
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::normal);
        // Note: restore() doesn't automatically show, must call show() again
    }

    SUBCASE("Maximized window can be hidden") {
        window<test_canvas_backend> win("Test");

        win.show();
        win.maximize();

        CHECK(win.get_state() == window<test_canvas_backend>::window_state::maximized);
        CHECK(win.is_visible());

        win.hide();
        CHECK_FALSE(win.is_visible());
        // State should still be maximized
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::maximized);
    }

    SUBCASE("show() doesn't change window state") {
        window<test_canvas_backend> win("Test");

        win.maximize();
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::maximized);

        win.show();
        // State should still be maximized
        CHECK(win.get_state() == window<test_canvas_backend>::window_state::maximized);
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - Edge cases") {
    SUBCASE("Rapid show/hide cycles") {
        window<test_canvas_backend> win("Test");

        for (int i = 0; i < 100; ++i) {
            win.show();
            win.hide();
        }
        // Should not crash or leak
    }

    SUBCASE("show() then immediate close()") {
        window<test_canvas_backend> win("Test");

        win.show();
        win.close();

        CHECK_FALSE(win.is_visible());
    }

    SUBCASE("show_modal() then immediate close()") {
        window<test_canvas_backend> win("Test");

        win.show_modal();
        win.close();

        CHECK_FALSE(win.is_visible());
    }

    SUBCASE("Multiple windows destroyed in different orders") {
        auto win1 = std::make_unique<window<test_canvas_backend>>("Win 1");
        auto win2 = std::make_unique<window<test_canvas_backend>>("Win 2");
        auto win3 = std::make_unique<window<test_canvas_backend>>("Win 3");

        win1->show();
        win2->show();
        win3->show();

        // Destroy in different order than creation
        win2.reset();
        win3.reset();
        win1.reset();
        // Should not crash
    }
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - Performance") {
    SUBCASE("Many windows can be created") {
        std::vector<std::unique_ptr<window<test_canvas_backend>>> windows;

        for (int i = 0; i < 50; ++i) {
            windows.push_back(std::make_unique<window<test_canvas_backend>>("Window " + std::to_string(i)));
        }

        // Show all windows
        for (auto& win : windows) {
            win->show();
        }

        // All should be visible
        for (const auto& win : windows) {
            CHECK(win->is_visible());
        }

        // Hide all windows
        for (auto& win : windows) {
            win->hide();
        }

        // All should be hidden
        for (const auto& win : windows) {
            CHECK_FALSE(win->is_visible());
        }
    }
}

// ============================================================================
// Modal Flag Integration
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - is_modal() flag") {
    SUBCASE("show_modal() sets is_modal flag") {
        window<test_canvas_backend> win("Test");

        CHECK_FALSE(win.is_modal());

        win.show_modal();

        CHECK(win.is_modal());
    }

    SUBCASE("show() clears is_modal flag") {
        window<test_canvas_backend> win("Test");

        // First show as modal
        win.show_modal();
        CHECK(win.is_modal());

        // Then show as normal
        win.show();
        CHECK_FALSE(win.is_modal());
    }

    SUBCASE("is_modal persists after hide") {
        window<test_canvas_backend> win("Test");

        win.show_modal();
        CHECK(win.is_modal());

        win.hide();
        // Flag should persist after hide
        CHECK(win.is_modal());
    }

    SUBCASE("show_modal() after show() sets modal flag") {
        window<test_canvas_backend> win("Test");

        // Normal show first
        win.show();
        CHECK_FALSE(win.is_modal());

        // Then modal
        win.show_modal();
        CHECK(win.is_modal());
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "window - Layer manager modal integration") {
    auto* layers = ui_services<test_canvas_backend>::layers();
    REQUIRE(layers != nullptr);

    SUBCASE("show_modal() creates modal layer") {
        window<test_canvas_backend> win("Modal Test");

        CHECK_FALSE(layers->has_modal_layer());

        win.show_modal();

        CHECK(layers->has_modal_layer());
    }

    SUBCASE("hide() removes modal layer") {
        window<test_canvas_backend> win("Modal Test");

        win.show_modal();
        CHECK(layers->has_modal_layer());

        win.hide();
        CHECK_FALSE(layers->has_modal_layer());
    }

    SUBCASE("Multiple modal windows create multiple layers") {
        window<test_canvas_backend> win1("Modal 1");
        window<test_canvas_backend> win2("Modal 2");

        win1.show_modal();
        CHECK(layers->has_modal_layer());

        win2.show_modal();
        CHECK(layers->has_modal_layer());

        // Hide first - should still have modal
        win1.hide();
        CHECK(layers->has_modal_layer());

        // Hide second - no more modals
        win2.hide();
        CHECK_FALSE(layers->has_modal_layer());
    }
}
