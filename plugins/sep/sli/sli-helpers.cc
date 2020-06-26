#include "sli-helpers.hh"
#include "../sep-helpers.hh"

SurfaceWrapper::Ptr SurfaceWrapper::create() {
  SurfaceWrapper::Ptr result(new SurfaceWrapper());

  return result;
}

SurfaceWrapper::Ptr SurfaceWrapper::create(int width, int height,
                                           cairo_format_t format) {
  SurfaceWrapper::Ptr result(new SurfaceWrapper(width, height, format));

  return result;
}

SurfaceWrapper::SurfaceWrapper() {
  clear = true;
  empty = true;
}

SurfaceWrapper::SurfaceWrapper(int width, int height, cairo_format_t format) {
  int stride = cairo_format_stride_for_width(format, width);
  uint8_t *bitmap = static_cast<uint8_t *>(calloc(height * stride, 1));
  surface = cairo_image_surface_create_for_data(bitmap, CAIRO_FORMAT_ARGB32,
                                                width, height, stride);
  empty = false;
  clear = true;
}

int SurfaceWrapper::getHeight() {
  return cairo_image_surface_get_height(surface);
}

int SurfaceWrapper::getWidth() {
  return cairo_image_surface_get_width(surface);
}

int SurfaceWrapper::getStride() {
  return cairo_image_surface_get_stride(surface);
}

uint8_t *SurfaceWrapper::getBitmap() {
  return cairo_image_surface_get_data(surface);
}

Scroom::Utils::Rectangle<int> SurfaceWrapper::toRectangle() {
  Scroom::Utils::Rectangle<int> rect{0, 0, getWidth(), getHeight()};

  return rect;
}

void SurfaceWrapper::clearSurface() {
  cairo_t *cr = cairo_create(surface);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_destroy(cr);
  clear = true;
}

void SurfaceWrapper::clearSurface(Scroom::Utils::Rectangle<int> rect) {
  cairo_t *cr = cairo_create(surface);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_rectangle(cr, rect.getTopLeft().x, rect.getTopLeft().y, rect.getWidth(),
                  rect.getHeight());
  cairo_fill(cr);
  cairo_destroy(cr);
  clear = true;
}

Scroom::Utils::Rectangle<int>
toBytesRectangle(Scroom::Utils::Rectangle<int> rect, int bpp) {
  Scroom::Utils::Rectangle<int> bytesRect{rect.getLeft() * bpp, rect.getTop(),
                                          rect.getWidth() * bpp,
                                          rect.getHeight()};

  return bytesRect;
}

int getArea(Scroom::Utils::Rectangle<int> rect) {
  return rect.getHeight() * rect.getWidth();
}

int pointToOffset(Scroom::Utils::Point<int> p, int stride) {
  return p.y * stride + p.x;
}

int pointToOffset(Scroom::Utils::Rectangle<int> rect,
                  Scroom::Utils::Point<int> p) {
  return std::max(0, (p.y - rect.getTop()) * rect.getWidth() +
                         (p.x - rect.getLeft()));
}

Scroom::Utils::Rectangle<int>
spannedRectangle(boost::dynamic_bitset<> bitmap,
                 std::vector<SliLayer::Ptr> layers, bool fromOrigin) {
  int min_x0 = INT_MAX;
  int min_y0 = INT_MAX;
  int max_x1 = INT_MIN;
  int max_y1 = INT_MIN;

  if (fromOrigin) {
    min_x0 = 0;
    min_y0 = 0;
  }

  for (size_t i = 0; i < bitmap.size(); i++) {
    if (!bitmap[i])
      continue;

    auto rect = layers[i]->toRectangle();

    if (rect.getLeft() < min_x0)
      min_x0 = rect.getLeft();

    if (rect.getTop() < min_y0)
      min_y0 = rect.getTop();

    if (rect.getRight() > max_x1)
      max_x1 = rect.getRight();

    if (rect.getBottom() > max_y1)
      max_y1 = rect.getBottom();
  }

  Scroom::Utils::Rectangle<int> rect{min_x0, min_y0, max_x1 - min_x0,
                                     max_y1 - min_y0};

  return rect;
}

int findBestSegFit(unsigned int nSegments, unsigned int height)
{
  int i = 1;
  while (i * nSegments <= height)
  {
    if (i * 2 * nSegments <= height)
    {
      i *= 2;
      continue;
    }

    break;
  }

  return i;
}

boost::dynamic_bitset<> halfSegBitmask(boost::dynamic_bitset<> toggledSegments)
{
  auto nSegments = toggledSegments.size();
  int first = -1, last = -1;
  boost::dynamic_bitset<> bitmask {nSegments};
  for (unsigned int i = 0; i < nSegments; i++)
  {
    if (toggledSegments[i] && first == -1)
      first = (int)i;

    if(toggledSegments[i])
      last = (int)i;
  }

  for (unsigned int i = first; i < first + (unsigned int)((last-first)/2); i++)
    bitmask.set(i);

  return bitmask;
}

SurfaceWrapper::~SurfaceWrapper() {
  if (!empty) {
    free(cairo_image_surface_get_data(surface));
    cairo_surface_destroy(surface);
  }
}