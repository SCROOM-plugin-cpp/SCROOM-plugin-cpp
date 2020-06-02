#include "naive-bitmap.hh"

#define TIFFGetFieldChecked(file, field, ...) \
	if(1!=TIFFGetField(file, field, ##__VA_ARGS__)) \
	  throw std::invalid_argument("Field not present in tiff file: " #field);

NaiveBitmap::Ptr NaiveBitmap::create(TIFF* tif)
{
  NaiveBitmap::Ptr result(new NaiveBitmap());
  result->load(tif);
  return result;
}

NaiveBitmap::NaiveBitmap(): height(0), width(0)
{
}

NaiveBitmap::~NaiveBitmap()
{
  for (uint8_t* row: bitmap)
  {
    free(row);
  }
}

bool NaiveBitmap::load(TIFF* tif)
{
  TIFFGetFieldChecked(tif, TIFFTAG_IMAGELENGTH, &height);
  TIFFGetFieldChecked(tif, TIFFTAG_IMAGEWIDTH, &width);
  // Could just use the width here but we don't want to make overflows too easy, right ;)
  int byteWidth = TIFFScanlineSize(tif);

  // Iterate over the rows and copy the bitmap data to newly allocated memory pointed to by currentBitmap
  for (int row = 0; row < height; row++)
  {
    uint8_t* currentRow = static_cast<uint8_t*>(malloc(byteWidth*sizeof(uint8_t)));
    TIFFReadScanline(tif, currentRow, row);
    bitmap.push_back(currentRow);
  }
  return true;
}