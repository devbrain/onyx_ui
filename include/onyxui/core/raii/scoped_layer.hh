/**
 * @file scoped_layer.hh
 * @brief RAII handle for automatic layer cleanup
 * @author Claude Code
 * @date October 19, 2025
 *
 * @details
 * Phase 1.4: RAII Handle - scoped_layer implementation
 *
 * Provides automatic layer cleanup using RAII pattern:
 * - Automatic removal on scope exit
 * - Exception-safe cleanup
 * - Move semantics support (movable but not copyable)
 * - Manual control (reset, release)
 *
 * Example usage:
 * @code
 * layer_manager<Backend> mgr;
 * auto elem = std::make_shared<ui_element<Backend>>();
 *
 * {
 *     auto layer = mgr.add_scoped_layer(layer_type::popup, elem);
 *     // Use layer...
 * }  // Automatically removed here
 * @endcode
 */

#pragma once

#include <onyxui/services/layer_manager.hh>

namespace onyxui {

/**
 * @brief RAII handle for automatic layer cleanup
 *
 * scoped_layer provides automatic lifetime management for layers.
 * When the scoped_layer is destroyed, it automatically removes
 * the layer from the layer_manager.
 *
 * Features:
 * - Automatic cleanup on scope exit
 * - Exception-safe (cleanup guaranteed)
 * - Move semantics (transfer ownership)
 * - Manual control (reset, release)
 *
 * @tparam Backend The UI backend type
 */
template<UIBackend Backend>
class scoped_layer {
public:
    using layer_manager_type = layer_manager<Backend>;

    /**
     * @brief Default constructor - creates invalid handle
     *
     * An invalid handle does nothing on destruction.
     */
    scoped_layer() noexcept
        : m_manager(nullptr)
        , m_id(layer_id::invalid())
    {}

    /**
     * @brief Construct from manager and layer ID
     *
     * Takes ownership of the layer. The layer will be automatically
     * removed when this handle is destroyed (unless released).
     *
     * @param manager Pointer to layer manager (can be nullptr)
     * @param id Layer ID to manage (can be invalid)
     */
    scoped_layer(layer_manager_type* manager, layer_id id) noexcept
        : m_manager(manager)
        , m_id(id)
    {}

    /**
     * @brief Deleted copy constructor (non-copyable)
     *
     * scoped_layer has unique ownership semantics.
     * Use std::move() to transfer ownership.
     */
    scoped_layer(const scoped_layer&) = delete;

    /**
     * @brief Move constructor
     *
     * Transfers ownership from other to this.
     * After the move, other becomes invalid.
     *
     * @param other The handle to move from
     */
    scoped_layer(scoped_layer&& other) noexcept
        : m_manager(other.m_manager)
        , m_id(other.m_id)
    {
        other.m_manager = nullptr;
        other.m_id = layer_id::invalid();
    }

    /**
     * @brief Destructor - automatically removes layer
     *
     * If this handle owns a valid layer, removes it from
     * the layer_manager. Does nothing if invalid or released.
     */
    ~scoped_layer() noexcept {
        reset();
    }

    /**
     * @brief Deleted copy assignment (non-copyable)
     */
    scoped_layer& operator=(const scoped_layer&) = delete;

    /**
     * @brief Move assignment operator
     *
     * Removes the currently owned layer (if any), then
     * transfers ownership from other.
     *
     * @param other The handle to move from
     * @return Reference to this
     */
    scoped_layer& operator=(scoped_layer&& other) noexcept {
        if (this != &other) {
            // Remove current layer
            reset();

            // Transfer ownership
            m_manager = other.m_manager;
            m_id = other.m_id;

            // Invalidate source
            other.m_manager = nullptr;
            other.m_id = layer_id::invalid();
        }
        return *this;
    }

    /**
     * @brief Check if handle owns a valid layer
     *
     * Returns true if:
     * - Manager pointer is valid
     * - Layer ID is valid
     *
     * Note: Does not check if the layer still exists in the manager.
     * The layer could have been manually removed.
     *
     * @return true if handle owns a layer
     */
    [[nodiscard]] bool is_valid() const noexcept {
        return m_manager != nullptr && m_id.is_valid();
    }

    /**
     * @brief Get the managed layer ID
     *
     * @return The layer ID (may be invalid)
     */
    [[nodiscard]] layer_id get() const noexcept {
        return m_id;
    }

    /**
     * @brief Manually remove the layer
     *
     * Removes the layer from the manager and invalidates this handle.
     * Safe to call multiple times (idempotent).
     * Safe to call on invalid handles.
     */
    void reset() noexcept {
        if (is_valid()) {
            m_manager->remove_layer(m_id);
            m_manager = nullptr;
            m_id = layer_id::invalid();
        }
    }

    /**
     * @brief Release ownership without removing layer
     *
     * Returns the layer ID and invalidates this handle,
     * but does NOT remove the layer from the manager.
     *
     * Use this when you want to keep the layer alive
     * beyond the scope of this handle.
     *
     * @return The layer ID (ownership transferred to caller)
     */
    [[nodiscard]] layer_id release() noexcept {
        const layer_id result = m_id;
        m_manager = nullptr;
        m_id = layer_id::invalid();
        return result;
    }

private:
    layer_manager_type* m_manager;
    layer_id m_id;
};

// ============================================================================
// Factory Method Implementation (Phase 1.4)
// ============================================================================

/**
 * @brief Implementation of layer_manager::add_scoped_layer()
 *
 * Defined here (after scoped_layer class definition) to avoid circular
 * dependency between layer_manager.hh and scoped_layer.hh.
 */
template<UIBackend Backend>
scoped_layer<Backend> layer_manager<Backend>::add_scoped_layer(
    layer_type type,
    std::shared_ptr<element_type> root,
    int custom_z_index)
{
    layer_id const id = add_layer(type, std::move(root), custom_z_index);
    return scoped_layer<Backend>(this, id);
}

}  // namespace onyxui
