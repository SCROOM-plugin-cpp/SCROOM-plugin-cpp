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
}

static void varnish_toggled(GtkToggleButton* button, gpointer data)
{
  // Force a redraw when varnish is toggled.
  static_cast<ViewInterface*>(data)->invalidate();
}

void Varnish::registerButton(ViewInterface::WeakPtr viewWeak)
{
  varnishToggle = gtk_toggle_button_new_with_label("Varnish");
  ViewInterface::Ptr view(viewWeak);
  g_signal_connect(static_cast<gpointer>(varnishToggle), "toggled", G_CALLBACK(varnish_toggled), view.get());

  // Add everything to the sidebar
  gdk_threads_enter();
  view->addSideWidget("Varnish", varnishToggle);
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
  if (!GTK_TOGGLE_BUTTON(varnishToggle)->active)
  {
    // if the varnish toggle button is disabled, return without drawing anything.
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
  if (inverted)
  {

  } else
  {
    // TODO; Add a sub cairo_t to add:
    //    * Color
    //    * Crop to canvas size
   // cairo_set_operator(cr, CAIRO_OPERATOR_DEST_ATOP);
  }
  
  cairo_mask_surface(cr, surface, 0, 0);
  cairo_restore(cr);
}