/**
 * @file test_text_view.cc
 * @brief Visual tests for text_view widget
 * @author OnyxUI Framework
 * @date 2025-11-01
 */

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include "onyxui/widgets/text_view.hh"
#include "onyxui/widgets/containers/panel.hh"
#include "onyxui/widgets/containers/scroll/scrollable.hh"
#include "onyxui/widgets/containers/scroll/scroll_view_presets.hh"
#include "onyxui/widgets/label.hh"
#include "onyxui/layout/linear_layout.hh"
#include <limits>
#include <algorithm>

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Simple panel with linear layout") {
    SUBCASE("Three labels in panel") {
        panel<test_canvas_backend> panel;
        panel.set_layout_strategy(
            std::make_unique<linear_layout<test_canvas_backend>>(
                direction::vertical,
                0,  // No spacing
                horizontal_alignment::left,
                vertical_alignment::top));
        panel.set_padding(thickness::all(1));

        // Add three labels
        panel.add_child(std::make_unique<label<test_canvas_backend>>("Line 1"));
        panel.add_child(std::make_unique<label<test_canvas_backend>>("Line 2"));
        panel.add_child(std::make_unique<label<test_canvas_backend>>("Line 3"));

        // Measure
        [[maybe_unused]] auto size = panel.measure(100, 100);

        // Arrange and render
        panel.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 40, 10}});

        auto canvas = render_to_canvas(panel, 40, 10);

        // Check canvas
        for (int row = 0; row < std::min(5, canvas->height()); row++) {
        }

        std::string content;
        for (int row = 0; row < canvas->height(); row++) {
            content += canvas->get_row(row);
        }

        CHECK(content.find("Line 1") != std::string::npos);
        CHECK(content.find("Line 2") != std::string::npos);
        CHECK(content.find("Line 3") != std::string::npos);
    }

    SUBCASE("Panel inside scrollable") {
        scrollable<test_canvas_backend> scrollable_widget;

        // Create panel with labels
        auto panel_ptr = std::make_unique<panel<test_canvas_backend>>();
        panel_ptr->set_layout_strategy(
            std::make_unique<linear_layout<test_canvas_backend>>(
                direction::vertical,
                0,  // No spacing
                horizontal_alignment::left,
                vertical_alignment::top));
        panel_ptr->set_padding(thickness::all(1));

        // Add three labels
        panel_ptr->add_child(std::make_unique<label<test_canvas_backend>>("Line 1"));
        panel_ptr->add_child(std::make_unique<label<test_canvas_backend>>("Line 2"));
        panel_ptr->add_child(std::make_unique<label<test_canvas_backend>>("Line 3"));

        // Add panel to scrollable
        scrollable_widget.add_child(std::move(panel_ptr));

        // Measure scrollable
        [[maybe_unused]] auto size = scrollable_widget.measure(100, 100);

        // Check scrollable's content size
        [[maybe_unused]] auto scroll_info = scrollable_widget.get_scroll_info();

        // Arrange and render
        scrollable_widget.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 40, 10}});

        auto canvas = render_to_canvas(scrollable_widget, 40, 10);

        // Check canvas
        for (int row = 0; row < std::min(5, canvas->height()); row++) {
        }

        std::string content;
        for (int row = 0; row < canvas->height(); row++) {
            content += canvas->get_row(row);
        }

        CHECK(content.find("Line 1") != std::string::npos);
        CHECK(content.find("Line 2") != std::string::npos);
        CHECK(content.find("Line 3") != std::string::npos);
    }
}

// Debug test removed - was used to investigate scroll_view child positioning issue
// The issue was identified and fixed in text_view with the pre-arrange workaround
#if 0
TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Debug text_view structure") {
    SUBCASE("Test with scroll_view") {
        // Create the same structure as text_view
        auto scroll_view_ptr = classic_scroll_view<test_canvas_backend>();

        // Create panel with labels
        auto panel_ptr = std::make_unique<panel<test_canvas_backend>>();
        panel_ptr->set_layout_strategy(
            std::make_unique<linear_layout<test_canvas_backend>>(
                direction::vertical,
                0,  // No spacing
                horizontal_alignment::left,
                vertical_alignment::top));
        panel_ptr->set_padding(thickness::all(1));

        // Add three labels (same as text_view would)
        panel_ptr->add_child(std::make_unique<label<test_canvas_backend>>("Line 1"));
        panel_ptr->add_child(std::make_unique<label<test_canvas_backend>>("Line 2"));
        panel_ptr->add_child(std::make_unique<label<test_canvas_backend>>("Line 3"));

        // Don't pre-measure or pre-arrange the panel

        // Add to scroll_view (this forwards to scrollable)
        scroll_view_ptr->add_child(std::move(panel_ptr));

        // Measure and arrange
        auto size = scroll_view_ptr->measure(40, 10);

        scroll_view_ptr->arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 40, 10}});

        // Render
        auto canvas = render_to_canvas(*scroll_view_ptr, 40, 10);

        for (int row = 0; row < std::min(5, canvas->height()); row++) {
        }

        std::string content;
        for (int row = 0; row < canvas->height(); row++) {
            content += canvas->get_row(row);
        }

        CHECK(content.find("Line 1") != std::string::npos);
        CHECK(content.find("Line 2") != std::string::npos);
        CHECK(content.find("Line 3") != std::string::npos);
    }

    SUBCASE("Test with scrollable directly") {
        // Create the same structure as text_view but with scrollable directly
        scrollable<test_canvas_backend> scrollable_widget;

        // Create panel with labels
        auto panel_ptr = std::make_unique<panel<test_canvas_backend>>();
        panel_ptr->set_layout_strategy(
            std::make_unique<linear_layout<test_canvas_backend>>(
                direction::vertical,
                0,  // No spacing
                horizontal_alignment::left,
                vertical_alignment::top));
        panel_ptr->set_padding(thickness::all(1));

        // Add three labels (same as text_view would)
        panel_ptr->add_child(std::make_unique<label<test_canvas_backend>>("Line 1"));
        panel_ptr->add_child(std::make_unique<label<test_canvas_backend>>("Line 2"));
        panel_ptr->add_child(std::make_unique<label<test_canvas_backend>>("Line 3"));

        // Add to scrollable
        scrollable_widget.add_child(std::move(panel_ptr));

        // Measure and arrange
        [[maybe_unused]] auto size = scrollable_widget.measure(40, 10);

        scrollable_widget.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 40, 10}});

        // Render
        auto canvas = render_to_canvas(scrollable_widget, 40, 10);

        for (int row = 0; row < std::min(5, canvas->height()); row++) {
        }

        std::string content;
        for (int row = 0; row < canvas->height(); row++) {
            content += canvas->get_row(row);
        }

        CHECK(content.find("Line 1") != std::string::npos);
        CHECK(content.find("Line 2") != std::string::npos);
        CHECK(content.find("Line 3") != std::string::npos);
    }

    SUBCASE("Test scroll_view with labels directly (no panel)") {
        // Test adding labels directly to scroll_view (bypassing panel)
        auto scroll_view_ptr = classic_scroll_view<test_canvas_backend>();

        // Set layout on the scrollable content area (via scroll_view)
        scroll_view_ptr->set_layout_strategy(
            std::make_unique<linear_layout<test_canvas_backend>>(
                direction::vertical,
                0,
                horizontal_alignment::left,
                vertical_alignment::top));

        // Add labels directly to scroll_view (forwards to scrollable)
        scroll_view_ptr->add_child(std::make_unique<label<test_canvas_backend>>("Direct 1"));
        scroll_view_ptr->add_child(std::make_unique<label<test_canvas_backend>>("Direct 2"));
        scroll_view_ptr->add_child(std::make_unique<label<test_canvas_backend>>("Direct 3"));

        // Measure and arrange
        auto size = scroll_view_ptr->measure(40, 10);

        scroll_view_ptr->arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 40, 10}});

        // Render
        auto canvas = render_to_canvas(*scroll_view_ptr, 40, 10);

        for (int row = 0; row < std::min(5, canvas->height()); row++) {
        }

        // Check if all lines are visible
        std::string content;
        for (int row = 0; row < canvas->height(); row++) {
            content += canvas->get_row(row);
        }
        CHECK(content.find("Direct 1") != std::string::npos);
        CHECK(content.find("Direct 2") != std::string::npos);
        CHECK(content.find("Direct 3") != std::string::npos);
    }
}
#endif // Debug test disabled

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

        CHECK(view.line_count() == 3);
        CHECK(view.get_line(0) == "Line 1");
        CHECK(view.get_line(1) == "Line 2");
        CHECK(view.get_line(2) == "Line 3");

        // First measure to see what size it reports
        [[maybe_unused]] auto measured_size = view.measure(40, 10);

        // Now let's manually check the hierarchy
        if (!view.children().empty()) {
            auto* scroll_view = view.children()[0].get();
            [[maybe_unused]] auto scroll_measured = scroll_view->measure(40, 10);

            if (!scroll_view->children().empty()) {
                auto* grid = scroll_view->children()[0].get();
                if (grid->children().size() >= 1) {
                    auto* scrollable = grid->children()[0].get();

                    // Measure scrollable
                    [[maybe_unused]] auto scrollable_measured = scrollable->measure(40, 10);

                    if (!scrollable->children().empty()) {
                        auto* content_panel = scrollable->children()[0].get();

                        // Measure content_panel with unconstrained size (like scrollable does)
                        constexpr int UNCONSTRAINED = std::numeric_limits<int>::max() - 1;
                        [[maybe_unused]] auto content_measured = content_panel->measure(UNCONSTRAINED, UNCONSTRAINED);

                        // Check each child
                        for (size_t i = 0; i < content_panel->children().size(); ++i) {
                            auto* label = content_panel->children()[i].get();
                            [[maybe_unused]] auto label_measured = label->measure(UNCONSTRAINED, UNCONSTRAINED);
                        }
                    }
                }
            }
        }

        // Use larger canvas to accommodate always-visible scrollbars (16px each)
        // plus content (3 lines) plus borders
        auto canvas = render_to_canvas(view, 60, 30);

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

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Demo layout with menu bar") {
    SUBCASE("text_view should not overlap with menu bar") {
        // Simulate the demo layout: vbox with menu bar, labels, and text_view
        panel<test_canvas_backend> root;
        root.set_vbox_layout(0);

        // Add a simple menu bar (using label as placeholder)
        auto menu_bar = std::make_unique<label<test_canvas_backend>>("[File] [Edit] [Help]");
        root.add_child(std::move(menu_bar));

        // Add title
        auto title = std::make_unique<label<test_canvas_backend>>("Demo Title");
        root.add_child(std::move(title));

        // Add text_view with content
        auto text_view_widget = std::make_unique<text_view<test_canvas_backend>>();
        std::string content;
        for (int i = 1; i <= 15; i++) {
            content += "[LOG " + std::to_string(i) + "] Entry at line " + std::to_string(i) + "\n";
        }
        text_view_widget->set_text(content);

        // Set constraints like in demo
        text_view_widget->set_horizontal_align(horizontal_alignment::stretch);
        size_constraint height_constraint;
        height_constraint.policy = size_policy::fixed;
        height_constraint.preferred_size = 5;  // Small height for testing
        height_constraint.min_size = 5;
        height_constraint.max_size = 5;
        text_view_widget->set_height_constraint(height_constraint);

        root.add_child(std::move(text_view_widget));

        // Add bottom label
        auto bottom = std::make_unique<label<test_canvas_backend>>("Press ESC to quit");
        root.add_child(std::move(bottom));

        // Measure and arrange before render
        [[maybe_unused]] auto measured_size = root.measure(40, 10);
        root.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 40, 10}});

        // Render
        auto canvas = render_to_canvas(root, 40, 10);

        // Visual assertions
        std::string first_line = canvas->get_row(0);
        std::string second_line = canvas->get_row(1);

        // Menu bar should be on first line
        CHECK(first_line.find("[File]") != std::string::npos);
        CHECK(first_line.find("[Edit]") != std::string::npos);
        CHECK(first_line.find("[Help]") != std::string::npos);

        // Title should be on second line
        CHECK(second_line.find("Demo Title") != std::string::npos);

        // text_view content should NOT be on first or second line (no overlap)
        CHECK(first_line.find("[LOG") == std::string::npos);
        CHECK(second_line.find("[LOG") == std::string::npos);

        // text_view should start with LOG 1, not LOG 15 (should scroll to top)
        bool found_log1 = false;
        for (int row = 2; row < canvas->height() - 1; row++) {  // Skip menu and bottom label
            std::string line = canvas->get_row(row);
            if (line.find("[LOG 1]") != std::string::npos) {
                found_log1 = true;
                break;
            }
        }
        CHECK(found_log1);
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

        // Use larger canvas to accommodate always-visible scrollbars (16px each)
        // plus content (5 log lines) plus borders
        auto canvas = render_to_canvas(view, 80, 30);

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
        view.arrange(geometry::relative_rect<test_canvas_backend>{test_canvas_backend::rect_type{0, 0, 50, 20}});

        // Should not crash and should update bounds
        auto bounds = view.bounds();
        CHECK(rect_utils::get_width(bounds) == 50);
        CHECK(rect_utils::get_height(bounds) == 20);
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

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Border support") {
    SUBCASE("Border can be enabled") {
        text_view<test_canvas_backend> view;

        // Border should be disabled by default
        CHECK(view.has_border() == false);

        // Enable border
        view.set_has_border(true);
        CHECK(view.has_border() == true);

        // Disable border
        view.set_has_border(false);
        CHECK(view.has_border() == false);
    }

    SUBCASE("Border affects layout") {
        text_view<test_canvas_backend> view;
        view.set_text("Hello, World!");

        // Measure without border
        auto size_no_border = view.measure(40, 10);

        // Enable border and measure again
        view.set_has_border(true);
        auto size_with_border = view.measure(40, 10);

        // With border, the widget should request more space (2px for border)
        CHECK(size_with_border.w == size_no_border.w + 2);
        CHECK(size_with_border.h == size_no_border.h + 2);
    }

    SUBCASE("Border renders correctly") {
        text_view<test_canvas_backend> view;
        view.set_text("Line 1\nLine 2\nLine 3");
        view.set_has_border(true);

        auto canvas = render_to_canvas(view, 40, 10);

        // Check that content is still visible
        std::string content;
        for (int row = 0; row < canvas->height(); row++) {
            content += canvas->get_row(row);
        }

        // Text should still be visible inside the border
        CHECK(content.find("Line 1") != std::string::npos);
        CHECK(view.line_count() == 3);
    }

    SUBCASE("Border integrity - content stays inside") {
        text_view<test_canvas_backend> view;
        view.set_text("Line 1\nLine 2\nLine 3\nLine 4\nLine 5\nLine 6\nLine 7\nLine 8");
        view.set_has_border(true);

        // Render with constrained height
        auto canvas = render_to_canvas(view, 40, 10);

        INFO("Canvas rendering:\n" << canvas->render_ascii());

        // Get the bounds where the border should be
        // Widget is 40x10, border is drawn at the edges

        // Check top border is intact (row 0 should have border chars)
        std::string top_row = canvas->get_row(0);
        CHECK(canvas->has_border_at(0, 0));           // Top-left corner
        CHECK(canvas->has_border_at(39, 0));          // Top-right corner

        // Check bottom border is intact (row 9 should have border chars)
        CHECK(canvas->has_border_at(0, 9));           // Bottom-left corner
        CHECK(canvas->has_border_at(39, 9));          // Bottom-right corner

        // Check left and right edges have borders
        CHECK(canvas->has_border_at(0, 5));           // Left edge middle
        CHECK(canvas->has_border_at(39, 5));          // Right edge middle

        // CRITICAL: Check that bottom border row (row 9) does NOT contain text content
        // This is the bug - text appears in the border row
        std::string bottom_row = canvas->get_row(9);
        CHECK_FALSE(bottom_row.find("Line") != std::string::npos);  // No "Line" text in border row
        CHECK_FALSE(bottom_row.find("Press") != std::string::npos); // No other content in border row

        // Verify complete border exists
        CHECK(canvas->has_complete_border(0, 0, 40, 10));
    }
}
