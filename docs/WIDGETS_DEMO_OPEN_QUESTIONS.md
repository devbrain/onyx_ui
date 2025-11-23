# Widgets Demo - Open Questions

**Purpose**: Identify open questions before implementation begins.

**Last Updated**: 2025-11-23

---

## 1. Application Structure

### Q1.1: Main application class name and location?
- **Question**: What should the main application class be called?
- **Options**:
  - `widgets_demo_app`
  - `demo_main_window`
  - `widgets_showcase`
- **Location**: `examples/widgets_demo/widgets_demo.hh`?

### Q1.2: Entry point?
- **Question**: How should main.cc be structured?
- **Current pattern** (from examples/demo.cc):
  ```cpp
  scoped_ui_context<conio_backend> ui_ctx;
  auto widget = std::make_unique<main_widget<conio_backend>>();
  ui_handle<conio_backend> ui(std::move(widget));
  // Event loop...
  ```
- **Should we follow the same pattern?**

---

## 2. Window Management

### Q2.1: How to spawn additional windows?
- **Question**: When user clicks "MVC Demo..." menu item, how to create and show the window?
- **Options**:
  - Store window instances in main app class?
  - Use window_manager service to track windows?
  - Create on-demand and let framework manage?
- **Example pattern needed**:
  ```cpp
  void show_mvc_demo() {
      auto mvc_window = std::make_unique<mvc_demo_window<Backend>>();
      // How to show this window?
      // How to register with window_manager?
      // How to handle window lifetime?
  }
  ```

### Q2.2: Single instance vs multiple instances?
- **Question**: Can user open multiple "MVC Demo" windows or only one?
- **For each window type**:
  - MVC Demo: Single or multiple?
  - Theme Editor: Single or multiple?
  - Debug Tools: Single or multiple?
  - Modal/Modeless dialogs: Obviously multiple

### Q2.3: Window closing and cleanup?
- **Question**: How to handle window close events?
- **Should windows**:
  - Delete themselves on close?
  - Be kept alive by main app?
  - Notify main app when closing?

---

## 3. Screenshot Functionality

### Q3.1: Screenshot implementation?
- **Question**: How to implement `take_screenshot()`?
- **Current code** (from demo.hh:111-136):
  ```cpp
  void take_screenshot(const std::string& filename = "") {
      if (!m_renderer) return;

      // Generate filename with timestamp
      std::ofstream file(output_file);
      m_renderer->take_screenshot(file);
  }
  ```
- **Questions**:
  - Does renderer need to be passed to widgets_demo_app?
  - Or can we get it from ui_services?
  - Default filename format? `screenshot_<timestamp>.txt`?

### Q3.2: Screenshot of which window?
- **Question**: When multiple windows are open, what does screenshot capture?
- **Options**:
  - Current focused window only?
  - All windows (layered)?
  - Ask user which window?

---

## 4. Performance Metrics

### Q4.1: How to measure FPS?
- **Question**: How to calculate and display frames per second?
- **Need**:
  - Frame timing mechanism
  - Counter per second
  - Update status bar label
- **Where to implement**:
  - In main event loop?
  - In debug tools window?
  - Separate performance monitor class?

### Q4.2: How to measure layout times?
- **Question**: How to measure measure/arrange/render times?
- **Options**:
  - Instrument ui_element::measure() and arrange()?
  - Add timing to render loop?
  - Use existing profiling infrastructure?
- **Example**:
  ```cpp
  auto start = std::chrono::high_resolution_clock::now();
  widget->measure(w, h);
  auto end = std::chrono::high_resolution_clock::now();
  auto measure_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  ```

### Q4.3: Widget count calculation?
- **Question**: How to count total widgets in all windows?
- **Implementation**:
  - Recursive tree walk?
  - Maintain counter during add_child?
  - Query window_manager for all windows and sum?

---

## 5. Debug Visualization

### Q5.1: Bounds visualization implementation?
- **Question**: How to draw widget bounds overlay?
- **Options**:
  - Add debug layer on top of all windows?
  - Modify rendering pipeline to draw bounds?
  - Post-render overlay pass?
- **What to draw**:
  - Widget rectangles (outline)?
  - Different colors for different widget types?
  - Padding/margin visualization?

### Q5.2: Focus chain visualization?
- **Question**: How to show tab order and focus chain?
- **Visual representation**:
  - Numbers on each widget (1, 2, 3...)?
  - Lines connecting widgets in tab order?
  - Highlight current focused widget?

---

## 6. Event Logging

### Q6.1: Global event capture?
- **Question**: How to capture all events for event log?
- **Options**:
  - Hook into event dispatch system?
  - Register global event handler?
  - Instrument ui_handle event processing?
- **Event filtering**:
  - Which events to log? (All? Mouse only? Keyboard only?)
  - How to filter by event type?

### Q6.2: Event log storage?
- **Question**: Where to store event log entries?
- **Implementation**:
  - Circular buffer (last N events)?
  - Vector with periodic truncation?
  - Separate event_logger class?
- **Thread safety**:
  - Events logged from main thread only?
  - Need synchronization?

---

## 7. Theme System Integration

### Q7.1: Live theme switching?
- **Question**: How to switch theme at runtime?
- **Current pattern** (from demo.hh:72):
  ```cpp
  demo_utils::apply_theme_by_name<Backend>(theme_name);
  ```
- **Questions**:
  - Does this update all windows?
  - Need to trigger repaint?
  - How to update status bar theme label?

### Q7.2: Theme reloading (F5)?
- **Question**: What does "Reload Themes" actually do?
- **Expected behavior**:
  - Re-scan theme directory for .yaml files?
  - Reload current theme from disk?
  - Rebuild theme registry?
- **Implementation location**: theme_registry?

---

## 8. Hotkey Registration

### Q8.1: Application-specific hotkeys?
- **Question**: How to register hotkeys for demo-specific actions?
- **Hotkeys to register**:
  - Ctrl+M → Show MVC Demo
  - Ctrl+T → Show Theme Editor
  - F12 → Show Debug Tools
  - F9 / Ctrl+S → Take Screenshot
  - Ctrl+B/P/F → Debug toggles
- **Pattern**:
  ```cpp
  // How to register these?
  hotkeys.register_action("show_mvc_demo", "Ctrl+M", [&]() {
      show_mvc_demo_window();
  });
  ```

### Q8.2: Hotkey scope?
- **Question**: Should hotkeys be global or window-specific?
- **Behavior**:
  - F9 (screenshot) → Screenshot focused window or all windows?
  - Ctrl+M (MVC demo) → Always available or only in main window?
  - F12 (debug tools) → Global or main window only?

---

## 9. Layer Examples

### Q9.1: Tooltip example implementation?
- **Question**: How to show tooltip layer programmatically?
- **Current pattern** (from existing code):
  - Tooltips usually auto-show on hover
  - How to trigger manually for demo?
- **Example needed**:
  ```cpp
  void show_tooltip_example() {
      // Show tooltip at cursor position?
      // Show tooltip near a specific widget?
      // Custom tooltip content?
  }
  ```

### Q9.2: Popup menu example?
- **Question**: How to demonstrate popup menu layer?
- **Implementation**:
  - Create menu programmatically?
  - Show at cursor position?
  - Demo click-outside-to-close?

### Q9.3: Dropdown example?
- **Question**: Is this the combo_box popup (when implemented)?
- **Or**: A separate dropdown widget?
- **Status**: combo_box popup is TODO (requires layer_manager integration)

---

## 10. MVC Models

### Q10.1: Shared vs independent models?
- **Question**: If user opens two MVC Demo windows, do they share the same model?
- **Options**:
  - **Shared model**: Both windows show same data, edits sync
  - **Independent models**: Each window has own data
- **Implication**: Demonstrates model sharing if shared

### Q10.2: Model lifetime?
- **Question**: Who owns the model?
- **Options**:
  - MVC Demo window owns model (destroyed with window)
  - Main app owns model (persists across window open/close)
  - Shared_ptr managed by multiple windows

---

## 11. Tab Widget Integration

### Q11.1: How to create tab widget with 3 tabs?
- **Question**: What's the API for tab_widget?
- **Expected usage**:
  ```cpp
  auto tabs = central_widget->emplace_child<tab_widget>();
  tabs->add_tab("All Widgets", create_all_widgets_tab());
  tabs->add_tab("Layout & Scrolling", create_layout_scrolling_tab());
  tabs->add_tab("Events & Interaction", create_events_interaction_tab());
  ```
- **Need**: Confirm tab_widget API

### Q11.2: Tab switching?
- **Question**: How does user switch tabs?
- **Mouse**: Click tab header?
- **Keyboard**: Ctrl+Tab? Ctrl+PageUp/PageDown?
- **Hotkeys**: Does tab widget support hotkeys?

---

## 12. Modal Dialog Examples

### Q12.1: Modal dialog implementation?
- **Question**: How to create and show modal dialog?
- **Expected pattern**:
  ```cpp
  void show_modal_dialog() {
      auto dialog = std::make_unique<modal_dialog_example<Backend>>();
      dialog->show_modal();  // Blocks parent window?
  }
  ```
- **Behavior**:
  - Focus trapped in dialog?
  - Parent window interaction blocked?
  - ESC to close?

### Q12.2: Modeless dialog?
- **Question**: Difference from modal?
- **Expected**:
  - Both windows remain interactive
  - No focus trapping
  - Just a separate window?

---

## 13. Build System

### Q13.1: CMakeLists.txt location?
- **Question**: Where does widgets_demo CMake config go?
- **Options**:
  - `examples/widgets_demo/CMakeLists.txt`
  - Added to `examples/CMakeLists.txt`
  - Top-level CMakeLists.txt

### Q13.2: Executable name?
- **Question**: What should the built executable be called?
- **Options**:
  - `widgets_demo`
  - `onyxui_demo`
  - `showcase`

---

## 14. Backend Selection

### Q14.1: Which backend to use?
- **Question**: Should demo support multiple backends?
- **Options**:
  - conio_backend only (TUI)
  - Template on Backend (works with any backend)
  - Multiple executables (one per backend)
- **Current codebase**: Primarily uses conio_backend

### Q14.2: Backend-specific features?
- **Question**: Any backend-specific code needed?
- **Examples**:
  - Screenshot format (text for conio, PNG for SDL?)
  - Input handling differences?
  - Rendering differences?

---

## 15. Status Bar Updates

### Q15.1: Real-time status updates?
- **Question**: How to update status bar metrics continuously?
- **Mechanism**:
  - Timer-based updates? (every 100ms?)
  - Event-driven updates? (on widget count change, focus change)
  - Update in render loop?
- **Which metrics update when**:
  - Theme: On theme switch
  - FPS: Continuously (every second?)
  - Focus: On focus change
  - Widget count: On add/remove child, window open/close

### Q15.2: Status bar API?
- **Question**: How to update status bar labels?
- **Expected pattern**:
  ```cpp
  status_bar->set_section_text(0, "Theme: " + current_theme);
  status_bar->set_section_text(1, "FPS: " + std::to_string(fps));
  ```
- **Or**: Direct label references?

---

## 16. Widget Tree Generation

### Q16.1: How to walk widget tree?
- **Question**: How to generate hierarchical widget tree text?
- **Implementation**:
  - Recursive function walking children?
  - Does ui_element provide children() accessor?
  - Format: ASCII tree like current example?
- **Example output**:
  ```
  main_window
  ├── menu_bar
  ├── tab_widget
  │   ├── panel (All Widgets)
  │   │   └── scroll_view
  │   │       └── vbox
  │   └── ...
  └── status_bar
  ```

### Q16.2: Multi-window tree?
- **Question**: Show all windows in tree, or just focused window?
- **If all windows**:
  - How to get list of all windows?
  - Window_manager API?

---

## 17. Error Handling

### Q17.1: What if no themes found?
- **Question**: How to handle missing theme files?
- **Current code** (demo.hh:52):
  ```cpp
  if (m_theme_names.empty()) {
      throw std::runtime_error("No themes registered!");
  }
  ```
- **Should we**:
  - Throw exception (fail fast)?
  - Use default colors (graceful degradation)?
  - Show error dialog?

### Q17.2: Failed window creation?
- **Question**: What if spawning MVC Demo window fails?
- **Error handling**:
  - Show error message?
  - Log and continue?
  - Graceful degradation?

---

## 18. Testing Strategy

### Q18.1: Unit tests for demo?
- **Question**: Should widgets_demo have unit tests?
- **What to test**:
  - Window creation?
  - Screenshot functionality?
  - Event logging?
- **Or**: Demo is itself the test?

### Q18.2: Visual regression baseline?
- **Question**: Should we commit baseline screenshots?
- **Location**: `screenshots/baseline/`?
- **Usage**: Compare new screenshots to detect visual regressions

---

## Priority Questions (Must Answer Before Starting)

### High Priority (Blocking)
1. **Window spawning pattern** (Q2.1) - How to create and show additional windows?
2. **Screenshot implementation** (Q3.1) - How to access renderer?
3. **Tab widget API** (Q11.1) - How to create tabs?
4. **Hotkey registration** (Q8.1) - How to register app-specific hotkeys?

### Medium Priority (Needed for Phase 1)
5. **Application class structure** (Q1.1, Q1.2) - Main app pattern
6. **Status bar updates** (Q15.1, Q15.2) - How to update metrics
7. **Theme switching** (Q7.1) - Runtime theme changes
8. **Performance metrics** (Q4.1, Q4.2, Q4.3) - FPS/timing measurement

### Low Priority (Can defer to later phases)
9. **Debug visualization** (Q5.1, Q5.2) - Bounds overlay implementation
10. **Event logging** (Q6.1, Q6.2) - Global event capture
11. **Layer examples** (Q9.1, Q9.2, Q9.3) - Tooltip/popup demos
12. **Widget tree generation** (Q16.1, Q16.2) - Tree walking implementation

---

## Suggested Approach

1. **Start with existing demo code** (`examples/demo.cc`, `demo.hh`) as template
2. **Answer high-priority questions** by examining existing patterns in codebase
3. **Implement Phase 1** (foundation) with simplified features
4. **Iterate** based on what works and what doesn't

---

## Next Steps

1. Review this document with project maintainer
2. Answer high-priority questions
3. Create initial implementation skeleton
4. Start with Phase 1: Foundation & Main Window
