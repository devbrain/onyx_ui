/**
 * @file test_window.cc
 * @brief Unit tests for window widget
 * @author Claude Code
 * @date 2025-11-08
 */

#include <doctest/doctest.h>

#include "../utils/test_backend.hh"
#include "../utils/test_helpers.hh"
#include "onyxui/widgets/window/window.hh"
#include "onyxui/widgets/label.hh"
#include "onyxui/widgets/containers/vbox.hh"

using namespace onyxui;

TEST_CASE("Window - Basic construction") {
    SUBCASE("Construction with title") {
        window<test_backend> win("My Window");

        CHECK(win.get_title() == "My Window");
        CHECK(win.get_state() == window<test_backend>::window_state::normal);
        CHECK(win.get_content() == nullptr);  // No content yet
    }

    SUBCASE("Construction with flags") {
        typename window<test_backend>::window_flags flags;
        flags.has_title_bar = false;
        flags.has_minimize_button = false;
        flags.has_maximize_button = false;

        window<test_backend> win("No Title Bar", flags);

        CHECK(win.get_title() == "No Title Bar");
        // Title bar existence will be tested in rendering tests
    }

    SUBCASE("Default window state") {
        window<test_backend> win;

        CHECK(win.get_state() == window<test_backend>::window_state::normal);
        CHECK(win.visible());  // Windows start visible
    }
}

TEST_CASE("Window - Title management") {
    SUBCASE("Set title") {
        window<test_backend> win("Initial Title");

        win.set_title("New Title");
        CHECK(win.get_title() == "New Title");
    }

    SUBCASE("Title propagates to title bar") {
        window<test_backend> win("First");

        win.set_title("Second");
        CHECK(win.get_title() == "Second");

        // TODO: Verify title bar label text when we can inspect children
    }
}

TEST_CASE("Window - Content management") {
    SUBCASE("Set content") {
        window<test_backend> win;

        auto label = std::make_unique<label<test_backend>>("Hello");
        auto* label_ptr = label.get();

        win.set_content(std::move(label));

        CHECK(win.get_content() == label_ptr);
    }

    SUBCASE("Replace content") {
        window<test_backend> win;

        auto label1 = std::make_unique<label<test_backend>>("First");
        win.set_content(std::move(label1));

        auto label2 = std::make_unique<label<test_backend>>("Second");
        auto* label2_ptr = label2.get();

        win.set_content(std::move(label2));

        CHECK(win.get_content() == label2_ptr);
    }

    SUBCASE("Set content with complex widget") {
        window<test_backend> win;

        auto vbox = std::make_unique<vbox<test_backend>>();
        vbox->emplace_child<label<test_backend>>("Line 1");
        vbox->emplace_child<label<test_backend>>("Line 2");

        auto* vbox_ptr = vbox.get();
        win.set_content(std::move(vbox));

        CHECK(win.get_content() == vbox_ptr);
    }
}

TEST_CASE("Window - State transitions") {
    SUBCASE("Minimize window") {
        window<test_backend> win;
        bool minimized_emitted = false;

        win.minimized_sig.connect([&]() {
            minimized_emitted = true;
        });

        win.minimize();

        CHECK(win.get_state() == window<test_backend>::window_state::minimized);
        CHECK(minimized_emitted);
        CHECK(!win.visible());  // Minimized windows are hidden
    }

    SUBCASE("Maximize window") {
        window<test_backend> win;
        bool maximized_emitted = false;

        win.maximized_sig.connect([&]() {
            maximized_emitted = true;
        });

        win.maximize();

        CHECK(win.get_state() == window<test_backend>::window_state::maximized);
        CHECK(maximized_emitted);
    }

    SUBCASE("Restore from minimized") {
        window<test_backend> win;
        bool restored_emitted = false;

        win.restored_sig.connect([&]() {
            restored_emitted = true;
        });

        // Minimize then restore
        win.minimize();
        CHECK(win.get_state() == window<test_backend>::window_state::minimized);

        win.restore();

        CHECK(win.get_state() == window<test_backend>::window_state::normal);
        CHECK(restored_emitted);
        CHECK(win.visible());  // Restored windows are visible
    }

    SUBCASE("Restore from maximized") {
        window<test_backend> win;

        // Maximize then restore
        win.maximize();
        CHECK(win.get_state() == window<test_backend>::window_state::maximized);

        win.restore();

        CHECK(win.get_state() == window<test_backend>::window_state::normal);
    }

    SUBCASE("Minimize already minimized does nothing") {
        window<test_backend> win;
        int minimize_count = 0;

        win.minimized_sig.connect([&]() {
            minimize_count++;
        });

        win.minimize();
        CHECK(minimize_count == 1);

        win.minimize();  // Already minimized
        CHECK(minimize_count == 1);  // Signal not emitted again
    }

    SUBCASE("Maximize already maximized does nothing") {
        window<test_backend> win;
        int maximize_count = 0;

        win.maximized_sig.connect([&]() {
            maximize_count++;
        });

        win.maximize();
        CHECK(maximize_count == 1);

        win.maximize();  // Already maximized
        CHECK(maximize_count == 1);  // Signal not emitted again
    }
}

TEST_CASE("Window - Close functionality") {
    SUBCASE("Close emits signals") {
        window<test_backend> win;
        bool closing_emitted = false;
        bool closed_emitted = false;

        win.closing.connect([&]() {
            closing_emitted = true;
        });

        win.closed.connect([&]() {
            closed_emitted = true;
        });

        win.close();

        CHECK(closing_emitted);
        CHECK(closed_emitted);
        CHECK(!win.visible());  // Closed windows are hidden
    }
}

TEST_CASE("Window - Position and size") {
    SUBCASE("Set position") {
        window<test_backend> win;
        bool moved_emitted = false;

        win.moved.connect([&]() {
            moved_emitted = true;
        });

        win.set_position(100, 50);

        CHECK(win.bounds().x == 100);
        CHECK(win.bounds().y == 50);
        CHECK(moved_emitted);
    }

    SUBCASE("Set size") {
        window<test_backend> win;
        bool resized_emitted = false;

        win.resized_sig.connect([&]() {
            resized_emitted = true;
        });

        win.set_size(400, 300);

        CHECK(win.bounds().width == 400);
        CHECK(win.bounds().height == 300);
        CHECK(resized_emitted);
    }
}

TEST_CASE("Window - Show and hide") {
    SUBCASE("Show window") {
        window<test_backend> win;
        win.hide();
        CHECK(!win.visible());

        win.show();
        CHECK(win.visible());
    }

    SUBCASE("Hide window") {
        window<test_backend> win;
        CHECK(win.visible());

        win.hide();
        CHECK(!win.visible());
    }

    SUBCASE("Show modal") {
        window<test_backend> win;
        win.hide();

        win.show_modal();
        CHECK(win.visible());
        // TODO Phase 5: Test modal layer integration
    }
}

TEST_CASE("Window - Signals") {
    SUBCASE("All state signals work") {
        window<test_backend> win;

        int closing_count = 0;
        int closed_count = 0;
        int minimized_count = 0;
        int maximized_count = 0;
        int restored_count = 0;
        int moved_count = 0;
        int resized_count = 0;

        win.closing.connect([&]() { closing_count++; });
        win.closed.connect([&]() { closed_count++; });
        win.minimized_sig.connect([&]() { minimized_count++; });
        win.maximized_sig.connect([&]() { maximized_count++; });
        win.restored_sig.connect([&]() { restored_count++; });
        win.moved.connect([&]() { moved_count++; });
        win.resized_sig.connect([&]() { resized_count++; });

        // Trigger all signals
        win.minimize();
        win.restore();
        win.maximize();
        win.restore();
        win.set_position(10, 20);
        win.set_size(100, 200);
        win.close();

        CHECK(minimized_count == 1);
        CHECK(restored_count == 2);
        CHECK(maximized_count == 1);
        CHECK(moved_count == 1);
        CHECK(resized_count == 1);
        CHECK(closing_count == 1);
        CHECK(closed_count == 1);
    }
}
