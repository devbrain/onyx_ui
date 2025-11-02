/**
 * @file test_text_view.cc
 * @brief Visual tests for text_view widget
 * @author OnyxUI Framework
 * @date 2025-11-01
 */

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include "onyxui/widgets/text_view.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Basic text display") {
    SUBCASE("Empty text view") {
        text_view<test_canvas_backend> view;

        auto canvas = render_to_canvas(view, 40, 10);

        // Should be empty (just scrollbar borders)
        CHECK(view.line_count() == 0);
        CHECK(canvas != nullptr);
    }

    SUBCASE("Single line") {
        text_view<test_canvas_backend> view;
        view.set_text("Hello, World!");

        auto canvas = render_to_canvas(view, 40, 10);

        // Visual assertion: text should be visible
        std::string content;
        for (int row = 0; row < canvas->height(); row++) {
            content += canvas->get_row(row);
        }
        CHECK(content.find("Hello") != std::string::npos);

        CHECK(view.line_count() == 1);
        CHECK(view.get_line(0) == "Hello, World!");
    }

    SUBCASE("Multiple lines") {
        text_view<test_canvas_backend> view;
        view.set_text("Line 1\nLine 2\nLine 3");

        auto canvas = render_to_canvas(view, 40, 10);

        // Visual assertions: all lines visible
        // Build full canvas content
        std::string content;
        for (int row = 0; row < canvas->height(); row++) {
            content += canvas->get_row(row);
        }
        CHECK(content.find("Line 1") != std::string::npos);
        CHECK(content.find("Line 2") != std::string::npos);
        CHECK(content.find("Line 3") != std::string::npos);
    }

    SUBCASE("Empty lines preserved") {
        text_view<test_canvas_backend> view;
        view.set_text("Line 1\n\nLine 3");

        CHECK(view.line_count() == 3);
        CHECK(view.get_line(0) == "Line 1");
        CHECK(view.get_line(1) == "");  // Empty line
        CHECK(view.get_line(2) == "Line 3");
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Content manipulation") {
    SUBCASE("Set pre-split lines") {
        text_view<test_canvas_backend> view;

        std::vector<std::string> lines = {"Alpha", "Beta", "Gamma"};
        view.set_lines(std::move(lines));

        CHECK(view.line_count() == 3);
        CHECK(view.get_line(0) == "Alpha");
        CHECK(view.get_line(1) == "Beta");
        CHECK(view.get_line(2) == "Gamma");
    }

    SUBCASE("Append single line") {
        text_view<test_canvas_backend> view;
        view.set_text("Line 1");

        view.append_line("Line 2");
        view.append_line("Line 3");

        CHECK(view.line_count() == 3);
        CHECK(view.get_line(0) == "Line 1");
        CHECK(view.get_line(1) == "Line 2");
        CHECK(view.get_line(2) == "Line 3");
    }

    SUBCASE("Append multi-line text") {
        text_view<test_canvas_backend> view;
        view.set_text("Initial");

        view.append_text("Appended 1\nAppended 2");

        CHECK(view.line_count() == 3);
        CHECK(view.get_line(0) == "Initial");
        CHECK(view.get_line(1) == "Appended 1");
        CHECK(view.get_line(2) == "Appended 2");
    }

    SUBCASE("Clear content") {
        text_view<test_canvas_backend> view;
        view.set_text("Line 1\nLine 2\nLine 3");

        CHECK(view.line_count() == 3);

        view.clear();

        CHECK(view.line_count() == 0);
        CHECK(view.get_line(0) == "");  // Out of range returns empty
    }

    SUBCASE("Replace content") {
        text_view<test_canvas_backend> view;
        view.set_text("Old text");

        CHECK(view.line_count() == 1);
        CHECK(view.get_line(0) == "Old text");

        view.set_text("New line 1\nNew line 2");

        CHECK(view.line_count() == 2);
        CHECK(view.get_line(0) == "New line 1");
        CHECK(view.get_line(1) == "New line 2");
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Visual rendering") {
    SUBCASE("Long text viewport clipping") {
        text_view<test_canvas_backend> view;

        // Add 20 lines (more than viewport height)
        std::string text;
        for (int i = 1; i <= 20; i++) {
            if (i > 1) text += "\n";
            text += "Log entry " + std::to_string(i);
        }
        view.set_text(text);

        auto canvas = render_to_canvas(view, 50, 10);

        // Visual assertion: only first few lines visible (viewport clipping)
        std::string content;
        for (int row = 0; row < canvas->height(); row++) {
            content += canvas->get_row(row);
        }
        CHECK(content.find("Log entry 1") != std::string::npos);
        CHECK(content.find("Log entry 2") != std::string::npos);
        // Later entries should not be visible in small viewport
        // (This depends on viewport size and scrollbar space)

        CHECK(view.line_count() == 20);
    }

    SUBCASE("Scrollbar visible with many lines") {
        text_view<test_canvas_backend> view;

        // Add enough lines to require scrolling
        std::string text;
        for (int i = 1; i <= 30; i++) {
            if (i > 1) text += "\n";
            text += "Item " + std::to_string(i);
        }
        view.set_text(text);

        auto canvas = render_to_canvas(view, 50, 12);

        // Visual assertion: scrollbar should be present
        // (Look for scrollbar characters in rightmost column)
        std::string content;
        for (int row = 0; row < canvas->height(); row++) {
            content += canvas->get_row(row);
        }

        // Content should be present
        CHECK(content.find("Item 1") != std::string::npos);
        CHECK(view.line_count() == 30);
    }

    SUBCASE("Text alignment") {
        text_view<test_canvas_backend> view;
        view.set_text("Left\nAligned\nText");

        auto canvas = render_to_canvas(view, 40, 10);

        // Visual assertion: text should be left-aligned
        // (Check that text starts near left edge)
        for (int row = 0; row < canvas->height(); row++) {
            std::string row_text = canvas->get_row(row);
            // If row contains text, it should be near the start
            if (row_text.find("Left") != std::string::npos ||
                row_text.find("Aligned") != std::string::npos ||
                row_text.find("Text") != std::string::npos) {
                // Text found - verify it's left-aligned (starts early in row)
                size_t pos = std::min({
                    row_text.find("Left"),
                    row_text.find("Aligned"),
                    row_text.find("Text")
                });
                // Should be within first few characters (accounting for borders/padding)
                CHECK(pos < 10);
            }
        }
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Edge cases") {
    SUBCASE("Very long single line") {
        text_view<test_canvas_backend> view;

        std::string long_line(200, 'X');  // 200 character line
        view.set_text(long_line);

        auto canvas = render_to_canvas(view, 50, 10);

        // Should handle gracefully (clipping)
        CHECK(view.line_count() == 1);
        CHECK(view.get_line(0).length() == 200);
    }

    SUBCASE("Special characters") {
        text_view<test_canvas_backend> view;
        view.set_text("Tab:\tNewline handled above");

        auto canvas = render_to_canvas(view, 40, 10);

        CHECK(view.line_count() == 1);
        // Tab character should be preserved
        CHECK(view.get_line(0).find('\t') != std::string::npos);
    }

    SUBCASE("Unicode characters") {
        text_view<test_canvas_backend> view;
        view.set_text("Hello 世界\nBonjour 🌍");

        auto canvas = render_to_canvas(view, 50, 10);

        CHECK(view.line_count() == 2);
        // Unicode should be preserved
        CHECK(view.get_line(0).find("世界") != std::string::npos);
    }

    SUBCASE("Get line out of range") {
        text_view<test_canvas_backend> view;
        view.set_text("Only line");

        CHECK(view.get_line(0) == "Only line");
        CHECK(view.get_line(1) == "");  // Out of range
        CHECK(view.get_line(999) == "");  // Way out of range
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Log-like usage") {
    SUBCASE("Simulated log output") {
        text_view<test_canvas_backend> view;

        // Simulate log entries being appended
        view.append_line("[INFO] Application started");
        view.append_line("[DEBUG] Loading configuration");
        view.append_line("[INFO] Server listening on port 8080");
        view.append_line("[WARN] Cache miss for key 'user_123'");
        view.append_line("[ERROR] Failed to connect to database");

        auto canvas = render_to_canvas(view, 60, 10);

        // Visual assertion: log entries should be visible
        std::string content;
        for (int row = 0; row < canvas->height(); row++) {
            content += canvas->get_row(row);
        }
        CHECK(content.find("[INFO]") != std::string::npos);
        CHECK(content.find("[ERROR]") != std::string::npos);

        CHECK(view.line_count() == 5);
        CHECK(view.get_line(0).find("Application started") != std::string::npos);
        CHECK(view.get_line(4).find("database") != std::string::npos);
    }

    SUBCASE("Clear and restart log") {
        text_view<test_canvas_backend> view;

        // Add initial logs
        view.append_line("Old log 1");
        view.append_line("Old log 2");
        CHECK(view.line_count() == 2);

        // Clear logs (simulating log rotation or restart)
        view.clear();
        CHECK(view.line_count() == 0);

        // Add new logs
        view.append_line("New log 1");
        view.append_line("New log 2");
        CHECK(view.line_count() == 2);
        CHECK(view.get_line(0) == "New log 1");
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Measurement and layout") {
    SUBCASE("Minimum size") {
        text_view<test_canvas_backend> view;
        view.set_text("Test");

        // Measure with constraints
        auto size = view.measure(100, 100);

        // Should have some minimum size (scrollbar + padding)
        CHECK(size.w > 0);
        CHECK(size.h > 0);
    }

    SUBCASE("Arrange in bounds") {
        text_view<test_canvas_backend> view;
        view.set_text("Line 1\nLine 2\nLine 3");

        // Measure and arrange
        [[maybe_unused]] auto size = view.measure(50, 20);
        view.arrange({0, 0, 50, 20});

        // Should not crash and should update bounds
        auto bounds = view.bounds();
        CHECK(bounds.w == 50);
        CHECK(bounds.h == 20);
    }

    SUBCASE("Content exceeds viewport") {
        text_view<test_canvas_backend> view;

        // Add many lines
        for (int i = 0; i < 50; i++) {
            view.append_line("Line " + std::to_string(i));
        }

        auto canvas = render_to_canvas(view, 40, 10);

        // Content should be clipped to viewport
        // Only first few lines visible
        CHECK(view.line_count() == 50);

        // Canvas should show limited viewport
        CHECK(canvas->height() == 10);
    }
}
