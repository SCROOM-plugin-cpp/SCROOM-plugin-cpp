#pragma once

#include <string>
#include <map>
#include <list>
#include <set>
#include <fstream>

#include <scroom/observable.hh>
#include <scroom/presentationinterface.hh>
#include <scroom/scroominterface.hh>
#include <scroom/transformpresentation.hh>
#include <scroom/tiledbitmapinterface.hh>
#include <scroom/tiledbitmaplayer.hh>

#include "naive-bitmap.hh"

class SliLayer
{
  public:
    std::string filepath; // absolute filepath to the tiff/sep file
    std::string name; // filename without extension; used as an ID
    int xoffset;
    int yoffset;
};

class SliPresentation : public PresentationBase,
                        public virtual Scroom::Utils::Base
{
public:
  typedef boost::shared_ptr<SliPresentation> Ptr;

private:
  typedef std::set<ViewInterface::WeakPtr> Views;
  std::map<std::string, std::string> properties;
  Views views;
  std::vector<NaiveBitmap::Ptr> bitmaps;
  ScroomInterface::Ptr scroomInterface;
  int bpp;

private:
  SliPresentation(ScroomInterface::Ptr scroomInterface);
  virtual void extractBitmap(TiledBitmapInterface::Ptr tiledBitmap, NaiveBitmap::Ptr naiveBitmap);
  int Xresolution;
  int Yresolution;
  std::vector<SliLayer> layers;

public:
  virtual ~SliPresentation();

  static Ptr create(ScroomInterface::Ptr scroomInterface);

  virtual bool load(const std::string& fileName);

  virtual void parseSli(const std::string &fileName);

  virtual const std::vector<SliLayer>& getLayers();

  ////////////////////////////////////////////////////////////////////////
  // PresentationInterface
  ////////////////////////////////////////////////////////////////////////

  virtual Scroom::Utils::Rectangle<double> getRect();
  virtual void redraw(ViewInterface::Ptr const& vi, cairo_t* cr, Scroom::Utils::Rectangle<double> presentationArea, int zoom);
  virtual bool getProperty(const std::string& name, std::string& value);
  virtual bool isPropertyDefined(const std::string& name);
  virtual std::string getTitle();

  ////////////////////////////////////////////////////////////////////////
  // PresentationBase
  ////////////////////////////////////////////////////////////////////////

  virtual void viewAdded(ViewInterface::WeakPtr viewInterface);
  virtual void viewRemoved(ViewInterface::WeakPtr vi);
  virtual std::set<ViewInterface::WeakPtr> getViews();

};
