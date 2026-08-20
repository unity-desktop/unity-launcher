/* unity-pinned-apps.c
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

#include "components/unity-pinned-apps.h"

void
unity_pinned_apps_toggle (GSettings *settings, const gchar *app_id)
{
  if (app_id == NULL || *app_id == '\0')
    return;

  g_auto (GStrv)        ids  = g_settings_get_strv (settings, UNITY_LAUNCHER_KEY_PINNED_APPS);
  g_autoptr (GPtrArray) next = g_ptr_array_new_with_free_func (g_free);

  gboolean was_pinned = FALSE;
  for (gchar **p = ids; p && *p; p++)
    {
      if (g_strcmp0 (*p, app_id) == 0) { was_pinned = TRUE; continue; }
      g_ptr_array_add (next, g_strdup (*p));
    }
  if (!was_pinned)
    g_ptr_array_add (next, g_strdup (app_id));
  g_ptr_array_add (next, NULL);

  g_settings_set_strv (settings, UNITY_LAUNCHER_KEY_PINNED_APPS,
                       (const gchar *const *) next->pdata);
}

gboolean
unity_pinned_apps_contains (GSettings *settings, const gchar *app_id)
{
  if (app_id == NULL || *app_id == '\0')
    return FALSE;

  g_auto (GStrv) ids = g_settings_get_strv (settings, UNITY_LAUNCHER_KEY_PINNED_APPS);
  for (gchar **p = ids; p && *p; p++)
    if (g_strcmp0 (*p, app_id) == 0)
      return TRUE;
  return FALSE;
}
