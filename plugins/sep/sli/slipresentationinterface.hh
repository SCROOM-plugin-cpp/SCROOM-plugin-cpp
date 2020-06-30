#include <boost/dynamic_bitset.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "slilayer.hh"

class SliPresentationInterface {
public:
  typedef boost::shared_ptr<SliPresentationInterface> Ptr;
  typedef boost::weak_ptr<SliPresentationInterface> WeakPtr;

  virtual ~SliPresentationInterface() {}

  /**
   * Clear (ie write 0s) the area of the bottom surface intersecting with the
   * toggled layers and trigger a redraw.
   */
  virtual void wipeCacheAndRedraw() = 0;

  /** Causes the SliPresentation to redraw the current presentation */
  virtual void triggerRedraw() = 0;

  /** Get a copy of the bitmap encoding the visibility of layers from SliSource
   */
  virtual boost::dynamic_bitset<> getVisible() = 0;

  /** Set the bits of the newly toggled bits in the toggled bitmap of SliSource
   */
  virtual void setToggled(boost::dynamic_bitset<> bitmap) = 0;

  /** Get a reference to the list of all layers in SliSource */
  virtual std::vector<SliLayer::Ptr> &getLayers() = 0;
};