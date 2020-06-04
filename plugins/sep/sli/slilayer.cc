#include "slilayer.hh"

#define TIFFGetFieldChecked(file, field, ...)        \
  if (1 != TIFFGetField(file, field, ##__VA_ARGS__)) \
    throw std::invalid_argument("Field not present in tiff file: " #field);

SliLayer::Ptr SliLayer::create(const std::string &filepath,
                              const std::string &name, int xoffset, int yoffset)
{
  SliLayer::Ptr layer(new SliLayer());
  layer->filepath = filepath;
  layer->name = name;
  layer->xoffset = xoffset;
  layer->yoffset = yoffset;
  layer->load(filepath);
  return layer;
}

SliLayer::SliLayer() : height(0), width(0)
{}

SliLayer::~SliLayer()
{
  free(bitmap);
}

// TODO set the instance bps, spp values too at some point
bool SliLayer::load(const std::string &filepath)
{
  try
  {
    TIFF *tif = TIFFOpen(filepath.c_str(), "r");
    if (!tif)
    {
      printf("PANIC: Failed to open file %s\n", filepath.c_str());
      return false;
    }

    uint16 spp_ = 0;
    if (1 != TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp_))
      spp_ = 1; // Default value, according to tiff spec
    if (spp_ != SPP)
    {
      printf("PANIC: Samples per pixel is not %d, but %d. Giving up\n", SPP, spp_);
      return false;
    }

    uint16 bps_ = 0;
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps_);
    if (bps_ != BPS)
    {
      printf("PANIC: Bits per sample is not %d, but %d. Giving up\n", BPS, bps_);
      return false;
    }

    float resolutionX;
    float resolutionY;
    uint16 resolutionUnit;

    if (TIFFGetField(tif, TIFFTAG_XRESOLUTION, &resolutionX) &&
        TIFFGetField(tif, TIFFTAG_YRESOLUTION, &resolutionY) &&
        TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &resolutionUnit))
    {
      if (resolutionUnit != RESUNIT_NONE)
      {
        // Fix aspect ratio only
        float base = std::max(resolutionX, resolutionY);
        resolutionX = resolutionX / base;
        resolutionY = resolutionY / base;
      }
    }
    else
    {
      resolutionX = 1;
      resolutionY = 1;
    }

    TIFFGetFieldChecked(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetFieldChecked(tif, TIFFTAG_IMAGELENGTH, &height);
    printf("This bitmap has size %d*%d, aspect ratio %.1f*%.1f\n",
           width, height, 1 / resolutionX, 1 / resolutionY);

    // create sli bitmap ------------------------------------
    // Could just use the width here but we don't want to make overflows too easy, right ;)
    int byteWidth = TIFFScanlineSize(tif);
    bitmap = static_cast<uint8_t*>(malloc(byteWidth * height));
    int stride = width*SPP;

    // Iterate over the rows and copy the bitmap data to newly allocated memory pointed to by currentBitmap
    for (int row = 0; row < height; row++)
    {
      TIFFReadScanline(tif, bitmap + row*stride, row);
    }
    
    TIFFClose(tif);
  }
  catch (const std::exception &ex)
  {
    printf("PANIC: %s\n", ex.what());
    return false;
  }
  
  return true;
}
