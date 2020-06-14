
#pragma once

#include <tiffio.h>

#include <scroom/tiledbitmapinterface.hh>
#include <scroom/transformpresentation.hh>

#include "sli/slilayer.hh"

struct SepFile {
    size_t width;
    size_t height;
    int white_ink_choice;
    std::map<std::string, std::string> files;
};

/**
 * The motivation for having this class and not implementing SourcePresentation
 * directly in SepPresentation is to avoid a memory leak through cyclic
 * dependencies.
 */
class SepSource : public SourcePresentation {
   public:
    typedef boost::shared_ptr<SepSource> Ptr;

   private:
    SepFile sep_file;
    const std::vector<std::string> channels{"C", "M", "Y", "K"};

    // stores pointers to channel files
    std::map<std::string, tiff *> channel_files = {
        {channels[0], nullptr},
        {channels[1], nullptr},
        {channels[2], nullptr},
        {channels[3], nullptr}};

    const size_t nr_channels = channel_files.size();

    tiff *white_ink = nullptr;
    tiff *varnish = nullptr;

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

    /**
	 * Hepler functions to parseSep().
	 */
    static std::string findPath(const std::string &sep_directory);
    static bool checkFile(const SepFile content);
    /**
	 * Parse the file into a struct.
	 */
    static SepFile parseSep(const std::string &file_name);

    /**
	 * Sets the tiff files to use as source data.
	 */
    void setData(SepFile sep_file_);

    /**
	 * Opens the required TIFF files for the individual channels.
	 */
    void openFiles();

    /**
	 * Applies the effect of white ink.
	 */
    static uint8_t applyWhiteInk(uint8_t white, uint8_t color, int type);

    /**
	 * Retrieves a scanline from all components combined.
	 * 
	 * @pre `openFiles` has been called.
	 */
    void readCombinedScanline(std::vector<byte> &out, size_t line_nr);

    // Resolution stuff
    TransformationData::Ptr getTransform();

    bool getResolution(uint16_t &unit, float &x_resolution, float &y_resolution);
    void getForOneChannel(struct tiff *channel, uint16_t &unit, float &x_resolution, float &y_resolution);

    /**
	 * Closes a tiff file and resets its pointer if the argument is not
	 * a nullptr.
	 */
    static void closeIfNeeded(struct tiff *&file);

    ////////////////////////////////////////////////////////////////////////
    // SourcePresentation
    ////////////////////////////////////////////////////////////////////////
    void fillTiles(int startLine, int lineCount, int tileWidth, int firstTile, std::vector<Tile::Ptr> &tiles) override;

    /**
	 * Closes the TIFF files opened by `openFiles()`.
	 */
    void done() override;
};
