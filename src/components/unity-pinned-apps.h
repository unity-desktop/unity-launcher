#pragma once

#include <gio/gio.h>

G_BEGIN_DECLS

#define UNITY_LAUNCHER_KEY_PINNED_APPS "pinned-apps"

void     unity_pinned_apps_toggle   (GSettings *settings, const gchar *app_id);

gboolean unity_pinned_apps_contains (GSettings *settings, const gchar *app_id);

G_END_DECLS
