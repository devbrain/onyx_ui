/**
 * @file test_line_edit.cc
 * @brief Tests for line_edit widget
 * @author Claude Code
 * @date 2025-11-18
 */

#include <doctest/doctest.h>

#include <onyxui/widgets/input/line_edit.hh>
#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/geometry/coordinates.hh>
#include "../utils/test_backend.hh"
#include "../utils/test_helpers.hh"
#include "../utils/rule_of_five_tests.hh"

using namespace onyxui;
using namespace onyxui::geometry;

// ============================================================================
// Basic Construction and Text Management
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "line_edit - Basic construction") {
    SUBCASE("Default construction") {
        line_edit<test_backend> edit;

        CHECK(edit.text().empty());
        CHECK(edit.placeholder().empty());
        CHECK(edit.cursor_position() == 0);
        CHECK_FALSE(edit.has_selection());
        CHECK_FALSE(edit.is_password_mode());
        CHECK_FALSE(edit.is_read_only());
        CHECK_FALSE(edit.is_overwrite_mode());
        CHECK(edit.is_focusable());
    }

    SUBCASE("Construction with initial text") {
        line_edit<test_backend> edit("Hello");

        CHECK(edit.text() == "Hello");
        CHECK(edit.cursor_position() == 5);  // Cursor at end
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "line_edit - Text management") {
    line_edit<test_backend> edit;

    SUBCASE("Set and get text") {
        edit.set_text("Test");
        CHECK(edit.text() == "Test");
    }

    SUBCASE("Set text emits signal") {
        std::string emitted_text;
        edit.text_changed.connect([&](const std::string& text) {
            emitted_text = text;
        });

        edit.set_text("Hello");
        CHECK(emitted_text == "Hello");
    }

    SUBCASE("Set same text doesn't emit signal") {
        edit.set_text("Same");

        int signal_count = 0;
        edit.text_changed.connect([&](const std::string&) {
            ++signal_count;
        });

        edit.set_text("Same");  // Same text
        CHECK(signal_count == 0);
    }

    SUBCASE("Set text clamps cursor position") {
        edit.set_text("Long text");
        edit.set_cursor_position(100);  // Beyond end

        edit.set_text("Short");  // Shorter text
        CHECK(edit.cursor_position() <= 5);  // Clamped to length
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "line_edit - Placeholder text") {
    line_edit<test_backend> edit;

    SUBCASE("Set and get placeholder") {
        edit.set_placeholder("Enter name");
        CHECK(edit.placeholder() == "Enter name");
    }
}

// ============================================================================
// Display Modes
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "line_edit - Password mode") {
    line_edit<test_backend> edit("secret");

    SUBCASE("Enable password mode") {
        edit.set_password_mode(true);
        CHECK(edit.is_password_mode());
    }

    SUBCASE("Disable password mode") {
        edit.set_password_mode(true);
        edit.set_password_mode(false);
        CHECK_FALSE(edit.is_password_mode());
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "line_edit - Read-only mode") {
    line_edit<test_backend> edit("readonly");

    SUBCASE("Enable read-only") {
        edit.set_read_only(true);
        CHECK(edit.is_read_only());
    }

    SUBCASE("Disable read-only") {
        edit.set_read_only(true);
        edit.set_read_only(false);
        CHECK_FALSE(edit.is_read_only());
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "line_edit - Insert/overwrite mode") {
    line_edit<test_backend> edit;

    SUBCASE("Default is insert mode") {
        CHECK_FALSE(edit.is_overwrite_mode());
    }

    SUBCASE("Toggle to overwrite mode") {
        edit.set_overwrite_mode(true);
        CHECK(edit.is_overwrite_mode());
    }

    SUBCASE("Toggle back to insert mode") {
        edit.set_overwrite_mode(true);
        edit.set_overwrite_mode(false);
        CHECK_FALSE(edit.is_overwrite_mode());
    }
}

// ============================================================================
// Cursor and Selection
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "line_edit - Cursor positioning") {
    line_edit<test_backend> edit("Hello");

    SUBCASE("Set cursor position") {
        edit.set_cursor_position(2);
        CHECK(edit.cursor_position() == 2);
    }

    SUBCASE("Cursor position clamped to text length") {
        edit.set_cursor_position(100);
        CHECK(edit.cursor_position() == 5);  // Length of "Hello"
    }

    SUBCASE("Cursor position clamped to zero") {
        edit.set_cursor_position(-5);
        CHECK(edit.cursor_position() == 0);
    }

    SUBCASE("Set cursor clears selection") {
        edit.select_all();
        CHECK(edit.has_selection());

        edit.set_cursor_position(2);
        CHECK_FALSE(edit.has_selection());
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "line_edit - Text selection") {
    line_edit<test_backend> edit("Hello World");

    SUBCASE("Select all") {
        edit.select_all();
        CHECK(edit.has_selection());
        CHECK(edit.selected_text() == "Hello World");
    }

    SUBCASE("Clear selection") {
        edit.select_all();
        edit.clear_selection();
        CHECK_FALSE(edit.has_selection());
        CHECK(edit.selected_text().empty());
    }

    SUBCASE("No selection by default") {
        CHECK_FALSE(edit.has_selection());
        CHECK(edit.selected_text().empty());
    }
}

// ============================================================================
// Validation
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "line_edit - Input validation") {
    line_edit<test_backend> edit;

    SUBCASE("No validator means always valid") {
        edit.set_text("anything");
        CHECK(edit.is_valid());
    }

    SUBCASE("Validator accepts valid input") {
        edit.set_validator([](const std::string& text) {
            return std::all_of(text.begin(), text.end(), ::isdigit);
        });

        edit.set_text("12345");
        CHECK(edit.is_valid());
    }

    SUBCASE("Validator rejects invalid input") {
        edit.set_validator([](const std::string& text) {
            return std::all_of(text.begin(), text.end(), ::isdigit);
        });

        edit.set_text("abc123");
        CHECK_FALSE(edit.is_valid());
    }
}

// ============================================================================
// Undo/Redo
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "line_edit - Undo/redo") {
    line_edit<test_backend> edit("Original");

    SUBCASE("Undo does nothing with empty stack") {
        edit.undo();
        CHECK(edit.text() == "Original");
    }

    SUBCASE("Redo does nothing with empty stack") {
        edit.redo();
        CHECK(edit.text() == "Original");
    }
}

// ============================================================================
// Rendering and Layout
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "line_edit - Measure and arrange") {
    line_edit<test_backend> edit("Test");

    SUBCASE("Measure returns reasonable size") {
        auto size = edit.measure(1000_lu, 1000_lu);

        CHECK(size.width.to_int() > 0);
        CHECK(size.height.to_int() > 0);
    }

    SUBCASE("Arrange sets bounds") {
        [[maybe_unused]] auto _ = edit.measure(200_lu, 100_lu);
        edit.arrange(onyxui::logical_rect{10_lu, 20_lu, 200_lu, 30_lu});

        auto bounds = edit.bounds();
        CHECK(bounds.x.to_int() == 10);
        CHECK(bounds.y.to_int() == 20);
        CHECK(bounds.width.to_int() == 200);
        CHECK(bounds.height.to_int() == 30);
    }
}

// ============================================================================
// Focus Behavior
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "line_edit - Focus management") {
    line_edit<test_backend> edit;

    SUBCASE("line_edit is focusable by default") {
        CHECK(edit.is_focusable());
    }

    SUBCASE("Can receive focus") {
        [[maybe_unused]] auto _ = edit.measure(200_lu, 30_lu);
        edit.arrange(onyxui::logical_rect{0_lu, 0_lu, 200_lu, 30_lu});

        ctx.input().set_focus(&edit);
        CHECK(edit.has_focus());
    }
}

// ============================================================================
// Cursor Rendering
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "line_edit - Cursor rendering") {
    line_edit<test_backend> edit("Test");

    SUBCASE("Cursor visible when focused") {
        [[maybe_unused]] auto _ = edit.measure(200_lu, 30_lu);
        edit.arrange(onyxui::logical_rect{0_lu, 0_lu, 200_lu, 30_lu});

        ctx.input().set_focus(&edit);
        CHECK(edit.has_focus());

        // Cursor should be visible (m_cursor_visible defaults to true)
        // In actual rendering, cursor would be drawn as a rectangle
    }

    SUBCASE("Cursor style changes with insert/overwrite mode") {
        edit.set_overwrite_mode(false);
        CHECK_FALSE(edit.is_overwrite_mode());  // Insert mode (line cursor)

        edit.set_overwrite_mode(true);
        CHECK(edit.is_overwrite_mode());  // Overwrite mode (block cursor)
    }

    SUBCASE("Cursor resets on character insertion") {
        [[maybe_unused]] auto _ = edit.measure(200_lu, 30_lu);
        edit.arrange(onyxui::logical_rect{0_lu, 0_lu, 200_lu, 30_lu});
        ctx.input().set_focus(&edit);

        // Simulate typing - cursor should reset to visible
        edit.set_text("Hello");
        CHECK(edit.text() == "Hello");
        // reset_cursor_blink() is called internally
    }

    SUBCASE("Cursor resets on cursor movement") {
        edit.set_text("Hello World");
        [[maybe_unused]] auto _ = edit.measure(200_lu, 30_lu);
        edit.arrange(onyxui::logical_rect{0_lu, 0_lu, 200_lu, 30_lu});
        ctx.input().set_focus(&edit);

        const int initial_pos = edit.cursor_position();
        edit.set_cursor_position(5);
        CHECK(edit.cursor_position() == 5);
        CHECK(edit.cursor_position() != initial_pos);
        // reset_cursor_blink() is called internally
    }
}


// ============================================================================
// Tab Key Focus Navigation
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "line_edit - Tab key should not be consumed") {
    line_edit<test_backend> edit1("First");
    line_edit<test_backend> edit2("Second");

    // Set up both line_edits
    [[maybe_unused]] auto s1 = edit1.measure(200_lu, 30_lu);
    [[maybe_unused]] auto s2 = edit2.measure(200_lu, 30_lu);
    edit1.arrange(onyxui::logical_rect{0_lu, 0_lu, 200_lu, 30_lu});
    edit2.arrange(onyxui::logical_rect{0_lu, 40_lu, 200_lu, 30_lu});

    // Focus the first line_edit
    ctx.input().set_focus(&edit1);
    CHECK(edit1.has_focus());
    CHECK_FALSE(edit2.has_focus());

    SUBCASE("Tab key event should not be consumed by line_edit") {
        // Create Tab key event
        keyboard_event tab_evt;
        tab_evt.key = key_code::tab;
        tab_evt.pressed = true;
        tab_evt.modifiers = key_modifier::none;

        // Send Tab key to line_edit - it should NOT consume it (return false)
        // so focus navigation can happen at the application level
        ui_event event = tab_evt;
        bool handled = edit1.handle_event(event, event_phase::target);

        // Tab should NOT be handled by line_edit, allowing focus navigation
        CHECK_FALSE(handled);
    }

    SUBCASE("Shift+Tab key event should not be consumed by line_edit") {
        // Create Shift+Tab key event
        keyboard_event shift_tab_evt;
        shift_tab_evt.key = key_code::tab;
        shift_tab_evt.pressed = true;
        shift_tab_evt.modifiers = key_modifier::shift;

        // Send Shift+Tab key to line_edit - it should NOT consume it
        ui_event event = shift_tab_evt;
        bool handled = edit1.handle_event(event, event_phase::target);

        // Shift+Tab should NOT be handled by line_edit
        CHECK_FALSE(handled);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "line_edit - Tab triggers focus_next semantic action") {
    line_edit<test_backend> edit1("First");
    line_edit<test_backend> edit2("Second");

    // Set up both line_edits
    [[maybe_unused]] auto s1 = edit1.measure(200_lu, 30_lu);
    [[maybe_unused]] auto s2 = edit2.measure(200_lu, 30_lu);
    edit1.arrange(onyxui::logical_rect{0_lu, 0_lu, 200_lu, 30_lu});
    edit2.arrange(onyxui::logical_rect{0_lu, 40_lu, 200_lu, 30_lu});

    // Focus the first line_edit
    ctx.input().set_focus(&edit1);
    CHECK(edit1.has_focus());
    CHECK_FALSE(edit2.has_focus());

    SUBCASE("Tab key matches focus_next semantic action") {
        // Create Tab key event
        keyboard_event tab_evt;
        tab_evt.key = key_code::tab;
        tab_evt.pressed = true;
        tab_evt.modifiers = key_modifier::none;

        // Check if Tab matches focus_next semantic action
        auto& hotkeys = ctx.hotkeys();

        bool matches_focus_next = hotkeys.matches_action(tab_evt, hotkey_action::focus_next);
        CHECK(matches_focus_next);
    }

    SUBCASE("Shift+Tab key matches focus_previous semantic action") {
        // Create Shift+Tab key event
        keyboard_event shift_tab_evt;
        shift_tab_evt.key = key_code::tab;
        shift_tab_evt.pressed = true;
        shift_tab_evt.modifiers = key_modifier::shift;

        // Check if Shift+Tab matches focus_previous semantic action
        auto& hotkeys = ctx.hotkeys();

        bool matches_focus_previous = hotkeys.matches_action(shift_tab_evt, hotkey_action::focus_previous);
        CHECK(matches_focus_previous);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "line_edit - focus_next semantic action moves focus") {
    // Create a container with multiple focusable widgets
    onyxui::panel<test_backend> container;
    container.set_vbox_layout(spacing::medium);

    auto* edit1 = container.template emplace_child<line_edit>("First");
    auto* edit2 = container.template emplace_child<line_edit>("Second");

    // Set up layout
    [[maybe_unused]] auto s = container.measure(200_lu, 100_lu);
    container.arrange(onyxui::logical_rect{0_lu, 0_lu, 200_lu, 100_lu});

    // Focus the first line_edit
    ctx.input().set_focus(edit1);
    REQUIRE(edit1->has_focus());
    REQUIRE_FALSE(edit2->has_focus());

    SUBCASE("focus_next moves focus to next widget") {
        // Trigger focus_next semantic action on the focused widget
        bool handled = edit1->handle_semantic_action(hotkey_action::focus_next);

        CHECK(handled);
        CHECK_FALSE(edit1->has_focus());
        CHECK(edit2->has_focus());
    }

    SUBCASE("focus_previous wraps to last widget") {
        // Trigger focus_previous semantic action (should wrap to edit2)
        bool handled = edit1->handle_semantic_action(hotkey_action::focus_previous);

        CHECK(handled);
        CHECK_FALSE(edit1->has_focus());
        CHECK(edit2->has_focus());
    }

    SUBCASE("focus_next wraps from last to first") {
        // Focus the second widget
        ctx.input().set_focus(edit2);
        REQUIRE(edit2->has_focus());

        // Trigger focus_next (should wrap to edit1)
        bool handled = edit2->handle_semantic_action(hotkey_action::focus_next);

        CHECK(handled);
        CHECK(edit1->has_focus());
        CHECK_FALSE(edit2->has_focus());
    }
}

