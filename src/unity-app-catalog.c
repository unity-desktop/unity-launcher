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

#include <gio/gdesktopappinfo.h>

AstalAppsApps *
unity_app_catalog_get_default (void)
{
  static AstalAppsApps *instance;
  if (g_once_init_enter_pointer (&instance))
    {
      AstalAppsApps *catalog = astal_apps_apps_new ();
      g_object_set (catalog,
                    "min-score",        50.0,
                    "entry-multiplier", 1.0,
                    NULL);
      g_once_init_leave_pointer (&instance, catalog);
    }
  return instance;
}

void
unity_app_catalog_launch (AstalAppsApplication *app)
{
  if (app == NULL)
    return;

  GDesktopAppInfo *info = astal_apps_application_get_app (app);
  if (info == NULL)
    return;

  GdkDisplay *display = gdk_display_get_default ();
  g_autoptr (GdkAppLaunchContext) ctx = display ? gdk_display_get_app_launch_context (display)
                                                : NULL;
  if (ctx != NULL)
    gdk_app_launch_context_set_timestamp (ctx, GDK_CURRENT_TIME);

  g_autoptr (GError) error = NULL;
  if (g_app_info_launch (G_APP_INFO (info), NULL,
                         ctx ? G_APP_LAUNCH_CONTEXT (ctx) : NULL, &error))
    astal_apps_application_set_frequency (app,
                                          astal_apps_application_get_frequency (app) + 1);
  else
    g_warning ("UnityAppCatalog: launch failed for %s: %s",
               astal_apps_application_get_entry (app) ?: "(null)",
               error ? error->message : "unknown error");
}
