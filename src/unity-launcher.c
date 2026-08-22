/* unity-launcher.c
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

#include "unity-launcher.h"

#include <adwaita.h>

#include "components/unity-hide.h"
#include "components/unity-launcher-tile.h"
#include "components/unity-pinned-apps-reorder.h"
#include "components/unity-pinned-apps.h"
#include "unity-app-entry.h"
#include "unity-app-list.h"
#include "unity-position.h"
#include "unity-settings.h"


#define DASH_BUTTON_PADDING 6

#define UNITY_LAUNCHER_KEY_LAUNCHER_ICON_SIZE "launcher-icon-size"

#define UNITY_LAUNCHER_CHANGED_PINNED_APPS "changed::" UNITY_LAUNCHER_KEY_PINNED_APPS

struct _UnityLauncher
{
  AstalWindow            parent_instance;
  GtkImage              *dash_image;
  GtkButton             *dash_button;
  GtkBox                *content;
  GtkScrolledWindow     *scroller;
  GtkBox                *strip;

  GSettings             *settings;
  UnityAppList          *apps;

  GtkOrientation         orientation;

  UnityHide         *hide;
  UnityPinnedAppsReorder *reorder;
};

static inline gboolean
is_horizontal (UnityLauncher *self)
{
  return self->orientation == GTK_ORIENTATION_HORIZONTAL;
}

G_DEFINE_FINAL_TYPE (UnityLauncher, unity_launcher, ASTAL_TYPE_WINDOW)

static void
commit_pinned (UnityLauncher *self, GPtrArray *items)
{
  g_ptr_array_add (items, NULL);
  g_settings_set_strv (self->settings, UNITY_LAUNCHER_KEY_PINNED_APPS,
                       (const gchar *const *) items->pdata);
}

static void
on_pin_toggle (GSimpleAction *action, GVariant *param, gpointer user_data)
{
  (void) action;
  UnityLauncher *self = user_data;
  if (param == NULL)
    return;
  unity_pinned_apps_toggle (self->settings, g_variant_get_string (param, NULL));
}

static void
on_quit (GSimpleAction *action, GVariant *param, gpointer user_data)
{
  (void) action;
  UnityLauncher *self = user_data;
  if (param == NULL)
    return;

  UnityAppEntry *entry =
    unity_app_list_get_entry (self->apps, g_variant_get_string (param, NULL));
  if (entry != NULL)
    unity_app_entry_close_all (entry);
}

static void
on_reorder_pinned (GSimpleAction *action, GVariant *param, gpointer user_data)
{
  (void) action;
  UnityLauncher *self = user_data;
  if (param == NULL)
    return;

  const gchar *source = NULL;
  gint32       dest   = 0;
  g_variant_get (param, "(&si)", &source, &dest);
  if (source == NULL || *source == '\0')
    return;

  g_auto (GStrv) ids = g_settings_get_strv (self->settings, UNITY_LAUNCHER_KEY_PINNED_APPS);
  if (ids == NULL)
    return;

  g_autoptr (GPtrArray) next = g_ptr_array_new_with_free_func (g_free);
  gboolean found = FALSE;
  for (gchar **p = ids; *p != NULL; p++)
    {
      if (g_strcmp0 (*p, source) == 0) { found = TRUE; continue; }
      g_ptr_array_add (next, g_strdup (*p));
    }
  if (!found)
    return;

  dest = CLAMP (dest, 0, (gint32) next->len);
  g_ptr_array_insert (next, dest, g_strdup (source));
  commit_pinned (self, next);
}

static void
install_action_group (UnityLauncher *self)
{
  static const GActionEntry entries[] = {
    { UNITY_LAUNCHER_ACTION_NAME_PIN_TOGGLE, on_pin_toggle,     "s",     NULL, NULL, { 0, 0, 0 } },
    { UNITY_LAUNCHER_ACTION_NAME_QUIT,       on_quit,           "s",     NULL, NULL, { 0, 0, 0 } },
    { UNITY_LAUNCHER_ACTION_NAME_REORDER,    on_reorder_pinned, "(si)",  NULL, NULL, { 0, 0, 0 } },
  };

  g_autoptr (GSimpleActionGroup) group = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (group),
                                   entries, G_N_ELEMENTS (entries), self);
  gtk_widget_insert_action_group (GTK_WIDGET (self),
                                  UNITY_LAUNCHER_ACTION_GROUP, G_ACTION_GROUP (group));
}

static void
sync_dash_button (UnityLauncher *self)
{
  gint size = g_settings_get_int (self->settings, UNITY_LAUNCHER_KEY_LAUNCHER_ICON_SIZE);
  gint side = size + DASH_BUTTON_PADDING;
  gtk_widget_set_size_request (GTK_WIDGET (self->dash_image), side, side);
  gtk_image_set_pixel_size (self->dash_image, size / 2);
}

static void
apply_alignment (UnityLauncher *self)
{
  GtkAlign align = unity_tile_alignment_to_align (
    g_settings_get_enum (self->settings, UNITY_LAUNCHER_KEY_TILE_ALIGNMENT));
  if (is_horizontal (self))
    gtk_widget_set_halign (GTK_WIDGET (self->strip), align);
  else
    gtk_widget_set_valign (GTK_WIDGET (self->strip), align);
}

/* Point the scroller and the dash-button gap along the strip's main axis. */
static void
configure_scroller (UnityLauncher *self)
{
  gboolean horiz = is_horizontal (self);

  gtk_scrolled_window_set_policy (self->scroller,
                                  horiz ? GTK_POLICY_EXTERNAL : GTK_POLICY_NEVER,
                                  horiz ? GTK_POLICY_NEVER : GTK_POLICY_EXTERNAL);
  gtk_scrolled_window_set_propagate_natural_width (self->scroller, !horiz);
  gtk_scrolled_window_set_propagate_natural_height (self->scroller, horiz);
  gtk_widget_set_hexpand (GTK_WIDGET (self->scroller), horiz);
  gtk_widget_set_vexpand (GTK_WIDGET (self->scroller), !horiz);

  gtk_widget_set_margin_end (GTK_WIDGET (self->dash_button), horiz ? 6 : 0);
  gtk_widget_set_margin_bottom (GTK_WIDGET (self->dash_button), horiz ? 0 : 6);
}

static void
apply_position (UnityLauncher *self)
{
  UnityPosition pos = g_settings_get_enum (self->settings, UNITY_LAUNCHER_KEY_POSITION);
  self->orientation = unity_position_orientation (pos);

  astal_window_set_anchor (ASTAL_WINDOW (self), unity_position_anchor (pos));
  gtk_orientable_set_orientation (GTK_ORIENTABLE (self->content), self->orientation);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (self->strip), self->orientation);
  configure_scroller (self);

  gtk_widget_remove_css_class (GTK_WIDGET (self), "pos-left");
  gtk_widget_remove_css_class (GTK_WIDGET (self), "pos-right");
  gtk_widget_remove_css_class (GTK_WIDGET (self), "pos-bottom");
  gtk_widget_add_css_class (GTK_WIDGET (self), unity_position_style_class (pos));

  /* The alignment axis flipped, so clear both axes then set the new main one. */
  gtk_widget_set_halign (GTK_WIDGET (self->strip), GTK_ALIGN_FILL);
  gtk_widget_set_valign (GTK_WIDGET (self->strip), GTK_ALIGN_FILL);
  apply_alignment (self);
}

/* A tile being dragged or showing its menu holds the launcher revealed, so it
 * never hides out from under the interaction. */
static void
on_tile_dragging (GObject *tile, GParamSpec *pspec, gpointer user_data)
{
  (void) pspec;
  UnityLauncher *self = user_data;
  unity_hide_set_hold (self->hide, UNITY_HIDE_HOLD_DRAG,
                           unity_launcher_tile_get_dragging (UNITY_LAUNCHER_TILE (tile)));
}

static void
on_tile_menu_shown (GObject *tile, GParamSpec *pspec, gpointer user_data)
{
  (void) pspec;
  UnityLauncher *self = user_data;
  unity_hide_set_hold (self->hide, UNITY_HIDE_HOLD_MENU,
                           unity_tile_get_menu_shown (UNITY_TILE (tile)));
}

static GtkWidget *
create_launcher_tile (UnityLauncher *self, UnityAppEntry *entry)
{
  GtkWidget *tile = unity_launcher_tile_new (entry);
  g_settings_bind (self->settings, UNITY_LAUNCHER_KEY_LAUNCHER_ICON_SIZE,
                   tile, "icon-size", G_SETTINGS_BIND_GET);
  g_signal_connect (tile, "notify::dragging", G_CALLBACK (on_tile_dragging), self);
  g_signal_connect (tile, "notify::menu-shown", G_CALLBACK (on_tile_menu_shown), self);
  return tile;
}

static gboolean
is_tile (GtkWidget *w)
{
  return w != NULL && UNITY_IS_LAUNCHER_TILE (w);
}

static GtkWidget *
nth_tile (UnityLauncher *self, gint n)
{
  for (GtkWidget *c = gtk_widget_get_first_child (GTK_WIDGET (self->strip));
       c != NULL; c = gtk_widget_get_next_sibling (c))
    if (is_tile (c) && n-- == 0)
      return c;
  return NULL;
}

static void
on_model_items_changed (GListModel *model, guint position, guint removed,
                        guint added, gpointer user_data)
{
  UnityLauncher *self = user_data;

  GtkWidget *child = nth_tile (self, position);
  for (guint i = 0; i < removed && child != NULL; i++)
    {
      GtkWidget *next = gtk_widget_get_next_sibling (child);
      gtk_box_remove (self->strip, child);
      child = next;
    }

  GtkWidget *after = position > 0 ? nth_tile (self, (gint) position - 1) : NULL;
  for (guint i = 0; i < added; i++)
    {
      g_autoptr (GObject) entry = g_list_model_get_item (model, position + i);
      GtkWidget *tile = create_launcher_tile (self, UNITY_APP_ENTRY (entry));
      gtk_box_insert_child_after (self->strip, tile, after);
      after = tile;
    }
}

static void
sync_pinned_apps (UnityLauncher *self)
{
  g_auto (GStrv) ids = g_settings_get_strv (self->settings, UNITY_LAUNCHER_KEY_PINNED_APPS);
  unity_app_list_set_pinned_app_ids (self->apps, (const gchar *const *) ids);
}

static void
unity_launcher_dispose (GObject *object)
{
  UnityLauncher *self = UNITY_LAUNCHER (object);

  g_clear_object (&self->hide);
  g_clear_object (&self->reorder);
  g_clear_object (&self->apps);

  G_OBJECT_CLASS (unity_launcher_parent_class)->dispose (object);
}

static void
unity_launcher_class_init (UnityLauncherClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = unity_launcher_dispose;

  gtk_widget_class_set_template_from_resource (
    widget_class, "/org/unity/launcher/unity-launcher.ui");
  gtk_widget_class_bind_template_child (widget_class, UnityLauncher, dash_image);
  gtk_widget_class_bind_template_child (widget_class, UnityLauncher, dash_button);
  gtk_widget_class_bind_template_child (widget_class, UnityLauncher, content);
  gtk_widget_class_bind_template_child (widget_class, UnityLauncher, scroller);
  gtk_widget_class_bind_template_child (widget_class, UnityLauncher, strip);

  gtk_widget_class_set_css_name (widget_class, "unity-launcher");
}

static void
unity_launcher_init (UnityLauncher *self)
{
  self->settings = unity_settings_get_default ();

  gtk_widget_init_template (GTK_WIDGET (self));

  apply_position (self);
  g_signal_connect_object (self->settings, "changed::" UNITY_LAUNCHER_KEY_POSITION,
                           G_CALLBACK (apply_position), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->settings, "changed::" UNITY_LAUNCHER_KEY_TILE_ALIGNMENT,
                           G_CALLBACK (apply_alignment), self, G_CONNECT_SWAPPED);

  sync_dash_button (self);
  g_signal_connect_object (self->settings, "changed::" UNITY_LAUNCHER_KEY_LAUNCHER_ICON_SIZE,
                           G_CALLBACK (sync_dash_button), self, G_CONNECT_SWAPPED);

  /* Before the tiles, so each can hold the launcher revealed while dragged. */
  self->hide = unity_hide_new (ASTAL_WINDOW (self));
  self->reorder  = unity_pinned_apps_reorder_new (self->strip);

  self->apps = unity_app_list_new ();
  g_signal_connect_object (self->apps, "items-changed",
                           G_CALLBACK (on_model_items_changed), self, G_CONNECT_DEFAULT);
  on_model_items_changed (G_LIST_MODEL (self->apps), 0, 0,
                          g_list_model_get_n_items (G_LIST_MODEL (self->apps)), self);

  install_action_group (self);

  g_signal_connect_object (self->settings, UNITY_LAUNCHER_CHANGED_PINNED_APPS,
                           G_CALLBACK (sync_pinned_apps), self, G_CONNECT_SWAPPED);
  sync_pinned_apps (self);
}

static void
ensure_style (void)
{
  static gsize done = 0;

  if (g_once_init_enter (&done))
    {
      g_autoptr (GtkCssProvider) provider = gtk_css_provider_new ();

      gtk_css_provider_load_from_resource (provider, "/org/unity/launcher/launcher.css");
      gtk_style_context_add_provider_for_display (
        gdk_display_get_default (), GTK_STYLE_PROVIDER (provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

      g_once_init_leave (&done, 1);
    }
}

UnityLauncher *
unity_launcher_new (GtkApplication *app)
{
  ensure_style ();
  return g_object_new (UNITY_TYPE_LAUNCHER, "application", app, NULL);
}
