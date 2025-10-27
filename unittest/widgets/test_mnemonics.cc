/**
 * @file test_mnemonics.cc
 * @brief Comprehensive tests for Phase 2 - Mnemonic Rendering
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Tests cover:
 * - styled_text structure and helper functions
 * - parse_mnemonic() with various inputs
 * - Widget mnemonic support (button, label)
 * - Theme integration
 * - Edge cases and error handling
 */

#include <doctest/doctest.h>
#include <memory>
#include <onyxui/widgets/styled_text.hh>
#include <onyxui/widgets/mnemonic_parser.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/ui_context.hh>
#include "onyxui/theme.hh"
#include "utils/test_backend.hh"

using namespace onyxui;

// ======================================================================
// Test Suite: styled_text - Basic Operations
// ======================================================================

TEST_SUITE("styled_text::basic") {
    TEST_CASE("text_segment construction") {
        using Backend = test_backend;
        using Font = Backend::renderer_type::font;

        Font const font{};
        text_segment<Backend> const segment{"Hello", font};

        CHECK(segment.text == "Hello");
    }

    TEST_CASE("text_segment default constructor") {
        using Backend = test_backend;
        text_segment<Backend> const segment;

        CHECK(segment.text.empty());
    }

    TEST_CASE("styled_text is a vector of segments") {
        using Backend = test_backend;
        using Font = Backend::renderer_type::font;

        styled_text<Backend> text;
        CHECK(text.empty());

        Font const font{};
        text.push_back(text_segment<Backend>{"Hello", font});
        CHECK(text.size() == 1);
    }

    TEST_CASE("make_plain_text creates single segment") {
        using Backend = test_backend;
        using Font = Backend::renderer_type::font;

        Font const font{};
        auto text = make_plain_text<Backend>("Hello World", font);

        REQUIRE(text.size() == 1);
        CHECK(text[0].text == "Hello World");
    }

    TEST_CASE("make_plain_text with empty string") {
        using Backend = test_backend;
        using Font = Backend::renderer_type::font;

        Font const font{};
        auto text = make_plain_text<Backend>("", font);

        REQUIRE(text.size() == 1);
        CHECK(text[0].text.empty());
    }
}

// ======================================================================
// Test Suite: styled_text - Helper Functions
// ======================================================================

TEST_SUITE("styled_text::helpers") {
    using Backend = test_backend;
    using Font = Backend::renderer_type::font;

    TEST_CASE("text_length counts total characters") {
        styled_text<Backend> text;
        Font const font{};

        text.push_back({"Hel", font});
        text.push_back({"lo", font});
        text.push_back({" World", font});

        CHECK(text_length(text) == 11);  // "Hel" + "lo" + " World"
    }

    TEST_CASE("text_length on empty styled_text") {
        styled_text<Backend> const text;
        CHECK(text_length(text) == 0);
    }

    TEST_CASE("text_length with empty segments") {
        styled_text<Backend> text;
        Font const font{};

        text.push_back({"", font});
        text.push_back({"Hello", font});
        text.push_back({"", font});

        CHECK(text_length(text) == 5);  // Only "Hello"
    }

    TEST_CASE("to_plain_text concatenates segments") {
        styled_text<Backend> text;
        Font font1{}, font2{};

        text.push_back({"Hello", font1});
        text.push_back({" ", font2});
        text.push_back({"World", font1});

        CHECK(to_plain_text(text) == "Hello World");
    }

    TEST_CASE("to_plain_text on empty styled_text") {
        styled_text<Backend> const text;
        CHECK(to_plain_text(text).empty());
    }

    TEST_CASE("is_empty detects empty styled_text") {
        styled_text<Backend> const text;
        CHECK(is_empty(text));
    }

    TEST_CASE("is_empty detects all-empty segments") {
        styled_text<Backend> text;
        Font const font{};

        text.push_back({"", font});
        text.push_back({"", font});

        CHECK(is_empty(text));
    }

    TEST_CASE("is_empty returns false for non-empty text") {
        styled_text<Backend> text;
        Font const font{};

        text.push_back({"", font});
        text.push_back({"Hi", font});

        CHECK(!is_empty(text));
    }
}

// ======================================================================
// Test Suite: parse_mnemonic - Basic Parsing
// ======================================================================

TEST_SUITE("parse_mnemonic::basic") {
    using Backend = test_backend;
    using Font = Backend::renderer_type::font;

    TEST_CASE("Parse simple mnemonic at start") {
        Font normal{}, mnemonic{};

        auto result = parse_mnemonic<Backend>("&Save", normal, mnemonic);

        REQUIRE(result.text.size() == 2);
        CHECK(result.text[0].text == "S");  // Mnemonic char
        CHECK(result.text[1].text == "ave");
        CHECK(result.mnemonic_char == 's');  // Normalized to lowercase
    }

    TEST_CASE("Parse mnemonic in middle") {
        Font normal{}, mnemonic{};

        auto result = parse_mnemonic<Backend>("E&xit", normal, mnemonic);

        REQUIRE(result.text.size() == 3);
        CHECK(result.text[0].text == "E");
        CHECK(result.text[1].text == "x");   // Mnemonic char
        CHECK(result.text[2].text == "it");
        CHECK(result.mnemonic_char == 'x');
    }

    TEST_CASE("Parse mnemonic at end") {
        Font normal{}, mnemonic{};

        auto result = parse_mnemonic<Backend>("Cop&y", normal, mnemonic);

        REQUIRE(result.text.size() == 2);
        CHECK(result.text[0].text == "Cop");
        CHECK(result.text[1].text == "y");   // Mnemonic char
        CHECK(result.mnemonic_char == 'y');
    }

    TEST_CASE("Parse uppercase mnemonic") {
        Font normal{}, mnemonic{};

        auto result = parse_mnemonic<Backend>("&File", normal, mnemonic);

        CHECK(result.mnemonic_char == 'f');  // Normalized to lowercase
    }

    TEST_CASE("Parse without mnemonic") {
        Font normal{}, mnemonic{};

        auto result = parse_mnemonic<Backend>("Plain Text", normal, mnemonic);

        REQUIRE(result.text.size() == 1);
        CHECK(result.text[0].text == "Plain Text");
        CHECK(result.mnemonic_char == '\0');  // No mnemonic
    }

    TEST_CASE("Parse empty string") {
        Font normal{}, mnemonic{};

        auto result = parse_mnemonic<Backend>("", normal, mnemonic);

        CHECK(result.text.empty());
        CHECK(result.mnemonic_char == '\0');
    }
}

// ======================================================================
// Test Suite: parse_mnemonic - Escape Sequences
// ======================================================================

TEST_SUITE("parse_mnemonic::escapes") {
    using Backend = test_backend;
    using Font = Backend::renderer_type::font;

    TEST_CASE("Parse double ampersand escape") {
        Font normal{}, mnemonic{};

        auto result = parse_mnemonic<Backend>("Save && Exit", normal, mnemonic);

        REQUIRE(result.text.size() == 1);
        CHECK(result.text[0].text == "Save & Exit");  // Single &
        CHECK(result.mnemonic_char == '\0');  // No mnemonic
    }

    TEST_CASE("Parse multiple escape sequences") {
        Font normal{}, mnemonic{};

        auto result = parse_mnemonic<Backend>("A && B && C", normal, mnemonic);

        REQUIRE(result.text.size() == 1);
        CHECK(result.text[0].text == "A & B & C");
        CHECK(result.mnemonic_char == '\0');
    }

    TEST_CASE("Parse mnemonic before escape") {
        Font normal{}, mnemonic{};

        auto result = parse_mnemonic<Backend>("&File && Edit", normal, mnemonic);

        // First & is mnemonic, second && is escape
        REQUIRE(result.text.size() == 2);
        CHECK(result.text[0].text == "F");
        CHECK(result.text[1].text == "ile & Edit");
        CHECK(result.mnemonic_char == 'f');
    }

    TEST_CASE("Parse trailing ampersand") {
        Font normal{}, mnemonic{};

        auto result = parse_mnemonic<Backend>("Text&", normal, mnemonic);

        // Trailing & is literal (no character after it)
        REQUIRE(result.text.size() == 1);
        CHECK(result.text[0].text == "Text&");
        CHECK(result.mnemonic_char == '\0');
    }
}

// ======================================================================
// Test Suite: parse_mnemonic - Multiple Mnemonics
// ======================================================================

TEST_SUITE("parse_mnemonic::multiple") {
    using Backend = test_backend;
    using Font = Backend::renderer_type::font;

    TEST_CASE("Only first mnemonic is recognized") {
        Font normal{}, mnemonic{};

        auto result = parse_mnemonic<Backend>("&File &Edit", normal, mnemonic);

        // First & is mnemonic, second & is literal
        CHECK(result.mnemonic_char == 'f');
        CHECK(to_plain_text(result.text) == "File &Edit");
    }

    TEST_CASE("Subsequent & after mnemonic are literal") {
        Font normal{}, mnemonic{};

        auto result = parse_mnemonic<Backend>("&A&B&C", normal, mnemonic);

        CHECK(result.mnemonic_char == 'a');
        CHECK(to_plain_text(result.text) == "A&B&C");
    }
}

// ======================================================================
// Test Suite: Mnemonic Helper Functions
// ======================================================================

TEST_SUITE("mnemonic_helpers") {
    TEST_CASE("has_mnemonic detects mnemonic syntax") {
        CHECK(has_mnemonic("&Save") == true);
        CHECK(has_mnemonic("E&xit") == true);
        CHECK(has_mnemonic("&F") == true);
    }

    TEST_CASE("has_mnemonic ignores escape sequences") {
        CHECK(has_mnemonic("Save && Exit") == false);
        CHECK(has_mnemonic("A && B && C") == false);
    }

    TEST_CASE("has_mnemonic with mixed syntax") {
        CHECK(has_mnemonic("&File && Edit") == true);  // Has mnemonic
        CHECK(has_mnemonic("Text&&") == false);  // Only escape
    }

    TEST_CASE("has_mnemonic with plain text") {
        CHECK(has_mnemonic("Plain Text") == false);
        CHECK(has_mnemonic("") == false);
    }

    TEST_CASE("has_mnemonic with trailing &") {
        CHECK(has_mnemonic("Text&") == false);  // No char after &
    }

    TEST_CASE("strip_mnemonic removes markers") {
        CHECK(strip_mnemonic("&Save") == "Save");
        CHECK(strip_mnemonic("E&xit") == "Exit");
        CHECK(strip_mnemonic("Cop&y") == "Copy");
    }

    TEST_CASE("strip_mnemonic handles escapes") {
        CHECK(strip_mnemonic("Save && Exit") == "Save & Exit");
        CHECK(strip_mnemonic("A && B") == "A & B");
    }

    TEST_CASE("strip_mnemonic with plain text") {
        CHECK(strip_mnemonic("Plain Text") == "Plain Text");
        CHECK(strip_mnemonic("") == "");
    }

    TEST_CASE("strip_mnemonic with multiple &") {
        CHECK(strip_mnemonic("&File &Edit") == "File &Edit");
        CHECK(strip_mnemonic("&A&B&C") == "A&B&C");
    }
}

// ======================================================================
// Test Suite: Button Mnemonic Support
// ======================================================================

TEST_SUITE("button::mnemonics") {
    using Backend = test_backend;

    TEST_CASE("Button has no mnemonic by default") {
        auto btn = std::make_unique<button<Backend>>();

        CHECK(!btn->has_mnemonic());
        CHECK(btn->get_mnemonic_char() == '\0');
    }

    TEST_CASE("Button set_text doesn't create mnemonic") {
        auto btn = std::make_unique<button<Backend>>();
        btn->set_text("&Save");

        // set_text treats & as literal
        CHECK(btn->text() == "&Save");
        CHECK(!btn->has_mnemonic());
    }

    TEST_CASE("Button set_mnemonic_text without theme") {
        auto btn = std::make_unique<button<Backend>>();

        // Mnemonic can be set even without theme (theme only needed for rendering fonts)
        btn->set_mnemonic_text("&Save");

        // Plain text is stored
        CHECK(btn->text() == "Save");
        // Mnemonic character is available (extracted from markup, no theme needed)
        CHECK(btn->has_mnemonic());
        CHECK(btn->get_mnemonic_char() == 's');
    }

    TEST_CASE("Button set_mnemonic_text with theme") {
        ui_theme<Backend> theme{};
        theme.name = "Test Theme";
        scoped_ui_context<Backend> ctx;
        ctx.themes().register_theme(std::move(theme));
        ctx.themes().set_current_theme("Test Theme");

        auto btn = std::make_unique<button<Backend>>();

        // Now set mnemonic text
        btn->set_mnemonic_text("&Save");

        CHECK(btn->text() == "Save");  // Plain text stripped
        CHECK(btn->has_mnemonic());
        CHECK(btn->get_mnemonic_char() == 's');
    }

    TEST_CASE("Button mnemonic with different positions") {
        ui_theme<Backend> theme{};
        theme.name = "Test Theme";
        scoped_ui_context<Backend> ctx;
        ctx.themes().register_theme(std::move(theme));
        ctx.themes().set_current_theme("Test Theme");

        auto btn = std::make_unique<button<Backend>>();

        // Start
        btn->set_mnemonic_text("&File");
        CHECK(btn->get_mnemonic_char() == 'f');

        // Middle
        btn->set_mnemonic_text("E&xit");
        CHECK(btn->get_mnemonic_char() == 'x');

        // End
        btn->set_mnemonic_text("Cop&y");
        CHECK(btn->get_mnemonic_char() == 'y');
    }

    TEST_CASE("Button mnemonic with escape sequence") {
        ui_theme<Backend> theme{};
        theme.name = "Test Theme";
        scoped_ui_context<Backend> ctx;
        ctx.themes().register_theme(std::move(theme));
        ctx.themes().set_current_theme("Test Theme");

        auto btn = std::make_unique<button<Backend>>();

        btn->set_mnemonic_text("Save && Exit");

        CHECK(btn->text() == "Save & Exit");
        CHECK(!btn->has_mnemonic());  // Escape, not mnemonic
        CHECK(btn->get_mnemonic_char() == '\0');
    }
}

// ======================================================================
// Test Suite: Label Mnemonic Support
// ======================================================================

TEST_SUITE("label::mnemonics") {
    using Backend = test_backend;

    TEST_CASE("Label has no mnemonic by default") {
        auto lbl = std::make_unique<label<Backend>>();

        CHECK(!lbl->has_mnemonic());
        CHECK(lbl->get_mnemonic_char() == '\0');
    }

    TEST_CASE("Label set_mnemonic_text without theme") {
        auto lbl = std::make_unique<label<Backend>>();

        // Mnemonic can be set even without theme (theme only needed for rendering fonts)
        lbl->set_mnemonic_text("&Name:");

        CHECK(lbl->text() == "Name:");
        // Mnemonic character is available (extracted from markup, no theme needed)
        CHECK(lbl->has_mnemonic());
        CHECK(lbl->get_mnemonic_char() == 'n');
    }

    TEST_CASE("Label set_mnemonic_text with theme") {
        ui_theme<Backend> theme{};
        theme.name = "Test Theme";
        scoped_ui_context<Backend> ctx;
        ctx.themes().register_theme(std::move(theme));
        ctx.themes().set_current_theme("Test Theme");

        auto lbl = std::make_unique<label<Backend>>();

        lbl->set_mnemonic_text("&Name:");

        CHECK(lbl->text() == "Name:");
        CHECK(lbl->has_mnemonic());
        CHECK(lbl->get_mnemonic_char() == 'n');
    }

    TEST_CASE("Label form field labels") {
        ui_theme<Backend> theme{};
        theme.name = "Test Theme";
        scoped_ui_context<Backend> ctx;
        ctx.themes().register_theme(std::move(theme));
        ctx.themes().set_current_theme("Test Theme");

        auto lbl1 = std::make_unique<label<Backend>>();
        auto lbl2 = std::make_unique<label<Backend>>();

        lbl1->set_mnemonic_text("&Username:");
        lbl2->set_mnemonic_text("&Password:");

        CHECK(lbl1->get_mnemonic_char() == 'u');
        CHECK(lbl2->get_mnemonic_char() == 'p');
    }
}

// ======================================================================
// Test Suite: Real-World Scenarios
// ======================================================================

TEST_SUITE("mnemonics::scenarios") {
    using Backend = test_backend;

    TEST_CASE("File menu with mnemonics") {
        ui_theme<Backend> theme{};
        theme.name = "Test Theme";
        scoped_ui_context<Backend> ctx;
        ctx.themes().register_theme(std::move(theme));
        ctx.themes().set_current_theme("Test Theme");

        auto file_btn = std::make_unique<button<Backend>>();
        auto new_btn = std::make_unique<button<Backend>>();
        auto open_btn = std::make_unique<button<Backend>>();
        auto save_btn = std::make_unique<button<Backend>>();
        auto exit_btn = std::make_unique<button<Backend>>();

        file_btn->set_mnemonic_text("&File");
        new_btn->set_mnemonic_text("&New");
        open_btn->set_mnemonic_text("&Open");
        save_btn->set_mnemonic_text("&Save");
        exit_btn->set_mnemonic_text("E&xit");

        CHECK(file_btn->get_mnemonic_char() == 'f');
        CHECK(new_btn->get_mnemonic_char() == 'n');
        CHECK(open_btn->get_mnemonic_char() == 'o');
        CHECK(save_btn->get_mnemonic_char() == 's');
        CHECK(exit_btn->get_mnemonic_char() == 'x');  // E is taken, use x
    }

    TEST_CASE("Form with labeled inputs") {
        ui_theme<Backend> theme{};
        theme.name = "Test Theme";
        scoped_ui_context<Backend> ctx;
        ctx.themes().register_theme(std::move(theme));
        ctx.themes().set_current_theme("Test Theme");

        auto name_label = std::make_unique<label<Backend>>();
        auto email_label = std::make_unique<label<Backend>>();
        auto password_label = std::make_unique<label<Backend>>();

        name_label->set_mnemonic_text("&Name:");
        email_label->set_mnemonic_text("&Email:");
        password_label->set_mnemonic_text("&Password:");

        CHECK(name_label->text() == "Name:");
        CHECK(email_label->text() == "Email:");
        CHECK(password_label->text() == "Password:");

        CHECK(name_label->get_mnemonic_char() == 'n');
        CHECK(email_label->get_mnemonic_char() == 'e');
        CHECK(password_label->get_mnemonic_char() == 'p');
    }

    TEST_CASE("Buttons with literal ampersands") {
        ui_theme<Backend> theme{};
        theme.name = "Test Theme";
        scoped_ui_context<Backend> ctx;
        ctx.themes().register_theme(std::move(theme));
        ctx.themes().set_current_theme("Test Theme");

        auto btn = std::make_unique<button<Backend>>();

        btn->set_mnemonic_text("Save && Exit");

        CHECK(btn->text() == "Save & Exit");
        CHECK(!btn->has_mnemonic());
    }

    TEST_CASE("Mixed plain text and mnemonic text") {
        ui_theme<Backend> theme{};
        theme.name = "Test Theme";
        scoped_ui_context<Backend> ctx;
        ctx.themes().register_theme(std::move(theme));
        ctx.themes().set_current_theme("Test Theme");

        auto btn1 = std::make_unique<button<Backend>>();
        auto btn2 = std::make_unique<button<Backend>>();

        btn1->set_text("Cancel");           // Plain text
        btn2->set_mnemonic_text("&OK");     // Mnemonic text

        CHECK(btn1->text() == "Cancel");
        CHECK(btn2->text() == "OK");

        CHECK(!btn1->has_mnemonic());
        CHECK(btn2->has_mnemonic());
    }
}

// ======================================================================
// Test Suite: Edge Cases
// ======================================================================

TEST_SUITE("mnemonics::edge_cases") {
    using Backend = test_backend;
    using Font = Backend::renderer_type::font;

    TEST_CASE("Single character mnemonic") {
        Font normal{}, mnemonic{};

        auto result = parse_mnemonic<Backend>("&X", normal, mnemonic);

        REQUIRE(result.text.size() == 1);
        CHECK(result.text[0].text == "X");
        CHECK(result.mnemonic_char == 'x');
    }

    TEST_CASE("Only ampersand") {
        Font normal{}, mnemonic{};

        auto result = parse_mnemonic<Backend>("&", normal, mnemonic);

        // Trailing &, no char after
        CHECK(result.mnemonic_char == '\0');
    }

    TEST_CASE("Consecutive mnemonics attempt") {
        Font normal{}, mnemonic{};

        auto result = parse_mnemonic<Backend>("&A&B", normal, mnemonic);

        // Only first & is mnemonic
        CHECK(result.mnemonic_char == 'a');
        CHECK(to_plain_text(result.text) == "A&B");
    }

    TEST_CASE("Whitespace as mnemonic") {
        Font normal{}, mnemonic{};

        auto result = parse_mnemonic<Backend>("Hello& World", normal, mnemonic);

        // Space is valid mnemonic character
        CHECK(result.mnemonic_char == ' ');
    }

    TEST_CASE("Digit as mnemonic") {
        Font normal{}, mnemonic{};

        auto result = parse_mnemonic<Backend>("Item &1", normal, mnemonic);

        CHECK(result.mnemonic_char == '1');
    }

    TEST_CASE("Punctuation as mnemonic") {
        Font normal{}, mnemonic{};

        auto result = parse_mnemonic<Backend>("&:", normal, mnemonic);

        CHECK(result.mnemonic_char == ':');
    }
}
