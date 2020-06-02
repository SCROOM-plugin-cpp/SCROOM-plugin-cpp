#include <gtk/gtk.h>

#include <scroom/unused.hh>

#include "slipresentationinterface.hh"

class SliControlPanel: public boost::enable_shared_from_this<SliControlPanel>
{
public:
  typedef boost::shared_ptr<SliControlPanel> Ptr;

private:
  SliPresentationInterface::WeakPtr presentation;

private:
  /** Constructor */
  SliControlPanel(ViewInterface::WeakPtr viewWeak);

public:
  /** Constructor */
  static SliControlPanel::Ptr create(ViewInterface::WeakPtr view, SliPresentationInterface::WeakPtr presentation_);

  /** Destructor */
  ~SliControlPanel();

};
