#include "varnish.hh"
#include <scroom/viewinterface.hh>

Varnish::Varnish(uint8_t* bitmap, int width, int height, ViewInterface::WeakPtr view)
{
  this->bitmap = bitmap;
  this->width = width;
  this->heigth = height;
  registerButton(view);
}

Varnish::Ptr Varnish::create(uint8_t* bitmap, int width, int height, ViewInterface::WeakPtr view)
{
  Varnish::Ptr result = Ptr(new Varnish(bitmap, width, height, view));
  return result;

}

Varnish::~Varnish()
{
  free(bitmap);
}

Varnish::registerButton(ViewInterface::WeakPtr viewWeak)
{
  GTKWidget *varnishToggle = gtk_toggle_button_new_with_label("Varnish");

  // Add everything to the sidebar
  ViewInterface::Ptr view(viewWeak);
  gdk_threads_enter();
  view->addSideWidget("Varnish", varnishToggle);
  gdk_threads_leave();
}

Varnish::drawOverlay(ViewInterface::Ptr const &vi, cairo_t *cr,
              Scroom::Utils::Rectangle<double> presentationArea, int zoom)
{
  if (!gtk_toggle_button_get_mode(varnishToggle))
  {
    // if the varnish toggle button is disabled, return without drawing anything.
    return;
  }
  int stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, width);
  cairo_surface_t* varnishSurface = cairo_image_surface_create_for_data(bitmap, CAIRO_FORMAT_A8, 
                                                                    width, height, stride);
  double pixelSize = pixelSizeFromZoom(zoom);
  GdkRectangle GTKPresArea = presentationArea.toGdkRectangle();
  cairo_save(cr);
  cairo_translate(cr, -GTKPresArea.x*pixelSize,-GTKPresArea.y*pixelSize);
  if(zoom >= 0)
  {
    cairo_scale(cr, 1<<zoom, 1<<zoom);
  } else {
    cairo_scale(cr, pow(2.0, zoom), pow(2.0, zoom));
  }
  cairo_set_source_surface(cr, varnishSurface, 0, 0);
  cairo_paint(cr);
  cairo_surface_destroy(varnishSurface);
  cairo_restore(cr);
}