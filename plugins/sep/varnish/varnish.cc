#include "varnish.hh"
#include <gdk/gdk.h>
#include <scroom/cairo-helpers.hh>
#include <scroom/viewinterface.hh>

Varnish::Varnish(SliLayer::Ptr layer) {
  this->layer = layer;
  inverted = false;
  // Precalculate the surface and save it.
  int stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, layer->width);
  surface =
      cairo_image_surface_create_for_data(layer->bitmap.get(), CAIRO_FORMAT_A8,
                                          layer->width, layer->height, stride);
  // Map is read inverted by cairo, so we invert it here once
  invertSurface();
}

Varnish::Ptr Varnish::create(SliLayer::Ptr layer) {
  Varnish::Ptr result = Ptr(new Varnish(layer));
  return result;
}

Varnish::~Varnish() { cairo_surface_destroy(surface); }

void Varnish::setView(ViewInterface::WeakPtr viewWeak) {
  registerUI(viewWeak);
  this->viewWeak = viewWeak;
}

void Varnish::resetView(ViewInterface::WeakPtr viewWeak) {
  this->viewWeak = viewWeak;

  gdk_threads_enter();
  GtkWidget *newBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  for (GList *iter = gtk_container_get_children(GTK_CONTAINER(box));
       iter != nullptr; iter = iter->next) {
    gtk_widget_reparent(GTK_WIDGET(iter->data), newBox);
  }
  box = newBox;
  viewWeak.lock()->addSideWidget("Varnish", box);
  gdk_threads_leave();
}

static void varnish_toggled(GtkToggleButton *, gpointer varnishP) {
  // Have a member function sort out the varnishState
  static_cast<Varnish *>(varnishP)->fixVarnishState();
  // Force a redraw when varnish is toggled.
  static_cast<Varnish *>(varnishP)->triggerRedraw();
}

void Varnish::fixVarnishState() {
  if (inverted) {
    // If we're currently inverted and moving to normal,
    // We need to invert back
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_enabled))) {
      invertSurface();
      inverted = false;
    }
  } else {
    // If we're currently normal and moving to normal,
    // We need to invert back
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_inverted))) {
      invertSurface();
      inverted = true;
    }
  }
}

void Varnish::registerUI(ViewInterface::WeakPtr viewWeak) {
  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *expander = gtk_expander_new("Overlay properties");
  GtkWidget *expander_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  colorpicker = gtk_color_selection_new();
  check_show_background = gtk_check_button_new_with_label("Show background");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_show_background), true);
  gtk_color_selection_set_has_palette(GTK_COLOR_SELECTION(colorpicker), false);
  // Set a default color for the overlay
  GdkColor color;
  gdk_color_parse("#000000", &color);
  gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(colorpicker),
                                        &color);
  // Add radio buttons for each display mode
  radio_disabled = gtk_radio_button_new_with_label(NULL, "Disabled");
  radio_enabled = gtk_radio_button_new_with_label_from_widget(
      GTK_RADIO_BUTTON(radio_disabled), "Normal");
  radio_inverted = gtk_radio_button_new_with_label_from_widget(
      GTK_RADIO_BUTTON(radio_disabled), "Inverted");
  // trigger a redraw when disabled is checked/unchecked
  g_signal_connect(static_cast<gpointer>(radio_disabled), "toggled",
                   G_CALLBACK(varnish_toggled), this);
  g_signal_connect(static_cast<gpointer>(radio_enabled), "toggled",
                   G_CALLBACK(varnish_toggled), this);
  g_signal_connect(static_cast<gpointer>(radio_disabled), "toggled",
                   G_CALLBACK(varnish_toggled), this);
  // We can use the same callback to force a redraw when the color is changed
  g_signal_connect(static_cast<gpointer>(colorpicker), "color-changed",
                   G_CALLBACK(varnish_toggled), this);
  g_signal_connect(static_cast<gpointer>(check_show_background), "toggled",
                   G_CALLBACK(varnish_toggled), this);

  // Add all elements into 1 box
  gtk_box_pack_start(GTK_BOX(box), radio_disabled, true, false, 0);
  gtk_box_pack_start(GTK_BOX(box), radio_enabled, true, false, 0);
  gtk_box_pack_start(GTK_BOX(box), radio_inverted, true, false, 0);
  gtk_box_pack_start(GTK_BOX(box), expander, true, false, 0);
  gtk_box_pack_start(GTK_BOX(expander_box), check_show_background, true, false,
                     0);
  gtk_box_pack_start(GTK_BOX(expander_box), colorpicker, true, false, 0);
  gtk_container_add(GTK_CONTAINER(expander), expander_box);
  gtk_widget_show_all(expander);
  gtk_widget_show_all(box);

  // Add the box to the sidebar
  ViewInterface::Ptr view(viewWeak);
  gdk_threads_enter();
  view->addSideWidget("Varnish", box);
  gdk_threads_leave();
}

void Varnish::invertSurface() {
  cairo_surface_flush(surface);
  int width = cairo_image_surface_get_width(surface);
  int height = cairo_image_surface_get_height(surface);
  unsigned char *data = cairo_image_surface_get_data(surface);
  for (int i = 0; i < width * height; i++) {
    // Invert each suface pixel.
    data[i] ^= 255;
  }
  cairo_surface_mark_dirty(surface);
}

void Varnish::drawOverlay(ViewInterface::Ptr const &, cairo_t *cr,
                          Scroom::Utils::Rectangle<double> presentationArea,
                          int zoom) {
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_disabled))) {
    // if the varnish overlay is disabled, return without drawing anything.
    return;
  }
  double pixelSize = pixelSizeFromZoom(zoom);
  GdkRectangle GTKPresArea = presentationArea.toGdkRectangle();
  cairo_save(cr);
  // Disable blurring/anti-ailiasing
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
  cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
  cairo_translate(cr, -GTKPresArea.x * pixelSize, -GTKPresArea.y * pixelSize);
  if (zoom >= 0) {
    cairo_scale(cr, 1 << zoom, 1 << zoom);
  } else {
    cairo_scale(cr, pow(2.0, zoom), pow(2.0, zoom));
  }

  // Read the overlay color and alpha
  GdkColor color;
  gint alpha;
  // This method is deprecated, but the alternative doesn't work
  gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(colorpicker),
                                        &color);
  alpha =
      gtk_color_selection_get_current_alpha(GTK_COLOR_SELECTION(colorpicker));
  // Cairo is expecting doubles as color values, so we have to convert them.
  double r, g, b, a;
  r = color.red / 65535.0;
  g = color.green / 65535.0;
  b = color.blue / 65535.0;
  a = alpha / 65535.0;

  if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_show_background))) {
    // Clear the background
    cairo_rectangle(cr, 0, 0, layer->width, layer->height);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_fill(cr);
  }

  cairo_set_source_rgba(cr, r, g, b, a);
  cairo_mask_surface(cr, surface, 0, 0);

  cairo_restore(cr);
}
