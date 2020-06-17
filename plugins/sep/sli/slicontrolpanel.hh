#include <gtk/gtk.h>
#include <boost/dynamic_bitset.hpp>

#include "slipresentationinterface.hh"

/** Enum encoding the indices of the columns in the GtkListStore */
enum
{
  COL_VISIBILITY = 0,
  COL_ID,
  NUM_COLS
};

/** Enum encoding the keys of each widget into the `widgets` and `oldValue` maps */
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
  /** Contains the pointers to the widgets of the control panel*/
  std::map<widget, GtkWidget*> widgets;

  /** Contains the previous value of each slider */
  std::map<widget, double> oldValue;

  /** Indicates the last focused widget. Used to regain focus after the widgets are disabled. */
  widget lastFocused = TREEVIEW;

  /** The SliPresentation that owns this SliControlPanel */
  SliPresentationInterface::WeakPtr presentation;

private:
  /** Constructor */
  SliControlPanel(ViewInterface::WeakPtr viewWeak, SliPresentationInterface::WeakPtr presentation_);

  /** Create a TreeView and link it to a ListStore model */
  virtual void create_view_and_model();

public:
  /** Constructor */
  static SliControlPanel::Ptr create(ViewInterface::WeakPtr view, SliPresentationInterface::WeakPtr presentation_);
  
  /** Disable the widgets while the cache is being computed. */
  virtual void disableInteractions();

   /** Re-enable all widgets */
  virtual void enableInteractions();

  /** Destructor */
  virtual ~SliControlPanel();

};