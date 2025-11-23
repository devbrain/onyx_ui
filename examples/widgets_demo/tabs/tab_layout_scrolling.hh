//
// Created by Claude Code on 2025-11-23.
//
// Tab 2: Layout & Scrolling - Layout system and scrolling demonstrations
// Shows layout algorithm, alignment, sizing, and scrolling features
//

#pragma once
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
 * @tparam Backend UI backend type
 * @return Panel containing the complete tab content
 */
template<onyxui::UIBackend Backend>
std::unique_ptr<onyxui::panel<Backend>> create_tab_layout_scrolling() {
    auto tab = std::make_unique<onyxui::panel<Backend>>();

    // Make entire tab scrollable
    auto* scroll = tab->template emplace_child<onyxui::scroll_view>();

    // Main content container (vertical layout)
    auto* content = scroll->template emplace_child<onyxui::vbox>(1);  // 1px spacing

    // Title
    auto* title = content->template emplace_child<onyxui::label>("Layout & Scrolling System");
    title->set_horizontal_align(onyxui::horizontal_alignment::center);

    content->template emplace_child<onyxui::label>("");  // Spacer

    // ========== LAYOUT SECTION ==========
    auto* layout_section = content->template emplace_child<onyxui::group_box>();
    layout_section->set_title("Layout System");
    layout_section->set_vbox_layout(1);

    // Two-Pass Algorithm
    layout_section->template emplace_child<onyxui::label>("Two-Pass Layout Algorithm:");
    layout_section->template emplace_child<onyxui::label>("  1. Measure Pass (bottom-up): Calculates desired sizes");
    layout_section->template emplace_child<onyxui::label>("  2. Arrange Pass (top-down): Assigns final positions");

    // Horizontal Alignment
    layout_section->template emplace_child<onyxui::label>("");  // Spacer
    layout_section->template emplace_child<onyxui::label>("Horizontal Alignment:");

    auto* left_label = layout_section->template emplace_child<onyxui::label>("[Left Aligned]");
    left_label->set_horizontal_align(onyxui::horizontal_alignment::left);

    auto* center_label = layout_section->template emplace_child<onyxui::label>("[Center Aligned]");
    center_label->set_horizontal_align(onyxui::horizontal_alignment::center);

    auto* right_label = layout_section->template emplace_child<onyxui::label>("[Right Aligned]");
    right_label->set_horizontal_align(onyxui::horizontal_alignment::right);

    auto* stretch_label = layout_section->template emplace_child<onyxui::label>("[Stretch - Fills Width]");
    stretch_label->set_horizontal_align(onyxui::horizontal_alignment::stretch);

    // Vertical Alignment (in a panel with fixed height)
    layout_section->template emplace_child<onyxui::label>("");  // Spacer
    layout_section->template emplace_child<onyxui::label>("Vertical Alignment (in container):");

    auto* valign_container = layout_section->template emplace_child<onyxui::hbox>(2);

    auto* top_btn = valign_container->template emplace_child<onyxui::button>("[Top]");
    top_btn->set_vertical_align(onyxui::vertical_alignment::top);

    auto* vcenter_btn = valign_container->template emplace_child<onyxui::button>("[Center]");
    vcenter_btn->set_vertical_align(onyxui::vertical_alignment::center);

    auto* bottom_btn = valign_container->template emplace_child<onyxui::button>("[Bottom]");
    bottom_btn->set_vertical_align(onyxui::vertical_alignment::bottom);

    auto* vstretch_btn = valign_container->template emplace_child<onyxui::button>("[Stretch]");
    vstretch_btn->set_vertical_align(onyxui::vertical_alignment::stretch);

    // Padding Examples
    layout_section->template emplace_child<onyxui::label>("");  // Spacer
    layout_section->template emplace_child<onyxui::label>("Padding (Thickness):");

    auto* padding_demo = layout_section->template emplace_child<onyxui::panel>();
    padding_demo->set_vbox_layout(1);
    padding_demo->set_padding(onyxui::thickness::all(2));  // 2px padding on all sides
    padding_demo->template emplace_child<onyxui::label>("This panel has 2px padding on all sides");
    padding_demo->template emplace_child<onyxui::label>("Content is inset from the edges");

    // Spacer and Spring
    layout_section->template emplace_child<onyxui::label>("");  // Spacer
    layout_section->template emplace_child<onyxui::label>("Spacer vs Spring:");

    auto* spacer_demo = layout_section->template emplace_child<onyxui::hbox>(0);
    spacer_demo->template emplace_child<onyxui::label>("[Left]");
    spacer_demo->template emplace_child<onyxui::spacer>(10);  // Fixed 10 char space
    spacer_demo->template emplace_child<onyxui::label>("[Fixed 10]");
    spacer_demo->template emplace_child<onyxui::spring>();  // Flexible space (fills remaining)
    spacer_demo->template emplace_child<onyxui::label>("[Right with Spring]");

    // Nested Layouts
    layout_section->template emplace_child<onyxui::label>("");  // Spacer
    layout_section->template emplace_child<onyxui::label>("Nested Layouts:");

    auto* nested_outer = layout_section->template emplace_child<onyxui::vbox>(1);
    nested_outer->template emplace_child<onyxui::label>("Outer VBox:");

    auto* nested_inner = nested_outer->template emplace_child<onyxui::hbox>(2);
    nested_inner->template emplace_child<onyxui::button>("Inner HBox 1");
    nested_inner->template emplace_child<onyxui::button>("Inner HBox 2");
    nested_inner->template emplace_child<onyxui::button>("Inner HBox 3");

    nested_outer->template emplace_child<onyxui::label>("VBox continues after HBox");

    // ========== SCROLLING SECTION ==========
    auto* scrolling_section = content->template emplace_child<onyxui::group_box>();
    scrolling_section->set_title("Scrolling System");
    scrolling_section->set_vbox_layout(1);

    scrolling_section->template emplace_child<onyxui::label>("Scrolling Architecture:");
    scrolling_section->template emplace_child<onyxui::label>("  - scroll_view: Container with automatic scrollbars");
    scrolling_section->template emplace_child<onyxui::label>("  - scrollable: Viewport for content");
    scrolling_section->template emplace_child<onyxui::label>("  - scrollbar: Visual scrollbar widget");
    scrolling_section->template emplace_child<onyxui::label>("  - scroll_controller: Bidirectional sync");

    // Keyboard Navigation
    scrolling_section->template emplace_child<onyxui::label>("");  // Spacer
    scrolling_section->template emplace_child<onyxui::label>("Keyboard Navigation:");
    scrolling_section->template emplace_child<onyxui::label>("  - Arrow Keys: Scroll line by line");
    scrolling_section->template emplace_child<onyxui::label>("  - Page Up/Down: Scroll by page");
    scrolling_section->template emplace_child<onyxui::label>("  - Home/End: Jump to top/bottom");
    scrolling_section->template emplace_child<onyxui::label>("  - Mouse Wheel: Scroll vertically");

    // Large Content Scrolling Demo
    scrolling_section->template emplace_child<onyxui::label>("");  // Spacer
    scrolling_section->template emplace_child<onyxui::label>("Large Content Demo (100 items):");

    auto* large_scroll = scrolling_section->template emplace_child<onyxui::scroll_view>();
    auto* large_content = large_scroll->template emplace_child<onyxui::vbox>(0);

    // Add 100 items to demonstrate scrolling
    for (int i = 1; i <= 100; ++i) {
        std::string item_text = "Item " + std::to_string(i);
        if (i % 10 == 0) {
            item_text += " (milestone)";
        }
        large_content->template emplace_child<onyxui::label>(item_text);
    }

    // Scrolling Tips
    scrolling_section->template emplace_child<onyxui::label>("");  // Spacer
    scrolling_section->template emplace_child<onyxui::label>("Tips:");
    scrolling_section->template emplace_child<onyxui::label>("  - This entire tab is scrollable!");
    scrolling_section->template emplace_child<onyxui::label>("  - Try scrolling the large content demo above");
    scrolling_section->template emplace_child<onyxui::label>("  - Nested scrolling is supported");

    return tab;
}

} // namespace widgets_demo_tabs
