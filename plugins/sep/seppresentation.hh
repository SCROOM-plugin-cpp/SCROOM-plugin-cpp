#pragma once

#include <string>
#include <map>
#include <set>
#include <iostream>
#include <stdlib.h>
#include <fstream>

#include <tiffio.h>
#include <boost/algorithm/string/trim.hpp>
// #include "tiff.hh"
// #include "../../scroom/gui/src/view.hh"
#include "../../../scroom/gui/src/loader.hh"

#include <scroom/presentationinterface.hh>
#include <scroom/scroominterface.hh>
#include <scroom/tiledbitmapinterface.hh>
#include <scroom/transformpresentation.hh>
#include "scroom/tile.hh"

//typedef struct Sep Sep;

/**
 * The motivation for having this class and not implement SourcePresentation directly in SepPresentation
 * is to avoid a memory leak through cyclic dependencies
 */
class SepSource: public SourcePresentation
{
public:
  typedef boost::shared_ptr<SepSource> Ptr;

private:
  SepSource();

public:
  ~SepSource();
  static Ptr create();

  ////////////////////////////////////////////////////////////////////////
  // SourcePresentation
  ////////////////////////////////////////////////////////////////////////
  virtual void fillTiles(int startLine, int lineCount, int tileWidth, int firstTile, std::vector<Tile::Ptr>& tiles);
  virtual void done();

};


class SepPresentation : public PresentationBase,
                        public virtual Scroom::Utils::Base
{
public:
  typedef boost::shared_ptr<SepPresentation> Ptr;

private:
  typedef std::set<ViewInterface::WeakPtr> Views;

  std::string fileName;
  int height;
  int width;
  TiledBitmapInterface::Ptr tbi;
  int bps;
  int spp;
  std::map<std::string, std::string> properties;
  Views views;
  TransformationData::Ptr transformationData;
  ScroomInterface::Ptr scroomInterface;
  SepSource::Ptr sepSource;

private:
  SepPresentation(ScroomInterface::Ptr scroomInterface_);

  virtual std::string findPathToTiff(std::string sep_directory);
	virtual std::map<std::string, std::string> parseSep(const std::string &fileName);

public:
  virtual ~SepPresentation();

  static Ptr create(ScroomInterface::Ptr scroomInterface_);

  /**
   * Called when this presentation should go away.
   *
   * Note that this doesn't happen automatically, since the
   * TiledBitmapInterface has a reference to this presentation, via
   * the SourcePresentation.
   */
  void destroy();

  virtual bool load(const std::string& fileName);
  TransformationData::Ptr getTransformationData() const;

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