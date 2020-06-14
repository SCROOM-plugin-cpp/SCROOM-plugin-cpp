#ifndef _varnishhelpers_HH_
#define _varnishhelpers_HH_

#include <cairo.h>

#include <scroom/utilities.hh>
#include <scroom/rectangle.hh>
#include <scroom/point.hh>
#include <scroom/bitmap-helpers.hh>
#include <scroom/colormappable.hh>

#include "../sli/slilayer.hh"

void redrawVarnishOverlay(SliLayer::Ptr layer, ViewInterface::Ptr const &vi, cairo_t *cr,
                              Scroom::Utils::Rectangle<double> presentationArea, int zoom);

void fillVarnishOverlay(SliLayer::Ptr layer);
#endif