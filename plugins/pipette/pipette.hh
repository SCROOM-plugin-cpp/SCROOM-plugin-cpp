#pragma once

#include <boost/shared_ptr.hpp>

#include <scroom/plugininformationinterface.hh>
#include <scroom/presentationinterface.hh>
#include <scroom/utilities.hh>

class Pipette : public PluginInformationInterface, virtual public  Scroom::Utils::Base
{
public:
  typedef boost::shared_ptr<Pipette> Ptr;

private:
  Pipette();

public:
  static Ptr create();

public:
  virtual std::string getPluginName();
  virtual std::string getPluginVersion();
  virtual void registerCapabilities(ScroomPluginInterface::Ptr host);

  virtual ~Pipette();
};

