/* unity-app-list.h
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

#include "unity-app-entry.h"

G_BEGIN_DECLS

#define UNITY_TYPE_APP_LIST (unity_app_list_get_type ())

/**
 * UnityAppList:
 *
 * A #GListModel of #UnityAppEntry that merges pinned apps and running windows
 * into one deduplicated list.
 */
G_DECLARE_FINAL_TYPE (UnityAppList, unity_app_list, UNITY, APP_LIST, GObject)

/**
 * unity_app_list_new:
 *
 * Makes a new app list.
 *
 * Returns: (transfer full): a new #UnityAppList.
 */
UnityAppList  *unity_app_list_new                (void);

/**
 * unity_app_list_set_pinned_app_ids:
 * @self: a #UnityAppList.
 * @app_ids: (nullable) (array zero-terminated=1): the pinned app ids, in order.
 *
 * Sets the pinned app ids and rebuilds the list.
 */
void           unity_app_list_set_pinned_app_ids (UnityAppList       *self,
                                                  const gchar *const *app_ids);

/**
 * unity_app_list_get_entry:
 * @self: a #UnityAppList.
 * @app_id: the app id to look up.
 *
 * Finds the entry for @app_id. The lookup uses the canonical id, so a raw id
 * works too.
 *
 * Returns: (transfer none) (nullable): the #UnityAppEntry, or %NULL.
 */
UnityAppEntry *unity_app_list_get_entry          (UnityAppList *self,
                                                  const gchar  *app_id);

G_END_DECLS
