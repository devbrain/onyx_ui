# Comprehensive Widgets Demo Design

**Purpose**: Create an advanced demo application that showcases every aspect of OnyxUI framework for debugging, testing, and demonstration purposes.

**Last Updated**: 2025-11-23

---

## Design Goals

### Primary Goals

1. **Complete Widget Coverage** - Every widget, every state, every configuration
2. **Framework Feature Showcase** - Layout, theming, scrolling, MVC, events, focus, hotkeys
3. **Interactive Testing** - Real-time interaction with all features
4. **Debugging Tool** - Visual feedback for identifying layout/rendering issues
5. **Documentation** - Living examples of framework capabilities

### Secondary Goals

1. **Performance Testing** - Stress test with many widgets, large datasets
2. **Visual Regression Testing** - Screenshot baseline for visual testing
3. **Educational** - Clear examples for learning the framework
4. **Extensible** - Easy to add new widget demonstrations

---

## Architecture

### Application Structure

```
┌─────────────────────────────────────────────────────────────────────┐
│ Menu Bar: File | Widgets | Windows | Layers | Layout | Theme | ... │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌───────────────────────────────────────────────────────────────┐ │
│  │ Tab Widget (Main Content Area)                                │ │
│  │ ┌─────────────────────────────────────────────────────────────┤ │
│  │ │ Basic | Containers | Input | MVC | Layout | Scrolling | ...│ │
│  │ ├─────────────────────────────────────────────────────────────┤ │
│  │ │                                                             │ │
│  │ │   [Current Tab Content - Scrollable]                       │ │
│  │ │                                                             │ │
│  │ │                                                             │ │
│  │ └─────────────────────────────────────────────────────────────┘ │
│  └───────────────────────────────────────────────────────────────┘ │
│                                                                     │
├─────────────────────────────────────────────────────────────────────┤
│ Status: Theme: Norton Blue | FPS: 60 | Focus: btn_ok | Widgets: 247│
└─────────────────────────────────────────────────────────────────────┘

Optional debug overlay (Ctrl+D to toggle):
┌─────────────────────────────────────────────────────────────────────┐
│ Debug: Bounds: (10,5,80,25) | Measure: 45ms | Arrange: 12ms | ...  │
└─────────────────────────────────────────────────────────────────────┘
```

### Main Window Tabs (3 Tabs)

#### Tab 1: All Widgets
- **Purpose**: Widget gallery - showcase all available widgets
- **Sections** (scrollable with group_box containers):
  - **Basic Widgets**: button (all states), label (alignments), spacer, spring
  - **Containers**: panel, vbox/hbox, grid, anchor_panel, group_box, tab_widget
  - **Input Widgets**: line_edit, checkbox, radio_button, slider, progress_bar, combo_box
  - **Other**: status_bar, menu examples
- **Organization**: Each widget type in its own group_box with state demonstrations

#### Tab 2: Layout & Scrolling
- **Purpose**: Core layout and scrolling systems
- **Layout Section**:
  - Two-pass measure/arrange visualization
  - Size policies (fixed, stretch, percentage)
  - Alignment showcase (left, center, right, top, bottom, stretch)
  - Padding and margin examples
  - Nested layouts
- **Scrolling Section**:
  - `scroll_view` presets (modern, classic, compact, vertical-only)
  - Large content scrolling (1000+ items)
  - Keyboard navigation (arrows, Page Up/Down, Home/End)
  - Scrollbar visibility modes (always, auto-hide, never)

#### Tab 3: Events & Interaction
- **Purpose**: Event system, focus, and keyboard interaction
- **Event System Section**:
  - Three-phase routing visualization (capture, target, bubble)
  - Mouse events (click, double-click, move, wheel)
  - Keyboard events (key press, mnemonics)
  - Event log viewer (real-time stream, last 20 events)
- **Focus Management Section**:
  - Tab order visualization
  - Focus indicators (which widget has focus)
  - Programmatic focus control buttons
  - Focus chain inspection
- **Hotkeys Section**:
  - Current hotkey scheme display
  - Hotkey scheme selector (Windows, Norton Commander)
  - Mnemonic navigation demo (&File, &Edit)

---

### Additional Windows (Spawned from Menus)

These are separate window instances opened via menu items, allowing them to stay open while browsing the main tabs.

#### MVC Demo Window (Windows → MVC Demo... / Ctrl+M)
- **Purpose**: Model-View-Controller pattern in a real window
- **Base Class**: `main_window<Backend>` (with menu bar and status bar)
- **Features**:
  - `list_view` with various selection modes
  - `combo_box` with model binding
  - Add/Remove items buttons (dynamic model updates)
  - Selection display (shows selected items)
  - Custom delegate example (striped rows)
  - Two synchronized views on same model
- **Benefit**: Real window demonstrates window management + MVC together

#### Theme Editor Window (Windows → Theme Editor... / Ctrl+T)
- **Purpose**: Live theme editing and preview
- **Base Class**: `main_window<Backend>` (with menu bar and status bar)
- **Features**:
  - Theme selector (combo_box with all registered themes)
  - Color palette editor (edit colors, see live preview)
  - CSS-style inheritance visualization
  - Widget state preview panel (normal/hover/pressed/disabled)
  - Theme properties inspector (view all theme values)
  - Save/Export theme button
- **Benefit**: Can keep theme editor open while testing theme on main window

#### Debug Tools Window (Windows → Debug Tools... / F12)
- **Purpose**: Framework debugging and performance profiling
- **Base Class**: `main_window<Backend>` (with menu bar and status bar)
- **Widget Inspector Section**:
  - Widget tree view (hierarchical, all windows)
  - Refresh button
  - Select widget in tree → highlight in UI
- **Visualization Section**:
  - Show widget bounds (checkbox)
  - Show padding (checkbox)
  - Show focus chain (checkbox)
  - Layer stack inspector
- **Performance Section**:
  - FPS counter (live updates)
  - Measure/Arrange/Render times
  - Widget count (all windows)
  - Memory usage
  - Layout cache hit ratio
- **Event Log Section**:
  - Last 50 events (more space in dedicated window)
  - Filter by event type
  - Clear button
  - Export trace
- **Benefit**: Can keep debug tools open while interacting with main demo

---

## Debug Features

### Visual Debugging

1. **Bounds Highlighting**
   - Toggle to show widget boundaries
   - Different colors for different widget types
   - Padding/margin visualization

2. **Focus Visualization**
   - Highlight focused widget
   - Show focus chain
   - Tab order indicators

3. **Layout Debug Mode**
   - Show measure/arrange passes
   - Display size constraints
   - Highlight layout strategy

### Performance Metrics

1. **Frame Stats**
   - FPS counter
   - Frame time (ms)
   - Measure time
   - Arrange time
   - Render time

2. **Widget Stats**
   - Total widget count
   - Visible widget count
   - Layout cache hit/miss ratio
   - Event handler count

3. **Memory Stats**
   - Heap usage
   - Widget allocation count
   - Signal connection count

### Event Logging

1. **Event Log Viewer**
   - Real-time event stream
   - Event type filtering
   - Event phase display (capture/target/bubble)
   - Event source widget
   - Timestamp

2. **Event Playback**
   - Record event sequence
   - Replay events for testing
   - Save/load event traces

---

## Menu Structure

### File Menu
- Open Theme... (Ctrl+O)
- Save Screenshot (Ctrl+S / F9)
- ---
- Exit (Alt+F4 / Ctrl+Q)

### Windows Menu
- MVC Demo... (Ctrl+M)
- Theme Editor... (Ctrl+T)
- Debug Tools... (F12)
- ---
- Modal Dialog Example...
- Modeless Dialog Example...
- ---
- Window List... (Ctrl+W)
- Next Window (Ctrl+F6)
- Previous Window (Ctrl+Shift+F6)
- ---
- Bring All to Front
- Close All Windows

### Layers Menu
- Show Tooltip Example
- Show Popup Menu Example
- Show Dropdown Example
- ---
- Show Modal Layer
- Show Popup Layer
- Show Tooltip Layer
- ---
- Layer Stack Inspector

### Theme Menu
- Norton Blue
- Windows 3.x
- Midnight Commander
- ---
- Reload Themes (F5)

### Debug Menu
- Show Bounds (Ctrl+B)
- Show Padding (Ctrl+P)
- Show Focus Chain (Ctrl+F)
- ---
- Dump Widget Tree

### Help Menu
- About OnyxUI
- Keyboard Shortcuts (F1)
- Framework Documentation
- ---
- Report Bug

---

## Status Bar Layout

The status bar appears at the **bottom** of the window (traditional placement):

```
┌──────────────────────────────────────────────────────────────────┐
│ Theme: Norton Blue │ FPS: 60 │ Focus: btn_ok │ Widgets: 247    │
└──────────────────────────────────────────────────────────────────┘
```

**Sections** (left to right):
1. **Current theme name** - Shows active theme, updates on theme switch
2. **FPS counter** - Real-time frame rate (updated every second)
3. **Currently focused widget** - Widget ID or class name with focus
4. **Total widget count** - Number of widgets in current window tree

**Live Updates**:
- Theme name changes when switching themes (Theme menu)
- FPS updates continuously during rendering
- Focus widget updates on Tab, mouse click, programmatic focus changes
- Widget count updates when tabs switch or widgets added/removed

---

## Implementation Details

### File Structure

```
examples/
  widgets_demo/
    main.cc                      # Entry point
    widgets_demo.hh              # Main widget class

    tabs/
      tab_all_widgets.hh         # Tab 1: All widgets gallery
      tab_layout_scrolling.hh    # Tab 2: Layout & scrolling systems
      tab_events_interaction.hh  # Tab 3: Events, focus, hotkeys

    windows/
      mvc_demo_window.hh         # MVC demo window (Windows → MVC Demo)
      theme_editor_window.hh     # Theme editor window (Theme → Theme Editor...)
      debug_tools_window.hh      # Debug tools window (Debug → Debug Tools... / F12)
      modal_dialog_example.hh    # Modal dialog example
      modeless_dialog_example.hh # Modeless dialog example

    layers/
      tooltip_example.hh         # Tooltip layer demonstration
      popup_menu_example.hh      # Popup menu demonstration
      dropdown_example.hh        # Dropdown demonstration
      layer_visualizer.hh        # Layer stack visualization

    utils/
      debug_panel.hh             # Debug visualization utilities
      event_logger.hh            # Event logging system
      performance_monitor.hh     # Performance tracking
      widget_inspector.hh        # Widget tree inspection
      bounds_visualizer.hh       # Visual bounds highlighting

    CMakeLists.txt               # Build configuration
```

### Widget Tree Structure

**Note**: All window classes inherit from `main_window<Backend>` which provides:
- Automatic menu bar management (top)
- Central content widget (middle, expands)
- Status bar management (bottom)
- Window title and lifecycle

```cpp
widgets_demo_app : public main_window<Backend>  // Main application window
├── menu_bar (top)
│   ├── menu (File)
│   ├── menu (Widgets)
│   ├── menu (Windows)        // Spawns demo windows
│   ├── menu (Layers)         // Shows popups/tooltips/modals
│   ├── menu (Layout)
│   ├── menu (Theme)
│   ├── menu (Debug)
│   └── menu (Help)
│
├── tab_widget (central content area - 3 tabs)
│   ├── panel (Tab 1: All Widgets)
│   │   └── scroll_view
│   │       └── vbox
│   │           ├── group_box (Basic Widgets)
│   │           │   ├── button (normal)
│   │           │   ├── button (disabled)
│   │           │   ├── label (left/center/right)
│   │           │   ├── spacer
│   │           │   └── spring
│   │           ├── group_box (Containers)
│   │           │   ├── panel examples
│   │           │   ├── vbox/hbox examples
│   │           │   ├── grid example
│   │           │   └── group_box nested
│   │           └── group_box (Input Widgets)
│   │               ├── line_edit
│   │               ├── checkbox
│   │               ├── radio_button group
│   │               ├── slider
│   │               ├── progress_bar
│   │               └── combo_box
│   ├── panel (Tab 2: Layout & Scrolling)
│   │   └── scroll_view
│   │       └── vbox
│   │           ├── group_box (Layout System)
│   │           └── group_box (Scrolling System)
│   └── panel (Tab 3: Events & Interaction)
│       └── scroll_view
│           └── vbox
│               ├── group_box (Event System)
│               ├── group_box (Focus Management)
│               └── group_box (Hotkeys)
│
└── status_bar (bottom)
    ├── label (theme name)
    ├── label (FPS counter)
    ├── label (focused widget)
    └── label (widget count)

// Optional debug overlay (toggle with Ctrl+D)
debug_panel (floating overlay, top of screen)
├── bounds_visualizer
├── event_logger
└── performance_monitor
```

---

## Windows and Layer Management via Menus

### Windows Menu (Interactive Window Features)

Instead of a tab, window management features are accessed through menu items:

**New Demo Window** - Spawns additional main_window instances to demonstrate:
- Multiple windows coexisting
- Window z-order (clicking brings to front)
- Window cycling (Ctrl+F6 / Ctrl+Shift+F6)
- Independent window state

**Modal Dialog Example** - Shows modal dialog:
- Blocks interaction with parent window
- Focus trap (Tab only cycles within modal)
- ESC to close
- Dark overlay on parent window
- Previous window restored on close

**Modeless Dialog Example** - Shows modeless dialog:
- Allows interaction with both windows
- Independent focus management
- Both windows remain interactive

**Window List Dialog** - Shows all open windows:
- List of window titles
- Current window highlighted
- Click or Enter to activate
- Demonstrates window_manager integration

### Layers Menu (Interactive Layer Features)

Layer management demonstrated through menu items that trigger popups:

**Show Tooltip Example** - Demonstrates tooltip layer:
- Tooltip appears near cursor
- Auto-hide after delay
- Proper z-order (above all widgets)
- Click-through behavior

**Show Popup Menu Example** - Context menu demonstration:
- Right-click or menu item triggers popup
- Click outside to close
- Proper layer ordering
- Menu item selection

**Show Dropdown Example** - Dropdown layer:
- combo_box popup rendering (when implemented)
- Click outside to close
- Scrollable dropdown list
- Selection feedback

**Layer Stack Inspector** - Visual debugging:
- Shows all active layers
- Z-order visualization
- Layer bounds highlighting
- Layer lifecycle tracking

### Benefits of Menu-Based Approach

1. **Natural UX** - Windows/layers are spawned, not displayed in tabs
2. **Real Interaction** - Actual window/layer behavior, not documentation
3. **Focus Testing** - Modal focus, layer focus, window focus all testable
4. **Z-Order** - Proper layer stacking, bring-to-front behavior
5. **Event Routing** - Click-outside-to-close, event bubbling through layers
6. **Lifecycle** - Window creation, activation, deactivation, closing

---

## Example Implementations

### Tab 1: All Widgets

```cpp
template<UIBackend Backend>
std::unique_ptr<panel<Backend>> create_all_widgets_tab() {
    auto tab = std::make_unique<panel<Backend>>();

    // Make tab content scrollable
    auto scroll = tab->template emplace_child<scroll_view>();
    auto* content = scroll->template emplace_child<vbox>(2);  // 2px spacing

    // Basic Widgets Section
    auto* basic_section = content->template emplace_child<group_box>();
    basic_section->set_title("Basic Widgets");
    basic_section->set_vbox_layout(1);

    auto* btn_row = basic_section->template emplace_child<hbox>(2);
    btn_row->emplace_child<label>("Button:")->set_width(15);
    btn_row->emplace_child<button>("Click Me");

    auto* btn_disabled = basic_section->template emplace_child<hbox>(2);
    btn_disabled->emplace_child<label>("Disabled:")->set_width(15);
    auto* disabled_btn = btn_disabled->template emplace_child<button>("Disabled");
    disabled_btn->set_enabled(false);

    basic_section->emplace_child<label>("Left Aligned")->set_horizontal_alignment(alignment::left);
    basic_section->emplace_child<label>("Center Aligned")->set_horizontal_alignment(alignment::center);

    // Containers Section
    auto* container_section = content->template emplace_child<group_box>();
    container_section->set_title("Container Widgets");
    container_section->set_vbox_layout(1);

    container_section->emplace_child<label>("VBox/HBox examples:");
    auto* hbox_demo = container_section->template emplace_child<hbox>(2);
    hbox_demo->emplace_child<button>("Item 1");
    hbox_demo->emplace_child<button>("Item 2");
    hbox_demo->emplace_child<button>("Item 3");

    // Input Widgets Section (continuation below)
    auto* input_section = content->template emplace_child<group_box>();
    input_section->set_title("Input Widgets");
    input_section->set_vbox_layout(1);

    auto* edit = input_section->template emplace_child<line_edit>();
    edit->set_text("Type here...");
    edit->set_width(40);

    auto* cb = input_section->template emplace_child<checkbox>("Enable feature");
    cb->set_checked(true);

    auto* slider = input_section->template emplace_child<slider>(orientation::horizontal);
    slider->set_range(0, 100);
    slider->set_value(50);
    slider->set_width(40);

    // Actions Section
    auto* actions_section = content->template emplace_child<group_box>();
    actions_section->set_title("Actions");
    actions_section->set_hbox_layout(2);

    auto* screenshot_btn = actions_section->template emplace_child<button>("Take Screenshot (F9)");
    screenshot_btn->clicked.connect([&]() {
        // Take screenshot using renderer
        take_screenshot();
    });

    auto* theme_btn = actions_section->template emplace_child<button>("Theme Editor (Ctrl+T)");
    theme_btn->clicked.connect([&]() {
        // Open theme editor window
        open_theme_editor();
    });

    return tab;
}
```

### MVC Demo Window

```cpp
template<UIBackend Backend>
class mvc_demo_window : public main_window<Backend> {
public:
    mvc_demo_window() : main_window<Backend>() {
        this->set_title("MVC System Demo");

        // Get central widget from main_window
        auto* content = this->central_widget();
        content->set_vbox_layout(2);

        // Title
        content->emplace_child<label>("Model-View-Controller Demonstration");

        // List View Section
        auto* list_section = content->template emplace_child<group_box>();
        list_section->set_title("List View with Model");
        list_section->set_vbox_layout(1);

        m_model = std::make_shared<list_model<std::string, Backend>>();
        m_model->set_items({"Apple", "Banana", "Cherry", "Date", "Elderberry"});

        m_list_view = list_section->template emplace_child<list_view>();
        m_list_view->set_model(m_model.get());
        m_list_view->set_height(10);

        // Controls Section
        auto* controls = content->template emplace_child<group_box>();
        controls->set_title("Model Controls");
        controls->set_hbox_layout(2);

        auto* add_btn = controls->template emplace_child<button>("Add Item");
        add_btn->clicked.connect([this]() {
            m_model->append("New Item " + std::to_string(m_item_counter++));
        });

        auto* remove_btn = controls->template emplace_child<button>("Remove Selected");
        remove_btn->clicked.connect([this]() {
            auto index = m_list_view->current_index();
            if (index.is_valid()) {
                m_model->remove(index.row);
            }
        });

        // Selection Display
        m_selection_label = content->template emplace_child<label>("No selection");
        m_list_view->clicked.connect([this](const model_index& index) {
            auto data = m_model->data(index, item_data_role::display);
            std::string text = std::get<std::string>(data);
            m_selection_label->set_text("Selected: " + text);
        });
    }

private:
    std::shared_ptr<list_model<std::string, Backend>> m_model;
    list_view<Backend>* m_list_view;
    label<Backend>* m_selection_label;
    int m_item_counter = 1;
};
```

### Debug Tools Window

```cpp
template<UIBackend Backend>
class debug_tools_window : public main_window<Backend> {
public:
    debug_tools_window() : main_window<Backend>() {
        this->set_title("Debug Tools");

        // Get central widget from main_window
        auto* content = this->central_widget();
        content->set_vbox_layout(2);

        // Widget Inspector
        auto* inspector_section = content->template emplace_child<group_box>();
        inspector_section->set_title("Widget Inspector");
        inspector_section->set_vbox_layout(1);

        m_tree_view = inspector_section->template emplace_child<text_view>();
        m_tree_view->set_height(10);
        m_tree_view->set_text(generate_widget_tree());

        auto* refresh_btn = inspector_section->template emplace_child<button>("Refresh Tree");
        refresh_btn->clicked.connect([this]() {
            m_tree_view->set_text(generate_widget_tree());
        });

        // Bounds Visualization
        auto* bounds_section = content->template emplace_child<group_box>();
        bounds_section->set_title("Visualization");
        bounds_section->set_hbox_layout(2);

        m_show_bounds = bounds_section->template emplace_child<checkbox>("Bounds");
        m_show_padding = bounds_section->template emplace_child<checkbox>("Padding");
        m_show_focus = bounds_section->template emplace_child<checkbox>("Focus");

        // Performance Metrics
        auto* perf_section = content->template emplace_child<group_box>();
        perf_section->set_title("Performance");
        perf_section->set_vbox_layout(1);

        m_fps_label = perf_section->template emplace_child<label>("FPS: --");
        m_measure_time = perf_section->template emplace_child<label>("Measure: --ms");
        m_render_time = perf_section->template emplace_child<label>("Render: --ms");
        m_widget_count = perf_section->template emplace_child<label>("Widgets: --");

        // Event Log
        auto* event_section = content->template emplace_child<group_box>();
        event_section->set_title("Event Log");
        event_section->set_vbox_layout(1);

        m_event_log = event_section->template emplace_child<text_view>();
        m_event_log->set_height(10);

        auto* clear_btn = event_section->template emplace_child<button>("Clear");
        clear_btn->clicked.connect([this]() {
            m_event_log->set_text("");
        });
    }

private:
    std::string generate_widget_tree() {
        return "main_window\n"
               "├── menu_bar\n"
               "├── tab_widget\n"
               "│   ├── All Widgets\n"
               "│   ├── Layout & Scrolling\n"
               "│   └── Events & Interaction\n"
               "└── status_bar\n";
    }

    text_view<Backend>* m_tree_view;
    checkbox<Backend>* m_show_bounds;
    checkbox<Backend>* m_show_padding;
    checkbox<Backend>* m_show_focus;
    label<Backend>* m_fps_label;
    label<Backend>* m_measure_time;
    label<Backend>* m_render_time;
    label<Backend>* m_widget_count;
    text_view<Backend>* m_event_log;
};
```

---

## Benefits

### For Development
- **Quick widget testing** - All widgets in one place
- **Visual debugging** - See layout/theming issues immediately
- **Performance profiling** - Identify bottlenecks
- **Regression testing** - Screenshots for visual comparison

### For Documentation
- **Living examples** - Up-to-date widget usage
- **Interactive learning** - Experiment with features
- **Screenshot source** - Generate documentation images
- **Feature discovery** - Explore framework capabilities

### For Users
- **Theme preview** - See themes before using
- **Widget gallery** - Browse available components
- **Feature reference** - Quick lookup of widget APIs
- **Configuration testing** - Try different settings

---

## Implementation Phases

### Phase 1: Foundation & Main Window (Week 1)
- Main window structure
- Menu bar (File, Windows, Layers, Theme, Debug, Help)
- Status bar with live metrics
- Tab 1: All Widgets (basic widgets, containers, input widgets)
- Tab 2: Layout & Scrolling (layout system, scrolling system)

### Phase 2: Events & Windows (Week 2)
- Tab 3: Events & Interaction (events, focus, hotkeys)
- Windows menu implementation
- MVC Demo Window (Ctrl+M)
- Modal/modeless dialog examples
- Window list dialog integration

### Phase 3: Theme & Layers (Week 3)
- Theme Editor Window (Ctrl+T)
- Live theme switching
- Color palette editor
- Layers menu implementation
- Tooltip/popup/dropdown examples
- Layer stack inspector

### Phase 4: Debug Tools & Polish (Week 4)
- Debug Tools Window (F12)
- Widget tree inspector
- Bounds visualization
- Performance metrics
- Event log viewer
- Polish and optimization

---

## Testing Strategy

### Manual Testing
1. Open demo application
2. Navigate through all tabs
3. Interact with all widgets
4. Switch themes
5. Test keyboard navigation
6. Take screenshots

### Automated Testing
1. Create screenshot baseline for each tab
2. Run demo in headless mode
3. Compare screenshots for visual regressions
4. Verify all widgets render without crashes
5. Performance benchmarks

### Stress Testing
1. Performance tab: 1000+ widgets
2. MVC tab: 100k+ items
3. Rapid theme switching
4. Layout thrashing scenarios
5. Memory leak detection

---

## Configuration

### Build Configuration

```cmake
# CMakeLists.txt
add_executable(widgets_demo
    main.cc
    widgets_demo.cc

    # Tabs (3 total - main window content)
    tabs/tab_all_widgets.cc
    tabs/tab_layout_scrolling.cc
    tabs/tab_events_interaction.cc

    # Windows (spawned from menus)
    windows/mvc_demo_window.cc
    windows/theme_editor_window.cc
    windows/debug_tools_window.cc
    windows/modal_dialog_example.cc
    windows/modeless_dialog_example.cc

    # Layer examples (spawned from Layers menu)
    layers/tooltip_example.cc
    layers/popup_menu_example.cc
    layers/dropdown_example.cc
    layers/layer_visualizer.cc

    # Utilities
    utils/debug_panel.cc
    utils/event_logger.cc
    utils/performance_monitor.cc
    utils/widget_inspector.cc
    utils/bounds_visualizer.cc
)

target_link_libraries(widgets_demo
    PRIVATE
        onyxui::headers
        onyxui::conio
)

# Optional: Enable debug features in debug builds
target_compile_definitions(widgets_demo PRIVATE
    $<$<CONFIG:Debug>:WIDGETS_DEMO_DEBUG_MODE>
)
```

### Runtime Configuration

```cpp
// widgets_demo.hh
struct demo_config {
    bool enable_debug_panel = true;
    bool enable_performance_monitor = true;
    bool enable_event_logging = true;
    bool enable_bounds_visualization = false;

    std::string default_theme = "Norton Blue";
    int default_tab_index = 0;

    // Performance settings
    bool enable_fps_counter = true;
    int max_event_log_lines = 100;
};
```

---

## Future Enhancements

### Phase 6: Advanced Debug Features
- Memory profiler integration
- CPU profiler visualization
- Widget allocation tracker
- Signal/slot graph visualizer
- Theme diff viewer (compare themes)

### Phase 7: Automation
- Headless mode for CI/CD
- Screenshot regression testing
- Automated widget interaction
- Performance benchmark suite
- Memory leak detection

### Phase 8: Extensions
- Plugin system for custom tabs
- Export widget configurations
- Theme export/import
- Widget code generator
- Interactive tutorial mode

---

## Success Criteria

### Completeness
- ✅ All widgets showcased
- ✅ All framework features demonstrated
- ✅ All tabs implemented
- ✅ All menu items functional

### Quality
- ✅ Zero crashes during normal use
- ✅ Smooth performance (60 FPS)
- ✅ Proper theme application
- ✅ Keyboard navigation works
- ✅ Focus management correct

### Usability
- ✅ Intuitive navigation
- ✅ Clear widget labeling
- ✅ Helpful debug information
- ✅ Good visual organization
- ✅ Responsive to user input

### Documentation
- ✅ Code comments for each tab
- ✅ README with usage instructions
- ✅ Screenshot gallery
- ✅ Performance benchmarks
- ✅ Known limitations documented

---

## Related Documents

- `docs/CLAUDE/ARCHITECTURE.md` - Framework architecture
- `docs/CLAUDE/THEMING.md` - Theming system
- `docs/scrolling_guide.md` - Scrolling system
- `docs/MVC_DESIGN.md` - MVC system design
- `examples/demo.cc` - Current basic demo
- `CLAUDE.md` - Development guidelines

---

**Next Steps**: Begin Phase 1 implementation with foundation structure and basic tabs.
