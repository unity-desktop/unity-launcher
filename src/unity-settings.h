/* unity-settings.h
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

#include <gio/gio.h>

G_BEGIN_DECLS

/* The keys of the "org.unity.launcher" schema. */
#define UNITY_LAUNCHER_KEY_POSITION            "position"
#define UNITY_LAUNCHER_KEY_HIDE                "hide"
#define UNITY_LAUNCHER_KEY_TILE_ALIGNMENT      "tile-alignment"
#define UNITY_LAUNCHER_KEY_PINNED_APPS         "pinned-apps"
#define UNITY_LAUNCHER_KEY_LAUNCHER_ICON_SIZE  "launcher-icon-size"
#define UNITY_LAUNCHER_KEY_DASH_ICON_SIZE      "dash-icon-size"
#define UNITY_LAUNCHER_KEY_SHOW_FREQUENT_APPS  "show-frequent-apps"

/**
 * unity_settings_get_default:
 *
 * Gets the shared #GSettings for "org.unity.launcher". It is made the first time
 * it is asked for, then reused.
 *
 * Returns: (transfer none): the shared #GSettings.
 */
GSettings *unity_settings_get_default (void);

G_END_DECLS
