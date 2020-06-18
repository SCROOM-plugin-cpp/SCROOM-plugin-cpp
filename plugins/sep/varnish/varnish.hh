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
  GtkWidget *radio_enabled;
  GtkWidget *radio_disabled;
  GtkWidget *radio_inverted;
  GtkWidget *check_show_background;
  GtkWidget *colorpicker;
  ViewInterface::WeakPtr viewWeak;
  void invertSurface();
  void registerButton(ViewInterface::WeakPtr view);
  SliLayer::Ptr layer;
  cairo_surface_t* surface;
  bool inverted;

public:
  static Ptr create(SliLayer::Ptr layer);
  void setView(ViewInterface::WeakPtr view);
  void fixVarnishState(); //TODO; This probably shouldn't be public?
  void forceRedraw();

  ~Varnish();
  void drawOverlay(ViewInterface::Ptr const &vi, cairo_t *cr,
              Scroom::Utils::Rectangle<double> presentationArea, int zoom);
};

#endif