/* unity-desktop-actions.c
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

#include "components/unity-desktop-actions.h"

void
unity_desktop_actions_append (GMenu           *menu,
                              GDesktopAppInfo *info,
                              const gchar     *action_name)
{
  if (info == NULL)
    return;

  const gchar *const *actions = g_desktop_app_info_list_actions (info);
  if (actions == NULL || actions[0] == NULL)
    return;

  g_autoptr (GMenu) section = g_menu_new ();
  for (guint i = 0; actions[i] != NULL; i++)
    {
      g_autofree gchar     *name = g_desktop_app_info_get_action_name (info, actions[i]);
      g_autoptr (GMenuItem) item = g_menu_item_new (name, NULL);
      g_menu_item_set_action_and_target_value (item, action_name,
                                               g_variant_new_string (actions[i]));
      g_menu_append_item (section, item);
    }
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
}
