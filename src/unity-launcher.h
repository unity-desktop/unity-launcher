#pragma once

#include <astal-4.h>

/* The launcher's action namespace: the launcher installs this group under the
 * bare names, and its tiles target the prefixed forms. Both are built from the
 * same NAME macros, so a rename touches one line and the two cannot drift. */
#define UNITY_LAUNCHER_ACTION_GROUP             "launcher"

#define UNITY_LAUNCHER_ACTION_NAME_PIN_TOGGLE   "pin-toggle"
#define UNITY_LAUNCHER_ACTION_NAME_QUIT         "quit"
#define UNITY_LAUNCHER_ACTION_NAME_REORDER      "reorder-pinned"

#define UNITY_LAUNCHER_ACTION_PIN_TOGGLE \
  UNITY_LAUNCHER_ACTION_GROUP "." UNITY_LAUNCHER_ACTION_NAME_PIN_TOGGLE
#define UNITY_LAUNCHER_ACTION_QUIT \
  UNITY_LAUNCHER_ACTION_GROUP "." UNITY_LAUNCHER_ACTION_NAME_QUIT
#define UNITY_LAUNCHER_ACTION_REORDER \
  UNITY_LAUNCHER_ACTION_GROUP "." UNITY_LAUNCHER_ACTION_NAME_REORDER

G_BEGIN_DECLS

#define UNITY_TYPE_LAUNCHER (unity_launcher_get_type ())

G_DECLARE_FINAL_TYPE (UnityLauncher, unity_launcher,
                      UNITY, LAUNCHER, AstalWindow)

UnityLauncher *unity_launcher_new (GtkApplication *app);

G_END_DECLS
