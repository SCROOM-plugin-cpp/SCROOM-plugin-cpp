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

#include "slilayer.hh"


class SliPresentation : public PresentationBase,
                        public virtual Scroom::Utils::Base,
                        public boost::enable_shared_from_this<SliPresentation>
{
public:
  typedef boost::shared_ptr<SliPresentation> Ptr;

private:
  typedef std::set<ViewInterface::WeakPtr> Views;
  std::map<std::string, std::string> properties;
  Views views;
  std::vector<SliLayer::Ptr> layers;
  ScroomInterface::Ptr scroomInterface;
  int Xresolution;
  int Yresolution;
  int bpp;

private:
  SliPresentation(ScroomInterface::Ptr scroomInterface);

public:
  virtual ~SliPresentation();

  static Ptr create(ScroomInterface::Ptr scroomInterface);

  virtual bool load(const std::string& fileName);

  virtual void parseSli(const std::string &fileName);
  // TODO look more into whether this is safe to do
  std::vector<SliLayer::Ptr>& getLayers() {return layers;};

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
