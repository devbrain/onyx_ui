/**
 * @file test_helpers.hh
 * @brief Testing utilities and helper classes for layout tests
 */

#pragma once

#include <doctest/doctest.h>
#include <onyxui/element.hh>
#include <vector>

// Test types
struct TestRect {
    int x, y, width, height;
};

struct TestSize {
    int width, height;
};

// Utility functions for test types
namespace onyxui {
    namespace rect_utils {
        template<>
        inline int get_x(const TestRect& r) { return r.x; }
        template<>
        inline int get_y(const TestRect& r) { return r.y; }
        template<>
        inline int get_width(const TestRect& r) { return r.width; }
        template<>
        inline int get_height(const TestRect& r) { return r.height; }
        template<>
        inline void set_bounds(TestRect& r, int x, int y, int w, int h) {
            r.x = x; r.y = y; r.width = w; r.height = h;
        }
        template<>
        inline bool contains(const TestRect& r, int x, int y) {
            return x >= r.x && x < r.x + r.width &&
                   y >= r.y && y < r.y + r.height;
        }
    }

    namespace size_utils {
        template<>
        inline int get_width(const TestSize& s) { return s.width; }
        template<>
        inline int get_height(const TestSize& s) { return s.height; }
        template<>
        inline void set_size(TestSize& s, int w, int h) {
            s.width = w; s.height = h;
        }
    }
}

using namespace onyxui;

// Test element that exposes protected members for testing
class TestElement : public ui_element<TestRect, TestSize> {
public:
    explicit TestElement(TestElement* parent = nullptr) : ui_element<TestRect, TestSize>(parent) {}

    // Get child at index for testing
    TestElement* child_at(size_t index) {
        const auto& ch = children();
        if (index < ch.size()) {
            return static_cast<TestElement*>(ch[index].get());
        }
        return nullptr;
    }

    size_t child_count() const { return children().size(); }

    // Helper to add test children
    void add_test_child(std::unique_ptr<TestElement> child) {
        add_child(std::move(child));
    }
};

// Custom element with content size
class ContentElement : public TestElement {
public:
    ContentElement(int w, int h) : TestElement(nullptr), content_width(w), content_height(h) {}
protected:
    TestSize get_content_size() const override {
        return {content_width, content_height};
    }
private:
    int content_width, content_height;
};

// Test fixture helper
class LayoutTestFixture {
public:
    void assert_bounds(TestElement* element, int x, int y, int w, int h) {
        CHECK(element->bounds().x == x);
        CHECK(element->bounds().y == y);
        CHECK(element->bounds().width == w);
        CHECK(element->bounds().height == h);
    }

    void assert_size(const TestSize& size, int w, int h) {
        CHECK(size.width == w);
        CHECK(size.height == h);
    }
};