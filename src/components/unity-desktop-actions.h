/* unity-desktop-actions.h
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

#include <gio/gdesktopappinfo.h>
#include <gio/gio.h>

G_BEGIN_DECLS

/**
 * unity_desktop_actions_append:
 * @menu: the #GMenu to add to.
 * @info: (nullable): the #GDesktopAppInfo to read actions from.
 * @action_name: the action each item targets.
 *
 * Appends a menu section that lists the .desktop actions of @info. Each item
 * targets @action_name and uses the action id as its target value.
 */
void unity_desktop_actions_append (GMenu           *menu,
                                   GDesktopAppInfo *info,
                                   const gchar     *action_name);

G_END_DECLS
