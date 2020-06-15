#include "varnish.hh"
#include <scroom/viewinterface.hh>
#include <scroom/cairo-helpers.hh>

Varnish::Varnish(SliLayer::Ptr layer)
{
  this->layer = layer;
  inverted = false;
  // Precalculate the surface and save it.
  int stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, layer->width);
  surface = cairo_image_surface_create_for_data(layer->bitmap, CAIRO_FORMAT_A8, 
                                                                    layer->width, layer->height, stride);
  // Map is read inverted by cairo, so we invert it here once                                                                  
  this->invertSurface();
}

Varnish::Ptr Varnish::create(SliLayer::Ptr layer)
{
  Varnish::Ptr result = Ptr(new Varnish(layer));
  return result;
}

Varnish::~Varnish()
{
  free(layer->bitmap);
  cairo_surface_destroy(surface);
}

void Varnish::setView(ViewInterface::WeakPtr viewWeak)
{
  registerButton(viewWeak);
  this->viewWeak = viewWeak;
}

static void varnish_toggled(GtkToggleButton* button, gpointer varnishP)
{
  // Have a member function sort out the varnishState
  static_cast<Varnish*>(varnishP)->fixVarnishState();
  // Force a redraw when varnish is toggled.
  static_cast<Varnish*>(varnishP)->forceRedraw();
}

void Varnish::forceRedraw()
{
  ViewInterface::Ptr view(this->viewWeak);
  view->invalidate();
}

void Varnish::fixVarnishState()
{
  if (inverted)
  {
    // If we're currently inverted and moving to normal,
    // We need to invert back
    if (GTK_TOGGLE_BUTTON(radio_enabled)->active)
    {
      this->invertSurface();
      inverted = false;
    }
  } else
  {
    // If we're currently normal and moving to normal,
    // We need to invert back
    if (GTK_TOGGLE_BUTTON(radio_inverted)->active)
    {
      this->invertSurface();
      inverted = true;
    }
  }
}

void Varnish::registerButton(ViewInterface::WeakPtr viewWeak)
{
  GtkWidget *box = gtk_vbox_new(TRUE, 0);
  radio_disabled = gtk_radio_button_new_with_label(NULL, "Disabled");
  radio_enabled = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_disabled), "Enabled");
  radio_inverted = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_disabled), "Inverted");
  ViewInterface::Ptr view(viewWeak);
  // trigger a redraw when disabled is checked/unchecked
  g_signal_connect(static_cast<gpointer>(radio_disabled), "toggled", G_CALLBACK(varnish_toggled), this);
  g_signal_connect(static_cast<gpointer>(radio_enabled), "toggled", G_CALLBACK(varnish_toggled), this);
  g_signal_connect(static_cast<gpointer>(radio_disabled), "toggled", G_CALLBACK(varnish_toggled), this);

  // Add all buttons into 1 box
  gtk_box_pack_start_defaults(GTK_BOX(box), radio_disabled);
  gtk_box_pack_start_defaults(GTK_BOX(box), radio_enabled);
  gtk_box_pack_start_defaults(GTK_BOX(box), radio_inverted);
  gtk_widget_show_all(box);

  // Add the box to the sidebar
  gdk_threads_enter();
  view->addSideWidget("Varnish", box);
  gdk_threads_leave();
}

void Varnish::invertSurface()
{
  cairo_surface_flush(surface);
  int width = cairo_image_surface_get_width(surface);
  int height = cairo_image_surface_get_height(surface);
  unsigned char *data = cairo_image_surface_get_data(surface);
  for (int i = 0; i < width * height; i++)
  {
    // TODO; this assumes 8 bpp
    data[i] ^= 255;
  }
  cairo_surface_mark_dirty(surface);
}

void Varnish::drawOverlay(ViewInterface::Ptr const &vi, cairo_t *cr,
              Scroom::Utils::Rectangle<double> presentationArea, int zoom)
{
  if (GTK_TOGGLE_BUTTON(radio_disabled)->active)
  {
    // if the varnish is disabled, return without drawing anything.
    return;
  }
  double pixelSize = pixelSizeFromZoom(zoom);
  GdkRectangle GTKPresArea = presentationArea.toGdkRectangle();
  cairo_save(cr);
  // Disable blurring/anti-ailiasing
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
  cairo_translate(cr, -GTKPresArea.x*pixelSize,-GTKPresArea.y*pixelSize);
  if(zoom >= 0)
  {
    cairo_scale(cr, 1<<zoom, 1<<zoom);
  } else {
    cairo_scale(cr, pow(2.0, zoom), pow(2.0, zoom));
  }

  /** Overlay color: 20FC8F */
  cairo_set_source_rgba(cr, 32.0/255.0, 252.0/255.0, 143.0/255.0, 1.0);
  cairo_mask_surface(cr, surface, 0, 0);
  cairo_restore(cr);
}