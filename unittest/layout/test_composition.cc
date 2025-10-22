/**
 * @file test_composition.cc
 * @brief Unit tests for layout composition (nested layouts)
 *
 * Tests various combinations of layout strategies to ensure they work
 * correctly when nested within each other. This covers real-world scenarios
 * like dashboards, forms, and application windows.
 */

#include "onyxui/layout_strategy.hh"
#include "utils/test_helpers.hh"
#include <memory>
#include <onyxui/layout/linear_layout.hh>
#include <onyxui/layout/grid_layout.hh>
#include <onyxui/layout/anchor_layout.hh>
#include <onyxui/layout/absolute_layout.hh>
#include <utility>

using TestLinearLayout = linear_layout<TestBackend>;
using TestGridLayout = grid_layout<TestBackend>;
using TestAnchorLayout = anchor_layout<TestBackend>;
using TestAbsoluteLayout = absolute_layout<TestBackend>;

TEST_SUITE("layout_composition") {
    /**
     * Test Pattern: Vertical container with horizontal rows
     * Use Case: Toolbar panels, menu bars, button groups
     */
    TEST_CASE("Vertical with horizontal rows") {
        // Root: Vertical layout
        auto panel = std::make_unique<TestElement>();
        panel->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::vertical, 10));

        // Row 1: Horizontal buttons (fixed sizes)
        auto row1 = std::make_unique<TestElement>();
        row1->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::horizontal, 5));

        for (int i = 0; i < 3; i++) {
            auto btn = std::make_unique<TestElement>();
            btn->set_width_constraint({size_policy::fixed, 80, 80});
            btn->set_height_constraint({size_policy::fixed, 30, 30});
            row1->add_test_child(std::move(btn));
        }
        panel->add_test_child(std::move(row1));

        // Row 2: Horizontal buttons (expanding)
        auto row2 = std::make_unique<TestElement>();
        row2->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::horizontal, 5));

        for (int i = 0; i < 2; i++) {
            auto btn = std::make_unique<TestElement>();
            btn->set_width_constraint({size_policy::expand});
            btn->set_height_constraint({size_policy::fixed, 30, 30});
            row2->add_test_child(std::move(btn));
        }
        panel->add_test_child(std::move(row2));

        // Measure and arrange
        TestSize const measured = panel->measure(300, 200);
        CHECK(measured.w == 250);  // 3*80 + 2*5 = 250
        CHECK(measured.h == 70);  // 30 + 10 + 30 = 70

        panel->arrange({0, 0, 300, 200});

        // Verify row 1 positions
        auto r1 = panel->child_at(0);
        CHECK(r1->bounds().x == 0);
        CHECK(r1->bounds().y == 0);
        CHECK(r1->bounds().h == 30);

        // Verify row 1's children
        CHECK(r1->child_at(0)->bounds().x == 0);
        CHECK(r1->child_at(0)->bounds().w == 80);
        CHECK(r1->child_at(1)->bounds().x == 85);  // 80 + 5
        CHECK(r1->child_at(2)->bounds().x == 170); // 80 + 5 + 80 + 5

        // Verify row 2 positions
        auto r2 = panel->child_at(1);
        CHECK(r2->bounds().x == 0);
        CHECK(r2->bounds().y == 40);  // 30 + 10 spacing
        CHECK(r2->bounds().h == 30);

        // Verify row 2's expanding children
        CHECK(r2->child_at(0)->bounds().w == 148);  // (300-5)/2, first gets extra
        CHECK(r2->child_at(1)->bounds().w == 147);
    }

    /**
     * Test Pattern: Horizontal container with vertical columns
     * Use Case: Multi-column layouts, side-by-side panels
     */
    TEST_CASE("Horizontal with vertical columns") {
        // Root: Horizontal layout
        auto container = std::make_unique<TestElement>();
        container->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::horizontal, 15));

        // Column 1: Vertical list (fixed width)
        auto col1 = std::make_unique<TestElement>();
        col1->set_width_constraint({size_policy::fixed, 150, 150});
        col1->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::vertical, 5));

        for (int i = 0; i < 3; i++) {
            auto item = std::make_unique<TestElement>();
            item->set_height_constraint({size_policy::fixed, 40, 40});
            col1->add_test_child(std::move(item));
        }
        container->add_test_child(std::move(col1));

        // Column 2: Vertical list (expanding width)
        auto col2 = std::make_unique<TestElement>();
        col2->set_width_constraint({size_policy::expand});
        col2->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::vertical, 5));

        for (int i = 0; i < 2; i++) {
            auto item = std::make_unique<TestElement>();
            item->set_height_constraint({size_policy::fixed, 40, 40});
            col2->add_test_child(std::move(item));
        }
        container->add_test_child(std::move(col2));

        // Measure and arrange
        TestSize const measured = container->measure(400, 300);
        CHECK(measured.w == 165);  // 150 + 15 (col2 has no measured width yet)
        CHECK(measured.h == 130);  // Max of columns: 3*40 + 2*5

        container->arrange({0, 0, 400, 300});

        // Verify column 1
        auto c1 = container->child_at(0);
        CHECK(c1->bounds().x == 0);
        CHECK(c1->bounds().w == 150);
        CHECK(c1->child_at(0)->bounds().y == 0);
        CHECK(c1->child_at(1)->bounds().y == 45);  // 40 + 5
        CHECK(c1->child_at(2)->bounds().y == 90);  // 40 + 5 + 40 + 5

        // Verify column 2 (should expand to fill remaining space)
        auto c2 = container->child_at(1);
        CHECK(c2->bounds().x == 165);  // 150 + 15
        CHECK(c2->bounds().w == 235);  // 400 - 150 - 15
        CHECK(c2->child_at(0)->bounds().y == 0);
        CHECK(c2->child_at(1)->bounds().y == 45);
    }

    /**
     * Test Pattern: Vertical with grid rows
     * Use Case: Dashboard layouts, galleries with multiple rows
     */
    TEST_CASE("Vertical with grid sections") {
        // Root: Vertical layout
        auto dashboard = std::make_unique<TestElement>();
        dashboard->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::vertical, 20));

        // Header: Single element
        auto header = std::make_unique<TestElement>();
        header->set_height_constraint({size_policy::fixed, 60, 60});
        dashboard->add_test_child(std::move(header));

        // Grid: 2x2 grid of cards
        auto grid = std::make_unique<TestElement>();
        auto grid_strategy = std::make_unique<TestGridLayout>(2, 2, 10, 10);
        grid->set_layout_strategy(std::move(grid_strategy));

        for (int i = 0; i < 4; i++) {
            auto card = std::make_unique<TestElement>();
            card->set_width_constraint({size_policy::expand});
            card->set_height_constraint({size_policy::fixed, 100, 100});
            grid->add_test_child(std::move(card));
        }
        dashboard->add_test_child(std::move(grid));

        // Measure and arrange
        auto measured = dashboard->measure(500, 600);
        CHECK(measured.w <= 500);
        CHECK(measured.h > 0);
        dashboard->arrange({0, 0, 500, 600});

        // Verify header
        auto hdr = dashboard->child_at(0);
        CHECK(hdr->bounds().y == 0);
        CHECK(hdr->bounds().h == 60);

        // Verify grid positioning
        auto g = dashboard->child_at(1);
        CHECK(g->bounds().y == 80);  // 60 + 20 spacing
        CHECK(g->bounds().w == 500);

        // Verify grid has all 4 children arranged
        CHECK(g->child_count() == 4);
        // Verify all children have valid heights (expand width may be 0 in measure)
        CHECK(g->child_at(0)->bounds().h == 100);
        CHECK(g->child_at(1)->bounds().h == 100);
        CHECK(g->child_at(2)->bounds().h == 100);
        CHECK(g->child_at(3)->bounds().h == 100);

        // Verify grid structure - should be in 2x2 arrangement
        CHECK(g->child_at(0)->bounds().y == g->child_at(1)->bounds().y);  // Row 1
        CHECK(g->child_at(2)->bounds().y == g->child_at(3)->bounds().y);  // Row 2
        CHECK(g->child_at(0)->bounds().x == g->child_at(2)->bounds().x);  // Col 1
        CHECK(g->child_at(1)->bounds().x == g->child_at(3)->bounds().x);  // Col 2
    }

    /**
     * Test Pattern: Grid with nested vertical layouts
     * Use Case: Form layouts, settings panels
     */
    TEST_CASE("Grid with vertical form fields") {
        // Root: Grid layout (2 columns)
        auto form = std::make_unique<TestElement>();
        auto grid_strategy = std::make_unique<TestGridLayout>(2, 0, 20, 15);
        form->set_layout_strategy(std::move(grid_strategy));

        // Label-Input pair 1 (vertical stack in cell 0,0)
        auto field1 = std::make_unique<TestElement>();
        field1->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::vertical, 5));
        auto label1 = std::make_unique<ContentElement>(80, 20);
        auto input1 = std::make_unique<TestElement>();
        input1->set_height_constraint({size_policy::fixed, 30, 30});
        field1->add_test_child(std::move(label1));
        field1->add_test_child(std::move(input1));
        form->add_test_child(std::move(field1));

        // Label-Input pair 2 (vertical stack in cell 1,0)
        auto field2 = std::make_unique<TestElement>();
        field2->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::vertical, 5));
        auto label2 = std::make_unique<ContentElement>(80, 20);
        auto input2 = std::make_unique<TestElement>();
        input2->set_height_constraint({size_policy::fixed, 30, 30});
        field2->add_test_child(std::move(label2));
        field2->add_test_child(std::move(input2));
        form->add_test_child(std::move(field2));

        // Measure and arrange
        TestSize const measured = form->measure(400, 300);
        CHECK(measured.w >= 160);  // At least 2 columns worth
        CHECK(measured.h == 55);  // 20 + 5 + 30

        form->arrange({0, 0, 400, 300});

        // Verify field1 (cell 0,0)
        auto f1 = form->child_at(0);
        CHECK(f1->bounds().x == 0);
        CHECK(f1->bounds().y == 0);
        CHECK(f1->child_at(0)->bounds().y == 0);  // label
        CHECK(f1->child_at(1)->bounds().y == 25); // input: 20 + 5

        // Verify field2 (cell 1,0) - auto-assigned to next column
        auto f2 = form->child_at(1);
        CHECK(f2->bounds().x > 80);  // Should be in second column
        CHECK(f2->bounds().y == 0);
        CHECK(f2->child_at(0)->bounds().y == 0);
        CHECK(f2->child_at(1)->bounds().y == 25);
    }

    /**
     * Test Pattern: Anchor layout with linear children
     * Use Case: Application windows, dialog boxes
     */
    TEST_CASE("Anchor with linear toolbar and content") {
        // Root: Anchor layout
        auto window = std::make_unique<TestElement>();
        auto anchor_strategy = std::make_unique<TestAnchorLayout>();

        // Toolbar: Horizontal layout at top
        auto toolbar = std::make_unique<TestElement>();
        toolbar->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::horizontal, 5));
        toolbar->set_height_constraint({size_policy::fixed, 40, 40});

        for (int i = 0; i < 3; i++) {
            auto btn = std::make_unique<TestElement>();
            btn->set_width_constraint({size_policy::fixed, 60, 60});
            btn->set_height_constraint({size_policy::fixed, 30, 30});
            toolbar->add_test_child(std::move(btn));
        }

        anchor_strategy->set_anchor(toolbar.get(), anchor_point::top_center);
        window->add_test_child(std::move(toolbar));

        // Sidebar: Vertical layout at left
        auto sidebar = std::make_unique<TestElement>();
        sidebar->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::vertical, 3));
        sidebar->set_width_constraint({size_policy::fixed, 100, 100});

        for (int i = 0; i < 3; i++) {
            auto item = std::make_unique<TestElement>();
            item->set_height_constraint({size_policy::fixed, 35, 35});
            sidebar->add_test_child(std::move(item));
        }

        anchor_strategy->set_anchor(sidebar.get(), anchor_point::center_left);
        window->add_test_child(std::move(sidebar));

        window->set_layout_strategy(std::move(anchor_strategy));

        // Measure and arrange
        auto measured = window->measure(600, 400);
        CHECK(measured.w <= 600);
        CHECK(measured.h <= 400);
        window->arrange({0, 0, 600, 400});

        // Verify toolbar (anchored at top_center)
        auto tb = window->child_at(0);
        // Toolbar width: 3*60 + 2*5 = 190
        // Centered: (600 - 190) / 2 = 205
        CHECK(tb->bounds().x == 205);
        CHECK(tb->bounds().y == 0);
        CHECK(tb->bounds().h == 40);

        // Verify toolbar's buttons
        CHECK(tb->child_at(0)->bounds().x == 205);
        CHECK(tb->child_at(1)->bounds().x == 270);  // 205 + 60 + 5
        CHECK(tb->child_at(2)->bounds().x == 335);  // 205 + 60 + 5 + 60 + 5

        // Verify sidebar (anchored at center_left)
        auto sb = window->child_at(1);
        CHECK(sb->bounds().x == 0);
        // Sidebar height: 3*35 + 2*3 = 111
        // Centered: (400 - 111) / 2 = 144 (or 145)
        CHECK(sb->bounds().y >= 144);
        CHECK(sb->bounds().y <= 145);
        CHECK(sb->bounds().w == 100);

        // Verify sidebar's items
        CHECK(sb->child_at(0)->bounds().h == 35);
        CHECK(sb->child_at(1)->bounds().h == 35);
        CHECK(sb->child_at(2)->bounds().h == 35);
    }

    /**
     * Test Pattern: Absolute layout with structured children
     * Use Case: Custom positioned panels with internal layouts
     */
    TEST_CASE("Absolute with linear child panels") {
        // Root: Absolute layout
        auto canvas = std::make_unique<TestElement>();
        auto abs_strategy = std::make_unique<TestAbsoluteLayout>();

        // Panel 1: Vertical list at specific position
        auto panel1 = std::make_unique<TestElement>();
        panel1->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::vertical, 5));

        for (int i = 0; i < 2; i++) {
            auto item = std::make_unique<TestElement>();
            item->set_height_constraint({size_policy::fixed, 30, 30});
            panel1->add_test_child(std::move(item));
        }

        abs_strategy->set_position(panel1.get(), 50, 50, 100, -1);  // x, y, width, auto-height
        canvas->add_test_child(std::move(panel1));

        // Panel 2: Horizontal buttons at another position
        auto panel2 = std::make_unique<TestElement>();
        panel2->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::horizontal, 5));

        for (int i = 0; i < 3; i++) {
            auto btn = std::make_unique<TestElement>();
            btn->set_width_constraint({size_policy::fixed, 40, 40});
            btn->set_height_constraint({size_policy::fixed, 25, 25});
            panel2->add_test_child(std::move(btn));
        }

        abs_strategy->set_position(panel2.get(), 200, 150, -1, -1);  // Auto size
        canvas->add_test_child(std::move(panel2));

        canvas->set_layout_strategy(std::move(abs_strategy));

        // Measure and arrange
        auto measured = canvas->measure(500, 400);
        CHECK(measured.w <= 500);
        CHECK(measured.h <= 400);
        canvas->arrange({0, 0, 500, 400});

        // Verify panel1
        auto p1 = canvas->child_at(0);
        CHECK(p1->bounds().x == 50);
        CHECK(p1->bounds().y == 50);
        CHECK(p1->bounds().w == 100);  // Explicit width
        CHECK(p1->bounds().h == 65);  // 2*30 + 5 spacing

        // Verify panel1's children
        CHECK(p1->child_at(0)->bounds().y == 50);
        CHECK(p1->child_at(1)->bounds().y == 85);  // 50 + 30 + 5

        // Verify panel2
        auto p2 = canvas->child_at(1);
        CHECK(p2->bounds().x == 200);
        CHECK(p2->bounds().y == 150);
        CHECK(p2->bounds().w == 130);  // 3*40 + 2*5
        CHECK(p2->bounds().h == 25);

        // Verify panel2's buttons
        CHECK(p2->child_at(0)->bounds().x == 200);
        CHECK(p2->child_at(1)->bounds().x == 245);  // 200 + 40 + 5
        CHECK(p2->child_at(2)->bounds().x == 290);  // 200 + 40 + 5 + 40 + 5
    }

    /**
     * Test Pattern: Deep nesting (3 levels)
     * Use Case: Complex card layouts, nested menus
     */
    TEST_CASE("Three-level nesting: Vertical -> Horizontal -> Vertical") {
        // Level 1: Vertical root
        auto root = std::make_unique<TestElement>();
        root->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::vertical, 10));

        // Level 2: Horizontal row
        auto row = std::make_unique<TestElement>();
        row->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::horizontal, 10));

        // Level 3: Two vertical columns
        for (int col = 0; col < 2; col++) {
            auto column = std::make_unique<TestElement>();
            column->set_width_constraint({size_policy::expand});
            column->set_layout_strategy(
                std::make_unique<TestLinearLayout>(direction::vertical, 5));

            for (int i = 0; i < 2; i++) {
                auto item = std::make_unique<TestElement>();
                item->set_height_constraint({size_policy::fixed, 30, 30});
                column->add_test_child(std::move(item));
            }
            row->add_test_child(std::move(column));
        }
        root->add_test_child(std::move(row));

        // Measure and arrange
        auto measured = root->measure(300, 200);
        CHECK(measured.w <= 300);
        CHECK(measured.h > 0);
        root->arrange({0, 0, 300, 200});

        // Verify level 1 (root)
        CHECK(root->child_count() == 1);
        auto l2 = root->child_at(0);
        CHECK(l2->bounds().w == 300);

        // Verify level 2 (horizontal row)
        CHECK(l2->child_count() == 2);
        auto col1 = l2->child_at(0);
        auto col2 = l2->child_at(1);

        // Columns should expand equally
        CHECK(col1->bounds().w == 145);  // (300-10)/2, first gets extra
        CHECK(col2->bounds().w == 145);
        CHECK(col1->bounds().x == 0);
        CHECK(col2->bounds().x == 155);  // 145 + 10

        // Verify level 3 (vertical columns)
        CHECK(col1->child_count() == 2);
        CHECK(col1->child_at(0)->bounds().h == 30);
        CHECK(col1->child_at(0)->bounds().y == 0);
        CHECK(col1->child_at(1)->bounds().y == 35);  // 30 + 5

        CHECK(col2->child_count() == 2);
        CHECK(col2->child_at(0)->bounds().h == 30);
        CHECK(col2->child_at(1)->bounds().h == 30);
    }

    /**
     * Test Pattern: Mixed sizing policies across levels
     * Use Case: Responsive layouts with different sizing strategies
     */
    TEST_CASE("Mixed sizing: Fixed, Expand, and Content policies") {
        // Root: Horizontal with mixed children
        auto container = std::make_unique<TestElement>();
        container->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::horizontal, 10));

        // Child 1: Fixed width vertical panel
        auto fixed_panel = std::make_unique<TestElement>();
        fixed_panel->set_width_constraint({size_policy::fixed, 100, 100});
        fixed_panel->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::vertical, 5));

        auto item1 = std::make_unique<ContentElement>(80, 40);
        fixed_panel->add_test_child(std::move(item1));
        container->add_test_child(std::move(fixed_panel));

        // Child 2: Expanding vertical panel
        auto expand_panel = std::make_unique<TestElement>();
        expand_panel->set_width_constraint({size_policy::expand});
        expand_panel->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::vertical, 5));

        auto item2 = std::make_unique<ContentElement>(80, 40);
        expand_panel->add_test_child(std::move(item2));
        container->add_test_child(std::move(expand_panel));

        // Child 3: Content-sized vertical panel
        auto content_panel = std::make_unique<TestElement>();
        content_panel->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::vertical, 5));

        auto item3 = std::make_unique<ContentElement>(120, 40);
        content_panel->add_test_child(std::move(item3));
        container->add_test_child(std::move(content_panel));

        // Measure and arrange
        TestSize const measured = container->measure(500, 200);
        CHECK(measured.w == 320);  // 100 + 10 + 0 (expand) + 10 + 120 + 10
        CHECK(measured.h == 40);

        container->arrange({0, 0, 500, 200});

        // Verify fixed panel
        auto fp = container->child_at(0);
        CHECK(fp->bounds().w == 100);
        CHECK(fp->bounds().x == 0);

        // Verify expanding panel (should take remaining space)
        auto ep = container->child_at(1);
        CHECK(ep->bounds().x == 110);  // 100 + 10
        CHECK(ep->bounds().w == 260);  // 500 - 100 - 10 - 120 - 10

        // Verify content panel
        auto cp = container->child_at(2);
        CHECK(cp->bounds().x == 380);  // 500 - 120
        CHECK(cp->bounds().w == 120);
    }

    /**
     * Test Pattern: Grid with mixed internal layouts
     * Use Case: Dashboard with different widget types
     */
    TEST_CASE("Grid with different child layouts") {
        // Root: 2x2 Grid
        auto grid = std::make_unique<TestElement>();
        auto grid_strategy = std::make_unique<TestGridLayout>(2, 2, 15, 15);
        grid->set_layout_strategy(std::move(grid_strategy));

        // Cell 0,0: Vertical list
        auto cell1 = std::make_unique<TestElement>();
        cell1->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::vertical, 3));
        for (int i = 0; i < 2; i++) {
            auto item = std::make_unique<TestElement>();
            item->set_height_constraint({size_policy::fixed, 25, 25});
            cell1->add_test_child(std::move(item));
        }
        grid->add_test_child(std::move(cell1));

        // Cell 1,0: Horizontal buttons
        auto cell2 = std::make_unique<TestElement>();
        cell2->set_layout_strategy(
            std::make_unique<TestLinearLayout>(direction::horizontal, 5));
        for (int i = 0; i < 2; i++) {
            auto btn = std::make_unique<TestElement>();
            btn->set_width_constraint({size_policy::expand});
            btn->set_height_constraint({size_policy::fixed, 30, 30});
            cell2->add_test_child(std::move(btn));
        }
        grid->add_test_child(std::move(cell2));

        // Cell 0,1: Single content element
        auto cell3 = std::make_unique<ContentElement>(100, 60);
        grid->add_test_child(std::move(cell3));

        // Cell 1,1: Nested grid
        auto cell4 = std::make_unique<TestElement>();
        auto nested_grid = std::make_unique<TestGridLayout>(2, 1, 5, 5);
        cell4->set_layout_strategy(std::move(nested_grid));
        for (int i = 0; i < 2; i++) {
            auto item = std::make_unique<ContentElement>(50, 30);
            cell4->add_test_child(std::move(item));
        }
        grid->add_test_child(std::move(cell4));

        // Measure and arrange
        auto measured = grid->measure(500, 400);
        CHECK(measured.w > 0);
        CHECK(measured.h > 0);
        grid->arrange({0, 0, 500, 400});

        // Verify all cells are positioned correctly (2x2 grid)
        CHECK(grid->child_at(0)->bounds().x == 0);
        CHECK(grid->child_at(0)->bounds().y == 0);

        CHECK(grid->child_at(1)->bounds().x > 0);  // Second column
        CHECK(grid->child_at(1)->bounds().y == 0);

        CHECK(grid->child_at(2)->bounds().x == 0);
        CHECK(grid->child_at(2)->bounds().y > 0);  // Second row

        CHECK(grid->child_at(3)->bounds().x > 0);  // Second column
        CHECK(grid->child_at(3)->bounds().y > 0);  // Second row

        // Verify nested layouts work
        auto vertical_cell = grid->child_at(0);
        CHECK(vertical_cell->child_count() == 2);
        CHECK(vertical_cell->child_at(0)->bounds().h == 25);
        CHECK(vertical_cell->child_at(1)->bounds().h == 25);

        auto horizontal_cell = grid->child_at(1);
        CHECK(horizontal_cell->child_count() == 2);
        // Both should expand to fill cell
        CHECK(horizontal_cell->child_at(0)->bounds().w > 0);
        CHECK(horizontal_cell->child_at(1)->bounds().w > 0);

        auto nested_grid_cell = grid->child_at(3);
        CHECK(nested_grid_cell->child_count() == 2);
    }
}
