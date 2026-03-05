/**
 * @file test_tile_game_ui_demo.cc
 * @brief Unit tests for tile game UI demo screens
 */

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include <onyxui/tile/tile_game_ui_demo.hh>
#include <onyxui/tile/tile_core.hh>

using namespace onyxui;
using namespace onyxui::tile;
using namespace onyxui::tile::game_ui;
using namespace onyxui::testing;

using Backend = test_canvas_backend;

// ============================================================================
// Main Menu Tests
// ============================================================================

TEST_SUITE("tile_game_ui_demo_main_menu") {
    TEST_CASE("create_main_menu creates all components") {
        auto menu = create_main_menu<Backend>();

        CHECK(menu.panel != nullptr);
        CHECK(menu.title_label != nullptr);
        CHECK(menu.new_game_button != nullptr);
        CHECK(menu.continue_button != nullptr);
        CHECK(menu.options_button != nullptr);
        CHECK(menu.quit_button != nullptr);
    }

    TEST_CASE("create_main_menu with custom title") {
        auto menu = create_main_menu<Backend>("MY GAME");

        CHECK(menu.title_label->text() == "MY GAME");
    }

    TEST_CASE("create_main_menu continue button disabled without save") {
        auto menu = create_main_menu<Backend>("Test", nullptr, nullptr, nullptr, nullptr, false);

        CHECK_FALSE(menu.continue_button->is_enabled());
    }

    TEST_CASE("create_main_menu continue button enabled with save") {
        auto menu = create_main_menu<Backend>("Test", nullptr, nullptr, nullptr, nullptr, true);

        CHECK(menu.continue_button->is_enabled());
    }

    TEST_CASE("create_main_menu button callbacks work") {
        bool new_game_clicked = false;
        bool options_clicked = false;
        bool quit_clicked = false;

        auto menu = create_main_menu<Backend>(
            "Test",
            [&new_game_clicked]() { new_game_clicked = true; },
            nullptr,
            [&options_clicked]() { options_clicked = true; },
            [&quit_clicked]() { quit_clicked = true; }
        );

        menu.new_game_button->clicked.emit();
        CHECK(new_game_clicked);

        menu.options_button->clicked.emit();
        CHECK(options_clicked);

        menu.quit_button->clicked.emit();
        CHECK(quit_clicked);
    }
}

// ============================================================================
// Options Menu Tests
// ============================================================================

TEST_SUITE("tile_game_ui_demo_options") {
    TEST_CASE("create_options_menu creates all components") {
        game_settings settings;
        auto options = create_options_menu<Backend>(settings);

        CHECK(options.panel != nullptr);
        CHECK(options.title_label != nullptr);

        // Graphics
        CHECK(options.resolution_combo != nullptr);
        CHECK(options.fullscreen_checkbox != nullptr);
        CHECK(options.vsync_checkbox != nullptr);
        CHECK(options.brightness_slider != nullptr);

        // Audio
        CHECK(options.master_volume_slider != nullptr);
        CHECK(options.music_volume_slider != nullptr);
        CHECK(options.sfx_volume_slider != nullptr);
        CHECK(options.mute_checkbox != nullptr);

        // Gameplay
        CHECK(options.difficulty_combo != nullptr);
        CHECK(options.hints_checkbox != nullptr);
        CHECK(options.autosave_checkbox != nullptr);

        // Buttons
        CHECK(options.apply_button != nullptr);
        CHECK(options.back_button != nullptr);
    }

    TEST_CASE("options menu reflects initial settings") {
        game_settings settings;
        settings.fullscreen = true;
        settings.vsync = false;
        settings.brightness = 75;
        settings.master_volume = 60;

        auto options = create_options_menu<Backend>(settings);

        CHECK(options.fullscreen_checkbox->is_checked());
        CHECK_FALSE(options.vsync_checkbox->is_checked());
        CHECK(options.brightness_slider->value() == 75);
        CHECK(options.master_volume_slider->value() == 60);
    }

    TEST_CASE("options menu updates settings on change") {
        game_settings settings;
        auto options = create_options_menu<Backend>(settings);

        options.fullscreen_checkbox->set_checked(true);
        CHECK(settings.fullscreen == true);

        options.brightness_slider->set_value(80);
        CHECK(settings.brightness == 80);
    }

    TEST_CASE("options menu button callbacks work") {
        game_settings settings;
        bool apply_clicked = false;
        bool back_clicked = false;

        auto options = create_options_menu<Backend>(
            settings,
            [&apply_clicked]() { apply_clicked = true; },
            [&back_clicked]() { back_clicked = true; }
        );

        options.apply_button->clicked.emit();
        CHECK(apply_clicked);

        options.back_button->clicked.emit();
        CHECK(back_clicked);
    }
}

// ============================================================================
// Character Creation Tests
// ============================================================================

TEST_SUITE("tile_game_ui_demo_character") {
    TEST_CASE("create_character_screen creates all components") {
        character_data character;
        auto screen = create_character_screen<Backend>(character);

        CHECK(screen.panel != nullptr);
        CHECK(screen.name_input != nullptr);
        CHECK(screen.class_combo != nullptr);
        CHECK(screen.str_slider != nullptr);
        CHECK(screen.int_slider != nullptr);
        CHECK(screen.dex_slider != nullptr);
        CHECK(screen.con_slider != nullptr);
        CHECK(screen.points_label != nullptr);
        CHECK(screen.create_button != nullptr);
        CHECK(screen.back_button != nullptr);
    }

    TEST_CASE("character screen reflects initial data") {
        character_data character;
        character.name = "TestHero";
        character.class_index = 2;  // Rogue
        character.strength = 15;

        auto screen = create_character_screen<Backend>(character);

        CHECK(screen.name_input->text() == "TestHero");
        CHECK(screen.class_combo->current_index() == 2);
        CHECK(screen.str_slider->value() == 15);
    }

    TEST_CASE("character screen updates data on change") {
        character_data character;
        auto screen = create_character_screen<Backend>(character);

        screen.class_combo->set_current_index(3);  // Cleric
        CHECK(character.class_index == 3);

        screen.str_slider->set_value(18);
        CHECK(character.strength == 18);
    }

    TEST_CASE("character screen button callbacks work") {
        character_data character;
        bool create_clicked = false;
        bool back_clicked = false;

        auto screen = create_character_screen<Backend>(
            character,
            [&create_clicked]() { create_clicked = true; },
            [&back_clicked]() { back_clicked = true; }
        );

        screen.create_button->clicked.emit();
        CHECK(create_clicked);

        screen.back_button->clicked.emit();
        CHECK(back_clicked);
    }
}

// ============================================================================
// HUD Tests
// ============================================================================

TEST_SUITE("tile_game_ui_demo_hud") {
    TEST_CASE("create_hud creates all components") {
        player_stats stats;
        auto hud = create_hud<Backend>(stats);

        CHECK(hud.top_bar != nullptr);
        CHECK(hud.health_bar != nullptr);
        CHECK(hud.mana_bar != nullptr);
        CHECK(hud.exp_bar != nullptr);
        CHECK(hud.level_label != nullptr);
        CHECK(hud.gold_label != nullptr);
    }

    TEST_CASE("hud reflects initial stats") {
        player_stats stats;
        stats.health = 80;
        stats.max_health = 100;
        stats.mana = 30;
        stats.max_mana = 50;
        stats.level = 5;
        stats.gold = 1000;

        auto hud = create_hud<Backend>(stats);

        CHECK(hud.health_bar->value() == 80);
        CHECK(hud.mana_bar->value() == 30);
        CHECK(hud.level_label->text() == "Lv.5");
        CHECK(hud.gold_label->text() == "Gold: 1000");
    }

    TEST_CASE("update_hud updates displays") {
        player_stats stats;
        auto hud = create_hud<Backend>(stats);

        stats.health = 50;
        stats.level = 10;
        stats.gold = 5000;
        update_hud(hud, stats);

        CHECK(hud.health_bar->value() == 50);
        CHECK(hud.level_label->text() == "Lv.10");
        CHECK(hud.gold_label->text() == "Gold: 5000");
    }
}

// ============================================================================
// Action Bar Tests
// ============================================================================

TEST_SUITE("tile_game_ui_demo_action_bar") {
    TEST_CASE("create_action_bar creates default slots") {
        auto action_bar = create_action_bar<Backend>();

        CHECK(action_bar.bar != nullptr);
        CHECK(action_bar.slots.size() == 8);
    }

    TEST_CASE("create_action_bar with custom slot count") {
        auto action_bar = create_action_bar<Backend>(12);

        CHECK(action_bar.slots.size() == 12);
    }

    TEST_CASE("action bar slot callbacks work") {
        int clicked_slot = -1;
        auto action_bar = create_action_bar<Backend>(4, [&clicked_slot](int slot) {
            clicked_slot = slot;
        });

        action_bar.slots[2]->clicked.emit();
        CHECK(clicked_slot == 2);
    }
}

// ============================================================================
// Dialog Box Tests
// ============================================================================

TEST_SUITE("tile_game_ui_demo_dialog") {
    TEST_CASE("create_dialog_box creates all components") {
        auto dialog = create_dialog_box<Backend>(
            "NPC",
            "Hello, adventurer!",
            {{"Accept", nullptr}, {"Decline", nullptr}}
        );

        CHECK(dialog.panel != nullptr);
        CHECK(dialog.speaker_label != nullptr);
        CHECK(dialog.text_label != nullptr);
        CHECK(dialog.choice_buttons.size() == 2);
    }

    TEST_CASE("dialog box shows speaker and text") {
        auto dialog = create_dialog_box<Backend>(
            "Merchant",
            "Would you like to buy something?",
            {}
        );

        CHECK(dialog.speaker_label->text() == "Merchant:");
        CHECK(dialog.text_label->text() == "Would you like to buy something?");
    }

    TEST_CASE("dialog box choice callbacks work") {
        bool accepted = false;
        auto dialog = create_dialog_box<Backend>(
            "NPC",
            "Quest?",
            {{"Accept", [&accepted]() { accepted = true; }}}
        );

        dialog.choice_buttons[0]->clicked.emit();
        CHECK(accepted);
    }

    TEST_CASE("dialog box disabled choices") {
        auto dialog = create_dialog_box<Backend>(
            "NPC",
            "Test",
            {
                {"Enabled", nullptr, true},
                {"Disabled", nullptr, false}
            }
        );

        CHECK(dialog.choice_buttons[0]->is_enabled());
        CHECK_FALSE(dialog.choice_buttons[1]->is_enabled());
    }

    TEST_CASE("update_dialog updates content") {
        auto dialog = create_dialog_box<Backend>(
            "NPC",
            "Initial",
            {}
        );

        update_dialog(dialog, "NewNPC", "Updated text");

        CHECK(dialog.speaker_label->text() == "NewNPC:");
        CHECK(dialog.text_label->text() == "Updated text");
    }
}

// ============================================================================
// Loading Screen Tests
// ============================================================================

TEST_SUITE("tile_game_ui_demo_loading") {
    TEST_CASE("create_loading_screen creates all components") {
        auto loading = create_loading_screen<Backend>();

        CHECK(loading.panel != nullptr);
        CHECK(loading.title_label != nullptr);
        CHECK(loading.status_label != nullptr);
        CHECK(loading.progress_bar != nullptr);
    }

    TEST_CASE("loading screen with custom values") {
        auto loading = create_loading_screen<Backend>(
            "Loading Level",
            "Generating terrain...",
            "Tip: Press Space to jump"
        );

        CHECK(loading.title_label->text() == "Loading Level");
        CHECK(loading.status_label->text() == "Generating terrain...");
        CHECK(loading.tip_label != nullptr);
        CHECK(loading.tip_label->text() == "Tip: Press Space to jump");
    }

    TEST_CASE("update_loading_screen updates progress") {
        auto loading = create_loading_screen<Backend>();

        update_loading_screen(loading, 50, "Loading assets...");

        CHECK(loading.progress_bar->value() == 50);
        CHECK(loading.status_label->text() == "Loading assets...");
    }
}

// ============================================================================
// Pause Menu Tests
// ============================================================================

TEST_SUITE("tile_game_ui_demo_pause") {
    TEST_CASE("create_pause_menu creates all components") {
        auto pause = create_pause_menu<Backend>();

        CHECK(pause.panel != nullptr);
        CHECK(pause.title_label != nullptr);
        CHECK(pause.resume_button != nullptr);
        CHECK(pause.options_button != nullptr);
        CHECK(pause.save_button != nullptr);
        CHECK(pause.load_button != nullptr);
        CHECK(pause.quit_button != nullptr);
    }

    TEST_CASE("pause menu button callbacks work") {
        bool resumed = false;
        bool saved = false;

        auto pause = create_pause_menu<Backend>(
            [&resumed]() { resumed = true; },
            nullptr,
            [&saved]() { saved = true; }
        );

        pause.resume_button->clicked.emit();
        CHECK(resumed);

        pause.save_button->clicked.emit();
        CHECK(saved);
    }
}

// ============================================================================
// Shop Interface Tests
// ============================================================================

TEST_SUITE("tile_game_ui_demo_shop") {
    TEST_CASE("create_shop_interface creates all components") {
        std::vector<shop_item> items = {
            {"Sword", 100, "A basic sword"},
            {"Shield", 150, "A basic shield"}
        };

        auto shop = create_shop_interface<Backend>("Blacksmith", 500, items);

        CHECK(shop.panel != nullptr);
        CHECK(shop.title_label != nullptr);
        CHECK(shop.gold_label != nullptr);
        CHECK(shop.item_buttons.size() == 2);
        CHECK(shop.description_label != nullptr);
        CHECK(shop.buy_button != nullptr);
        CHECK(shop.close_button != nullptr);
    }

    TEST_CASE("shop displays gold correctly") {
        auto shop = create_shop_interface<Backend>("Shop", 999, {});

        CHECK(shop.gold_label->text() == "Your Gold: 999");
    }

    TEST_CASE("shop disabled items for unaffordable") {
        std::vector<shop_item> items = {
            {"Cheap", 10, "", true},
            {"Expensive", 1000, "", false}
        };

        auto shop = create_shop_interface<Backend>("Shop", 100, items);

        CHECK(shop.item_buttons[0]->is_enabled());
        CHECK_FALSE(shop.item_buttons[1]->is_enabled());
    }

    TEST_CASE("shop item selection callback") {
        std::vector<shop_item> items = {
            {"Item1", 10, ""},
            {"Item2", 20, ""}
        };

        int selected = -1;
        auto shop = create_shop_interface<Backend>(
            "Shop", 100, items,
            [&selected](int idx) { selected = idx; }
        );

        shop.item_buttons[1]->clicked.emit();
        CHECK(selected == 1);
    }
}

// ============================================================================
// Confirm Popup Tests
// ============================================================================

TEST_SUITE("tile_game_ui_demo_confirm") {
    TEST_CASE("create_confirm_popup creates panel") {
        bool confirmed = false;
        bool cancelled = false;

        auto popup = create_confirm_popup<Backend>(
            "Confirm",
            "Are you sure?",
            [&confirmed]() { confirmed = true; },
            [&cancelled]() { cancelled = true; }
        );

        CHECK(popup != nullptr);
    }

    TEST_CASE("confirm popup with custom button text") {
        auto popup = create_confirm_popup<Backend>(
            "Delete",
            "Delete this file?",
            nullptr,
            nullptr,
            "Delete",
            "Keep"
        );

        CHECK(popup != nullptr);
    }
}

// ============================================================================
// Tooltip Tests
// ============================================================================

TEST_SUITE("tile_game_ui_demo_tooltip") {
    TEST_CASE("create_tooltip basic") {
        auto tooltip = create_tooltip<Backend>("Sword of Power", "A legendary weapon");

        CHECK(tooltip != nullptr);
    }

    TEST_CASE("create_tooltip with stats") {
        auto tooltip = create_tooltip<Backend>(
            "Health Potion",
            "Restores health",
            {"+50 HP", "Cooldown: 30s"}
        );

        CHECK(tooltip != nullptr);
    }

    TEST_CASE("create_tooltip empty description") {
        auto tooltip = create_tooltip<Backend>("Gold Coin", "");

        CHECK(tooltip != nullptr);
    }
}
