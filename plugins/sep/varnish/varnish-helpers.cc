#include "varnish-helpers.hh"

void redrawVarnishOverlay(PresentationInterface::Ptr varnishPI, ViewInterface::Ptr const &vi, cairo_t *cr,
              Scroom::Utils::Rectangle<double> presentationArea, int zoom)
{
  using Scroom::Bitmap::BitmapSurface;
  // For now we assume there is only one presentation, and ours will be the last one
  // to display something

  // Create a new CR to draw our overlay on
  GdkRectangle GTKpresentationArea = presentationArea.toGdkRectangle();

  GdkRectangle viewArea;
  viewArea.x=0;
  viewArea.y=0;

  if(zoom > 0)
  {
    const int pixelSize = 1<<zoom;
    viewArea.width = GTKpresentationArea.width*pixelSize;
    viewArea.height = GTKpresentationArea.height*pixelSize;
  }
  else
  {
    const int pixelSize = 1<<-zoom;
    viewArea.width = GTKpresentationArea.width/pixelSize;
    viewArea.height = GTKpresentationArea.height/pixelSize;
  }
  BitmapSurface::Ptr s = BitmapSurface::create(viewArea.width, viewArea.height, CAIRO_FORMAT_ARGB32);
  cairo_surface_t* surface = s->get();
  cairo_t* cr_overlay = cairo_create(surface);

  // TODO; Edit the overlay colormap (This only works on cmyk tiffs)

  varnishPI->redraw(vi, cr_overlay, GTKpresentationArea, zoom);
  cairo_set_source_surface(cr, surface, 0, 0);
  // TODO (later); Figure out if the color map needs to be updated (alpha sliders?)
  cairo_paint_with_alpha(cr, 0.5); // Set to 1 if background is transparent
  cairo_destroy(cr_overlay);
}