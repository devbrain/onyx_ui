/**
 * @file test_text_view_keyboard.cc
 * @brief Tests for text_view keyboard scrolling functionality
 */

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include "onyxui/widgets/text_view.hh"
#include "onyxui/widgets/containers/scroll_view.hh"
#include "onyxui/concepts/point_like.hh"
#include "onyxui/services/ui_context.hh"
#include "onyxui/hotkeys/hotkey_action.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Keyboard scrolling with semantic actions") {
    using Backend = test_canvas_backend;

    SUBCASE("text_view should be focusable") {
        auto text_view_widget = std::make_unique<text_view<Backend>>();

        // Add content that requires scrolling
        std::string content;
        for (int i = 1; i <= 20; i++) {
            content += "Line " + std::to_string(i) + "\n";
        }
        text_view_widget->set_text(content);

        // Measure and arrange
        auto size = text_view_widget->measure(40_lu, 10_lu);
        (void)size; // Suppress unused warning
        text_view_widget->arrange(logical_rect{0_lu, 0_lu, 40_lu, 10_lu});

        // text_view itself should be focusable for keyboard scrolling
        REQUIRE(text_view_widget->is_focusable());
    }

    SUBCASE("Giving focus to text_view programmatically") {
        auto text_view_widget = std::make_unique<text_view<Backend>>();

        // Add content
        text_view_widget->set_text("Line 1\nLine 2\nLine 3\n");
        auto size = text_view_widget->measure(40_lu, 10_lu);
        (void)size;
        text_view_widget->arrange(logical_rect{0_lu, 0_lu, 40_lu, 10_lu});

        // Give focus to text_view directly
        auto* input = ui_services<Backend>::input();
        REQUIRE(input != nullptr);
        bool focused = input->set_focus(text_view_widget.get());
        REQUIRE(focused);

        // Verify focus was set
        REQUIRE(text_view_widget->has_focus());
    }

    SUBCASE("Keyboard scrolling via semantic actions") {
        auto text_view_widget = std::make_unique<text_view<Backend>>();

        // Add scrollable content (20 lines, viewport shows ~8)
        std::string content;
        for (int i = 1; i <= 20; i++) {
            content += "Line " + std::to_string(i) + "\n";
        }
        text_view_widget->set_text(content);

        // Measure and arrange with limited height to force scrolling
        auto size = text_view_widget->measure(40_lu, 8_lu);
        (void)size;
        text_view_widget->arrange(logical_rect{0_lu, 0_lu, 40_lu, 8_lu});

        // Give focus to text_view directly (NEW ARCHITECTURE)
        auto* input = ui_services<Backend>::input();
        input->set_focus(text_view_widget.get());
        REQUIRE(text_view_widget->has_focus());

        // Get internal scroll_view and its scrollable content to check scroll position
        auto* scroll_view_child = text_view_widget->children()[0].get();
        auto* sv = dynamic_cast<onyxui::scroll_view<Backend>*>(scroll_view_child);
        REQUIRE(sv != nullptr);
        auto* scrollable = sv->content();
        REQUIRE(scrollable != nullptr);

        auto initial_offset = scrollable->get_scroll_offset();
        int initial_scroll_y = point_utils::get_y(initial_offset);
        REQUIRE(initial_scroll_y == 0);  // Should start at top

        // Simulate scroll_down semantic action ON TEXT_VIEW (not scroll_view)
        bool handled = text_view_widget->handle_semantic_action(hotkey_action::scroll_down);
        REQUIRE(handled);

        // Verify scroll position changed
        auto after_offset = scrollable->get_scroll_offset();
        int after_scroll_y = point_utils::get_y(after_offset);
        REQUIRE(after_scroll_y > initial_scroll_y);  // Should have scrolled down

        // Simulate scroll_up semantic action
        handled = text_view_widget->handle_semantic_action(hotkey_action::scroll_up);
        REQUIRE(handled);

        // Verify we scrolled back up
        auto final_offset = scrollable->get_scroll_offset();
        int final_scroll_y = point_utils::get_y(final_offset);
        REQUIRE(final_scroll_y == initial_scroll_y);  // Back to top
    }

    SUBCASE("PageDown/PageUp keyboard scrolling") {
        auto text_view_widget = std::make_unique<text_view<Backend>>();

        std::string content;
        for (int i = 1; i <= 30; i++) {
            content += "Line " + std::to_string(i) + "\n";
        }
        text_view_widget->set_text(content);

        auto size = text_view_widget->measure(40_lu, 8_lu);
        (void)size;
        text_view_widget->arrange(logical_rect{0_lu, 0_lu, 40_lu, 8_lu});

        // Give focus to text_view directly (NEW ARCHITECTURE)
        auto* input = ui_services<Backend>::input();
        input->set_focus(text_view_widget.get());
        REQUIRE(text_view_widget->has_focus());

        // Get internal scroll_view and its scrollable content
        auto* scroll_view_child = text_view_widget->children()[0].get();
        auto* sv = dynamic_cast<onyxui::scroll_view<Backend>*>(scroll_view_child);
        REQUIRE(sv != nullptr);
        auto* scrollable = sv->content();
        REQUIRE(scrollable != nullptr);

        auto initial_offset = scrollable->get_scroll_offset();
        int initial_scroll = point_utils::get_y(initial_offset);

        // Page down should scroll by viewport height
        text_view_widget->handle_semantic_action(hotkey_action::scroll_page_down);
        auto after_page_down_offset = scrollable->get_scroll_offset();
        int after_page_down = point_utils::get_y(after_page_down_offset);

        REQUIRE(after_page_down > initial_scroll);
        int page_size = after_page_down - initial_scroll;
        REQUIRE(page_size >= 5);  // Should scroll by at least viewport height

        // Page up should scroll back
        text_view_widget->handle_semantic_action(hotkey_action::scroll_page_up);
        auto after_page_up_offset = scrollable->get_scroll_offset();
        int after_page_up = point_utils::get_y(after_page_up_offset);

        REQUIRE(after_page_up == initial_scroll);  // Back to start
    }

    SUBCASE("Home/End keyboard scrolling") {
        auto text_view_widget = std::make_unique<text_view<Backend>>();

        std::string content;
        for (int i = 1; i <= 30; i++) {
            content += "Line " + std::to_string(i) + "\n";
        }
        text_view_widget->set_text(content);

        auto size = text_view_widget->measure(40_lu, 8_lu);
        (void)size;
        text_view_widget->arrange(logical_rect{0_lu, 0_lu, 40_lu, 8_lu});

        // Give focus to text_view directly (NEW ARCHITECTURE)
        auto* input = ui_services<Backend>::input();
        input->set_focus(text_view_widget.get());
        REQUIRE(text_view_widget->has_focus());

        // Get internal scroll_view and its scrollable content
        auto* scroll_view_child = text_view_widget->children()[0].get();
        auto* sv = dynamic_cast<onyxui::scroll_view<Backend>*>(scroll_view_child);
        REQUIRE(sv != nullptr);
        auto* scrollable = sv->content();
        REQUIRE(scrollable != nullptr);

        // Start at top
        auto initial_offset = scrollable->get_scroll_offset();
        int initial_y = point_utils::get_y(initial_offset);
        REQUIRE(initial_y == 0);

        // End should jump to bottom
        text_view_widget->handle_semantic_action(hotkey_action::scroll_end);
        auto at_end_offset = scrollable->get_scroll_offset();
        int at_end = point_utils::get_y(at_end_offset);
        REQUIRE(at_end > 0);  // Should be scrolled down

        // Home should jump to top
        text_view_widget->handle_semantic_action(hotkey_action::scroll_home);
        auto at_home_offset = scrollable->get_scroll_offset();
        int at_home = point_utils::get_y(at_home_offset);
        REQUIRE(at_home == 0);  // Back to top
    }
}
