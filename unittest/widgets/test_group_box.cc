//
// Tests for group_box widget
//

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include <memory>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/widgets/containers/group_box.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/widgets/label.hh>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/services/ui_context.hh>
#include "../utils/test_helpers.hh"
#include <utility>
#include "../utils/test_helpers.hh"
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"
#include "onyxui/concepts/size_like.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_SUITE("group_box") {
    using Backend = test_backend;

    TEST_CASE("Default construction") {
        auto group = std::make_unique<group_box<Backend>>();

        CHECK(group->get_title().empty());
        CHECK(!group->has_title());
        CHECK(group->get_border() == group_box_border::single);
    }

    TEST_CASE("Set and get title") {
        auto group = std::make_unique<group_box<Backend>>();

        group->set_title("System Information");
        CHECK(group->get_title() == "System Information");
        CHECK(group->has_title());

        group->set_title("Registered To");
        CHECK(group->get_title() == "Registered To");
        CHECK(group->has_title());
    }

    TEST_CASE("Empty title") {
        auto group = std::make_unique<group_box<Backend>>();

        group->set_title("Title");
        CHECK(group->has_title());

        group->set_title("");
        CHECK(!group->has_title());
        CHECK(group->get_title().empty());
    }

    TEST_CASE("Border style") {
        auto group = std::make_unique<group_box<Backend>>();

        CHECK(group->get_border() == group_box_border::single);

        group->set_border(group_box_border::double_);
        CHECK(group->get_border() == group_box_border::double_);

        group->set_border(group_box_border::rounded);
        CHECK(group->get_border() == group_box_border::rounded);

        group->set_border(group_box_border::none);
        CHECK(group->get_border() == group_box_border::none);
    }

    TEST_CASE("Title position") {
        auto group = std::make_unique<group_box<Backend>>();

        // Default position is 2
        group->set_title_position(5);
        // No getter, just check it doesn't crash

        group->set_title_position(0);
        group->set_title_position(10);
    }

    TEST_CASE("Add children to group box") {
        auto group = std::make_unique<group_box<Backend>>();
        group->set_title("User Info");

        auto label1 = std::make_unique<label<Backend>>();
        label1->set_text("Name: John Doe");

        auto label2 = std::make_unique<label<Backend>>();
        label2->set_text("Company: Acme Corp");

        group->add_child(std::move(label1));
        group->add_child(std::move(label2));

        CHECK(group->children().size() == 2);
    }

    TEST_CASE("Group box with no children") {
        auto group = std::make_unique<group_box<Backend>>();
        group->set_title("Empty Box");

        CHECK(group->children().empty());
    }

    TEST_CASE("Group box size includes border") {
        // Setup ui_context with theme for measurement
        ui_theme<Backend> theme;
        theme.name = "Test";
        scoped_ui_context<Backend> ctx{make_terminal_metrics<Backend>()};
        ctx.themes().register_theme(std::move(theme));
        ctx.themes().set_current_theme("Test");

        auto group = std::make_unique<group_box<Backend>>();
        group->set_title("Box");

        // Add a child with known size
        auto lbl = std::make_unique<label<Backend>>();
        lbl->set_text("Test");  // 4 chars
        group->add_child(std::move(lbl));

        // Measure
        auto size = group->measure(80_lu, 25_lu);

        // Size should include border space (+2 for width, +2 for height)
        int const width = size.width.to_int();
        int const height = size.height.to_int();

        // Width should be at least label width + borders
        CHECK(width >= 6);  // 4 chars + 2 borders
        // Height should be at least label height + borders
        CHECK(height >= 3);  // 1 line + 2 borders
    }

    TEST_CASE("Setting same title doesn't cause issues") {
        auto group = std::make_unique<group_box<Backend>>();

        group->set_title("Title");
        group->set_title("Title");  // Same title

        CHECK(group->get_title() == "Title");
    }

    TEST_CASE("Long title") {
        auto group = std::make_unique<group_box<Backend>>();

        std::string long_title(100, 'X');
        group->set_title(long_title);

        CHECK(group->get_title() == long_title);
    }

    TEST_CASE("Special characters in title") {
        auto group = std::make_unique<group_box<Backend>>();

        group->set_title("═ System Info ═");
        CHECK(group->get_title() == "═ System Info ═");

        group->set_title("→ Settings ←");
        CHECK(group->get_title() == "→ Settings ←");
    }

    TEST_CASE("Norton Utilities style group box") {
        auto group = std::make_unique<group_box<Backend>>();
        group->set_title(" Registered To ");
        group->set_border(group_box_border::single);

        auto user = std::make_unique<label<Backend>>();
        user->set_text("User");

        auto company = std::make_unique<label<Backend>>();
        company->set_text("Company");

        group->add_child(std::move(user));
        group->add_child(std::move(company));

        CHECK(group->has_title());
        CHECK(group->children().size() == 2);
    }

    TEST_CASE("Nested group boxes") {
        auto outer = std::make_unique<group_box<Backend>>();
        outer->set_title("Outer");

        auto inner = std::make_unique<group_box<Backend>>();
        inner->set_title("Inner");

        outer->add_child(std::move(inner));

        CHECK(outer->children().size() == 1);
    }

    TEST_CASE("Group box inherits from panel") {
        auto group = std::make_unique<group_box<Backend>>();

        // Should be able to use panel methods
        group->set_visible(true);
        CHECK(group->is_visible());

        group->set_visible(false);
        CHECK(!group->is_visible());
    }

    TEST_CASE("Border style none removes border") {
        auto group = std::make_unique<group_box<Backend>>();
        group->set_title("No Border");
        group->set_border(group_box_border::none);

        CHECK(group->get_border() == group_box_border::none);
        CHECK(group->has_title());  // Title still present
    }
}
