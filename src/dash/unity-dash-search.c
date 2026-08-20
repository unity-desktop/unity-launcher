#include "dash/unity-dash-search.h"

#include "dash/search/unity-search.h"
#include "dash/search/unity-search-result-rows.h"
#include "dash/search/unity-search-app-results.h"

struct _UnityDashSearch
{
  AdwBin                 parent_instance;

  UnitySearch           *search;

  GtkBox                *groups;
  UnitySearchAppResults *apps;
  AdwStatusPage         *placeholder;

  GPtrArray             *provider_groups;
};

/**
 * UnityDashSearch:
 *
 * The dash's search page.
 *
 * A matching-apps group plus a group per search provider. It owns the search
 * orchestration and the parent drives it. When a query matches nothing it shows
 * an inline "no results" placeholder rather than a separate page.
 */
G_DEFINE_FINAL_TYPE (UnityDashSearch, unity_dash_search, ADW_TYPE_BIN)

static void
update_placeholder (UnityDashSearch *self)
{
  gboolean nothing = !gtk_widget_get_visible (GTK_WIDGET (self->apps)) &&
                     self->provider_groups->len == 0;
  gtk_widget_set_visible (GTK_WIDGET (self->placeholder), nothing);
}

static void
on_provider_results (UnitySearch *search, UnitySearchProvider *provider,
                     GPtrArray *results, gpointer user_data)
{
  (void) search;
  UnityDashSearch *self = user_data;

  GtkWidget *rows = unity_search_result_rows_new (provider, results);

  gtk_box_append (self->groups, rows);
  g_ptr_array_add (self->provider_groups, rows);
  update_placeholder (self);
}

static void
clear_provider_groups (UnityDashSearch *self)
{
  for (guint i = 0; i < self->provider_groups->len; i++)
    gtk_box_remove (self->groups, self->provider_groups->pdata[i]);
  g_ptr_array_set_size (self->provider_groups, 0);
}

/**
 * unity_dash_search_run:
 * @self: a UnityDashSearch
 * @query: the text to search for.
 *
 * Runs @query. Matching apps are filled synchronously and search providers are
 * queried asynchronously. The top match is highlighted while the entry keeps
 * focus. The "no results" placeholder shows if nothing matches.
 */
void
unity_dash_search_run (UnityDashSearch *self, const gchar *query)
{
  g_return_if_fail (UNITY_IS_DASH_SEARCH (self));

  unity_search_app_results_fill (self->apps, query);
  clear_provider_groups (self);
  update_placeholder (self);
  unity_search_query (self->search, query, 0);
}

/**
 * unity_dash_search_activate_selected:
 * @self: a UnityDashSearch
 *
 * Launches the highlighted default match. Called for Enter while the entry is
 * focused.
 */
void
unity_dash_search_activate_selected (UnityDashSearch *self)
{
  g_return_if_fail (UNITY_IS_DASH_SEARCH (self));
  unity_search_app_results_activate_selected (self->apps);
}

/**
 * unity_dash_search_focus_results:
 * @self: a UnityDashSearch
 *
 * Moves keyboard focus into the results. Called for Down from the entry, so the
 * arrow keys then navigate the results natively.
 */
void
unity_dash_search_focus_results (UnityDashSearch *self)
{
  g_return_if_fail (UNITY_IS_DASH_SEARCH (self));
  unity_search_app_results_focus (self->apps);
}

/**
 * unity_dash_search_reset:
 * @self: a UnityDashSearch
 *
 * Cancels any in-flight query and clears the results.
 */
void
unity_dash_search_reset (UnityDashSearch *self)
{
  g_return_if_fail (UNITY_IS_DASH_SEARCH (self));
  unity_search_query (self->search, NULL, 0);
  clear_provider_groups (self);
  unity_search_app_results_clear (self->apps);
  gtk_widget_set_visible (GTK_WIDGET (self->placeholder), FALSE);
}

/**
 * unity_dash_search_new:
 *
 * Creates a new search page for the dash.
 *
 * Returns: (transfer full): a new UnityDashSearch
 */
GtkWidget *
unity_dash_search_new (void)
{
  return g_object_new (UNITY_TYPE_DASH_SEARCH, NULL);
}

static void
unity_dash_search_dispose (GObject *object)
{
  UnityDashSearch *self = UNITY_DASH_SEARCH (object);
  g_clear_pointer (&self->provider_groups, g_ptr_array_unref);
  gtk_widget_dispose_template (GTK_WIDGET (object), UNITY_TYPE_DASH_SEARCH);
  G_OBJECT_CLASS (unity_dash_search_parent_class)->dispose (object);
}

static void
unity_dash_search_class_init (UnityDashSearchClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  G_OBJECT_CLASS (klass)->dispose = unity_dash_search_dispose;

  gtk_widget_class_set_template_from_resource (
    widget_class, "/org/unity/launcher/dash/unity-dash-search.ui");
  gtk_widget_class_bind_template_child (widget_class, UnityDashSearch, groups);
  gtk_widget_class_bind_template_child (widget_class, UnityDashSearch, placeholder);
}

static void
unity_dash_search_init (UnityDashSearch *self)
{
  self->search          = unity_search_get_default ();
  self->provider_groups = g_ptr_array_new ();

  gtk_widget_init_template (GTK_WIDGET (self));

  /* The app matches sit above the placeholder; provider rows append after it. */
  self->apps = UNITY_SEARCH_APP_RESULTS (unity_search_app_results_new ());
  gtk_widget_set_visible (GTK_WIDGET (self->apps), FALSE);
  gtk_box_prepend (self->groups, GTK_WIDGET (self->apps));

  g_signal_connect_object (self->search, "provider-results",
                           G_CALLBACK (on_provider_results), self, 0);
}
