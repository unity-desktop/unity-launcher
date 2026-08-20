/* unity-dash-search.h
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

#define UNITY_TYPE_DASH_SEARCH (unity_dash_search_get_type ())

/**
 * UnityDashSearch:
 *
 * The dash search page that shows a matching-apps group and a group per search
 * provider.
 */
G_DECLARE_FINAL_TYPE (UnityDashSearch, unity_dash_search,
                      UNITY, DASH_SEARCH, AdwBin)

/**
 * unity_dash_search_new:
 *
 * Makes a new dash search page.
 *
 * Returns: (transfer full): a new search page as a #GtkWidget.
 */
GtkWidget *unity_dash_search_new (void);

/**
 * unity_dash_search_run:
 * @self: a #UnityDashSearch.
 * @query: the text to search for.
 *
 * Runs a search for @query. It fills matching apps at once and queries the
 * providers in the background.
 */
void unity_dash_search_run (UnityDashSearch *self, const gchar *query);

/**
 * unity_dash_search_activate_selected:
 * @self: a #UnityDashSearch.
 *
 * Launches the highlighted default match.
 */
void unity_dash_search_activate_selected (UnityDashSearch *self);

/**
 * unity_dash_search_focus_results:
 * @self: a #UnityDashSearch.
 *
 * Moves keyboard focus into the results.
 */
void unity_dash_search_focus_results (UnityDashSearch *self);

/**
 * unity_dash_search_reset:
 * @self: a #UnityDashSearch.
 *
 * Clears the search and all its results.
 */
void unity_dash_search_reset (UnityDashSearch *self);

G_END_DECLS
