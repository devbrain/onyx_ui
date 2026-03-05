//
// Created by Claude Code on 2025-11-23.
//
// Tab 2: Layout & Scrolling - Layout system and scrolling demonstrations
// Shows layout algorithm, alignment, sizing, and scrolling features
//

#pragma once
#include "../backend_config.hh"  // Defines concrete Backend type
#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/containers/hbox.hh>
#include <onyxui/widgets/containers/grid.hh>
#include <onyxui/widgets/containers/group_box.hh>
#include <onyxui/widgets/containers/scroll_view.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/layout/spacer.hh>
#include <onyxui/widgets/layout/spring.hh>
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
inline std::unique_ptr<onyxui::panel<Backend>> create_tab_layout_scrolling() {
    auto tab = std::make_unique<onyxui::panel<Backend>>();

    // Make entire tab scrollable
    auto* scroll = tab->emplace_child<onyxui::scroll_view>();

    // Main content container (vertical layout)
    auto* content = scroll->emplace_child<onyxui::vbox>(onyxui::spacing::tiny);  // 1px spacing

    // Title
    auto* title = content->emplace_child<onyxui::label>("Layout & Scrolling System");
    title->set_horizontal_align(onyxui::horizontal_alignment::center);

    content->emplace_child<onyxui::label>("");  // Spacer

    // ========== ROW 1: Layout System | Scrolling System ==========
    auto* row1 = content->emplace_child<onyxui::hbox>(onyxui::spacing::medium);

    // ========== LAYOUT SECTION (left column) ==========
    auto* layout_section = row1->emplace_child<onyxui::group_box>();
    layout_section->set_title("Layout System");
    layout_section->set_vbox_layout(onyxui::spacing::tiny);

    // Two-Pass Algorithm
    layout_section->emplace_child<onyxui::label>("Two-Pass Layout Algorithm:");
    layout_section->emplace_child<onyxui::label>("  1. Measure Pass (bottom-up)");
    layout_section->emplace_child<onyxui::label>("  2. Arrange Pass (top-down)");

    // Horizontal Alignment
    layout_section->emplace_child<onyxui::label>("");  // Spacer
    layout_section->emplace_child<onyxui::label>("Horizontal Alignment:");

    auto* left_label = layout_section->emplace_child<onyxui::label>("[Left]");
    left_label->set_horizontal_align(onyxui::horizontal_alignment::left);

    auto* center_label = layout_section->emplace_child<onyxui::label>("[Center]");
    center_label->set_horizontal_align(onyxui::horizontal_alignment::center);

    auto* right_label = layout_section->emplace_child<onyxui::label>("[Right]");
    right_label->set_horizontal_align(onyxui::horizontal_alignment::right);

    // Vertical Alignment
    layout_section->emplace_child<onyxui::label>("");  // Spacer
    layout_section->emplace_child<onyxui::label>("Vertical Alignment:");

    auto* valign_container = layout_section->emplace_child<onyxui::hbox>(onyxui::spacing::tiny);
    auto* top_btn = valign_container->emplace_child<onyxui::button>("[Top]");
    top_btn->set_vertical_align(onyxui::vertical_alignment::top);
    auto* vcenter_btn = valign_container->emplace_child<onyxui::button>("[Ctr]");
    vcenter_btn->set_vertical_align(onyxui::vertical_alignment::center);
    auto* bottom_btn = valign_container->emplace_child<onyxui::button>("[Bot]");
    bottom_btn->set_vertical_align(onyxui::vertical_alignment::bottom);

    // Spacer and Spring
    layout_section->emplace_child<onyxui::label>("");  // Spacer
    layout_section->emplace_child<onyxui::label>("Spacer vs Spring:");

    auto* spacer_demo = layout_section->emplace_child<onyxui::hbox>(onyxui::spacing::none);
    spacer_demo->emplace_child<onyxui::label>("[L]");
    spacer_demo->emplace_child<onyxui::spacer>(5);
    spacer_demo->emplace_child<onyxui::label>("[5]");
    spacer_demo->emplace_child<onyxui::spring>();
    spacer_demo->emplace_child<onyxui::label>("[R]");

    // ========== SCROLLING SECTION (right column) ==========
    auto* scrolling_section = row1->emplace_child<onyxui::group_box>();
    scrolling_section->set_title("Scrolling System");
    scrolling_section->set_vbox_layout(onyxui::spacing::tiny);

    scrolling_section->emplace_child<onyxui::label>("Scrolling Architecture:");
    scrolling_section->emplace_child<onyxui::label>("  - scroll_view: Container");
    scrolling_section->emplace_child<onyxui::label>("  - scrollable: Viewport");
    scrolling_section->emplace_child<onyxui::label>("  - scrollbar: Visual widget");
    scrolling_section->emplace_child<onyxui::label>("  - scroll_controller: Sync");

    // Keyboard Navigation
    scrolling_section->emplace_child<onyxui::label>("");  // Spacer
    scrolling_section->emplace_child<onyxui::label>("Keyboard Navigation:");
    scrolling_section->emplace_child<onyxui::label>("  - Arrow Keys: Line scroll");
    scrolling_section->emplace_child<onyxui::label>("  - Page Up/Down: Page scroll");
    scrolling_section->emplace_child<onyxui::label>("  - Home/End: Jump to edges");
    scrolling_section->emplace_child<onyxui::label>("  - Mouse Wheel: Vertical");

    // ========== ROW 2: Demos ==========
    auto* row2 = content->emplace_child<onyxui::hbox>(onyxui::spacing::medium);

    // ========== NESTED LAYOUTS DEMO (left column) ==========
    auto* nested_section = row2->emplace_child<onyxui::group_box>();
    nested_section->set_title("Nested Layouts Demo");
    nested_section->set_vbox_layout(onyxui::spacing::tiny);

    nested_section->emplace_child<onyxui::label>("Outer VBox:");

    auto* nested_inner = nested_section->emplace_child<onyxui::hbox>(onyxui::spacing::tiny);
    nested_inner->emplace_child<onyxui::button>("HBox 1");
    nested_inner->emplace_child<onyxui::button>("HBox 2");
    nested_inner->emplace_child<onyxui::button>("HBox 3");

    nested_section->emplace_child<onyxui::label>("VBox continues...");

    // Padding demo
    nested_section->emplace_child<onyxui::label>("");  // Spacer
    nested_section->emplace_child<onyxui::label>("Padding Demo:");

    auto* padding_demo = nested_section->emplace_child<onyxui::panel>();
    padding_demo->set_vbox_layout(onyxui::spacing::tiny);
    padding_demo->set_padding(onyxui::logical_thickness(onyxui::logical_unit(2.0)));
    padding_demo->emplace_child<onyxui::label>("2px padding all sides");

    // ========== LARGE CONTENT DEMO (right column) ==========
    auto* large_section = row2->emplace_child<onyxui::group_box>();
    large_section->set_title("Nested Scroll Demo");
    large_section->set_vbox_layout(onyxui::spacing::tiny);

    large_section->emplace_child<onyxui::label>("100 items in scroll_view:");

    // Inner scroll_view demonstrates nested scrolling
    auto* nested_scroll = large_section->emplace_child<onyxui::scroll_view>();
    auto* large_content = nested_scroll->emplace_child<onyxui::vbox>(onyxui::spacing::none);

    // Add 100 items to demonstrate scrolling
    for (int i = 1; i <= 100; ++i) {
        large_content->emplace_child<onyxui::label>("Item " + std::to_string(i));
    }

    // Tips at the bottom
    content->emplace_child<onyxui::label>("");  // Spacer
    auto* tips = content->emplace_child<onyxui::label>("Tips: This tab is scrollable! Try the nested scroll demo above.");
    tips->set_horizontal_align(onyxui::horizontal_alignment::center);

    return tab;
}

} // namespace widgets_demo_tabs
