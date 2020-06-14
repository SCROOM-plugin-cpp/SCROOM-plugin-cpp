#include <gtk/gtk.h>
#include <boost/dynamic_bitset.hpp>

#include "slipresentationinterface.hh"

enum
{
  COL_VISIBILITY = 0,
  COL_ID,
  NUM_COLS
};

enum widget
{
  TREEVIEW = 0,
  SLIDER_LOW,
  SLIDER_HIGH
};

class SliControlPanel: public boost::enable_shared_from_this<SliControlPanel>
{
public:
  typedef boost::shared_ptr<SliControlPanel> Ptr;

private:
  /** The number of layers that the SliPresentation consists of*/
  int n_layers;

public:
  std::map<widget, GtkWidget*> widgets;
  std::map<widget, double> oldValue;
  widget lastFocused = TREEVIEW;

  /** The SliPresentation that owns this SliControlPanel */
  SliPresentationInterface::WeakPtr presentation;

private:
  /** Constructor */
  SliControlPanel(ViewInterface::WeakPtr viewWeak, SliPresentationInterface::WeakPtr presentation_);

  virtual void create_view_and_model();

public:
  /** Constructor */
  static SliControlPanel::Ptr create(ViewInterface::WeakPtr view, SliPresentationInterface::WeakPtr presentation_);

  virtual void disableInteractions();

  virtual void enableInteractions();

  /** Destructor */
  virtual ~SliControlPanel();

};