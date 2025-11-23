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
┌──────────────────────────────────────────────────────────────────┐
│ Menu Bar: File | Widgets | Layout | Theme | Debug | Help         │
├──────────────────────────────────────────────────────────────────┤
│ Status Bar: Theme: Norton Blue | FPS: 60 | Focus: button_1      │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ Tab Widget (Main Content)                                  │ │
│  │ ┌──────────────────────────────────────────────────────────┤ │
│  │ │ Basic | Containers | Input | MVC | Layout | Advanced    │ │
│  │ ├──────────────────────────────────────────────────────────┤ │
│  │ │                                                          │ │
│  │ │   [Current Tab Content - Scrollable]                    │ │
│  │ │                                                          │ │
│  │ │                                                          │ │
│  │ └──────────────────────────────────────────────────────────┘ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
├──────────────────────────────────────────────────────────────────┤
│ Debug Panel: Bounds: (10,5,80,25) | Measure: 450ms | Draw: 30ms │
└──────────────────────────────────────────────────────────────────┘
```

### Tab Organization

#### Tab 1: Basic Widgets
- **Purpose**: Core UI elements
- **Widgets**:
  - `button` - All states (normal, hover, pressed, disabled, focused)
  - `label` - Various alignments, colors, styled text
  - `spacer` - Fixed spacing examples
  - `spring` - Flexible spacing examples
  - `status_bar` - Status display

#### Tab 2: Container Widgets
- **Purpose**: Layout containers
- **Widgets**:
  - `panel` - Generic container
  - `vbox` / `hbox` - Linear layouts with various spacing
  - `grid` - Grid layout with different alignments
  - `anchor_panel` - Anchor positioning examples
  - `absolute_panel` - Absolute positioning
  - `group_box` - Bordered container with title
  - `tab_widget` - Nested tabs demonstration

#### Tab 3: Input Widgets
- **Purpose**: User input controls
- **Widgets**:
  - `line_edit` - Text input, cursor, selection
  - `checkbox` - Two-state and tri-state
  - `radio_button` - Button groups, mutual exclusion
  - `slider` - Horizontal and vertical, different ranges
  - `progress_bar` - Determinate and indeterminate
  - `combo_box` - MVC dropdown selection

#### Tab 4: MVC Widgets
- **Purpose**: Model-View-Controller pattern
- **Widgets**:
  - `list_view` - Various selection modes
  - `combo_box` - Model binding examples
- **Features**:
  - Multiple models (string, custom types)
  - Selection tracking
  - Custom delegates
  - Dynamic model updates
  - Multi-view synchronization

#### Tab 5: Layout System
- **Purpose**: Layout algorithm demonstration
- **Features**:
  - Two-pass measure/arrange visualization
  - Size policies (fixed, stretch, percentage)
  - Alignment showcase (left, center, right, top, bottom, stretch)
  - Padding and margin examples
  - Nested layouts complexity
  - Layout performance testing

#### Tab 6: Scrolling System
- **Purpose**: Scrolling architecture showcase
- **Widgets**:
  - `scroll_view` - Different presets (modern, classic, compact)
  - `scrollable` - Core scrolling logic
  - `scrollbar` - Standalone scrollbar widget
  - `scroll_controller` - Bidirectional synchronization
- **Features**:
  - Large content scrolling (1000+ items)
  - Keyboard navigation (arrow keys, Page Up/Down, Home/End)
  - Mouse wheel scrolling
  - Scrollbar visibility modes (always, auto-hide, never)
  - Nested scrolling containers

#### Tab 7: Theming System
- **Purpose**: Theme system demonstration
- **Features**:
  - All registered themes in dropdown
  - Live theme switching
  - CSS-style inheritance visualization
  - Color palette display
  - Theme creation wizard
  - Widget state theming (normal/hover/pressed/disabled)
  - Custom theme editor

#### Tab 8: Event System
- **Purpose**: Event handling demonstration
- **Features**:
  - Three-phase routing (capture, target, bubble)
  - Mouse events (click, double-click, move, wheel)
  - Keyboard events (key press, mnemonics)
  - Focus events (gain, lose)
  - Event log viewer (real-time event stream)
  - Event filtering and search

#### Tab 9: Focus Management
- **Purpose**: Keyboard focus testing
- **Features**:
  - Tab order visualization
  - Focus indicators
  - Modal focus behavior
  - Focus restoration
  - Programmatic focus control
  - Focus chain inspection

#### Tab 10: Hotkeys System
- **Purpose**: Keyboard shortcuts showcase
- **Features**:
  - All hotkey schemes (Windows, Norton Commander, Emacs, Vim)
  - Global vs widget-specific hotkeys
  - Hotkey conflict detection
  - Custom hotkey registration
  - Mnemonic navigation (&File, &Edit)
  - Key sequence display (Ctrl+X Ctrl+C)

#### Tab 11: Layer Management
- **Purpose**: Layer system demonstration
- **Features**:
  - Popup layers (tooltips, dropdowns)
  - Modal dialogs
  - Z-order visualization
  - Click-outside-to-close behavior
  - Layer transitions
  - Multiple overlapping layers

#### Tab 12: Window System
- **Purpose**: Window management (if using window widgets)
- **Features**:
  - Multiple windows
  - Window z-order (bring to front)
  - Window cycling (Ctrl+F6)
  - Minimize/maximize/restore
  - Modal vs modeless windows
  - Window list dialog

#### Tab 13: Advanced Features
- **Purpose**: Complex integrations
- **Features**:
  - Menu system (menu_bar, menu, menu_item)
  - Styled text rendering
  - Background rendering
  - RAII guards (scoped_clip, scoped_layer, scoped_tooltip)
  - Service locator pattern
  - Signal/slot pattern examples

#### Tab 14: Performance Testing
- **Purpose**: Stress testing and profiling
- **Features**:
  - 1000+ button stress test
  - Large list rendering (100k items)
  - Rapid theme switching
  - Layout thrashing detection
  - Render performance metrics
  - Memory usage display

#### Tab 15: Debug Tools
- **Purpose**: Framework debugging utilities
- **Features**:
  - Widget tree inspector (hierarchical view)
  - Bounds visualizer (highlight widget rectangles)
  - Focus chain viewer
  - Layer stack inspector
  - Event trace viewer
  - Theme property inspector
  - Layout cache statistics
  - Render context stats

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
- New Window (Ctrl+N)
- Open Theme... (Ctrl+O)
- Save Screenshot (Ctrl+S)
- ---
- Exit (Alt+F4)

### Widgets Menu
- Basic Widgets (Tab 1)
- Containers (Tab 2)
- Input Widgets (Tab 3)
- MVC Widgets (Tab 4)
- ---
- Show All Widgets

### Layout Menu
- Layout System (Tab 5)
- Scrolling System (Tab 6)
- ---
- Show Bounds (Ctrl+B)
- Show Padding (Ctrl+P)
- Show Focus (Ctrl+F)

### Theme Menu
- Norton Blue
- Windows 3.x
- Midnight Commander
- ---
- Theme Editor...
- Reload Themes (F5)

### Debug Menu
- Widget Inspector (F12)
- Event Log (Ctrl+E)
- Performance Metrics (Ctrl+M)
- ---
- Layout Debug Mode
- Bounds Visualization
- Focus Chain Viewer
- ---
- Take Screenshot (F9)
- Dump Widget Tree

### Help Menu
- About OnyxUI
- Keyboard Shortcuts (F1)
- Framework Documentation
- ---
- Report Bug

---

## Status Bar Layout

```
┌──────────────────────────────────────────────────────────────────┐
│ Theme: Norton Blue │ FPS: 60 │ Focus: btn_ok │ Widgets: 247    │
└──────────────────────────────────────────────────────────────────┘
```

**Sections** (left to right):
1. Current theme name
2. FPS counter
3. Currently focused widget ID
4. Total widget count

---

## Implementation Details

### File Structure

```
examples/
  widgets_demo/
    main.cc                      # Entry point
    widgets_demo.hh              # Main widget class

    tabs/
      tab_basic_widgets.hh       # Tab 1: Basic widgets
      tab_containers.hh          # Tab 2: Container widgets
      tab_input_widgets.hh       # Tab 3: Input widgets
      tab_mvc_widgets.hh         # Tab 4: MVC widgets
      tab_layout_system.hh       # Tab 5: Layout system
      tab_scrolling_system.hh    # Tab 6: Scrolling system
      tab_theming_system.hh      # Tab 7: Theming system
      tab_event_system.hh        # Tab 8: Event system
      tab_focus_management.hh    # Tab 9: Focus management
      tab_hotkeys_system.hh      # Tab 10: Hotkeys system
      tab_layer_management.hh    # Tab 11: Layer management
      tab_window_system.hh       # Tab 12: Window system
      tab_advanced_features.hh   # Tab 13: Advanced features
      tab_performance_testing.hh # Tab 14: Performance testing
      tab_debug_tools.hh         # Tab 15: Debug tools

    utils/
      debug_panel.hh             # Debug visualization utilities
      event_logger.hh            # Event logging system
      performance_monitor.hh     # Performance tracking
      widget_inspector.hh        # Widget tree inspection
      bounds_visualizer.hh       # Visual bounds highlighting

    CMakeLists.txt               # Build configuration
```

### Widget Tree Structure

```cpp
main_window
├── menu_bar
│   ├── menu (File)
│   ├── menu (Widgets)
│   ├── menu (Layout)
│   ├── menu (Theme)
│   ├── menu (Debug)
│   └── menu (Help)
├── tab_widget (central widget)
│   ├── panel (Tab 1: Basic Widgets)
│   │   └── scroll_view
│   │       └── vbox
│   │           ├── section: Buttons
│   │           ├── section: Labels
│   │           └── section: Spacers/Springs
│   ├── panel (Tab 2: Containers)
│   ├── panel (Tab 3: Input Widgets)
│   ├── panel (Tab 4: MVC Widgets)
│   ├── panel (Tab 5: Layout System)
│   ├── panel (Tab 6: Scrolling System)
│   ├── panel (Tab 7: Theming System)
│   ├── panel (Tab 8: Event System)
│   ├── panel (Tab 9: Focus Management)
│   ├── panel (Tab 10: Hotkeys System)
│   ├── panel (Tab 11: Layer Management)
│   ├── panel (Tab 12: Window System)
│   ├── panel (Tab 13: Advanced Features)
│   ├── panel (Tab 14: Performance Testing)
│   └── panel (Tab 15: Debug Tools)
├── status_bar
│   ├── label (theme)
│   ├── label (FPS)
│   ├── label (focus)
│   └── label (widget count)
└── debug_panel (optional overlay)
    ├── bounds_visualizer
    ├── event_logger
    └── performance_monitor
```

---

## Example Tab Implementations

### Tab 1: Basic Widgets

```cpp
template<UIBackend Backend>
std::unique_ptr<panel<Backend>> create_basic_widgets_tab() {
    auto tab = std::make_unique<panel<Backend>>();
    tab->set_vbox_layout(2);  // 2px spacing

    // Section: Button States
    auto* section = tab->template emplace_child<group_box>();
    section->set_title("Button States");
    section->set_vbox_layout(1);

    auto* hbox1 = section->template emplace_child<hbox>(2);
    hbox1->emplace_child<label>("Normal:")->set_width(15);
    hbox1->emplace_child<button>("Click Me");

    auto* hbox2 = section->template emplace_child<hbox>(2);
    hbox2->emplace_child<label>("Disabled:")->set_width(15);
    auto* disabled_btn = hbox2->template emplace_child<button>("Disabled");
    disabled_btn->set_enabled(false);

    auto* hbox3 = section->template emplace_child<hbox>(2);
    hbox3->emplace_child<label>("With Mnemonic:")->set_width(15);
    auto* mnemonic_btn = hbox3->template emplace_child<button>("&Save File");

    // Section: Label Variations
    auto* label_section = tab->template emplace_child<group_box>();
    label_section->set_title("Label Variations");
    label_section->set_vbox_layout(1);

    label_section->emplace_child<label>("Left Aligned")->set_horizontal_alignment(alignment::left);
    label_section->emplace_child<label>("Center Aligned")->set_horizontal_alignment(alignment::center);
    label_section->emplace_child<label>("Right Aligned")->set_horizontal_alignment(alignment::right);

    // Section: Spacers and Springs
    auto* spacing_section = tab->template emplace_child<group_box>();
    spacing_section->set_title("Spacing Controls");
    spacing_section->set_hbox_layout(0);

    spacing_section->emplace_child<button>("Left");
    spacing_section->emplace_child<spacer>(20);  // Fixed 20px space
    spacing_section->emplace_child<button>("Middle");
    spacing_section->emplace_child<spring>();    // Flexible space
    spacing_section->emplace_child<button>("Right");

    return tab;
}
```

### Tab 3: Input Widgets

```cpp
template<UIBackend Backend>
std::unique_ptr<panel<Backend>> create_input_widgets_tab() {
    auto tab = std::make_unique<panel<Backend>>();
    tab->set_vbox_layout(2);

    // Line Edit
    auto* edit_section = tab->template emplace_child<group_box>();
    edit_section->set_title("Text Input (line_edit)");
    edit_section->set_vbox_layout(1);

    auto* edit = edit_section->template emplace_child<line_edit>();
    edit->set_text("Type here...");
    edit->set_width(40);

    // Checkboxes
    auto* checkbox_section = tab->template emplace_child<group_box>();
    checkbox_section->set_title("Checkboxes");
    checkbox_section->set_vbox_layout(1);

    auto* cb1 = checkbox_section->template emplace_child<checkbox>("Enable feature A");
    auto* cb2 = checkbox_section->template emplace_child<checkbox>("Enable feature B");
    cb2->set_checked(true);
    auto* cb3 = checkbox_section->template emplace_child<checkbox>("Tri-state checkbox");
    cb3->set_tri_state(true);
    cb3->set_check_state(check_state::partially_checked);

    // Radio Buttons
    auto* radio_section = tab->template emplace_child<group_box>();
    radio_section->set_title("Radio Buttons");
    radio_section->set_vbox_layout(1);

    auto group = std::make_shared<button_group<Backend>>();
    auto* r1 = radio_section->template emplace_child<radio_button>("Option 1", group);
    auto* r2 = radio_section->template emplace_child<radio_button>("Option 2", group);
    auto* r3 = radio_section->template emplace_child<radio_button>("Option 3", group);
    r1->set_checked(true);

    // Slider
    auto* slider_section = tab->template emplace_child<group_box>();
    slider_section->set_title("Slider (0-100)");
    slider_section->set_vbox_layout(1);

    auto* slider = slider_section->template emplace_child<slider>(orientation::horizontal);
    slider->set_range(0, 100);
    slider->set_value(50);
    slider->set_width(40);

    auto* value_label = slider_section->template emplace_child<label>("Value: 50");
    slider->value_changed.connect([value_label](int val) {
        value_label->set_text("Value: " + std::to_string(val));
    });

    // Progress Bar
    auto* progress_section = tab->template emplace_child<group_box>();
    progress_section->set_title("Progress Bar");
    progress_section->set_vbox_layout(1);

    auto* progress = progress_section->template emplace_child<progress_bar>();
    progress->set_range(0, 100);
    progress->set_value(75);
    progress->set_width(40);

    // Combo Box
    auto* combo_section = tab->template emplace_child<group_box>();
    combo_section->set_title("Combo Box (MVC)");
    combo_section->set_vbox_layout(1);

    auto model = std::make_shared<list_model<std::string, Backend>>();
    model->set_items({"Small", "Medium", "Large", "X-Large"});

    auto* combo = combo_section->template emplace_child<combo_box>();
    combo->set_model(model.get());
    combo->set_current_index(1);  // Select "Medium"

    return tab;
}
```

### Tab 15: Debug Tools

```cpp
template<UIBackend Backend>
class debug_tools_tab : public panel<Backend> {
public:
    debug_tools_tab() {
        this->set_vbox_layout(2);

        // Widget Inspector
        auto* inspector_section = this->template emplace_child<group_box>();
        inspector_section->set_title("Widget Inspector");
        inspector_section->set_vbox_layout(1);

        m_tree_view = inspector_section->template emplace_child<text_view>();
        m_tree_view->set_height(10);
        m_tree_view->set_text(generate_widget_tree());

        auto* refresh_btn = inspector_section->template emplace_child<button>("Refresh Tree");
        refresh_btn->clicked.connect([this]() {
            m_tree_view->set_text(generate_widget_tree());
        });

        // Bounds Visualizer
        auto* bounds_section = this->template emplace_child<group_box>();
        bounds_section->set_title("Bounds Visualization");
        bounds_section->set_vbox_layout(1);

        m_show_bounds = bounds_section->template emplace_child<checkbox>("Show Widget Bounds");
        m_show_bounds->checked_changed.connect([this](bool checked) {
            enable_bounds_visualization(checked);
        });

        m_show_padding = bounds_section->template emplace_child<checkbox>("Show Padding");
        m_show_focus = bounds_section->template emplace_child<checkbox>("Show Focus Chain");

        // Performance Monitor
        auto* perf_section = this->template emplace_child<group_box>();
        perf_section->set_title("Performance Metrics");
        perf_section->set_vbox_layout(1);

        m_fps_label = perf_section->template emplace_child<label>("FPS: --");
        m_measure_time = perf_section->template emplace_child<label>("Measure: --ms");
        m_arrange_time = perf_section->template emplace_child<label>("Arrange: --ms");
        m_render_time = perf_section->template emplace_child<label>("Render: --ms");
        m_widget_count = perf_section->template emplace_child<label>("Widgets: --");

        // Event Log
        auto* event_section = this->template emplace_child<group_box>();
        event_section->set_title("Event Log (Last 20)");
        event_section->set_vbox_layout(1);

        m_event_log = event_section->template emplace_child<text_view>();
        m_event_log->set_height(8);

        auto* clear_btn = event_section->template emplace_child<button>("Clear Log");
        clear_btn->clicked.connect([this]() {
            m_event_log->set_text("");
        });
    }

    void update_performance_metrics(const performance_stats& stats) {
        m_fps_label->set_text("FPS: " + std::to_string(stats.fps));
        m_measure_time->set_text("Measure: " + std::to_string(stats.measure_time_ms) + "ms");
        m_arrange_time->set_text("Arrange: " + std::to_string(stats.arrange_time_ms) + "ms");
        m_render_time->set_text("Render: " + std::to_string(stats.render_time_ms) + "ms");
        m_widget_count->set_text("Widgets: " + std::to_string(stats.widget_count));
    }

    void log_event(const std::string& event_description) {
        std::string current = m_event_log->text();
        std::string new_log = event_description + "\n" + current;

        // Keep only last 20 lines
        size_t line_count = 0;
        size_t pos = 0;
        while (line_count < 20 && pos != std::string::npos) {
            pos = new_log.find('\n', pos + 1);
            ++line_count;
        }
        if (pos != std::string::npos) {
            new_log = new_log.substr(0, pos);
        }

        m_event_log->set_text(new_log);
    }

private:
    std::string generate_widget_tree() {
        // Walk widget tree and generate hierarchical text representation
        // This would use the framework's widget tree structure
        return "main_window\n"
               "├── menu_bar\n"
               "├── tab_widget\n"
               "│   ├── panel (Basic Widgets)\n"
               "│   ├── panel (Containers)\n"
               "│   └── ...\n"
               "└── status_bar\n";
    }

    void enable_bounds_visualization(bool enable) {
        // Enable visual overlay showing widget bounds
        // This would integrate with the rendering system
    }

    text_view<Backend>* m_tree_view;
    checkbox<Backend>* m_show_bounds;
    checkbox<Backend>* m_show_padding;
    checkbox<Backend>* m_show_focus;
    label<Backend>* m_fps_label;
    label<Backend>* m_measure_time;
    label<Backend>* m_arrange_time;
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

### Phase 1: Foundation (Week 1)
- Main window structure
- Tab widget with basic tabs
- Menu bar and status bar
- Basic widgets tab (Tab 1)
- Containers tab (Tab 2)

### Phase 2: Input & MVC (Week 2)
- Input widgets tab (Tab 3)
- MVC widgets tab (Tab 4)
- Event logging infrastructure
- Performance monitoring

### Phase 3: Systems (Week 3)
- Layout system tab (Tab 5)
- Scrolling system tab (Tab 6)
- Theming system tab (Tab 7)
- Event system tab (Tab 8)

### Phase 4: Management (Week 4)
- Focus management tab (Tab 9)
- Hotkeys system tab (Tab 10)
- Layer management tab (Tab 11)
- Window system tab (Tab 12)

### Phase 5: Advanced (Week 5)
- Advanced features tab (Tab 13)
- Performance testing tab (Tab 14)
- Debug tools tab (Tab 15)
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
    tabs/tab_basic_widgets.cc
    tabs/tab_containers.cc
    tabs/tab_input_widgets.cc
    tabs/tab_mvc_widgets.cc
    tabs/tab_layout_system.cc
    tabs/tab_scrolling_system.cc
    tabs/tab_theming_system.cc
    tabs/tab_event_system.cc
    tabs/tab_focus_management.cc
    tabs/tab_hotkeys_system.cc
    tabs/tab_layer_management.cc
    tabs/tab_window_system.cc
    tabs/tab_advanced_features.cc
    tabs/tab_performance_testing.cc
    tabs/tab_debug_tools.cc
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
