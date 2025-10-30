/**
 * @file scoped_clip.hh
 * @brief RAII wrapper for render context clipping operations
 * @author OnyxUI Framework
 * @date 2025-10-29
 */

#pragma once

#include <onyxui/core/rendering/render_context.hh>
#include <utility>

namespace onyxui {

    /**
     * @class scoped_clip
     * @brief RAII guard for render context clip regions
     *
     * @details
     * Automatically manages push/pop of clip regions using RAII pattern.
     * The clip region is pushed in the constructor and popped in the destructor,
     * ensuring proper cleanup even in the presence of exceptions.
     *
     * This follows the same pattern as scoped_connection and scoped_layer.
     *
     * Usage:
     * @code
     * void do_render(render_context<Backend>& ctx) const override {
     *     // Push clip region - automatically popped when guard goes out of scope
     *     scoped_clip clip(ctx, viewport_bounds);
     *
     *     // Render children - clipped to viewport
     *     for (auto& child : children) {
     *         child->render(ctx);
     *     }
     *     // Clip automatically popped here when 'clip' is destroyed
     * }
     * @endcode
     *
     * @tparam Backend UIBackend implementation
     */
    template<UIBackend Backend>
    class scoped_clip {
    public:
        /**
         * @brief Construct guard and push clip region
         * @param ctx Render context to operate on
         * @param clip_rect Rectangle defining the clip region
         */
        scoped_clip(render_context<Backend>& ctx, typename Backend::rect_type clip_rect)
            : m_ctx(&ctx)
        {
            m_ctx->push_clip(clip_rect);
        }

        /**
         * @brief Destructor - automatically pops clip region
         */
        ~scoped_clip() {
            if (m_ctx) {
                m_ctx->pop_clip();
            }
        }

        // Non-copyable
        scoped_clip(const scoped_clip&) = delete;
        scoped_clip& operator=(const scoped_clip&) = delete;

        // Movable
        scoped_clip(scoped_clip&& other) noexcept
            : m_ctx(other.m_ctx)
        {
            other.m_ctx = nullptr;
        }

        scoped_clip& operator=(scoped_clip&& other) noexcept {
            if (this != &other) {
                // Pop current clip if we have one
                if (m_ctx) {
                    m_ctx->pop_clip();
                }

                // Take ownership from other
                m_ctx = other.m_ctx;
                other.m_ctx = nullptr;
            }
            return *this;
        }

    private:
        render_context<Backend>* m_ctx = nullptr;  ///< Context to manage (nullptr after move)
    };

} // namespace onyxui
