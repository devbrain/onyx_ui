/**
 * @file tile_panel.hh
 * @brief Container widget with nine-slice tile background
 */

#pragma once

#include <onyxui/widgets/core/widget_container.hh>
#include <onyxui/tile/tile_core.hh>
#include <onyxui/tile/tile_renderer.hh>

namespace onyxui::tile {

/**
 * @class tile_panel
 * @brief Container widget that draws a nine-slice tile background
 *
 * @tparam Backend The UI backend type (typically sdlpp_tile_backend)
 *
 * @details
 * A tile_panel is a container that uses the tile renderer to draw
 * a nine-slice background from the tile atlas. It inherits from
 * widget_container to get layout and children support.
 *
 * @example
 * @code
 * auto panel = std::make_unique<tile_panel<sdlpp_tile_backend>>();
 * panel->add_child(std::make_unique<tile_label<sdlpp_tile_backend>>("Hello!"));
 * @endcode
 */
template<UIBackend Backend>
class tile_panel : public widget_container<Backend> {
public:
    using base = widget_container<Backend>;
    using size_type = typename Backend::size_type;

    /**
     * @brief Construct a tile panel with default vertical layout
     * @param parent Parent element (nullptr for none)
     * @param spacing Spacing between children (default 0)
     */
    explicit tile_panel(ui_element<Backend>* parent = nullptr, int spacing = 0)
        : base(
            std::make_unique<linear_layout<Backend>>(
                direction::vertical,
                spacing
            ),
            parent
          )
    {
        this->set_focusable(false);  // Panels aren't focusable
    }

    /**
     * @brief Destructor
     */
    ~tile_panel() override = default;

    // Rule of Five
    tile_panel(const tile_panel&) = delete;
    tile_panel& operator=(const tile_panel&) = delete;
    tile_panel(tile_panel&&) noexcept = default;
    tile_panel& operator=(tile_panel&&) noexcept = default;

    /**
     * @brief Set custom nine-slice for this panel
     * @param slice Nine-slice to use (overrides theme)
     */
    void set_nine_slice(const nine_slice& slice) {
        m_custom_slice = slice;
        m_use_custom_slice = true;
    }

    /**
     * @brief Reset to using theme's panel tiles
     */
    void use_theme_slice() {
        m_use_custom_slice = false;
    }

protected:
    /**
     * @brief Render the panel using tile renderer
     *
     * @details
     * Uses the global tile_renderer to draw a nine-slice background.
     * Children are rendered by the base class.
     */
    void do_render(render_context<Backend>& ctx) const override {
        auto* renderer = get_renderer();
        if (!renderer) {
            return;  // No renderer available
        }

        // Get physical position and size from context (already converted from logical)
        const auto& pos = ctx.position();
        const int x = point_utils::get_x(pos);
        const int y = point_utils::get_y(pos);
        const auto& size = ctx.available_size();
        const int w = size_utils::get_width(size);
        const int h = size_utils::get_height(size);

        // Create absolute rect for tile renderer (in physical pixels)
        tile_renderer::tile_rect abs_bounds{x, y, w, h};

        // Get the appropriate nine-slice
        const nine_slice* slice = nullptr;
        if (m_use_custom_slice) {
            slice = &m_custom_slice;
        } else if (has_theme()) {
            slice = &get_theme().panel.background;
        }

        // Draw background if we have a valid slice
        if (slice && slice->is_valid()) {
            renderer->draw_nine_slice(*slice, abs_bounds);
        }

        // Render children
        base::do_render(ctx);
    }

private:
    nine_slice m_custom_slice{};      ///< Custom nine-slice (if set)
    bool m_use_custom_slice = false;  ///< True to use custom slice instead of theme
};

} // namespace onyxui::tile
