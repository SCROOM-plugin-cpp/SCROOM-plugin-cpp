#include "sepsource.hh"

SepSource::SepSource() {}
SepSource::~SepSource() {}

SepSource::Ptr SepSource::create()
{
	return Ptr(new SepSource());
}

void SepSource::setData(boost::shared_ptr<byte> data, size_t width)
{
	this->image_data = data;
	this->image_width = width;
}

void SepSource::fillTiles(int startLine, int line_count, int tileWidth, int firstTile, std::vector<Tile::Ptr> &tiles)
{
	const size_t bpp = 4; // bytes per pixel
	const size_t start_line = static_cast<size_t>(startLine);
	const size_t first_tile = static_cast<size_t>(firstTile);
	const size_t tile_stride = static_cast<size_t>(tileWidth * bpp);

	const size_t tile_count = tiles.size();
	auto tile_data = std::vector<byte*>(tile_count);
	for (size_t tile = 0; tile < tile_count; tile++) {
		tile_data[tile] = tiles[tile]->data.get();
	}

	for (size_t i = 0; i < static_cast<size_t>(line_count); i++) {
		// offset to the beginning of this line.
		const size_t offset = bpp * (i + start_line) * this->image_width;

		for (size_t tile = 0; tile < tile_count - 1; tile++)
		{
			memcpy(
				tile_data[tile],
				this->image_data.get() + offset + (first_tile + tile) * tile_stride,
				tile_stride
			);
			tile_data[tile] += tile_stride;
		}

		// This memcpy call seems to give problems for some files and cause a segfault
		memcpy(
			tile_data[tile_count - 1],
			this->image_data.get() + offset + (first_tile + tile_count - 1) * tile_stride,
			tile_stride - (first_tile + tile_count - 1) * tile_stride
		);

		tile_data[tile_count - 1] += tile_stride;
	}
}

void SepSource::done()
{
	this->image_data.reset();
}
