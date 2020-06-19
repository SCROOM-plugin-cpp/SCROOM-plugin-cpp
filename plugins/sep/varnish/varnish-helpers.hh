#ifndef _varnishhelpers_HH_
#define _varnishhelpers_HH_

#include <cairo.h>

#include <scroom/utilities.hh>
#include <scroom/rectangle.hh>
#include <scroom/point.hh>
#include <scroom/bitmap-helpers.hh>
#include <scroom/colormappable.hh>

#include "../sli/slilayer.hh"

bool fillVarnishOverlay(SliLayer::Ptr layer);
#endif