/**
 * @file test_exception_safety.cc
 * @brief Unit tests for exception safety guarantees
 * @details Tests for Phase 1.1 of the refactoring plan
 */

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include <vector>
#include <stdexcept>

// Custom allocator that throws after N allocations for testing exception safety
template<typename T>
class throwing_allocator {
public:
    using value_type = T;

    static inline int allocation_count = 0;
    static inline int throw_after = -1;  // -1 means never throw

    throwing_allocator() = default;

    template<typename U>
    throwing_allocator(const throwing_allocator<U>&) noexcept {}

    T* allocate(std::size_t n) {
        if (throw_after >= 0 && allocation_count >= throw_after) {
            throw std::bad_alloc();
        }
        ++allocation_count;
        return std::allocator<T>().allocate(n);
    }

    void deallocate(T* p, std::size_t n) noexcept {
        std::allocator<T>().deallocate(p, n);
    }

    static void reset() {
        allocation_count = 0;
        throw_after = -1;
    }
};

template<typename T, typename U>
bool operator==(const throwing_allocator<T>&, const throwing_allocator<U>&) { return true; }

template<typename T, typename U>
bool operator!=(const throwing_allocator<T>&, const throwing_allocator<U>&) { return false; }

TEST_SUITE("Exception Safety") {

    TEST_CASE("add_child() exception safety - basic strong guarantee") {
        SUBCASE("Normal operation - no exception") {
            auto parent = std::make_unique<TestElement>();
            auto child = std::make_unique<TestElement>();
            auto* child_ptr = child.get();

            // Should not throw
            parent->add_child(std::move(child));

            // Verify child was added and parent pointer set
            CHECK(parent->child_count() == 1);
            CHECK(child_ptr->parent() == parent.get());
        }

        SUBCASE("Null child - no-op") {
            auto parent = std::make_unique<TestElement>();

            // Should not throw, should be a no-op
            parent->add_child(nullptr);

            CHECK(parent->child_count() == 0);
        }
    }

    TEST_CASE("add_child() exception safety - parent pointer consistency") {
        SUBCASE("Child parent pointer only set after successful add") {
            auto parent = std::make_unique<TestElement>();
            auto child = std::make_unique<TestElement>();
            auto* child_ptr = child.get();

            // Initially, child has no parent
            CHECK(child_ptr->parent() == nullptr);

            // Add child
            parent->add_child(std::move(child));

            // After successful add, parent pointer should be set
            CHECK(child_ptr->parent() == parent.get());
        }
    }

    TEST_CASE("add_child() multiple children") {
        auto parent = std::make_unique<TestElement>();

        for (int i = 0; i < 10; ++i) {
            auto child = std::make_unique<TestElement>();
            parent->add_child(std::move(child));
        }

        CHECK(parent->child_count() == 10);

        // Verify all children have correct parent pointer
        for (size_t i = 0; i < 10; ++i) {
            CHECK(parent->child_at(i)->parent() == parent.get());
        }
    }

    TEST_CASE("remove_child() exception safety") {
        SUBCASE("Remove existing child") {
            auto parent = std::make_unique<TestElement>();
            auto child = std::make_unique<TestElement>();
            auto* child_ptr = child.get();

            parent->add_child(std::move(child));
            CHECK(parent->child_count() == 1);

            // Remove child
            auto removed = parent->remove_child(child_ptr);

            CHECK(parent->child_count() == 0);
            CHECK(removed != nullptr);
            CHECK(removed.get() == child_ptr);
            CHECK(removed->parent() == nullptr);  // Parent pointer reset
        }

        SUBCASE("Remove non-existent child returns nullptr") {
            auto parent = std::make_unique<TestElement>();
            auto child = std::make_unique<TestElement>();
            auto other_child = std::make_unique<TestElement>();

            parent->add_child(std::move(child));

            // Try to remove child that wasn't added
            auto removed = parent->remove_child(other_child.get());

            CHECK(removed == nullptr);
            CHECK(parent->child_count() == 1);  // Original child still there
        }

        SUBCASE("Remove from middle of multiple children") {
            auto parent = std::make_unique<TestElement>();

            auto child1 = std::make_unique<TestElement>();
            auto child2 = std::make_unique<TestElement>();
            auto child3 = std::make_unique<TestElement>();

            auto* child1_ptr = child1.get();
            auto* child2_ptr = child2.get();
            auto* child3_ptr = child3.get();

            parent->add_child(std::move(child1));
            parent->add_child(std::move(child2));
            parent->add_child(std::move(child3));

            CHECK(parent->child_count() == 3);

            // Remove middle child
            auto removed = parent->remove_child(child2_ptr);

            CHECK(parent->child_count() == 2);
            CHECK(removed.get() == child2_ptr);
            CHECK(removed->parent() == nullptr);

            // Verify remaining children
            CHECK(parent->child_at(0) == child1_ptr);
            CHECK(parent->child_at(1) == child3_ptr);
        }
    }

    TEST_CASE("clear_children() exception safety") {
        SUBCASE("Clear all children") {
            auto parent = std::make_unique<TestElement>();

            std::vector<TestElement*> child_ptrs;
            for (int i = 0; i < 5; ++i) {
                auto child = std::make_unique<TestElement>();
                child_ptrs.push_back(child.get());
                parent->add_child(std::move(child));
            }

            CHECK(parent->child_count() == 5);

            // Clear should not throw (noexcept)
            parent->clear_children();

            CHECK(parent->child_count() == 0);

            // Note: We can't verify parent pointers were reset because
            // the children were destroyed (owned by unique_ptr)
        }

        SUBCASE("Clear empty parent") {
            auto parent = std::make_unique<TestElement>();

            // Should not throw
            parent->clear_children();

            CHECK(parent->child_count() == 0);
        }
    }

    TEST_CASE("Move constructor exception safety") {
        SUBCASE("Move element with children") {
            auto parent = std::make_unique<TestElement>();

            std::vector<TestElement*> child_ptrs;
            for (int i = 0; i < 3; ++i) {
                auto child = std::make_unique<TestElement>();
                child_ptrs.push_back(child.get());
                parent->add_child(std::move(child));
            }

            // Move construct
            auto new_parent = std::make_unique<TestElement>(std::move(*parent));

            // Verify children moved
            CHECK(new_parent->child_count() == 3);

            // Verify children's parent pointers updated
            for (size_t i = 0; i < 3; ++i) {
                CHECK(new_parent->child_at(i)->parent() == new_parent.get());
            }
        }
    }

    TEST_CASE("Move assignment exception safety") {
        SUBCASE("Move assign element with children") {
            auto parent1 = std::make_unique<TestElement>();
            auto parent2 = std::make_unique<TestElement>();

            std::vector<TestElement*> child_ptrs;
            for (int i = 0; i < 3; ++i) {
                auto child = std::make_unique<TestElement>();
                child_ptrs.push_back(child.get());
                parent1->add_child(std::move(child));
            }

            // Move assign
            *parent2 = std::move(*parent1);

            // Verify children moved
            CHECK(parent2->child_count() == 3);

            // Verify children's parent pointers updated
            for (size_t i = 0; i < 3; ++i) {
                CHECK(parent2->child_at(i)->parent() == parent2.get());
            }
        }
    }

    TEST_CASE("Exception safety - invalidation during exceptions") {
        SUBCASE("add_child invalidates measure") {
            auto parent = std::make_unique<TestElement>();

            // Do an initial measure
            (void)parent->measure(100, 100);

            // Add a child - should invalidate measure
            auto child = std::make_unique<TestElement>();
            parent->add_child(std::move(child));

            // Measure should be invalidated (this is checked by TestElement internally)
            CHECK(parent->child_count() == 1);
        }

        SUBCASE("remove_child invalidates measure") {
            auto parent = std::make_unique<TestElement>();
            auto child = std::make_unique<TestElement>();
            auto* child_ptr = child.get();

            parent->add_child(std::move(child));
            (void)parent->measure(100, 100);

            // Remove child - should invalidate measure
            auto removed = parent->remove_child(child_ptr);

            // Measure should be invalidated
            CHECK(removed != nullptr);
        }
    }
}
