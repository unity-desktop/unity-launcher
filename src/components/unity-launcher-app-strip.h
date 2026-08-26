/* unity-launcher-app-strip.h
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

/* Action installed on the strip; the reorder helper activates it via widget
 * action lookup. */
#define UNITY_LAUNCHER_ACTION_REORDER  "launcher.reorder-pinned"

#define UNITY_TYPE_LAUNCHER_APP_STRIP (unity_launcher_app_strip_get_type ())

/**
 * UnityLauncherAppStrip:
 *
 * The launcher's scrolling run of app tiles. It owns the app model, the tiles,
 * the pinned-apps reorder drag, and the `launcher` action group the tiles target.
 *
 * The widget is a #GtkOrientable: set the orientation to the launcher axis and
 * it points the scroller and the tile alignment to match.
 */
G_DECLARE_FINAL_TYPE (UnityLauncherAppStrip, unity_launcher_app_strip,
                      UNITY, LAUNCHER_APP_STRIP, GtkWidget)

/**
 * unity_launcher_app_strip_get_dragging:
 * @self: a #UnityLauncherAppStrip.
 *
 * Gets whether a tile drag is in progress.
 *
 * Returns: %TRUE while a tile is dragged.
 */
gboolean   unity_launcher_app_strip_get_dragging   (UnityLauncherAppStrip *self);

/**
 * unity_launcher_app_strip_get_menu_shown:
 * @self: a #UnityLauncherAppStrip.
 *
 * Gets whether a tile context menu is open.
 *
 * Returns: %TRUE while a tile menu is open.
 */
gboolean   unity_launcher_app_strip_get_menu_shown (UnityLauncherAppStrip *self);

G_END_DECLS
