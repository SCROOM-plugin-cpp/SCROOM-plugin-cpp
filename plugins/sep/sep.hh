#pragma once

#include <scroom/plugininformationinterface.hh>
#include <scroom/presentationinterface.hh>
#include <scroom/utilities.hh>

class Sep : public PluginInformationInterface, public OpenPresentationInterface, virtual public Scroom::Utils::Base
{
public:
	typedef boost::shared_ptr<Sep> Ptr;

private:
	Sep();
	ScroomPluginInterface::Ptr host;

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

	virtual std::list<GtkFileFilter *> getFilters();
	virtual PresentationInterface::Ptr open(const std::string &fileName);
	std::string findPathToTiff(std::string sep_directory);
	std::map<std::string, std::string> parseSep(const std::string &fileName);
	// void fillTiles(int startLine, int lineCount, int tileWidth,
	// 			   int firstTile, std::vector<Tile::Ptr> &tiles);
		////////////////////////////////////////////////////////////////////////

		virtual ~Sep();
};
