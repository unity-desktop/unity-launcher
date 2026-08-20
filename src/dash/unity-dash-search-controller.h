/* unity-dash-search-controller.h
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

#include "dash/unity-dash-search.h"

G_BEGIN_DECLS

#define UNITY_TYPE_DASH_SEARCH_CONTROLLER (unity_dash_search_controller_get_type ())

/**
 * UnityDashSearchController:
 *
 * Drives the dash search interaction, turning entry input into queries and
 * routing keys between the entry and the results.
 */
G_DECLARE_FINAL_TYPE (UnityDashSearchController, unity_dash_search_controller,
                      UNITY, DASH_SEARCH_CONTROLLER, GObject)

/**
 * unity_dash_search_controller_new:
 * @entry: the search entry.
 * @stack: the view stack that holds the apps and search pages.
 * @search_page: the search page.
 *
 * Makes a new search controller. The widgets are borrowed and stay owned by the
 * caller.
 *
 * Returns: (transfer full): a new #UnityDashSearchController.
 */
UnityDashSearchController *unity_dash_search_controller_new (GtkSearchEntry  *entry,
                                                            AdwViewStack    *stack,
                                                            UnityDashSearch *search_page);

/**
 * unity_dash_search_controller_reset:
 * @self: a #UnityDashSearchController.
 *
 * Returns the search to its resting state. It cancels any pending query, clears
 * the results and the entry, and shows the apps page.
 */
void unity_dash_search_controller_reset (UnityDashSearchController *self);

G_END_DECLS
