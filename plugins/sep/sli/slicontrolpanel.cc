#include <boost/dynamic_bitset.hpp>
#include <gdk/gdkkeysyms.h>
#include <scroom/unused.hh>

#include "slicontrolpanel.hh"
#include "slilayer.hh"

using Scroom::GtkHelpers::sync_on_ui_thread;

/* Ignore scroll events on both sliders */
gboolean scroll_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  UNUSED(widget);
  UNUSED(event);
  UNUSED(user_data);
  return TRUE;
}

/* Remove focus from the widget when the ESC key is pressed */
gboolean escape_key_pressed(GtkWidget *widget, GdkEventKey *event,
                            gpointer user_data) {
  require(Scroom::GtkHelpers::on_ui_thread());

  UNUSED(user_data);

  if (event->keyval == GDK_KEY_Escape) {
    gtk_widget_set_sensitive(widget, FALSE);
    gtk_widget_set_sensitive(widget, TRUE);
    return TRUE;
  }

  return FALSE;
}

/* Check whether the constraint defined by the sliders' bounds are respected */
gboolean check_constraints(gdouble old_low, gdouble old_high,
                           SliControlPanel *cPanel) {
  auto presPtr = cPanel->presentation.lock();
  boost::dynamic_bitset<> toggled{cPanel->getNumLayers()};
  auto visible = presPtr->getVisible();

  for (size_t i = old_low; i <= old_high; i++)
    toggled.set(i);

  if (toggled == visible)
    return TRUE;

  return FALSE;
}

/* Toggle the layers and trigger a redraw */
void toggle_and_redraw(double old_value, double new_value, double other_value,
                       GtkWidget *widget, SliControlPanel *cPanel) {
  auto presPtr = cPanel->presentation.lock();
  boost::dynamic_bitset<> toggled{cPanel->getNumLayers()};
  auto visible = presPtr->getVisible();
  int start = std::min(old_value, new_value);
  int finish = std::max(old_value, new_value);

  if (widget == cPanel->widgets[SLIDER_LOW]) {
    if (!check_constraints(old_value, other_value, cPanel)) {
      for (int i = new_value; i <= other_value; i++)
        toggled.set(i);

      toggled ^= visible;
    } else {
      for (size_t i = start; i < static_cast<size_t>(finish); i++)
        toggled.set(i);
    }
  } else if (widget == cPanel->widgets[SLIDER_HIGH]) {
    if (!check_constraints(other_value, old_value, cPanel)) {
      for (int i = other_value; i <= new_value; i++)
        toggled.set(i);

      toggled ^= visible;
    } else {
      for (size_t i = start + 1; i <= static_cast<size_t>(finish); i++)
        toggled.set(i);
    }
  }

  presPtr->setToggled(toggled);
  presPtr->wipeCacheAndRedraw();
}

/* Update the Tree Model according to the new sliders' bounds */
void update_tree_model(GtkTreeView *treeview, int min, int max, int low,
                       int high) {
  require(Scroom::GtkHelpers::on_ui_thread());

  GtkTreeIter iter;
  gchar *path;
  GtkTreeModel *model = gtk_tree_view_get_model(treeview);

  for (int i = min; i <= max; i++) {
    path = g_strdup_printf("%i", i);
    if (gtk_tree_model_get_iter_from_string(model, &iter, path)) {
      if (i >= low && i <= high) {
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, COL_VISIBILITY, TRUE,
                           -1);
      } else {
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, COL_VISIBILITY, FALSE,
                           -1);
      }
    }
  }
}

/* Takes care of all focus-out, button-press, and key-release events */
gboolean slider_event_handler(GtkWidget *widget, GdkEvent *event,
                              SliControlPanel *cPanel) {
  require(Scroom::GtkHelpers::on_ui_thread());

  UNUSED(event);

  if (widget == cPanel->widgets[SLIDER_LOW]) {
    GtkAdjustment *adj_other = gtk_range_get_adjustment(
        reinterpret_cast<GtkRange *>(cPanel->widgets[SLIDER_HIGH]));
    int max = gtk_adjustment_get_upper(adj_other);
    int min = gtk_adjustment_get_lower(adj_other);
    gdouble other_value = gtk_adjustment_get_value(adj_other);
    gdouble this_old_value = cPanel->oldValue[SLIDER_LOW];
    gdouble this_new_value =
        gtk_range_get_value(reinterpret_cast<GtkRange *>(widget));

    if (abs(this_old_value - this_new_value) > 0) {
      cPanel->lastFocused = SLIDER_LOW;
      cPanel->oldValue[SLIDER_LOW] = this_new_value;
      update_tree_model(
          reinterpret_cast<GtkTreeView *>(cPanel->widgets[TREEVIEW]), min, max,
          this_new_value, other_value);
      toggle_and_redraw(this_old_value, this_new_value, other_value, widget,
                        cPanel);
    }
  } else if (widget == cPanel->widgets[SLIDER_HIGH]) {
    GtkAdjustment *adj_other = gtk_range_get_adjustment(
        reinterpret_cast<GtkRange *>(cPanel->widgets[SLIDER_LOW]));
    int max = gtk_adjustment_get_upper(adj_other);
    int min = gtk_adjustment_get_lower(adj_other);
    gdouble other_value = gtk_adjustment_get_value(adj_other);
    gdouble this_old_value = cPanel->oldValue[SLIDER_HIGH];
    gdouble this_new_value =
        gtk_range_get_value(reinterpret_cast<GtkRange *>(widget));

    if (abs(this_old_value - this_new_value) > 0) {
      cPanel->lastFocused = SLIDER_HIGH;
      cPanel->oldValue[SLIDER_HIGH] = this_new_value;
      update_tree_model(
          reinterpret_cast<GtkTreeView *>(cPanel->widgets[TREEVIEW]), min, max,
          other_value, this_new_value);
      toggle_and_redraw(this_old_value, this_new_value, other_value, widget,
                        cPanel);
    }
  }
  return FALSE;
}

/* Makes sure a slider can't go above or below the other slider's value */
gboolean change_value(GtkRange *range, GtkScrollType scroll, gdouble this_value,
                      SliControlPanel *cPanel) {
  require(Scroom::GtkHelpers::on_ui_thread());

  UNUSED(range);
  UNUSED(scroll);

  if (range == reinterpret_cast<GtkRange *>(cPanel->widgets[SLIDER_LOW])) {
    gdouble other_value = gtk_range_get_value(
        reinterpret_cast<GtkRange *>(cPanel->widgets[SLIDER_HIGH]));
    if (other_value - this_value < 0)
      return TRUE;
  } else if (range ==
             reinterpret_cast<GtkRange *>(cPanel->widgets[SLIDER_HIGH])) {
    gdouble other_value = gtk_range_get_value(
        reinterpret_cast<GtkRange *>(cPanel->widgets[SLIDER_LOW]));
    if (this_value - other_value < 0)
      return TRUE;
  }

  return FALSE;
}

/* Updates the Tree Model and triggers a redraw when a checkmark is toggled */
void on_toggle(GtkCellRendererToggle *renderer, gchar *path,
               SliControlPanel *cPanel) {
  require(Scroom::GtkHelpers::on_ui_thread());

  UNUSED(renderer);
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean state;

  SliPresentationInterface::Ptr presPtr = cPanel->presentation.lock();
  std::vector<SliLayer::Ptr> layers = presPtr->getLayers();
  boost::dynamic_bitset<> toggled{cPanel->getNumLayers()};

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(cPanel->widgets[TREEVIEW]));

  if (gtk_tree_model_get_iter_from_string(model, &iter, path)) {
    gtk_tree_model_get(model, &iter, COL_VISIBILITY, &state, -1);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, COL_VISIBILITY, !state,
                       -1);
    cPanel->lastFocused = TREEVIEW;
    toggled.set(atoi(path));
    presPtr->setToggled(toggled);
    presPtr->wipeCacheAndRedraw();
  }
}

void SliControlPanel::create_view_and_model() {
  require(Scroom::GtkHelpers::on_ui_thread());

  // Create the view ----------------------------------------
  GtkCellRenderer *renderer;

  /* --- Column Visibility --- */
  renderer = gtk_cell_renderer_toggle_new();
  gtk_cell_renderer_toggle_set_activatable(GTK_CELL_RENDERER_TOGGLE(renderer),
                                           TRUE);
  g_signal_connect(G_OBJECT(renderer), "toggled", G_CALLBACK(on_toggle), this);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(widgets[TREEVIEW]),
                                              -1, "Show", renderer, "active",
                                              COL_VISIBILITY, NULL);

  /* --- Column ID --- */
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(widgets[TREEVIEW]),
                                              -1, "Layer", renderer, "text",
                                              COL_ID, NULL);

  // Create the model ----------------------------------------
  GtkListStore *list_store;
  GtkTreeIter iter;
  SliPresentationInterface::Ptr presPtr = presentation.lock();

  list_store = gtk_list_store_new(NUM_COLS, G_TYPE_BOOLEAN, G_TYPE_STRING);

  for (SliLayer::Ptr layer : presPtr->getLayers()) {
    gtk_list_store_append(list_store, &iter);
    gtk_list_store_set(list_store, &iter, COL_VISIBILITY, TRUE, COL_ID,
                       layer->name.c_str(), -1);
  }

  // Wrap up ----------------------------------------
  gtk_tree_view_set_model(GTK_TREE_VIEW(widgets[TREEVIEW]),
                          GTK_TREE_MODEL(list_store));

  /* The tree view has acquired its own reference to the
   *  model, so we can drop ours. That way the model will
   *  be freed automatically when the tree view is destroyed */
  g_object_unref(list_store);
}

SliControlPanel::SliControlPanel(
    ViewInterface::WeakPtr viewWeak_,
    SliPresentationInterface::WeakPtr presentation_)
    : presentation(presentation_), viewWeak(viewWeak_) {
  require(Scroom::GtkHelpers::on_ui_thread());

  printf("Multilayer control panel has been created\n");
  SliPresentationInterface::Ptr presPtr = presentation.lock();
  std::vector<SliLayer::Ptr> layers = presPtr->getLayers();
  n_layers = layers.size();
  ViewInterface::Ptr view(viewWeak);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  GtkWidget *treeview = gtk_tree_view_new();
  widgets[TREEVIEW] = treeview;

  create_view_and_model();
  gtk_box_pack_start(GTK_BOX(hbox), treeview, false, false, 0);

  if (n_layers > 1) {
    GtkWidget *slider_low =
        gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0, n_layers - 1, 1);
    GtkWidget *slider_high =
        gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0, n_layers - 1, 1);
    widgets[SLIDER_LOW] = slider_low;
    widgets[SLIDER_HIGH] = slider_high;

    // Initially display all layers
    gtk_range_set_value(GTK_RANGE(slider_low), 0);
    gtk_range_set_value(GTK_RANGE(slider_high), n_layers - 1);
    oldValue[SLIDER_LOW] = 0;
    oldValue[SLIDER_HIGH] = n_layers - 1;

    // Connect the callbacks
    g_signal_connect(slider_low, "change-value", G_CALLBACK(change_value),
                     this);
    g_signal_connect(slider_low, "button-release-event",
                     G_CALLBACK(slider_event_handler), this);
    g_signal_connect(slider_low, "focus-out-event",
                     G_CALLBACK(slider_event_handler), this);
    g_signal_connect(slider_low, "key-release-event",
                     G_CALLBACK(slider_event_handler), this);
    g_signal_connect(slider_low, "scroll-event", G_CALLBACK(scroll_event),
                     NULL);
    g_signal_connect(slider_low, "key-press-event",
                     G_CALLBACK(escape_key_pressed), NULL);

    g_signal_connect(slider_high, "change-value", G_CALLBACK(change_value),
                     this);
    g_signal_connect(slider_high, "button-release-event",
                     G_CALLBACK(slider_event_handler), this);
    g_signal_connect(slider_high, "focus-out-event",
                     G_CALLBACK(slider_event_handler), this);
    g_signal_connect(slider_high, "key-release-event",
                     G_CALLBACK(slider_event_handler), this);
    g_signal_connect(slider_high, "scroll-event", G_CALLBACK(scroll_event),
                     NULL);
    g_signal_connect(slider_high, "key-press-event",
                     G_CALLBACK(escape_key_pressed), NULL);

    // Set the number of decimal places to display for each widget.
    gtk_scale_set_digits(GTK_SCALE(slider_low), 0);
    gtk_scale_set_digits(GTK_SCALE(slider_high), 0);

    // Set the position of the value with respect to the widget.
    gtk_scale_set_value_pos(GTK_SCALE(slider_low), GTK_POS_TOP);
    gtk_scale_set_value_pos(GTK_SCALE(slider_high), GTK_POS_TOP);

    // Pack the sliders into the hbox
    gtk_box_pack_start(GTK_BOX(hbox), slider_low, false, false, 0);
    gtk_box_pack_start(GTK_BOX(hbox), slider_high, false, false, 0);
  }

  gtk_widget_show_all(hbox);

  // Add the hbox to the sidebar
  sync_on_ui_thread([&] { view->addSideWidget("Layers", hbox); });
}

void SliControlPanel::disableInteractions() {
  sync_on_ui_thread([&] {
    for (auto widget : widgets) {
      gtk_widget_set_sensitive(widget.second, false);
    }
  });
}

void SliControlPanel::enableInteractions() {
  sync_on_ui_thread([&] {
    for (auto widget : widgets) {
      gtk_widget_set_sensitive(widget.second, true);
    }
    gtk_widget_grab_focus(widgets[lastFocused]);
  });
}

void SliControlPanel::reAttach(ViewInterface::WeakPtr viewWeak_) {
  require(Scroom::GtkHelpers::on_ui_thread());

  viewWeak = viewWeak_;

  // Re-assign all widgets to a new hbox and attach it to the sidebar
  sync_on_ui_thread([&] {
    GtkWidget *newHbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    for (GList *iter = gtk_container_get_children(GTK_CONTAINER(hbox));
         iter != nullptr; iter = iter->next) {
      g_object_ref(iter->data);
      gtk_container_remove(GTK_CONTAINER(hbox), GTK_WIDGET(iter->data));
      gtk_container_add(GTK_CONTAINER(newHbox), GTK_WIDGET(iter->data));
      g_object_unref(iter->data);
    }
    hbox = newHbox;
    viewWeak.lock()->addSideWidget("Layers", hbox);
  });
}

SliControlPanel::~SliControlPanel() {
  printf("Multilayer control panel has been destroyed\n");
}

SliControlPanel::Ptr
SliControlPanel::create(ViewInterface::WeakPtr viewWeak_,
                        SliPresentationInterface::WeakPtr presentation_) {
  SliControlPanel::Ptr result =
      SliControlPanel::Ptr(new SliControlPanel(viewWeak_, presentation_));

  return result;
}