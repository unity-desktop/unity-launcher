/* unity-pinned-apps.h
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

#include <gio/gio.h>

G_BEGIN_DECLS

/**
 * unity_pinned_apps_toggle:
 * @settings: the launcher #GSettings.
 * @app_id: the app id to toggle.
 *
 * Toggles @app_id in the pinned-apps list. If the id is present, it removes it.
 * If the id is absent, it appends it.
 */
void      unity_pinned_apps_toggle   (GSettings *settings, const gchar *app_id);

/**
 * unity_pinned_apps_insert:
 * @settings: the launcher #GSettings.
 * @app_id: the app id to place.
 * @index: the position to place it at, clamped to the list without @app_id.
 *
 * Moves @app_id to @index in the pinned-apps list. An id that was not pinned
 * gets pinned there, so a drag out of the running tiles is a pin.
 */
void      unity_pinned_apps_insert   (GSettings *settings, const gchar *app_id, gint index);

/**
 * unity_pinned_apps_contains:
 * @settings: the launcher #GSettings.
 * @app_id: the app id to look for.
 *
 * Checks whether @app_id is in the pinned-apps list.
 *
 * Returns: %TRUE if the app is pinned.
 */
gboolean  unity_pinned_apps_contains (GSettings *settings, const gchar *app_id);

G_END_DECLS
