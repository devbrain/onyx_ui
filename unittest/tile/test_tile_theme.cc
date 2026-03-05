/**
 * @file test_tile_theme.cc
 * @brief Unit tests for tile theme and widget tile configurations
 */

#include <doctest/doctest.h>
#include <onyxui/tile/tile_theme.hh>

using namespace onyxui::tile;

// ============================================================================
// button_tiles Tests
// ============================================================================

TEST_CASE("button_tiles - default construction") {
    button_tiles tiles;
    CHECK_FALSE(tiles.normal.is_valid());
    CHECK_FALSE(tiles.hover.is_valid());
    CHECK_FALSE(tiles.pressed.is_valid());
    CHECK_FALSE(tiles.disabled.is_valid());
    CHECK_FALSE(tiles.focused.is_valid());
}

TEST_CASE("button_tiles - designated initializer") {
    button_tiles tiles{
        .normal = {0,1,2, 8,9,10, 16,17,18, 4, 4},
        .hover = {3,4,5, 11,12,13, 19,20,21, 4, 4},
        .pressed = {6,7,8, 14,15,16, 22,23,24, 4, 4}
    };

    CHECK(tiles.normal.is_valid());
    CHECK(tiles.normal.top_left == 0);
    CHECK(tiles.normal.margin_h == 4);
    CHECK(tiles.normal.margin_v == 4);

    CHECK(tiles.hover.is_valid());
    CHECK(tiles.hover.top_left == 3);

    CHECK(tiles.pressed.is_valid());
    CHECK(tiles.pressed.top_left == 6);

    // Disabled not set
    CHECK_FALSE(tiles.disabled.is_valid());
}

// ============================================================================
// progress_bar_tiles Tests
// ============================================================================

TEST_CASE("progress_bar_tiles - default construction") {
    progress_bar_tiles tiles;
    CHECK_FALSE(tiles.track.is_valid());
    CHECK_FALSE(tiles.fill.is_valid());
}

TEST_CASE("progress_bar_tiles - with different start/end caps") {
    progress_bar_tiles tiles{
        .track = {.left = 48, .center = 49, .right = 50, .margin = 4},
        .fill = {.left = 51, .center = 52, .right = 53, .margin = 4}
    };

    CHECK(tiles.track.is_valid());
    CHECK(tiles.track.left == 48);
    CHECK(tiles.track.center == 49);
    CHECK(tiles.track.right == 50);

    CHECK(tiles.fill.is_valid());
    CHECK(tiles.fill.left == 51);
    CHECK(tiles.fill.center == 52);
    CHECK(tiles.fill.right == 53);
}

// ============================================================================
// scrollbar_tiles Tests
// ============================================================================

TEST_CASE("scrollbar_tiles - default construction") {
    scrollbar_tiles tiles;
    CHECK_FALSE(tiles.track_v.is_valid());
    CHECK_FALSE(tiles.thumb_v.is_valid());
    CHECK(tiles.arrow_up == -1);
    CHECK(tiles.arrow_down == -1);
    CHECK_FALSE(tiles.track_h.is_valid());
    CHECK_FALSE(tiles.thumb_h.is_valid());
    CHECK(tiles.arrow_left == -1);
    CHECK(tiles.arrow_right == -1);
}

TEST_CASE("scrollbar_tiles - vertical and horizontal") {
    scrollbar_tiles tiles{
        .track_v = {.top = 60, .center = 61, .bottom = 62, .margin = 4},
        .thumb_v = {.top = 63, .center = 64, .bottom = 65, .margin = 4},
        .arrow_up = 66,
        .arrow_down = 67,
        .track_h = {.left = 70, .center = 71, .right = 72, .margin = 4},
        .thumb_h = {.left = 73, .center = 74, .right = 75, .margin = 4},
        .arrow_left = 76,
        .arrow_right = 77
    };

    CHECK(tiles.track_v.is_valid());
    CHECK(tiles.thumb_v.is_valid());
    CHECK(tiles.arrow_up == 66);
    CHECK(tiles.arrow_down == 67);

    CHECK(tiles.track_h.is_valid());
    CHECK(tiles.thumb_h.is_valid());
    CHECK(tiles.arrow_left == 76);
    CHECK(tiles.arrow_right == 77);
}

// ============================================================================
// checkbox_tiles / radio_tiles Tests
// ============================================================================

TEST_CASE("checkbox_tiles - default construction") {
    checkbox_tiles tiles;
    CHECK(tiles.unchecked == -1);
    CHECK(tiles.checked == -1);
    CHECK(tiles.indeterminate == -1);
}

TEST_CASE("checkbox_tiles - basic setup") {
    checkbox_tiles tiles{
        .unchecked = 64,
        .checked = 65,
        .indeterminate = 66
    };

    CHECK(tiles.unchecked == 64);
    CHECK(tiles.checked == 65);
    CHECK(tiles.indeterminate == 66);
}

TEST_CASE("radio_tiles - basic setup") {
    radio_tiles tiles{
        .unchecked = 80,
        .checked = 81
    };

    CHECK(tiles.unchecked == 80);
    CHECK(tiles.checked == 81);
}

// ============================================================================
// slider_tiles Tests
// ============================================================================

TEST_CASE("slider_tiles - default construction") {
    slider_tiles tiles;
    CHECK_FALSE(tiles.track.is_valid());
    CHECK(tiles.thumb == -1);
    CHECK(tiles.thumb_hover == -1);
    CHECK(tiles.thumb_pressed == -1);
}

TEST_CASE("slider_tiles - with states") {
    slider_tiles tiles{
        .track = {.left = 90, .center = 91, .right = 92, .margin = 4},
        .thumb = 93,
        .thumb_hover = 94,
        .thumb_pressed = 95
    };

    CHECK(tiles.track.is_valid());
    CHECK(tiles.thumb == 93);
    CHECK(tiles.thumb_hover == 94);
    CHECK(tiles.thumb_pressed == 95);
}

// ============================================================================
// text_input_tiles Tests
// ============================================================================

TEST_CASE("text_input_tiles - default construction") {
    text_input_tiles tiles;
    CHECK_FALSE(tiles.normal.is_valid());
    CHECK_FALSE(tiles.focused.is_valid());
    CHECK_FALSE(tiles.disabled.is_valid());
    CHECK(tiles.cursor == -1);
}

TEST_CASE("text_input_tiles - with cursor") {
    text_input_tiles tiles{
        .normal = {0,1,2, 8,9,10, 16,17,18, 2, 2},
        .focused = {3,4,5, 11,12,13, 19,20,21, 2, 2},
        .cursor = 100
    };

    CHECK(tiles.normal.is_valid());
    CHECK(tiles.focused.is_valid());
    CHECK(tiles.cursor == 100);
}

// ============================================================================
// combo_tiles Tests
// ============================================================================

TEST_CASE("combo_tiles - default construction") {
    combo_tiles tiles;
    CHECK_FALSE(tiles.button.is_valid());
    CHECK(tiles.arrow == -1);
    CHECK_FALSE(tiles.dropdown.is_valid());
    CHECK_FALSE(tiles.item_hover.is_valid());
}

TEST_CASE("combo_tiles - full setup") {
    combo_tiles tiles{
        .button = {0,1,2, 8,9,10, 16,17,18, 4, 4},
        .arrow = 110,
        .dropdown = {32,33,34, 40,41,42, 48,49,50, 4, 4},
        .item_hover = {64,65,66, 72,73,74, 80,81,82, 2, 2}
    };

    CHECK(tiles.button.is_valid());
    CHECK(tiles.arrow == 110);
    CHECK(tiles.dropdown.is_valid());
    CHECK(tiles.item_hover.is_valid());
}

// ============================================================================
// window_tiles Tests
// ============================================================================

TEST_CASE("window_tiles - default construction") {
    window_tiles tiles;
    CHECK_FALSE(tiles.frame.is_valid());
    CHECK_FALSE(tiles.titlebar.is_valid());
    CHECK(tiles.close_button == -1);
    CHECK(tiles.minimize_button == -1);
    CHECK(tiles.maximize_button == -1);
}

TEST_CASE("window_tiles - with all buttons") {
    window_tiles tiles{
        .frame = {0,1,2, 8,9,10, 16,17,18, 8, 8},
        .titlebar = {32,33,34, 40,41,42, 48,49,50, 4, 4},
        .close_button = 120,
        .close_hover = 121,
        .minimize_button = 122,
        .maximize_button = 124
    };

    CHECK(tiles.frame.is_valid());
    CHECK(tiles.titlebar.is_valid());
    CHECK(tiles.close_button == 120);
    CHECK(tiles.close_hover == 121);
    CHECK(tiles.minimize_button == 122);
    CHECK(tiles.maximize_button == 124);
}

// ============================================================================
// animation_settings Tests
// ============================================================================

TEST_CASE("animation_settings - default values") {
    animation_settings settings;
    CHECK(settings.modal_open_ms == 200);
    CHECK(settings.modal_close_ms == 150);
    CHECK(settings.tooltip_delay_ms == 500);
    CHECK(settings.tooltip_fade_ms == 100);
    CHECK(settings.blink_interval_ms == 500);
    CHECK(settings.enable_animations == true);
}

TEST_CASE("animation_settings - custom values") {
    animation_settings settings{
        .modal_open_ms = 300,
        .modal_close_ms = 200,
        .tooltip_delay_ms = 1000,
        .enable_animations = false
    };

    CHECK(settings.modal_open_ms == 300);
    CHECK(settings.modal_close_ms == 200);
    CHECK(settings.tooltip_delay_ms == 1000);
    CHECK(settings.enable_animations == false);
}

// ============================================================================
// tile_theme Tests
// ============================================================================

TEST_CASE("tile_theme - default construction") {
    tile_theme theme;
    CHECK(theme.atlas == nullptr);
    CHECK_FALSE(theme.is_valid());

    // All widget tiles should be default (invalid)
    CHECK_FALSE(theme.button.normal.is_valid());
    CHECK_FALSE(theme.panel.background.is_valid());
    CHECK_FALSE(theme.progress_bar.track.is_valid());
}

TEST_CASE("tile_theme - validity requires atlas") {
    tile_theme theme;
    CHECK_FALSE(theme.is_valid());

    tile_atlas atlas{
        .texture = reinterpret_cast<void*>(1),
        .tile_width = 16,
        .tile_height = 16,
        .columns = 16
    };

    theme.atlas = &atlas;
    CHECK(theme.is_valid());
}

TEST_CASE("tile_theme - partial configuration") {
    // Games may only configure widgets they use
    tile_atlas atlas{
        .texture = reinterpret_cast<void*>(1),
        .tile_width = 16,
        .tile_height = 16,
        .columns = 16
    };

    tile_theme theme{
        .atlas = &atlas,
        .button = {
            .normal = {0,1,2, 8,9,10, 16,17,18, 4, 4}
        },
        .checkbox = {
            .unchecked = 64,
            .checked = 65
        },
        .font_normal = {
            .atlas = &atlas,
            .glyph_width = 8,
            .glyph_height = 12,
            .first_tile_id = 256
        }
    };

    CHECK(theme.is_valid());
    CHECK(theme.button.normal.is_valid());
    CHECK(theme.checkbox.unchecked == 64);
    CHECK(theme.font_normal.is_valid());

    // Unused widgets remain invalid (that's OK)
    CHECK_FALSE(theme.panel.background.is_valid());
    CHECK_FALSE(theme.progress_bar.track.is_valid());
}

TEST_CASE("tile_theme - complete game theme example") {
    tile_atlas atlas{
        .texture = reinterpret_cast<void*>(1),
        .tile_width = 16,
        .tile_height = 16,
        .columns = 16
    };

    tile_theme theme{
        .atlas = &atlas,

        // Button with all states
        .button = {
            .normal = {0,1,2, 16,17,18, 32,33,34, 4, 4},
            .hover = {3,4,5, 19,20,21, 35,36,37, 4, 4},
            .pressed = {6,7,8, 22,23,24, 38,39,40, 4, 4},
            .disabled = {9,10,11, 25,26,27, 41,42,43, 4, 4}
        },

        // Panel
        .panel = {
            .background = {48,49,50, 64,65,66, 80,81,82, 8, 8}
        },

        // Progress bar
        .progress_bar = {
            .track = {96, 97, 98, 4},
            .fill = {99, 100, 101, 4}
        },

        // Checkbox
        .checkbox = {
            .unchecked = 112,
            .checked = 113
        },

        // Fonts
        .font_normal = {
            .atlas = &atlas,
            .glyph_width = 8,
            .glyph_height = 12,
            .first_char = 32,
            .char_count = 96,
            .first_tile_id = 256
        },

        .font_disabled = {
            .atlas = &atlas,
            .glyph_width = 8,
            .glyph_height = 12,
            .first_char = 32,
            .char_count = 96,
            .first_tile_id = 352
        },

        // Animation settings
        .animations = {
            .modal_open_ms = 150,
            .enable_animations = true
        }
    };

    CHECK(theme.is_valid());
    CHECK(theme.button.normal.is_valid());
    CHECK(theme.button.hover.is_valid());
    CHECK(theme.button.pressed.is_valid());
    CHECK(theme.button.disabled.is_valid());
    CHECK(theme.panel.background.is_valid());
    CHECK(theme.progress_bar.track.is_valid());
    CHECK(theme.progress_bar.fill.is_valid());
    CHECK(theme.checkbox.unchecked == 112);
    CHECK(theme.checkbox.checked == 113);
    CHECK(theme.font_normal.is_valid());
    CHECK(theme.font_disabled.is_valid());
    CHECK(theme.animations.modal_open_ms == 150);
}

// ============================================================================
// Additional Widget Tiles Tests
// ============================================================================

TEST_CASE("tab_tiles - setup") {
    tab_tiles tiles{
        .active = {.left = 0, .center = 1, .right = 2, .margin = 4},
        .inactive = {.left = 3, .center = 4, .right = 5, .margin = 4},
        .panel = {16,17,18, 24,25,26, 32,33,34, 4, 4}
    };

    CHECK(tiles.active.is_valid());
    CHECK(tiles.inactive.is_valid());
    CHECK(tiles.panel.is_valid());
}

TEST_CASE("list_tiles - setup") {
    list_tiles tiles{
        .background = {0,1,2, 8,9,10, 16,17,18, 4, 4},
        .item_hover = {32,33,34, 40,41,42, 48,49,50, 2, 2},
        .item_selected = {64,65,66, 72,73,74, 80,81,82, 2, 2}
    };

    CHECK(tiles.background.is_valid());
    CHECK(tiles.item_hover.is_valid());
    CHECK(tiles.item_selected.is_valid());
}

TEST_CASE("menu_tiles - setup") {
    menu_tiles tiles{
        .bar = {0,1,2, 8,9,10, 16,17,18, 4, 4},
        .popup = {32,33,34, 40,41,42, 48,49,50, 4, 4},
        .item_hover = {64,65,66, 72,73,74, 80,81,82, 2, 2},
        .separator = 128,
        .submenu_arrow = 129,
        .checkmark = 130
    };

    CHECK(tiles.bar.is_valid());
    CHECK(tiles.popup.is_valid());
    CHECK(tiles.separator == 128);
    CHECK(tiles.submenu_arrow == 129);
    CHECK(tiles.checkmark == 130);
}

TEST_CASE("tooltip_tiles - setup") {
    tooltip_tiles tiles{
        .background = {0,1,2, 8,9,10, 16,17,18, 2, 2}
    };

    CHECK(tiles.background.is_valid());
}

TEST_CASE("group_box_tiles - setup") {
    group_box_tiles tiles{
        .frame = {0,1,2, 8,9,10, 16,17,18, 4, 4}
    };

    CHECK(tiles.frame.is_valid());
}
