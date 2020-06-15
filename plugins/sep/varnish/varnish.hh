#include "gtk/gtk.hh"

class Varnish
{
public:
  typedef boost::shared_ptr<SliLayer> Ptr;

private:
  Varnish(uint8_t* bitmap, int width, int height, ViewInterface::WeakPtr view);
  GtkWidget * varnishToggle;
  void registerButton(ViewInterface::WeakPtr view);

  int height;
  int width;
  uint8_t* bitmap;

public:
  static Ptr create(uint8_t* bitmap, int width, int height, ViewInterface::WeakPtr view);

  ~Varnish();
  void drawOverlay(ViewInterface::Ptr const &vi, cairo_t *cr,
              Scroom::Utils::Rectangle<double> presentationArea, int zoom);
};