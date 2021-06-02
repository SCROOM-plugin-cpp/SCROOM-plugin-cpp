//
// Created by jelle on 02/06/2021.
//

#include "CustomColorOperations.hh"

boost::shared_ptr<unsigned char> shared_malloc(size_t size)
{
    return boost::shared_ptr<unsigned char>(static_cast<unsigned char*>(malloc(size)), free);
}


Scroom::Utils::Stuff OperationsCustomColors::cache(const ConstTile::Ptr tile) {
    /* EXAMPLE FUNCTION (TAKEN FROM CMYK32 OPERATIONS)


    // Allocate the space for the cache - stride is the height of one row
    const int                  stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, tile->width);
    boost::shared_ptr<uint8_t> data   = shared_malloc(static_cast<size_t>(stride * tile->height));

    // Row is a pointer to a row of pixels (destination)
    auto* row = reinterpret_cast<uint32_t*>(data.get());
    // Cur is a pointer to the start of the row in the tile (source)
    const uint8_t* cur = tile->data.get();

    for(int i = 0; i < 4 * tile->height * tile->width; i += 4)
    {
        // Convert CMYK to ARGB, because cairo doesn't know how to render CMYK.
        auto C_i = static_cast<uint8_t>(255 - cur[i]);
        auto M_i = static_cast<uint8_t>(255 - cur[i + 1]);
        auto Y_i = static_cast<uint8_t>(255 - cur[i + 2]);
        auto K_i = static_cast<uint8_t>(255 - cur[i + 3]);

        uint32_t R = static_cast<uint8_t>((C_i * K_i) / 255);
        uint32_t G = static_cast<uint8_t>((M_i * K_i) / 255);
        uint32_t B = static_cast<uint8_t>((Y_i * K_i) / 255);

        // Write 255 as alpha (fully opaque)
        row[i / 4] = 255u << 24 | R << 16 | G << 8 | B;*/
}
