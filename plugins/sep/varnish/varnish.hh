#ifndef _varnish_HH
#define _varnish_HH
#include <gtk/gtk.h>
#include "../sli/slilayer.hh"

class Varnish
{
public:
  typedef boost::shared_ptr<Varnish> Ptr;

private:
  Varnish(SliLayer::Ptr layer);
  GtkWidget *varnishToggle;
  void registerButton(ViewInterface::WeakPtr view);
  SliLayer::Ptr layer;
  bool inverted;

public:
  static Ptr create(SliLayer::Ptr layer);
  void setView(ViewInterface::WeakPtr view);

  ~Varnish();
  void drawOverlay(ViewInterface::Ptr const &vi, cairo_t *cr,
              Scroom::Utils::Rectangle<double> presentationArea, int zoom);
};

#endif