// Copyright (c) 2025 OnyxUI Framework
// SPDX-License-Identifier: MIT

#include <doctest/doctest.h>

#include <onyxui/widgets/input/button_group.hh>
#include <onyxui/widgets/input/radio_button.hh>
#include "../utils/test_backend.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;

using Backend = test_backend;

// ===== Construction and Basic Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - Default construction") {
    button_group<Backend> group;

    REQUIRE(group.count() == 0);
    REQUIRE(group.checked_id() == -1);
    REQUIRE(group.checked_button() == nullptr);
    REQUIRE(group.buttons().empty());
}

// ===== Button Management Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - add_button() adds button to group") {
    button_group<Backend> group;
    radio_button<Backend> rb("Option 1");

    group.add_button(&rb, 0);

    REQUIRE(group.count() == 1);
    REQUIRE(rb.group() == &group);
    REQUIRE(group.button(0) == &rb);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - add_button() with auto-assigned ID") {
    button_group<Backend> group;
    radio_button<Backend> rb1("Option 1");
    radio_button<Backend> rb2("Option 2");
    radio_button<Backend> rb3("Option 3");

    group.add_button(&rb1);  // ID = 0
    group.add_button(&rb2);  // ID = 1
    group.add_button(&rb3);  // ID = 2

    REQUIRE(group.count() == 3);
    REQUIRE(group.button(0) == &rb1);
    REQUIRE(group.button(1) == &rb2);
    REQUIRE(group.button(2) == &rb3);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - add_button() with explicit IDs") {
    button_group<Backend> group;
    radio_button<Backend> rb1("Option 1");
    radio_button<Backend> rb2("Option 2");

    group.add_button(&rb1, 10);
    group.add_button(&rb2, 20);

    REQUIRE(group.count() == 2);
    REQUIRE(group.button(10) == &rb1);
    REQUIRE(group.button(20) == &rb2);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - remove_button() removes button from group") {
    button_group<Backend> group;
    radio_button<Backend> rb("Option 1");

    group.add_button(&rb, 0);
    REQUIRE(group.count() == 1);

    group.remove_button(&rb);

    REQUIRE(group.count() == 0);
    REQUIRE(rb.group() == nullptr);
    REQUIRE(group.button(0) == nullptr);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - remove_button() clears checked_id if checked button removed") {
    button_group<Backend> group;
    radio_button<Backend> rb("Option 1");

    group.add_button(&rb, 0);
    group.set_checked_id(0);
    REQUIRE(group.checked_id() == 0);

    group.remove_button(&rb);

    REQUIRE(group.checked_id() == -1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - buttons() returns all buttons") {
    button_group<Backend> group;
    radio_button<Backend> rb1("Option 1");
    radio_button<Backend> rb2("Option 2");
    radio_button<Backend> rb3("Option 3");

    group.add_button(&rb1, 0);
    group.add_button(&rb2, 1);
    group.add_button(&rb3, 2);

    auto button_list = group.buttons();

    REQUIRE(button_list.size() == 3);
    // Order is unspecified (unordered_map), but all should be present
    REQUIRE(std::find(button_list.begin(), button_list.end(), &rb1) != button_list.end());
    REQUIRE(std::find(button_list.begin(), button_list.end(), &rb2) != button_list.end());
    REQUIRE(std::find(button_list.begin(), button_list.end(), &rb3) != button_list.end());
}

// ===== Selection Management Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - set_checked_id() checks button") {
    button_group<Backend> group;
    radio_button<Backend> rb1("Option 1");
    radio_button<Backend> rb2("Option 2");

    group.add_button(&rb1, 0);
    group.add_button(&rb2, 1);

    group.set_checked_id(1);

    REQUIRE(group.checked_id() == 1);
    REQUIRE(group.checked_button() == &rb2);
    REQUIRE_FALSE(rb1.is_checked());
    REQUIRE(rb2.is_checked());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - set_checked_id(-1) unchecks all buttons") {
    button_group<Backend> group;
    radio_button<Backend> rb1("Option 1");
    radio_button<Backend> rb2("Option 2");

    group.add_button(&rb1, 0);
    group.add_button(&rb2, 1);
    group.set_checked_id(0);

    REQUIRE(rb1.is_checked());

    group.set_checked_id(-1);

    REQUIRE_FALSE(rb1.is_checked());
    REQUIRE_FALSE(rb2.is_checked());
    REQUIRE(group.checked_id() == -1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - set_checked_id() with invalid ID does nothing") {
    button_group<Backend> group;
    radio_button<Backend> rb("Option 1");

    group.add_button(&rb, 0);
    group.set_checked_id(0);

    group.set_checked_id(999);  // Invalid ID

    REQUIRE(group.checked_id() == 0);  // Unchanged
    REQUIRE(rb.is_checked());  // Still checked
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - set_checked_button() checks button") {
    button_group<Backend> group;
    radio_button<Backend> rb1("Option 1");
    radio_button<Backend> rb2("Option 2");

    group.add_button(&rb1, 0);
    group.add_button(&rb2, 1);

    group.set_checked_button(&rb2);

    REQUIRE(group.checked_button() == &rb2);
    REQUIRE_FALSE(rb1.is_checked());
    REQUIRE(rb2.is_checked());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - set_checked_button(nullptr) unchecks all") {
    button_group<Backend> group;
    radio_button<Backend> rb("Option 1");

    group.add_button(&rb, 0);
    group.set_checked_id(0);

    group.set_checked_button(nullptr);

    REQUIRE_FALSE(rb.is_checked());
    REQUIRE(group.checked_id() == -1);
}

// ===== Mutual Exclusion Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - Mutual exclusion: checking one unchecks others") {
    button_group<Backend> group;
    radio_button<Backend> rb1("Option 1");
    radio_button<Backend> rb2("Option 2");
    radio_button<Backend> rb3("Option 3");

    group.add_button(&rb1, 0);
    group.add_button(&rb2, 1);
    group.add_button(&rb3, 2);

    // Check first button
    group.set_checked_id(0);
    REQUIRE(rb1.is_checked());
    REQUIRE_FALSE(rb2.is_checked());
    REQUIRE_FALSE(rb3.is_checked());

    // Check second button - first should uncheck
    group.set_checked_id(1);
    REQUIRE_FALSE(rb1.is_checked());
    REQUIRE(rb2.is_checked());
    REQUIRE_FALSE(rb3.is_checked());

    // Check third button - second should uncheck
    group.set_checked_id(2);
    REQUIRE_FALSE(rb1.is_checked());
    REQUIRE_FALSE(rb2.is_checked());
    REQUIRE(rb3.is_checked());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - Adding checked button unchecks previous") {
    button_group<Backend> group;
    radio_button<Backend> rb1("Option 1", true);  // Initially checked
    radio_button<Backend> rb2("Option 2", true);  // Also initially checked

    group.add_button(&rb1, 0);
    REQUIRE(rb1.is_checked());

    group.add_button(&rb2, 1);  // Adding checked button should uncheck rb1

    REQUIRE_FALSE(rb1.is_checked());  // Unchecked by group
    REQUIRE(rb2.is_checked());  // Still checked
    REQUIRE(group.checked_id() == 1);
}

// ===== Signal Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - button_toggled signal emitted on check") {
    button_group<Backend> group;
    radio_button<Backend> rb1("Option 1");
    radio_button<Backend> rb2("Option 2");

    group.add_button(&rb1, 0);
    group.add_button(&rb2, 1);

    int last_id = -1;
    bool last_checked = false;
    int signal_count = 0;

    group.button_toggled.connect([&](int id, bool checked) {
        last_id = id;
        last_checked = checked;
        signal_count++;
    });

    group.set_checked_id(1);

    REQUIRE(signal_count == 1);
    REQUIRE(last_id == 1);
    REQUIRE(last_checked);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - button_toggled signal emitted for both buttons on switch") {
    button_group<Backend> group;
    radio_button<Backend> rb1("Option 1");
    radio_button<Backend> rb2("Option 2");

    group.add_button(&rb1, 0);
    group.add_button(&rb2, 1);
    group.set_checked_id(0);  // Start with rb1 checked

    std::vector<std::pair<int, bool>> signals;

    group.button_toggled.connect([&](int id, bool checked) {
        signals.push_back({id, checked});
    });

    group.set_checked_id(1);  // Switch to rb2

    REQUIRE(signals.size() == 2);
    // First signal: rb1 unchecked
    REQUIRE(signals[0].first == 0);
    REQUIRE(signals[0].second == false);
    // Second signal: rb2 checked
    REQUIRE(signals[1].first == 1);
    REQUIRE(signals[1].second == true);
}

// ===== Navigation Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - select_next() moves to next button") {
    button_group<Backend> group;
    radio_button<Backend> rb1("Option 1");
    radio_button<Backend> rb2("Option 2");
    radio_button<Backend> rb3("Option 3");

    group.add_button(&rb1, 0);
    group.add_button(&rb2, 1);
    group.add_button(&rb3, 2);
    group.set_checked_id(0);

    group.select_next(&rb1);

    REQUIRE(group.checked_id() == 1);  // Moved to rb2
    REQUIRE(rb2.is_checked());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - select_next() wraps around at end") {
    button_group<Backend> group;
    radio_button<Backend> rb1("Option 1");
    radio_button<Backend> rb2("Option 2");

    group.add_button(&rb1, 0);
    group.add_button(&rb2, 1);
    group.set_checked_id(1);  // Last button

    group.select_next(&rb2);

    REQUIRE(group.checked_id() == 0);  // Wrapped to first
    REQUIRE(rb1.is_checked());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - select_previous() moves to previous button") {
    button_group<Backend> group;
    radio_button<Backend> rb1("Option 1");
    radio_button<Backend> rb2("Option 2");
    radio_button<Backend> rb3("Option 3");

    group.add_button(&rb1, 0);
    group.add_button(&rb2, 1);
    group.add_button(&rb3, 2);
    group.set_checked_id(2);

    group.select_previous(&rb3);

    REQUIRE(group.checked_id() == 1);  // Moved to rb2
    REQUIRE(rb2.is_checked());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - select_previous() wraps around at beginning") {
    button_group<Backend> group;
    radio_button<Backend> rb1("Option 1");
    radio_button<Backend> rb2("Option 2");

    group.add_button(&rb1, 0);
    group.add_button(&rb2, 1);
    group.set_checked_id(0);  // First button

    group.select_previous(&rb1);

    REQUIRE(group.checked_id() == 1);  // Wrapped to last
    REQUIRE(rb2.is_checked());
}

// ===== Edge Case Tests =====

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - Empty group operations") {
    button_group<Backend> group;

    group.set_checked_id(0);  // Should do nothing
    REQUIRE(group.checked_id() == -1);

    group.set_checked_button(nullptr);  // Should do nothing
    REQUIRE(group.checked_id() == -1);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - Single button group") {
    button_group<Backend> group;
    radio_button<Backend> rb("Only option");

    group.add_button(&rb, 0);
    group.set_checked_id(0);

    REQUIRE(rb.is_checked());

    // Navigate next - should wrap to same button
    group.select_next(&rb);
    REQUIRE(rb.is_checked());  // Still checked (wrapped to self)
}

TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "button_group - Multiple groups with same buttons (invalid but tested)") {
    button_group<Backend> group1;
    button_group<Backend> group2;
    radio_button<Backend> rb("Option");

    group1.add_button(&rb, 0);
    REQUIRE(rb.group() == &group1);

    // Adding to second group overwrites first
    group2.add_button(&rb, 0);
    REQUIRE(rb.group() == &group2);
}
