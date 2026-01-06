/**
 * @file test_minimal_overflow.cc
 * @brief Minimal test to isolate heap-buffer-overflow
 *
 * This test progressively simplifies to find the exact cause of memory corruption.
 */

#include <doctest/doctest.h>
#include <onyxui/core/element.hh>
#include <utils/test_canvas_backend.hh>

using namespace onyxui;
using namespace onyxui::testing;

/**
 * Minimal test element - no custom functionality
 */
class minimal_element : public ui_element<test_canvas_backend> {
public:
    explicit minimal_element(ui_element<test_canvas_backend>* parent = nullptr)
        : ui_element<test_canvas_backend>(parent) {}

protected:
    canvas_size get_content_size() const override {
        return canvas_size{10, 10};
    }
};

TEST_CASE("Minimal - Just create element") {
    auto root = std::make_unique<minimal_element>();
    CHECK(root != nullptr);
}

TEST_CASE("Minimal - Create element with child") {
    auto root = std::make_unique<minimal_element>();
    root->add_child(std::make_unique<minimal_element>());
    CHECK(root->children().size() == 1);
}

TEST_CASE("Minimal - Create and arrange parent only") {
    auto root = std::make_unique<minimal_element>();
    root->arrange({0, 0, 100, 100});
    // Verify bounds were set correctly
    CHECK(root->bounds().x == 0);
    CHECK(root->bounds().y == 0);
    CHECK(root->bounds().width == 100);
    CHECK(root->bounds().height == 100);
}

TEST_CASE("Minimal - Create parent+child, arrange parent only") {
    auto root = std::make_unique<minimal_element>();
    root->add_child(std::make_unique<minimal_element>());

    // HYPOTHESIS: Buffer overflow happens during arrange() when there are children
    root->arrange({0, 0, 100, 100});

    // Verify parent bounds were set and child still exists
    CHECK(root->bounds().width == 100);
    CHECK(root->bounds().height == 100);
    CHECK(root->children().size() == 1);
}

TEST_CASE("Minimal - Arrange parent then child") {
    auto root = std::make_unique<minimal_element>();
    root->add_child(std::make_unique<minimal_element>());

    auto* child = static_cast<minimal_element*>(root->children()[0].get());

    // First arrange parent
    root->arrange({0, 0, 100, 100});

    // Then arrange child
    child->arrange({10, 10, 80, 80});

    // Verify both parent and child bounds
    CHECK(root->bounds().width == 100);
    CHECK(root->bounds().height == 100);
    CHECK(child->bounds().x == 10);
    CHECK(child->bounds().y == 10);
    CHECK(child->bounds().width == 80);
    CHECK(child->bounds().height == 80);
}
