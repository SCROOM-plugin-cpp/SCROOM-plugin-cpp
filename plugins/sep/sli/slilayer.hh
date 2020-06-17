#pragma once

#include <scroom/scroominterface.hh>

class SliLayer : public virtual Scroom::Utils::Base
{
public:
  typedef boost::shared_ptr<SliLayer> Ptr;

  /** Height of the layer (in pixels) */
  int height;

  /** Width of the layer (in pixels) */
  int width;
  
  /** Samples per pixel */
  unsigned int spp = 0;

  /** Bits per sample */
  unsigned int bps = 0;

  /** The 'x' part of the aspect ratio x:y */
  float xAspect=0;

  /** The 'y' part of the aspect ratio x:y */
  float yAspect=0;

  /** Horizontal offset from the top-left point of the canvas (in pixels) */
  int xoffset;

  /** Vertical offset from the top-left point of the canvas (in pixels) */
  int yoffset;

  /** Filename without extension; used as an ID */
  std::string name;

  /** Absolute filepath to the tiff/sep file */
  std::string filepath;

  /** The memory chunk containing the bitmap */
  uint8_t* bitmap;

private:
  SliLayer();

public:
  /** Constructor */
  static Ptr create(const std::string &filepath, const std::string &name, int xoffset, int yoffset);

  /** Destructor */
  virtual ~SliLayer();

  /** Returns the Rectangle representation of the layer (in pixels) */
  virtual Scroom::Utils::Rectangle<int> toRectangle();

};