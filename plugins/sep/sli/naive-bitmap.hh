#include <tiffio.h>

#include <scroom/scroominterface.hh>
#include <scroom/global.hh>

class NaiveBitmap : public virtual Scroom::Utils::Base
{

public:
  typedef boost::shared_ptr<NaiveBitmap> Ptr;

private:
  int height;
  int width;
  int spp;
  int bps;
  // The actual bitmap, the elements of the vector are pointers to the rows of the bitmap
  std::vector<uint8_t*> bitmap;

private:
  NaiveBitmap();
  virtual bool load(TIFF* tif);

public:
  static Ptr create(TIFF* tif);
  virtual ~NaiveBitmap();
  int getHeight(){return height;};
  int getWidth(){return width;};
  std::vector<uint8_t*>* getBitmap(){return &bitmap;};
};