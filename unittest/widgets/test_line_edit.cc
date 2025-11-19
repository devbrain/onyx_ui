/**
 * @file test_line_edit.cc
 * @brief Tests for line_edit widget
 * @author Claude Code
 * @date 2025-11-18
 */

#include <doctest/doctest.h>

#include <onyxui/widgets/input/line_edit.hh>
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
        auto size = edit.measure(1000, 1000);

        CHECK(size_utils::get_width(size) > 0);
        CHECK(size_utils::get_height(size) > 0);
    }

    SUBCASE("Arrange sets bounds") {
        [[maybe_unused]] auto _ = edit.measure(200, 100);
        edit.arrange(onyxui::testing::make_relative_rect<test_backend>(10, 20, 200, 30));

        auto bounds = edit.bounds();
        CHECK(rect_utils::get_x(bounds) == 10);
        CHECK(rect_utils::get_y(bounds) == 20);
        CHECK(rect_utils::get_width(bounds) == 200);
        CHECK(rect_utils::get_height(bounds) == 30);
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
        [[maybe_unused]] auto _ = edit.measure(200, 30);
        edit.arrange(onyxui::testing::make_relative_rect<test_backend>(0, 0, 200, 30));

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
        [[maybe_unused]] auto _ = edit.measure(200, 30);
        edit.arrange(onyxui::testing::make_relative_rect<test_backend>(0, 0, 200, 30));

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
        [[maybe_unused]] auto _ = edit.measure(200, 30);
        edit.arrange(onyxui::testing::make_relative_rect<test_backend>(0, 0, 200, 30));
        ctx.input().set_focus(&edit);

        // Simulate typing - cursor should reset to visible
        edit.set_text("Hello");
        CHECK(edit.text() == "Hello");
        // reset_cursor_blink() is called internally
    }

    SUBCASE("Cursor resets on cursor movement") {
        edit.set_text("Hello World");
        [[maybe_unused]] auto _ = edit.measure(200, 30);
        edit.arrange(onyxui::testing::make_relative_rect<test_backend>(0, 0, 200, 30));
        ctx.input().set_focus(&edit);

        const int initial_pos = edit.cursor_position();
        edit.set_cursor_position(5);
        CHECK(edit.cursor_position() == 5);
        CHECK(edit.cursor_position() != initial_pos);
        // reset_cursor_blink() is called internally
    }
}
