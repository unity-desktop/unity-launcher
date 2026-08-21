/* unity-position.h
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

#include <astal-4.h>

G_BEGIN_DECLS

#define UNITY_LAUNCHER_KEY_POSITION "position"
#define UNITY_LAUNCHER_KEY_TILE_ALIGNMENT "tile-alignment"

/**
 * UnityPosition:
 * @UNITY_POSITION_LEFT: the left edge, tiles stacked vertically.
 * @UNITY_POSITION_RIGHT: the right edge, tiles stacked vertically.
 * @UNITY_POSITION_BOTTOM: the bottom edge, tiles in a row.
 *
 * The screen edge the launcher sits on. The values match the "position" enum in
 * the gschema, so they map straight from g_settings_get_enum().
 */
typedef enum
{
  UNITY_POSITION_LEFT,
  UNITY_POSITION_RIGHT,
  UNITY_POSITION_BOTTOM,
} UnityPosition;

/**
 * UnityTileAlignment:
 * @UNITY_TILE_ALIGNMENT_START: pack the tiles from the near edge.
 * @UNITY_TILE_ALIGNMENT_CENTER: center the tile group along the strip.
 * @UNITY_TILE_ALIGNMENT_END: pack the tiles at the far edge.
 *
 * Where the app tiles sit along the launcher. The values match the
 * "tile-alignment" enum in the gschema, so they map straight from
 * g_settings_get_enum().
 */
typedef enum
{
  UNITY_TILE_ALIGNMENT_START,
  UNITY_TILE_ALIGNMENT_CENTER,
  UNITY_TILE_ALIGNMENT_END,
} UnityTileAlignment;

/**
 * unity_position_anchor:
 * @position: a #UnityPosition.
 *
 * Gets the layer-shell anchor for @position. The launcher anchors the chosen
 * edge and the two edges beside it, so it spans that side of the screen.
 *
 * Returns: the anchor flags to pass to astal_window_set_anchor().
 */
AstalWindowAnchor unity_position_anchor (UnityPosition position);

/**
 * unity_position_orientation:
 * @position: a #UnityPosition.
 *
 * Gets the axis the tile strip runs along. Left and right stack vertically, the
 * bottom runs horizontally.
 *
 * Returns: the #GtkOrientation for the strip.
 */
GtkOrientation unity_position_orientation (UnityPosition position);

/**
 * unity_position_style_class:
 * @position: a #UnityPosition.
 *
 * Gets the CSS class that draws the divider on the edge facing the screen.
 *
 * Returns: a static string, one of "pos-left", "pos-right", "pos-bottom".
 */
const gchar *unity_position_style_class (UnityPosition position);

/**
 * unity_position_dash_halign:
 * @position: a #UnityPosition.
 *
 * Gets the horizontal alignment that opens the dash in the corner next to the
 * launcher's dash button.
 *
 * Returns: the #GtkAlign for the dash panel.
 */
GtkAlign unity_position_dash_halign (UnityPosition position);

/**
 * unity_position_dash_valign:
 * @position: a #UnityPosition.
 *
 * Gets the vertical alignment that opens the dash in the corner next to the
 * launcher's dash button.
 *
 * Returns: the #GtkAlign for the dash panel.
 */
GtkAlign unity_position_dash_valign (UnityPosition position);

/**
 * unity_tile_alignment_to_align:
 * @alignment: a #UnityTileAlignment.
 *
 * Maps a tile alignment to the #GtkAlign to set on the strip's main axis.
 *
 * Returns: the matching #GtkAlign.
 */
GtkAlign unity_tile_alignment_to_align (UnityTileAlignment alignment);

G_END_DECLS
