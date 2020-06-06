#pragma once

#include <map>
#include <string>

#include <scroom/presentationinterface.hh>

#include "sepsource.hh"
#include "sli/slilayer.hh"

class SepPresentation : public PresentationBase,
						public virtual Scroom::Utils::Base
{

	struct SepFile
	{
		size_t width, height;
		std::string C;
		std::string M;
		std::string Y;
		std::string K;
		std::string V;
		std::string W;
	};

public:
	typedef boost::shared_ptr<SepPresentation> Ptr;

private:
	typedef std::set<ViewInterface::WeakPtr> Views;

	std::string file_name;
	TiledBitmapInterface::Ptr tbi;
	SepSource::Ptr sep_source;

	size_t width;
	size_t height;

	Views views;
	ScroomInterface::Ptr scroomInterface;

	std::map<std::string, std::string> properties;

private:
	/**
	 * Constructor for a standalone SepPresentation to be passed to
	 * the Scroom core.
	 */
	SepPresentation(ScroomInterface::Ptr interface);

	/**
	 * Constructor for using the SepPresentation for only parsing a
	 * SEP file.
	 */
	SepPresentation();

	std::string findPath(std::string sep_directory);
	std::map<std::string, std::string> parseSep(const std::string &file_name);
	bool checkFile(const std::map<std::string, std::string> content);

public:
	virtual ~SepPresentation();

	/**
	 * Constructor to be called for a standalone SepPresentation to
	 * be passed to the Scroom core
	 */
	static Ptr create(ScroomInterface::Ptr interface);

	/**
	 * Constructor to be called when only wanting to use the
	 * SepPresentation for parsing a SEP file
	 */
	static Ptr create();

	/**
	 * Load the SEP file whose filename is passed as argument.
	 */
	bool load(const std::string &file_name);

	/** 
	 * This function is only needed when the SepPresentation is used by
	 * the SliPresentation to parse and retrieve a layer of an SLI file.
	 * Upon being called, it fills the bitmap and all other relevant
	 * attributes of the SliLayer.
	 * 
	 * TODO: provide implementation
	 */
	void fillSliLayer(SliLayer::Ptr) {}

	/**
	 * Is this function still needed? It has an empty implementation...
	 * 
	 * TODO: Investigate this.
	 */
	void destroy() {}

	////////////////////////////////////////////////////////////////////////
	// PresentationInterface
	////////////////////////////////////////////////////////////////////////

	Scroom::Utils::Rectangle<double> getRect() override;
	void redraw(ViewInterface::Ptr const &vi, cairo_t *cr, Scroom::Utils::Rectangle<double> presentationArea, int zoom) override;
	bool getProperty(const std::string &name, std::string &value) override;
	bool isPropertyDefined(const std::string &name) override;
	std::string getTitle() override;

	////////////////////////////////////////////////////////////////////////
	// PresentationBase
	////////////////////////////////////////////////////////////////////////

	void viewAdded(ViewInterface::WeakPtr interface) override;
	void viewRemoved(ViewInterface::WeakPtr interface) override;
	std::set<ViewInterface::WeakPtr> getViews() override;
};