//
// Created by Claude Code on 2025-11-23.
//
// Tab 2: Layout & Scrolling - Layout system and scrolling demonstrations
// Shows layout algorithm, alignment, sizing, and scrolling features
//

#pragma once
#include "../backend_config.hh"  // Pulls the simple-shell bundle + all widget headers.

#include <memory>
#include <string>

namespace widgets_demo_tabs {

/**
 * @brief Create Tab 2: Layout & Scrolling
 *
 * @details
 * Demonstrates:
 * - Layout system (measure/arrange, alignment, sizing)
 * - Scrolling system (scroll_view, large content, keyboard navigation)
 * - Padding and margin examples
 * - Nested layouts
 *
 * @return Panel containing the complete tab content
 */
inline std::unique_ptr<ui::panel> create_tab_layout_scrolling() {
    auto tab = std::make_unique<ui::panel>();
    tab->set_padding(onyxui::logical_thickness(onyxui::logical_unit(1.0)));

    // Make entire tab scrollable
    auto* scroll = tab->emplace_child<ui::scroll_view>();

    // Main content container (vertical layout)
    auto* content = scroll->content_emplace_child<ui::vbox>(onyxui::spacing::tiny);  // 1px spacing

    // Title
    auto* title = content->emplace_child<ui::label>("Layout & Scrolling System");
    title->set_horizontal_align(onyxui::horizontal_alignment::center);

    content->emplace_child<ui::label>("");  // Spacer

    // ========== ROW 1: Layout System | Scrolling System ==========
    auto* row1 = content->emplace_child<ui::hbox>(onyxui::spacing::medium);

    // ========== LAYOUT SECTION (left column) ==========
    auto* layout_section = row1->emplace_child<ui::group_box>();
    layout_section->set_title("Layout System");
    layout_section->set_vbox_layout(onyxui::spacing::tiny);

    // Two-Pass Algorithm
    layout_section->emplace_child<ui::label>("Two-Pass Layout Algorithm:");
    layout_section->emplace_child<ui::label>("  1. Measure Pass (bottom-up)");
    layout_section->emplace_child<ui::label>("  2. Arrange Pass (top-down)");

    // Horizontal Alignment
    layout_section->emplace_child<ui::label>("");  // Spacer
    layout_section->emplace_child<ui::label>("Horizontal Alignment:");

    auto* left_label = layout_section->emplace_child<ui::label>("[Left]");
    left_label->set_horizontal_align(onyxui::horizontal_alignment::left);

    auto* center_label = layout_section->emplace_child<ui::label>("[Center]");
    center_label->set_horizontal_align(onyxui::horizontal_alignment::center);

    auto* right_label = layout_section->emplace_child<ui::label>("[Right]");
    right_label->set_horizontal_align(onyxui::horizontal_alignment::right);

    // Vertical Alignment
    layout_section->emplace_child<ui::label>("");  // Spacer
    layout_section->emplace_child<ui::label>("Vertical Alignment:");

    auto* valign_container = layout_section->emplace_child<ui::hbox>(onyxui::spacing::tiny);
    auto* top_btn = valign_container->emplace_child<ui::button>("[Top]");
    top_btn->set_vertical_align(onyxui::vertical_alignment::top);
    auto* vcenter_btn = valign_container->emplace_child<ui::button>("[Ctr]");
    vcenter_btn->set_vertical_align(onyxui::vertical_alignment::center);
    auto* bottom_btn = valign_container->emplace_child<ui::button>("[Bot]");
    bottom_btn->set_vertical_align(onyxui::vertical_alignment::bottom);

    // Spacer and Spring
    layout_section->emplace_child<ui::label>("");  // Spacer
    layout_section->emplace_child<ui::label>("Spacer vs Spring:");

    auto* spacer_demo = layout_section->emplace_child<ui::hbox>(onyxui::spacing::none);
    spacer_demo->emplace_child<ui::label>("[L]");
    spacer_demo->emplace_child<ui::spacer>(5);
    spacer_demo->emplace_child<ui::label>("[5]");
    spacer_demo->emplace_child<ui::spring>();
    spacer_demo->emplace_child<ui::label>("[R]");

    // ========== SCROLLING SECTION (right column) ==========
    auto* scrolling_section = row1->emplace_child<ui::group_box>();
    scrolling_section->set_title("Scrolling System");
    scrolling_section->set_vbox_layout(onyxui::spacing::tiny);

    scrolling_section->emplace_child<ui::label>("Scrolling Architecture:");
    scrolling_section->emplace_child<ui::label>("  - scroll_view: Container");
    scrolling_section->emplace_child<ui::label>("  - scrollable: Viewport");
    scrolling_section->emplace_child<ui::label>("  - scrollbar: Visual widget");
    scrolling_section->emplace_child<ui::label>("  - scroll_controller: Sync");

    // Keyboard Navigation
    scrolling_section->emplace_child<ui::label>("");  // Spacer
    scrolling_section->emplace_child<ui::label>("Keyboard Navigation:");
    scrolling_section->emplace_child<ui::label>("  - Arrow Keys: Line scroll");
    scrolling_section->emplace_child<ui::label>("  - Page Up/Down: Page scroll");
    scrolling_section->emplace_child<ui::label>("  - Home/End: Jump to edges");
    scrolling_section->emplace_child<ui::label>("  - Mouse Wheel: Vertical");

    // ========== ROW 2: Demos ==========
    auto* row2 = content->emplace_child<ui::hbox>(onyxui::spacing::medium);

    // ========== NESTED LAYOUTS DEMO (left column) ==========
    auto* nested_section = row2->emplace_child<ui::group_box>();
    nested_section->set_title("Nested Layouts Demo");
    nested_section->set_vbox_layout(onyxui::spacing::tiny);

    nested_section->emplace_child<ui::label>("Outer VBox:");

    auto* nested_inner = nested_section->emplace_child<ui::hbox>(onyxui::spacing::tiny);
    nested_inner->emplace_child<ui::button>("HBox 1");
    nested_inner->emplace_child<ui::button>("HBox 2");
    nested_inner->emplace_child<ui::button>("HBox 3");

    nested_section->emplace_child<ui::label>("VBox continues...");

    // Padding demo
    nested_section->emplace_child<ui::label>("");  // Spacer
    nested_section->emplace_child<ui::label>("Padding Demo:");

    auto* padding_demo = nested_section->emplace_child<ui::panel>();
    padding_demo->set_vbox_layout(onyxui::spacing::tiny);
    padding_demo->set_padding(onyxui::logical_thickness(onyxui::logical_unit(2.0)));
    padding_demo->emplace_child<ui::label>("2px padding all sides");

    // ========== LARGE CONTENT DEMO (right column) ==========
    auto* large_section = row2->emplace_child<ui::group_box>();
    large_section->set_title("Nested Scroll Demo");
    large_section->set_vbox_layout(onyxui::spacing::tiny);

    large_section->emplace_child<ui::label>("100 items in scroll_view:");

    // Inner scroll_view demonstrates nested scrolling
    auto* nested_scroll = large_section->emplace_child<ui::scroll_view>();
    auto* large_content = nested_scroll->content_emplace_child<ui::vbox>(onyxui::spacing::none);

    // Add 100 items to demonstrate scrolling
    for (int i = 1; i <= 100; ++i) {
        large_content->emplace_child<ui::label>("Item " + std::to_string(i));
    }

    // Tips at the bottom
    content->emplace_child<ui::label>("");  // Spacer
    auto* tips = content->emplace_child<ui::label>("Tips: This tab is scrollable! Try the nested scroll demo above.");
    tips->set_horizontal_align(onyxui::horizontal_alignment::center);

    return tab;
}

} // namespace widgets_demo_tabs
