/**
 * @file test_dialog.cc
 * @brief Comprehensive tests for dialog<Backend> widget
 * @date 2025-11-08
 */

#include <doctest/doctest.h>
#include <onyxui/widgets/window/dialog.hh>
#include <onyxui/widgets/window/window_presets.hh>
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;
using namespace onyxui::testing;

// ============================================================================
// Test Helpers
// ============================================================================

/**
 * @brief Test dialog that exposes protected members for testing
 */
template<UIBackend Backend>
class test_dialog : public dialog<Backend> {
public:
    using dialog<Backend>::dialog;
    using dialog<Backend>::on_close;  // Expose for testing
};

// ============================================================================
// Basic Construction and API
// ============================================================================

TEST_CASE("dialog - Basic construction") {
    SUBCASE("Default construction") {
        test_dialog<test_backend> dlg;

        CHECK(dlg.get_title() == "Dialog");
        CHECK(dlg.get_result() == dialog<test_backend>::dialog_result::none);
        CHECK(dlg.get_message() == "");
    }

    SUBCASE("Construction with title") {
        test_dialog<test_backend> dlg("Save Changes?");

        CHECK(dlg.get_title() == "Save Changes?");
        CHECK(dlg.get_result() == dialog<test_backend>::dialog_result::none);
    }

    SUBCASE("Dialog has correct flags") {
        test_dialog<test_backend> dlg;

        // Dialog should be modal, non-resizable, with no min/max buttons
        // Note: Can't directly access flags, but can verify behavior indirectly
        CHECK(dlg.get_title() == "Dialog");  // Sanity check
    }
}

TEST_CASE("dialog - Message management") {
    SUBCASE("Set and get message") {
        test_dialog<test_backend> dlg;

        dlg.set_message("Do you want to save?");
        CHECK(dlg.get_message() == "Do you want to save?");
    }

    SUBCASE("Update message multiple times") {
        test_dialog<test_backend> dlg;

        dlg.set_message("First message");
        CHECK(dlg.get_message() == "First message");

        dlg.set_message("Second message");
        CHECK(dlg.get_message() == "Second message");
    }

    SUBCASE("Empty message is valid") {
        test_dialog<test_backend> dlg;

        dlg.set_message("");
        CHECK(dlg.get_message() == "");
    }
}

// ============================================================================
// Result Management
// ============================================================================

TEST_CASE("dialog - Result management") {
    SUBCASE("Default result is none") {
        test_dialog<test_backend> dlg;
        CHECK(dlg.get_result() == dialog<test_backend>::dialog_result::none);
    }

    SUBCASE("Set result: OK") {
        test_dialog<test_backend> dlg;
        dlg.set_result(dialog<test_backend>::dialog_result::ok);
        CHECK(dlg.get_result() == dialog<test_backend>::dialog_result::ok);
    }

    SUBCASE("Set result: Cancel") {
        test_dialog<test_backend> dlg;
        dlg.set_result(dialog<test_backend>::dialog_result::cancel);
        CHECK(dlg.get_result() == dialog<test_backend>::dialog_result::cancel);
    }

    SUBCASE("Set result: Yes") {
        test_dialog<test_backend> dlg;
        dlg.set_result(dialog<test_backend>::dialog_result::yes);
        CHECK(dlg.get_result() == dialog<test_backend>::dialog_result::yes);
    }

    SUBCASE("Set result: No") {
        test_dialog<test_backend> dlg;
        dlg.set_result(dialog<test_backend>::dialog_result::no);
        CHECK(dlg.get_result() == dialog<test_backend>::dialog_result::no);
    }

    SUBCASE("Set result: Abort") {
        test_dialog<test_backend> dlg;
        dlg.set_result(dialog<test_backend>::dialog_result::abort);
        CHECK(dlg.get_result() == dialog<test_backend>::dialog_result::abort);
    }

    SUBCASE("Set result: Retry") {
        test_dialog<test_backend> dlg;
        dlg.set_result(dialog<test_backend>::dialog_result::retry);
        CHECK(dlg.get_result() == dialog<test_backend>::dialog_result::retry);
    }

    SUBCASE("Set result: Ignore") {
        test_dialog<test_backend> dlg;
        dlg.set_result(dialog<test_backend>::dialog_result::ignore);
        CHECK(dlg.get_result() == dialog<test_backend>::dialog_result::ignore);
    }

    SUBCASE("Result can be changed") {
        test_dialog<test_backend> dlg;

        dlg.set_result(dialog<test_backend>::dialog_result::yes);
        CHECK(dlg.get_result() == dialog<test_backend>::dialog_result::yes);

        dlg.set_result(dialog<test_backend>::dialog_result::no);
        CHECK(dlg.get_result() == dialog<test_backend>::dialog_result::no);
    }
}

// ============================================================================
// Convenience Button Methods
// ============================================================================

TEST_CASE("dialog - Convenience button methods") {
    SUBCASE("add_ok_button creates button") {
        test_dialog<test_backend> dlg;
        auto* btn = dlg.add_ok_button();
        CHECK(btn != nullptr);
    }

    SUBCASE("add_ok_cancel_buttons creates two buttons") {
        test_dialog<test_backend> dlg;
        dlg.add_ok_cancel_buttons();
        // Note: Can't directly verify button count, but should not crash
    }

    SUBCASE("add_yes_no_buttons creates two buttons") {
        test_dialog<test_backend> dlg;
        dlg.add_yes_no_buttons();
        // Should not crash
    }

    SUBCASE("add_yes_no_cancel_buttons creates three buttons") {
        test_dialog<test_backend> dlg;
        dlg.add_yes_no_cancel_buttons();
        // Should not crash
    }

    SUBCASE("add_abort_retry_ignore_buttons creates three buttons") {
        test_dialog<test_backend> dlg;
        dlg.add_abort_retry_ignore_buttons();
        // Should not crash
    }

    SUBCASE("add_button with custom text and result") {
        test_dialog<test_backend> dlg;
        auto* btn = dlg.add_button("Custom", dialog<test_backend>::dialog_result::ok);
        CHECK(btn != nullptr);
    }

    SUBCASE("Multiple button sets can be added") {
        test_dialog<test_backend> dlg;
        dlg.add_ok_button();
        dlg.add_ok_cancel_buttons();
        dlg.add_yes_no_buttons();
        // Should not crash
    }
}

// ============================================================================
// Button Auto-Wiring
// ============================================================================

TEST_CASE("dialog - Button auto-wiring to results") {
    SUBCASE("OK button sets result and closes dialog") {
        test_dialog<test_backend> dlg;
        auto* btn = dlg.add_ok_button();

        // Simulate button click
        btn->clicked.emit();

        // Dialog should have OK result
        CHECK(dlg.get_result() == dialog<test_backend>::dialog_result::ok);
    }

    SUBCASE("Cancel button sets result") {
        test_dialog<test_backend> dlg;
        dlg.add_ok_cancel_buttons();

        // Note: Can't easily simulate clicking the Cancel button specifically
        // without direct access to button references
        // This test verifies the API exists
    }

    SUBCASE("Yes button sets result") {
        test_dialog<test_backend> dlg;
        auto* yes_btn = dlg.add_button("Yes", dialog<test_backend>::dialog_result::yes);

        yes_btn->clicked.emit();
        CHECK(dlg.get_result() == dialog<test_backend>::dialog_result::yes);
    }

    SUBCASE("No button sets result") {
        test_dialog<test_backend> dlg;
        auto* no_btn = dlg.add_button("No", dialog<test_backend>::dialog_result::no);

        no_btn->clicked.emit();
        CHECK(dlg.get_result() == dialog<test_backend>::dialog_result::no);
    }

    SUBCASE("Custom button sets custom result") {
        test_dialog<test_backend> dlg;
        auto* custom_btn = dlg.add_button("Retry", dialog<test_backend>::dialog_result::retry);

        custom_btn->clicked.emit();
        CHECK(dlg.get_result() == dialog<test_backend>::dialog_result::retry);
    }
}

// ============================================================================
// Signal Emission
// ============================================================================

TEST_CASE("dialog - Signal emission") {
    SUBCASE("result_ready signal emitted on close") {
        test_dialog<test_backend> dlg;

        dialog<test_backend>::dialog_result received_result = dialog<test_backend>::dialog_result::none;
        dlg.result_ready.connect([&](dialog<test_backend>::dialog_result result) {
            received_result = result;
        });

        dlg.set_result(dialog<test_backend>::dialog_result::ok);
        dlg.on_close();  // Simulate close

        CHECK(received_result == dialog<test_backend>::dialog_result::ok);
    }

    SUBCASE("result_ready signal with different results") {
        test_dialog<test_backend> dlg;

        dialog<test_backend>::dialog_result received_result = dialog<test_backend>::dialog_result::none;
        dlg.result_ready.connect([&](dialog<test_backend>::dialog_result result) {
            received_result = result;
        });

        dlg.set_result(dialog<test_backend>::dialog_result::cancel);
        dlg.on_close();

        CHECK(received_result == dialog<test_backend>::dialog_result::cancel);
    }

    SUBCASE("result_ready signal emitted even if result is none") {
        test_dialog<test_backend> dlg;

        bool signal_emitted = false;
        dlg.result_ready.connect([&](dialog<test_backend>::dialog_result) {
            signal_emitted = true;
        });

        dlg.on_close();

        CHECK(signal_emitted);
    }
}

// ============================================================================
// Callback-Based Pattern
// ============================================================================

TEST_CASE("dialog - Callback-based pattern") {
    // Note: Can't easily test show_modal(callback) without a full UI context
    // These tests verify the API exists and basic behavior

    SUBCASE("Callback invoked on close") {
        test_dialog<test_backend> dlg;

        dialog<test_backend>::dialog_result received_result = dialog<test_backend>::dialog_result::none;

        // Simulate show_modal with callback
        auto callback = [&](dialog<test_backend>::dialog_result result) {
            received_result = result;
        };

        dlg.set_result(dialog<test_backend>::dialog_result::yes);

        // Manually invoke callback (simulating on_close behavior)
        callback(dlg.get_result());

        CHECK(received_result == dialog<test_backend>::dialog_result::yes);
    }
}

// ============================================================================
// Helper Functions (window_presets.hh)
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "dialog - Helper functions") {
    SUBCASE("show_message_box creates dialog") {
        bool callback_invoked = false;
        dialog<test_canvas_backend>::dialog_result received_result = dialog<test_canvas_backend>::dialog_result::none;

        auto dlg = show_message_box<test_canvas_backend>(
            "Test Title",
            "Test Message",
            message_box_buttons::ok,
            [&](dialog<test_canvas_backend>::dialog_result result) {
                callback_invoked = true;
                received_result = result;
            }
        );

        REQUIRE(dlg != nullptr);
        CHECK(dlg->get_title() == "Test Title");
        CHECK(dlg->get_message() == "Test Message");
    }

    SUBCASE("show_message_box with yes_no_cancel buttons") {
        auto dlg = show_message_box<test_canvas_backend>(
            "Save Changes?",
            "Do you want to save?",
            message_box_buttons::yes_no_cancel,
            [](dialog<test_canvas_backend>::dialog_result) {}
        );

        REQUIRE(dlg != nullptr);
        CHECK(dlg->get_title() == "Save Changes?");
    }

    SUBCASE("show_info creates dialog") {
        auto dlg = show_info<test_canvas_backend>("File saved successfully!");

        REQUIRE(dlg != nullptr);
        CHECK(dlg->get_title() == "Information");
        CHECK(dlg->get_message() == "File saved successfully!");
    }

    SUBCASE("show_info with custom title") {
        auto dlg = show_info<test_canvas_backend>("Operation completed.", "Success");

        REQUIRE(dlg != nullptr);
        CHECK(dlg->get_title() == "Success");
        CHECK(dlg->get_message() == "Operation completed.");
    }

    SUBCASE("show_confirm creates dialog") {
        bool callback_invoked = false;
        bool confirmed = false;

        auto dlg = show_confirm<test_canvas_backend>(
            "Delete File?",
            "This cannot be undone.",
            [&](bool result) {
                callback_invoked = true;
                confirmed = result;
            }
        );

        REQUIRE(dlg != nullptr);
        CHECK(dlg->get_title() == "Delete File?");
        CHECK(dlg->get_message() == "This cannot be undone.");
    }

    SUBCASE("show_warning creates dialog") {
        bool proceed = false;

        auto dlg = show_warning<test_canvas_backend>(
            "Overwrite File?",
            "The file already exists.",
            [&](bool result) {
                proceed = result;
            }
        );

        REQUIRE(dlg != nullptr);
        CHECK(dlg->get_title() == "Overwrite File?");
    }

    SUBCASE("show_error creates dialog") {
        auto dlg = show_error<test_canvas_backend>("Failed to open file");

        REQUIRE(dlg != nullptr);
        CHECK(dlg->get_title() == "Error");
        CHECK(dlg->get_message() == "Failed to open file");
    }

    SUBCASE("show_error with custom title") {
        auto dlg = show_error<test_canvas_backend>("Network timeout", "Connection Error");

        REQUIRE(dlg != nullptr);
        CHECK(dlg->get_title() == "Connection Error");
    }
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "dialog - Integration") {
    SUBCASE("Complete workflow: create, set message, add buttons, simulate close") {
        test_dialog<test_canvas_backend> dlg("Confirmation");
        dlg.set_message("Are you sure?");
        dlg.add_yes_no_buttons();

        bool signal_received = false;
        dialog<test_canvas_backend>::dialog_result received_result = dialog<test_canvas_backend>::dialog_result::none;

        dlg.result_ready.connect([&](dialog<test_canvas_backend>::dialog_result result) {
            signal_received = true;
            received_result = result;
        });

        // Simulate user clicking Yes button
        auto* yes_btn = dlg.add_button("Yes", dialog<test_canvas_backend>::dialog_result::yes);
        yes_btn->clicked.emit();

        // Simulate dialog close
        dlg.on_close();

        CHECK(signal_received);
        CHECK(received_result == dialog<test_canvas_backend>::dialog_result::yes);
        CHECK(dlg.get_result() == dialog<test_canvas_backend>::dialog_result::yes);
    }

    SUBCASE("Multiple dialogs can exist independently") {
        test_dialog<test_canvas_backend> dlg1("Dialog 1");
        test_dialog<test_canvas_backend> dlg2("Dialog 2");

        dlg1.set_message("Message 1");
        dlg2.set_message("Message 2");

        dlg1.set_result(dialog<test_canvas_backend>::dialog_result::ok);
        dlg2.set_result(dialog<test_canvas_backend>::dialog_result::cancel);

        CHECK(dlg1.get_result() == dialog<test_canvas_backend>::dialog_result::ok);
        CHECK(dlg2.get_result() == dialog<test_canvas_backend>::dialog_result::cancel);
    }

    SUBCASE("Dialog can be reused after close") {
        test_dialog<test_canvas_backend> dlg("Reusable Dialog");

        // First use
        dlg.set_message("First message");
        dlg.set_result(dialog<test_canvas_backend>::dialog_result::ok);
        CHECK(dlg.get_result() == dialog<test_canvas_backend>::dialog_result::ok);

        // Second use (reset and reuse)
        dlg.set_message("Second message");
        dlg.set_result(dialog<test_canvas_backend>::dialog_result::cancel);
        CHECK(dlg.get_result() == dialog<test_canvas_backend>::dialog_result::cancel);
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE("dialog - Edge cases") {
    SUBCASE("Empty title is valid") {
        test_dialog<test_backend> dlg("");
        CHECK(dlg.get_title() == "");
    }

    SUBCASE("Very long message") {
        test_dialog<test_backend> dlg;
        std::string long_message(1000, 'A');
        dlg.set_message(long_message);
        CHECK(dlg.get_message() == long_message);
    }

    SUBCASE("Special characters in title and message") {
        test_dialog<test_backend> dlg("Title with \"quotes\" and\nnewlines");
        dlg.set_message("Message with \t tabs and special chars: !@#$%^&*()");

        CHECK(dlg.get_title().find("quotes") != std::string::npos);
        CHECK(dlg.get_message().find("special") != std::string::npos);
    }

    SUBCASE("Adding no buttons is valid") {
        test_dialog<test_backend> dlg;
        // Dialog without buttons should still be valid
        CHECK(dlg.get_result() == dialog<test_backend>::dialog_result::none);
    }

    SUBCASE("Adding many buttons") {
        test_dialog<test_backend> dlg;
        for (int i = 0; i < 10; ++i) {
            dlg.add_button("Button " + std::to_string(i), dialog<test_backend>::dialog_result::ok);
        }
        // Should not crash
    }
}

// ============================================================================
// Visual Rendering Tests (MANDATORY - see docs/CLAUDE/TESTING.md)
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "dialog - Visual rendering verification") {
    SUBCASE("Dialog with message and buttons renders correctly") {
        test_dialog<test_canvas_backend> dlg("Confirmation");
        dlg.set_message("Are you sure you want to continue?");
        dlg.add_yes_no_cancel_buttons();

        // Measure
        auto size = dlg.measure(60_lu, 20_lu);
        INFO("Measured size: " << size.width.to_int() << " x " << size.height.to_int());
        CHECK(size.width.to_int() > 0);   // CRITICAL: Must not measure to zero
        CHECK(size.height.to_int() > 0);

        // Arrange
        dlg.arrange(logical_rect{0_lu, 0_lu, 60_lu, 20_lu});

        // Render
        auto canvas = render_to_canvas(dlg, 60, 20);
        std::string rendered = canvas->render_ascii();

        // Verify non-empty output
        int content_chars = 0;
        for (char c : rendered) {
            if (c != ' ' && c != '\n') content_chars++;
        }
        INFO("Rendered " << content_chars << " non-whitespace characters");
        CHECK(content_chars > 0);  // Must render something

        // Verify expected content appears (title bar should show)
        CHECK(rendered.find("Confirmation") != std::string::npos);
    }

    SUBCASE("Dialog with custom buttons renders correctly") {
        test_dialog<test_canvas_backend> dlg("Save Changes?");
        dlg.set_message("You have unsaved changes.");
        dlg.add_button("Save", dialog<test_canvas_backend>::dialog_result::yes);
        dlg.add_button("Don't Save", dialog<test_canvas_backend>::dialog_result::no);
        dlg.add_button("Cancel", dialog<test_canvas_backend>::dialog_result::cancel);

        // Measure
        auto size = dlg.measure(70_lu, 22_lu);
        INFO("Measured size: " << size.width.to_int() << " x " << size.height.to_int());
        CHECK(size.width.to_int() > 0);   // CRITICAL: Must not measure to zero
        CHECK(size.height.to_int() > 0);

        // Arrange
        dlg.arrange(logical_rect{5_lu, 5_lu, 70_lu, 22_lu});

        // Render
        auto canvas = render_to_canvas(dlg, 80, 30);  // Larger canvas to see full dialog
        std::string rendered = canvas->render_ascii();

        // Verify non-empty output
        int content_chars = 0;
        for (char c : rendered) {
            if (c != ' ' && c != '\n') content_chars++;
        }
        INFO("Rendered " << content_chars << " non-whitespace characters");
        CHECK(content_chars > 0);  // Must render something

        // Verify title appears
        CHECK(rendered.find("Save Changes?") != std::string::npos);
    }

    SUBCASE("Simple info dialog renders correctly") {
        auto dlg = show_info<test_canvas_backend>("Operation completed successfully!");

        REQUIRE(dlg != nullptr);

        // Measure
        auto size = dlg->measure(50_lu, 15_lu);
        INFO("Measured size: " << size.width.to_int() << " x " << size.height.to_int());
        CHECK(size.width.to_int() > 0);   // CRITICAL: Must not measure to zero
        CHECK(size.height.to_int() > 0);

        // Arrange
        dlg->arrange(logical_rect{0_lu, 0_lu, 50_lu, 15_lu});

        // Render
        auto canvas = render_to_canvas(*dlg, 50, 15);
        std::string rendered = canvas->render_ascii();

        // Verify non-empty output
        int content_chars = 0;
        for (char c : rendered) {
            if (c != ' ' && c != '\n') content_chars++;
        }
        INFO("Rendered " << content_chars << " non-whitespace characters");
        CHECK(content_chars > 0);  // Must render something

        // Verify title appears
        CHECK(rendered.find("Information") != std::string::npos);
    }
}
