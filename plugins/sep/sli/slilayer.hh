#pragma once

#include <memory>

#include <scroom/scroominterface.hh>

class SliLayer : public virtual Scroom::Utils::Base {
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
  float xAspect = 0;

  /** The 'y' part of the aspect ratio x:y */
  float yAspect = 0;

  /** Horizontal offset from the top-left point of the canvas (in pixels) */
  int xoffset;

  /** Vertical offset from the top-left point of the canvas (in pixels) */
  int yoffset;

  /** Filename of the layer. Used as an identifier in the TreeView */
  std::string name;

  /** Absolute filepath to the tiff/sep file */
  std::string filepath;

  /** The memory chunk containing the bitmap */
  std::unique_ptr<uint8_t[]> bitmap;

private:
  SliLayer();

public:
  /** Constructor */
  static Ptr create(const std::string &filepath, const std::string &name,
                    int xoffset, int yoffset);

  /** Destructor */
  ~SliLayer() override = default;

  /** Returns the Rectangle representation of the layer (in pixels) */
  virtual Scroom::Utils::Rectangle<int> toRectangle();

  /**
   * Reads the layers tiff file and populates the layer with all contained
   * attributes except for the bitmap data
   * @param allowedBps the bits per sample that the TIFF file is allowed to have
   * @param allowedSpp the samples per pixel that the TIFF file is allowed to
   * have
   * @return true if all attributes are OK, false if not
   * */
  virtual bool fillMetaFromTiff(unsigned int allowedBps,
                                unsigned int allowedSpp);

  /**
   * Copies the TIFF files' bitmap data into the layer
   * Requires fillMetaFromTiff() to have been called previously
   */
  virtual void fillBitmapFromTiff();
};
