/* unity-search-result-rows.h
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

#include "dash/search/unity-search-provider.h"
#include "dash/search/unity-search-result.h"

G_BEGIN_DECLS

#define UNITY_TYPE_SEARCH_RESULT_ROWS (unity_search_result_rows_get_type ())

/**
 * UnitySearchResultRows:
 *
 * A provider's search results shown as a title over a grid of activatable link
 * rows.
 */
G_DECLARE_FINAL_TYPE (UnitySearchResultRows, unity_search_result_rows,
                      UNITY, SEARCH_RESULT_ROWS, GtkBox)

/**
 * unity_search_result_rows_new:
 * @provider: the provider that made the results.
 * @results: (element-type UnitySearchResult): the results to show.
 *
 * Makes a new result-rows widget for @provider.
 *
 * Returns: (transfer full): a new result-rows widget as a #GtkWidget.
 */
GtkWidget *unity_search_result_rows_new (UnitySearchProvider *provider,
                                         GPtrArray           *results);

G_END_DECLS
