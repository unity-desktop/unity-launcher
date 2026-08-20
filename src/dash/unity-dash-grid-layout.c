#include "dash/unity-dash-grid-layout.h"

#define GRID_GAP 24

struct _UnityDashGridLayout
{
  GtkLayoutManager parent_instance;

  gint max_rows;   /* rows to show at most, or 0 for no limit */
};

/**
 * UnityDashGridLayout:
 *
 * A layout manager arranging children in a grid of equal square cells.
 *
 * It works off the horizontal: as many columns as fit the allocated width, then
 * the square side grown to fill the row exactly, and the height derived from the
 * resulting row count. The base cell is the tallest child's natural height so no
 * child is ever clipped. It suits any container of uniform tiles, so the dash
 * browse grid and the search matches share it.
 */
G_DEFINE_FINAL_TYPE (UnityDashGridLayout, unity_dash_grid_layout, GTK_TYPE_LAYOUT_MANAGER)

/* The base cell is a child's natural height (icon plus one label line). Height,
 * not width, drives the square so a long label does not inflate the cell; a wide
 * label simply ellipsizes, and nothing is clipped because the cell is never
 * shorter than the content. The tiles are uniform, so the first one measured
 * gives the cell for all of them, in O(1) rather than a per-child sweep. */
static gint
base_cell_size (GtkWidget *box)
{
  for (GtkWidget *child = gtk_widget_get_first_child (box);
       child != NULL; child = gtk_widget_get_next_sibling (child))
    {
      if (!gtk_widget_should_layout (child))
        continue;

      gint child_height;
      gtk_widget_measure (child, GTK_ORIENTATION_VERTICAL, -1,
                          NULL, &child_height, NULL, NULL);
      return MAX (1, child_height);
    }
  return 1;
}

static guint
count_tiles (GtkWidget *box)
{
  guint tiles = 0;
  for (GtkWidget *child = gtk_widget_get_first_child (box);
       child != NULL; child = gtk_widget_get_next_sibling (child))
    if (gtk_widget_should_layout (child))
      tiles++;
  return tiles;
}

/* How many base cells fit the width, and the side each grows to so the row fills
 * exactly. The count is not clamped to the tile count, so every box sizes its
 * cells the same way and rows stay flush. */
static void
resolve_grid (gint available_width, gint base_cell,
              gint *out_columns, gint *out_cell_size)
{
  gint columns = MAX (1, (available_width + GRID_GAP) / (base_cell + GRID_GAP));

  *out_columns   = columns;
  *out_cell_size = (available_width - (columns - 1) * GRID_GAP) / columns;
}

static GtkSizeRequestMode
unity_dash_grid_layout_get_request_mode (GtkLayoutManager *manager, GtkWidget *box)
{
  (void) manager; (void) box;
  return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
unity_dash_grid_layout_measure (GtkLayoutManager *manager, GtkWidget *box,
                                 GtkOrientation orientation, gint for_size,
                                 gint *minimum, gint *natural,
                                 gint *minimum_baseline, gint *natural_baseline)
{
  UnityDashGridLayout *self = UNITY_DASH_GRID_LAYOUT (manager);
  gint base_cell = base_cell_size (box);

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      /* One cell wide is the narrowest the grid can be. */
      *minimum = base_cell;
      *natural = base_cell;
    }
  else
    {
      guint tiles           = count_tiles (box);
      gint  available_width = (for_size > 0) ? for_size : base_cell;
      gint  columns, cell_size;
      resolve_grid (available_width, base_cell, &columns, &cell_size);

      gint rows = MAX (1, (gint) (tiles + columns - 1) / columns);
      if (self->max_rows > 0)
        rows = MIN (rows, self->max_rows);

      gint total_height = rows * cell_size + (rows - 1) * GRID_GAP;
      *minimum = total_height;
      *natural = total_height;
    }

  if (minimum_baseline) *minimum_baseline = -1;
  if (natural_baseline) *natural_baseline = -1;
}

static void
unity_dash_grid_layout_allocate (GtkLayoutManager *manager, GtkWidget *box,
                                  gint width, gint height, gint baseline)
{
  (void) height; (void) baseline;
  UnityDashGridLayout *self = UNITY_DASH_GRID_LAYOUT (manager);
  gint columns, cell_size;
  resolve_grid (width, base_cell_size (box), &columns, &cell_size);

  /* With a row cap, only the first (rows * columns) tiles are shown; the rest
   * are hidden from the layout so nothing spills past the capped height. */
  gint shown = (self->max_rows > 0) ? self->max_rows * columns : -1;

  gint index = 0;
  for (GtkWidget *child = gtk_widget_get_first_child (box);
       child != NULL; child = gtk_widget_get_next_sibling (child))
    {
      if (!gtk_widget_get_visible (child))
        continue;

      gboolean show = (shown < 0) || (index < shown);
      if (gtk_widget_get_child_visible (child) != show)
        gtk_widget_set_child_visible (child, show);

      if (show)
        {
          gint column = index % columns;
          gint row    = index / columns;
          GtkAllocation allocation = {
            .x      = column * (cell_size + GRID_GAP),
            .y      = row    * (cell_size + GRID_GAP),
            .width  = cell_size,
            .height = cell_size,
          };
          gtk_widget_size_allocate (child, &allocation, -1);
        }
      index++;
    }
}

static void
unity_dash_grid_layout_class_init (UnityDashGridLayoutClass *klass)
{
  GtkLayoutManagerClass *layout_class = GTK_LAYOUT_MANAGER_CLASS (klass);
  layout_class->get_request_mode = unity_dash_grid_layout_get_request_mode;
  layout_class->measure          = unity_dash_grid_layout_measure;
  layout_class->allocate         = unity_dash_grid_layout_allocate;
}

static void
unity_dash_grid_layout_init (UnityDashGridLayout *self)
{
  self->max_rows = 0;
}

/**
 * unity_dash_grid_layout_new:
 *
 * Creates a new square-cell grid layout manager.
 *
 * Returns: (transfer full): a new UnityDashGridLayout
 */
GtkLayoutManager *
unity_dash_grid_layout_new (void)
{
  return g_object_new (UNITY_TYPE_DASH_GRID_LAYOUT, NULL);
}

/**
 * unity_dash_grid_layout_set_max_rows:
 * @manager: a UnityDashGridLayout
 * @max_rows: the most rows to lay out, or 0 for no limit
 *
 * Caps the grid to @max_rows rows. Tiles past the cap are hidden, so the grid
 * shows a fixed number of rows whose column count still follows the width.
 */
void
unity_dash_grid_layout_set_max_rows (GtkLayoutManager *manager, gint max_rows)
{
  g_return_if_fail (UNITY_IS_DASH_GRID_LAYOUT (manager));
  UnityDashGridLayout *self = UNITY_DASH_GRID_LAYOUT (manager);

  if (self->max_rows == max_rows)
    return;
  self->max_rows = max_rows;
  gtk_layout_manager_layout_changed (manager);
}
