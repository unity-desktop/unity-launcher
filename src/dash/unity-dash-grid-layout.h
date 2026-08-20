/* unity-dash-grid-layout.h
 *
 * Copyright 2026 Muqtadir
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define UNITY_TYPE_DASH_GRID_LAYOUT (unity_dash_grid_layout_get_type ())

/**
 * UnityDashGridLayout:
 *
 * A #GtkLayoutManager that arranges uniform children in a grid of equal square
 * cells, with as many columns as fit the width.
 */
G_DECLARE_FINAL_TYPE (UnityDashGridLayout, unity_dash_grid_layout,
                      UNITY, DASH_GRID_LAYOUT, GtkLayoutManager)

/**
 * unity_dash_grid_layout_new:
 *
 * Makes a new grid layout manager.
 *
 * Returns: (transfer full): a new grid layout as a #GtkLayoutManager.
 */
GtkLayoutManager *unity_dash_grid_layout_new (void);

/**
 * unity_dash_grid_layout_set_max_rows:
 * @manager: a #UnityDashGridLayout.
 * @max_rows: the row cap, or 0 for no limit.
 *
 * Caps the grid to @max_rows rows. Tiles past the cap are hidden.
 */
void unity_dash_grid_layout_set_max_rows (GtkLayoutManager *manager, gint max_rows);

G_END_DECLS
