/* unity-app-tile.h
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

#include <astal-apps.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define UNITY_TYPE_APP_TILE (unity_app_tile_get_type ())

/**
 * UnityAppTile:
 *
 * Abstract base for a launcher tile or a dash tile: a flat button showing one
 * #AstalAppsApplication as a centered icon, with a secondary-click menu.
 *
 * The base takes the icon from the application, installs the "tile" action group
 * with `launch`, `launch-action` and `pin-toggle`, and fills the menu with
 * "Open", the desktop file's own actions and the pin item. A subclass adds
 * what is particular to it.
 */
G_DECLARE_DERIVABLE_TYPE (UnityAppTile, unity_app_tile, UNITY, APP_TILE, GtkButton)

/**
 * UnityAppTileClass:
 * @parent_class: the parent class.
 * @populate_menu: adds the subclass's items to @menu for the secondary-click
 *   menu. The base has already put "Open" and the desktop file's own actions
 *   there. The menu shows only if it has items.
 */
struct _UnityAppTileClass
{
  GtkButtonClass  parent_class;

  void (*populate_menu) (UnityAppTile *self, GMenu *menu);
};

/**
 * unity_app_tile_get_box:
 * @self: a #UnityAppTile.
 *
 * Gets the content box that holds the icon. Subclasses append their widgets to
 * it.
 *
 * Returns: (transfer none): the content #GtkBox.
 */
GtkBox   *unity_app_tile_get_box           (UnityAppTile *self);

/**
 * unity_app_tile_get_icon_size:
 * @self: a #UnityAppTile.
 *
 * Gets the icon size in pixels.
 *
 * Returns: the icon size.
 */
gint      unity_app_tile_get_icon_size     (UnityAppTile *self);

/**
 * unity_app_tile_set_application:
 * @self: a #UnityAppTile.
 * @app: (nullable): the application the tile stands for.
 *
 * Sets the tile's application. The tile takes its icon from it, launches it for
 * the "tile" action group, and lists its desktop actions in the menu.
 */
void      unity_app_tile_set_application   (UnityAppTile *self, AstalAppsApplication *app);

/**
 * unity_app_tile_get_application:
 * @self: a #UnityAppTile.
 *
 * Gets the application the tile stands for.
 *
 * Returns: (transfer none) (nullable): the #AstalAppsApplication, or %NULL.
 */
AstalAppsApplication *unity_app_tile_get_application (UnityAppTile *self);

/**
 * unity_app_tile_launch:
 * @self: a #UnityAppTile.
 *
 * Launches the tile's application. astal counts the launch, which is what
 * the dash's frequent row reads.
 */
void      unity_app_tile_launch            (UnityAppTile *self);

/**
 * unity_app_tile_set_menu_position:
 * @self: a #UnityAppTile.
 * @position: where the menu opens.
 *
 * Sets the side the secondary-click menu opens on.
 */
void      unity_app_tile_set_menu_position (UnityAppTile *self, GtkPositionType position);

/**
 * unity_app_tile_get_menu_shown:
 * @self: a #UnityAppTile.
 *
 * Gets whether the tile's secondary-click menu is currently open.
 *
 * Returns: %TRUE while the menu is open.
 */
gboolean  unity_app_tile_get_menu_shown    (UnityAppTile *self);

G_END_DECLS
