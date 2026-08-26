/* unity-launcher-dash-button.h
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

#define UNITY_TYPE_LAUNCHER_DASH_BUTTON (unity_launcher_dash_button_get_type ())

/**
 * UnityLauncherDashButton:
 *
 * The launcher's Show-Applications button: a symbolic that toggles the dash and
 * sizes to the launcher icon size.
 */
G_DECLARE_FINAL_TYPE (UnityLauncherDashButton, unity_launcher_dash_button,
                      UNITY, LAUNCHER_DASH_BUTTON, GtkButton)

G_END_DECLS
