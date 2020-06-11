#include <gtk/gtk.h>

#include "slipresentationinterface.hh"

class SliControlPanel: public boost::enable_shared_from_this<SliControlPanel>
{
public:
  typedef boost::shared_ptr<SliControlPanel> Ptr;

private:

  /** The number of layers that the SliPresentation consists of*/
  int n_layers;

  std::vector<GtkWidget*> widgets;

public:
  GtkWidget* treeview;
  GtkRange* range_high;
  GtkRange* range_low;

  /** The SliPresentation that owns this SliControlPanel */
  SliPresentationInterface::WeakPtr presentation;

  boost::mutex* cacheMtx;

private:
  /** Constructor */
  SliControlPanel(ViewInterface::WeakPtr viewWeak, SliPresentationInterface::WeakPtr presentation_, boost::mutex* cacheMtx_);

  virtual void create_view_and_model();

public:
  /** Constructor */
  static SliControlPanel::Ptr create(ViewInterface::WeakPtr view, SliPresentationInterface::WeakPtr presentation_, boost::mutex* cacheMtx_);

  virtual void disableInteractions();

  virtual void enableInteractions();

  /** Destructor */
  virtual ~SliControlPanel();

};