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
#include "slicontrolpanel.hh"


class SliPresentation : public PresentationBase,
                        public virtual Scroom::Utils::Base,
                        public SliPresentationInterface, 
                        public boost::enable_shared_from_this<SliPresentation>
{
public:
  typedef boost::shared_ptr<SliPresentation> Ptr;
  typedef boost::weak_ptr<SliPresentation> WeakPtr;

private:
  typedef std::set<ViewInterface::WeakPtr> Views;
  std::map<std::string, std::string> properties;
  Views views;

  /** The SliLayers that are part of the presentation */
  std::vector<SliLayer::Ptr> layers;

  /** Reference to the ScroomInterface, needed for showing presentation */
  ScroomInterface::Ptr scroomInterface;

  /** Reference to the associated SliControlPanel */
  SliControlPanel::Ptr controlPanel;

  /** Weak pointer to this. Needed for passing a reference to SliControlPanel */
  static SliPresentationInterface::WeakPtr weakPtrToThis;

  /** Width of all layers combined */
  int total_width = 0;

  /** Height of all layers combined */
  int total_height = 0;

  int bpp;
  int Xresolution;
  int Yresolution;

private:
  SliPresentation(ScroomInterface::Ptr scroomInterface);

public:
  virtual ~SliPresentation();

  //virtual void dummyfunc();

  static Ptr create(ScroomInterface::Ptr scroomInterface);

  virtual bool load(const std::string& fileName);

  virtual void parseSli(const std::string &fileName);
  // TODO look more into whether this is safe to do
  std::vector<SliLayer::Ptr>& getLayers() {return layers;};

  ////////////////////////////////////////////////////////////////////////
  // SliPresentationInterface
  ////////////////////////////////////////////////////////////////////////

  virtual void toggleLayer(int index);

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
