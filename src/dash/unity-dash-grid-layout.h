#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define UNITY_TYPE_DASH_GRID_LAYOUT (unity_dash_grid_layout_get_type ())

G_DECLARE_FINAL_TYPE (UnityDashGridLayout, unity_dash_grid_layout,
                      UNITY, DASH_GRID_LAYOUT, GtkLayoutManager)

GtkLayoutManager *unity_dash_grid_layout_new (void);

void unity_dash_grid_layout_set_max_rows (GtkLayoutManager *manager, gint max_rows);

G_END_DECLS
