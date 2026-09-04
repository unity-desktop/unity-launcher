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

#include <gio/gio.h>
#include <libdex.h>

#include "dash/search/unity-search-result.h"

G_BEGIN_DECLS

#define UNITY_TYPE_SEARCH_PROVIDER (unity_search_provider_get_type ())

/**
 * UnitySearchProvider:
 *
 * One installed GNOME SearchProvider2, wrapping the provider app's
 * #GDesktopAppInfo and issuing D-Bus calls against its well-known bus name.
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
GList       *unity_search_provider_discover        (void);

/**
 * unity_search_provider_get_name:
 * @self: a #UnitySearchProvider.
 *
 * Gets the display name of the provider's app.
 *
 * Returns: (transfer none) (nullable): the name, or %NULL.
 */
const gchar *unity_search_provider_get_name        (UnitySearchProvider *self);

/**
 * unity_search_provider_query:
 * @self: a #UnitySearchProvider.
 * @terms: (array zero-terminated=1): the search terms.
 * @limit: the most results to return.
 *
 * Starts a query on the provider. The returned future resolves to a
 * (transfer full) #GPtrArray of #UnitySearchResult, or rejects with a #GError.
 *
 * Returns: (transfer full): a #DexFuture.
 */
DexFuture   *unity_search_provider_query           (UnitySearchProvider *self,
                                                    const gchar *const  *terms,
                                                    guint                limit);

/**
 * unity_search_provider_activate_result:
 * @self: a #UnitySearchProvider.
 * @id: the id of the result to activate.
 * @terms: (array zero-terminated=1): the search terms.
 * @timestamp: the event timestamp.
 *
 * Activates one result by its id. Fire-and-forget.
 */
void         unity_search_provider_activate_result (UnitySearchProvider *self,
                                                    const gchar         *id,
                                                    const gchar *const  *terms,
                                                    guint32              timestamp);

/**
 * unity_search_provider_launch_search:
 * @self: a #UnitySearchProvider.
 * @terms: (array zero-terminated=1): the search terms.
 * @timestamp: the event timestamp.
 *
 * Opens the provider's app on the full results for @terms. Fire-and-forget.
 */
void         unity_search_provider_launch_search   (UnitySearchProvider *self,
                                                    const gchar *const  *terms,
                                                    guint32              timestamp);

/**
 * unity_search_provider_reset:
 * @self: a #UnitySearchProvider.
 *
 * Forgets the last query, so the next query starts fresh.
 */
void         unity_search_provider_reset           (UnitySearchProvider *self);

G_END_DECLS
