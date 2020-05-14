#pragma once

#include <scroom/plugininformationinterface.hh>
#include <scroom/presentationinterface.hh>
#include <scroom/utilities.hh>


class Sep : public PluginInformationInterface, public OpenPresentationInterface, virtual public Scroom::Utils::Base{
public:
  typedef boost::shared_ptr<Sep> Ptr;

private:
  Sep();

public:
  static Ptr create();

public:
  ////////////////////////////////////////////////////////////////////////
  // PluginInformationInterface

  virtual std::string getPluginName();
  virtual std::string getPluginVersion();
  virtual void registerCapabilities(ScroomPluginInterface::Ptr host);

  ////////////////////////////////////////////////////////////////////////
  // OpenPresentationInterface

  virtual std::list<GtkFileFilter*> getFilters();
  virtual PresentationInterface::Ptr open(const std::string& fileName);

  ////////////////////////////////////////////////////////////////////////

  virtual ~Sep();
};
