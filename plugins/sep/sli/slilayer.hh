#pragma once

#include <tiffio.h>
#include <iostream>

#include <scroom/scroominterface.hh>
#include <scroom/global.hh>


// Keep it simple for now and hardcode the allowed parameters
#define BPS 8
#define SPP 4

class SliLayer : public virtual Scroom::Utils::Base
{

public:
  // TODO does an SliLayer need to be a pointer?
  typedef boost::shared_ptr<SliLayer> Ptr;

  /** Whether or not the layer is toggled to be visible */
  bool visible;

private:
  int height;
  int width;
  int spp;
  int bps;
  int xoffset;
  int yoffset;
  std::string name;     // filename without extension; used as an ID
  std::string filepath; // absolute filepath to the tiff/sep file
  // The actual bitmap, the elements of the vector are pointers to the rows of the bitmap
  uint8_t* bitmap;

private:
  SliLayer();
  virtual bool load(const std::string &filepath);

public:
  static Ptr create(const std::string &filepath, const std::string &name, int xoffset, int yoffset);
  virtual ~SliLayer();
  int getHeight() { return height; };
  int getWidth() { return width; };
  int getXoffset() { return xoffset; };
  int getYoffset() { return yoffset; };
  std::string getName() { return name; };
  std::string getFilepath() { return filepath; };
  uint8_t *getBitmap() { return bitmap; };
};