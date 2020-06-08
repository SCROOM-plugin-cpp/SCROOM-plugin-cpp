
#pragma once

#include <scroom/tiledbitmapinterface.hh>
#include <tiffio.h>

#include "sli/slilayer.hh"

struct SepFile {
	size_t width;
	size_t height;
	std::map<std::string, std::string> files;
};

/**
 * The motivation for having this class and not implementing SourcePresentation
 * directly in SepPresentation is to avoid a memory leak through cyclic
 * dependencies.
 */
class SepSource : public SourcePresentation
{
public:
	typedef boost::shared_ptr<SepSource> Ptr;

private:
	std::string file_name_c;
	std::string file_name_m;
	std::string file_name_y;
	std::string file_name_k;
	size_t image_width;

	struct tiff* file_c;
	struct tiff* file_m;
	struct tiff* file_y;
	struct tiff* file_k;

	SepSource();

public:
	~SepSource();
	static Ptr create();

	/**
	 * This function is only needed when the SepPresentation is used by
	 * the SliPresentation to parse and retrieve a layer of an SLI file.
	 * Upon being called, it fills the bitmap and all other relevant
	 * attributes of the SliLayer.
	 */
	static void fillSliLayer(SliLayer::Ptr sli);

	static std::string findPath(const std::string& sep_directory);
	static SepFile parseSep(const std::string& file_name);
	static bool checkFile(const SepFile content);

	/**
	 * Sets the tiff files to use as source data.
	 */
	void setData(std::string c_name, std::string m_name, std::string y_name, std::string k_name, size_t width);
	
	/**
	 * Opens the required TIFF files for the individual channels.
	 */
	void openFiles();

	/**
	 * Retrieves a scanline from all components combined.
	 * 
	 * @pre `openFiles` has been called.
	 */
	void readCombinedScanline(std::vector<byte>& out, size_t line_nr);

	////////////////////////////////////////////////////////////////////////
	// SourcePresentation
	////////////////////////////////////////////////////////////////////////
	void fillTiles(int startLine, int lineCount, int tileWidth, int firstTile, std::vector<Tile::Ptr>& tiles) override;

	/**
	 * Closes the TIFF files opened by `openFiles`.
	 * 
	 * @pre `openFiles` has been called.
	 */
	void done() override;
};
