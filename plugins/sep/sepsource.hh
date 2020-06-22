
#pragma once

#include <tiffio.h>

#include <boost/filesystem.hpp>
#include <scroom/tiledbitmapinterface.hh>
#include <scroom/transformpresentation.hh>

#include "sli/slilayer.hh"
#include "varnish/varnish.hh"

struct SepFile {
    size_t width;
    size_t height;
    int white_ink_choice;
    std::map<std::string, boost::filesystem::path> files;
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
    /** Data structure to represent the opened SEP file in memory. */
    SepFile sep_file;

    const std::vector<std::string> channels = {"C", "M", "Y", "K"};

    /** Stores pointers to CMYK files. */
    std::map<std::string, tiff *> channel_files = {
        {channels[0], nullptr},
        {channels[1], nullptr},
        {channels[2], nullptr},
        {channels[3], nullptr}};

    const size_t nr_channels = channel_files.size();

    /** Pointer to white ink file. */
    tiff *white_ink = nullptr;

    /** Constructor */
    SepSource();

   public:
    /** Destructor */
    ~SepSource();

    /** Pointer to varnish layer. */
    Varnish::Ptr varnish;

    /**
     * Create a pointer to SepSource using constructor and return it.
     */
    static Ptr create();

    /**
     * This function is only needed when the SepPresentation is used by
     * the SliPresentation to parse and retrieve a layer of an SLI file.
     * Upon being called, it fills the bitmap and all other relevant
     * attributes of the SliLayer.
     * @param sli - pointer to SliLayer
     */
    static void fillSliLayer(SliLayer::Ptr sli);

    /**
     * Helper function to parseSep().
     * Given a file path, returns the parent directory.
     * @param file_path - path to file.
     */
    static boost::filesystem::path findParentDir(const std::string &file_path);

    /**
     * Parse the file into a struct.
     * @param file_name - path to file.
     */
    static SepFile parseSep(const std::string &file_name);

    /**
     * Setter for SepSource::sep_file.
     *
     * @param file - value to set.
     */
    void setData(SepFile file);

    /**
     * Opens the required TIFF files for the individual channels.
     * Sets value of channel_files, white_ink and varnish.
     */
    void openFiles();

    /**
     * Applies the effect of white ink.
     * @param white - value of white pixel
     * @param color - value of colored pixel
     * @param type - desired effect of white ink
     */
    static uint8_t applyWhiteInk(uint8_t white, uint8_t color, int type);

    /**
     * Wrapper around the TIFFReadScanLine() function from LibTiff.
     * Motivation for having this wrapper is because TIFFReadScanLine
     * acts expectedly/throws errors when supplied with a NULL. So,
     * this function adds a brief error handling case around it.
     */
    static int TIFFReadScanline_(tiff *file, void *buf, uint32 row, uint16 sample = 0);

    /**
     * Retrieves a scanline from all components combined.
     *
     * @pre `openFiles()` has been called.
     */
    void readCombinedScanline(std::vector<byte> &out, size_t line_nr);

    /**
     * Retrieves the transformation data for the loaded sep file, with
     * the correct aspect ratio set.
     *
     * @see TransformationData
     */
    TransformationData::Ptr getTransform();

    /**
     * Closes a tiff file and resets its pointer if the argument is not
     * a nullptr.
     * @param file - the file that is to be closed.
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

   private:
    /**
     * Fills the passed parameters with the correct resolution for this sep
     * file.
     *
     * Returns whether all channels have the same resolution and unit.
     */
    bool getResolution(uint16_t &unit, float &x_resolution, float &y_resolution);

    /**
     * Fills the `unit`, `x_resolution` and `y_resolution` with values from
     * the specified `channel`.
     *
     * If a resolution unit is specified in `channel`, the x and y resolution
     * are scaled so both values are between 0 and 1. If one of the values is
     * not specified in `channel`, the values are set to their defaults according
     * to the TIFF file format specification.
     */
    void getForOneChannel(struct tiff *channel, uint16_t &unit, float &x_resolution, float &y_resolution);
};
