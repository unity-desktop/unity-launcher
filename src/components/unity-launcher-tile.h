/* unity-launcher-tile.h
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

#include "components/unity-tile.h"
#include "unity-app-entry.h"

G_BEGIN_DECLS

#define UNITY_TYPE_LAUNCHER_TILE (unity_launcher_tile_get_type ())

/**
 * UnityLauncherTile:
 *
 * A launcher dock tile bound to a #UnityAppEntry that shows the app icon and its
 * running state.
 */
G_DECLARE_FINAL_TYPE (UnityLauncherTile, unity_launcher_tile,
                      UNITY, LAUNCHER_TILE, UnityTile)

/**
 * unity_launcher_tile_new:
 * @entry: the #UnityAppEntry the tile shows.
 *
 * Makes a new launcher tile for @entry.
 *
 * Returns: (transfer full): a new launcher tile as a #GtkWidget.
 */
GtkWidget   *unity_launcher_tile_new        (UnityAppEntry *entry);

/**
 * unity_launcher_tile_get_app_id:
 * @self: a #UnityLauncherTile.
 *
 * Gets the app id of the tile's entry.
 *
 * Returns: (transfer none) (nullable): the app id, or %NULL.
 */
const gchar *unity_launcher_tile_get_app_id (UnityLauncherTile *self);

/**
 * unity_launcher_tile_get_pinned:
 * @self: a #UnityLauncherTile.
 *
 * Gets whether the tile's entry is pinned.
 *
 * Returns: %TRUE if the entry is pinned.
 */
gboolean     unity_launcher_tile_get_pinned (UnityLauncherTile *self);

G_END_DECLS
