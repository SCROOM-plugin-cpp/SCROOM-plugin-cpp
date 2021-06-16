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
  Varnish(const SliLayer::Ptr &sliLayer);
  GtkWidget *box;
  GtkWidget *radio_enabled;
  GtkWidget *radio_disabled;
  GtkWidget *radio_inverted;
  GtkWidget *check_show_background;
  GtkWidget *colorpicker;
  void invertSurface();
  void registerUI(const ViewInterface::WeakPtr &viewWeakPtr);
  SliLayer::Ptr layer;
  cairo_surface_t *surface;
  bool inverted;

public:
  static Ptr create(const SliLayer::Ptr &layer);
  void setView(const ViewInterface::WeakPtr &viewWeakPtr);
  void resetView(const ViewInterface::WeakPtr &viewWeakPtr);
  void fixVarnishState();

  ~Varnish();
  void drawOverlay(ViewInterface::Ptr const &vi, cairo_t *cr,
                   Scroom::Utils::Rectangle<double> presentationArea, int zoom);
};