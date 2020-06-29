#include <boost/dynamic_bitset.hpp>
#include <gtk/gtk.h>

#include "slipresentationinterface.hh"

/** Enum encoding the indices of the columns in the GtkListStore */
enum { COL_VISIBILITY = 0, COL_ID, NUM_COLS };

/** Enum encoding the keys of each widget into the `widgets` and `oldValue` maps
 */
enum widget { TREEVIEW = 0, SLIDER_LOW, SLIDER_HIGH };

class SliControlPanel : public boost::enable_shared_from_this<SliControlPanel> {
public:
  typedef boost::shared_ptr<SliControlPanel> Ptr;

private:
  /** The number of layers that the SliPresentation consists of*/
  unsigned int n_layers;

  GtkWidget *hbox;

public:
  /** Contains the pointers to the widgets of the control panel*/
  std::map<widget, GtkWidget *> widgets;

  /** Contains the previous value of each slider */
  std::map<widget, double> oldValue;

  /** Indicates the last focused widget. Used to regain focus after the widgets
   * are disabled. */
  widget lastFocused = TREEVIEW;

  /** The SliPresentation that owns this SliControlPanel */
  SliPresentationInterface::WeakPtr presentation;

  /** The view to which the control panel is attached*/
  ViewInterface::WeakPtr viewWeak;

private:
  /** Constructor */
  SliControlPanel(ViewInterface::WeakPtr viewWeak_,
                  SliPresentationInterface::WeakPtr presentation_);

  /** Create a TreeView and link it to a ListStore model */
  virtual void create_view_and_model();

public:
  /** Constructor */
  static SliControlPanel::Ptr
  create(ViewInterface::WeakPtr viewWeak_,
         SliPresentationInterface::WeakPtr presentation_);

  /** Get the number of layers in the model */
  unsigned int getNumLayers() { return n_layers; };

  /** Disable the widgets while the cache is being computed. */
  virtual void disableInteractions();

  /** Re-enable all widgets */
  virtual void enableInteractions();

  /**
   * Remove the control panel from the current view and attach it to a new view
   * @param viewWeak_ the new view to attach the sidebar to
   */
  virtual void reAttach(ViewInterface::WeakPtr viewWeak_);

  /** Destructor */
  virtual ~SliControlPanel();
};