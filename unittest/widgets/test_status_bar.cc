//
// Tests for status_bar widget
//

#include <doctest/doctest.h>
#include <onyxui/widgets/status_bar.hh>
#include "../utils/test_backend.hh"

using namespace onyxui;

TEST_SUITE("status_bar") {
    using Backend = test_backend;

    TEST_CASE("Default construction") {
        auto status = std::make_unique<status_bar<Backend>>();

        CHECK(status->get_left_text().empty());
        CHECK(status->get_right_text().empty());
    }

    TEST_CASE("Set and get left text") {
        auto status = std::make_unique<status_bar<Backend>>();

        status->set_left_text("F1 Help   Alt-X Exit");
        CHECK(status->get_left_text() == "F1 Help   Alt-X Exit");

        status->set_left_text("Ready");
        CHECK(status->get_left_text() == "Ready");
    }

    TEST_CASE("Set and get right text") {
        auto status = std::make_unique<status_bar<Backend>>();

        status->set_right_text("2025-10-16 14:30");
        CHECK(status->get_right_text() == "2025-10-16 14:30");

        status->set_right_text("Line 10:25");
        CHECK(status->get_right_text() == "Line 10:25");
    }

    TEST_CASE("Set both left and right text") {
        auto status = std::make_unique<status_bar<Backend>>();

        status->set_left_text("F1 Help");
        status->set_right_text("Ready");

        CHECK(status->get_left_text() == "F1 Help");
        CHECK(status->get_right_text() == "Ready");
    }

    TEST_CASE("Clear resets both text sections") {
        auto status = std::make_unique<status_bar<Backend>>();

        status->set_left_text("Left");
        status->set_right_text("Right");

        status->clear();

        CHECK(status->get_left_text().empty());
        CHECK(status->get_right_text().empty());
    }

    TEST_CASE("Empty text is valid") {
        auto status = std::make_unique<status_bar<Backend>>();

        status->set_left_text("");
        status->set_right_text("");

        CHECK(status->get_left_text().empty());
        CHECK(status->get_right_text().empty());
    }

    TEST_CASE("Setting same text doesn't cause issues") {
        auto status = std::make_unique<status_bar<Backend>>();

        status->set_left_text("Test");
        status->set_left_text("Test");  // Same text

        CHECK(status->get_left_text() == "Test");
    }

    TEST_CASE("Long text is accepted") {
        auto status = std::make_unique<status_bar<Backend>>();

        std::string long_text(200, 'A');
        status->set_left_text(long_text);

        CHECK(status->get_left_text() == long_text);
    }

    TEST_CASE("Status bar size - single line height") {
        auto status = std::make_unique<status_bar<Backend>>();
        status->set_left_text("Some text");

        // Measure
        auto size = status->measure(80, 25);

        // Status bar should be single line height
        CHECK(size_utils::get_height(size) == 1);
    }

    TEST_CASE("Update text dynamically") {
        auto status = std::make_unique<status_bar<Backend>>();

        // Simulate clock updates
        status->set_right_text("12:00:00");
        CHECK(status->get_right_text() == "12:00:00");

        status->set_right_text("12:00:01");
        CHECK(status->get_right_text() == "12:00:01");

        status->set_right_text("12:00:02");
        CHECK(status->get_right_text() == "12:00:02");
    }

    TEST_CASE("Left text independent of right text") {
        auto status = std::make_unique<status_bar<Backend>>();

        status->set_left_text("Help");
        CHECK(status->get_right_text().empty());

        status->set_right_text("Status");
        CHECK(status->get_left_text() == "Help");
    }

    TEST_CASE("Special characters in text") {
        auto status = std::make_unique<status_bar<Backend>>();

        status->set_left_text("F1 Help   Alt+X Exit");
        status->set_right_text("→ ← ↑ ↓");

        CHECK(status->get_left_text() == "F1 Help   Alt+X Exit");
        CHECK(status->get_right_text() == "→ ← ↑ ↓");
    }

    TEST_CASE("Norton Utilities style status bar") {
        auto status = std::make_unique<status_bar<Backend>>();

        // Typical NU8 status bar
        status->set_left_text("F1 Help   Alt-X Exit");
        status->set_right_text("Norton Utilities 8.0");

        CHECK(status->get_left_text() == "F1 Help   Alt-X Exit");
        CHECK(status->get_right_text() == "Norton Utilities 8.0");
    }
}
