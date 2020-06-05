
#pragma once

#include <scroom/tiledbitmapinterface.hh>

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
	boost::shared_ptr<byte> image_data;
	size_t image_width;

	SepSource();

public:
	~SepSource();
	static Ptr create();

	void setData(boost::shared_ptr<byte> image_data, size_t width);

	////////////////////////////////////////////////////////////////////////
	// SourcePresentation
	////////////////////////////////////////////////////////////////////////
	void fillTiles(int startLine, int lineCount, int tileWidth, int firstTile, std::vector<Tile::Ptr>& tiles) override;
	void done() override;
};
