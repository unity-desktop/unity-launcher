/* unity-position.c
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

#include "unity-position.h"

AstalWindowAnchor
unity_position_anchor (UnityPosition position)
{
  switch (position)
    {
    case UNITY_POSITION_RIGHT:
      return ASTAL_WINDOW_ANCHOR_TOP | ASTAL_WINDOW_ANCHOR_RIGHT | ASTAL_WINDOW_ANCHOR_BOTTOM;
    case UNITY_POSITION_BOTTOM:
      return ASTAL_WINDOW_ANCHOR_LEFT | ASTAL_WINDOW_ANCHOR_RIGHT | ASTAL_WINDOW_ANCHOR_BOTTOM;
    case UNITY_POSITION_LEFT:
    default:
      return ASTAL_WINDOW_ANCHOR_TOP | ASTAL_WINDOW_ANCHOR_LEFT | ASTAL_WINDOW_ANCHOR_BOTTOM;
    }
}

GtkOrientation
unity_position_orientation (UnityPosition position)
{
  return position == UNITY_POSITION_BOTTOM ? GTK_ORIENTATION_HORIZONTAL
                                           : GTK_ORIENTATION_VERTICAL;
}

gboolean
unity_position_is_horizontal (UnityPosition position)
{
  return unity_position_orientation (position) == GTK_ORIENTATION_HORIZONTAL;
}

const gchar *
unity_position_edge_margin (UnityPosition position)
{
  switch (position)
    {
    case UNITY_POSITION_RIGHT:  return "margin-right";
    case UNITY_POSITION_BOTTOM: return "margin-bottom";
    case UNITY_POSITION_LEFT:
    default:                    return "margin-left";
    }
}

const gchar *
unity_position_style_class (UnityPosition position)
{
  switch (position)
    {
    case UNITY_POSITION_RIGHT:  return "pos-right";
    case UNITY_POSITION_BOTTOM: return "pos-bottom";
    case UNITY_POSITION_LEFT:
    default:                    return "pos-left";
    }
}

GtkAlign
unity_position_dash_halign (UnityPosition position)
{
  return position == UNITY_POSITION_RIGHT ? GTK_ALIGN_END : GTK_ALIGN_START;
}

GtkAlign
unity_position_dash_valign (UnityPosition position)
{
  return position == UNITY_POSITION_BOTTOM ? GTK_ALIGN_END : GTK_ALIGN_START;
}

GtkAlign
unity_tile_alignment_to_align (UnityTileAlignment alignment)
{
  switch (alignment)
    {
    case UNITY_TILE_ALIGNMENT_CENTER: return GTK_ALIGN_CENTER;
    case UNITY_TILE_ALIGNMENT_END:    return GTK_ALIGN_END;
    case UNITY_TILE_ALIGNMENT_START:
    default:                          return GTK_ALIGN_START;
    }
}
