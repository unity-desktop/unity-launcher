/* unity-launcher-app-tile.h
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

#include "components/unity-app-tile.h"
#include "unity-app-entry.h"

G_BEGIN_DECLS

#define UNITY_TYPE_LAUNCHER_APP_TILE (unity_launcher_app_tile_get_type ())

/**
 * UnityLauncherAppTile:
 *
 * A launcher dock tile bound to a #UnityAppEntry that shows the app icon and its
 * running state.
 */
G_DECLARE_FINAL_TYPE (UnityLauncherAppTile, unity_launcher_app_tile,
                      UNITY, LAUNCHER_APP_TILE, UnityAppTile)

/**
 * unity_launcher_app_tile_new:
 * @entry: the #UnityAppEntry the tile shows.
 *
 * Makes a new launcher tile for @entry.
 *
 * Returns: (transfer full): a new launcher tile as a #GtkWidget.
 */
GtkWidget   *unity_launcher_app_tile_new          (UnityAppEntry *entry);

/**
 * unity_launcher_app_tile_get_app_id:
 * @self: a #UnityLauncherAppTile.
 *
 * Gets the app id of the tile's entry.
 *
 * Returns: (transfer none) (nullable): the app id, or %NULL.
 */
const gchar *unity_launcher_app_tile_get_app_id   (UnityLauncherAppTile *self);

/**
 * unity_launcher_app_tile_get_pinned:
 * @self: a #UnityLauncherAppTile.
 *
 * Gets whether the tile's entry is pinned.
 *
 * Returns: %TRUE if the entry is pinned.
 */
gboolean     unity_launcher_app_tile_get_pinned   (UnityLauncherAppTile *self);

/**
 * unity_launcher_app_tile_get_dragging:
 * @self: a #UnityLauncherAppTile.
 *
 * Gets whether the tile is currently being dragged.
 *
 * Returns: %TRUE while a drag of this tile is in progress.
 */
gboolean     unity_launcher_app_tile_get_dragging (UnityLauncherAppTile *self);

G_END_DECLS
