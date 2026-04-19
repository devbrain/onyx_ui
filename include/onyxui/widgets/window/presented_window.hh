/**
 * @file presented_window.hh
 * @brief RAII-owned overlay presentation handle.
 *
 * @details
 * Consumers that present dialogs or other overlay windows shouldn't have
 * to manually track "is this window currently shown?" with booleans and
 * pair every `show(layers)` with a matching `hide(layers)` call. The
 * `presented_window<B>` handle is the presentation: owning it means the
 * window is on screen; dropping it takes the window off screen.
 *
 * ```cpp
 * // scene state:
 * std::unique_ptr<presented_window<B>> m_selector;
 *
 * // showing the dialog:
 * auto dlg = std::make_unique<window<B>>("Select Scenario", flags);
 * // ...populate dlg with content...
 * m_selector = std::make_unique<presented_window<B>>(
 *     layers, std::move(dlg), presentation_kind::modeless);
 *
 * // hiding the dialog:
 * m_selector.reset();
 *
 * // scene teardown drops m_selector implicitly — the dialog's layer is
 * // removed, the window is destroyed, no choreography needed.
 * ```
 *
 * Compare to the non-owning `scoped_layer<B>`: that handle only removes
 * the layer on destruction, not the widget behind it. `presented_window`
 * owns the widget too, which is what consumers almost always actually
 * want.
 *
 * Cross-manager safety and use-after-free protection are inherited for
 * free: `window::show()` / `show_modal()` register the window with the
 * passed `layer_manager` and subscribe to its `destroying` signal, so
 * the window is robust to either the presenter or the manager going
 * away first.
 */

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/services/layer_manager.hh>
#include <onyxui/widgets/window/window.hh>

#include <cassert>
#include <memory>
#include <utility>

namespace onyxui {

    /// Presentation style for `presented_window`. Maps onto window's
    /// `show` / `show_modal` entry points.
    enum class presentation_kind : std::uint8_t {
        modeless,   ///< Non-modal window layer. Uses `window::show`.
        modal,      ///< Modal dialog, dims background, traps focus.
    };

    /**
     * @brief RAII handle that presents a `window<Backend>` in a
     *        `layer_manager<Backend>` for as long as it lives.
     *
     * Owning a `presented_window` means the underlying window is shown.
     * Destroying it takes the window off screen AND drops the window.
     *
     * @tparam Backend The UI backend type.
     */
    template<UIBackend Backend>
    class presented_window {
    public:
        using window_type = window<Backend>;
        using layer_manager_type = layer_manager<Backend>;

        /**
         * @brief Present the given window via the given layer manager.
         *
         * @param layers  The layer manager that will host the window.
         * @param win     The window to present (moved in). Must not be
         *                null.
         * @param kind    Modal or modeless presentation.
         * @param pos     Dialog position for modal presentations. Ignored
         *                for modeless — `window::show` auto-centers or
         *                honors any bounds the caller pre-set on `win`.
         */
        presented_window(layer_manager_type& layers,
                         std::unique_ptr<window_type> win,
                         presentation_kind kind = presentation_kind::modal,
                         dialog_position pos = dialog_position::center)
            : m_window(std::move(win))
        {
            assert(m_window != nullptr && "presented_window: null window");
            if (kind == presentation_kind::modal) {
                m_window->show_modal(layers, pos);
            } else {
                m_window->show(layers);
            }
        }

        /// Destructor — hides and destroys the underlying window. Safe if
        /// the hosting `layer_manager` has already been torn down: the
        /// window's destructor is null-safe through its `m_layer_owner`
        /// observer on the manager's `destroying` signal.
        ~presented_window() = default;

        presented_window(const presented_window&) = delete;
        presented_window& operator=(const presented_window&) = delete;

        /// Move construction transfers ownership of the window. The
        /// moved-from object is a no-op presenter (does nothing on
        /// destruction).
        presented_window(presented_window&&) noexcept = default;

        /// Move assignment. The previously-owned window, if any, is
        /// hidden and destroyed first.
        presented_window& operator=(presented_window&&) noexcept = default;

        /// @brief Pointer to the hosted window. Non-null while this
        ///        presenter owns one; null after a move-from.
        [[nodiscard]] window_type* get() noexcept {
            return m_window.get();
        }

        /// @brief Const pointer to the hosted window.
        [[nodiscard]] const window_type* get() const noexcept {
            return m_window.get();
        }

        /// @brief Pointer-like access to the hosted window.
        [[nodiscard]] window_type* operator->() noexcept {
            return m_window.get();
        }

        /// @brief Const pointer-like access to the hosted window.
        [[nodiscard]] const window_type* operator->() const noexcept {
            return m_window.get();
        }

        /// @brief True if this presenter owns a window (i.e. not
        ///        moved-from).
        [[nodiscard]] explicit operator bool() const noexcept {
            return m_window != nullptr;
        }

        /// @brief True if the underlying window is visible. A valid
        ///        presenter in steady state returns true; during a
        ///        `window::close()` sequence it may be false briefly.
        [[nodiscard]] bool is_visible() const noexcept {
            return m_window && m_window->is_visible();
        }

        /// @brief Relinquish ownership of the window without hiding it.
        ///
        /// The caller takes ownership; the presenter becomes a no-op.
        /// Use this if you need to hand a window off to code that will
        /// manage its own lifetime.
        [[nodiscard]] std::unique_ptr<window_type> release() noexcept {
            return std::move(m_window);
        }

    private:
        /// The window. Its destructor runs when the presenter is
        /// destroyed; window::~window calls hide() internally, which
        /// removes the layer from the `m_layer_owner` it remembered
        /// at show-time.
        std::unique_ptr<window_type> m_window;
    };

} // namespace onyxui
