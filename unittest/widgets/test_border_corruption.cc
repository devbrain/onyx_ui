/**
 * @file test_border_corruption.cc
 * @brief Test to reproduce and fix border corruption bug in widgets_demo
 * @author OnyxUI Framework
 * @date 2025-11-05
 *
 * @details
 * This test reproduces the exact issue seen in screenshot_1762331714.txt:28
 * where scrollable content bleeds into the bottom border line.
 *
 * Uses the EXACT UI structure from demo.hh to ensure accurate reproduction.
 */

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include "onyxui/widgets/text_view.hh"
#include "onyxui/widgets/containers/panel.hh"
#include "onyxui/widgets/label.hh"
#include "onyxui/widgets/button.hh"
#include <iostream>
#include <sstream>
#include <fstream>

using namespace onyxui;
using namespace onyxui::testing;

// Helper to add label (mimics demo.hh)
template<UIBackend Backend>
label<Backend>* add_label(widget_container<Backend>& parent, const std::string& text) {
    auto label_ptr = std::make_unique<label<Backend>>(text);
    auto* ptr = label_ptr.get();
    parent.add_child(std::move(label_ptr));
    return ptr;
}

// Helper to add button (mimics demo.hh)
template<UIBackend Backend>
button<Backend>* add_button(widget_container<Backend>& parent, const std::string& text) {
    auto button_ptr = std::make_unique<button<Backend>>(text);
    auto* ptr = button_ptr.get();
    parent.add_child(std::move(button_ptr));
    return ptr;
}

// Helper to add panel (mimics demo.hh)
template<UIBackend Backend>
panel<Backend>* add_panel(widget_container<Backend>& parent) {
    auto panel_ptr = std::make_unique<panel<Backend>>();
    auto* ptr = panel_ptr.get();
    parent.add_child(std::move(panel_ptr));
    return ptr;
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "widgets_demo - Border corruption bug") {

    SUBCASE("Minimal reproduction - text_view with border and constrained height") {
        // Minimal test that focuses ONLY on the text_view border corruption
        // This reproduces the exact issue without all the other UI clutter

        // Create a text_view with border (like in demo)
        auto text_view_widget = std::make_unique<text_view<test_canvas_backend>>();
        text_view_widget->set_has_border(true);

        // Add content that will need scrolling
        std::string demo_text =
            "Welcome to OnyxUI Text View Demo!\n"
            "\n"
            "This widget demonstrates:\n"
            "  * Multi-line text display\n"
            "  * Automatic scrolling\n"
            "  * Keyboard navigation:\n"
            "    - Arrow Up/Down: Scroll line by line\n"
            "    - Page Up/Down: Scroll by viewport\n"
            "    - Home: Jump to top\n"
            "    - End: Jump to bottom\n"
            "\n"
            "The text view uses scroll_view internally,\n"
            "so it inherits all scrolling features!\n"
            "\n"
            "Try switching themes (1-4 keys) to see\n"
            "how the scrollbar and text colors adapt.\n"
            "\n"
            "Log Entries:\n";

        for (int i = 1; i <= 15; i++) {
            demo_text += "[LOG " + std::to_string(i) + "] Entry at timestamp " +
                        std::to_string(1000 + i * 100) + " ms\n";
        }

        text_view_widget->set_text(demo_text);

        // CRITICAL: Set the exact height constraint from demo.hh
        size_constraint height_constraint;
        height_constraint.policy = size_policy::content;
        height_constraint.preferred_size = logical_unit(static_cast<double>(8));
        height_constraint.min_size = logical_unit(static_cast<double>(5));
        height_constraint.max_size = logical_unit(static_cast<double>(8));
        text_view_widget->set_height_constraint(height_constraint);

        // Measure and arrange
        [[maybe_unused]] auto measured_size = text_view_widget->measure(200_lu, 50_lu);
        text_view_widget->arrange(logical_rect{0_lu, 0_lu, 200_lu, 10_lu});  // Constrained to 10 rows

        // Render to canvas
        auto canvas = render_to_canvas(*text_view_widget, 200, 10);

        // Write to file for inspection
        std::ofstream debug_file("/tmp/border_corruption_minimal.txt");
        debug_file << canvas->render_ascii();
        debug_file.close();

        INFO("=== Minimal canvas written to /tmp/border_corruption_minimal.txt ===");
        INFO("Canvas size: " << canvas->width() << "x" << canvas->height());
        INFO(canvas->render_ascii());

        // The text_view should have borders
        // Top-left corner
        CHECK(canvas->has_border_at(0, 0));

        // Top-right corner
        CHECK(canvas->has_border_at(canvas->width() - 1, 0));

        // Bottom row - the CRITICAL check
        int bottom_y = canvas->height() - 1;
        std::string bottom_row = canvas->get_row(bottom_y);

        INFO("Bottom row (" << bottom_y << "): " << bottom_row);

        // Bottom-left corner should be border
        CHECK(canvas->has_border_at(0, bottom_y));

        // Bottom-right corner should be border
        CHECK(canvas->has_border_at(canvas->width() - 1, bottom_y));

        // CRITICAL: Bottom row should NOT contain text content
        // This is the bug we're trying to reproduce!
        CHECK(bottom_row.find("Keyboard navigation") == std::string::npos);
        CHECK(bottom_row.find("Arrow Up/Down") == std::string::npos);
        CHECK(bottom_row.find("LOG") == std::string::npos);
        CHECK(bottom_row.find("Multi-line") == std::string::npos);

        // Count content vs border characters in bottom row
        size_t content_chars = 0;
        size_t border_chars = 0;
        // UTF-8 box drawing characters are multi-byte, so check strings
        std::string border_chars_str = "-─═+└┘│┌┐├┤";
        for (size_t i = 0; i < bottom_row.length(); ) {
            char c = bottom_row[i];
            if (c == ' ' || c == '\t') {
                i++;
                continue;
            }
            // Check for ASCII border characters
            if (c == '-' || c == '+') {
                border_chars++;
                i++;
            }
            // Check for UTF-8 box drawing characters (multi-byte)
            else if (static_cast<unsigned char>(c) >= 0x80) {
                // UTF-8 character - check if it's a border character
                bool is_border = false;
                for (size_t j = 0; j < border_chars_str.length(); ) {
                    if (i < bottom_row.length() && j < border_chars_str.length()) {
                        // Compare UTF-8 sequences
                        size_t utf8_len = 1;
                        if ((static_cast<unsigned char>(border_chars_str[j]) & 0xE0) == 0xC0) utf8_len = 2;
                        else if ((static_cast<unsigned char>(border_chars_str[j]) & 0xF0) == 0xE0) utf8_len = 3;
                        else if ((static_cast<unsigned char>(border_chars_str[j]) & 0xF8) == 0xF0) utf8_len = 4;

                        if (bottom_row.substr(i, utf8_len) == border_chars_str.substr(j, utf8_len)) {
                            is_border = true;
                            i += utf8_len - 1;  // Will be incremented at end of loop
                            break;
                        }
                        j += utf8_len;
                    } else {
                        break;
                    }
                }
                if (is_border) {
                    border_chars++;
                }
                i++;
            }
            else if (std::isalnum(static_cast<unsigned char>(c)) ||
                     (std::ispunct(static_cast<unsigned char>(c)) && c != '-' && c != '+')) {
                content_chars++;
                i++;
            } else {
                i++;
            }
        }

        INFO("Bottom row analysis: border_chars=" << border_chars << " content_chars=" << content_chars);
        CHECK(content_chars == 0);  // NO content should bleed into border!
    }

    SUBCASE("EXACT reproduction of demo.hh UI structure") {
        // Create root panel with exact same layout as demo.hh
        panel<test_canvas_backend> root;
        root.set_vbox_layout(spacing::none);  // Vertical layout with no spacing
        root.set_padding(logical_thickness(0_lu));  // No internal padding

        // Build UI structure EXACTLY as in demo.hh (lines 98-231)

        // Title
        add_label(root, "DOS Theme Showcase - New Theme System");

        // Theme label
        add_label(root, "Current: NU8 (5/6)");

        // Spacer
        add_label(root, "");

        // Demo panel with border
        auto* demo_panel = add_panel(root);
        demo_panel->set_has_border(true);
        demo_panel->set_padding(logical_thickness(1_lu));
        demo_panel->set_vbox_layout(spacing::tiny);

        add_label(*demo_panel, "Panel with Border");
        add_label(*demo_panel, "Themes via service locator");

        // Spacer
        add_label(root, "");

        // Button section
        add_label(root, "Button States:");
        auto* normal_btn = add_button(root, "Normal");
        normal_btn->set_horizontal_align(horizontal_alignment::left);

        auto* screenshot_btn = add_button(root, "Screenshot");
        screenshot_btn->set_horizontal_align(horizontal_alignment::left);

        auto* disabled_btn = add_button(root, "Disabled");
        disabled_btn->set_enabled(false);
        disabled_btn->set_horizontal_align(horizontal_alignment::left);

        // Spacer
        add_label(root, "");

        auto* quit_btn = add_button(root, "Quit");
        quit_btn->set_horizontal_align(horizontal_alignment::left);

        // Spacer
        add_label(root, "");

        // Text View Section - THE CRITICAL PART
        add_label(root, "Scrollable Text View (Arrow keys, PgUp/PgDn, Home/End):");

        // Create text view with EXACT content and constraints from demo.hh
        auto text_view_widget = std::make_unique<text_view<test_canvas_backend>>();
        text_view_widget->set_has_border(true);  // Line 180 in demo.hh

        // Generate demo text content (lines 183-206 in demo.hh)
        std::string demo_text =
            "Welcome to OnyxUI Text View Demo!\n"
            "\n"
            "This widget demonstrates:\n"
            "  * Multi-line text display\n"
            "  * Automatic scrolling\n"
            "  * Keyboard navigation:\n"
            "    - Arrow Up/Down: Scroll line by line\n"
            "    - Page Up/Down: Scroll by viewport\n"
            "    - Home: Jump to top\n"
            "    - End: Jump to bottom\n"
            "\n"
            "The text view uses scroll_view internally,\n"
            "so it inherits all scrolling features!\n"
            "\n"
            "Try switching themes (1-4 keys) to see\n"
            "how the scrollbar and text colors adapt.\n"
            "\n"
            "Log Entries:\n";

        // Add simulated log entries
        for (int i = 1; i <= 15; i++) {
            demo_text += "[LOG " + std::to_string(i) + "] Entry at timestamp " +
                        std::to_string(1000 + i * 100) + " ms\n";
        }

        text_view_widget->set_text(demo_text);

        // Set alignment (lines 212-213 in demo.hh)
        text_view_widget->set_horizontal_align(horizontal_alignment::stretch);
        text_view_widget->set_vertical_align(vertical_alignment::center);

        // EXACT constraints from demo.hh (lines 216-221)
        size_constraint height_constraint;
        height_constraint.policy = size_policy::content;
        height_constraint.preferred_size = logical_unit(static_cast<double>(8));  // Preferred height of 8 lines
        height_constraint.min_size = logical_unit(static_cast<double>(5));        // Minimum 5 lines
        height_constraint.max_size = logical_unit(static_cast<double>(8));        // Maximum 8 lines
        text_view_widget->set_height_constraint(height_constraint);

        root.add_child(std::move(text_view_widget));

        // Spacer and instructions
        add_label(root, "");
        add_label(root, "Press 1-4 to switch themes | F10 for menu (try File->Open!)");
        add_label(root, "ESC, Ctrl+C, or Alt+F4 to quit");

        // Measure and arrange with sufficient space for all content
        // Need much more height than original 32 to accommodate all widgets without compression
        // Complex layout with multiple buttons, labels, panel, and text_view requires ~80 lines
        [[maybe_unused]] auto root_size = root.measure(240_lu, 80_lu);
        root.arrange(logical_rect{0_lu, 0_lu, 240_lu, 80_lu});

        // Render to canvas with sufficient height
        auto canvas = render_to_canvas(root, 240, 80);

        // Write full canvas to file for inspection
        std::ofstream debug_file("/tmp/border_corruption_canvas.txt");
        debug_file << canvas->render_ascii();
        debug_file.close();

        INFO("=== Canvas written to /tmp/border_corruption_canvas.txt ===");
        INFO("Canvas size: " << canvas->width() << "x" << canvas->height());

        // Find the text_view's border in the canvas
        // It should be around row 20-28 based on the layout
        bool found_text_view_border = false;
        int text_view_top = -1;
        int text_view_bottom = -1;

        // Scan for the text_view border (look for "Welcome to OnyxUI")
        for (int y = 0; y < canvas->height(); y++) {
            std::string row = canvas->get_row(y);
            if (row.find("Welcome to OnyxUI") != std::string::npos) {
                // Found content row, so border should be one row above
                text_view_top = y - 1;
                found_text_view_border = true;
                break;
            }
        }

        REQUIRE(found_text_view_border);
        INFO("Found text_view top border at row " << text_view_top);

        // Find the bottom border - scan down from top border
        for (int y = text_view_top + 1; y < canvas->height(); y++) {
            std::string row = canvas->get_row(y);
            // Bottom border should start with corner character and contain horizontal lines
            if (y > text_view_top + 2) {  // Skip at least a few content rows
                // Check if first character looks like a border character (ASCII or UTF-8 box drawing)
                bool starts_with_border = false;
                if (row.length() > 0) {
                    char first_char = row[0];
                    starts_with_border = (first_char == '+' || first_char == '-' ||
                                         static_cast<unsigned char>(first_char) >= 0x80);  // UTF-8 multi-byte
                }

                if (starts_with_border) {
                    // Check if row contains horizontal border characters
                    size_t border_chars = 0;
                    std::string horizontal_borders = "-─═+└┘";
                    for (size_t i = 0; i < row.length(); ) {
                        char c = row[i];
                        if (c == '-' || c == '+') {
                            border_chars++;
                            i++;
                        } else if (static_cast<unsigned char>(c) >= 0x80) {
                            // Check if it's a box drawing character
                            bool is_border = false;
                            for (size_t j = 0; j < horizontal_borders.length(); ) {
                                unsigned char uc = static_cast<unsigned char>(horizontal_borders[j]);
                                size_t utf8_len = 1;
                                if ((uc & 0xE0) == 0xC0) utf8_len = 2;
                                else if ((uc & 0xF0) == 0xE0) utf8_len = 3;
                                else if ((uc & 0xF8) == 0xF0) utf8_len = 4;

                                if (i + utf8_len <= row.length() && j + utf8_len <= horizontal_borders.length()) {
                                    if (row.substr(i, utf8_len) == horizontal_borders.substr(j, utf8_len)) {
                                        is_border = true;
                                        border_chars++;
                                        i += utf8_len - 1;
                                        break;
                                    }
                                }
                                j += utf8_len;
                            }
                            if (!is_border) i++;
                            else i++;
                        } else {
                            i++;
                        }
                    }
                    if (border_chars > row.length() / 2) {  // More than half are border chars
                        text_view_bottom = y;
                        break;
                    }
                }
            }
        }

        REQUIRE(text_view_bottom > text_view_top);
        INFO("Found text_view bottom border at row " << text_view_bottom);

        // Print the text_view region for inspection
        INFO("=== TEXT VIEW REGION (rows " << text_view_top << " to " << text_view_bottom << ") ===");
        for (int y = text_view_top; y <= text_view_bottom && y < canvas->height(); y++) {
            INFO("Row " << y << ": " << canvas->get_row(y));
        }

        // CRITICAL ASSERTION: The bottom border row should NOT contain text content
        std::string bottom_row = canvas->get_row(text_view_bottom);

        INFO("Bottom border row: " << bottom_row);

        // Check for content bleeding into border
        CHECK(bottom_row.find("Keyboard navigation") == std::string::npos);
        CHECK(bottom_row.find("Arrow Up/Down") == std::string::npos);
        CHECK(bottom_row.find("LOG") == std::string::npos);
        CHECK(bottom_row.find("Welcome") == std::string::npos);
        CHECK(bottom_row.find("Multi-line") == std::string::npos);

        // The bottom row should consist primarily of border characters
        size_t content_chars = 0;
        size_t border_chars = 0;
        std::string all_borders = "-─═+└┘│┌┐├┤";
        for (size_t i = 0; i < bottom_row.length(); ) {
            char c = bottom_row[i];
            if (c == ' ' || c == '\t') {
                i++;
                continue;
            }
            // Check for ASCII border characters
            if (c == '-' || c == '+') {
                border_chars++;
                i++;
            }
            // Check for UTF-8 box drawing characters (multi-byte)
            else if (static_cast<unsigned char>(c) >= 0x80) {
                bool is_border = false;
                for (size_t j = 0; j < all_borders.length(); ) {
                    unsigned char uc = static_cast<unsigned char>(all_borders[j]);
                    size_t utf8_len = 1;
                    if ((uc & 0xE0) == 0xC0) utf8_len = 2;
                    else if ((uc & 0xF0) == 0xE0) utf8_len = 3;
                    else if ((uc & 0xF8) == 0xF0) utf8_len = 4;

                    if (i + utf8_len <= bottom_row.length() && j + utf8_len <= all_borders.length()) {
                        if (bottom_row.substr(i, utf8_len) == all_borders.substr(j, utf8_len)) {
                            is_border = true;
                            i += utf8_len - 1;
                            break;
                        }
                    }
                    j += utf8_len;
                }
                if (is_border) border_chars++;
                i++;
            }
            else if (std::isalnum(static_cast<unsigned char>(c)) || std::ispunct(static_cast<unsigned char>(c))) {
                content_chars++;
                i++;
            } else {
                i++;
            }
        }

        INFO("Bottom row analysis: border_chars=" << border_chars << " content_chars=" << content_chars);
        CHECK(content_chars == 0);  // NO content characters should appear in border
    }
}
