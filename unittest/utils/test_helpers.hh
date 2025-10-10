/**
 * @file test_helpers.hh
 * @brief Testing utilities and helper classes for layout tests
 */

#pragma once

#include <doctest/doctest.h>
#include <onyxui/element.hh>
#include "test_backend.hh"
#include <vector>

using namespace onyxui;

// Use the test_backend for all tests
using TestBackend = test_backend;
using TestRect = TestBackend::rect_type;
using TestSize = TestBackend::size_type;
using TestPoint = TestBackend::point_type;

// Test element that exposes protected members for testing
class TestElement : public ui_element<TestBackend> {
public:
    explicit TestElement(TestElement* parent = nullptr) : ui_element<TestBackend>(parent) {}

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

    // Expose bounds for testing
    const TestRect& test_bounds() const { return bounds(); }
};

// Custom element with content size
class ContentElement : public TestElement {
public:
    ContentElement(int w, int h) : TestElement(nullptr), content_width(w), content_height(h) {}
protected:
    TestSize get_content_size() const override {
        return TestSize{content_width, content_height};
    }
private:
    int content_width, content_height;
};

// Test fixture helper
class LayoutTestFixture {
public:
    void assert_bounds(TestElement* element, int x, int y, int w, int h) {
        const auto& bounds = element->test_bounds();
        CHECK(bounds.x == x);
        CHECK(bounds.y == y);
        CHECK(bounds.w == w);
        CHECK(bounds.h == h);
    }

    void assert_size(const TestSize& size, int w, int h) {
        CHECK(size.w == w);
        CHECK(size.h == h);
    }
};