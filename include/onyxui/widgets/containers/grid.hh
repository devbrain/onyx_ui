/**
 * @file grid.hh
 * @brief Grid layout widget
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Convenience widget that arranges children in a two-dimensional grid.
 * This is a pre-configured widget with grid_layout.
 */

#pragma once

#include <onyxui/widgets/core/widget.hh>
#include <onyxui/layout/grid_layout.hh>
#include <onyxui/services/ui_services.hh>

namespace onyxui {
    /**
     * @class grid
     * @brief Grid container (2D table-like layout)
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * grid is a convenience widget that automatically uses grid_layout
     * to arrange children in a two-dimensional table-like structure.
     *
     * ## Features
     *
     * - Configurable number of columns and rows
     * - Cell spanning support (elements can span multiple rows/columns)
     * - Auto-sizing or fixed cell dimensions
     * - Configurable spacing between cells
     * - Automatic cell assignment or explicit positioning
     *
     * ## Auto-Assignment
     *
     * Children without explicit cell assignments are placed left-to-right,
     * top-to-bottom automatically.
     *
     * @example Simple 3-Column Grid
     * @code
     * auto container = std::make_unique<grid<Backend>>(3);  // 3 columns
     * for (int i = 0; i < 9; i++) {
     *     container->add_child(create_button(std::to_string(i)));
     * }
     * // Creates a 3x3 grid of buttons
     * @endcode
     *
     * @example Grid with Spanning
     * @code
     * auto container = std::make_unique<grid<Backend>>(
     *     4, -1,                          // 4 cols, auto rows
     *     spacing::small, spacing::small  // Small spacing between cells
     * );
     *
     * // Header spans all 4 columns
     * auto header = std::make_unique<label<Backend>>("Header");
     * container->add_child(std::move(header));
     * container->set_cell(header.get(), 0, 0, 1, 4);  // row 0, col 0, span 1x4
     *
     * // Sidebar spans 3 rows
     * auto sidebar = std::make_unique<panel<Backend>>();
     * container->add_child(std::move(sidebar));
     * container->set_cell(sidebar.get(), 1, 0, 3, 1);  // row 1, col 0, span 3x1
     *
     * // Other items auto-assigned around occupied cells
     * @endcode
     *
     * @example Fixed-Size Grid
     * @code
     * std::vector<int> col_widths = {100, 200, 100};  // 3 columns
     * std::vector<int> row_heights = {50, 100, 50};   // 3 rows
     *
     * auto container = std::make_unique<grid<Backend>>(
     *     3, 3,                           // 3x3 grid
     *     spacing::small, spacing::small, // Spacing between cells
     *     false,                          // Use fixed sizing
     *     col_widths,                     // Column widths
     *     row_heights                     // Row heights
     * );
     * @endcode
     */
    template<UIBackend Backend>
    class grid : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using element_type = ui_element<Backend>;

        /**
         * @brief Construct a grid container
         * @param num_columns Number of columns in grid (minimum 1)
         * @param num_rows Number of rows (-1 for auto-calculate)
         * @param column_spacing Semantic horizontal spacing between cells
         * @param row_spacing Semantic vertical spacing between cells
         * @param auto_size If true, size cells from content; if false, use fixed sizes
         * @param fixed_column_widths Fixed column widths (ignored if auto_size=true)
         * @param fixed_row_heights Fixed row heights (ignored if auto_size=true)
         * @param parent Parent element (nullptr for none)
         */
        explicit grid(
            int num_columns = 1,
            int num_rows = -1,
            spacing column_spacing = spacing::none,
            spacing row_spacing = spacing::none,
            bool auto_size = true,
            std::vector<int> fixed_column_widths = {},
            std::vector<int> fixed_row_heights = {},
            ui_element<Backend>* parent = nullptr
        )
            : base(parent)
            , m_column_spacing(column_spacing)
            , m_row_spacing(row_spacing) {
            auto layout = std::make_unique<grid_layout<Backend>>(
                num_columns,
                num_rows,
                resolve_column_spacing(),
                resolve_row_spacing(),
                auto_size,
                std::move(fixed_column_widths),
                std::move(fixed_row_heights)
            );

            // Store pointer before moving
            m_grid_layout = layout.get();
            this->set_layout_strategy(std::move(layout));
            this->set_focusable(false);  // Container, not focusable
        }

        /**
         * @brief Destructor
         */
        ~grid() override = default;

        // Rule of Five
        grid(const grid&) = delete;
        grid& operator=(const grid&) = delete;

        /**
         * @brief Move constructor
         * @note Updates raw layout pointer to maintain validity
         */
        grid(grid&& other) noexcept
            : base(std::move(static_cast<base&>(other)))
            , m_grid_layout(std::exchange(other.m_grid_layout, nullptr)) {}

        /**
         * @brief Move assignment operator
         * @note Updates raw layout pointer to maintain validity
         */
        grid& operator=(grid&& other) noexcept {
            if (this != &other) {
                base::operator=(std::move(static_cast<base&>(other)));
                m_grid_layout = std::exchange(other.m_grid_layout, nullptr);
            }
            return *this;
        }

        /**
         * @brief Explicitly assign a child to a grid cell
         *
         * @param child Pointer to the child element
         * @param row Row index (0-based)
         * @param col Column index (0-based)
         * @param row_span Number of rows to span (default 1)
         * @param col_span Number of columns to span (default 1)
         * @return true if cell was successfully assigned, false if validation failed
         *
         * @details
         * Use this method to position children at specific grid locations or
         * make them span multiple rows/columns. Children without explicit
         * assignments are auto-positioned left-to-right, top-to-bottom.
         */
        bool set_cell(element_type* child, int row, int col,
                      int row_span = 1, int col_span = 1) {
            if (!m_grid_layout) return false;
            return m_grid_layout->set_cell(child, row, col, row_span, col_span);
        }

        /**
         * @brief Get number of columns
         * @return Number of columns in the grid
         */
        [[nodiscard]] int num_columns() const noexcept {
            return m_grid_layout ? m_grid_layout->num_columns() : 0;
        }

        /**
         * @brief Get number of rows
         * @return Number of rows (-1 if auto-calculated)
         */
        [[nodiscard]] int num_rows() const noexcept {
            return m_grid_layout ? m_grid_layout->num_rows() : 0;
        }

        /**
         * @brief Get column spacing
         * @return Column spacing enum value
         */
        [[nodiscard]] spacing get_column_spacing() const noexcept {
            return m_column_spacing;
        }

        /**
         * @brief Get row spacing
         * @return Row spacing enum value
         */
        [[nodiscard]] spacing get_row_spacing() const noexcept {
            return m_row_spacing;
        }

        /**
         * @brief Set column spacing
         * @param spacing_value New column spacing value
         */
        void set_column_spacing(spacing spacing_value) {
            if (m_column_spacing != spacing_value) {
                m_column_spacing = spacing_value;
                this->invalidate_layout();
            }
        }

        /**
         * @brief Set row spacing
         * @param spacing_value New row spacing value
         */
        void set_row_spacing(spacing spacing_value) {
            if (m_row_spacing != spacing_value) {
                m_row_spacing = spacing_value;
                this->invalidate_layout();
            }
        }

    private:
        /**
         * @brief Resolve column spacing to backend-specific value
         * @return Column spacing in backend units (pixels for GUI, characters for TUI)
         */
        [[nodiscard]] int resolve_column_spacing() const {
            auto* themes = ui_services<Backend>::themes();
            if (!themes) {
                return 0;  // Default no spacing
            }
            auto* theme = themes->get_current_theme();
            if (!theme) {
                return 0;
            }
            return theme->spacing.resolve(m_column_spacing);
        }

        /**
         * @brief Resolve row spacing to backend-specific value
         * @return Row spacing in backend units (pixels for GUI, characters for TUI)
         */
        [[nodiscard]] int resolve_row_spacing() const {
            auto* themes = ui_services<Backend>::themes();
            if (!themes) {
                return 0;  // Default no spacing
            }
            auto* theme = themes->get_current_theme();
            if (!theme) {
                return 0;
            }
            return theme->spacing.resolve(m_row_spacing);
        }

        spacing m_column_spacing;                       ///< Column spacing enum value
        spacing m_row_spacing;                          ///< Row spacing enum value
        grid_layout<Backend>* m_grid_layout = nullptr;  ///< Non-owning pointer to layout
    };
}
