# SDL2 Backend Implementation Roadmap

## Project Status: **Alpha - Skeleton Complete**

The SDL2 backend has a complete architectural foundation following the conio backend pattern, but core rendering and event handling features are stubbed out.

---

## ✅ Implemented (Complete)

### Architecture & Structure
- [x] **Backend Definition** (`sdl2_backend.hh`)
  - Satisfies `UIBackend` concept
  - All required type aliases defined
  - Static assertion verified
  - Theme registration system integrated

- [x] **Geometry Types** (`geometry.hh`)
  - `rect` - Rectangle with x, y, w, h
  - `size` - Width and height
  - `point` - X and Y coordinates
  - All satisfy respective concepts (RectLike, SizeLike, PointLike)

- [x] **Color Type** (`colors.hh`)
  - RGB color structure (r, g, b)
  - Satisfies ColorLike concept
  - Equality operator

- [x] **Event Types** (`sdl2_events.hh`)
  - Uses SDL_Event for all event types
  - Type aliases defined for backend concept

- [x] **Theme System** (`src/sdl2_themes.hh`)
  - **Windows 3.11 theme** (default)
    - Authentic gray (192, 192, 192)
    - Raised/sunken border styles
    - Classic highlight/shadow colors
    - Navy blue for active elements
  - Theme registration via `sdl2_backend::register_themes()`
  - Automatic registration on first ui_context creation

- [x] **Build System** (`CMakeLists.txt`)
  - SDL2 and SDL2_ttf dependencies
  - Library target: `onyxui_sdl2_backend`
  - Demo target: `sdl2_demo`
  - Integrated into main CMakeLists.txt

- [x] **Demo Application** (`demo.cc`)
  - Window creation and initialization
  - Basic event loop (SDL_PollEvent)
  - Resize handling
  - UI tree creation with widgets
  - Layout (measure/arrange) pipeline

### Renderer Infrastructure
- [x] **Renderer Class** (`sdl2_renderer.hh/.cc`)
  - SDL_Renderer creation and management
  - PIMPL pattern for encapsulation
  - Clipping stack with push/pop
  - Viewport management
  - Color management (foreground/background)
  - Resize handling

- [x] **Static Measurement Methods**
  - `measure_text()` - Approximation (8px per char)
  - `get_icon_size()` - Returns 16x16
  - `get_border_thickness()` - Accurate calculations

- [x] **Box Style Enum**
  - none, flat, raised, sunken
  - thick_raised, thick_sunken
  - Windows 3.x style definitions

- [x] **Font Structure**
  - Family, size, bold, italic, underline
  - Defaults to "MS Sans Serif" 8pt

- [x] **Icon Enum**
  - check, cross, arrows (4 directions)
  - bullet, folder, file
  - minimize, maximize, close buttons

---

## ⚠️ Stubbed (Needs Implementation)

### Priority 1: Critical Rendering

#### 1.1 Windows 3.x Box Drawing
**Status:** Stub (draws simple rectangle)
**File:** `src/sdl2_renderer.cc:draw_box()`
**Current:** Single-line SDL_RenderDrawRect()
**Needed:**
```cpp
void sdl2_renderer::draw_box(const rect& r, box_style style) {
    switch (style) {
        case box_style::raised:
            // Draw white highlight on top/left
            // Draw dark gray shadow on bottom/right
            break;
        case box_style::sunken:
            // Reverse: dark gray top/left, white bottom/right
            break;
        case box_style::flat:
            // Single pixel border
            break;
        // ... etc
    }
}
```

**Implementation Notes:**
- Use SDL_RenderDrawLine() for bevels
- 2px thickness for raised/sunken (Windows 3.x standard)
- 4px thickness for thick variants
- Reference: Windows 3.x USER32 DrawEdge() behavior

**Complexity:** Medium
**Estimated Time:** 2-3 hours

---

#### 1.2 TTF Text Rendering
**Status:** Stub (clears region only)
**File:** `src/sdl2_renderer.cc:draw_text()`
**Current:** Calls `clear_region(r)` - no text displayed
**Needed:**

1. **Font Loading & Caching**
   ```cpp
   struct impl {
       std::map<font, TTF_Font*> font_cache;

       TTF_Font* load_font(const font& f) {
           // Check cache first
           // Load from system or bundled fonts
           // TTF_OpenFont() with size
           // Apply bold/italic via TTF_SetFontStyle()
       }
   };
   ```

2. **Text Rendering**
   ```cpp
   void draw_text(const rect& r, std::string_view text, const font& f) {
       TTF_Font* ttf = load_font(f);
       SDL_Color color = {fg_color.r, fg_color.g, fg_color.b, 255};

       // Render text to surface
       SDL_Surface* surface = TTF_RenderUTF8_Blended(ttf, text.data(), color);

       // Convert to texture
       SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

       // Draw with clipping
       SDL_Rect dst = {r.x, r.y, surface->w, surface->h};
       SDL_RenderCopy(renderer, texture, nullptr, &dst);

       SDL_DestroyTexture(texture);
       SDL_FreeSurface(surface);
   }
   ```

3. **Static Text Measurement** (Update stub)
   ```cpp
   static size measure_text(std::string_view text, const font& f) {
       // Load font (or use cached measurements)
       TTF_Font* ttf = load_font_static(f);

       int w, h;
       TTF_SizeUTF8(ttf, text.data(), &w, &h);
       return size{w, h};
   }
   ```

**Challenges:**
- Font file location (system fonts vs bundled)
- Font caching to avoid repeated loads
- UTF-8 handling (already correct with SDL_ttf UTF8 functions)
- Performance (texture creation per frame)

**Optimization Ideas:**
- Cache rendered text as textures (text + font + color → texture)
- Invalidate cache on resize
- Use SDL_ttf glyph cache

**Complexity:** High
**Estimated Time:** 4-6 hours

---

#### 1.3 Icon Rendering
**Status:** Stub (draws filled rectangle)
**File:** `src/sdl2_renderer.cc:draw_icon()`
**Current:** Placeholder rectangle
**Needed:**

**Option A: Bitmap Icons** (Authentic Windows 3.x)
```cpp
struct impl {
    std::map<icon_style, SDL_Texture*> icon_cache;

    void load_icons() {
        // Load from embedded XPM or PNG resources
        // Classic 16x16 Windows 3.x icons
        icon_cache[icon_style::check] = load_icon_bitmap("check.png");
        icon_cache[icon_style::minimize] = load_icon_bitmap("minimize.png");
        // ...
    }
};

void draw_icon(const rect& r, icon_style style) {
    SDL_Texture* icon = icon_cache[style];
    SDL_Rect dst = {r.x, r.y, 16, 16};
    SDL_RenderCopy(renderer, icon, nullptr, &dst);
}
```

**Option B: Procedural Drawing** (Simpler, no assets needed)
```cpp
void draw_icon(const rect& r, icon_style style) {
    switch (style) {
        case icon_style::check:
            // Draw checkmark using SDL_RenderDrawLine()
            break;
        case icon_style::minimize:
            // Draw horizontal line
            SDL_RenderDrawLine(r.x+2, r.y+r.h-3, r.x+r.w-2, r.y+r.h-3);
            break;
        case icon_style::close:
            // Draw X using two diagonal lines
            break;
    }
}
```

**Recommendation:** Start with Option B (procedural), add Option A later for polish.

**Complexity:** Low (procedural) / Medium (bitmap)
**Estimated Time:** 1-2 hours (procedural) / 3-4 hours (bitmap)

---

### Priority 2: Event Handling

#### 2.1 Event Dispatching to UI Tree
**Status:** TODO comment in demo
**File:** `demo.cc` main loop
**Current:** Events polled but not sent to widgets
**Needed:**

1. **Check ui_element Interface**
   ```bash
   # Search for event handling methods
   grep -r "handle_event\|dispatch_event" include/onyxui/
   ```

2. **Implement Event Dispatch**
   ```cpp
   // In demo.cc event loop:
   while (SDL_PollEvent(&event)) {
       if (event.type == SDL_QUIT) {
           running = false;
       } else {
           // Dispatch to UI tree
           root->handle_event(event);  // or similar method
       }
   }
   ```

3. **Possible Event Conversion Needed**
   - The UI system might expect a different event format
   - May need to convert SDL_Event to a generic event structure
   - Check how conio backend handles tb_event

**Investigation Required:**
- [ ] Read `include/onyxui/event_target.hh`
- [ ] Read `backends/conio/main.cc` to see tb_event handling
- [ ] Determine if event conversion/adapter is needed

**Complexity:** Medium (if adapter needed) / Low (if direct dispatch works)
**Estimated Time:** 2-4 hours

---

#### 2.2 Mouse Hit Testing
**Status:** Not implemented
**Dependency:** Event dispatching
**Needed:**
- Determine which widget is under mouse cursor
- Convert SDL mouse coordinates to UI coordinates
- Route mouse events to correct widget
- Handle hover states

**Notes:**
- UI system likely has built-in hit testing
- Check if `ui_element` provides `hit_test(point)` method
- May be handled automatically by event_target

---

#### 2.3 Keyboard Event Handling
**Status:** Not implemented
**Dependency:** Event dispatching
**Needed:**
- SDL_KEYDOWN/SDL_KEYUP mapping
- Keyboard focus management
- Tab navigation
- Hotkey support

---

### Priority 3: Polish & Optimization

#### 3.1 Text Rendering Optimization
- [ ] Texture caching for static text
- [ ] Dirty rectangle tracking
- [ ] Only re-render changed text

#### 3.2 Font Fallback System
- [ ] Try system fonts first (fontconfig on Linux)
- [ ] Fall back to bundled font
- [ ] Handle missing fonts gracefully

#### 3.3 Additional Themes
- [ ] Windows 95 theme
- [ ] Windows 98 theme
- [ ] High contrast theme
- [ ] Dark mode variant

#### 3.4 Advanced Box Styles
- [ ] Etched borders
- [ ] Ridge/groove effects
- [ ] Custom border colors

---

## 🔧 Technical Debt

### Code Quality
- [ ] Add error handling for SDL operations
- [ ] Add logging for debugging
- [ ] Improve texture lifecycle management
- [ ] Add resource cleanup verification

### Documentation
- [x] This roadmap
- [ ] API documentation comments
- [ ] Usage examples
- [ ] Performance considerations

### Testing
- [ ] Unit tests for renderer
- [ ] Visual regression tests
- [ ] Performance benchmarks
- [ ] Memory leak checks

---

## 📊 Completion Estimate

| Component | Status | Completion |
|-----------|--------|------------|
| Architecture | ✅ Complete | 100% |
| Theme System | ✅ Complete | 100% |
| Build System | ✅ Complete | 100% |
| Basic Rendering | ⚠️ Stubbed | 20% |
| Text Rendering | ❌ Missing | 0% |
| Icon Rendering | ❌ Missing | 0% |
| Event Handling | ❌ Missing | 0% |
| **Overall** | **Alpha** | **35%** |

---

## 🎯 Recommended Implementation Order

### Phase 1: Make It Visible (MVP)
1. **Procedural Icon Drawing** (1-2 hours)
   - Simple geometric shapes
   - Good enough for testing

2. **Basic TTF Text Rendering** (4-6 hours)
   - Single font, no caching
   - Just get text on screen

3. **Windows 3.x Box Drawing** (2-3 hours)
   - Proper beveled borders
   - Makes it look authentic

**Result:** Visual demo that looks like Windows 3.11 (8-11 hours)

---

### Phase 2: Make It Interactive
4. **Event Dispatching** (2-4 hours)
   - Wire up SDL events to UI tree
   - Mouse clicks work

5. **Mouse Hit Testing** (1-2 hours)
   - Hover effects
   - Click targeting

**Result:** Clickable buttons and interactive UI (3-6 hours)

---

### Phase 3: Make It Production-Ready
6. **Text Rendering Optimization** (3-4 hours)
   - Texture caching
   - Performance improvements

7. **Font System** (2-3 hours)
   - System font loading
   - Fallback handling

8. **Testing & Bug Fixes** (4-6 hours)
   - Edge cases
   - Memory leaks
   - Performance profiling

**Result:** Production-ready backend (9-13 hours)

---

## 📝 Notes for Implementation

### Resources Needed
- Windows 3.11 screenshots (for visual reference)
- SDL2 documentation (already available)
- SDL_ttf documentation
- Optional: Windows 3.x icon set (for bitmap icons)

### Key Files to Reference
- `backends/conio/src/conio_renderer.cc` - Example renderer implementation
- `backends/conio/main.cc` - Example event handling
- SDL2 examples for text rendering patterns

### Design Decisions to Make
1. **Font Strategy:**
   - Bundle a font? (portable but larger binary)
   - System fonts only? (smaller but platform-dependent)
   - Hybrid approach?

2. **Icon Strategy:**
   - Procedural (clean, scalable, no assets)
   - Bitmap (authentic, requires assets)
   - Both (best of both worlds)

3. **Performance:**
   - Immediate mode (re-render every frame)
   - Retained mode (cache textures)
   - Dirty rectangles (optimal but complex)

---

## 🚀 Quick Start for Contributors

To implement a specific feature:

1. **Read this roadmap** - Understand current state
2. **Check the priority** - Focus on P1 items first
3. **Reference conio backend** - See how it's done
4. **Write tests** - Verify it works
5. **Update this roadmap** - Mark completed items

### Example: Implementing Windows 3.x Box Drawing

```bash
# 1. Open the renderer implementation
vim backends/sdl2/src/sdl2_renderer.cc

# 2. Find the draw_box() stub
/draw_box

# 3. Implement raised border style first
# 4. Test with demo app
cmake --build build --target sdl2_demo
./build/bin/sdl2_demo

# 5. Mark as complete in roadmap
# 6. Commit changes
```

---

## 🎨 Visual Goals

The SDL2 backend should produce output that looks like this:

```
┌─────────────────────────────────────┐
│ Windows 3.11 Theme Demo             │  ← Title (black text)
├─────────────────────────────────────┤
│  ┌─────────────────────┐            │
│  │ Button 1            │            │  ← Raised button
│  └─────────────────────┘            │
│  ┌─────────────────────┐            │
│  │ Button 2            │            │
│  └─────────────────────┘            │
│  ┌─────────────────────┐            │
│  │ Disabled Button     │            │  ← Gray text
│  └─────────────────────┘            │
│                                     │
│  Classic Windows 3.11 Look and Feel │
└─────────────────────────────────────┘
```

With authentic Windows 3.x gray (RGB 192, 192, 192) throughout.

---

**Last Updated:** 2025-10-22
**Maintainer:** OnyxUI Project
**Status:** Living document - update as implementation progresses
