/* unity-search.h
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

#include "dash/search/unity-search-provider.h"

G_BEGIN_DECLS

#define UNITY_TYPE_SEARCH (unity_search_get_type ())

/**
 * UnitySearch:
 *
 * Runs a query across all installed providers and reports each provider's
 * results through the UnitySearch::provider-results signal.
 */
G_DECLARE_FINAL_TYPE (UnitySearch, unity_search, UNITY, SEARCH, GObject)

/**
 * unity_search_get_default:
 *
 * Gets the shared search object. It is made the first time it is asked for, then
 * reused.
 *
 * Returns: (transfer none): the shared #UnitySearch.
 */
UnitySearch *unity_search_get_default (void);

/**
 * unity_search_query:
 * @self: a #UnitySearch.
 * @query: the text to search for.
 *
 * Runs @query on every provider. Results come back through the
 * UnitySearch::provider-results signal, once for each provider that answers. An
 * empty or %NULL query cancels the current search and sends nothing.
 */
void unity_search_query (UnitySearch *self, const gchar *query);

G_END_DECLS
