/* unity-app-entry.h
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

G_BEGIN_DECLS

#define UNITY_TYPE_APP_ENTRY (unity_app_entry_get_type ())

/**
 * UnityAppEntry:
 *
 * One entry in the app list, holding an app's identity, its optional #GAppInfo,
 * and a live model of its toplevel windows.
 */
G_DECLARE_FINAL_TYPE (UnityAppEntry, unity_app_entry, UNITY, APP_ENTRY, GObject)

/**
 * unity_app_entry_get_app_id:
 * @self: a #UnityAppEntry.
 *
 * Gets the app id of the entry.
 *
 * Returns: (transfer none): the app id.
 */
const gchar   *unity_app_entry_get_app_id         (UnityAppEntry *self);

/**
 * unity_app_entry_get_app_info:
 * @self: a #UnityAppEntry.
 *
 * Gets the app info of the entry, if it has one.
 *
 * Returns: (transfer none) (nullable): the #GAppInfo, or %NULL.
 */
GAppInfo      *unity_app_entry_get_app_info       (UnityAppEntry *self);

/**
 * unity_app_entry_get_application:
 * @self: a #UnityAppEntry.
 *
 * Gets the catalog application the entry stands for. An entry backed only by a
 * window has none.
 *
 * Returns: (transfer none) (nullable): the #AstalAppsApplication, or %NULL.
 */
AstalAppsApplication *unity_app_entry_get_application (UnityAppEntry *self);

/**
 * unity_app_entry_get_toplevels:
 * @self: a #UnityAppEntry.
 *
 * Gets the live model of the app's toplevel windows.
 *
 * Returns: (transfer none): the toplevels #GListModel.
 */
GListModel    *unity_app_entry_get_toplevels      (UnityAppEntry *self);

/**
 * unity_app_entry_get_pinned:
 * @self: a #UnityAppEntry.
 *
 * Gets whether the entry is pinned.
 *
 * Returns: %TRUE if the entry is pinned.
 */
gboolean       unity_app_entry_get_pinned         (UnityAppEntry *self);

/**
 * unity_app_entry_get_running:
 * @self: a #UnityAppEntry.
 *
 * Gets whether the app has any open window.
 *
 * Returns: %TRUE if the app is running.
 */
gboolean       unity_app_entry_get_running        (UnityAppEntry *self);

/**
 * unity_app_entry_get_activated:
 * @self: a #UnityAppEntry.
 *
 * Gets whether one of the app's windows has focus.
 *
 * Returns: %TRUE if the app is activated.
 */
gboolean       unity_app_entry_get_activated      (UnityAppEntry *self);

/**
 * unity_app_entry_activate_or_launch:
 * @self: a #UnityAppEntry.
 *
 * Acts on a click. If the app has no window, it launches the app. If one of the
 * app's windows has focus, it minimizes that window. If not, it raises the first
 * window.
 */
void           unity_app_entry_activate_or_launch (UnityAppEntry *self);

/**
 * unity_app_entry_close_all:
 * @self: a #UnityAppEntry.
 *
 * Closes every open window of the app.
 */
void           unity_app_entry_close_all          (UnityAppEntry *self);

/**
 * _unity_app_entry_new:
 * @app_id: the app id.
 * @app: (nullable): the catalog application this entry stands for.
 * @toplevels: a #GListModel of the app's toplevel windows.
 *
 * Makes a new entry. UnityAppList calls this. Consumers do not.
 *
 * Returns: (transfer full): a new #UnityAppEntry.
 */
UnityAppEntry *_unity_app_entry_new               (const gchar *app_id,
                                                   AstalAppsApplication *app,
                                                   GListModel  *toplevels);

/**
 * _unity_app_entry_recompute:
 * @self: a #UnityAppEntry.
 *
 * Re-derives the running and activated flags from the toplevels list. Called by
 * UnityAppList when a toplevel property that affects them changes.
 */
void           _unity_app_entry_recompute         (UnityAppEntry *self);

/**
 * _unity_app_entry_set_pinned:
 * @self: a #UnityAppEntry.
 * @pinned: the new pinned state.
 *
 * Sets the pinned state of the entry. UnityAppList calls this.
 */
void           _unity_app_entry_set_pinned        (UnityAppEntry *self,
                                                   gboolean       pinned);

G_END_DECLS
