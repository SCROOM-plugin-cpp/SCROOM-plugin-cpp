#include "varnish.hh"
#include <gdk/gdk.h>
#include <scroom/cairo-helpers.hh>
#include <scroom/viewinterface.hh>

Varnish::Varnish(const SliLayer::Ptr &sliLayer) {
  this->layer = sliLayer;
  inverted = false;
  // Precalculate the surface and save it.
  int stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, sliLayer->width);
  surface = cairo_image_surface_create_for_data(
      sliLayer->bitmap.get(), CAIRO_FORMAT_A8, sliLayer->width,
      sliLayer->height, stride);
  // Map is read inverted by cairo, so we invert it here once
  invertSurface();
}

Varnish::Ptr Varnish::create(const SliLayer::Ptr &layer) {
  Varnish::Ptr result = Ptr(new Varnish(layer));
  return result;
}

Varnish::~Varnish() { cairo_surface_destroy(surface); }

void Varnish::setView(const ViewInterface::WeakPtr &viewWeakPtr) {
  registerUI(viewWeakPtr);
  this->viewWeak = viewWeakPtr;
}

void Varnish::resetView(const ViewInterface::WeakPtr &viewWeakPtr) {
  require(Scroom::GtkHelpers::on_ui_thread());

  this->viewWeak = viewWeakPtr;

  Scroom::GtkHelpers::sync_on_ui_thread([&] {
    GtkWidget *newBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    for (GList *iter = gtk_container_get_children(GTK_CONTAINER(box));
         iter != nullptr; iter = iter->next) {

      g_object_ref(iter->data);
      gtk_container_remove(GTK_CONTAINER(box), GTK_WIDGET(iter->data));
      gtk_container_add(GTK_CONTAINER(newBox), GTK_WIDGET(iter->data));
      g_object_unref(iter->data);
    }
    box = newBox;
    viewWeakPtr.lock()->addSideWidget("Varnish", box);
  });
}

static void varnish_toggled(GtkToggleButton *, gpointer varnishP) {
  // Have a member function sort out the varnishState
  static_cast<Varnish *>(varnishP)->fixVarnishState();
  // Force a redraw when varnish is toggled.
  static_cast<Varnish *>(varnishP)->triggerRedraw();
}

void color_changed(GObject *, GParamSpec *, gpointer varnishP) {
  varnish_toggled(nullptr, varnishP);
}

void Varnish::fixVarnishState() {
  require(Scroom::GtkHelpers::on_ui_thread());

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

void Varnish::registerUI(const ViewInterface::WeakPtr &viewWeakPtr) {
  require(Scroom::GtkHelpers::on_ui_thread());

  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *expander = gtk_expander_new("Overlay properties");
  GtkWidget *expander_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  colorpicker = gtk_color_chooser_widget_new();
  g_object_set(G_OBJECT(colorpicker), "show-editor", true, NULL);
  check_show_background = gtk_check_button_new_with_label("Show background");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_show_background), true);
  // Set a default color for the overlay
  GdkRGBA color;
  gdk_rgba_parse(&color, "#000000");
  gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(colorpicker), true);
  gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(colorpicker), &color);
  // Add radio buttons for each display mode
  radio_disabled = gtk_radio_button_new_with_label(nullptr, "Disabled");
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
  g_signal_connect(static_cast<gpointer>(colorpicker), "notify::rgba",
                   G_CALLBACK(color_changed), this);
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
  ViewInterface::Ptr view(viewWeakPtr);
  Scroom::GtkHelpers::sync_on_ui_thread(
      [&] { view->addSideWidget("Varnish", box); });
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
  require(Scroom::GtkHelpers::on_ui_thread());

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_disabled))) {
    // if the varnish overlay is disabled, return without drawing anything.
    return;
  }
  double pixelSize = pixelSizeFromZoom(zoom);
  cairo_save(cr);
  // Disable blurring/anti-aliasing
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
  cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
  cairo_translate(cr, -presentationArea.getLeft() * pixelSize,
                  -presentationArea.getTop() * pixelSize);
  cairo_scale(cr, pixelSize, pixelSize);

  // Read the overlay color and alpha
  GdkRGBA color;
  gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(colorpicker), &color);

  if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_show_background))) {
    // Clear the background
    cairo_rectangle(cr, 0, 0, layer->width, layer->height);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_fill(cr);
  }

  cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
  cairo_mask_surface(cr, surface, 0, 0);

  cairo_restore(cr);
}
