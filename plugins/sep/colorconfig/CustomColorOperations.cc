//
// Created by jelle on 02/06/2021.
//

#include <utility>
#include "CustomColorHelpers.hh"
#include "CustomColorOperations.hh"

boost::shared_ptr<unsigned char> shared_malloc(size_t size)
{
    return boost::shared_ptr<unsigned char>(static_cast<unsigned char*>(malloc(size)), free);
}


Scroom::Utils::Stuff OperationsCustomColors::cache(const ConstTile::Ptr tile) {

    // Allocate the space for the cache - stride is the height of one row
    const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, tile->width);
    boost::shared_ptr<uint8_t> data = shared_malloc(static_cast<size_t>(stride * tile->height));

    // Row is a pointer to a row of pixels (destination)
    auto *row = reinterpret_cast<uint32_t *>(data.get());
    // Cur is a pointer to the start of the row in the tile (source)
    const uint8_t *cur = tile->data.get();

    for (int i = 0; i < spp * tile->height * tile->width; i += spp) {
        // Convert custom colors to CMKY and then to ARGB, because cairo doesn't know how to render CMYK.
        int32_t C = 0;
        int32_t M = 0;
        int32_t Y = 0;
        int32_t K = 0;
        for (int j = 0; j < spp; j++){
            auto color = colors.at(j);
            C += color->cMultiplier * static_cast<float>(cur[i + j]);
            M += color->mMultiplier * static_cast<float>(cur[i + j]);
            Y += color->yMultiplier * static_cast<float>(cur[i + j]);
            K += color->kMultiplier * static_cast<float>(cur[i + j]);
        }

        auto C_i = static_cast<uint8_t>(255 - CustomColorHelpers::toUint8(C));
        auto M_i = static_cast<uint8_t>(255 - CustomColorHelpers::toUint8(M));
        auto Y_i = static_cast<uint8_t>(255 - CustomColorHelpers::toUint8(Y));
        auto K_i = static_cast<uint8_t>(255 - CustomColorHelpers::toUint8(K));

        uint32_t R = static_cast<uint8_t>((C_i * K_i) / 255);
        uint32_t G = static_cast<uint8_t>((M_i * K_i) / 255);
        uint32_t B = static_cast<uint8_t>((Y_i * K_i) / 255);

        // Write 255 as alpha (fully opaque)
        row[i / 4] = 255u << 24 | R << 16 | G << 8 | B;
    }


}


void PipetteCommonOperationsCustomColor::setSpp(int samplesPerPixel) {
    spp = samplesPerPixel;
}

void PipetteCommonOperationsCustomColor::setColors(std::vector<CustomColor*> colors_) {
    colors = colors_;
}
