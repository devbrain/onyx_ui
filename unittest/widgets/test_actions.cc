//
// Created by claude on 16/10/2025.
//
// Tests for action system and action groups

#include <doctest/doctest.h>

#include <onyxui/widgets/action.hh>
#include <onyxui/widgets/action_group.hh>
#include <onyxui/widgets/button.hh>
#include "../utils/test_backend.hh"

using namespace onyxui;

TEST_CASE("Action - Basic Properties") {
    SUBCASE("Default construction") {
        auto action_ptr = std::make_shared<action<test_backend>>();

        CHECK(action_ptr->text() == "");
        CHECK(action_ptr->is_enabled() == true);
        CHECK(action_ptr->is_checkable() == false);
        CHECK(action_ptr->is_checked() == false);
    }

    SUBCASE("Set and get text") {
        auto action_ptr = std::make_shared<action<test_backend>>();

        action_ptr->set_text("Save");
        CHECK(action_ptr->text() == "Save");

        action_ptr->set_text("Save As...");
        CHECK(action_ptr->text() == "Save As...");
    }

    SUBCASE("Enable and disable") {
        auto action_ptr = std::make_shared<action<test_backend>>();

        CHECK(action_ptr->is_enabled() == true);

        action_ptr->set_enabled(false);
        CHECK(action_ptr->is_enabled() == false);

        action_ptr->set_enabled(true);
        CHECK(action_ptr->is_enabled() == true);
    }

    SUBCASE("Checkable state") {
        auto action_ptr = std::make_shared<action<test_backend>>();

        CHECK(action_ptr->is_checkable() == false);
        CHECK(action_ptr->is_checked() == false);

        action_ptr->set_checkable(true);
        CHECK(action_ptr->is_checkable() == true);
        CHECK(action_ptr->is_checked() == false);

        // Making non-checkable should reset checked state
        action_ptr->set_checked(true);
        CHECK(action_ptr->is_checked() == true);

        action_ptr->set_checkable(false);
        CHECK(action_ptr->is_checkable() == false);
        CHECK(action_ptr->is_checked() == false);
    }

    SUBCASE("Checked state - only works when checkable") {
        auto action_ptr = std::make_shared<action<test_backend>>();

        // Non-checkable actions cannot be checked
        action_ptr->set_checked(true);
        CHECK(action_ptr->is_checked() == false);

        // Make checkable, then it can be checked
        action_ptr->set_checkable(true);
        action_ptr->set_checked(true);
        CHECK(action_ptr->is_checked() == true);

        action_ptr->set_checked(false);
        CHECK(action_ptr->is_checked() == false);
    }
}

TEST_CASE("Action - Signals") {
    SUBCASE("text_changed signal") {
        auto action_ptr = std::make_shared<action<test_backend>>();

        std::string captured_text;
        action_ptr->text_changed.connect([&captured_text](std::string_view text) {
            captured_text = text;
        });

        action_ptr->set_text("Open");
        CHECK(captured_text == "Open");

        action_ptr->set_text("Close");
        CHECK(captured_text == "Close");
    }

    SUBCASE("enabled_changed signal") {
        auto action_ptr = std::make_shared<action<test_backend>>();

        bool captured_state = true;
        action_ptr->enabled_changed.connect([&captured_state](bool enabled) {
            captured_state = enabled;
        });

        action_ptr->set_enabled(false);
        CHECK(captured_state == false);

        action_ptr->set_enabled(true);
        CHECK(captured_state == true);
    }

    SUBCASE("checked_changed signal") {
        auto action_ptr = std::make_shared<action<test_backend>>();
        action_ptr->set_checkable(true);

        bool captured_checked = false;
        action_ptr->checked_changed.connect([&captured_checked](bool checked) {
            captured_checked = checked;
        });

        action_ptr->set_checked(true);
        CHECK(captured_checked == true);

        action_ptr->set_checked(false);
        CHECK(captured_checked == false);
    }

    SUBCASE("triggered signal") {
        auto action_ptr = std::make_shared<action<test_backend>>();

        int trigger_count = 0;
        action_ptr->triggered.connect([&trigger_count]() {
            trigger_count++;
        });

        action_ptr->trigger();
        CHECK(trigger_count == 1);

        action_ptr->trigger();
        CHECK(trigger_count == 2);
    }

    SUBCASE("Disabled action doesn't trigger") {
        auto action_ptr = std::make_shared<action<test_backend>>();
        action_ptr->set_enabled(false);

        int trigger_count = 0;
        action_ptr->triggered.connect([&trigger_count]() {
            trigger_count++;
        });

        action_ptr->trigger();
        CHECK(trigger_count == 0);  // Didn't trigger because disabled
    }

    SUBCASE("Checkable action toggles on trigger") {
        auto action_ptr = std::make_shared<action<test_backend>>();
        action_ptr->set_checkable(true);

        CHECK(action_ptr->is_checked() == false);

        action_ptr->trigger();
        CHECK(action_ptr->is_checked() == true);

        action_ptr->trigger();
        CHECK(action_ptr->is_checked() == false);
    }
}

TEST_CASE("Action Group - Basic Functionality") {
    SUBCASE("Default construction - exclusive mode") {
        auto group = std::make_shared<action_group<test_backend>>();

        CHECK(group->is_exclusive() == true);
        CHECK(group->actions().empty() == true);
        CHECK(group->checked_action() == nullptr);
    }

    SUBCASE("Non-exclusive mode construction") {
        auto group = std::make_shared<action_group<test_backend>>(false);

        CHECK(group->is_exclusive() == false);
    }

    SUBCASE("Add actions to group") {
        auto group = std::make_shared<action_group<test_backend>>();

        auto action1 = std::make_shared<action<test_backend>>();
        auto action2 = std::make_shared<action<test_backend>>();

        group->add_action(action1);
        group->add_action(action2);

        auto actions = group->actions();
        CHECK(actions.size() == 2);
    }

    SUBCASE("Actions become checkable when added to group") {
        auto group = std::make_shared<action_group<test_backend>>();
        auto action_ptr = std::make_shared<action<test_backend>>();

        CHECK(action_ptr->is_checkable() == false);

        group->add_action(action_ptr);

        CHECK(action_ptr->is_checkable() == true);
    }

    SUBCASE("Remove action from group") {
        auto group = std::make_shared<action_group<test_backend>>();

        auto action1 = std::make_shared<action<test_backend>>();
        auto action2 = std::make_shared<action<test_backend>>();

        group->add_action(action1);
        group->add_action(action2);

        CHECK(group->actions().size() == 2);

        group->remove_action(action1);

        CHECK(group->actions().size() == 1);
        CHECK(group->actions()[0] == action2);
    }
}

TEST_CASE("Action Group - Exclusive Mode") {
    SUBCASE("Only one action can be checked in exclusive group") {
        auto group = std::make_shared<action_group<test_backend>>();

        auto left = std::make_shared<action<test_backend>>();
        auto center = std::make_shared<action<test_backend>>();
        auto right = std::make_shared<action<test_backend>>();

        left->set_text("Left");
        center->set_text("Center");
        right->set_text("Right");

        group->add_action(left);
        group->add_action(center);
        group->add_action(right);

        // Check left
        left->set_checked(true);
        CHECK(left->is_checked() == true);
        CHECK(center->is_checked() == false);
        CHECK(right->is_checked() == false);

        // Check center - left should uncheck
        center->set_checked(true);
        CHECK(left->is_checked() == false);
        CHECK(center->is_checked() == true);
        CHECK(right->is_checked() == false);

        // Check right - center should uncheck
        right->set_checked(true);
        CHECK(left->is_checked() == false);
        CHECK(center->is_checked() == false);
        CHECK(right->is_checked() == true);
    }

    SUBCASE("checked_action() returns the checked action") {
        auto group = std::make_shared<action_group<test_backend>>();

        auto left = std::make_shared<action<test_backend>>();
        auto center = std::make_shared<action<test_backend>>();

        group->add_action(left);
        group->add_action(center);

        CHECK(group->checked_action() == nullptr);

        left->set_checked(true);
        CHECK(group->checked_action() == left);

        center->set_checked(true);
        CHECK(group->checked_action() == center);
    }
}

TEST_CASE("Action Group - Non-Exclusive Mode") {
    SUBCASE("Multiple actions can be checked in non-exclusive group") {
        auto group = std::make_shared<action_group<test_backend>>(false);  // non-exclusive

        auto bold = std::make_shared<action<test_backend>>();
        auto italic = std::make_shared<action<test_backend>>();
        auto underline = std::make_shared<action<test_backend>>();

        group->add_action(bold);
        group->add_action(italic);
        group->add_action(underline);

        // Check all three
        bold->set_checked(true);
        italic->set_checked(true);
        underline->set_checked(true);

        // All should remain checked
        CHECK(bold->is_checked() == true);
        CHECK(italic->is_checked() == true);
        CHECK(underline->is_checked() == true);
    }

    SUBCASE("Switch from exclusive to non-exclusive") {
        auto group = std::make_shared<action_group<test_backend>>(true);  // exclusive

        auto action1 = std::make_shared<action<test_backend>>();
        auto action2 = std::make_shared<action<test_backend>>();
        auto action3 = std::make_shared<action<test_backend>>();

        group->add_action(action1);
        group->add_action(action2);
        group->add_action(action3);

        // Check multiple in exclusive mode (only last should remain)
        action1->set_checked(true);
        action2->set_checked(true);

        CHECK(action1->is_checked() == false);
        CHECK(action2->is_checked() == true);

        // Switch to non-exclusive
        group->set_exclusive(false);

        // Now both can be checked
        action1->set_checked(true);
        CHECK(action1->is_checked() == true);
        CHECK(action2->is_checked() == true);
    }

    SUBCASE("Switch from non-exclusive to exclusive") {
        auto group = std::make_shared<action_group<test_backend>>(false);  // non-exclusive

        auto action1 = std::make_shared<action<test_backend>>();
        auto action2 = std::make_shared<action<test_backend>>();
        auto action3 = std::make_shared<action<test_backend>>();

        group->add_action(action1);
        group->add_action(action2);
        group->add_action(action3);

        // Check multiple
        action1->set_checked(true);
        action2->set_checked(true);
        action3->set_checked(true);

        CHECK(action1->is_checked() == true);
        CHECK(action2->is_checked() == true);
        CHECK(action3->is_checked() == true);

        // Switch to exclusive - only first checked should remain
        group->set_exclusive(true);

        int checked_count = 0;
        if (action1->is_checked()) checked_count++;
        if (action2->is_checked()) checked_count++;
        if (action3->is_checked()) checked_count++;

        CHECK(checked_count == 1);  // Only one should remain checked
    }
}

TEST_CASE("Widget Integration - Button with Action") {
    SUBCASE("Button syncs text from action") {
        auto save_action = std::make_shared<action<test_backend>>();
        save_action->set_text("Save");

        auto btn = std::make_unique<button<test_backend>>();
        btn->set_action(save_action);

        CHECK(btn->text() == "Save");
    }

    SUBCASE("Button syncs enabled state from action") {
        auto action_ptr = std::make_shared<action<test_backend>>();
        action_ptr->set_enabled(false);

        auto btn = std::make_unique<button<test_backend>>();
        btn->set_action(action_ptr);

        CHECK(btn->is_enabled() == false);
    }

    SUBCASE("Button updates when action text changes") {
        auto action_ptr = std::make_shared<action<test_backend>>();
        action_ptr->set_text("Open");

        auto btn = std::make_unique<button<test_backend>>();
        btn->set_action(action_ptr);

        CHECK(btn->text() == "Open");

        action_ptr->set_text("Close");
        CHECK(btn->text() == "Close");
    }

    SUBCASE("Button updates when action enabled state changes") {
        auto action_ptr = std::make_shared<action<test_backend>>();
        action_ptr->set_enabled(true);

        auto btn = std::make_unique<button<test_backend>>();
        btn->set_action(action_ptr);

        CHECK(btn->is_enabled() == true);

        action_ptr->set_enabled(false);
        CHECK(btn->is_enabled() == false);
    }

    SUBCASE("Button click triggers action") {
        auto action_ptr = std::make_shared<action<test_backend>>();

        int trigger_count = 0;
        action_ptr->triggered.connect([&trigger_count]() {
            trigger_count++;
        });

        auto btn = std::make_unique<button<test_backend>>();
        btn->set_action(action_ptr);

        // Simulate button click
        btn->clicked.emit();

        CHECK(trigger_count == 1);
    }

    SUBCASE("Multiple buttons share one action") {
        auto save_action = std::make_shared<action<test_backend>>();
        save_action->set_text("Save");

        auto toolbar_button = std::make_unique<button<test_backend>>();
        auto menu_button = std::make_unique<button<test_backend>>();

        toolbar_button->set_action(save_action);
        menu_button->set_action(save_action);

        // Both buttons have same text
        CHECK(toolbar_button->text() == "Save");
        CHECK(menu_button->text() == "Save");

        // Disable action - both buttons disabled
        save_action->set_enabled(false);
        CHECK(toolbar_button->is_enabled() == false);
        CHECK(menu_button->is_enabled() == false);

        // Change text - both buttons update
        save_action->set_text("Save All");
        CHECK(toolbar_button->text() == "Save All");
        CHECK(menu_button->text() == "Save All");
    }

    SUBCASE("Action triggered through any connected button") {
        auto action_ptr = std::make_shared<action<test_backend>>();

        int trigger_count = 0;
        action_ptr->triggered.connect([&trigger_count]() {
            trigger_count++;
        });

        auto button1 = std::make_unique<button<test_backend>>();
        auto button2 = std::make_unique<button<test_backend>>();

        button1->set_action(action_ptr);
        button2->set_action(action_ptr);

        button1->clicked.emit();
        CHECK(trigger_count == 1);

        button2->clicked.emit();
        CHECK(trigger_count == 2);
    }

    SUBCASE("Removing action from button") {
        auto action_ptr = std::make_shared<action<test_backend>>();
        action_ptr->set_text("Test");

        auto btn = std::make_unique<button<test_backend>>();
        btn->set_action(action_ptr);

        CHECK(btn->text() == "Test");

        // Set action to nullptr
        btn->set_action(nullptr);

        CHECK(btn->get_action() == nullptr);

        // Button should no longer update when action changes
        action_ptr->set_text("Changed");
        CHECK(btn->text() == "Test");  // Still old text
    }
}

TEST_CASE("Real-World Scenarios") {
    SUBCASE("Toolbar with multiple actions") {
        // Create actions
        auto new_action = std::make_shared<action<test_backend>>();
        auto open_action = std::make_shared<action<test_backend>>();
        auto save_action = std::make_shared<action<test_backend>>();

        new_action->set_text("New");
        open_action->set_text("Open");
        save_action->set_text("Save");
        save_action->set_enabled(false);  // Disabled initially

        // Create toolbar buttons
        auto new_btn = std::make_unique<button<test_backend>>();
        auto open_btn = std::make_unique<button<test_backend>>();
        auto save_btn = std::make_unique<button<test_backend>>();

        new_btn->set_action(new_action);
        open_btn->set_action(open_action);
        save_btn->set_action(save_action);

        // Verify initial state
        CHECK(save_btn->is_enabled() == false);

        // "Open" a document - enable save
        save_action->set_enabled(true);
        CHECK(save_btn->is_enabled() == true);
    }

    SUBCASE("Alignment buttons with exclusive group") {
        // Create alignment actions
        auto left_align = std::make_shared<action<test_backend>>();
        auto center_align = std::make_shared<action<test_backend>>();
        auto right_align = std::make_shared<action<test_backend>>();

        left_align->set_text("Left");
        center_align->set_text("Center");
        right_align->set_text("Right");

        // Create exclusive group
        auto align_group = std::make_shared<action_group<test_backend>>();
        align_group->add_action(left_align);
        align_group->add_action(center_align);
        align_group->add_action(right_align);

        // Create buttons
        auto left_btn = std::make_unique<button<test_backend>>();
        auto center_btn = std::make_unique<button<test_backend>>();
        auto right_btn = std::make_unique<button<test_backend>>();

        left_btn->set_action(left_align);
        center_btn->set_action(center_align);
        right_btn->set_action(right_align);

        // Default to left alignment
        left_align->set_checked(true);
        CHECK(left_align->is_checked() == true);
        CHECK(center_align->is_checked() == false);
        CHECK(right_align->is_checked() == false);

        // Click center button
        center_btn->clicked.emit();
        CHECK(left_align->is_checked() == false);
        CHECK(center_align->is_checked() == true);
        CHECK(right_align->is_checked() == false);

        // Verify which alignment is active
        auto checked = align_group->checked_action();
        REQUIRE(checked != nullptr);
        CHECK(checked->text() == "Center");
    }

    SUBCASE("Format toolbar with non-exclusive group") {
        // Create format actions
        auto bold_action = std::make_shared<action<test_backend>>();
        auto italic_action = std::make_shared<action<test_backend>>();
        auto underline_action = std::make_shared<action<test_backend>>();

        bold_action->set_text("Bold");
        italic_action->set_text("Italic");
        underline_action->set_text("Underline");

        // Create non-exclusive group
        auto format_group = std::make_shared<action_group<test_backend>>(false);
        format_group->add_action(bold_action);
        format_group->add_action(italic_action);
        format_group->add_action(underline_action);

        // Create buttons
        auto bold_btn = std::make_unique<button<test_backend>>();
        auto italic_btn = std::make_unique<button<test_backend>>();
        auto underline_btn = std::make_unique<button<test_backend>>();

        bold_btn->set_action(bold_action);
        italic_btn->set_action(italic_action);
        underline_btn->set_action(underline_action);

        // User clicks bold
        bold_btn->clicked.emit();
        CHECK(bold_action->is_checked() == true);

        // User clicks italic (bold stays checked)
        italic_btn->clicked.emit();
        CHECK(bold_action->is_checked() == true);
        CHECK(italic_action->is_checked() == true);

        // User clicks underline (both stay checked)
        underline_btn->clicked.emit();
        CHECK(bold_action->is_checked() == true);
        CHECK(italic_action->is_checked() == true);
        CHECK(underline_action->is_checked() == true);

        // Toggle bold off
        bold_btn->clicked.emit();
        CHECK(bold_action->is_checked() == false);
        CHECK(italic_action->is_checked() == true);
        CHECK(underline_action->is_checked() == true);
    }
}
