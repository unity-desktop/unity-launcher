/* unity-app-catalog.c
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

#include "unity-app-catalog.h"

/**
 * unity_app_catalog_get_default:
 *
 * Gets the shared application catalog. One #AstalAppsApps is parsed once and
 * reused by the launcher, the dash browse grid and the search, rather than each
 * building its own copy.
 *
 * Returns: (transfer none): the shared catalog.
 */
AstalAppsApps *
unity_app_catalog_get_default (void)
{
  static AstalAppsApps *instance;
  if (g_once_init_enter_pointer (&instance))
    g_once_init_leave_pointer (&instance, astal_apps_apps_new ());
  return instance;
}
