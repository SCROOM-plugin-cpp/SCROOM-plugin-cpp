#include <scroom/presentationinterface.hh>

#include "slicontrolpanel.hh"

enum
{
  COL_VISIBILITY = 0,
  COL_ID,
  NUM_COLS
};

static void update_layers_upper(GtkRange *this_range,
                                GtkTreeView *view)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *path;

  GtkAdjustment *adj = gtk_range_get_adjustment(this_range);
  model = gtk_tree_view_get_model(view);
  int high = gtk_adjustment_get_upper(adj);
  int this_value = gtk_range_get_value(this_range);

  for (int i = this_value; i <= high; i++)
  {
    path = g_strdup_printf("%i", i);
    if (gtk_tree_model_get_iter_from_string(model, &iter, path))
    {
      if (i <= this_value)
      {
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, COL_VISIBILITY, TRUE, -1);
      }
      else
      {
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, COL_VISIBILITY, FALSE, -1);
      }
    }
  }
}

void update_layers_lower(GtkRange *this_range,
                         GtkTreeView *view)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *path;

  GtkAdjustment *adj = gtk_range_get_adjustment(this_range);
  model = gtk_tree_view_get_model(view);
  int low = gtk_adjustment_get_lower(adj);
  int this_value = gtk_range_get_value(this_range);

  for (int i = this_value; low <= i; i--)
  {
    path = g_strdup_printf("%i", i);
    if (gtk_tree_model_get_iter_from_string(model, &iter, path))
    {
      if (i >= this_value)
      {
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, COL_VISIBILITY, TRUE, -1);
      }
      else
      {
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, COL_VISIBILITY, FALSE, -1);
      }
    }
  }
}

static gboolean on_change_value_upper(GtkRange *this_range,
                                      GtkScrollType scroll,
                                      gdouble this_value,
                                      GtkRange *other_range)
{
  UNUSED(this_range);
  UNUSED(scroll);

  int other_value;
  other_value = gtk_range_get_value(other_range);

  if (this_value - other_value <= 0)
  {
    return TRUE;
  }

  return FALSE;
}

static gboolean on_change_value_lower(GtkRange *this_range,
                                      GtkScrollType scroll,
                                      gdouble this_value,
                                      GtkRange *other_range)
{
  UNUSED(this_range);
  UNUSED(scroll);

  int other_value;
  other_value = gtk_range_get_value(other_range);

  if (other_value - this_value <= 0)
  {
    return TRUE;
  }

  return FALSE;
}

// Toggle callback
static void on_toggle(GtkCellRendererToggle *renderer, gchar *path, GtkTreeView *view)
{
  UNUSED(renderer);
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean state;

  model = gtk_tree_view_get_model(view);

  if (gtk_tree_model_get_iter_from_string(model, &iter, path))
  {
    gtk_tree_model_get(model, &iter, COL_VISIBILITY, &state, -1);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, COL_VISIBILITY, !state, -1);
  }
}

// The model
// TODO: making the model an instance variable might be a good idea
static GtkTreeModel *create_model(int n_layers)
{
  GtkListStore *list_store;
  GtkTreeIter iter;

  list_store = gtk_list_store_new(NUM_COLS,
                                  G_TYPE_BOOLEAN,
                                  G_TYPE_STRING);
  char id[16];
  int i = 0;
  while (i < n_layers)
  {
    sprintf(id, "Layer %d", i);
    gtk_list_store_append(list_store, &iter);
    gtk_list_store_set(list_store, &iter,
                       COL_VISIBILITY, TRUE,
                       COL_ID, id,
                       -1);
    i++;
  }

  return GTK_TREE_MODEL(list_store);
}

// The view: only displays data from the model
static GtkWidget *create_view_and_model(int n_layers)
{
  GtkCellRenderer *renderer;
  GtkTreeModel *model;
  GtkWidget *treeview;

  treeview = gtk_tree_view_new();

  /* --- Column Visibility --- */
  renderer = gtk_cell_renderer_toggle_new();
  gtk_cell_renderer_toggle_set_activatable(GTK_CELL_RENDERER_TOGGLE(renderer), TRUE);
  g_signal_connect(G_OBJECT(renderer), "toggled", G_CALLBACK(on_toggle), treeview);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview),
                                              -1,
                                              "Show",
                                              renderer,
                                              "active", COL_VISIBILITY,
                                              NULL);

  /* --- Column ID --- */
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview),
                                              -1,
                                              "Layer",
                                              renderer,
                                              "text", COL_ID,
                                              NULL);

  model = create_model(n_layers);

  gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), model);

  /* The tree view has acquired its own reference to the
  *  model, so we can drop ours. That way the model will
  *  be freed automatically when the tree view is destroyed */
  g_object_unref(model);

  return GTK_WIDGET(treeview);
}

SliControlPanel::SliControlPanel(ViewInterface::WeakPtr viewWeak)
{
  printf("Multilayer control panel has been created\n");

  ViewInterface::Ptr view(viewWeak);

  GtkWidget *slider_low,
            *slider_high,
            *treeview;

  // TODO: this should be an instance variable
  int n_layers = 5;
  
  treeview = create_view_and_model(n_layers);
  slider_low = gtk_vscale_new_with_range(0, n_layers - 1, 1);
  slider_high = gtk_vscale_new_with_range(0, n_layers - 1, 1);

  // Initially display all layers
  gtk_range_set_value(GTK_RANGE(slider_low), 0);
  gtk_range_set_value(GTK_RANGE(slider_high), n_layers - 1);
  gtk_widget_set_can_focus(slider_low, FALSE);
  gtk_widget_set_can_focus(slider_high, FALSE);

  // Connect the callbacks
  g_signal_connect(GTK_RANGE(slider_high), "change-value",
                   G_CALLBACK(on_change_value_upper), GTK_RANGE(slider_low));
  g_signal_connect(G_OBJECT(slider_high), "value-changed",
                   G_CALLBACK(update_layers_upper), GTK_TREE_VIEW(treeview));
  g_signal_connect(GTK_RANGE(slider_low), "change-value",
                   G_CALLBACK(on_change_value_lower), GTK_RANGE(slider_high));
  g_signal_connect(G_OBJECT(slider_low), "value-changed",
                   G_CALLBACK(update_layers_lower), GTK_TREE_VIEW(treeview));

  // Set the number of decimal places to display for each widget.
  gtk_scale_set_digits(GTK_SCALE(slider_low), 0);
  gtk_scale_set_digits(GTK_SCALE(slider_high), 0);

  // Set the position of the value with respect to the widget.
  gtk_scale_set_value_pos(GTK_SCALE(slider_low), GTK_POS_TOP);
  gtk_scale_set_value_pos(GTK_SCALE(slider_high), GTK_POS_TOP);

  // Pack the widgets into a hbox
  GtkWidget* hbox = gtk_hbox_new(false, 10);
  gtk_box_pack_start(GTK_BOX(hbox), treeview, false, false, 0);
  gtk_box_pack_start(GTK_BOX(hbox), slider_low, false, false, 0);
  gtk_box_pack_start(GTK_BOX(hbox), slider_high, false, false, 0);
  gtk_widget_show_all(hbox);

  // Add the hbox to the sidebar
  gdk_threads_enter();
  view->addSideWidget("Layers", hbox);
  gdk_threads_leave();
}

SliControlPanel::~SliControlPanel()
{
  printf("Multilayer control panel has been destroyed\n");
}

SliControlPanel::Ptr SliControlPanel::create(ViewInterface::WeakPtr viewWeak, SliPresentation::Ptr presentation_)
{
  SliControlPanel::Ptr result = SliControlPanel::Ptr(new SliControlPanel(viewWeak));
  result->presentation = presentation_;

  return result;
}

