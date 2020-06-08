#pragma once

#include <scroom/scroominterface.hh>

// Keep it simple for now and hardcode the allowed parameters
#define BPS 8
#define SPP 4

class SliLayer : public virtual Scroom::Utils::Base
{

public:
  typedef boost::shared_ptr<SliLayer> Ptr;

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

  /** Whether or not the layer is toggled to be visible */
  bool visible;

private:
  SliLayer();
  virtual bool load(const std::string &filepath);

public:
  static Ptr create(const std::string &filepath, const std::string &name, int xoffset, int yoffset);
  virtual ~SliLayer();

};