/* unity-tile.h
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

#define UNITY_TYPE_TILE (unity_tile_get_type ())

/**
 * UnityTile:
 *
 * Abstract base for a launcher tile or a dash tile, drawn as a flat button with
 * a centered icon and a secondary-click menu.
 */
G_DECLARE_DERIVABLE_TYPE (UnityTile, unity_tile, UNITY, TILE, GtkButton)

/**
 * UnityTileClass:
 * @parent_class: the parent class.
 * @populate_menu: fills @menu for the tile's secondary-click menu. A subclass
 *   sets this to add its own menu items. The menu shows only if it has items.
 */
struct _UnityTileClass
{
  GtkButtonClass parent_class;

  void (*populate_menu) (UnityTile *self, GMenu *menu);
};

/**
 * unity_tile_get_box:
 * @self: a #UnityTile.
 *
 * Gets the content box that holds the icon. Subclasses append their widgets to
 * it.
 *
 * Returns: (transfer none): the content #GtkBox.
 */
GtkBox   *unity_tile_get_box          (UnityTile *self);

/**
 * unity_tile_get_icon_size:
 * @self: a #UnityTile.
 *
 * Gets the icon size in pixels.
 *
 * Returns: the icon size.
 */
gint      unity_tile_get_icon_size    (UnityTile *self);

/**
 * unity_tile_set_gicon:
 * @self: a #UnityTile.
 * @icon: (nullable): the icon to show, or %NULL.
 *
 * Sets the tile icon. A %NULL icon falls back to a generic executable icon.
 */
void      unity_tile_set_gicon        (UnityTile *self, GIcon *icon);

/**
 * unity_tile_set_running:
 * @self: a #UnityTile.
 * @running: the new running state.
 *
 * Sets whether the tile shows the running look.
 */
void      unity_tile_set_running      (UnityTile *self, gboolean running);

/**
 * unity_tile_set_active:
 * @self: a #UnityTile.
 * @active: the new active state.
 *
 * Sets whether the tile shows the active look.
 */
void      unity_tile_set_active       (UnityTile *self, gboolean active);

/**
 * unity_tile_set_menu_position:
 * @self: a #UnityTile.
 * @position: where the menu opens.
 *
 * Sets the side the secondary-click menu opens on.
 */
void      unity_tile_set_menu_position (UnityTile *self, GtkPositionType position);

G_END_DECLS
