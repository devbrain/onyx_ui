/**
 * @file tile_game_ui_demo.hh
 * @brief Example game UI screens demonstrating tile-based widgets
 *
 * This file provides complete example game UI screens using the tile
 * widget system. These can be used as references or starting points
 * for building game UIs.
 *
 * ## Included Screens
 * - Main Menu: Title, play/options/quit buttons
 * - Options Menu: Graphics, audio, controls settings
 * - Character Creation: Name input, class selection, stats
 * - In-Game HUD: Health/mana bars, minimap placeholder, action bar
 * - Inventory Panel: Item grid with scrolling
 * - Dialog Box: NPC conversation with choices
 * - Loading Screen: Progress bar with status text
 *
 * ## Usage
 * @code
 * #include <onyxui/tile/tile_game_ui_demo.hh>
 *
 * // Create main menu
 * auto menu = game_ui::create_main_menu<MyBackend>(
 *     [](){ start_game(); },
 *     [](){ show_options(); },
 *     [](){ quit_game(); }
 * );
 *
 * // Add to your UI root and render
 * root->add_child(std::move(menu));
 * @endcode
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

#include <onyxui/tile/tile_factory.hh>
#include <onyxui/tile/tile_animation.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/containers/hbox.hh>

namespace onyxui::tile::game_ui {

// ============================================================================
// Main Menu Screen
// ============================================================================

/**
 * @brief Create a classic game main menu
 *
 * Layout:
 * ```
 * ┌──────────────────────┐
 * │      GAME TITLE      │
 * │                      │
 * │    [ New Game  ]     │
 * │    [ Continue  ]     │
 * │    [ Options   ]     │
 * │    [ Quit      ]     │
 * └──────────────────────┘
 * ```
 */
template<UIBackend Backend>
struct main_menu_result {
    std::unique_ptr<tile_panel<Backend>> panel;
    tile_label<Backend>* title_label = nullptr;
    tile_button<Backend>* new_game_button = nullptr;
    tile_button<Backend>* continue_button = nullptr;
    tile_button<Backend>* options_button = nullptr;
    tile_button<Backend>* quit_button = nullptr;
};

template<UIBackend Backend>
[[nodiscard]] main_menu_result<Backend> create_main_menu(
    const std::string& game_title = "EPIC QUEST",
    std::function<void()> on_new_game = nullptr,
    std::function<void()> on_continue = nullptr,
    std::function<void()> on_options = nullptr,
    std::function<void()> on_quit = nullptr,
    bool has_save = false) {

    main_menu_result<Backend> result;
    result.panel = std::make_unique<tile_panel<Backend>>(nullptr, 16);

    // Title
    auto title = make_label_centered<Backend>(game_title);
    result.title_label = title.get();
    result.panel->add_child(std::move(title));

    // Spacer
    result.panel->add_child(make_label<Backend>(""));

    // Menu buttons
    auto new_game = make_button<Backend>("New Game", std::move(on_new_game));
    result.new_game_button = new_game.get();
    result.panel->add_child(std::move(new_game));

    auto continue_btn = make_button<Backend>("Continue", std::move(on_continue));
    result.continue_button = continue_btn.get();
    if (!has_save) {
        continue_btn->set_enabled(false);
    }
    result.panel->add_child(std::move(continue_btn));

    auto options = make_button<Backend>("Options", std::move(on_options));
    result.options_button = options.get();
    result.panel->add_child(std::move(options));

    auto quit = make_button<Backend>("Quit", std::move(on_quit));
    result.quit_button = quit.get();
    result.panel->add_child(std::move(quit));

    return result;
}

// ============================================================================
// Options/Settings Menu
// ============================================================================

/**
 * @brief Game settings structure
 */
struct game_settings {
    // Graphics
    int resolution_index = 2;  // 0=800x600, 1=1024x768, 2=1920x1080
    bool fullscreen = false;
    bool vsync = true;
    int brightness = 50;

    // Audio
    int master_volume = 80;
    int music_volume = 70;
    int sfx_volume = 90;
    bool mute_all = false;

    // Gameplay
    int difficulty_index = 1;  // 0=Easy, 1=Normal, 2=Hard
    bool show_hints = true;
    bool auto_save = true;
};

template<UIBackend Backend>
struct options_menu_result {
    std::unique_ptr<tile_panel<Backend>> panel;
    tile_label<Backend>* title_label = nullptr;

    // Graphics controls
    tile_combo_box<Backend>* resolution_combo = nullptr;
    tile_checkbox<Backend>* fullscreen_checkbox = nullptr;
    tile_checkbox<Backend>* vsync_checkbox = nullptr;
    tile_slider<Backend>* brightness_slider = nullptr;

    // Audio controls
    tile_slider<Backend>* master_volume_slider = nullptr;
    tile_slider<Backend>* music_volume_slider = nullptr;
    tile_slider<Backend>* sfx_volume_slider = nullptr;
    tile_checkbox<Backend>* mute_checkbox = nullptr;

    // Gameplay controls
    tile_combo_box<Backend>* difficulty_combo = nullptr;
    tile_checkbox<Backend>* hints_checkbox = nullptr;
    tile_checkbox<Backend>* autosave_checkbox = nullptr;

    tile_button<Backend>* apply_button = nullptr;
    tile_button<Backend>* back_button = nullptr;
};

template<UIBackend Backend>
[[nodiscard]] options_menu_result<Backend> create_options_menu(
    game_settings& settings,
    std::function<void()> on_apply = nullptr,
    std::function<void()> on_back = nullptr) {

    options_menu_result<Backend> result;
    result.panel = std::make_unique<tile_panel<Backend>>(nullptr, 8);

    // Title
    auto title = make_label_centered<Backend>("OPTIONS");
    result.title_label = title.get();
    result.panel->add_child(std::move(title));

    // === Graphics Section ===
    result.panel->add_child(make_label<Backend>("--- Graphics ---"));

    // Resolution
    auto res_row = std::make_unique<hbox<Backend>>(spacing::small);
    res_row->add_child(make_label<Backend>("Resolution:"));
    auto resolution = make_combo_box<Backend>(
        {"800x600", "1024x768", "1920x1080", "2560x1440"},
        settings.resolution_index,
        [&settings](int idx) { settings.resolution_index = idx; });
    result.resolution_combo = resolution.get();
    res_row->add_child(std::move(resolution));
    result.panel->add_child(std::move(res_row));

    // Fullscreen
    auto fullscreen = make_checkbox<Backend>("Fullscreen", settings.fullscreen,
        [&settings](bool val) { settings.fullscreen = val; });
    result.fullscreen_checkbox = fullscreen.get();
    result.panel->add_child(std::move(fullscreen));

    // VSync
    auto vsync = make_checkbox<Backend>("V-Sync", settings.vsync,
        [&settings](bool val) { settings.vsync = val; });
    result.vsync_checkbox = vsync.get();
    result.panel->add_child(std::move(vsync));

    // Brightness
    auto bright_row = std::make_unique<hbox<Backend>>(spacing::small);
    bright_row->add_child(make_label<Backend>("Brightness:"));
    auto brightness = make_slider<Backend>(settings.brightness, 0, 100,
        [&settings](int val) { settings.brightness = val; });
    result.brightness_slider = brightness.get();
    bright_row->add_child(std::move(brightness));
    result.panel->add_child(std::move(bright_row));

    // === Audio Section ===
    result.panel->add_child(make_label<Backend>("--- Audio ---"));

    // Master Volume
    auto master_row = std::make_unique<hbox<Backend>>(spacing::small);
    master_row->add_child(make_label<Backend>("Master:"));
    auto master = make_slider<Backend>(settings.master_volume, 0, 100,
        [&settings](int val) { settings.master_volume = val; });
    result.master_volume_slider = master.get();
    master_row->add_child(std::move(master));
    result.panel->add_child(std::move(master_row));

    // Music Volume
    auto music_row = std::make_unique<hbox<Backend>>(spacing::small);
    music_row->add_child(make_label<Backend>("Music:"));
    auto music = make_slider<Backend>(settings.music_volume, 0, 100,
        [&settings](int val) { settings.music_volume = val; });
    result.music_volume_slider = music.get();
    music_row->add_child(std::move(music));
    result.panel->add_child(std::move(music_row));

    // SFX Volume
    auto sfx_row = std::make_unique<hbox<Backend>>(spacing::small);
    sfx_row->add_child(make_label<Backend>("SFX:"));
    auto sfx = make_slider<Backend>(settings.sfx_volume, 0, 100,
        [&settings](int val) { settings.sfx_volume = val; });
    result.sfx_volume_slider = sfx.get();
    sfx_row->add_child(std::move(sfx));
    result.panel->add_child(std::move(sfx_row));

    // Mute
    auto mute = make_checkbox<Backend>("Mute All", settings.mute_all,
        [&settings](bool val) { settings.mute_all = val; });
    result.mute_checkbox = mute.get();
    result.panel->add_child(std::move(mute));

    // === Gameplay Section ===
    result.panel->add_child(make_label<Backend>("--- Gameplay ---"));

    // Difficulty
    auto diff_row = std::make_unique<hbox<Backend>>(spacing::small);
    diff_row->add_child(make_label<Backend>("Difficulty:"));
    auto difficulty = make_combo_box<Backend>(
        {"Easy", "Normal", "Hard", "Nightmare"},
        settings.difficulty_index,
        [&settings](int idx) { settings.difficulty_index = idx; });
    result.difficulty_combo = difficulty.get();
    diff_row->add_child(std::move(difficulty));
    result.panel->add_child(std::move(diff_row));

    // Hints
    auto hints = make_checkbox<Backend>("Show Hints", settings.show_hints,
        [&settings](bool val) { settings.show_hints = val; });
    result.hints_checkbox = hints.get();
    result.panel->add_child(std::move(hints));

    // Auto-save
    auto autosave = make_checkbox<Backend>("Auto-Save", settings.auto_save,
        [&settings](bool val) { settings.auto_save = val; });
    result.autosave_checkbox = autosave.get();
    result.panel->add_child(std::move(autosave));

    // Buttons
    auto buttons = std::make_unique<hbox<Backend>>(spacing::medium);
    auto apply = make_button<Backend>("Apply", std::move(on_apply));
    result.apply_button = apply.get();
    buttons->add_child(std::move(apply));
    auto back = make_button<Backend>("Back", std::move(on_back));
    result.back_button = back.get();
    buttons->add_child(std::move(back));
    result.panel->add_child(std::move(buttons));

    return result;
}

// ============================================================================
// Character Creation Screen
// ============================================================================

/**
 * @brief Character data structure
 */
struct character_data {
    std::string name = "Hero";
    int class_index = 0;  // 0=Warrior, 1=Mage, 2=Rogue, 3=Cleric
    int strength = 10;
    int intelligence = 10;
    int dexterity = 10;
    int constitution = 10;
    int points_remaining = 10;
};

template<UIBackend Backend>
struct character_creation_result {
    std::unique_ptr<tile_panel<Backend>> panel;
    tile_text_input<Backend>* name_input = nullptr;
    tile_combo_box<Backend>* class_combo = nullptr;
    tile_slider<Backend>* str_slider = nullptr;
    tile_slider<Backend>* int_slider = nullptr;
    tile_slider<Backend>* dex_slider = nullptr;
    tile_slider<Backend>* con_slider = nullptr;
    tile_label<Backend>* points_label = nullptr;
    tile_button<Backend>* create_button = nullptr;
    tile_button<Backend>* back_button = nullptr;
};

template<UIBackend Backend>
[[nodiscard]] character_creation_result<Backend> create_character_screen(
    character_data& character,
    std::function<void()> on_create = nullptr,
    std::function<void()> on_back = nullptr) {

    character_creation_result<Backend> result;
    result.panel = std::make_unique<tile_panel<Backend>>(nullptr, 8);

    // Title
    result.panel->add_child(make_label_centered<Backend>("CREATE CHARACTER"));

    // Name input
    auto name_row = std::make_unique<hbox<Backend>>(spacing::small);
    name_row->add_child(make_label<Backend>("Name:"));
    auto name_input = make_text_input<Backend>(character.name, "Enter name",
        [&character](const std::string& text) { character.name = text; });
    result.name_input = name_input.get();
    name_row->add_child(std::move(name_input));
    result.panel->add_child(std::move(name_row));

    // Class selection
    auto class_row = std::make_unique<hbox<Backend>>(spacing::small);
    class_row->add_child(make_label<Backend>("Class:"));
    auto class_combo = make_combo_box<Backend>(
        {"Warrior", "Mage", "Rogue", "Cleric"},
        character.class_index,
        [&character](int idx) { character.class_index = idx; });
    result.class_combo = class_combo.get();
    class_row->add_child(std::move(class_combo));
    result.panel->add_child(std::move(class_row));

    // Stats section
    result.panel->add_child(make_label<Backend>("--- Attributes ---"));

    // Points remaining label
    auto points_label = make_label<Backend>("Points: " + std::to_string(character.points_remaining));
    result.points_label = points_label.get();
    result.panel->add_child(std::move(points_label));

    // Strength
    auto str_row = std::make_unique<hbox<Backend>>(spacing::small);
    str_row->add_child(make_label<Backend>("STR:"));
    auto str_slider = make_slider<Backend>(character.strength, 1, 20,
        [&character](int val) { character.strength = val; });
    result.str_slider = str_slider.get();
    str_row->add_child(std::move(str_slider));
    result.panel->add_child(std::move(str_row));

    // Intelligence
    auto int_row = std::make_unique<hbox<Backend>>(spacing::small);
    int_row->add_child(make_label<Backend>("INT:"));
    auto int_slider = make_slider<Backend>(character.intelligence, 1, 20,
        [&character](int val) { character.intelligence = val; });
    result.int_slider = int_slider.get();
    int_row->add_child(std::move(int_slider));
    result.panel->add_child(std::move(int_row));

    // Dexterity
    auto dex_row = std::make_unique<hbox<Backend>>(spacing::small);
    dex_row->add_child(make_label<Backend>("DEX:"));
    auto dex_slider = make_slider<Backend>(character.dexterity, 1, 20,
        [&character](int val) { character.dexterity = val; });
    result.dex_slider = dex_slider.get();
    dex_row->add_child(std::move(dex_slider));
    result.panel->add_child(std::move(dex_row));

    // Constitution
    auto con_row = std::make_unique<hbox<Backend>>(spacing::small);
    con_row->add_child(make_label<Backend>("CON:"));
    auto con_slider = make_slider<Backend>(character.constitution, 1, 20,
        [&character](int val) { character.constitution = val; });
    result.con_slider = con_slider.get();
    con_row->add_child(std::move(con_slider));
    result.panel->add_child(std::move(con_row));

    // Buttons
    auto buttons = std::make_unique<hbox<Backend>>(spacing::medium);
    auto create = make_button<Backend>("Create", std::move(on_create));
    result.create_button = create.get();
    buttons->add_child(std::move(create));
    auto back = make_button<Backend>("Back", std::move(on_back));
    result.back_button = back.get();
    buttons->add_child(std::move(back));
    result.panel->add_child(std::move(buttons));

    return result;
}

// ============================================================================
// In-Game HUD
// ============================================================================

/**
 * @brief Player stats for HUD display
 */
struct player_stats {
    int health = 100;
    int max_health = 100;
    int mana = 50;
    int max_mana = 50;
    int experience = 0;
    int exp_to_level = 100;
    int level = 1;
    int gold = 0;
};

template<UIBackend Backend>
struct hud_result {
    std::unique_ptr<hbox<Backend>> top_bar;
    tile_progress_bar<Backend>* health_bar = nullptr;
    tile_progress_bar<Backend>* mana_bar = nullptr;
    tile_progress_bar<Backend>* exp_bar = nullptr;
    tile_label<Backend>* level_label = nullptr;
    tile_label<Backend>* gold_label = nullptr;
};

template<UIBackend Backend>
[[nodiscard]] hud_result<Backend> create_hud(player_stats& stats) {
    hud_result<Backend> result;
    result.top_bar = std::make_unique<hbox<Backend>>(spacing::medium);

    // Health bar section
    auto health_section = std::make_unique<vbox<Backend>>(spacing::tiny);
    health_section->add_child(make_label<Backend>("HP"));
    auto health_bar = make_progress_bar<Backend>(stats.health, 0, stats.max_health);
    result.health_bar = health_bar.get();
    health_section->add_child(std::move(health_bar));
    result.top_bar->add_child(std::move(health_section));

    // Mana bar section
    auto mana_section = std::make_unique<vbox<Backend>>(spacing::tiny);
    mana_section->add_child(make_label<Backend>("MP"));
    auto mana_bar = make_progress_bar<Backend>(stats.mana, 0, stats.max_mana);
    result.mana_bar = mana_bar.get();
    mana_section->add_child(std::move(mana_bar));
    result.top_bar->add_child(std::move(mana_section));

    // Level display
    auto level_label = make_label<Backend>("Lv." + std::to_string(stats.level));
    result.level_label = level_label.get();
    result.top_bar->add_child(std::move(level_label));

    // Experience bar
    auto exp_section = std::make_unique<vbox<Backend>>(spacing::tiny);
    exp_section->add_child(make_label<Backend>("EXP"));
    auto exp_bar = make_progress_bar<Backend>(stats.experience, 0, stats.exp_to_level);
    result.exp_bar = exp_bar.get();
    exp_section->add_child(std::move(exp_bar));
    result.top_bar->add_child(std::move(exp_section));

    // Gold display
    auto gold_label = make_label<Backend>("Gold: " + std::to_string(stats.gold));
    result.gold_label = gold_label.get();
    result.top_bar->add_child(std::move(gold_label));

    return result;
}

/**
 * @brief Update HUD displays with current stats
 */
template<UIBackend Backend>
void update_hud(hud_result<Backend>& hud, const player_stats& stats) {
    if (hud.health_bar) {
        hud.health_bar->set_range(0, stats.max_health);
        hud.health_bar->set_value(stats.health);
    }
    if (hud.mana_bar) {
        hud.mana_bar->set_range(0, stats.max_mana);
        hud.mana_bar->set_value(stats.mana);
    }
    if (hud.exp_bar) {
        hud.exp_bar->set_range(0, stats.exp_to_level);
        hud.exp_bar->set_value(stats.experience);
    }
    if (hud.level_label) {
        hud.level_label->set_text("Lv." + std::to_string(stats.level));
    }
    if (hud.gold_label) {
        hud.gold_label->set_text("Gold: " + std::to_string(stats.gold));
    }
}

// ============================================================================
// Action Bar (Hotbar)
// ============================================================================

template<UIBackend Backend>
struct action_bar_result {
    std::unique_ptr<hbox<Backend>> bar;
    std::vector<tile_button<Backend>*> slots;
};

template<UIBackend Backend>
[[nodiscard]] action_bar_result<Backend> create_action_bar(
    int num_slots = 8,
    std::function<void(int)> on_slot_click = nullptr) {

    action_bar_result<Backend> result;
    result.bar = std::make_unique<hbox<Backend>>(spacing::tiny);

    for (int i = 0; i < num_slots; ++i) {
        std::string label = std::to_string(i + 1);
        auto slot = make_button<Backend>(label, [on_slot_click, i]() {
            if (on_slot_click) {
                on_slot_click(i);
            }
        });
        result.slots.push_back(slot.get());
        result.bar->add_child(std::move(slot));
    }

    return result;
}

// ============================================================================
// Dialog/Conversation Box
// ============================================================================

/**
 * @brief Dialog choice structure
 */
struct dialog_choice {
    std::string text;
    std::function<void()> on_select;
    bool enabled = true;
};

template<UIBackend Backend>
struct dialog_box_result {
    std::unique_ptr<tile_panel<Backend>> panel;
    tile_label<Backend>* speaker_label = nullptr;
    tile_label<Backend>* text_label = nullptr;
    std::vector<tile_button<Backend>*> choice_buttons;
};

template<UIBackend Backend>
[[nodiscard]] dialog_box_result<Backend> create_dialog_box(
    const std::string& speaker,
    const std::string& text,
    const std::vector<dialog_choice>& choices) {

    dialog_box_result<Backend> result;
    result.panel = std::make_unique<tile_panel<Backend>>(nullptr, 8);

    // Speaker name
    auto speaker_label = make_label<Backend>(speaker + ":");
    result.speaker_label = speaker_label.get();
    result.panel->add_child(std::move(speaker_label));

    // Dialog text
    auto text_label = make_label<Backend>(text);
    result.text_label = text_label.get();
    result.panel->add_child(std::move(text_label));

    // Spacer
    result.panel->add_child(make_label<Backend>(""));

    // Choices
    for (const auto& choice : choices) {
        auto button = make_button<Backend>(choice.text, choice.on_select);
        if (!choice.enabled) {
            button->set_enabled(false);
        }
        result.choice_buttons.push_back(button.get());
        result.panel->add_child(std::move(button));
    }

    return result;
}

/**
 * @brief Update dialog content
 */
template<UIBackend Backend>
void update_dialog(
    dialog_box_result<Backend>& dialog,
    const std::string& speaker,
    const std::string& text) {
    if (dialog.speaker_label) {
        dialog.speaker_label->set_text(speaker + ":");
    }
    if (dialog.text_label) {
        dialog.text_label->set_text(text);
    }
}

// ============================================================================
// Loading Screen
// ============================================================================

template<UIBackend Backend>
struct loading_screen_result {
    std::unique_ptr<tile_panel<Backend>> panel;
    tile_label<Backend>* title_label = nullptr;
    tile_label<Backend>* status_label = nullptr;
    tile_progress_bar<Backend>* progress_bar = nullptr;
    tile_label<Backend>* tip_label = nullptr;
};

template<UIBackend Backend>
[[nodiscard]] loading_screen_result<Backend> create_loading_screen(
    const std::string& title = "Loading...",
    const std::string& initial_status = "Initializing...",
    const std::string& tip = "") {

    loading_screen_result<Backend> result;
    result.panel = std::make_unique<tile_panel<Backend>>(nullptr, 16);

    // Title
    auto title_label = make_label_centered<Backend>(title);
    result.title_label = title_label.get();
    result.panel->add_child(std::move(title_label));

    // Status text
    auto status_label = make_label_centered<Backend>(initial_status);
    result.status_label = status_label.get();
    result.panel->add_child(std::move(status_label));

    // Progress bar
    auto progress = make_progress_bar<Backend>(0, 0, 100);
    result.progress_bar = progress.get();
    result.panel->add_child(std::move(progress));

    // Tip label (if provided)
    if (!tip.empty()) {
        auto tip_label = make_label_centered<Backend>(tip);
        result.tip_label = tip_label.get();
        result.panel->add_child(std::move(tip_label));
    }

    return result;
}

/**
 * @brief Update loading screen progress
 */
template<UIBackend Backend>
void update_loading_screen(
    loading_screen_result<Backend>& screen,
    int progress,
    const std::string& status = "") {
    if (screen.progress_bar) {
        screen.progress_bar->set_value(progress);
    }
    if (screen.status_label && !status.empty()) {
        screen.status_label->set_text(status);
    }
}

// ============================================================================
// Pause Menu
// ============================================================================

template<UIBackend Backend>
struct pause_menu_result {
    std::unique_ptr<tile_panel<Backend>> panel;
    tile_label<Backend>* title_label = nullptr;
    tile_button<Backend>* resume_button = nullptr;
    tile_button<Backend>* options_button = nullptr;
    tile_button<Backend>* save_button = nullptr;
    tile_button<Backend>* load_button = nullptr;
    tile_button<Backend>* quit_button = nullptr;
};

template<UIBackend Backend>
[[nodiscard]] pause_menu_result<Backend> create_pause_menu(
    std::function<void()> on_resume = nullptr,
    std::function<void()> on_options = nullptr,
    std::function<void()> on_save = nullptr,
    std::function<void()> on_load = nullptr,
    std::function<void()> on_quit = nullptr) {

    pause_menu_result<Backend> result;
    result.panel = std::make_unique<tile_panel<Backend>>(nullptr, 12);

    // Title
    auto title = make_label_centered<Backend>("PAUSED");
    result.title_label = title.get();
    result.panel->add_child(std::move(title));

    // Buttons
    auto resume = make_button<Backend>("Resume", std::move(on_resume));
    result.resume_button = resume.get();
    result.panel->add_child(std::move(resume));

    auto options = make_button<Backend>("Options", std::move(on_options));
    result.options_button = options.get();
    result.panel->add_child(std::move(options));

    auto save = make_button<Backend>("Save Game", std::move(on_save));
    result.save_button = save.get();
    result.panel->add_child(std::move(save));

    auto load = make_button<Backend>("Load Game", std::move(on_load));
    result.load_button = load.get();
    result.panel->add_child(std::move(load));

    auto quit = make_button<Backend>("Quit to Menu", std::move(on_quit));
    result.quit_button = quit.get();
    result.panel->add_child(std::move(quit));

    return result;
}

// ============================================================================
// Shop Interface
// ============================================================================

/**
 * @brief Shop item structure
 */
struct shop_item {
    std::string name;
    int price;
    std::string description;
    bool can_afford = true;
};

template<UIBackend Backend>
struct shop_interface_result {
    std::unique_ptr<tile_panel<Backend>> panel;
    tile_label<Backend>* title_label = nullptr;
    tile_label<Backend>* gold_label = nullptr;
    std::vector<tile_button<Backend>*> item_buttons;
    tile_label<Backend>* description_label = nullptr;
    tile_button<Backend>* buy_button = nullptr;
    tile_button<Backend>* close_button = nullptr;
};

template<UIBackend Backend>
[[nodiscard]] shop_interface_result<Backend> create_shop_interface(
    const std::string& shop_name,
    int player_gold,
    const std::vector<shop_item>& items,
    std::function<void(int)> on_select_item = nullptr,
    std::function<void()> on_buy = nullptr,
    std::function<void()> on_close = nullptr) {

    shop_interface_result<Backend> result;
    result.panel = std::make_unique<tile_panel<Backend>>(nullptr, 8);

    // Title
    auto title = make_label_centered<Backend>(shop_name);
    result.title_label = title.get();
    result.panel->add_child(std::move(title));

    // Gold display
    auto gold = make_label<Backend>("Your Gold: " + std::to_string(player_gold));
    result.gold_label = gold.get();
    result.panel->add_child(std::move(gold));

    // Item list
    result.panel->add_child(make_label<Backend>("--- Items ---"));

    for (std::size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        std::string label = item.name + " - " + std::to_string(item.price) + "g";
        auto button = make_button<Backend>(label, [on_select_item, i]() {
            if (on_select_item) {
                on_select_item(static_cast<int>(i));
            }
        });
        if (!item.can_afford) {
            button->set_enabled(false);
        }
        result.item_buttons.push_back(button.get());
        result.panel->add_child(std::move(button));
    }

    // Description area
    auto desc = make_label<Backend>("Select an item");
    result.description_label = desc.get();
    result.panel->add_child(std::move(desc));

    // Buttons
    auto buttons = std::make_unique<hbox<Backend>>(spacing::medium);
    auto buy = make_button<Backend>("Buy", std::move(on_buy));
    result.buy_button = buy.get();
    buttons->add_child(std::move(buy));
    auto close = make_button<Backend>("Close", std::move(on_close));
    result.close_button = close.get();
    buttons->add_child(std::move(close));
    result.panel->add_child(std::move(buttons));

    return result;
}

// ============================================================================
// Confirmation Popup
// ============================================================================

template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<tile_panel<Backend>> create_confirm_popup(
    const std::string& title,
    const std::string& message,
    std::function<void()> on_confirm,
    std::function<void()> on_cancel,
    const std::string& confirm_text = "Yes",
    const std::string& cancel_text = "No") {

    auto panel = std::make_unique<tile_panel<Backend>>(nullptr, 8);

    panel->add_child(make_label_centered<Backend>(title));
    panel->add_child(make_label<Backend>(message));

    auto buttons = std::make_unique<hbox<Backend>>(spacing::medium);
    buttons->add_child(make_button<Backend>(confirm_text, std::move(on_confirm)));
    buttons->add_child(make_button<Backend>(cancel_text, std::move(on_cancel)));
    panel->add_child(std::move(buttons));

    return panel;
}

// ============================================================================
// Tooltip Helper
// ============================================================================

template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<tile_panel<Backend>> create_tooltip(
    const std::string& title,
    const std::string& description,
    const std::vector<std::string>& stats = {}) {

    auto panel = std::make_unique<tile_panel<Backend>>(nullptr, 4);

    // Title
    panel->add_child(make_label<Backend>(title));

    // Description
    if (!description.empty()) {
        panel->add_child(make_label<Backend>(description));
    }

    // Stats
    for (const auto& stat : stats) {
        panel->add_child(make_label<Backend>(stat));
    }

    return panel;
}

} // namespace onyxui::tile::game_ui
