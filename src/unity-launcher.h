/* unity-launcher.h
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

#define UNITY_TYPE_LAUNCHER (unity_launcher_get_type ())

/**
 * UnityLauncher:
 *
 * A layer-shell window that holds the dash button and the app strip. The
 * "position" setting anchors it to the left, right, or bottom edge, and the
 * strip runs along that edge.
 */
G_DECLARE_FINAL_TYPE (UnityLauncher, unity_launcher,
                      UNITY, LAUNCHER, AstalWindow)

/**
 * unity_launcher_new:
 * @app: the #GtkApplication the window belongs to.
 *
 * Makes a new launcher window. This also registers the launcher stylesheet,
 * which styles the application grid too.
 *
 * Returns: (transfer full): a new #UnityLauncher.
 */
UnityLauncher *unity_launcher_new (GtkApplication *app);

G_END_DECLS
