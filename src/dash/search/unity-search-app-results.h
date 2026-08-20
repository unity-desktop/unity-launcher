/* unity-search-app-results.h
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

#define UNITY_TYPE_SEARCH_APP_RESULTS (unity_search_app_results_get_type ())

/**
 * UnitySearchAppResults:
 *
 * The application matches on the dash search page, shown as a single row of app
 * tiles.
 */
G_DECLARE_FINAL_TYPE (UnitySearchAppResults, unity_search_app_results,
                      UNITY, SEARCH_APP_RESULTS, AdwBin)

/**
 * unity_search_app_results_new:
 *
 * Makes a new app-results widget.
 *
 * Returns: (transfer full): a new app-results widget as a #GtkWidget.
 */
GtkWidget *unity_search_app_results_new               (void);

/**
 * unity_search_app_results_fill:
 * @self: a #UnitySearchAppResults.
 * @query: the text to search for.
 *
 * Fuzzy-queries the catalog for @query and fills the row. It hides the widget
 * when nothing matches. Every match is added, but the one-row layout shows only
 * those that fit.
 *
 * Returns: the number of matches added.
 */
guint      unity_search_app_results_fill              (UnitySearchAppResults *self,
                                                       const gchar           *query);

/**
 * unity_search_app_results_clear:
 * @self: a #UnitySearchAppResults.
 *
 * Clears the results and hides the widget.
 */
void       unity_search_app_results_clear             (UnitySearchAppResults *self);

/**
 * unity_search_app_results_activate_selected:
 * @self: a #UnitySearchAppResults.
 *
 * Launches the first match.
 */
void       unity_search_app_results_activate_selected (UnitySearchAppResults *self);

/**
 * unity_search_app_results_focus:
 * @self: a #UnitySearchAppResults.
 *
 * Moves keyboard focus into the grid.
 */
void       unity_search_app_results_focus             (UnitySearchAppResults *self);

G_END_DECLS
