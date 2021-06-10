//
// Created by jelle on 02/06/2021.
//

#include <utility>
#include <scroom/bitmap-helpers.hh>
#include "CustomColorHelpers.hh"
#include "CustomColorOperations.hh"

boost::shared_ptr<unsigned char> shared_malloc(size_t size)
{
    return boost::shared_ptr<unsigned char>(static_cast<unsigned char*>(malloc(size)), free);
}

PipetteLayerOperations::PipetteColor PipetteCommonOperationsCustomColor::sumPixelValues(Scroom::Utils::Rectangle<int> area,
                                                                                 const ConstTile::Ptr          tile)
{
    int                                           offset = spp * (area.getTop() * tile->width + area.getLeft());
    int                                           stride = spp * (tile->width - area.getWidth());
    Scroom::Bitmap::SampleIterator<const uint8_t> si(tile->data.get(), 0, bps);
    si += offset;

    std::map<std::string, size_t> sums = {};

    // Initialize sums of all colors to 0
    for (auto color : colors){
        sums[color->getName()] = 0;
    }

    for(int y = area.getTop(); y < area.getBottom(); y++)
    {
        for(int x = area.getLeft(); x < area.getRight(); x++)
        {
            for (auto color: colors){
                sums[color->getName()] += *si++;
            }
        }
        si += stride;
    }

    // Copy map to vector of pairs
    PipetteColor result(sums.size());
    copy(sums.begin(), sums.end(), result.begin());

    return result;
}




OperationsCustomColors::OperationsCustomColors() : PipetteCommonOperationsCustomColor(8){}

PipetteCommonOperationsCustomColor::Ptr OperationsCustomColors::create() { return PipetteCommonOperationsCustomColor::Ptr(new OperationsCustomColors()); }

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

    return Scroom::Bitmap::BitmapSurface::create(tile->width, tile->height, CAIRO_FORMAT_ARGB32, stride, data);

}

void OperationsCustomColors::reduce(Tile::Ptr target, const ConstTile::Ptr source, int top_left_x, int top_left_y) {
    // Reducing by a factor 8
    int         sourceStride = getBpp() * source->width / 8; // stride in bytes
    const byte* sourceBase   = source->data.get();

    int   targetStride = getBpp() * target->width / 8; // stride in bytes
    byte* targetBase   = target->data.get() + (target->height * top_left_y + top_left_x) * targetStride / 8;

    for(int y = 0; y < source->height / 8; y++)
    {
        byte* targetPtr = targetBase;

        for(int x = 0; x < source->width / 8; x++)
        {
            // We want to store the average colour of the 8*8 pixel image
            // with (x, y) as its top-left corner into targetPtr.
            const byte* base = sourceBase + 8 * spp * x;  // start of the row
            const byte* end  = base + 8 * sourceStride; // end of the row

            std::map<std::string, size_t> sums = {};

            // Initialize sums of all colors to 0
            for (auto color : colors){
                sums[color->getName()] = 0;
            }


            for(const byte* row = base; row < end; row += sourceStride) // Iterate over rows
            {
                for(size_t current = 0; current < 8 * spp; current += spp) // Iterate over pixels
                {
                    for (int i = 0; i < spp; i++){ // Iterate over samples in pixel
                        sums[colors.at(i)->getName()] += row[current+i];
                    }
                }
            }

            // Calculate and store the average in the target
            for (int i = 0; i < spp; i++) {
                targetPtr[i] = static_cast<byte>(sums[colors.at(i)->getName()] / 64);
            }


            targetPtr += spp;
        }

        targetBase += targetStride;
        sourceBase += sourceStride * 8;
    }
}

int OperationsCustomColors::getBpp() {
    return spp * bps;
}




void PipetteCommonOperationsCustomColor::setSpp(int samplesPerPixel) {
    spp = samplesPerPixel;
}

void PipetteCommonOperationsCustomColor::setColors(std::vector<CustomColor*> colors_) {
    colors = colors_;
}
