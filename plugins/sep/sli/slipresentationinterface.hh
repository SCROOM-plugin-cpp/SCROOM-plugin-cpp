#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/dynamic_bitset.hpp>

#include "slilayer.hh"


class SliPresentationInterface
{
public:
  typedef boost::shared_ptr<SliPresentationInterface> Ptr;
  typedef boost::weak_ptr<SliPresentationInterface> WeakPtr;

  virtual ~SliPresentationInterface() {}

  virtual void wipeCache()=0;
  virtual void triggerRedraw()=0;
  virtual boost::dynamic_bitset<> getToggled()=0;
  virtual boost::dynamic_bitset<> getVisible()=0;
  virtual void setToggled(boost::dynamic_bitset<> bitmap)=0;
  virtual void setVisible(boost::dynamic_bitset<> bitmap)=0;
  virtual std::vector<SliLayer::Ptr>& getLayers()=0;

};