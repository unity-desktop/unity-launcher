/* unity-search-result-row.h
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

#include "dash/search/unity-search-result.h"

G_BEGIN_DECLS

#define UNITY_TYPE_SEARCH_RESULT_ROW (unity_search_result_row_get_type ())

/**
 * UnitySearchResultRow:
 *
 * One provider search result shown as an activatable #AdwActionRow.
 */
G_DECLARE_FINAL_TYPE (UnitySearchResultRow, unity_search_result_row,
                      UNITY, SEARCH_RESULT_ROW, AdwActionRow)

/**
 * unity_search_result_row_new:
 * @result: the #UnitySearchResult the row shows.
 *
 * Makes a new result row for @result.
 *
 * Returns: (transfer full): a new result row as a #GtkWidget.
 */
GtkWidget *unity_search_result_row_new (UnitySearchResult *result);

G_END_DECLS
