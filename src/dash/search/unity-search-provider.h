/* unity-search-provider.h
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

#include "dash/search/unity-search-result.h"

G_BEGIN_DECLS

#define UNITY_TYPE_SEARCH_PROVIDER (unity_search_provider_get_type ())

/**
 * UnitySearchProvider:
 *
 * One installed GNOME SearchProvider2, wrapping a #GDBusProxy and the provider
 * app's #GDesktopAppInfo.
 */
G_DECLARE_FINAL_TYPE (UnitySearchProvider, unity_search_provider,
                      UNITY, SEARCH_PROVIDER, GObject)

/**
 * unity_search_provider_discover:
 *
 * Discovers the installed Version 2 search providers. It honors the desktop
 * search-providers settings. A provider in the user data dir shadows a system
 * one with the same filename.
 *
 * Returns: (transfer full) (element-type UnitySearchProvider): the providers.
 */
GList *unity_search_provider_discover (void);

/**
 * unity_search_provider_get_name:
 * @self: a #UnitySearchProvider.
 *
 * Gets the display name of the provider's app.
 *
 * Returns: (transfer none) (nullable): the name, or %NULL.
 */
const gchar *unity_search_provider_get_name  (UnitySearchProvider *self);

/**
 * unity_search_provider_get_gicon:
 * @self: a #UnitySearchProvider.
 *
 * Gets the icon of the provider's app.
 *
 * Returns: (transfer none) (nullable): the icon, or %NULL.
 */
GIcon       *unity_search_provider_get_gicon (UnitySearchProvider *self);

/**
 * unity_search_provider_query_async:
 * @self: a #UnitySearchProvider.
 * @terms: (array zero-terminated=1): the search terms.
 * @limit: the most results to return.
 * @cancellable: (nullable): a #GCancellable.
 * @callback: the callback to run when the query finishes.
 * @user_data: the data for @callback.
 *
 * Starts a query on the provider.
 */
void       unity_search_provider_query_async  (UnitySearchProvider *self,
                                                     const gchar *const       *terms,
                                                     guint                     limit,
                                                     GCancellable             *cancellable,
                                                     GAsyncReadyCallback       callback,
                                                     gpointer                  user_data);

/**
 * unity_search_provider_query_finish:
 * @self: a #UnitySearchProvider.
 * @result: the #GAsyncResult from the callback.
 * @error: (nullable): return location for an error.
 *
 * Finishes a query started with unity_search_provider_query_async().
 *
 * Returns: (transfer full) (element-type UnitySearchResult): the results.
 */
GPtrArray *unity_search_provider_query_finish (UnitySearchProvider *self,
                                                     GAsyncResult             *result,
                                                     GError                  **error);

/**
 * unity_search_provider_activate_result:
 * @self: a #UnitySearchProvider.
 * @id: the id of the result to activate.
 * @terms: (array zero-terminated=1): the search terms.
 * @timestamp: the event timestamp.
 *
 * Activates one result by its id.
 */
void       unity_search_provider_activate_result (UnitySearchProvider *self,
                                                        const gchar              *id,
                                                        const gchar *const       *terms,
                                                        guint32                   timestamp);

/**
 * unity_search_provider_launch_search:
 * @self: a #UnitySearchProvider.
 * @terms: (array zero-terminated=1): the search terms.
 * @timestamp: the event timestamp.
 *
 * Opens the provider's app on the full results for @terms.
 */
void       unity_search_provider_launch_search   (UnitySearchProvider *self,
                                                        const gchar *const       *terms,
                                                        guint32                   timestamp);

/**
 * unity_search_provider_reset:
 * @self: a #UnitySearchProvider.
 *
 * Forgets the last query, so the next query starts fresh.
 */
void       unity_search_provider_reset           (UnitySearchProvider *self);

G_END_DECLS
