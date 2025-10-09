# Comprehensive Refactoring Plan for Onyx UI Layout Library

## Executive Summary

This document outlines a structured refactoring plan for the Onyx UI layout library, addressing all issues identified in the code review. The plan is organized into 5 phases, from critical fixes to long-term enhancements, with clear priorities and estimated effort.

**Current Status**: Phase 1 (Critical Compilation) ✅ COMPLETE
**Next Phase**: Phase 2 (High Priority Correctness)
**Total Estimated Effort**: 40-50 hours

---

## Phase 1: Critical Compilation Issues ✅ COMPLETE

**Status**: ✅ Completed
**Effort**: 2 hours (DONE)

### 1.1 Fixed z_index Member Naming
- **Issue**: Inconsistent member variable naming
- **Solution**: Changed `z_index` to `m_z_index` throughout
- **Files**: element.hh (line 515)
- **Status**: ✅ DONE

### 1.2 Added Friend Declaration
- **Issue**: Layout strategies couldn't access protected members
- **Solution**: Added `friend class layout_strategy<TRect, TSize>;`
- **Files**: element.hh (line 103)
- **Status**: ✅ DONE

---

## Phase 2: High Priority Correctness Issues 🔴

**Status**: TODO
**Estimated Effort**: 12-15 hours
**Priority**: CRITICAL - These are bugs that affect core functionality

### 2.1 Grid Layout Cell Spanning Implementation
**Issue**: Elements spanning multiple cells are ignored during measurement
**Severity**: HIGH
**Location**: `grid_layout.hh:399-408`

**Current Problem**:
```cpp
// Only single-span cells are measured
if (cell.column_span == 1) {
    m_column_widths[cell.column] = std::max(m_column_widths[cell.column], meas_w);
}
```

**Solution**:
```cpp
// Implement proper spanning distribution
void distribute_span_sizes(const grid_cell_info& cell, int meas_w, int meas_h) {
    if (cell.column_span > 1) {
        int total_width = calculate_span_total(cell.column, cell.column_span, m_column_widths);
        if (meas_w > total_width) {
            int extra = meas_w - total_width;
            distribute_extra_space(cell.column, cell.column_span, extra, m_column_widths);
        }
    }
    // Similar for row_span
}
```

**Tasks**:
- [ ] Implement `distribute_span_sizes()` method
- [ ] Add span distribution algorithm (like HTML table algorithm)
- [ ] Handle spacing between spanned cells
- [ ] Add unit tests for spanning scenarios

### 2.2 Memory Safety - Layout Strategy Cleanup
**Issue**: Layout strategies hold raw pointers that can become dangling
**Severity**: HIGH
**Location**: All layout strategies with mappings

**Current Problem**:
```cpp
std::unordered_map<elt_t*, grid_cell_info> m_cell_mapping;  // Dangling if child removed
```

**Solution**:
```cpp
// Add cleanup mechanism to layout_strategy base class
class layout_strategy {
public:
    virtual void on_child_removed(ui_element* child) {
        // Default no-op
    }
    virtual void on_children_cleared() {
        // Default no-op
    }
};

// In grid_layout
void on_child_removed(ui_element* child) override {
    m_cell_mapping.erase(child);
}

// In ui_element::remove_child
if (m_layout_strategy) {
    m_layout_strategy->on_child_removed(child);
}
```

**Tasks**:
- [ ] Add virtual cleanup methods to layout_strategy base
- [ ] Implement cleanup in grid_layout
- [ ] Implement cleanup in anchor_layout
- [ ] Implement cleanup in absolute_layout
- [ ] Call cleanup from ui_element methods
- [ ] Add tests for dangling pointer scenarios

### 2.3 Grid Auto-Assignment Overlap Prevention
**Issue**: Auto-assignment doesn't check if cells are occupied
**Severity**: HIGH
**Location**: `grid_layout.hh:335-356`

**Current Problem**:
```cpp
// Doesn't check if cell is already occupied by spanning element
m_cell_mapping[child.get()] = {current_row, current_col, 1, 1};
```

**Solution**:
```cpp
class grid_layout {
private:
    // Track occupied cells
    std::set<std::pair<int, int>> calculate_occupied_cells() const {
        std::set<std::pair<int, int>> occupied;
        for (const auto& [child, info] : m_cell_mapping) {
            for (int r = info.row; r < info.row + info.row_span; ++r) {
                for (int c = info.column; c < info.column + info.column_span; ++c) {
                    occupied.insert({r, c});
                }
            }
        }
        return occupied;
    }

    std::pair<int, int> find_next_free_cell(const std::set<std::pair<int, int>>& occupied,
                                            int start_row, int start_col) const {
        int row = start_row;
        int col = start_col;

        while (occupied.count({row, col})) {
            col++;
            if (col >= m_num_columns) {
                col = 0;
                row++;
            }
        }
        return {row, col};
    }
};
```

**Tasks**:
- [ ] Implement occupied cell tracking
- [ ] Implement find_next_free_cell algorithm
- [ ] Update auto_assign_cells to use free cell finder
- [ ] Add tests for overlap scenarios

### 2.4 Linear Layout Spacing Calculation
**Issue**: Includes invisible children in spacing count
**Severity**: MEDIUM-HIGH
**Location**: `linear_layout.hh:145`

**Current Problem**:
```cpp
int total_spacing = m_spacing * (children.size() - 1);  // Wrong if some invisible
```

**Solution**:
```cpp
// Count visible children first
int visible_count = 0;
for (const auto& child : parent->children()) {
    if (child->is_visible()) visible_count++;
}
int total_spacing = (visible_count > 1) ? m_spacing * (visible_count - 1) : 0;
```

**Tasks**:
- [ ] Fix spacing calculation in measure_children
- [ ] Update both vertical and horizontal paths
- [ ] Add tests for mixed visible/invisible children

---

## Phase 3: Medium Priority - Robustness 🟡

**Status**: TODO
**Estimated Effort**: 10-12 hours
**Priority**: MEDIUM - Important for production quality

### 3.1 Property Accessor Redesign
**Issue**: Accessors invalidate on read access
**Location**: `element.hh:263-326`

**Solution**:
```cpp
// Separate const and non-const access patterns
class ui_element {
public:
    // Read-only access (no invalidation)
    const size_constraint& width_constraint() const noexcept {
        return m_width_constraint;
    }

    // Modification access (with invalidation)
    void set_width_constraint(const size_constraint& constraint) {
        if (m_width_constraint != constraint) {
            m_width_constraint = constraint;
            invalidate_measure();
        }
    }

    // Or use a proxy pattern for modification tracking
    constraint_proxy width_constraint() {
        return constraint_proxy(m_width_constraint,
                               [this]() { invalidate_measure(); });
    }
};
```

**Tasks**:
- [ ] Design new accessor pattern (choose approach)
- [ ] Implement for all properties
- [ ] Update all usage sites
- [ ] Document the new pattern

### 3.2 Weighted Distribution with Constraints
**Issue**: Weighted distribution doesn't properly handle min/max constraints
**Location**: `linear_layout.hh:257-266`

**Solution**: Implement iterative distribution algorithm
```cpp
void distribute_weighted_space(std::vector<ChildInfo>& children, int available_space) {
    bool changed = true;
    while (changed) {
        changed = false;
        float total_active_weight = calculate_active_weight(children);

        for (auto& child : children) {
            if (!child.satisfied && child.weight > 0) {
                int ideal = (int)(available_space * (child.weight / total_active_weight));
                int clamped = child.constraint.clamp(ideal);

                if (clamped != ideal) {
                    child.size = clamped;
                    child.satisfied = true;
                    available_space -= clamped;
                    changed = true;
                    break;  // Restart distribution
                }
            }
        }
    }
}
```

**Tasks**:
- [ ] Implement iterative weighted distribution
- [ ] Handle edge cases (no space, all constrained)
- [ ] Test with various constraint combinations

### 3.3 Integer Overflow Protection
**Issue**: Potential overflow in size calculations
**Location**: Multiple layout files

**Solution**:
```cpp
template<typename T>
bool safe_add(T a, T b, T& result) {
    if (a > 0 && b > std::numeric_limits<T>::max() - a) {
        return false;  // Overflow would occur
    }
    result = a + b;
    return true;
}

// Usage
int total_width = 0;
for (int w : m_column_widths) {
    if (!safe_add(total_width, w, total_width)) {
        // Handle overflow - clamp to max
        total_width = std::numeric_limits<int>::max();
        break;
    }
}
```

**Tasks**:
- [ ] Implement safe arithmetic utilities
- [ ] Apply to all accumulation operations
- [ ] Add overflow tests

### 3.4 Grid Layout Bounds Validation
**Issue**: No upper bounds checking in set_cell
**Location**: `grid_layout.hh:117-123`

**Solution**:
```cpp
void set_cell(elt_t* child, int row, int col, int row_span = 1, int col_span = 1) {
    if (!child) return;

    // Comprehensive validation
    if (row < 0 || col < 0 || row_span < 1 || col_span < 1) {
        throw std::invalid_argument("Invalid cell parameters");
    }

    if (col >= m_num_columns) {
        throw std::out_of_range("Column index exceeds grid columns");
    }

    if (col + col_span > m_num_columns) {
        throw std::out_of_range("Column span exceeds grid bounds");
    }

    // Row bounds checked if m_num_rows is fixed
    if (m_num_rows > 0 && row + row_span > m_num_rows) {
        throw std::out_of_range("Row span exceeds grid bounds");
    }

    m_cell_mapping[child] = {row, col, row_span, col_span};
}
```

**Tasks**:
- [ ] Add comprehensive validation
- [ ] Decide on error handling strategy (exceptions vs. silent fail)
- [ ] Add validation tests

---

## Phase 4: Low Priority - Code Quality 🟢

**Status**: TODO
**Estimated Effort**: 8-10 hours
**Priority**: LOW - Nice to have improvements

### 4.1 Consistent Naming Convention
**Issue**: Inconsistent getter naming (get_ prefix)
**Location**: `element.hh`

**Tasks**:
- [ ] Remove `get_` prefix from margin() and padding()
- [ ] Update all usage sites
- [ ] Update documentation

### 4.2 Rule of Five Implementation
**Issue**: Missing move semantics
**Location**: `element.hh`

**Solution**:
```cpp
class ui_element {
public:
    // Rule of Five
    ui_element(ui_element&&) noexcept = default;
    ui_element& operator=(ui_element&&) noexcept = default;
    ui_element(const ui_element&) = delete;
    ui_element& operator=(const ui_element&) = delete;
    virtual ~ui_element() = default;
};
```

**Tasks**:
- [ ] Add move constructor/assignment
- [ ] Explicitly delete copy operations
- [ ] Test move semantics

### 4.3 Propagated State Implementation
**Issue**: `propagated` state never used
**Location**: `element.hh:488-491`

**Solution**:
```cpp
void invalidate_measure() {
    if (measure_state == layout_state::propagated) {
        return;  // Already propagated, stop infinite recursion
    }

    if (measure_state == layout_state::valid) {
        measure_state = layout_state::propagated;
        arrange_state = layout_state::dirty;

        if (m_parent) {
            m_parent->invalidate_measure();
        }

        // After propagation, mark as dirty for actual recalc
        measure_state = layout_state::dirty;
    }
}
```

**Tasks**:
- [ ] Implement propagated state properly
- [ ] Or remove if not needed
- [ ] Add tests for invalidation chains

### 4.4 Performance Optimizations
**Issue**: Unnecessary allocations and copies
**Location**: Various

**Tasks**:
- [ ] Remove visible_children vector copy in linear_layout
- [ ] Use std::round for float-to-int conversions
- [ ] Add move semantics where appropriate
- [ ] Profile and optimize hot paths

### 4.5 Documentation Improvements
**Tasks**:
- [ ] Add complexity annotations (O(n))
- [ ] Document protected member purpose
- [ ] Add more code examples
- [ ] Create architecture diagram

---

## Phase 5: Testing and Validation 🧪

**Status**: TODO
**Estimated Effort**: 10-12 hours
**Priority**: HIGH - Essential for quality

### 5.1 Unit Test Suite
Create comprehensive test coverage for all components.

**Test Categories**:

#### Layout Strategy Tests
```cpp
TEST_CASE("Linear Layout") {
    SUBCASE("Vertical stacking") { /* ... */ }
    SUBCASE("Horizontal stacking") { /* ... */ }
    SUBCASE("Mixed visibility") { /* ... */ }
    SUBCASE("Weighted distribution") { /* ... */ }
    SUBCASE("Constraints") { /* ... */ }
}

TEST_CASE("Grid Layout") {
    SUBCASE("Basic grid") { /* ... */ }
    SUBCASE("Cell spanning") { /* ... */ }
    SUBCASE("Auto assignment") { /* ... */ }
    SUBCASE("Overlap prevention") { /* ... */ }
}
```

#### Memory Safety Tests
```cpp
TEST_CASE("Memory Safety") {
    SUBCASE("Child removal cleanup") { /* ... */ }
    SUBCASE("Layout strategy lifetime") { /* ... */ }
    SUBCASE("Circular references") { /* ... */ }
}
```

#### Edge Cases
```cpp
TEST_CASE("Edge Cases") {
    SUBCASE("Empty container") { /* ... */ }
    SUBCASE("Single child") { /* ... */ }
    SUBCASE("Zero size") { /* ... */ }
    SUBCASE("Negative margins") { /* ... */ }
    SUBCASE("Integer overflow") { /* ... */ }
}
```

**Tasks**:
- [ ] Create test infrastructure
- [ ] Write layout strategy tests
- [ ] Write invalidation tests
- [ ] Write memory safety tests
- [ ] Write edge case tests
- [ ] Add performance benchmarks
- [ ] Set up CI/CD with test runs

### 5.2 Integration Tests
Test with real UI frameworks:
- [ ] SDL integration test
- [ ] SFML integration test
- [ ] Custom renderer test

### 5.3 Stress Tests
- [ ] Large tree (1000+ elements)
- [ ] Deep nesting (100+ levels)
- [ ] Rapid invalidation cycles
- [ ] Memory leak detection

---

## Implementation Schedule

### Week 1: High Priority Fixes
- Day 1-2: Grid spanning implementation (2.1)
- Day 3-4: Memory safety cleanup (2.2)
- Day 5: Grid overlap prevention (2.3) and spacing fix (2.4)

### Week 2: Robustness
- Day 1-2: Property accessor redesign (3.1)
- Day 3: Weighted distribution (3.2)
- Day 4: Overflow protection (3.3)
- Day 5: Bounds validation (3.4)

### Week 3: Testing
- Day 1-2: Unit test infrastructure
- Day 3-4: Write comprehensive tests
- Day 5: Integration tests

### Week 4: Polish
- Day 1-2: Code quality improvements
- Day 3: Documentation
- Day 4: Performance optimization
- Day 5: Final validation

---

## Success Criteria

### Phase 2 Completion
- [ ] All grid layout features work correctly
- [ ] No memory safety issues
- [ ] No layout calculation bugs
- [ ] Tests pass for all fixes

### Phase 3 Completion
- [ ] Robust against edge cases
- [ ] Clean API design
- [ ] No integer overflows
- [ ] Proper error handling

### Phase 4 Completion
- [ ] Consistent code style
- [ ] Comprehensive documentation
- [ ] Optimized performance
- [ ] Clean architecture

### Phase 5 Completion
- [ ] >80% test coverage
- [ ] All tests passing
- [ ] No memory leaks
- [ ] Performance benchmarks established

---

## Risk Mitigation

### Technical Risks
1. **Breaking API changes**: Version properly, document migrations
2. **Performance regression**: Benchmark before/after each phase
3. **Integration issues**: Test with real frameworks early

### Process Risks
1. **Scope creep**: Stick to planned phases
2. **Testing gaps**: Write tests alongside fixes
3. **Documentation lag**: Update docs with each change

---

## Appendix: Issue Reference

### Critical Issues (Fixed) ✅
- z_index naming inconsistency
- Friend declaration missing

### High Priority Issues 🔴
- Grid spanning not implemented
- Memory safety with raw pointers
- Grid auto-assignment overlap
- Linear layout spacing bug

### Medium Priority Issues 🟡
- Property accessor design
- Weighted distribution bugs
- Integer overflow risks
- Bounds validation missing

### Low Priority Issues 🟢
- Naming consistency
- Rule of Five
- Propagated state unused
- Performance optimizations
- Documentation gaps

---

## Notes

This plan provides a structured approach to improving the Onyx UI library. Each phase builds upon the previous one, ensuring stability while progressively enhancing functionality. The estimated 40-50 hours of work will result in a production-ready, well-tested layout library.

Priority should be given to Phase 2 (High Priority Correctness) as these are actual bugs affecting functionality. Phase 5 (Testing) should be done in parallel with other phases to ensure quality throughout the refactoring process.