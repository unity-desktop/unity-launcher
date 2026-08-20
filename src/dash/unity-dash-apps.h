/* unity-dash-apps.h
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

#include <adwaita.h>

G_BEGIN_DECLS

#define UNITY_TYPE_DASH_APPS (unity_dash_apps_get_type ())

/**
 * UnityDashApps:
 *
 * The dash browse page that shows every installed application in a square grid.
 */
G_DECLARE_FINAL_TYPE (UnityDashApps, unity_dash_apps,
                      UNITY, DASH_APPS, AdwBin)

/**
 * unity_dash_apps_new:
 *
 * Makes a new dash browse page.
 *
 * Returns: (transfer full): a new browse page as a #GtkWidget.
 */
GtkWidget *unity_dash_apps_new (void);

/**
 * unity_dash_apps_reset:
 * @self: a #UnityDashApps.
 *
 * Scrolls the grid back to the top for a fresh open.
 */
void unity_dash_apps_reset (UnityDashApps *self);

G_END_DECLS
