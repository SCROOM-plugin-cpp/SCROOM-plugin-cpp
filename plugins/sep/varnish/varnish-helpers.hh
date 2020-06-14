#ifndef _varnishhelpers_HH_
#define _varnishhelpers_HH_

#include <scroom/bitmap-helpers.hh>
#include <scroom/colormappable.hh>

void redrawVarnishOverlay(PresentationInterface::Ptr varnishPI, ViewInterface::Ptr const &vi, cairo_t *cr,
                              Scroom::Utils::Rectangle<double> presentationArea, int zoom);

#endif