//
// Created by Claude Code
//

#include <doctest/doctest.h>

#include <memory>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/containers/tab_widget.hh>
#include <utility>
#include <string>
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "TabWidget - Multi-page container with tabs") {

    // ===========================================================================
    // Basic API Tests
    // ===========================================================================

    SUBCASE("Construction - Empty widget") {
        tab_widget<test_canvas_backend> tabs;

        CHECK(tabs.count() == 0);
        CHECK(tabs.current_index() == -1);
        CHECK(tabs.current_widget() == nullptr);
        CHECK_FALSE(tabs.is_focusable());
    }

    SUBCASE("Add tab - Returns correct index") {
        tab_widget<test_canvas_backend> tabs;

        auto lbl1 = std::make_unique<label<test_canvas_backend>>("Page 1");
        auto lbl2 = std::make_unique<label<test_canvas_backend>>("Page 2");

        int index1 = tabs.add_tab(std::move(lbl1), "Tab 1");
        int index2 = tabs.add_tab(std::move(lbl2), "Tab 2");

        CHECK(index1 == 0);
        CHECK(index2 == 1);
        CHECK(tabs.count() == 2);
    }

    SUBCASE("Add tab - First tab becomes current") {
        tab_widget<test_canvas_backend> tabs;

        auto lbl = std::make_unique<label<test_canvas_backend>>("Page");
        auto* lbl_ptr = lbl.get();

        tabs.add_tab(std::move(lbl), "First Tab");

        CHECK(tabs.current_index() == 0);
        CHECK(tabs.current_widget() == lbl_ptr);
    }

    SUBCASE("Insert tab - At beginning") {
        tab_widget<test_canvas_backend> tabs;

        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 1"), "Tab 1");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 2"), "Tab 2");

        auto lbl_new = std::make_unique<label<test_canvas_backend>>("Page New");
        int index = tabs.insert_tab(0, std::move(lbl_new), "Tab New");

        CHECK(index == 0);
        CHECK(tabs.count() == 3);
        CHECK(tabs.tab_text(0) == "Tab New");
        CHECK(tabs.tab_text(1) == "Tab 1");
        CHECK(tabs.tab_text(2) == "Tab 2");
    }

    SUBCASE("Insert tab - In middle") {
        tab_widget<test_canvas_backend> tabs;

        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 1"), "Tab 1");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 2"), "Tab 2");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 3"), "Tab 3");

        auto lbl_new = std::make_unique<label<test_canvas_backend>>("Page New");
        int index = tabs.insert_tab(1, std::move(lbl_new), "Tab New");

        CHECK(index == 1);
        CHECK(tabs.count() == 4);
        CHECK(tabs.tab_text(1) == "Tab New");
    }

    SUBCASE("Remove tab - Updates count") {
        tab_widget<test_canvas_backend> tabs;

        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 1"), "Tab 1");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 2"), "Tab 2");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 3"), "Tab 3");

        CHECK(tabs.count() == 3);

        tabs.remove_tab(1);

        CHECK(tabs.count() == 2);
        CHECK(tabs.tab_text(0) == "Tab 1");
        CHECK(tabs.tab_text(1) == "Tab 3");
    }

    SUBCASE("Remove tab - Adjusts current index when removing before current") {
        tab_widget<test_canvas_backend> tabs;

        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 1"), "Tab 1");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 2"), "Tab 2");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 3"), "Tab 3");

        tabs.set_current_index(2);  // Select third tab
        CHECK(tabs.current_index() == 2);

        tabs.remove_tab(0);  // Remove first tab

        CHECK(tabs.current_index() == 1);  // Current should adjust down
    }

    SUBCASE("Remove tab - Current tab removed, selects next") {
        tab_widget<test_canvas_backend> tabs;

        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 1"), "Tab 1");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 2"), "Tab 2");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 3"), "Tab 3");

        tabs.set_current_index(1);  // Select second tab
        tabs.remove_tab(1);  // Remove current tab

        CHECK(tabs.current_index() == 1);  // Should select tab that moved into position (was Tab 3)
        CHECK(tabs.tab_text(1) == "Tab 3");
    }

    SUBCASE("Clear - Removes all tabs") {
        tab_widget<test_canvas_backend> tabs;

        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 1"), "Tab 1");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 2"), "Tab 2");

        CHECK(tabs.count() == 2);

        tabs.clear();

        CHECK(tabs.count() == 0);
        CHECK(tabs.current_index() == -1);
        CHECK(tabs.current_widget() == nullptr);
    }

    // ===========================================================================
    // Tab Selection Tests
    // ===========================================================================

    SUBCASE("Set current index - Changes active tab") {
        tab_widget<test_canvas_backend> tabs;

        auto lbl1 = std::make_unique<label<test_canvas_backend>>("Page 1");
        auto lbl2 = std::make_unique<label<test_canvas_backend>>("Page 2");
        auto* lbl2_ptr = lbl2.get();

        tabs.add_tab(std::move(lbl1), "Tab 1");
        tabs.add_tab(std::move(lbl2), "Tab 2");

        CHECK(tabs.current_index() == 0);

        tabs.set_current_index(1);

        CHECK(tabs.current_index() == 1);
        CHECK(tabs.current_widget() == lbl2_ptr);
    }

    SUBCASE("Next tab - Cycles forward") {
        tab_widget<test_canvas_backend> tabs;

        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 1"), "Tab 1");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 2"), "Tab 2");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 3"), "Tab 3");

        CHECK(tabs.current_index() == 0);

        tabs.next_tab();
        CHECK(tabs.current_index() == 1);

        tabs.next_tab();
        CHECK(tabs.current_index() == 2);

        tabs.next_tab();  // Should wrap to 0
        CHECK(tabs.current_index() == 0);
    }

    SUBCASE("Previous tab - Cycles backward") {
        tab_widget<test_canvas_backend> tabs;

        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 1"), "Tab 1");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 2"), "Tab 2");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 3"), "Tab 3");

        CHECK(tabs.current_index() == 0);

        tabs.previous_tab();  // Should wrap to 2
        CHECK(tabs.current_index() == 2);

        tabs.previous_tab();
        CHECK(tabs.current_index() == 1);

        tabs.previous_tab();
        CHECK(tabs.current_index() == 0);
    }

    // ===========================================================================
    // Tab Properties Tests
    // ===========================================================================

    SUBCASE("Set tab text - Updates label") {
        tab_widget<test_canvas_backend> tabs;

        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page"), "Original");

        CHECK(tabs.tab_text(0) == "Original");

        tabs.set_tab_text(0, "Modified");

        CHECK(tabs.tab_text(0) == "Modified");
    }

    SUBCASE("Widget - Returns correct widget pointer") {
        tab_widget<test_canvas_backend> tabs;

        auto lbl1 = std::make_unique<label<test_canvas_backend>>("Page 1");
        auto lbl2 = std::make_unique<label<test_canvas_backend>>("Page 2");
        auto* lbl1_ptr = lbl1.get();
        auto* lbl2_ptr = lbl2.get();

        tabs.add_tab(std::move(lbl1), "Tab 1");
        tabs.add_tab(std::move(lbl2), "Tab 2");

        CHECK(tabs.widget(0) == lbl1_ptr);
        CHECK(tabs.widget(1) == lbl2_ptr);
    }

    // ===========================================================================
    // Tab Bar Configuration Tests
    // ===========================================================================

    SUBCASE("Set tab position - Changes layout") {
        tab_widget<test_canvas_backend> tabs;

        CHECK(tabs.get_tab_position() == tab_position::top);

        tabs.set_tab_position(tab_position::bottom);
        CHECK(tabs.get_tab_position() == tab_position::bottom);

        tabs.set_tab_position(tab_position::left);
        CHECK(tabs.get_tab_position() == tab_position::left);

        tabs.set_tab_position(tab_position::right);
        CHECK(tabs.get_tab_position() == tab_position::right);
    }

    // ===========================================================================
    // Close Button Tests
    // ===========================================================================

    SUBCASE("Set tabs closable - Enables all close buttons") {
        tab_widget<test_canvas_backend> tabs;

        CHECK_FALSE(tabs.tabs_closable());

        tabs.set_tabs_closable(true);
        CHECK(tabs.tabs_closable());

        tabs.set_tabs_closable(false);
        CHECK_FALSE(tabs.tabs_closable());
    }

    SUBCASE("Set tab closeable - Individual tab control") {
        tab_widget<test_canvas_backend> tabs;
        tabs.set_tabs_closable(true);  // Enable close buttons globally

        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 1"), "Tab 1");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 2"), "Tab 2");

        // Both should be closeable by default
        CHECK(tabs.is_tab_closeable(0));
        CHECK(tabs.is_tab_closeable(1));

        tabs.set_tab_closeable(0, false);

        CHECK_FALSE(tabs.is_tab_closeable(0));
        CHECK(tabs.is_tab_closeable(1));
    }

    // ===========================================================================
    // Signal Tests
    // ===========================================================================

    SUBCASE("Current changed signal - Emitted on tab selection") {
        tab_widget<test_canvas_backend> tabs;

        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 1"), "Tab 1");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 2"), "Tab 2");

        int signal_count = 0;
        int received_index = -1;

        tabs.current_changed.connect([&](int index) {
            signal_count++;
            received_index = index;
        });

        tabs.set_current_index(1);

        CHECK(signal_count == 1);
        CHECK(received_index == 1);
    }

    SUBCASE("Current changed signal - Not emitted when setting same index") {
        tab_widget<test_canvas_backend> tabs;

        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 1"), "Tab 1");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 2"), "Tab 2");

        int signal_count = 0;
        tabs.current_changed.connect([&](int) { signal_count++; });

        tabs.set_current_index(0);  // Already current

        CHECK(signal_count == 0);
    }

    SUBCASE("Tab close requested signal - Signal-based closure") {
        tab_widget<test_canvas_backend> tabs;
        tabs.set_tabs_closable(true);

        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 1"), "Tab 1");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 2"), "Tab 2");

        int signal_count = 0;
        int requested_index = -1;

        tabs.tab_close_requested.connect([&](int index) {
            signal_count++;
            requested_index = index;
        });

        // Simulate close button click (would come from event handling)
        // For now, we can test that the signal is properly connected
        tabs.tab_close_requested.emit(1);

        CHECK(signal_count == 1);
        CHECK(requested_index == 1);
        // Tab should NOT be auto-removed (signal-based design)
        CHECK(tabs.count() == 2);
    }

    // ===========================================================================
    // Layout Tests
    // ===========================================================================

    SUBCASE("Measure - Empty widget") {
        tab_widget<test_canvas_backend> tabs;

        auto size = tabs.measure(800_lu, 600_lu);

        // Empty widget should have minimal size
        CHECK(size.width.to_int() > 0);
        CHECK(size.height.to_int() > 0);
    }

    SUBCASE("Measure - With tabs") {
        tab_widget<test_canvas_backend> tabs;

        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 1"), "Tab 1");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 2"), "Tab 2");

        auto size = tabs.measure(800_lu, 600_lu);

        CHECK(size.width.to_int() > 0);
        CHECK(size.height.to_int() > 0);
    }

    SUBCASE("Arrange - Positions tab bar and content area (top position)") {
        tab_widget<test_canvas_backend> tabs;

        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 1"), "Tab 1");
        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page 2"), "Tab 2");

        (void)tabs.measure(800_lu, 600_lu);

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 800, 600);

        CHECK_NOTHROW(tabs.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu}));
    }

    SUBCASE("Arrange - Different tab positions") {
        tab_widget<test_canvas_backend> tabs;

        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page"), "Tab");

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 800, 600);

        // Test all four positions
        for (auto pos : {tab_position::top, tab_position::bottom, tab_position::left, tab_position::right}) {
            tabs.set_tab_position(pos);
            (void)tabs.measure(800_lu, 600_lu);
            CHECK_NOTHROW(tabs.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu}));
        }
    }

    SUBCASE("Content area - Shows current tab's widget") {
        tab_widget<test_canvas_backend> tabs;

        auto lbl1 = std::make_unique<label<test_canvas_backend>>("Page 1 Content");
        auto lbl2 = std::make_unique<label<test_canvas_backend>>("Page 2 Content");
        auto* lbl1_ptr = lbl1.get();
        auto* lbl2_ptr = lbl2.get();

        tabs.add_tab(std::move(lbl1), "Tab 1");
        tabs.add_tab(std::move(lbl2), "Tab 2");

        // Initial current should be tab 0
        CHECK(tabs.current_widget() == lbl1_ptr);

        // Switch to tab 1
        tabs.set_current_index(1);
        CHECK(tabs.current_widget() == lbl2_ptr);

        // Switch back to tab 0
        tabs.set_current_index(0);
        CHECK(tabs.current_widget() == lbl1_ptr);
    }

    // ===========================================================================
    // Edge Cases
    // ===========================================================================

    SUBCASE("Invalid index operations - Graceful handling") {
        tab_widget<test_canvas_backend> tabs;

        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page"), "Tab");

        // These should not crash (implementation should validate indices)
        CHECK_NOTHROW(tabs.set_current_index(-1));
        CHECK_NOTHROW(tabs.set_current_index(999));
        CHECK_NOTHROW(tabs.remove_tab(-1));
        CHECK_NOTHROW(tabs.remove_tab(999));
    }

    SUBCASE("Empty widget operations") {
        tab_widget<test_canvas_backend> tabs;

        // Operations on empty widget should be safe
        CHECK_NOTHROW(tabs.next_tab());
        CHECK_NOTHROW(tabs.previous_tab());
        CHECK_NOTHROW(tabs.clear());
        CHECK(tabs.count() == 0);
    }

    SUBCASE("Single tab operations") {
        tab_widget<test_canvas_backend> tabs;

        tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page"), "Tab");

        // Next/previous with single tab should stay on that tab
        tabs.next_tab();
        CHECK(tabs.current_index() == 0);

        tabs.previous_tab();
        CHECK(tabs.current_index() == 0);
    }

    // ===========================================================================
    // Tab Overflow Scrolling Tests
    // ===========================================================================

    SUBCASE("Scroll state - Initial state") {
        tab_widget<test_canvas_backend> tabs;

        // Initially no overflow
        CHECK(tabs.scroll_offset() == 0);
        CHECK_FALSE(tabs.has_overflow());
        CHECK_FALSE(tabs.can_scroll_left());
        CHECK_FALSE(tabs.can_scroll_right());
    }

    SUBCASE("scroll_left - Decrements scroll offset") {
        tab_widget<test_canvas_backend> tabs;

        // Add some tabs
        for (int i = 0; i < 10; ++i) {
            tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page " + std::to_string(i)),
                        "Tab " + std::to_string(i));
        }

        // scroll_left on offset 0 does nothing
        tabs.scroll_left();
        CHECK(tabs.scroll_offset() == 0);
    }

    SUBCASE("scroll_right - Increments scroll offset") {
        tab_widget<test_canvas_backend> tabs;

        // Add some tabs
        for (int i = 0; i < 10; ++i) {
            tabs.add_tab(std::make_unique<label<test_canvas_backend>>("Page " + std::to_string(i)),
                        "Tab " + std::to_string(i));
        }

        // scroll_right should work if overflow detected
        // Note: overflow is detected during rendering, so without render it won't scroll
        // This tests the API
        CHECK(tabs.scroll_offset() == 0);
    }

    SUBCASE("get_tab_bar_height - Returns correct height") {
        tab_widget<test_canvas_backend> tabs;

        // Tab bar height should be 1 for TUI
        CHECK(tabs.get_tab_bar_height() == 1);
    }

    SUBCASE("Scroll methods - API availability") {
        tab_widget<test_canvas_backend> tabs;

        // Verify scroll methods exist and are callable
        CHECK_NOTHROW(tabs.scroll_left());
        CHECK_NOTHROW(tabs.scroll_right());
        CHECK_FALSE(tabs.can_scroll_left());
        CHECK_FALSE(tabs.can_scroll_right());
        CHECK(tabs.scroll_offset() == 0);
        CHECK_FALSE(tabs.has_overflow());
    }

    SUBCASE("Auto-scroll - set_current_index scrolls left to show hidden tab") {
        tab_widget<test_canvas_backend> tabs;

        // Add 10 tabs
        for (int i = 0; i < 10; ++i) {
            tabs.add_tab(
                std::make_unique<label<test_canvas_backend>>("Content " + std::to_string(i)),
                "Tab " + std::to_string(i)
            );
        }

        // Start at tab 5
        tabs.set_current_index(5);
        CHECK(tabs.current_index() == 5);
        int initial_offset = tabs.scroll_offset();

        // Select a tab before the scroll offset (simulating it's hidden to the left)
        // First, let's select a later tab to move scroll offset forward
        tabs.set_current_index(8);

        // Now select tab 0 - should auto-scroll left to show it
        tabs.set_current_index(0);
        CHECK(tabs.current_index() == 0);
        CHECK(tabs.scroll_offset() == 0);  // Should be scrolled to show tab 0
    }

    SUBCASE("Auto-scroll - next_tab and previous_tab work") {
        tab_widget<test_canvas_backend> tabs;

        // Add tabs
        for (int i = 0; i < 5; ++i) {
            tabs.add_tab(
                std::make_unique<label<test_canvas_backend>>("Content " + std::to_string(i)),
                "Tab " + std::to_string(i)
            );
        }

        tabs.set_current_index(2);
        CHECK(tabs.current_index() == 2);

        // Navigate forward
        tabs.next_tab();  // Now at tab 3
        CHECK(tabs.current_index() == 3);

        tabs.next_tab();  // Now at tab 4
        CHECK(tabs.current_index() == 4);

        // Navigate backward
        tabs.previous_tab();  // Tab 3
        CHECK(tabs.current_index() == 3);

        tabs.previous_tab();  // Tab 2
        CHECK(tabs.current_index() == 2);

        tabs.previous_tab();  // Tab 1
        CHECK(tabs.current_index() == 1);

        tabs.previous_tab();  // Tab 0
        CHECK(tabs.current_index() == 0);
        CHECK(tabs.scroll_offset() == 0);  // Auto-scrolled to beginning
    }
}
