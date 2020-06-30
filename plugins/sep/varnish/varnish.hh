#pragma once

#include "../sli/slilayer.hh"
#include <gtk/gtk.h>

class Varnish {
public:
  typedef boost::shared_ptr<Varnish> Ptr;

  /** Callback to trigger a redraw of the presentation */
  boost::function<void()> triggerRedraw;

  ViewInterface::WeakPtr viewWeak;

private:
  Varnish(SliLayer::Ptr layer);
  GtkWidget *box;
  GtkWidget *radio_enabled;
  GtkWidget *radio_disabled;
  GtkWidget *radio_inverted;
  GtkWidget *check_show_background;
  GtkWidget *colorpicker;
  void invertSurface();
  void registerUI(ViewInterface::WeakPtr view);
  SliLayer::Ptr layer;
  cairo_surface_t *surface;
  bool inverted;

public:
  static Ptr create(SliLayer::Ptr layer);
  void setView(ViewInterface::WeakPtr view);
  void resetView(ViewInterface::WeakPtr view);
  void fixVarnishState();

  ~Varnish();
  void drawOverlay(ViewInterface::Ptr const &vi, cairo_t *cr,
                   Scroom::Utils::Rectangle<double> presentationArea, int zoom);
};