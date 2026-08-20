/* unity-search-result.h
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

G_BEGIN_DECLS

#define UNITY_TYPE_SEARCH_RESULT (unity_search_result_get_type ())

/**
 * UnitySearchResult:
 *
 * One result from a search provider, holding the data to show it and a reference
 * to the provider that made it.
 */
G_DECLARE_FINAL_TYPE (UnitySearchResult, unity_search_result,
                      UNITY, SEARCH_RESULT, GObject)

typedef struct _UnitySearchProvider UnitySearchProvider;

/**
 * unity_search_result_new:
 * @provider: (nullable): the provider that made the result.
 * @id: the result id.
 * @name: the result name.
 * @description: (nullable): the result description.
 * @clipboard_text: (nullable): the text to copy for the result.
 * @gicon: (nullable): the result icon.
 * @terms: (array zero-terminated=1): the query terms.
 *
 * Makes a new search result.
 *
 * Returns: (transfer full): a new #UnitySearchResult.
 */
UnitySearchResult *unity_search_result_new (UnitySearchProvider *provider,
                                                       const gchar              *id,
                                                       const gchar              *name,
                                                       const gchar              *description,
                                                       const gchar              *clipboard_text,
                                                       GIcon                    *gicon,
                                                       const gchar *const       *terms);

/**
 * unity_search_result_get_id:
 * @self: a #UnitySearchResult.
 *
 * Returns: (transfer none): the result id.
 */
const gchar *unity_search_result_get_id             (UnitySearchResult *self);

/**
 * unity_search_result_get_name:
 * @self: a #UnitySearchResult.
 *
 * Returns: (transfer none): the result name.
 */
const gchar *unity_search_result_get_name           (UnitySearchResult *self);

/**
 * unity_search_result_get_description:
 * @self: a #UnitySearchResult.
 *
 * Returns: (transfer none) (nullable): the description, or %NULL.
 */
const gchar *unity_search_result_get_description     (UnitySearchResult *self);

/**
 * unity_search_result_get_clipboard_text:
 * @self: a #UnitySearchResult.
 *
 * Returns: (transfer none) (nullable): the text to copy, or %NULL.
 */
const gchar *unity_search_result_get_clipboard_text  (UnitySearchResult *self);

/**
 * unity_search_result_get_gicon:
 * @self: a #UnitySearchResult.
 *
 * Returns: (transfer none) (nullable): the icon, or %NULL.
 */
GIcon       *unity_search_result_get_gicon           (UnitySearchResult *self);

/**
 * unity_search_result_get_terms:
 * @self: a #UnitySearchResult.
 *
 * Returns: (transfer none) (array zero-terminated=1): the query terms.
 */
const gchar *const *unity_search_result_get_terms    (UnitySearchResult *self);

/**
 * unity_search_result_activate:
 * @self: a #UnitySearchResult.
 * @timestamp: the event timestamp.
 *
 * Activates the result through its provider.
 */
void         unity_search_result_activate         (UnitySearchResult *self,
                                                         guint32                 timestamp);

G_END_DECLS
