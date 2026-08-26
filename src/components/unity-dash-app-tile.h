/* unity-dash-app-tile.h
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

#include "components/unity-app-tile.h"

G_BEGIN_DECLS

#define UNITY_TYPE_DASH_APP_TILE (unity_dash_app_tile_get_type ())

/**
 * UnityDashAppTile:
 *
 * A dash cell that shows the icon and label for one #AstalAppsApplication.
 */
G_DECLARE_FINAL_TYPE (UnityDashAppTile, unity_dash_app_tile,
                      UNITY, DASH_APP_TILE, UnityAppTile)

/**
 * unity_dash_app_tile_new:
 * @app: the #AstalAppsApplication the tile shows.
 *
 * Makes a new dash tile for @app.
 *
 * Returns: (transfer full): a new dash tile as a #GtkWidget.
 */
GtkWidget *unity_dash_app_tile_new (AstalAppsApplication *app);

G_END_DECLS
