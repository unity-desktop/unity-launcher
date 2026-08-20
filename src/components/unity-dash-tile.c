#include "components/unity-dash-tile.h"

#include <gio/gdesktopappinfo.h>

#include "unity-settings.h"
#include "components/unity-pinned-apps.h"
#include "components/unity-desktop-actions.h"

struct _UnityDashTile
{
  UnityTile        parent_instance;

  AstalAppsApplication *app;
  GtkLabel             *label;
  GSettings            *settings;
};

/**
 * UnityDashTile:
 *
 * An dash cell showing the icon and label for one AstalApps.Application.
 *
 * A primary click launches the app and activates the dash.close action to
 * dismiss the dash. A secondary click offers Open, the app's .desktop actions,
 * and Pin to Launcher.
 */
G_DEFINE_FINAL_TYPE (UnityDashTile, unity_dash_tile, UNITY_TYPE_TILE)

typedef enum
{
  PROP_APPLICATION = 1,
} UnityDashTileProperty;
static GParamSpec *properties[PROP_APPLICATION + 1];

static void
on_launch (GSimpleAction *action, GVariant *param, gpointer user_data)
{
  (void) action; (void) param;
  UnityDashTile *self = user_data;
  astal_apps_application_launch (self->app);
  gtk_widget_activate_action (GTK_WIDGET (self), "dash.close", NULL);
}

static void
on_launch_action (GSimpleAction *action, GVariant *param, gpointer user_data)
{
  (void) action;
  UnityDashTile *self = user_data;
  GDesktopAppInfo *info = astal_apps_application_get_app (self->app);
  if (info != NULL)
    g_desktop_app_info_launch_action (info, g_variant_get_string (param, NULL), NULL);
  gtk_widget_activate_action (GTK_WIDGET (self), "dash.close", NULL);
}

static void
on_pin_launcher (GSimpleAction *action, GVariant *param, gpointer user_data)
{
  (void) action; (void) param;
  UnityDashTile *self = user_data;
  unity_pinned_apps_toggle (self->settings, astal_apps_application_get_entry (self->app));
}

static void
install_actions (UnityDashTile *self)
{
  static const GActionEntry entries[] = {
    { "launch",        on_launch,        NULL, NULL, NULL, { 0, 0, 0 } },
    { "launch-action", on_launch_action, "s",  NULL, NULL, { 0, 0, 0 } },
    { "pin-launcher",  on_pin_launcher,  NULL, NULL, NULL, { 0, 0, 0 } },
  };

  g_autoptr (GSimpleActionGroup) group = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (group),
                                   entries, G_N_ELEMENTS (entries), self);
  gtk_widget_insert_action_group (GTK_WIDGET (self), "app", G_ACTION_GROUP (group));
}

static void
unity_dash_tile_populate_menu (UnityTile *tile, GMenu *menu)
{
  UnityDashTile *self = UNITY_DASH_TILE (tile);
  GDesktopAppInfo       *info = self->app ? astal_apps_application_get_app (self->app) : NULL;

  {
    g_autoptr (GMenu) section = g_menu_new ();
    g_menu_append (section, "Open", "app.launch");
    g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
  }

  unity_desktop_actions_append (menu, info, "app.launch-action");

  {
    const gchar *entry = self->app ? astal_apps_application_get_entry (self->app) : NULL;
    gboolean     pinned = unity_pinned_apps_contains (self->settings, entry);

    g_autoptr (GMenu) section = g_menu_new ();
    g_menu_append (section, pinned ? "Unpin from Launcher" : "Pin to Launcher",
                  "app.pin-launcher");
    g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
  }
}

static void
on_clicked (GtkButton *button, gpointer user_data)
{
  (void) user_data;
  UnityDashTile *self = UNITY_DASH_TILE (button);
  if (self->app == NULL)
    return;
  astal_apps_application_launch (self->app);
  gtk_widget_activate_action (GTK_WIDGET (self), "dash.close", NULL);
}

static void
bind_application (UnityDashTile *self, AstalAppsApplication *app)
{
  if (app == NULL)
    return;

  self->app = g_object_ref (app);

  const gchar *icon_name = astal_apps_application_get_icon_name (app);
  const gchar *name      = astal_apps_application_get_name (app);

  if (icon_name != NULL && *icon_name != '\0')
    {
      g_autoptr (GIcon) icon = g_icon_new_for_string (icon_name, NULL);
      unity_tile_set_gicon (UNITY_TILE (self), icon);
    }
  else
    {
      unity_tile_set_gicon (UNITY_TILE (self), NULL);
    }

  gtk_label_set_text (self->label, name != NULL ? name : "");
}

/**
 * unity_dash_tile_new:
 * @app: the application the tile represents.
 *
 * Creates a new dash tile for @app.
 *
 * Returns: (transfer full): a new UnityDashTile
 */
GtkWidget *
unity_dash_tile_new (AstalAppsApplication *app)
{
  return g_object_new (UNITY_TYPE_DASH_TILE, "application", app, NULL);
}

static void
unity_dash_tile_dispose (GObject *object)
{
  UnityDashTile *self = UNITY_DASH_TILE (object);
  g_clear_object (&self->app);
  G_OBJECT_CLASS (unity_dash_tile_parent_class)->dispose (object);
}

static void
unity_dash_tile_set_property (GObject *object, guint id, const GValue *value, GParamSpec *pspec)
{
  UnityDashTile *self = UNITY_DASH_TILE (object);
  switch ((UnityDashTileProperty) id)
    {
    case PROP_APPLICATION: bind_application (self, g_value_get_object (value)); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
    }
}

static void
unity_dash_tile_class_init (UnityDashTileClass *klass)
{
  GObjectClass    *object_class = G_OBJECT_CLASS (klass);
  UnityTileClass *tile_class = UNITY_TILE_CLASS (klass);

  object_class->dispose      = unity_dash_tile_dispose;
  object_class->set_property = unity_dash_tile_set_property;

  tile_class->populate_menu = unity_dash_tile_populate_menu;

  properties[PROP_APPLICATION] = g_param_spec_object (
    "application", NULL, NULL, ASTAL_APPS_TYPE_APPLICATION,
    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), "dashtile");
}

static void
unity_dash_tile_init (UnityDashTile *self)
{
  self->settings = unity_settings_get_default ();

  self->label = GTK_LABEL (gtk_label_new (NULL));
  gtk_label_set_ellipsize (self->label, PANGO_ELLIPSIZE_END);
  gtk_label_set_wrap (self->label, FALSE);
  gtk_label_set_max_width_chars (self->label, 18);
  gtk_widget_set_halign (GTK_WIDGET (self->label), GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top (GTK_WIDGET (self->label), 2);
  gtk_widget_add_css_class (GTK_WIDGET (self->label), "body");
  gtk_box_append (unity_tile_get_box (UNITY_TILE (self)),
                  GTK_WIDGET (self->label));

  unity_tile_set_menu_position (UNITY_TILE (self), GTK_POS_RIGHT);

  install_actions (self);
  g_signal_connect (self, "clicked", G_CALLBACK (on_clicked), NULL);
}
