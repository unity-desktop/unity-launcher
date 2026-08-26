/* unity-dash.h
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

#include <astal-4.h>

G_BEGIN_DECLS

#define UNITY_TYPE_DASH (unity_dash_get_type ())

/* Marker set on a UnityDash toplevel so other components can find it without
 * including this header. Its value has meaning only through pointer equality
 * with g_intern_static_string ("dash"). */
#define UNITY_DASH_WINDOW_ROLE_KEY  "unity-dash-role"
#define UNITY_DASH_WINDOW_ROLE      "dash"

/**
 * UnityDash:
 *
 * An application grid window, shown as a layer-shell overlay that fills the work
 * area so a click outside the panel dismisses it.
 */
G_DECLARE_FINAL_TYPE (UnityDash, unity_dash, UNITY, DASH, AstalWindow)

/**
 * unity_dash_new:
 * @app: the #GtkApplication the window belongs to.
 *
 * Makes a new dash window.
 *
 * Returns: (transfer full): a new dash as a #GtkWidget.
 */
GtkWidget *unity_dash_new    (GtkApplication *app);

/**
 * unity_dash_reset:
 * @self: a #UnityDash.
 *
 * Resets the dash for a fresh open. It applies the saved mode, clears the query
 * back to the apps page, and focuses the search entry.
 */
void       unity_dash_reset  (UnityDash *self);

/**
 * unity_dash_close:
 * @self: a #UnityDash.
 *
 * Hides the dash and discards its state, so the next open starts fresh.
 */
void       unity_dash_close  (UnityDash *self);

/**
 * unity_dash_toggle:
 * @self: a #UnityDash.
 *
 * Toggles the dash. A visible dash hides and keeps its state. A hidden dash
 * shows the state it holds.
 */
void       unity_dash_toggle (UnityDash *self);

G_END_DECLS
